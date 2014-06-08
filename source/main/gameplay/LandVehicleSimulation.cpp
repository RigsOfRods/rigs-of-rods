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

#include "LandVehicleSimulation.h"

#include "Application.h"
#include "Beam.h"
#include "BeamEngine.h"
#include "InputEngine.h"

using namespace RoR;

void LandVehicleSimulation::UpdateCruiseControl(Beam* curr_truck, float dt)
{
	if (RoR::Application::GetInputEngine()->getEventValue(EV_TRUCK_BRAKE) > 0.05f ||
		RoR::Application::GetInputEngine()->getEventValue(EV_TRUCK_MANUAL_CLUTCH) > 0.05f ||
		(curr_truck->cc_target_speed < curr_truck->cc_target_speed_lower_limit) ||
		(curr_truck->parkingbrake && curr_truck->engine->getGear() > 0) ||
		!curr_truck->engine->isRunning() ||
		!curr_truck->engine->hasContact())
	{
		curr_truck->cruisecontrolToggle();
		return;
	}

	if (curr_truck->engine->getGear() > 0)
	{
		// Try to maintain the target speed
		if (curr_truck->cc_target_speed > curr_truck->WheelSpeed)
		{
			float accl = (curr_truck->cc_target_speed - curr_truck->WheelSpeed) * 2.0f;
			accl = std::max(curr_truck->engine->getAcc(), accl);
			accl = std::min(accl, 1.0f);
			curr_truck->engine->setAcc(accl);
		}
	} else if (curr_truck->engine->getGear() == 0) // out of gear
	{
		// Try to maintain the target rpm
		if (curr_truck->cc_target_rpm > curr_truck->engine->getRPM())
		{
			float accl = (curr_truck->cc_target_rpm - curr_truck->engine->getRPM()) * 0.01f;
			accl = std::max(curr_truck->engine->getAcc(), accl);
			accl = std::min(accl, 1.0f);
			curr_truck->engine->setAcc(accl);
		}
	}

	if (RoR::Application::GetInputEngine()->getEventBoolValue(EV_TRUCK_CRUISE_CONTROL_ACCL))
	{
		if (curr_truck->engine->getGear() > 0)
		{
			curr_truck->cc_target_speed += 2.5f * dt;
			curr_truck->cc_target_speed  = std::max(curr_truck->cc_target_speed_lower_limit, curr_truck->cc_target_speed);
			if (curr_truck->sl_enabled)
			{
				curr_truck->cc_target_speed  = std::min(curr_truck->cc_target_speed, curr_truck->sl_speed_limit);
			}
		} else if (curr_truck->engine->getGear() == 0) // out of gear
		{
			curr_truck->cc_target_rpm += 1000.0f * dt;
			curr_truck->cc_target_rpm  = std::min(curr_truck->cc_target_rpm, curr_truck->engine->getMaxRPM());
		}
	}
	if (RoR::Application::GetInputEngine()->getEventBoolValue(EV_TRUCK_CRUISE_CONTROL_DECL))
	{
		if (curr_truck->engine->getGear() > 0)
		{
			curr_truck->cc_target_speed -= 2.5f * dt;
			curr_truck->cc_target_speed  = std::max(curr_truck->cc_target_speed_lower_limit, curr_truck->cc_target_speed);
		} else if (curr_truck->engine->getGear() == 0) // out of gear
		{
			curr_truck->cc_target_rpm -= 1000.0f * dt;
			curr_truck->cc_target_rpm  = std::max(curr_truck->engine->getMinRPM(), curr_truck->cc_target_rpm);
		}
	}
	if (RoR::Application::GetInputEngine()->getEventBoolValue(EV_TRUCK_CRUISE_CONTROL_READJUST))
	{
		curr_truck->cc_target_speed = std::max(curr_truck->WheelSpeed, curr_truck->cc_target_speed);
		if (curr_truck->sl_enabled)
		{
			curr_truck->cc_target_speed = std::min(curr_truck->cc_target_speed, curr_truck->sl_speed_limit);
		}
		curr_truck->cc_target_rpm   = curr_truck->engine->getRPM();
	}

	if (curr_truck->cc_can_brake)
	{
		if (curr_truck->WheelSpeed > curr_truck->cc_target_speed + 0.5f && !RoR::Application::GetInputEngine()->getEventValue(EV_TRUCK_ACCELERATE))
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
