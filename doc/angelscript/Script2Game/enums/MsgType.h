
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
* Binding of RoR::MsgType; Global gameplay message loop; used with [`game.pushMessage()`](@ref Script2Game::GameScriptClass::pushMessage).
*/
enum MsgType
{
    MSG_INVALID,
    // Application
    MSG_APP_SHUTDOWN_REQUESTED,                //!< Immediate application shutdown. No params.
    MSG_APP_SCREENSHOT_REQUESTED,              //!< Capture screenshot. No params.
    MSG_APP_DISPLAY_FULLSCREEN_REQUESTED,      //!< Switch to fullscreen. No params.
    MSG_APP_DISPLAY_WINDOWED_REQUESTED,        //!< Switch to windowed display. No params.
    MSG_APP_MODCACHE_LOAD_REQUESTED,           //!< Internal for game startup, DO NOT PUSH MANUALLY.
    MSG_APP_MODCACHE_UPDATE_REQUESTED,         //!< Rescan installed mods and update cache. No params.
    MSG_APP_MODCACHE_PURGE_REQUESTED,          //!< Request cleanup and full rebuild of mod cache.
    MSG_APP_LOAD_SCRIPT_REQUESTED,             //!< Request loading a script from resource(file) or memory; Params 'filename' (string)/'buffer'(string - has precedence over filename), 'category' (ScriptCategory), 'associated_actor' (int - only for SCRIPT_CATEGORY_ACTOR)
    MSG_APP_UNLOAD_SCRIPT_REQUESTED,           //!< Request unloading a script; Param 'id' (int - the ID of the script unit, see 'Script Monitor' tab in console UI.)   
    MSG_APP_REINIT_INPUT_REQUESTED,            //!< Request restarting the entire input subsystem (mouse, keyboard, controllers) including reloading input mappings. Use with caution.
    // Networking
    MSG_NET_CONNECT_REQUESTED,                 //!< Request connection to multiplayer server specified by cvars 'mp_server_host, mp_server_port, mp_server_password'. No params.
    MSG_NET_CONNECT_STARTED,                   //!< Networking notification, DO NOT PUSH MANUALLY.
    MSG_NET_CONNECT_PROGRESS,                  //!< Networking notification, DO NOT PUSH MANUALLY.
    MSG_NET_CONNECT_SUCCESS,                   //!< Networking notification, DO NOT PUSH MANUALLY.
    MSG_NET_CONNECT_FAILURE,                   //!< Networking notification, DO NOT PUSH MANUALLY.
    MSG_NET_SERVER_KICK,                       //!< Networking notification, DO NOT PUSH MANUALLY.
    MSG_NET_DISCONNECT_REQUESTED,              //!< Request disconnect from multiplayer. No params.
    MSG_NET_USER_DISCONNECT,                   //!< Networking notification, DO NOT PUSH MANUALLY.
    MSG_NET_RECV_ERROR,                        //!< Networking notification, DO NOT PUSH MANUALLY.
    MSG_NET_REFRESH_SERVERLIST_SUCCESS,        //!< Background task notification, DO NOT PUSH MANUALLY.
    MSG_NET_REFRESH_SERVERLIST_FAILURE,        //!< Background task notification, DO NOT PUSH MANUALLY.
    MSG_NET_REFRESH_REPOLIST_SUCCESS,          //!< Background task notification, DO NOT PUSH MANUALLY.
    MSG_NET_OPEN_RESOURCE_SUCCESS,             //!< Background task notification, DO NOT PUSH MANUALLY.
    MSG_NET_REFRESH_REPOLIST_FAILURE,          //!< Background task notification, DO NOT PUSH MANUALLY.
    MSG_NET_FETCH_AI_PRESETS_SUCCESS,          //!< Background task notification, DO NOT PUSH MANUALLY.
    MSG_NET_FETCH_AI_PRESETS_FAILURE,          //!< Background task notification, DO NOT PUSH MANUALLY.
    MSG_NET_INSTALL_REPOFILE_REQUESTED         //!< Download file from repository and install it. You must known the exact IDs. Params: 'resource_id' (int), 'file_id' (int), 'filename' (string)
    MSG_NET_INSTALL_REPOFILE_SUCCESS,          //!< Background task notification, DO NOT PUSH MANUALLY.
    MSG_NET_INSTALL_REPOFILE_FAILURE,          //!< Background task notification, DO NOT PUSH MANUALLY.
    MSG_NET_INSTALL_REPOFILE_PROGRESS,         //!< Background task notification, DO NOT PUSH MANUALLY.
    // Simulation
    MSG_SIM_PAUSE_REQUESTED,                   //!< Pause game. No params.
    MSG_SIM_UNPAUSE_REQUESTED,                 //!< Unpause game. No params.
    MSG_SIM_LOAD_TERRN_REQUESTED,              //!< Request loading terrain. Param 'filename' (string)
    MSG_SIM_LOAD_SAVEGAME_REQUESTED,           //!< Request loading saved game. Param 'filename' (string)
    MSG_SIM_UNLOAD_TERRN_REQUESTED,            //!< Request returning to main menu. No params.
    MSG_SIM_SPAWN_ACTOR_REQUESTED,             //!< Request spawning an actor. Params: 'filename' (string), 'position' (vector3), 'rotation' (quaternion), 'instance_id' (int, optional), 'config' (string, optional), 'skin' (string, optional), 'enter' (bool, optional, default true), , 'free_position' (bool, default false)
    MSG_SIM_MODIFY_ACTOR_REQUESTED,            //!< Request change of actor. Params: 'type' (enum ActorModifyRequestType)
    MSG_SIM_DELETE_ACTOR_REQUESTED,            //!< Request actor removal. Params: 'instance_id' (int)
    MSG_SIM_SEAT_PLAYER_REQUESTED,             //!< Put player character in a vehicle. Params: 'instance_id' (int), use -1 to get out of vehicle.
    MSG_SIM_TELEPORT_PLAYER_REQUESTED,         //!< Teleport player character anywhere on terrain. Param 'position' (vector3)
    MSG_SIM_HIDE_NET_ACTOR_REQUESTED,          //!< Request hiding of networked actor; used internally by top menubar. Params: 'instance_id' (int)
    MSG_SIM_UNHIDE_NET_ACTOR_REQUESTED,        //!< Request revealing of hidden networked actor; used internally by top menubar. Params: 'instance_id' (int)
    MSG_SIM_MUTE_NET_ACTOR_REQUESTED,          //!< Request muting of networked actor; used internally by MP userlist UI. Params: 'instance_id' (int)
    MSG_SIM_UNMUTE_NET_ACTOR_REQUESTED,        //!< Request unmuting of networked actor; used internally by MP userlist UI. Params: 'instance_id' (int)
    MSG_SIM_SCRIPT_EVENT_TRIGGERED,            //!< Internal notification about triggering a script event, DO NOT PUSH MANUALLY.
    MSG_SIM_SCRIPT_CALLBACK_QUEUED,            //!< Internal notification about triggering a script event, DO NOT PUSH MANUALLY.
    MSG_SIM_ACTOR_LINKING_REQUESTED,           //!< Request (un)-hooking/tying/roping 2 actors; Params derive from `RoR::ActorLinkingRequest`.
    MSG_SIM_ADD_FREEFORCE_REQUESTED,           //!< Request adding a freeforce (affects single node); Required params: 'id' (int) - use `game.getFreeForceNextId()`, 'type' (enum FreeForceType), 'force_magnitude' (float), 'base_actor' (int - actor instance ID), 'base_node' (int); For type `CONSTANT` add 'force_const_direction' (vector3), For type `TOWARDS_COORDS` add 'target_coords' (vector3); For types `TOWARDS_NODE` and `HALFBEAM_*` add 'target_actor' (int - actor instance ID), 'target_node' (int); Optional args for `HALFBEAM_*` (floats): 'halfb_spring', 'halfb_damp', 'halfb_deform', 'halfb_strength', 'halfb_diameter'; For internals see `RoR::FreeForceRequest`.
    MSG_SIM_MODIFY_FREEFORCE_REQUESTED,        //!< Request modifying a freeforce (affects single node); Required params: 'id' (int) - use `game.getFreeForceNextId()`, 'type' (enum FreeForceType), 'force_magnitude' (float), 'base_actor' (int - actor instance ID), 'base_node' (int); For type `CONSTANT` add 'force_const_direction' (vector3), For type `TOWARDS_COORDS` add 'target_coords' (vector3); For types `TOWARDS_NODE` and `HALFBEAM_*` add 'target_actor' (int - actor instance ID), 'target_node' (int); Optional args for `HALFBEAM_*` (floats): 'halfb_spring', 'halfb_damp', 'halfb_deform', 'halfb_strength', 'halfb_diameter'; For internals see `RoR::FreeForceRequest`.
    MSG_SIM_REMOVE_FREEFORCE_REQUESTED,        //!< Request removing a freeforce; param: `RoR::FreeForceID_t`
    // GUI
    MSG_GUI_OPEN_MENU_REQUESTED,
    MSG_GUI_CLOSE_MENU_REQUESTED,
    MSG_GUI_OPEN_SELECTOR_REQUESTED,           //!< Use `game.showChooser()` instead.
    MSG_GUI_CLOSE_SELECTOR_REQUESTED,          //!< No params.
    MSG_GUI_MP_CLIENTS_REFRESH,                //!< No params.
    MSG_GUI_SHOW_MESSAGE_BOX_REQUESTED,        //!< Use `game.showMessageBox()` instead.
    MSG_GUI_DOWNLOAD_PROGRESS,                 //!< Background task notification, DO NOT PUSH MANUALLY.
    MSG_GUI_DOWNLOAD_FINISHED,                 //!< Background task notification, DO NOT PUSH MANUALLY.
    MSG_GUI_REFRESH_TUNING_MENU_REQUESTED,     //!< Used internally by Top Menubar UI, DO NOT PUSH MANUALLY.
    MSG_GUI_SHOW_CHATBOX_REQUESTED,            //!< Param: message or server command to pre-fill in the chatbox (deleting whatever was there previously); Used by MP userlist UI, DO NOT PUSH MANUALLY.
    // Editing
    MSG_EDI_MODIFY_GROUNDMODEL_REQUESTED,      //!< Used by Friction UI, DO NOT PUSH MANUALLY.
    MSG_EDI_ENTER_TERRN_EDITOR_REQUESTED,      //!< No params.
    MSG_EDI_LEAVE_TERRN_EDITOR_REQUESTED,      //!< No params.
    MSG_EDI_SAVE_TERRN_CHANGES_REQUESTED,      //!< No params.
    MSG_EDI_LOAD_BUNDLE_REQUESTED,             //!< Load a resource bundle (= ZIP or directory) for a given cache entry. Params: 'cache_entry' (CacheEntryClass@)
    MSG_EDI_RELOAD_BUNDLE_REQUESTED,           //!< This deletes all actors using that bundle (= ZIP or directory)! Params: 'cache_entry' (CacheEntryClass@)
    MSG_EDI_UNLOAD_BUNDLE_REQUESTED,           //!< This deletes all actors using that bundle (= ZIP or directory)! Params: 'cache_entry' (CacheEntryClass@)
    MSG_EDI_CREATE_PROJECT_REQUESTED,          //!< Creates a subdir under 'projects/', pre-populates it and adds to modcache. Params: 'name' (string), 'ext' (string, optional), 'source_entry' (CacheEntryClass@)
    MSG_EDI_ADD_FREEBEAMGFX_REQUESTED,         //!< Adds visuals for a freebeam (pair of HALFBEAM freeforces); Params: 'id' (int, use `game.getFreeBeamGfxNextId()`), 'freeforce_primary' (int), 'freeforce_secondary' (), 'mesh_name' (string), 'material_name' (string) ; For internals see `RoR::FreeBeamGfxRequest`
    MSG_EDI_MODIFY_FREEBEAMGFX_REQUESTED,      //!< Updates visuals for a freebeam (pair of HALFBEAM freeforces); Params: 'id' (int, use `game.getFreeBeamGfxNextId()`), 'freeforce_primary' (int), 'freeforce_secondary' (), 'mesh_name' (string), 'material_name' (string) ; For internals see `RoR::FreeBeamGfxRequest`
    MSG_EDI_DELETE_FREEBEAMGFX_REQUESTED,      //!< Removes visuals of a freebeam (pair of HALFBEAM freeforces).
};

} // namespace Script2Game

/// @}    //addtogroup Script2Game
/// @}    //addtogroup ScriptSideAPIs