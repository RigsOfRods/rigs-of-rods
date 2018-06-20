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

#include "AircraftSimulation.h"

#include <Overlay/OgreOverlayManager.h>
#include <Overlay/OgreOverlayElement.h>

#include "AeroEngine.h"
#include "Application.h"
#include "AutoPilot.h"
#include "Beam.h"
#include "BeamEngine.h"
#include "InputEngine.h"

using namespace RoR;

void AircraftSimulation::UpdateVehicle(Actor* vehicle, float seconds_since_last_frame)
{
    //autopilot
    if (vehicle->ar_autopilot && vehicle->ar_autopilot->wantsdisconnect)
    {
        vehicle->disconnectAutopilot();
    }
    //AIRPLANE KEYS
    float commandrate = 4.0;
    //turning
    if (vehicle->ar_replay_mode)
    {
        if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_REPLAY_FORWARD, 0.1f) && vehicle->ar_replay_pos <= 0)
        {
            vehicle->ar_replay_pos++;
        }
        if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_REPLAY_BACKWARD, 0.1f) && vehicle->ar_replay_pos > -vehicle->ar_replay_length)
        {
            vehicle->ar_replay_pos--;
        }
        if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_REPLAY_FAST_FORWARD, 0.1f) && vehicle->ar_replay_pos + 10 <= 0)
        {
            vehicle->ar_replay_pos += 10;
        }
        if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_REPLAY_FAST_BACKWARD, 0.1f) && vehicle->ar_replay_pos - 10 > -vehicle->ar_replay_length)
        {
            vehicle->ar_replay_pos -= 10;
        }
    }
    else
    {
        float tmp_left = RoR::App::GetInputEngine()->getEventValue(EV_AIRPLANE_STEER_LEFT);
        float tmp_right = RoR::App::GetInputEngine()->getEventValue(EV_AIRPLANE_STEER_RIGHT);
        float sum_steer = -tmp_left + tmp_right;
        RoR::App::GetInputEngine()->smoothValue(vehicle->ar_aileron, sum_steer, seconds_since_last_frame * commandrate);
        vehicle->ar_hydro_dir_command = vehicle->ar_aileron;
        vehicle->ar_hydro_speed_coupling = !(RoR::App::GetInputEngine()->isEventAnalog(EV_AIRPLANE_STEER_LEFT) && RoR::App::GetInputEngine()->isEventAnalog(EV_AIRPLANE_STEER_RIGHT));
    }

    //pitch
    float tmp_pitch_up = RoR::App::GetInputEngine()->getEventValue(EV_AIRPLANE_ELEVATOR_UP);
    float tmp_pitch_down = RoR::App::GetInputEngine()->getEventValue(EV_AIRPLANE_ELEVATOR_DOWN);
    float sum_pitch = tmp_pitch_down - tmp_pitch_up;
    RoR::App::GetInputEngine()->smoothValue(vehicle->ar_elevator, sum_pitch, seconds_since_last_frame * commandrate);

    //rudder
    float tmp_rudder_left = RoR::App::GetInputEngine()->getEventValue(EV_AIRPLANE_RUDDER_LEFT);
    float tmp_rudder_right = RoR::App::GetInputEngine()->getEventValue(EV_AIRPLANE_RUDDER_RIGHT);
    float sum_rudder = tmp_rudder_left - tmp_rudder_right;
    RoR::App::GetInputEngine()->smoothValue(vehicle->ar_rudder, sum_rudder, seconds_since_last_frame * commandrate);

    //brake
    if (!vehicle->ar_replay_mode && !vehicle->ar_parking_brake)
    {
        vehicle->ar_brake = 0.0;
        float brakevalue = RoR::App::GetInputEngine()->getEventValue(EV_AIRPLANE_BRAKE);
        vehicle->ar_brake = vehicle->ar_brake_force * 0.66 * brakevalue;
    }
    if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_AIRPLANE_PARKING_BRAKE))
    {
        vehicle->ToggleParkingBrake();
        if (RoR::App::GetOverlayWrapper())
        {
            Ogre::OverlayManager::getSingleton().getOverlayElement("tracks/ap_brks_but")->setMaterialName(
                    vehicle->ar_parking_brake ? "tracks/brks-on" : "tracks/brks-off");
        }
    }
    //reverse
    if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_AIRPLANE_REVERSE))
    {
        for (int i = 0; i < vehicle->ar_num_aeroengines; i++)
        {
            vehicle->ar_aeroengines[i]->toggleReverse();
        }
    }

    // toggle engines
    if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_AIRPLANE_TOGGLE_ENGINES))
    {
        for (int i = 0; i < vehicle->ar_num_aeroengines; i++)
        {
            vehicle->ar_aeroengines[i]->flipStart();
        }
    }

    //flaps
    if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_AIRPLANE_FLAPS_NONE))
    {
        if (vehicle->ar_aerial_flap > 0)
        {
            vehicle->ar_aerial_flap = 0;
        }
    }
    if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_AIRPLANE_FLAPS_FULL))
    {
        if (vehicle->ar_aerial_flap < 5)
        {
            vehicle->ar_aerial_flap = 5;
        }
    }
    if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_AIRPLANE_FLAPS_LESS))
    {
        if (vehicle->ar_aerial_flap > 0)
        {
            vehicle->ar_aerial_flap = (vehicle->ar_aerial_flap) - 1;
        }
    }
    if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_AIRPLANE_FLAPS_MORE))
    {
        if (vehicle->ar_aerial_flap < 5)
        {
            vehicle->ar_aerial_flap = (vehicle->ar_aerial_flap) + 1;
        }
    }

    //airbrakes
    if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_AIRPLANE_AIRBRAKES_NONE))
    {
        if (vehicle->ar_airbrake_intensity > 0)
        {
            vehicle->ar_airbrake_intensity = 0;
        }
    }
    if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_AIRPLANE_AIRBRAKES_FULL))
    {
        if (vehicle->ar_airbrake_intensity < 5)
        {
            vehicle->ar_airbrake_intensity = 5;
        }
    }
    if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_AIRPLANE_AIRBRAKES_LESS))
    {
        if (vehicle->ar_airbrake_intensity > 0)
        {
            vehicle->ar_airbrake_intensity = (vehicle->ar_airbrake_intensity) - 1;
        }
    }
    if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_AIRPLANE_AIRBRAKES_MORE))
    {
        if (vehicle->ar_airbrake_intensity < 5)
        {
            vehicle->ar_airbrake_intensity = (vehicle->ar_airbrake_intensity) + 1;
        }
    }

    //throttle
    float tmp_throttle = RoR::App::GetInputEngine()->getEventBoolValue(EV_AIRPLANE_THROTTLE);
    if (tmp_throttle > 0)
    {
        for (int i = 0; i < vehicle->ar_num_aeroengines; i++)
        {
            vehicle->ar_aeroengines[i]->setThrottle(tmp_throttle);
        }
    }
    if (RoR::App::GetInputEngine()->isEventDefined(EV_AIRPLANE_THROTTLE_AXIS))
    {
        float f = RoR::App::GetInputEngine()->getEventValue(EV_AIRPLANE_THROTTLE_AXIS);
        for (int i = 0; i < vehicle->ar_num_aeroengines; i++)
        {
            vehicle->ar_aeroengines[i]->setThrottle(f);
        }
    }
    if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_AIRPLANE_THROTTLE_DOWN, 0.1f))
    {
        //throttle down
        for (int i = 0; i < vehicle->ar_num_aeroengines; i++)
        {
            vehicle->ar_aeroengines[i]->setThrottle(vehicle->ar_aeroengines[i]->getThrottle() - 0.05);
        }
    }
    if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_AIRPLANE_THROTTLE_UP, 0.1f))
    {
        //throttle up
        for (int i = 0; i < vehicle->ar_num_aeroengines; i++)
        {
            vehicle->ar_aeroengines[i]->setThrottle(vehicle->ar_aeroengines[i]->getThrottle() + 0.05);
        }
    }
    if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_AIRPLANE_THROTTLE_NO, 0.1f))
    {
        // no throttle
        for (int i = 0; i < vehicle->ar_num_aeroengines; i++)
        {
            vehicle->ar_aeroengines[i]->setThrottle(0);
        }
    }
    if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_AIRPLANE_THROTTLE_FULL, 0.1f))
    {
        // full throttle
        for (int i = 0; i < vehicle->ar_num_aeroengines; i++)
        {
            vehicle->ar_aeroengines[i]->setThrottle(1);
        }
    }
    if (vehicle->ar_autopilot)
    {
        for (int i = 0; i < vehicle->ar_num_aeroengines; i++)
        {
            vehicle->ar_aeroengines[i]->setThrottle(vehicle->ar_autopilot->getThrottle(vehicle->ar_aeroengines[i]->getThrottle(), seconds_since_last_frame));
        }
    }
}
