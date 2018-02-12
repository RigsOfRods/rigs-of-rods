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

#include "BeamEngine.h"

#include "BeamFactory.h"
#include "Scripting.h"
#include "SoundScriptManager.h"
#include "TorqueCurve.h"

using namespace Ogre;

EngineSim::EngineSim(float minRPM, float maxRPM, float torque, std::vector<float> gears, float dratio, Actor* actor) :
    apressure(0.0f)
    , autocurAcc(0.0f)
    , automode(AUTOMATIC)
    , autoselect(DRIVE)
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
    , engineTorque(torque - m_braking_torque)
    , m_gear_ratios(gears)
    , m_engine_has_air(true)
    , m_engine_has_turbo(true)
    , hydropump(0.0f)
    , idleRPM(std::min(minRPM, 800.0f))
    , inertia(10.0f)
    , kickdownDelayCounter(0)
    , maxIdleMixture(0.2f)
    , maxRPM(std::abs(maxRPM))
    , minIdleMixture(0.0f)
    , minRPM(std::abs(minRPM))
    , m_num_gears((int)gears.size() - 2)
    , post_shift_time(0.2f)
    , postshiftclock(0.0f)
    , postshifting(0)
    , is_priming(false)
    , m_engine_is_running(false)
    , shiftBehaviour(0.0f)
    , shift_time(0.5f)
    , shiftclock(0.0f)
    , shifting(0)
    , shiftval(0)
    , stallRPM(300.0f)
    , starter(0)
    , torqueCurve(new TorqueCurve())
    , m_actor(actor)
    , m_engine_turbo_mode(OLD)
    , m_engine_type('t')
    , upShiftDelayCounter(0)
    , m_engine_is_electric(false)
    , turboInertiaFactor(1)
    , numTurbos(1)
    , maxTurboRPM(200000.0f)
    , turboEngineRpmOperation(0.0f)
    , turboVer(1)
    , minBOVPsi(11)
    , minWGPsi(20)
    , b_WasteGate(false)
    , b_BOV(false)
    , b_flutter(false)
    , wastegate_threshold_p(0)
    , wastegate_threshold_n(0)
    , b_anti_lag(false)
    , rnd_antilag_chance(0.9975)
    , minRPM_antilag(3000)
    , antilag_power_factor(170)
{
    fullRPMRange = (maxRPM - minRPM);
    oneThirdRPMRange = fullRPMRange / 3.0f;
    halfRPMRange = fullRPMRange / 2.0f;

    m_gear_ratios[0] = -m_gear_ratios[0];
    for (std::vector<float>::iterator it = m_gear_ratios.begin(); it != m_gear_ratios.end(); ++it)
    {
        (*it) *= m_diff_ratio;
    }

    for (int i = 0; i < MAXTURBO; i++)
    {
        EngineAddiTorque[i] = 0;
        curTurboRPM[i] = 0;
    }
}

EngineSim::~EngineSim()
{
    // delete NULL is safe
    delete torqueCurve;
    torqueCurve = NULL;
}

void EngineSim::setTurboOptions(int type, float tinertiaFactor, int nturbos, float param1, float param2, float param3, float param4, float param5, float param6, float param7, float param8, float param9, float param10, float param11)
{
    m_engine_has_turbo = true;
    m_engine_turbo_mode = NEW;

    if (nturbos > MAXTURBO)
    {
        numTurbos = 4;
        LOG("Turbo: No more than 4 turbos allowed"); //TODO: move this under RigParser
    }
    else
        numTurbos = nturbos;

    turboVer = type;
    turboInertiaFactor = tinertiaFactor;

    if (param2 != 9999)
        turboEngineRpmOperation = param2;

    if (turboVer == 1)
    {
        for (int i = 0; i < numTurbos; i++)
            EngineAddiTorque[i] = param1 / numTurbos; //Additional torque
    }
    else
    {
        turboMaxPSI = param1; //maxPSI
        maxTurboRPM = turboMaxPSI * 10000;

        //Duh
        if (param3 == 1)
            b_BOV = true;
        else
            b_BOV = false;

        if (param3 != 9999)
            minBOVPsi = param4;

        if (param5 == 1)
            b_WasteGate = true;
        else
            b_WasteGate = false;

        if (param6 != 9999)
            minWGPsi = param6 * 10000;

        if (param7 != 9999)
        {
            wastegate_threshold_n = 1 - param7;
            wastegate_threshold_p = 1 + param7;
        }

        if (param8 == 1)
            b_anti_lag = true;
        else
            b_anti_lag = false;

        if (param9 != 9999)
            rnd_antilag_chance = param9;

        if (param10 != 9999)
            minRPM_antilag = param10;

        if (param11 != 9999)
            antilag_power_factor = param11;
    }
}

void EngineSim::setOptions(float einertia, char etype, float eclutch, float ctime, float stime, float pstime, float irpm, float srpm, float maximix, float minimix)
{
    inertia = einertia;
    m_engine_type = etype;
    m_clutch_force = eclutch;

    if (ctime > 0)
        m_clutch_time = ctime;
    if (stime > 0)
        shift_time = stime;
    if (pstime > 0)
        post_shift_time = pstime;
    if (irpm > 0)
        idleRPM = irpm;
    if (srpm > 0)
        stallRPM = srpm;
    if (maximix > 0)
        maxIdleMixture = maximix;
    if (minimix > 0)
        minIdleMixture = minimix;

    m_clutch_time = std::max(0.0f, m_clutch_time);
    shift_time = std::max(0.0f, shift_time);
    post_shift_time = std::max(0.0f, post_shift_time);

    m_clutch_time = std::min(m_clutch_time, shift_time * 0.9f);

    idleRPM = std::max(0.0f, idleRPM);
    stallRPM = std::max(0.0f, stallRPM);
    stallRPM = std::min(idleRPM, stallRPM);

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

void EngineSim::update(float dt, int doUpdate)
{
    int actor_id = m_actor->ar_instance_id;
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
        apressure += dt * m_cur_engine_rpm;
        if (apressure > 50000.0f)
        {
            SOUND_PLAY_ONCE(m_actor, SS_TRIG_AIR_PURGE);
            apressure = 0.0f;
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
            turbotorque -= curTurboRPM[0] / 200000.0f;

            // powering (exhaust) with limiter
            if (curTurboRPM[0] < 200000.0f && m_engine_is_running && m_cur_acc > 0.06f)
            {
                turbotorque += 1.5f * m_cur_acc * (m_cur_engine_rpm / maxRPM);
            }
            else
            {
                turbotorque += 0.1f * (m_cur_engine_rpm / maxRPM);
            }

            // integration
            curTurboRPM[0] += dt * turbotorque / turboInertia;
        }
        else
        {
            for (int i = 0; i < numTurbos; i++)
            {
                // update turbo speed (lag)
                // reset each of the values for each turbo
                float turbotorque = 0.0f;
                float turboBOVtorque = 0.0f;

                float turboInertia = 0.000003f * turboInertiaFactor;

                // braking (compression)
                turbotorque -= curTurboRPM[i] / maxTurboRPM;
                turboBOVtorque -= curBOVTurboRPM[i] / maxTurboRPM;

                // powering (exhaust) with limiter
                if (m_cur_engine_rpm >= turboEngineRpmOperation)
                {
                    if (curTurboRPM[i] <= maxTurboRPM && m_engine_is_running && acc > 0.06f)
                    {
                        if (b_WasteGate)
                        {
                            if (curTurboRPM[i] < minWGPsi * wastegate_threshold_p && !b_flutter)
                            {
                                turbotorque += 1.5f * acc * (((m_cur_engine_rpm - turboEngineRpmOperation) / (maxRPM - turboEngineRpmOperation)));
                            }
                            else
                            {
                                b_flutter = true;
                                turbotorque -= (curTurboRPM[i] / maxTurboRPM) * 1.5;
                            }

                            if (b_flutter)
                            {
                                SOUND_START(m_actor, SS_TRIG_TURBOWASTEGATE);
                                if (curTurboRPM[i] < minWGPsi * wastegate_threshold_n)
                                {
                                    b_flutter = false;
                                    SOUND_STOP(m_actor, SS_TRIG_TURBOWASTEGATE);
                                }
                            }
                        }
                        else
                            turbotorque += 1.5f * acc * (((m_cur_engine_rpm - turboEngineRpmOperation) / (maxRPM - turboEngineRpmOperation)));
                    }
                    else
                    {
                        turbotorque += 0.1f * (((m_cur_engine_rpm - turboEngineRpmOperation) / (maxRPM - turboEngineRpmOperation)));
                    }

                    //Update waste gate, it's like a BOV on the exhaust part of the turbo, acts as a limiter
                    if (b_WasteGate)
                    {
                        if (curTurboRPM[i] > minWGPsi * 0.95)
                            turboInertia = turboInertia * 0.7; //Kill inertia so it flutters
                        else
                            turboInertia = turboInertia * 1.3; //back to normal inertia
                    }
                }

                //simulate compressor surge
                if (!b_BOV)
                {
                    if (curTurboRPM[i] > 13 * 10000 && m_cur_acc < 0.06f)
                    {
                        turbotorque += (turbotorque * 2.5);
                    }
                }

                // anti lag
                if (b_anti_lag && m_cur_acc < 0.5)
                {
                    float f = frand();
                    if (m_cur_engine_rpm > minRPM_antilag && f > rnd_antilag_chance)
                    {
                        if (curTurboRPM[i] > maxTurboRPM * 0.35 && curTurboRPM[i] < maxTurboRPM)
                        {
                            turbotorque -= (turbotorque * (f * antilag_power_factor));
                            SOUND_START(m_actor, SS_TRIG_TURBOBACKFIRE);
                        }
                    }
                    else
                    {
                        SOUND_STOP(m_actor, SS_TRIG_TURBOBACKFIRE);
                    }
                }

                // update main turbo rpm
                curTurboRPM[i] += dt * turbotorque / turboInertia;

                //Update BOV
                //It's basicly an other turbo which is limmited to the main one's rpm, but it doesn't affect its rpm.  It only affects the power going into the engine.
                //This one is used to simulate the pressure between the engine and the compressor.
                //I should make the whole turbo code work this way. -Max98
                if (b_BOV)
                {
                    if (curBOVTurboRPM[i] < curTurboRPM[i])
                        turboBOVtorque += 1.5f * acc * (((m_cur_engine_rpm) / (maxRPM)));
                    else
                        turboBOVtorque += 0.07f * (((m_cur_engine_rpm) / (maxRPM)));

                    if (m_cur_acc < 0.06 && curTurboRPM[i] > minBOVPsi * 10000)
                    {
                        SOUND_START(m_actor, SS_TRIG_TURBOBOV);
                        curBOVTurboRPM[i] += dt * turboBOVtorque / (turboInertia * 0.1);
                    }
                    else
                    {
                        SOUND_STOP(m_actor, SS_TRIG_TURBOBOV);
                        if (curBOVTurboRPM[i] < curTurboRPM[i])
                            curBOVTurboRPM[i] += dt * turboBOVtorque / (turboInertia * 0.05);
                        else
                            curBOVTurboRPM[i] += dt * turboBOVtorque / turboInertia;
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
        totaltorque += m_braking_torque * m_cur_engine_rpm / maxRPM;
    }
    else
    {
        totaltorque += m_braking_torque;
    }

    // braking by hydropump
    if (m_cur_engine_rpm > 100.0f)
    {
        totaltorque -= 8.0f * hydropump / (m_cur_engine_rpm * 0.105f * dt);
    }

    if (m_engine_is_running && m_starter_has_contact && m_cur_engine_rpm < (maxRPM * 1.25f))
    {
        totaltorque += getEnginePower(m_cur_engine_rpm) * acc;
    }

    if (!m_engine_is_electric)
    {
        if (m_engine_is_running && m_cur_engine_rpm < stallRPM)
        {
            stop();
        }

        if (m_starter_has_contact && starter && !m_engine_is_running)
        {
            if (m_cur_engine_rpm < stallRPM)
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
    m_cur_engine_rpm += dt * totaltorque / inertia;

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

    if (automode < MANUAL)
    {
        // auto-shift
        if (shifting)
        {
            shiftclock += dt;

            if (shiftval)
            {
                float declutchTime = std::min(shift_time - m_clutch_time, m_clutch_time);
                if (shiftclock <= declutchTime)
                {
                    // depress the clutch 
                    float ratio = pow(1.0f - (shiftclock / declutchTime), 2);
                    m_cur_clutch = std::min(ratio, m_cur_clutch);
                    m_cur_acc = std::min(ratio, autocurAcc);
                }
                else
                {
                    // engage a gear
                    SOUND_START(m_actor, SS_TRIG_SHIFT);
                    if (autoselect != NEUTRAL)
                    {
                        m_cur_gear += shiftval;
                        m_cur_gear = std::max(-1, m_cur_gear);
                        m_cur_gear = std::min(m_cur_gear, m_num_gears);
                    }
                    shiftval = 0;
                }
            }

            if (shiftclock > shift_time)
            {
                // we're done shifting
                SOUND_STOP(m_actor, SS_TRIG_SHIFT);
                setAcc(autocurAcc);
                shifting = 0;
                postshifting = 1;
                postshiftclock = 0.0f;
            }
            else if (!shiftval && m_cur_gear && shiftclock >= (shift_time - m_clutch_time))
            {
                // release the clutch <-- Done below
                float timer = shiftclock - (shift_time - m_clutch_time);
                float ratio = sqrt(timer / m_clutch_time);
                m_cur_acc = (autocurAcc / 2.0f) * ratio;
            }
        }

        if (postshifting)
        {
            postshiftclock += dt;
            if (postshiftclock > post_shift_time)
            {
                postshifting = 0;
            }
            else if (autocurAcc > 0.0f)
            {
                float ratio = postshiftclock / post_shift_time;
                m_cur_acc = (autocurAcc / 2.0f) + (autocurAcc / 2.0f) * ratio;
            }
            else if (m_cur_gear)
            {
                float gearboxspinner = m_cur_engine_rpm / m_gear_ratios[m_cur_gear + 1];
                if (m_cur_wheel_revolutions > gearboxspinner)
                {
                    float ratio = sqrt(postshiftclock / post_shift_time);
                    m_cur_clutch = std::max(m_cur_clutch, ratio);
                }
            }
        }

        // auto clutch
        float declutchRPM = (minRPM + stallRPM) / 2.0f;
        if (m_cur_gear == 0 || m_cur_engine_rpm < declutchRPM || (fabs(m_cur_wheel_revolutions) < 1.0f && (m_cur_engine_rpm < minRPM * 1.01f || autocurAcc == 0.0f)) || (autocurAcc == 0.0f && m_actor->ar_brake > 0.0f && retorque >= 0.0f))
        {
            m_cur_clutch = 0.0f;
        }
        else if (m_cur_engine_rpm < minRPM && minRPM > declutchRPM)
        {
            float clutch = (m_cur_engine_rpm - declutchRPM) / (minRPM - declutchRPM);
            m_cur_clutch = std::min(clutch * clutch, m_cur_clutch);
        }
        else if (!shiftval && m_cur_engine_rpm > minRPM && m_cur_clutch < 1.0f)
        {
            float range = (maxRPM - minRPM);
            float tAcc = std::max(0.2f, acc);
            if (abs(m_cur_gear) == 1)
            {
                range *= 0.8f * sqrt(tAcc);
            }
            else
            {
                range *= 0.4f * sqrt(tAcc);
            }
            float powerRatio = std::min((m_cur_engine_rpm - minRPM) / range, 1.0f);
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

    if (doUpdate && !shifting && !postshifting)
    {
        // gear hack
        m_abs_velocity = m_actor->ar_nodes[0].Velocity.length();
        float velocity = m_abs_velocity;

        if (m_actor->ar_camera_node_pos[0] >= 0 && m_actor->ar_camera_node_dir[0] >= 0)
        {
            Vector3 hdir = (m_actor->ar_nodes[m_actor->ar_camera_node_pos[0]].RelPosition - m_actor->ar_nodes[m_actor->ar_camera_node_dir[0]].RelPosition).normalisedCopy();
            velocity = hdir.dotProduct(m_actor->ar_nodes[0].Velocity);
        }
        m_rel_velocity = std::abs(velocity);

        if (m_actor->ar_wheels[0].wh_radius != 0)
        {
            m_ref_wheel_revolutions = velocity / m_actor->ar_wheels[0].wh_radius * RAD_PER_SEC_TO_RPM;
        }

        if (!m_engine_is_electric && automode == AUTOMATIC && (autoselect == DRIVE || autoselect == TWO) && m_cur_gear > 0)
        {
            if ((m_cur_engine_rpm > maxRPM - 100.0f && m_cur_gear > 1) || m_cur_wheel_revolutions * m_gear_ratios[m_cur_gear + 1] > maxRPM - 100.0f)
            {
                if ((autoselect == DRIVE && m_cur_gear < m_num_gears && m_cur_clutch > 0.99f) || (autoselect == TWO && m_cur_gear < std::min(2, m_num_gears)))
                {
                    kickdownDelayCounter = 100;
                    shift(1);
                }
            }
            else if (m_cur_gear > 1 && m_ref_wheel_revolutions * m_gear_ratios[m_cur_gear] < maxRPM && (m_cur_engine_rpm < minRPM || (m_cur_engine_rpm < minRPM + shiftBehaviour * halfRPMRange / 2.0f &&
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

            rpms.push_front(m_cur_engine_rpm);
            accs.push_front(acc);
            brakes.push_front(brake);

            float avgRPM50 = 0.0f;
            float avgRPM200 = 0.0f;
            float avgAcc50 = 0.0f;
            float avgAcc200 = 0.0f;
            float avgBrake50 = 0.0f;
            float avgBrake200 = 0.0f;

            for (unsigned int i = 0; i < accs.size(); i++)
            {
                if (i < 50)
                {
                    avgRPM50 += rpms[i];
                    avgAcc50 += accs[i];
                    avgBrake50 += brakes[i];
                }

                avgRPM200 += rpms[i];
                avgAcc200 += accs[i];
                avgBrake200 += brakes[i];
            }

            avgRPM50 /= std::min(rpms.size(), (std::deque<float>::size_type)50);
            avgAcc50 /= std::min(accs.size(), (std::deque<float>::size_type)50);
            avgBrake50 /= std::min(brakes.size(), (std::deque<float>::size_type)50);

            avgRPM200 /= rpms.size();
            avgAcc200 /= accs.size();
            avgBrake200 /= brakes.size();

            if (avgAcc50 > 0.8f || avgAcc200 > 0.8f || avgBrake50 > 0.8f || avgBrake200 > 0.8f)
            {
                shiftBehaviour = std::min(shiftBehaviour + 0.01f, 1.0f);
            }
            else if (acc < 0.5f && avgAcc50 < 0.5f && avgAcc200 < 0.5f && brake < 0.5f && avgBrake50 < 0.5f && avgBrake200 < 0.5)
            {
                shiftBehaviour /= 1.01;
            }

            if (avgAcc50 > 0.8f && m_cur_engine_rpm < maxRPM - oneThirdRPMRange)
            {
                while (newGear > 1 && m_cur_wheel_revolutions * m_gear_ratios[newGear] < maxRPM - oneThirdRPMRange &&
                    getEnginePower(m_cur_wheel_revolutions * m_gear_ratios[newGear]) * m_gear_ratios[newGear] >
                    getEnginePower(m_cur_wheel_revolutions * m_gear_ratios[newGear + 1]) * m_gear_ratios[newGear + 1])
                {
                    newGear--;
                }
            }
            else if (avgAcc50 > 0.6f && acc < 0.8f && acc > avgAcc50 + 0.1f && m_cur_engine_rpm < minRPM + halfRPMRange)
            {
                if (newGear > 1 && m_cur_wheel_revolutions * m_gear_ratios[newGear] < minRPM + halfRPMRange &&
                    getEnginePower(m_cur_wheel_revolutions * m_gear_ratios[newGear]) * m_gear_ratios[newGear] >
                    getEnginePower(m_cur_wheel_revolutions * m_gear_ratios[newGear + 1]) * m_gear_ratios[newGear + 1])
                {
                    newGear--;
                }
            }
            else if (avgAcc50 > 0.4f && acc < 0.8f && acc > avgAcc50 + 0.1f && m_cur_engine_rpm < minRPM + halfRPMRange)
            {
                if (newGear > 1 && m_cur_wheel_revolutions * m_gear_ratios[newGear] < minRPM + oneThirdRPMRange &&
                    getEnginePower(m_cur_wheel_revolutions * m_gear_ratios[newGear]) * m_gear_ratios[newGear] >
                    getEnginePower(m_cur_wheel_revolutions * m_gear_ratios[newGear + 1]) * m_gear_ratios[newGear + 1])
                {
                    newGear--;
                }
            }
            else if (m_cur_gear < (autoselect == TWO ? std::min(2, m_num_gears) : m_num_gears) &&
                avgBrake200 < 0.2f && acc < std::min(avgAcc200 + 0.1f, 1.0f) && m_cur_engine_rpm > avgRPM200 - fullRPMRange / 20.0f)
            {
                if (avgAcc200 < 0.6f && avgAcc200 > 0.4f && m_cur_engine_rpm > minRPM + oneThirdRPMRange && m_cur_engine_rpm < maxRPM - oneThirdRPMRange)
                {
                    if (m_cur_wheel_revolutions * m_gear_ratios[newGear + 2] > minRPM + oneThirdRPMRange)
                    {
                        newGear++;
                    }
                }
                else if (avgAcc200 < 0.4f && avgAcc200 > 0.2f && m_cur_engine_rpm > minRPM + oneThirdRPMRange)
                {
                    if (m_cur_wheel_revolutions * m_gear_ratios[newGear + 2] > minRPM + oneThirdRPMRange / 2.0f)
                    {
                        newGear++;
                    }
                }
                else if (avgAcc200 < 0.2f && m_cur_engine_rpm > minRPM + oneThirdRPMRange / 2.0f && m_cur_engine_rpm < minRPM + halfRPMRange)
                {
                    if (m_cur_wheel_revolutions * m_gear_ratios[newGear + 2] > minRPM + oneThirdRPMRange / 2.0f)
                    {
                        newGear++;
                    }
                }

                if (newGear > m_cur_gear)
                {
                    upShiftDelayCounter++;
                    if (upShiftDelayCounter <= 100 * shiftBehaviour)
                    {
                        newGear = m_cur_gear;
                    }
                }
                else
                {
                    upShiftDelayCounter = 0;
                }
            }

            if (newGear < m_cur_gear && kickdownDelayCounter > 0)
            {
                newGear = m_cur_gear;
            }
            kickdownDelayCounter = std::max(0, kickdownDelayCounter - 1);

            if (newGear < m_cur_gear && std::abs(m_cur_wheel_revolutions * (m_gear_ratios[newGear + 1] - m_gear_ratios[m_cur_gear + 1])) > oneThirdRPMRange / 6.0f ||
                newGear > m_cur_gear && std::abs(m_cur_wheel_revolutions * (m_gear_ratios[newGear + 1] - m_gear_ratios[m_cur_gear + 1])) > oneThirdRPMRange / 3.0f)
            {
                if (m_abs_velocity - m_rel_velocity < 0.5f)
                    shiftTo(newGear);
            }

            if (accs.size() > 200)
            {
                rpms.pop_back();
                accs.pop_back();
                brakes.pop_back();
            }
            // avoid over-revving
            if (automode <= SEMIAUTO && m_cur_gear != 0)
            {
                if (std::abs(m_cur_wheel_revolutions * m_gear_ratios[m_cur_gear + 1]) > maxRPM * 1.25f)
                {
                    float clutch = 0.0f + 1.0f / (1.0f + std::abs(m_cur_wheel_revolutions * m_gear_ratios[m_cur_gear + 1] - maxRPM * 1.25f) / 2.0f);
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
    updateAudio(doUpdate);
}

void EngineSim::updateAudio(int doUpdate)
{
#ifdef USE_OPENAL
    if (m_engine_has_turbo)
    {
        for (int i = 0; i < numTurbos; i++)
             SOUND_MODULATE(m_actor, SS_MOD_TURBO, curTurboRPM[i]);
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

float EngineSim::getRPM()
{
    return m_cur_engine_rpm;
}

void EngineSim::toggleAutoMode()
{
    automode = (automode + 1) % (MANUAL_RANGES + 1);

    if (automode == AUTOMATIC)
    {
        if (m_cur_gear > 0)
            autoselect = DRIVE;
        if (m_cur_gear < 0)
            autoselect = REAR;
        if (m_cur_gear == 0)
            autoselect = NEUTRAL;
    }
    else
    {
        autoselect = MANUALMODE;
    }

    if (automode == MANUAL_RANGES)
    {
        m_cur_gear_range = 0;
    }
}

RoR::SimGearboxMode EngineSim::getAutoMode()
{
    return (RoR::SimGearboxMode)this->automode;
}

void EngineSim::setAutoMode(RoR::SimGearboxMode mode)
{
    this->automode = (shiftmodes)mode;
}

void EngineSim::setAcc(float val)
{
    m_cur_acc = val;
}

float EngineSim::getTurboPSI()
{
    if (m_engine_turbo_mode == OLD)
    {
        return curTurboRPM[0] / 10000.0f;
    }

    float turboPSI = 0;

    if (b_BOV)
    {
        for (int i = 0; i < numTurbos; i++)
            turboPSI += curBOVTurboRPM[i] / 10000.0f;
    }
    else
    {
        for (int i = 0; i < numTurbos; i++)
            turboPSI += curTurboRPM[i] / 10000.0f;
    }

    return turboPSI;
}

float EngineSim::getAcc()
{
    return m_cur_acc;
}

// this is mainly for smoke...
void EngineSim::netForceSettings(float rpm, float force, float clutch, int gear, bool _running, bool _contact, char _automode)
{
    m_cur_engine_rpm = rpm;
    m_cur_acc = force;
    m_cur_clutch = clutch;
    m_cur_gear = gear;
    m_engine_is_running = _running; //(fabs(rpm)>10.0);
    m_starter_has_contact = _contact;
    if (_automode != -1)
    {
        automode = _automode;
    }
}

float EngineSim::getSmoke()
{
    if (m_engine_is_running)
    {
        return m_cur_acc * (1.0f - curTurboRPM[0] /* doesn't matter */ / maxTurboRPM);// * engineTorque / 5000.0f;
    }

    return -1;
}

float EngineSim::getTorque()
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
    is_priming = p;
}

void EngineSim::setHydroPumpWork(float work)
{
    hydropump = work;
}

void EngineSim::setSpin(float rpm)
{
    m_cur_wheel_revolutions = rpm;
}

// for hydros acceleration
float EngineSim::getCrankFactor()
{
    float minWorkingRPM = idleRPM * 1.1f; // minWorkingRPM > idleRPM avoids commands deadlocking the engine

    float rpmRatio = (m_cur_engine_rpm - minWorkingRPM) / (maxRPM - minWorkingRPM);
    rpmRatio = std::max(0.0f, rpmRatio); // Avoids a negative rpmRatio when m_cur_engine_rpm < minWorkingRPM
    rpmRatio = std::min(rpmRatio, 1.0f); // Avoids a rpmRatio > 1.0f when m_cur_engine_rpm > maxRPM

    float crankfactor = 5.0f * rpmRatio;

    return crankfactor;
}

void EngineSim::setClutch(float clutch)
{
    m_cur_clutch = clutch;
}

float EngineSim::getClutch()
{
    return m_cur_clutch;
}

float EngineSim::getClutchForce()
{
    return m_clutch_force;
}

void EngineSim::toggleContact()
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

void EngineSim::start()
{
    offstart();
    m_starter_has_contact = true;
    m_cur_engine_rpm = idleRPM;
    m_engine_is_running = true;
    if (automode <= SEMIAUTO)
    {
        m_cur_gear = 1;
    }
    if (automode == AUTOMATIC)
    {
        autoselect = DRIVE;
    }
    SOUND_START(m_actor, SS_TRIG_IGNITION);
    SOUND_START(m_actor, SS_TRIG_ENGINE);
}

void EngineSim::offstart()
{
    apressure = 0.0f;
    autoselect = MANUALMODE;
    m_starter_has_contact = false;
    m_cur_acc = 0.0f;
    m_cur_clutch = 0.0f;
    m_cur_clutch_torque = 0.0f;
    m_cur_engine_rpm = 0.0f;
    m_cur_gear = 0;
    postshifting = 0;
    m_engine_is_running = false;
    shifting = 0;
    shiftval = 0;
    if (automode == AUTOMATIC)
    {
        autoselect = NEUTRAL;
    }
    for (int i = 0; i < numTurbos; i++)
    {
        curTurboRPM[i] = 0.0f;
        curBOVTurboRPM[i] = 0.0f;
    }
}

void EngineSim::setstarter(bool v)
{
    starter = static_cast<int>(v);
}

int EngineSim::getGear()
{
    return m_cur_gear;
}

// low level gear changing
void EngineSim::setGear(int v)
{
    m_cur_gear = v;
}

int EngineSim::getGearRange()
{
    return m_cur_gear_range;
}

void EngineSim::setGearRange(int v)
{
    m_cur_gear_range = v;
}

void EngineSim::stop()
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
    autocurAcc = val;
    if (!shifting)
    {
        setAcc(val);
    }
}

void EngineSim::shift(int val)
{
    if (!val || m_cur_gear + val < -1 || m_cur_gear + val > getNumGears())
        return;
    if (automode < MANUAL)
    {
        shiftval = val;
        shifting = 1;
        shiftclock = 0.0f;
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
    if (autoselect == MANUALMODE)
        return;

    SOUND_PLAY_ONCE(m_actor, SS_TRIG_SHIFT);

    if (autoselect == REAR)
    {
        m_cur_gear = -1;
    }
    else if (autoselect == NEUTRAL)
    {
        m_cur_gear = 0;
    }
    else if (autoselect == ONE)
    {
        m_cur_gear = 1;
    }
    else
    {
        // search for an appropriate gear
        int newGear = 1;

        while (newGear < m_num_gears && m_cur_wheel_revolutions > 0.0f && m_cur_wheel_revolutions * m_gear_ratios[newGear + 1] > maxRPM - 100.0f)
        {
            newGear++;
        }

        m_cur_gear = newGear;

        if (autoselect == TWO)
        {
            m_cur_gear = std::min(m_cur_gear, 2);
        }
    }
}

void EngineSim::autoShiftSet(int mode)
{
    autoselect = (autoswitch)mode;
    updateShifts();
}

void EngineSim::autoShiftUp()
{
    if (autoselect != REAR)
    {
        autoselect = (autoswitch)(autoselect - 1);
        updateShifts();
    }
}

void EngineSim::autoShiftDown()
{
    if (autoselect != ONE)
    {
        autoselect = (autoswitch)(autoselect + 1);
        updateShifts();
    }
}

int EngineSim::getAutoShift()
{
    return (int)autoselect;
}

void EngineSim::setManualClutch(float val)
{
    if (automode >= MANUAL)
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

    if (turboVer == 1)
    {
        for (int i = 0; i < numTurbos; i++)
        {
            atValue += EngineAddiTorque[i] * (curTurboRPM[i] / maxTurboRPM);
        }
    }
    else
    {
        atValue = (((getTurboPSI() * 6.8) * engineTorque) / 100); //1psi = 6% more power
    }

    return atValue;
}

float EngineSim::getEnginePower(float rpm)
{
    // engine power with limiter
    float tqValue = 1.0f; // ratio (0-1)

    float rpmRatio = rpm / (maxRPM * 1.25f);

    rpmRatio = std::max(0.0f, rpmRatio);
    rpmRatio = std::min(rpmRatio, 1.0f);

    if (torqueCurve)
    {
        tqValue = torqueCurve->getEngineTorque(rpmRatio);
    }

    return (engineTorque * tqValue) + getTurboPower();
}

float EngineSim::getAccToHoldRPM(float rpm)
{
    float rpmRatio = rpm / (maxRPM * 1.25f);

    rpmRatio = std::min(rpmRatio, 1.0f);

    return (-m_braking_torque * rpmRatio) / getEnginePower(m_cur_engine_rpm);
}

float EngineSim::getIdleMixture()
{
    if (m_cur_engine_rpm < idleRPM)
    {
        // determine the fuel injection needed to counter the engine braking force
        float idleMix = getAccToHoldRPM(m_cur_engine_rpm);

        idleMix = std::max(0.06f, idleMix);

        idleMix = idleMix * (1.0f + (idleRPM - m_cur_engine_rpm) / 100.0f);

        idleMix = std::max(minIdleMixture, idleMix);
        idleMix = std::min(idleMix, maxIdleMixture);

        return idleMix;
    }

    return 0.0f;
}

float EngineSim::getPrimeMixture()
{
    if (is_priming)
    {
        float crankfactor = getCrankFactor();

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
