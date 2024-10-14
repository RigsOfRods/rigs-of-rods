/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2013-2023 Petr Ohlidal

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

/// @file   Application.h
/// @author Petr Ohlidal
/// @date   05/2014
/// @brief  Central state/object manager and communications hub.

#pragma once

#include "BitFlags.h"
#include "CVar.h"
#include "ForwardDeclarations.h"
#include "Str.h"

#include <fmt/format.h>
#include <OgreStringConverter.h>

#include <assert.h>
#include <string>

#define ROR_ASSERT(_EXPR)  assert(_EXPR)

#define CHARACTER_ANIM_NAME_LEN 10 // Restricted for networking

// OGRE resource group names
#define RGN_TEMP "Temp"
#define RGN_CACHE "Cache"
#define RGN_REPO "Repo"
#define RGN_CONFIG "Config"
#define RGN_CONTENT "Content"
#define RGN_SAVEGAMES "Savegames"
#define RGN_MANAGED_MATS "ManagedMaterials"
#define RGN_SCRIPTS "Scripts"
#define RGN_LOGS "Logs"

// Legacy macros
#define TOSTRING(x)     Ogre::StringConverter::toString(x)
#define PARSEINT(x)     Ogre::StringConverter::parseInt(x)
#define PARSEREAL(x)    Ogre::StringConverter::parseReal(x)
#define HydraxLOG(msg)  Ogre::LogManager::getSingleton().logMessage("[Hydrax] " + Ogre::String(msg));

namespace RoR {

// ------------------------------------------------------------------------------------------------
// Global definitions and enumerations
// ------------------------------------------------------------------------------------------------

/// @addtogroup GameState Game state
/// @{

/// @addtogroup MsgQueue Message queue
/// @{

/// Global gameplay message loop, see `struct Message` in GameContext.h
enum MsgType
{
    // CHECKLIST for adding new message types:
    // * Provide name as string constant - see `MsgTypeToString()` in Application.cpp
    // * Register it with AngelScript - see `RegisterMessageQueue()` in 'scripting/bindings/MsgQueueAngelscript.cpp'
    // * Allow/Deny pushing from AngelScript - see `pushMessage()` in 'scripting/GameScript.cpp'
    // * Document the AngelScript usage - see 'doc/angelscript/Script2Game/globals.h'.
    // * Make it do something useful in 'main.cpp' ;)

    MSG_INVALID,
    // Application
    MSG_APP_SHUTDOWN_REQUESTED,
    MSG_APP_SCREENSHOT_REQUESTED,
    MSG_APP_DISPLAY_FULLSCREEN_REQUESTED,
    MSG_APP_DISPLAY_WINDOWED_REQUESTED,
    MSG_APP_MODCACHE_LOAD_REQUESTED,
    MSG_APP_MODCACHE_UPDATE_REQUESTED,
    MSG_APP_MODCACHE_PURGE_REQUESTED,
    MSG_APP_LOAD_SCRIPT_REQUESTED,         //!< Payload = RoR::LoadScriptRequest* (owner)
    MSG_APP_UNLOAD_SCRIPT_REQUESTED,       //!< Payload = RoR::ScriptUnitId_t* (owner)
    MSG_APP_SCRIPT_THREAD_STATUS,          //!< Payload = RoR::ScriptEventArgs* (owner)
    MSG_APP_REINIT_INPUT_REQUESTED,
    // Networking
    MSG_NET_CONNECT_REQUESTED,
    MSG_NET_CONNECT_STARTED,
    MSG_NET_CONNECT_PROGRESS,
    MSG_NET_CONNECT_SUCCESS,
    MSG_NET_CONNECT_FAILURE,
    MSG_NET_SERVER_KICK,
    MSG_NET_DISCONNECT_REQUESTED,
    MSG_NET_USER_DISCONNECT,
    MSG_NET_RECV_ERROR,
    MSG_NET_REFRESH_SERVERLIST_SUCCESS,    //!< Payload = GUI::MpServerInfoVec* (owner)
    MSG_NET_REFRESH_SERVERLIST_FAILURE,    //!< Payload = RoR::CurlFailInfo* (owner)
    MSG_NET_REFRESH_REPOLIST_SUCCESS,      //!< Payload = GUI::ResourcesCollection* (owner)
    MSG_NET_OPEN_RESOURCE_SUCCESS,         //!< Payload = GUI::ResourcesCollection* (owner)
    MSG_NET_REFRESH_REPOLIST_FAILURE,      //!< Payload = RoR::CurlFailInfo* (owner)
    MSG_NET_FETCH_AI_PRESETS_SUCCESS,      //!< Description = JSON string
    MSG_NET_FETCH_AI_PRESETS_FAILURE,      //!< Description = message
    // Simulation
    MSG_SIM_PAUSE_REQUESTED,
    MSG_SIM_UNPAUSE_REQUESTED,
    MSG_SIM_LOAD_TERRN_REQUESTED,
    MSG_SIM_LOAD_SAVEGAME_REQUESTED,
    MSG_SIM_UNLOAD_TERRN_REQUESTED,
    MSG_SIM_SPAWN_ACTOR_REQUESTED,         //!< Payload = RoR::ActorSpawnRequest* (owner)
    MSG_SIM_MODIFY_ACTOR_REQUESTED,        //!< Payload = RoR::ActorModifyRequest* (owner)
    MSG_SIM_DELETE_ACTOR_REQUESTED,        //!< Payload = RoR::ActorPtr* (owner)
    MSG_SIM_SEAT_PLAYER_REQUESTED,         //!< Payload = RoR::ActorPtr (owner) | nullptr
    MSG_SIM_TELEPORT_PLAYER_REQUESTED,     //!< Payload = Ogre::Vector3* (owner)
    MSG_SIM_HIDE_NET_ACTOR_REQUESTED,      //!< Payload = ActorPtr* (owner)
    MSG_SIM_UNHIDE_NET_ACTOR_REQUESTED,    //!< Payload = ActorPtr* (owner)
    MSG_SIM_SCRIPT_EVENT_TRIGGERED,        //!< Payload = RoR::ScriptEventArgs* (owner)
    MSG_SIM_SCRIPT_CALLBACK_QUEUED,        //!< Payload = RoR::ScriptCallbackArgs* (owner)
    MSG_SIM_ACTOR_LINKING_REQUESTED,       //!< Payload = RoR::ActorLinkingRequest* (owner)
    MSG_SIM_ADD_FREEFORCE_REQUESTED,       //!< Payload = RoR::FreeForceRequest* (owner)
    MSG_SIM_MODIFY_FREEFORCE_REQUESTED,    //!< Payload = RoR::FreeForceRequest* (owner)
    MSG_SIM_REMOVE_FREEFORCE_REQUESTED,    //!< Payload = RoR::FreeForceID_t* (owner)
    // GUI
    MSG_GUI_OPEN_MENU_REQUESTED,
    MSG_GUI_CLOSE_MENU_REQUESTED,
    MSG_GUI_OPEN_SELECTOR_REQUESTED,       //!< Payload = LoaderType* (owner), Description = GUID | empty
    MSG_GUI_CLOSE_SELECTOR_REQUESTED,
    MSG_GUI_MP_CLIENTS_REFRESH,
    MSG_GUI_SHOW_MESSAGE_BOX_REQUESTED,    //!< Payload = MessageBoxConfig* (owner)
    MSG_GUI_HIDE_MESSAGE_BOX_REQUESTED,
    MSG_GUI_DOWNLOAD_PROGRESS,
    MSG_GUI_DOWNLOAD_FINISHED,
    MSG_GUI_REFRESH_TUNING_MENU_REQUESTED,
    // Editing
    MSG_EDI_MODIFY_GROUNDMODEL_REQUESTED,  //!< Payload = RoR::ground_model_t* (weak)
    MSG_EDI_ENTER_TERRN_EDITOR_REQUESTED,
    MSG_EDI_LEAVE_TERRN_EDITOR_REQUESTED,
    MSG_EDI_LOAD_BUNDLE_REQUESTED,         //!< Payload = RoR::CacheEntryPtr* (owner)
    MSG_EDI_RELOAD_BUNDLE_REQUESTED,       //!< Payload = RoR::CacheEntryPtr* (owner)
    MSG_EDI_UNLOAD_BUNDLE_REQUESTED,       //!< Payload = RoR::CacheEntryPtr* (owner)
    MSG_EDI_CREATE_PROJECT_REQUESTED,      //!< Payload = RoR::CreateProjectRequest* (owner)
    MSG_EDI_MODIFY_PROJECT_REQUESTED,      //!< Payload = RoR::UpdateProjectRequest* (owner)
    MSG_EDI_DELETE_PROJECT_REQUESTED,      //!< Payload = RoR::CacheEntryPtr* (owner)
};

const char* MsgTypeToString(MsgType type);

/// @} // addtogroup MsgQueue

} // namespace RoR

namespace RigDef
{
enum class Keyword
{
    // IMPORTANT! If you add a value here, you must also modify `RigDef::Regexes::IDENTIFY_KEYWORD`, it relies on numeric values of this enum.

    INVALID = 0,

    ADD_ANIMATION = 1,
    AIRBRAKES,
    ANIMATORS,
    ANTILOCKBRAKES,
    ASSETPACKS,
    AUTHOR,
    AXLES,
    BACKMESH,
    BEAMS,
    BRAKES,
    CAB,
    CAMERARAIL,
    CAMERAS,
    CINECAM,
    COLLISIONBOXES,
    COMMANDS,
    COMMANDS2,
    COMMENT,
    CONTACTERS,
    CRUISECONTROL,
    DEFAULT_SKIN,
    DESCRIPTION,
    DETACHER_GROUP,
    DISABLEDEFAULTSOUNDS,
    ENABLE_ADVANCED_DEFORMATION,
    END,
    END_COMMENT,
    END_DESCRIPTION,
    END_SECTION,
    ENGINE,
    ENGOPTION,
    ENGTURBO,
    ENVMAP,
    EXHAUSTS,
    EXTCAMERA,
    FILEFORMATVERSION,
    FILEINFO,
    FIXES,
    FLARES,
    FLARES2,
    FLARES3,
    FLEXBODIES,
    FLEXBODY_CAMERA_MODE,
    FLEXBODYWHEELS,
    FORSET,
    FORWARDCOMMANDS,
    FUSEDRAG,
    GLOBALS,
    GUID,
    GUISETTINGS,
    HELP,
    HIDEINCHOOSER,
    HOOKGROUP, // obsolete, ignored
    HOOKS,
    HYDROS,
    IMPORTCOMMANDS,
    INTERAXLES,
    LOCKGROUPS,
    LOCKGROUP_DEFAULT_NOLOCK,
    MANAGEDMATERIALS,
    MATERIALFLAREBINDINGS,
    MESHWHEELS,
    MESHWHEELS2,
    MINIMASS,
    NODECOLLISION, // obsolete
    NODES,
    NODES2,
    PARTICLES,
    PISTONPROPS,
    PROP_CAMERA_MODE,
    PROPS,
    RAILGROUPS,
    RESCUER,
    RIGIDIFIERS, // obsolete
    ROLLON,
    ROPABLES,
    ROPES,
    ROTATORS,
    ROTATORS2,
    SCREWPROPS,
    SCRIPTS,
    SECTION,
    SECTIONCONFIG,
    SET_BEAM_DEFAULTS,
    SET_BEAM_DEFAULTS_SCALE,
    SET_COLLISION_RANGE,
    SET_DEFAULT_MINIMASS,
    SET_INERTIA_DEFAULTS,
    SET_MANAGEDMATERIALS_OPTIONS,
    SET_NODE_DEFAULTS,
    SET_SHADOWS,
    SET_SKELETON_SETTINGS,
    SHOCKS,
    SHOCKS2,
    SHOCKS3,
    SLIDENODE_CONNECT_INSTANTLY,
    SLIDENODES,
    SLOPE_BRAKE,
    SOUNDSOURCES,
    SOUNDSOURCES2,
    SPEEDLIMITER,
    SUBMESH,
    SUBMESH_GROUNDMODEL,
    TEXCOORDS,
    TIES,
    TORQUECURVE,
    TRACTIONCONTROL,
    TRANSFERCASE,
    TRIGGERS,
    TURBOJETS,
    TURBOPROPS,
    TURBOPROPS2,
    VIDEOCAMERA,
    WHEELDETACHERS,
    WHEELS,
    WHEELS2,
    WINGS
};

const char* KeywordToString(Keyword keyword);

} // namespace RigDef

namespace RoR
{

enum class AppState
{
    BOOTSTRAP,          //!< Initial state
    MAIN_MENU,
    SIMULATION,
    SHUTDOWN,
    PRINT_HELP_EXIT,
    PRINT_VERSION_EXIT,
};

enum class MpState
{
    DISABLED,  //!< Not connected for whatever reason.
    CONNECTING,
    CONNECTED,
};

enum class SimState
{
    OFF,
    RUNNING,
    PAUSED,
    EDITOR_MODE //!< Hacky, but whatever... added by Ulteq, 2016
};

/// @} // addtogroup GameState

/// @addtogroup Gameplay
/// @{

/// @addtogroup Trucks
/// @{

enum class SimGearboxMode
{
    AUTO,          //!< Automatic shift
    SEMI_AUTO,     //!< Manual shift with auto clutch
    MANUAL,        //!< Fully manual: sequential shift
    MANUAL_STICK,  //!< Fully manual: stick shift
    MANUAL_RANGES, //!< Fully manual: stick shift with ranges
};
std::string ToLocalizedString(SimGearboxMode e);

/// @}
/// @}

enum class GfxShadowType
{
    NONE,
    PSSM
};
std::string ToLocalizedString(GfxShadowType e);

enum class GfxExtCamMode
{
    NONE,
    STATIC,
    PITCHING,
};
std::string ToLocalizedString(GfxExtCamMode e);

enum class GfxTexFilter
{
    NONE,
    BILINEAR,
    TRILINEAR,
    ANISOTROPIC,
};
std::string ToLocalizedString(GfxTexFilter e);

enum class GfxVegetation
{
    NONE,
    x20PERC,
    x50PERC,
    FULL,
};
std::string ToLocalizedString(GfxVegetation e);

enum class GfxFlaresMode
{
    NONE,                    //!< None (fastest)
    NO_LIGHTSOURCES,         //!< No light sources
    CURR_VEHICLE_HEAD_ONLY,  //!< Only current vehicle, main lights
    ALL_VEHICLES_HEAD_ONLY,  //!< All vehicles, main lights
    ALL_VEHICLES_ALL_LIGHTS, //!< All vehicles, all lights
};
std::string ToLocalizedString(GfxFlaresMode e);

enum class GfxWaterMode
{
    NONE,       //!< None
    BASIC,      //!< Basic (fastest)
    REFLECT,    //!< Reflection
    FULL_FAST,  //!< Reflection + refraction (speed optimized)
    FULL_HQ,    //!< Reflection + refraction (quality optimized)
    HYDRAX,     //!< HydraX
};
std::string ToLocalizedString(GfxWaterMode e);

enum class GfxSkyMode
{
    SANDSTORM,  //!< Sandstorm (fastest)
    CAELUM,     //!< Caelum (best looking, slower)
    SKYX,       //!< SkyX (best looking, slower)
};
std::string ToLocalizedString(GfxSkyMode e);

enum class IoInputGrabMode
{
    NONE,
    ALL,
    DYNAMIC,
};
std::string ToLocalizedString(IoInputGrabMode e);

enum class SimResetMode
{
    HARD = 0, // 'sim_soft_reset=false'
    SOFT = 1  // 'sim_soft_reset=true'
};
std::string ToLocalizedString(SimResetMode e);

enum VisibilityMasks
{
    DEPTHMAP_ENABLED  = BITMASK(1),
    DEPTHMAP_DISABLED = BITMASK(2),
    HIDE_MIRROR       = BITMASK(3),
};

enum LoaderType //!< Search mode for `ModCache::Query()` & Operation mode for `GUI::MainSelector`
{
    LT_None,
    LT_Terrain,   // Invocable from GUI; No script alias, used in main menu
    LT_Vehicle,   // Script "vehicle",   ext: truck car
    LT_Truck,     // Script "truck",     ext: truck car
    LT_Car,       // Script "car",       ext: car
    LT_Boat,      // Script "boat",      ext: boat
    LT_Airplane,  // Script "airplane",  ext: airplane
    LT_Trailer,   // Script "trailer",   ext: trailer
    LT_Train,     // Script "train",     ext: train
    LT_Load,      // Script "load",      ext: load
    LT_Extension, // Script "extension", ext: trailer load
    LT_Skin,      // No script alias, invoked automatically
    LT_AllBeam,   // Invocable from GUI; Script "all",  ext: truck car boat airplane train load
    LT_AddonPart, // No script alias, invoked manually, ext: addonpart
    LT_Tuneup,    // No script alias, invoked manually, ext: tuneup
    LT_AssetPack, // No script alias, invoked manually, ext: assetpack
};

enum class WheelBraking: int /// Wheels are braked by three mechanisms: A footbrake, a handbrake/parkingbrake, and directional brakes used for skidsteer steering.
{
    // DO NOT MODIFY NUMBERS - de/serialized from/to truck definition file, see RigDef_File.h, `struct BaseWheel`

    NONE                 = 0, //!< = no  footbrake, no  handbrake, no  direction control -- wheel is unbraked
    FOOT_HAND            = 1, //!< = yes footbrake, yes handbrake, no  direction control
    FOOT_HAND_SKID_LEFT  = 2, //!< = yes footbrake, yes handbrake, yes direction control (braked when vehicle steers to the left)
    FOOT_HAND_SKID_RIGHT = 3, //!< = yes footbrake, yes handbrake, yes direction control (braked when vehicle steers to the right)
    FOOT_ONLY            = 4, //!< = yes footbrake, no  handbrake, no  direction control -- footbrake only, such as with the front wheels of a passenger car
};

enum class WheelPropulsion: int
{
    // DO NOT MODIFY NUMBERS - de/serialized from/to truck definition file, see RigDef_File.h, `struct BaseWheel`

    NONE                 = 0,
    FORWARD              = 1,
    BACKWARD             = 2,
};

/// Used by rig-def/addonpart/tuneup formats to specify wheel rim mesh orientation.
enum class WheelSide: char
{
    // DO NOT MODIFY LETTERS - de/serialized from/to truck definition file, see 'RigDef_File.h' and 'ActorExport.cpp'

    INVALID   = 'n',
    RIGHT     = 'r',
    LEFT      = 'l'
};

enum class FlareType: char
{
    // DO NOT MODIFY LETTERS - de/serialized from/to truck definition file, see 'RigDef_File.h' and 'ActorExport.cpp'

    NONE           = 0,
    // Front lights
    HEADLIGHT      = 'f',
    HIGH_BEAM      = 'h',
    FOG_LIGHT      = 'g',
    // Rear lighs
    TAIL_LIGHT     = 't',
    BRAKE_LIGHT    = 'b',
    REVERSE_LIGHT  = 'R',
    // Special lights
    SIDELIGHT      = 's',
    BLINKER_LEFT   = 'l',
    BLINKER_RIGHT  = 'r',
    USER           = 'u',
    DASHBOARD      = 'd'
};

enum class ExtCameraMode
{
    // DO NOT MODIFY NUMBERS - de/serialized from/to truck definition file, see RigDef_File.h and 'ActorExport.cpp'

    INVALID = -1,
    CLASSIC = 0,
    CINECAM = 1,
    NODE    = 2,
};

// Dynamic visibility control (value 0 and higher is cinecam index) - common to 'props' and 'flexbodies'
// DO NOT MODIFY NUMBERS - de/serialized from/to truck definition file, see RigDef_File.h and 'ActorExport.cpp'
typedef int CameraMode_t;
static CameraMode_t CAMERA_MODE_ALWAYS_HIDDEN = -3;
static CameraMode_t CAMERA_MODE_ALWAYS_VISIBLE = -2;
static CameraMode_t CAMERA_MODE_3RDPERSON_ONLY = -1;

// ------------------------------------------------------------------------------------------------
// Global variables
// ------------------------------------------------------------------------------------------------

namespace App {

// App
extern CVar* app_state;
extern CVar* app_language;
extern CVar* app_country;
extern CVar* app_skip_main_menu;
extern CVar* app_async_physics;
extern CVar* app_num_workers;
extern CVar* app_screenshot_format;
extern CVar* app_rendersys_override;
extern CVar* app_extra_mod_path;
extern CVar* app_force_cache_purge;
extern CVar* app_force_cache_update;
extern CVar* app_disable_online_api;
extern CVar* app_config_long_names;
extern CVar* app_custom_scripts;
extern CVar* app_recent_scripts;

// Simulation
extern CVar* sim_state;
extern CVar* sim_terrain_name;
extern CVar* sim_terrain_gui_name;
extern CVar* sim_spawn_running;
extern CVar* sim_replay_enabled;
extern CVar* sim_replay_length;
extern CVar* sim_replay_stepping;
extern CVar* sim_realistic_commands;
extern CVar* sim_races_enabled;
extern CVar* sim_no_collisions;
extern CVar* sim_no_self_collisions;
extern CVar* sim_gearbox_mode;
extern CVar* sim_soft_reset_mode;
extern CVar* sim_quickload_dialog;
extern CVar* sim_live_repair_interval; //!< Hold EV_COMMON_REPAIR_TRUCK to enter LiveRepair mode. 0 or negative interval disables.
extern CVar* sim_tuning_enabled;

// Multiplayer
extern CVar* mp_state;
extern CVar* mp_join_on_startup;
extern CVar* mp_chat_auto_hide;
extern CVar* mp_hide_net_labels;
extern CVar* mp_hide_own_net_label;
extern CVar* mp_pseudo_collisions;
extern CVar* mp_server_host;
extern CVar* mp_server_port;
extern CVar* mp_server_password;
extern CVar* mp_player_name;
extern CVar* mp_player_token;
extern CVar* mp_api_url;
extern CVar* mp_cyclethru_net_actors; //!< Include remote actors when cycling through with CTRL + [ and CTRL + ]

// New remote API
extern CVar* remote_query_url;

// Diagnostic
extern CVar* diag_auto_spawner_report;
extern CVar* diag_camera;
extern CVar* diag_trace_globals;
extern CVar* diag_rig_log_node_import;
extern CVar* diag_rig_log_node_stats;
extern CVar* diag_truck_mass;
extern CVar* diag_envmap;
extern CVar* diag_videocameras;
extern CVar* diag_preset_terrain;
extern CVar* diag_preset_spawn_pos;
extern CVar* diag_preset_spawn_rot;
extern CVar* diag_preset_vehicle;
extern CVar* diag_preset_veh_config;
extern CVar* diag_preset_veh_enter;
extern CVar* diag_log_console_echo;
extern CVar* diag_log_beam_break;
extern CVar* diag_log_beam_deform;
extern CVar* diag_log_beam_trigger;
extern CVar* diag_simple_materials;
extern CVar* diag_warning_texture;
extern CVar* diag_hide_broken_beams;
extern CVar* diag_hide_beam_stress;
extern CVar* diag_hide_wheel_info;
extern CVar* diag_hide_wheels;
extern CVar* diag_hide_nodes;
extern CVar* diag_terrn_log_roads;
extern CVar* diag_actor_dump;
extern CVar* diag_allow_window_resize;

// System
extern CVar* sys_process_dir;
extern CVar* sys_user_dir;
extern CVar* sys_config_dir;
extern CVar* sys_cache_dir;
extern CVar* sys_thumbnails_dir;
extern CVar* sys_logs_dir;
extern CVar* sys_resources_dir;
extern CVar* sys_profiler_dir;
extern CVar* sys_savegames_dir;
extern CVar* sys_screenshot_dir;
extern CVar* sys_scripts_dir;
extern CVar* sys_projects_dir;

// OS command line
extern CVar* cli_server_host;
extern CVar* cli_server_port;
extern CVar* cli_preset_vehicle;
extern CVar* cli_preset_veh_config;
extern CVar* cli_preset_terrain;
extern CVar* cli_preset_spawn_pos;
extern CVar* cli_preset_spawn_rot;
extern CVar* cli_preset_veh_enter;
extern CVar* cli_force_cache_update;
extern CVar* cli_resume_autosave;
extern CVar* cli_custom_scripts;

// Input - Output
extern CVar* io_analog_smoothing;
extern CVar* io_analog_sensitivity;
extern CVar* io_blink_lock_range;
extern CVar* io_ffb_enabled;
extern CVar* io_ffb_camera_gain;
extern CVar* io_ffb_center_gain;
extern CVar* io_ffb_master_gain;
extern CVar* io_ffb_stress_gain;
extern CVar* io_input_grab_mode;
extern CVar* io_arcade_controls;
extern CVar* io_hydro_coupling;
extern CVar* io_outgauge_mode;
extern CVar* io_outgauge_ip;
extern CVar* io_outgauge_port;
extern CVar* io_outgauge_delay;
extern CVar* io_outgauge_id;
extern CVar* io_discord_rpc;
extern CVar* io_invert_orbitcam;

// Audio
extern CVar* audio_master_volume;
extern CVar* audio_enable_creak;
extern CVar* audio_device_name;
extern CVar* audio_menu_music;

// Graphics
extern CVar* gfx_flares_mode;
extern CVar* gfx_polygon_mode;
extern CVar* gfx_shadow_type;
extern CVar* gfx_extcam_mode;
extern CVar* gfx_sky_mode;
extern CVar* gfx_sky_time_cycle;
extern CVar* gfx_sky_time_speed;
extern CVar* gfx_texture_filter;
extern CVar* gfx_vegetation_mode;
extern CVar* gfx_water_mode;
extern CVar* gfx_anisotropy;
extern CVar* gfx_water_waves;
extern CVar* gfx_particles_mode;
extern CVar* gfx_enable_videocams;
extern CVar* gfx_window_videocams;
extern CVar* gfx_surveymap_icons;
extern CVar* gfx_declutter_map;
extern CVar* gfx_envmap_enabled;
extern CVar* gfx_envmap_rate;
extern CVar* gfx_shadow_quality;
extern CVar* gfx_skidmarks_mode;
extern CVar* gfx_sight_range;
extern CVar* gfx_camera_height;
extern CVar* gfx_fov_external;
extern CVar* gfx_fov_external_default;
extern CVar* gfx_fov_internal;
extern CVar* gfx_fov_internal_default;
extern CVar* gfx_static_cam_fov_exp;
extern CVar* gfx_fixed_cam_tracking;
extern CVar* gfx_fps_limit;
extern CVar* gfx_speedo_digital;
extern CVar* gfx_speedo_imperial;
extern CVar* gfx_flexbody_cache;
extern CVar* gfx_reduce_shadows;
extern CVar* gfx_enable_rtshaders;
extern CVar* gfx_alt_actor_materials;

// Flexbodies
extern CVar* flexbody_defrag_enabled;
extern CVar* flexbody_defrag_const_penalty;
extern CVar* flexbody_defrag_prog_up_penalty;
extern CVar* flexbody_defrag_prog_down_penalty;
extern CVar* flexbody_defrag_reorder_indices;
extern CVar* flexbody_defrag_reorder_texcoords;
extern CVar* flexbody_defrag_invert_lookup;

// GUI
extern CVar* ui_show_live_repair_controls;
extern CVar* ui_show_vehicle_buttons;

// ------------------------------------------------------------------------------------------------
// Global objects
// ------------------------------------------------------------------------------------------------

// Getters
AppContext*          GetAppContext();
ContentManager*      GetContentManager();
OverlayWrapper*      GetOverlayWrapper();
GUIManager*          GetGuiManager();
Console*             GetConsole();
InputEngine*         GetInputEngine();
CacheSystem*         GetCacheSystem();
MumbleIntegration*   GetMumble();
ThreadPool*          GetThreadPool();
CameraManager*       GetCameraManager();
GfxScene*            GetGfxScene();
SoundScriptManager*  GetSoundScriptManager();
LanguageEngine*      GetLanguageEngine();
ScriptEngine*        GetScriptEngine();
Network*             GetNetwork();
GameContext*         GetGameContext();
OutGauge*            GetOutGauge();
DiscordRpc*          GetDiscordRpc();

// Factories
void CreateOverlayWrapper();
void CreateGuiManager();
void CreateInputEngine();
void CreateMumble();
void CreateThreadPool();
void CreateCameraManager();
void CreateGfxScene();
void CreateSoundScriptManager();
void CreateScriptEngine();

// Cleanups
void DestroyOverlayWrapper();
void DestroyInputEngine();

} // namespace App

// ------------------------------------------------------------------------------------------------
// Exception handling
// ------------------------------------------------------------------------------------------------

enum HandleGenericExceptionFlags //!< Flags for `HandleGenericException()`
{
    HANDLEGENERICEXCEPTION_LOGFILE      = BITMASK(1), // Log to 'RoR.log'; Note OGRE exceptions are logged by OGRE but we still want the 'from' argument.
    HANDLEGENERICEXCEPTION_SCRIPTEVENT  = BITMASK(2), // Forward to script engine as game event `SE_GENERIC_EXCEPTION_CAUGHT`.
    HANDLEGENERICEXCEPTION_CONSOLE      = BITMASK(3), // Print to console, which auto-logs to RoR.log.

    HANDLEGENERICEXCEPTION_DEFAULT = HANDLEGENERICEXCEPTION_LOGFILE | HANDLEGENERICEXCEPTION_SCRIPTEVENT,
};

void HandleGenericException(const std::string& from, BitMask_t flags = HANDLEGENERICEXCEPTION_DEFAULT);

void HandleMsgQueueException(MsgType from);

// ------------------------------------------------------------------------------------------------
// Logging
// ------------------------------------------------------------------------------------------------

void          Log(const char* msg);               //!< The ultimate, application-wide logging function. Adds a line (any length) in 'RoR.log' file.
void          LogFormat(const char* format, ...); //!< Improved logging utility. Uses fixed 2Kb buffer.

} // namespace RoR

inline void   LOG(const char* msg)           { RoR::Log(msg); }         //!< Legacy alias - formerly a macro
inline void   LOG(std::string const & msg)   { RoR::Log(msg.c_str()); } //!< Legacy alias - formerly a macro

