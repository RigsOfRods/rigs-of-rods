/*
This source file is part of Rigs of Rods
Copyright 2005-2012 Pierre-Michel Ricordel
Copyright 2007-2012 Thomas Fischer

For more information, see http://www.rigsofrods.com/

Rigs of Rods is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License version 3, as
published by the Free Software Foundation.

Rigs of Rods is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Rigs of Rods.  If not, see <http://www.gnu.org/licenses/>.
*/
#pragma once
#ifndef __BeamEngine_H_
#define __BeamEngine_H_

#include "RoRPrerequisites.h"
#include <pthread.h>

/**
* Represents a virtual engine of a vehicle (not "engine" as in "physics engine").
*/
class BeamEngine : public ZeroedMemoryAllocator
{
	friend class RigSpawner;
	friend class RigInspector; // Debug utility class

public:

	BeamEngine(float m_conf_engine_min_rpm, float m_conf_engine_max_rpm, float torque, std::vector<float> gears, float dratio, int m_vehicle_index);
	~BeamEngine();

    // Full state logging for debugging LuaPowertrain
    void LogFullState();

	float getAcc();
	float getClutch();
	float getClutchForce();
	float getCrankFactor(bool must_lock = true);
	float getRPM();
	float getSmoke();
	float getTorque();
	float getTurboPSI(bool must_lock = true);
	int getAutoMode();

	/**
	* Sets current engine state; Needed mainly for smoke.
	* @param rpm Current engine RPM
	* @param force Current acceleration force
	* @param clutch 
	* @param gear Current gear {-1 = reverse, 0 = neutral, 1...21 = forward}
	* @param m_is_engine_running
	* @param m_starter_has_contact
	* @param m_transmission_mode
	*/
	void netForceSettings(float rpm, float force, float clutch, int gear, bool m_is_engine_running, bool m_starter_has_contact, char m_transmission_mode);

	void setAcc(float val);
	void setAutoMode(int mode);
	void setClutch(float clutch);

	/**
	* Sets engine options.
	* @param einertia Engine m_conf_engine_inertia
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
	* @param tinertiatinertiaFactor Turbo m_conf_engine_inertia factor
	* @param nturbos Number of turbos
	* @param additionalTorque Torque that will be added to the engine at max turbo rpm
	**/
	void setTurboOptions(int type, float tinertiaFactor, int nturbos, float param1, float param2, float param3, float param4, float param5, float param6, float param7, float param8, float param9, float param10, float param11);

	/**
	* Set current engine RPM.
	*/
	void setRPM(float rpm);

	/**
	* Set current wheel spinning speed.
	*/
	void setSpin(float rpm);

	void toggleAutoMode();
	void toggleContact();

	/**
	* Quick start of vehicle engine. Plays sounds.
	*/
	void offstart();

	/**
	* Controls vehicle m_starter_is_running. No side effects.
	* @param v 1 to run m_starter_is_running, 0 to stop it.
	*/
	void setstarter(int v);

	/**
	* Quick engine start. Plays sounds.
	*/
	void start();

	// low level gear changing
	int getGear();
	int getGearRange();
	void setGear(int v);
	void setGearRange(int v);
	
	// stall engine
	void stop();

	// high level controls
	bool hasContact() { return m_starter_has_contact; };
	bool hasTurbo() { return m_conf_engine_has_turbo; };
	bool isRunning() { return m_is_engine_running; };
	char getType() { return m_conf_engine_type; };
	float getAccToHoldRPM(float rpm);
	float CalcEnginePower(float rpm);
	float getEngineTorque() { return m_conf_engine_torque; };
	float CalcIdleMixture();
	float getIdleRPM() { return m_conf_engine_idle_rpm; };
	float getMaxRPM() { return m_conf_engine_max_rpm; };
	float getMinRPM() { return m_conf_engine_min_rpm; };
	float CalcPrimeMixture();
	int getAutoShift();
	int getNumGears() { return m_conf_gear_ratios.size() - 2; };
	int getNumGearsRanges() { return getNumGears() / 6 + 1; };
	TorqueCurve *getTorqueCurve() { return m_conf_engine_torque_curve; };
	void autoSetAcc(float val);
	void autoShiftDown();
	void autoShiftSet(int mode);
	void autoShiftUp();
	void setManualClutch(float val);
	inline void SetHydroPump(float value)        { m_engine_hydropump = value; }
	inline void SetPrime(int prime)              { m_prime = prime; }
	inline float GetTurboInertiaFactor() const   { return m_conf_turbo_inertia_factor; }
	inline bool  HasAir() const                  { return m_conf_engine_has_air; }
	inline void  ScaleEngineTorque(float scale)  { m_conf_engine_torque *= scale; }

	// Intentionally repetitive names, for easier code searching

	/**
	* Changes gear by a relative offset. Plays sounds.
	* @param shift_change_relative 1 = shift up by 1, -1 = shift down by 1
	*/
	void BeamEngineShift(int shift_change_relative, bool must_lock = true);
	
	/**
	* Changes gear to given value. Plays sounds.
	* @see BeamEngine::shift
	*/
	void BeamEngineShiftTo(int val, bool must_lock = true);

	void UpdateBeamEngine(float dt, int doUpdate);

	enum shiftmodes {AUTOMATIC, SEMIAUTO, MANUAL, MANUAL_STICK, MANUAL_RANGES};
	enum autoswitch {REAR, NEUTRAL, DRIVE, TWO, ONE, MANUALMODE};

protected:

	/**
	* Updates sound effects (engine/turbo/clutch/etc...)
	*/
	void UpdateBeamEngineAudio(int doUpdate);

	/**
	* Changes gears. Plays sounds.
	*/
	void UpdateBeamEngineShifts();

	void DEBUG_LogClassState(BeamEngine snapshot_before, float dt, int doUpdate);

	// ================================================================
	// VARIABLES
	// NOTE: "conf_" prefix means the value is read-only.
	// ================================================================

	// gear stuff
	float   m_ref_wheel_revolutions; //!< Gears; estimated wheel revolutions based on current vehicle speed along the long axis
	float   m_cur_wheel_revolutions; //!< Gears; measured wheel revolutions
	int     m_curr_gear; //!< Gears; Current gear {-1 = reverse, 0 = neutral, 1...21 = forward} 
	int     m_curr_gear_range; //!< Gears

	int                m_conf_num_gears; //!< Gears
	std::vector<float> m_conf_gear_ratios; //!< Gears

	// truck stuff
	float   m_abs_velocity; // Vehicle; current velocity of the vehicle
	float   m_rel_velocity; // Vehicle; current velocity of the vehicle along the long axis

	// clutch stuff
	float   m_conf_clutch_force; //!< Clutch attribute
	float   m_conf_clutch_time; //!< Clutch attribute
	float   m_curr_clutch;
	float   m_curr_clutch_torque;

	// engine stuff
	TorqueCurve* m_conf_engine_torque_curve;
	
	bool    m_conf_engine_has_air; //!< Engine attribute
	bool    m_conf_engine_has_turbo; //!< Engine attribute
	char    m_conf_engine_type; //!< Engine attribute {'t' = truck (default), 'c' = car}
	float   m_conf_engine_braking_torque; //!< Engine
	
	float      m_conf_engine_diff_ratio; //!< Engine
	float      m_conf_engine_torque; //!< Engine
	float      m_conf_engine_idle_rpm; //!< Engine attribute
	float      m_conf_engine_min_idle_mixture; //!< Engine attribute
	float      m_conf_engine_max_idle_mixture; //!< Engine attribute
	float      m_conf_engine_inertia; //!< Engine attribute
	float      m_conf_engine_max_rpm; //!< Engine attribute
	float      m_conf_engine_min_rpm; //!< Engine attribute
	float      m_conf_engine_stall_rpm; //!< Engine
	float      m_conf_shift_time; //!< Shift attribute
	float      m_conf_post_shift_time; //!< Shift attribute

	float      m_engine_hydropump; //!< Engine;
	float      m_post_shift_clock;
	float      m_shift_clock;
	bool       m_is_post_shifting;
	bool       m_is_shifting;
	int        m_curr_gear_change_relative; //!< 1 = shift up by 1, -1 = shift down by 1
	bool       m_is_engine_running; //!< Engine state
	bool       m_starter_has_contact; //!< Engine
	float      m_curr_acc; //!< Engine
	float      m_curr_engine_rpm; //!< Engine
	autoswitch m_autoselect;
	float      m_auto_curr_acc;
	bool       m_starter_is_running;
	int        m_prime;

	// auto transmission
	float             m_conf_autotrans_full_rpm_range;
	float             m_autotrans_curr_shift_behavior;
	int               m_autotrans_up_shift_delay_counter;
	std::deque<float> m_autotrans_rpm_buffer;
	std::deque<float> m_autotrans_acc_buffer;
	std::deque<float> m_autotrans_brake_buffer;

	// turbo
	#define   MAX_NUM_TURBOS 4
	int       m_conf_num_turbos;
	int       m_conf_turbo_version;
	float     m_conf_turbo_inertia_factor;
	int       m_conf_turbo_max_rpm;
	float     m_conf_turbo_max_psi;
	bool      m_conf_turbo_has_bov;
	int       m_conf_turbo_min_bov_psi;
	float     m_conf_turbo_addi_torque[MAX_NUM_TURBOS];
	float     m_conf_turbo_engine_rpm_operation;
	bool      m_conf_turbo_has_wastegate;
	float     m_conf_turbo_wg_min_psi;
	float     m_conf_turbo_wg_threshold_p;
	float     m_conf_turbo_wg_threshold_n;
	bool      m_conf_turbo_has_antilag;
	float     m_conf_turbo_antilag_min_rpm;
	float     m_conf_turbo_antilag_chance_rand;
	float     m_conf_turbo_antilag_power_factor;
	bool      m_conf_turbo_has_flutter;

	float     m_turbo_curr_rpm[MAX_NUM_TURBOS];
	float     m_turbo_torque;
	float     m_turbo_inertia;
	float     m_turbo_psi;
	float     m_turbo_cur_bov_rpm[MAX_NUM_TURBOS];
	float     m_turbo_bov_torque;
	
	// air pressure
	
	float m_air_pressure; //!< Sound effect only
	int   m_transmission_mode; //!< Transmission mode (@see enum BeamEngine::shiftmodes)

	int m_vehicle_index;

    pthread_mutex_t m_mutex;
};

#endif // __BeamEngine_H_
