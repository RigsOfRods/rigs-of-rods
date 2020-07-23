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
#include "Console.h"
#include "EngineSim.h"
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
    if (vehicle->isBeingReset() || vehicle->ar_physics_paused)
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

void LandVehicleSimulation::UpdateInputEvents(Actor* vehicle, float dt)
{
    if (vehicle->isBeingReset() || vehicle->ar_physics_paused)
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

    if (vehicle->ar_engine)
    {
        vehicle->ar_engine->UpdateInputEvents(dt);
    }

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

    if (vehicle->GetTyrePressure().IsEnabled())
    {
        vehicle->GetTyrePressure().UpdateInputEvents(dt);
    }
}
