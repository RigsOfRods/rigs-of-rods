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

#include "../../autowrapper/aswrappedcall.h"
#include "Actor.h"
#include "AngelScriptBindings.h"
#include "ScriptEngine.h"
#include "ScriptUtils.h"
#include "SimData.h"
#include <angelscript.h>

using namespace AngelScript;

void RoR::RegisterActorGeneric(asIScriptEngine *engine)
{
    int result;

    // class Actor (historically Beam)
    Actor::RegisterRefCountingObject(engine, "BeamClass");
    ActorPtr::RegisterRefCountingObjectPtr(engine, "BeamClassPtr", "BeamClass");

    // - physics state (PLEASE maintain the same order as 'Actor.h' and 'doc/angelscript/.../BeamClass.h')
    result = engine->RegisterObjectMethod("BeamClass", "TruckState getTruckState()", WRAP_MFN(Actor,getTruckState), asCALL_GENERIC); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("BeamClass", "vector3 getPosition()", WRAP_MFN(Actor,getPosition), asCALL_GENERIC); ROR_ASSERT(result>=0);
    /*alias:*/result = engine->RegisterObjectMethod("BeamClass", "vector3 getVehiclePosition()", WRAP_MFN(Actor,getPosition), asCALL_GENERIC); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("BeamClass", "float getRotation()", WRAP_MFN(Actor,getRotation), asCALL_GENERIC); ROR_ASSERT(result>=0);
    /*alias:*/result = engine->RegisterObjectMethod("BeamClass", "float getHeadingDirectionAngle()", WRAP_MFN(Actor,getRotation), asCALL_GENERIC); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("BeamClass", "quaternion getOrientation()", WRAP_MFN(Actor,getOrientation), asCALL_GENERIC); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("BeamClass", "float getSpeed()", WRAP_MFN(Actor,getSpeed), asCALL_GENERIC); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("BeamClass", "vector3 getGForces()", WRAP_MFN(Actor,getGForces), asCALL_GENERIC); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("BeamClass", "float getTotalMass(bool)", WRAP_MFN(Actor,getTotalMass), asCALL_GENERIC); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("BeamClass", "float getDryMass()", WRAP_MFN(Actor, getDryMass), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("BeamClass", "float getLoadedMass()", WRAP_MFN(Actor, getLoadedMass), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("BeamClass", "float getInitialDryMass()", WRAP_MFN(Actor, getInitialDryMass), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("BeamClass", "float getInitialLoadedMass()", WRAP_MFN(Actor, getInitialLoadedMass), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("BeamClass", "int getNodeCount()", WRAP_MFN(Actor,getNodeCount), asCALL_GENERIC); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("BeamClass", "vector3 getNodePosition(int)", WRAP_MFN(Actor, getNodePosition), asCALL_GENERIC); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("BeamClass", "float getNodeInitialMass(int)", WRAP_MFN(Actor, getNodeInitialMass), asCALL_GENERIC); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("BeamClass", "float getNodeMass(int)", WRAP_MFN(Actor, getNodeMass), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("BeamClass", "vector3 getNodeVelocity(int)", WRAP_MFN(Actor, getNodeVelocity), asCALL_GENERIC); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("BeamClass", "vector3 getNodeForces(int)", WRAP_MFN(Actor, getNodeForces), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("BeamClass", "void getNodeMassOptions(int, bool&inout, bool&inout)", WRAP_MFN(Actor, getNodeMassOptions), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("BeamClass", "bool isNodeWheelRim(int)", WRAP_MFN(Actor,isNodeWheelRim), asCALL_GENERIC); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("BeamClass", "bool isNodeWheelTire(int)", WRAP_MFN(Actor,isNodeWheelTire), asCALL_GENERIC); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("BeamClass", "int getWheelNodeCount()", WRAP_MFN(Actor,getWheelNodeCount), asCALL_GENERIC); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("BeamClass", "float getWheelSpeed()", WRAP_MFN(Actor,getWheelSpeed), asCALL_GENERIC); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("BeamClass", "void reset(bool)", WRAP_MFN(Actor,reset), asCALL_GENERIC); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("BeamClass", "int getShockCount()", WRAP_MFN(Actor,getShockCount), AngelScript::asCALL_GENERIC); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("BeamClass", "float getShockSpringRate(int)", WRAP_MFN(Actor,getShockSpringRate), AngelScript::asCALL_GENERIC); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("BeamClass", "float getShockDamping(int)", WRAP_MFN(Actor,getShockDamping), AngelScript::asCALL_GENERIC); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("BeamClass", "float getShockVelocity(int)", WRAP_MFN(Actor,getShockVelocity), AngelScript::asCALL_GENERIC); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("BeamClass", "int getShockNode1(int)", WRAP_MFN(Actor, getShockNode1), AngelScript::asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("BeamClass", "int getShockNode2(int)", WRAP_MFN(Actor, getShockNode2), AngelScript::asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("BeamClass", "float getAirbrakeIntensity()", WRAP_MFN(Actor,getAirbrakeIntensity), asCALL_GENERIC); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("BeamClass", "int getAircraftFlaps()", WRAP_MFN(Actor,getAircraftFlaps), asCALL_GENERIC); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("BeamClass", "void wakeUp()", WRAP_MFN(Actor, wakeUp), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("BeamClass", "void sendToSleep()", WRAP_MFN(Actor, sendToSleep), asCALL_GENERIC); ROR_ASSERT(result >= 0);

    // - physics editing (PLEASE maintain the same order as 'Actor.h' and 'doc/angelscript/.../BeamClass.h')
    result = engine->RegisterObjectMethod("BeamClass", "void scaleTruck(float)", WRAP_MFN(Actor,scaleTruck), asCALL_GENERIC); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("BeamClass", "void setMass(float)", WRAP_MFN(Actor,setMass), asCALL_GENERIC); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("BeamClass", "void setLoadedMass(float)", WRAP_MFN(Actor,setLoadedMass), asCALL_GENERIC); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("BeamClass", "void setNodeMass(int, float)", WRAP_MFN(Actor, setNodeMass), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("BeamClass", "void setNodeMassOptions(int, bool, bool)", WRAP_MFN(Actor, setNodeMassOptions), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("BeamClass", "void setSimAttribute(ActorSimAttr, float)", WRAP_MFN(Actor, setSimAttribute), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("BeamClass", "void setIndexedSimAttribute(ActorSimAttr, float, int)", WRAP_MFN(Actor, setIndexedSimAttribute), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("BeamClass", "float getSimAttribute(ActorSimAttr)", WRAP_MFN(Actor, getSimAttribute), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("BeamClass", "float getIndexedSimAttribute(ActorSimAttr, int)", WRAP_MFN(Actor, getIndexedSimAttribute), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("BeamClass", "void recalculateNodeMasses()", WRAP_MFN(Actor,recalculateNodeMasses), asCALL_GENERIC); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("BeamClass", "void setAirbrakeIntensity(float)", WRAP_MFN(Actor,setAirbrakeIntensity), asCALL_GENERIC); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("BeamClass", "void setAircraftFlaps(int)", WRAP_MFN(Actor,setAircraftFlaps), asCALL_GENERIC); ROR_ASSERT(result>=0);
    
    // - user interaction (PLEASE maintain the same order as 'Actor.h' and 'doc/angelscript/.../BeamClass.h')
    result = engine->RegisterObjectMethod("BeamClass", "bool getParkingBrake()", WRAP_MFN(Actor, getParkingBrake), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("BeamClass", "bool getTractionControl()", WRAP_MFN(Actor, getTractionControl), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("BeamClass", "bool getAntiLockBrake()", WRAP_MFN(Actor, getAntiLockBrake), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("BeamClass", "bool getCruiseControl()", WRAP_MFN(Actor, getCruiseControl), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("BeamClass", "void parkingbrakeToggle()", WRAP_MFN(Actor,parkingbrakeToggle), asCALL_GENERIC); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("BeamClass", "void tractioncontrolToggle()", WRAP_MFN(Actor,tractioncontrolToggle), asCALL_GENERIC); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("BeamClass", "void antilockbrakeToggle()", WRAP_MFN(Actor,antilockbrakeToggle), asCALL_GENERIC); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("BeamClass", "void cruisecontrolToggle()", WRAP_MFN(Actor, cruisecontrolToggle), asCALL_GENERIC); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("BeamClass", "void toggleCustomParticles()", WRAP_MFN(Actor,toggleCustomParticles), asCALL_GENERIC); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("BeamClass", "bool getCustomParticleMode()", WRAP_MFN(Actor,getCustomParticleMode), asCALL_GENERIC); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("BeamClass", "bool isLocked()", WRAP_MFN(Actor,isLocked), asCALL_GENERIC); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("BeamClass", "void setForcedCinecam(int, int)", WRAP_MFN(Actor, setForcedCinecam), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("BeamClass", "void clearForcedCinecam()", WRAP_MFN(Actor, clearForcedCinecam), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("BeamClass", "bool getForcedCinecam(int& inout, int& inout)", WRAP_MFN(Actor, getForcedCinecam), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("BeamClass", "int getNumCinecams() const", WRAP_MFN(Actor, getNumCinecams), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("BeamClass", "int getCameraCount() const", WRAP_MFN(Actor, getCameraCount), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("BeamClass", "int getCameraPosNode(int) const", WRAP_MFN(Actor, getCameraPosNode), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("BeamClass", "int getCameraDirNode(int) const", WRAP_MFN(Actor, getCameraDirNode), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("BeamClass", "int getCameraRollNode(int) const", WRAP_MFN(Actor, getCameraRollNode), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    
    // - input engine overrides (PLEASE maintain the same order as 'Actor.h' and 'doc/angelscript/.../BeamClass.h')
    result = engine->RegisterObjectMethod("BeamClass", "float getEventValue(inputEvents, bool = false, inputSourceType = inputSourceType::IST_ANY)", WRAP_MFN(Actor, getEventValue), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("BeamClass", "bool getEventBoolValue(inputEvents)", WRAP_MFN(Actor, getEventBoolValue), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("BeamClass", "bool getEventBoolValueBounce(inputEvents, float = 0.2f)", WRAP_MFN(Actor, getEventBoolValueBounce), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("BeamClass", "void clearEventSimulatedValues()", WRAP_MFN(Actor, clearEventSimulatedValues), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("BeamClass", "bool hasEventSimulatedValue(inputEvents)", WRAP_MFN(Actor, hasEventSimulatedValue), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("BeamClass", "float getEventSimulatedValue(inputEvents)", WRAP_MFN(Actor, getEventSimulatedValue), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("BeamClass", "void setEventSimulatedValue(inputEvents, float)", WRAP_MFN(Actor, setEventSimulatedValue), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("BeamClass", "void resetEventSimulatedValue(inputEvents)", WRAP_MFN(Actor, resetEventSimulatedValue), asCALL_GENERIC); ROR_ASSERT(result >= 0);

    // - subsystems (PLEASE maintain the same order as 'Actor.h' and 'doc/angelscript/.../BeamClass.h')
    result = engine->RegisterObjectMethod("BeamClass", "DashBoardManagerClassPtr@ getDashboardManager()", WRAP_MFN(Actor, getDashboardManager), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("BeamClass", "VehicleAIClassPtr @getVehicleAI()", WRAP_MFN(Actor,getVehicleAI), asCALL_GENERIC); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("BeamClass", "EngineClassPtr @getEngine()", WRAP_MFN(Actor, getEngine), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("BeamClass", "int getAircraftEngineCount()", WRAP_MFN(Actor, getAircraftEngineCount), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("BeamClass", "AircraftEngineClassPtr@ getAircraftEngine(int)", WRAP_MFN(Actor, getAircraftEngine), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("BeamClass", "TurbojetClassPtr@ getTurbojet(int)", WRAP_MFN(Actor, getTurbojet), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("BeamClass", "TurbopropClassPtr@ getTurboprop(int)", WRAP_MFN(Actor, getTurboprop), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("BeamClass", "AutopilotClassPtr@ getAutopilot()", WRAP_MFN(Actor, getAutopilot), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("BeamClass", "int getScrewpropCount()", WRAP_MFN(Actor, getScrewpropCount), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("BeamClass", "ScrewpropClassPtr@ getScrewprop(int)", WRAP_MFN(Actor, getScrewprop), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("BeamClass", "Ogre::MaterialPtr getManagedMaterialInstance(const string &in)", WRAP_MFN(Actor, getManagedMaterialInstance), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("BeamClass", "array<string>@ getManagedMaterialNames()", asFUNCTIONPR([](Actor* self) -> CScriptArray*{
        return RoR::VectorToScriptArray(self->getManagedMaterialNames(), "string"); }, (Actor*), CScriptArray*), asCALL_CDECL_OBJFIRST); ROR_ASSERT(result>=0);

    // - lights (PLEASE maintain the same ordering as 'Actor.h' and 'doc/angelscript/.../BeamClass.h')
    result = engine->RegisterObjectMethod("BeamClass", "int getBlinkType()", WRAP_MFN(Actor, getBlinkType), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("BeamClass", "void setBlinkType(int)", WRAP_MFN(Actor, setBlinkType), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("BeamClass", "bool getCustomLightVisible(int)", WRAP_MFN(Actor, getCustomLightVisible), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("BeamClass", "void setCustomLightVisible(int, bool)", WRAP_MFN(Actor, setCustomLightVisible), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("BeamClass", "bool getBeaconMode()", WRAP_MFN(Actor, getBeaconMode), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("BeamClass", "void beaconsToggle()", WRAP_MFN(Actor, beaconsToggle), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("BeamClass", "bool getBrakeLightVisible()", WRAP_MFN(Actor, getBrakeLightVisible), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("BeamClass", "bool getReverseLightVisible()", WRAP_MFN(Actor, getReverseLightVisible), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("BeamClass", "int countCustomLights(int)", WRAP_MFN(Actor, countCustomLights), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("BeamClass", "int countFlaresByType(FlareType)", WRAP_MFN(Actor, countFlaresByType), asCALL_GENERIC); ROR_ASSERT(result >= 0);

    // - organizational (PLEASE maintain the same ordering as 'Actor.h' and 'doc/angelscript/.../BeamClass.h')
    result = engine->RegisterObjectMethod("BeamClass", "string getTruckName()", WRAP_MFN(Actor,getTruckName), asCALL_GENERIC); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("BeamClass", "string getTruckFileName()", WRAP_MFN(Actor,getTruckFileName), asCALL_GENERIC); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("BeamClass", "string getTruckFileResourceGroup()", WRAP_MFN(Actor, getTruckFileResourceGroup), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("BeamClass", "int  getTruckType()", WRAP_MFN(Actor,getTruckType), asCALL_GENERIC); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("BeamClass", "string getSectionConfig()", WRAP_MFN(Actor, getSectionConfig), asCALL_GENERIC); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("BeamClass", "int getInstanceId()", WRAP_MFN(Actor, getInstanceId), asCALL_GENERIC); ROR_ASSERT(result >= 0);
}
