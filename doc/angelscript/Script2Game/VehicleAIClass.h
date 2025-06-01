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

/**
	@file
	@brief  Simple waypoint AI
	@author AnotherFoxGuy
	@date   03/2016
*/

namespace Script2Game {

/** \addtogroup ScriptSideAPIs
 *  @{
 */    

/** \addtogroup Script2Game
 *  @{
 */    

/**
 * @brief Binding of RoR::VehicleAI; Vehicle driving AI using waypoints.
 */
class VehicleAIClass
{
    // PLEASE maintain the same order as in 'bindings/VehicleAiAngelscript.cpp' and 'VehicleAI.h'
    
public:
	/**
	 *  Activates/Deactivates the AI.
	 *  @param value Activate or deactivation the AI
	 */
	void setActive(bool value);
    
    /**
     *  Returns the status of the AI.
     *  @return True if the AI is driving
     */
    bool isActive();    
    
	/**
	 *  Adds one waypoint.
	 *
	 *  @param id The waypoint ID.
	 *  @param point The coordinates of the waypoint.
	 */
	void addWaypoint(string const&in id, vector3 const&in pos);
    
	/**
	 *  Adds a dictionary with waypoints.
	 *  @param d Dictionary with waypoints (string ID -> vector3 pos)
	 */
	void addWaypoints(dictionary &in d);
    
	/**
	 *  Adds a event
	 *
	 *  @param id The waypoint ID.
	 *  @param ev The ID of the event.
	 *
	 *  @see Ai_events
	 */
	void addEvent(string const&in id, int ev);
    
	/**
	 *  Sets a value at a waypoint.
	 *
	 *  @param id The waypoint ID.
	 *  @param value_id The ID of the value that will be set.
	 *  @param value The value itself.
	 *
	 *  @see Ai_values
	 */
	void setValueAtWaypoint(string const&in id, int value_id, float value);
    
    /**
     *  Gets offset translation based on vehicle rotation and waypoints
     *
     *  @param offset The offset.
     *  @param wp The waypoint index.
     */
    vector3 getTranslation(int offset, unsigned int wp);

}

/// @}    //addtogroup Script2Game
/// @}    //addtogroup ScriptSideAPIs

} //namespace ScriptSideAPIs
