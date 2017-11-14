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
    if ((curr_truck->engine->getGear() > 0 && RoR::App::GetInputEngine()->getEventValue(EV_TRUCK_BRAKE) > 0.05f) ||
        (curr_truck->engine->getGear() > 0 && RoR::App::GetInputEngine()->getEventValue(EV_TRUCK_MANUAL_CLUTCH) > 0.05f) ||
        (curr_truck->engine->getGear() > 0 && curr_truck->parkingbrake) ||
        (curr_truck->engine->getGear() < 0) ||
        (curr_truck->cc_target_speed < curr_truck->cc_target_speed_lower_limit) ||
        !curr_truck->engine->isRunning() ||
        !curr_truck->engine->hasContact())
    {
        curr_truck->cruisecontrolToggle();
        return;
    }

    float acc = curr_truck->engine->getAccToHoldRPM(curr_truck->engine->getRPM());

    if (curr_truck->engine->getGear() > 0)
    {
        // Try to maintain the target speed
        float torque = (curr_truck->engine->getEngineTorque() + curr_truck->engine->getBrakingTorque()) * 0.8f;
        float forceRatio = curr_truck->getTotalMass(true) / torque;
        acc += (curr_truck->cc_target_speed - curr_truck->WheelSpeed) * forceRatio * 0.25;
        acc = std::max(-2.0f, acc);
        acc = std::min(acc, +2.0f);
    }
    else if (curr_truck->engine->getGear() == 0) // out of gear
    {
        // Try to maintain the target rpm
        float rpmRatio = 1.0f / (curr_truck->engine->getMaxRPM() - curr_truck->engine->getMinRPM());
        acc += (curr_truck->cc_target_rpm - curr_truck->engine->getRPM()) * rpmRatio * 2.0f;
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
    accl = std::max(curr_truck->engine->getAcc(), accl);
    accl = std::min(accl, 1.0f);
    curr_truck->engine->autoSetAcc(accl);

    if (RoR::App::GetInputEngine()->getEventBoolValue(EV_TRUCK_CRUISE_CONTROL_ACCL))
    {
        if (curr_truck->engine->getGear() > 0)
        {
            curr_truck->cc_target_speed += 2.5f * dt;
            curr_truck->cc_target_speed = std::max(curr_truck->cc_target_speed_lower_limit, curr_truck->cc_target_speed);
            if (curr_truck->sl_enabled)
            {
                curr_truck->cc_target_speed = std::min(curr_truck->cc_target_speed, curr_truck->sl_speed_limit);
            }
        }
        else if (curr_truck->engine->getGear() == 0) // out of gear
        {
            curr_truck->cc_target_rpm += 1000.0f * dt;
            curr_truck->cc_target_rpm = std::min(curr_truck->cc_target_rpm, curr_truck->engine->getMaxRPM());
        }
    }
    if (RoR::App::GetInputEngine()->getEventBoolValue(EV_TRUCK_CRUISE_CONTROL_DECL))
    {
        if (curr_truck->engine->getGear() > 0)
        {
            curr_truck->cc_target_speed -= 2.5f * dt;
            curr_truck->cc_target_speed = std::max(curr_truck->cc_target_speed_lower_limit, curr_truck->cc_target_speed);
        }
        else if (curr_truck->engine->getGear() == 0) // out of gear
        {
            curr_truck->cc_target_rpm -= 1000.0f * dt;
            curr_truck->cc_target_rpm = std::max(curr_truck->engine->getMinRPM(), curr_truck->cc_target_rpm);
        }
    }
    if (RoR::App::GetInputEngine()->getEventBoolValue(EV_TRUCK_CRUISE_CONTROL_READJUST))
    {
        curr_truck->cc_target_speed = std::max(curr_truck->WheelSpeed, curr_truck->cc_target_speed);
        if (curr_truck->sl_enabled)
        {
            curr_truck->cc_target_speed = std::min(curr_truck->cc_target_speed, curr_truck->sl_speed_limit);
        }
        curr_truck->cc_target_rpm = curr_truck->engine->getRPM();
    }

    if (curr_truck->cc_can_brake)
    {
        if (curr_truck->WheelSpeed > curr_truck->cc_target_speed + 0.5f && !RoR::App::GetInputEngine()->getEventValue(EV_TRUCK_ACCELERATE))
        {
            float brake = (curr_truck->WheelSpeed - curr_truck->cc_target_speed) * 0.5f;
            brake = std::min(brake, 1.0f);
            curr_truck->brake = curr_truck->brakeforce * brake;
        }
    }
}

void LandVehicleSimulation::CheckSpeedLimit(Beam* curr_truck, float dt)
{
    if (curr_truck->sl_enabled && curr_truck->engine->getGear() != 0)
    {
        float accl = (curr_truck->sl_speed_limit - std::abs(curr_truck->WheelSpeed)) * 2.0f;
        accl = std::max(0.0f, accl);
        accl = std::min(accl, curr_truck->engine->getAcc());
        curr_truck->engine->setAcc(accl);
    }
}

void LandVehicleSimulation::UpdateVehicle(Beam* curr_truck, float seconds_since_last_frame)
{
    using namespace Ogre;

    if (!curr_truck->replaymode)
    {
        if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_LEFT_MIRROR_LEFT))
            curr_truck->leftMirrorAngle -= 0.001;

        if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_LEFT_MIRROR_RIGHT))
            curr_truck->leftMirrorAngle += 0.001;

        if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_RIGHT_MIRROR_LEFT))
            curr_truck->rightMirrorAngle -= 0.001;

        if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_RIGHT_MIRROR_RIGHT))
            curr_truck->rightMirrorAngle += 0.001;
    } // end of (!curr_truck->replaymode) block

#ifdef USE_ANGELSCRIPT
    if (!curr_truck->replaymode && !curr_truck->vehicle_ai->IsActive())
#else
    if (!curr_truck->replaymode)
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

        curr_truck->hydrodircommand = sum;

        if ((tmp_left_digital < tmp_left_analog) || (tmp_right_digital < tmp_right_analog))
        {
            curr_truck->hydroSpeedCoupling = false;
        }
        else
        {
            curr_truck->hydroSpeedCoupling = true;
        }

        if (curr_truck->engine)
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
            if (!App::io_arcade_controls.GetActive() || (curr_truck->engine->getAutoMode() >= SimGearboxMode::MANUAL))
            {
                // classic mode, realistic
                curr_truck->engine->autoSetAcc(accl);
                curr_truck->brake = brake * curr_truck->brakeforce;
            }
            else
            {
                // start engine
                if (curr_truck->engine->hasContact() && !curr_truck->engine->isRunning() && (accl > 0 || brake > 0))
                {
                    curr_truck->engine->start();
                }

                // arcade controls: hey - people wanted it x| ... <- and it's convenient
                if (curr_truck->engine->getGear() >= 0)
                {
                    // neutral or drive forward, everything is as its used to be: brake is brake and accel. is accel.
                    curr_truck->engine->autoSetAcc(accl);
                    curr_truck->brake = brake * curr_truck->brakeforce;
                }
                else
                {
                    // reverse gear, reverse controls: brake is accel. and accel. is brake.
                    curr_truck->engine->autoSetAcc(brake);
                    curr_truck->brake = accl * curr_truck->brakeforce;
                }

                // only when the truck really is not moving anymore
                if (fabs(curr_truck->WheelSpeed) <= 1.0f)
                {
                    Vector3 hdir = curr_truck->getDirection();
                    float velocity = hdir.dotProduct(curr_truck->nodes[0].Velocity);

                    // switching point, does the user want to drive forward from backward or the other way round? change gears?
                    if (velocity < 1.0f && brake > 0.5f && accl < 0.5f && curr_truck->engine->getGear() > 0)
                    {
                        // we are on the brake, jump to reverse gear
                        if (curr_truck->engine->getAutoMode() == SimGearboxMode::AUTO)
                        {
                            curr_truck->engine->autoShiftSet(BeamEngine::REAR);
                        }
                        else
                        {
                            curr_truck->engine->setGear(-1);
                        }
                    }
                    else if (velocity > -1.0f && brake < 0.5f && accl > 0.5f && curr_truck->engine->getGear() < 0)
                    {
                        // we are on the gas pedal, jump to first gear when we were in rear gear
                        if (curr_truck->engine->getAutoMode() == SimGearboxMode::AUTO)
                        {
                            curr_truck->engine->autoShiftSet(BeamEngine::DRIVE);
                        }
                        else
                        {
                            curr_truck->engine->setGear(1);
                        }
                    }
                }
            }

            // IMI
            // gear management -- it might, should be transferred to a standalone function of Beam or RoRFrameListener
            if (curr_truck->engine->getAutoMode() == SimGearboxMode::AUTO)
            {
                if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_AUTOSHIFT_UP))
                {
                    curr_truck->engine->autoShiftUp();
                }
                if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_AUTOSHIFT_DOWN))
                {
                    curr_truck->engine->autoShiftDown();
                }
            }

            if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_TOGGLE_CONTACT))
            {
                curr_truck->engine->toggleContact();
            }

            if (RoR::App::GetInputEngine()->getEventBoolValue(EV_TRUCK_STARTER) && curr_truck->engine->hasContact() && !curr_truck->engine->isRunning())
            {
                // starter
                curr_truck->engine->setstarter(1);
#ifdef USE_OPENAL
                SoundScriptManager::getSingleton().trigStart(curr_truck, SS_TRIG_STARTER);
#endif // OPENAL
            }
            else
            {
                curr_truck->engine->setstarter(0);
#ifdef USE_OPENAL
                SoundScriptManager::getSingleton().trigStop(curr_truck, SS_TRIG_STARTER);
#endif // OPENAL
            }

            if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_SWITCH_SHIFT_MODES))
            {
                // toggle Auto shift
                curr_truck->engine->toggleAutoMode();

                // force gui update
                curr_truck->triggerGUIFeaturesChanged();
                const char* msg = nullptr;
                switch (curr_truck->engine->getAutoMode())
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
            curr_truck->engine->setManualClutch(cval);

            SimGearboxMode shiftmode = curr_truck->engine->getAutoMode();

            if (shiftmode <= SimGearboxMode::MANUAL) // auto, semi auto and sequential shifting
            {
                if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_SHIFT_UP))
                {
                    curr_truck->engine->shift(1);
                }
                else if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_SHIFT_DOWN))
                {
                    if (shiftmode > SimGearboxMode::SEMI_AUTO ||
                        shiftmode == SimGearboxMode::SEMI_AUTO && (!App::io_arcade_controls.GetActive()) ||
                        shiftmode == SimGearboxMode::SEMI_AUTO && curr_truck->engine->getGear() > 0 ||
                        shiftmode == SimGearboxMode::AUTO)
                    {
                        curr_truck->engine->shift(-1);
                    }
                }
                else if (shiftmode != SimGearboxMode::AUTO && RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_SHIFT_NEUTRAL))
                {
                    curr_truck->engine->shiftTo(0);
                }
            }
            else //if (shiftmode > SimGearboxMode::MANUAL) // h-shift or h-shift with ranges shifting
            {
                bool gear_changed = false;
                bool found = false;
                int curgear = curr_truck->engine->getGear();
                int curgearrange = curr_truck->engine->getGearRange();
                int gearoffset = std::max(0, curgear - curgearrange * 6);

                // one can select range only if in neutral
                if (shiftmode == SimGearboxMode::MANUAL_RANGES && curgear == 0)
                {
                    //  maybe this should not be here, but should experiment
                    if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_SHIFT_LOWRANGE) && curgearrange != 0)
                    {
                        curr_truck->engine->setGearRange(0);
                        gear_changed = true;
                        RoR::App::GetConsole()->putMessage(RoR::Console::CONSOLE_MSGTYPE_INFO, RoR::Console::CONSOLE_SYSTEM_NOTICE, _L("Low range selected"), "cog.png", 3000);
                    }
                    else if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_SHIFT_MIDRANGE) && curgearrange != 1 && curr_truck->engine->getNumGearsRanges() > 1)
                    {
                        curr_truck->engine->setGearRange(1);
                        gear_changed = true;
                        RoR::App::GetConsole()->putMessage(RoR::Console::CONSOLE_MSGTYPE_INFO, RoR::Console::CONSOLE_SYSTEM_NOTICE, _L("Mid range selected"), "cog.png", 3000);
                    }
                    else if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_SHIFT_HIGHRANGE) && curgearrange != 2 && curr_truck->engine->getNumGearsRanges() > 2)
                    {
                        curr_truck->engine->setGearRange(2);
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
                        curr_truck->engine->shiftTo(-1);
                        found = true;
                    }
                    else if (RoR::App::GetInputEngine()->getEventBoolValue(EV_TRUCK_SHIFT_NEUTRAL))
                    {
                        curr_truck->engine->shiftTo(0);
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
                                    curr_truck->engine->shiftTo(i);
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
                                    curr_truck->engine->shiftTo(i + curgearrange * 6);
                                    found = true;
                                }
                            }
                        }
                    }
                    if (!found)
                    {
                        curr_truck->engine->shiftTo(0);
                    }
                } // end of if (gear_changed)
            } // end of shitmode > SimGearboxMode::MANUAL

            if (curr_truck->engine->hasContact() &&
                curr_truck->engine->getAutoMode() == SimGearboxMode::AUTO &&
                curr_truck->engine->getAutoShift() != BeamEngine::NEUTRAL &&
                std::abs(curr_truck->WheelSpeed) < 0.1f)
            {
                Vector3 dirDiff = curr_truck->getDirection();
                Degree pitchAngle = Radian(asin(dirDiff.dotProduct(Vector3::UNIT_Y)));

                if (std::abs(pitchAngle.valueDegrees()) > 1.0f)
                {
                    if (curr_truck->engine->getAutoShift() > BeamEngine::NEUTRAL && curr_truck->WheelSpeed < +0.1f && pitchAngle.valueDegrees() > +1.0f ||
                        curr_truck->engine->getAutoShift() < BeamEngine::NEUTRAL && curr_truck->WheelSpeed > -0.1f && pitchAngle.valueDegrees() < -1.0f)
                    {
                        // anti roll back in SimGearboxMode::AUTO (DRIVE, TWO, ONE) mode
                        // anti roll forth in SimGearboxMode::AUTO (REAR) mode
                        float downhill_force = std::abs(sin(pitchAngle.valueRadians()) * curr_truck->getTotalMass());
                        float engine_force = std::abs(curr_truck->engine->getTorque());
                        float ratio = std::max(0.0f, 1.0f - (engine_force / downhill_force) / 2.0f);
                        curr_truck->brake = curr_truck->brakeforce * sqrt(ratio);
                    }
                }
                else if (brake == 0.0f && accl == 0.0f && curr_truck->parkingbrake == 0)
                {
                    float ratio = std::max(0.0f, 0.1f - std::abs(curr_truck->WheelSpeed)) * 5.0f;
                    curr_truck->brake = curr_truck->brakeforce * ratio;
                }
            }
        } // end of ->engine
#ifdef USE_OPENAL
        if (curr_truck->brake > curr_truck->brakeforce / 6.0f)
        {
            SoundScriptManager::getSingleton().trigStart(curr_truck, SS_TRIG_BRAKE);
        }
        else
        {
            SoundScriptManager::getSingleton().trigStop(curr_truck, SS_TRIG_BRAKE);
        }
#endif // USE_OPENAL
    } // end of ->replaymode

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

#ifdef USE_OPENAL
    if (curr_truck->ispolice)
    {
        if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_HORN))
        {
            SoundScriptManager::getSingleton().trigToggle(curr_truck, SS_TRIG_HORN);
        }
    }
    else
    {
        if (RoR::App::GetInputEngine()->getEventBoolValue(EV_TRUCK_HORN) && !curr_truck->replaymode)
        {
            SoundScriptManager::getSingleton().trigStart(curr_truck, SS_TRIG_HORN);
        }
        else
        {
            SoundScriptManager::getSingleton().trigStop(curr_truck, SS_TRIG_HORN);
        }
    }
#endif // OPENAL

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
