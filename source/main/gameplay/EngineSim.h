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

#pragma once

/// @file

#include "Application.h"
#include "RefCountingObject.h"

#include <Ogre.h>
#include <vector>

namespace RoR {

/// @addtogroup Gameplay
/// @{

/// @addtogroup Trucks
/// @{

/// A land vehicle engine + transmission
class EngineSim : public RefCountingObject<EngineSim>
{
    friend class ActorSpawner;

public:

    EngineSim(float min_rpm, float max_rpm, float torque, float reverse_gear, float neutral_gear, std::vector<float> gears, float dratio, ActorPtr actor);
    ~EngineSim();


    void           SetEngineOptions(float einertia, char etype, float eclutch, float ctime, float stime, float pstime, float irpm, float srpm, float maximix, float minimix, float ebraking);
    void           SetTurboOptions(int type, float tinertiaFactor, int nturbos, float param1, float param2, float param3, float param4, float param5, float param6, float param7, float param8, float param9, float param10, float param11);
    
    /// @name Definition; keyword 'engine'
    /// @{
    float          getMinRPM() const { return m_engine_min_rpm; } //!< Shift down RPM ('engine' attr #1)
    float          getMaxRPM() const { return m_engine_max_rpm; } //!< Shift up RPM ('engine' attr #2)
    float          getEngineTorque() const { return m_engine_torque; } //!< Torque in N/m ('engine' attr #3)
    float          getDiffRatio() const { return m_diff_ratio; } //!< Global gear ratio ('engine' attr #4)
    float          getGearRatio(int pos); //!< -1=R, 0=N, 1... ('engine' attrs #5[R],#6[N],#7[1]...)
    int            getNumGears() const { return m_num_gears; }
    int            getNumGearsRanges() const { return getNumGears() / 6 + 1; }
    /// @}

    /// @name Definition, Keyword 'engoption'
    /// @{
    float          getEngineInertia() const { return m_engine_inertia; } //!< ('engoption' attr #1)
    char           getEngineType() const { return m_engine_type; } //!< 't' = truck (default), 'c' = car, 'e' = electric car ('engoption' attr #2)
    bool           isElectric() const { return m_engine_is_electric; }
    bool           hasAir() const { return m_engine_has_air; }
    bool           hasTurbo() const { return m_engine_has_turbo; }
    float          getClutchForce() const { return m_clutch_force; } //!< ('engoption' attr #3)
    float          getShiftTime() const { return m_shift_time; } //!< Time (in seconds) that it takes to shift ('engoption' attr #4)
    float          getClutchTime() const { return m_clutch_time; } //!< Time (in seconds) the clutch takes to apply ('engoption' attr #5)
    float          getPostShiftTime() const { return m_post_shift_time; } //!< Time (in seconds) until full torque is transferred ('engoption' attr #6)
    float          getStallRPM() const { return m_engine_stall_rpm; } //!< ('engoption' attr #7)
    float          getIdleRPM() const { return m_engine_idle_rpm; } //!< ('engoption' attr #8)
    float          getMaxIdleMixture() const { return m_max_idle_mixture; } //!< Maximum throttle to maintain the idle RPM ('engoption' attr #9)
    float          getMinIdleMixture() const { return m_min_idle_mixture; } //!< Minimum throttle to maintain the idle RPM ('engoption' attr #10)
    float          getBrakingTorque() const { return m_braking_torque; }
    /// @}

    /// @name Definition, Keyword 'torquecurve'
    /// @{
    TorqueCurve*   getTorqueCurve() { return m_torque_curve; }
    /// @}
    
    /// @name General state getters
    /// @{
    float          getAcc();
    float          getClutch() const { return m_cur_clutch; };
    float          getCrankFactor();
    float          getRPM() { return m_cur_engine_rpm; }
    float          getSmoke();
    float          getTorque();
    float          getTurboPSI();
    SimGearboxMode getAutoMode();
    int            getGear();
    int            getGearRange();
    bool           isRunning() { return m_engine_is_running; }
    bool           hasContact() { return m_contact; } //!< Ignition
    float          getCurEngineTorque() { return m_cur_engine_torque; }
    float          getInputShaftRPM() { return m_cur_wheel_revolutions * m_gear_ratios[m_cur_gear + 1]; }
    float          getDriveRatio() { return m_gear_ratios[m_cur_gear + 1]; }
    float          getEnginePower() { return getEnginePower(m_cur_engine_rpm); }
    float          getEnginePower(float rpm);
    float          getTurboPower();
    float          getIdleMixture();
    float          getPrimeMixture();
    int            getAutoShift();
    float          getAccToHoldRPM(); //!< estimate required throttle input to hold the current rpm
    float          getWheelSpin() const { return m_cur_wheel_revolutions; } //!< Wheel RPM
    /// @}

    /// @name Shifting diagnostic
    /// @{
    float          getPostShiftClock() { return m_post_shift_clock; }
    float          getShiftClock() { return m_shift_clock; }
    bool           isPostShifting() { return m_post_shifting != 0; }
    bool           isShifting() { return m_shifting != 0; }
    int            getShifTargetGear() { return m_shift_val; }
    float          getAutoShiftBehavior() { return m_shift_behaviour; }
    int            getUpshiftDelayCounter() { return m_upshift_delay_counter; }
    int            getKickdownDelayCounter() { return m_kickdown_delay_counter; }
    /// @}

    /// @name General state changes
    /// @{
    void           pushNetworkState(float engine_rpm, float acc, float clutch, int gear, bool running, bool contact, char auto_mode, char auto_select = -1);
    void           setAcc(float val);
    void           autoSetAcc(float val);
    void           setClutch(float clutch) { m_cur_clutch = clutch; }
    void           setRPM(float rpm);
    void           setWheelSpin(float rpm);
    void           setAutoMode(SimGearboxMode mode);
    void           setPrime(bool p);
    void           setHydroPump(float work);
    void           setManualClutch(float val);
    void           setTCaseRatio(float ratio);   //!< Set current transfer case gear (reduction) ratio
    void           toggleContact();              //!< Ignition
    void           offStart();                   //!< Quick start of vehicle engine.
    void           startEngine();                //!< Quick engine start. Plays sounds.
    void           stopEngine();                 //!< stall engine
    /// @}

    /// @name Shifting
    /// @{
    void           toggleAutoMode();
    void           autoShiftDown();
    void           autoShiftSet(int mode);
    void           autoShiftUp();
    void           setGear(int v);               //!< low level gear changing (bypasses shifting logic)
    void           setGearRange(int v);          //!< low level gear changing (bypasses shifting logic)
    void           shift(int val);               //!< Changes gear by a relative offset. Plays sounds.
    void           shiftTo(int val);             //!< Changes gear to given value. Plays sounds.
    /// @}

    /// @name Updates
    /// @{
    void           updateShifts();             //!< Changes gears. Plays sounds.
    void           UpdateEngineSim(float dt, int doUpdate);
    void           UpdateEngineAudio();
    void           UpdateInputEvents(float dt);
    /// @}

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

    // Vehicle
    ActorPtr         m_actor;

    // Gearbox
    float          m_ref_wheel_revolutions; //!< Gears; estimated wheel revolutions based on current vehicle speed along the longi. axis
    float          m_cur_wheel_revolutions; //!< Gears; measured wheel revolutions
    int            m_cur_gear;              //!< Gears; Current gear {-1 = reverse, 0 = neutral, 1...21 = forward} 
    int            m_cur_gear_range;        //!< Gears
    int            m_num_gears;             //!< Num. forward gears
    std::vector<float> m_gear_ratios;       //!< [R|N|1|...] ('engine' attrs #4[global],#5[R],#6[N],#7[1]...)

    // Clutch
    float          m_clutch_force;          //!< ('engoption' attr #3)
    float          m_clutch_time;           //!< Time (in seconds) the clutch takes to apply ('engoption' attr #5)
    float          m_cur_clutch;
    float          m_cur_clutch_torque;

    // Engine
    bool           m_engine_is_electric;    //!< Engine attribute
    bool           m_engine_has_air;        //!< Engine attribute
    bool           m_engine_has_turbo;      //!< Engine attribute
    int            m_engine_turbo_mode;     //!< Engine attribute
    bool           m_engine_is_running;     //!< Engine state
    char           m_engine_type;           //!< 't' = truck (default), 'c' = car ('engoption' attr #2)
    float          m_braking_torque;        //!< Engine attribute
    float          m_cur_acc;               //!< Engine
    float          m_cur_engine_rpm;        //!< Engine
    float          m_cur_engine_torque;     //!< Engine
    float          m_diff_ratio;            //!< Global gear ratio ('engine' attr #4) 
    float          m_tcase_ratio;           //!< Engine
    float          m_engine_torque;         //!< Torque in N/m ('engine' attr #3) 
    float          m_hydropump_state;       //!< Engine
    float          m_min_idle_mixture;      //!< Minimum throttle to maintain the idle RPM ('engoption' attr #10)
    float          m_max_idle_mixture;      //!< Maximum throttle to maintain the idle RPM ('engoption' attr #9)
    float          m_engine_inertia;        //!< ('engoption' attr #1)
    float          m_engine_max_rpm;        //!< Shift up RPM ('engine' attr #2)
    float          m_engine_min_rpm;        //!< Shift down RPM ('engine' attr #1)
    float          m_engine_idle_rpm;       //!< ('engoption' attr #8)
    float          m_engine_stall_rpm;      //!< ('engoption' attr #7)
    bool           m_engine_is_priming;     //!< Engine
    TorqueCurve*   m_torque_curve;
    float          m_air_pressure;

    // Ignition
    bool           m_contact;               //!< Ignition switch is in ON/RUN position.
    bool           m_starter;               //!< Ignition switch is in START position.

    // Shifting
    float          m_post_shift_time;       //!< Time (in seconds) until full torque is transferred ('engoption' attr #6)
    float          m_post_shift_clock;
    float          m_shift_time;            //!< Time (in seconds) that it takes to shift ('engoption' attr #4)
    float          m_shift_clock;
    int            m_post_shifting;
    int            m_shifting;
    int            m_shift_val;

    // Auto transmission
    SimGearboxMode m_auto_mode;
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
