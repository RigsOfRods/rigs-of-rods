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

    SE_ANGELSCRIPT_MANIPULATIONS       = BITMASK(18), //!< triggered when the user tries to dynamically use the scripting capabilities (prevent cheating) args: #1 angelScriptManipulationType - see enum doc comments for more args.
    SE_ANGELSCRIPT_MSGCALLBACK         = BITMASK(19), //!< The diagnostic info directly from AngelScript engine (see `asSMessageInfo`), args: #1 ScriptUnitID, #2 asEMsgType, #3 row, #4 col, #5 sectionName, #6 message
    SE_ANGELSCRIPT_LINECALLBACK        = BITMASK(20), //!< The diagnostic info directly from AngelScript engine (see `SetLineCallback()`), args: #1 ScriptUnitID, #2 LineNumber, #3 CallstackSize, #4 unused, #5 FunctionName, #6 FunctionObjectTypeName #7 ObjectName
    SE_ANGELSCRIPT_EXCEPTIONCALLBACK   = BITMASK(21), //!< The diagnostic info directly from AngelScript engine (see `SetExceptionCallback()`), args: #1 ScriptUnitID, #2 unused, #3 row (`GetExceptionLineNumber()`), #4 unused, #5 funcName, #6 message (`GetExceptionString()`)
    SE_ANGELSCRIPT_THREAD_STATUS       = BITMASK(22), //!< Sent by background threads (i.e. CURL) when there's something important (like finishing a download). args: #1 type, see `RoR::angelScriptThreadStatus`.

    SE_GENERIC_MESSAGEBOX_CLICK        = BITMASK(23), //!< triggered when the user clicks on a message box button, the argument refers to the button pressed
    SE_GENERIC_EXCEPTION_CAUGHT        = BITMASK(24), //!< Triggered when C++ exception (usually Ogre::Exception) is thrown; #1 ScriptUnitID, #5 originFuncName, #6 type, #7 message.
    SE_GENERIC_MODCACHE_ACTIVITY       = BITMASK(25), //!< Triggered when status of modcache changes, args: #1 type, #2 entry number, for other args see `RoR::modCacheActivityType`
    SE_GENERIC_TRUCK_LINKING_CHANGED   = BITMASK(26), //!< Triggered when 2 actors become linked or unlinked via ties/hooks/ropes/slidenodes; args: #1 state (1=linked, 0=unlinked), #2 action `ActorLinkingRequestType` #3 master ActorInstanceID_t, #4 slave ActorInstanceID_t
    SE_GENERIC_FREEFORCES_ACTIVITY     = BITMASK(27), //!< Triggered on freeforce add/update/delete or breaking; args: #1 `freeForcesActivityType`, #2 freeforce ID

    SE_ALL_EVENTS                      = 0xffffffff,
    SE_NO_EVENTS                       = 0

};

/// Argument #1 of script event `SE_ANGELSCRIPT_MANIPULATIONS`
enum angelScriptManipulationType
{
    ASMANIP_CONSOLE_SNIPPET_EXECUTED = 0, // 0 for Backwards compatibility.
    ASMANIP_SCRIPT_LOADED,                //!< Triggered after the script's `main()` completed; may trigger additional processing (for example, it delivers the *.mission file to mission system script). Args: #2 ScriptUnitId_t, #3 RoR::ScriptCategory, #4 unused, #5 filename.
    ASMANIP_SCRIPT_UNLOADING,             //!< Triggered before unloading the script to let it clean up (important for missions). Args: #2 ScriptUnitId_t, #3 RoR::ScriptCategory, #4 unused, #5 filename.
    ASMANIP_ACTORSIMATTR_SET              //!< Triggered when `setSimAttribute()` is called; additional args: #2 `RoR::ActorSimAtrr`, #3 ---, #4 ---, #5 attr name, #6 value converted to string.
};

enum angelScriptThreadStatus
{
    ASTHREADSTATUS_NONE,
    ASTHREADSTATUS_CURLSTRING_PROGRESS, //!< Args of `RoR::SE_ANGELSCRIPT_THREAD_STATUS`: arg#1 type, arg#2 percentage, arg#3 unused, arg#4 unused, arg#5 progress message (formatted by RoR)
    ASTHREADSTATUS_CURLSTRING_SUCCESS,  //!< Args of `RoR::SE_ANGELSCRIPT_THREAD_STATUS`: arg#1 type, arg#2 HTTP code, arg#3 CURLcode, arg#4 unused, arg#5 payload
    ASTHREADSTATUS_CURLSTRING_FAILURE,  //!< Args of `RoR::SE_ANGELSCRIPT_THREAD_STATUS`: arg#1 type, arg#2 HTTP code, arg#3 CURLcode, arg#4 unused, arg#5 message from `curl_easy_strerror()`
};

/// Argument #1 of script event `RoR::SE_GENERIC_MODCACHE_ACTIVITY`
enum modCacheActivityType
{
    MODCACHEACTIVITY_NONE,

    MODCACHEACTIVITY_ENTRY_ADDED,      //!< Args: #1 type, #2 entry number, --, --, #5 fname, #6 fext
    MODCACHEACTIVITY_ENTRY_MODIFIED,   //!< Args: #1 type, #2 entry number, --, --, #5 fname, #6 fext
    MODCACHEACTIVITY_ENTRY_DELETED,    //!< Flagged as `deleted`, managed by shared pointers; Args: #1 type, #2 entry number, --, --, #5 fname, #6 fext

    MODCACHEACTIVITY_BUNDLE_LOADED,    //!< Args: #1 type, #2 entry number, --, --, #5 rg name
    MODCACHEACTIVITY_BUNDLE_RELOADED,  //!< Args: #1 type, #2 entry number, --, --, #5 rg name
    MODCACHEACTIVITY_BUNDLE_UNLOADED   //!< Args: #1 type, #2 entry number
};

/// Argument #1 of script event `RoR::SE_GENERIC_FREEFORCES_ACTIVITY`
enum freeForcesActivityType
{
    FREEFORCESACTIVITY_NONE,

    FREEFORCESACTIVITY_ADDED,
    FREEFORCESACTIVITY_MODIFIED,
    FREEFORCESACTIVITY_REMOVED,

    FREEFORCESACTIVITY_DEFORMED, //!< Only with `HALFBEAM_*` types; arg #5 (string containing float) the actual stress, arg #6 (string containing float) maximum stress.
    FREEFORCESACTIVITY_BROKEN, //!< Only with `HALFBEAM_*` types; arg #5 (string containing float) the applied force, arg #6 (string containing float) breaking threshold force.
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
