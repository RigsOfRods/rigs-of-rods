
/** 
    @file
    @date   01/2016
    @author Petr Ohlidal
*/

#include "Powertrain.h"

#include "Beam.h"
#include "BeamEngine.h"
#include "Settings.h"
#include "SoundScriptManager.h"

using namespace RoR;

Powertrain::Powertrain():
    m_engine(nullptr),
    m_main_thread_index(0),
    m_sim_thread_index(1)
{
    m_state[0].Reset();
    m_state[1].Reset();

    m_command_queue[0].queue.reserve(20);
    m_command_queue[1].queue.reserve(20);
}

Powertrain::~Powertrain()
{
    delete m_engine;
}

void Powertrain::PowertrainExportState()
{
    PowertrainState state;
    state.Reset();
    
    state.conf_engine_has_turbo              = m_engine->hasTurbo();
    state.conf_engine_max_rpm                = m_engine->getMaxRPM();
    state.conf_transmission_clutch_force     = m_engine->getClutchForce();
    state.current_acceleration               = m_engine->getAcc();
    state.engine_crank_factor                = m_engine->getCrankFactor();
    state.engine_current_rpm                 = m_engine->getRPM();
    state.engine_current_torque              = m_engine->getTorque();
    state.engine_is_running                  = m_engine->isRunning();
    state.engine_starter_has_contact         = m_engine->hasContact();
    state.gfx_engine_exhaust_smoke           = m_engine->getSmoke();
    state.transmission_auto_shift_mode       = m_engine->getAutoShift();
    state.transmission_current_clutch        = m_engine->getClutch();
    state.transmission_current_gear          = m_engine->getGear();
    state.transmission_num_forward_gears     = m_engine->getNumGears();
    state.turbo_current_psi                  = m_engine->getTurboPSI();
    state.conf_engine_min_rpm                = m_engine->getMinRPM();
    state.transmission_current_gear_range    = m_engine->getGearRange();
    state.conf_transmission_num_gears_ranges = m_engine->getNumGearsRanges();
    state.transmission_mode                  = m_engine->getAutoMode();

    this->GetStateOnSimThread() = state;
}

void Powertrain::PowertrainUpdate(float dt_seconds, bool just_synced)
{
    if (just_synced)
    {
        if (m_vehicle->cc_mode)
        {
            this->UpdateCruiseControl(dt_seconds);
        }
        this->CheckSpeedLimit(dt_seconds);
    }
    m_engine->UpdateBeamEngine(dt_seconds, just_synced);
}

void Powertrain::CheckSpeedLimit(float dt_seconds)
{
    if (m_vehicle->sl_enabled && m_engine->getGear() != 0)
    {
        float accl = (m_vehicle->sl_speed_limit - std::abs(m_vehicle->WheelSpeed)) * 2.0f;
        accl = std::max(0.0f, accl);
        accl = std::min(accl, m_engine->getAcc());

        m_engine->setAcc(accl);
    }
}

void Powertrain::PowertrainProcessInputStates()
{
    // TODO: Cache this!
    bool use_arcade_controls = BSETTING("ArcadeControls", false);

    BeamEngine* engine               = this->GetEngine();
    PowertrainInputStates inputs     = this->GetQueueOnSimThread().input_states;

    PowertrainState pw_out_state;
    pw_out_state.Reset();

    RoR::Gearbox::shiftmodes transmission_mode = m_engine->getAutoMode();

    if (!use_arcade_controls && engine->getAutoShift() > Gearbox::SHIFTMODE_SEMIAUTO)
    {
        // Classic mode, realistic
        engine->autoSetAcc(inputs.acceleration);
        m_vehicle->brake = inputs.brake * m_vehicle->brakeforce;
    }
    else // arcade controls: hey - people wanted it x| ... <- and it's convenient
    {
        // start engine
        if (engine->hasContact() && !engine->isRunning() && (inputs.acceleration > 0 || inputs.brake > 0))
        {
            engine->BeamEngineStart();
        }

        if (engine->getGear() >= 0)
        {
            // neutral or drive forward, everything is as its used to be: brake is brake and accel. is accel.
            engine->autoSetAcc(inputs.acceleration);
            m_vehicle->brake = inputs.brake * m_vehicle->brakeforce;
        }
        else
        {
            // reverse gear, reverse controls: brake is accel. and accel. is brake.
            engine->autoSetAcc(inputs.brake);
            m_vehicle->brake = inputs.acceleration * m_vehicle->brakeforce;
        }

        // only when the truck really is not moving anymore
        if (fabs(m_vehicle->WheelSpeed) <= 1.f)
        {
            float velocity = 0;
            if (m_vehicle->cameranodepos[0] >= 0 && m_vehicle->cameranodedir[0] >= 0)
            {
                Ogre::Vector3 hdir = (m_vehicle->nodes[m_vehicle->cameranodepos[0]].RelPosition - m_vehicle->nodes[m_vehicle->cameranodedir[0]].RelPosition).normalisedCopy();
                velocity = hdir.dotProduct(m_vehicle->nodes[0].Velocity);
            }
            // switching point, does the user want to drive forward from backward or the other way round? change gears?
            if (velocity < 1.0f && inputs.brake > 0.5f && inputs.acceleration < 0.5f && engine->getGear() > 0)
            {
                // we are on the brake, jump to reverse gear
                if (engine->getAutoShift() == Gearbox::SHIFTMODE_AUTOMATIC)
                {
                    engine->autoShiftSet(Gearbox::AUTOSWITCH_REAR);
                } 
                else
                {
                    engine->setGear(-1.f);
                }
            } 
            else if (velocity > -1.0f && inputs.brake < 0.5f && inputs.acceleration > 0.5f && engine->getGear() < 0)
            {
                // we are on the gas pedal, jump to first gear when we were in rear gear
                if (engine->getAutoShift() == Gearbox::SHIFTMODE_AUTOMATIC)
                {
                    engine->autoShiftSet(Gearbox::AUTOSWITCH_DRIVE);
                } 
                else
                {
                    engine->setGear(1.f);
                }
            }
        }
    } // end of arcade controls

    if (engine->getAutoShift() == Gearbox::SHIFTMODE_AUTOMATIC)
    {
        if (inputs.autoshift_up)
        {
            engine->autoShiftUp();
        }
        if (inputs.autoshift_down)
        {
            engine->autoShiftDown();
        }
    }

    if (inputs.toggle_contact)
    {
        engine->toggleContact();
    }

    if (inputs.starter && engine->hasContact())
    {
        // starter
        engine->setstarter(1);
#ifdef USE_OPENAL
        SoundScriptManager::getSingleton().trigStart(m_vehicle, SS_TRIG_STARTER);
#endif // OPENAL
    } 
    else
    {
        engine->setstarter(0);
#ifdef USE_OPENAL
        SoundScriptManager::getSingleton().trigStop(m_vehicle, SS_TRIG_STARTER);
#endif // OPENAL
    }

    if (inputs.switch_shift_modes)
    {
        // toggle Auto shift
        engine->toggleAutoMode();
    }

    // joy clutch
    engine->setClutch(inputs.manual_clutch);

    bool gear_changed_rel = false;
    int shiftmode = engine->getAutoShift();

    if (shiftmode <= Gearbox::SHIFTMODE_MANUAL) // auto, semi auto and sequential shifting
    {
        if (inputs.shift_up)
        {
            engine->BeamEngineShift(1); // shift up
            gear_changed_rel = true;
        } 
        else if (inputs.shift_down)
        {
            if (shiftmode  > Gearbox::SHIFTMODE_SEMIAUTO ||
            shiftmode == Gearbox::SHIFTMODE_SEMIAUTO  && !use_arcade_controls ||
            shiftmode == Gearbox::SHIFTMODE_SEMIAUTO  && engine->getGear() > 0 ||
            shiftmode == Gearbox::SHIFTMODE_AUTOMATIC)
            {
                engine->BeamEngineShift(-1); // shift down
                gear_changed_rel = true;
            }
        } 
        else if (shiftmode != Gearbox::SHIFTMODE_AUTOMATIC && inputs.shift_neutral)
        {
            engine->BeamEngineShiftTo(0);
        }
    } 
    else //if (shiftmode > BeamEngine::MANUAL) // h-shift or h-shift with ranges shifting
    {
        bool gear_changed = false;
        bool found        = false;
        int  curgear      = engine->getGear();
        int  curgearrange = engine->getGearRange();
        int  gearoffset   = std::max(0, curgear - curgearrange * 6);

        // one can select range only if in neutral
        if (shiftmode==Gearbox::SHIFTMODE_MANUAL_RANGES && curgear == 0)
        {
            if (inputs.shift_lowrange && curgearrange != 0)
            {
                engine->setGearRange(0);
                pw_out_state.transmission_gear_range_selected = 1;
            } 
            else if (inputs.shift_midrange && curgearrange != 1 && engine->getNumGearsRanges()>1)
            {
                engine->setGearRange(1);
                pw_out_state.transmission_gear_range_selected = 2;
            } 
            else if (inputs.shift_highrange && curgearrange != 2 && engine->getNumGearsRanges()>2)
            {
                engine->setGearRange(2);
                pw_out_state.transmission_gear_range_selected = 3;
            }
        }
        //zaxxon
        if (curgear == -1)
        {
            gear_changed = !inputs.shift_gear_reverse;
        } 
        else if (curgear > 0 && curgear < 19)
        {
            if (shiftmode==Gearbox::SHIFTMODE_MANUAL)
            {
                gear_changed = !inputs.direct_shift[curgear - 1];
            } 
            else
            {
                gear_changed = !inputs.direct_shift[gearoffset - 1];
            }
        }

        if (gear_changed || curgear == 0)
        {
            if (inputs.shift_gear_reverse)
            {
                engine->BeamEngineShiftTo(-1);
                found = true;
            } 
            else if (inputs.shift_neutral)
            {
                engine->BeamEngineShiftTo(0);
                found = true;
            } 
            else
            {
                if (shiftmode == Gearbox::SHIFTMODE_MANUAL_STICK)
                {
                    for (int i=0; i < 18 && !found; i++)
                    {
                        if (inputs.direct_shift[i])
                        {
                            engine->BeamEngineShiftTo(i + 1);
                            found = true;
                        }
                    }
                } 
                else // BeamEngine::MANUALMANUAL_RANGES
                {
                    for (int i=1; i < 7 && !found; i++)
                    {
                        if (inputs.direct_shift[i - 1])
                        {
                            int val = i + curgearrange * 6;
                            engine->BeamEngineShiftTo(val);
                            found = true;
                        }
                    }
                }
            }
            if (!found)
            {
                engine->BeamEngineShiftTo(0);
            }
        } // end of if (gear_changed)
    } // end of shitmode > BeamEngine::MANUAL

    // anti roll back in BeamEngine::AUTOMATIC (DRIVE, TWO, ONE) mode
    int autoshift = engine->getAutoShift();
    if  (transmission_mode  == Gearbox::SHIFTMODE_AUTOMATIC &&
        (autoshift == Gearbox::AUTOSWITCH_DRIVE ||
        autoshift  == Gearbox::AUTOSWITCH_TWO ||
        autoshift  == Gearbox::AUTOSWITCH_ONE) &&
        m_vehicle->WheelSpeed < +0.1f)
    {
        Ogre::Vector3 dirDiff = (m_vehicle->nodes[m_vehicle->cameranodepos[0]].RelPosition - m_vehicle->nodes[m_vehicle->cameranodedir[0]].RelPosition).normalisedCopy();
        Ogre::Degree pitchAngle = Ogre::Radian(asin(dirDiff.dotProduct(Ogre::Vector3::UNIT_Y)));

        if (pitchAngle.valueDegrees() > +1.0f)
        {
            if (sin(pitchAngle.valueRadians()) * m_vehicle->getTotalMass() > engine->getTorque() / 2.0f)
            {
                m_vehicle->brake = m_vehicle->brakeforce;
            } 
            else
            {
                m_vehicle->brake = m_vehicle->brakeforce * (1.0f - engine->getAcc());
            }
        }
    }

    // anti roll forth in BeamEngine::AUTOMATIC (REAR) mode
    if (transmission_mode  == Gearbox::SHIFTMODE_AUTOMATIC &&
        engine->getAutoShift()  == Gearbox::AUTOSWITCH_REAR &&
        m_vehicle->WheelSpeed > -0.1f)
    {
        Ogre::Vector3 dirDiff = (m_vehicle->nodes[m_vehicle->cameranodepos[0]].RelPosition - m_vehicle->nodes[m_vehicle->cameranodedir[0]].RelPosition).normalisedCopy();
        Ogre::Degree pitchAngle = Ogre::Radian(asin(dirDiff.dotProduct(Ogre::Vector3::UNIT_Y)));

        if (pitchAngle.valueDegrees() < -1.0f)
        {
            if (sin(pitchAngle.valueRadians()) * m_vehicle->getTotalMass() < engine->getTorque() / 2.0f)
            {
                m_vehicle->brake = m_vehicle->brakeforce;
            } 
            else
            {
                m_vehicle->brake = m_vehicle->brakeforce * (1.0f - inputs.acceleration);
            }
        }
    }

#ifdef USE_OPENAL
    if (m_vehicle->brake > m_vehicle->brakeforce / 6.0f)
    {
        SoundScriptManager::getSingleton().trigStart(m_vehicle, SS_TRIG_BRAKE);
    } 
    else
    {
        SoundScriptManager::getSingleton().trigStop(m_vehicle, SS_TRIG_BRAKE);
    }
#endif // USE_OPENAL

    if (inputs.toggle_axle_lock)
    {
        // toggle auto shift
        if (m_vehicle->getAxleLockCount())
        {
            m_vehicle->toggleAxleLock();
            pw_out_state.transmission_axlelock_toggled = 1;
        }
        else
        {
            pw_out_state.transmission_axlelock_toggled = -1;
        }
    }

    if (inputs.parking_brake_toggle)
    {
        m_vehicle->parkingbrakeToggle();
    }

    if (inputs.antilock_brake_toggle && m_vehicle->alb_present && !m_vehicle->alb_notoggle)
    {
        m_vehicle->antilockbrakeToggle();
    }

    if (inputs.traction_control_toggle && m_vehicle->tc_present && !m_vehicle->tc_notoggle)
    {
        m_vehicle->tractioncontrolToggle();
    }

    if (inputs.cruise_control_toggle)
    {
        m_vehicle->cruisecontrolToggle();
    }
}

void Powertrain::UpdateCruiseControl(float dt)
{
    PowertrainInputStates inputs = this->GetQueueOnSimThread().input_states;

    if (inputs.brake > 0.05f || inputs.manual_clutch > 0.05f ||
        (m_vehicle->cc_target_speed < m_vehicle->cc_target_speed_lower_limit) ||
        (m_vehicle->parkingbrake && m_engine->getGear() > 0) ||
        !m_engine->isRunning() ||
        !m_engine->hasContact())
    {
        m_vehicle->cruisecontrolToggle();
        return;
    }

    if (m_engine->getGear() > 0)
    {
        // Try to maintain the target speed
        if (m_vehicle->cc_target_speed > m_vehicle->WheelSpeed)
        {
            float accl = (m_vehicle->cc_target_speed - m_vehicle->WheelSpeed) * 2.0f;
            accl = std::max(m_engine->getAcc(), accl);
            accl = std::min(accl, 1.0f);
            m_engine->setAcc(accl);
        }
    }
    else if (m_engine->getGear() == 0)
    {
        // Try to maintain the target rpm
        if (m_vehicle->cc_target_rpm > m_engine->getRPM())
        {
            float accl = (m_vehicle->cc_target_rpm - m_engine->getRPM()) * 0.01f;
            accl = std::max(m_engine->getAcc(), accl);
            accl = std::min(accl, 1.0f);
            m_engine->setAcc(accl);
        }
    }

    if (inputs.cruise_control_accel)
    {
        if (m_engine->getGear() > 0)
        {
            m_vehicle->cc_target_speed += 2.5f * dt;
            m_vehicle->cc_target_speed  = std::max(m_vehicle->cc_target_speed_lower_limit, m_vehicle->cc_target_speed);
            if (m_vehicle->sl_enabled)
            {
                m_vehicle->cc_target_speed  = std::min(m_vehicle->cc_target_speed, m_vehicle->sl_speed_limit);
            }
        }
        else if (m_engine->getGear() == 0)
        {
            m_vehicle->cc_target_rpm += 1000.0f * dt;
            m_vehicle->cc_target_rpm  = std::min(m_vehicle->cc_target_rpm, m_engine->getMaxRPM());
        }
    }

    if (inputs.cruise_control_decel)
    {
        if (m_engine->getGear() > 0)
        {
            m_vehicle->cc_target_speed -= 2.5f * dt;
            m_vehicle->cc_target_speed  = std::max(m_vehicle->cc_target_speed_lower_limit, m_vehicle->cc_target_speed);
        }
        else if (m_engine->getGear() == 0)
        {
            m_vehicle->cc_target_rpm -= 1000.0f * dt;
            m_vehicle->cc_target_rpm  = std::max(m_engine->getMaxRPM(), m_vehicle->cc_target_rpm);
        }
    }

	if (inputs.cruise_control_readjust)
	{
		m_vehicle->cc_target_speed = std::max(m_vehicle->WheelSpeed, m_vehicle->cc_target_speed);
		if (m_vehicle->sl_enabled)
		{
			m_vehicle->cc_target_speed = std::min(m_vehicle->cc_target_speed, m_vehicle->sl_speed_limit);
		}
		m_vehicle->cc_target_rpm   = m_engine->getRPM();
	}

	if (m_vehicle->cc_can_brake)
	{
		if (m_vehicle->WheelSpeed > m_vehicle->cc_target_speed + 0.5f && !inputs.acceleration)
		{
			float brake = (m_vehicle->WheelSpeed - m_vehicle->cc_target_speed) * 0.5f;
			brake = std::min(brake, 1.0f);
			m_vehicle->brake = m_vehicle->brakeforce * brake;
		}
	}
}

void Powertrain::PowertrainProcessInput()
{
    this->PowertrainProcessInputStates();

    // Process command queue
    auto& cmd_queue = this->GetQueueOnSimThread();
    auto itor = cmd_queue.queue.begin();
    auto endi = cmd_queue.queue.end();
    for (; itor != endi; ++itor)
    {
        auto cmd = *itor;
        switch (cmd.type)
        {
        case PowertrainCommand::COMMAND_AUTO_SET_ACC:
            m_engine->autoSetAcc(cmd.value);
            break;
        case PowertrainCommand::COMMAND_OFFSTART:
            m_engine->offstart();
            break;
        case PowertrainCommand::COMMAND_SET_ACC:
            m_engine->setAcc(cmd.value);
            break;
        case PowertrainCommand::COMMAND_SET_CLUTCH:
            m_engine->setClutch(cmd.value);
            break;
        case PowertrainCommand::COMMAND_SHIFT:
            m_engine->BeamEngineShift(static_cast<int>(cmd.value));
            break;
        case PowertrainCommand::COMMAND_START:
            m_engine->BeamEngineStart();
            break;
        case PowertrainCommand::COMMAND_AUTO_SHIFT_SET:
            m_engine->autoShiftSet(static_cast<int>(cmd.value));
            break;
        case PowertrainCommand::COMMAND_SET_GEAR:
            m_engine->setGear(static_cast<int>(cmd.value));
            break;
        case PowertrainCommand::COMMAND_AUTO_SHIFT_UP:
            m_engine->autoShiftUp();
            break;
        case PowertrainCommand::COMMAND_AUTO_SHIFT_DOWN:
            m_engine->autoShiftDown();
            break;
        case PowertrainCommand::COMMAND_TOGGLE_CONTACT:
            m_engine->toggleContact();
            break;
        case PowertrainCommand::COMMAND_SET_STARTER:
            m_engine->setstarter(static_cast<int>(cmd.value));
            break;
        case PowertrainCommand::COMMAND_TOGGLE_AUTO_MODE:
            m_engine->toggleAutoMode();
            break;
        case PowertrainCommand::COMMAND_SET_MANUAL_CLUTCH:
            m_engine->setManualClutch(cmd.value);
            break;
        case PowertrainCommand::COMMAND_SHIFT_TO:
            m_engine->BeamEngineShiftTo(static_cast<int>(cmd.value));
            break;
        case PowertrainCommand::COMMAND_SET_GEAR_RANGE:
            m_engine->setGearRange(static_cast<int>(cmd.value));
            break;
        }
    }

    // Process networked update
    if (cmd_queue.was_network_updated)
    {
        m_engine->netForceSettings(
            cmd_queue.net_rpm,
            cmd_queue.net_force,
            cmd_queue.net_clutch,
            cmd_queue.net_gear,
            cmd_queue.net_is_running,
            cmd_queue.net_has_contact,
            cmd_queue.net_auto_mode
            );
    }
    cmd_queue.Reset();
}

void PowertrainCommandQueue::AddCommand(PowertrainCommand::CommandType type, float value /* = 0.f */)
{
    PowertrainCommand cmd;
    cmd.type = type;
    cmd.value = value;
    queue.push_back(cmd);
}

void PowertrainCommandQueue::NetworkedUpdate(float rpm, float force, float clutch, int gear, bool _running, bool _contact, char _automode)
{
    net_rpm = rpm;
    net_force = force;
    net_clutch = clutch;
    net_gear = gear;
    net_is_running = _running;
    net_has_contact = _contact;
    net_auto_mode = _automode;

    was_network_updated = true;
}

void PowertrainCommandQueue::Reset()
{
    queue.clear(); // Doesn't affect pre-allocated capacity

    was_network_updated = false;
    net_rpm = 0.f;
    net_force = 0.f;
    net_clutch = 0.f;
    net_gear = 0;
    net_is_running = false;
    net_has_contact = false;
    net_auto_mode = 0;

    input_states.Reset();
}
