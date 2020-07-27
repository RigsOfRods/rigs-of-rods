/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2013-2017 Petr Ohlidal & contributors

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
/// @author Thomas Fischer
/// @date   24th of February 2009

#include "ScriptEngine.h"

// AS addons start
#include "scriptstdstring/scriptstdstring.h"
#include "scriptmath/scriptmath.h"
#include "scriptany/scriptany.h"
#include "scriptarray/scriptarray.h"
#include "scripthelper/scripthelper.h"
// AS addons end

#ifdef USE_CURL
#include <stdio.h>
#include <curl/curl.h>
//#include <curl/types.h>
#include <curl/easy.h>
#endif //USE_CURL

#include "Application.h"
#include "Beam.h"
#include "BeamFactory.h"
#include "Collisions.h"
#include "Console.h"
#include "GameScript.h"
#include "LocalStorage.h"
#include "OgreAngelscript.h"
#include "OgreScriptBuilder.h"
#include "OgreSubsystem.h"
#include "PlatformUtils.h"
#include "RoRFrameListener.h" // SimController
#include "ScriptEvents.h"
#include "VehicleAI.h"

const char *ScriptEngine::moduleName = "RoRScript";

using namespace Ogre;
using namespace RoR;

// some hacky functions

void logString(const std::string &str)
{
    ScriptEngine::getSingleton().SLOG(str);
}

// the class implementation

ScriptEngine::ScriptEngine(Collisions *coll) :
      coll(coll)
    , context(0)
    , defaultEventCallbackFunctionPtr(nullptr)
    , engine(0)
    , eventCallbackFunctionPtr(nullptr)
    , eventMask(0)
    , frameStepFunctionPtr(nullptr)
    , scriptHash()
    , scriptLog(0)
    , scriptName()
{
    setSingleton(this);

    // create our own log
    scriptLog = LogManager::getSingleton().createLog(PathCombine(App::sys_logs_dir->GetActiveStr(), "Angelscript.log"), false);

    // init not earlier, otherwise crash
    this->init();
}

ScriptEngine::~ScriptEngine()
{
    // Clean up
    if (engine)  engine->Release();
    if (context) context->Release();
}



void ScriptEngine::messageLogged( const String& message, LogMessageLevel lml, bool maskDebug, const String &logName, bool& skipThisMessage)
{
    RoR::App::GetConsole()->ForwardLogMessage(Console::CONSOLE_MSGTYPE_SCRIPT, message, lml);
}

void AS_RequestActorReset(Actor* a, bool keep_position) // Substitute for removed `Actor` function
{
    ActorModifyRequest rq;
    rq.amr_actor = a;
    rq.amr_type  = (keep_position) ? ActorModifyRequest::Type::RESET_ON_SPOT : ActorModifyRequest::Type::RESET_ON_INIT_POS;
    RoR::App::GetSimController()->QueueActorModify(rq);
}

// continue with initializing everything
void ScriptEngine::init()
{
    SLOG("ScriptEngine initializing ...");
    int result;

    // Create the script engine
    engine = AngelScript::asCreateScriptEngine(ANGELSCRIPT_VERSION);

    // Set the message callback to receive information on errors in human readable form.
    // It's recommended to do this right after the creation of the engine, because if
    // some registration fails the engine may send valuable information to the message
    // stream.
    result = engine->SetMessageCallback(AngelScript::asMETHOD(ScriptEngine,msgCallback), this, AngelScript::asCALL_THISCALL);
    if (result < 0)
    {
        if (result == AngelScript::asINVALID_ARG)
        {
            SLOG("One of the arguments is incorrect, e.g. obj is null for a class method.");
            return;
        }
        else if (result == AngelScript::asNOT_SUPPORTED)
        {
            SLOG("	The arguments are not supported, e.g. asCALL_GENERIC.");
            return;
        }
        SLOG("Unkown error while setting up message callback");
        return;
    }

    // AngelScript doesn't have a built-in string type, as there is no definite standard
    // string type for C++ applications. Every developer is free to register it's own string type.
    // The SDK do however provide a standard add-on for registering a string type, so it's not
    // necessary to register your own string type if you don't want to.
    AngelScript::RegisterScriptArray(engine, true);
    AngelScript::RegisterStdString(engine);
    AngelScript::RegisterStdStringUtils(engine);
    AngelScript::RegisterScriptMath(engine);
    AngelScript::RegisterScriptAny(engine);
    AngelScript::RegisterScriptDictionary(engine);

    // register some Ogre objects like the vector3 and the quaternion
    registerOgreObjects(engine);

    // Register the local storage object.
    // This needs to be done after the registration of the ogre objects!
    registerLocalStorage(engine);

    // some useful global functions
    result = engine->RegisterGlobalFunction("void log(const string &in)", AngelScript::asFUNCTION(logString), AngelScript::asCALL_CDECL); MYASSERT( result >= 0 );
    result = engine->RegisterGlobalFunction("void print(const string &in)", AngelScript::asFUNCTION(logString), AngelScript::asCALL_CDECL); MYASSERT( result >= 0 );

    result = engine->RegisterObjectType("BeamFactoryClass", sizeof(ActorManager), AngelScript::asOBJ_REF); MYASSERT(result>=0);
    result = engine->RegisterObjectMethod("BeamFactoryClass", "void setSimulationSpeed(float)", AngelScript::asMETHOD(ActorManager,SetSimulationSpeed), AngelScript::asCALL_THISCALL); MYASSERT(result>=0);
    result = engine->RegisterObjectBehaviour("BeamFactoryClass", AngelScript::asBEHAVE_ADDREF, "void f()", AngelScript::asMETHOD(ActorManager,AddRef), AngelScript::asCALL_THISCALL); MYASSERT(result>=0);
    result = engine->RegisterObjectBehaviour("BeamFactoryClass", AngelScript::asBEHAVE_RELEASE, "void f()", AngelScript::asMETHOD(ActorManager,Release), AngelScript::asCALL_THISCALL); MYASSERT(result>=0);

    // enum aiEvents
    result = engine->RegisterEnum("aiEvents"); MYASSERT(result >= 0);
    result = engine->RegisterEnumValue("aiEvents", "AI_LIGHTSTOGGLE", AI_LIGHTSTOGGLE); MYASSERT(result >= 0);
    result = engine->RegisterEnumValue("aiEvents", "AI_WAIT_SECONDS", AI_WAIT_SECONDS); MYASSERT(result >= 0);
    result = engine->RegisterEnumValue("aiEvents", "AI_BEACONSTOGGLE", AI_BEACONSTOGGLE); MYASSERT(result >= 0);

    // enum aiEvents
    result = engine->RegisterEnum("AiValues"); MYASSERT(result >= 0);
    result = engine->RegisterEnumValue("AiValues", "AI_SPEED", AI_SPEED); MYASSERT(result >= 0);
    result = engine->RegisterEnumValue("AiValues", "AI_POWER", AI_POWER); MYASSERT(result >= 0);

    result = engine->RegisterObjectType("VehicleAIClass", sizeof(VehicleAI), AngelScript::asOBJ_REF); MYASSERT(result >= 0);
    result = engine->RegisterObjectMethod("VehicleAIClass", "void addWaypoint(string &in, vector3 &in)", AngelScript::asMETHOD(VehicleAI, AddWaypoint), AngelScript::asCALL_THISCALL); MYASSERT(result >= 0);
    result = engine->RegisterObjectMethod("VehicleAIClass", "void addWaypoints(dictionary &in)", AngelScript::asMETHOD(VehicleAI, AddWaypoint), AngelScript::asCALL_THISCALL); MYASSERT(result >= 0);
    result = engine->RegisterObjectMethod("VehicleAIClass", "void setActive(bool)", AngelScript::asMETHOD(VehicleAI, SetActive), AngelScript::asCALL_THISCALL); MYASSERT(result >= 0);
    result = engine->RegisterObjectMethod("VehicleAIClass", "void addEvent(string &in,int &in)", AngelScript::asMETHOD(VehicleAI, AddEvent), AngelScript::asCALL_THISCALL); MYASSERT(result >= 0);
    result = engine->RegisterObjectMethod("VehicleAIClass", "void setValueAtWaypoint(string &in, int &in, float &in)", AngelScript::asMETHOD(VehicleAI, SetValueAtWaypoint), AngelScript::asCALL_THISCALL); MYASSERT(result >= 0);
    result = engine->RegisterObjectBehaviour("VehicleAIClass", AngelScript::asBEHAVE_ADDREF, "void f()", AngelScript::asMETHOD(VehicleAI, addRef), AngelScript::asCALL_THISCALL); MYASSERT(result >= 0);
    result = engine->RegisterObjectBehaviour("VehicleAIClass", AngelScript::asBEHAVE_RELEASE, "void f()", AngelScript::asMETHOD(VehicleAI, release), AngelScript::asCALL_THISCALL); MYASSERT(result >= 0);



    // Register everything
    // class Beam
    result = engine->RegisterObjectType("BeamClass", sizeof(Actor), AngelScript::asOBJ_REF); MYASSERT(result>=0);
    result = engine->RegisterObjectMethod("BeamClass", "void scaleTruck(float)", AngelScript::asMETHOD(Actor,ScaleActor), AngelScript::asCALL_THISCALL); MYASSERT(result>=0);
    result = engine->RegisterObjectMethod("BeamClass", "string getTruckName()", AngelScript::asMETHOD(Actor,GetActorDesignName), AngelScript::asCALL_THISCALL); MYASSERT(result>=0);
    result = engine->RegisterObjectMethod("BeamClass", "string getTruckFileName()", AngelScript::asMETHOD(Actor,GetActorFileName), AngelScript::asCALL_THISCALL); MYASSERT(result>=0);
    result = engine->RegisterObjectMethod("BeamClass", "int  getTruckType()", AngelScript::asMETHOD(Actor,GetActorType), AngelScript::asCALL_THISCALL); MYASSERT(result>=0);
    result = engine->RegisterObjectMethod("BeamClass", "void reset(bool)", AngelScript::asFUNCTION(AS_RequestActorReset), AngelScript::asCALL_CDECL_OBJFIRST); MYASSERT(result>=0);
    result = engine->RegisterObjectMethod("BeamClass", "void parkingbrakeToggle()", AngelScript::asMETHOD(Actor,ToggleParkingBrake), AngelScript::asCALL_THISCALL); MYASSERT(result>=0);
    result = engine->RegisterObjectMethod("BeamClass", "void tractioncontrolToggle()", AngelScript::asMETHOD(Actor,ToggleTractionControl), AngelScript::asCALL_THISCALL); MYASSERT(result>=0);
    result = engine->RegisterObjectMethod("BeamClass", "void antilockbrakeToggle()", AngelScript::asMETHOD(Actor,ToggleAntiLockBrake), AngelScript::asCALL_THISCALL); MYASSERT(result>=0);
    result = engine->RegisterObjectMethod("BeamClass", "void beaconsToggle()", AngelScript::asMETHOD(Actor,ToggleBeacons), AngelScript::asCALL_THISCALL); MYASSERT(result>=0);
    result = engine->RegisterObjectMethod("BeamClass", "void setReplayMode(bool)", AngelScript::asMETHOD(Actor,setReplayMode), AngelScript::asCALL_THISCALL); MYASSERT(result>=0);
    result = engine->RegisterObjectMethod("BeamClass", "void ToggleCustomParticles()", AngelScript::asMETHOD(Actor,ToggleCustomParticles), AngelScript::asCALL_THISCALL); MYASSERT(result>=0);
    result = engine->RegisterObjectMethod("BeamClass", "int getNodeCount()", AngelScript::asMETHOD(Actor,GetNumNodes), AngelScript::asCALL_THISCALL); MYASSERT(result>=0);
    result = engine->RegisterObjectMethod("BeamClass", "float getTotalMass(bool)", AngelScript::asMETHOD(Actor,getTotalMass), AngelScript::asCALL_THISCALL); MYASSERT(result>=0);
    result = engine->RegisterObjectMethod("BeamClass", "int getWheelNodeCount()", AngelScript::asMETHOD(Actor,getWheelNodeCount), AngelScript::asCALL_THISCALL); MYASSERT(result>=0);
    result = engine->RegisterObjectMethod("BeamClass", "void setMass(float)", AngelScript::asMETHOD(Actor,setMass), AngelScript::asCALL_THISCALL); MYASSERT(result>=0);
    result = engine->RegisterObjectMethod("BeamClass", "bool getBrakeLightVisible()", AngelScript::asMETHOD(Actor,getBrakeLightVisible), AngelScript::asCALL_THISCALL); MYASSERT(result>=0);
    result = engine->RegisterObjectMethod("BeamClass", "bool getCustomLightVisible(int)", AngelScript::asMETHOD(Actor,getCustomLightVisible), AngelScript::asCALL_THISCALL); MYASSERT(result>=0);
    result = engine->RegisterObjectMethod("BeamClass", "void setCustomLightVisible(int, bool)", AngelScript::asMETHOD(Actor,setCustomLightVisible), AngelScript::asCALL_THISCALL); MYASSERT(result>=0);
    result = engine->RegisterObjectMethod("BeamClass", "bool getBeaconMode()", AngelScript::asMETHOD(Actor,getBeaconMode), AngelScript::asCALL_THISCALL); MYASSERT(result>=0);
    result = engine->RegisterObjectMethod("BeamClass", "void setBlinkType(int)", AngelScript::asMETHOD(Actor,setBlinkType), AngelScript::asCALL_THISCALL); MYASSERT(result>=0);
    result = engine->RegisterObjectMethod("BeamClass", "int getBlinkType()", AngelScript::asMETHOD(Actor,getBlinkType), AngelScript::asCALL_THISCALL); MYASSERT(result>=0);
    result = engine->RegisterObjectMethod("BeamClass", "bool getCustomParticleMode()", AngelScript::asMETHOD(Actor,getCustomParticleMode), AngelScript::asCALL_THISCALL); MYASSERT(result>=0);
    result = engine->RegisterObjectMethod("BeamClass", "bool getReverseLightVisible()", AngelScript::asMETHOD(Actor,getCustomParticleMode), AngelScript::asCALL_THISCALL); MYASSERT(result>=0);
    result = engine->RegisterObjectMethod("BeamClass", "float getHeadingDirectionAngle()", AngelScript::asMETHOD(Actor,getRotation), AngelScript::asCALL_THISCALL); MYASSERT(result>=0);
    result = engine->RegisterObjectMethod("BeamClass", "bool isLocked()", AngelScript::asMETHOD(Actor,isLocked), AngelScript::asCALL_THISCALL); MYASSERT(result>=0);
    result = engine->RegisterObjectMethod("BeamClass", "float getWheelSpeed()", AngelScript::asMETHOD(Actor,getWheelSpeed), AngelScript::asCALL_THISCALL); MYASSERT(result>=0);
    result = engine->RegisterObjectMethod("BeamClass", "float getSpeed()", AngelScript::asMETHOD(Actor,getSpeed), AngelScript::asCALL_THISCALL); MYASSERT(result>=0);
    result = engine->RegisterObjectMethod("BeamClass", "vector3 getGForces()", AngelScript::asMETHOD(Actor,GetGForcesCur), AngelScript::asCALL_THISCALL); MYASSERT(result>=0);

    result = engine->RegisterObjectMethod("BeamClass", "float getRotation()", AngelScript::asMETHOD(Actor,getRotation), AngelScript::asCALL_THISCALL); MYASSERT(result>=0);
    result = engine->RegisterObjectMethod("BeamClass", "vector3 getVehiclePosition()", AngelScript::asMETHOD(Actor,getPosition), AngelScript::asCALL_THISCALL); MYASSERT(result>=0);
    result = engine->RegisterObjectMethod("BeamClass", "vector3 getNodePosition(int)", AngelScript::asMETHOD(Actor,getNodePosition), AngelScript::asCALL_THISCALL); MYASSERT(result>=0);

    result = engine->RegisterObjectMethod("BeamClass", "VehicleAIClass @getVehicleAI()", AngelScript::asMETHOD(Actor,getVehicleAI), AngelScript::asCALL_THISCALL); MYASSERT(result>=0);
    

    /*
    // impossible to use offsetof for derived classes
    // unusable, read http://www.angelcode.com/angelscript/sdk/docs/manual/doc_adv_class_hierarchy.html
    */

    result = engine->RegisterObjectBehaviour("BeamClass", AngelScript::asBEHAVE_ADDREF, "void f()", AngelScript::asMETHOD(Actor,addRef), AngelScript::asCALL_THISCALL); MYASSERT(result>=0);
    result = engine->RegisterObjectBehaviour("BeamClass", AngelScript::asBEHAVE_RELEASE, "void f()", AngelScript::asMETHOD(Actor,release), AngelScript::asCALL_THISCALL); MYASSERT(result>=0);

    // TODO: add Vector3 classes and other utility classes!

    // class GameScript
    result = engine->RegisterObjectType("GameScriptClass", sizeof(GameScript), AngelScript::asOBJ_VALUE | AngelScript::asOBJ_POD | AngelScript::asOBJ_APP_CLASS); MYASSERT(result>=0);
    result = engine->RegisterObjectMethod("GameScriptClass", "void log(const string &in)", AngelScript::asMETHOD(GameScript,log), AngelScript::asCALL_THISCALL); MYASSERT(result>=0);
    result = engine->RegisterObjectMethod("GameScriptClass", "float getTime()", AngelScript::asMETHOD(GameScript,getTime), AngelScript::asCALL_THISCALL); MYASSERT(result>=0);
    result = engine->RegisterObjectMethod("GameScriptClass", "float rangeRandom(float, float)", AngelScript::asMETHOD(GameScript,rangeRandom), AngelScript::asCALL_THISCALL); MYASSERT(result>=0);

    result = engine->RegisterObjectMethod("GameScriptClass", "void activateAllVehicles()", AngelScript::asMETHOD(GameScript,activateAllVehicles), AngelScript::asCALL_THISCALL); MYASSERT(result>=0);
    result = engine->RegisterObjectMethod("GameScriptClass", "void setTrucksForcedActive(bool forceActive)", AngelScript::asMETHOD(GameScript,SetTrucksForcedAwake), AngelScript::asCALL_THISCALL); MYASSERT(result>=0);

    result = engine->RegisterObjectMethod("GameScriptClass", "void setBestLapTime(float time)", AngelScript::asMETHOD(GameScript,setBestLapTime), AngelScript::asCALL_THISCALL); MYASSERT(result>=0);
    result = engine->RegisterObjectMethod("GameScriptClass", "void setTimeDiff(float diff)", AngelScript::asMETHOD(GameScript,setTimeDiff), AngelScript::asCALL_THISCALL); MYASSERT(result>=0);
    result = engine->RegisterObjectMethod("GameScriptClass", "void startTimer(int id)", AngelScript::asMETHOD(GameScript,startTimer), AngelScript::asCALL_THISCALL); MYASSERT(result>=0);
    result = engine->RegisterObjectMethod("GameScriptClass", "void stopTimer()", AngelScript::asMETHOD(GameScript,stopTimer), AngelScript::asCALL_THISCALL); MYASSERT(result>=0);
    result = engine->RegisterObjectMethod("GameScriptClass", "void flashMessage(const string &in, float, float)", AngelScript::asMETHOD(GameScript,flashMessage), AngelScript::asCALL_THISCALL); MYASSERT(result>=0);
    result = engine->RegisterObjectMethod("GameScriptClass", "void message(const string &in, const string &in, float, bool)", AngelScript::asMETHOD(GameScript,message), AngelScript::asCALL_THISCALL); MYASSERT(result>=0);
    result = engine->RegisterObjectMethod("GameScriptClass", "void UpdateDirectionArrow(const string &in, vector3 &in)", AngelScript::asMETHOD(GameScript,UpdateDirectionArrow), AngelScript::asCALL_THISCALL); MYASSERT(result>=0);
    result = engine->RegisterObjectMethod("GameScriptClass", "void hideDirectionArrow()", AngelScript::asMETHOD(GameScript,hideDirectionArrow), AngelScript::asCALL_THISCALL); MYASSERT(result>=0);
    result = engine->RegisterObjectMethod("GameScriptClass", "void showChooser(const string &in, const string &in, const string &in)", AngelScript::asMETHOD(GameScript,showChooser), AngelScript::asCALL_THISCALL); MYASSERT(result>=0);
    result = engine->RegisterObjectMethod("GameScriptClass", "int getChatFontSize()", AngelScript::asMETHOD(GameScript,getChatFontSize), AngelScript::asCALL_THISCALL); MYASSERT(result>=0);
    result = engine->RegisterObjectMethod("GameScriptClass", "void setChatFontSize(int)", AngelScript::asMETHOD(GameScript,setChatFontSize), AngelScript::asCALL_THISCALL); MYASSERT(result>=0);
    
    result = engine->RegisterObjectMethod("GameScriptClass", "void loadTerrain(const string &in)", AngelScript::asMETHOD(GameScript,loadTerrain), AngelScript::asCALL_THISCALL); MYASSERT(result>=0);
    result = engine->RegisterObjectMethod("GameScriptClass", "int getLoadedTerrain(string &out)", AngelScript::asMETHOD(GameScript,getLoadedTerrain), AngelScript::asCALL_THISCALL); MYASSERT(result>=0);
    result = engine->RegisterObjectMethod("GameScriptClass", "bool getCaelumAvailable()", AngelScript::asMETHOD(GameScript,getCaelumAvailable), AngelScript::asCALL_THISCALL); MYASSERT(result>=0);
    result = engine->RegisterObjectMethod("GameScriptClass", "string getCaelumTime()", AngelScript::asMETHOD(GameScript,getCaelumTime), AngelScript::asCALL_THISCALL); MYASSERT(result>=0);
    result = engine->RegisterObjectMethod("GameScriptClass", "void setCaelumTime(float)", AngelScript::asMETHOD(GameScript,setCaelumTime), AngelScript::asCALL_THISCALL); MYASSERT(result>=0);
    result = engine->RegisterObjectMethod("GameScriptClass", "void setWaterHeight(float)", AngelScript::asMETHOD(GameScript,setWaterHeight), AngelScript::asCALL_THISCALL); MYASSERT(result>=0);
    result = engine->RegisterObjectMethod("GameScriptClass", "float getWaterHeight()", AngelScript::asMETHOD(GameScript,getWaterHeight), AngelScript::asCALL_THISCALL); MYASSERT(result>=0);
    result = engine->RegisterObjectMethod("GameScriptClass", "float getGroundHeight(vector3 &in)", AngelScript::asMETHOD(GameScript,getGroundHeight), AngelScript::asCALL_THISCALL); MYASSERT(result>=0);
    result = engine->RegisterObjectMethod("GameScriptClass", "float getGravity()", AngelScript::asMETHOD(GameScript,getGravity), AngelScript::asCALL_THISCALL); MYASSERT(result>=0);
    result = engine->RegisterObjectMethod("GameScriptClass", "void setGravity(float)", AngelScript::asMETHOD(GameScript,setGravity), AngelScript::asCALL_THISCALL); MYASSERT(result>=0);
    
    result = engine->RegisterObjectMethod("GameScriptClass", "void spawnObject(const string &in, const string &in, vector3 &in, vector3 &in, const string &in, bool)", AngelScript::asMETHOD(GameScript,spawnObject), AngelScript::asCALL_THISCALL); MYASSERT(result>=0);
    result = engine->RegisterObjectMethod("GameScriptClass", "void MoveObjectVisuals(const string &in, vector3 &in)", AngelScript::asMETHOD(GameScript,MoveTerrainObjectVisuals), AngelScript::asCALL_THISCALL); MYASSERT(result>=0);
    result = engine->RegisterObjectMethod("GameScriptClass", "void destroyObject(const string &in)", AngelScript::asMETHOD(GameScript,destroyObject), AngelScript::asCALL_THISCALL); MYASSERT(result>=0);
    result = engine->RegisterObjectMethod("GameScriptClass", "int setMaterialAmbient(const string &in, float, float, float)", AngelScript::asMETHOD(GameScript,setMaterialAmbient), AngelScript::asCALL_THISCALL); MYASSERT(result>=0);
    result = engine->RegisterObjectMethod("GameScriptClass", "int setMaterialDiffuse(const string &in, float, float, float, float)", AngelScript::asMETHOD(GameScript,setMaterialDiffuse), AngelScript::asCALL_THISCALL); MYASSERT(result>=0);
    result = engine->RegisterObjectMethod("GameScriptClass", "int setMaterialSpecular(const string &in, float, float, float, float)", AngelScript::asMETHOD(GameScript,setMaterialSpecular), AngelScript::asCALL_THISCALL); MYASSERT(result>=0);
    result = engine->RegisterObjectMethod("GameScriptClass", "int setMaterialEmissive(const string &in, float, float, float)", AngelScript::asMETHOD(GameScript,setMaterialEmissive), AngelScript::asCALL_THISCALL); MYASSERT(result>=0);
    result = engine->RegisterObjectMethod("GameScriptClass", "int setMaterialTextureName(const string &in, int, int, int, const string &in)", AngelScript::asMETHOD(GameScript,setMaterialTextureName), AngelScript::asCALL_THISCALL); MYASSERT(result>=0);
    result = engine->RegisterObjectMethod("GameScriptClass", "int setMaterialTextureRotate(const string &in, int, int, int, float)", AngelScript::asMETHOD(GameScript,setMaterialTextureRotate), AngelScript::asCALL_THISCALL); MYASSERT(result>=0);
    result = engine->RegisterObjectMethod("GameScriptClass", "int setMaterialTextureScroll(const string &in, int, int, int, float, float)", AngelScript::asMETHOD(GameScript,setMaterialTextureScroll), AngelScript::asCALL_THISCALL); MYASSERT(result>=0);
    result = engine->RegisterObjectMethod("GameScriptClass", "int setMaterialTextureScale(const string &in, int, int, int, float, float)", AngelScript::asMETHOD(GameScript,setMaterialTextureScale), AngelScript::asCALL_THISCALL); MYASSERT(result>=0);

    result = engine->RegisterObjectMethod("GameScriptClass", "void repairVehicle(const string &in, const string &in, bool)", AngelScript::asMETHOD(GameScript,repairVehicle), AngelScript::asCALL_THISCALL); MYASSERT(result>=0);
    result = engine->RegisterObjectMethod("GameScriptClass", "void removeVehicle(const string &in, const string &in)", AngelScript::asMETHOD(GameScript,removeVehicle), AngelScript::asCALL_THISCALL); MYASSERT(result>=0);
    result = engine->RegisterObjectMethod("GameScriptClass", "int getCurrentTruckNumber()", AngelScript::asMETHOD(GameScript,GetPlayerActorId), AngelScript::asCALL_THISCALL); MYASSERT(result>=0);
    result = engine->RegisterObjectMethod("GameScriptClass", "void boostCurrentTruck(float)", AngelScript::asMETHOD(GameScript, boostCurrentTruck), AngelScript::asCALL_THISCALL); MYASSERT(result>=0);
    result = engine->RegisterObjectMethod("GameScriptClass", "int getNumTrucks()", AngelScript::asMETHOD(GameScript,getNumTrucks), AngelScript::asCALL_THISCALL); MYASSERT(result>=0);
    result = engine->RegisterObjectMethod("GameScriptClass", "BeamClass @getCurrentTruck()", AngelScript::asMETHOD(GameScript,getCurrentTruck), AngelScript::asCALL_THISCALL); MYASSERT(result>=0);
    result = engine->RegisterObjectMethod("GameScriptClass", "BeamClass @getTruckByNum(int)", AngelScript::asMETHOD(GameScript,getTruckByNum), AngelScript::asCALL_THISCALL); MYASSERT(result>=0);
    result = engine->RegisterObjectMethod("GameScriptClass", "int getNumTrucksByFlag(int)", AngelScript::asMETHOD(GameScript,getNumTrucksByFlag), AngelScript::asCALL_THISCALL); MYASSERT(result>=0);
    
    result = engine->RegisterObjectMethod("GameScriptClass", "void setPersonPosition(vector3 &in)", AngelScript::asMETHOD(GameScript,setPersonPosition), AngelScript::asCALL_THISCALL); MYASSERT(result>=0);
    result = engine->RegisterObjectMethod("GameScriptClass", "vector3 getPersonPosition()", AngelScript::asMETHOD(GameScript,getPersonPosition), AngelScript::asCALL_THISCALL); MYASSERT(result>=0);
    result = engine->RegisterObjectMethod("GameScriptClass", "void setPersonRotation(radian &in)", AngelScript::asMETHOD(GameScript,setPersonRotation), AngelScript::asCALL_THISCALL); MYASSERT(result>=0);
    result = engine->RegisterObjectMethod("GameScriptClass", "radian getPersonRotation()", AngelScript::asMETHOD(GameScript,getPersonRotation), AngelScript::asCALL_THISCALL); MYASSERT(result>=0);
    
    result = engine->RegisterObjectMethod("GameScriptClass", "void setCameraPosition(vector3 &in)",  AngelScript::asMETHOD(GameScript,setCameraPosition),  AngelScript::asCALL_THISCALL); MYASSERT(result>=0);
    result = engine->RegisterObjectMethod("GameScriptClass", "void setCameraDirection(vector3 &in)", AngelScript::asMETHOD(GameScript,setCameraDirection), AngelScript::asCALL_THISCALL); MYASSERT(result>=0);
    result = engine->RegisterObjectMethod("GameScriptClass", "void setCameraYaw(float)",         AngelScript::asMETHOD(GameScript,setCameraYaw),       AngelScript::asCALL_THISCALL); MYASSERT(result>=0);
    result = engine->RegisterObjectMethod("GameScriptClass", "void setCameraPitch(float)",       AngelScript::asMETHOD(GameScript,setCameraPitch),     AngelScript::asCALL_THISCALL); MYASSERT(result>=0);
    result = engine->RegisterObjectMethod("GameScriptClass", "void setCameraRoll(float)",        AngelScript::asMETHOD(GameScript,setCameraRoll),      AngelScript::asCALL_THISCALL); MYASSERT(result>=0);
    result = engine->RegisterObjectMethod("GameScriptClass", "vector3 getCameraPosition()",      AngelScript::asMETHOD(GameScript,getCameraPosition),  AngelScript::asCALL_THISCALL); MYASSERT(result>=0);
    result = engine->RegisterObjectMethod("GameScriptClass", "vector3 getCameraDirection()",     AngelScript::asMETHOD(GameScript,getCameraDirection), AngelScript::asCALL_THISCALL); MYASSERT(result>=0);
    result = engine->RegisterObjectMethod("GameScriptClass", "void cameraLookAt(vector3 &in)",       AngelScript::asMETHOD(GameScript,cameraLookAt),       AngelScript::asCALL_THISCALL); MYASSERT(result>=0);
    result = engine->RegisterObjectMethod("GameScriptClass", "void setCameraOrientation(vector3 &in)",       AngelScript::asMETHOD(GameScript,setCameraOrientation),       AngelScript::asCALL_THISCALL); MYASSERT(result>=0);
    result = engine->RegisterObjectMethod("GameScriptClass", "vector3 getCameraOrientation()",       AngelScript::asMETHOD(GameScript,getCameraOrientation),       AngelScript::asCALL_THISCALL); MYASSERT(result>=0);

    result = engine->RegisterObjectMethod("GameScriptClass", "int addScriptFunction(const string &in)", AngelScript::asMETHOD(GameScript,addScriptFunction), AngelScript::asCALL_THISCALL); MYASSERT(result>=0);
    result = engine->RegisterObjectMethod("GameScriptClass", "int scriptFunctionExists(const string &in)", AngelScript::asMETHOD(GameScript,scriptFunctionExists), AngelScript::asCALL_THISCALL); MYASSERT(result>=0);
    result = engine->RegisterObjectMethod("GameScriptClass", "int deleteScriptFunction(const string &in)", AngelScript::asMETHOD(GameScript,deleteScriptFunction), AngelScript::asCALL_THISCALL); MYASSERT(result>=0);
    result = engine->RegisterObjectMethod("GameScriptClass", "int addScriptVariable(const string &in)", AngelScript::asMETHOD(GameScript,addScriptVariable), AngelScript::asCALL_THISCALL); MYASSERT(result>=0);
    result = engine->RegisterObjectMethod("GameScriptClass", "int deleteScriptVariable(const string &in)", AngelScript::asMETHOD(GameScript,deleteScriptVariable), AngelScript::asCALL_THISCALL); MYASSERT(result>=0);
    result = engine->RegisterObjectMethod("GameScriptClass", "void clearEventCache()", AngelScript::asMETHOD(GameScript,clearEventCache), AngelScript::asCALL_THISCALL); MYASSERT(result>=0);
    result = engine->RegisterObjectMethod("GameScriptClass", "void registerForEvent(int)", AngelScript::asMETHOD(GameScript,registerForEvent), AngelScript::asCALL_THISCALL); MYASSERT(result>=0);
    
    result = engine->RegisterObjectMethod("GameScriptClass", "int sendGameCmd(const string &in)", AngelScript::asMETHOD(GameScript,sendGameCmd), AngelScript::asCALL_THISCALL); MYASSERT(result>=0);
    result = engine->RegisterObjectMethod("GameScriptClass", "int useOnlineAPI(const string &in, const dictionary &in, string &out)", AngelScript::asMETHOD(GameScript,useOnlineAPI), AngelScript::asCALL_THISCALL); MYASSERT(result>=0);

    result = engine->RegisterObjectMethod("GameScriptClass", "VehicleAIClass @getCurrentTruckAI()", AngelScript::asMETHOD(GameScript, getCurrentTruckAI), AngelScript::asCALL_THISCALL); MYASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "VehicleAIClass @getTruckAIByNum(int)", AngelScript::asMETHOD(GameScript, getTruckAIByNum), AngelScript::asCALL_THISCALL); MYASSERT(result >= 0);
    
    result = engine->RegisterObjectMethod("GameScriptClass", "void showMessageBox(string &in, string &in, bool button1, string &in, bool AllowClose, bool button2,string &in)", AngelScript::asMETHOD(GameScript, showMessageBox), AngelScript::asCALL_THISCALL); MYASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "BeamClass @spawnTruck(string &in, vector3 &in, vector3 &in, const string&in = string())", AngelScript::asMETHOD(GameScript, spawnTruck), AngelScript::asCALL_THISCALL); MYASSERT(result >= 0);
    
    result = engine->RegisterObjectMethod("GameScriptClass", "float getFPS()", AngelScript::asMETHOD(GameScript, getFPS), AngelScript::asCALL_THISCALL); MYASSERT(result >= 0);
    
    result = engine->RegisterObjectMethod("GameScriptClass", "void backToMenu()", AngelScript::asMETHOD(GameScript, backToMenu), AngelScript::asCALL_THISCALL); MYASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "void quitGame()", AngelScript::asMETHOD(GameScript, quitGame), AngelScript::asCALL_THISCALL); MYASSERT(result >= 0);


    
    // enum scriptEvents
    result = engine->RegisterEnum("scriptEvents"); MYASSERT(result>=0);
    result = engine->RegisterEnumValue("scriptEvents", "SE_COLLISION_BOX_ENTER", SE_COLLISION_BOX_ENTER); MYASSERT(result>=0);
    result = engine->RegisterEnumValue("scriptEvents", "SE_COLLISION_BOX_LEAVE", SE_COLLISION_BOX_LEAVE); MYASSERT(result>=0);
    result = engine->RegisterEnumValue("scriptEvents", "SE_TRUCK_ENTER", SE_TRUCK_ENTER); MYASSERT(result>=0);
    result = engine->RegisterEnumValue("scriptEvents", "SE_TRUCK_EXIT", SE_TRUCK_EXIT); MYASSERT(result>=0);
    result = engine->RegisterEnumValue("scriptEvents", "SE_TRUCK_ENGINE_DIED", SE_TRUCK_ENGINE_DIED); MYASSERT(result>=0);
    result = engine->RegisterEnumValue("scriptEvents", "SE_TRUCK_ENGINE_FIRE", SE_TRUCK_ENGINE_FIRE); MYASSERT(result>=0);
    result = engine->RegisterEnumValue("scriptEvents", "SE_TRUCK_TOUCHED_WATER", SE_TRUCK_TOUCHED_WATER); MYASSERT(result>=0);
    result = engine->RegisterEnumValue("scriptEvents", "SE_TRUCK_BEAM_BROKE", SE_TRUCK_BEAM_BROKE); MYASSERT(result>=0);
    result = engine->RegisterEnumValue("scriptEvents", "SE_TRUCK_LOCKED", SE_TRUCK_LOCKED); MYASSERT(result>=0);
    result = engine->RegisterEnumValue("scriptEvents", "SE_TRUCK_UNLOCKED", SE_TRUCK_UNLOCKED); MYASSERT(result>=0);
    result = engine->RegisterEnumValue("scriptEvents", "SE_TRUCK_LIGHT_TOGGLE", SE_TRUCK_LIGHT_TOGGLE); MYASSERT(result>=0);
    result = engine->RegisterEnumValue("scriptEvents", "SE_TRUCK_SKELETON_TOGGLE", SE_TRUCK_SKELETON_TOGGLE); MYASSERT(result>=0);
    result = engine->RegisterEnumValue("scriptEvents", "SE_TRUCK_TIE_TOGGLE", SE_TRUCK_TIE_TOGGLE); MYASSERT(result>=0);
    result = engine->RegisterEnumValue("scriptEvents", "SE_TRUCK_PARKINGBREAK_TOGGLE", SE_TRUCK_PARKINGBREAK_TOGGLE); MYASSERT(result>=0);
    result = engine->RegisterEnumValue("scriptEvents", "SE_TRUCK_RESET", SE_TRUCK_RESET); MYASSERT(result>=0);
    result = engine->RegisterEnumValue("scriptEvents", "SE_TRUCK_TELEPORT", SE_TRUCK_TELEPORT); MYASSERT(result>=0);
    result = engine->RegisterEnumValue("scriptEvents", "SE_TRUCK_BEACONS_TOGGLE", SE_TRUCK_BEACONS_TOGGLE); MYASSERT(result>=0);
    result = engine->RegisterEnumValue("scriptEvents", "SE_TRUCK_CPARTICLES_TOGGLE", SE_TRUCK_CPARTICLES_TOGGLE); MYASSERT(result>=0);
    result = engine->RegisterEnumValue("scriptEvents", "SE_TRUCK_GROUND_CONTACT_CHANGED", SE_TRUCK_GROUND_CONTACT_CHANGED); MYASSERT(result>=0);
    result = engine->RegisterEnumValue("scriptEvents", "SE_GENERIC_NEW_TRUCK", SE_GENERIC_NEW_TRUCK); MYASSERT(result>=0);
    result = engine->RegisterEnumValue("scriptEvents", "SE_GENERIC_DELETED_TRUCK", SE_GENERIC_DELETED_TRUCK); MYASSERT(result>=0);
    result = engine->RegisterEnumValue("scriptEvents", "SE_GENERIC_INPUT_EVENT", SE_GENERIC_INPUT_EVENT); MYASSERT(result>=0);
    result = engine->RegisterEnumValue("scriptEvents", "SE_GENERIC_MOUSE_BEAM_INTERACTION", SE_GENERIC_MOUSE_BEAM_INTERACTION); MYASSERT(result>=0);
    result = engine->RegisterEnumValue("scriptEvents", "SE_ANGELSCRIPT_MANIPULATIONS", SE_ANGELSCRIPT_MANIPULATIONS); MYASSERT(result>=0);
    result = engine->RegisterEnumValue("scriptEvents", "SE_GENERIC_MESSAGEBOX_CLICK", SE_GENERIC_MESSAGEBOX_CLICK); MYASSERT(result>=0);
    result = engine->RegisterEnumValue("scriptEvents", "SE_ALL_EVENTS", SE_ALL_EVENTS); MYASSERT(result>=0);
    
    // enum truckStates
    result = engine->RegisterEnum("truckStates"); MYASSERT(result>=0);
    result = engine->RegisterEnumValue("truckStates", "TS_SIMULATED", static_cast<int>(Actor::SimState::LOCAL_SIMULATED)); MYASSERT(result>=0);
    result = engine->RegisterEnumValue("truckStates", "TS_SLEEPING",  static_cast<int>(Actor::SimState::LOCAL_SLEEPING)); MYASSERT(result>=0);
    result = engine->RegisterEnumValue("truckStates", "TS_NETWORKED", static_cast<int>(Actor::SimState::NETWORKED_OK)); MYASSERT(result>=0);

    // enum truckTypes
    result = engine->RegisterEnum("truckTypes"); MYASSERT(result>=0);
    result = engine->RegisterEnumValue("truckTypes", "TT_NOT_DRIVEABLE", NOT_DRIVEABLE); MYASSERT(result>=0);
    result = engine->RegisterEnumValue("truckTypes", "TT_TRUCK", TRUCK); MYASSERT(result>=0);
    result = engine->RegisterEnumValue("truckTypes", "TT_AIRPLANE", AIRPLANE); MYASSERT(result>=0);
    result = engine->RegisterEnumValue("truckTypes", "TT_BOAT", BOAT); MYASSERT(result>=0);
    result = engine->RegisterEnumValue("truckTypes", "TT_MACHINE", MACHINE); MYASSERT(result>=0);
    result = engine->RegisterEnumValue("truckTypes", "TT_AI", AI); MYASSERT(result>=0);

    // now the global instances
    GameScript *gamescript = new GameScript(this);
    result = engine->RegisterGlobalProperty("GameScriptClass game", gamescript); MYASSERT(result>=0);
    //result = engine->RegisterGlobalProperty("CacheSystemClass cache", &CacheSystem::getSingleton()); MYASSERT(result>=0);

    SLOG("Type registrations done. If you see no error above everything should be working");
}

void ScriptEngine::msgCallback(const AngelScript::asSMessageInfo *msg)
{
    const char *type = "Error";
    if ( msg->type == AngelScript::asMSGTYPE_INFORMATION )
        type = "Info";
    else if ( msg->type == AngelScript::asMSGTYPE_WARNING )
        type = "Warning";

    char tmp[1024]="";
    sprintf(tmp, "%s (%d, %d): %s = %s", msg->section, msg->row, msg->col, type, msg->message);
    SLOG(tmp);
}

int ScriptEngine::framestep(Real dt)
{
    // Check if we need to execute any strings
    std::vector<String> tmpQueue;
    stringExecutionQueue.pull(tmpQueue);
    std::vector<String>::iterator it;
    for (it=tmpQueue.begin(); it!=tmpQueue.end();it++)
    {
        executeString(*it);
    }

    // framestep stuff below
    if (frameStepFunctionPtr==nullptr) return 1;
    if (!engine) return 0;
    if (!context) context = engine->CreateContext();
    context->Prepare(frameStepFunctionPtr);

    // Set the function arguments
    context->SetArgFloat(0, dt);

    int r = context->Execute();
    if ( r == AngelScript::asEXECUTION_FINISHED )
    {
      // The return value is only valid if the execution finished successfully
        AngelScript::asDWORD ret = context->GetReturnDWord();
    }
    return 0;
}

int ScriptEngine::fireEvent(std::string instanceName, float intensity)
{
    if (!engine)
        return 0;

    AngelScript::asIScriptModule *mod = engine->GetModule(moduleName, AngelScript::asGM_CREATE_IF_NOT_EXISTS);
    AngelScript::asIScriptFunction* func = mod->GetFunctionByDecl("void fireEvent(string, float)"); // TODO: this shouldn't be hard coded --neorej16
    if (func == nullptr)
        return 0; // TODO: This function returns 0 no matter what - WTF? ~ only_a_ptr, 08/2017

    if (!context)
        context = engine->CreateContext();

    context->Prepare(func);

    // Set the function arguments
    std::string *instance_name = new std::string(instanceName);
    context->SetArgObject(0, &instanceName);
    context->SetArgFloat (1, intensity);

    int r = context->Execute();
    if ( r == AngelScript::asEXECUTION_FINISHED )
    {
      // The return value is only valid if the execution finished successfully
        AngelScript::asDWORD ret = context->GetReturnDWord();
    }
    delete(instance_name);

    return 0;
}

int ScriptEngine::envokeCallback(int functionId, eventsource_t *source, node_t *node, int type)
{
    if (!engine)
        return 0; // TODO: this function returns 0 no matter what - WTF? ~ only_a_ptr, 08/2017

    if (functionId <= 0 && (defaultEventCallbackFunctionPtr != nullptr))
    {
        // use the default event handler instead then
        functionId = defaultEventCallbackFunctionPtr->GetId();
    }
    else if (functionId <= 0)
    {
        // no default callback available, discard the event
        return 0;
    }
    if (!context)
        context = engine->CreateContext();

    context->Prepare(engine->GetFunctionById(functionId));

    // Set the function arguments
    std::string *instance_name = new std::string(source->instancename);
    std::string *boxname = new std::string(source->boxname);
    context->SetArgDWord (0, type);
    context->SetArgObject(1, instance_name);
    context->SetArgObject(2, boxname);
    if (node)
        context->SetArgDWord (3, node->pos);
    else
        context->SetArgDWord (3, -1); // conversion from 'int' to 'AngelScript::asDWORD', signed/unsigned mismatch!

    int r = context->Execute();
    if ( r == AngelScript::asEXECUTION_FINISHED )
    {
      // The return value is only valid if the execution finished successfully
        AngelScript::asDWORD ret = context->GetReturnDWord();
    }
    delete(instance_name);
    delete(boxname);

    return 0;
}

void ScriptEngine::queueStringForExecution(const String command)
{
    stringExecutionQueue.push(command);
}

int ScriptEngine::executeString(String command)
{
    if (!engine)
        return 1;

    if (!context)
        context = engine->CreateContext();

    AngelScript::asIScriptModule *mod = engine->GetModule(moduleName, AngelScript::asGM_CREATE_IF_NOT_EXISTS);
    int result = ExecuteString(engine, command.c_str(), mod, context);
    if (result < 0)
    {
        SLOG("error " + TOSTRING(result) + " while executing string: " + command + ".");
    }
    return result;
}

int ScriptEngine::addFunction(const String &arg)
{
    if (!engine)
        return 1;

    if (!context)
        context = engine->CreateContext();

    AngelScript::asIScriptModule *mod = engine->GetModule(moduleName, AngelScript::asGM_CREATE_IF_NOT_EXISTS);

    AngelScript::asIScriptFunction *func = 0;
    int r = mod->CompileFunction("addfunc", arg.c_str(), 0, AngelScript::asCOMP_ADD_TO_MODULE, &func);
    
    if ( r < 0 )
    {
        char tmp[512] = "";
        snprintf(tmp, 512, "An error occurred while trying to add a function ('%s') to script module '%s'.", arg.c_str(), moduleName);
        SLOG(tmp);
    }
    else
    {
        // successfully added function; Check if we added a "special" function

        if (func == mod->GetFunctionByDecl("void frameStep(float)"))
        {
            if (frameStepFunctionPtr == nullptr)
                frameStepFunctionPtr = func;
        }
        else if (func == mod->GetFunctionByDecl("void eventCallback(int, int)"))
        {
            if (eventCallbackFunctionPtr == nullptr)
                eventCallbackFunctionPtr = func;
        }
        else if (func == mod->GetFunctionByDecl("void defaultEventCallback(int, string, string, int)"))
        {
            if (defaultEventCallbackFunctionPtr == nullptr)
                defaultEventCallbackFunctionPtr = func;
        }
    }

    // We must release the function object
    if ( func )
        func->Release();

    return r;
}

int ScriptEngine::functionExists(const String &arg)
{
    if (!engine) // WTF? If the scripting engine failed to start, how would it invoke this function?
        return -1; // ... OK, I guess the author wanted the fn. to be usable both within script and C++, but IMO that's bad design (generally good, but bad for a game.. bad for RoR), really ~ only_a_ptr, 09/2017

    if (!context)
        context = engine->CreateContext(); // Same as above, I don't think this is a good design ~ only_a_ptr, 09/2017

    AngelScript::asIScriptModule *mod = engine->GetModule(moduleName, AngelScript::asGM_ONLY_IF_EXISTS);

    if (mod == 0)
    {
        return AngelScript::asNO_FUNCTION; // Nope, it's an internal error, not a "function not found" case ~ only_a_ptr, 09/2017
    }
    else
    {
        AngelScript::asIScriptFunction* fn = mod->GetFunctionByDecl(arg.c_str());
        if (fn != nullptr)
            return fn->GetId();
        else
            return AngelScript::asNO_FUNCTION;
    }
}

int ScriptEngine::deleteFunction(const String &arg)
{
    if (!engine)
        return AngelScript::asERROR;

    if (!context)
        context = engine->CreateContext();

    AngelScript::asIScriptModule *mod = engine->GetModule(moduleName, AngelScript::asGM_ONLY_IF_EXISTS);

    if ( mod == 0 || mod->GetFunctionCount() == 0 )
    {
        char tmp[512] = "";
        sprintf(tmp, "An error occurred while trying to remove a function ('%s') from script module '%s': No functions have been added (and consequently: the function does not exist).", arg.c_str(), moduleName);
        SLOG(tmp);
        return AngelScript::asNO_FUNCTION;
    }

    AngelScript::asIScriptFunction* func = mod->GetFunctionByDecl(arg.c_str());
    if (func != nullptr)
    {
        // Warning: The function is not destroyed immediately, only when no more references point to it.
        mod->RemoveFunction(func);

        // Since functions can be recursive, we'll call the garbage
        // collector to make sure the object is really freed
        engine->GarbageCollect();

        // Check if we removed a "special" function

        if ( frameStepFunctionPtr == func )
            frameStepFunctionPtr = nullptr;

        if ( eventCallbackFunctionPtr == func )
            eventCallbackFunctionPtr = nullptr;

        if ( defaultEventCallbackFunctionPtr == func )
            defaultEventCallbackFunctionPtr = nullptr;

        return func->GetId();
    }
    else
    {
        char tmp[512] = "";
        sprintf(tmp, "An error occurred while trying to remove a function ('%s') from script module '%s'.", arg.c_str(), moduleName);
        SLOG(tmp);
        return AngelScript::asERROR;
    }
}

int ScriptEngine::addVariable(const String &arg)
{
    if (!engine) return 1;
    if (!context) context = engine->CreateContext();
    AngelScript::asIScriptModule *mod = engine->GetModule(moduleName, AngelScript::asGM_CREATE_IF_NOT_EXISTS);

    int r = mod->CompileGlobalVar("addvar", arg.c_str(), 0);
    if ( r < 0 )
    {
        char tmp[512] = "";
        sprintf(tmp, "An error occurred while trying to add a variable ('%s') to script module '%s'.", arg.c_str(), moduleName);
        SLOG(tmp);
    }

    return r;
}

int ScriptEngine::deleteVariable(const String &arg)
{
    if (!engine) return 1;
    if (!context) context = engine->CreateContext();
    AngelScript::asIScriptModule *mod = engine->GetModule(moduleName, AngelScript::asGM_ONLY_IF_EXISTS);

    if ( mod == 0 || mod->GetGlobalVarCount() == 0 )
    {
        char tmp[512] = "";
        sprintf(tmp, "An error occurred while trying to remove a variable ('%s') from script module '%s': No variables have been added (and consequently: the variable does not exist).", arg.c_str(), moduleName);
        SLOG(tmp);
        return AngelScript::asNO_GLOBAL_VAR;
    }

    int index = mod->GetGlobalVarIndexByName(arg.c_str());
    if ( index >= 0 )
    {
        index = mod->RemoveGlobalVar(index);
    }
    else
    {
        char tmp[512] = "";
        sprintf(tmp, "An error occurred while trying to remove a variable ('%s') from script module '%s'.", arg.c_str(), moduleName);
        SLOG(tmp);
    }

    return index;
}

void ScriptEngine::triggerEvent(int eventnum, int value)
{
    if (!engine) return;
    if (eventCallbackFunctionPtr==nullptr) return;
    if (eventMask & eventnum)
    {
        // script registered for that event, so sent it
        if (!context) context = engine->CreateContext();
        context->Prepare(eventCallbackFunctionPtr);

        // Set the function arguments
        context->SetArgDWord(0, eventnum);
        context->SetArgDWord(1, value);

        int r = context->Execute();
        if ( r == AngelScript::asEXECUTION_FINISHED )
        {
          // The return value is only valid if the execution finished successfully
            AngelScript::asDWORD ret = context->GetReturnDWord();
        }
        return;
    }
}

int ScriptEngine::loadScript(String _scriptName)
{
    scriptName = _scriptName;

    // Load the entire script file into the buffer
    int result=0;

    // The builder is a helper class that will load the script file,
    // search for #include directives, and load any included files as
    // well.
    OgreScriptBuilder builder;

    AngelScript::asIScriptModule *mod = 0;

    result = builder.StartNewModule(engine, moduleName);
    if ( result < 0 )
    {
        SLOG("Failed to start new module");
        return result;
    }

    mod = engine->GetModule(moduleName, AngelScript::asGM_ONLY_IF_EXISTS);

    result = builder.AddSectionFromFile(scriptName.c_str());
    if ( result < 0 )
    {
        SLOG("Unkown error while loading script file: "+scriptName);
        SLOG("Failed to add script file");
        return result;
    }
    result = builder.BuildModule();
    if ( result < 0 )
    {
        SLOG("Failed to build the module");
        return result;
    }

    scriptHash = builder.GetHash();

    // get some other optional functions
    frameStepFunctionPtr = mod->GetFunctionByDecl("void frameStep(float)");

    eventCallbackFunctionPtr = mod->GetFunctionByDecl("void eventCallback(int, int)");

    defaultEventCallbackFunctionPtr = mod->GetFunctionByDecl("void defaultEventCallback(int, string, string, int)");

    // Find the function that is to be called.
    auto main_func = mod->GetFunctionByDecl("void main()");
    if ( main_func == nullptr )
    {
        // The function couldn't be found. Instruct the script writer to include the
        // expected function in the script.
        SLOG("The script should have the function 'void main()'.");
        return 0;
    }

    // Create our context, prepare it, and then execute
    context = engine->CreateContext();

    // Prepare the script context with the function we wish to execute. Prepare()
    // must be called on the context before each new script function that will be
    // executed. Note, that if you intend to execute the same function several
    // times, it might be a good idea to store the function id returned by
    // GetFunctionIDByDecl(), so that this relatively slow call can be skipped.
    result = context->Prepare(main_func);
    if (result < 0)
    {
        SLOG("Failed to prepare the context.");
        context->Release();
        return -1;
    }

    SLOG("Executing main()");
    result = context->Execute();
    if ( result != AngelScript::asEXECUTION_FINISHED )
    {
        // The execution didn't complete as expected. Determine what happened.
        if ( result == AngelScript::asEXECUTION_ABORTED )
        {
            SLOG("The script was aborted before it could finish. Probably it timed out.");
        }
        else if ( result == AngelScript::asEXECUTION_EXCEPTION )
        {
            // An exception occurred, let the script writer know what happened so it can be corrected.
            SLOG("An exception '" + String(context->GetExceptionString()) + "' occurred. Please correct the code in file '" + scriptName + "' and try again.");

            // Write some information about the script exception
            AngelScript::asIScriptFunction* func = context->GetExceptionFunction();
            SLOG("func: " + String(func->GetDeclaration()));
            SLOG("modl: " + String(func->GetModuleName()));
            SLOG("sect: " + String(func->GetScriptSectionName()));
            SLOG("line: " + TOSTRING(context->GetExceptionLineNumber()));
            SLOG("desc: " + String(context->GetExceptionString()));
        }
        else
        {
            SLOG("The script ended for some unforeseen reason " + TOSTRING(result));
        }
    }
    else
    {
        SLOG("The script finished successfully.");
    }

    return 0;
}


StringVector ScriptEngine::getAutoComplete(String command)
{
    StringVector result;
    if (!engine) return result;
    if (!context) context = engine->CreateContext();
    AngelScript::asIScriptModule *mod = engine->GetModule(moduleName, AngelScript::asGM_CREATE_IF_NOT_EXISTS);

    for (unsigned int i = 0; i < mod->GetGlobalVarCount(); i++)
    {
        const char *name = mod->GetGlobalVarDeclaration(i);
        result.push_back(String(name));
    }

    for (unsigned int i = 0; i < mod->GetFunctionCount(); i++)
    {
        AngelScript::asIScriptFunction *desc = mod->GetFunctionByIndex(i);
        result.push_back(String(desc->GetName()));
    }


    for (unsigned int i = 0; i < engine->GetGlobalPropertyCount(); i++)
    {
        const char *name;

        if (!engine->GetGlobalPropertyByIndex(i, &name))
        {
            result.push_back(String(name));
        }

    }

    if (!command.empty())
    {
        StringVector res2;
        // now check if we hit anything
        for (unsigned int i = 0; i < result.size(); i++)
        {
            if (result[i].substr(0, command.size()) == command)
            {
                res2.push_back(result[i]);
            }
        }

        if (command.find(".") != command.npos)
        {
            // oh, property of an object D:
            // TODO
        }

        // give out filtered result
        return res2;
    }

    return result;
}

void ScriptEngine::activateLogging()
{
    scriptLog->addListener(this);
}
