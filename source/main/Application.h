/*
    This source file is part of Rigs of Rods
    Copyright 2013-2017 Petr Ohlidal & contributors

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
    SIMULATION,
    SHUTDOWN,
    PRINT_HELP_EXIT,
    PRINT_VERSION_EXIT,
};
const char* EnumToStr(AppState v);

enum class MpState
{
    DISABLED,  ///< Not connected for whatever reason.
    CONNECTED,
};
const char* EnumToStr(MpState v);

enum class SimState
{
    OFF,
    RUNNING,
    PAUSED,
    SELECTING,  ///< The selector GUI window is displayed.
    EDITOR_MODE ///< Hacky, but whatever... added by Ulteq, 2016
};
const char* EnumToStr(SimState v);

enum class SimGearboxMode
{
    AUTO,          ///< Automatic shift
    SEMI_AUTO,     ///< Manual shift - Auto clutch
    MANUAL,        ///< Fully Manual: sequential shift
    MANUAL_STICK,  ///< Fully manual: stick shift
    MANUAL_RANGES, ///< Fully Manual: stick shift with ranges
};
const char* EnumToStr(SimGearboxMode v);

enum class GfxShadowType
{
    NONE,
    TEXTURE,
    PSSM
};
const char* EnumToStr(GfxShadowType v);

enum class GfxExtCamMode
{
    NONE,
    STATIC,
    PITCHING,
};
const char* EnumToStr(GfxExtCamMode v);

enum class GfxTexFilter
{
    NONE,
    BILINEAR,
    TRILINEAR,
    ANISOTROPIC,
};
const char* EnumToStr(GfxTexFilter v);

enum class GfxVegetation
{
    NONE,
    x20PERC,
    x50PERC,
    FULL,
};
const char* EnumToStr(GfxVegetation v);

enum class GfxFlaresMode
{
    NONE,                    ///< None (fastest)
    NO_LIGHTSOURCES,         ///< No light sources
    CURR_VEHICLE_HEAD_ONLY,  ///< Only current vehicle, main lights
    ALL_VEHICLES_HEAD_ONLY,  ///< All vehicles, main lights
    ALL_VEHICLES_ALL_LIGHTS, ///< All vehicles, all lights
};
const char* EnumToStr(GfxFlaresMode v);

enum class GfxWaterMode
{
    NONE,       ///< None
    BASIC,      ///< Basic (fastest)
    REFLECT,    ///< Reflection
    FULL_FAST,  ///< Reflection + refraction (speed optimized)
    FULL_HQ,    ///< Reflection + refraction (quality optimized)
    HYDRAX,     ///< HydraX
};
const char* EnumToStr(GfxWaterMode v);

enum class GfxSkyMode
{
    SANDSTORM,  ///< Sandstorm (fastest)
    CAELUM,     ///< Caelum (best looking, slower)
    SKYX,       ///< SkyX (best looking, slower)
};
const char* EnumToStr(GfxSkyMode v);

enum class IoInputGrabMode
{
    NONE,
    ALL,
    DYNAMIC,
};
const char* EnumToStr(IoInputGrabMode v);


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
inline const char*     BoolToStr(bool b)                        { return (b) ? "true" : "false"; }


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
/// GVar classes are named _A, _AP or _APS depending on which values they contain. NOTE this concept is not fully implemented yet.
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
/// Implementation notes:
///   GVars use virtual functions (-> overhead: vtable per class, vptr per instance, function-ptr calls).
///   The choice was 1) duplicate code in A/AP/APS classes 2) virtual inheritance. Rationale:
///   * Call overhead is not considered a problem because virtuals only do non-frequent tasks:
///     setting value (user action) and retrieving 'Stored' value (user console action/saving config on game exit).
///   * Vtable pointer is not considered a problem because sane number of GVars is not more than ~250 (RoR has ~100 now).
struct GVarBase
{
public:
    GVarBase(const char* name, const char* conf_name):
        name(name), conf_name(conf_name)
    {}

    const char* const name;
    const char* const conf_name;

protected:

    static const char* LOG_FMT_S;
    static const char* LOG_FMT_D;
    static const char* LOG_FMT_F;

    void LogFormat(const char* format, ...) const;

    // arg = the new value, cur = the old value
    inline void LogSetPending(const char* arg, const char* cur) const     { this->LogFormat(LOG_FMT_S, name, "SetPending", arg, cur); }
    inline void LogSetPending(int         arg, int         cur) const     { this->LogFormat(LOG_FMT_D, name, "SetPending", arg, cur); }
    inline void LogSetPending(float       arg, float       cur) const     { this->LogFormat(LOG_FMT_F, name, "SetPending", arg, cur); }
    inline void LogSetPending(bool        arg, bool        cur) const     { this->LogSetPending(BoolToStr(arg), BoolToStr(cur)); }

    inline void LogSetActive(const char* arg, const char* cur) const      { this->LogFormat(LOG_FMT_S, name, "SetActive", arg, cur); }
    inline void LogSetActive(int         arg, int         cur) const      { this->LogFormat(LOG_FMT_D, name, "SetActive", arg, cur); }
    inline void LogSetActive(float       arg, float       cur) const      { this->LogFormat(LOG_FMT_F, name, "SetActive", arg, cur); }
    inline void LogSetActive(bool        arg, bool        cur) const      { this->LogSetActive(BoolToStr(arg), BoolToStr(cur)); }

    inline void LogSetStored(const char* arg, const char* cur) const      { this->LogFormat(LOG_FMT_S, name, "SetStored", arg, cur); }
    inline void LogSetStored(int         arg, int         cur) const      { this->LogFormat(LOG_FMT_D, name, "SetStored", arg, cur); }
    inline void LogSetStored(float       arg, float       cur) const      { this->LogFormat(LOG_FMT_F, name, "SetStored", arg, cur); }
    inline void LogSetStored(bool        arg, bool        cur) const      { this->LogSetStored(BoolToStr(arg), BoolToStr(cur)); }

    // p = 'Pending' value, a = 'Active' value
    inline void LogApplyPending(const char* p, const char* a) const       { this->LogFormat(LOG_FMT_S, name, "ApplyPending", p, a); }
    inline void LogApplyPending(int         p, int         a) const       { this->LogFormat(LOG_FMT_D, name, "ApplyPending", p, a); }
    inline void LogApplyPending(float       p, float       a) const       { this->LogFormat(LOG_FMT_F, name, "ApplyPending", p, a); }
    inline void LogApplyPending(bool        p, bool        a) const       { this->LogApplyPending(BoolToStr(p), BoolToStr(a)); }

    inline void LogResetPending(const char* p, const char* a) const       { this->LogFormat(LOG_FMT_S, name, "ResetPending", a, p); }
    inline void LogResetPending(int         p, int         a) const       { this->LogFormat(LOG_FMT_D, name, "ResetPending", a, p); }
    inline void LogResetPending(float       p, float       a) const       { this->LogFormat(LOG_FMT_F, name, "ResetPending", a, p); }
    inline void LogResetPending(bool        p, bool        a) const       { this->LogResetPending(BoolToStr(p), BoolToStr(a)); }
};


/// POD = Plain old data (C++ concept); _A = Has only 'Active' field
template <typename T> class GVarPod_A: public GVarBase
{
public:
    GVarPod_A(const char* name, const char* conf, T active_val):
        GVarBase(name, conf), m_value_active(active_val)
    {}

    virtual ~GVarPod_A() {}

    inline T GetActive() const
    {
        return m_value_active;
    }

    virtual T GetStored() const
    {
        return m_value_active;
    }

    virtual void SetActive(T val)
    {
        if (val != m_value_active)
        {
            GVarBase::LogSetActive(val, m_value_active);
            m_value_active = val;
        }
    }

protected:
    T m_value_active;
};


/// POD = Plain old data (C++ concept); _AP = Has 'Active' and 'Pending' fields
template <typename T> class GVarPod_AP: public GVarPod_A<T>
{
public:
    GVarPod_AP(const char* name, const char* conf, T active_val, T pending_val):
        GVarPod_A<T>(name, conf, active_val), m_value_pending(pending_val)
    {}

    virtual ~GVarPod_AP() {}

    inline T GetPending() const
    {
        return m_value_pending;
    }

    void SetPending(T val)
    {
        if (val != m_value_pending)
        {
            GVarBase::LogSetPending(val, m_value_pending);
            m_value_pending = val;
        }
    }

    virtual void SetActive(T val) override
    {
        if ((val != GVarPod_A<T>::m_value_active) || (val != m_value_pending))
        {
            GVarBase::LogSetActive(val, GVarPod_A<T>::m_value_active);
            GVarPod_A<T>::m_value_active = val;
            m_value_pending = val;
        }
    }

    void ApplyPending()
    {
        if (GVarPod_A<T>::m_value_active != m_value_pending)
        {
            GVarBase::LogApplyPending(m_value_pending, GVarPod_A<T>::m_value_active);
            GVarPod_A<T>::m_value_active = m_value_pending;
        }
    }

    void ResetPending()
    {
        if (GVarPod_A<T>::m_value_active != m_value_pending)
        {
            GVarBase::LogResetPending(m_value_pending, GVarPod_A<T>::m_value_active);
            m_value_pending = GVarPod_A<T>::m_value_active;
        }
    }

protected:
    T m_value_pending;
};


/// POD = Plain old data (C++ concept); _APS = Has 'Active, Pending, Stored' fields
template <typename T> class GVarPod_APS: public GVarPod_AP<T>
{
public:
    GVarPod_APS(const char* name, const char* conf, T active_val, T pending_val, T stored_val):
        GVarPod_AP<T>(name, conf, active_val, pending_val), m_value_stored(stored_val)
    {}

    virtual ~GVarPod_APS() {}

    virtual T GetStored() const override
    {
        return m_value_stored;
    }

    void SetStored(T val)
    {
        if (val != m_value_stored)
        {
            GVarBase::LogSetStored(val, m_value_stored);
            m_value_stored = val;
        }
    }

protected:
    T m_value_stored;
};


/// _A = Has only 'Active' field
template <typename E> class GVarEnum_A: public GVarBase
{
public:
    GVarEnum_A(const char* name, const char* conf, E active_val):
        GVarBase(name, conf),  m_value_active(active_val)
    {}

    virtual ~GVarEnum_A() {};

    inline E GetActive() const
    {
        return m_value_active;
    }

    virtual void SetActive(E val)
    {
        if (val != m_value_active)
        {
            GVarBase::LogSetActive(EnumToStr(val), EnumToStr(m_value_active)); // Conversion needed
            m_value_active = val;
        }
    }

protected:
    E m_value_active;
};


/// _AP = Has 'Active' and 'Pending' fields
template <typename E> class GVarEnum_AP: public GVarEnum_A<E>
{
public:
    GVarEnum_AP(const char* name, const char* conf, E active_val, E pending_val):
        GVarEnum_A<E>(name, conf, active_val), m_value_pending(pending_val)
    {}

    virtual ~GVarEnum_AP() {}

    inline E GetPending()
    {
        return m_value_pending;
    }

    void SetPending(E val)
    {
        if (val != m_value_pending)
        {
            GVarBase::LogSetPending(EnumToStr(val), EnumToStr(m_value_pending));
            m_value_pending = val;
        }
    }

    virtual void SetActive(E val) override
    {
        if ((val != GVarEnum_A<E>::m_value_active) || (val != m_value_pending))
        {
            GVarBase::LogSetPending(EnumToStr(val), EnumToStr(GVarEnum_A<E>::m_value_active));
            GVarEnum_A<E>::m_value_active = val;
            m_value_pending = val;
        }
    }

    void ApplyPending()
    {
        if (GVarEnum_A<E>::m_value_active != m_value_pending)
        {
            GVarBase::LogApplyPending(EnumToStr(m_value_pending), EnumToStr(GVarEnum_A<E>::m_value_active));
            GVarEnum_A<E>::m_value_active = m_value_pending;
        }
    }

    void ResetPending()
    {
        if (GVarEnum_A<E>::m_value_active != m_value_pending)
        {
            GVarBase::LogResetPending(EnumToStr(m_value_pending), EnumToStr(GVarEnum_A<E>::m_value_active));
            m_value_pending = GVarEnum_A<E>::m_value_active;
        }
    }

protected:
    E m_value_pending;
};


/// _APS = Has 'Active, Pending, Stored' fields
template <typename E> class GVarEnum_APS: public GVarEnum_AP<E>
{
public:
    GVarEnum_APS(const char* name, const char* conf, E active_val, E pending_val, E stored_val):
        GVarEnum_AP<E>(name, conf, active_val, pending_val), m_value_stored(stored_val)
    {}

    virtual ~GVarEnum_APS() {}

    void SetStored(E val)
    {
        if (val != m_value_stored)
        {
            GVarBase::LogSetStored(EnumToStr(val), EnumToStr(m_value_stored));
            m_value_stored = val;
        }
    }

protected:
    E m_value_stored;
};


template <size_t L> class GVarStr_A: public GVarBase
{
public:
    GVarStr_A(const char* name, const char* conf, const char* active_val):
        GVarBase(name, conf), m_value_active(active_val)
    {}

    virtual ~GVarStr_A() {}

    inline const char* GetActive() const
    {
        return m_value_active;
    }

    virtual const char* GetStored() const
    {
        return m_value_active;
    }

    virtual void SetActive(const char* val)
    {
        if (val != m_value_active)
        {
            GVarBase::LogSetActive(val, m_value_active);
            m_value_active = val;
        }
    }

    inline bool IsActiveEmpty() const
    {
        return m_value_active.IsEmpty();
    }

protected:
    Str<L> m_value_active;
};


/// _AP = Has 'Active' and 'Pending' fields
template <size_t L> class GVarStr_AP: public GVarStr_A<L>
{
public:
    GVarStr_AP(const char* name, const char* conf, const char* active_val, const char* pending_val):
        GVarStr_A<L>(name, conf, active_val), m_value_pending(pending_val)
    {}

    virtual ~GVarStr_AP() {}

    inline const char* GetPending() const
    {
        return m_value_pending;
    }

    inline bool IsPendingEmpty() const
    {
        return m_value_pending.IsEmpty();
    }

    void SetPending(const char* val)
    {
        if (val != m_value_pending)
        {
            GVarBase::LogSetPending(val, m_value_pending);
            m_value_pending = val;
        }
    }

    virtual void SetActive(const char* val) override
    {
        if ((val != GVarStr_A<L>::m_value_active) || (val != m_value_pending))
        {
            GVarBase::LogSetActive(val, GVarStr_A<L>::m_value_active);
            GVarStr_A<L>::m_value_active = val;
            m_value_pending = val;
        }
    }

    void ApplyPending()
    {
        if (GVarStr_A<L>::m_value_active != m_value_pending)
        {
            GVarBase::LogApplyPending(m_value_pending, GVarStr_A<L>::m_value_active);
            GVarStr_A<L>::m_value_active.Assign(m_value_pending);
        }
    }

    void ResetPending()
    {
        if (GVarStr_A<L>::m_value_active != m_value_pending)
        {
            GVarBase::LogResetPending(m_value_pending, GVarStr_A<L>::m_value_active);
            m_value_pending.Assign(GVarStr_A<L>::m_value_active);
        }
    }

protected:
    Str<L> m_value_pending;
};


/// _APS = Has 'Active, Pending, Stored' fields
template <size_t L> class GVarStr_APS: public GVarStr_AP<L>
{
public:
    GVarStr_APS(const char* name, const char* conf, const char* active_val, const char* pending_val, const char* stored_val):
        GVarStr_AP<L>(name, conf, active_val, pending_val), m_value_stored(stored_val)
    {}

    virtual ~GVarStr_APS() {}

    virtual const char* GetStored() const override
    {
        return m_value_stored;
    }

    void SetStored(const char* val)
    {
        if (val != m_value_stored)
        {
            GVarBase::LogSetStored(val, m_value_stored);
            m_value_stored = val;
        }
    }

protected:
    Str<L> m_value_stored;
};


namespace App {


// App
extern GVarEnum_AP<AppState>   app_state;
extern GVarStr_A<100>          app_language;
extern GVarStr_A<50>           app_locale;
extern GVarPod_A<bool>         app_multithread;
extern GVarStr_AP<50>          app_screenshot_format;

// Simulation
extern GVarEnum_AP<SimState>   sim_state;
extern GVarStr_AP<200>         sim_terrain_name;
extern GVarPod_A<bool>         sim_replay_enabled;
extern GVarPod_A<int>          sim_replay_length;
extern GVarPod_A<int>          sim_replay_stepping;
extern GVarPod_A<bool>         sim_hires_wheel_col;
extern GVarPod_A<bool>             sim_position_storage;
extern GVarEnum_AP<SimGearboxMode> sim_gearbox_mode;

// Multiplayer
extern GVarEnum_AP<MpState>    mp_state;
extern GVarPod_A<bool>         mp_join_on_startup;
extern GVarStr_AP<200>         mp_server_host;
extern GVarPod_A<int>          mp_server_port;
extern GVarStr_A<100>          mp_server_password;
extern GVarStr_AP<100>         mp_player_name;
extern GVarStr_AP<250>         mp_player_token_hash;
extern GVarStr_AP<400>         mp_portal_url;

// Diagnostic
extern GVarPod_A<bool>         diag_trace_globals;
extern GVarPod_A<bool>         diag_rig_log_node_import;
extern GVarPod_A<bool>         diag_rig_log_node_stats;
extern GVarPod_A<bool>         diag_rig_log_messages;
extern GVarPod_A<bool>         diag_collisions;
extern GVarPod_A<bool>         diag_truck_mass;
extern GVarPod_A<bool>         diag_envmap;
extern GVarPod_A<bool>         diag_videocameras;
extern GVarStr_APS<100>        diag_preset_terrain;
extern GVarStr_A<100>          diag_preset_spawn_pos;
extern GVarStr_A<100>          diag_preset_spawn_rot;
extern GVarStr_A<100>          diag_preset_vehicle;
extern GVarStr_A<100>          diag_preset_veh_config;
extern GVarPod_A<bool>         diag_preset_veh_enter;
extern GVarPod_A<bool>         diag_log_console_echo;
extern GVarPod_A<bool>         diag_log_beam_break;
extern GVarPod_A<bool>         diag_log_beam_deform;
extern GVarPod_A<bool>         diag_log_beam_trigger;
extern GVarPod_A<bool>         diag_dof_effect;
extern GVarStr_AP<300>         diag_extra_resource_dir;

// System
extern GVarStr_A<300>          sys_process_dir;
extern GVarStr_A<300>          sys_user_dir;
extern GVarStr_A<300>          sys_config_dir;
extern GVarStr_A<300>          sys_cache_dir;
extern GVarStr_A<300>          sys_logs_dir;
extern GVarStr_A<300>          sys_resources_dir;
extern GVarStr_A<300>          sys_profiler_dir;
extern GVarStr_A<300>          sys_screenshot_dir;

// Input - Output
extern GVarPod_A<bool>         io_ffb_enabled;
extern GVarPod_A<float>        io_ffb_camera_gain;
extern GVarPod_A<float>        io_ffb_center_gain;
extern GVarPod_A<float>        io_ffb_master_gain;
extern GVarPod_A<float>             io_ffb_stress_gain;
extern GVarEnum_AP<IoInputGrabMode> io_input_grab_mode;
extern GVarPod_A<bool>              io_arcade_controls;
extern GVarPod_A<int>          io_outgauge_mode;
extern GVarStr_A<50>           io_outgauge_ip;
extern GVarPod_A<int>          io_outgauge_port;
extern GVarPod_A<float>        io_outgauge_delay;
extern GVarPod_A<int>          io_outgauge_id;

// Audio
extern GVarPod_A<float>        audio_master_volume;
extern GVarPod_A<bool>         audio_enable_creak;
extern GVarStr_AP<100>         audio_device_name;
extern GVarPod_A<bool>         audio_menu_music;

// Graphics
extern GVarEnum_AP<GfxFlaresMode> gfx_flares_mode;
extern GVarEnum_AP<GfxShadowType> gfx_shadow_type;
extern GVarEnum_AP<GfxExtCamMode> gfx_extcam_mode;
extern GVarEnum_AP<GfxSkyMode>    gfx_sky_mode;
extern GVarEnum_AP<GfxTexFilter>  gfx_texture_filter;
extern GVarEnum_AP<GfxVegetation> gfx_vegetation_mode;
extern GVarEnum_AP<GfxWaterMode>  gfx_water_mode;
extern GVarPod_A<bool>            gfx_enable_sunburn;
extern GVarPod_A<bool>         gfx_water_waves;
extern GVarPod_A<bool>         gfx_minimap_disabled;
extern GVarPod_A<int>          gfx_particles_mode;
extern GVarPod_A<bool>         gfx_enable_glow;
extern GVarPod_A<bool>         gfx_enable_hdr;
extern GVarPod_A<bool>         gfx_enable_dof;
extern GVarPod_A<bool>         gfx_enable_heathaze;
extern GVarPod_A<bool>         gfx_enable_videocams;
extern GVarPod_A<bool>         gfx_envmap_enabled;
extern GVarPod_A<int>          gfx_envmap_rate;
extern GVarPod_A<int>          gfx_skidmarks_mode;
extern GVarPod_A<float>        gfx_sight_range;
extern GVarPod_APS<float>      gfx_fov_external;
extern GVarPod_APS<float>      gfx_fov_internal;
extern GVarPod_A<int>          gfx_fps_limit;
extern GVarPod_A<bool>         gfx_speedo_digital;
extern GVarPod_A<bool>         gfx_speedo_imperial;
extern GVarPod_A<bool>         gfx_motion_blur;

// Getters
OgreSubsystem*       GetOgreSubsystem();
ContentManager*      GetContentManager();
OverlayWrapper*      GetOverlayWrapper();
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

