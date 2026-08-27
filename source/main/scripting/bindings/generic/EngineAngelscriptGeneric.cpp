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
#include "../../autowrapper/aswrappedcall.h"

#include <angelscript.h>

using namespace AngelScript;

void RoR::RegisterEngineGeneric(asIScriptEngine* engine)
{
    int r;

    // class Engine
    Engine::RegisterRefCountingObject(engine, "EngineClass");
    EnginePtr::RegisterRefCountingObjectPtr(engine, "EngineClassPtr", "EngineClass");

    //  > Definition, Keyword 'engine'
    r = engine->RegisterObjectMethod("EngineClass", "float getShiftDownRPM() const", WRAP_MFN(Engine, getShiftDownRPM), asCALL_GENERIC); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineClass", "float getShiftUpRPM() const", WRAP_MFN(Engine, getShiftUpRPM), asCALL_GENERIC); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineClass", "float getEngineTorque() const", WRAP_MFN(Engine, getEngineTorque), asCALL_GENERIC); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineClass", "float getDiffRatio() const", WRAP_MFN(Engine, getDiffRatio), asCALL_GENERIC); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineClass", "float getGearRatio(int) const", WRAP_MFN_PR(Engine, getGearRatio, (int), float), asCALL_GENERIC); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineClass", "int getNumGears() const", WRAP_MFN(Engine, getNumGears), asCALL_GENERIC); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineClass", "int getNumGearsRanges() const", WRAP_MFN(Engine, getNumGearsRanges), asCALL_GENERIC); ROR_ASSERT(r >= 0);

    //  > Definition, Keyword 'engoption'
    r = engine->RegisterObjectMethod("EngineClass", "float getEngineInertia() const", WRAP_MFN(Engine, getEngineInertia), asCALL_GENERIC); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineClass", "uint8 getEngineType() const", WRAP_MFN(Engine, getEngineType), asCALL_GENERIC); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineClass", "bool isElectric() const", WRAP_MFN(Engine, isElectric), asCALL_GENERIC); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineClass", "bool hasAir() const", WRAP_MFN(Engine, hasAir), asCALL_GENERIC); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineClass", "bool hasTurbo() const", WRAP_MFN(Engine, hasTurbo), asCALL_GENERIC); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineClass", "float getClutchForce() const", WRAP_MFN(Engine, getClutchForce), asCALL_GENERIC); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineClass", "float getShiftTime() const", WRAP_MFN(Engine, getShiftTime), asCALL_GENERIC); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineClass", "float getClutchTime() const", WRAP_MFN(Engine, getClutchTime), asCALL_GENERIC); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineClass", "float getPostShiftTime() const", WRAP_MFN(Engine, getPostShiftTime), asCALL_GENERIC); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineClass", "float getStallRPM() const", WRAP_MFN(Engine, getStallRPM), asCALL_GENERIC); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineClass", "float getIdleRPM() const", WRAP_MFN(Engine, getIdleRPM), asCALL_GENERIC); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineClass", "float getMaxIdleMixture() const", WRAP_MFN(Engine, getMaxIdleMixture), asCALL_GENERIC); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineClass", "float getMinIdleMixture() const", WRAP_MFN(Engine, getMinIdleMixture), asCALL_GENERIC); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineClass", "float getBrakingTorque() const", WRAP_MFN(Engine, getBrakingTorque), asCALL_GENERIC); ROR_ASSERT(r >= 0);
 
    //  > General state getters
    r = engine->RegisterObjectMethod("EngineClass", "float getAcc()", WRAP_MFN(Engine, getAcc), asCALL_GENERIC); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineClass", "float getClutch()", WRAP_MFN(Engine, getClutch), asCALL_GENERIC); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineClass", "float getCrankFactor()", WRAP_MFN(Engine, getCrankFactor), asCALL_GENERIC); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineClass", "float getRPM()", WRAP_MFN(Engine, getRPM), asCALL_GENERIC); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineClass", "float getSmoke()", WRAP_MFN(Engine, getSmoke), asCALL_GENERIC); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineClass", "float getTorque()", WRAP_MFN(Engine, getTorque), asCALL_GENERIC); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineClass", "float getTurboPSI()", WRAP_MFN(Engine, getTurboPSI), asCALL_GENERIC); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineClass", "SimGearboxMode getAutoMode()", WRAP_MFN(Engine, getAutoMode), asCALL_GENERIC); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineClass", "int getGear()", WRAP_MFN(Engine, getGear), asCALL_GENERIC); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineClass", "int getGearRange()", WRAP_MFN(Engine, getGearRange), asCALL_GENERIC); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineClass", "bool isRunning()", WRAP_MFN(Engine, isRunning), asCALL_GENERIC); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineClass", "bool hasContact()", WRAP_MFN(Engine, hasContact), asCALL_GENERIC); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineClass", "float getCurEngineTorque()", WRAP_MFN(Engine, getCurEngineTorque), asCALL_GENERIC); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineClass", "float getInputShaftRPM()", WRAP_MFN(Engine, getInputShaftRPM), asCALL_GENERIC); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineClass", "float getDriveRatio()", WRAP_MFN(Engine, getDriveRatio), asCALL_GENERIC); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineClass", "float getEnginePower()", WRAP_MFN_PR(Engine, getEnginePower, (), float), asCALL_GENERIC); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineClass", "float getEnginePower(float)", WRAP_MFN_PR(Engine, getEnginePower, (float), float), asCALL_GENERIC); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineClass", "float getTurboPower()", WRAP_MFN(Engine, getTurboPower), asCALL_GENERIC); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineClass", "float getIdleMixture()", WRAP_MFN(Engine, getIdleMixture), asCALL_GENERIC); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineClass", "float getPrimeMixture()", WRAP_MFN(Engine, getPrimeMixture), asCALL_GENERIC); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineClass", "autoswitch getAutoShift()", WRAP_MFN(Engine, getAutoShift), asCALL_GENERIC); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineClass", "float getAccToHoldRPM()", WRAP_MFN(Engine, getAccToHoldRPM), asCALL_GENERIC); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineClass", "float getWheelSpin()", WRAP_MFN(Engine, getWheelSpin), asCALL_GENERIC); ROR_ASSERT(r >= 0);
 
    //  > shifting diagnostic
    r = engine->RegisterObjectMethod("EngineClass", "float getPostShiftClock()", WRAP_MFN(Engine, getPostShiftClock), asCALL_GENERIC); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineClass", "float getShiftClock()", WRAP_MFN(Engine, getShiftClock), asCALL_GENERIC); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineClass", "bool isPostShifting()", WRAP_MFN(Engine, isPostShifting), asCALL_GENERIC); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineClass", "bool isShifting()", WRAP_MFN(Engine, isShifting), asCALL_GENERIC); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineClass", "int getShifTargetGear()", WRAP_MFN(Engine, getShifTargetGear), asCALL_GENERIC); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineClass", "float getAutoShiftBehavior()", WRAP_MFN(Engine, getAutoShiftBehavior), asCALL_GENERIC); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineClass", "int getUpshiftDelayCounter()", WRAP_MFN(Engine, getUpshiftDelayCounter), asCALL_GENERIC); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineClass", "int getKickdownDelayCounter()", WRAP_MFN(Engine, getKickdownDelayCounter), asCALL_GENERIC); ROR_ASSERT(r >= 0);
 
    //  > State setters
    r = engine->RegisterObjectMethod("EngineClass", "void setAcc(float val)", WRAP_MFN(Engine, setAcc), asCALL_GENERIC); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineClass", "void autoSetAcc(float val)", WRAP_MFN(Engine, autoSetAcc), asCALL_GENERIC); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineClass", "void setClutch(float clutch)", WRAP_MFN(Engine, setClutch), asCALL_GENERIC); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineClass", "void setRPM(float rpm)", WRAP_MFN(Engine, setRPM), asCALL_GENERIC); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineClass", "void setWheelSpin(float rpm)", WRAP_MFN(Engine, setWheelSpin), asCALL_GENERIC); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineClass", "void setAutoMode(SimGearboxMode mode)", WRAP_MFN(Engine, setAutoMode), asCALL_GENERIC); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineClass", "void setPrime(bool p)", WRAP_MFN(Engine, setPrime), asCALL_GENERIC); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineClass", "void setHydroPump(float work)", WRAP_MFN(Engine, setHydroPump), asCALL_GENERIC); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineClass", "void setManualClutch(float val)", WRAP_MFN(Engine, setManualClutch), asCALL_GENERIC); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineClass", "void setTCaseRatio(float ratio)", WRAP_MFN(Engine, setTCaseRatio), asCALL_GENERIC); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineClass", "void toggleContact()", WRAP_MFN(Engine, toggleContact), asCALL_GENERIC); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineClass", "void offStart()", WRAP_MFN(Engine, offStart), asCALL_GENERIC); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineClass", "void startEngine()", WRAP_MFN(Engine, startEngine), asCALL_GENERIC); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineClass", "void stopEngine()", WRAP_MFN(Engine, stopEngine), asCALL_GENERIC); ROR_ASSERT(r >= 0);

    //  > Shifting
    r = engine->RegisterObjectMethod("EngineClass", "void toggleAutoMode()", WRAP_MFN(Engine, toggleAutoMode), asCALL_GENERIC); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineClass", "void autoShiftDown()", WRAP_MFN(Engine, autoShiftDown), asCALL_GENERIC); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineClass", "void autoShiftSet(autoswitch mode)", WRAP_MFN(Engine, autoShiftSet), asCALL_GENERIC); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineClass", "void autoShiftUp()", WRAP_MFN(Engine, autoShiftUp), asCALL_GENERIC); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineClass", "void setGear(int v)", WRAP_MFN(Engine, setGear), asCALL_GENERIC); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineClass", "void setGearRange(int v)", WRAP_MFN(Engine, setGearRange), asCALL_GENERIC); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineClass", "void shift(int val)", WRAP_MFN(Engine, shift), asCALL_GENERIC); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectMethod("EngineClass", "void shiftTo(int val)", WRAP_MFN(Engine, shiftTo), asCALL_GENERIC); ROR_ASSERT(r >= 0);
}
