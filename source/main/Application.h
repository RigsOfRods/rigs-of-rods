/*
    This source file is part of Rigs of Rods
    Copyright 2013-2018 Petr Ohlidal & contributors

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

#include "ForwardDeclarations.h"

#include <cstdio>
#include <cstring>
#include <string>

namespace RoR {


// ------------------------------------------------------------------------------------------------
// Global definitions and enumerations
// ------------------------------------------------------------------------------------------------


enum class AppState
{
    BOOTSTRAP,          ///< Initial state
    MAIN_MENU,
    CHANGE_MAP,         ///< Enter main menu & immediatelly launch singleplayer map selector.
    SIMULATION,
    SHUTDOWN,
    PRINT_HELP_EXIT,
    PRINT_VERSION_EXIT,
};
const char* ToStr(AppState v);

enum class MpState
{
    DISABLED,  ///< Not connected for whatever reason.
    CONNECTED,
};
const char* ToStr(MpState v);

enum class SimState
{
    OFF,
    RUNNING,
    PAUSED,
    SELECTING,  ///< The selector GUI window is displayed.
    EDITOR_MODE ///< Hacky, but whatever... added by Ulteq, 2016
};
const char* ToStr(SimState v);

enum class SimGearboxMode
{
    AUTO,          ///< Automatic shift
    SEMI_AUTO,     ///< Manual shift - Auto clutch
    MANUAL,        ///< Fully Manual: sequential shift
    MANUAL_STICK,  ///< Fully manual: stick shift
    MANUAL_RANGES, ///< Fully Manual: stick shift with ranges
};
const char* ToStr(SimGearboxMode v);

enum class GfxShadowType
{
    NONE,
    TEXTURE,
    PSSM
};
const char* ToStr(GfxShadowType v);

enum class GfxExtCamMode
{
    NONE,
    STATIC,
    PITCHING,
};
const char* ToStr(GfxExtCamMode v);

enum class GfxTexFilter
{
    NONE,
    BILINEAR,
    TRILINEAR,
    ANISOTROPIC,
};
const char* ToStr(GfxTexFilter v);

enum class GfxVegetation
{
    NONE,
    x20PERC,
    x50PERC,
    FULL,
};
const char* ToStr(GfxVegetation v);

enum class GfxFlaresMode
{
    NONE,                    ///< None (fastest)
    NO_LIGHTSOURCES,         ///< No light sources
    CURR_VEHICLE_HEAD_ONLY,  ///< Only current vehicle, main lights
    ALL_VEHICLES_HEAD_ONLY,  ///< All vehicles, main lights
    ALL_VEHICLES_ALL_LIGHTS, ///< All vehicles, all lights
};
const char* ToStr(GfxFlaresMode v);

enum class GfxWaterMode
{
    NONE,       ///< None
    BASIC,      ///< Basic (fastest)
    REFLECT,    ///< Reflection
    FULL_FAST,  ///< Reflection + refraction (speed optimized)
    FULL_HQ,    ///< Reflection + refraction (quality optimized)
    HYDRAX,     ///< HydraX
};
const char* ToStr(GfxWaterMode v);

enum class GfxSkyMode
{
    SANDSTORM,  ///< Sandstorm (fastest)
    CAELUM,     ///< Caelum (best looking, slower)
    SKYX,       ///< SkyX (best looking, slower)
};
const char* ToStr(GfxSkyMode v);

enum class IoInputGrabMode
{
    NONE,
    ALL,
    DYNAMIC,
};
const char* ToStr(IoInputGrabMode v);


// ------------------------------------------------------------------------------------------------
// Generic utilities
// ------------------------------------------------------------------------------------------------


/// Wrapper for classic c-string (local buffer)
/// Refresher: `strlen()` excludes '\0' terminator; `strncat()` Appends '\0' terminator
/// @author Petr Ohlidal, 2017
template<size_t L> class Str
{
public:
    // Constructors
    inline             Str()                                 { this->Clear(); }
    inline             Str(Str<L> const & src)               { this->Assign(src); }
    inline             Str(const char* src)                  { this->Assign(src); }

    // Reading
    inline const char* ToCStr() const                        { return m_buffer; }
    inline bool        IsEmpty() const                       { return m_buffer[0] == '\0'; }
    inline char*       GetBuffer()                           { return m_buffer; }
    inline size_t      GetCapacity() const                   { return m_capacity; }
    inline int         Compare(const char* str) const        { return std::strncmp(m_buffer, str, L); }
    inline size_t      GetLength() const                     { return std::strlen(m_buffer); }

    // Writing
    inline Str&        Clear()                               { std::memset(m_buffer, 0, L); return *this; }
    inline Str&        Assign(const char* src)               { this->Clear(); this->Append(src); return *this; }
    inline Str&        Append(const char* src)               { std::strncat(m_buffer, src, (L - (this->GetLength() + 1))); return *this; }
    inline Str&        Append(float f)                       { char buf[50]; std::snprintf(buf, 50, "%f", f); this->Append(buf); return *this; }
    inline Str&        Append(int i)                         { char buf[50]; std::snprintf(buf, 50, "%d", i); this->Append(buf); return *this; }
    inline Str&        Append(size_t z)                      { char buf[50]; std::snprintf(buf, 50, "%lu", static_cast<unsigned long>(z)); this->Append(buf); return *this; }
    inline Str&        Append(char c)                        { char buf[2] = {}; buf[0] = c; this->Append(buf); return *this; }

    // Operators
    inline             operator const char*() const          { return this->ToCStr(); }
    inline Str&        operator=  (const char* src)          { return this->Assign(src); }
    inline Str&        operator=  (std::string const& str)   { return this->Assign(str.c_str()); }
    inline Str&        operator<< (const char* src)          { return this->Append(src); }
    inline Str&        operator<< (std::string const& str)   { return this->Append(str.c_str()); }
    inline Str&        operator<< (float f)                  { return this->Append(f); }
    inline Str&        operator<< (int i)                    { return this->Append(i); }
    inline Str&        operator<< (size_t z)                 { return this->Append(z); }
    inline Str&        operator<< (char c)                   { return this->Append(c); }
    inline bool        operator== (const char* other) const  { return (this->Compare(other) == 0); }

private:
    char         m_buffer[L];
    const size_t m_capacity = L;
};


void                   Log(const char* msg);                    ///< The ultimate, application-wide logging function. Adds a line (any length) in 'RoR.log' file.
void                   LogFormat(const char* format, ...);      ///< Improved logging utility. Uses fixed 2Kb buffer.
inline const char*     ToStr(bool b)                        { return (b) ? "true" : "false"; }
inline const char*     ToStr(std::string const & s)         { return s.c_str(); }


// ------------------------------------------------------------------------------------------------
// Global variables
// ------------------------------------------------------------------------------------------------


/// A global variable - only for use in 'Application.(h/cpp)'
///
/// Concept: A GVar may contain 1-3 values:
///   * [A] Active value = the value currently in effect. Each GVar has an active value.
///   * [P] Pending value = the value to be set as active on earliest occasion (occasion may be anything from next frame to game restart)
///                         When no change is requested, value of Pending equals value of Active.
///                         Currently all GVars contain a pending value, but this design may be changed in the future.
///   * [S] Stored value = The user-defined value to be persisted in config file.
///                        - When not present, Active is taken as Stored.
///                        - When present, it's unaffected by Active and only changes when user wants.
/// GVar classes are named _A, _AP or _APS depending on which values they contain.
///
/// API usage:
///   SetPending():   new pending value, active stands. GVars without pending value don't have this function.
///   SetActive():    direct update of active (+pending, if present) value
///   ApplyPending(): updates active from pending. GVars without pending value don't have this function.
///   ResetPending(): updates pending from active. GVars without pending value don't have this function.
///   GetStored():    gets stored value, if present, or active value if not.
///   SetStored():    direct update of stored value. GVars without stored value don't have this function.
///
/// Usage guidelines:
///   * There are no definite rules how to use and update a GVar. Each one is specific.
///   * Each GVar should have an Owner - a piece of code which checks for 'pending' values and Apply()-ies them.
///      This may be any thread and any code location, but there should be just 1 per GVar.
///
struct GVarBase
{
public:
    GVarBase(const char* name, const char* conf_name):
        name(name), conf_name(conf_name)
    {}

    const char* const name;
    const char* const conf_name;

protected:

    void LogFormat(const char* format, ...) const;

    template <typename T> void Log(const char* action, T arg, T cur) const
    {
        this->LogFormat("[RoR|GVar]  %20s:  %s(), new: \"%s\", old: \"%s\"", name, action, ToStr(arg), ToStr(cur));
    }

    void Log(const char* action, int arg, int cur) const
    {
        this->LogFormat("[RoR|GVar]  %20s:  %s(), new: \"%d\", old: \"%d\"", name, action, arg, cur);
    }
    
    void Log(const char* action, float arg, float cur) const
    {
        this->LogFormat("[RoR|GVar]  %20s:  %s(), new: \"%f\", old: \"%f\"", name, action, arg, cur);
    }
};


/// _A = Has only 'Active' field
template <typename T> class GVar_A: public GVarBase
{
public:
    typedef T Value_t;

    GVar_A(const char* name, const char* conf, T active_val):
        GVarBase(name, conf), m_value_active(active_val)
    {}

    T GetActive() const
    {
        return m_value_active;
    }

    T GetStored() const
    {
        return m_value_active;
    }

    void SetActive(T val)
    {
        if (val != m_value_active)
        {
            this->Log("SetActive", val, m_value_active);
            m_value_active = val;
        }
    }

private:
    T m_value_active;
};


/// _AP = Has 'Active' and 'Pending' fields
template <typename T> class GVar_AP: public GVarBase
{
public:
    typedef T Value_t;

    GVar_AP(const char* name, const char* conf, T active_val, T pending_val):
        GVarBase(name, conf), m_value_pending(pending_val),m_value_active(active_val)
    {}

    T GetActive() const
    {
        return m_value_active;
    }

    T GetPending() const
    {
        return m_value_pending;
    }

    void SetPending(T val)
    {
        if (val != m_value_pending)
        {
            this->Log("SetPending", val, m_value_pending);
            m_value_pending = val;
        }
    }

    void SetActive(T val) 
    {
        if ((val != m_value_active) || (val != m_value_pending))
        {
            this->Log("SetActive", val, m_value_active);
            m_value_active = val;
            m_value_pending = val;
        }
    }

    void ApplyPending()
    {
        if (m_value_active != m_value_pending)
        {
            this->Log("ApplyPending", m_value_pending, m_value_active);
            m_value_active = m_value_pending;
        }
    }

    void ResetPending()
    {
        if (m_value_active != m_value_pending)
        {
            this->Log("ResetPending", m_value_pending, m_value_active);
            m_value_pending = m_value_active;
        }
    }

private:
    T m_value_active;
    T m_value_pending;
};


/// _APS = Has 'Active, Pending, Stored' fields
template <typename T> class GVar_APS: public GVarBase
{
public:
    typedef T Value_t;

    GVar_APS(const char* name, const char* conf, T active_val, T pending_val, T stored_val):
        GVarBase(name, conf), m_value_stored(stored_val), m_value_pending(pending_val), m_value_active(active_val)
    {}

    T GetActive() const
    {
        return m_value_active;
    }

    T GetPending() const
    {
        return m_value_pending;
    }

    T GetStored() const
    {
        return m_value_stored;
    }

    void SetPending(T val)
    {
        if (val != m_value_pending)
        {
            this->Log("SetPending", val, m_value_pending);
            m_value_pending = val;
        }
    }

    void SetActive(T val)
    {
        if ((val != m_value_active) || (val != m_value_pending))
        {
            this->Log("SetActive", val, m_value_active);
            m_value_active = val;
            m_value_pending = val;
        }
    }

    void ApplyPending()
    {
        if (m_value_active != m_value_pending)
        {
            this->Log("ApplyPending", m_value_pending, m_value_active);
            m_value_active = m_value_pending;
        }
    }

    void ResetPending()
    {
        if (m_value_active != m_value_pending)
        {
            this->Log("ResetPending", m_value_pending, m_value_active);
            m_value_pending = m_value_active;
        }
    }

    void SetStored(T val)
    {
        if (val != m_value_stored)
        {
            this->Log("SetStored", val, m_value_stored);
            m_value_stored = val;
        }
    }

private:
    T m_value_active;
    T m_value_pending;
    T m_value_stored;
};


namespace App {


// App
extern GVar_AP<AppState>        app_state;
extern GVar_A<std::string>      app_language;
extern GVar_A<std::string>      app_locale;
extern GVar_A<bool>             app_multithread;
extern GVar_AP<std::string>     app_screenshot_format;

// Simulation
extern GVar_AP<SimState>        sim_state;
extern GVar_AP<std::string>     sim_terrain_name;
extern GVar_A<bool>             sim_replay_enabled;
extern GVar_A<int>              sim_replay_length;
extern GVar_A<int>              sim_replay_stepping;
extern GVar_A<bool>             sim_hires_wheel_col;
extern GVar_A<bool>             sim_position_storage;
extern GVar_AP<SimGearboxMode>  sim_gearbox_mode;

// Multiplayer
extern GVar_AP<MpState>         mp_state;
extern GVar_A<bool>             mp_join_on_startup;
extern GVar_AP<std::string>     mp_server_host;
extern GVar_A<int>              mp_server_port;
extern GVar_A<std::string>      mp_server_password;
extern GVar_AP<std::string>     mp_player_name;
extern GVar_AP<std::string>     mp_player_token_hash;
extern GVar_AP<std::string>     mp_portal_url;

// Diagnostic
extern GVar_A<bool>             diag_trace_globals;
extern GVar_A<bool>             diag_rig_log_node_import;
extern GVar_A<bool>             diag_rig_log_node_stats;
extern GVar_A<bool>             diag_rig_log_messages;
extern GVar_A<bool>             diag_collisions;
extern GVar_A<bool>             diag_truck_mass;
extern GVar_A<bool>             diag_envmap;
extern GVar_A<bool>             diag_videocameras;
extern GVar_APS<std::string>    diag_preset_terrain;
extern GVar_A<std::string>      diag_preset_vehicle;
extern GVar_A<std::string>      diag_preset_veh_config;
extern GVar_A<bool>             diag_preset_veh_enter;
extern GVar_A<bool>             diag_log_console_echo;
extern GVar_A<bool>             diag_log_beam_break;
extern GVar_A<bool>             diag_log_beam_deform;
extern GVar_A<bool>             diag_log_beam_trigger;
extern GVar_A<bool>             diag_dof_effect;
extern GVar_AP<std::string>     diag_extra_resource_dir;

// System
extern GVar_A<std::string>      sys_process_dir;
extern GVar_A<std::string>      sys_user_dir;
extern GVar_A<std::string>      sys_config_dir;
extern GVar_A<std::string>      sys_cache_dir;
extern GVar_A<std::string>      sys_logs_dir;
extern GVar_A<std::string>      sys_resources_dir;
extern GVar_A<std::string>      sys_profiler_dir;
extern GVar_A<std::string>      sys_screenshot_dir;

// Input - Output
extern GVar_A<bool>             io_ffb_enabled;
extern GVar_A<float>            io_ffb_camera_gain;
extern GVar_A<float>            io_ffb_center_gain;
extern GVar_A<float>            io_ffb_master_gain;
extern GVar_A<float>            io_ffb_stress_gain;
extern GVar_AP<IoInputGrabMode> io_input_grab_mode;
extern GVar_A<bool>             io_arcade_controls;
extern GVar_A<int>              io_outgauge_mode;
extern GVar_A<std::string>      io_outgauge_ip;
extern GVar_A<int>              io_outgauge_port;
extern GVar_A<float>            io_outgauge_delay;
extern GVar_A<int>              io_outgauge_id;

// Audio
extern GVar_A<float>            audio_master_volume;
extern GVar_A<bool>             audio_enable_creak;
extern GVar_AP<std::string>     audio_device_name;
extern GVar_A<bool>             audio_menu_music;

// Graphics
extern GVar_AP<GfxFlaresMode>   gfx_flares_mode;
extern GVar_AP<GfxShadowType>   gfx_shadow_type;
extern GVar_AP<GfxExtCamMode>   gfx_extcam_mode;
extern GVar_AP<GfxSkyMode>      gfx_sky_mode;
extern GVar_AP<GfxTexFilter>    gfx_texture_filter;
extern GVar_AP<GfxVegetation>   gfx_vegetation_mode;
extern GVar_AP<GfxWaterMode>    gfx_water_mode;
extern GVar_A<bool>             gfx_enable_sunburn;
extern GVar_A<bool>             gfx_water_waves;
extern GVar_A<bool>             gfx_minimap_disabled;
extern GVar_A<int>              gfx_particles_mode;
extern GVar_A<bool>             gfx_enable_glow;
extern GVar_A<bool>             gfx_enable_hdr;
extern GVar_A<bool>             gfx_enable_dof;
extern GVar_A<bool>             gfx_enable_heathaze;
extern GVar_A<bool>             gfx_enable_videocams;
extern GVar_A<bool>             gfx_envmap_enabled;
extern GVar_A<int>              gfx_envmap_rate;
extern GVar_A<int>              gfx_skidmarks_mode;
extern GVar_A<float>            gfx_sight_range;
extern GVar_APS<float>          gfx_fov_external;
extern GVar_APS<float>          gfx_fov_internal;
extern GVar_A<int>              gfx_fps_limit;
extern GVar_A<bool>             gfx_speedo_digital;
extern GVar_A<bool>             gfx_speedo_imperial;
extern GVar_A<bool>             gfx_motion_blur;

// Getters
OgreSubsystem*       GetOgreSubsystem();
ContentManager*      GetContentManager();
OverlayWrapper*      GetOverlayWrapper();
SceneMouse*          GetSceneMouse();
GUIManager*          GetGuiManager();
Console*             GetConsole();
InputEngine*         GetInputEngine();
CacheSystem*         GetCacheSystem();
MainMenu*            GetMainMenu();
SimController*       GetSimController();
MumbleIntegration*   GetMumble();
TerrainManager*      GetSimTerrain();

// Factories
void StartOgreSubsystem();
void ShutdownOgreSubsystem();
void CreateContentManager();
void DestroyContentManager();
void CreateOverlayWrapper();
void DestroyOverlayWrapper();
void CreateSceneMouse();
void DeleteSceneMouse();
void CreateGuiManagerIfNotExists();
void DeleteGuiManagerIfExists();
void CreateInputEngine();
void CreateCacheSystem();
void CheckAndCreateMumble();

// Setters
void SetMainMenu             (MainMenu*          obj);
void SetSimController        (SimController*  obj);
void SetSimTerrain           (TerrainManager*    obj);


} // namespace App
} // namespace RoR

inline void          LOG(const char* msg)           { RoR::Log(msg); }         ///< Legacy alias - formerly a macro
inline void          LOG(std::string const & msg)   { RoR::Log(msg.c_str()); } ///< Legacy alias - formerly a macro

