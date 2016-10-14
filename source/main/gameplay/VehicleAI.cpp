/*                                                  
	This source file is part of Rigs of Rods

	Copyright 2005-2012 Pierre-Michel Ricordel
	Copyright 2007-2012 Thomas Fischer
	Copyright 2013-2016 Petr Ohlidal

	For more information, see http://www.rigsofrods.org/

	Rigs of Rods is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License version 3, as
	published by the Free Software Foundation.

	Rigs of Rods is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Rigs of Rods.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifdef USE_ANGELSCRIPT

#include "VehicleAI.h"

#include "Beam.h"
#include "BeamEngine.h"
#include "GUI_GameConsole.h"

#include "MainThread.h"

#include "scriptdictionary/scriptdictionary.h"

using namespace Ogre;

VehicleAI::VehicleAI(Beam* b)
{
	beam = b;
}

VehicleAI::~VehicleAI()
{
}

void VehicleAI::SetActive(bool value)
{
	is_enabled = value;
}

bool VehicleAI::IsActive()
{
	return is_enabled;
}

void VehicleAI::AddWaypoint(String& id, Vector3& point)
{
	if (current_waypoint == Vector3::ZERO)
		current_waypoint = point;

	free_waypoints++;
	waypoints.emplace(free_waypoints, point);
	waypoint_ids.emplace(id, free_waypoints);
	waypoint_names.emplace(free_waypoints, id);
}

void VehicleAI::AddWaypoints(AngelScript::CScriptDictionary& d)
{
	for (auto dict : d.dict)
	{
		String id = dict.first;
		Vector3* point = (Vector3 *)dict.second.valueObj;
		AddWaypoint(id, *(point));
	}
}

void VehicleAI::AddEvent(String& id, int& ev)
{
	int waypointid = waypoint_ids[id];
	if (waypointid)
		waypoint_events.emplace(waypointid, ev);
}

void VehicleAI::SetValueAtWaypoint(String& id, int& value_id, float& value)
{
	int waypointid = waypoint_ids[id];
	if (waypointid)
	{
		switch (value_id)
		{
		case AI_SPEED:
			waypoint_speed.emplace(waypointid, value);
			break;
		case AI_POWER:
			waypoint_power.emplace(waypointid, value);
			break;
		default:
			break;
		}
	}
}

void VehicleAI::updateWaypoint()
{
	RoR::App::GetConsole()->putMessage(RoR::Console::CONSOLE_MSGTYPE_SCRIPT, RoR::Console::CONSOLE_SYSTEM_NOTICE, "Reached waypoint: " + waypoint_names[current_waypoint_id], "note.png");

	int event = waypoint_events[current_waypoint_id];
	if (event)
	{
		switch (event)
		{
		case AI_LIGHTSTOGGLE:
			beam->lightsToggle();
			break;
		case AI_BEACONSTOGGLE:
			beam->beaconsToggle();
			break;
		default:
			break;
		}
	}

	float speed = waypoint_speed[current_waypoint_id];
	if (speed)
		maxspeed = speed;

	float power = waypoint_power[current_waypoint_id];
	if (power)
		acc_power = power;

	current_waypoint_id++;
	if (current_waypoint_id > free_waypoints)
	{
		current_waypoint_id = 0;
		is_enabled = false;
		beam->parkingbrakeToggle();
	}
	current_waypoint = waypoints[current_waypoint_id];
}


void VehicleAI::update(float dt, int doUpdate)
{
	if (is_waiting)
	{
		wait_time -= dt;
		if (wait_time < 0)
		{
			is_waiting = false;
		}
		return;
	}

	Vector3 mAgentPosition = beam->getPosition();

	if (current_waypoint.distance(mAgentPosition) < 5)
	{
		updateWaypoint();
		return;
	}


	Vector3 TargetPosition = current_waypoint;
	TargetPosition.y = 0; //Vector3 > Vector2
	Quaternion TargetOrientation = Quaternion::ZERO;


	mAgentPosition.y = 0; //Vector3 > Vector2
	Quaternion mAgentOrientation = Quaternion(Radian(beam->getHeadingDirectionAngle()), Vector3::NEGATIVE_UNIT_Y);
	mAgentOrientation.normalise();

	Vector3 mVectorToTarget = TargetPosition - mAgentPosition; // A-B = B->A
	mAgentPosition.normalise();

	Vector3 mAgentHeading = mAgentOrientation * mAgentPosition;
	Vector3 mTargetHeading = TargetOrientation * TargetPosition;
	mAgentHeading.normalise();
	mTargetHeading.normalise();

	/* Orientation control - Vector3::UNIT_Y is common up vector.
	Vector3 mAgentVO = mAgentOrientation.Inverse() * Vector3::UNIT_Y;
	Vector3 mTargetVO = TargetOrientation * Vector3::UNIT_Y;*/

	// Compute new torque scalar (-1.0 to 1.0) based on heading vector to target.
	Vector3 mSteeringForce = mAgentOrientation.Inverse() * mVectorToTarget;
	mSteeringForce.normalise();

	float mYaw = mSteeringForce.x;
	float mPitch = mSteeringForce.z;
	//float mRoll   = mTargetVO.getRotationTo( mAgentVO ).getRoll().valueRadians();

	if (mPitch > 0)
	{
		if (mYaw > 0)
			mYaw = 1;
		else
			mYaw = -1;
	}

	// actually steer
	beam->hydrodircommand = mYaw;//mYaw


	if (beam->engine)
	{
		// start engine if not running
		if (!beam->engine->isRunning())
			beam->engine->start();

		float kmh_wheel_speed = beam->getWheelSpeed() * 3.6;

		if (abs(mYaw) < 0.5f)
		{
			if (kmh_wheel_speed < maxspeed - 1)
			{
				beam->brake = 0;
				beam->engine->autoSetAcc(acc_power);
			}
			else if (kmh_wheel_speed > maxspeed + 1)
			{
				beam->brake = beam->brakeforce / 3;
				beam->engine->autoSetAcc(0);
			}
			else
			{
				beam->brake = 0;
				beam->engine->autoSetAcc(0);
			}
		}
		else
		{
			if (kmh_wheel_speed < maxspeed - 1)
			{
				beam->brake = 0;
				beam->engine->autoSetAcc(acc_power / 3);
			}
			else if (kmh_wheel_speed > maxspeed + 1)
			{
				beam->brake = beam->brakeforce / 2;
				beam->engine->autoSetAcc(0);
			}
			else
			{
				beam->brake = 0;
				beam->engine->autoSetAcc(0);
			}
		}
	}


#if 0
	// this is for debugging purposes
	static SceneNode *n = 0;
	if (!n)
	{
		Entity *e = gEnv->sceneManager->createEntity("axes.mesh");
		n = gEnv->sceneManager->getRootSceneNode()->createChildSceneNode();
		n->attachObject(e);
	}
	n->setPosition(beam->getPosition() + mVectorToTarget);
	n->setOrientation(mAgentOrientation);

#endif

	/* Old code
		// accelerate / brake
		float maxvelo = 1;

		maxvelo = std::max<float>(0.2f, 1 - fabs(mYaw)) * 50;

		maxvelo += std::max<float>(5, std::min<float>(mVectorToTarget.length(), 50)) - 50;

		if (maxvelo < 0)
			maxvelo = 0;

		float pitch = 0.0f;
		// pitch
		if (cameranodepos[0] >= 0 && cameranodepos[0] < MAX_NODES)
		{
			Vector3 dir = nodes[cameranodepos[0]].RelPosition - nodes[cameranodedir[0]].RelPosition;
			dir.normalise();
			float angle = asin(dir.dotProduct(Vector3::UNIT_Y));
			if (angle < -1) angle = -1;
			if (angle > 1) angle = 1;

			pitch = Radian(angle).valueDegrees();
		}

		//More power for uphill
		float power = 80 + pitch;
		power = power / 100;


		//String txt = "brakePower: "+TOSTRING(brakePower);//+" @ "+TOSTRING(maxvelo)
		//RoR::App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_SCRIPT, Console::CONSOLE_SYSTEM_NOTICE, txt, "note.png");


		if (engine)
		{
			if (mVectorToTarget.length() > 5.0f)
			{
				if (pitch < -5 && WheelSpeed > 10)
				{
					if (pitch < 0) pitch = -pitch;
					brake = pitch * brakeforce / 90;
					engine->autoSetAcc(0);
				}
				else
				{
					if (WheelSpeed < maxvelo - 5)
					{
						brake = 0;
						engine->autoSetAcc(power);
					}
					else if (WheelSpeed > maxvelo + 5)
					{
						brake = brakeforce / 3;
						engine->autoSetAcc(0);
					}
					else
					{
						brake = 0;
						engine->autoSetAcc(0);
					}
				}
				return false;
			}
			else
			{
				engine->autoSetAcc(0);
				brake = brakeforce;
				return true;
			}
		}
		else
		{
			return true;
		}
		*/
}

#endif // USE_ANGELSCRIPT
