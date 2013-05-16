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

class BeamEngine : public ZeroedMemoryAllocator
{
	friend class Beam;

public:

	BeamEngine(float idleRPM, float maxRPM, float torque, std::vector<float> gears, float diff, int trucknum);
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

	void netForceSettings(float rpm, float force, float clutch, int gear, bool running, bool contact, char automode);

	void setAcc(float val);
	void setAutoMode(int mode);
	void setBrake(float val);
	void setClutch(float clutch);
	void setOptions(float einertia, char etype, float eclutch, float ctime, float stime, float pstime);
	void setRPM(float rpm);
	void setSpin(float rpm);

	void toggleAutoMode();
	void toggleContact();

	// quick start
	void offstart();
	void setstarter(int v);
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
	float getEngineTorque() { return engineTorque; };
	float getIdleRPM() { return idleRPM; };
	float getMaxRPM() { return maxRPM; };
	int getAutoShift();
	size_t getNumGears() { return gearsRatio.size() - 2; };
	size_t getNumGearsRanges() { return getNumGears() / 6 + 1; };
	TorqueCurve *getTorqueCurve() { return torqueCurve; };
	void autoSetAcc(float val);
	void autoShiftDown();
	void autoShiftSet(int mode);
	void autoShiftUp();
	void setManualClutch(float val);
	void shift(int val);
	void shiftTo(int val);
	void updateShifts();

	void update(float dt, int doUpdate);
	void updateAudio(int doUpdate);

	enum shiftmodes {AUTOMATIC, SEMIAUTO, MANUAL, MANUAL_STICK, MANUAL_RANGES};
	enum autoswitch {REAR, NEUTRAL, DRIVE, TWO, ONE, MANUALMODE};

protected:

	// gear stuff
	float curWheelRevolutions;
	int curGear;
	int curGearRange;
	int numGears;
	std::vector<float> gearsRatio;

	// clutch
	float clutchForce;
	float clutch_time;
	float curClutch;
	float curClutchTorque;

	// engine stuff
	bool contact;
	bool hasair;
	bool hasturbo;
	bool running;
	char type;
	float brakingTorque;
	float curAcc;
	float curBrake;
	float curEngineRPM;
	float engineTorque;
	float hydropump;
	float idleRPM;
	float inertia;
	float maxRPM;
	float stallRPM;
	int prime;

	// shifting
	float post_shift_time;
	float postshiftclock;
	float shift_time;
	float shiftclock;
	int postshifting;
	int shifting;
	int shiftval;

	// auto
	autoswitch autoselect;
	float autocurAcc;
	int starter;

	// turbo
	float curTurboRPM;

	// air pressure
	TorqueCurve *torqueCurve;
	float apressure;
	int automode;

	int trucknum;
};

#endif // __BeamEngine_H_
