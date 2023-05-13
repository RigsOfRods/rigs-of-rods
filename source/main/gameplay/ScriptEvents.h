/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer

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

#pragma once

#include "BitFlags.h"

#include <string>

namespace RoR {

/// This enum describes what events are existing. The script can register to receive events.
enum scriptEvents
{
    SE_EVENTBOX_ENTER                  = BITMASK(1), //!< Actor or person entered an eventbox; arguments: #1 type, #2 actorID (actor only), #3 node ID (actor only), #4 unused, #5 object instance name #6 eventbox name, #7 unused #8 unused.
    SE_EVENTBOX_EXIT                   = BITMASK(2), //!< Actor or person left an eventbox; arguments: #1 type, #2 actorID (actor only), #3 unused, #4 unused, #5 object instance name #6 eventbox name, #7 unused #8 unused.

    SE_TRUCK_ENTER                     = BITMASK(3), //!< triggered when switching from person mode to vehicle mode, the argument refers to the actor ID of the vehicle
    SE_TRUCK_EXIT                      = BITMASK(4), //!< triggered when switching from vehicle mode to person mode, the argument refers to the actor ID of the vehicle

    SE_TRUCK_ENGINE_DIED               = BITMASK(5), //!< triggered when the vehicle's engine dies (from underrev, water, etc), the argument refers to the actor ID of the vehicle
    SE_TRUCK_ENGINE_FIRE               = BITMASK(6), //!< triggered when the planes engines start to get on fire, the argument refers to the actor ID of the vehicle
    SE_TRUCK_TOUCHED_WATER             = BITMASK(7), //!< triggered when any part of the actor touches water, the argument refers to the actor ID
    SE_TRUCK_LIGHT_TOGGLE              = BITMASK(8), //!< triggered when the main light is toggled, the argument refers to the actor ID
    SE_TRUCK_TIE_TOGGLE                = BITMASK(9), //!< triggered when the user toggles ties, the argument refers to the actor ID
    SE_TRUCK_PARKINGBRAKE_TOGGLE       = BITMASK(10), //!< triggered when the user toggles the parking brake, the argument refers to the actor ID
    SE_TRUCK_BEACONS_TOGGLE            = BITMASK(11), //!< triggered when the user toggles beacons, the argument refers to the actor ID
    SE_TRUCK_CPARTICLES_TOGGLE         = BITMASK(12), //!< triggered when the user toggles custom particles, the argument refers to the actor ID

    SE_GENERIC_NEW_TRUCK               = BITMASK(13), //!< triggered when the user spawns a new actor, the argument refers to the actor ID
    SE_GENERIC_DELETED_TRUCK           = BITMASK(14), //!< triggered when the user deletes an actor, the argument refers to the actor ID

    SE_TRUCK_RESET                     = BITMASK(15), //!< triggered when the user resets the truck, the argument refers to the actor ID of the vehicle
    SE_TRUCK_TELEPORT                  = BITMASK(16), //!< triggered when the user teleports the truck, the argument refers to the actor ID of the vehicle
    SE_TRUCK_MOUSE_GRAB                = BITMASK(17), //!< triggered when the user uses the mouse to interact with the actor, the argument refers to the actor ID

    SE_ANGELSCRIPT_MANIPULATIONS       = BITMASK(18), //!< triggered when the user tries to dynamically use the scripting capabilities (prevent cheating)

    SE_GENERIC_MESSAGEBOX_CLICK        = BITMASK(19), //!< triggered when the user clicks on a message box button, the argument refers to the button pressed

    SE_ALL_EVENTS                      = 0xffffffff,

};

/// Args for `eventCallbackEx()` queued via `MSG_SIM_SCRIPT_EVENT_TRIGGERED`
/// See descriptions at `enum RoR::scriptEvents`.
struct ScriptEventArgs
{
    // both `eventCallback()` and `eventCallbackEx()` ...
    scriptEvents type;
    int arg1;
    // only `eventCallbackEx()` ...
    int arg2ex;
    int arg3ex;
    int arg4ex;
    std::string arg5ex;
    std::string arg6ex;
    std::string arg7ex;
    std::string arg8ex;
};

} // namespace RoR
