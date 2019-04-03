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
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Rigs of Rods. If not, see <http://www.gnu.org/licenses/>.
*/

#ifdef USE_ANGELSCRIPT

#include "VehicleAI.h"

#include "Beam.h"
#include "BeamEngine.h"
#include "GUI_GameConsole.h"

#include "MainMenu.h"

#include "scriptdictionary/scriptdictionary.h"

using namespace Ogre;

VehicleAI::VehicleAI(Actor* b) :
    is_waiting(false),
    wait_time(0.0f)
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
    for (auto item : d)
    {
        Ogre::Vector3 point;
        item.GetValue(&point, item.GetTypeId());
        Ogre::String key(item.GetKey());
        this->AddWaypoint(key, point);
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
            beam->ToggleLights();
            break;
        case AI_BEACONSTOGGLE:
            beam->ToggleBeacons();
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
        beam->ToggleParkingBrake();
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
    Quaternion mAgentOrientation = Quaternion(Radian(beam->getRotation()), Vector3::NEGATIVE_UNIT_Y);
    mAgentOrientation.normalise();

    Vector3 mVectorToTarget = TargetPosition - mAgentPosition; // A-B = B->A
    mAgentPosition.normalise();

    Vector3 mAgentHeading = mAgentOrientation * mAgentPosition;
    Vector3 mTargetHeading = TargetOrientation * TargetPosition;
    mAgentHeading.normalise();
    mTargetHeading.normalise();

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
    beam->ar_hydro_dir_command = mYaw;//mYaw

    if (beam->ar_engine)
    {
        // start engine if not running
        if (!beam->ar_engine->IsRunning())
            beam->ar_engine->StartEngine();

        float kmh_wheel_speed = beam->getWheelSpeed() * 3.6;

        if (abs(mYaw) < 0.5f)
        {
            if (kmh_wheel_speed < maxspeed - 1)
            {
                beam->ar_brake = 0;
                beam->ar_engine->autoSetAcc(acc_power);
            }
            else if (kmh_wheel_speed > maxspeed + 1)
            {
                beam->ar_brake = 1.0f / 3.0f;
                beam->ar_engine->autoSetAcc(0);
            }
            else
            {
                beam->ar_brake = 0;
                beam->ar_engine->autoSetAcc(0);
            }
        }
        else
        {
            if (kmh_wheel_speed < maxspeed - 1)
            {
                beam->ar_brake = 0;
                beam->ar_engine->autoSetAcc(acc_power / 3);
            }
            else if (kmh_wheel_speed > maxspeed + 1)
            {
                beam->ar_brake = 1.0f / 2.0f;
                beam->ar_engine->autoSetAcc(0);
            }
            else
            {
                beam->ar_brake = 0;
                beam->ar_engine->autoSetAcc(0);
            }
        }
    }
}

#endif // USE_ANGELSCRIPT
