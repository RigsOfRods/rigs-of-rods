/*
This source file is part of Rigs of Rods

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
// created on 25th May 2011.

/**
 * This is an alias for game.log(string message).
 * @see game.log
 */
void log(const string message);

/**
 * This is an alias for game.log(string message).
 * @see game.log
 */
void print(const string message);

/**
 * All the events that can be used by the script.
 * @see game::registerForEvent
 * @see The scriptEvents enum in the Rigs of Rods source code.
 */
 enum scriptEvents
 {
 	SE_TRUCK_ENTER                     = 0x00000004, //!< triggered when switching from person mode to truck mode, the argument refers to the truck number
 	SE_TRUCK_EXIT                      = 0x00000008, //!< triggered when switching from truck mode to person mode, the argument refers to the truck number

 	SE_TRUCK_ENGINE_DIED               = 0x00000010, //!< triggered when the trucks engine dies (from underrev, water, etc), the argument refers to the truck number
 	SE_TRUCK_ENGINE_FIRE               = 0x00000020, //!< triggered when the planes engines start to get on fire, the argument refers to the truck number
 	SE_TRUCK_TOUCHED_WATER             = 0x00000040, //!< triggered when any part of the truck touches water, the argument refers to the truck number
 	SE_TRUCK_LIGHT_TOGGLE              = 0x00000400, //!< triggered when the main light is toggled, the argument refers to the truck number
 	SE_TRUCK_TIE_TOGGLE                = 0x00001000, //!< triggered when the user toggles ties, the argument refers to the truck number
 	SE_TRUCK_PARKINGBREAK_TOGGLE       = 0x00002000, //!< triggered when the user toggles the parking break, the argument refers to the truck number
 	SE_TRUCK_BEACONS_TOGGLE            = 0x00004000, //!< triggered when the user toggles beacons, the argument refers to the truck number
 	SE_TRUCK_CPARTICLES_TOGGLE         = 0x00008000, //!< triggered when the user toggles custom particles, the argument refers to the truck number

 	SE_GENERIC_NEW_TRUCK               = 0x00020000, //!< triggered when the user spawns a new truck, the argument refers to the truck number
 	SE_GENERIC_DELETED_TRUCK           = 0x00040000, //!< triggered when the user deletes a truck, the argument refers to the truck number

 	SE_TRUCK_TRACTIONCONTROL_TOGGLE    = 0x00200000, //!< triggered when the user toggles the tractioncontrol system, the argument refers to the truck number
 	SE_TRUCK_ANTILOCKBRAKE_TOGGLE      = 0x00400000, //!< triggered when the user toggles the antilockbrake, the argument refers to the truck number

 	SE_ANGELSCRIPT_MANIPULATIONS       = 0x00800000, //!< triggered when the user tries to dynamically use the scripting capabilities (prevent cheating)

 	SE_GENERIC_MESSAGEBOX_CLICK        = 0x01000000, //!< triggered when the user clicks on a message box button, the argument refers to the button pressed

 	SE_ALL_EVENTS                      = 0xffffffff,

 };


/**
 * Different truck states registered with the script.
 */
enum truckStates {
	TS_ACTIVATED,      //!< leading truck
	TS_DESACTIVATED,   //!< not leading but active
	TS_MAYSLEEP,       //!< active but wanting to sleep
	TS_GOSLEEP,        //!< active but ordered to sleep ASAP (synchronously)
	TS_SLEEPING,       //!< not active, sleeping
	TS_NETWORKED,      //!< not calculated, gets remote data
	TS_RECYCLE,        //!< waiting for reusage
	TS_DELETED,        //!< special used when truck pointer is 0
};
