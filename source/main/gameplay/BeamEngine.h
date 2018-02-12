/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2013-2016 Petr Ohlidal & contributors

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
#include "RoRPrerequisites.h"


/// A land vehicle engine + transmission
/// NOTE: Formerly named 'BeamEngine' because 'Actor' was 'Beam'
class EngineSim : public ZeroedMemoryAllocator
{
    friend class RigSpawner;

public:

    EngineSim(float minRPM, float maxRPM, float torque, std::vector<float> gears, float dratio, Actor* actor);
    ~EngineSim();

    float getAcc();
    float getClutch();
    float getClutchForce();
    float getCrankFactor();
    float getRPM();
    float getSmoke();
    float getTorque();
    float getTurboPSI();
    RoR::SimGearboxMode getAutoMode();

    /**
    * Sets current engine state; Needed mainly for smoke.
    * @param rpm Current engine RPM
    * @param force Current acceleration force
    * @param clutch 
    * @param gear Current gear {-1 = reverse, 0 = neutral, 1...21 = forward}
    */
    void netForceSettings(float rpm, float force, float clutch, int gear, bool running, bool contact, char automode);

    void setAcc(float val);
    void setAutoMode(RoR::SimGearboxMode mode);
    void setClutch(float clutch);

    /**
    * Sets engine options.
    * @param einertia Engine inertia
    * @param etype Engine type {'t' = truck (default), 'c' = car}
    * @param eclutch
    * @param ctime Clutch time
    * @param stime Shift time
    * @param pstime Post-shift time
    * @param irpm Idle RPM
    * @param srpm Stall RPM
    * @param maximix Max. idle mixture
    * @param minimix Min. idle mixture
    */
    void setOptions(float einertia, char etype, float eclutch, float ctime, float stime, float pstime, float irpm, float srpm, float maximix, float minimix);

    /**
    * Sets turbo options.
    * @param tinertiatinertiaFactor Turbo inertia factor
    * @param nturbos Number of turbos
    * @param additionalTorque Torque that will be added to the engine at max turbo rpm
    **/
    void setTurboOptions(int type, float tinertiaFactor, int nturbos, float param1, float param2, float param3, float param4, float param5, float param6, float param7, float param8, float param9, float param10, float param11);

    void           SetEngineRpm(float rpm);      //!< Set current engine RPM.
    void           SetEnginePriming(bool p);     //!< Set current engine prime.
    void           setHydroPumpWork(float work); //!< Set current hydro pump work.
    void           setSpin(float rpm);           //!< Set current wheel spinning speed.
    void           toggleAutoMode();
    void           toggleContact();
    void           offstart();                   //!< Quick start of vehicle engine. Plays sounds.
    void           setstarter(bool v);           //!< Controls vehicle starter. No side effects.
    void           start();                      //!< Quick engine start. Plays sounds.
    int            getGear();                    //!< low level gear changing
    int            getGearRange();               //!< low level gear changing
    void           setGear(int v);               //!< low level gear changing
    void           setGearRange(int v);          //!< low level gear changing
    void           stop();                       //!< stall engine
    bool           hasContact() const       { return m_starter_has_contact; };
    bool           hasTurbo() const         { return m_engine_has_turbo; };
    bool           isRunning() const        { return m_engine_is_running; };
    char           getType() const          { return m_engine_type; };
    float          getEngineTorque() const  { return engineTorque; };
    float          getBrakingTorque() const { return m_braking_torque; };
    float          getIdleRPM() const       { return idleRPM; };
    float          getMaxRPM() const        { return maxRPM; };
    float          getMinRPM() const        { return minRPM; };
    int            getNumGears() const      { return m_gear_ratios.size() - 2; };
    int            getNumGearsRanges() const{ return getNumGears() / 6 + 1; };
    TorqueCurve*   getTorqueCurve()         { return torqueCurve; };
    float          getAccToHoldRPM(float rpm);
    float          getTurboPower();
    float          getEnginePower(float rpm);
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
    void           update(float dt, int doUpdate);
    void           updateAudio(int doUpdate);  //!< Updates sound effects (engine/turbo/clutch/etc...)

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

protected:

    enum shiftmodes
    {
        AUTOMATIC,
        SEMIAUTO,
        MANUAL,
        MANUAL_STICK,
        MANUAL_RANGES
    };

    // Vehicle
    Actor*         m_actor;
    float          m_abs_velocity;          //!< Vehicle; current velocity of the vehicle
    float          m_rel_velocity;          //!< Vehicle; current velocity of the vehicle along the long axis

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
    bool           m_engine_is_electric;    //!< attribute
    bool           m_starter_has_contact;   //!< Engine state
    bool           m_engine_has_air;        //!< Engine attribute
    bool           m_engine_has_turbo;      //!< Engine attribute
    int            m_engine_turbo_mode;     //!< Engine attribute
    bool           m_engine_is_running;     //!< Engine state
    char           m_engine_type;           //!< Engine attribute {'t' = truck (default), 'c' = car}
    float          m_braking_torque;        //!< Engine
    float          m_cur_acc;               //!< Engine
    float          m_cur_engine_rpm;        //!< Engine
    float          m_diff_ratio;            //!< Engine
    float engineTorque; //!< Engine
    float hydropump; //!< Engine
    float idleRPM; //!< Engine attribute
    float minIdleMixture; //!< Engine attribute
    float maxIdleMixture; //!< Engine attribute
    float inertia; //!< Engine attribute
    float maxRPM; //!< Engine attribute
    float minRPM; //!< Engine attribute
    float stallRPM; //!< Engine
    bool is_priming; //!< Engine

    // shifting
    float post_shift_time; //!< Shift attribute
    float postshiftclock;
    float shift_time; //!< Shift attribute
    float shiftclock;
    int postshifting;
    int shifting;
    int shiftval;

    // auto
    autoswitch autoselect;
    float autocurAcc;
    int starter;

    // auto transmission
    float fullRPMRange;
    float oneThirdRPMRange;
    float halfRPMRange;
    float shiftBehaviour;
    int upShiftDelayCounter;
    int kickdownDelayCounter;
    std::deque<float> rpms;
    std::deque<float> accs;
    std::deque<float> brakes;

    // turbo
    //Yeah i know, a bit dirty
    int turboVer;
#define MAXTURBO 4
    float curTurboRPM[MAXTURBO];
    float turboInertiaFactor;
    int numTurbos;
    int maxTurboRPM;
    float EngineAddiTorque[MAXTURBO];
    float turboEngineRpmOperation;
    float turboMaxPSI;
    bool b_BOV;
    float curBOVTurboRPM[MAXTURBO];
    int minBOVPsi;
    bool b_WasteGate;
    float minWGPsi;
    bool b_flutter;
    float wastegate_threshold_p, wastegate_threshold_n;
    bool b_anti_lag;
    float minRPM_antilag;
    float rnd_antilag_chance;
    float antilag_power_factor;

    // air pressure
    TorqueCurve* torqueCurve;
    float apressure;

    int automode; //!< Transmission mode (@see enum EngineSim::shiftmodes)
};
