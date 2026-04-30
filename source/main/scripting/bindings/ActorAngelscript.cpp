/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2013-2022 Petr Ohlidal

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

#include "Actor.h"
#include "AngelScriptBindings.h"
#include "ScriptEngine.h"
#include "ScriptUtils.h"
#include "SimData.h"
#include <angelscript.h>

using namespace AngelScript;

void RoR::RegisterActor(asIScriptEngine *engine)
{
    int result;
    
    // enum truckStates
    result = engine->RegisterEnum("TruckState"); ROR_ASSERT(result>=0);
    result = engine->RegisterEnumValue("TruckState", "TS_SIMULATED", static_cast<int>(ActorState::LOCAL_SIMULATED)); ROR_ASSERT(result>=0);
    result = engine->RegisterEnumValue("TruckState", "TS_SLEEPING",  static_cast<int>(ActorState::LOCAL_SLEEPING)); ROR_ASSERT(result>=0);
    result = engine->RegisterEnumValue("TruckState", "TS_NETWORKED", static_cast<int>(ActorState::NETWORKED_OK)); ROR_ASSERT(result>=0);

    // enum truckTypes
    result = engine->RegisterEnum("truckTypes"); ROR_ASSERT(result>=0);
    result = engine->RegisterEnumValue("truckTypes", "TT_NOT_DRIVEABLE", NOT_DRIVEABLE); ROR_ASSERT(result>=0);
    result = engine->RegisterEnumValue("truckTypes", "TT_TRUCK", TRUCK); ROR_ASSERT(result>=0);
    result = engine->RegisterEnumValue("truckTypes", "TT_AIRPLANE", AIRPLANE); ROR_ASSERT(result>=0);
    result = engine->RegisterEnumValue("truckTypes", "TT_BOAT", BOAT); ROR_ASSERT(result>=0);
    result = engine->RegisterEnumValue("truckTypes", "TT_MACHINE", MACHINE); ROR_ASSERT(result>=0);
    result = engine->RegisterEnumValue("truckTypes", "TT_AI", AI); ROR_ASSERT(result>=0);

    // enum FlareType
    result = engine->RegisterEnum("FlareType"); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("FlareType", "FLARE_TYPE_NONE", (int)FlareType::NONE); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("FlareType", "FLARE_TYPE_HEADLIGHT", (int)FlareType::HEADLIGHT); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("FlareType", "FLARE_TYPE_HIGH_BEAM", (int)FlareType::HIGH_BEAM); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("FlareType", "FLARE_TYPE_FOG_LIGHT", (int)FlareType::FOG_LIGHT); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("FlareType", "FLARE_TYPE_TAIL_LIGHT", (int)FlareType::TAIL_LIGHT); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("FlareType", "FLARE_TYPE_BRAKE_LIGHT", (int)FlareType::BRAKE_LIGHT); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("FlareType", "FLARE_TYPE_REVERSE_LIGHT", (int)FlareType::REVERSE_LIGHT); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("FlareType", "FLARE_TYPE_SIDELIGHT", (int)FlareType::SIDELIGHT); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("FlareType", "FLARE_TYPE_BLINKER_LEFT", (int)FlareType::BLINKER_LEFT); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("FlareType", "FLARE_TYPE_BLINKER_RIGHT", (int)FlareType::BLINKER_RIGHT); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("FlareType", "FLARE_TYPE_USER", (int)FlareType::USER); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("FlareType", "FLARE_TYPE_DASHBOARD", (int)FlareType::DASHBOARD); ROR_ASSERT(result >= 0);

    // enum BlinkType
    result = engine->RegisterEnum("BlinkType"); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("BlinkType", "BLINK_NONE", (int)BLINK_NONE); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("BlinkType", "BLINK_LEFT", (int)BLINK_LEFT); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("BlinkType", "BLINK_RIGHT", (int)BLINK_RIGHT); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("BlinkType", "BLINK_WARN", (int)BLINK_WARN); ROR_ASSERT(result >= 0);

    // enum ActorModifyRequestType
    result = engine->RegisterEnum("ActorModifyRequestType"); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("ActorModifyRequestType", "ACTOR_MODIFY_REQUEST_INVALID", (int)ActorModifyRequest::Type::INVALID); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("ActorModifyRequestType", "ACTOR_MODIFY_REQUEST_RELOAD", (int)ActorModifyRequest::Type::RELOAD); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("ActorModifyRequestType", "ACTOR_MODIFY_REQUEST_RESET_ON_INIT_POS", (int)ActorModifyRequest::Type::RESET_ON_INIT_POS); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("ActorModifyRequestType", "ACTOR_MODIFY_REQUEST_RESET_ON_SPOT", (int)ActorModifyRequest::Type::RESET_ON_SPOT); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("ActorModifyRequestType", "ACTOR_MODIFY_REQUEST_SOFT_RESET", (int)ActorModifyRequest::Type::SOFT_RESET); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("ActorModifyRequestType", "ACTOR_MODIFY_REQUEST_RESTORE_SAVED", (int)ActorModifyRequest::Type::RESTORE_SAVED); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("ActorModifyRequestType", "ACTOR_MODIFY_REQUEST_WAKE_UP", (int)ActorModifyRequest::Type::WAKE_UP); ROR_ASSERT(result >= 0);

    // enum ActorSimAttr
    result = engine->RegisterEnum("ActorSimAttr"); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("ActorSimAttr", "ACTORSIMATTR_NONE", (int)ACTORSIMATTR_NONE); ROR_ASSERT(result >= 0);
        // ... TractionControl
    result = engine->RegisterEnumValue("ActorSimAttr", "ACTORSIMATTR_TC_RATIO", (int)ACTORSIMATTR_TC_RATIO); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("ActorSimAttr", "ACTORSIMATTR_TC_PULSE_TIME", (int)ACTORSIMATTR_TC_PULSE_TIME); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("ActorSimAttr", "ACTORSIMATTR_TC_WHEELSLIP_CONSTANT", (int)ACTORSIMATTR_TC_WHEELSLIP_CONSTANT); ROR_ASSERT(result >= 0);
        // ... Engine
    result = engine->RegisterEnumValue("ActorSimAttr", "ACTORSIMATTR_ENGINE_SHIFTDOWN_RPM", (int)ACTORSIMATTR_ENGINE_SHIFTDOWN_RPM); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("ActorSimAttr", "ACTORSIMATTR_ENGINE_SHIFTUP_RPM", (int)ACTORSIMATTR_ENGINE_SHIFTUP_RPM); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("ActorSimAttr", "ACTORSIMATTR_ENGINE_TORQUE", (int)ACTORSIMATTR_ENGINE_TORQUE); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("ActorSimAttr", "ACTORSIMATTR_ENGINE_DIFF_RATIO", (int)ACTORSIMATTR_ENGINE_DIFF_RATIO); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("ActorSimAttr", "ACTORSIMATTR_ENGINE_GEAR_RATIOS_ARRAY", (int)ACTORSIMATTR_ENGINE_GEAR_RATIOS_ARRAY); ROR_ASSERT(result >= 0);
        // ... Engoption 
    result = engine->RegisterEnumValue("ActorSimAttr", "ACTORSIMATTR_ENGOPTION_ENGINE_INERTIA", (int)ACTORSIMATTR_ENGOPTION_ENGINE_INERTIA); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("ActorSimAttr", "ACTORSIMATTR_ENGOPTION_ENGINE_TYPE", (int)ACTORSIMATTR_ENGOPTION_ENGINE_TYPE); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("ActorSimAttr", "ACTORSIMATTR_ENGOPTION_CLUTCH_FORCE", (int)ACTORSIMATTR_ENGOPTION_CLUTCH_FORCE); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("ActorSimAttr", "ACTORSIMATTR_ENGOPTION_SHIFT_TIME", (int)ACTORSIMATTR_ENGOPTION_SHIFT_TIME); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("ActorSimAttr", "ACTORSIMATTR_ENGOPTION_CLUTCH_TIME", (int)ACTORSIMATTR_ENGOPTION_CLUTCH_TIME); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("ActorSimAttr", "ACTORSIMATTR_ENGOPTION_POST_SHIFT_TIME", (int)ACTORSIMATTR_ENGOPTION_POST_SHIFT_TIME); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("ActorSimAttr", "ACTORSIMATTR_ENGOPTION_STALL_RPM", (int)ACTORSIMATTR_ENGOPTION_STALL_RPM); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("ActorSimAttr", "ACTORSIMATTR_ENGOPTION_IDLE_RPM", (int)ACTORSIMATTR_ENGOPTION_IDLE_RPM); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("ActorSimAttr", "ACTORSIMATTR_ENGOPTION_MAX_IDLE_MIXTURE", (int)ACTORSIMATTR_ENGOPTION_MAX_IDLE_MIXTURE); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("ActorSimAttr", "ACTORSIMATTR_ENGOPTION_MIN_IDLE_MIXTURE", (int)ACTORSIMATTR_ENGOPTION_MIN_IDLE_MIXTURE); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("ActorSimAttr", "ACTORSIMATTR_ENGOPTION_BRAKING_TORQUE", (int)ACTORSIMATTR_ENGOPTION_BRAKING_TORQUE); ROR_ASSERT(result >= 0);
        // ... Engturbo2 (actually'engturbo'withtype=2)
    result = engine->RegisterEnumValue("ActorSimAttr", "ACTORSIMATTR_ENGTURBO2_INERTIA_FACTOR", (int)ACTORSIMATTR_ENGTURBO2_INERTIA_FACTOR); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("ActorSimAttr", "ACTORSIMATTR_ENGTURBO2_NUM_TURBOS", (int)ACTORSIMATTR_ENGTURBO2_NUM_TURBOS); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("ActorSimAttr", "ACTORSIMATTR_ENGTURBO2_MAX_RPM", (int)ACTORSIMATTR_ENGTURBO2_MAX_RPM); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("ActorSimAttr", "ACTORSIMATTR_ENGTURBO2_ENGINE_RPM_OP", (int)ACTORSIMATTR_ENGTURBO2_ENGINE_RPM_OP); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("ActorSimAttr", "ACTORSIMATTR_ENGTURBO2_BOV_ENABLED", (int)ACTORSIMATTR_ENGTURBO2_BOV_ENABLED); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("ActorSimAttr", "ACTORSIMATTR_ENGTURBO2_BOV_MIN_PSI", (int)ACTORSIMATTR_ENGTURBO2_BOV_MIN_PSI); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("ActorSimAttr", "ACTORSIMATTR_ENGTURBO2_WASTEGATE_ENABLED", (int)ACTORSIMATTR_ENGTURBO2_WASTEGATE_ENABLED); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("ActorSimAttr", "ACTORSIMATTR_ENGTURBO2_WASTEGATE_MAX_PSI", (int)ACTORSIMATTR_ENGTURBO2_WASTEGATE_MAX_PSI); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("ActorSimAttr", "ACTORSIMATTR_ENGTURBO2_WASTEGATE_THRESHOLD_N", (int)ACTORSIMATTR_ENGTURBO2_WASTEGATE_THRESHOLD_N); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("ActorSimAttr", "ACTORSIMATTR_ENGTURBO2_WASTEGATE_THRESHOLD_P", (int)ACTORSIMATTR_ENGTURBO2_WASTEGATE_THRESHOLD_P); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("ActorSimAttr", "ACTORSIMATTR_ENGTURBO2_ANTILAG_ENABLED", (int)ACTORSIMATTR_ENGTURBO2_ANTILAG_ENABLED); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("ActorSimAttr", "ACTORSIMATTR_ENGTURBO2_ANTILAG_CHANCE", (int)ACTORSIMATTR_ENGTURBO2_ANTILAG_CHANCE); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("ActorSimAttr", "ACTORSIMATTR_ENGTURBO2_ANTILAG_MIN_RPM", (int)ACTORSIMATTR_ENGTURBO2_ANTILAG_MIN_RPM); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("ActorSimAttr", "ACTORSIMATTR_ENGTURBO2_ANTILAG_POWER", (int)ACTORSIMATTR_ENGTURBO2_ANTILAG_POWER); ROR_ASSERT(result >= 0);

    // Truck controls
    result = engine->RegisterEnumValue("ActorSimAttr", "ACTORSIMATTR_TRUCK_STEERING", (int)ACTORSIMATTR_TRUCK_STEERING); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("ActorSimAttr", "ACTORSIMATTR_TRUCK_BRAKE", (int)ACTORSIMATTR_TRUCK_BRAKE); ROR_ASSERT(result >= 0);

    // Aircraft control surfaces
    result = engine->RegisterEnumValue("ActorSimAttr", "ACTORSIMATTR_AIRCRAFT_AIRBRAKES", (int)ACTORSIMATTR_AIRCRAFT_AIRBRAKES); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("ActorSimAttr", "ACTORSIMATTR_AIRCRAFT_FLAPS", (int)ACTORSIMATTR_AIRCRAFT_FLAPS); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("ActorSimAttr", "ACTORSIMATTR_AIRCRAFT_AILERON", (int)ACTORSIMATTR_AIRCRAFT_AILERON); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("ActorSimAttr", "ACTORSIMATTR_AIRCRAFT_ELEVATOR", (int)ACTORSIMATTR_AIRCRAFT_ELEVATOR); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("ActorSimAttr", "ACTORSIMATTR_AIRCRAFT_RUDDER", (int)ACTORSIMATTR_AIRCRAFT_RUDDER); ROR_ASSERT(result >= 0);

    // class Actor (historically Beam)
    Actor::RegisterRefCountingObject(engine, "BeamClass");
    ActorPtr::RegisterRefCountingObjectPtr(engine, "BeamClassPtr", "BeamClass");

    // - physics state (PLEASE maintain the same order as 'Actor.h' and 'doc/angelscript/.../BeamClass.h')
    result = engine->RegisterObjectMethod("BeamClass", "TruckState getTruckState()", asMETHOD(Actor,getTruckState), asCALL_THISCALL); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("BeamClass", "vector3 getPosition()", asMETHOD(Actor,getPosition), asCALL_THISCALL); ROR_ASSERT(result>=0);
    /*alias:*/result = engine->RegisterObjectMethod("BeamClass", "vector3 getVehiclePosition()", asMETHOD(Actor,getPosition), asCALL_THISCALL); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("BeamClass", "float getRotation()", asMETHOD(Actor,getRotation), asCALL_THISCALL); ROR_ASSERT(result>=0);
    /*alias:*/result = engine->RegisterObjectMethod("BeamClass", "float getHeadingDirectionAngle()", asMETHOD(Actor,getRotation), asCALL_THISCALL); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("BeamClass", "quaternion getOrientation()", asMETHOD(Actor,getOrientation), asCALL_THISCALL); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("BeamClass", "float getSpeed()", asMETHOD(Actor,getSpeed), asCALL_THISCALL); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("BeamClass", "vector3 getGForces()", asMETHOD(Actor,getGForces), asCALL_THISCALL); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("BeamClass", "float getTotalMass(bool)", asMETHOD(Actor,getTotalMass), asCALL_THISCALL); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("BeamClass", "float getDryMass()", asMETHOD(Actor, getDryMass), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("BeamClass", "float getLoadedMass()", asMETHOD(Actor, getLoadedMass), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("BeamClass", "float getInitialDryMass()", asMETHOD(Actor, getInitialDryMass), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("BeamClass", "float getInitialLoadedMass()", asMETHOD(Actor, getInitialLoadedMass), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("BeamClass", "int getNodeCount()", asMETHOD(Actor,getNodeCount), asCALL_THISCALL); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("BeamClass", "vector3 getNodePosition(int)", asMETHOD(Actor, getNodePosition), asCALL_THISCALL); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("BeamClass", "float getNodeInitialMass(int)", asMETHOD(Actor, getNodeInitialMass), asCALL_THISCALL); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("BeamClass", "float getNodeMass(int)", asMETHOD(Actor, getNodeMass), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("BeamClass", "vector3 getNodeVelocity(int)", asMETHOD(Actor, getNodeVelocity), asCALL_THISCALL); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("BeamClass", "vector3 getNodeForces(int)", asMETHOD(Actor, getNodeForces), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("BeamClass", "void getNodeMassOptions(int, bool&inout, bool&inout)", asMETHOD(Actor, getNodeMassOptions), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("BeamClass", "bool isNodeWheelRim(int)", asMETHOD(Actor,isNodeWheelRim), asCALL_THISCALL); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("BeamClass", "bool isNodeWheelTire(int)", asMETHOD(Actor,isNodeWheelTire), asCALL_THISCALL); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("BeamClass", "int getWheelNodeCount()", asMETHOD(Actor,getWheelNodeCount), asCALL_THISCALL); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("BeamClass", "float getWheelSpeed()", asMETHOD(Actor,getWheelSpeed), asCALL_THISCALL); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("BeamClass", "void reset(bool)", asMETHOD(Actor,reset), asCALL_THISCALL); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("BeamClass", "int getShockCount()", AngelScript::asMETHOD(Actor,getShockCount), AngelScript::asCALL_THISCALL); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("BeamClass", "float getShockSpringRate(int)", AngelScript::asMETHOD(Actor,getShockSpringRate), AngelScript::asCALL_THISCALL); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("BeamClass", "float getShockDamping(int)", AngelScript::asMETHOD(Actor,getShockDamping), AngelScript::asCALL_THISCALL); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("BeamClass", "float getShockVelocity(int)", AngelScript::asMETHOD(Actor,getShockVelocity), AngelScript::asCALL_THISCALL); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("BeamClass", "int getShockNode1(int)", AngelScript::asMETHOD(Actor, getShockNode1), AngelScript::asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("BeamClass", "int getShockNode2(int)", AngelScript::asMETHOD(Actor, getShockNode2), AngelScript::asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("BeamClass", "float getAirbrakeIntensity()", asMETHOD(Actor,getAirbrakeIntensity), asCALL_THISCALL); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("BeamClass", "int getAircraftFlaps()", asMETHOD(Actor,getAircraftFlaps), asCALL_THISCALL); ROR_ASSERT(result>=0);

    // - physics editing (PLEASE maintain the same order as 'Actor.h' and 'doc/angelscript/.../BeamClass.h')
    result = engine->RegisterObjectMethod("BeamClass", "void scaleTruck(float)", asMETHOD(Actor,scaleTruck), asCALL_THISCALL); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("BeamClass", "void setMass(float)", asMETHOD(Actor,setMass), asCALL_THISCALL); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("BeamClass", "void setLoadedMass(float)", asMETHOD(Actor,setLoadedMass), asCALL_THISCALL); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("BeamClass", "void setNodeMass(int, float)", asMETHOD(Actor, setNodeMass), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("BeamClass", "void setNodeMassOptions(int, bool, bool)", asMETHOD(Actor, setNodeMassOptions), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("BeamClass", "void setSimAttribute(ActorSimAttr, float)", asMETHOD(Actor, setSimAttribute), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("BeamClass", "float getSimAttribute(ActorSimAttr)", asMETHOD(Actor, getSimAttribute), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("BeamClass", "void recalculateNodeMasses()", asMETHOD(Actor,recalculateNodeMasses), asCALL_THISCALL); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("BeamClass", "void setAirbrakeIntensity(float)", asMETHOD(Actor,setAirbrakeIntensity), asCALL_THISCALL); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("BeamClass", "void setAircraftFlaps(int)", asMETHOD(Actor,setAircraftFlaps), asCALL_THISCALL); ROR_ASSERT(result>=0);
    
    // - user interaction (PLEASE maintain the same order as 'Actor.h' and 'doc/angelscript/.../BeamClass.h')
    result = engine->RegisterObjectMethod("BeamClass", "bool getParkingBrake()", asMETHOD(Actor, getParkingBrake), asCALL_THISCALL); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("BeamClass", "bool getCruiseControl()", asMETHOD(Actor, getCruiseControl), asCALL_THISCALL); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("BeamClass", "bool getTractionControl()", asMETHOD(Actor, getTractionControl), asCALL_THISCALL); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("BeamClass", "bool getAntiLockBrake()", asMETHOD(Actor, getAntiLockBrake), asCALL_THISCALL); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("BeamClass", "void parkingbrakeToggle()", asMETHOD(Actor,parkingbrakeToggle), asCALL_THISCALL); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("BeamClass", "void tractioncontrolToggle()", asMETHOD(Actor,tractioncontrolToggle), asCALL_THISCALL); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("BeamClass", "void antilockbrakeToggle()", asMETHOD(Actor, antilockbrakeToggle), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("BeamClass", "void cruisecontrolToggle()", asMETHOD(Actor, cruisecontrolToggle), asCALL_THISCALL); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("BeamClass", "void toggleCustomParticles()", asMETHOD(Actor,toggleCustomParticles), asCALL_THISCALL); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("BeamClass", "bool getCustomParticleMode()", asMETHOD(Actor,getCustomParticleMode), asCALL_THISCALL); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("BeamClass", "bool isLocked()", asMETHOD(Actor,isLocked), asCALL_THISCALL); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("BeamClass", "void setForcedCinecam(int, int)", asMETHOD(Actor, setForcedCinecam), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("BeamClass", "void clearForcedCinecam()", asMETHOD(Actor, clearForcedCinecam), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("BeamClass", "bool getForcedCinecam(int& inout, int& inout)", asMETHOD(Actor, getForcedCinecam), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("BeamClass", "int getNumCinecams() const", asMETHOD(Actor, getNumCinecams), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("BeamClass", "float getEventValue(inputEvents, bool = false, inputSourceType = inputSourceType::IST_ANY)", asMETHOD(Actor, getEventValue), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("BeamClass", "bool getEventBoolValue(inputEvents)", asMETHOD(Actor, getEventBoolValue), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("BeamClass", "bool getEventBoolValueBounce(inputEvents, float = 0.2f)", asMETHOD(Actor, getEventBoolValueBounce), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("BeamClass", "void clearEventSimulatedValues()", asMETHOD(Actor, clearEventSimulatedValues), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("BeamClass", "bool hasEventSimulatedValue(inputEvents)", asMETHOD(Actor, hasEventSimulatedValue), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("BeamClass", "float getEventSimulatedValue(inputEvents)", asMETHOD(Actor, getEventSimulatedValue), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("BeamClass", "void setEventSimulatedValue(inputEvents, float)", asMETHOD(Actor, setEventSimulatedValue), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("BeamClass", "void removeEventSimulatedValue(inputEvents)", asMETHOD(Actor, removeEventSimulatedValue), asCALL_THISCALL); ROR_ASSERT(result >= 0);

    // - subsystems (PLEASE maintain the same order as 'Actor.h' and 'doc/angelscript/.../BeamClass.h')
    result = engine->RegisterObjectMethod("BeamClass", "DashBoardManagerClassPtr@ getDashboardManager()", asMETHOD(Actor, getDashboardManager), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("BeamClass", "VehicleAIClassPtr @getVehicleAI()", asMETHOD(Actor,getVehicleAI), asCALL_THISCALL); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("BeamClass", "EngineClassPtr @getEngine()", asMETHOD(Actor, getEngine), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("BeamClass", "int getAircraftEngineCount()", asMETHOD(Actor, getAircraftEngineCount), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("BeamClass", "AircraftEngineClassPtr@ getAircraftEngine(int)", asMETHOD(Actor, getAircraftEngine), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("BeamClass", "TurbojetClassPtr@ getTurbojet(int)", asMETHOD(Actor, getTurbojet), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("BeamClass", "TurbopropClassPtr@ getTurboprop(int)", asMETHOD(Actor, getTurboprop), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("BeamClass", "AutopilotClassPtr@ getAutopilot()", asMETHOD(Actor, getAutopilot), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("BeamClass", "int getScrewpropCount()", asMETHOD(Actor, getScrewpropCount), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("BeamClass", "ScrewpropClassPtr@ getScrewprop(int)", asMETHOD(Actor, getScrewprop), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("BeamClass", "Ogre::MaterialPtr getManagedMaterialInstance(const string &in)", asMETHOD(Actor, getManagedMaterialInstance), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("BeamClass", "array<string>@ getManagedMaterialNames()", asFUNCTIONPR([](Actor* self) -> CScriptArray*{
        return RoR::VectorToScriptArray(self->getManagedMaterialNames(), "string"); }, (Actor*), CScriptArray*), asCALL_CDECL_OBJFIRST); ROR_ASSERT(result>=0);

    // - lights (PLEASE maintain the same ordering as 'Actor.h' and 'doc/angelscript/.../BeamClass.h')
    result = engine->RegisterObjectMethod("BeamClass", "int getBlinkType()", asMETHOD(Actor, getBlinkType), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("BeamClass", "void setBlinkType(int)", asMETHOD(Actor, setBlinkType), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("BeamClass", "bool getCustomLightVisible(int)", asMETHOD(Actor, getCustomLightVisible), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("BeamClass", "void setCustomLightVisible(int, bool)", asMETHOD(Actor, setCustomLightVisible), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("BeamClass", "bool getBeaconMode()", asMETHOD(Actor, getBeaconMode), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("BeamClass", "void beaconsToggle()", asMETHOD(Actor, beaconsToggle), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("BeamClass", "bool getBrakeLightVisible()", asMETHOD(Actor, getBrakeLightVisible), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("BeamClass", "bool getReverseLightVisible()", asMETHOD(Actor, getReverseLightVisible), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("BeamClass", "int countCustomLights(int)", asMETHOD(Actor, countCustomLights), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("BeamClass", "int countFlaresByType(FlareType)", asMETHOD(Actor, countFlaresByType), asCALL_THISCALL); ROR_ASSERT(result >= 0);

    // - organizational (PLEASE maintain the same ordering as 'Actor.h' and 'doc/angelscript/.../BeamClass.h')
    result = engine->RegisterObjectMethod("BeamClass", "string getTruckName()", asMETHOD(Actor,getTruckName), asCALL_THISCALL); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("BeamClass", "string getTruckFileName()", asMETHOD(Actor,getTruckFileName), asCALL_THISCALL); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("BeamClass", "string getTruckFileResourceGroup()", asMETHOD(Actor, getTruckFileResourceGroup), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("BeamClass", "int  getTruckType()", asMETHOD(Actor,getTruckType), asCALL_THISCALL); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("BeamClass", "string getSectionConfig()", asMETHOD(Actor, getSectionConfig), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("BeamClass", "int getInstanceId()", asMETHOD(Actor, getInstanceId), asCALL_THISCALL); ROR_ASSERT(result >= 0);
}
