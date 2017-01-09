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

#pragma once

#ifdef USE_ANGELSCRIPT

#include "Application.h"
#include "VehicleAI.h" // enums Ai_events, Ai_values
#include "scriptdictionary/scriptdictionary.h"

namespace RoR {

class RacingAI: public RefCountingObject<RacingAI>
{
public:
    RacingAI(ActorPtr b);
    ~RacingAI();
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
    int task_after_waiting;//!< The task to do after it has waited.

    //cosmic vole added auto reset code for stuck vehicles October 9 2016
    bool is_stuck;
    float stuck_time;
    Ogre::Vector3 stuck_position;
    float stuck_cancel_distance;
    float stuck_reset_delay;
    //cosmic vole added optional driver models for active AI cars
    Character* character;

    float maxspeed;// = 50;//!<(KM/H) The max speed the AI is allowed to drive.
    ActorPtr beam;//!< The verhicle the AI is driving.
    bool is_enabled;// = false;//!< True if the AI is driving.
    Ogre::Vector3 current_waypoint;//!< The coordinates of the waypoint that the AI is driving to.
    int current_waypoint_id;// = 0;//!< The curent waypoint ID.
    std::map<int, Ogre::Vector3> waypoints;//!< Map with all waypoints.
    std::map<Ogre::String, int> waypoint_ids;//!< Map with all waypoint IDs.
    std::map<int, Ogre::String> waypoint_names;//!< Map with all waypoint names.
    std::map<int, int> waypoint_events;//!< Map with all waypoint events.
    std::map<int, float> waypoint_speed;//!< Map with all waypoint speeds.
    std::map<int, float> waypoint_power;//!< Map with all waypoint engine power.
    std::map<int, float> waypoint_wait_time;//!< Map with all waypoint wait times.
    int free_waypoints;// = 0;//!< The amount of waypoints.
    float acc_power;// = 0.8;//!< The engine power.
};

} // namespace RoR

#endif // USE_ANGELSCRIPT
