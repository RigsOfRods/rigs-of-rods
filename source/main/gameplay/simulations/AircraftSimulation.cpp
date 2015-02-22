/*
	This source file is part of Rigs of Rods
	Copyright 2005-2012 Pierre-Michel Ricordel
	Copyright 2007-2012 Thomas Fischer
	Copyright 2013-2014 Petr Ohlidal

	For more information, see http://www.rigsofrods.com/

	Rigs of Rods is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License version 3, as
	published by the Free Software Foundation.

	Rigs of Rods is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Rigs of Rods. If not, see <http://www.gnu.org/licenses/>.
*/

#include "AircraftSimulation.h"

#include "AeroEngine.h"
#include "Application.h"
#include "AutoPilot.h"
#include "Beam.h"
#include "BeamEngine.h"
#include "InputEngine.h"

#include <OgreOverlayManager.h>

using namespace RoR;

void AircraftSimulation::UpdateVehicle(Beam* vehicle, float seconds_since_last_frame)
{
	//autopilot
	if (vehicle->autopilot && vehicle->autopilot->wantsdisconnect)
	{
		vehicle->disconnectAutopilot();
	}
	//AIRPLANE KEYS
	float commandrate=4.0;
	//turning
	if (vehicle->replaymode)
	{
		if (RoR::Application::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_REPLAY_FORWARD, 0.1f) && vehicle->replaypos<=0)
		{
			vehicle->replaypos++;
		}
		if (RoR::Application::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_REPLAY_BACKWARD, 0.1f) && vehicle->replaypos > -vehicle->replaylen)
		{
			vehicle->replaypos--;
		}
		if (RoR::Application::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_REPLAY_FAST_FORWARD, 0.1f) && vehicle->replaypos+10<=0)
		{
			vehicle->replaypos+=10;
		}
		if (RoR::Application::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_REPLAY_FAST_BACKWARD, 0.1f) && vehicle->replaypos-10 > -vehicle->replaylen)
		{
			vehicle->replaypos-=10;
		}
	} 
	else
	{
		float tmp_left = RoR::Application::GetInputEngine()->getEventValue(EV_AIRPLANE_STEER_LEFT);
		float tmp_right = RoR::Application::GetInputEngine()->getEventValue(EV_AIRPLANE_STEER_RIGHT);
		float sum_steer = -tmp_left + tmp_right;
		RoR::Application::GetInputEngine()->smoothValue(vehicle->aileron, sum_steer, seconds_since_last_frame*commandrate);
		vehicle->hydrodircommand = vehicle->aileron;
		vehicle->hydroSpeedCoupling = !(RoR::Application::GetInputEngine()->isEventAnalog(EV_AIRPLANE_STEER_LEFT) && RoR::Application::GetInputEngine()->isEventAnalog(EV_AIRPLANE_STEER_RIGHT));
	}

	//pitch
	float tmp_pitch_up = RoR::Application::GetInputEngine()->getEventValue(EV_AIRPLANE_ELEVATOR_UP);
	float tmp_pitch_down = RoR::Application::GetInputEngine()->getEventValue(EV_AIRPLANE_ELEVATOR_DOWN);
	float sum_pitch = tmp_pitch_down - tmp_pitch_up;
	RoR::Application::GetInputEngine()->smoothValue(vehicle->elevator, sum_pitch, seconds_since_last_frame*commandrate);

	//rudder
	float tmp_rudder_left = RoR::Application::GetInputEngine()->getEventValue(EV_AIRPLANE_RUDDER_LEFT);
	float tmp_rudder_right = RoR::Application::GetInputEngine()->getEventValue(EV_AIRPLANE_RUDDER_RIGHT);
	float sum_rudder = tmp_rudder_left - tmp_rudder_right;
	RoR::Application::GetInputEngine()->smoothValue(vehicle->rudder, sum_rudder, seconds_since_last_frame*commandrate);

	//brake
	if (!vehicle->replaymode && !vehicle->parkingbrake)
	{
		vehicle->brake=0.0;
		float brakevalue = RoR::Application::GetInputEngine()->getEventValue(EV_AIRPLANE_BRAKE);
		vehicle->brake=vehicle->brakeforce*0.66*brakevalue;
	}
	if (RoR::Application::GetInputEngine()->getEventBoolValueBounce(EV_AIRPLANE_PARKING_BRAKE))
	{
		vehicle->parkingbrakeToggle();
		if (RoR::Application::GetOverlayWrapper())
		{
			if (vehicle->parkingbrake)
			{
				Ogre::OverlayManager::getSingleton().getOverlayElement("tracks/ap_brks_but")->setMaterialName("tracks/brks-on");
			}
			else
			{
				Ogre::OverlayManager::getSingleton().getOverlayElement("tracks/ap_brks_but")->setMaterialName("tracks/brks-off");
			}
		}
	}
	//reverse
	if (RoR::Application::GetInputEngine()->getEventBoolValueBounce(EV_AIRPLANE_REVERSE))
	{
		for (int i=0; i<vehicle->free_aeroengine; i++)
		{
			vehicle->aeroengines[i]->toggleReverse();
		}
	}

	// toggle engines
	if (RoR::Application::GetInputEngine()->getEventBoolValueBounce(EV_AIRPLANE_TOGGLE_ENGINES))
	{
		for (int i=0; i<vehicle->free_aeroengine; i++)
		{
			vehicle->aeroengines[i]->flipStart();
		}
	}

	//flaps
	if (RoR::Application::GetInputEngine()->getEventBoolValueBounce(EV_AIRPLANE_FLAPS_NONE))
	{
		if (vehicle->flap>0)
		{
			vehicle->flap=0;
		}
	}
	if (RoR::Application::GetInputEngine()->getEventBoolValueBounce(EV_AIRPLANE_FLAPS_FULL))
	{
		if (vehicle->flap<5)
		{
			vehicle->flap=5;
		}
	}
	if (RoR::Application::GetInputEngine()->getEventBoolValueBounce(EV_AIRPLANE_FLAPS_LESS))
	{
		if (vehicle->flap>0)
		{
			vehicle->flap=(vehicle->flap)-1;
		}
	}
	if (RoR::Application::GetInputEngine()->getEventBoolValueBounce(EV_AIRPLANE_FLAPS_MORE))
	{
		if (vehicle->flap<5)
		{
			vehicle->flap=(vehicle->flap)+1;
		}
	}

	//airbrakes
	if (RoR::Application::GetInputEngine()->getEventBoolValueBounce(EV_AIRPLANE_AIRBRAKES_NONE))
	{
		if (vehicle->airbrakeval>0)
		{
			vehicle->airbrakeval=0;
		}
	}
	if (RoR::Application::GetInputEngine()->getEventBoolValueBounce(EV_AIRPLANE_AIRBRAKES_FULL))
	{
		if (vehicle->airbrakeval<5)
		{
			vehicle->airbrakeval=5;
		}
	}
	if (RoR::Application::GetInputEngine()->getEventBoolValueBounce(EV_AIRPLANE_AIRBRAKES_LESS))
	{
		if (vehicle->airbrakeval>0)
		{
			vehicle->airbrakeval=(vehicle->airbrakeval)-1;
		}
	}
	if (RoR::Application::GetInputEngine()->getEventBoolValueBounce(EV_AIRPLANE_AIRBRAKES_MORE))
	{
		if (vehicle->airbrakeval<5)
		{
			vehicle->airbrakeval=(vehicle->airbrakeval)+1;
		}
	}

	//throttle
	float tmp_throttle = RoR::Application::GetInputEngine()->getEventBoolValue(EV_AIRPLANE_THROTTLE);
	if (tmp_throttle > 0)
	{
		for (int i=0; i<vehicle->free_aeroengine; i++)
		{
			vehicle->aeroengines[i]->setThrottle(tmp_throttle);
		}
	}
	if (RoR::Application::GetInputEngine()->isEventDefined(EV_AIRPLANE_THROTTLE_AXIS))
	{
		float f = RoR::Application::GetInputEngine()->getEventValue(EV_AIRPLANE_THROTTLE_AXIS);
		for (int i=0; i<vehicle->free_aeroengine; i++)
		{
			vehicle->aeroengines[i]->setThrottle(f);
		}
	}
	if (RoR::Application::GetInputEngine()->getEventBoolValueBounce(EV_AIRPLANE_THROTTLE_DOWN, 0.1f))
	{
		//throttle down
		for (int i=0; i<vehicle->free_aeroengine; i++)
		{
			vehicle->aeroengines[i]->setThrottle(vehicle->aeroengines[i]->getThrottle()-0.05);
		}
	}
	if (RoR::Application::GetInputEngine()->getEventBoolValueBounce(EV_AIRPLANE_THROTTLE_UP, 0.1f))
	{
		//throttle up
		for (int i=0; i<vehicle->free_aeroengine; i++)
		{
			vehicle->aeroengines[i]->setThrottle(vehicle->aeroengines[i]->getThrottle()+0.05);
		}
	}
	if (RoR::Application::GetInputEngine()->getEventBoolValueBounce(EV_AIRPLANE_THROTTLE_NO, 0.1f))
	{
		// no throttle
		for (int i=0; i<vehicle->free_aeroengine; i++)
		{
			vehicle->aeroengines[i]->setThrottle(0);
		}
	}
	if (RoR::Application::GetInputEngine()->getEventBoolValueBounce(EV_AIRPLANE_THROTTLE_FULL, 0.1f))
	{
		// full throttle
		for (int i=0; i<vehicle->free_aeroengine; i++)
		{
			vehicle->aeroengines[i]->setThrottle(1);
		}
	}
	if (vehicle->autopilot)
	{
		for (int i=0; i<vehicle->free_aeroengine; i++)
		{
			vehicle->aeroengines[i]->setThrottle(vehicle->autopilot->getThrottle(vehicle->aeroengines[i]->getThrottle(), seconds_since_last_frame));
		}
	}
}