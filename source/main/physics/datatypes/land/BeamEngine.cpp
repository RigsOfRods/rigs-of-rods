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
#include "SoundScriptManager.h"
#include "TorqueCurve.h"
#include "Scripting.h"

using namespace Ogre;

BeamEngine::BeamEngine(float minRPM, float maxRPM, float torque, std::vector<float> gears, float dratio, int trucknum) :
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
	, curTurboRPM(0.0f)
	, curWheelRevolutions(0.0f)
	, diffRatio(dratio)
	, engineTorque(torque - brakingTorque)
	, gearsRatio(gears)
	, hasair(true)
	, hasturbo(true)
	, hydropump(0.0f)
	, idleRPM(std::min(minRPM, 800.0f))
	, inertia(10.0f)
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
	, trucknum(trucknum)
	, type('t')
	, upShiftDelayCounter(0)
	, is_Electric(false)
{
	fullRPMRange = (maxRPM - minRPM);
	oneThirdRPMRange = fullRPMRange / 3.0f;
	halfRPMRange = fullRPMRange / 2.0f;

	gearsRatio[0] = -gearsRatio[0];
	for (std::vector< float >::iterator it = gearsRatio.begin(); it != gearsRatio.end(); ++it)
	{
		(*it) *= diffRatio;
	}
}

BeamEngine::~BeamEngine()
{
	// delete NULL is safe
	delete torqueCurve;
	torqueCurve = NULL;
}

void BeamEngine::setOptions(float einertia, char etype, float eclutch, float ctime, float stime, float pstime, float irpm, float srpm, float maximix, float minimix)
{
	inertia = einertia;
	type = etype;
	clutchForce = eclutch;

	if (ctime > 0)  clutchTime = ctime;
	if (pstime > 0) post_shift_time = pstime;
	if (stime > 0)  shift_time = stime;
	if (irpm > 0) idleRPM = irpm;
	if (srpm > 0) idleRPM = srpm;
	if (maximix > 0) maxIdleMixture = maximix;
	if (minimix > 0) minIdleMixture = minimix;

	if (etype == 'c')
	{
		// it's a car!
		hasturbo = false;
		hasair = false;
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
		hasturbo = false;
		hasair = false;
		if (clutchForce < 0.0f)
		{
			clutchForce = 5000.0f;
		}
	} else
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
	Beam* truck = BeamFactory::getSingleton().getTruck(trucknum);

	if (!truck) return;

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
		// update turbo speed (lag)
		float turbotorque = 0.0f;
		float turboInertia = 0.000003f;

		// braking (compression)
		turbotorque -= curTurboRPM / 200000.0f;

		// powering (exhaust) with limiter
		if (curTurboRPM < 200000.0f && running && acc > 0.06f)
		{
			turbotorque += 1.5f * acc * (curEngineRPM / maxRPM);
		} else
		{
			turbotorque += 0.1f * (curEngineRPM / maxRPM);
		}

		// integration
		curTurboRPM += dt * turbotorque / turboInertia;
	}

	// update engine speed
	float totaltorque = 0.0f;

	// engine braking
	if (contact)
	{
		totaltorque += brakingTorque * curEngineRPM / maxRPM;
	} else
	{
		totaltorque += 10.0f * brakingTorque * curEngineRPM / maxRPM;
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
			stop(); //No, electric engine has no stop
		}
		// starter

		if (contact && starter && curEngineRPM < stallRPM * 1.5f)
		{
			totaltorque += -brakingTorque; //No starter in electric engines
		}
		// restart

		if (!running && curEngineRPM > stallRPM && contact)
		{
			running = true;
#ifdef USE_OPENAL
			SoundScriptManager::getSingleton().trigStart(trucknum, SS_TRIG_ENGINE);
#endif // USE_OPENAL
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
		curClutchTorque = (gearboxspinner - curWheelRevolutions) * curClutch * curClutch * clutchForce;
	} else
	{
		curClutchTorque = 0.0f;
	}

	curEngineRPM = std::max(0.0f, curEngineRPM);

	if (automode < MANUAL)
	{
		// auto-shift
		if (shifting && !is_Electric) //No shifting in electric cars
		{
			shiftclock += dt;

			// clutch
			if (shiftclock < clutchTime)
			{
				curClutch = 1.0f - (shiftclock / clutchTime);
			} else if (shiftclock > (shift_time - clutchTime))
			{
				curClutch = 1.0f - (shift_time - shiftclock) / clutchTime;
			} else
			{
				curClutch = 0.0f;
			}

			// shift
			if (shiftval && shiftclock > clutchTime / 2.0f)
			{
#ifdef USE_OPENAL
				SoundScriptManager::getSingleton().trigStart(trucknum, SS_TRIG_SHIFT);
#endif // USE_OPENAL
				curGear += shiftval;
				curGear = std::max(-1, curGear);
				curGear = std::min(curGear, numGears);
				shiftval = 0;
			}

			// end of shifting
			if (shiftclock > shift_time)
			{
#ifdef USE_OPENAL
				SoundScriptManager::getSingleton().trigStop(trucknum, SS_TRIG_SHIFT);
#endif // USE_OPENAL
				setAcc(autocurAcc);
				shifting = 0;
				curClutch = 1.0f;
				postshifting = 1;
				postshiftclock = 0.0f;
			}
		} else
			setAcc(autocurAcc);



		// auto declutch
		if (!is_Electric)
		{
			if (postshifting)
			{
				postshiftclock += dt;
				if (postshiftclock > post_shift_time)
				{
					postshifting = 0;
				}
			}
			if (shifting)
			{
				// we are shifting, just avoid stalling in worst case
				if (curEngineRPM < stallRPM * 1.2f)
				{
					curClutch = 0.0f;
				}
			}
			else if (postshifting)
			{
				// we are postshifting, no gear change
				if (curEngineRPM < stallRPM * 1.2f && acc < 0.5f)
				{
					curClutch = 0.0f;
				}
				else
				{
					curClutch = 1.0f;
				}
			}
			else if (curEngineRPM < stallRPM * 1.2f && acc < 0.5f)
			{
				curClutch = 0.0f;
			}
			else if (std::abs(curGear) == 1)
			{
				// 1st gear : special
				if (curEngineRPM > minRPM)
				{
					curClutch = (curEngineRPM - minRPM) / (maxRPM - minRPM);
					curClutch = std::min(curClutch, 1.0f);
				}
				else
				{
					curClutch = 0.0f;
				}
			}
			else
			{
				curClutch = 1.0f;
			}
		} else
			curClutch = 1.0f;
	}

	if (doUpdate && !shifting && !postshifting)
	{
		// gear hack
		absVelocity = truck->nodes[0].Velocity.length();
		float velocity = absVelocity;

		if (truck->cameranodepos[0] >= 0 && truck->cameranodedir[0] >=0)
		{
			Vector3 hdir = (truck->nodes[truck->cameranodepos[0]].RelPosition - truck->nodes[truck->cameranodedir[0]].RelPosition).normalisedCopy();
			velocity = hdir.dotProduct(truck->nodes[0].Velocity);
		}
		relVelocity = std::abs(velocity);

		if (truck->wheels[0].radius != 0)
		{
			refWheelRevolutions = velocity / truck->wheels[0].radius * RAD_PER_SEC_TO_RPM;
		}

		if (automode == AUTOMATIC && (autoselect == DRIVE || autoselect == TWO) && curGear > 0)
		{
			if ((curEngineRPM > maxRPM - 100.0f && curGear > 1) || curWheelRevolutions * gearsRatio[curGear + 1] > maxRPM - 100.0f)
			{
				if ((autoselect == DRIVE && curGear < numGears) || (autoselect == TWO && curGear < std::min(2, numGears)) && !is_Electric)
				{
					shift(1);
				}
			} else if (curGear > 1 && refWheelRevolutions * gearsRatio[curGear] < maxRPM && (curEngineRPM < minRPM || (curEngineRPM < minRPM + shiftBehaviour * halfRPMRange / 2.0f &&
				getEnginePower(curWheelRevolutions * gearsRatio[curGear]) > getEnginePower(curWheelRevolutions * gearsRatio[curGear + 1]))) && !is_Electric)
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

			for (unsigned int i=0; i < accs.size(); i++)
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

			if (!is_Electric)
			{

				if (avgAcc50 > 0.8f || avgAcc200 > 0.8f || avgBrake50 > 0.8f || avgBrake200 > 0.8f)
				{
					shiftBehaviour = std::min(shiftBehaviour + 0.01f, 1.0f);
				}
				else if (acc < 0.5f && avgAcc50 < 0.5f && avgAcc200 < 0.5f && brake < 0.5f && avgBrake50 < 0.5f && avgBrake200 < 0.5 )
				{
					shiftBehaviour /= 1.01;
				}
			

				if (avgAcc50 > 0.8f && curEngineRPM < maxRPM - oneThirdRPMRange)
				{
					while (newGear > 1 && curWheelRevolutions * gearsRatio[newGear] < maxRPM - oneThirdRPMRange &&
						getEnginePower(curWheelRevolutions * gearsRatio[newGear])   * gearsRatio[newGear] >
						getEnginePower(curWheelRevolutions * gearsRatio[newGear+1]) * gearsRatio[newGear+1])
					{
						newGear--;
					}
				} else if (avgAcc50 > 0.6f && acc < 0.8f && acc > avgAcc50 + 0.1f && curEngineRPM < minRPM + halfRPMRange)
				{
					if (newGear > 1 && curWheelRevolutions * gearsRatio[newGear] < minRPM + halfRPMRange &&
						getEnginePower(curWheelRevolutions * gearsRatio[newGear])   * gearsRatio[newGear] >
						getEnginePower(curWheelRevolutions * gearsRatio[newGear+1]) * gearsRatio[newGear+1])
					{
						newGear--;
					}
				} else if (avgAcc50 > 0.4f && acc < 0.8f && acc > avgAcc50 + 0.1f && curEngineRPM < minRPM + halfRPMRange)
				{
					if (newGear > 1 && curWheelRevolutions * gearsRatio[newGear] < minRPM + oneThirdRPMRange &&
						getEnginePower(curWheelRevolutions * gearsRatio[newGear])   * gearsRatio[newGear] >
						getEnginePower(curWheelRevolutions * gearsRatio[newGear+1]) * gearsRatio[newGear+1])
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
				if (newGear < curGear && std::abs(curWheelRevolutions * (gearsRatio[newGear + 1] - gearsRatio[curGear + 1])) > oneThirdRPMRange / 6.0f ||
					newGear > curGear && std::abs(curWheelRevolutions * (gearsRatio[newGear + 1] - gearsRatio[curGear + 1])) > oneThirdRPMRange / 3.0f && !is_Electric)
				{
					if (absVelocity - relVelocity < 0.5f)
						shiftTo(newGear);
				}
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
	if (hasturbo)
	{
		SoundScriptManager::getSingleton().modulate(trucknum, SS_MOD_TURBO, curTurboRPM);
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
	} else
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

	// this switches off all automatic symbols when in manual mode
	if (automode != AUTOMATIC)
	{
		autoselect = MANUALMODE;
	} else
	{
		autoselect = NEUTRAL;
	}

	if (automode == MANUAL_RANGES)
	{
		this->setGearRange(0);
		this->setGear(0);
	}
}

int BeamEngine::getAutoMode()
{
	return automode;
}

void BeamEngine::setAutoMode(int mode)
{
	automode = mode;
}

void BeamEngine::setAcc(float val)
{
	curAcc = val;
}

float BeamEngine::getTurboPSI()
{
	return curTurboRPM / 10000.0f;
}

float BeamEngine::getAcc()
{
	return curAcc;
}

// this is mainly for smoke...
void BeamEngine::netForceSettings(float rpm, float force, float clutch, int gear, bool _running, bool _contact, char _automode)
{
	curEngineRPM = rpm;
	curAcc       = force;
	curClutch    = clutch;
	curGear      = gear;
	running      = _running; //(fabs(rpm)>10.0);
	contact      = _contact;
	if (_automode != -1)
	{
		automode = _automode;
	}
}

float BeamEngine::getSmoke()
{
	const int maxTurboRPM = 200000.0f;

	if (running)
	{
		return curAcc * (1.0f - curTurboRPM / maxTurboRPM);// * engineTorque / 5000.0f;
	}

	return -1;
}

float BeamEngine::getTorque()
{
	if (curClutchTorque >  1000000.0) return  1000000.0;
	if (curClutchTorque < -1000000.0) return -1000000.0;
	return curClutchTorque;
}

void BeamEngine::setRPM(float rpm)
{
	curEngineRPM = rpm;
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
		SoundScriptManager::getSingleton().trigStart(trucknum, SS_TRIG_IGNITION);
	} else
	{
		SoundScriptManager::getSingleton().trigStop(trucknum, SS_TRIG_IGNITION);
	}
#endif // USE_OPENAL
}

// quick start
void BeamEngine::start()
{
	if (automode == AUTOMATIC)
	{
		curGear = 1;
		autoselect = DRIVE;
	} else
	{
		if (automode == SEMIAUTO)
		{
			curGear = 1;
		} else
		{
			curGear = 0;
		}
		autoselect = MANUALMODE;
	}
	curClutch = 0.0f;
	curEngineRPM = 750.0f;
	curClutchTorque = 0.0f;
	curTurboRPM = 0.0f;
	apressure = 0.0f;
	running = true;
	contact = true;
#ifdef USE_OPENAL
	SoundScriptManager::getSingleton().trigStart(trucknum, SS_TRIG_IGNITION);
	setAcc(0.0f);
	SoundScriptManager::getSingleton().trigStart(trucknum, SS_TRIG_ENGINE);
#endif // USE_OPENAL
}

void BeamEngine::offstart()
{
	curGear = 0;
	curClutch = 0.0f;
	if (!is_Electric)
		autoselect = NEUTRAL; //no Neutral in electric engines
	else
		autoselect = ONE;

	curEngineRPM = 0.0f;
	running = false;
	contact = false;
#ifdef USE_OPENAL
	SoundScriptManager::getSingleton().trigStop(trucknum, SS_TRIG_IGNITION);
	SoundScriptManager::getSingleton().trigStop(trucknum, SS_TRIG_ENGINE);
#endif // USE_OPENAL
}

void BeamEngine::setstarter(int v)
{
	starter = v;
	if (v && curEngineRPM < 750.0f)
	{
		setAcc(1.0f);
	}
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
	if (!running) return;

	running = false;
	// Script Event - engine death
	TRIGGER_EVENT(SE_TRUCK_ENGINE_DIED, trucknum);
#ifdef USE_OPENAL
	SoundScriptManager::getSingleton().trigStop(trucknum, SS_TRIG_ENGINE);
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
	if (!val || curGear + val < -1 || curGear + val > getNumGears()) return;
	if (automode < MANUAL)
	{
#ifdef USE_OPENAL
		SoundScriptManager::getSingleton().trigStart(trucknum, SS_TRIG_SHIFT);
#endif // USE_OPENAL
		shiftval = val;
		shifting = 1;
		shiftclock = 0.0f;
		setAcc(0.0f);
	} else
	{
		if (curClutch > 0.25f)
		{
#ifdef USE_OPENAL
			SoundScriptManager::getSingleton().trigOnce(trucknum, SS_TRIG_GEARSLIDE);
#endif // USE_OPENAL
		} else
		{
#ifdef USE_OPENAL
			SoundScriptManager::getSingleton().trigOnce(trucknum, SS_TRIG_SHIFT);
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
	if (autoselect == MANUALMODE) return;

#ifdef USE_OPENAL
	SoundScriptManager::getSingleton().trigOnce(trucknum, SS_TRIG_SHIFT);
#endif // USE_OPENAL

	if (autoselect == REAR)
	{
		curGear = -1;
	} else if (autoselect == NEUTRAL && !is_Electric)
	{
		curGear =  0;
	} else if (autoselect == ONE)
	{
		curGear =  1;
	}
	else if (!is_Electric) //no other gears for electric cars
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
		autoselect = (autoswitch)(autoselect-1);
		updateShifts();
	}
}

void BeamEngine::autoShiftDown()
{
	if (autoselect != ONE)
	{
		autoselect = (autoswitch)(autoselect+1);
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

float BeamEngine::getEnginePower(float rpm)
{
	// engine power with limiter
	float tqValue = 1.0f;

	float rpmRatio = rpm / (maxRPM * 1.25f);

	rpmRatio = std::min(rpmRatio, 1.0f);

	if (torqueCurve)
	{
		tqValue = torqueCurve->getEngineTorque(rpmRatio);
	}

	return engineTorque * tqValue;
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
		} else if (crankfactor < 1.0f)
		{
			// crankfactor is between 0.9f and 1.0f
			return 10.0f * (1.0f - crankfactor);
		}
	}

	return 0.0f;
}
