
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
 * Binding of `RoR::scriptEvents`; All the events that can be used by the script.
 * @see Script2Game::GameScriptClass::registerForEvent()
 */
 enum scriptEvents
 {
    SE_EVENTBOX_ENTER,                  //!< An actor or person entered an eventbox; Arguments of `eventCallbackEx()`: #1 type, #2 Actor Instance ID (use `game.getTruckByNum()`), #3 Actor node ID, #4 unused, #5 object instance name, #6 eventbox name #7 unused #8 unused.
    SE_EVENTBOX_EXIT,                   //!< An actor or person entered an eventbox; Arguments of `eventCallbackEx()`: #1 type, #2 Actor Instance ID (use `game.getTruckByNum()`), #3 unused, #4 unused, #5 object instance name, #6 eventbox name #7 unused #8 unused.
     
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

    SE_ANGELSCRIPT_MANIPULATIONS,       //!< triggered when the user tries to dynamically use the scripting capabilities (prevent cheating) args:  #1 `angelScriptManipulationType` - see enum doc comments for more args.
    SE_ANGELSCRIPT_MSGCALLBACK,         //!< The diagnostic info directly from AngelScript engine (see `asSMessageInfo`), args: #1 ScriptUnitID, #2 asEMsgType, #3 row, #4 col, #5 sectionName, #6 message
    SE_ANGELSCRIPT_LINECALLBACK,        //!< The diagnostic info directly from AngelScript engine (see `SetLineCallback()`), args: #1 ScriptUnitID, #2 LineNumber, #3 CallstackSize, #4 unused, #5 FunctionName, #6 FunctionObjectTypeName #7 ObjectName
    SE_ANGELSCRIPT_EXCEPTIONCALLBACK,   //!< The diagnostic info directly from AngelScript engine (see `SetExceptionCallback()`), args: #1 ScriptUnitID, #2 unused, #3 row (`GetExceptionLineNumber()`), #4 unused, #5 funcName, #6 message (`GetExceptionString()`)
    SE_ANGELSCRIPT_THREAD_STATUS,       //!< Sent by background threads (i.e. CURL) when there's something important (like finishing a download). args: #1 type, see `Script2Game::angelScriptThreadStatus`.    

 	SE_GENERIC_MESSAGEBOX_CLICK,        //!< triggered when the user clicks on a message box button, the argument refers to the button pressed
    SE_GENERIC_EXCEPTION_CAUGHT,        //!< Triggered when C++ exception (usually Ogre::Exception) is thrown; #1 ScriptUnitID, #5 originFuncName, #6 type, #7 message.
    SE_GENERIC_MODCACHE_ACTIVITY,       //!< Triggered when status of modcache changes, args: #1 type, #2 entry number, for other args see `RoR::modCacheActivityType`  

    SE_GENERIC_TRUCK_LINKING_CHANGED,   //!< Triggered when 2 actors become linked or unlinked via ties/hooks/ropes/slidenodes; args: #1 state (1=linked, 0=unlinked), #2 action `ActorLinkingRequestType` #3 master ActorInstanceID_t, #4 slave ActorInstanceID_t

 	SE_ALL_EVENTS                      = 0xffffffff,
    SE_NO_EVENTS                       = 0

 };

} // namespace Script2Game

/// @}    //addtogroup Script2Game
/// @}    //addtogroup ScriptSideAPIs