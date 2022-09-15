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
#include "SimData.h"
#include <angelscript.h>

using namespace AngelScript;

void RoR::RegisterActor(asIScriptEngine *engine)
{
    int result;
    
    // enum truckStates
    result = engine->RegisterEnum("truckStates"); ROR_ASSERT(result>=0);
    result = engine->RegisterEnumValue("truckStates", "TS_SIMULATED", static_cast<int>(ActorState::LOCAL_SIMULATED)); ROR_ASSERT(result>=0);
    result = engine->RegisterEnumValue("truckStates", "TS_SLEEPING",  static_cast<int>(ActorState::LOCAL_SLEEPING)); ROR_ASSERT(result>=0);
    result = engine->RegisterEnumValue("truckStates", "TS_NETWORKED", static_cast<int>(ActorState::NETWORKED_OK)); ROR_ASSERT(result>=0);

    // enum truckTypes
    result = engine->RegisterEnum("truckTypes"); ROR_ASSERT(result>=0);
    result = engine->RegisterEnumValue("truckTypes", "TT_NOT_DRIVEABLE", NOT_DRIVEABLE); ROR_ASSERT(result>=0);
    result = engine->RegisterEnumValue("truckTypes", "TT_TRUCK", TRUCK); ROR_ASSERT(result>=0);
    result = engine->RegisterEnumValue("truckTypes", "TT_AIRPLANE", AIRPLANE); ROR_ASSERT(result>=0);
    result = engine->RegisterEnumValue("truckTypes", "TT_BOAT", BOAT); ROR_ASSERT(result>=0);
    result = engine->RegisterEnumValue("truckTypes", "TT_MACHINE", MACHINE); ROR_ASSERT(result>=0);
    result = engine->RegisterEnumValue("truckTypes", "TT_AI", AI); ROR_ASSERT(result>=0);
    

    // class Actor (historically Beam)
    result = engine->RegisterObjectType("BeamClass", sizeof(Actor), asOBJ_REF); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("BeamClass", "void scaleTruck(float)", asMETHOD(Actor,scaleTruck), asCALL_THISCALL); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("BeamClass", "string getTruckName()", asMETHOD(Actor,getTruckName), asCALL_THISCALL); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("BeamClass", "string getTruckFileName()", asMETHOD(Actor,getTruckFileName), asCALL_THISCALL); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("BeamClass", "string getSectionConfig()", asMETHOD(Actor, getSectionConfig), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("BeamClass", "int  getTruckType()", asMETHOD(Actor,getTruckType), asCALL_THISCALL); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("BeamClass", "void reset(bool)", asMETHOD(Actor,reset), asCALL_THISCALL); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("BeamClass", "void parkingbrakeToggle()", asMETHOD(Actor,parkingbrakeToggle), asCALL_THISCALL); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("BeamClass", "void tractioncontrolToggle()", asMETHOD(Actor,tractioncontrolToggle), asCALL_THISCALL); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("BeamClass", "void antilockbrakeToggle()", asMETHOD(Actor,antilockbrakeToggle), asCALL_THISCALL); ROR_ASSERT(result>=0);
    
    result = engine->RegisterObjectMethod("BeamClass", "void toggleCustomParticles()", asMETHOD(Actor,toggleCustomParticles), asCALL_THISCALL); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("BeamClass", "int getNodeCount()", asMETHOD(Actor,getNodeCount), asCALL_THISCALL); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("BeamClass", "float getTotalMass(bool)", asMETHOD(Actor,getTotalMass), asCALL_THISCALL); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("BeamClass", "int getWheelNodeCount()", asMETHOD(Actor,getWheelNodeCount), asCALL_THISCALL); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("BeamClass", "void setMass(float)", asMETHOD(Actor,setMass), asCALL_THISCALL); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("BeamClass", "bool getCustomParticleMode()", asMETHOD(Actor,getCustomParticleMode), asCALL_THISCALL); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("BeamClass", "float getHeadingDirectionAngle()", asMETHOD(Actor,getRotation), asCALL_THISCALL); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("BeamClass", "bool isLocked()", asMETHOD(Actor,isLocked), asCALL_THISCALL); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("BeamClass", "float getWheelSpeed()", asMETHOD(Actor,getWheelSpeed), asCALL_THISCALL); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("BeamClass", "float getSpeed()", asMETHOD(Actor,getSpeed), asCALL_THISCALL); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("BeamClass", "vector3 getGForces()", asMETHOD(Actor,getGForces), asCALL_THISCALL); ROR_ASSERT(result>=0);

    result = engine->RegisterObjectMethod("BeamClass", "float getRotation()", asMETHOD(Actor,getRotation), asCALL_THISCALL); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("BeamClass", "vector3 getVehiclePosition()", asMETHOD(Actor,getPosition), asCALL_THISCALL); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("BeamClass", "vector3 getNodePosition(int)", asMETHOD(Actor,getNodePosition), asCALL_THISCALL); ROR_ASSERT(result>=0);

    result = engine->RegisterObjectMethod("BeamClass", "VehicleAIClass @getVehicleAI()", asMETHOD(Actor,getVehicleAI), asCALL_THISCALL); ROR_ASSERT(result>=0);
    
    // - lights (PLEASE maintain the same order as in 'Actor.h')
    result = engine->RegisterObjectMethod("BeamClass", "int getBlinkType()", asMETHOD(Actor, getBlinkType), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("BeamClass", "void setBlinkType(int)", asMETHOD(Actor, setBlinkType), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("BeamClass", "bool getCustomLightVisible(int)", asMETHOD(Actor, getCustomLightVisible), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("BeamClass", "void setCustomLightVisible(int, bool)", asMETHOD(Actor, setCustomLightVisible), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("BeamClass", "bool getBeaconMode()", asMETHOD(Actor, getBeaconMode), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("BeamClass", "void beaconsToggle()", asMETHOD(Actor, beaconsToggle), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("BeamClass", "bool getBrakeLightVisible()", asMETHOD(Actor, getBrakeLightVisible), asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("BeamClass", "bool getReverseLightVisible()", asMETHOD(Actor, getReverseLightVisible), asCALL_THISCALL); ROR_ASSERT(result >= 0);

    // - refcount
    result = engine->RegisterObjectBehaviour("BeamClass", asBEHAVE_ADDREF, "void f()", asMETHOD(Actor,addRef), asCALL_THISCALL); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectBehaviour("BeamClass", asBEHAVE_RELEASE, "void f()", asMETHOD(Actor,release), asCALL_THISCALL); ROR_ASSERT(result>=0);

}
