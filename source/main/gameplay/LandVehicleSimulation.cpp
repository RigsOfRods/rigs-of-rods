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
#include "Console.h"
#include "InputEngine.h"
#include "Language.h"
#include "SoundScriptManager.h"
#include "TerrainManager.h"
#include "GUIManager.h"
#include "VehicleAI.h"

using namespace RoR;

void LandVehicleSimulation::UpdateCruiseControl(Actor* vehicle, float dt)
{
    EngineSim* engine = vehicle->ar_engine;

    if ((engine->GetGear() > 0 && RoR::App::GetInputEngine()->getEventValue(EV_TRUCK_BRAKE) > 0.05f) ||
        (engine->GetGear() > 0 && vehicle->cc_target_speed < vehicle->cc_target_speed_lower_limit) ||
        (engine->GetGear() > 0 && vehicle->ar_parking_brake) ||
        (engine->GetGear() < 0) ||
        !engine->IsRunning() ||
        !engine->HasStarterContact())
    {
        vehicle->ToggleCruiseControl();
        return;
    }

    if (engine->GetGear() != 0 && RoR::App::GetInputEngine()->getEventValue(EV_TRUCK_MANUAL_CLUTCH) > 0.05f)
        return;

    float acc = engine->GetAccToHoldRPM();

    if (engine->GetGear() > 0)
    {
        // Try to maintain the target speed
        float power_weight_ratio = vehicle->getTotalMass(true) / engine->getEnginePower();
        acc += (vehicle->cc_target_speed - vehicle->ar_wheel_speed) * power_weight_ratio * 0.25;
    }
    else if (engine->GetGear() == 0) // out of gear
    {
        // Try to maintain the target rpm
        float speed_range = (engine->getMaxRPM() - engine->getMinRPM()) / 50.0f;
        acc += engine->GetEngineInertia() * (vehicle->cc_target_rpm - engine->GetEngineRpm()) / speed_range;
    }

    vehicle->cc_accs.push_front(Ogre::Math::Clamp(acc, -1.0f, +1.0f));
    if (vehicle->cc_accs.size() > 30)
    {
        vehicle->cc_accs.pop_back();
    }

    float avg_acc = 0.0f;
    for (unsigned int i = 0; i < vehicle->cc_accs.size(); i++)
    {
        avg_acc += vehicle->cc_accs[i];
    }
    avg_acc /= vehicle->cc_accs.size();

    engine->autoSetAcc(Ogre::Math::Clamp(avg_acc, engine->GetAcceleration(), 1.0f));

    if (RoR::App::GetInputEngine()->getEventBoolValue(EV_TRUCK_CRUISE_CONTROL_ACCL))
    {
        if (engine->GetGear() > 0)
        {
            vehicle->cc_target_speed *= pow(2.0f, dt / 5.0f);
            vehicle->cc_target_speed = std::max(vehicle->cc_target_speed_lower_limit, vehicle->cc_target_speed);
            if (vehicle->sl_enabled)
            {
                vehicle->cc_target_speed = std::min(vehicle->cc_target_speed, vehicle->sl_speed_limit);
            }
        }
        else if (engine->GetGear() == 0) // out of gear
        {
            vehicle->cc_target_rpm *= pow(2.0f, dt / 5.0f);
            vehicle->cc_target_rpm = std::min(vehicle->cc_target_rpm, engine->getMaxRPM());
        }
    }
    if (RoR::App::GetInputEngine()->getEventBoolValue(EV_TRUCK_CRUISE_CONTROL_DECL))
    {
        if (engine->GetGear() > 0)
        {
            vehicle->cc_target_speed *= pow(0.5f, dt / 5.0f);
            vehicle->cc_target_speed = std::max(vehicle->cc_target_speed_lower_limit, vehicle->cc_target_speed);
        }
        else if (engine->GetGear() == 0) // out of gear
        {
            vehicle->cc_target_rpm *= pow(0.5f, dt / 5.0f);
            vehicle->cc_target_rpm = std::max(engine->getMinRPM(), vehicle->cc_target_rpm);
        }
    }
    if (RoR::App::GetInputEngine()->getEventBoolValue(EV_TRUCK_CRUISE_CONTROL_READJUST))
    {
        vehicle->cc_target_speed = std::max(vehicle->ar_avg_wheel_speed, vehicle->cc_target_speed);
        if (vehicle->sl_enabled)
        {
            vehicle->cc_target_speed = std::min(vehicle->cc_target_speed, vehicle->sl_speed_limit);
        }
        vehicle->cc_target_rpm = engine->GetEngineRpm();
    }

    if (vehicle->cc_can_brake)
    {
        if (vehicle->ar_avg_wheel_speed > vehicle->cc_target_speed + 0.5f && !RoR::App::GetInputEngine()->getEventValue(EV_TRUCK_ACCELERATE))
        {
            float brake = (vehicle->ar_avg_wheel_speed - vehicle->cc_target_speed) * 0.5f;
            vehicle->ar_brake = std::min(brake, 1.0f);
        }
    }
}

void LandVehicleSimulation::CheckSpeedLimit(Actor* vehicle, float dt)
{
    EngineSim* engine = vehicle->ar_engine;

    if (engine && engine->GetGear() != 0)
    {
        float accl = (vehicle->sl_speed_limit - std::abs(vehicle->ar_wheel_speed / 1.02f)) * 2.0f;
        engine->SetAcceleration(Ogre::Math::Clamp(accl, 0.0f, engine->GetAcceleration()));
    }
}

void LandVehicleSimulation::UpdateVehicle(Actor* vehicle, float seconds_since_last_frame)
{
    if (vehicle->isBeingReset() || vehicle->ar_physics_paused || vehicle->ar_replay_mode)
        return;
#ifdef USE_ANGELSCRIPT
    if (vehicle->ar_vehicle_ai && vehicle->ar_vehicle_ai->IsActive())
        return;
#endif // USE_ANGELSCRIPT

    EngineSim* engine = vehicle->ar_engine;

    if (engine && engine->HasStarterContact() &&
        engine->GetAutoShiftMode() == SimGearboxMode::AUTO &&
        engine->getAutoShift() != EngineSim::NEUTRAL)
    {
        Ogre::Vector3 dirDiff = vehicle->getDirection();
        Ogre::Degree pitchAngle = Ogre::Radian(asin(dirDiff.dotProduct(Ogre::Vector3::UNIT_Y)));

        if (std::abs(pitchAngle.valueDegrees()) > 2.0f)
        {
            if (engine->getAutoShift() > EngineSim::NEUTRAL && vehicle->ar_avg_wheel_speed < +0.02f && pitchAngle.valueDegrees() > 0.0f ||
                engine->getAutoShift() < EngineSim::NEUTRAL && vehicle->ar_avg_wheel_speed > -0.02f && pitchAngle.valueDegrees() < 0.0f)
            {
                // anti roll back in SimGearboxMode::AUTO (DRIVE, TWO, ONE) mode
                // anti roll forth in SimGearboxMode::AUTO (REAR) mode
                float g = std::abs(App::GetSimTerrain()->getGravity());
                float downhill_force = std::abs(sin(pitchAngle.valueRadians()) * vehicle->getTotalMass()) * g;
                float engine_force = std::abs(engine->GetTorque()) / vehicle->getAvgPropedWheelRadius();
                float ratio = std::max(0.0f, 1.0f - (engine_force / downhill_force));
                if (vehicle->ar_avg_wheel_speed * pitchAngle.valueDegrees() > 0.0f)
                {
                    ratio *= sqrt((0.02f - vehicle->ar_avg_wheel_speed) / 0.02f);
                }
                vehicle->ar_brake = sqrt(ratio);
            }
        }
        else if (vehicle->ar_brake == 0.0f && !vehicle->ar_parking_brake && engine->GetTorque() == 0.0f)
        {
            float ratio = std::max(0.0f, 0.2f - std::abs(vehicle->ar_avg_wheel_speed)) / 0.2f;
            vehicle->ar_brake = ratio;
        }
    }

    if (vehicle->cc_mode)
    {
        LandVehicleSimulation::UpdateCruiseControl(vehicle, seconds_since_last_frame);
    }
    if (vehicle->sl_enabled)
    {
        LandVehicleSimulation::CheckSpeedLimit(vehicle, seconds_since_last_frame);
    }
}

void LandVehicleSimulation::UpdateInputEvents(Actor* vehicle, float seconds_since_last_frame)
{
    if (vehicle->isBeingReset() || vehicle->ar_physics_paused || vehicle->ar_replay_mode)
        return;
#ifdef USE_ANGELSCRIPT
    if (vehicle->ar_vehicle_ai && vehicle->ar_vehicle_ai->IsActive())
        return;
#endif // USE_ANGELSCRIPT

    if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_LEFT_MIRROR_LEFT))
        vehicle->ar_left_mirror_angle -= 0.001;

    if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_LEFT_MIRROR_RIGHT))
        vehicle->ar_left_mirror_angle += 0.001;

    if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_RIGHT_MIRROR_LEFT))
        vehicle->ar_right_mirror_angle -= 0.001;

    if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_RIGHT_MIRROR_RIGHT))
        vehicle->ar_right_mirror_angle += 0.001;

    // steering
    float tmp_left_digital  = App::GetInputEngine()->getEventValue(EV_TRUCK_STEER_LEFT , false, InputEngine::ET_DIGITAL);
    float tmp_right_digital = App::GetInputEngine()->getEventValue(EV_TRUCK_STEER_RIGHT, false, InputEngine::ET_DIGITAL);
    float tmp_left_analog   = App::GetInputEngine()->getEventValue(EV_TRUCK_STEER_LEFT , false, InputEngine::ET_ANALOG);
    float tmp_right_analog  = App::GetInputEngine()->getEventValue(EV_TRUCK_STEER_RIGHT, false, InputEngine::ET_ANALOG);

    float sum = -std::max(tmp_left_digital, tmp_left_analog) + std::max(tmp_right_digital, tmp_right_analog);

    vehicle->ar_hydro_dir_command = Ogre::Math::Clamp(sum, -1.0f, 1.0f);

    vehicle->ar_hydro_speed_coupling = (tmp_left_digital >= tmp_left_analog) && (tmp_right_digital >= tmp_right_analog);

    EngineSim* engine = vehicle->ar_engine;

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
        if (!App::io_arcade_controls->GetActiveVal<bool>() || (engine->GetAutoShiftMode() >= SimGearboxMode::MANUAL))
        {
            // classic mode, realistic
            engine->autoSetAcc(accl);
            vehicle->ar_brake = brake;
        }
        else
        {
            // start engine
            if (engine->HasStarterContact() && !engine->IsRunning() && (accl > 0 || brake > 0))
            {
                engine->StartEngine();
            }

            // arcade controls: hey - people wanted it x| ... <- and it's convenient
            if (engine->GetGear() >= 0)
            {
                // neutral or drive forward, everything is as its used to be: brake is brake and accel. is accel.
                engine->autoSetAcc(accl);
                vehicle->ar_brake = brake;
            }
            else
            {
                // reverse gear, reverse controls: brake is accel. and accel. is brake.
                engine->autoSetAcc(brake);
                vehicle->ar_brake = accl;
            }

            // only when the truck really is not moving anymore
            if (fabs(vehicle->ar_avg_wheel_speed) <= 1.0f)
            {
                Ogre::Vector3 hdir = vehicle->getDirection();
                float velocity = hdir.dotProduct(vehicle->ar_nodes[0].Velocity);

                // switching point, does the user want to drive forward from backward or the other way round? change gears?
                if (velocity < 1.0f && brake > 0.5f && accl < 0.5f && engine->GetGear() > 0)
                {
                    // we are on the brake, jump to reverse gear
                    if (engine->GetAutoShiftMode() == SimGearboxMode::AUTO)
                    {
                        engine->autoShiftSet(EngineSim::REAR);
                    }
                    else
                    {
                        engine->SetGear(-1);
                    }
                }
                else if (velocity > -1.0f && brake < 0.5f && accl > 0.5f && engine->GetGear() < 0)
                {
                    // we are on the gas pedal, jump to first gear when we were in rear gear
                    if (engine->GetAutoShiftMode() == SimGearboxMode::AUTO)
                    {
                        engine->autoShiftSet(EngineSim::DRIVE);
                    }
                    else
                    {
                        engine->SetGear(1);
                    }
                }
            }
        }

        // IMI
        // gear management -- it might, should be transferred to a standalone function of Beam or SimController
        if (engine->GetAutoShiftMode() == SimGearboxMode::AUTO)
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
            engine->ToggleStarterContact();
        }

        if (RoR::App::GetInputEngine()->getEventBoolValue(EV_TRUCK_STARTER) && engine->HasStarterContact() && !engine->IsRunning())
        {
            // starter
            engine->SetStarter(1);
            SOUND_START(vehicle, SS_TRIG_STARTER);
        }
        else
        {
            engine->SetStarter(0);
            SOUND_STOP(vehicle, SS_TRIG_STARTER);
        }

        if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_SWITCH_SHIFT_MODES))
        {
            // toggle Auto shift
            engine->ToggleAutoShiftMode();

            // force gui update
            vehicle->RequestUpdateHudFeatures();
            const char* msg = nullptr;
            switch (engine->GetAutoShiftMode())
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
        }

        // joy clutch
        float cval = RoR::App::GetInputEngine()->getEventValue(EV_TRUCK_MANUAL_CLUTCH);
        engine->setManualClutch(cval);

        SimGearboxMode shiftmode = engine->GetAutoShiftMode();

        if (shiftmode <= SimGearboxMode::MANUAL) // auto, semi auto and sequential shifting
        {
            if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_SHIFT_UP))
            {
                engine->shift(1);
            }
            else if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_SHIFT_DOWN))
            {
                if (shiftmode > SimGearboxMode::SEMI_AUTO ||
                    shiftmode == SimGearboxMode::SEMI_AUTO && (!App::io_arcade_controls->GetActiveVal<bool>()) ||
                    shiftmode == SimGearboxMode::SEMI_AUTO && engine->GetGear() > 0 ||
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
            int curgear = engine->GetGear();
            int curgearrange = engine->GetGearRange();
            int gearoffset = std::max(0, curgear - curgearrange * 6);

            // one can select range only if in neutral
            if (shiftmode == SimGearboxMode::MANUAL_RANGES && curgear == 0)
            {
                //  maybe this should not be here, but should experiment
                if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_SHIFT_LOWRANGE) && curgearrange != 0)
                {
                    engine->SetGearRange(0);
                    gear_changed = true;
                    RoR::App::GetConsole()->putMessage(RoR::Console::CONSOLE_MSGTYPE_INFO, RoR::Console::CONSOLE_SYSTEM_NOTICE, _L("Low range selected"), "cog.png", 3000);
                }
                else if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_SHIFT_MIDRANGE) && curgearrange != 1 && engine->getNumGearsRanges() > 1)
                {
                    engine->SetGearRange(1);
                    gear_changed = true;
                    RoR::App::GetConsole()->putMessage(RoR::Console::CONSOLE_MSGTYPE_INFO, RoR::Console::CONSOLE_SYSTEM_NOTICE, _L("Mid range selected"), "cog.png", 3000);
                }
                else if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_SHIFT_HIGHRANGE) && curgearrange != 2 && engine->getNumGearsRanges() > 2)
                {
                    engine->SetGearRange(2);
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
                gear_changed = !RoR::App::GetInputEngine()->getEventBoolValue(EV_TRUCK_SHIFT_GEAR01 + gearoffset - 1); // range mode
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
    } // end of ->engine
    if (vehicle->ar_brake > 1.0f / 6.0f)
    {
        SOUND_START(vehicle, SS_TRIG_BRAKE);
    }
    else
    {
        SOUND_STOP(vehicle, SS_TRIG_BRAKE);
    }

    if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_TOGGLE_INTER_AXLE_DIFF))
    {
        vehicle->ToggleAxleDiffMode();
        vehicle->DisplayAxleDiffMode();
    }

    if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_TOGGLE_INTER_WHEEL_DIFF))
    {
        vehicle->ToggleWheelDiffMode();
        vehicle->DisplayWheelDiffMode();
    }

    if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_TOGGLE_TCASE_4WD_MODE))
    {
        vehicle->ToggleTransferCaseMode();
        vehicle->DisplayTransferCaseMode();
    }

    if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_TOGGLE_TCASE_GEAR_RATIO))
    {
        vehicle->ToggleTransferCaseGearRatio();
        vehicle->DisplayTransferCaseMode();
    }

    if (vehicle->ar_is_police)
    {
        if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_HORN))
        {
            SOUND_TOGGLE(vehicle, SS_TRIG_HORN);
        }
    }
    else
    {
        if (RoR::App::GetInputEngine()->getEventBoolValue(EV_TRUCK_HORN))
        {
            SOUND_START(vehicle, SS_TRIG_HORN);
        }
        else
        {
            SOUND_STOP(vehicle, SS_TRIG_HORN);
        }
    }

    if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_PARKING_BRAKE) &&
            !RoR::App::GetInputEngine()->getEventBoolValue(EV_TRUCK_TRAILER_PARKING_BRAKE))
    {
        vehicle->ToggleParkingBrake();
    }

    if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_ANTILOCK_BRAKE))
    {
        vehicle->ToggleAntiLockBrake();
    }

    if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_TRACTION_CONTROL))
    {
        vehicle->ToggleTractionControl();
    }

    if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_CRUISE_CONTROL))
    {
        vehicle->ToggleCruiseControl();
    }
}
