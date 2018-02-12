/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2013-2014 Petr Ohlidal

    For more information, see http://www.rigsofrods.org/

    Rigs of Rods is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 3, as
    published by the Free Software Foundation.

    Rigs of Rods is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Rigs of Rods. If not, see <http://www.gnu.org/licenses/>.
*/

#include "LandVehicleSimulation.h"

#include "Application.h"
#include "Beam.h"
#include "BeamEngine.h"
#include "GUI_GameConsole.h"
#include "InputEngine.h"
#include "Language.h"
#include "SoundScriptManager.h"
#include "GUIManager.h"
#include "VehicleAI.h"

using namespace RoR;

void LandVehicleSimulation::UpdateCruiseControl(Beam* curr_truck, float dt)
{
    BeamEngine* engine = curr_truck->ar_engine;

    if ((engine->getGear() > 0 && RoR::App::GetInputEngine()->getEventValue(EV_TRUCK_BRAKE) > 0.05f) ||
        (engine->getGear() > 0 && RoR::App::GetInputEngine()->getEventValue(EV_TRUCK_MANUAL_CLUTCH) > 0.05f) ||
        (engine->getGear() > 0 && curr_truck->ar_parking_brake) ||
        (engine->getGear() < 0) ||
        (curr_truck->cc_target_speed < curr_truck->cc_target_speed_lower_limit) ||
        !engine->isRunning() ||
        !engine->hasContact())
    {
        curr_truck->cruisecontrolToggle();
        return;
    }

    float acc = engine->getAccToHoldRPM(engine->getRPM());

    if (engine->getGear() > 0)
    {
        // Try to maintain the target speed
        float torque = (engine->getEngineTorque() + engine->getBrakingTorque()) * 0.8f;
        float forceRatio = curr_truck->getTotalMass(true) / torque;
        acc += (curr_truck->cc_target_speed - curr_truck->ar_wheel_speed) * forceRatio * 0.25;
        acc = std::max(-2.0f, acc);
        acc = std::min(acc, +2.0f);
    }
    else if (engine->getGear() == 0) // out of gear
    {
        // Try to maintain the target rpm
        float rpmRatio = 1.0f / (engine->getMaxRPM() - engine->getMinRPM());
        acc += (curr_truck->cc_target_rpm - engine->getRPM()) * rpmRatio * 2.0f;
    }

    curr_truck->cc_accs.push_front(acc);

    float avgAcc = 0.0f;

    for (unsigned int i = 0; i < curr_truck->cc_accs.size(); i++)
    {
        avgAcc += curr_truck->cc_accs[i];
    }

    if (curr_truck->cc_accs.size() > 10)
    {
        curr_truck->cc_accs.pop_back();
    }

    avgAcc /= curr_truck->cc_accs.size();

    float accl = avgAcc;
    accl = std::max(engine->getAcc(), accl);
    accl = std::min(accl, 1.0f);
    engine->autoSetAcc(accl);

    if (RoR::App::GetInputEngine()->getEventBoolValue(EV_TRUCK_CRUISE_CONTROL_ACCL))
    {
        if (engine->getGear() > 0)
        {
            curr_truck->cc_target_speed += 2.5f * dt;
            curr_truck->cc_target_speed = std::max(curr_truck->cc_target_speed_lower_limit, curr_truck->cc_target_speed);
            if (curr_truck->sl_enabled)
            {
                curr_truck->cc_target_speed = std::min(curr_truck->cc_target_speed, curr_truck->sl_speed_limit);
            }
        }
        else if (engine->getGear() == 0) // out of gear
        {
            curr_truck->cc_target_rpm += 1000.0f * dt;
            curr_truck->cc_target_rpm = std::min(curr_truck->cc_target_rpm, engine->getMaxRPM());
        }
    }
    if (RoR::App::GetInputEngine()->getEventBoolValue(EV_TRUCK_CRUISE_CONTROL_DECL))
    {
        if (engine->getGear() > 0)
        {
            curr_truck->cc_target_speed -= 2.5f * dt;
            curr_truck->cc_target_speed = std::max(curr_truck->cc_target_speed_lower_limit, curr_truck->cc_target_speed);
        }
        else if (engine->getGear() == 0) // out of gear
        {
            curr_truck->cc_target_rpm -= 1000.0f * dt;
            curr_truck->cc_target_rpm = std::max(engine->getMinRPM(), curr_truck->cc_target_rpm);
        }
    }
    if (RoR::App::GetInputEngine()->getEventBoolValue(EV_TRUCK_CRUISE_CONTROL_READJUST))
    {
        curr_truck->cc_target_speed = std::max(curr_truck->ar_wheel_speed, curr_truck->cc_target_speed);
        if (curr_truck->sl_enabled)
        {
            curr_truck->cc_target_speed = std::min(curr_truck->cc_target_speed, curr_truck->sl_speed_limit);
        }
        curr_truck->cc_target_rpm = engine->getRPM();
    }

    if (curr_truck->cc_can_brake)
    {
        if (curr_truck->ar_wheel_speed > curr_truck->cc_target_speed + 0.5f && !RoR::App::GetInputEngine()->getEventValue(EV_TRUCK_ACCELERATE))
        {
            float brake = (curr_truck->ar_wheel_speed - curr_truck->cc_target_speed) * 0.5f;
            brake = std::min(brake, 1.0f);
            curr_truck->ar_brake = curr_truck->ar_brake_force * brake;
        }
    }
}

void LandVehicleSimulation::CheckSpeedLimit(Beam* curr_truck, float dt)
{
    BeamEngine* engine = curr_truck->ar_engine;

    if (curr_truck->sl_enabled && engine->getGear() != 0)
    {
        float accl = (curr_truck->sl_speed_limit - std::abs(curr_truck->ar_wheel_speed)) * 2.0f;
        accl = std::max(0.0f, accl);
        accl = std::min(accl, engine->getAcc());
        engine->setAcc(accl);
    }
}

void LandVehicleSimulation::UpdateVehicle(Beam* curr_truck, float seconds_since_last_frame)
{
    using namespace Ogre;

    BeamEngine* engine = curr_truck->ar_engine;

    if (!curr_truck->ar_replay_mode)
    {
        if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_LEFT_MIRROR_LEFT))
            curr_truck->ar_left_mirror_angle -= 0.001;

        if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_LEFT_MIRROR_RIGHT))
            curr_truck->ar_left_mirror_angle += 0.001;

        if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_RIGHT_MIRROR_LEFT))
            curr_truck->ar_right_mirror_angle -= 0.001;

        if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_RIGHT_MIRROR_RIGHT))
            curr_truck->ar_right_mirror_angle += 0.001;
    } // end of (!curr_truck->ar_replay_mode) block

#ifdef USE_ANGELSCRIPT
    if (!curr_truck->ar_replay_mode && !curr_truck->ar_vehicle_ai->IsActive())
#else
    if (!curr_truck->ar_replay_mode)
#endif // USE_ANGELSCRIPT
    {
        // steering
        float tmp_left_digital = RoR::App::GetInputEngine()->getEventValue(EV_TRUCK_STEER_LEFT, false, InputEngine::ET_DIGITAL);
        float tmp_right_digital = RoR::App::GetInputEngine()->getEventValue(EV_TRUCK_STEER_RIGHT, false, InputEngine::ET_DIGITAL);
        float tmp_left_analog = RoR::App::GetInputEngine()->getEventValue(EV_TRUCK_STEER_LEFT, false, InputEngine::ET_ANALOG);
        float tmp_right_analog = RoR::App::GetInputEngine()->getEventValue(EV_TRUCK_STEER_RIGHT, false, InputEngine::ET_ANALOG);

        float sum = -std::max(tmp_left_digital, tmp_left_analog) + std::max(tmp_right_digital, tmp_right_analog);

        if (sum < -1)
            sum = -1;
        if (sum > 1)
            sum = 1;

        curr_truck->ar_hydro_dir_command = sum;

        if ((tmp_left_digital < tmp_left_analog) || (tmp_right_digital < tmp_right_analog))
        {
            curr_truck->ar_hydro_speed_coupling = false;
        }
        else
        {
            curr_truck->ar_hydro_speed_coupling = true;
        }

        if (engine)
        {
            float accl = RoR::App::GetInputEngine()->getEventValue(EV_TRUCK_ACCELERATE);
            float brake = RoR::App::GetInputEngine()->getEventValue(EV_TRUCK_BRAKE);

            if (RoR::App::GetInputEngine()->getEventValue(EV_TRUCK_ACCELERATE_MODIFIER_25) ||
                RoR::App::GetInputEngine()->getEventValue(EV_TRUCK_ACCELERATE_MODIFIER_50))
            {
                float acclModifier = 0.0f;
                if (RoR::App::GetInputEngine()->getEventValue(EV_TRUCK_ACCELERATE_MODIFIER_25))
                {
                    acclModifier += 0.25f;
                }
                if (RoR::App::GetInputEngine()->getEventValue(EV_TRUCK_ACCELERATE_MODIFIER_50))
                {
                    acclModifier += 0.50f;
                }
                accl *= acclModifier;
            }

            if (RoR::App::GetInputEngine()->getEventValue(EV_TRUCK_BRAKE_MODIFIER_25) ||
                RoR::App::GetInputEngine()->getEventValue(EV_TRUCK_BRAKE_MODIFIER_50))
            {
                float brakeModifier = 0.0f;
                if (RoR::App::GetInputEngine()->getEventValue(EV_TRUCK_BRAKE_MODIFIER_25))
                {
                    brakeModifier += 0.25f;
                }
                if (RoR::App::GetInputEngine()->getEventValue(EV_TRUCK_BRAKE_MODIFIER_50))
                {
                    brakeModifier += 0.50f;
                }
                brake *= brakeModifier;
            }

            // arcade controls are only working with auto-clutch!
            if (!App::io_arcade_controls.GetActive() || (engine->getAutoMode() >= SimGearboxMode::MANUAL))
            {
                // classic mode, realistic
                engine->autoSetAcc(accl);
                curr_truck->ar_brake = brake * curr_truck->ar_brake_force;
            }
            else
            {
                // start engine
                if (engine->hasContact() && !engine->isRunning() && (accl > 0 || brake > 0))
                {
                    engine->start();
                }

                // arcade controls: hey - people wanted it x| ... <- and it's convenient
                if (engine->getGear() >= 0)
                {
                    // neutral or drive forward, everything is as its used to be: brake is brake and accel. is accel.
                    engine->autoSetAcc(accl);
                    curr_truck->ar_brake = brake * curr_truck->ar_brake_force;
                }
                else
                {
                    // reverse gear, reverse controls: brake is accel. and accel. is brake.
                    engine->autoSetAcc(brake);
                    curr_truck->ar_brake = accl * curr_truck->ar_brake_force;
                }

                // only when the truck really is not moving anymore
                if (fabs(curr_truck->ar_wheel_speed) <= 1.0f)
                {
                    Vector3 hdir = curr_truck->getDirection();
                    float velocity = hdir.dotProduct(curr_truck->ar_nodes[0].Velocity);

                    // switching point, does the user want to drive forward from backward or the other way round? change gears?
                    if (velocity < 1.0f && brake > 0.5f && accl < 0.5f && engine->getGear() > 0)
                    {
                        // we are on the brake, jump to reverse gear
                        if (engine->getAutoMode() == SimGearboxMode::AUTO)
                        {
                            engine->autoShiftSet(BeamEngine::REAR);
                        }
                        else
                        {
                            engine->setGear(-1);
                        }
                    }
                    else if (velocity > -1.0f && brake < 0.5f && accl > 0.5f && engine->getGear() < 0)
                    {
                        // we are on the gas pedal, jump to first gear when we were in rear gear
                        if (engine->getAutoMode() == SimGearboxMode::AUTO)
                        {
                            engine->autoShiftSet(BeamEngine::DRIVE);
                        }
                        else
                        {
                            engine->setGear(1);
                        }
                    }
                }
            }

            // IMI
            // gear management -- it might, should be transferred to a standalone function of Beam or RoRFrameListener
            if (engine->getAutoMode() == SimGearboxMode::AUTO)
            {
                if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_AUTOSHIFT_UP))
                {
                    engine->autoShiftUp();
                }
                if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_AUTOSHIFT_DOWN))
                {
                    engine->autoShiftDown();
                }
            }

            if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_TOGGLE_CONTACT))
            {
                engine->toggleContact();
            }

            if (RoR::App::GetInputEngine()->getEventBoolValue(EV_TRUCK_STARTER) && engine->hasContact() && !engine->isRunning())
            {
                // starter
                engine->setstarter(1);
                SOUND_START(curr_truck, SS_TRIG_STARTER);
            }
            else
            {
                engine->setstarter(0);
                SOUND_STOP(curr_truck, SS_TRIG_STARTER);
            }

            if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_SWITCH_SHIFT_MODES))
            {
                // toggle Auto shift
                engine->toggleAutoMode();

                // force gui update
                curr_truck->RequestUpdateHudFeatures();
                const char* msg = nullptr;
                switch (engine->getAutoMode())
                {
                case SimGearboxMode::AUTO: msg = "Automatic shift";
                    break;
                case SimGearboxMode::SEMI_AUTO: msg = "Manual shift - Auto clutch";
                    break;
                case SimGearboxMode::MANUAL: msg = "Fully Manual: sequential shift";
                    break;
                case SimGearboxMode::MANUAL_STICK: msg = "Fully manual: stick shift";
                    break;
                case SimGearboxMode::MANUAL_RANGES: msg = "Fully Manual: stick shift with ranges";
                    break;
                }
                RoR::App::GetConsole()->putMessage(RoR::Console::CONSOLE_MSGTYPE_INFO, RoR::Console::CONSOLE_SYSTEM_NOTICE, _L(msg), "cog.png", 3000);
                RoR::App::GetGuiManager()->PushNotification("Gearbox Mode:", msg);
            }

            // joy clutch
            float cval = RoR::App::GetInputEngine()->getEventValue(EV_TRUCK_MANUAL_CLUTCH);
            engine->setManualClutch(cval);

            SimGearboxMode shiftmode = engine->getAutoMode();

            if (shiftmode <= SimGearboxMode::MANUAL) // auto, semi auto and sequential shifting
            {
                if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_SHIFT_UP))
                {
                    engine->shift(1);
                }
                else if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_SHIFT_DOWN))
                {
                    if (shiftmode > SimGearboxMode::SEMI_AUTO ||
                        shiftmode == SimGearboxMode::SEMI_AUTO && (!App::io_arcade_controls.GetActive()) ||
                        shiftmode == SimGearboxMode::SEMI_AUTO && engine->getGear() > 0 ||
                        shiftmode == SimGearboxMode::AUTO)
                    {
                        engine->shift(-1);
                    }
                }
                else if (shiftmode != SimGearboxMode::AUTO && RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_SHIFT_NEUTRAL))
                {
                    engine->shiftTo(0);
                }
            }
            else //if (shiftmode > SimGearboxMode::MANUAL) // h-shift or h-shift with ranges shifting
            {
                bool gear_changed = false;
                bool found = false;
                int curgear = engine->getGear();
                int curgearrange = engine->getGearRange();
                int gearoffset = std::max(0, curgear - curgearrange * 6);

                // one can select range only if in neutral
                if (shiftmode == SimGearboxMode::MANUAL_RANGES && curgear == 0)
                {
                    //  maybe this should not be here, but should experiment
                    if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_SHIFT_LOWRANGE) && curgearrange != 0)
                    {
                        engine->setGearRange(0);
                        gear_changed = true;
                        RoR::App::GetConsole()->putMessage(RoR::Console::CONSOLE_MSGTYPE_INFO, RoR::Console::CONSOLE_SYSTEM_NOTICE, _L("Low range selected"), "cog.png", 3000);
                    }
                    else if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_SHIFT_MIDRANGE) && curgearrange != 1 && engine->getNumGearsRanges() > 1)
                    {
                        engine->setGearRange(1);
                        gear_changed = true;
                        RoR::App::GetConsole()->putMessage(RoR::Console::CONSOLE_MSGTYPE_INFO, RoR::Console::CONSOLE_SYSTEM_NOTICE, _L("Mid range selected"), "cog.png", 3000);
                    }
                    else if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_SHIFT_HIGHRANGE) && curgearrange != 2 && engine->getNumGearsRanges() > 2)
                    {
                        engine->setGearRange(2);
                        gear_changed = true;
                        RoR::App::GetConsole()->putMessage(RoR::Console::CONSOLE_MSGTYPE_INFO, RoR::Console::CONSOLE_SYSTEM_NOTICE, _L("High range selected"), "cog.png", 3000);
                    }
                }
                //zaxxon
                if (curgear == -1)
                {
                    gear_changed = !RoR::App::GetInputEngine()->getEventBoolValue(EV_TRUCK_SHIFT_GEAR_REVERSE);
                }
                else if (curgear > 0 && curgear < 19)
                {
                    if (shiftmode == SimGearboxMode::MANUAL)
                    {
                        gear_changed = !RoR::App::GetInputEngine()->getEventBoolValue(EV_TRUCK_SHIFT_GEAR01 + curgear - 1);
                    }
                    else
                    {
                        gear_changed = !RoR::App::GetInputEngine()->getEventBoolValue(EV_TRUCK_SHIFT_GEAR01 + gearoffset - 1); // range mode
                    }
                }

                if (gear_changed || curgear == 0)
                {
                    if (RoR::App::GetInputEngine()->getEventBoolValue(EV_TRUCK_SHIFT_GEAR_REVERSE))
                    {
                        engine->shiftTo(-1);
                        found = true;
                    }
                    else if (RoR::App::GetInputEngine()->getEventBoolValue(EV_TRUCK_SHIFT_NEUTRAL))
                    {
                        engine->shiftTo(0);
                        found = true;
                    }
                    else
                    {
                        if (shiftmode == SimGearboxMode::MANUAL_STICK)
                        {
                            for (int i = 1; i < 19 && !found; i++)
                            {
                                if (RoR::App::GetInputEngine()->getEventBoolValue(EV_TRUCK_SHIFT_GEAR01 + i - 1))
                                {
                                    engine->shiftTo(i);
                                    found = true;
                                }
                            }
                        }
                        else // SimGearboxMode::MANUALMANUAL_RANGES
                        {
                            for (int i = 1; i < 7 && !found; i++)
                            {
                                if (RoR::App::GetInputEngine()->getEventBoolValue(EV_TRUCK_SHIFT_GEAR01 + i - 1))
                                {
                                    engine->shiftTo(i + curgearrange * 6);
                                    found = true;
                                }
                            }
                        }
                    }
                    if (!found)
                    {
                        engine->shiftTo(0);
                    }
                } // end of if (gear_changed)
            } // end of shitmode > SimGearboxMode::MANUAL

            if (engine->hasContact() &&
                engine->getAutoMode() == SimGearboxMode::AUTO &&
                engine->getAutoShift() != BeamEngine::NEUTRAL &&
                std::abs(curr_truck->ar_wheel_speed) < 0.1f)
            {
                Vector3 dirDiff = curr_truck->getDirection();
                Degree pitchAngle = Radian(asin(dirDiff.dotProduct(Vector3::UNIT_Y)));

                if (std::abs(pitchAngle.valueDegrees()) > 1.0f)
                {
                    if (engine->getAutoShift() > BeamEngine::NEUTRAL && curr_truck->ar_wheel_speed < +0.1f && pitchAngle.valueDegrees() > +1.0f ||
                        engine->getAutoShift() < BeamEngine::NEUTRAL && curr_truck->ar_wheel_speed > -0.1f && pitchAngle.valueDegrees() < -1.0f)
                    {
                        // anti roll back in SimGearboxMode::AUTO (DRIVE, TWO, ONE) mode
                        // anti roll forth in SimGearboxMode::AUTO (REAR) mode
                        float downhill_force = std::abs(sin(pitchAngle.valueRadians()) * curr_truck->getTotalMass());
                        float engine_force = std::abs(engine->getTorque());
                        float ratio = std::max(0.0f, 1.0f - (engine_force / downhill_force) / 2.0f);
                        curr_truck->ar_brake = curr_truck->ar_brake_force * sqrt(ratio);
                    }
                }
                else if (brake == 0.0f && accl == 0.0f && curr_truck->ar_parking_brake == 0)
                {
                    float ratio = std::max(0.0f, 0.1f - std::abs(curr_truck->ar_wheel_speed)) * 5.0f;
                    curr_truck->ar_brake = curr_truck->ar_brake_force * ratio;
                }
            }
        } // end of ->engine
        if (curr_truck->ar_brake > curr_truck->ar_brake_force / 6.0f)
        {
            SOUND_START(curr_truck, SS_TRIG_BRAKE);
        }
        else
        {
            SOUND_STOP(curr_truck, SS_TRIG_BRAKE);
        }
    } // end of ->ar_replay_mode

    if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_TOGGLE_AXLE_LOCK))
    {
        // toggle auto shift
        if (!curr_truck->getAxleLockCount())
        {
            RoR::App::GetConsole()->putMessage(RoR::Console::CONSOLE_MSGTYPE_INFO, RoR::Console::CONSOLE_SYSTEM_NOTICE, _L("No differential installed on current vehicle!"), "warning.png", 3000);
            RoR::App::GetGuiManager()->PushNotification("Differential:", "No differential installed on current vehicle!");
        }
        else
        {
            curr_truck->toggleAxleLock();
            RoR::App::GetConsole()->putMessage(RoR::Console::CONSOLE_MSGTYPE_INFO, RoR::Console::CONSOLE_SYSTEM_NOTICE, _L("Differentials switched to: ") + curr_truck->getAxleLockName(), "cog.png", 3000);
            RoR::App::GetGuiManager()->PushNotification("Differential:", "Differentials switched to: " + curr_truck->getAxleLockName());
        }
    }

    if (curr_truck->ar_is_police)
    {
        if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_HORN))
        {
            SOUND_TOGGLE(curr_truck, SS_TRIG_HORN);
        }
    }
    else
    {
        if (RoR::App::GetInputEngine()->getEventBoolValue(EV_TRUCK_HORN) && !curr_truck->ar_replay_mode)
        {
            SOUND_START(curr_truck, SS_TRIG_HORN);
        }
        else
        {
            SOUND_STOP(curr_truck, SS_TRIG_HORN);
        }
    }

    if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_PARKING_BRAKE))
    {
        curr_truck->parkingbrakeToggle();
    }

    if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_ANTILOCK_BRAKE))
    {
        if (curr_truck->alb_present && !curr_truck->alb_notoggle)
        {
            curr_truck->antilockbrakeToggle();
        }
    }

    if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_TRACTION_CONTROL))
    {
        if (!curr_truck->tc_notoggle)
            curr_truck->tractioncontrolToggle();
    }

    if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_CRUISE_CONTROL))
    {
        curr_truck->cruisecontrolToggle();
    }
    if (curr_truck->cc_mode)
    {
        LandVehicleSimulation::UpdateCruiseControl(curr_truck, seconds_since_last_frame);
    }
    LandVehicleSimulation::CheckSpeedLimit(curr_truck, seconds_since_last_frame);
}
