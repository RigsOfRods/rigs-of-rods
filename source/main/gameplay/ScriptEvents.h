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

/// This enum describes what events are existing. The script can register to receive events.
enum scriptEvents
{
    SE_TRUCK_ENTER                     = 0x00000004, //!< triggered when switching from person mode to vehicle mode, the argument refers to the actor ID of the vehicle
    SE_TRUCK_EXIT                      = 0x00000008, //!< triggered when switching from vehicle mode to person mode, the argument refers to the actor ID of the vehicle

    SE_TRUCK_ENGINE_DIED               = 0x00000010, //!< triggered when the vehicle's engine dies (from underrev, water, etc), the argument refers to the actor ID of the vehicle
    SE_TRUCK_ENGINE_FIRE               = 0x00000020, //!< triggered when the planes engines start to get on fire, the argument refers to the actor ID of the vehicle
    SE_TRUCK_TOUCHED_WATER             = 0x00000040, //!< triggered when any part of the actor touches water, the argument refers to the actor ID
    SE_TRUCK_LIGHT_TOGGLE              = 0x00000400, //!< triggered when the main light is toggled, the argument refers to the actor ID
    SE_TRUCK_TIE_TOGGLE                = 0x00001000, //!< triggered when the user toggles ties, the argument refers to the actor ID
    SE_TRUCK_PARKINGBRAKE_TOGGLE       = 0x00002000, //!< triggered when the user toggles the parking brake, the argument refers to the actor ID
    SE_TRUCK_BEACONS_TOGGLE            = 0x00004000, //!< triggered when the user toggles beacons, the argument refers to the actor ID
    SE_TRUCK_CPARTICLES_TOGGLE         = 0x00008000, //!< triggered when the user toggles custom particles, the argument refers to the actor ID

    SE_GENERIC_NEW_TRUCK               = 0x00020000, //!< triggered when the user spawns a new actor, the argument refers to the actor ID
    SE_GENERIC_DELETED_TRUCK           = 0x00040000, //!< triggered when the user deletes an actor, the argument refers to the actor ID

    SE_TRUCK_RESET                     = 0x00200000, //!< triggered when the user resets the truck, the argument refers to the actor ID of the vehicle
    SE_TRUCK_TELEPORT                  = 0x00400000, //!< triggered when the user teleports the truck, the argument refers to the actor ID of the vehicle
    SE_TRUCK_MOUSE_GRAB                = 0x00100000, //!< triggered when the user uses the mouse to interact with the actor, the argument refers to the actor ID

    SE_ANGELSCRIPT_MANIPULATIONS       = 0x00800000, //!< triggered when the user tries to dynamically use the scripting capabilities (prevent cheating)

    SE_GENERIC_MESSAGEBOX_CLICK        = 0x01000000, //!< triggered when the user clicks on a message box button, the argument refers to the button pressed

    SE_ALL_EVENTS                      = 0xffffffff,

};

