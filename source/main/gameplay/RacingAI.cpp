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

/// @file   
/// @brief  Copy of VehicleAI at revision fecbb3ae09a7bad2b9bd88ed33e31689c49b63b2 with changes for AI racing.
/// @author AnotherFoxGuy & cosmic vole

#ifdef USE_ANGELSCRIPT

#include "RacingAI.h"

#include "Application.h"
#include "VehicleAI.h"
#include "Actor.h"
#include "ActorManager.h"
#include "Console.h"
#include "Engine.h"

//cosmic vole added AI driver model November 21 2016
#include "CharacterFactory.h"

#include "scriptdictionary/scriptdictionary.h"

using namespace Ogre;
using namespace RoR;

RacingAI::RacingAI(ActorPtr b)
{
    //Initialization moved into constructor to avoid Visual Studio error 'only static const integral data members can be initialized within a class'
    maxspeed = 50;//!<(KM/H) The max speed the AI is allowed to drive.
    is_enabled = false;//!< True if the AI is driving.
    current_waypoint_id = 0;//!< The curent waypoint ID.
    free_waypoints = 0;//!< The amount of waypoints.
    acc_power = 0.8;//!< The engine power.

    //cosmic vole added auto reset code for stuck vehicles October 9 2016
    stuck_time = 0.0;
    stuck_position = b->getPosition();
    stuck_cancel_distance = 2.0;
    stuck_reset_delay = 15.0;//Time in seconds until a stuck vehicle resets
    //cosmic vole added AI driver model November 21 2016
    character = nullptr;

    beam = b;
}

RacingAI::~RacingAI()
{
}

void RacingAI::SetActive(bool value)
{
    is_enabled = value;
    //cosmic vole added AI Character driver model November 21 2016
    if (value && character == nullptr)
    {
        int aiColour = App::GetNetwork()->getRandomColourNum();//Ogre::ColourValue(frand(), frand(), frand(), 1.0f);
        character = App::GetGameContext()->GetCharacterFactory()->createAIInstance(beam->ar_instance_id, (int)aiColour);
        character->SetActorCoupling(true, beam);
    }
    else if (!value && character != nullptr)
    {
        App::GetGameContext()->GetCharacterFactory()->removeAIInstance(beam->ar_instance_id);
    }
}

bool RacingAI::IsActive()
{
    return is_enabled;
}

void RacingAI::AddWaypoint(String& id, Vector3& point)
{
    if (current_waypoint == Vector3::ZERO)
        current_waypoint = point;

    free_waypoints++;
    waypoints.emplace(free_waypoints, point);
    waypoint_ids.emplace(id, free_waypoints);
    waypoint_names.emplace(free_waypoints, id);
}

void RacingAI::AddWaypoints(AngelScript::CScriptDictionary& d)
{
    for (auto dict : d)
    {
        String id = dict.GetKey();
        Vector3* point = (Vector3 *)dict.GetAddressOfValue();
        AddWaypoint(id, *(point));
    }
}

void RacingAI::AddEvent(String& id, int& ev)
{
    int waypointid = waypoint_ids[id];
    if (waypointid)
        waypoint_events.emplace(waypointid, ev);
}

void RacingAI::SetValueAtWaypoint(String& id, int& value_id, float& value)
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

void RacingAI::updateWaypoint()
{
    RoR::App::GetConsole()->putMessage(RoR::Console::CONSOLE_MSGTYPE_SCRIPT, RoR::Console::CONSOLE_SYSTEM_NOTICE, "Reached waypoint: " + waypoint_names[current_waypoint_id], "note.png");

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
        is_enabled = false;
        beam->parkingbrakeToggle();
    }
    current_waypoint = waypoints[current_waypoint_id];
}

void RacingAI::update(float dt, int doUpdate)
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
    //Keep the absolute position in a separate variable as the other one will be normalized. cosmic vole October 10 2016.
    Vector3 mAgentAbsPosition = mAgentPosition;
    float curDistance;

    if (current_waypoint.distance(mAgentPosition) < 5)
    {
        updateWaypoint();
        //Bug fix for auto reset. If the waypoints are very close together, the reset can fire off if we don't keep checking this here. cosmic vole October 10 2016
        if (is_enabled && stuck_reset_delay > 0.0f)
        {
            curDistance = mAgentAbsPosition.distance(stuck_position);
            if (curDistance >= stuck_cancel_distance)
            {
                is_stuck = false;
                stuck_time = 0.0f;
                stuck_position = mAgentPosition;
            }
        }
        return;
    }

    Vector3 TargetPosition = current_waypoint;
    TargetPosition.y = 0; //Vector3 > Vector2
    Quaternion TargetOrientation = Quaternion::ZERO;

    mAgentPosition.y = 0; //Vector3 > Vector2
    float agentRotation = beam->getRotation();//OLD NAME: beam->getHeadingDirectionAngle(); ~ ohlidalp, 01/2026
    Quaternion mAgentOrientation = Quaternion(Radian(agentRotation), Vector3::NEGATIVE_UNIT_Y);
    mAgentOrientation.normalise();

    Vector3 mVectorToTarget = TargetPosition - mAgentPosition; // A-B = B->A
    mAgentPosition.normalise();

    Vector3 mAgentHeading = mAgentOrientation * mAgentPosition;
    Vector3 mTargetHeading = TargetOrientation * TargetPosition;
    mAgentHeading.normalise();
    mTargetHeading.normalise();

    // Compute new torque scalar (-1.0 to 1.0) based on heading vector to target.
    Vector3 mSteeringForce = mAgentOrientation.Inverse() * mVectorToTarget;

    //cosmic vole added auto reset code for stuck vehicles October 9 2016
    if (is_enabled && stuck_reset_delay > 0.0f)
    {
        float curDistance = mAgentAbsPosition.distance(stuck_position);
        if (curDistance >= stuck_cancel_distance)
        {
            is_stuck = false;
            stuck_time = 0.0f;
            stuck_position = mAgentAbsPosition;
        }
        else
        {
            is_stuck = true;
            stuck_time += dt;
            if (stuck_time >= stuck_reset_delay)
            {
                RoR::App::GetConsole()->putMessage(RoR::Console::CONSOLE_MSGTYPE_SCRIPT, RoR::Console::CONSOLE_SYSTEM_NOTICE, 
                    "Resetting vehicle: " + TOSTRING(beam->ar_instance_id) + ". Got stuck at (" + TOSTRING(stuck_position.x) + ", " + TOSTRING(stuck_position.y) + ", " + TOSTRING(stuck_position.z) + "); cur. pos (" + 
                    TOSTRING(mAgentAbsPosition.x) + ", " + TOSTRING(mAgentAbsPosition.y) + ", " + TOSTRING(mAgentAbsPosition.z) + "); for time " + TOSTRING(stuck_time) + ". Dist. moved: " + TOSTRING(curDistance) + ".", "note.png");
                //Reset the vehicle's position to the last waypoint - TODO we may or may not want to repair the vehicle
                Vector3 resetPos;
                if (current_waypoint_id <= free_waypoints)
                {
                    //if (current_waypoint_id > 0)
                    //{
                    //    resetPos = waypoints[current_waypoint_id-1];
                    //}
                    //else
                    //{
                    resetPos = beam->ar_initial_node_positions[0];//waypoints[current_waypoint_id];
                    if (resetPos.squaredDistance(beam->ar_nodes[0].AbsPosition) < 9.0)
                    {
                        //It's already on the spawn point so don't reset
                        resetPos = Vector3::ZERO;
                    }
                    //}

                    //When the vehicle's at the start, the 0th waypoint is Vector3::ZERO. TODO we should probably detect this better.
                    if (resetPos != Vector3::ZERO)
                    {
                        //We will aim to reset to the last waypoint visited, but
                        //before we move to the waypoint, find a clear space on the track from that point back
                        float maxRelocateDistanceSq = 100.0 * 100.0;//If there's no clear track within 100 metres, this truck will just sit and wait
                        int i = current_waypoint_id-1;
                        bool found_space = false;
                        for (int j=0; !found_space && j<free_waypoints; j++)
                        {
                            //0th waypoint is not a point on the track, we need to move round to the very last waypoint
                            if (i<=0)
                            {
                                i = free_waypoints;
                            }
                            int iprev = i-1;
                            if (i == 1)
                            {
                                iprev = free_waypoints;
                            }
                            Vector3 thisWaypoint = waypoints[i];
                            if (j > 0 && thisWaypoint.squaredDistance(mAgentAbsPosition) > maxRelocateDistanceSq)
                            {
                                //Too far away. Give up and wait for the track to clear.
                                break;
                            }
                            ActorPtrVec trucks = App::GetGameContext()->GetActorManager()->GetActors();
                            int numTrucks = (int)trucks.size();

                            do
                            {
                                //Assume this new position is OK until we've checked all the trucks
                                found_space = true;
                                for (int t=0; t<numTrucks; t++)
                                {
                                    if (t == beam->ar_instance_id)
                                    {
                                        continue;
                                    }
                                    //TODO something's not working quite right with this as a car respawned right inside the player's car today! cosmic vole October 11 2016
                                    if (thisWaypoint.squaredDistance(trucks[t]->getPosition()) < 9.0)
                                    {
                                        //That truck's too close
                                        found_space = false;
                                        break;
                                    }
                                }
                                if (!found_space)
                                {
                                    Vector3 prevWaypoint = waypoints[iprev];
                                    //If we've reversed past the previous waypoint, we need to continue our outer loop to start looking at the other waypoints (busy track!)
                                    if (waypoints[i].squaredDistance(thisWaypoint) - waypoints[i].squaredDistance(prevWaypoint) > 0.1)
                                    {
                                        break;
                                    }
                                    Vector3 headBack = prevWaypoint - waypoints[i];//thisWaypoint;
                                    if (headBack.squaredLength() < 0.0001f)
                                    {
                                        //The two waypoints coincide. We need to keep looking at the other ones
                                        break;
                                    }
                                    headBack = headBack.normalisedCopy();//Should be 1 metre length which is good for our purposes
                                    thisWaypoint += headBack;
                                    if (j > 0 && thisWaypoint.squaredDistance(mAgentAbsPosition) > maxRelocateDistanceSq)
                                    {
                                        //Too far away. Give up and wait for the track to clear.
                                        break;
                                    }
                                }
                            }
                            while (!found_space);

                            if (found_space)
                            {
                                resetPos = thisWaypoint;
                                //Don't forget to update the current waypoint for the AI!
                                current_waypoint_id = i+1;
                                if (current_waypoint_id > free_waypoints)
                                {
                                    //BUG / TODO review this. It will create an infinite loop which is not the normal behaviour!
                                    current_waypoint_id = 1;
                                }
                                //As we've changed the waypoint, at the very least we need to recalc the Target Heading so we know how to rotate the car into it's new position
                                TargetPosition = current_waypoint = waypoints[current_waypoint_id];
                                TargetPosition.y = 0; //Vector3 > Vector2
                                Quaternion TargetOrientation = Quaternion::ZERO;
                                //mAgentPosition.y = 0; //Vector3 > Vector2
                                //float agentRotation = beam->getHeadingDirectionAngle();
                                //Quaternion mAgentOrientation = Quaternion(Radian(agentRotation), Vector3::NEGATIVE_UNIT_Y);
                                //mAgentOrientation.normalise();
                                //Vector3 mVectorToTarget = TargetPosition - mAgentPosition; // A-B = B->A
                                //mAgentPosition.normalise();
                                //Vector3 mAgentHeading = mAgentOrientation * mAgentPosition;
                                mTargetHeading = TargetOrientation * TargetPosition;
                                //mAgentHeading.normalise();
                                mTargetHeading.normalise();
                                //We've finally found our respawn point. Good to go!
                                break;
                            }
                            i--;
                        }

                        if (found_space && resetPos != Vector3::ZERO)
                        {
                            //Vector3 mSpawnRotationXZ = Vector3(0.0f, 0.0f, 1.0f);
                            Vector3 beamDir = beam->getDirection();
                            beamDir.normalise();
                            Degree pitchAngle = Radian(asin(beamDir.dotProduct(Vector3::UNIT_Y)));
                            //360 degree Pitch using atan2 and pythag: http://stackoverflow.com/questions/3755059/3d-accelerometer-calculate-the-orientation
                            float beamDirY = beamDir.dotProduct(Vector3::UNIT_Y);
                            float beamDirZ = beamDir.dotProduct(Vector3::UNIT_Z);
                            //Yaw: return atan2(idir.dotProduct(Vector3::UNIT_X), (idir).dotProduct(-Vector3::UNIT_Z));
                            Degree pitchAngle2 = Radian(atan2(beamDir.dotProduct(-Vector3::UNIT_X), sqrt(beamDirY * beamDirY + beamDirZ * beamDirZ)));
                            Degree rollAngle = Radian(atan2(beamDirY, beamDirZ));
                            Vector3 mTargetHeadingXZ = mTargetHeading;
                            mAgentHeading.y = 0.0f;
                            mTargetHeadingXZ.y = 0.0f;

                            //agentRotation gives the vehicle's current rotation - shouldn't be relevant when we do resetAngle()
                            //we can also get the spawnRotation but that is already accounted for in resetAngle()
                            Degree targetRotation = Radian(atan2(mTargetHeading.dotProduct(Vector3::UNIT_X), mTargetHeading.dotProduct(-Vector3::UNIT_Z)));






                            Vector3 rollv = beam->ar_nodes[beam->ar_camera_node_pos[0]].RelPosition - beam->ar_nodes[beam->ar_camera_node_roll[0]].RelPosition;
                            rollv.normalise();
                            rollAngle=Radian(asin(rollv.dotProduct(Vector3::UNIT_Y)));

                            //Find the up vector to tell if upside down or not
                            Vector3 cam_pos  = beam->ar_nodes[beam->ar_camera_node_pos[0]].RelPosition;
                            Vector3 cam_roll = beam->ar_nodes[beam->ar_camera_node_roll[0]].RelPosition;
                            Vector3 cam_dir  = beam->ar_nodes[beam->ar_camera_node_dir[0]].RelPosition;

                            //Vector3 rollv = (cam_pos - cam_roll).normalisedCopy();
                            Vector3 dirv  = (cam_pos - cam_dir ).normalisedCopy();
                            Vector3 upv   = dirv.crossProduct(-rollv);

                            //Quaternions to reorientate the vehicle (experimental based on http://www.ogre3d.org/tikiwiki/Quaternion+and+Rotation+Primer#Q_How_can_I_make_my_objects_stand_upright_after_a_bunch_of_rotations_)
                            //Also based on https://github.com/opengl-tutorials/ogl/blob/master/common/quaternion_utils.cpp
                            //Vector3 localY = mNode->getOrientation() * Vector3::UNIT_Y;
                            // Get rotation to original facing                                          
                            //Vector3 currentFacing = mNode->getOrientation() * mInitFacing;                          
                            Quaternion quatRestoreFwd = dirv.getRotationTo(TargetPosition - resetPos);//UNIT_Z worked great: Vector3::UNIT_Z);//currentFacing.getRotationTo(mInitFacing);
                            // Because of the 1rst rotation, the up is probably completely screwed up.
                            // Find the rotation between the "up" of the rotated object, and the desired up
                            Vector3 newUp = quatRestoreFwd * upv;//Vector3::UNIT_Y;
                            Quaternion quatRestoreUp = newUp.getRotationTo(Vector3::UNIT_Y);//upv.getRotationTo(Vector3::UNIT_Y);
                            Quaternion quatRestore = quatRestoreUp * quatRestoreFwd;//remember, in reverse order.







                            RoR::App::GetConsole()->putMessage(RoR::Console::CONSOLE_MSGTYPE_SCRIPT, RoR::Console::CONSOLE_SYSTEM_NOTICE, 
                                "Vehicle: " + TOSTRING(beam->ar_instance_id) + "  Yaw: " + TOSTRING(Radian(agentRotation).valueDegrees()) + " Pitch: " + TOSTRING(pitchAngle.valueDegrees()) + " Pitch (alt calc): " + TOSTRING(pitchAngle2.valueDegrees()) + " Roll: " + TOSTRING(rollAngle.valueDegrees()) + " Target Yaw: " + TOSTRING(targetRotation.valueDegrees()) + " cur. pos (" + 
                                TOSTRING(mAgentAbsPosition.x) + ", " + TOSTRING(mAgentAbsPosition.y) + ", " + TOSTRING(mAgentAbsPosition.z) + "); for time " + TOSTRING(stuck_time) + ". Dist. moved: " + TOSTRING(curDistance) + ".", "note.png");


                            //resetAngle() WON'T WORK if the truck is upside down or on its side as it uses a matrix made from Euler angles and only deals with yaw!
                            //Doesn't seem to work beam->resetAngle(targetRotation);// + mAgentHeading.angleBetween(mTargetHeadingXZ).valueDegrees());//mSteeringForce.x);//mTargetHeading.x);//beam->getRotation());
                            //beam->updateFlexbodiesFinal();
                            //I think the reason this isn't quite right is we need to know the heading of the target FROM resetPos
                            //beam->displace(Vector3::ZERO, mAgentHeading.angleBetween(mTargetHeadingXZ).valueDegrees());//targetRotation);


                            //TODO Do this instead (this will be slow so we may need to look at deferring it to a later time slice??):
                            node_t* nodes = beam->ar_nodes;
                            int nodeCount = beam->ar_num_nodes;
                            //This code is taken from LoadTruck(), but we've got problems here. We can't easily base the rotations on each node's initial position because the nodes may be deformed.
                            //I am not sure there's an obvious way to tell whether the whole vehicle is rotated (e.g.) upside down or just some of the nodes are rotated relative to the others!
                            //We need the full quaternion direction the vehicle's nodes are currently pointing and to first reverse that rotation, then to apply the new one


                            //Something like this: look at calcAnimators() from line 2389 as well to work out if vehicle upside down etc.
                            //Also look at RORFrameListener.cpp, inside advanced repair, how it steers and moves the nodes of the whole truck (based on input events) using beam->displace(): curr_truck->displace(translation * scale, rotation * scale); line 780
                            //Nice tutorial says how to get character upright after a bunch of rotations, using quaternions http://www.ogre3d.org/tikiwiki/Quaternion+and+Rotation+Primer#Q_How_can_I_make_my_objects_stand_upright_after_a_bunch_of_rotations_
                            //Good quaternion cheat sheet http://www.opengl-tutorial.org/intermediate-tutorials/tutorial-17-quaternions/

                            //if (cameranodepos[0] >= 0 && cameranodepos[0] < MAX_NODES)
                            //{
                            //    // pitch
                            //    Vector3 dir = nodes[cameranodepos[0]].RelPosition - nodes[cameranodedir[0]].RelPosition;
                            //    dir.normalise();
                            //    //TODO check this handles 360 degree rotation - look into using asin2 with y and z offsets if necessary
                            //    float angle = asin(dir.dotProduct(Vector3::UNIT_Y));
                            //    if (angle < -1) angle = -1;
                            //    if (angle > 1) angle = 1;

                            //    pitch = Radian(angle).valueDegrees();

                            //How pitch angle is calced for anti rollback:
                            //Vector3 dirDiff = curr_truck->getDirection();
                            //Degree pitchAngle = Radian(asin(dirDiff.dotProduct(Vector3::UNIT_Y)));

                            //    //TODO not sure how to calc roll yet but we may not need it as 2 axes may be enough to right the vehicle
                            //}

                            //Isn't this susceptible to gimbal lock?
                            //Quaternion rot = Quaternion(Degree(rz), Vector3::UNIT_Z) * Quaternion(Degree(ry), Vector3::UNIT_Y) * Quaternion(Degree(rx), Vector3::UNIT_X);

                            //CHECK OUT THIS CameraBehaviorVehicle::mousePressed Seems to do exactly what we need! Line 96 on...

                            // Set origin of rotation to camera node
                            Vector3 origin = nodes[0].AbsPosition;

                            if (beam->ar_camera_node_pos[0] >= 0 && beam->ar_camera_node_pos[0] != NODENUM_INVALID)
                            {
                                origin = nodes[beam->ar_camera_node_pos[0]].AbsPosition;
                            }
                            for (int i=0; i<nodeCount; i++)
                            {
                                nodes[i].AbsPosition = quatRestore * nodes[i].AbsPosition;//spawn_position + spawn_rotation * (nodes[i].AbsPosition - spawn_position);
                                nodes[i].RelPosition = nodes[i].AbsPosition - origin;
                            }

                            beam->resetPosition(resetPos.x, resetPos.z, false, resetPos.y);//false, 0.0f);//resetPos.y);
                            if (!beam->ar_engine->isRunning())
                            {
                                beam->ar_engine->startEngine();
                            }
                            if (beam->ar_parking_brake)
                            {
                                beam->parkingbrakeToggle();
                            }

                        }
                    }

                    mAgentPosition = beam->getPosition();
                    is_stuck = false;
                    stuck_time = 0.0f;
                    stuck_position = mAgentPosition;
                    return;
                }

            }
        }
    }

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
        if (!beam->ar_engine->isRunning())
            beam->ar_engine->startEngine();

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
                beam->ar_brake = beam->ar_brake_force / 3;
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
                beam->ar_brake = beam->ar_brake_force / 2;
                beam->ar_engine->autoSetAcc(0);
            }
            else
            {
                beam->ar_brake = 0;
                beam->ar_engine->autoSetAcc(0);
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
