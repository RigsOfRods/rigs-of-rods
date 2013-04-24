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

#include "Scripting.h"
#include "SoundScriptManager.h"
#include "TorqueCurve.h"

BeamEngine::BeamEngine( float iddle, float max, float torque, std::vector<float> gears, float diff, int trucknum) :
	  apressure(0.0f)
	, autocurAcc(0.0f)
	, automode(AUTOMATIC)
	, autoselect(DRIVE)
	, brakingTorque(-torque / 5.0f)
	, clutchForce(10000.0f)
	, clutch_time(0.2f)
	, contact(false)
	, curAcc(0.0f)
	, curClutch(0.0f)
	, curClutchTorque(0.0f)
	, curEngineRPM(0.0f)
	, curGear(0)
	, curGearRange(0)
	, curWheelRevolutions(0.0f)
	, curTurboRPM(0.0f)
	, engineTorque(torque - brakingTorque)
	, gearsRatio(gears)
	, hasair(true)
	, hasturbo(true)
	, hydropump(0.0f)
	, idleRPM(iddle)
	, inertia(10.0f)
	, maxRPM(max)
	, numGears((int)gears.size() - 2)
	, post_shift_time(0.2f)
	, postshiftclock(0.0f)
	, postshifting(0)
	, prime(0)
	, running(false)
	, shift_time(0.5f)
	, shiftclock(0.0f)
	, shifting(0)
	, shiftval(0)
	, stallRPM(300.0f)
	, starter(0)
	, torqueCurve(new TorqueCurve())
	, trucknum(trucknum)
	, type('t')
{
	gearsRatio[0] = -gearsRatio[0];
	for (std::vector< float >::iterator it = gearsRatio.begin(); it != gearsRatio.end(); ++it)
	{
		(*it) *= diff;
	}
}

void BeamEngine::setOptions(float einertia, char etype, float eclutch, float ctime, float stime, float pstime)
{
	inertia = einertia;
	type = etype;
	clutchForce = eclutch;

	if (ctime > 0)  clutch_time = ctime;
	if (pstime > 0) post_shift_time = pstime;
	if (stime > 0)  shift_time = stime;

	if (etype == 'c')
	{
		// it's a car!
		hasturbo = false;
		hasair = false;
		// set default clutch force
		if (clutchForce < 0.0f)
		{
			clutchForce = 5000.0f;
		}
	} else
	{
		// it's a truck
		if (clutchForce < 0.0f)
		{
			clutchForce = 10000.0f;
		}
	}
}

void BeamEngine::update(float dt, int doUpdate)
{
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
		if (curTurboRPM < 200000.0f && running && curAcc > 0.06f)
		{
			turbotorque += 1.5f * curAcc * (curEngineRPM / maxRPM);
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

	// engine power with limiter
	float tqValue = 1.0f;

	float rpmRatio = curEngineRPM / (maxRPM * 1.25f);

	rpmRatio = std::min(rpmRatio, 1.0f);

	if (torqueCurve)
	{
		tqValue = torqueCurve->getEngineTorque(rpmRatio);
	}

	if (running && contact && curEngineRPM < (maxRPM * 1.25f) && curEngineRPM > stallRPM)
	{
		totaltorque += engineTorque * curAcc * tqValue;
	}

	if (running && curEngineRPM < stallRPM)
	{
		stop();
	}

	// starter
	if (contact && starter && curEngineRPM < stallRPM * 1.5f)
	{
		totaltorque += -brakingTorque;//1000.0f;
	}

	// restart
	if (!running && curEngineRPM > stallRPM && contact)
	{
		running = true;
#ifdef USE_OPENAL
		SoundScriptManager::getSingleton().trigStart(trucknum, SS_TRIG_ENGINE);
#endif // USE_OPENAL
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
	} else
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

			// clutch
			if (shiftclock < clutch_time)
			{
				curClutch = 1.0f - (shiftclock / clutch_time);
			} else if (shiftclock > (shift_time - clutch_time))
			{
				curClutch = 1.0f - (shift_time - shiftclock) / clutch_time;
			} else
			{
				curClutch = 0.0f;
			}

			// shift
			if (shiftval && shiftclock > clutch_time / 2.0f)
			{
#ifdef USE_OPENAL
				SoundScriptManager::getSingleton().trigStart(trucknum, SS_TRIG_SHIFT);
#endif // USE_OPENAL
				curGear += shiftval;
				if (automode == AUTOMATIC)
				{
					while (curGear > 2 && curWheelRevolutions * gearsRatio[curGear + 1] < idleRPM)
					{
						curGear -= 2;
					}
				}
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
		}

		if (postshifting)
		{
			postshiftclock += dt;
			if (postshiftclock > post_shift_time)
			{
				postshifting = 0;
			}
		}

		// auto declutch
		if (shifting)
		{
			// we are shifting, just avoid stalling in worst case
			if (curEngineRPM < stallRPM * 1.2f)
			{
				curClutch = 0.0f;
			}
		} else if (postshifting)
		{
			// we are postshifting, no gear change
			if (curEngineRPM < stallRPM * 1.2f && curAcc < 0.5f)
			{
				curClutch = 0.0f;
			} else
			{
				curClutch = 1.0f;
			}
		} else if (curEngineRPM < stallRPM * 1.2f && curAcc < 0.5f)
		{
			curClutch = 0.0f; /*curGear=0;*/
		} else if (std::abs(curGear) == 1)
		{
			// 1st gear : special
			if (curEngineRPM > idleRPM)
			{
				curClutch = ((curEngineRPM - idleRPM) / (maxRPM - idleRPM));
				curClutch = std::min(curClutch, 1.0f);
			} else
			{
				curClutch = 0.0f;
			}
		} else
		{
			curClutch = 1.0f;
		}
	}

	if (doUpdate && !shifting && !postshifting)
	{
		// gear hack
		if (automode == AUTOMATIC && curGear >= 0 && (autoselect == DRIVE || autoselect == TWO))
		{
			if ((curEngineRPM > maxRPM - 100.0f && curGear > 1) || curWheelRevolutions * gearsRatio[curGear + 1] > maxRPM - 100.0f)
			{
				if ((autoselect == DRIVE && curGear < numGears) || (autoselect == TWO && curGear < 2))
				{
					shift(1);
				}
			} else if (curGear > 1 && curEngineRPM < idleRPM)
			{
				shift(-1);
			}
		}

		// gear hack++
		if (automode == AUTOMATIC && autoselect == DRIVE && curGear > 0)
		{
			static float oneThirdRPMRange = (maxRPM - idleRPM) / 3.0f;
			static float halfRPMRange = (maxRPM - idleRPM) / 2.0f;
			static std::deque<float> rpms;
			static std::deque<float> accs;
			static std::deque<float> brakes;

			rpms.push_front(curEngineRPM);
			accs.push_front(curAcc);
			brakes.push_front(curBrake);

			float avgRPM = 0.0f;
			float avgAcc = 0.0f;
			float avgBrake = 0.0f;

			for (unsigned int i=0; i < accs.size(); i++)
			{
				avgRPM += rpms[i];
				avgAcc += accs[i];
				avgBrake += brakes[i];
			}

			avgRPM /= rpms.size();
			avgAcc /= accs.size();
			avgBrake /= brakes.size();

			int newGear = curGear;

			if (avgAcc > 0.8f && curEngineRPM < maxRPM - oneThirdRPMRange)
			{
				while (newGear > 1 && curWheelRevolutions * gearsRatio[newGear] < maxRPM - oneThirdRPMRange)
				{
					newGear--;
				}
			} else if (avgAcc > 0.6f && curAcc < 0.8f && curAcc > avgAcc + 0.3f && curEngineRPM < idleRPM + halfRPMRange)
			{
				if (newGear > 1 && curWheelRevolutions * gearsRatio[newGear] < idleRPM + halfRPMRange)
				{
					newGear--;
				}
			} else if (avgAcc > 0.4f && curAcc < 0.8f && curAcc > avgAcc + 0.1f && curEngineRPM < idleRPM + halfRPMRange)
			{
				if (newGear > 1 && curWheelRevolutions * gearsRatio[newGear] < idleRPM + oneThirdRPMRange)
				{
					newGear--;
				}
			} else if (avgBrake < 0.2f && curAcc < avgAcc + 0.1f && curEngineRPM > avgRPM - halfRPMRange / 10.0f)
			{
				if (avgAcc < 0.6f && avgAcc > 0.4f && curEngineRPM > idleRPM + oneThirdRPMRange && curEngineRPM < maxRPM - oneThirdRPMRange)
				{
					if (newGear < numGears && curWheelRevolutions * gearsRatio[newGear + 2] > idleRPM + oneThirdRPMRange)
					{
						newGear++;
					}
				} else if (avgAcc < 0.4f && avgAcc > 0.2f && curEngineRPM > idleRPM + oneThirdRPMRange)
				{
					if (newGear < numGears && curWheelRevolutions * gearsRatio[newGear + 2] > idleRPM + oneThirdRPMRange / 2.0f)
					{
						newGear++;
					}
				} else if (avgAcc < 0.2f && curEngineRPM > idleRPM + oneThirdRPMRange / 2.0f && curEngineRPM < idleRPM + halfRPMRange)
				{
					if (newGear < numGears && curWheelRevolutions * gearsRatio[newGear + 2] > idleRPM + oneThirdRPMRange / 2.0f)
					{
						newGear++;
					}
				}
			}

			if (newGear < curGear && std::abs(curWheelRevolutions * (gearsRatio[newGear + 1] - gearsRatio[curGear + 1])) > oneThirdRPMRange / 6.0f ||
				newGear > curGear && std::abs(curWheelRevolutions * (gearsRatio[newGear + 1] - gearsRatio[curGear + 1])) > oneThirdRPMRange / 3.0f)
			{
				shiftTo(newGear);
			}

			if (accs.size() > 50)
			{
				rpms.pop_back();
				accs.pop_back();
				brakes.pop_back();
			}
		}
#if 0
		// engine speed limiter
		if (curEngineRPM > maxRPM + 500.0f) // significantly lowers the top speed of most vehicles
			setAcc(0.0f);
	}
#endif
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
	if (prime)
	{
		if (curEngineRPM < 850.0f)
		{
			val = 1.0f;
		} else if (curEngineRPM < 900.0f)
		{
			float t = (900.0f - curEngineRPM) / 50.0f;

			val = std::max(val, t);
		}
	}
#ifdef USE_OPENAL
	SoundScriptManager::getSingleton().modulate(trucknum, SS_MOD_INJECTOR, val);
#endif // USE_OPENAL
	curAcc = val * 0.94f + 0.06f;
}

void BeamEngine::setBrake(float val)
{
	curBrake = val;
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
	return 1.0f + 4.0f * (curEngineRPM - 800.0f) / (maxRPM - 800.0f);
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
	autoselect = NEUTRAL;
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
			curGear  = std::max(-1, curGear);
			curGear  = std::min(curGear, numGears);
		}
	}
}

void BeamEngine::shiftTo(int newGear)
{
	if (newGear == curGear || newGear > (int)getNumGears()) return;
	if (automode < MANUAL)
	{
		shiftval = newGear - curGear;
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
			curGear = newGear;
			curGear = std::max(-1, curGear);
			curGear = std::min(curGear, numGears);
		}
	}
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
	} else if (autoselect == NEUTRAL)
	{
		curGear =  0;
	} else if (autoselect == ONE)
	{
		curGear =  1;
	} else
	{
		// search for an appropriate gear
		int newGear = 1;

		while (newGear < numGears && curWheelRevolutions > 0.0f && curWheelRevolutions * gearsRatio[newGear + 1] > maxRPM - 100.0f)
		{
			newGear++;
		}

		if (autoselect == DRIVE)
		{
			curGear = newGear;
		} else if (autoselect == TWO)
		{
			curGear = std::min(newGear, 2);
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

BeamEngine::~BeamEngine()
{
	// delete NULL is safe
	delete torqueCurve;
	torqueCurve = NULL;
}
