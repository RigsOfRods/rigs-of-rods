/*
 This source file is part of Rigs of Rods
 Copyright 2005-2012 Pierre-Michel Ricordel
 Copyright 2007-2012 Thomas Fischer
 Copyright 2013-2023 Petr Ohlidal

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

/// @file

#include "AngelScriptBindings.h"
#include "EngineSim.h"

#include <angelscript.h>

using namespace AngelScript;

void RoR::RegisterEngineSim(asIScriptEngine* engine)
{
    int r;

    r = engine->RegisterEnum("autoswitch"); ROR_ASSERT(r >= 0);
    r = engine->RegisterEnumValue("autoswitch", "AUTOSWITCH_REAR", EngineSim::REAR); ROR_ASSERT(r >= 0);
    r = engine->RegisterEnumValue("autoswitch", "AUTOSWITCH_NEUTRAL", EngineSim::NEUTRAL); ROR_ASSERT(r >= 0);
    r = engine->RegisterEnumValue("autoswitch", "AUTOSWITCH_DRIVE", EngineSim::DRIVE); ROR_ASSERT(r >= 0);
    r = engine->RegisterEnumValue("autoswitch", "AUTOSWITCH_TWO", EngineSim::TWO); ROR_ASSERT(r >= 0);
    r = engine->RegisterEnumValue("autoswitch", "AUTOSWITCH_ONE", EngineSim::ONE); ROR_ASSERT(r >= 0);
    r = engine->RegisterEnumValue("autoswitch", "AUTOSWITCH_MANUALMODE", EngineSim::MANUALMODE); ROR_ASSERT(r >= 0);

    r = engine->RegisterEnum("SimGearboxMode"); ROR_ASSERT(r >= 0);
    r = engine->RegisterEnumValue("SimGearboxMode", "AUTO", static_cast<int>(SimGearboxMode::AUTO)); ROR_ASSERT(r >= 0);
    r = engine->RegisterEnumValue("SimGearboxMode", "SEMI_AUTO", static_cast<int>(SimGearboxMode::SEMI_AUTO)); ROR_ASSERT(r >= 0);
    r = engine->RegisterEnumValue("SimGearboxMode", "MANUAL", static_cast<int>(SimGearboxMode::MANUAL)); ROR_ASSERT(r >= 0);
    r = engine->RegisterEnumValue("SimGearboxMode", "MANUAL_STICK", static_cast<int>(SimGearboxMode::MANUAL_STICK)); ROR_ASSERT(r >= 0);
    r = engine->RegisterEnumValue("SimGearboxMode", "MANUAL_RANGES", static_cast<int>(SimGearboxMode::MANUAL_RANGES)); ROR_ASSERT(r >= 0);

    // class EngineSim
    EngineSim::RegisterRefCountingObject(engine, "EngineSimClass");
    EngineSimPtr::RegisterRefCountingObjectPtr(engine, "EngineSimClassPtr", "EngineSimClass");

    //  > Definition, Keyword 'engine'
    r = engine->RegisterObjectMethod("EngineSimClass", "float getMinRPM() const", asMETHOD(EngineSim, getMinRPM), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineSimClass", "float getMaxRPM() const", asMETHOD(EngineSim, getMaxRPM), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineSimClass", "float getEngineTorque() const", asMETHOD(EngineSim, getEngineTorque), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineSimClass", "float getDiffRatio() const", asMETHOD(EngineSim, getDiffRatio), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineSimClass", "float getGearRatio(int) const", asMETHODPR(EngineSim, getGearRatio, (int), float), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineSimClass", "int getNumGears() const", asMETHOD(EngineSim, getNumGears), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineSimClass", "int getNumGearsRanges() const", asMETHOD(EngineSim, getNumGearsRanges), asCALL_THISCALL); ROR_ASSERT(r >= 0);

    //  > Definition, Keyword 'engoption'
    r = engine->RegisterObjectMethod("EngineSimClass", "float getEngineInertia() const", asMETHOD(EngineSim, getEngineInertia), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineSimClass", "uint8 getEngineType() const", asMETHOD(EngineSim, getEngineType), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineSimClass", "bool isElectric() const", asMETHOD(EngineSim, isElectric), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineSimClass", "bool hasAir() const", asMETHOD(EngineSim, hasAir), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineSimClass", "bool hasTurbo() const", asMETHOD(EngineSim, hasTurbo), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineSimClass", "float getClutchForce() const", asMETHOD(EngineSim, getClutchForce), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineSimClass", "float getShiftTime() const", asMETHOD(EngineSim, getShiftTime), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineSimClass", "float getClutchTime() const", asMETHOD(EngineSim, getClutchTime), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineSimClass", "float getPostShiftTime() const", asMETHOD(EngineSim, getPostShiftTime), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineSimClass", "float getStallRPM() const", asMETHOD(EngineSim, getStallRPM), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineSimClass", "float getIdleRPM() const", asMETHOD(EngineSim, getIdleRPM), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineSimClass", "float getMaxIdleMixture() const", asMETHOD(EngineSim, getMaxIdleMixture), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineSimClass", "float getMinIdleMixture() const", asMETHOD(EngineSim, getMinIdleMixture), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineSimClass", "float getBrakingTorque() const", asMETHOD(EngineSim, getBrakingTorque), asCALL_THISCALL); ROR_ASSERT(r >= 0);
 
    //  > General state getters
    r = engine->RegisterObjectMethod("EngineSimClass", "float getAcc()", asMETHOD(EngineSim, getAcc), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineSimClass", "float getClutch()", asMETHOD(EngineSim, getClutch), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineSimClass", "float getCrankFactor()", asMETHOD(EngineSim, getCrankFactor), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineSimClass", "float getRPM()", asMETHOD(EngineSim, getRPM), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineSimClass", "float getSmoke()", asMETHOD(EngineSim, getSmoke), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineSimClass", "float getTorque()", asMETHOD(EngineSim, getTorque), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineSimClass", "float getTurboPSI()", asMETHOD(EngineSim, getTurboPSI), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineSimClass", "SimGearboxMode getAutoMode()", asMETHOD(EngineSim, getAutoMode), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineSimClass", "int getGear()", asMETHOD(EngineSim, getGear), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineSimClass", "int getGearRange()", asMETHOD(EngineSim, getGearRange), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineSimClass", "bool isRunning()", asMETHOD(EngineSim, isRunning), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineSimClass", "bool hasContact()", asMETHOD(EngineSim, hasContact), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineSimClass", "float getCurEngineTorque()", asMETHOD(EngineSim, getCurEngineTorque), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineSimClass", "float getInputShaftRPM()", asMETHOD(EngineSim, getInputShaftRPM), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineSimClass", "float getDriveRatio()", asMETHOD(EngineSim, getDriveRatio), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineSimClass", "float getEnginePower()", asMETHODPR(EngineSim, getEnginePower, (), float), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineSimClass", "float getEnginePower(float)", asMETHODPR(EngineSim, getEnginePower, (float), float), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineSimClass", "float getTurboPower()", asMETHOD(EngineSim, getTurboPower), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineSimClass", "float getIdleMixture()", asMETHOD(EngineSim, getIdleMixture), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineSimClass", "float getPrimeMixture()", asMETHOD(EngineSim, getPrimeMixture), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineSimClass", "autoswitch getAutoShift()", asMETHOD(EngineSim, getAutoShift), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineSimClass", "float getAccToHoldRPM()", asMETHOD(EngineSim, getAccToHoldRPM), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineSimClass", "float getWheelSpin()", asMETHOD(EngineSim, getWheelSpin), asCALL_THISCALL); ROR_ASSERT(r >= 0);
 
    //  > shifting diagnostic
    r = engine->RegisterObjectMethod("EngineSimClass", "float getPostShiftClock()", asMETHOD(EngineSim, getPostShiftClock), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineSimClass", "float getShiftClock()", asMETHOD(EngineSim, getShiftClock), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineSimClass", "bool isPostShifting()", asMETHOD(EngineSim, isPostShifting), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineSimClass", "bool isShifting()", asMETHOD(EngineSim, isShifting), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineSimClass", "int getShifTargetGear()", asMETHOD(EngineSim, getShifTargetGear), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineSimClass", "float getAutoShiftBehavior()", asMETHOD(EngineSim, getAutoShiftBehavior), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineSimClass", "int getUpshiftDelayCounter()", asMETHOD(EngineSim, getUpshiftDelayCounter), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineSimClass", "int getKickdownDelayCounter()", asMETHOD(EngineSim, getKickdownDelayCounter), asCALL_THISCALL); ROR_ASSERT(r >= 0);
 
    //  > State setters
    r = engine->RegisterObjectMethod("EngineSimClass", "void setAcc(float val)", asMETHOD(EngineSim, setAcc), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineSimClass", "void autoSetAcc(float val)", asMETHOD(EngineSim, autoSetAcc), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineSimClass", "void setClutch(float clutch)", asMETHOD(EngineSim, setClutch), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineSimClass", "void setRPM(float rpm)", asMETHOD(EngineSim, setRPM), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineSimClass", "void setWheelSpin(float rpm)", asMETHOD(EngineSim, setWheelSpin), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineSimClass", "void setAutoMode(SimGearboxMode mode)", asMETHOD(EngineSim, setAutoMode), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineSimClass", "void setPrime(bool p)", asMETHOD(EngineSim, setPrime), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineSimClass", "void setHydroPump(float work)", asMETHOD(EngineSim, setHydroPump), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineSimClass", "void setManualClutch(float val)", asMETHOD(EngineSim, setManualClutch), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineSimClass", "void setTCaseRatio(float ratio)", asMETHOD(EngineSim, setTCaseRatio), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineSimClass", "void toggleContact()", asMETHOD(EngineSim, toggleContact), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineSimClass", "void offStart()", asMETHOD(EngineSim, offStart), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineSimClass", "void startEngine()", asMETHOD(EngineSim, startEngine), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineSimClass", "void stopEngine()", asMETHOD(EngineSim, stopEngine), asCALL_THISCALL); ROR_ASSERT(r >= 0);

    //  > Shifting
    r = engine->RegisterObjectMethod("EngineSimClass", "void toggleAutoMode()", asMETHOD(EngineSim, toggleAutoMode), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineSimClass", "void autoShiftDown()", asMETHOD(EngineSim, autoShiftDown), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineSimClass", "void autoShiftSet(autoswitch mode)", asMETHOD(EngineSim, autoShiftSet), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineSimClass", "void autoShiftUp()", asMETHOD(EngineSim, autoShiftUp), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineSimClass", "void setGear(int v)", asMETHOD(EngineSim, setGear), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineSimClass", "void setGearRange(int v)", asMETHOD(EngineSim, setGearRange), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineSimClass", "void shift(int val)", asMETHOD(EngineSim, shift), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineSimClass", "void shiftTo(int val)", asMETHOD(EngineSim, shiftTo), asCALL_THISCALL); ROR_ASSERT(r >= 0);
}
