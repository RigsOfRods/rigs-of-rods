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
#ifndef __BeamEngine_H_
#define __BeamEngine_H_

#include "RoRPrerequisites.h"

/**
* Represents a virtual engine of a vehicle (not "engine" as in "physics engine").
*/
class BeamEngine : public ZeroedMemoryAllocator
{
	friend class Beam;
	friend class RigSpawner;
	friend class RigInspector; // Debug utility class

public:

	BeamEngine(float minRPM, float maxRPM, float torque, std::vector<float> gears, float dratio, int trucknum);
	~BeamEngine();

	float getAcc();
	float getClutch();
	float getClutchForce();
	float getCrankFactor();
	float getRPM();
	float getSmoke();
	float getTorque();
	float getTurboPSI();
	int getAutoMode();

	/**
	* Sets current engine state; Needed mainly for smoke.
	* @param rpm Current engine RPM
	* @param force Current acceleration force
	* @param clutch 
	* @param gear Current gear {-1 = reverse, 0 = neutral, 1...15 = forward}
	* @param running
	* @param contact
	* @param automode
	*/
	void netForceSettings(float rpm, float force, float clutch, int gear, bool running, bool contact, char automode);

	void setAcc(float val);
	void setAutoMode(int mode);
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
	* Controls vehicle starter. No side effects.
	* @param v 1 to run starter, 0 to stop it.
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
	bool hasContact() { return contact; };
	bool hasTurbo() { return hasturbo; };
	bool isRunning() { return running; };
	char getType() { return type; };
	float getAccToHoldRPM(float rpm);
	float getEnginePower(float rpm);
	float getEngineTorque() { return engineTorque; };
	float getIdleMixture();
	float getIdleRPM() { return idleRPM; };
	float getMaxRPM() { return maxRPM; };
	float getMinRPM() { return minRPM; };
	float getPrimeMixture();
	int getAutoShift();
	int getNumGears() { return gearsRatio.size() - 2; };
	int getNumGearsRanges() { return getNumGears() / 6 + 1; };
	TorqueCurve *getTorqueCurve() { return torqueCurve; };
	void autoSetAcc(float val);
	void autoShiftDown();
	void autoShiftSet(int mode);
	void autoShiftUp();
	void setManualClutch(float val);

	/**
	* Changes gear by a relative offset. Plays sounds.
	*/
	void shift(int val);
	
	/**
	* Changes gear to given value. Plays sounds.
	* @see BeamEngine::shift
	*/
	void shiftTo(int val);

	/**
	* Changes gears. Plays sounds.
	*/
	void updateShifts();

	void update(float dt, int doUpdate);

	/**
	* Updates sound effects (engine/turbo/clutch/etc...)
	*/
	void updateAudio(int doUpdate);

	enum shiftmodes {AUTOMATIC, SEMIAUTO, MANUAL, MANUAL_STICK, MANUAL_RANGES};
	enum autoswitch {REAR, NEUTRAL, DRIVE, TWO, ONE, MANUALMODE};

protected:

	// gear stuff
	float refWheelRevolutions; //!< Gears; estimated wheel revolutions based on current vehicle speed along the long axis
	float curWheelRevolutions; //!< Gears; measured wheel revolutions
	int curGear; //!< Gears; Current gear {-1 = reverse, 0 = neutral, 1...15 = forward} 
	int curGearRange; //!< Gears
	int numGears; //!< Gears
	std::vector<float> gearsRatio; //!< Gears

	// truck stuff
	float absVelocity; // Vehicle; current velocity of the vehicle
	float relVelocity; // Vehicle; current velocity of the vehicle along the long axis

	// clutch stuff
	float clutchForce; //!< Clutch attribute
	float clutchTime; //!< Clutch attribute
	float curClutch;
	float curClutchTorque;

	// engine stuff
	bool contact; //!< Engine
	bool hasair; //!< Engine attribute
	bool hasturbo; //!< Engine attribute
	bool running; //!< Engine state
	char type; //!< Engine attribute {'t' = truck (default), 'c' = car}
	float brakingTorque; //!< Engine
	float curAcc; //!< Engine
	float curEngineRPM; //!< Engine
	float diffRatio; //!< Engine
	float engineTorque; //!< Engine
	float hydropump; //!< Engine
	float idleRPM; //!< Engine attribute
	float minIdleMixture; //!< Engine attribute
	float maxIdleMixture; //!< Engine attribute
	float inertia; //!< Engine attribute
	float maxRPM; //!< Engine attribute
	float minRPM; //!< Engine attribute
	float stallRPM; //!< Engine
	int prime; //!< Engine

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
	std::deque<float> rpms;
	std::deque<float> accs;
	std::deque<float> brakes;

	// turbo
	float curTurboRPM;

	// air pressure
	TorqueCurve *torqueCurve;
	float apressure;
	int automode; //!< Transmission mode (@see enum BeamEngine::shiftmodes)

	int trucknum;

	bool is_Electric;
};

#endif // __BeamEngine_H_
