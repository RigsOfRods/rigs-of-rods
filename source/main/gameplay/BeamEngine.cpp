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

BeamEngine::BeamEngine(float minRPM, float maxRPM, float torque, std::vector<float> gears, float dratio, Beam* actor) :
    apressure(0.0f)
    , autocurAcc(0.0f)
    , automode(AUTOMATIC)
    , autoselect(DRIVE)
    , brakingTorque(-torque / 5.0f)
    , clutchForce(10000.0f)
    , clutchTime(0.2f)
    , contact(false)
    , curAcc(0.0f)
    , curClutch(0.0f)
    , curClutchTorque(0.0f)
    , curEngineRPM(0.0f)
    , curGear(0)
    , curGearRange(0)
    , curWheelRevolutions(0.0f)
    , diffRatio(dratio)
    , engineTorque(torque - brakingTorque)
    , gearsRatio(gears)
    , hasair(true)
    , hasturbo(true)
    , hydropump(0.0f)
    , idleRPM(std::min(minRPM, 800.0f))
    , inertia(10.0f)
    , kickdownDelayCounter(0)
    , maxIdleMixture(0.2f)
    , maxRPM(std::abs(maxRPM))
    , minIdleMixture(0.0f)
    , minRPM(std::abs(minRPM))
    , numGears((int)gears.size() - 2)
    , post_shift_time(0.2f)
    , postshiftclock(0.0f)
    , postshifting(0)
    , prime(0)
    , running(false)
    , shiftBehaviour(0.0f)
    , shift_time(0.5f)
    , shiftclock(0.0f)
    , shifting(0)
    , shiftval(0)
    , stallRPM(300.0f)
    , starter(0)
    , torqueCurve(new TorqueCurve())
    , m_actor(actor)
    , turbomode(OLD)
    , type('t')
    , upShiftDelayCounter(0)
    , is_Electric(false)
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

    gearsRatio[0] = -gearsRatio[0];
    for (std::vector<float>::iterator it = gearsRatio.begin(); it != gearsRatio.end(); ++it)
    {
        (*it) *= diffRatio;
    }

    for (int i = 0; i < MAXTURBO; i++)
    {
        EngineAddiTorque[i] = 0;
        curTurboRPM[i] = 0;
    }
}

BeamEngine::~BeamEngine()
{
    // delete NULL is safe
    delete torqueCurve;
    torqueCurve = NULL;
}

void BeamEngine::setTurboOptions(int type, float tinertiaFactor, int nturbos, float param1, float param2, float param3, float param4, float param5, float param6, float param7, float param8, float param9, float param10, float param11)
{
    hasturbo = true;
    turbomode = NEW;

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

void BeamEngine::setOptions(float einertia, char etype, float eclutch, float ctime, float stime, float pstime, float irpm, float srpm, float maximix, float minimix)
{
    inertia = einertia;
    type = etype;
    clutchForce = eclutch;

    if (ctime > 0)
        clutchTime = ctime;
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

    clutchTime = std::max(0.0f, clutchTime);
    shift_time = std::max(0.0f, shift_time);
    post_shift_time = std::max(0.0f, post_shift_time);

    clutchTime = std::min(clutchTime, shift_time * 0.9f);

    idleRPM = std::max(0.0f, idleRPM);
    stallRPM = std::max(0.0f, stallRPM);
    stallRPM = std::min(idleRPM, stallRPM);

    if (etype == 'c')
    {
        // it's a car!
        hasair = false;
        hasturbo = false;
        is_Electric = false;
        // set default clutch force
        if (clutchForce < 0.0f)
        {
            clutchForce = 5000.0f;
        }
    }
    else if (etype == 'e') //electric
    {
        is_Electric = true;
        hasair = false;
        hasturbo = false;
        if (clutchForce < 0.0f)
        {
            clutchForce = 5000.0f;
        }
    }
    else
    {
        is_Electric = false;
        // it's a truck
        if (clutchForce < 0.0f)
        {
            clutchForce = 10000.0f;
        }
    }
}

void BeamEngine::update(float dt, int doUpdate)
{
    Beam* truck = m_actor;
    int trucknum = m_actor->trucknum;
    float acc = curAcc;

    acc = std::max(getIdleMixture(), acc);
    acc = std::max(getPrimeMixture(), acc);

    if (doUpdate)
    {
#ifdef USE_OPENAL
        SoundScriptManager::getSingleton().modulate(trucknum, SS_MOD_INJECTOR, acc);
#endif // USE_OPENAL
    }

    if (hasair)
    {
        // air pressure
        apressure += dt * curEngineRPM;
        if (apressure > 50000.0f)
        {
#ifdef USE_OPENAL
            SoundScriptManager::getSingleton().trigOnce(trucknum, SS_TRIG_AIR_PURGE);
#endif // USE_OPENAL
            apressure = 0.0f;
        }
    }

    if (hasturbo)
    {
        if (turbomode == OLD)
        {
            // update turbo speed (lag)
            float turbotorque = 0.0f;
            float turboInertia = 0.000003f;

            // braking (compression)
            turbotorque -= curTurboRPM[0] / 200000.0f;

            // powering (exhaust) with limiter
            if (curTurboRPM[0] < 200000.0f && running && curAcc > 0.06f)
            {
                turbotorque += 1.5f * curAcc * (curEngineRPM / maxRPM);
            }
            else
            {
                turbotorque += 0.1f * (curEngineRPM / maxRPM);
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
                if (curEngineRPM >= turboEngineRpmOperation)
                {
                    if (curTurboRPM[i] <= maxTurboRPM && running && acc > 0.06f)
                    {
                        if (b_WasteGate)
                        {
                            if (curTurboRPM[i] < minWGPsi * wastegate_threshold_p && !b_flutter)
                            {
                                turbotorque += 1.5f * acc * (((curEngineRPM - turboEngineRpmOperation) / (maxRPM - turboEngineRpmOperation)));
                            }
                            else
                            {
                                b_flutter = true;
                                turbotorque -= (curTurboRPM[i] / maxTurboRPM) * 1.5;
                            }

                            if (b_flutter)
                            {
                                SoundScriptManager::getSingleton().trigStart(trucknum, SS_TRIG_TURBOWASTEGATE);
                                if (curTurboRPM[i] < minWGPsi * wastegate_threshold_n)
                                {
                                    b_flutter = false;
                                    SoundScriptManager::getSingleton().trigStop(trucknum, SS_TRIG_TURBOWASTEGATE);
                                }
                            }
                        }
                        else
                            turbotorque += 1.5f * acc * (((curEngineRPM - turboEngineRpmOperation) / (maxRPM - turboEngineRpmOperation)));
                    }
                    else
                    {
                        turbotorque += 0.1f * (((curEngineRPM - turboEngineRpmOperation) / (maxRPM - turboEngineRpmOperation)));
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
                    if (curTurboRPM[i] > 13 * 10000 && curAcc < 0.06f)
                    {
                        turbotorque += (turbotorque * 2.5);
                    }
                }

                // anti lag
                if (b_anti_lag && curAcc < 0.5)
                {
                    float f = frand();
                    if (curEngineRPM > minRPM_antilag && f > rnd_antilag_chance)
                    {
                        if (curTurboRPM[i] > maxTurboRPM * 0.35 && curTurboRPM[i] < maxTurboRPM)
                        {
                            turbotorque -= (turbotorque * (f * antilag_power_factor));
                            SoundScriptManager::getSingleton().trigStart(trucknum, SS_TRIG_TURBOBACKFIRE);
                        }
                    }
                    else
                        SoundScriptManager::getSingleton().trigStop(trucknum, SS_TRIG_TURBOBACKFIRE);
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
                        turboBOVtorque += 1.5f * acc * (((curEngineRPM) / (maxRPM)));
                    else
                        turboBOVtorque += 0.07f * (((curEngineRPM) / (maxRPM)));

                    if (curAcc < 0.06 && curTurboRPM[i] > minBOVPsi * 10000)
                    {
                        SoundScriptManager::getSingleton().trigStart(trucknum, SS_TRIG_TURBOBOV);
                        curBOVTurboRPM[i] += dt * turboBOVtorque / (turboInertia * 0.1);
                    }
                    else
                    {
                        SoundScriptManager::getSingleton().trigStop(trucknum, SS_TRIG_TURBOBOV);
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
    if (contact)
    {
        totaltorque += brakingTorque * curEngineRPM / maxRPM;
    }
    else
    {
        totaltorque += brakingTorque;
    }

    // braking by hydropump
    if (curEngineRPM > 100.0f)
    {
        totaltorque -= 8.0f * hydropump / (curEngineRPM * 0.105f * dt);
    }

    if (running && contact && curEngineRPM < (maxRPM * 1.25f))
    {
        totaltorque += getEnginePower(curEngineRPM) * acc;
    }

    if (!is_Electric)
    {
        if (running && curEngineRPM < stallRPM)
        {
            stop();
        }

        if (contact && starter && !running)
        {
            if (curEngineRPM < stallRPM)
            {
                totaltorque += -brakingTorque;
            }
            else
            {
                running = true;
#ifdef USE_OPENAL
                SoundScriptManager::getSingleton().trigStart(trucknum, SS_TRIG_ENGINE);
#endif // USE_OPENAL
            }
        }
    }

    // clutch
    float retorque = 0.0f;

    if (curGear)
    {
        retorque = curClutchTorque / gearsRatio[curGear + 1];
    }

    totaltorque -= retorque;

    // integration
    curEngineRPM += dt * totaltorque / inertia;

    // update clutch torque
    if (curGear)
    {
        float gearboxspinner = curEngineRPM / gearsRatio[curGear + 1];
        curClutchTorque = (gearboxspinner - curWheelRevolutions) * curClutch * clutchForce;
    }
    else
    {
        curClutchTorque = 0.0f;
    }

    curEngineRPM = std::max(0.0f, curEngineRPM);

    if (automode < MANUAL)
    {
        // auto-shift
        if (shifting)
        {
            shiftclock += dt;

            if (shiftval)
            {
                float declutchTime = std::min(shift_time - clutchTime, clutchTime);
                if (shiftclock <= declutchTime)
                {
                    // depress the clutch 
                    float ratio = pow(1.0f - (shiftclock / declutchTime), 2);
                    curClutch = std::min(ratio, curClutch);
                    curAcc = std::min(ratio, autocurAcc);
                }
                else
                {
                    // engage a gear
#ifdef USE_OPENAL
                    SoundScriptManager::getSingleton().trigStart(trucknum, SS_TRIG_SHIFT);
#endif // USE_OPENAL
                    if (autoselect != NEUTRAL)
                    {
                        curGear += shiftval;
                        curGear = std::max(-1, curGear);
                        curGear = std::min(curGear, numGears);
                    }
                    shiftval = 0;
                }
            }

            if (shiftclock > shift_time)
            {
                // we're done shifting
#ifdef USE_OPENAL
                SoundScriptManager::getSingleton().trigStop(trucknum, SS_TRIG_SHIFT);
#endif // USE_OPENAL
                setAcc(autocurAcc);
                shifting = 0;
                postshifting = 1;
                postshiftclock = 0.0f;
            }
            else if (!shiftval && curGear && shiftclock >= (shift_time - clutchTime))
            {
                // release the clutch <-- Done below
                float timer = shiftclock - (shift_time - clutchTime);
                float ratio = sqrt(timer / clutchTime);
                curAcc = (autocurAcc / 2.0f) * ratio;
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
                curAcc = (autocurAcc / 2.0f) + (autocurAcc / 2.0f) * ratio;
            }
            else if (curGear)
            {
                float gearboxspinner = curEngineRPM / gearsRatio[curGear + 1];
                if (curWheelRevolutions > gearboxspinner)
                {
                    float ratio = sqrt(postshiftclock / post_shift_time);
                    curClutch = std::max(curClutch, ratio);
                }
            }
        }

        // auto clutch
        float declutchRPM = (minRPM + stallRPM) / 2.0f;
        if (curGear == 0 || curEngineRPM < declutchRPM || (fabs(curWheelRevolutions) < 1.0f && (curEngineRPM < minRPM * 1.01f || autocurAcc == 0.0f)) || (autocurAcc == 0.0f && truck->brake > 0.0f && retorque >= 0.0f))
        {
            curClutch = 0.0f;
        }
        else if (curEngineRPM < minRPM && minRPM > declutchRPM)
        {
            float clutch = (curEngineRPM - declutchRPM) / (minRPM - declutchRPM);
            curClutch = std::min(clutch * clutch, curClutch);
        }
        else if (!shiftval && curEngineRPM > minRPM && curClutch < 1.0f)
        {
            float range = (maxRPM - minRPM);
            float tAcc = std::max(0.2f, acc);
            if (abs(curGear) == 1)
            {
                range *= 0.8f * sqrt(tAcc);
            }
            else
            {
                range *= 0.4f * sqrt(tAcc);
            }
            float powerRatio = std::min((curEngineRPM - minRPM) / range, 1.0f);
            float enginePower = getEnginePower(curEngineRPM) * tAcc * powerRatio;

            float gearboxspinner = curEngineRPM / gearsRatio[curGear + 1];
            float clutchTorque = (gearboxspinner - curWheelRevolutions) * clutchForce;
            float reTorque = clutchTorque / gearsRatio[curGear + 1];

            float torqueDiff = std::abs(reTorque);
            float newRPM = std::abs(curWheelRevolutions * gearsRatio[curGear + 1]);
            if (getEnginePower(newRPM) >= getEnginePower(curEngineRPM))
            {
                torqueDiff = std::min(enginePower * 2.0f, torqueDiff);
            }
            else
            {
                torqueDiff = std::min(enginePower * 0.9f, torqueDiff);
            }
            float newClutch = torqueDiff * gearsRatio[curGear + 1] / ((gearboxspinner - curWheelRevolutions) * clutchForce);

            curClutch = std::max(curClutch, newClutch);
        }

        curClutch = std::max(0.0f, curClutch);
        curClutch = std::min(curClutch, 1.0f);
    }

    if (doUpdate && !shifting && !postshifting)
    {
        // gear hack
        absVelocity = truck->nodes[0].Velocity.length();
        float velocity = absVelocity;

        if (truck->cameranodepos[0] >= 0 && truck->cameranodedir[0] >= 0)
        {
            Vector3 hdir = (truck->nodes[truck->cameranodepos[0]].RelPosition - truck->nodes[truck->cameranodedir[0]].RelPosition).normalisedCopy();
            velocity = hdir.dotProduct(truck->nodes[0].Velocity);
        }
        relVelocity = std::abs(velocity);

        if (truck->wheels[0].radius != 0)
        {
            refWheelRevolutions = velocity / truck->wheels[0].radius * RAD_PER_SEC_TO_RPM;
        }

        if (!is_Electric && automode == AUTOMATIC && (autoselect == DRIVE || autoselect == TWO) && curGear > 0)
        {
            if ((curEngineRPM > maxRPM - 100.0f && curGear > 1) || curWheelRevolutions * gearsRatio[curGear + 1] > maxRPM - 100.0f)
            {
                if ((autoselect == DRIVE && curGear < numGears && curClutch > 0.99f) || (autoselect == TWO && curGear < std::min(2, numGears)))
                {
                    kickdownDelayCounter = 100;
                    shift(1);
                }
            }
            else if (curGear > 1 && refWheelRevolutions * gearsRatio[curGear] < maxRPM && (curEngineRPM < minRPM || (curEngineRPM < minRPM + shiftBehaviour * halfRPMRange / 2.0f &&
                getEnginePower(curWheelRevolutions * gearsRatio[curGear]) > getEnginePower(curWheelRevolutions * gearsRatio[curGear + 1]))))
            {
                shift(-1);
            }

            int newGear = curGear;

            float brake = 0.0f;

            if (truck->brakeforce > 0.0f)
            {
                brake = truck->brake / truck->brakeforce;
            }

            rpms.push_front(curEngineRPM);
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

            if (avgAcc50 > 0.8f && curEngineRPM < maxRPM - oneThirdRPMRange)
            {
                while (newGear > 1 && curWheelRevolutions * gearsRatio[newGear] < maxRPM - oneThirdRPMRange &&
                    getEnginePower(curWheelRevolutions * gearsRatio[newGear]) * gearsRatio[newGear] >
                    getEnginePower(curWheelRevolutions * gearsRatio[newGear + 1]) * gearsRatio[newGear + 1])
                {
                    newGear--;
                }
            }
            else if (avgAcc50 > 0.6f && acc < 0.8f && acc > avgAcc50 + 0.1f && curEngineRPM < minRPM + halfRPMRange)
            {
                if (newGear > 1 && curWheelRevolutions * gearsRatio[newGear] < minRPM + halfRPMRange &&
                    getEnginePower(curWheelRevolutions * gearsRatio[newGear]) * gearsRatio[newGear] >
                    getEnginePower(curWheelRevolutions * gearsRatio[newGear + 1]) * gearsRatio[newGear + 1])
                {
                    newGear--;
                }
            }
            else if (avgAcc50 > 0.4f && acc < 0.8f && acc > avgAcc50 + 0.1f && curEngineRPM < minRPM + halfRPMRange)
            {
                if (newGear > 1 && curWheelRevolutions * gearsRatio[newGear] < minRPM + oneThirdRPMRange &&
                    getEnginePower(curWheelRevolutions * gearsRatio[newGear]) * gearsRatio[newGear] >
                    getEnginePower(curWheelRevolutions * gearsRatio[newGear + 1]) * gearsRatio[newGear + 1])
                {
                    newGear--;
                }
            }
            else if (curGear < (autoselect == TWO ? std::min(2, numGears) : numGears) &&
                avgBrake200 < 0.2f && acc < std::min(avgAcc200 + 0.1f, 1.0f) && curEngineRPM > avgRPM200 - fullRPMRange / 20.0f)
            {
                if (avgAcc200 < 0.6f && avgAcc200 > 0.4f && curEngineRPM > minRPM + oneThirdRPMRange && curEngineRPM < maxRPM - oneThirdRPMRange)
                {
                    if (curWheelRevolutions * gearsRatio[newGear + 2] > minRPM + oneThirdRPMRange)
                    {
                        newGear++;
                    }
                }
                else if (avgAcc200 < 0.4f && avgAcc200 > 0.2f && curEngineRPM > minRPM + oneThirdRPMRange)
                {
                    if (curWheelRevolutions * gearsRatio[newGear + 2] > minRPM + oneThirdRPMRange / 2.0f)
                    {
                        newGear++;
                    }
                }
                else if (avgAcc200 < 0.2f && curEngineRPM > minRPM + oneThirdRPMRange / 2.0f && curEngineRPM < minRPM + halfRPMRange)
                {
                    if (curWheelRevolutions * gearsRatio[newGear + 2] > minRPM + oneThirdRPMRange / 2.0f)
                    {
                        newGear++;
                    }
                }

                if (newGear > curGear)
                {
                    upShiftDelayCounter++;
                    if (upShiftDelayCounter <= 100 * shiftBehaviour)
                    {
                        newGear = curGear;
                    }
                }
                else
                {
                    upShiftDelayCounter = 0;
                }
            }

            if (newGear < curGear && kickdownDelayCounter > 0)
            {
                newGear = curGear;
            }
            kickdownDelayCounter = std::max(0, kickdownDelayCounter - 1);

            if (newGear < curGear && std::abs(curWheelRevolutions * (gearsRatio[newGear + 1] - gearsRatio[curGear + 1])) > oneThirdRPMRange / 6.0f ||
                newGear > curGear && std::abs(curWheelRevolutions * (gearsRatio[newGear + 1] - gearsRatio[curGear + 1])) > oneThirdRPMRange / 3.0f)
            {
                if (absVelocity - relVelocity < 0.5f)
                    shiftTo(newGear);
            }

            if (accs.size() > 200)
            {
                rpms.pop_back();
                accs.pop_back();
                brakes.pop_back();
            }
            // avoid over-revving
            if (automode <= SEMIAUTO && curGear != 0)
            {
                if (std::abs(curWheelRevolutions * gearsRatio[curGear + 1]) > maxRPM * 1.25f)
                {
                    float clutch = 0.0f + 1.0f / (1.0f + std::abs(curWheelRevolutions * gearsRatio[curGear + 1] - maxRPM * 1.25f) / 2.0f);
                    curClutch = std::min(clutch, curClutch);
                }
                if (curGear * curWheelRevolutions < -10.0f)
                {
                    float clutch = 0.0f + 1.0f / (1.0f + std::abs(-10.0f - curGear * curWheelRevolutions) / 2.0f);
                    curClutch = std::min(clutch, curClutch);
                }
            }
        }
    }

    // audio stuff
    updateAudio(doUpdate);
}

void BeamEngine::updateAudio(int doUpdate)
{
#ifdef USE_OPENAL
    const int trucknum = m_actor->trucknum;

    if (hasturbo)
    {
        for (int i = 0; i < numTurbos; i++)
            SoundScriptManager::getSingleton().modulate(trucknum, SS_MOD_TURBO, curTurboRPM[i]);
    }

    if (doUpdate)
    {
        SoundScriptManager::getSingleton().modulate(trucknum, SS_MOD_ENGINE, curEngineRPM);
        SoundScriptManager::getSingleton().modulate(trucknum, SS_MOD_TORQUE, curClutchTorque);
        SoundScriptManager::getSingleton().modulate(trucknum, SS_MOD_GEARBOX, curWheelRevolutions);
    }
    // reverse gear beep
    if (curGear == -1 && running)
    {
        SoundScriptManager::getSingleton().trigStart(trucknum, SS_TRIG_REVERSE_GEAR);
    }
    else
    {
        SoundScriptManager::getSingleton().trigStop(trucknum, SS_TRIG_REVERSE_GEAR);
    }
#endif // USE_OPENAL
}

float BeamEngine::getRPM()
{
    return curEngineRPM;
}

void BeamEngine::toggleAutoMode()
{
    automode = (automode + 1) % (MANUAL_RANGES + 1);

    if (automode == AUTOMATIC)
    {
        if (curGear > 0)
            autoselect = DRIVE;
        if (curGear < 0)
            autoselect = REAR;
        if (curGear == 0)
            autoselect = NEUTRAL;
    }
    else
    {
        autoselect = MANUALMODE;
    }

    if (automode == MANUAL_RANGES)
    {
        curGearRange = 0;
    }
}

RoR::SimGearboxMode BeamEngine::getAutoMode()
{
    return (RoR::SimGearboxMode)this->automode;
}

void BeamEngine::setAutoMode(RoR::SimGearboxMode mode)
{
    this->automode = (shiftmodes)mode;
}

void BeamEngine::setAcc(float val)
{
    curAcc = val;
}

float BeamEngine::getTurboPSI()
{
    if (turbomode == OLD)
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

float BeamEngine::getAcc()
{
    return curAcc;
}

// this is mainly for smoke...
void BeamEngine::netForceSettings(float rpm, float force, float clutch, int gear, bool _running, bool _contact, char _automode)
{
    curEngineRPM = rpm;
    curAcc = force;
    curClutch = clutch;
    curGear = gear;
    running = _running; //(fabs(rpm)>10.0);
    contact = _contact;
    if (_automode != -1)
    {
        automode = _automode;
    }
}

float BeamEngine::getSmoke()
{
    if (running)
    {
        return curAcc * (1.0f - curTurboRPM[0] /* doesn't matter */ / maxTurboRPM);// * engineTorque / 5000.0f;
    }

    return -1;
}

float BeamEngine::getTorque()
{
    if (curClutchTorque > 1000000.0)
        return 1000000.0;
    if (curClutchTorque < -1000000.0)
        return -1000000.0;
    return curClutchTorque;
}

void BeamEngine::setRPM(float rpm)
{
    curEngineRPM = rpm;
}

void BeamEngine::setPrime(int p)
{
    prime = p;
}

void BeamEngine::setHydroPumpWork(float work)
{
    hydropump = work;
}

void BeamEngine::setSpin(float rpm)
{
    curWheelRevolutions = rpm;
}

// for hydros acceleration
float BeamEngine::getCrankFactor()
{
    float minWorkingRPM = idleRPM * 1.1f; // minWorkingRPM > idleRPM avoids commands deadlocking the engine

    float rpmRatio = (curEngineRPM - minWorkingRPM) / (maxRPM - minWorkingRPM);
    rpmRatio = std::max(0.0f, rpmRatio); // Avoids a negative rpmRatio when curEngineRPM < minWorkingRPM
    rpmRatio = std::min(rpmRatio, 1.0f); // Avoids a rpmRatio > 1.0f when curEngineRPM > maxRPM

    float crankfactor = 5.0f * rpmRatio;

    return crankfactor;
}

void BeamEngine::setClutch(float clutch)
{
    curClutch = clutch;
}

float BeamEngine::getClutch()
{
    return curClutch;
}

float BeamEngine::getClutchForce()
{
    return clutchForce;
}

void BeamEngine::toggleContact()
{
    contact = !contact;
#ifdef USE_OPENAL
    if (contact)
    {
        SoundScriptManager::getSingleton().trigStart(m_actor->trucknum, SS_TRIG_IGNITION);
    }
    else
    {
        SoundScriptManager::getSingleton().trigStop(m_actor->trucknum, SS_TRIG_IGNITION);
    }
#endif // USE_OPENAL
}

void BeamEngine::start()
{
    offstart();
    contact = true;
    curEngineRPM = idleRPM;
    running = true;
    if (automode <= SEMIAUTO)
    {
        curGear = 1;
    }
    if (automode == AUTOMATIC)
    {
        autoselect = DRIVE;
    }
#ifdef USE_OPENAL
    SoundScriptManager::getSingleton().trigStart(m_actor->trucknum, SS_TRIG_IGNITION);
    SoundScriptManager::getSingleton().trigStart(m_actor->trucknum, SS_TRIG_ENGINE);
#endif // USE_OPENAL
}

void BeamEngine::offstart()
{
    apressure = 0.0f;
    autoselect = MANUALMODE;
    contact = false;
    curAcc = 0.0f;
    curClutch = 0.0f;
    curClutchTorque = 0.0f;
    curEngineRPM = 0.0f;
    curGear = 0;
    postshifting = 0;
    running = false;
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

void BeamEngine::setstarter(int v)
{
    starter = v;
}

int BeamEngine::getGear()
{
    return curGear;
}

// low level gear changing
void BeamEngine::setGear(int v)
{
    curGear = v;
}

int BeamEngine::getGearRange()
{
    return curGearRange;
}

void BeamEngine::setGearRange(int v)
{
    curGearRange = v;
}

void BeamEngine::stop()
{
    if (!running)
        return;

    running = false;
    // Script Event - engine death
    TRIGGER_EVENT(SE_TRUCK_ENGINE_DIED, m_actor->trucknum);
#ifdef USE_OPENAL
    SoundScriptManager::getSingleton().trigStop(m_actor->trucknum, SS_TRIG_ENGINE);
#endif // USE_OPENAL
}

// high level controls
void BeamEngine::autoSetAcc(float val)
{
    autocurAcc = val;
    if (!shifting)
    {
        setAcc(val);
    }
}

void BeamEngine::shift(int val)
{
    if (!val || curGear + val < -1 || curGear + val > getNumGears())
        return;
    if (automode < MANUAL)
    {
        shiftval = val;
        shifting = 1;
        shiftclock = 0.0f;
    }
    else
    {
        if (curClutch > 0.25f)
        {
#ifdef USE_OPENAL
            SoundScriptManager::getSingleton().trigOnce(m_actor->trucknum, SS_TRIG_GEARSLIDE);
#endif // USE_OPENAL
        }
        else
        {
#ifdef USE_OPENAL
            SoundScriptManager::getSingleton().trigOnce(m_actor->trucknum, SS_TRIG_SHIFT);
#endif // USE_OPENAL
            curGear += val;
        }
    }
}

void BeamEngine::shiftTo(int newGear)
{
    shift(newGear - curGear);
}

void BeamEngine::updateShifts()
{
    if (is_Electric)
        return;
    if (autoselect == MANUALMODE)
        return;

#ifdef USE_OPENAL
    SoundScriptManager::getSingleton().trigOnce(m_actor->trucknum, SS_TRIG_SHIFT);
#endif // USE_OPENAL

    if (autoselect == REAR)
    {
        curGear = -1;
    }
    else if (autoselect == NEUTRAL)
    {
        curGear = 0;
    }
    else if (autoselect == ONE)
    {
        curGear = 1;
    }
    else
    {
        // search for an appropriate gear
        int newGear = 1;

        while (newGear < numGears && curWheelRevolutions > 0.0f && curWheelRevolutions * gearsRatio[newGear + 1] > maxRPM - 100.0f)
        {
            newGear++;
        }

        curGear = newGear;

        if (autoselect == TWO)
        {
            curGear = std::min(curGear, 2);
        }
    }
}

void BeamEngine::autoShiftSet(int mode)
{
    autoselect = (autoswitch)mode;
    updateShifts();
}

void BeamEngine::autoShiftUp()
{
    if (autoselect != REAR)
    {
        autoselect = (autoswitch)(autoselect - 1);
        updateShifts();
    }
}

void BeamEngine::autoShiftDown()
{
    if (autoselect != ONE)
    {
        autoselect = (autoswitch)(autoselect + 1);
        updateShifts();
    }
}

int BeamEngine::getAutoShift()
{
    return (int)autoselect;
}

void BeamEngine::setManualClutch(float val)
{
    if (automode >= MANUAL)
    {
        val = std::max(0.0f, val);
        curClutch = 1.0 - val;
    }
}

float BeamEngine::getTurboPower()
{
    if (!hasturbo)
        return 0.0f;
    if (turbomode != NEW)
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

float BeamEngine::getEnginePower(float rpm)
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

float BeamEngine::getAccToHoldRPM(float rpm)
{
    float rpmRatio = rpm / (maxRPM * 1.25f);

    rpmRatio = std::min(rpmRatio, 1.0f);

    return (-brakingTorque * rpmRatio) / getEnginePower(curEngineRPM);
}

float BeamEngine::getIdleMixture()
{
    if (curEngineRPM < idleRPM)
    {
        // determine the fuel injection needed to counter the engine braking force
        float idleMix = getAccToHoldRPM(curEngineRPM);

        idleMix = std::max(0.06f, idleMix);

        idleMix = idleMix * (1.0f + (idleRPM - curEngineRPM) / 100.0f);

        idleMix = std::max(minIdleMixture, idleMix);
        idleMix = std::min(idleMix, maxIdleMixture);

        return idleMix;
    }

    return 0.0f;
}

float BeamEngine::getPrimeMixture()
{
    if (prime)
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
