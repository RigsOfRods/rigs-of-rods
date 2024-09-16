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
    result = engine->RegisterObjectMethod("BeamClass", "int getNodeCount()", asMETHOD(Actor,getNodeCount), asCALL_THISCALL); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("BeamClass", "vector3 getNodePosition(int)", asMETHOD(Actor,getNodePosition), asCALL_THISCALL); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("BeamClass", "bool isNodeWheelRim(int)", asMETHOD(Actor,isNodeWheelRim), asCALL_THISCALL); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("BeamClass", "bool isNodeWheelTire(int)", asMETHOD(Actor,isNodeWheelTire), asCALL_THISCALL); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("BeamClass", "int getWheelNodeCount()", asMETHOD(Actor,getWheelNodeCount), asCALL_THISCALL); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("BeamClass", "float getWheelSpeed()", asMETHOD(Actor,getWheelSpeed), asCALL_THISCALL); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("BeamClass", "void reset(bool)", asMETHOD(Actor,reset), asCALL_THISCALL); ROR_ASSERT(result>=0);

    // - physics editing (PLEASE maintain the same order as 'Actor.h' and 'doc/angelscript/.../BeamClass.h')
    result = engine->RegisterObjectMethod("BeamClass", "void scaleTruck(float)", asMETHOD(Actor,scaleTruck), asCALL_THISCALL); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("BeamClass", "void setMass(float)", asMETHOD(Actor,setMass), asCALL_THISCALL); ROR_ASSERT(result>=0);

    // - user interaction (PLEASE maintain the same order as 'Actor.h' and 'doc/angelscript/.../BeamClass.h')
    result = engine->RegisterObjectMethod("BeamClass", "void parkingbrakeToggle()", asMETHOD(Actor,parkingbrakeToggle), asCALL_THISCALL); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("BeamClass", "void tractioncontrolToggle()", asMETHOD(Actor,tractioncontrolToggle), asCALL_THISCALL); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("BeamClass", "void antilockbrakeToggle()", asMETHOD(Actor,antilockbrakeToggle), asCALL_THISCALL); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("BeamClass", "void toggleCustomParticles()", asMETHOD(Actor,toggleCustomParticles), asCALL_THISCALL); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("BeamClass", "bool getCustomParticleMode()", asMETHOD(Actor,getCustomParticleMode), asCALL_THISCALL); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("BeamClass", "bool isLocked()", asMETHOD(Actor,isLocked), asCALL_THISCALL); ROR_ASSERT(result>=0);
    
    // - subsystems (PLEASE maintain the same order as 'Actor.h' and 'doc/angelscript/.../BeamClass.h')
    result = engine->RegisterObjectMethod("BeamClass", "VehicleAIClassPtr @getVehicleAI()", asMETHOD(Actor,getVehicleAI), asCALL_THISCALL); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("BeamClass", "EngineSimClassPtr @getEngineSim()", asMETHOD(Actor, getEngineSim), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("BeamClass", "Ogre::MaterialPtr getManagedMaterialInstance(const string &in)", asMETHOD(Actor,getManagedMaterialInstance), asCALL_THISCALL); ROR_ASSERT(result>=0);
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
