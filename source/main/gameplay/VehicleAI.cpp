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

#include "Actor.h"
#include "Console.h"
#include "EngineSim.h"
#include "GameContext.h"
#include "AeroEngine.h"
#include "ScrewProp.h"
#include "GUIManager.h"
#include "GUI_TopMenubar.h"
#include "GUI_SurveyMap.h"

#include "scriptdictionary/scriptdictionary.h"

using namespace Ogre;
using namespace RoR;

VehicleAI::VehicleAI(ActorPtr b) :
    is_waiting(false),
    wait_time(0.0f)
{
    beam = b;
}

VehicleAI::~VehicleAI()
{
}

void VehicleAI::setActive(bool value)
{
    is_enabled = value;
    init_y = beam->getPosition().y;
}

bool VehicleAI::isActive()
{
    return is_enabled;
}

void VehicleAI::addWaypoint(std::string const& id, Ogre::Vector3 const& point)
{
    if (current_waypoint == Vector3::ZERO)
        current_waypoint = point;

    free_waypoints++;
    waypoints.emplace(free_waypoints, point);
    waypoint_ids.emplace(id, free_waypoints);
    waypoint_names.emplace(free_waypoints, id);
}

void VehicleAI::addWaypoints(AngelScript::CScriptDictionary& d)
{
    for (auto item : d)
    {
        Ogre::Vector3 point;
        item.GetValue(&point, item.GetTypeId());
        std::string key(item.GetKey());
        this->addWaypoint(key, point);
    }
}

Ogre::Vector3 VehicleAI::getTranslation(int offset, unsigned int wp)
{
    ROR_ASSERT(wp < App::GetGuiManager()->SurveyMap.ai_waypoints.size());

    Ogre::Vector3 translation = Ogre::Vector3::ZERO;

    if (int(wp) == 0) // First waypoint we have nothing to compare, return translation based on initial vehicle rotation
    {
        if (App::GetGuiManager()->TopMenubar.ai_position_scheme == 0) // Set vehicle behind vehicle
        {
            translation.x += offset * cos(beam->getRotation() - Ogre::Math::HALF_PI);
            translation.z += offset * sin(beam->getRotation() - Ogre::Math::HALF_PI);
        }
        else if (App::GetGuiManager()->TopMenubar.ai_position_scheme == 1) // Set vehicle parallel to vehicle
        {
            translation.x += offset * cos(beam->getRotation());
            translation.z += offset * sin(beam->getRotation());
        }
    }
    else // Return translation based on two waypoints
    {
        if (App::GetGuiManager()->TopMenubar.ai_position_scheme == 0)
        {
            Ogre::Vector3 dir = App::GetGuiManager()->SurveyMap.ai_waypoints[int(wp)-1] - App::GetGuiManager()->SurveyMap.ai_waypoints[int(wp)];
            translation -= offset * dir.normalisedCopy();
        }
        else if (App::GetGuiManager()->TopMenubar.ai_position_scheme == 1)
        {
            Ogre::Vector3 dir = App::GetGuiManager()->SurveyMap.ai_waypoints[int(wp)] - App::GetGuiManager()->SurveyMap.ai_waypoints[int(wp)-1];
            float angle = Ogre::Vector3::UNIT_Z.angleBetween(dir.normalisedCopy()).valueRadians();

            if (dir.x > 0) // Direction on the right fails to produce offset in some angles, invert to have the same offset on both sides
            {
                angle = -Ogre::Vector3::UNIT_Z.angleBetween(dir.normalisedCopy()).valueRadians();
            }

            translation.x -= offset * cos(angle);
            translation.z -= offset * sin(angle);
        }
    }

    return translation;
}

void VehicleAI::addEvent(std::string const& id, int ev)
{
    int waypointid = waypoint_ids[id];
    if (waypointid)
        waypoint_events.emplace(waypointid, ev);
}

void VehicleAI::setValueAtWaypoint(std::string const& id, int value_id, float value)
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
    if (waypoint_names[current_waypoint_id] != "")
    {
        RoR::App::GetConsole()->putMessage(RoR::Console::CONSOLE_MSGTYPE_SCRIPT, RoR::Console::CONSOLE_SYSTEM_NOTICE, "Reached waypoint: " + waypoint_names[current_waypoint_id], "note.png");
    }

    int event = waypoint_events[current_waypoint_id];
    if (event)
    {
        switch (event)
        {
        case AI_LIGHTSTOGGLE:
            beam->toggleHeadlights();
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
        last_waypoint = true;

        if (beam->ar_engine || beam->ar_num_screwprops > 0) // Keep airplanes going
        {
            is_enabled = false;
        }

        if (beam->ar_engine) // Stop truck
        {
            beam->ar_parking_brake = true;
        }
        else if (beam->ar_num_screwprops > 0) // Stop boat
        {
            for (int i = 0; i < beam->ar_num_screwprops; i++)
            {
                beam->ar_screwprops[i]->reset();
            }
        }
    }
    current_waypoint = waypoints[current_waypoint_id];

    if (beam->ar_engine)
    {
        prev_waypoint = waypoints[current_waypoint_id - 1];
        next_waypoint = waypoints[current_waypoint_id + 1];
    }
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
    // Vector3 > Vector2
    mAgentPosition.y = 0;
    current_waypoint.y = 0;
    prev_waypoint.y = 0;
    next_waypoint.y = 0;

    int dist = 5;
    if (beam->ar_num_screwprops > 0)
    {
        dist = 50; // More tolerance for boats
    }
    else if (beam->ar_num_aeroengines > 0)
    {
        dist = 100; // Even more tolerance for airplanes
    }

    // Find the angle (Radian) of the upcoming turn
    Ogre:;Vector3 dir1 = current_waypoint - prev_waypoint;
    Ogre::Vector3 dir2 = next_waypoint - current_waypoint;
    float angle_rad = 0;
    float angle_deg = 0;
    if (App::GetGuiManager()->TopMenubar.ai_mode == 0) // Normal driving mode
    {
        angle_rad = dir1.angleBetween(dir2.normalisedCopy()).valueRadians(); // PI
        angle_deg = dir1.angleBetween(dir2.normalisedCopy()).valueDegrees(); // Degrees 0 - 180
    }

    if (App::GetGuiManager()->TopMenubar.ai_mode == 4) // Chase driving mode
    {
        if (App::GetGameContext()->GetPlayerActor()) // We are in vehicle
        {
            current_waypoint = App::GetGameContext()->GetPlayerActor()->getPosition();
        }
        else // We are in feet
        {
            current_waypoint = App::GetGameContext()->GetPlayerCharacter()->getPosition();
        }
    }
    else
    {
        for (int i = 0; i < static_cast<int>(beam->ar_nodes.size()); i++)
        {
            Ogre::Vector3 pos = beam->getNodePosition(i);
            pos.y = 0;
            if (current_waypoint.distance(pos) < dist)
            {
                updateWaypoint();
                return;
            }
        }
    }

    Vector3 TargetPosition = current_waypoint;
    TargetPosition.y = 0; // Vector3 > Vector2
    Quaternion TargetOrientation = Quaternion::ZERO;

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

    // Steer truck
    if (beam->ar_engine)
    {
        beam->ar_hydro_dir_command = mYaw; // mYaw
    }

    // Steer boat
    if (beam->ar_num_screwprops > 0)
    {
        for (int i = 0; i < beam->ar_num_screwprops; i++)
        {
            beam->ar_screwprops[i]->setRudder(-mYaw);
        }
    }

    // Steer airplane
    if (beam->ar_num_aeroengines > 0)
    {
        if (!last_waypoint) // Not last waypoint yet, follow waypoints
        {
            beam->ar_hydro_dir_command = mYaw; // Wheels
            beam->ar_aileron = mYaw / 2; // Wings

            if (abs(beam->GetCameraRoll().y) > 0.5f) // Oversteer, avoid flip
            {
                beam->ar_aileron = 0;
            }
        }

        if (abs(mYaw) < 0.1f || last_waypoint) // Too little steering or last waypoint, stabilize
        {
            beam->ar_aileron = beam->GetCameraRoll().y;
        }
    }

    if (beam->ar_engine) // Truck
    {
        // Start engine if not running
        if (!beam->ar_engine->isRunning())
            beam->ar_engine->StartEngine();

        beam->ar_parking_brake = false;
        float kmh_wheel_speed = beam->getWheelSpeed() * 3.6;

        if (abs(mYaw) < 0.5f)
        {
            if (kmh_wheel_speed < maxspeed - 1)
            {
                beam->ar_brake = 0;
                beam->ar_engine->autoSetAcc(acc_power - (angle_rad * 0.1f)); // Start easy after turn
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

        if (App::GetGuiManager()->TopMenubar.ai_mode == 0) // Normal driving mode
        {
            Ogre::Vector3 pos = beam->getPosition();
            pos.y = 0;

            // Turn ahead, reduce speed relative to the angle and the current speed
            if (angle_deg > 0 && current_waypoint.distance(pos) < kmh_wheel_speed)
            {
                // Speed limit: 10 - 180 degree angle -> 50 - 5 km/h
                float t = ((angle_deg - 10) / (180 - 10))*1.4f; // Reduce a bit to achive ~20 km/h for a 90 degree angle
                maxspeed = (1 - t)*50 + t*5;
                if (maxspeed > 50) // Limit to 50 km/h
                {
                    maxspeed = 50;
                }
                if (maxspeed > App::GetGuiManager()->TopMenubar.ai_speed) // Respect user defined lower speed
                {
                    maxspeed = App::GetGuiManager()->TopMenubar.ai_speed;
                }
            }
            else // Reset
            {
                maxspeed = App::GetGuiManager()->TopMenubar.ai_speed;
            }

            // Collision avoidance with other actors
            for (ActorPtr actor : App::GetGameContext()->GetActorManager()->GetActors())
            {
                if (actor->ar_driveable == NOT_DRIVEABLE) // Ignore objects that may be actors
                    continue;
                if (actor == beam) // Ignore ourselves
                    continue;

                Ogre::Vector3 a = actor->getPosition() - beam->getPosition();

                if (beam->getDirection().angleBetween(a).valueDegrees() < 30) // Is in front
                {
                    // Actor ahead, slow down - distance relative to current speed so the faster we go the earlier we slow down
                    if (beam->getPosition().distance(actor->getPosition()) < kmh_wheel_speed)
                    {
                        beam->ar_brake = 1;
                        beam->ar_engine->autoSetAcc(0);
                    }

                    for (int i = 0; i < static_cast<int>(beam->ar_nodes.size()); i++)
                    {
                        for (int k = 0; k < static_cast<int>(actor->ar_nodes.size()); k++)
                        {
                            // Too close, stop
                            if (beam->getNodePosition(i).distance(actor->getNodePosition(k)) < 5)
                            {
                                beam->ar_parking_brake = true;
                                beam->toggleHeadlights();
                                break;
                            }
                        }
                    }
                }
            }

            // Collision avoidance with character
            Ogre::Vector3 b = App::GetGameContext()->GetPlayerCharacter()->getPosition() - beam->getPosition();

            if (beam->getDirection().angleBetween(b).valueDegrees() < 30 && // Is in front
                App::GetGameContext()->GetPlayerCharacter()->GetActorCoupling() == nullptr)
            {
                // Character ahead, slow down - distance relative to current speed so the faster we go the earlier we slow down
                if (beam->getPosition().distance(App::GetGameContext()->GetPlayerCharacter()->getPosition()) < kmh_wheel_speed)
                {
                    beam->ar_brake = 1;
                    beam->ar_engine->autoSetAcc(0);
                }

                for (int i = 0; i < static_cast<int>(beam->ar_nodes.size()); i++)
                {
                    // Too close, steer
                    if (beam->getNodePosition(i).distance(App::GetGameContext()->GetPlayerCharacter()->getPosition()) < 5)
                    {
                        beam->ar_hydro_dir_command = -1;
                        beam->toggleHeadlights();
                        break;
                    }
                }
            }
        }
        else if (App::GetGuiManager()->TopMenubar.ai_mode == 4) // Chase driving mode
        {
            if (App::GetGameContext()->GetPlayerActor())
            {
                maxspeed += App::GetGameContext()->GetPlayerActor()->getSpeed(); // Get him!!
            }
            else // Reset
            {
                maxspeed = App::GetGuiManager()->TopMenubar.ai_speed;
            }

            // Collision avoidance with other actors
            for (ActorPtr actor : App::GetGameContext()->GetActorManager()->GetActors())
            {
                if (actor->ar_driveable == NOT_DRIVEABLE) // Ignore objects that may be actors
                    continue;
                if (actor == beam) // Ignore ourselves
                    continue;

                Ogre::Vector3 a = actor->getPosition() - beam->getPosition();

                if (beam->getDirection().angleBetween(a).valueDegrees() < 30) // Is in front
                {
                    // Too close, stop
                    if (beam->getPosition().distance(actor->getPosition()) < 10)
                    {
                        maxspeed = App::GetGuiManager()->TopMenubar.ai_speed;
                        beam->ar_parking_brake = true;
                        beam->toggleHeadlights();
                    }
                }
            }

            // Collision avoidance with character
            Ogre::Vector3 b = App::GetGameContext()->GetPlayerCharacter()->getPosition() - beam->getPosition();

            if (beam->getDirection().angleBetween(b).valueDegrees() < 30 && // Is in front
                App::GetGameContext()->GetPlayerCharacter()->GetActorCoupling() == nullptr)
            {
                // Too close, stop
                if (beam->getPosition().distance(App::GetGameContext()->GetPlayerCharacter()->getPosition()) < 20)
                {
                    beam->ar_parking_brake = true;
                    beam->toggleHeadlights();
                }
            }
        }
    }
    else if (beam->ar_num_aeroengines > 0) // Airplane
    {
        if (beam->getParkingBrake())
        {
            beam->ar_parking_brake = false;
        }

        for (int i = 0; i < beam->ar_num_aeroengines; i++) // Start engines
        {
            if (!beam->ar_aeroengines[i]->getIgnition())
            {
                beam->ar_aeroengines[i]->flipStart();
                beam->ar_aeroengines[i]->setThrottle(1);
            }
        }

        float target_alt = App::GetGuiManager()->TopMenubar.ai_altitude / 3.28083f; // Feet

        if (beam->getPosition().y - init_y < target_alt * 0.8f)
        {
            hold = false;
        }

        if (beam->getPosition().y - init_y < target_alt && !hold) // Reach defined altitude
        {
            beam->ar_elevator = 0.5f - abs(beam->GetCameraDir().y);
            beam->ar_aerial_flap = 4*beam->ar_elevator;

            if (beam->GetCameraDir().y > 0.5f) // Avoid over-elevate flip
            {
                beam->ar_elevator = -0.05f;
            }
        }
        else if (beam->getPosition().y - init_y > target_alt) // We reached defined altitude, hold
        {
            for (int i = 0; i < beam->ar_num_aeroengines; i++)
            {
                beam->ar_aeroengines[i]->setThrottle(0.9f);
            }
            hold = true;
        }

        if (hold)
        {
            beam->ar_elevator = -beam->GetCameraDir().y;

            if (beam->GetCameraDir().y < 0)
            {
                beam->ar_aerial_flap = 1;
            }
            else if (beam->GetCameraDir().y > 0)
            {
                beam->ar_aerial_flap = 0;
            }
        }
    }
    else if (beam->ar_num_screwprops > 0) // Boat
    {
        Vector3 hdir = beam->GetCameraDir();
        float knots = hdir.dotProduct(beam->ar_nodes[beam->ar_main_camera_node_pos].Velocity) * 1.9438f; // 1.943 = m/s in knots/s

        if (abs(mYaw) < 0.5f)
        {
            if (knots < maxspeed - 1)
            {
                for (int i = 0; i < beam->ar_num_screwprops; i++)
                {
                    beam->ar_screwprops[i]->setThrottle(acc_power);
                }
            }
            else if (knots > maxspeed + 1)
            {
                for (int i = 0; i < beam->ar_num_screwprops; i++)
                {
                    beam->ar_screwprops[i]->setThrottle(0);
                }
            }
            else
            {
                for (int i = 0; i < beam->ar_num_screwprops; i++)
                {
                    beam->ar_screwprops[i]->setThrottle(0);
                }
            }
        }
        else
        {
            if (knots < maxspeed - 1)
            {
                for (int i = 0; i < beam->ar_num_screwprops; i++)
                {
                    beam->ar_screwprops[i]->setThrottle(acc_power / 3);
                }
            }
            else if (knots > maxspeed + 1)
            {
                for (int i = 0; i < beam->ar_num_screwprops; i++)
                {
                    beam->ar_screwprops[i]->setThrottle(0);
                }
            }
            else
            {
                for (int i = 0; i < beam->ar_num_screwprops; i++)
                {
                    beam->ar_screwprops[i]->setThrottle(0);
                }
            }
        }
    }
}

#endif // USE_ANGELSCRIPT
