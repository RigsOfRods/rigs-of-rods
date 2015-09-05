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
#include "BeamEngine.h"

#include "BeamFactory.h"
#include "Scripting.h"
#include "SoundScriptManager.h"
#include "TorqueCurve.h"

using namespace Ogre;

/*
ENGINE UPDATE TRACE (static):
bool RoRFrameListener::frame Started(const FrameEvent& evt)
	void BeamFactory::calc Physics(float dt)
		trucks[t]->engine->Update Beam Engine(dt, 1);
        [Executed for idle vehicles with engine m_is_engine_running (rig_t::state > DESACTIVATED)]

void Beam::run()
bool Beam::frameStep(Real dt)

void Beam::thread entry()
	void Beam::calc Forces Euler Compute(int doUpdate_int, Real dt, int step, int maxsteps)
		calc Truck Engine(doUpdate, dt);
			Update Beam Engine(doUpdate)
            [Executed for player-driven vehicle, for N steps (N dynamic based on frame-deltatime)]
			[doUpdate is 1 in first iteration, then 0]
*/

BeamEngine::BeamEngine(float m_conf_engine_min_rpm, float m_conf_engine_max_rpm, float torque, std::vector<float> gears, float dratio, int m_vehicle_index) :
	  m_air_pressure(0.0f)
	, m_auto_curr_acc(0.0f)
	, m_transmission_mode(AUTOMATIC)
	, m_autoselect(DRIVE)
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
}

BeamEngine::~BeamEngine()
{
	// delete NULL is safe
    /* ============== DISABLED for snapshotting, temporary ~only_a_ptr ================== //
	delete m_conf_engine_torque_curve;
	m_conf_engine_torque_curve = NULL;
    */
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

// TIGHT-LOOP: Called at least once per frame.
void BeamEngine::UpdateBeamEngine(float dt, int doUpdate)
{
	BeamEngine snapshot_before = *this;

	Beam* truck = BeamFactory::getSingleton().getTruck(this->m_vehicle_index);

	if (!truck) return;

	float acc = this->m_curr_acc;
	bool engine_is_electric = (m_conf_engine_type == 'e');

	acc = std::max(getIdleMixture(), acc);
	acc = std::max(getPrimeMixture(), acc);

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

	// braking by m_conf_engine_hydropump
	if (m_curr_engine_rpm > 100.0f)
	{
		totaltorque -= 8.0f * m_engine_hydropump / (m_curr_engine_rpm * 0.105f * dt);
	}

	if (m_is_engine_running && m_starter_has_contact && m_curr_engine_rpm < (m_conf_engine_max_rpm * 1.25f))
	{
		totaltorque += getEnginePower(m_curr_engine_rpm) * acc;
	}

	if (!engine_is_electric)
	{
		if (m_is_engine_running && m_curr_engine_rpm < m_conf_engine_stall_rpm)
		{
			stop(); //No, electric engine has no stop
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

	// clutch
	float retorque = 0.0f;

	if (m_curr_gear)
	{
		retorque = m_curr_clutch_torque / m_conf_gear_ratios[m_curr_gear + 1];
	}

	totaltorque -= retorque;

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

	if (m_transmission_mode < MANUAL)
	{
		// auto-shift
		if (m_is_shifting && !engine_is_electric) //No m_is_shifting in electric cars
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

			// end of m_is_shifting
			if (m_shift_clock > m_conf_shift_time)
			{
#ifdef USE_OPENAL
				SoundScriptManager::getSingleton().trigStop(m_vehicle_index, SS_TRIG_SHIFT);
#endif // USE_OPENAL
				setAcc(m_auto_curr_acc);
				m_is_shifting = 0;
				m_curr_clutch = 1.0f;
				m_is_post_shifting = true;
				m_post_shift_clock = 0.0f;
			}
		} else
			setAcc(m_auto_curr_acc);



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
				// we are m_is_shifting, just avoid stalling in worst case
				if (m_curr_engine_rpm < m_conf_engine_stall_rpm * 1.2f)
				{
					m_curr_clutch = 0.0f;
				}
			}
			else if (m_is_post_shifting)
			{
				// we are m_is_post_shifting, no gear change
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

		if (m_transmission_mode == AUTOMATIC && (m_autoselect == DRIVE || m_autoselect == TWO) && m_curr_gear > 0)
		{
			if ((m_curr_engine_rpm > m_conf_engine_max_rpm - 100.0f && m_curr_gear > 1) || m_cur_wheel_revolutions * m_conf_gear_ratios[m_curr_gear + 1] > m_conf_engine_max_rpm - 100.0f)
			{
				if ((m_autoselect == DRIVE && m_curr_gear < m_conf_num_gears) || (m_autoselect == TWO && m_curr_gear < std::min(2, m_conf_num_gears)) && !engine_is_electric)
				{
					this->BeamEngineShift(1);
				}
			} else if (m_curr_gear > 1 && m_ref_wheel_revolutions * m_conf_gear_ratios[m_curr_gear] < m_conf_engine_max_rpm && (m_curr_engine_rpm < m_conf_engine_min_rpm || (m_curr_engine_rpm < m_conf_engine_min_rpm + m_autotrans_curr_shift_behavior * halfRPMRange / 2.0f &&
				getEnginePower(m_cur_wheel_revolutions * m_conf_gear_ratios[m_curr_gear]) > getEnginePower(m_cur_wheel_revolutions * m_conf_gear_ratios[m_curr_gear + 1]))) && !engine_is_electric)
			{
				this->BeamEngineShift(-1);
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
						getEnginePower(m_cur_wheel_revolutions * m_conf_gear_ratios[newGear])   * m_conf_gear_ratios[newGear] >
						getEnginePower(m_cur_wheel_revolutions * m_conf_gear_ratios[newGear+1]) * m_conf_gear_ratios[newGear+1])
					{
						newGear--;
					}
				} else if (avgAcc50 > 0.6f && acc < 0.8f && acc > avgAcc50 + 0.1f && m_curr_engine_rpm < m_conf_engine_min_rpm + halfRPMRange)
				{
					if (newGear > 1 && m_cur_wheel_revolutions * m_conf_gear_ratios[newGear] < m_conf_engine_min_rpm + halfRPMRange &&
						getEnginePower(m_cur_wheel_revolutions * m_conf_gear_ratios[newGear])   * m_conf_gear_ratios[newGear] >
						getEnginePower(m_cur_wheel_revolutions * m_conf_gear_ratios[newGear+1]) * m_conf_gear_ratios[newGear+1])
					{
						newGear--;
					}
				} else if (avgAcc50 > 0.4f && acc < 0.8f && acc > avgAcc50 + 0.1f && m_curr_engine_rpm < m_conf_engine_min_rpm + halfRPMRange)
				{
					if (newGear > 1 && m_cur_wheel_revolutions * m_conf_gear_ratios[newGear] < m_conf_engine_min_rpm + oneThirdRPMRange &&
						getEnginePower(m_cur_wheel_revolutions * m_conf_gear_ratios[newGear])   * m_conf_gear_ratios[newGear] >
						getEnginePower(m_cur_wheel_revolutions * m_conf_gear_ratios[newGear+1]) * m_conf_gear_ratios[newGear+1])
					{
						newGear--;
					}
				}
				else if (m_curr_gear < (m_autoselect == TWO ? std::min(2, m_conf_num_gears) : m_conf_num_gears) &&
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
						this->BeamEngineShiftTo(newGear);
				}
			}


			if (m_autotrans_acc_buffer.size() > 200)
			{
				m_autotrans_rpm_buffer.pop_back();
				m_autotrans_acc_buffer.pop_back();
				m_autotrans_brake_buffer.pop_back();
			}
			// avoid over-revving
			if (m_transmission_mode <= SEMIAUTO && m_curr_gear != 0)
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

void BeamEngine::DEBUG_LogClassState(BeamEngine snapshot_before, float dt, int doUpdate)
{
    using namespace std;
    int width = 20;
	std::stringstream msg;
	msg.str().reserve(5000);
    pthread_t thread = pthread_self();
	
	msg << " ====== Engine updated (thread: " << thread.p 
        << ", truck: " << m_vehicle_index << ", deltatime: " << dt  
        << ", doUpdate: " << doUpdate << ") =========";
        /*
	<<"\nrefWheelRevolutions       "<< setw(width) << snapshot_before.m_ref_wheel_revolutions                      << ", " << setw(width) << m_ref_wheel_revolutions
	<<"\ncurWheelRevolutions       "<< setw(width) << snapshot_before.m_cur_wheel_revolutions                      << ", " << setw(width) << m_cur_wheel_revolutions 
	<<"\ncurGear                   "<< setw(width) << snapshot_before.m_curr_gear                                  << ", " << setw(width) << m_curr_gear
	<<"\ncurGearRange              "<< setw(width) << snapshot_before.m_curr_gear_range                             << ", " << setw(width) << m_curr_gear_range 
	<<"\nnumGears                  "<< setw(width) << snapshot_before.m_conf_num_gears                                 << ", " << setw(width) << m_conf_num_gears 
	<<"\nabsVelocity               "<< setw(width) << snapshot_before.m_abs_velocity                              << ", " << setw(width) << m_abs_velocity 
	<<"\nrelVelocity               "<< setw(width) << snapshot_before.m_rel_velocity                              << ", " << setw(width) << m_rel_velocity 
	<<"\nclutchForce               "<< setw(width) << snapshot_before.m_conf_clutch_force                              << ", " << setw(width) << m_conf_clutch_force
	<<"\nclutchTime                "<< setw(width) << snapshot_before.m_conf_clutch_time                               << ", " << setw(width) << m_conf_clutch_time
	<<"\ncurClutch                 "<< setw(width) << snapshot_before.m_curr_clutch                                << ", " << setw(width) << m_curr_clutch
	<<"\ncurClutchTorque           "<< setw(width) << snapshot_before.m_curr_clutch_torque                          << ", " << setw(width) << m_curr_clutch_torque
	<<"\ncontact                   "<< setw(width) << snapshot_before.m_starter_has_contact                                  << ", " << setw(width) << m_starter_has_contact 
	<<"\nhasair                    "<< setw(width) << snapshot_before.m_conf_engine_has_air                                   << ", " << setw(width) << m_conf_engine_has_air 
	<<"\nhasturbo                  "<< setw(width) << snapshot_before.m_conf_engine_has_turbo                                 << ", " << setw(width) << m_conf_engine_has_turbo 
	<<"\nrunning                   "<< setw(width) << snapshot_before.m_is_engine_running                                  << ", " << setw(width) << m_is_engine_running 
	<<"\ntype                      "<< setw(width) << snapshot_before.type                                     << ", " << setw(width) << type 
	<<"\nbrakingTorque             "<< setw(width) << snapshot_before.m_conf_engine_braking_torque                            << ", " << setw(width) << m_conf_engine_braking_torque 
	<<"\ncurAcc                    "<< setw(width) << snapshot_before.m_curr_acc                                   << ", " << setw(width) << m_curr_acc 
	<<"\ncurEngineRPM              "<< setw(width) << snapshot_before.m_curr_engine_rpm                             << ", " << setw(width) << m_curr_engine_rpm 
	<<"\ndiffRatio                 "<< setw(width) << snapshot_before.m_conf_engine_diff_ratio                                << ", " << setw(width) << m_conf_engine_diff_ratio 
	<<"\nengineTorque              "<< setw(width) << snapshot_before.m_conf_engine_torque                             << ", " << setw(width) << m_conf_engine_torque 
	<<"\nhydropump                 "<< setw(width) << snapshot_before.m_conf_engine_hydropump                                << ", " << setw(width) << m_conf_engine_hydropump 
	<<"\nidleRPM                   "<< setw(width) << snapshot_before.m_conf_engine_idle_rpm                                  << ", " << setw(width) << m_conf_engine_idle_rpm 
	<<"\nminIdleMixture            "<< setw(width) << snapshot_before.m_conf_engine_min_idle_mixture                           << ", " << setw(width) << m_conf_engine_min_idle_mixture 
	<<"\nmaxIdleMixture            "<< setw(width) << snapshot_before.m_conf_engine_max_idle_mixture                           << ", " << setw(width) << m_conf_engine_max_idle_mixture 
	<<"\ninertia                   "<< setw(width) << snapshot_before.m_conf_engine_inertia                                  << ", " << setw(width) << m_conf_engine_inertia 
	<<"\nmaxRPM                    "<< setw(width) << snapshot_before.m_conf_engine_max_rpm                                   << ", " << setw(width) << m_conf_engine_max_rpm 
	<<"\nminRPM                    "<< setw(width) << snapshot_before.m_conf_engine_min_rpm                                   << ", " << setw(width) << m_conf_engine_min_rpm 
	<<"\nstallRPM                  "<< setw(width) << snapshot_before.m_conf_engine_stall_rpm                                 << ", " << setw(width) << m_conf_engine_stall_rpm 
	<<"\nprime                     "<< setw(width) << snapshot_before.prime                                    << ", " << setw(width) << prime 
	<<"\npost_shift_time           "<< setw(width) << snapshot_before.m_conf_post_shift_time                          << ", " << setw(width) << m_conf_post_shift_time 
	<<"\npostshiftclock            "<< setw(width) << snapshot_before.m_post_shift_clock                           << ", " << setw(width) << m_post_shift_clock
	<<"\nshift_time                "<< setw(width) << snapshot_before.m_conf_shift_time                               << ", " << setw(width) << m_conf_shift_time 
	<<"\nshiftclock                "<< setw(width) << snapshot_before.m_shift_clock                               << ", " << setw(width) << m_shift_clock
	<<"\npostshifting              "<< setw(width) << snapshot_before.m_is_post_shifting                             << ", " << setw(width) << m_is_post_shifting
	<<"\nshifting                  "<< setw(width) << snapshot_before.m_is_shifting                                 << ", " << setw(width) << m_is_shifting
	<<"\nshiftval                  "<< setw(width) << snapshot_before.m_curr_gear_change_relative                                 << ", " << setw(width) << m_curr_gear_change_relative
	<<"\nautoselect                "<< setw(width) << snapshot_before.m_autoselect                               << ", " << setw(width) << m_autoselect
	<<"\nautocurAcc                "<< setw(width) << snapshot_before.m_auto_curr_acc                               << ", " << setw(width) << m_auto_curr_acc
	<<"\nstarter                   "<< setw(width) << snapshot_before.m_starter_is_running                                  << ", " << setw(width) << m_starter_is_running
	<<"\nfullRPMRange              "<< setw(width) << snapshot_before.m_conf_autotrans_full_rpm_range                             << ", " << setw(width) << m_conf_autotrans_full_rpm_range
	<<"\noneThirdRPMRange          "<< setw(width) << snapshot_before.oneThirdRPMRange                         << ", " << setw(width) << oneThirdRPMRange
	<<"\nhalfRPMRange              "<< setw(width) << snapshot_before.halfRPMRange                             << ", " << setw(width) << halfRPMRange
	<<"\nshiftBehaviour            "<< setw(width) << snapshot_before.m_autotrans_curr_shift_behavior                           << ", " << setw(width) << m_autotrans_curr_shift_behavior
	<<"\nupShiftDelayCounter       "<< setw(width) << snapshot_before.m_autotrans_up_shift_delay_counter                      << ", " << setw(width) << m_autotrans_up_shift_delay_counter
	<<"\nturboVer                  "<< setw(width) << snapshot_before.m_conf_turbo_version                                 << ", " << setw(width) << m_conf_turbo_version
	<<"\ncurTurboRPM[MAX_NUM_TURBOS]     "<< setw(width) << snapshot_before.m_turbo_curr_rpm[MAX_NUM_TURBOS]                    << ", " << setw(width) << m_turbo_curr_rpm[MAX_NUM_TURBOS]
	<<"\nturboInertiaFactor        "<< setw(width) << snapshot_before.m_conf_turbo_inertia_factor                       << ", " << setw(width) << m_conf_turbo_inertia_factor
	<<"\nnumTurbos                 "<< setw(width) << snapshot_before.m_conf_num_turbos                                << ", " << setw(width) << m_conf_num_turbos
	<<"\nmaxTurboRPM               "<< setw(width) << snapshot_before.m_conf_turbo_max_rpm                              << ", " << setw(width) << m_conf_turbo_max_rpm
	<<"\nturbotorque               "<< setw(width) << snapshot_before.m_turbo_torque                              << ", " << setw(width) << m_turbo_torque
	<<"\nturboInertia              "<< setw(width) << snapshot_before.m_turbo_inertia                             << ", " << setw(width) << m_turbo_inertia
	<<"\nEngineAddiTorque[MAX_NUM_TURBOS]"<< setw(width) << snapshot_before.m_conf_turbo_addi_torque[MAX_NUM_TURBOS]               << ", " << setw(width) << m_conf_turbo_addi_torque[MAX_NUM_TURBOS]
	<<"\nturboEngineRpmOperation   "<< setw(width) << snapshot_before.m_conf_turbo_engine_rpm_operation                  << ", " << setw(width) << m_conf_turbo_engine_rpm_operation
	<<"\nturboMaxPSI               "<< setw(width) << snapshot_before.m_conf_turbo_max_psi                              << ", " << setw(width) << m_conf_turbo_max_psi
	<<"\nturboPSI                  "<< setw(width) << snapshot_before.m_turbo_psi                                 << ", " << setw(width) << m_turbo_psi
	<<"\nb_BOV                     "<< setw(width) << snapshot_before.m_conf_turbo_has_bov                                    << ", " << setw(width) << m_conf_turbo_has_bov
	<<"\ncurBOVTurboRPM[MAX_NUM_TURBOS]  "<< setw(width) << snapshot_before.m_turbo_cur_bov_rpm[MAX_NUM_TURBOS]                 << ", " << setw(width) << m_turbo_cur_bov_rpm[MAX_NUM_TURBOS]
	<<"\nturboBOVtorque            "<< setw(width) << snapshot_before.m_turbo_bov_torque                           << ", " << setw(width) << m_turbo_bov_torque
	<<"\nminBOVPsi                 "<< setw(width) << snapshot_before.m_conf_turbo_min_bov_psi                                << ", " << setw(width) << m_conf_turbo_min_bov_psi
	<<"\nb_WasteGate               "<< setw(width) << snapshot_before.m_conf_turbo_has_wastegate                              << ", " << setw(width) << m_conf_turbo_has_wastegate
	<<"\nminWGPsi                  "<< setw(width) << snapshot_before.m_conf_turbo_wg_min_psi                                 << ", " << setw(width) << m_conf_turbo_wg_min_psi
	<<"\nb_flutter                 "<< setw(width) << snapshot_before.m_conf_turbo_has_flutter                                << ", " << setw(width) << m_conf_turbo_has_flutter
	<<"\nwastegate_threshold_p     "<< setw(width) << snapshot_before.m_conf_turbo_wg_threshold_p                    << ", " << setw(width) << m_conf_turbo_wg_threshold_p
    <<"\nwastegate_threshold_n     "<< setw(width) << snapshot_before.m_conf_turbo_wg_threshold_n                    << ", " << setw(width) << m_conf_turbo_wg_threshold_n
	<<"\nb_anti_lag                "<< setw(width) << snapshot_before.m_conf_turbo_has_antilag                               << ", " << setw(width) << m_conf_turbo_has_antilag
	<<"\nminRPM_antilag            "<< setw(width) << snapshot_before.m_conf_turbo_antilag_min_rpm                           << ", " << setw(width) << m_conf_turbo_antilag_min_rpm
	<<"\nrnd_antilag_chance        "<< setw(width) << snapshot_before.m_conf_turbo_antilag_chance_rand                       << ", " << setw(width) << m_conf_turbo_antilag_chance_rand
	<<"\nantilag_power_factor      "<< setw(width) << snapshot_before.m_conf_turbo_antilag_power_factor                     << ", " << setw(width) << m_conf_turbo_antilag_power_factor
	<<"\nm_air_pressure            "<< setw(width) << snapshot_before.m_air_pressure                           << ", " << setw(width) << m_air_pressure
	<<"\nautomode                  "<< setw(width) << snapshot_before.m_transmission_mode                                 << ", " << setw(width) << m_transmission_mode
    ;*/
    //msg << "\nDEQUE m_autotrans_rpm_buffer BEFORE"; LOG_DEQUE_FLOAT(msg, snapshot_before.m_autotrans_rpm_buffer); 
    //msg << "\nDEQUE m_autotrans_rpm_buffer   AFTER"; LOG_DEQUE_FLOAT(msg, m_autotrans_rpm_buffer);
    //msg << "\nDEQUE m_autotrans_acc_buffer BEFORE"; LOG_DEQUE_FLOAT(msg, snapshot_before.m_autotrans_acc_buffer); 
    //msg << "\nDEQUE m_autotrans_acc_buffer   AFTER"; LOG_DEQUE_FLOAT(msg, m_autotrans_acc_buffer);
    //msg << "\nDEQUE m_autotrans_brake_buffer BEFORE"; LOG_DEQUE_FLOAT(msg, snapshot_before.m_autotrans_brake_buffer); 
    //msg << "\nDEQUE m_autotrans_brake_buffer AFTER"; LOG_DEQUE_FLOAT(msg, m_autotrans_brake_buffer);

    LOG(msg.str());
}

void BeamEngine::UpdateBeamEngineAudio(int doUpdate)
{
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
	return m_curr_engine_rpm;
}

void BeamEngine::toggleAutoMode()
{
	m_transmission_mode = (m_transmission_mode + 1) % (MANUAL_RANGES + 1);

	// this switches off all automatic symbols when in manual mode
	if (m_transmission_mode != AUTOMATIC)
	{
		m_autoselect = MANUALMODE;
	} else
	{
		m_autoselect = NEUTRAL;
	}

	if (m_transmission_mode == MANUAL_RANGES)
	{
		this->setGearRange(0);
		this->setGear(0);
	}
}

int BeamEngine::getAutoMode()
{
	return m_transmission_mode;
}

void BeamEngine::setAutoMode(int mode)
{
	m_transmission_mode = mode;
}

void BeamEngine::setAcc(float val)
{
	m_curr_acc = val;
}

float BeamEngine::getTurboPSI()
{
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
	return m_curr_acc;
}

// this is mainly for smoke...
void BeamEngine::netForceSettings(float rpm, float force, float clutch, int gear, bool _running, bool _contact, char _automode)
{
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
	if (m_is_engine_running)
	{
		return m_curr_acc * (1.0f - m_turbo_curr_rpm[0] /* doesn't matter */ / m_conf_turbo_max_rpm);// * m_conf_engine_torque / 5000.0f;
	}

	return -1;
}

float BeamEngine::getTorque()
{
	if (m_curr_clutch_torque >  1000000.0) return  1000000.0;
	if (m_curr_clutch_torque < -1000000.0) return -1000000.0;
	return m_curr_clutch_torque;
}

void BeamEngine::setRPM(float rpm)
{
	m_curr_engine_rpm = rpm;
}

void BeamEngine::setSpin(float rpm)
{
	m_cur_wheel_revolutions = rpm;
}

// for hydros acceleration
float BeamEngine::getCrankFactor()
{
	float minWorkingRPM = m_conf_engine_idle_rpm * 1.1f; // minWorkingRPM > m_conf_engine_idle_rpm avoids commands deadlocking the engine

	float rpmRatio = (m_curr_engine_rpm - minWorkingRPM) / (m_conf_engine_max_rpm - minWorkingRPM);
	rpmRatio = std::max(0.0f, rpmRatio); // Avoids a negative rpmRatio when m_curr_engine_rpm < minWorkingRPM
	rpmRatio = std::min(rpmRatio, 1.0f); // Avoids a rpmRatio > 1.0f when m_curr_engine_rpm > m_conf_engine_max_rpm

	float crankfactor = 5.0f * rpmRatio;

	return crankfactor;
}

void BeamEngine::setClutch(float clutch)
{
	m_curr_clutch = clutch;
}

float BeamEngine::getClutch()
{
	return m_curr_clutch;
}

float BeamEngine::getClutchForce()
{
	return m_conf_clutch_force;
}

void BeamEngine::toggleContact()
{
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
void BeamEngine::start()
{
	if (m_transmission_mode == AUTOMATIC)
	{
		m_curr_gear = 1;
		m_autoselect = DRIVE;
	} else
	{
		if (m_transmission_mode == SEMIAUTO)
		{
			m_curr_gear = 1;
		} else
		{
			m_curr_gear = 0;
		}
		m_autoselect = MANUALMODE;
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
	setAcc(0.0f);
	SoundScriptManager::getSingleton().trigStart(m_vehicle_index, SS_TRIG_ENGINE);
#endif // USE_OPENAL
}

void BeamEngine::offstart()
{
	m_curr_gear = 0;
	m_curr_clutch = 0.0f;
	if (m_conf_engine_type != 'e') // e = Electric engine
		m_autoselect = NEUTRAL; //no Neutral in electric engines
	else
		m_autoselect = ONE;

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
	m_starter_is_running = (v == 1);
	if (v && m_curr_engine_rpm < 750.0f)
	{
		setAcc(1.0f);
	}
}

int BeamEngine::getGear()
{
	return m_curr_gear;
}

// low level gear changing
void BeamEngine::setGear(int v)
{
	m_curr_gear = v;
}

int BeamEngine::getGearRange()
{
	return m_curr_gear_range;
}

void BeamEngine::setGearRange(int v)
{
	m_curr_gear_range = v;
}

void BeamEngine::stop()
{
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
	m_auto_curr_acc = val;
	if (!m_is_shifting)
	{
		setAcc(val);
	}
}

void BeamEngine::BeamEngineShift(int val)
{
	if (!val || m_curr_gear + val < -1 || m_curr_gear + val > getNumGears()) return;
	if (m_transmission_mode < MANUAL)
	{
#ifdef USE_OPENAL
		SoundScriptManager::getSingleton().trigStart(m_vehicle_index, SS_TRIG_SHIFT);
#endif // USE_OPENAL
		m_curr_gear_change_relative = val;
		m_is_shifting = 1;
		m_shift_clock = 0.0f;
		setAcc(0.0f);
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

void BeamEngine::BeamEngineShiftTo(int newGear)
{
	this->BeamEngineShift(newGear - m_curr_gear);
}

void BeamEngine::UpdateBeamEngineShifts()
{
	if (m_autoselect == MANUALMODE) return;

#ifdef USE_OPENAL
	SoundScriptManager::getSingleton().trigOnce(m_vehicle_index, SS_TRIG_SHIFT);
#endif // USE_OPENAL
	bool engine_is_electric = (m_conf_engine_type == 'e');

	if (m_autoselect == REAR)
	{
		m_curr_gear = -1;
	} else if (m_autoselect == NEUTRAL && !engine_is_electric)
	{
		m_curr_gear =  0;
	} else if (m_autoselect == ONE)
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
		
		if (m_autoselect == TWO)
		{
			m_curr_gear = std::min(m_curr_gear, 2);
		}
	}
}

void BeamEngine::autoShiftSet(int mode)
{
	m_autoselect = (autoswitch)mode;
	this->UpdateBeamEngineShifts();
}

void BeamEngine::autoShiftUp()
{
	if (m_autoselect != REAR)
	{
		m_autoselect = (autoswitch)(m_autoselect-1);
		this->UpdateBeamEngineShifts();
	}
}

void BeamEngine::autoShiftDown()
{
	if (m_autoselect != ONE)
	{
		m_autoselect = (autoswitch)(m_autoselect+1);
		this->UpdateBeamEngineShifts();
	}
}

int BeamEngine::getAutoShift()
{
	return (int)m_autoselect;
}

void BeamEngine::setManualClutch(float val)
{
	if (m_transmission_mode >= MANUAL)
	{
		val = std::max(0.0f, val);
		m_curr_clutch = 1.0 - val;
	}
}

float BeamEngine::getEnginePower(float rpm)
{
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
			atValue = (((getTurboPSI() * 6.8) * m_conf_engine_torque) / 100); //1psi = 6% more power
		}
	}

	return (m_conf_engine_torque * tqValue) + atValue;
}

float BeamEngine::getAccToHoldRPM(float rpm)
{
	float rpmRatio = rpm / (m_conf_engine_max_rpm * 1.25f);

	rpmRatio = std::min(rpmRatio, 1.0f);

	return (-m_conf_engine_braking_torque * rpmRatio) / getEnginePower(m_curr_engine_rpm);
}

float BeamEngine::getIdleMixture()
{
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

float BeamEngine::getPrimeMixture()
{
	if (m_prime)
	{
		float crankfactor = getCrankFactor();

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
