
  // =================================================== //
  // THIS IS NOT A C++ HEADER! Only a dummy for Doxygen. //
  // =================================================== //

/** \addtogroup ScriptSideAPIs
 *  @{
 */    

/** \addtogroup Script2Game
 *  @{
 */   

namespace Script2Game {

/**
 * Binding of `RoR::scriptEvents`; All the events that can be used by the script; use ([`game.registerForEvent()`](@ref Script2Game::GameScriptClass::registerForEvent)) to start receiving a given event type, and implement [`eventCallbackEx()`](@ref Game2Script::eventCallbackEx) to intercept events.
 */
 enum scriptEvents
 {
    SE_EVENTBOX_ENTER,                  //!< An actor or person entered an eventbox<br> Arguments of `eventCallbackEx()`:<br> #1 type,<br> #2 Actor Instance ID (use `game.getTruckByNum()`),<br> #3 Actor node ID,<br> #4 unused,<br> #5 object instance name,<br> #6 eventbox name<br> #7 unused #8 unused.
    SE_EVENTBOX_EXIT,                   //!< An actor or person entered an eventbox<br> Arguments of `eventCallbackEx()`:<br> #1 type<br> #2 Actor Instance ID (use `game.getTruckByNum()`)<br> #3 unused<br> #4 unused<br> #5 object instance name<br> #6 eventbox name<br> #7 unused<br> #8 unused.
     
 	SE_TRUCK_ENTER,                     //!< triggered when switching from person mode to truck mode, the argument refers to the Actor Instance ID (use `game.getTruckByNum()`)
 	SE_TRUCK_EXIT,                      //!< triggered when switching from truck mode to person mode, the argument refers to the Actor Instance ID (use `game.getTruckByNum()`)

 	SE_TRUCK_ENGINE_DIED,               //!< triggered when the trucks engine dies (from underrev, water, etc), the argument refers to the Actor Instance ID (use `game.getTruckByNum()`)
 	SE_TRUCK_ENGINE_FIRE,               //!< triggered when the planes engines start to get on fire, the argument refers to the Actor Instance ID (use `game.getTruckByNum()`)
 	SE_TRUCK_TOUCHED_WATER,             //!< triggered when any part of the truck touches water, the argument refers to the Actor Instance ID (use `game.getTruckByNum()`)
 	SE_TRUCK_LIGHT_TOGGLE,              //!< triggered when the main light is toggled, the argument refers to the Actor Instance ID (use `game.getTruckByNum()`)
 	SE_TRUCK_TIE_TOGGLE,                //!< triggered when the user toggles ties, the argument refers to the Actor Instance ID (use `game.getTruckByNum()`)
 	SE_TRUCK_PARKINGBRAKE_TOGGLE,       //!< triggered when the user toggles the parking brake, the argument refers to the Actor Instance ID (use `game.getTruckByNum()`)
 	SE_TRUCK_BEACONS_TOGGLE,            //!< triggered when the user toggles beacons, the argument refers to the Actor Instance ID (use `game.getTruckByNum()`)
 	SE_TRUCK_CPARTICLES_TOGGLE,         //!< triggered when the user toggles custom particles, the argument refers to the Actor Instance ID (use `game.getTruckByNum()`)

 	SE_GENERIC_NEW_TRUCK,               //!< triggered when the user spawns a new truck, the argument refers to the Actor Instance ID (use `game.getTruckByNum()`)
 	SE_GENERIC_DELETED_TRUCK,           //!< triggered when the user deletes a truck, the argument refers to the Actor Instance ID (use `game.getTruckByNum()`)
    
    SE_TRUCK_RESET,                     //!< triggered when the user resets the truck, the argument refers to the Actor Instance ID (use `game.getTruckByNum()`)
    SE_TRUCK_TELEPORT,                  //!< triggered when the user teleports the truck, the argument refers to the Actor Instance ID (use `game.getTruckByNum()`)
    SE_TRUCK_MOUSE_GRAB,                //!< triggered when the user uses the mouse to interact with the actor, the argument refers to the Actor Instance ID (use `game.getTruckByNum()`)

    SE_ANGELSCRIPT_MANIPULATIONS,       //!< triggered when the user tries to dynamically use the scripting capabilities (prevent cheating)<br> args:<br>  #1 `angelScriptManipulationType` - see enum doc comments for more args.
    SE_ANGELSCRIPT_MSGCALLBACK,         //!< The diagnostic info directly from AngelScript engine (see `asSMessageInfo`),<br> args:<br> #1 ScriptUnitID,<br> #2 asEMsgType,<br> #3 row,<br> #4 col,<br> #5 sectionName,<br> #6 message
    SE_ANGELSCRIPT_LINECALLBACK,        //!< The diagnostic info directly from AngelScript engine (see `SetLineCallback()`),<br> args:<br> #1 ScriptUnitID,<br> #2 LineNumber,<br> #3 CallstackSize,<br> #4 unused,<br> #5 FunctionName,<br> #6 FunctionObjectTypeName<br> #7 ObjectName
    SE_ANGELSCRIPT_EXCEPTIONCALLBACK,   //!< The diagnostic info directly from AngelScript engine (see `SetExceptionCallback()`),<br> args:<br> #1 ScriptUnitID,<br> #2 unused,<br> #3 row (`GetExceptionLineNumber()`),<br> #4 unused,<br> #5 funcName,<br> #6 message (`GetExceptionString()`)
    SE_ANGELSCRIPT_THREAD_STATUS,       //!< Sent by background threads (i.e. CURL) when there's something important (like finishing a download).<br> args:<br> #1 type, see `Script2Game::angelScriptThreadStatus`.    

 	SE_GENERIC_MESSAGEBOX_CLICK,        //!< triggered when the user clicks on a message box button, the argument refers to the button pressed
    SE_GENERIC_EXCEPTION_CAUGHT,        //!< Triggered when C++ exception (usually Ogre::Exception) is thrown;<br> #1 ScriptUnitID,<br> #5 originFuncName,<br> #6 type,<br> #7 message.
    SE_GENERIC_MODCACHE_ACTIVITY,       //!< Triggered when status of modcache changes,<br> args:<br> #1 type,<br> #2 entry number,<br> for other args see `RoR::modCacheActivityType`  

    SE_GENERIC_TRUCK_LINKING_CHANGED,   //!< Triggered when 2 actors become linked or unlinked via ties/hooks/ropes/slidenodes;<br> args:<br> #1 state (1=linked, 0=unlinked),<br> #2 action `ActorLinkingRequestType`<br> #3 master ActorInstanceID_t,<br> #4 slave ActorInstanceID_t

 	SE_ALL_EVENTS                      = 0xffffffff,
    SE_NO_EVENTS                       = 0

 };

} // namespace Script2Game

/// @}    //addtogroup Script2Game
/// @}    //addtogroup ScriptSideAPIs
