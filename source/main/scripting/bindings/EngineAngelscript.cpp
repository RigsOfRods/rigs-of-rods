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
#include "Engine.h"

#include <angelscript.h>

using namespace AngelScript;

void RoR::RegisterEngine(asIScriptEngine* engine)
{
    int r;

    r = engine->RegisterEnum("autoswitch"); ROR_ASSERT(r >= 0);
    r = engine->RegisterEnumValue("autoswitch", "AUTOSWITCH_REAR", Engine::REAR); ROR_ASSERT(r >= 0);
    r = engine->RegisterEnumValue("autoswitch", "AUTOSWITCH_NEUTRAL", Engine::NEUTRAL); ROR_ASSERT(r >= 0);
    r = engine->RegisterEnumValue("autoswitch", "AUTOSWITCH_DRIVE", Engine::DRIVE); ROR_ASSERT(r >= 0);
    r = engine->RegisterEnumValue("autoswitch", "AUTOSWITCH_TWO", Engine::TWO); ROR_ASSERT(r >= 0);
    r = engine->RegisterEnumValue("autoswitch", "AUTOSWITCH_ONE", Engine::ONE); ROR_ASSERT(r >= 0);
    r = engine->RegisterEnumValue("autoswitch", "AUTOSWITCH_MANUALMODE", Engine::MANUALMODE); ROR_ASSERT(r >= 0);

    r = engine->RegisterEnum("SimGearboxMode"); ROR_ASSERT(r >= 0);
    r = engine->RegisterEnumValue("SimGearboxMode", "AUTO", static_cast<int>(SimGearboxMode::AUTO)); ROR_ASSERT(r >= 0);
    r = engine->RegisterEnumValue("SimGearboxMode", "SEMI_AUTO", static_cast<int>(SimGearboxMode::SEMI_AUTO)); ROR_ASSERT(r >= 0);
    r = engine->RegisterEnumValue("SimGearboxMode", "MANUAL", static_cast<int>(SimGearboxMode::MANUAL)); ROR_ASSERT(r >= 0);
    r = engine->RegisterEnumValue("SimGearboxMode", "MANUAL_STICK", static_cast<int>(SimGearboxMode::MANUAL_STICK)); ROR_ASSERT(r >= 0);
    r = engine->RegisterEnumValue("SimGearboxMode", "MANUAL_RANGES", static_cast<int>(SimGearboxMode::MANUAL_RANGES)); ROR_ASSERT(r >= 0);

    // class Engine
    Engine::RegisterRefCountingObject(engine, "EngineClass");
    EnginePtr::RegisterRefCountingObjectPtr(engine, "EngineClassPtr", "EngineClass");

    //  > Definition, Keyword 'engine'
    r = engine->RegisterObjectMethod("EngineClass", "float getMinRPM() const", asMETHOD(Engine, getMinRPM), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineClass", "float getMaxRPM() const", asMETHOD(Engine, getMaxRPM), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineClass", "float getEngineTorque() const", asMETHOD(Engine, getEngineTorque), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineClass", "float getDiffRatio() const", asMETHOD(Engine, getDiffRatio), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineClass", "float getGearRatio(int) const", asMETHODPR(Engine, getGearRatio, (int), float), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineClass", "int getNumGears() const", asMETHOD(Engine, getNumGears), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineClass", "int getNumGearsRanges() const", asMETHOD(Engine, getNumGearsRanges), asCALL_THISCALL); ROR_ASSERT(r >= 0);

    //  > Definition, Keyword 'engoption'
    r = engine->RegisterObjectMethod("EngineClass", "float getEngineInertia() const", asMETHOD(Engine, getEngineInertia), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineClass", "uint8 getEngineType() const", asMETHOD(Engine, getEngineType), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineClass", "bool isElectric() const", asMETHOD(Engine, isElectric), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineClass", "bool hasAir() const", asMETHOD(Engine, hasAir), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineClass", "bool hasTurbo() const", asMETHOD(Engine, hasTurbo), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineClass", "float getClutchForce() const", asMETHOD(Engine, getClutchForce), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineClass", "float getShiftTime() const", asMETHOD(Engine, getShiftTime), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineClass", "float getClutchTime() const", asMETHOD(Engine, getClutchTime), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineClass", "float getPostShiftTime() const", asMETHOD(Engine, getPostShiftTime), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineClass", "float getStallRPM() const", asMETHOD(Engine, getStallRPM), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineClass", "float getIdleRPM() const", asMETHOD(Engine, getIdleRPM), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineClass", "float getMaxIdleMixture() const", asMETHOD(Engine, getMaxIdleMixture), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineClass", "float getMinIdleMixture() const", asMETHOD(Engine, getMinIdleMixture), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineClass", "float getBrakingTorque() const", asMETHOD(Engine, getBrakingTorque), asCALL_THISCALL); ROR_ASSERT(r >= 0);
 
    //  > General state getters
    r = engine->RegisterObjectMethod("EngineClass", "float getAcc()", asMETHOD(Engine, getAcc), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineClass", "float getClutch()", asMETHOD(Engine, getClutch), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineClass", "float getCrankFactor()", asMETHOD(Engine, getCrankFactor), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineClass", "float getRPM()", asMETHOD(Engine, getRPM), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineClass", "float getSmoke()", asMETHOD(Engine, getSmoke), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineClass", "float getTorque()", asMETHOD(Engine, getTorque), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineClass", "float getTurboPSI()", asMETHOD(Engine, getTurboPSI), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineClass", "SimGearboxMode getAutoMode()", asMETHOD(Engine, getAutoMode), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineClass", "int getGear()", asMETHOD(Engine, getGear), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineClass", "int getGearRange()", asMETHOD(Engine, getGearRange), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineClass", "bool isRunning()", asMETHOD(Engine, isRunning), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineClass", "bool hasContact()", asMETHOD(Engine, hasContact), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineClass", "float getCurEngineTorque()", asMETHOD(Engine, getCurEngineTorque), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineClass", "float getInputShaftRPM()", asMETHOD(Engine, getInputShaftRPM), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineClass", "float getDriveRatio()", asMETHOD(Engine, getDriveRatio), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineClass", "float getEnginePower()", asMETHODPR(Engine, getEnginePower, (), float), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineClass", "float getEnginePower(float)", asMETHODPR(Engine, getEnginePower, (float), float), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineClass", "float getTurboPower()", asMETHOD(Engine, getTurboPower), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineClass", "float getIdleMixture()", asMETHOD(Engine, getIdleMixture), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineClass", "float getPrimeMixture()", asMETHOD(Engine, getPrimeMixture), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineClass", "autoswitch getAutoShift()", asMETHOD(Engine, getAutoShift), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineClass", "float getAccToHoldRPM()", asMETHOD(Engine, getAccToHoldRPM), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineClass", "float getWheelSpin()", asMETHOD(Engine, getWheelSpin), asCALL_THISCALL); ROR_ASSERT(r >= 0);
 
    //  > shifting diagnostic
    r = engine->RegisterObjectMethod("EngineClass", "float getPostShiftClock()", asMETHOD(Engine, getPostShiftClock), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineClass", "float getShiftClock()", asMETHOD(Engine, getShiftClock), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineClass", "bool isPostShifting()", asMETHOD(Engine, isPostShifting), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineClass", "bool isShifting()", asMETHOD(Engine, isShifting), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineClass", "int getShifTargetGear()", asMETHOD(Engine, getShifTargetGear), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineClass", "float getAutoShiftBehavior()", asMETHOD(Engine, getAutoShiftBehavior), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineClass", "int getUpshiftDelayCounter()", asMETHOD(Engine, getUpshiftDelayCounter), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineClass", "int getKickdownDelayCounter()", asMETHOD(Engine, getKickdownDelayCounter), asCALL_THISCALL); ROR_ASSERT(r >= 0);
 
    //  > State setters
    r = engine->RegisterObjectMethod("EngineClass", "void setAcc(float val)", asMETHOD(Engine, setAcc), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineClass", "void autoSetAcc(float val)", asMETHOD(Engine, autoSetAcc), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineClass", "void setClutch(float clutch)", asMETHOD(Engine, setClutch), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineClass", "void setRPM(float rpm)", asMETHOD(Engine, setRPM), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineClass", "void setWheelSpin(float rpm)", asMETHOD(Engine, setWheelSpin), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineClass", "void setAutoMode(SimGearboxMode mode)", asMETHOD(Engine, setAutoMode), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineClass", "void setPrime(bool p)", asMETHOD(Engine, setPrime), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineClass", "void setHydroPump(float work)", asMETHOD(Engine, setHydroPump), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineClass", "void setManualClutch(float val)", asMETHOD(Engine, setManualClutch), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineClass", "void setTCaseRatio(float ratio)", asMETHOD(Engine, setTCaseRatio), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineClass", "void toggleContact()", asMETHOD(Engine, toggleContact), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineClass", "void offStart()", asMETHOD(Engine, offStart), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineClass", "void startEngine()", asMETHOD(Engine, startEngine), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineClass", "void stopEngine()", asMETHOD(Engine, stopEngine), asCALL_THISCALL); ROR_ASSERT(r >= 0);

    //  > Shifting
    r = engine->RegisterObjectMethod("EngineClass", "void toggleAutoMode()", asMETHOD(Engine, toggleAutoMode), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineClass", "void autoShiftDown()", asMETHOD(Engine, autoShiftDown), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineClass", "void autoShiftSet(autoswitch mode)", asMETHOD(Engine, autoShiftSet), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineClass", "void autoShiftUp()", asMETHOD(Engine, autoShiftUp), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineClass", "void setGear(int v)", asMETHOD(Engine, setGear), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineClass", "void setGearRange(int v)", asMETHOD(Engine, setGearRange), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineClass", "void shift(int val)", asMETHOD(Engine, shift), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineClass", "void shiftTo(int val)", asMETHOD(Engine, shiftTo), asCALL_THISCALL); ROR_ASSERT(r >= 0);
}
