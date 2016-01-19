
/** 
    @file
    @date   01/2016
    @author Petr Ohlidal
*/

#pragma once

#include "ForwardDeclarations.h"
#include "transmission.h"

#include <vector>

namespace RoR
{

/// Powertrain state
/// Each member replaces a member of legacy class BeamEngine (referenced in doc comments).
struct PowertrainState
{
    void Reset()
    {
        std::memset(this, 0, sizeof(PowertrainState));
    }

    /// m_curr_gear
    int transmission_current_gear; 

    /// int getNumGears() { return m_conf_gear_ratios.size() - 2; };
    int transmission_num_forward_gears;
    
    /// enum autoswitch m_autoselect
    Gearbox::autoswitch transmission_auto_shift_mode;

    /// float m_curr_clutch;
    float transmission_current_clutch;

    /// float m_curr_acc
    float current_acceleration;

    /// float m_conf_engine_max_rpm
    float conf_engine_max_rpm;

    /// float m_curr_engine_rpm
    float engine_current_rpm;

    /// Precomputed value of ```float BeamEngine::getTurboPSI()```
    float turbo_current_psi;

    /// bool m_starter_has_contact
    bool engine_starter_has_contact;

    /// Precomputed return of ```float BeamEngine::getTorque()```
    float engine_current_torque;

    /// bool isRunning() { return m_is_engine_running; };
    bool engine_is_running;

    /// float m_conf_clutch_force
    float conf_transmission_clutch_force;

    /// Precomputed ```float BeamEngine::getSmoke()```
    float gfx_engine_exhaust_smoke;

    /// bool hasTurbo() { return m_conf_engine_has_turbo; };
    bool conf_engine_has_turbo;

    /// Precalculated ```float BeamEngine::getCrankFactor()```
    float engine_crank_factor;

    /// m_conf_engine_min_rpm
    float conf_engine_min_rpm;

    /// int m_curr_gear_range
    int transmission_current_gear_range;

    /// int m_transmission_mode
    Gearbox::shiftmodes transmission_mode;

    /// Precomputed ```int getNumGearsRanges() { return getNumGears() / 6 + 1; };```
    int conf_transmission_num_gears_ranges;

    // Simthread -> mainthread updates

    bool transmission_shiftmodes_toggled;
    int  transmission_gear_range_selected; // 0=no change, 1=lowrange, 2=midrange, 3=highrange
    int  transmission_axlelock_toggled; // 0 = no change, -1 = `no differentials` console message, 1 = toggled, show console message.

};

struct PowertrainInputStates
{
    void Reset()
    {
        std::memset(this, 0, sizeof(PowertrainInputStates));
    }

    bool autoshift_up;
    bool autoshift_down;
    bool toggle_contact;
    bool starter;
    bool switch_shift_modes;
    bool shift_up;
    bool shift_down;
    bool shift_neutral;
    bool shift_lowrange;
    bool shift_midrange;
    bool shift_highrange;
    bool toggle_axle_lock;
    bool parking_brake_toggle;
    bool antilock_brake_toggle;
    bool traction_control_toggle;
    bool cruise_control_toggle;
    bool shift_gear_reverse;
    bool cruise_control_accel;
    bool cruise_control_decel;
    bool cruise_control_readjust;

    // Direct-to-gear shift
    bool direct_shift[18];

    float acceleration;
    float brake;
    float manual_clutch;
};

struct PowertrainCommandQueue
{
    PowertrainCommandQueue()
    {
        this->Reset();
    }

    void  NetworkedUpdate(float rpm, float force, float clutch, int gear, bool _running, bool _contact, char _automode);

    void  Reset();

    // Networked update
    float net_rpm;
    float net_force;
    float net_clutch;
    int   net_gear;
    bool  net_is_running;
    bool  net_has_contact;
    char  net_auto_mode;
    bool  was_network_updated;

    // Local update
    bool  command_auto_set_acc;
    float command_auto_set_acc_value;
    bool  command_offstart;
    bool  command_start;

    PowertrainInputStates input_states;
};

/// Land vehicle powertrain
class Powertrain
{
    friend class RigSpawner; // assigns BeamEngine* m_engine
public:
    Powertrain();

    ~Powertrain();

    void CriticalSection_SwapData()
    {
        m_main_thread_index = (m_main_thread_index + 1) % 2;
        m_sim_thread_index  = (m_sim_thread_index + 1)  % 2;
    }

    void PowertrainProcessInput();
    void PowertrainUpdate(float dt_seconds, bool just_synced);
    void PowertrainExportState();

    BeamEngine*             GetEngine()            { return m_engine; }
    PowertrainState&        GetStateOnMainThread() { return m_state[m_main_thread_index]; }
    PowertrainState&        GetStateOnSimThread()  { return m_state[m_sim_thread_index];  }
    PowertrainCommandQueue& GetQueueOnMainThread() { return m_command_queue[m_main_thread_index]; }
    PowertrainCommandQueue& GetQueueOnSimThread()  { return m_command_queue[m_sim_thread_index];  }

private:

    void PowertrainProcessInputStates();
    void UpdateCruiseControl(float dt_seconds);
    void CheckSpeedLimit(float dt_seconds);

    Beam*                    m_vehicle;
    BeamEngine*              m_engine;
    PowertrainState          m_state[2];
    PowertrainCommandQueue   m_command_queue[2];
    unsigned int             m_main_thread_index;
    unsigned int             m_sim_thread_index;
};

} // namespace RoR

