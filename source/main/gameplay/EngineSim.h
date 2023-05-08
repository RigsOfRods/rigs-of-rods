/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2013-2020 Petr Ohlidal

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

#include "Application.h"
#include "Actor.h"

namespace RoR {

/// @addtogroup Gameplay
/// @{

/// @addtogroup Trucks
/// @{

/// A land vehicle engine + transmission
class EngineSim
{
    friend class ActorSpawner;

public:

    EngineSim(float min_rpm, float max_rpm, float torque, std::vector<float> gears, float dratio, ActorPtr actor);
    ~EngineSim();

    /// Sets current engine state;
    /// @param gear Current gear {-1 = reverse, 0 = neutral, 1...21 = forward}
    void PushNetworkState(float engine_rpm, float acc, float clutch, int gear, bool running, bool contact, char auto_mode, char auto_select=-1);

    /// Sets engine options.
    /// @param einertia Engine inertia
    /// @param etype Engine type {'t' = truck (default), 'c' = car}
    /// @param eclutch
    /// @param ctime Clutch time
    /// @param stime Shift time
    /// @param pstime Post-shift time
    /// @param irpm Idle RPM
    /// @param srpm Stall RPM
    /// @param maximix Max. idle mixture
    /// @param minimix Min. idle mixture
    void SetEngineOptions(float einertia, char etype, float eclutch, float ctime, float stime, float pstime, float irpm, float srpm, float maximix, float minimix, float ebraking);

    /// Sets turbo options.
    /// @param tinertiatinertiaFactor Turbo inertia factor
    /// @param nturbos Number of turbos
    /// @param additionalTorque Torque that will be added to the engine at max turbo rpm
    void SetTurboOptions(int type, float tinertiaFactor, int nturbos, float param1, float param2, float param3, float param4, float param5, float param6, float param7, float param8, float param9, float param10, float param11);

    void           SetAcceleration(float val);
    void           SetAutoMode(RoR::SimGearboxMode mode);
    void           SetClutch(float clutch);
    float          GetAcceleration();
    float          GetClutch();
    float          GetClutchForce();
    float          GetCrankFactor();
    float          GetSmoke();
    float          GetTorque();
    float          GetTurboPsi();
    RoR::SimGearboxMode GetAutoShiftMode();
    void           SetEngineRpm(float rpm);      //!< Set current engine RPM.
    void           SetEnginePriming(bool p);     //!< Set current engine prime.
    void           SetHydroPumpWork(float work); //!< Set current hydro pump work.
    void           SetWheelSpin(float rpm);      //!< Set current wheel spinning speed.
    void           SetTCaseRatio(float ratio);   //!< Set current transfer case gear (reduction) ratio
    void           ToggleAutoShiftMode();
    void           OffStart();                   //!< Quick start of vehicle engine.
    void           StartEngine();                //!< Quick engine start. Plays sounds.
    int            GetGear();                    //!< low level gear changing
    int            GetGearRange();               //!< low level gear changing
    void           SetGear(int v);               //!< low level gear changing
    void           SetGearRange(int v);          //!< low level gear changing
    void           StopEngine();                 //!< stall engine
    float          GetAccToHoldRPM();            //!< estimate required throttle input to hold the current rpm
    bool           HasTurbo() const         { return m_engine_has_turbo; };
    bool           isRunning() const        { return m_engine_is_running; };
    int            GetAutoMode() const      { return static_cast<int>(m_auto_mode); };
    char           GetEngineType() const    { return m_engine_type; };
    float          getIdleRPM() const       { return m_engine_idle_rpm; };
    float          getMaxRPM() const        { return m_engine_max_rpm; };
    float          getMinRPM() const        { return m_engine_min_rpm; };
    int            getNumGears() const      { return static_cast<int>(m_gear_ratios.size() - 2); };
    int            getNumGearsRanges() const{ return getNumGears() / 6 + 1; };
    TorqueCurve*   getTorqueCurve()         { return m_torque_curve; };
    float          GetEngineRpm() const     { return m_cur_engine_rpm; }
    float          GetEngineTorque() const  { return m_cur_engine_torque; }
    float          GetInputShaftRpm()       { return m_cur_wheel_revolutions * m_gear_ratios[m_cur_gear + 1]; };
    float          GetDriveRatio()          { return m_gear_ratios[m_cur_gear + 1]; };
    float          GetEngineInertia()       { return m_engine_inertia; };
    float          getEnginePower()         { return getEnginePower(m_cur_engine_rpm); };
    float          getEnginePower(float rpm);
    float          getTurboPower();
    float          getIdleMixture();
    float          getPrimeMixture();
    int            getAutoShift();
    void           autoSetAcc(float val);
    void           autoShiftDown();
    void           autoShiftSet(int mode);
    void           autoShiftUp();
    void           setManualClutch(float val);
    void           shift(int val);             //!< Changes gear by a relative offset. Plays sounds.
    void           shiftTo(int val);           //!< Changes gear to given value. Plays sounds.
    void           updateShifts();             //!< Changes gears. Plays sounds.
    void           UpdateEngineSim(float dt, int doUpdate);
    void           UpdateEngineAudio();
    void           UpdateInputEvents(float dt);

    // Ignition
    void           toggleContact();
    bool           hasContact() const { return m_contact; };

    enum autoswitch
    {
        REAR,
        NEUTRAL,
        DRIVE,
        TWO,
        ONE,
        MANUALMODE
    };

    enum turbomode
    {
        OLD,
        NEW
    };

private:

    enum shiftmodes
    {
        AUTOMATIC,
        SEMIAUTO,
        MANUAL,
        MANUAL_STICK,
        MANUAL_RANGES
    };

    // Vehicle
    ActorPtr         m_actor;

    // Gearbox
    float          m_ref_wheel_revolutions; //!< Gears; estimated wheel revolutions based on current vehicle speed along the longi. axis
    float          m_cur_wheel_revolutions; //!< Gears; measured wheel revolutions
    int            m_cur_gear;              //!< Gears; Current gear {-1 = reverse, 0 = neutral, 1...21 = forward} 
    int            m_cur_gear_range;        //!< Gears
    int            m_num_gears;             //!< Gears
    std::vector<float> m_gear_ratios;       //!< Gears

    // Clutch
    float          m_clutch_force;          //!< Clutch attribute
    float          m_clutch_time;           //!< Clutch attribute
    float          m_cur_clutch;
    float          m_cur_clutch_torque;

    // Engine
    bool           m_engine_is_electric;    //!< Engine attribute
    bool           m_engine_has_air;        //!< Engine attribute
    bool           m_engine_has_turbo;      //!< Engine attribute
    int            m_engine_turbo_mode;     //!< Engine attribute
    bool           m_engine_is_running;     //!< Engine state
    char           m_engine_type;           //!< Engine attribute {'t' = truck (default), 'c' = car}
    float          m_braking_torque;        //!< Engine attribute
    float          m_cur_acc;               //!< Engine
    float          m_cur_engine_rpm;        //!< Engine
    float          m_cur_engine_torque;     //!< Engine
    float          m_diff_ratio;            //!< Engine
    float          m_tcase_ratio;           //!< Engine
    float          m_engine_torque;         //!< Engine attribute
    float          m_hydropump_state;       //!< Engine
    float          m_min_idle_mixture;      //!< Engine attribute
    float          m_max_idle_mixture;      //!< Engine attribute
    float          m_engine_inertia;        //!< Engine attribute
    float          m_engine_max_rpm;        //!< Engine attribute
    float          m_engine_min_rpm;        //!< Engine attribute
    float          m_engine_idle_rpm;       //!< Engine attribute
    float          m_engine_stall_rpm;      //!< Engine attribute
    bool           m_engine_is_priming;     //!< Engine
    TorqueCurve*   m_torque_curve;
    float          m_air_pressure;

    // Ignition
    bool           m_contact;               //!< Ignition switch is in ON/RUN position.
    bool           m_starter;               //!< Ignition switch is in START position.

    // Shifting
    float          m_post_shift_time;       //!< Shift attribute
    float          m_post_shift_clock;
    float          m_shift_time;            //!< Shift attribute
    float          m_shift_clock;
    int            m_post_shifting;
    int            m_shifting;
    int            m_shift_val;

    // Auto transmission
    int            m_auto_mode; //!< Transmission mode (@see enum EngineSim::shiftmodes)
    autoswitch     m_autoselect;
    float          m_auto_cur_acc;
    float          m_full_rpm_range;
    float          m_one_third_rpm_range;
    float          m_half_rpm_range;
    float          m_shift_behaviour;
    int            m_upshift_delay_counter;
    int            m_kickdown_delay_counter;
    std::deque<float> m_rpms;
    std::deque<float> m_accs;
    std::deque<float> m_brakes;

    // Turbo
#define MAXTURBO 4
    int            m_turbo_ver;
    float          m_cur_turbo_rpm[MAXTURBO];
    float          m_turbo_inertia_factor;
    int            m_num_turbos;
    int            m_max_turbo_rpm;
    float          m_engine_addi_torque[MAXTURBO];
    float          m_turbo_engine_rpm_operation;
    bool           m_turbo_has_bov;
    float          m_turbo_bov_rpm[MAXTURBO];
    int            m_min_bov_psi;
    bool           m_turbo_has_wastegate;
    float          m_min_wastegate_psi;
    bool           m_turbo_flutters;
    float          m_turbo_wg_threshold_p;
    float          m_turbo_wg_threshold_n;
    bool           m_turbo_has_antilag;
    float          m_antilag_min_rpm;
    float          m_antilag_rand_chance;
    float          m_antilag_power_factor;
};

/// @} // addtogroup Trucks
/// @} // addtogroup Gameplay

} // namespace RoR
