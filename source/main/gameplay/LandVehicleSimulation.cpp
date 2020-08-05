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
#include "Actor.h"
#include "Console.h"
#include "EngineSim.h"
#include "InputEngine.h"
#include "Language.h"
#include "SoundScriptManager.h"
#include "TerrainManager.h"
#include "GUIManager.h"
#include "VehicleAI.h"

using namespace RoR;

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
        vehicle->UpdateCruiseControl(seconds_since_last_frame);
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
