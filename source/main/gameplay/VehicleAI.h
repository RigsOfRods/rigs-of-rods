﻿/*
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

/// @file   VehicleAI.h
/// @brief  Simple waypoint AI
/// @author AnotherFoxGuy
/// @date   03/2016

#pragma once

#ifdef USE_ANGELSCRIPT

#include "RoRPrerequisites.h"
#include "scriptdictionary/scriptdictionary.h"

/**
*	Enum with AI events
*/
enum Ai_events
{
    AI_HORN,
    AI_LIGHTSTOGGLE,
    AI_WAIT_SECONDS,
    AI_BEACONSTOGGLE
};

/**
*	Enum with AI values that can be set.
*/
enum Ai_values
{
    AI_SPEED,
    AI_POWER
};

class VehicleAI : public ZeroedMemoryAllocator
{
public:
    VehicleAI(Actor* b);
    ~VehicleAI();
    /**
     *  Activates/Deactivates the AI.
     *  @param [in] value Activate or deactivation the AI
     */
    void SetActive(bool value);
    /**
     *  Returns the status of the AI.
     *  @return True if the AI is driving
     */
    bool IsActive();

#ifdef USE_ANGELSCRIPT
    // we have to add this to be able to use the class as reference inside scripts
    void addRef()
    {
    };

    void release()
    {
    };
#endif
    /**
     *  Updates the AI.
     */
    void update(float dt, int doUpdate);
    /**
     *  Adds one waypoint.
     *
     *  @param [in] id The waypoint ID.
     *  @param [in] point The coordinates of the waypoint.
     */
    void AddWaypoint(Ogre::String& id, Ogre::Vector3& point);
    /**
     *  Adds a dictionary with waypoints.
     *  @param [in] d Dictionary with waypoints
     */
    void AddWaypoints(AngelScript::CScriptDictionary& d);
    /**
     *  Adds a event
     *
     *  @param [in] id The waypoint ID.
     *  @param [in] ev The ID of the event.
     *
     *  @see Ai_events
     */
    void AddEvent(Ogre::String& id, int& ev);
    /**
     *  Sets a value at a waypoint.
     *
     *  @param [in] id The waypoint ID.
     *  @param [in] value_id The ID of the value that will be set.
     *  @param [in] value The value itself.
     *
     *  @see Ai_values
     */
    void SetValueAtWaypoint(Ogre::String& id, int& value_id, float& value);

private:
    /**
     *   Updates the AI waypoint.
     */
    void updateWaypoint();

    bool is_waiting;//!< 
    float wait_time;//!<(seconds) The amount of time the AI has to wait on this waypoint.

    float maxspeed = 50;//!<(KM/H) The max speed the AI is allowed to drive.
    Actor* beam;//!< The verhicle the AI is driving.
    bool is_enabled = false;//!< True if the AI is driving.
    Ogre::Vector3 current_waypoint;//!< The coordinates of the waypoint that the AI is driving to.
    int current_waypoint_id = 0;//!< The curent waypoint ID.
    std::map<int, Ogre::Vector3> waypoints;//!< Map with all waypoints.
    std::map<Ogre::String, int> waypoint_ids;//!< Map with all waypoint IDs.
    std::map<int, Ogre::String> waypoint_names;//!< Map with all waypoint names.
    std::map<int, int> waypoint_events;//!< Map with all waypoint events.
    std::map<int, float> waypoint_speed;//!< Map with all waypoint speeds.
    std::map<int, float> waypoint_power;//!< Map with all waypoint engine power.
    std::map<int, float> waypoint_wait_time;//!< Map with all waypoint wait times.
    int free_waypoints = 0;//!< The amount of waypoints.
    float acc_power = 0.8;//!< The engine power.
};

#endif // USE_ANGELSCRIPT
