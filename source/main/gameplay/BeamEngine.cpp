/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
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

#include "BeamEngine.h"

#include "ApproxMath.h" // frand()
#include "BeamFactory.h"
#include "Scripting.h"
#include "SoundScriptManager.h"
#include "TorqueCurve.h"

using namespace Ogre;

EngineSim::EngineSim(float _min_rpm, float _max_rpm, float torque, std::vector<float> gears, float dratio, Actor* actor) :
    m_air_pressure(0.0f)
    , m_auto_cur_acc(0.0f)
    , m_auto_mode(AUTOMATIC)
    , m_autoselect(DRIVE)
    , m_braking_torque(-torque / 5.0f)
    , m_clutch_force(10000.0f)
    , m_clutch_time(0.2f)
    , m_starter_has_contact(false)
    , m_cur_acc(0.0f)
    , m_cur_clutch(0.0f)
    , m_cur_clutch_torque(0.0f)
    , m_cur_engine_rpm(0.0f)
    , m_cur_gear(0)
    , m_cur_gear_range(0)
    , m_cur_wheel_revolutions(0.0f)
    , m_diff_ratio(dratio)
    , m_engine_torque(torque - m_braking_torque)
    , m_gear_ratios(gears)
    , m_engine_has_air(true)
    , m_engine_has_turbo(true)
    , m_hydropump_state(0.0f)
    , m_idle_rpm(std::min(_min_rpm, 800.0f))
    , m_engine_inertia(10.0f)
    , m_kickdown_delay_counter(0)
    , m_max_idle_mixture(0.2f)
    , m_engine_max_rpm(std::abs(_max_rpm))
    , m_min_idle_mixture(0.0f)
    , m_engine_min_rpm(std::abs(_min_rpm))
    , m_num_gears((int)gears.size() - 2)
    , m_post_shift_time(0.2f)
    , m_post_shift_clock(0.0f)
    , m_post_shifting(0)
    , m_engine_is_priming(false)
    , m_engine_is_running(false)
    , m_shift_behaviour(0.0f)
    , m_shift_time(0.5f)
    , m_shift_clock(0.0f)
    , m_shifting(0)
    , m_shift_val(0)
    , m_engine_stall_rpm(300.0f)
    , m_starter(0)
    , m_torque_curve(new TorqueCurve())
    , m_actor(actor)
    , m_engine_turbo_mode(OLD)
    , m_engine_type('t')
    , m_upshift_delay_counter(0)
    , m_engine_is_electric(false)
    , m_turbo_inertia_factor(1)
    , m_num_turbos(1)
    , m_max_turbo_rpm(200000.0f)
    , m_turbo_engine_rpm_operation(0.0f)
    , m_turbo_ver(1)
    , m_min_bov_psi(11)
    , m_min_wastegate_psi(20)
    , m_turbo_has_wastegate(false)
    , m_turbo_has_bov(false)
    , m_turbo_flutters(false)
    , m_turbo_wg_threshold_p(0)
    , m_turbo_wg_threshold_n(0)
    , m_turbo_has_antilag(false)
    , m_antilag_rand_chance(0.9975)
    , m_antilag_min_rpm(3000)
    , m_antilag_power_factor(170)
{
    m_full_rpm_range = (m_engine_max_rpm - m_engine_min_rpm);
    m_one_third_rpm_range = m_full_rpm_range / 3.0f;
    m_half_rpm_range = m_full_rpm_range / 2.0f;

    m_gear_ratios[0] = -m_gear_ratios[0];
    for (std::vector<float>::iterator it = m_gear_ratios.begin(); it != m_gear_ratios.end(); ++it)
    {
        (*it) *= m_diff_ratio;
    }

    for (int i = 0; i < MAXTURBO; i++)
    {
        m_engine_addi_torque[i] = 0;
        m_cur_turbo_rpm[i] = 0;
    }
}

EngineSim::~EngineSim()
{
    // delete NULL is safe
    delete m_torque_curve;
    m_torque_curve = NULL;
}

void EngineSim::SetTurboOptions(int type, float tinertiaFactor, int nturbos, float param1, float param2, float param3, float param4, float param5, float param6, float param7, float param8, float param9, float param10, float param11)
{
    m_engine_has_turbo = true;
    m_engine_turbo_mode = NEW;

    if (nturbos > MAXTURBO)
    {
        m_num_turbos = 4;
        LOG("Turbo: No more than 4 turbos allowed"); //TODO: move this under RigParser
    }
    else
        m_num_turbos = nturbos;

    m_turbo_ver = type;
    m_turbo_inertia_factor = tinertiaFactor;

    if (param2 != 9999)
        m_turbo_engine_rpm_operation = param2;

    if (m_turbo_ver == 1)
    {
        for (int i = 0; i < m_num_turbos; i++)
            m_engine_addi_torque[i] = param1 / m_num_turbos; //Additional torque
    }
    else
    {
        m_turbo_max_psi = param1; //maxPSI
        m_max_turbo_rpm = m_turbo_max_psi * 10000;

        //Duh
        if (param3 == 1)
            m_turbo_has_bov = true;
        else
            m_turbo_has_bov = false;

        if (param3 != 9999)
            m_min_bov_psi = param4;

        if (param5 == 1)
            m_turbo_has_wastegate = true;
        else
            m_turbo_has_wastegate = false;

        if (param6 != 9999)
            m_min_wastegate_psi = param6 * 10000;

        if (param7 != 9999)
        {
            m_turbo_wg_threshold_n = 1 - param7;
            m_turbo_wg_threshold_p = 1 + param7;
        }

        if (param8 == 1)
            m_turbo_has_antilag = true;
        else
            m_turbo_has_antilag = false;

        if (param9 != 9999)
            m_antilag_rand_chance = param9;

        if (param10 != 9999)
            m_antilag_min_rpm = param10;

        if (param11 != 9999)
            m_antilag_power_factor = param11;
    }
}

void EngineSim::SetEngineOptions(float einertia, char etype, float eclutch, float ctime, float stime, float pstime, float irpm, float srpm, float maximix, float minimix)
{
    m_engine_inertia = einertia;
    m_engine_type = etype;
    m_clutch_force = eclutch;

    if (ctime > 0)
        m_clutch_time = ctime;
    if (stime > 0)
        m_shift_time = stime;
    if (pstime > 0)
        m_post_shift_time = pstime;
    if (irpm > 0)
        m_idle_rpm = irpm;
    if (srpm > 0)
        m_engine_stall_rpm = srpm;
    if (maximix > 0)
        m_max_idle_mixture = maximix;
    if (minimix > 0)
        m_min_idle_mixture = minimix;

    m_clutch_time = std::max(0.0f, m_clutch_time);
    m_shift_time = std::max(0.0f, m_shift_time);
    m_post_shift_time = std::max(0.0f, m_post_shift_time);

    m_clutch_time = std::min(m_clutch_time, m_shift_time * 0.9f);

    m_idle_rpm = std::max(0.0f, m_idle_rpm);
    m_engine_stall_rpm = std::max(0.0f, m_engine_stall_rpm);
    m_engine_stall_rpm = std::min(m_idle_rpm, m_engine_stall_rpm);

    if (etype == 'c')
    {
        // it's a car!
        m_engine_has_air = false;
        m_engine_has_turbo = false;
        m_engine_is_electric = false;
        // set default clutch force
        if (m_clutch_force < 0.0f)
        {
            m_clutch_force = 5000.0f;
        }
    }
    else if (etype == 'e') //electric
    {
        m_engine_is_electric = true;
        m_engine_has_air = false;
        m_engine_has_turbo = false;
        if (m_clutch_force < 0.0f)
        {
            m_clutch_force = 5000.0f;
        }
    }
    else
    {
        m_engine_is_electric = false;
        // it's a truck
        if (m_clutch_force < 0.0f)
        {
            m_clutch_force = 10000.0f;
        }
    }
}

void EngineSim::UpdateEngineSim(float dt, int doUpdate)
{
    float acc = m_cur_acc;

    acc = std::max(getIdleMixture(), acc);
    acc = std::max(getPrimeMixture(), acc);

    if (doUpdate)
    {
        SOUND_MODULATE(m_actor, SS_MOD_INJECTOR, acc);
    }

    if (m_engine_has_air)
    {
        // air pressure
        m_air_pressure += dt * m_cur_engine_rpm;
        if (m_air_pressure > 50000.0f)
        {
            SOUND_PLAY_ONCE(m_actor, SS_TRIG_AIR_PURGE);
            m_air_pressure = 0.0f;
        }
    }

    if (m_engine_has_turbo)
    {
        if (m_engine_turbo_mode == OLD)
        {
            // update turbo speed (lag)
            float turbotorque = 0.0f;
            float turboInertia = 0.000003f;

            // braking (compression)
            turbotorque -= m_cur_turbo_rpm[0] / 200000.0f;

            // powering (exhaust) with limiter
            if (m_cur_turbo_rpm[0] < 200000.0f && m_engine_is_running && m_cur_acc > 0.06f)
            {
                turbotorque += 1.5f * m_cur_acc * (m_cur_engine_rpm / m_engine_max_rpm);
            }
            else
            {
                turbotorque += 0.1f * (m_cur_engine_rpm / m_engine_max_rpm);
            }

            // integration
            m_cur_turbo_rpm[0] += dt * turbotorque / turboInertia;
        }
        else
        {
            for (int i = 0; i < m_num_turbos; i++)
            {
                // update turbo speed (lag)
                // reset each of the values for each turbo
                float turbotorque = 0.0f;
                float turboBOVtorque = 0.0f;

                float turboInertia = 0.000003f * m_turbo_inertia_factor;

                // braking (compression)
                turbotorque -= m_cur_turbo_rpm[i] / m_max_turbo_rpm;
                turboBOVtorque -= m_turbo_bov_rpm[i] / m_max_turbo_rpm;

                // powering (exhaust) with limiter
                if (m_cur_engine_rpm >= m_turbo_engine_rpm_operation)
                {
                    if (m_cur_turbo_rpm[i] <= m_max_turbo_rpm && m_engine_is_running && acc > 0.06f)
                    {
                        if (m_turbo_has_wastegate)
                        {
                            if (m_cur_turbo_rpm[i] < m_min_wastegate_psi * m_turbo_wg_threshold_p && !m_turbo_flutters)
                            {
                                turbotorque += 1.5f * acc * (((m_cur_engine_rpm - m_turbo_engine_rpm_operation) / (m_engine_max_rpm - m_turbo_engine_rpm_operation)));
                            }
                            else
                            {
                                m_turbo_flutters = true;
                                turbotorque -= (m_cur_turbo_rpm[i] / m_max_turbo_rpm) * 1.5;
                            }

                            if (m_turbo_flutters)
                            {
                                SOUND_START(m_actor, SS_TRIG_TURBOWASTEGATE);
                                if (m_cur_turbo_rpm[i] < m_min_wastegate_psi * m_turbo_wg_threshold_n)
                                {
                                    m_turbo_flutters = false;
                                    SOUND_STOP(m_actor, SS_TRIG_TURBOWASTEGATE);
                                }
                            }
                        }
                        else
                            turbotorque += 1.5f * acc * (((m_cur_engine_rpm - m_turbo_engine_rpm_operation) / (m_engine_max_rpm - m_turbo_engine_rpm_operation)));
                    }
                    else
                    {
                        turbotorque += 0.1f * (((m_cur_engine_rpm - m_turbo_engine_rpm_operation) / (m_engine_max_rpm - m_turbo_engine_rpm_operation)));
                    }

                    //Update waste gate, it's like a BOV on the exhaust part of the turbo, acts as a limiter
                    if (m_turbo_has_wastegate)
                    {
                        if (m_cur_turbo_rpm[i] > m_min_wastegate_psi * 0.95)
                            turboInertia = turboInertia * 0.7; //Kill inertia so it flutters
                        else
                            turboInertia = turboInertia * 1.3; //back to normal inertia
                    }
                }

                //simulate compressor surge
                if (!m_turbo_has_bov)
                {
                    if (m_cur_turbo_rpm[i] > 13 * 10000 && m_cur_acc < 0.06f)
                    {
                        turbotorque += (turbotorque * 2.5);
                    }
                }

                // anti lag
                if (m_turbo_has_antilag && m_cur_acc < 0.5)
                {
                    float f = frand();
                    if (m_cur_engine_rpm > m_antilag_min_rpm && f > m_antilag_rand_chance)
                    {
                        if (m_cur_turbo_rpm[i] > m_max_turbo_rpm * 0.35 && m_cur_turbo_rpm[i] < m_max_turbo_rpm)
                        {
                            turbotorque -= (turbotorque * (f * m_antilag_power_factor));
                            SOUND_START(m_actor, SS_TRIG_TURBOBACKFIRE);
                        }
                    }
                    else
                    {
                        SOUND_STOP(m_actor, SS_TRIG_TURBOBACKFIRE);
                    }
                }

                // update main turbo rpm
                m_cur_turbo_rpm[i] += dt * turbotorque / turboInertia;

                //Update BOV
                //It's basicly an other turbo which is limmited to the main one's rpm, but it doesn't affect its rpm.  It only affects the power going into the engine.
                //This one is used to simulate the pressure between the engine and the compressor.
                //I should make the whole turbo code work this way. -Max98
                if (m_turbo_has_bov)
                {
                    if (m_turbo_bov_rpm[i] < m_cur_turbo_rpm[i])
                        turboBOVtorque += 1.5f * acc * (((m_cur_engine_rpm) / (m_engine_max_rpm)));
                    else
                        turboBOVtorque += 0.07f * (((m_cur_engine_rpm) / (m_engine_max_rpm)));

                    if (m_cur_acc < 0.06 && m_cur_turbo_rpm[i] > m_min_bov_psi * 10000)
                    {
                        SOUND_START(m_actor, SS_TRIG_TURBOBOV);
                        m_turbo_bov_rpm[i] += dt * turboBOVtorque / (turboInertia * 0.1);
                    }
                    else
                    {
                        SOUND_STOP(m_actor, SS_TRIG_TURBOBOV);
                        if (m_turbo_bov_rpm[i] < m_cur_turbo_rpm[i])
                            m_turbo_bov_rpm[i] += dt * turboBOVtorque / (turboInertia * 0.05);
                        else
                            m_turbo_bov_rpm[i] += dt * turboBOVtorque / turboInertia;
                    }
                }
            }
        }
    }

    // update engine speed
    float totaltorque = 0.0f;

    // engine braking
    if (m_starter_has_contact)
    {
        totaltorque += m_braking_torque * m_cur_engine_rpm / m_engine_max_rpm;
    }
    else
    {
        totaltorque += m_braking_torque;
    }

    // braking by m_hydropump_state
    if (m_cur_engine_rpm > 100.0f)
    {
        totaltorque -= 8.0f * m_hydropump_state / (m_cur_engine_rpm * 0.105f * dt);
    }

    if (m_engine_is_running && m_starter_has_contact && m_cur_engine_rpm < (m_engine_max_rpm * 1.25f))
    {
        totaltorque += getEnginePower(m_cur_engine_rpm) * acc;
    }

    if (!m_engine_is_electric)
    {
        if (m_engine_is_running && m_cur_engine_rpm < m_engine_stall_rpm)
        {
            this->StopEngine();
        }

        if (m_starter_has_contact && m_starter && !m_engine_is_running)
        {
            if (m_cur_engine_rpm < m_engine_stall_rpm)
            {
                totaltorque += -m_braking_torque;
            }
            else
            {
                m_engine_is_running = true;
                SOUND_START(m_actor, SS_TRIG_ENGINE);
            }
        }
    }

    // clutch
    float retorque = 0.0f;

    if (m_cur_gear)
    {
        retorque = m_cur_clutch_torque / m_gear_ratios[m_cur_gear + 1];
    }

    totaltorque -= retorque;

    // integration
    m_cur_engine_rpm += dt * totaltorque / m_engine_inertia;

    // update clutch torque
    if (m_cur_gear)
    {
        float gearboxspinner = m_cur_engine_rpm / m_gear_ratios[m_cur_gear + 1];
        m_cur_clutch_torque = (gearboxspinner - m_cur_wheel_revolutions) * m_cur_clutch * m_clutch_force;
    }
    else
    {
        m_cur_clutch_torque = 0.0f;
    }

    m_cur_engine_rpm = std::max(0.0f, m_cur_engine_rpm);

    if (m_auto_mode < MANUAL)
    {
        // auto-shift
        if (m_shifting)
        {
            m_shift_clock += dt;

            if (m_shift_val)
            {
                float declutchTime = std::min(m_shift_time - m_clutch_time, m_clutch_time);
                if (m_shift_clock <= declutchTime)
                {
                    // depress the clutch 
                    float ratio = pow(1.0f - (m_shift_clock / declutchTime), 2);
                    m_cur_clutch = std::min(ratio, m_cur_clutch);
                    m_cur_acc = std::min(ratio, m_auto_cur_acc);
                }
                else
                {
                    // engage a gear
                    SOUND_START(m_actor, SS_TRIG_SHIFT);
                    if (m_autoselect != NEUTRAL)
                    {
                        m_cur_gear += m_shift_val;
                        m_cur_gear = std::max(-1, m_cur_gear);
                        m_cur_gear = std::min(m_cur_gear, m_num_gears);
                    }
                    m_shift_val = 0;
                }
            }

            if (m_shift_clock > m_shift_time)
            {
                // we're done m_shifting
                SOUND_STOP(m_actor, SS_TRIG_SHIFT);
                SetAcceleration(m_auto_cur_acc);
                m_shifting = 0;
                m_post_shifting = 1;
                m_post_shift_clock = 0.0f;
            }
            else if (!m_shift_val && m_cur_gear && m_shift_clock >= (m_shift_time - m_clutch_time))
            {
                // release the clutch <-- Done below
                float timer = m_shift_clock - (m_shift_time - m_clutch_time);
                float ratio = sqrt(timer / m_clutch_time);
                m_cur_acc = (m_auto_cur_acc / 2.0f) * ratio;
            }
        }

        if (m_post_shifting)
        {
            m_post_shift_clock += dt;
            if (m_post_shift_clock > m_post_shift_time)
            {
                m_post_shifting = 0;
            }
            else if (m_auto_cur_acc > 0.0f)
            {
                float ratio = m_post_shift_clock / m_post_shift_time;
                m_cur_acc = (m_auto_cur_acc / 2.0f) + (m_auto_cur_acc / 2.0f) * ratio;
            }
            else if (m_cur_gear)
            {
                float gearboxspinner = m_cur_engine_rpm / m_gear_ratios[m_cur_gear + 1];
                if (m_cur_wheel_revolutions > gearboxspinner)
                {
                    float ratio = sqrt(m_post_shift_clock / m_post_shift_time);
                    m_cur_clutch = std::max(m_cur_clutch, ratio);
                }
            }
        }

        // auto clutch
        float declutchRPM = (m_engine_min_rpm + m_engine_stall_rpm) / 2.0f;
        if (m_cur_gear == 0 || m_cur_engine_rpm < declutchRPM)
        {
            m_cur_clutch = 0.0f;
        }
        else if (m_cur_engine_rpm < m_engine_min_rpm && m_engine_min_rpm > declutchRPM)
        {
            float clutch = (m_cur_engine_rpm - declutchRPM) / (m_engine_min_rpm - declutchRPM);
            m_cur_clutch = std::min(clutch * clutch, m_cur_clutch);
        }
        else if (!m_shift_val && m_cur_engine_rpm > m_engine_min_rpm && m_cur_clutch < 1.0f)
        {
            float range = (m_engine_max_rpm - m_engine_min_rpm);
            float tAcc = std::max(0.2f, acc);
            if (abs(m_cur_gear) == 1)
            {
                range *= 0.8f * sqrt(tAcc);
            }
            else
            {
                range *= 0.4f * sqrt(tAcc);
            }
            float powerRatio = std::min((m_cur_engine_rpm - m_engine_min_rpm) / range, 1.0f);
            float enginePower = getEnginePower(m_cur_engine_rpm) * tAcc * powerRatio;

            float gearboxspinner = m_cur_engine_rpm / m_gear_ratios[m_cur_gear + 1];
            float clutchTorque = (gearboxspinner - m_cur_wheel_revolutions) * m_clutch_force;
            float reTorque = clutchTorque / m_gear_ratios[m_cur_gear + 1];

            float torqueDiff = std::abs(reTorque);
            float newRPM = std::abs(m_cur_wheel_revolutions * m_gear_ratios[m_cur_gear + 1]);
            if (getEnginePower(newRPM) >= getEnginePower(m_cur_engine_rpm))
            {
                torqueDiff = std::min(enginePower * 2.0f, torqueDiff);
            }
            else
            {
                torqueDiff = std::min(enginePower * 0.9f, torqueDiff);
            }
            float newClutch = torqueDiff * m_gear_ratios[m_cur_gear + 1] / ((gearboxspinner - m_cur_wheel_revolutions) * m_clutch_force);

            m_cur_clutch = std::max(m_cur_clutch, newClutch);
        }

        m_cur_clutch = std::max(0.0f, m_cur_clutch);
        m_cur_clutch = std::min(m_cur_clutch, 1.0f);
    }

    if (doUpdate && !m_shifting && !m_post_shifting)
    {
        // gear hack
        m_abs_velocity = m_actor->ar_nodes[0].Velocity.length();
        float velocity = m_abs_velocity;

        Vector3 hdir = m_actor->GetCameraDir();
        if (hdir != Vector3::ZERO)
        {
            velocity = hdir.dotProduct(m_actor->ar_nodes[0].Velocity);
        }
        m_rel_velocity = std::abs(velocity);

        if (m_actor->ar_wheels[0].wh_radius != 0)
        {
            m_ref_wheel_revolutions = velocity / m_actor->ar_wheels[0].wh_radius * RAD_PER_SEC_TO_RPM;
        }

        if (!m_engine_is_electric && m_auto_mode == AUTOMATIC && (m_autoselect == DRIVE || m_autoselect == TWO) && m_cur_gear > 0)
        {
            if ((m_cur_engine_rpm > m_engine_max_rpm - 100.0f && m_cur_gear > 1) || m_cur_wheel_revolutions * m_gear_ratios[m_cur_gear + 1] > m_engine_max_rpm - 100.0f)
            {
                if ((m_autoselect == DRIVE && m_cur_gear < m_num_gears && m_cur_clutch > 0.99f) || (m_autoselect == TWO && m_cur_gear < std::min(2, m_num_gears)))
                {
                    m_kickdown_delay_counter = 100;
                    shift(1);
                }
            }
            else if (m_cur_gear > 1 && m_ref_wheel_revolutions * m_gear_ratios[m_cur_gear] < m_engine_max_rpm && (m_cur_engine_rpm < m_engine_min_rpm || (m_cur_engine_rpm < m_engine_min_rpm + m_shift_behaviour * m_half_rpm_range / 2.0f &&
                getEnginePower(m_cur_wheel_revolutions * m_gear_ratios[m_cur_gear]) > getEnginePower(m_cur_wheel_revolutions * m_gear_ratios[m_cur_gear + 1]))))
            {
                shift(-1);
            }

            int newGear = m_cur_gear;

            float brake = 0.0f;

            if (m_actor->ar_brake_force > 0.0f)
            {
                brake = m_actor->ar_brake / m_actor->ar_brake_force;
            }

            m_rpms.push_front(m_cur_engine_rpm);
            m_accs.push_front(acc);
            m_brakes.push_front(brake);

            float avgRPM50 = 0.0f;
            float avgRPM200 = 0.0f;
            float avgAcc50 = 0.0f;
            float avgAcc200 = 0.0f;
            float avgBrake50 = 0.0f;
            float avgBrake200 = 0.0f;

            for (unsigned int i = 0; i < m_accs.size(); i++)
            {
                if (i < 50)
                {
                    avgRPM50 += m_rpms[i];
                    avgAcc50 += m_accs[i];
                    avgBrake50 += m_brakes[i];
                }

                avgRPM200 += m_rpms[i];
                avgAcc200 += m_accs[i];
                avgBrake200 += m_brakes[i];
            }

            avgRPM50 /= std::min(m_rpms.size(), (std::deque<float>::size_type)50);
            avgAcc50 /= std::min(m_accs.size(), (std::deque<float>::size_type)50);
            avgBrake50 /= std::min(m_brakes.size(), (std::deque<float>::size_type)50);

            avgRPM200 /= m_rpms.size();
            avgAcc200 /= m_accs.size();
            avgBrake200 /= m_brakes.size();

            if (avgAcc50 > 0.8f || avgAcc200 > 0.8f || avgBrake50 > 0.8f || avgBrake200 > 0.8f)
            {
                m_shift_behaviour = std::min(m_shift_behaviour + 0.01f, 1.0f);
            }
            else if (acc < 0.5f && avgAcc50 < 0.5f && avgAcc200 < 0.5f && brake < 0.5f && avgBrake50 < 0.5f && avgBrake200 < 0.5)
            {
                m_shift_behaviour /= 1.01;
            }

            if (avgAcc50 > 0.8f && m_cur_engine_rpm < m_engine_max_rpm - m_one_third_rpm_range)
            {
                while (newGear > 1 && m_cur_wheel_revolutions * m_gear_ratios[newGear] < m_engine_max_rpm - m_one_third_rpm_range &&
                    getEnginePower(m_cur_wheel_revolutions * m_gear_ratios[newGear]) * m_gear_ratios[newGear] >
                    getEnginePower(m_cur_wheel_revolutions * m_gear_ratios[newGear + 1]) * m_gear_ratios[newGear + 1])
                {
                    newGear--;
                }
            }
            else if (avgAcc50 > 0.6f && acc < 0.8f && acc > avgAcc50 + 0.1f && m_cur_engine_rpm < m_engine_min_rpm + m_half_rpm_range)
            {
                if (newGear > 1 && m_cur_wheel_revolutions * m_gear_ratios[newGear] < m_engine_min_rpm + m_half_rpm_range &&
                    getEnginePower(m_cur_wheel_revolutions * m_gear_ratios[newGear]) * m_gear_ratios[newGear] >
                    getEnginePower(m_cur_wheel_revolutions * m_gear_ratios[newGear + 1]) * m_gear_ratios[newGear + 1])
                {
                    newGear--;
                }
            }
            else if (avgAcc50 > 0.4f && acc < 0.8f && acc > avgAcc50 + 0.1f && m_cur_engine_rpm < m_engine_min_rpm + m_half_rpm_range)
            {
                if (newGear > 1 && m_cur_wheel_revolutions * m_gear_ratios[newGear] < m_engine_min_rpm + m_one_third_rpm_range &&
                    getEnginePower(m_cur_wheel_revolutions * m_gear_ratios[newGear]) * m_gear_ratios[newGear] >
                    getEnginePower(m_cur_wheel_revolutions * m_gear_ratios[newGear + 1]) * m_gear_ratios[newGear + 1])
                {
                    newGear--;
                }
            }
            else if (m_cur_gear < (m_autoselect == TWO ? std::min(2, m_num_gears) : m_num_gears) &&
                avgBrake200 < 0.2f && acc < std::min(avgAcc200 + 0.1f, 1.0f) && m_cur_engine_rpm > avgRPM200 - m_full_rpm_range / 20.0f)
            {
                if (avgAcc200 < 0.6f && avgAcc200 > 0.4f && m_cur_engine_rpm > m_engine_min_rpm + m_one_third_rpm_range && m_cur_engine_rpm < m_engine_max_rpm - m_one_third_rpm_range)
                {
                    if (m_cur_wheel_revolutions * m_gear_ratios[newGear + 2] > m_engine_min_rpm + m_one_third_rpm_range)
                    {
                        newGear++;
                    }
                }
                else if (avgAcc200 < 0.4f && avgAcc200 > 0.2f && m_cur_engine_rpm > m_engine_min_rpm + m_one_third_rpm_range)
                {
                    if (m_cur_wheel_revolutions * m_gear_ratios[newGear + 2] > m_engine_min_rpm + m_one_third_rpm_range / 2.0f)
                    {
                        newGear++;
                    }
                }
                else if (avgAcc200 < 0.2f && m_cur_engine_rpm > m_engine_min_rpm + m_one_third_rpm_range / 2.0f && m_cur_engine_rpm < m_engine_min_rpm + m_half_rpm_range)
                {
                    if (m_cur_wheel_revolutions * m_gear_ratios[newGear + 2] > m_engine_min_rpm + m_one_third_rpm_range / 2.0f)
                    {
                        newGear++;
                    }
                }

                if (newGear > m_cur_gear)
                {
                    m_upshift_delay_counter++;
                    if (m_upshift_delay_counter <= 100 * m_shift_behaviour)
                    {
                        newGear = m_cur_gear;
                    }
                }
                else
                {
                    m_upshift_delay_counter = 0;
                }
            }

            if (newGear < m_cur_gear && m_kickdown_delay_counter > 0)
            {
                newGear = m_cur_gear;
            }
            m_kickdown_delay_counter = std::max(0, m_kickdown_delay_counter - 1);

            if (newGear < m_cur_gear && std::abs(m_cur_wheel_revolutions * (m_gear_ratios[newGear + 1] - m_gear_ratios[m_cur_gear + 1])) > m_one_third_rpm_range / 6.0f ||
                newGear > m_cur_gear && std::abs(m_cur_wheel_revolutions * (m_gear_ratios[newGear + 1] - m_gear_ratios[m_cur_gear + 1])) > m_one_third_rpm_range / 3.0f)
            {
                if (m_abs_velocity - m_rel_velocity < 0.5f)
                    shiftTo(newGear);
            }

            if (m_accs.size() > 200)
            {
                m_rpms.pop_back();
                m_accs.pop_back();
                m_brakes.pop_back();
            }
            // avoid over-revving
            if (m_auto_mode <= SEMIAUTO && m_cur_gear != 0)
            {
                if (std::abs(m_cur_wheel_revolutions * m_gear_ratios[m_cur_gear + 1]) > m_engine_max_rpm * 1.25f)
                {
                    float clutch = 0.0f + 1.0f / (1.0f + std::abs(m_cur_wheel_revolutions * m_gear_ratios[m_cur_gear + 1] - m_engine_max_rpm * 1.25f) / 2.0f);
                    m_cur_clutch = std::min(clutch, m_cur_clutch);
                }
                if (m_cur_gear * m_cur_wheel_revolutions < -10.0f)
                {
                    float clutch = 0.0f + 1.0f / (1.0f + std::abs(-10.0f - m_cur_gear * m_cur_wheel_revolutions) / 2.0f);
                    m_cur_clutch = std::min(clutch, m_cur_clutch);
                }
            }
        }
    }

    // audio stuff
    this->UpdateEngineAudio(doUpdate);
}

void EngineSim::UpdateEngineAudio(int doUpdate)
{
#ifdef USE_OPENAL
    if (m_engine_has_turbo)
    {
        for (int i = 0; i < m_num_turbos; i++)
        {
            SOUND_MODULATE(m_actor, SS_MOD_TURBO, m_cur_turbo_rpm[i]);
        }
    }

    if (doUpdate)
    {
        SOUND_MODULATE(m_actor, SS_MOD_ENGINE, m_cur_engine_rpm);
        SOUND_MODULATE(m_actor, SS_MOD_TORQUE, m_cur_clutch_torque);
        SOUND_MODULATE(m_actor, SS_MOD_GEARBOX, m_cur_wheel_revolutions);
    }
    // reverse gear beep
    if (m_cur_gear == -1 && m_engine_is_running)
    {
        SOUND_START(m_actor, SS_TRIG_REVERSE_GEAR);
    }
    else
    {
        SOUND_STOP(m_actor, SS_TRIG_REVERSE_GEAR);
    }
#endif // USE_OPENAL
}

void EngineSim::ToggleAutoShiftMode()
{
    m_auto_mode = (m_auto_mode + 1) % (MANUAL_RANGES + 1);

    if (m_auto_mode == AUTOMATIC)
    {
        if (m_cur_gear > 0)
            m_autoselect = DRIVE;
        if (m_cur_gear < 0)
            m_autoselect = REAR;
        if (m_cur_gear == 0)
            m_autoselect = NEUTRAL;
    }
    else
    {
        m_autoselect = MANUALMODE;
    }

    if (m_auto_mode == MANUAL_RANGES)
    {
        m_cur_gear_range = 0;
    }
}

RoR::SimGearboxMode EngineSim::GetAutoShiftMode()
{
    return (RoR::SimGearboxMode)this->m_auto_mode;
}

void EngineSim::SetAutoMode(RoR::SimGearboxMode mode)
{
    this->m_auto_mode = (shiftmodes)mode;
}

void EngineSim::SetAcceleration(float val)
{
    m_cur_acc = val;
}

float EngineSim::GetTurboPsi()
{
    if (m_engine_turbo_mode == OLD)
    {
        return m_cur_turbo_rpm[0] / 10000.0f;
    }

    float turboPSI = 0;

    if (m_turbo_has_bov)
    {
        for (int i = 0; i < m_num_turbos; i++)
            turboPSI += m_turbo_bov_rpm[i] / 10000.0f;
    }
    else
    {
        for (int i = 0; i < m_num_turbos; i++)
            turboPSI += m_cur_turbo_rpm[i] / 10000.0f;
    }

    return turboPSI;
}

float EngineSim::GetAcceleration()
{
    return m_cur_acc;
}

// this is mainly for smoke...
void EngineSim::PushNetworkState(float rpm, float force, float clutch, int gear, bool _running, bool _contact, char _automode)
{
    m_cur_engine_rpm = rpm;
    m_cur_acc = force;
    m_cur_clutch = clutch;
    m_cur_gear = gear;
    m_engine_is_running = _running; //(fabs(rpm)>10.0);
    m_starter_has_contact = _contact;
    if (_automode != -1)
    {
        m_auto_mode = _automode;
    }
}

float EngineSim::GetSmoke()
{
    if (m_engine_is_running)
    {
        return m_cur_acc * (1.0f - m_cur_turbo_rpm[0] /* doesn't matter */ / m_max_turbo_rpm);// * m_engine_torque / 5000.0f;
    }

    return -1;
}

float EngineSim::GetTorque()
{
    if (m_cur_clutch_torque > 1000000.0)
        return 1000000.0;
    if (m_cur_clutch_torque < -1000000.0)
        return -1000000.0;
    return m_cur_clutch_torque;
}

void EngineSim::SetEngineRpm(float rpm)
{
    m_cur_engine_rpm = rpm;
}

void EngineSim::SetEnginePriming(bool p)
{
    m_engine_is_priming = p;
}

void EngineSim::SetHydroPumpWork(float work)
{
    m_hydropump_state = work;
}

void EngineSim::SetWheelSpin(float rpm)
{
    m_cur_wheel_revolutions = rpm;
}

// for hydros acceleration
float EngineSim::GetCrankFactor()
{
    float minWorkingRPM = m_idle_rpm * 1.1f; // minWorkingRPM > m_idle_rpm avoids commands deadlocking the engine

    float rpmRatio = (m_cur_engine_rpm - minWorkingRPM) / (m_engine_max_rpm - minWorkingRPM);
    rpmRatio = std::max(0.0f, rpmRatio); // Avoids a negative rpmRatio when m_cur_engine_rpm < minWorkingRPM
    rpmRatio = std::min(rpmRatio, 1.0f); // Avoids a rpmRatio > 1.0f when m_cur_engine_rpm > m_engine_max_rpm

    float crankfactor = 5.0f * rpmRatio;

    return crankfactor;
}

void EngineSim::SetClutch(float clutch)
{
    m_cur_clutch = clutch;
}

float EngineSim::GetClutch()
{
    return m_cur_clutch;
}

float EngineSim::GetClutchForce()
{
    return m_clutch_force;
}

void EngineSim::ToggleStarterContact()
{
    m_starter_has_contact = !m_starter_has_contact;
    if (m_starter_has_contact)
    {
        SOUND_START(m_actor, SS_TRIG_IGNITION);
    }
    else
    {
        SOUND_STOP(m_actor, SS_TRIG_IGNITION);
    }
}

void EngineSim::StartEngine()
{
    this->OffStart();
    m_starter_has_contact = true;
    m_cur_engine_rpm = m_idle_rpm;
    m_engine_is_running = true;
    if (m_auto_mode <= SEMIAUTO)
    {
        m_cur_gear = 1;
    }
    if (m_auto_mode == AUTOMATIC)
    {
        m_autoselect = DRIVE;
    }
    SOUND_START(m_actor, SS_TRIG_IGNITION);
    SOUND_START(m_actor, SS_TRIG_ENGINE);
}

void EngineSim::OffStart()
{
    m_air_pressure = 0.0f;
    m_autoselect = MANUALMODE;
    m_starter_has_contact = false;
    m_cur_acc = 0.0f;
    m_cur_clutch = 0.0f;
    m_cur_clutch_torque = 0.0f;
    m_cur_engine_rpm = 0.0f;
    m_cur_gear = 0;
    m_post_shifting = 0;
    m_engine_is_running = false;
    m_shifting = 0;
    m_shift_val = 0;
    if (m_auto_mode == AUTOMATIC)
    {
        m_autoselect = NEUTRAL;
    }
    for (int i = 0; i < m_num_turbos; i++)
    {
        m_cur_turbo_rpm[i] = 0.0f;
        m_turbo_bov_rpm[i] = 0.0f;
    }
}

void EngineSim::SetStarter(bool v)
{
    m_starter = static_cast<int>(v);
}

int EngineSim::GetGear()
{
    return m_cur_gear;
}

// low level gear changing
void EngineSim::SetGear(int v)
{
    m_cur_gear = v;
}

int EngineSim::GetGearRange()
{
    return m_cur_gear_range;
}

void EngineSim::SetGearRange(int v)
{
    m_cur_gear_range = v;
}

void EngineSim::StopEngine()
{
    if (!m_engine_is_running)
        return;

    m_engine_is_running = false;
    TRIGGER_EVENT(SE_TRUCK_ENGINE_DIED, m_actor->ar_instance_id);
    SOUND_STOP(m_actor, SS_TRIG_ENGINE);
}

// high level controls
void EngineSim::autoSetAcc(float val)
{
    m_auto_cur_acc = val;
    if (!m_shifting)
    {
        SetAcceleration(val);
    }
}

void EngineSim::shift(int val)
{
    if (!val || m_cur_gear + val < -1 || m_cur_gear + val > getNumGears())
        return;
    if (m_auto_mode < MANUAL)
    {
        m_shift_val = val;
        m_shifting = 1;
        m_shift_clock = 0.0f;
    }
    else
    {
        if (m_cur_clutch > 0.25f)
        {
            SOUND_PLAY_ONCE(m_actor, SS_TRIG_GEARSLIDE);
        }
        else
        {
            SOUND_PLAY_ONCE(m_actor, SS_TRIG_SHIFT);
            m_cur_gear += val;
        }
    }
}

void EngineSim::shiftTo(int newGear)
{
    shift(newGear - m_cur_gear);
}

void EngineSim::updateShifts()
{
    if (m_engine_is_electric)
        return;
    if (m_autoselect == MANUALMODE)
        return;

    SOUND_PLAY_ONCE(m_actor, SS_TRIG_SHIFT);

    if (m_autoselect == REAR)
    {
        m_cur_gear = -1;
    }
    else if (m_autoselect == NEUTRAL)
    {
        m_cur_gear = 0;
    }
    else if (m_autoselect == ONE)
    {
        m_cur_gear = 1;
    }
    else
    {
        // search for an appropriate gear
        int newGear = 1;

        while (newGear < m_num_gears && m_cur_wheel_revolutions > 0.0f && m_cur_wheel_revolutions * m_gear_ratios[newGear + 1] > m_engine_max_rpm - 100.0f)
        {
            newGear++;
        }

        m_cur_gear = newGear;

        if (m_autoselect == TWO)
        {
            m_cur_gear = std::min(m_cur_gear, 2);
        }
    }
}

void EngineSim::autoShiftSet(int mode)
{
    m_autoselect = (autoswitch)mode;
    updateShifts();
}

void EngineSim::autoShiftUp()
{
    if (m_autoselect != REAR)
    {
        m_autoselect = (autoswitch)(m_autoselect - 1);
        updateShifts();
    }
}

void EngineSim::autoShiftDown()
{
    if (m_autoselect != ONE)
    {
        m_autoselect = (autoswitch)(m_autoselect + 1);
        updateShifts();
    }
}

int EngineSim::getAutoShift()
{
    return (int)m_autoselect;
}

void EngineSim::setManualClutch(float val)
{
    if (m_auto_mode >= MANUAL)
    {
        val = std::max(0.0f, val);
        m_cur_clutch = 1.0 - val;
    }
}

float EngineSim::getTurboPower()
{
    if (!m_engine_has_turbo)
        return 0.0f;
    if (m_engine_turbo_mode != NEW)
        return 0.0f;

    float atValue = 0.0f; // torque (turbo integreation)

    if (m_turbo_ver == 1)
    {
        for (int i = 0; i < m_num_turbos; i++)
        {
            atValue += m_engine_addi_torque[i] * (m_cur_turbo_rpm[i] / m_max_turbo_rpm);
        }
    }
    else
    {
        atValue = (((GetTurboPsi() * 6.8) * m_engine_torque) / 100); //1psi = 6% more power
    }

    return atValue;
}

float EngineSim::getEnginePower(float rpm)
{
    // engine power with limiter
    float tqValue = 1.0f; // ratio (0-1)

    float rpmRatio = rpm / (m_engine_max_rpm * 1.25f);

    rpmRatio = std::max(0.0f, rpmRatio);
    rpmRatio = std::min(rpmRatio, 1.0f);

    if (m_torque_curve)
    {
        tqValue = m_torque_curve->getEngineTorque(rpmRatio);
    }

    return (m_engine_torque * tqValue) + getTurboPower();
}

float EngineSim::getAccToHoldRPM(float rpm)
{
    float rpmRatio = rpm / (m_engine_max_rpm * 1.25f);

    rpmRatio = std::min(rpmRatio, 1.0f);

    return (-m_braking_torque * rpmRatio) / getEnginePower(m_cur_engine_rpm);
}

float EngineSim::getIdleMixture()
{
    if (m_cur_engine_rpm < m_idle_rpm)
    {
        // determine the fuel injection needed to counter the engine braking force
        float idleMix = getAccToHoldRPM(m_cur_engine_rpm);

        idleMix = std::max(0.06f, idleMix);

        idleMix = idleMix * (1.0f + (m_idle_rpm - m_cur_engine_rpm) / 100.0f);

        idleMix = std::max(m_min_idle_mixture, idleMix);
        idleMix = std::min(idleMix, m_max_idle_mixture);

        return idleMix;
    }

    return 0.0f;
}

float EngineSim::getPrimeMixture()
{
    if (m_engine_is_priming)
    {
        float crankfactor = GetCrankFactor();

        if (crankfactor < 0.9f)
        {
            // crankfactor is between 0.0f and 0.9f
            return 1.0f;
        }
        else if (crankfactor < 1.0f)
        {
            // crankfactor is between 0.9f and 1.0f
            return 10.0f * (1.0f - crankfactor);
        }
    }

    return 0.0f;
}
