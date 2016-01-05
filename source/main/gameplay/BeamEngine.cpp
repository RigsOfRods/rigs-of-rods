/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2013-2016 Petr Ohlidal

    For more information, see http://www.rigsofrods.com/

    Rigs of Rods is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 3, as
    published by the Free Software Foundation.

    Rigs of Rods is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Rigs of Rods. If not, see <http://www.gnu.org/licenses/>.
*/

#include "BeamEngine.h"

#include "BeamFactory.h"
#include "Scripting.h"
#include "SoundScriptManager.h"
#include "TorqueCurve.h"

using namespace Ogre;
using namespace RoR;

#ifdef PTW32_VERSION // Win32 pthreads
#   define PTHREAD_ID() reinterpret_cast<int32_t>(pthread_self().p)
#else
#   define PTHREAD_ID() -1
#endif

struct BeamEngineScopedLock
{
    BeamEngineScopedLock(pthread_mutex_t* mtx, const char* func_name, bool should_lock = true):
        m_mtx(mtx)
    {
        if (should_lock)
        {
            MUTEX_LOCK(mtx);
            m_is_locked = true;

            char msg[500];
            int32_t tid = PTHREAD_ID();
            sprintf(msg, "                           ## BeamEngine [thread: %d] %s", tid, func_name);
            LOG(msg);
        }
    }
    ~BeamEngineScopedLock()
    {
        if (m_is_locked)
        {
            MUTEX_UNLOCK(m_mtx);
        }
    }
private:
    pthread_mutex_t* m_mtx;
    bool m_is_locked;
};

#define SCOPED_LOCK(func_name) BeamEngineScopedLock beamengine_lock(&m_mutex, func_name);

#define SCOPED_LOCK_OPT(func_name, do_lock) BeamEngineScopedLock beamengine_lock(&m_mutex, func_name, do_lock);

BeamEngine::BeamEngine(float m_conf_engine_min_rpm, float m_conf_engine_max_rpm, float torque, std::vector<float> gears, float dratio, int m_vehicle_index) :
	  m_air_pressure(0.0f)
	, m_auto_curr_acc(0.0f)
	, m_transmission_mode(Gearbox::SHIFTMODE_AUTOMATIC)
	, m_autoselect(Gearbox::AUTOSWITCH_DRIVE)
	, m_conf_engine_braking_torque(-torque / 5.0f)
	, m_conf_clutch_force(10000.0f)
	, m_conf_clutch_time(0.2f)
	, m_starter_has_contact(false)
	, m_curr_acc(0.0f)
	, m_curr_clutch(0.0f)
	, m_curr_clutch_torque(0.0f)
	, m_prime(0)
	, m_curr_engine_rpm(0.0f)
	, m_curr_gear(0)
	, m_curr_gear_range(0)
	, m_cur_wheel_revolutions(0.0f)
	, m_conf_engine_diff_ratio(dratio)
	, m_conf_engine_torque(torque - m_conf_engine_braking_torque)
	, m_conf_gear_ratios(gears)
	, m_conf_engine_has_air(true)
	, m_conf_engine_has_turbo(false)
	, m_engine_hydropump(0.0f)
	, m_conf_engine_idle_rpm(std::min(m_conf_engine_min_rpm, 800.0f))
	, m_conf_engine_inertia(10.0f)
	, m_conf_engine_max_idle_mixture(0.2f)
	, m_conf_engine_max_rpm(std::abs(m_conf_engine_max_rpm))
	, m_conf_engine_min_idle_mixture(0.0f)
	, m_conf_engine_min_rpm(std::abs(m_conf_engine_min_rpm))
	, m_conf_num_gears((int)gears.size() - 2)
	, m_conf_post_shift_time(0.2f)
	, m_post_shift_clock(0.0f)
	, m_is_post_shifting(false)
	, m_is_engine_running(false)
	, m_autotrans_curr_shift_behavior(0.0f)
	, m_conf_shift_time(0.5f)
	, m_shift_clock(0.0f)
	, m_is_shifting(0)
	, m_curr_gear_change_relative(0)
	, m_conf_engine_stall_rpm(300.0f)
	, m_starter_is_running(0)
	, m_conf_engine_torque_curve(new TorqueCurve())
	, m_vehicle_index(m_vehicle_index)
	, m_conf_engine_type('t')
	, m_autotrans_up_shift_delay_counter(0)
	, m_conf_turbo_inertia_factor(1)
	, m_conf_num_turbos(1)
	, m_conf_turbo_max_rpm(200000.0f)
	, m_conf_turbo_engine_rpm_operation(0.0f)
	, m_conf_turbo_version(1)
	, m_conf_turbo_min_bov_psi(11)
	, m_conf_turbo_wg_min_psi(20)
	, m_conf_turbo_has_wastegate(false)
	, m_conf_turbo_has_bov(false)
	, m_conf_turbo_has_flutter(false)
	, m_conf_turbo_wg_threshold_p(0)
	, m_conf_turbo_wg_threshold_n(0)
	, m_conf_turbo_has_antilag(false)
	, m_conf_turbo_antilag_chance_rand(0.9975)
	, m_conf_turbo_antilag_min_rpm(3000)
	, m_conf_turbo_antilag_power_factor(170)
{
	m_conf_autotrans_full_rpm_range = (m_conf_engine_max_rpm - m_conf_engine_min_rpm);

	m_conf_gear_ratios[0] = -m_conf_gear_ratios[0];
	for (std::vector< float >::iterator it = m_conf_gear_ratios.begin(); it != m_conf_gear_ratios.end(); ++it)
	{
		(*it) *= m_conf_engine_diff_ratio;
	}

	for (int i = 0; i < MAX_NUM_TURBOS; i++)
	{ 
		m_conf_turbo_addi_torque[i] = 0;
		m_turbo_curr_rpm[i] = 0;
	}

    pthread_mutex_init(&m_mutex, NULL);
}

BeamEngine::~BeamEngine()
{
	// delete NULL is safe
	delete m_conf_engine_torque_curve;
	m_conf_engine_torque_curve = NULL;
    pthread_mutex_destroy(&m_mutex);
}

// Full state logging for debugging LuaPowertrain
// The output is intended for comparsion by DIFF tool, not for reading by human
const char* str_bool(bool val) { return val ? " TRUE" : "FALSE"; }
void BeamEngine::LogFullState()
{
    char msg[6000];
    
    sprintf(msg,"DBG Powertrain "
        "| ref_wheel_revolutions: "            "%12.2f "
        "| cur_wheel_revolutions: "            "%12.2f "
        "| curr_gear: "                        "%9d.00 "
        "| curr_gear_range: "                  "%9d.00 "
        "| abs_velocity: "                     "%12.2f "
        "| rel_velocity: "                     "%12.2f "
        "| vehicle_index: "                    "%9d.00 "
        "| curr_clutch: "                      "%12.2f "
        "| curr_clutch_torque: "               "%40.2f "
        "| engine_hydropump: "                 "%12.2f "
        "| post_shift_clock: "                 "%12.2f "
        "| shift_clock: "                      "%12.2f "
        "| conf_shift_time: "                  "%12.2f "
        "| is_post_shifting: "                 "%s "
        "| is_engine_running: "                "%s "
        "| curr_gear_change_relative: "        "%9d.00 "
        "| air_pressure: "                     "%12.2f "
        "| auto_curr_acc: "                    "%12.2f "
        "| transmission_mode: "                "%9d.00 "
        "| autoselect: "                       "%9d.00 "
        "| starter_has_contact: "              "%s "
        "| starter_is_running: "               "%s "
        "| curr_acc: "                         "%12.2f "
        "| prime: "                            "%9d.00 "
        "| curr_engine_rpm: "                  "%40.2f "
        "| is_shifting: "                      "%s "
        "| autotrans_curr_shift_behavior: "    "%12.2f "
        "| autotrans_up_shift_delay_counter: " "%9d.00 "
        "| conf_autotrans_full_rpm_range: "    "%12.2f "
        "| turbo_curr_rpm[0]: "                "%12.2f "
        "| turbo_torque: "                     "%12.2f "
        "| turbo_inertia: "                    "%12.2f "
        "| turbo_psi: "                        "%12.2f "
        "| turbo_bov_torque: "                 "%12.2f "
        "| air_pressure: "                     "%12.2f "
        "| conf_turbo_has_wastegate "          "%s "
        "| conf_turbo_has_bov "                "%s "
        "| conf_turbo_has_flutter "            "%s "
        "| conf_turbo_has_antilag "            "%s "
        "| conf_engine_has_air "               "%s "
        "| conf_engine_has_turbo "             "%s "
        "| conf_num_turbos "                   "%9d.00 "
        "| conf_turbo_max_rpm "                "%9d.00 "
        "| conf_turbo_engine_rpm_operation "   "%12.2f "
        "| conf_turbo_version "                "%9d.00 "
        "| conf_turbo_min_bov_psi "            "%9d.00 "
        "| conf_turbo_wg_max_psi "             "%12.2f "
        "| conf_turbo_wg_threshold_p "         "%12.2f "
        "| conf_turbo_wg_threshold_n "         "%12.2f "
        "| conf_turbo_antilag_chance_rand "    "%12.2f "
        "| conf_turbo_antilag_min_rpm "        "%12.2f "
        "| conf_turbo_antilag_power_factor "   "%12.2f "
        "| conf_turbo_max_psi "                "%12.2f "
        "| conf_turbo_inertia_factor "         "%12.2f "
        "| conf_engine_diff_ratio "            "%12.2f "
        "| conf_engine_torque "                "%12.2f "
        "| conf_clutch_force "                 "%12.2f "
        "| conf_clutch_time "                  "%12.2f "
        "| conf_engine_idle_rpm "              "%12.2f "
        "| conf_engine_inertia "               "%12.2f "
        "| conf_engine_max_idle_mixture "      "%12.2f "
        "| conf_engine_max_rpm "               "%12.2f "
        "| conf_engine_min_idle_mixture "      "%12.2f "
        "| conf_engine_min_rpm "               "%12.2f "
        "| conf_engine_braking_torque "        "%12.2f "
        "| conf_engine_torque "                "%12.2f "
        "| conf_engine_stall_rpm "             "%12.2f "
        "| conf_post_shift_time "              "%12.2f "
        "| engine_type: "                      "%c",

        m_ref_wheel_revolutions,
        m_cur_wheel_revolutions,
        m_curr_gear, // int
        m_curr_gear_range, // int
        m_abs_velocity,
        m_rel_velocity,
        m_vehicle_index, // int
        m_curr_clutch,
        m_curr_clutch_torque,
        m_engine_hydropump,
        m_post_shift_clock,
        m_shift_clock,
        m_conf_shift_time,
        str_bool(m_is_post_shifting),
        str_bool(m_is_engine_running),
        m_curr_gear_change_relative, // int
        m_air_pressure,
        m_auto_curr_acc,
        m_transmission_mode, // int
        (int)m_autoselect, // int
        str_bool(m_starter_has_contact),
        str_bool(m_starter_is_running),
        m_curr_acc,
        m_prime, // int
        m_curr_engine_rpm,
        str_bool(m_is_shifting),
        m_autotrans_curr_shift_behavior,
        m_autotrans_up_shift_delay_counter, // int
        m_conf_autotrans_full_rpm_range,
        m_turbo_curr_rpm[0],
        m_turbo_torque,
        m_turbo_inertia,
        m_turbo_psi,
        m_turbo_bov_torque,
        m_air_pressure,
        
        str_bool(m_conf_turbo_has_wastegate),
        str_bool(m_conf_turbo_has_bov),
        str_bool(m_conf_turbo_has_flutter),
        str_bool(m_conf_turbo_has_antilag),
        str_bool(m_conf_engine_has_air),
        str_bool(m_conf_engine_has_turbo),

        m_conf_num_turbos, // int
        m_conf_turbo_max_rpm, // int
        m_conf_turbo_engine_rpm_operation,
        m_conf_turbo_version, // int
        m_conf_turbo_min_bov_psi, // int
        m_conf_turbo_wg_min_psi, // or MAX psi? It's uncertain what exactly this value represents ~ only_a_ptr, 12/2015
        m_conf_turbo_wg_threshold_p,
        m_conf_turbo_wg_threshold_n,
        m_conf_turbo_antilag_chance_rand,
        m_conf_turbo_antilag_min_rpm,
        m_conf_turbo_antilag_power_factor,
        m_conf_turbo_max_psi,
        m_conf_turbo_inertia_factor,
        m_conf_engine_diff_ratio,
        m_conf_engine_torque,
        m_conf_clutch_force,
        m_conf_clutch_time,
        m_conf_engine_idle_rpm,
        m_conf_engine_inertia,
        m_conf_engine_max_idle_mixture,
        m_conf_engine_max_rpm,
        m_conf_engine_min_idle_mixture,
        m_conf_engine_min_rpm,
        m_conf_engine_braking_torque,
        m_conf_engine_torque,
        m_conf_engine_stall_rpm,
        m_conf_post_shift_time,
        m_conf_engine_type // char
        );

    LOG(msg);
}



void BeamEngine::setTurboOptions(int type, float tinertiaFactor, int nturbos, float param1, float param2, float param3, float param4, float param5, float param6, float param7, float param8, float param9, float param10, float param11)
{
	if (!m_conf_engine_has_turbo)
		m_conf_engine_has_turbo = true; //Should have a turbo

	if (nturbos > MAX_NUM_TURBOS)
	{
		m_conf_num_turbos = 4;
		LOG("Turbo: No more than 4 turbos allowed"); //TODO: move this under RigParser
	} else
		m_conf_num_turbos = nturbos;

	m_conf_turbo_version = type;
	m_conf_turbo_inertia_factor = tinertiaFactor;

	if (param2 != 9999)
		m_conf_turbo_engine_rpm_operation = param2;
	

	if (m_conf_turbo_version == 1)
	{
		for (int i = 0; i < m_conf_num_turbos; i++)
			m_conf_turbo_addi_torque[i] = param1 / m_conf_num_turbos; //Additional torque
	}
	else
	{
		m_conf_turbo_max_psi = param1; //maxPSI
		m_conf_turbo_max_rpm = m_conf_turbo_max_psi * 10000;

		//Duh
		if (param3 == 1)
			m_conf_turbo_has_bov = true;
		else
			m_conf_turbo_has_bov = false;

		if (param3 != 9999)
			m_conf_turbo_min_bov_psi = param4;

		if (param5 == 1)
			m_conf_turbo_has_wastegate = true;
		else
			m_conf_turbo_has_wastegate = false;

		if (param6 != 9999)
			m_conf_turbo_wg_min_psi = param6 * 10000;

		if (param7 != 9999)
		{
			m_conf_turbo_wg_threshold_n = 1 - param7;
			m_conf_turbo_wg_threshold_p = 1 + param7;
		}

		if (param8 == 1)
			m_conf_turbo_has_antilag = true;
		else
			m_conf_turbo_has_antilag = false;

		if (param9 != 9999)
			m_conf_turbo_antilag_chance_rand = param9;

		if (param10 != 9999)
			m_conf_turbo_antilag_min_rpm = param10;

		if (param11 != 9999)
			m_conf_turbo_antilag_power_factor = param11;
	}
}

void BeamEngine::setOptions(float einertia, char etype, float eclutch, float ctime, float stime, float pstime, float irpm, float srpm, float maximix, float minimix)
{
	m_conf_engine_inertia = einertia;
	m_conf_engine_type = etype;
	m_conf_clutch_force = eclutch;

	if (ctime > 0)  m_conf_clutch_time = ctime;
	if (pstime > 0) m_conf_post_shift_time = pstime;
	if (stime > 0)  m_conf_shift_time = stime;
	if (irpm > 0) m_conf_engine_idle_rpm = irpm;
	if (srpm > 0) m_conf_engine_idle_rpm = srpm;
	if (maximix > 0) m_conf_engine_max_idle_mixture = maximix;
	if (minimix > 0) m_conf_engine_min_idle_mixture = minimix;

	if (etype == 'c')
	{
		// it's a car!
		m_conf_engine_has_air = false;
		// set default clutch force
		if (m_conf_clutch_force < 0.0f)
		{
			m_conf_clutch_force = 5000.0f;
		}
	}
	else if (etype == 'e') //electric
	{
		m_conf_engine_has_air = false;
		if (m_conf_clutch_force < 0.0f)
		{
			m_conf_clutch_force = 5000.0f;
		}
	}
	else
	{
		// it's a truck
		if (m_conf_clutch_force < 0.0f)
		{
			m_conf_clutch_force = 10000.0f;
		}
	}
}

void DBG_LogProjectedLuaArguments(Beam* truck, int doUpdate)
{
    // Log projected Lua arguments
    // node0_velocity ~~ [number] ~~ truck->nodes[0].Velocity.length();
    // 
    // hdir_velocity ~~ [number | nil] ~~ "velocity" below (NOTE: I have no idea what the calculation does ~ only_a_ptr, 12/2015)
    //     if (truck->cameranodepos[0] >= 0 && truck->cameranodedir[0] >=0)
    //     {
    //         Vector3 hdir = (truck->nodes[truck->cameranodepos[0]].RelPosition - truck->nodes[truck->cameranodedir[0]].RelPosition).normalisedCopy();
    //         velocity = hdir.dotProduct(truck->nodes[0].Velocity);
    //     }
    // 
    // wheel0_radius ~~ [number] ~~ truck->wheels[0].radius
    // 
    // vehicle_brake_force ~~ [number] ~~ truck->brakeforce
    // vehicle_brake_ratio ~~ [number] ~~ truck->brake

    char lua_arg_msg[1000];
    float hdir_velocity = -1.f;
    if (truck->cameranodepos[0] >= 0 && truck->cameranodedir[0] >=0)
    {
        Vector3 hdir = (truck->nodes[truck->cameranodepos[0]].RelPosition - truck->nodes[truck->cameranodedir[0]].RelPosition).normalisedCopy();
        hdir_velocity = hdir.dotProduct(truck->nodes[0].Velocity);
    }
    sprintf(lua_arg_msg, "do_update: %s | node0_velocity: %12.2f | hdir_velocity: %12.2f "
        "| wheel0_radius: %12.2f | vehicle_brake_force: %12.2f | vehicle_brake_ratio: %12.2f",
        (doUpdate == 1) ? " TRUE" : "FALSE",
        truck->nodes[0].Velocity.length(),
        hdir_velocity,
        truck->wheels[0].radius,
        truck->brakeforce,
        truck->brake);
    LOG(lua_arg_msg);
}

// TIGHT-LOOP: Called at least once per frame.
void BeamEngine::UpdateBeamEngine(float dt, int doUpdate)
{
    SCOPED_LOCK("UpdateBeamEngine() ******************************************************* ")
    Beam* truck = BeamFactory::getSingleton().getTruck(this->m_vehicle_index);

    // DBG_LogProjectedLuaArguments(truck, doUpdate);
    //this->LogFullState();


	if (!truck) return;

	float acc = this->m_curr_acc;
	bool engine_is_electric = (m_conf_engine_type == 'e');

    float idle_mixture = CalcIdleMixture();
    float prime_mixture = CalcPrimeMixture();

	acc = std::max(idle_mixture, acc);
    float DBG_acc_snap1 = acc;
	acc = std::max(prime_mixture, acc);
    float DBG_acc_snap2 = acc;

	if (doUpdate)
	{
#ifdef USE_OPENAL
		SoundScriptManager::getSingleton().modulate(m_vehicle_index, SS_MOD_INJECTOR, acc);
#endif // USE_OPENAL
	}

	if (m_conf_engine_has_air)
	{
		// air pressure
		m_air_pressure += dt * m_curr_engine_rpm;
		if (m_air_pressure > 50000.0f)
		{
#ifdef USE_OPENAL
			SoundScriptManager::getSingleton().trigOnce(m_vehicle_index, SS_TRIG_AIR_PURGE);
#endif // USE_OPENAL
			m_air_pressure = 0.0f;
		}
	}

	if (m_conf_engine_has_turbo)
	{
		for (int i = 0; i < m_conf_num_turbos; i++)
		{
			// update turbo speed (lag)
			// reset each of the values for each turbo
			m_turbo_torque = 0.0f;
			m_turbo_bov_torque = 0.0f;

			m_turbo_inertia = 0.000003f * m_conf_turbo_inertia_factor;

			// braking (compression)
			m_turbo_torque -= m_turbo_curr_rpm[i] / m_conf_turbo_max_rpm;
			m_turbo_bov_torque -= m_turbo_cur_bov_rpm[i] / m_conf_turbo_max_rpm;

			// powering (exhaust) with limiter
			if (m_curr_engine_rpm >= m_conf_turbo_engine_rpm_operation)
			{
				if (m_turbo_curr_rpm[i] <= m_conf_turbo_max_rpm && m_is_engine_running && acc > 0.06f)
				{
					if (m_conf_turbo_has_wastegate)
					{
						if (m_turbo_curr_rpm[i] < m_conf_turbo_wg_min_psi * m_conf_turbo_wg_threshold_p && !m_conf_turbo_has_flutter)
						{
							m_turbo_torque += 1.5f * acc * (((m_curr_engine_rpm - m_conf_turbo_engine_rpm_operation) / (m_conf_engine_max_rpm - m_conf_turbo_engine_rpm_operation)));
						}
						else
						{
							m_conf_turbo_has_flutter = true;
							m_turbo_torque -= (m_turbo_curr_rpm[i] / m_conf_turbo_max_rpm) *1.5;
						}	

						if (m_conf_turbo_has_flutter)
						{
							SoundScriptManager::getSingleton().trigStart(m_vehicle_index, SS_TRIG_TURBOWASTEGATE);
							if (m_turbo_curr_rpm[i] < m_conf_turbo_wg_min_psi * m_conf_turbo_wg_threshold_n)
							{
								m_conf_turbo_has_flutter = false;
								SoundScriptManager::getSingleton().trigStop(m_vehicle_index, SS_TRIG_TURBOWASTEGATE);
							}
								
						}
					}
					else
						m_turbo_torque += 1.5f * acc * (((m_curr_engine_rpm - m_conf_turbo_engine_rpm_operation) / (m_conf_engine_max_rpm - m_conf_turbo_engine_rpm_operation)));
				}
				else
				{
					m_turbo_torque += 0.1f * (((m_curr_engine_rpm - m_conf_turbo_engine_rpm_operation) / (m_conf_engine_max_rpm - m_conf_turbo_engine_rpm_operation)));
				}

				//Update waste gate, it's like a BOV on the exhaust part of the turbo, acts as a limiter
				if (m_conf_turbo_has_wastegate)
				{
					if (m_turbo_curr_rpm[i] > m_conf_turbo_wg_min_psi * 0.95)
						m_turbo_inertia = m_turbo_inertia *0.7; //Kill m_conf_engine_inertia so it flutters
					else
						m_turbo_inertia = m_turbo_inertia *1.3; //back to normal m_conf_engine_inertia
				}
			}
			
			//simulate compressor surge
			if (!m_conf_turbo_has_bov)
			{
				if (m_turbo_curr_rpm[i] > 13 * 10000 && m_curr_acc < 0.06f)
				{
					m_turbo_torque += (m_turbo_torque * 2.5);
				}
			}

			// anti lag
			if (m_conf_turbo_has_antilag && m_curr_acc < 0.5)
			{
				float f = frand();
				if (m_curr_engine_rpm > m_conf_turbo_antilag_min_rpm && f > m_conf_turbo_antilag_chance_rand)
				{
					if (m_turbo_curr_rpm[i] > m_conf_turbo_max_rpm*0.35 && m_turbo_curr_rpm[i] < m_conf_turbo_max_rpm)
					{
						m_turbo_torque -= (m_turbo_torque * (f * m_conf_turbo_antilag_power_factor));
						SoundScriptManager::getSingleton().trigStart(m_vehicle_index, SS_TRIG_TURBOBACKFIRE);
					}
				}
				else
					SoundScriptManager::getSingleton().trigStop(m_vehicle_index, SS_TRIG_TURBOBACKFIRE);
			}

			// update main turbo rpm
			m_turbo_curr_rpm[i] += dt * m_turbo_torque / m_turbo_inertia;

			//Update BOV
			//It's basicly an other turbo which is limmited to the main one's rpm, but it doesn't affect its rpm.  It only affects the power going into the engine.
			//This one is used to simulate the pressure between the engine and the compressor.
			//I should make the whole turbo code work this way. -Max98
			if (m_conf_turbo_has_bov)
			{
				
				if (m_turbo_cur_bov_rpm[i] < m_turbo_curr_rpm[i])
					m_turbo_bov_torque += 1.5f * acc * (((m_curr_engine_rpm) / (m_conf_engine_max_rpm)));
				else
					m_turbo_bov_torque += 0.07f * (((m_curr_engine_rpm) / (m_conf_engine_max_rpm)));


				if (m_curr_acc < 0.06 && m_turbo_curr_rpm[i] > m_conf_turbo_min_bov_psi * 10000) 
				{
					SoundScriptManager::getSingleton().trigStart(m_vehicle_index, SS_TRIG_TURBOBOV);
					m_turbo_cur_bov_rpm[i] += dt * m_turbo_bov_torque / (m_turbo_inertia * 0.1);
				}
				else
				{
					SoundScriptManager::getSingleton().trigStop(m_vehicle_index, SS_TRIG_TURBOBOV);
					if (m_turbo_cur_bov_rpm[i] < m_turbo_curr_rpm[i])
						m_turbo_cur_bov_rpm[i] += dt * m_turbo_bov_torque / (m_turbo_inertia * 0.05);
					else
						m_turbo_cur_bov_rpm[i] += dt * m_turbo_bov_torque / m_turbo_inertia;
				}	
			}
		}
	}

	// update engine speed
	float totaltorque = 0.0f;

	// engine braking
	if (m_starter_has_contact)
	{
		totaltorque += m_conf_engine_braking_torque * m_curr_engine_rpm / m_conf_engine_max_rpm;
	} else
	{
		totaltorque += 10.0f * m_conf_engine_braking_torque * m_curr_engine_rpm / m_conf_engine_max_rpm;
	}

    float DBG_totaltorque_sample1 = totaltorque;

	// braking by m_conf_engine_hydropump
	if (m_curr_engine_rpm > 100.0f)
	{
		totaltorque -= 8.0f * m_engine_hydropump / (m_curr_engine_rpm * 0.105f * dt);
	}

    float DBG_totaltorque_sample2 = totaltorque;

	if (m_is_engine_running && m_starter_has_contact && m_curr_engine_rpm < (m_conf_engine_max_rpm * 1.25f))
	{
		totaltorque += CalcEnginePower(m_curr_engine_rpm) * acc;
	}

    float DBG_totaltorque_sample3 = totaltorque;

	if (!engine_is_electric)
	{
		if (m_is_engine_running && m_curr_engine_rpm < m_conf_engine_stall_rpm)
		{
            bool const must_lock = false;
			this->BeamEngineStop(must_lock); //No, electric engine has no stop
		}
		// m_starter_is_running

		if (m_starter_has_contact && m_starter_is_running && m_curr_engine_rpm < m_conf_engine_stall_rpm * 1.5f)
		{
			totaltorque += -m_conf_engine_braking_torque; //No m_starter_is_running in electric engines
		}
		// restart

		if (!m_is_engine_running && m_curr_engine_rpm > m_conf_engine_stall_rpm && m_starter_has_contact)
		{
			m_is_engine_running = true;
#ifdef USE_OPENAL
			SoundScriptManager::getSingleton().trigStart(m_vehicle_index, SS_TRIG_ENGINE);
#endif // USE_OPENAL
		}
	}

    float DBG_totaltorque_sample4 = totaltorque;

	// clutch
	float retorque = 0.0f;

	if (m_curr_gear)
	{
		retorque = m_curr_clutch_torque / m_conf_gear_ratios[m_curr_gear + 1];
	}

	totaltorque -= retorque;

    /*//  Debugging internal flow
    char msg[1000] = {0};
    sprintf(msg, 
        "Lua - DBG "
        "| totaltorque { 1: %10.3f | 2: %10.3f | 3: %10.3f | 4: %10.3f } "
        "| acc: { 1: %10.3f (idlemix: %10.3f) | 2: %10.3f (primemix: %10.3f) | 3: %10.3f } "
        "| retorque: %10.3f "
        "| self.curr_engine_rpm: %10.3f "
        "| self.conf_engine_inertia: %10.3f",
        DBG_totaltorque_sample1, DBG_totaltorque_sample2, DBG_totaltorque_sample3, DBG_totaltorque_sample4,
        DBG_acc_snap1, idle_mixture, DBG_acc_snap2, prime_mixture, acc,
        retorque, m_curr_engine_rpm, m_conf_engine_inertia);
    LOG(msg);
    // */

	// integration
	m_curr_engine_rpm += dt * totaltorque / m_conf_engine_inertia;

	// update clutch torque
	if (m_curr_gear)
	{
		float gearboxspinner = m_curr_engine_rpm / m_conf_gear_ratios[m_curr_gear + 1];
		m_curr_clutch_torque = (gearboxspinner - m_cur_wheel_revolutions) * m_curr_clutch * m_curr_clutch * m_conf_clutch_force;
	} else
	{
		m_curr_clutch_torque = 0.0f;
	}

	m_curr_engine_rpm = std::max(0.0f, m_curr_engine_rpm);

	if (m_transmission_mode < Gearbox::SHIFTMODE_MANUAL)
	{
		// auto-shift
		if (m_is_shifting && !engine_is_electric) //No shifting in electric cars
		{
			m_shift_clock += dt;

			// clutch
			if (m_shift_clock < m_conf_clutch_time)
			{
				m_curr_clutch = 1.0f - (m_shift_clock / m_conf_clutch_time);
			} else if (m_shift_clock > (m_conf_shift_time - m_conf_clutch_time))
			{
				m_curr_clutch = 1.0f - (m_conf_shift_time - m_shift_clock) / m_conf_clutch_time;
			} else
			{
				m_curr_clutch = 0.0f;
			}

			// shift
			if (m_curr_gear_change_relative && m_shift_clock > m_conf_clutch_time / 2.0f)
			{
#ifdef USE_OPENAL
				SoundScriptManager::getSingleton().trigStart(m_vehicle_index, SS_TRIG_SHIFT);
#endif // USE_OPENAL
				m_curr_gear += m_curr_gear_change_relative;
				m_curr_gear = std::max(-1, m_curr_gear);
				m_curr_gear = std::min(m_curr_gear, m_conf_num_gears);
				m_curr_gear_change_relative = 0;
			}

			// end of shifting
			if (m_shift_clock > m_conf_shift_time)
			{
#ifdef USE_OPENAL
				SoundScriptManager::getSingleton().trigStop(m_vehicle_index, SS_TRIG_SHIFT);
#endif // USE_OPENAL
				m_curr_acc = m_auto_curr_acc; //INLINED setAcc(m_auto_curr_acc);
				m_is_shifting = 0;
				m_curr_clutch = 1.0f;
				m_is_post_shifting = true;
				m_post_shift_clock = 0.0f;
			}
		} else
			m_curr_acc = m_auto_curr_acc; //INLINED setAcc(m_auto_curr_acc);



		// auto declutch
		if (!engine_is_electric)
		{
			if (m_is_post_shifting)
			{
				m_post_shift_clock += dt;
				if (m_post_shift_clock > m_conf_post_shift_time)
				{
					m_is_post_shifting = false;
				}
			}
			if (m_is_shifting)
			{
				// we are shifting, just avoid stalling in worst case
				if (m_curr_engine_rpm < m_conf_engine_stall_rpm * 1.2f)
				{
					m_curr_clutch = 0.0f;
				}
			}
			else if (m_is_post_shifting)
			{
				// we are postshifting, no gear change
				if (m_curr_engine_rpm < m_conf_engine_stall_rpm * 1.2f && acc < 0.5f)
				{
					m_curr_clutch = 0.0f;
				}
				else
				{
					m_curr_clutch = 1.0f;
				}
			}
			else if (m_curr_engine_rpm < m_conf_engine_stall_rpm * 1.2f && acc < 0.5f)
			{
				m_curr_clutch = 0.0f;
			}
			else if (std::abs(m_curr_gear) == 1)
			{
				// 1st gear : special
				if (m_curr_engine_rpm > m_conf_engine_min_rpm)
				{
					m_curr_clutch = (m_curr_engine_rpm - m_conf_engine_min_rpm) / (m_conf_engine_max_rpm - m_conf_engine_min_rpm);
					m_curr_clutch = std::min(m_curr_clutch, 1.0f);
				}
				else
				{
					m_curr_clutch = 0.0f;
				}
			}
			else
			{
				m_curr_clutch = 1.0f;
			}
		} else
			m_curr_clutch = 1.0f;
	}

	if (doUpdate && !m_is_shifting && !m_is_post_shifting)
	{
		float halfRPMRange     = m_conf_autotrans_full_rpm_range / 2.f;
		float oneThirdRPMRange = m_conf_autotrans_full_rpm_range / 3.f;

		// gear hack
		this->m_abs_velocity = truck->nodes[0].Velocity.length();
		float velocity = m_abs_velocity;

		if (truck->cameranodepos[0] >= 0 && truck->cameranodedir[0] >=0)
		{
			Vector3 hdir = (truck->nodes[truck->cameranodepos[0]].RelPosition - truck->nodes[truck->cameranodedir[0]].RelPosition).normalisedCopy();
			velocity = hdir.dotProduct(truck->nodes[0].Velocity);
		}
		m_rel_velocity = std::abs(velocity);

		if (truck->wheels[0].radius != 0)
		{
			m_ref_wheel_revolutions = velocity / truck->wheels[0].radius * RAD_PER_SEC_TO_RPM;
		}

		if (m_transmission_mode == Gearbox::SHIFTMODE_AUTOMATIC && (m_autoselect == Gearbox::AUTOSWITCH_DRIVE || m_autoselect == Gearbox::AUTOSWITCH_TWO) && m_curr_gear > 0)
		{
			if ((m_curr_engine_rpm > m_conf_engine_max_rpm - 100.0f && m_curr_gear > 1) || m_cur_wheel_revolutions * m_conf_gear_ratios[m_curr_gear + 1] > m_conf_engine_max_rpm - 100.0f)
			{
				if ((m_autoselect == Gearbox::AUTOSWITCH_DRIVE && m_curr_gear < m_conf_num_gears) || (m_autoselect == Gearbox::AUTOSWITCH_TWO && m_curr_gear < std::min(2, m_conf_num_gears)) && !engine_is_electric)
				{
					this->BeamEngineShift(1, false);
				}
			} else if (m_curr_gear > 1 && m_ref_wheel_revolutions * m_conf_gear_ratios[m_curr_gear] < m_conf_engine_max_rpm && (m_curr_engine_rpm < m_conf_engine_min_rpm || (m_curr_engine_rpm < m_conf_engine_min_rpm + m_autotrans_curr_shift_behavior * halfRPMRange / 2.0f &&
				CalcEnginePower(m_cur_wheel_revolutions * m_conf_gear_ratios[m_curr_gear]) > CalcEnginePower(m_cur_wheel_revolutions * m_conf_gear_ratios[m_curr_gear + 1]))) && !engine_is_electric)
			{
				this->BeamEngineShift(-1, false);
			}

			int newGear = m_curr_gear;
			
			float brake = 0.0f;

			if (truck->brakeforce > 0.0f)
			{
				brake = truck->brake / truck->brakeforce;
			}

			m_autotrans_rpm_buffer.push_front(m_curr_engine_rpm);
			m_autotrans_acc_buffer.push_front(acc);
			m_autotrans_brake_buffer.push_front(brake);

			float avgRPM50 = 0.0f;
			float avgRPM200 = 0.0f;
			float avgAcc50 = 0.0f;
			float avgAcc200 = 0.0f;
			float avgBrake50 = 0.0f;
			float avgBrake200 = 0.0f;

			for (unsigned int i=0; i < m_autotrans_acc_buffer.size(); i++)
			{
				if (i < 50)
				{
					avgRPM50 += m_autotrans_rpm_buffer[i];
					avgAcc50 += m_autotrans_acc_buffer[i];
					avgBrake50 += m_autotrans_brake_buffer[i];
				}

				avgRPM200 += m_autotrans_rpm_buffer[i];
				avgAcc200 += m_autotrans_acc_buffer[i];
				avgBrake200 += m_autotrans_brake_buffer[i];
			}

			avgRPM50 /= std::min(m_autotrans_rpm_buffer.size(), (std::deque<float>::size_type)50);
			avgAcc50 /= std::min(m_autotrans_acc_buffer.size(), (std::deque<float>::size_type)50);
			avgBrake50 /= std::min(m_autotrans_brake_buffer.size(), (std::deque<float>::size_type)50);

			avgRPM200 /= m_autotrans_rpm_buffer.size();
			avgAcc200 /= m_autotrans_acc_buffer.size();
			avgBrake200 /= m_autotrans_brake_buffer.size();

			if (!engine_is_electric)
			{

				if (avgAcc50 > 0.8f || avgAcc200 > 0.8f || avgBrake50 > 0.8f || avgBrake200 > 0.8f)
				{
					m_autotrans_curr_shift_behavior = std::min(m_autotrans_curr_shift_behavior + 0.01f, 1.0f);
				}
				else if (acc < 0.5f && avgAcc50 < 0.5f && avgAcc200 < 0.5f && brake < 0.5f && avgBrake50 < 0.5f && avgBrake200 < 0.5 )
				{
					m_autotrans_curr_shift_behavior /= 1.01;
				}
			

				if (avgAcc50 > 0.8f && m_curr_engine_rpm < m_conf_engine_max_rpm - oneThirdRPMRange)
				{
					while (newGear > 1 && m_cur_wheel_revolutions * m_conf_gear_ratios[newGear] < m_conf_engine_max_rpm - oneThirdRPMRange &&
						CalcEnginePower(m_cur_wheel_revolutions * m_conf_gear_ratios[newGear])   * m_conf_gear_ratios[newGear] >
						CalcEnginePower(m_cur_wheel_revolutions * m_conf_gear_ratios[newGear+1]) * m_conf_gear_ratios[newGear+1])
					{
						newGear--;
					}
				} else if (avgAcc50 > 0.6f && acc < 0.8f && acc > avgAcc50 + 0.1f && m_curr_engine_rpm < m_conf_engine_min_rpm + halfRPMRange)
				{
					if (newGear > 1 && m_cur_wheel_revolutions * m_conf_gear_ratios[newGear] < m_conf_engine_min_rpm + halfRPMRange &&
						CalcEnginePower(m_cur_wheel_revolutions * m_conf_gear_ratios[newGear])   * m_conf_gear_ratios[newGear] >
						CalcEnginePower(m_cur_wheel_revolutions * m_conf_gear_ratios[newGear+1]) * m_conf_gear_ratios[newGear+1])
					{
						newGear--;
					}
				} else if (avgAcc50 > 0.4f && acc < 0.8f && acc > avgAcc50 + 0.1f && m_curr_engine_rpm < m_conf_engine_min_rpm + halfRPMRange)
				{
					if (newGear > 1 && m_cur_wheel_revolutions * m_conf_gear_ratios[newGear] < m_conf_engine_min_rpm + oneThirdRPMRange &&
						CalcEnginePower(m_cur_wheel_revolutions * m_conf_gear_ratios[newGear])   * m_conf_gear_ratios[newGear] >
						CalcEnginePower(m_cur_wheel_revolutions * m_conf_gear_ratios[newGear+1]) * m_conf_gear_ratios[newGear+1])
					{
						newGear--;
					}
				}
				else if (m_curr_gear < (m_autoselect == Gearbox::AUTOSWITCH_TWO ? std::min(2, m_conf_num_gears) : m_conf_num_gears) &&
					avgBrake200 < 0.2f && acc < std::min(avgAcc200 + 0.1f, 1.0f) && m_curr_engine_rpm > avgRPM200 - m_conf_autotrans_full_rpm_range / 20.0f)
				{
					if (avgAcc200 < 0.6f && avgAcc200 > 0.4f && m_curr_engine_rpm > m_conf_engine_min_rpm + oneThirdRPMRange && m_curr_engine_rpm < m_conf_engine_max_rpm - oneThirdRPMRange)
					{
						if (m_cur_wheel_revolutions * m_conf_gear_ratios[newGear + 2] > m_conf_engine_min_rpm + oneThirdRPMRange)
						{
							newGear++;
						}
					}
					else if (avgAcc200 < 0.4f && avgAcc200 > 0.2f && m_curr_engine_rpm > m_conf_engine_min_rpm + oneThirdRPMRange)
					{
						if (m_cur_wheel_revolutions * m_conf_gear_ratios[newGear + 2] > m_conf_engine_min_rpm + oneThirdRPMRange / 2.0f)
						{
							newGear++;
						}
					}
					else if (avgAcc200 < 0.2f && m_curr_engine_rpm > m_conf_engine_min_rpm + oneThirdRPMRange / 2.0f && m_curr_engine_rpm < m_conf_engine_min_rpm + halfRPMRange)
					{
						if (m_cur_wheel_revolutions * m_conf_gear_ratios[newGear + 2] > m_conf_engine_min_rpm + oneThirdRPMRange / 2.0f)
						{
							newGear++;
						}
					}

					if (newGear > m_curr_gear)
					{
						m_autotrans_up_shift_delay_counter++;
						if (m_autotrans_up_shift_delay_counter <= 100 * m_autotrans_curr_shift_behavior)
						{
							newGear = m_curr_gear;
						}
					}
					else
					{
						m_autotrans_up_shift_delay_counter = 0;
					}
				}
				if (newGear < m_curr_gear && std::abs(m_cur_wheel_revolutions * (m_conf_gear_ratios[newGear + 1] - m_conf_gear_ratios[m_curr_gear + 1])) > oneThirdRPMRange / 6.0f ||
					newGear > m_curr_gear && std::abs(m_cur_wheel_revolutions * (m_conf_gear_ratios[newGear + 1] - m_conf_gear_ratios[m_curr_gear + 1])) > oneThirdRPMRange / 3.0f && !engine_is_electric)
				{
					if (m_abs_velocity - m_rel_velocity < 0.5f)
						this->BeamEngineShiftTo(newGear, false);
				}
			}


			if (m_autotrans_acc_buffer.size() > 200)
			{
				m_autotrans_rpm_buffer.pop_back();
				m_autotrans_acc_buffer.pop_back();
				m_autotrans_brake_buffer.pop_back();
			}
			// avoid over-revving
			if (m_transmission_mode <= Gearbox::SHIFTMODE_SEMIAUTO && m_curr_gear != 0)
			{
				if (std::abs(m_cur_wheel_revolutions * m_conf_gear_ratios[m_curr_gear + 1]) > m_conf_engine_max_rpm * 1.25f)
				{
					float clutch = 0.0f + 1.0f / (1.0f + std::abs(m_cur_wheel_revolutions * m_conf_gear_ratios[m_curr_gear + 1] - m_conf_engine_max_rpm * 1.25f) / 2.0f);
					m_curr_clutch = std::min(clutch, m_curr_clutch);
				}
				if (m_curr_gear * m_cur_wheel_revolutions < -10.0f)
				{
					float clutch = 0.0f + 1.0f / (1.0f + std::abs(-10.0f - m_curr_gear * m_cur_wheel_revolutions) / 2.0f);
					m_curr_clutch = std::min(clutch, m_curr_clutch);
				}
			}
		}
	}

	// audio stuff
	UpdateBeamEngineAudio(doUpdate);

	// ============ DEBUG =================
	// Print all contents
    //this->DEBUG_LogClassState(snapshot_before, dt, doUpdate);
}

#define LOG_DEQUE_FLOAT(sstream, var) { \
    auto itor = var.begin();\
    auto iend = var.end();\
    sstream << "[size: "<<var.size()<<"]";\
    for (; itor != iend; ++itor) { sstream << std::setw(10) << *itor << ""; } \
    } 

void BeamEngine::UpdateBeamEngineAudio(int doUpdate)
{
    // NO LOCK - only called internally

#ifdef USE_OPENAL
	if (m_conf_engine_has_turbo)
	{
		for (int i = 0; i < m_conf_num_turbos; i++)
			 SoundScriptManager::getSingleton().modulate(m_vehicle_index, SS_MOD_TURBO, m_turbo_curr_rpm[i]);
	}

	if (doUpdate)
	{
		SoundScriptManager::getSingleton().modulate(m_vehicle_index, SS_MOD_ENGINE, m_curr_engine_rpm);
		SoundScriptManager::getSingleton().modulate(m_vehicle_index, SS_MOD_TORQUE, m_curr_clutch_torque);
		SoundScriptManager::getSingleton().modulate(m_vehicle_index, SS_MOD_GEARBOX, m_cur_wheel_revolutions);
	}
	// reverse gear beep
	if (m_curr_gear == -1 && m_is_engine_running)
	{
		SoundScriptManager::getSingleton().trigStart(m_vehicle_index, SS_TRIG_REVERSE_GEAR);
	} else
	{
		SoundScriptManager::getSingleton().trigStop(m_vehicle_index, SS_TRIG_REVERSE_GEAR);
	}
#endif // USE_OPENAL
}

float BeamEngine::getRPM()
{
    SCOPED_LOCK("getRPM()") // only external calls
	return m_curr_engine_rpm;
}

void BeamEngine::toggleAutoMode()
{
    SCOPED_LOCK("toggleAutoMode()") // only external calls
	m_transmission_mode = (m_transmission_mode + 1) % (Gearbox::SHIFTMODE_MANUAL_RANGES + 1);

	// this switches off all automatic symbols when in manual mode
	if (m_transmission_mode != Gearbox::SHIFTMODE_AUTOMATIC)
	{
		m_autoselect = Gearbox::AUTOSWITCH_MANUALMODE;
	} else
	{
		m_autoselect = Gearbox::AUTOSWITCH_NEUTRAL;
	}

	if (m_transmission_mode == Gearbox::SHIFTMODE_MANUAL_RANGES)
	{
		m_curr_gear_range = 0.f; //INLINED this->setGearRange(0);
		m_curr_gear = 0.f; //INLINED this->setGear(0);
	}
}

int BeamEngine::getAutoMode()
{
    SCOPED_LOCK("getAutoMode()") // only external calls
	return m_transmission_mode;
}

void BeamEngine::setAutoMode(int mode)
{
    SCOPED_LOCK("setAutoMode()") // OK
	m_transmission_mode = mode;
}

void BeamEngine::setAcc(float val)
{
    SCOPED_LOCK("setAcc()") // OK
	m_curr_acc = val;
}

float BeamEngine::getTurboPSI(bool must_lock /*=true*/)
{
    SCOPED_LOCK_OPT("getTurboPSI()", must_lock);

	m_turbo_psi = 0;

	if (m_conf_turbo_has_bov)
	{
		for (int i = 0; i < m_conf_num_turbos; i++)
			m_turbo_psi += m_turbo_cur_bov_rpm[i] / 10000.0f;
	}
	else
	{
		for (int i = 0; i < m_conf_num_turbos; i++)
			m_turbo_psi += m_turbo_curr_rpm[i] / 10000.0f;
	}

	return m_turbo_psi;
}

float BeamEngine::getAcc()
{
    SCOPED_LOCK("getAcc()") // OK
	return m_curr_acc;
}

// this is mainly for smoke...
void BeamEngine::netForceSettings(float rpm, float force, float clutch, int gear, bool _running, bool _contact, char _automode)
{
    SCOPED_LOCK("netForceSettings()") // ok
	m_curr_engine_rpm = rpm;
	m_curr_acc       = force;
	m_curr_clutch    = clutch;
	m_curr_gear      = gear;
	m_is_engine_running      = _running; //(fabs(rpm)>10.0);
	m_starter_has_contact      = _contact;
	if (_automode != -1)
	{
		m_transmission_mode = _automode;
	}
}

float BeamEngine::getSmoke()
{
    SCOPED_LOCK("getSmoke()") // OK
	if (m_is_engine_running)
	{
		return m_curr_acc * (1.0f - m_turbo_curr_rpm[0] /* doesn't matter */ / m_conf_turbo_max_rpm);// * m_conf_engine_torque / 5000.0f;
	}

	return -1;
}

float BeamEngine::getTorque()
{
    SCOPED_LOCK("getTorque()") // OK
	if (m_curr_clutch_torque >  1000000.0) return  1000000.0;
	if (m_curr_clutch_torque < -1000000.0) return -1000000.0;
	return m_curr_clutch_torque;
}

void BeamEngine::setRPM(float rpm)
{
    SCOPED_LOCK("setRPM()") // OK
	m_curr_engine_rpm = rpm;
}

void BeamEngine::setSpin(float rpm)
{
    SCOPED_LOCK("setSpin()") // OK
	m_cur_wheel_revolutions = rpm;
}

// for hydros acceleration
float BeamEngine::getCrankFactor(bool must_lock /*= true */)
{
    SCOPED_LOCK_OPT("getCrankFactor()", must_lock)
	float minWorkingRPM = m_conf_engine_idle_rpm * 1.1f; // minWorkingRPM > m_conf_engine_idle_rpm avoids commands deadlocking the engine

	float rpmRatio = (m_curr_engine_rpm - minWorkingRPM) / (m_conf_engine_max_rpm - minWorkingRPM);
	rpmRatio = std::max(0.0f, rpmRatio); // Avoids a negative rpmRatio when m_curr_engine_rpm < minWorkingRPM
	rpmRatio = std::min(rpmRatio, 1.0f); // Avoids a rpmRatio > 1.0f when m_curr_engine_rpm > m_conf_engine_max_rpm

	float crankfactor = 5.0f * rpmRatio;

	return crankfactor;
}

void BeamEngine::setClutch(float clutch)
{
    SCOPED_LOCK("setClutch()") // OK
	m_curr_clutch = clutch;
}

float BeamEngine::getClutch()
{
    SCOPED_LOCK("getClutch()") // OK
	return m_curr_clutch;
}

float BeamEngine::getClutchForce()
{
    SCOPED_LOCK("getClutchForce()") // OK
	return m_conf_clutch_force;
}

void BeamEngine::toggleContact()
{
    SCOPED_LOCK("toggleContact()")//OK
	m_starter_has_contact = !m_starter_has_contact;
#ifdef USE_OPENAL
	if (m_starter_has_contact)
	{
		SoundScriptManager::getSingleton().trigStart(m_vehicle_index, SS_TRIG_IGNITION);
	} else
	{
		SoundScriptManager::getSingleton().trigStop(m_vehicle_index, SS_TRIG_IGNITION);
	}
#endif // USE_OPENAL
}

// quick start
void BeamEngine::BeamEngineStart()
{
    SCOPED_LOCK("start()") // OK, only external calls
	if (m_transmission_mode == Gearbox::SHIFTMODE_AUTOMATIC)
	{
		m_curr_gear = 1;
		m_autoselect = Gearbox::AUTOSWITCH_DRIVE;
	} else
	{
		if (m_transmission_mode == Gearbox::SHIFTMODE_SEMIAUTO)
		{
			m_curr_gear = 1;
		} else
		{
			m_curr_gear = 0;
		}
		m_autoselect = Gearbox::AUTOSWITCH_MANUALMODE;
	}
	m_curr_clutch = 0.0f;
	m_curr_engine_rpm = 750.0f;
	m_curr_clutch_torque = 0.0f;

	for (int i = 0; i < m_conf_num_turbos; i++)
	{
		m_turbo_curr_rpm[i] = 0.0f;
		m_turbo_cur_bov_rpm[i] = 0.0f;
	}

	m_air_pressure = 0.0f;
	m_is_engine_running = true;
	m_starter_has_contact = true;
#ifdef USE_OPENAL
	SoundScriptManager::getSingleton().trigStart(m_vehicle_index, SS_TRIG_IGNITION);
	m_curr_acc = 0.f; //INLINED: setAcc(0.0f);
	SoundScriptManager::getSingleton().trigStart(m_vehicle_index, SS_TRIG_ENGINE);
#endif // USE_OPENAL
}

void BeamEngine::offstart()
{
    SCOPED_LOCK("offstart()") // OK
	m_curr_gear = 0;
	m_curr_clutch = 0.0f;
	if (m_conf_engine_type != 'e') // e = Electric engine
		m_autoselect = Gearbox::AUTOSWITCH_NEUTRAL; //no Neutral in electric engines
	else
		m_autoselect = Gearbox::AUTOSWITCH_ONE;

	m_curr_engine_rpm = 0.0f;
	m_is_engine_running = false;
	m_starter_has_contact = false;
#ifdef USE_OPENAL
	SoundScriptManager::getSingleton().trigStop(m_vehicle_index, SS_TRIG_IGNITION);
	SoundScriptManager::getSingleton().trigStop(m_vehicle_index, SS_TRIG_ENGINE);
#endif // USE_OPENAL
}

void BeamEngine::setstarter(int v)
{
    SCOPED_LOCK("setstarter()") // OK
	m_starter_is_running = (v == 1);
	if (v && m_curr_engine_rpm < 750.0f)
	{
		m_curr_acc = 1.f; //INLINED setAcc(1.0f);
	}
}

int BeamEngine::getGear()
{
    SCOPED_LOCK("getGear()") // OK
	return m_curr_gear;
}

// low level gear changing
void BeamEngine::setGear(int v)
{
    SCOPED_LOCK("setGear()") // OK
	m_curr_gear = v;
}

int BeamEngine::getGearRange()
{
    SCOPED_LOCK("getGearRange()") // OK
	return m_curr_gear_range;
}

void BeamEngine::setGearRange(int v)
{
    SCOPED_LOCK("setGearRange()") // OK
	m_curr_gear_range = v;
}

void BeamEngine::BeamEngineStop(bool must_lock)
{
    SCOPED_LOCK_OPT("stop()", must_lock)
	if (!m_is_engine_running) return;

	m_is_engine_running = false;
	// Script Event - engine death
	TRIGGER_EVENT(SE_TRUCK_ENGINE_DIED, m_vehicle_index);
#ifdef USE_OPENAL
	SoundScriptManager::getSingleton().trigStop(m_vehicle_index, SS_TRIG_ENGINE);
#endif // USE_OPENAL
}

// high level controls
void BeamEngine::autoSetAcc(float val)
{
    SCOPED_LOCK("autoSetAcc()") // only called externally
	m_auto_curr_acc = val;
	if (!m_is_shifting)
	{
		m_curr_acc = val; //INLINED setAcc(val);
	}
}

void BeamEngine::BeamEngineShift(int val, bool must_lock /*=true*/)
{
    SCOPED_LOCK_OPT("BeamEngineShift()", must_lock)
	if (!val || m_curr_gear + val < -1 || m_curr_gear + val > getNumGears()) return;
	if (m_transmission_mode < Gearbox::SHIFTMODE_MANUAL)
	{
#ifdef USE_OPENAL
		SoundScriptManager::getSingleton().trigStart(m_vehicle_index, SS_TRIG_SHIFT);
#endif // USE_OPENAL
		m_curr_gear_change_relative = val;
		m_is_shifting = 1;
		m_shift_clock = 0.0f;
		m_curr_acc = 0.f; //INLINED setAcc(0.0f);
	} else
	{
		if (m_curr_clutch > 0.25f)
		{
#ifdef USE_OPENAL
			SoundScriptManager::getSingleton().trigOnce(m_vehicle_index, SS_TRIG_GEARSLIDE);
#endif // USE_OPENAL
		} else
		{
#ifdef USE_OPENAL
			SoundScriptManager::getSingleton().trigOnce(m_vehicle_index, SS_TRIG_SHIFT);
#endif // USE_OPENAL
			m_curr_gear += val;
		}
	}
}

void BeamEngine::BeamEngineShiftTo(int newGear, bool must_lock /*=true*/)
{
    SCOPED_LOCK_OPT("BeamEngineShiftTo()", must_lock);

    bool const should_lock = false;
	this->BeamEngineShift(newGear - m_curr_gear, should_lock);
}

void BeamEngine::UpdateBeamEngineShifts()
{
    // NO LOCK - only called internally


	if (m_autoselect == Gearbox::AUTOSWITCH_MANUALMODE) return;

#ifdef USE_OPENAL
	SoundScriptManager::getSingleton().trigOnce(m_vehicle_index, SS_TRIG_SHIFT);
#endif // USE_OPENAL
	bool engine_is_electric = (m_conf_engine_type == 'e');

	if (m_autoselect == Gearbox::AUTOSWITCH_REAR)
	{
		m_curr_gear = -1;
	} else if (m_autoselect == Gearbox::AUTOSWITCH_NEUTRAL && !engine_is_electric)
	{
		m_curr_gear =  0;
	} else if (m_autoselect == Gearbox::AUTOSWITCH_ONE)
	{
		m_curr_gear =  1;
	}
	else if (!engine_is_electric) //no other gears for electric cars
	{
		// search for an appropriate gear
		int newGear = 1;

		while (newGear < m_conf_num_gears && m_cur_wheel_revolutions > 0.0f && m_cur_wheel_revolutions * m_conf_gear_ratios[newGear + 1] > m_conf_engine_max_rpm - 100.0f)
		{
			newGear++;
		}

		m_curr_gear = newGear;
		
		if (m_autoselect == Gearbox::AUTOSWITCH_TWO)
		{
			m_curr_gear = std::min(m_curr_gear, 2);
		}
	}
}

void BeamEngine::autoShiftSet(int mode)
{
    SCOPED_LOCK("autoShiftSet()") // OK
	m_autoselect = (Gearbox::autoswitch)mode;
	this->UpdateBeamEngineShifts();
}

void BeamEngine::autoShiftUp()
{
    SCOPED_LOCK("autoShiftUp()") // OK
	if (m_autoselect != Gearbox::AUTOSWITCH_REAR)
	{
		m_autoselect = (Gearbox::autoswitch)(m_autoselect-1);
		this->UpdateBeamEngineShifts();
	}
}

void BeamEngine::autoShiftDown()
{
    SCOPED_LOCK("autoShiftDown()") // OK
	if (m_autoselect != Gearbox::AUTOSWITCH_ONE)
	{
		m_autoselect = (Gearbox::autoswitch)(m_autoselect+1);
		this->UpdateBeamEngineShifts();
	}
}

Gearbox::autoswitch BeamEngine::getAutoShift()
{
    SCOPED_LOCK("getAutoShift()") // OK
	return m_autoselect;
}

void BeamEngine::setManualClutch(float val)
{
    SCOPED_LOCK("setManualClutch()") // OK
	if (m_transmission_mode >= Gearbox::SHIFTMODE_MANUAL)
	{
		val = std::max(0.0f, val);
		m_curr_clutch = 1.0 - val;
	}
}

float BeamEngine::CalcEnginePower(float rpm)
{
    // No lock - only called from BeamEngineUpdate()


	// engine power with limiter
	float tqValue = 0.0f; //This is not a value, it's more of a ratio(0-1), really got me lost..

	float atValue = 0.0f; //Additional torque (turbo integreation)

	float rpmRatio = rpm / (m_conf_engine_max_rpm * 1.25f);

	rpmRatio = std::min(rpmRatio, 1.0f);

	if (m_conf_engine_torque_curve)
	{
		tqValue = m_conf_engine_torque_curve->getEngineTorque(rpmRatio);
	}

	if (m_conf_engine_has_turbo)
	{
		if (m_conf_turbo_version == 1)
		{
			for (int i = 0; i < m_conf_num_turbos; i++)
				atValue = m_conf_turbo_addi_torque[i] * (m_turbo_curr_rpm[i] / m_conf_turbo_max_rpm);
		}
		else
		{
            bool const lock_mutex = false;
			atValue = (((getTurboPSI(lock_mutex) * 6.8) * m_conf_engine_torque) / 100); //1psi = 6% more power
		}
	}

    float result = (m_conf_engine_torque * tqValue) + atValue;

    /*
    char msg[1000] = {0};
    sprintf(msg, "Powertrain.CalcEnginePower(): input RPM: %10.3f | rpm_ratio: %10.3f | torque_ratio: %10.3f | addi_torque_value:  %10.3f | RESULT: %10.3f",
        rpm, rpmRatio, tqValue, atValue, result);
    LOG(msg);
    */

	return result;
}

float BeamEngine::getAccToHoldRPM(float rpm)
{
    // NO LOCK --- called from within UpdateBeamEngine() only (through CalcIdleMixture)

	float rpmRatio = rpm / (m_conf_engine_max_rpm * 1.25f);

	rpmRatio = std::min(rpmRatio, 1.0f);

	return (-m_conf_engine_braking_torque * rpmRatio) / CalcEnginePower(m_curr_engine_rpm);
}

float BeamEngine::CalcIdleMixture()
{
    // NO LOCK --- called from within UpdateBeamEngine() only


	if (m_curr_engine_rpm < m_conf_engine_idle_rpm)
	{
		// determine the fuel injection needed to counter the engine braking force
		float idleMix = getAccToHoldRPM(m_curr_engine_rpm);

		idleMix = std::max(0.06f, idleMix);

		idleMix = idleMix * (1.0f + (m_conf_engine_idle_rpm - m_curr_engine_rpm) / 100.0f);

		idleMix = std::max(m_conf_engine_min_idle_mixture, idleMix);
		idleMix = std::min(idleMix, m_conf_engine_max_idle_mixture);

		return idleMix;
	}

	return 0.0f;
}

float BeamEngine::CalcPrimeMixture()
{
    // NO LOCK --- called from within UpdateBeamEngine() only


	if (m_prime)
	{
        bool const do_lock = false;
		float crankfactor = getCrankFactor(do_lock);

		if (crankfactor < 0.9f)
		{
			// crankfactor is between 0.0f and 0.9f
			return 1.0f;
		} else if (crankfactor < 1.0f)
		{
			// crankfactor is between 0.9f and 1.0f
			return 10.0f * (1.0f - crankfactor);
		}
	}

	return 0.0f;
}

