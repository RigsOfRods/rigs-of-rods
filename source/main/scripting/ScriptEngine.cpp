/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2013-2020 Petr Ohlidal

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
#include "Actor.h"
#include "ActorManager.h"
#include "Collisions.h"
#include "Console.h"
#include "GameContext.h"
#include "GameScript.h"
#include "LocalStorage.h"
#include "OgreScriptBuilder.h"
#include "PlatformUtils.h"
#include "ScriptEvents.h"
#include "VehicleAI.h"

#include "InputEngine.h"

using namespace Ogre;
using namespace RoR;

// some hacky functions

void logString(const std::string &str)
{
    App::GetScriptEngine()->SLOG(str);
}

// the class implementation

ScriptEngine::ScriptEngine() :
     context(0)
    , engine(0)
    , scriptLog(0)
{
    scriptLog = LogManager::getSingleton().createLog(PathCombine(App::sys_logs_dir->getStr(), "Angelscript.log"), false);
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
    RoR::App::GetConsole()->forwardLogMessage(Console::CONSOLE_MSGTYPE_SCRIPT, message, lml);
}

// continue with initializing everything
void ScriptEngine::init()
{
    SLOG("ScriptEngine initializing ...");
    int result;

    // Create the script engine
    engine = AngelScript::asCreateScriptEngine(ANGELSCRIPT_VERSION);

    engine->SetEngineProperty(AngelScript::asEP_ALLOW_UNSAFE_REFERENCES, true); // Needed for ImGui

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
    RegisterOgreObjects(engine);

    // Register the local storage object.
    // This needs to be done after the registration of the ogre objects!
    registerLocalStorage(engine);

    registerInputEngine(engine);

    RegisterImGuiBindings(engine);

    // some useful global functions
    result = engine->RegisterGlobalFunction("void log(const string &in)", AngelScript::asFUNCTION(logString), AngelScript::asCALL_CDECL); ROR_ASSERT( result >= 0 );
    result = engine->RegisterGlobalFunction("void print(const string &in)", AngelScript::asFUNCTION(logString), AngelScript::asCALL_CDECL); ROR_ASSERT( result >= 0 );

    result = engine->RegisterObjectType("BeamFactoryClass", sizeof(ActorManager), AngelScript::asOBJ_REF); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("BeamFactoryClass", "void setSimulationSpeed(float)", AngelScript::asMETHOD(ActorManager,SetSimulationSpeed), AngelScript::asCALL_THISCALL); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectBehaviour("BeamFactoryClass", AngelScript::asBEHAVE_ADDREF, "void f()", AngelScript::asMETHOD(ActorManager,AddRef), AngelScript::asCALL_THISCALL); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectBehaviour("BeamFactoryClass", AngelScript::asBEHAVE_RELEASE, "void f()", AngelScript::asMETHOD(ActorManager,Release), AngelScript::asCALL_THISCALL); ROR_ASSERT(result>=0);

    // enum aiEvents
    result = engine->RegisterEnum("aiEvents"); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("aiEvents", "AI_LIGHTSTOGGLE", AI_LIGHTSTOGGLE); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("aiEvents", "AI_WAIT_SECONDS", AI_WAIT_SECONDS); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("aiEvents", "AI_BEACONSTOGGLE", AI_BEACONSTOGGLE); ROR_ASSERT(result >= 0);

    // enum aiEvents
    result = engine->RegisterEnum("AiValues"); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("AiValues", "AI_SPEED", AI_SPEED); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("AiValues", "AI_POWER", AI_POWER); ROR_ASSERT(result >= 0);

    result = engine->RegisterObjectType("VehicleAIClass", sizeof(VehicleAI), AngelScript::asOBJ_REF); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("VehicleAIClass", "void addWaypoint(string &in, vector3 &in)", AngelScript::asMETHOD(VehicleAI, AddWaypoint), AngelScript::asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("VehicleAIClass", "void addWaypoints(dictionary &in)", AngelScript::asMETHOD(VehicleAI, AddWaypoint), AngelScript::asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("VehicleAIClass", "void setActive(bool)", AngelScript::asMETHOD(VehicleAI, SetActive), AngelScript::asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("VehicleAIClass", "void addEvent(string &in,int &in)", AngelScript::asMETHOD(VehicleAI, AddEvent), AngelScript::asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("VehicleAIClass", "void setValueAtWaypoint(string &in, int &in, float &in)", AngelScript::asMETHOD(VehicleAI, SetValueAtWaypoint), AngelScript::asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectBehaviour("VehicleAIClass", AngelScript::asBEHAVE_ADDREF, "void f()", AngelScript::asMETHOD(VehicleAI, addRef), AngelScript::asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectBehaviour("VehicleAIClass", AngelScript::asBEHAVE_RELEASE, "void f()", AngelScript::asMETHOD(VehicleAI, release), AngelScript::asCALL_THISCALL); ROR_ASSERT(result >= 0);

    // class CVar
    result = engine->RegisterObjectType("CVarClass", sizeof(Console), AngelScript::asOBJ_REF | AngelScript::asOBJ_NOCOUNT); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("CVarClass", "const string& getName()", AngelScript::asMETHOD(CVar,getName), AngelScript::asCALL_THISCALL); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("CVarClass", "const string& getStr()", AngelScript::asMETHOD(CVar,getStr), AngelScript::asCALL_THISCALL); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("CVarClass", "int getInt()", AngelScript::asMETHOD(CVar,getInt), AngelScript::asCALL_THISCALL); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("CVarClass", "float getFloat()", AngelScript::asMETHOD(CVar,getFloat), AngelScript::asCALL_THISCALL); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("CVarClass", "bool getBool()", AngelScript::asMETHOD(CVar,getBool), AngelScript::asCALL_THISCALL); ROR_ASSERT(result>=0);

    // enum CVarFlags
    result = engine->RegisterEnum("CVarFlags"); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("CVarFlags", "CVAR_TYPE_BOOL", CVAR_TYPE_BOOL); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("CVarFlags", "CVAR_TYPE_INT", CVAR_TYPE_INT); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("CVarFlags", "CVAR_TYPE_FLOAT", CVAR_TYPE_FLOAT); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("CVarFlags", "CVAR_ARCHIVE", CVAR_ARCHIVE); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("CVarFlags", "CVAR_NO_LOG", CVAR_NO_LOG); ROR_ASSERT(result >= 0);

    // class Console
    result = engine->RegisterObjectType("ConsoleClass", sizeof(Console), AngelScript::asOBJ_REF | AngelScript::asOBJ_NOCOUNT); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("ConsoleClass", "CVarClass @cVarCreate(const string &in, const string &in, int, const string &in)", AngelScript::asMETHOD(Console,cVarCreate), AngelScript::asCALL_THISCALL); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("ConsoleClass", "CVarClass @cVarFind(const string &in)", AngelScript::asMETHOD(Console,cVarFind), AngelScript::asCALL_THISCALL); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("ConsoleClass", "CVarClass @cVarGet(const string &in, int)", AngelScript::asMETHOD(Console,cVarGet), AngelScript::asCALL_THISCALL); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("ConsoleClass", "CVarClass @cVarSet(const string &in, const string &in)", AngelScript::asMETHOD(Console,cVarSet), AngelScript::asCALL_THISCALL); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("ConsoleClass", "void cVarAssign(CVarClass@, const string &in)", AngelScript::asMETHOD(Console,cVarAssign), AngelScript::asCALL_THISCALL); ROR_ASSERT(result>=0);

    // class Actor (historically Beam)
    result = engine->RegisterObjectType("BeamClass", sizeof(Actor), AngelScript::asOBJ_REF); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("BeamClass", "void scaleTruck(float)", AngelScript::asMETHOD(Actor,scaleTruck), AngelScript::asCALL_THISCALL); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("BeamClass", "string getTruckName()", AngelScript::asMETHOD(Actor,getTruckName), AngelScript::asCALL_THISCALL); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("BeamClass", "string getTruckFileName()", AngelScript::asMETHOD(Actor,getTruckFileName), AngelScript::asCALL_THISCALL); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("BeamClass", "string getSectionConfig()", AngelScript::asMETHOD(Actor, getSectionConfig), AngelScript::asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("BeamClass", "int  getTruckType()", AngelScript::asMETHOD(Actor,getTruckType), AngelScript::asCALL_THISCALL); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("BeamClass", "void reset(bool)", AngelScript::asMETHOD(Actor,reset), AngelScript::asCALL_THISCALL); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("BeamClass", "void parkingbrakeToggle()", AngelScript::asMETHOD(Actor,parkingbrakeToggle), AngelScript::asCALL_THISCALL); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("BeamClass", "void tractioncontrolToggle()", AngelScript::asMETHOD(Actor,tractioncontrolToggle), AngelScript::asCALL_THISCALL); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("BeamClass", "void antilockbrakeToggle()", AngelScript::asMETHOD(Actor,antilockbrakeToggle), AngelScript::asCALL_THISCALL); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("BeamClass", "void beaconsToggle()", AngelScript::asMETHOD(Actor,beaconsToggle), AngelScript::asCALL_THISCALL); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("BeamClass", "void toggleCustomParticles()", AngelScript::asMETHOD(Actor,toggleCustomParticles), AngelScript::asCALL_THISCALL); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("BeamClass", "int getNodeCount()", AngelScript::asMETHOD(Actor,getNodeCount), AngelScript::asCALL_THISCALL); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("BeamClass", "float getTotalMass(bool)", AngelScript::asMETHOD(Actor,getTotalMass), AngelScript::asCALL_THISCALL); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("BeamClass", "int getWheelNodeCount()", AngelScript::asMETHOD(Actor,getWheelNodeCount), AngelScript::asCALL_THISCALL); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("BeamClass", "void setMass(float)", AngelScript::asMETHOD(Actor,setMass), AngelScript::asCALL_THISCALL); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("BeamClass", "bool getBrakeLightVisible()", AngelScript::asMETHOD(Actor,getBrakeLightVisible), AngelScript::asCALL_THISCALL); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("BeamClass", "bool getCustomLightVisible(int)", AngelScript::asMETHOD(Actor,getCustomLightVisible), AngelScript::asCALL_THISCALL); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("BeamClass", "void setCustomLightVisible(int, bool)", AngelScript::asMETHOD(Actor,setCustomLightVisible), AngelScript::asCALL_THISCALL); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("BeamClass", "bool getBeaconMode()", AngelScript::asMETHOD(Actor,getBeaconMode), AngelScript::asCALL_THISCALL); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("BeamClass", "void setBlinkType(int)", AngelScript::asMETHOD(Actor,setBlinkType), AngelScript::asCALL_THISCALL); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("BeamClass", "int getBlinkType()", AngelScript::asMETHOD(Actor,getBlinkType), AngelScript::asCALL_THISCALL); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("BeamClass", "bool getCustomParticleMode()", AngelScript::asMETHOD(Actor,getCustomParticleMode), AngelScript::asCALL_THISCALL); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("BeamClass", "bool getReverseLightVisible()", AngelScript::asMETHOD(Actor,getCustomParticleMode), AngelScript::asCALL_THISCALL); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("BeamClass", "float getHeadingDirectionAngle()", AngelScript::asMETHOD(Actor,getRotation), AngelScript::asCALL_THISCALL); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("BeamClass", "bool isLocked()", AngelScript::asMETHOD(Actor,isLocked), AngelScript::asCALL_THISCALL); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("BeamClass", "float getWheelSpeed()", AngelScript::asMETHOD(Actor,getWheelSpeed), AngelScript::asCALL_THISCALL); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("BeamClass", "float getSpeed()", AngelScript::asMETHOD(Actor,getSpeed), AngelScript::asCALL_THISCALL); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("BeamClass", "vector3 getGForces()", AngelScript::asMETHOD(Actor,getGForces), AngelScript::asCALL_THISCALL); ROR_ASSERT(result>=0);

    result = engine->RegisterObjectMethod("BeamClass", "float getRotation()", AngelScript::asMETHOD(Actor,getRotation), AngelScript::asCALL_THISCALL); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("BeamClass", "vector3 getVehiclePosition()", AngelScript::asMETHOD(Actor,getPosition), AngelScript::asCALL_THISCALL); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("BeamClass", "vector3 getNodePosition(int)", AngelScript::asMETHOD(Actor,getNodePosition), AngelScript::asCALL_THISCALL); ROR_ASSERT(result>=0);

    result = engine->RegisterObjectMethod("BeamClass", "VehicleAIClass @getVehicleAI()", AngelScript::asMETHOD(Actor,getVehicleAI), AngelScript::asCALL_THISCALL); ROR_ASSERT(result>=0);
    

    /*
    // impossible to use offsetof for derived classes
    // unusable, read http://www.angelcode.com/angelscript/sdk/docs/manual/doc_adv_class_hierarchy.html
    */

    result = engine->RegisterObjectBehaviour("BeamClass", AngelScript::asBEHAVE_ADDREF, "void f()", AngelScript::asMETHOD(Actor,addRef), AngelScript::asCALL_THISCALL); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectBehaviour("BeamClass", AngelScript::asBEHAVE_RELEASE, "void f()", AngelScript::asMETHOD(Actor,release), AngelScript::asCALL_THISCALL); ROR_ASSERT(result>=0);

    // TODO: add Vector3 classes and other utility classes!

    // class GameScript
    result = engine->RegisterObjectType("GameScriptClass", sizeof(GameScript), AngelScript::asOBJ_VALUE | AngelScript::asOBJ_POD | AngelScript::asOBJ_APP_CLASS); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("GameScriptClass", "void log(const string &in)", AngelScript::asMETHOD(GameScript,log), AngelScript::asCALL_THISCALL); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("GameScriptClass", "float getTime()", AngelScript::asMETHOD(GameScript,getTime), AngelScript::asCALL_THISCALL); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("GameScriptClass", "float rangeRandom(float, float)", AngelScript::asMETHOD(GameScript,rangeRandom), AngelScript::asCALL_THISCALL); ROR_ASSERT(result>=0);

    result = engine->RegisterObjectMethod("GameScriptClass", "void activateAllVehicles()", AngelScript::asMETHOD(GameScript,activateAllVehicles), AngelScript::asCALL_THISCALL); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("GameScriptClass", "void setTrucksForcedActive(bool forceActive)", AngelScript::asMETHOD(GameScript,SetTrucksForcedAwake), AngelScript::asCALL_THISCALL); ROR_ASSERT(result>=0);

    result = engine->RegisterObjectMethod("GameScriptClass", "void setBestLapTime(float time)", AngelScript::asMETHOD(GameScript,setBestLapTime), AngelScript::asCALL_THISCALL); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("GameScriptClass", "void setTimeDiff(float diff)", AngelScript::asMETHOD(GameScript,setTimeDiff), AngelScript::asCALL_THISCALL); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("GameScriptClass", "void startTimer(int id)", AngelScript::asMETHOD(GameScript,startTimer), AngelScript::asCALL_THISCALL); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("GameScriptClass", "void stopTimer()", AngelScript::asMETHOD(GameScript,stopTimer), AngelScript::asCALL_THISCALL); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("GameScriptClass", "void flashMessage(const string &in, float, float)", AngelScript::asMETHOD(GameScript,flashMessage), AngelScript::asCALL_THISCALL); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("GameScriptClass", "void message(const string &in, const string &in, float, bool)", AngelScript::asMETHOD(GameScript,message), AngelScript::asCALL_THISCALL); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("GameScriptClass", "void UpdateDirectionArrow(const string &in, vector3 &in)", AngelScript::asMETHOD(GameScript,UpdateDirectionArrow), AngelScript::asCALL_THISCALL); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("GameScriptClass", "void hideDirectionArrow()", AngelScript::asMETHOD(GameScript,hideDirectionArrow), AngelScript::asCALL_THISCALL); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("GameScriptClass", "void showChooser(const string &in, const string &in, const string &in)", AngelScript::asMETHOD(GameScript,showChooser), AngelScript::asCALL_THISCALL); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("GameScriptClass", "int getChatFontSize()", AngelScript::asMETHOD(GameScript,getChatFontSize), AngelScript::asCALL_THISCALL); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("GameScriptClass", "void setChatFontSize(int)", AngelScript::asMETHOD(GameScript,setChatFontSize), AngelScript::asCALL_THISCALL); ROR_ASSERT(result>=0);
    
    result = engine->RegisterObjectMethod("GameScriptClass", "void loadTerrain(const string &in)", AngelScript::asMETHOD(GameScript,loadTerrain), AngelScript::asCALL_THISCALL); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("GameScriptClass", "int getLoadedTerrain(string &out)", AngelScript::asMETHOD(GameScript,getLoadedTerrain), AngelScript::asCALL_THISCALL); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("GameScriptClass", "bool getCaelumAvailable()", AngelScript::asMETHOD(GameScript,getCaelumAvailable), AngelScript::asCALL_THISCALL); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("GameScriptClass", "string getCaelumTime()", AngelScript::asMETHOD(GameScript,getCaelumTime), AngelScript::asCALL_THISCALL); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("GameScriptClass", "void setCaelumTime(float)", AngelScript::asMETHOD(GameScript,setCaelumTime), AngelScript::asCALL_THISCALL); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("GameScriptClass", "void setWaterHeight(float)", AngelScript::asMETHOD(GameScript,setWaterHeight), AngelScript::asCALL_THISCALL); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("GameScriptClass", "float getWaterHeight()", AngelScript::asMETHOD(GameScript,getWaterHeight), AngelScript::asCALL_THISCALL); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("GameScriptClass", "float getGroundHeight(vector3 &in)", AngelScript::asMETHOD(GameScript,getGroundHeight), AngelScript::asCALL_THISCALL); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("GameScriptClass", "float getGravity()", AngelScript::asMETHOD(GameScript,getGravity), AngelScript::asCALL_THISCALL); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("GameScriptClass", "void setGravity(float)", AngelScript::asMETHOD(GameScript,setGravity), AngelScript::asCALL_THISCALL); ROR_ASSERT(result>=0);
    
    result = engine->RegisterObjectMethod("GameScriptClass", "void spawnObject(const string &in, const string &in, vector3 &in, vector3 &in, const string &in, bool)", AngelScript::asMETHOD(GameScript,spawnObject), AngelScript::asCALL_THISCALL); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("GameScriptClass", "void MoveObjectVisuals(const string &in, vector3 &in)", AngelScript::asMETHOD(GameScript,MoveTerrainObjectVisuals), AngelScript::asCALL_THISCALL); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("GameScriptClass", "void destroyObject(const string &in)", AngelScript::asMETHOD(GameScript,destroyObject), AngelScript::asCALL_THISCALL); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("GameScriptClass", "int setMaterialAmbient(const string &in, float, float, float)", AngelScript::asMETHOD(GameScript,setMaterialAmbient), AngelScript::asCALL_THISCALL); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("GameScriptClass", "int setMaterialDiffuse(const string &in, float, float, float, float)", AngelScript::asMETHOD(GameScript,setMaterialDiffuse), AngelScript::asCALL_THISCALL); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("GameScriptClass", "int setMaterialSpecular(const string &in, float, float, float, float)", AngelScript::asMETHOD(GameScript,setMaterialSpecular), AngelScript::asCALL_THISCALL); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("GameScriptClass", "int setMaterialEmissive(const string &in, float, float, float)", AngelScript::asMETHOD(GameScript,setMaterialEmissive), AngelScript::asCALL_THISCALL); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("GameScriptClass", "int setMaterialTextureName(const string &in, int, int, int, const string &in)", AngelScript::asMETHOD(GameScript,setMaterialTextureName), AngelScript::asCALL_THISCALL); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("GameScriptClass", "int setMaterialTextureRotate(const string &in, int, int, int, float)", AngelScript::asMETHOD(GameScript,setMaterialTextureRotate), AngelScript::asCALL_THISCALL); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("GameScriptClass", "int setMaterialTextureScroll(const string &in, int, int, int, float, float)", AngelScript::asMETHOD(GameScript,setMaterialTextureScroll), AngelScript::asCALL_THISCALL); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("GameScriptClass", "int setMaterialTextureScale(const string &in, int, int, int, float, float)", AngelScript::asMETHOD(GameScript,setMaterialTextureScale), AngelScript::asCALL_THISCALL); ROR_ASSERT(result>=0);

    result = engine->RegisterObjectMethod("GameScriptClass", "void repairVehicle(const string &in, const string &in, bool)", AngelScript::asMETHOD(GameScript,repairVehicle), AngelScript::asCALL_THISCALL); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("GameScriptClass", "void removeVehicle(const string &in, const string &in)", AngelScript::asMETHOD(GameScript,removeVehicle), AngelScript::asCALL_THISCALL); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("GameScriptClass", "int getCurrentTruckNumber()", AngelScript::asMETHOD(GameScript,GetPlayerActorId), AngelScript::asCALL_THISCALL); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("GameScriptClass", "void boostCurrentTruck(float)", AngelScript::asMETHOD(GameScript, boostCurrentTruck), AngelScript::asCALL_THISCALL); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("GameScriptClass", "int getNumTrucks()", AngelScript::asMETHOD(GameScript,getNumTrucks), AngelScript::asCALL_THISCALL); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("GameScriptClass", "BeamClass @getCurrentTruck()", AngelScript::asMETHOD(GameScript,getCurrentTruck), AngelScript::asCALL_THISCALL); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("GameScriptClass", "BeamClass @getTruckByNum(int)", AngelScript::asMETHOD(GameScript,getTruckByNum), AngelScript::asCALL_THISCALL); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("GameScriptClass", "int getNumTrucksByFlag(int)", AngelScript::asMETHOD(GameScript,getNumTrucksByFlag), AngelScript::asCALL_THISCALL); ROR_ASSERT(result>=0);
    
    result = engine->RegisterObjectMethod("GameScriptClass", "void setPersonPosition(vector3 &in)", AngelScript::asMETHOD(GameScript,setPersonPosition), AngelScript::asCALL_THISCALL); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("GameScriptClass", "vector3 getPersonPosition()", AngelScript::asMETHOD(GameScript,getPersonPosition), AngelScript::asCALL_THISCALL); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("GameScriptClass", "void setPersonRotation(radian &in)", AngelScript::asMETHOD(GameScript,setPersonRotation), AngelScript::asCALL_THISCALL); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("GameScriptClass", "radian getPersonRotation()", AngelScript::asMETHOD(GameScript,getPersonRotation), AngelScript::asCALL_THISCALL); ROR_ASSERT(result>=0);
    
    result = engine->RegisterObjectMethod("GameScriptClass", "void setCameraPosition(vector3 &in)",  AngelScript::asMETHOD(GameScript,setCameraPosition),  AngelScript::asCALL_THISCALL); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("GameScriptClass", "void setCameraDirection(vector3 &in)", AngelScript::asMETHOD(GameScript,setCameraDirection), AngelScript::asCALL_THISCALL); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("GameScriptClass", "void setCameraYaw(float)",         AngelScript::asMETHOD(GameScript,setCameraYaw),       AngelScript::asCALL_THISCALL); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("GameScriptClass", "void setCameraPitch(float)",       AngelScript::asMETHOD(GameScript,setCameraPitch),     AngelScript::asCALL_THISCALL); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("GameScriptClass", "void setCameraRoll(float)",        AngelScript::asMETHOD(GameScript,setCameraRoll),      AngelScript::asCALL_THISCALL); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("GameScriptClass", "vector3 getCameraPosition()",      AngelScript::asMETHOD(GameScript,getCameraPosition),  AngelScript::asCALL_THISCALL); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("GameScriptClass", "vector3 getCameraDirection()",     AngelScript::asMETHOD(GameScript,getCameraDirection), AngelScript::asCALL_THISCALL); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("GameScriptClass", "void cameraLookAt(vector3 &in)",       AngelScript::asMETHOD(GameScript,cameraLookAt),       AngelScript::asCALL_THISCALL); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("GameScriptClass", "void setCameraOrientation(vector3 &in)",       AngelScript::asMETHOD(GameScript,setCameraOrientation),       AngelScript::asCALL_THISCALL); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("GameScriptClass", "vector3 getCameraOrientation()",       AngelScript::asMETHOD(GameScript,getCameraOrientation),       AngelScript::asCALL_THISCALL); ROR_ASSERT(result>=0);

    result = engine->RegisterObjectMethod("GameScriptClass", "int addScriptFunction(const string &in)", AngelScript::asMETHOD(GameScript,addScriptFunction), AngelScript::asCALL_THISCALL); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("GameScriptClass", "int scriptFunctionExists(const string &in)", AngelScript::asMETHOD(GameScript,scriptFunctionExists), AngelScript::asCALL_THISCALL); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("GameScriptClass", "int deleteScriptFunction(const string &in)", AngelScript::asMETHOD(GameScript,deleteScriptFunction), AngelScript::asCALL_THISCALL); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("GameScriptClass", "int addScriptVariable(const string &in)", AngelScript::asMETHOD(GameScript,addScriptVariable), AngelScript::asCALL_THISCALL); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("GameScriptClass", "int deleteScriptVariable(const string &in)", AngelScript::asMETHOD(GameScript,deleteScriptVariable), AngelScript::asCALL_THISCALL); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("GameScriptClass", "void clearEventCache()", AngelScript::asMETHOD(GameScript,clearEventCache), AngelScript::asCALL_THISCALL); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("GameScriptClass", "void registerForEvent(int)", AngelScript::asMETHOD(GameScript,registerForEvent), AngelScript::asCALL_THISCALL); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("GameScriptClass", "void unRegisterEvent(int)", AngelScript::asMETHOD(GameScript,unRegisterEvent), AngelScript::asCALL_THISCALL); ROR_ASSERT(result>=0);
    
    result = engine->RegisterObjectMethod("GameScriptClass", "int sendGameCmd(const string &in)", AngelScript::asMETHOD(GameScript,sendGameCmd), AngelScript::asCALL_THISCALL); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("GameScriptClass", "int useOnlineAPI(const string &in, const dictionary &in, string &out)", AngelScript::asMETHOD(GameScript,useOnlineAPI), AngelScript::asCALL_THISCALL); ROR_ASSERT(result>=0);

    result = engine->RegisterObjectMethod("GameScriptClass", "VehicleAIClass @getCurrentTruckAI()", AngelScript::asMETHOD(GameScript, getCurrentTruckAI), AngelScript::asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "VehicleAIClass @getTruckAIByNum(int)", AngelScript::asMETHOD(GameScript, getTruckAIByNum), AngelScript::asCALL_THISCALL); ROR_ASSERT(result >= 0);
    
    result = engine->RegisterObjectMethod("GameScriptClass", "void showMessageBox(string &in, string &in, bool button1, string &in, bool AllowClose, bool button2,string &in)", AngelScript::asMETHOD(GameScript, showMessageBox), AngelScript::asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "BeamClass @spawnTruck(string &in, vector3 &in, vector3 &in)", AngelScript::asMETHOD(GameScript, spawnTruck), AngelScript::asCALL_THISCALL); ROR_ASSERT(result >= 0);
    
    result = engine->RegisterObjectMethod("GameScriptClass", "float getFPS()", AngelScript::asMETHOD(GameScript, getFPS), AngelScript::asCALL_THISCALL); ROR_ASSERT(result >= 0);
    
    result = engine->RegisterObjectMethod("GameScriptClass", "void backToMenu()", AngelScript::asMETHOD(GameScript, backToMenu), AngelScript::asCALL_THISCALL); ROR_ASSERT(result >= 0);
    result = engine->RegisterObjectMethod("GameScriptClass", "void quitGame()", AngelScript::asMETHOD(GameScript, quitGame), AngelScript::asCALL_THISCALL); ROR_ASSERT(result >= 0);


    
    // enum scriptEvents
    result = engine->RegisterEnum("scriptEvents"); ROR_ASSERT(result>=0);
    result = engine->RegisterEnumValue("scriptEvents", "SE_COLLISION_BOX_ENTER", SE_COLLISION_BOX_ENTER); ROR_ASSERT(result>=0);
    result = engine->RegisterEnumValue("scriptEvents", "SE_COLLISION_BOX_LEAVE", SE_COLLISION_BOX_LEAVE); ROR_ASSERT(result>=0);
    result = engine->RegisterEnumValue("scriptEvents", "SE_TRUCK_ENTER", SE_TRUCK_ENTER); ROR_ASSERT(result>=0);
    result = engine->RegisterEnumValue("scriptEvents", "SE_TRUCK_EXIT", SE_TRUCK_EXIT); ROR_ASSERT(result>=0);
    result = engine->RegisterEnumValue("scriptEvents", "SE_TRUCK_ENGINE_DIED", SE_TRUCK_ENGINE_DIED); ROR_ASSERT(result>=0);
    result = engine->RegisterEnumValue("scriptEvents", "SE_TRUCK_ENGINE_FIRE", SE_TRUCK_ENGINE_FIRE); ROR_ASSERT(result>=0);
    result = engine->RegisterEnumValue("scriptEvents", "SE_TRUCK_TOUCHED_WATER", SE_TRUCK_TOUCHED_WATER); ROR_ASSERT(result>=0);
    result = engine->RegisterEnumValue("scriptEvents", "SE_TRUCK_BEAM_BROKE", SE_TRUCK_BEAM_BROKE); ROR_ASSERT(result>=0);
    result = engine->RegisterEnumValue("scriptEvents", "SE_TRUCK_LOCKED", SE_TRUCK_LOCKED); ROR_ASSERT(result>=0);
    result = engine->RegisterEnumValue("scriptEvents", "SE_TRUCK_UNLOCKED", SE_TRUCK_UNLOCKED); ROR_ASSERT(result>=0);
    result = engine->RegisterEnumValue("scriptEvents", "SE_TRUCK_LIGHT_TOGGLE", SE_TRUCK_LIGHT_TOGGLE); ROR_ASSERT(result>=0);
    result = engine->RegisterEnumValue("scriptEvents", "SE_TRUCK_SKELETON_TOGGLE", SE_TRUCK_SKELETON_TOGGLE); ROR_ASSERT(result>=0);
    result = engine->RegisterEnumValue("scriptEvents", "SE_TRUCK_TIE_TOGGLE", SE_TRUCK_TIE_TOGGLE); ROR_ASSERT(result>=0);
    result = engine->RegisterEnumValue("scriptEvents", "SE_TRUCK_PARKINGBREAK_TOGGLE", SE_TRUCK_PARKINGBREAK_TOGGLE); ROR_ASSERT(result>=0);
    result = engine->RegisterEnumValue("scriptEvents", "SE_TRUCK_RESET", SE_TRUCK_RESET); ROR_ASSERT(result>=0);
    result = engine->RegisterEnumValue("scriptEvents", "SE_TRUCK_TELEPORT", SE_TRUCK_TELEPORT); ROR_ASSERT(result>=0);
    result = engine->RegisterEnumValue("scriptEvents", "SE_TRUCK_BEACONS_TOGGLE", SE_TRUCK_BEACONS_TOGGLE); ROR_ASSERT(result>=0);
    result = engine->RegisterEnumValue("scriptEvents", "SE_TRUCK_CPARTICLES_TOGGLE", SE_TRUCK_CPARTICLES_TOGGLE); ROR_ASSERT(result>=0);
    result = engine->RegisterEnumValue("scriptEvents", "SE_TRUCK_GROUND_CONTACT_CHANGED", SE_TRUCK_GROUND_CONTACT_CHANGED); ROR_ASSERT(result>=0);
    result = engine->RegisterEnumValue("scriptEvents", "SE_GENERIC_NEW_TRUCK", SE_GENERIC_NEW_TRUCK); ROR_ASSERT(result>=0);
    result = engine->RegisterEnumValue("scriptEvents", "SE_GENERIC_DELETED_TRUCK", SE_GENERIC_DELETED_TRUCK); ROR_ASSERT(result>=0);
    result = engine->RegisterEnumValue("scriptEvents", "SE_GENERIC_INPUT_EVENT", SE_GENERIC_INPUT_EVENT); ROR_ASSERT(result>=0);
    result = engine->RegisterEnumValue("scriptEvents", "SE_GENERIC_MOUSE_BEAM_INTERACTION", SE_GENERIC_MOUSE_BEAM_INTERACTION); ROR_ASSERT(result>=0);
    result = engine->RegisterEnumValue("scriptEvents", "SE_ANGELSCRIPT_MANIPULATIONS", SE_ANGELSCRIPT_MANIPULATIONS); ROR_ASSERT(result>=0);
    result = engine->RegisterEnumValue("scriptEvents", "SE_GENERIC_MESSAGEBOX_CLICK", SE_GENERIC_MESSAGEBOX_CLICK); ROR_ASSERT(result>=0);
    result = engine->RegisterEnumValue("scriptEvents", "SE_ALL_EVENTS", SE_ALL_EVENTS); ROR_ASSERT(result>=0);
    
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

    // now the global instances
    result = engine->RegisterGlobalProperty("GameScriptClass game", &m_game_script); ROR_ASSERT(result>=0);
    result = engine->RegisterGlobalProperty("ConsoleClass console", App::GetConsole()); ROR_ASSERT(result>=0);
    result = engine->RegisterGlobalProperty("InputEngineClass inputs", App::GetInputEngine()); ROR_ASSERT(result>=0);

    SLOG("Type registrations done. If you see no error above everything should be working");

    context = engine->CreateContext();
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
    if (!engine || !context) return 0;

    for (size_t i = 0; i < m_script_units.size(); i++)
    {
        if (!m_script_units[i].frameStepFunctionPtr)
        {
            continue;
        }

        context->Prepare(m_script_units[i].frameStepFunctionPtr);

        // Set the function arguments
        context->SetArgFloat(0, dt);

        m_currently_executing_script_unit = (int)i;
        int r = context->Execute();
        m_currently_executing_script_unit = -1;
        if ( r == AngelScript::asEXECUTION_FINISHED )
        {
            // The return value is only valid if the execution finished successfully
            AngelScript::asDWORD ret = context->GetReturnDWord();
        }
    }
    return 0;
}

int ScriptEngine::fireEvent(std::string instanceName, float intensity)
{
    if (!engine || !context)
        return 0;

    for (size_t i = 0; i < m_script_units.size(); i++)
    {
        AngelScript::asIScriptFunction* func = m_script_units[i].scriptModule->GetFunctionByDecl(
            "void fireEvent(string, float)"); // TODO: this shouldn't be hard coded --neorej16

        context->Prepare(func);

        // Set the function arguments
        context->SetArgObject(0, &instanceName);
        context->SetArgFloat (1, intensity);

        m_currently_executing_script_unit = (int)i;
        int r = context->Execute();
        m_currently_executing_script_unit = -1;
        if ( r == AngelScript::asEXECUTION_FINISHED )
        {
          // The return value is only valid if the execution finished successfully
            AngelScript::asDWORD ret = context->GetReturnDWord();
        }
    }

    return 0;
}

int ScriptEngine::envokeCallback(int _functionId, eventsource_t *source, node_t *node, int type)
{
    if (!engine || !context)
        return 0; // TODO: this function returns 0 no matter what - WTF? ~ only_a_ptr, 08/2017

    for (size_t i = 0; i < m_script_units.size(); i++)
    {
        int functionId = _functionId;
        if (functionId <= 0 && (m_script_units[i].defaultEventCallbackFunctionPtr != nullptr))
        {
            // use the default event handler instead then
            functionId = m_script_units[i].defaultEventCallbackFunctionPtr->GetId();
        }
        else if (functionId <= 0)
        {
            // no default callback available, discard the event
            return 0;
        }

        context->Prepare(engine->GetFunctionById(functionId));

        // Set the function arguments
        std::string instance_name(source->instancename);
        std::string boxname = (source->boxname);
        context->SetArgDWord (0, type);
        context->SetArgObject(1, &instance_name);
        context->SetArgObject(2, &boxname);
        if (node)
            context->SetArgDWord (3, static_cast<AngelScript::asDWORD>(node->pos));
        else
            context->SetArgDWord (3, static_cast<AngelScript::asDWORD>(-1));

        m_currently_executing_script_unit = (int)i;
        int r = context->Execute();
        m_currently_executing_script_unit = -1;

        if ( r == AngelScript::asEXECUTION_FINISHED )
        {
            // The return value is only valid if the execution finished successfully
            AngelScript::asDWORD ret = context->GetReturnDWord();
        }
    }

    return 0;
}

void ScriptEngine::queueStringForExecution(const String command)
{
    stringExecutionQueue.push(command);
}

int ScriptEngine::executeString(String command)
{
    if (!engine || !context)
        return 1;

    // Only works with terrain script module (classic behavior)
    if (m_terrain_script_unit < 0)
        return 1;

    AngelScript::asIScriptModule *mod = m_script_units[m_terrain_script_unit].scriptModule;
    int result = ExecuteString(engine, command.c_str(), mod, context);
    if (result < 0)
    {
        SLOG("error " + TOSTRING(result) + " while executing string: " + command + ".");
    }
    return result;
}

int ScriptEngine::addFunction(const String &arg)
{
    if (!engine || !context)
        return 1;

    if (!context)
        

    // Only works with terrain script module (classic behavior)
    if (m_terrain_script_unit < 0)
        return 1;

    AngelScript::asIScriptModule *mod = m_script_units[m_terrain_script_unit].scriptModule;

    AngelScript::asIScriptFunction *func = 0;
    int r = mod->CompileFunction("addfunc", arg.c_str(), 0, AngelScript::asCOMP_ADD_TO_MODULE, &func);
    
    if ( r < 0 )
    {
        char tmp[512] = "";
        snprintf(tmp, 512, "An error occurred while trying to add a function ('%s') to script module '%s'.", arg.c_str(), mod->GetName());
        SLOG(tmp);
    }
    else
    {
        // successfully added function; Check if we added a "special" function

        if (func == mod->GetFunctionByDecl("void frameStep(float)"))
        {
            if (m_script_units[m_terrain_script_unit].frameStepFunctionPtr == nullptr)
                m_script_units[m_terrain_script_unit].frameStepFunctionPtr = func;
        }
        else if (func == mod->GetFunctionByDecl("void eventCallback(int, int)"))
        {
            if (m_script_units[m_terrain_script_unit].eventCallbackFunctionPtr == nullptr)
                m_script_units[m_terrain_script_unit].eventCallbackFunctionPtr = func;
        }
        else if (func == mod->GetFunctionByDecl("void defaultEventCallback(int, string, string, int)"))
        {
            if (m_script_units[m_terrain_script_unit].defaultEventCallbackFunctionPtr == nullptr)
                m_script_units[m_terrain_script_unit].defaultEventCallbackFunctionPtr = func;
        }
    }

    // We must release the function object
    if ( func )
        func->Release();

    return r;
}

int ScriptEngine::functionExists(const String &arg)
{
    if (!engine || !context) // WTF? If the scripting engine failed to start, how would it invoke this function?
        return -1; // ... OK, I guess the author wanted the fn. to be usable both within script and C++, but IMO that's bad design (generally good, but bad for a game.. bad for RoR), really ~ only_a_ptr, 09/2017

    // Only works with terrain script module (classic behavior)
    if (m_terrain_script_unit < 0)
        return -1;

    AngelScript::asIScriptModule *mod = m_script_units[m_terrain_script_unit].scriptModule;

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
    if (!engine || !context)
        return AngelScript::asERROR;

    // Only works with terrain script module (classic behavior)
    if (m_terrain_script_unit < 0)
        return -1;

    AngelScript::asIScriptModule *mod = m_script_units[m_terrain_script_unit].scriptModule;

    if ( mod->GetFunctionCount() == 0 )
    {
        char tmp[512] = "";
        sprintf(tmp, "An error occurred while trying to remove a function ('%s') from script module '%s': No functions have been added (and consequently: the function does not exist).", arg.c_str(), mod->GetName());
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

        if ( m_script_units[m_terrain_script_unit].frameStepFunctionPtr == func )
            m_script_units[m_terrain_script_unit].frameStepFunctionPtr = nullptr;

        if ( m_script_units[m_terrain_script_unit].eventCallbackFunctionPtr == func )
            m_script_units[m_terrain_script_unit].eventCallbackFunctionPtr = nullptr;

        if ( m_script_units[m_terrain_script_unit].defaultEventCallbackFunctionPtr == func )
            m_script_units[m_terrain_script_unit].defaultEventCallbackFunctionPtr = nullptr;

        return func->GetId();
    }
    else
    {
        char tmp[512] = "";
        sprintf(tmp, "An error occurred while trying to remove a function ('%s') from script module '%s'.", arg.c_str(), mod->GetName());
        SLOG(tmp);
        return AngelScript::asERROR;
    }
}

int ScriptEngine::addVariable(const String &arg)
{
    if (!engine || !context) return 1;
    // Only works with terrain script module (classic behavior)
    if (m_terrain_script_unit < 0)
        return 1;

    AngelScript::asIScriptModule *mod = m_script_units[m_terrain_script_unit].scriptModule;

    int r = mod->CompileGlobalVar("addvar", arg.c_str(), 0);
    if ( r < 0 )
    {
        char tmp[512] = "";
        sprintf(tmp, "An error occurred while trying to add a variable ('%s') to script module '%s'.", arg.c_str(), mod->GetName());
        SLOG(tmp);
    }

    return r;
}

int ScriptEngine::deleteVariable(const String &arg)
{
    if (!engine || !context) return 1;
    // Only works with terrain script module (classic behavior)
    if (m_terrain_script_unit < 0)
        return 1;
    AngelScript::asIScriptModule *mod = m_script_units[m_terrain_script_unit].scriptModule;

    if ( mod == 0 || mod->GetGlobalVarCount() == 0 )
    {
        char tmp[512] = "";
        sprintf(tmp, "An error occurred while trying to remove a variable ('%s') from script module '%s': No variables have been added (and consequently: the variable does not exist).", arg.c_str(), mod->GetName());
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
        sprintf(tmp, "An error occurred while trying to remove a variable ('%s') from script module '%s'.", arg.c_str(), mod->GetName());
        SLOG(tmp);
    }

    return index;
}

void ScriptEngine::triggerEvent(int eventnum, int value)
{
    if (!engine || !context) return;

    for (size_t i = 0; i < m_script_units.size(); i++)
    {
        if (m_script_units[i].eventCallbackFunctionPtr==nullptr)
            continue;
        if (m_script_units[i].eventMask & eventnum)
        {
            // script registered for that event, so sent it
            context->Prepare(m_script_units[i].eventCallbackFunctionPtr);

            // Set the function arguments
            context->SetArgDWord(0, eventnum);
            context->SetArgDWord(1, value);

            m_currently_executing_script_unit = (int)i;
            int r = context->Execute();
            m_currently_executing_script_unit = -1;
            if ( r == AngelScript::asEXECUTION_FINISHED )
            {
                // The return value is only valid if the execution finished successfully
                AngelScript::asDWORD ret = context->GetReturnDWord();
            }
        }
    }
}

String ScriptEngine::composeModuleName(String const& scriptName, ScriptCategory origin)
{
    switch (origin)
    {
    case ScriptCategory::TERRAIN: return fmt::format("TERRAIN:{}", scriptName);

    case ScriptCategory::CUSTOM: return fmt::format("CUSTOM:{}", scriptName);

    default: return "";
    }
}

int ScriptEngine::loadScript(String scriptName, ScriptCategory category/* = ScriptCategory::TERRAIN*/)
{
    // This function creates a new script unit, tries to set it up and removes it if setup fails.
    // -----------------------------------------------------------------------------------------
    // A script unit is how Rigs of Rods organizes scripts from various sources.
    // Because the script is executed during loading, it's wrapping unit must
    // be created early, and removed if setup fails.
    int unit_id = (int)m_script_units.size();
    m_script_units.resize(m_script_units.size() + 1);
    m_script_units[unit_id].scriptName = scriptName;
    m_script_units[unit_id].scriptCategory = category;
    if (category == ScriptCategory::TERRAIN)
    {
        m_terrain_script_unit = (int)m_script_units.size() - 1;
    }    

    // Perform the actual script loading, building and running main().
    int result = this->setupScriptUnit(unit_id);

    // If setup failed, remove the unit.
    if (result != 0)
    {
        m_script_units.pop_back();
        if (category == ScriptCategory::TERRAIN)
        {
            m_terrain_script_unit = -1;
        } 
    }

    return result;
}

int ScriptEngine::setupScriptUnit(int unit_id)
{
    int result=0;

    String moduleName = this->composeModuleName(
        m_script_units[unit_id].scriptName, m_script_units[unit_id].scriptCategory);
    if (moduleName == "")
        return -1;

    // The builder is a helper class that will load the script file,
    // search for #include directives, and load any included files as
    // well.
    OgreScriptBuilder builder;

    // A script module is how AngelScript organizes scripts.
    // It contains the script loaded by user plus all `#include`-d scripts.
    result = builder.StartNewModule(engine, moduleName.c_str());
    if ( result < 0 )
    {
        App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_ERROR,
            fmt::format("Could not load script '{}' - failed to create module.", moduleName));
        return result;
    }
    m_script_units[unit_id].scriptModule = engine->GetModule(moduleName.c_str(), AngelScript::asGM_ONLY_IF_EXISTS);

    // Load the script from the file system.
    result = builder.AddSectionFromFile(m_script_units[unit_id].scriptName.c_str());
    if ( result < 0 )
    {
        App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_ERROR,
            fmt::format("Could not load script '{}' - failed to process file.", moduleName));
        return result;
    }

    // Build the AngelScript module - this loads `#include`-d scripts
    // and runs any global statements, for example constructors of
    // global objects like raceManager in 'races.as'. For this reason,
    // the game must already be aware of the script, but only temporarily.
    m_currently_executing_script_unit = unit_id; // for `BuildModule()` below.
    result = builder.BuildModule();
    m_currently_executing_script_unit = -1; // Tidy up.
    if ( result < 0 )
    {
        App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_ERROR,
            fmt::format("Could not load script '{}' - failed to build module.", moduleName));
        return result;
    }

    String scriptHash;
    if (m_script_units[unit_id].scriptCategory == ScriptCategory::TERRAIN) // Classic behavior
        scriptHash = builder.GetHash();

    // get some other optional functions
    m_script_units[unit_id].frameStepFunctionPtr = m_script_units[unit_id].scriptModule->GetFunctionByDecl("void frameStep(float)");

    m_script_units[unit_id].eventCallbackFunctionPtr = m_script_units[unit_id].scriptModule->GetFunctionByDecl("void eventCallback(int, int)");

    m_script_units[unit_id].defaultEventCallbackFunctionPtr = m_script_units[unit_id].scriptModule->GetFunctionByDecl("void defaultEventCallback(int, string, string, int)");

    // Find the function that is to be called.
    auto main_func = m_script_units[unit_id].scriptModule->GetFunctionByDecl("void main()");
    if ( main_func == nullptr )
    {
        // The function couldn't be found. Instruct the script writer to include the
        // expected function in the script.
        App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_ERROR,
            fmt::format("Could not load script '{}' - there is no function `main()`", moduleName));
        return 0;
    }

    // Prepare the script context with the function we wish to execute. Prepare()
    // must be called on the context before each new script function that will be
    // executed. Note, that if you intend to execute the same function several
    // times, it might be a good idea to store the function id returned by
    // GetFunctionIDByDecl(), so that this relatively slow call can be skipped.
    result = context->Prepare(main_func);
    if (result < 0)
    {
        App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_ERROR,
            fmt::format("Could not load script '{}' - failed to build module.", moduleName));
        context->Release();
        return -1;
    }

    // Execute the `main()` function in the script.
    // The function must have full access to the game API.
    SLOG(fmt::format("Executing main() in {}", moduleName));
    m_currently_executing_script_unit = unit_id; // for `Execute()` below.
    result = context->Execute();
    m_currently_executing_script_unit = -1; // Tidy up.
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
            SLOG("An exception '" + String(context->GetExceptionString()) 
            + "' occurred. Please correct the code in file '" 
            + m_script_units[unit_id].scriptName + "' and try again.");

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

        App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_ERROR,
            fmt::format("Could not load script '{}' - error running function `main()`, check AngelScript.log", moduleName));
    }
    else
    {
        SLOG("The script finished successfully.");
    }

    return 0;
}

void ScriptEngine::unloadScript(String scriptName, ScriptCategory category)
{
    ROR_ASSERT(m_currently_executing_script_unit == -1);

    String module_name = this->composeModuleName(scriptName, category);
    if (module_name == "")
        return;

    for (size_t i = 0; i < m_script_units.size(); i++)
    {
        if (m_script_units[i].scriptModule->GetName() == module_name)
        {
            m_script_units.erase(m_script_units.begin() + i);
            if (i == (size_t)m_terrain_script_unit)
            {
                m_terrain_script_unit = -1;
            }
        }
    }
}

void ScriptEngine::activateLogging()
{
    scriptLog->addListener(this);
}
