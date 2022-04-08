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

/**
 * @brief Binding of RoR::VehicleAI; Vehicle driving AI using waypoints.
 */
class VehicleAIClass
{
public:
	/**
	 *  Activates/Deactivates the AI.
	 *  @param [in] value Activate or deactivation the AI
	 */
	void SetActive(bool value);
	/**
	 *  Adds one waypoint.
	 *
	 *  @param [in] id The waypoint ID.
	 *  @param [in] point The coordinates of the waypoint.
	 */
	void AddWaypoint(Ogre::String& id, Ogre::Vector3 point);
	/**
	 *  Adds a dictionary with waypoints.
	 *  @param [in] d Dictionary with waypoints
	 */
	void AddWaypoints(AngelScript::CScriptDictionary d);
	/**
	 *  Adds a event
	 *
	 *  @param [in] id The waypoint ID.
	 *  @param [in] ev The ID of the event.
	 *
	 *  @see Ai_events
	 */
	void AddEvent(Ogre::String id, int ev);
	/**
	 *  Sets a value at a waypoint.
	 *
	 *  @param [in] id The waypoint ID.
	 *  @param [in] value_id The ID of the value that will be set.
	 *  @param [in] value The value itself.
	 *
	 *  @see Ai_values
	 */
	void SetValueAtWaypoint(Ogre::String id, int value_id, float value);

}

/// @}    //addtogroup Script2Game
/// @}    //addtogroup ScriptSideAPIs

} //namespace ScriptSideAPIs
