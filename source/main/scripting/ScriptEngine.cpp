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
// created on 24th of February 2009 by Thomas Fischer
#include "ScriptEngine.h"

// AS addons start
#include "scriptstdstring/scriptstdstring.h"
#include "scriptmath/scriptmath.h"
#include "contextmgr/contextmgr.h"
#include "scriptany/scriptany.h"
#include "scriptarray/scriptarray.h"
#include "scripthelper/scripthelper.h"
// AS addons end

#ifdef USE_CURL
#define CURL_STATICLIB
#include <stdio.h>
#include <curl/curl.h>
//#include <curl/types.h>
#include <curl/easy.h>
#endif //USE_CURL

#include "Application.h"
#include "OgreAngelscript.h"
#include "Beam.h"
#include "Collisions.h"
#include "Console.h"
#include "LocalStorage.h"
#include "Settings.h"
#include "Application.h"
#include "OgreSubsystem.h"
#include "GameScript.h"
#include "OgreScriptBuilder.h"
#include "CBytecodeStream.h"
#include "ScriptEvents.h"

#include "BeamFactory.h"

using namespace Ogre;
using namespace RoR;

// some hacky functions

void logString(const std::string &str)
{
	SLOG(str);
}

void moduleUserDataCleanupCallback(AngelScript::asIScriptModule *mod)
{
	scriptModuleUserData_t *userdata = (scriptModuleUserData_t*)mod->GetUserData();
	if (userdata != NULL)
	{
		delete userdata;
		mod->SetUserData((void*)NULL);
	}
}

// the class implementation

ScriptEngine::ScriptEngine(Collisions *coll) :
	  mefl(nullptr)
	, coll(coll)
	, context(0)
	, engine(0)
	, eventMask(0)
	, scriptLog(0)
	, defaultModuleName(0)
{
	setSingleton(this);

	// Initialize callbacks vector
	callbacks["on_terrain_loading"] = std::vector<AngelScript::asIScriptFunction*>();
	callbacks["frameStep"] = std::vector<AngelScript::asIScriptFunction*>();
	callbacks["wheelEvents"] = std::vector<AngelScript::asIScriptFunction*>();
	callbacks["eventCallback"] = std::vector<AngelScript::asIScriptFunction*>();
	callbacks["defaultEventCallback"] = std::vector<AngelScript::asIScriptFunction*>();

	// create our own log
	scriptLog = LogManager::getSingleton().createLog(SSETTING("Log Path", "")+"/Angelscript.log", false);
	
	scriptLog->logMessage("ScriptEngine initialized");

	// init not earlier, otherwise crash
	init();
}

ScriptEngine::~ScriptEngine()
{
	// Clean up
	if (engine)  engine->Release();
	if (context) context->Release();
}



#if OGRE_VERSION < ((1 << 16) | (8 << 8 ) | 0)
void ScriptEngine::messageLogged( const String& message, LogMessageLevel lml, bool maskDebug, const String &logName)
#else
void ScriptEngine::messageLogged( const String& message, LogMessageLevel lml, bool maskDebug, const String &logName, bool& skipThisMessage)
#endif // OGRE_VERSION
{
#ifdef USE_MYGUI
	Console *c = RoR::Application::GetConsole();
	if (c) c->putMessage(Console::CONSOLE_MSGTYPE_SCRIPT, Console::CONSOLE_LOGMESSAGE_SCRIPT, message, "page_white_code.png");
#endif // USE_MYGUI
}

void ScriptEngine::ExceptionCallback(AngelScript::asIScriptContext *ctx, void *param)
{
	AngelScript::asIScriptEngine *engine = ctx->GetEngine();
	const AngelScript::asIScriptFunction *function = ctx->GetExceptionFunction();
	SLOG("--- exception ---");
	SLOG("desc: " + String(ctx->GetExceptionString()));
	SLOG("func: " + String(function->GetDeclaration()));
	SLOG("modl: " + String(function->GetModuleName()));
	SLOG("sect: " + String(function->GetScriptSectionName()));
	int col, line = ctx->GetExceptionLineNumber(&col);
	SLOG("line: "+TOSTRING(line)+","+TOSTRING(col));

	// Show the call stack with the variables
	SLOG("--- call stack ---");
	char tmp[2048]="";
	for (AngelScript::asUINT n = 0; n < ctx->GetCallstackSize(); n++)
	{
		function = ctx->GetFunction(n);
		sprintf(tmp, "%s (%d): %s", function->GetScriptSectionName(), ctx->GetLineNumber(n), function->GetDeclaration());
		SLOG(String(tmp));
		PrintVariables(ctx, n);
	}
	SLOG("--- end of script exception message ---");
}

void ScriptEngine::exploreScripts()
{
	// this shouldn't be used at the moment
#if 0
#if USE_ANGELSCRIPT
	FileInfoListPtr files= ResourceGroupManager::getSingleton().findResourceFileInfo("Scripts", "*.rs", false);
	for (FileInfoList::iterator iterFiles = files->begin(); iterFiles!= files->end(); ++iterFiles)
	{
		loadScript(iterFiles->filename);
	}
#endif //USE_ANGELSCRIPT
#endif
}

void ScriptEngine::LineCallback(AngelScript::asIScriptContext *ctx, unsigned long *timeOut)
{
	// If the time out is reached we abort the script
	if (RoR::Application::GetOgreSubsystem()->GetTimeSinceStartup() > *timeOut)
	{
		ctx->Abort();
	}

	// It would also be possible to only suspend the script,
	// instead of aborting it. That would allow the application
	// to resume the execution where it left of at a later
	// time, by simply calling Execute() again.
}


void ScriptEngine::PrintVariables(AngelScript::asIScriptContext *ctx, AngelScript::asUINT stackLevel)
{
	char tmp[1024]="";
	AngelScript::asIScriptEngine *engine = ctx->GetEngine();

	// First print the this pointer if this is a class method
	int typeId = ctx->GetThisTypeId(stackLevel);
	void *varPointer = ctx->GetThisPointer(stackLevel);
	if (typeId)
	{
		sprintf(tmp, " this = 0x%x", varPointer);
		SLOG(tmp);
	}

	// Print the value of each variable, including parameters
	int numVars = ctx->GetVarCount(stackLevel);
	for (int n = 0; n < numVars; n++)
	{
		int typeId = ctx->GetVarTypeId(n, stackLevel);
		void *varPointer = ctx->GetAddressOfVar(n, stackLevel);
		if (typeId == AngelScript::asTYPEID_INT32)
		{
			sprintf(tmp, " %s = %d", ctx->GetVarDeclaration(n, stackLevel), *(int*)varPointer);
			SLOG(tmp);
		}
		else if (typeId == AngelScript::asTYPEID_FLOAT)
		{
			sprintf(tmp, " %s = %f", ctx->GetVarDeclaration(n, stackLevel), *(float*)varPointer);
			SLOG(tmp);
		}
		else if (typeId & AngelScript::asTYPEID_SCRIPTOBJECT)
		{
			AngelScript::asIScriptObject *obj = (AngelScript::asIScriptObject*)varPointer;
			if (obj)
				sprintf(tmp, " %s = {...}", ctx->GetVarDeclaration(n, stackLevel));
			else
				sprintf(tmp, " %s = <null>", ctx->GetVarDeclaration(n, stackLevel));
			SLOG(tmp);
		}
		else if (typeId == engine->GetTypeIdByDecl("string"))
		{
			std::string *str = (std::string*)varPointer;
			if ( str )
				sprintf(tmp, " %s = '%s'", ctx->GetVarDeclaration(n, stackLevel), str->c_str());
			else
				sprintf(tmp, " %s = <null>", ctx->GetVarDeclaration(n, stackLevel));
			SLOG(tmp);
		}
		else
		{
			sprintf(tmp, " %s = {...}", ctx->GetVarDeclaration(n, stackLevel));
			SLOG(tmp);
		}
	}
}

// continue with initializing everything
void ScriptEngine::init()
{
	SLOG("ScriptEngine (SE) initializing ...");
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
		} else if (result == AngelScript::asNOT_SUPPORTED)
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
	//AngelScript::RegisterScriptString(engine);
	//AngelScript::RegisterScriptStringUtils(engine);

	// register some Ogre objects like the vector3 and the quaternion
	registerOgreObjects(engine);

	// Register the local storage object.
	// This needs to be done after the registration of the ogre objects!
	registerLocalStorage(engine);

	// some useful global functions
	result = engine->RegisterGlobalFunction("void log(const string &in)", AngelScript::asFUNCTION(logString), AngelScript::asCALL_CDECL); MYASSERT( result >= 0 );
	result = engine->RegisterGlobalFunction("void print(const string &in)", AngelScript::asFUNCTION(logString), AngelScript::asCALL_CDECL); MYASSERT( result >= 0 );

	result = engine->RegisterObjectType("BeamFactoryClass", sizeof(BeamFactory), AngelScript::asOBJ_REF); MYASSERT(result>=0);
	result = engine->RegisterObjectMethod("BeamFactoryClass", "bool enterRescueTruck()", AngelScript::asMETHOD(BeamFactory,enterRescueTruck), AngelScript::asCALL_THISCALL); MYASSERT(result>=0);
	result = engine->RegisterObjectMethod("BeamFactoryClass", "void setSimulationSpeed(float)", AngelScript::asMETHOD(BeamFactory,setSimulationSpeed), AngelScript::asCALL_THISCALL); MYASSERT(result>=0);
	result = engine->RegisterObjectBehaviour("BeamFactoryClass", AngelScript::asBEHAVE_ADDREF, "void f()", AngelScript::asMETHOD(BeamFactory,addRef), AngelScript::asCALL_THISCALL); MYASSERT(result>=0);
	result = engine->RegisterObjectBehaviour("BeamFactoryClass", AngelScript::asBEHAVE_RELEASE, "void f()", AngelScript::asMETHOD(BeamFactory,release), AngelScript::asCALL_THISCALL); MYASSERT(result>=0);



	// Register everything
	// class Beam
	result = engine->RegisterObjectType("BeamClass", sizeof(Beam), AngelScript::asOBJ_REF); MYASSERT(result>=0);
	result = engine->RegisterObjectMethod("BeamClass", "void scaleTruck(float)", AngelScript::asMETHOD(Beam,scaleTruck), AngelScript::asCALL_THISCALL); MYASSERT(result>=0);
	result = engine->RegisterObjectMethod("BeamClass", "string getTruckName()", AngelScript::asMETHOD(Beam,getTruckName), AngelScript::asCALL_THISCALL); MYASSERT(result>=0);
	result = engine->RegisterObjectMethod("BeamClass", "string getTruckFileName()", AngelScript::asMETHOD(Beam,getTruckFileName), AngelScript::asCALL_THISCALL); MYASSERT(result>=0);
	result = engine->RegisterObjectMethod("BeamClass", "string getTruckHash()", AngelScript::asMETHOD(Beam,getTruckHash), AngelScript::asCALL_THISCALL); MYASSERT(result>=0);
	result = engine->RegisterObjectMethod("BeamClass", "int  getTruckType()", AngelScript::asMETHOD(Beam,getTruckType), AngelScript::asCALL_THISCALL); MYASSERT(result>=0);
	result = engine->RegisterObjectMethod("BeamClass", "void reset(bool)", AngelScript::asMETHOD(Beam,reset), AngelScript::asCALL_THISCALL); MYASSERT(result>=0);
	result = engine->RegisterObjectMethod("BeamClass", "void setDetailLevel(int)", AngelScript::asMETHOD(Beam,setDetailLevel), AngelScript::asCALL_THISCALL); MYASSERT(result>=0);
	result = engine->RegisterObjectMethod("BeamClass", "void showSkeleton(bool, bool)", AngelScript::asMETHOD(Beam,showSkeleton), AngelScript::asCALL_THISCALL); MYASSERT(result>=0);
	result = engine->RegisterObjectMethod("BeamClass", "void hideSkeleton(bool)", AngelScript::asMETHOD(Beam,hideSkeleton), AngelScript::asCALL_THISCALL); MYASSERT(result>=0);
	result = engine->RegisterObjectMethod("BeamClass", "void parkingbrakeToggle()", AngelScript::asMETHOD(Beam,parkingbrakeToggle), AngelScript::asCALL_THISCALL); MYASSERT(result>=0);
	result = engine->RegisterObjectMethod("BeamClass", "void tractioncontrolToggle()", AngelScript::asMETHOD(Beam,tractioncontrolToggle), AngelScript::asCALL_THISCALL); MYASSERT(result>=0);
	result = engine->RegisterObjectMethod("BeamClass", "void antilockbrakeToggle()", AngelScript::asMETHOD(Beam,antilockbrakeToggle), AngelScript::asCALL_THISCALL); MYASSERT(result>=0);
	result = engine->RegisterObjectMethod("BeamClass", "void beaconsToggle()", AngelScript::asMETHOD(Beam,beaconsToggle), AngelScript::asCALL_THISCALL); MYASSERT(result>=0);
	result = engine->RegisterObjectMethod("BeamClass", "void setReplayMode(bool)", AngelScript::asMETHOD(Beam,setReplayMode), AngelScript::asCALL_THISCALL); MYASSERT(result>=0);
	result = engine->RegisterObjectMethod("BeamClass", "void resetAutopilot()", AngelScript::asMETHOD(Beam,resetAutopilot), AngelScript::asCALL_THISCALL); MYASSERT(result>=0);
	result = engine->RegisterObjectMethod("BeamClass", "void toggleCustomParticles()", AngelScript::asMETHOD(Beam,toggleCustomParticles), AngelScript::asCALL_THISCALL); MYASSERT(result>=0);
	result = engine->RegisterObjectMethod("BeamClass", "float getDefaultDeformation()", AngelScript::asMETHOD(Beam,getDefaultDeformation), AngelScript::asCALL_THISCALL); MYASSERT(result>=0);
	result = engine->RegisterObjectMethod("BeamClass", "int getNodeCount()", AngelScript::asMETHOD(Beam,getNodeCount), AngelScript::asCALL_THISCALL); MYASSERT(result>=0);
	result = engine->RegisterObjectMethod("BeamClass", "float getTotalMass(bool)", AngelScript::asMETHOD(Beam,getTotalMass), AngelScript::asCALL_THISCALL); MYASSERT(result>=0);
	result = engine->RegisterObjectMethod("BeamClass", "int getWheelNodeCount()", AngelScript::asMETHOD(Beam,getWheelNodeCount), AngelScript::asCALL_THISCALL); MYASSERT(result>=0);
	result = engine->RegisterObjectMethod("BeamClass", "void recalc_masses()", AngelScript::asMETHOD(Beam,recalc_masses), AngelScript::asCALL_THISCALL); MYASSERT(result>=0);
	result = engine->RegisterObjectMethod("BeamClass", "void setMass(float)", AngelScript::asMETHOD(Beam,setMass), AngelScript::asCALL_THISCALL); MYASSERT(result>=0);
	result = engine->RegisterObjectMethod("BeamClass", "bool getBrakeLightVisible()", AngelScript::asMETHOD(Beam,getBrakeLightVisible), AngelScript::asCALL_THISCALL); MYASSERT(result>=0);
	result = engine->RegisterObjectMethod("BeamClass", "bool getCustomLightVisible(int)", AngelScript::asMETHOD(Beam,getCustomLightVisible), AngelScript::asCALL_THISCALL); MYASSERT(result>=0);
	result = engine->RegisterObjectMethod("BeamClass", "void setCustomLightVisible(int, bool)", AngelScript::asMETHOD(Beam,setCustomLightVisible), AngelScript::asCALL_THISCALL); MYASSERT(result>=0);
	result = engine->RegisterObjectMethod("BeamClass", "bool getBeaconMode()", AngelScript::asMETHOD(Beam,getBeaconMode), AngelScript::asCALL_THISCALL); MYASSERT(result>=0);
	result = engine->RegisterObjectMethod("BeamClass", "void setBlinkType(int)", AngelScript::asMETHOD(Beam,setBlinkType), AngelScript::asCALL_THISCALL); MYASSERT(result>=0);
	result = engine->RegisterObjectMethod("BeamClass", "int getBlinkType()", AngelScript::asMETHOD(Beam,getBlinkType), AngelScript::asCALL_THISCALL); MYASSERT(result>=0);
	result = engine->RegisterObjectMethod("BeamClass", "bool getCustomParticleMode()", AngelScript::asMETHOD(Beam,getCustomParticleMode), AngelScript::asCALL_THISCALL); MYASSERT(result>=0);
	result = engine->RegisterObjectMethod("BeamClass", "int getLowestNode()", AngelScript::asMETHOD(Beam,getLowestNode), AngelScript::asCALL_THISCALL); MYASSERT(result>=0);
	result = engine->RegisterObjectMethod("BeamClass", "bool setMeshVisibility(bool)", AngelScript::asMETHOD(Beam,setMeshVisibility), AngelScript::asCALL_THISCALL); MYASSERT(result>=0);
	result = engine->RegisterObjectMethod("BeamClass", "bool getReverseLightVisible()", AngelScript::asMETHOD(Beam,getCustomParticleMode), AngelScript::asCALL_THISCALL); MYASSERT(result>=0);
	result = engine->RegisterObjectMethod("BeamClass", "float getHeadingDirectionAngle()", AngelScript::asMETHOD(Beam,getHeadingDirectionAngle), AngelScript::asCALL_THISCALL); MYASSERT(result>=0);
	result = engine->RegisterObjectMethod("BeamClass", "bool isLocked()", AngelScript::asMETHOD(Beam,isLocked), AngelScript::asCALL_THISCALL); MYASSERT(result>=0);
	result = engine->RegisterObjectMethod("BeamClass", "float getWheelSpeed()", AngelScript::asMETHOD(Beam,getWheelSpeed), AngelScript::asCALL_THISCALL); MYASSERT(result>=0);
	result = engine->RegisterObjectMethod("BeamClass", "vector3 getGForces()", AngelScript::asMETHOD(Beam,getGForces), AngelScript::asCALL_THISCALL); MYASSERT(result>=0);

	result = engine->RegisterObjectMethod("BeamClass", "float getRotation()", AngelScript::asMETHOD(Beam,getRotation), AngelScript::asCALL_THISCALL); MYASSERT(result>=0);
	result = engine->RegisterObjectMethod("BeamClass", "vector3 getVehiclePosition()", AngelScript::asMETHOD(Beam,getPosition), AngelScript::asCALL_THISCALL); MYASSERT(result>=0);
	result = engine->RegisterObjectMethod("BeamClass", "bool navigateTo(vector3 &in)", AngelScript::asMETHOD(Beam,navigateTo), AngelScript::asCALL_THISCALL); MYASSERT(result>=0);
	result = engine->RegisterObjectMethod("BeamClass", "vector3 getNodePosition(int)", AngelScript::asMETHOD(Beam,getNodePosition), AngelScript::asCALL_THISCALL); MYASSERT(result>=0);

	

	/*
	// impossible to use offsetof for derived classes
	// unusable, read http://www.angelcode.com/angelscript/sdk/docs/manual/doc_adv_class_hierarchy.html

	result = engine->RegisterObjectProperty("BeamClass", "float WheelSpeed", offsetof(Beam, WheelSpeed)); MYASSERT(result>=0);
	result = engine->RegisterObjectProperty("BeamClass", "float brake", offsetof(Beam, brake)); MYASSERT(result>=0);
	result = engine->RegisterObjectProperty("BeamClass", "float currentScale", offsetof(Beam, currentScale)); MYASSERT(result>=0);
	result = engine->RegisterObjectProperty("BeamClass", "int debugVisuals", offsetof(Beam, debugVisuals)); MYASSERT(result>=0);
	result = engine->RegisterObjectProperty("BeamClass", "bool networking", offsetof(Beam, networking)); MYASSERT(result>=0);
	result = engine->RegisterObjectProperty("BeamClass", "int label", offsetof(Beam, label)); MYASSERT(result>=0);
	result = engine->RegisterObjectProperty("BeamClass", "int trucknum", offsetof(Beam, trucknum)); MYASSERT(result>=0);
	result = engine->RegisterObjectProperty("BeamClass", "int skeleton", offsetof(Beam, skeleton)); MYASSERT(result>=0);
	result = engine->RegisterObjectProperty("BeamClass", "bool replaymode", offsetof(Beam, replaymode)); MYASSERT(result>=0);
	result = engine->RegisterObjectProperty("BeamClass", "int replaylen", offsetof(Beam, replaylen)); MYASSERT(result>=0);
	result = engine->RegisterObjectProperty("BeamClass", "int replaypos", offsetof(Beam, replaypos)); MYASSERT(result>=0);
	result = engine->RegisterObjectProperty("BeamClass", "bool cparticle_enabled", offsetof(Beam, cparticle_enabled)); MYASSERT(result>=0);
	//result = engine->RegisterObjectProperty("BeamClass", "int hookId", offsetof(Beam, hookId)); MYASSERT(result>=0);
	//result = engine->RegisterObjectProperty("BeamClass", "BeamClass @lockTruck", offsetof(Beam, lockTruck)); MYASSERT(result>=0);
	result = engine->RegisterObjectProperty("BeamClass", "int free_node", offsetof(Beam, free_node)); MYASSERT(result>=0);
	result = engine->RegisterObjectProperty("BeamClass", "int dynamicMapMode", offsetof(Beam, dynamicMapMode)); MYASSERT(result>=0);
	//result = engine->RegisterObjectProperty("BeamClass", "int tied", offsetof(Beam, tied)); MYASSERT(result>=0);
	result = engine->RegisterObjectProperty("BeamClass", "int canwork", offsetof(Beam, canwork)); MYASSERT(result>=0);
	result = engine->RegisterObjectProperty("BeamClass", "int hashelp", offsetof(Beam, hashelp)); MYASSERT(result>=0);
	result = engine->RegisterObjectProperty("BeamClass", "float minx", offsetof(Beam, minx)); MYASSERT(result>=0);
	result = engine->RegisterObjectProperty("BeamClass", "float maxx", offsetof(Beam, maxx)); MYASSERT(result>=0);
	result = engine->RegisterObjectProperty("BeamClass", "float miny", offsetof(Beam, miny)); MYASSERT(result>=0);
	result = engine->RegisterObjectProperty("BeamClass", "float maxy", offsetof(Beam, maxy)); MYASSERT(result>=0);
	result = engine->RegisterObjectProperty("BeamClass", "float minz", offsetof(Beam, minz)); MYASSERT(result>=0);
	result = engine->RegisterObjectProperty("BeamClass", "float maxz", offsetof(Beam, maxz)); MYASSERT(result>=0);
	result = engine->RegisterObjectProperty("BeamClass", "int state", offsetof(Beam, state)); MYASSERT(result>=0);
	result = engine->RegisterObjectProperty("BeamClass", "int sleepcount", offsetof(Beam, sleepcount)); MYASSERT(result>=0);
	result = engine->RegisterObjectProperty("BeamClass", "int driveable", offsetof(Beam, driveable)); MYASSERT(result>=0);
	result = engine->RegisterObjectProperty("BeamClass", "int importcommands", offsetof(Beam, importcommands)); MYASSERT(result>=0);
	result = engine->RegisterObjectProperty("BeamClass", "bool requires_wheel_contact", offsetof(Beam, requires_wheel_contact)); MYASSERT(result>=0);
	result = engine->RegisterObjectProperty("BeamClass", "bool wheel_contact_requested", offsetof(Beam, wheel_contact_requested)); MYASSERT(result>=0);
	result = engine->RegisterObjectProperty("BeamClass", "bool rescuer", offsetof(Beam, rescuer)); MYASSERT(result>=0);
	result = engine->RegisterObjectProperty("BeamClass", "int parkingbrake", offsetof(Beam, parkingbrake)); MYASSERT(result>=0);
	result = engine->RegisterObjectProperty("BeamClass", "int antilockbrake", offsetof(Beam, antilockbrake)); MYASSERT(result>=0);
	result = engine->RegisterObjectProperty("BeamClass", "int tractioncontrol", offsetof(Beam, tractioncontrol)); MYASSERT(result>=0);
	result = engine->RegisterObjectProperty("BeamClass", "int lights", offsetof(Beam, lights)); MYASSERT(result>=0);
	result = engine->RegisterObjectProperty("BeamClass", "int smokeId", offsetof(Beam, smokeId)); MYASSERT(result>=0);
	result = engine->RegisterObjectProperty("BeamClass", "int editorId", offsetof(Beam, editorId)); MYASSERT(result>=0);
	result = engine->RegisterObjectProperty("BeamClass", "float leftMirrorAngle", offsetof(Beam, leftMirrorAngle)); MYASSERT(result>=0);
	result = engine->RegisterObjectProperty("BeamClass", "float refpressure", offsetof(Beam, refpressure)); MYASSERT(result>=0);
	result = engine->RegisterObjectProperty("BeamClass", "int free_pressure_beam", offsetof(Beam, free_pressure_beam)); MYASSERT(result>=0);
	result = engine->RegisterObjectProperty("BeamClass", "int done_count", offsetof(Beam, done_count)); MYASSERT(result>=0);
	result = engine->RegisterObjectProperty("BeamClass", "int free_prop", offsetof(Beam, free_prop)); MYASSERT(result>=0);
	result = engine->RegisterObjectProperty("BeamClass", "float default_beam_diameter", offsetof(Beam, default_beam_diameter)); MYASSERT(result>=0);
	result = engine->RegisterObjectProperty("BeamClass", "int free_aeroengine", offsetof(Beam, free_aeroengine)); MYASSERT(result>=0);
	result = engine->RegisterObjectProperty("BeamClass", "float elevator", offsetof(Beam, elevator)); MYASSERT(result>=0);
	result = engine->RegisterObjectProperty("BeamClass", "float rudder", offsetof(Beam, rudder)); MYASSERT(result>=0);
	result = engine->RegisterObjectProperty("BeamClass", "float aileron", offsetof(Beam, aileron)); MYASSERT(result>=0);
	result = engine->RegisterObjectProperty("BeamClass", "int flap", offsetof(Beam, flap)); MYASSERT(result>=0);
	result = engine->RegisterObjectProperty("BeamClass", "int free_wing", offsetof(Beam, free_wing)); MYASSERT(result>=0);
	result = engine->RegisterObjectProperty("BeamClass", "bool disableDrag", offsetof(Beam, disableDrag)); MYASSERT(result>=0);
	result = engine->RegisterObjectProperty("BeamClass", "int currentcamera", offsetof(Beam, currentcamera)); MYASSERT(result>=0);
	result = engine->RegisterObjectProperty("BeamClass", "int freecinecamera", offsetof(Beam, freecinecamera)); MYASSERT(result>=0);
	result = engine->RegisterObjectProperty("BeamClass", "float brakeforce", offsetof(Beam, brakeforce)); MYASSERT(result>=0);
	result = engine->RegisterObjectProperty("BeamClass", "bool ispolice", offsetof(Beam, ispolice)); MYASSERT(result>=0);
	result = engine->RegisterObjectProperty("BeamClass", "int loading_finished", offsetof(Beam, loading_finished)); MYASSERT(result>=0);
	result = engine->RegisterObjectProperty("BeamClass", "int freecamera", offsetof(Beam, freecamera)); MYASSERT(result>=0);
	result = engine->RegisterObjectProperty("BeamClass", "int first_wheel_node", offsetof(Beam, first_wheel_node)); MYASSERT(result>=0);
	result = engine->RegisterObjectProperty("BeamClass", "int netbuffersize", offsetof(Beam, netbuffersize)); MYASSERT(result>=0);
	result = engine->RegisterObjectProperty("BeamClass", "int nodebuffersize", offsetof(Beam, nodebuffersize)); MYASSERT(result>=0);
	result = engine->RegisterObjectProperty("BeamClass", "float speedoMax", offsetof(Beam, speedoMax)); MYASSERT(result>=0);
	result = engine->RegisterObjectProperty("BeamClass", "bool useMaxRPMforGUI", offsetof(Beam, useMaxRPMforGUI)); MYASSERT(result>=0);
	result = engine->RegisterObjectProperty("BeamClass", "string realtruckfilename", offsetof(Beam, realtruckfilename)); MYASSERT(result>=0);
	result = engine->RegisterObjectProperty("BeamClass", "int free_wheel", offsetof(Beam, free_wheel)); MYASSERT(result>=0);
	result = engine->RegisterObjectProperty("BeamClass", "int airbrakeval", offsetof(Beam, airbrakeval)); MYASSERT(result>=0);
	result = engine->RegisterObjectProperty("BeamClass", "int cameranodecount", offsetof(Beam, cameranodecount)); MYASSERT(result>=0);
	result = engine->RegisterObjectProperty("BeamClass", "int free_cab", offsetof(Beam, free_cab)); MYASSERT(result>=0);
	// wont work: result = engine->RegisterObjectProperty("BeamClass", "int airbrakeval", offsetof(Beam, airbrakeval)); MYASSERT(result>=0);
	result = engine->RegisterObjectProperty("BeamClass", "bool meshesVisible", offsetof(Beam, meshesVisible)); MYASSERT(result>=0);
	*/

	result = engine->RegisterObjectBehaviour("BeamClass", AngelScript::asBEHAVE_ADDREF, "void f()", AngelScript::asMETHOD(Beam,addRef), AngelScript::asCALL_THISCALL); MYASSERT(result>=0);
	result = engine->RegisterObjectBehaviour("BeamClass", AngelScript::asBEHAVE_RELEASE, "void f()", AngelScript::asMETHOD(Beam,release), AngelScript::asCALL_THISCALL); MYASSERT(result>=0);

	// class Settings
	result = engine->RegisterObjectType("SettingsClass", sizeof(Settings), AngelScript::asOBJ_REF); MYASSERT(result>=0);
	result = engine->RegisterObjectMethod("SettingsClass", "string getSetting(const string &in)", AngelScript::asMETHOD(Settings,getSettingScriptSafe), AngelScript::asCALL_THISCALL); MYASSERT(result>=0);
	result = engine->RegisterObjectMethod("SettingsClass", "void setSetting(const string &in, const string &in)", AngelScript::asMETHOD(Settings,setSettingScriptSafe), AngelScript::asCALL_THISCALL); MYASSERT(result>=0);
	result = engine->RegisterObjectBehaviour("SettingsClass", AngelScript::asBEHAVE_ADDREF, "void f()", AngelScript::asMETHOD(Settings,addRef), AngelScript::asCALL_THISCALL); MYASSERT(result>=0);
	result = engine->RegisterObjectBehaviour("SettingsClass", AngelScript::asBEHAVE_RELEASE, "void f()", AngelScript::asMETHOD(Settings,release), AngelScript::asCALL_THISCALL); MYASSERT(result>=0);

	// TODO: add Vector3 classes and other utility classes!

	// class GameScript
	result = engine->RegisterObjectType("GameScriptClass", sizeof(GameScript), AngelScript::asOBJ_VALUE | AngelScript::asOBJ_POD | AngelScript::asOBJ_APP_CLASS); MYASSERT(result>=0);
	result = engine->RegisterObjectMethod("GameScriptClass", "void log(const string &in)", AngelScript::asMETHOD(GameScript,log), AngelScript::asCALL_THISCALL); MYASSERT(result>=0);
	result = engine->RegisterObjectMethod("GameScriptClass", "double getTime()", AngelScript::asMETHOD(GameScript,getTime), AngelScript::asCALL_THISCALL); MYASSERT(result>=0);
	result = engine->RegisterObjectMethod("GameScriptClass", "float rangeRandom(float, float)", AngelScript::asMETHOD(GameScript,rangeRandom), AngelScript::asCALL_THISCALL); MYASSERT(result>=0);

	result = engine->RegisterObjectMethod("GameScriptClass", "void activateAllVehicles()", AngelScript::asMETHOD(GameScript,activateAllVehicles), AngelScript::asCALL_THISCALL); MYASSERT(result>=0);
	result = engine->RegisterObjectMethod("GameScriptClass", "void setTrucksForcedActive(bool forceActive)", AngelScript::asMETHOD(GameScript,setTrucksForcedActive), AngelScript::asCALL_THISCALL); MYASSERT(result>=0);

	result = engine->RegisterObjectMethod("GameScriptClass", "void startTimer()", AngelScript::asMETHOD(GameScript,startTimer), AngelScript::asCALL_THISCALL); MYASSERT(result>=0);
	result = engine->RegisterObjectMethod("GameScriptClass", "float stopTimer()", AngelScript::asMETHOD(GameScript,stopTimer), AngelScript::asCALL_THISCALL); MYASSERT(result>=0);
	result = engine->RegisterObjectMethod("GameScriptClass", "void flashMessage(const string &in, float, float)", AngelScript::asMETHOD(GameScript,flashMessage), AngelScript::asCALL_THISCALL); MYASSERT(result>=0);
	result = engine->RegisterObjectMethod("GameScriptClass", "void message(const string &in, const string &in, float, bool)", AngelScript::asMETHOD(GameScript,message), AngelScript::asCALL_THISCALL); MYASSERT(result>=0);
	result = engine->RegisterObjectMethod("GameScriptClass", "void setDirectionArrow(const string &in, vector3 &in)", AngelScript::asMETHOD(GameScript,setDirectionArrow), AngelScript::asCALL_THISCALL); MYASSERT(result>=0);
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
	result = engine->RegisterObjectMethod("GameScriptClass", "void moveObjectVisuals(const string &in, vector3 &in)", AngelScript::asMETHOD(GameScript,moveObjectVisuals), AngelScript::asCALL_THISCALL); MYASSERT(result>=0);
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
	result = engine->RegisterObjectMethod("GameScriptClass", "int getCurrentTruckNumber()", AngelScript::asMETHOD(GameScript,getCurrentTruckNumber), AngelScript::asCALL_THISCALL); MYASSERT(result>=0);
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
	result = engine->RegisterEnumValue("scriptEvents", "SE_TRUCK_TRACTIONCONTROL_TOGGLE", SE_TRUCK_TRACTIONCONTROL_TOGGLE); MYASSERT(result>=0);
	result = engine->RegisterEnumValue("scriptEvents", "SE_TRUCK_ANTILOCKBRAKE_TOGGLE", SE_TRUCK_ANTILOCKBRAKE_TOGGLE); MYASSERT(result>=0);
	result = engine->RegisterEnumValue("scriptEvents", "SE_TRUCK_BEACONS_TOGGLE", SE_TRUCK_BEACONS_TOGGLE); MYASSERT(result>=0);
	result = engine->RegisterEnumValue("scriptEvents", "SE_TRUCK_CPARTICLES_TOGGLE", SE_TRUCK_CPARTICLES_TOGGLE); MYASSERT(result>=0);
	result = engine->RegisterEnumValue("scriptEvents", "SE_TRUCK_GROUND_CONTACT_CHANGED", SE_TRUCK_GROUND_CONTACT_CHANGED); MYASSERT(result>=0);
	result = engine->RegisterEnumValue("scriptEvents", "SE_GENERIC_NEW_TRUCK", SE_GENERIC_NEW_TRUCK); MYASSERT(result>=0);
	result = engine->RegisterEnumValue("scriptEvents", "SE_GENERIC_DELETED_TRUCK", SE_GENERIC_DELETED_TRUCK); MYASSERT(result>=0);
	result = engine->RegisterEnumValue("scriptEvents", "SE_GENERIC_INPUT_EVENT", SE_GENERIC_INPUT_EVENT); MYASSERT(result>=0);
	result = engine->RegisterEnumValue("scriptEvents", "SE_GENERIC_MOUSE_BEAM_INTERACTION", SE_GENERIC_MOUSE_BEAM_INTERACTION); MYASSERT(result>=0);
	result = engine->RegisterEnumValue("scriptEvents", "SE_ANGELSCRIPT_MANIPULATIONS", SE_ANGELSCRIPT_MANIPULATIONS); MYASSERT(result>=0);
	result = engine->RegisterEnumValue("scriptEvents", "SE_ALL_EVENTS", SE_ALL_EVENTS); MYASSERT(result>=0);
	
	// enum truckStates
	result = engine->RegisterEnum("truckStates"); MYASSERT(result>=0);
	result = engine->RegisterEnumValue("truckStates", "TS_ACTIVATED", ACTIVATED); MYASSERT(result>=0);
	result = engine->RegisterEnumValue("truckStates", "TS_DESACTIVATED", DESACTIVATED); MYASSERT(result>=0);
	result = engine->RegisterEnumValue("truckStates", "TS_MAYSLEEP", MAYSLEEP); MYASSERT(result>=0);
	result = engine->RegisterEnumValue("truckStates", "TS_GOSLEEP", GOSLEEP); MYASSERT(result>=0);
	result = engine->RegisterEnumValue("truckStates", "TS_SLEEPING", SLEEPING); MYASSERT(result>=0);
	result = engine->RegisterEnumValue("truckStates", "TS_NETWORKED", NETWORKED); MYASSERT(result>=0);
	result = engine->RegisterEnumValue("truckStates", "TS_RECYCLE", RECYCLE); MYASSERT(result>=0);
	result = engine->RegisterEnumValue("truckStates", "TS_DELETED", DELETED); MYASSERT(result>=0);

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
	result = engine->RegisterGlobalProperty("SettingsClass settings", &SETTINGS); MYASSERT(result>=0);

	SLOG("Type registrations done. If you see no error above everything should be working");

	// Register a callback to delete our user data from it when the module gets destroyed
	engine->SetModuleUserDataCleanupCallback(moduleUserDataCleanupCallback);

	// Create our context
	context = engine->CreateContext();

	// Initialize timeout system
	unsigned long timeOut = 0;
	/*
	// TOFIX: AS crashes badly when using these :-\
	result = context->SetLineCallback(AngelScript::asMETHOD(ScriptEngine, LineCallback), &timeOut, AngelScript::asCALL_THISCALL);
	if (result < 0)
	{
		SLOG("Failed to set the line callback function.");
		context->Release();
		return -1;
	}*/

	// Initialize the exception handling
	result = context->SetExceptionCallback(AngelScript::asMETHOD(ScriptEngine, ExceptionCallback), this, AngelScript::asCALL_THISCALL);
	if (result < 0)
	{
		SLOG("Failed to set the exception callback function.");
		context->Release();
		return;
	}
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
	int r;

	// Check if we need to execute any strings
	std::vector<String> tmpQueue;
	stringExecutionQueue.pull(tmpQueue);
	std::vector<String>::iterator it;
	for (it=tmpQueue.begin(); it!=tmpQueue.end();it++)
	{
		// TODO: allow different modules
		if (!defaultModuleName)
		{
			SLOG("Cannot execute string. No module available.");
		}
		else
			executeString(defaultModuleName, *it);
	}

	// framestep stuff below
	if (callbacks["frameStep"].empty()) return 1;
	if (!engine) return 0;
	if (!context) context = engine->CreateContext();

	// loop over all callbacks
	for (unsigned int i = 0; i<callbacks["frameStep"].size(); ++i)
	{
		// prepare the call
		r = context->Prepare(callbacks["frameStep"][i]);
		if (r<0)
		{
			SLOG("Failed to prepare frameStep function.");
			continue;
		}

		// Set the arguments
		context->SetArgFloat(0, dt);

		// Execute it
		r = context->Execute();
		if (r != AngelScript::asEXECUTION_FINISHED)
		{
			SLOG("Failed to execute frameStep function.");
			continue;
		}
	}

	// Collect garbage in frameStep
	engine->GarbageCollect(AngelScript::asGC_ONE_STEP);

	return 0;
}

int ScriptEngine::fireEvent(std::string instanceName, float intensity)
{
// TODO: update fire
#if 0
	if (!engine) return 0;
	AngelScript::asIScriptModule *mod = engine->GetModule(moduleName, AngelScript::asGM_CREATE_IF_NOT_EXISTS);
	AngelScript::asIScriptFunction* functionPtr = mod->GetFunctionByDecl("void fireEvent(string, float)"); // TODO: this shouldn't be hard coded --neorej16
	if (functionPtr == NULL) return 0;
	if (!context) context = engine->CreateContext();
	context->Prepare(functionPtr);

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
#endif //0

	return 0;
}

int ScriptEngine::envokeCallback(void* functionPtr, eventsource_t *source, node_t *node, int type)
{
	if (!engine) return 0;

	if (functionPtr == NULL && callbacks["defaultEventCallback"].empty())
	{
		// no callback available, discard the event
		return 0;
	}
	else if (functionPtr == NULL)
	{
		// We'll call the default event callback functions recursively
		for (unsigned int i = 0; i<callbacks["defaultEventCallback"].size(); ++i)
			if (callbacks["defaultEventCallback"][i] != NULL)
					envokeCallback((void*)callbacks["defaultEventCallback"][i], source, node, type);
		return 0;
	}
	if (!context) context = engine->CreateContext();
	context->Prepare((AngelScript::asIScriptFunction*)functionPtr);

	// Set the function arguments
	std::string *instance_name = new std::string(source->instancename);
	std::string *boxname = new std::string(source->boxname);
	context->SetArgDWord (0, type);
	context->SetArgObject(1, instance_name);
	context->SetArgObject(2, boxname);
	if (node)
		context->SetArgDWord (3, node->id);
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

int ScriptEngine::executeString(String moduleName, String code)
{
	if (!engine) return 1;
	if (!context) context = engine->CreateContext();
	AngelScript::asIScriptModule *mod = engine->GetModule(moduleName.c_str(), AngelScript::asGM_CREATE_IF_NOT_EXISTS);

	// Wrap the code in a function so that it can be compiled and executed
	code = String("void ExecuteString() {\n") + code + String("\n;}");

	// Compile the function that can be executed
	AngelScript::asIScriptFunction *func = 0;
	int r = mod->CompileFunction("ExecuteString", code.c_str(), -1, 0, &func);
	if (r < 0 || func == NULL)
		return r;

	// This function isn't part of the module that we'll execute it in
	// So we'll add the module as user data to the function
	func->SetUserData((void*)mod);

	// Prepare to execute
	r = context->Prepare(func);
	if (r < 0)
	{
		func->Release();
		return r;
	}

	// Execute the function
	r = context->Execute();

	// Clean up
	func->Release();

	if (r < 0)
	{
		SLOG("error " + TOSTRING(r) + " while executing string: " + code + ".");
	}
	return r;

}

int ScriptEngine::addFunction(const String &moduleName, const String &arg)
{
	if (!engine) return 1;
	if (!context) context = engine->CreateContext();
	AngelScript::asIScriptModule *mod = engine->GetModule(moduleName.c_str(), AngelScript::asGM_CREATE_IF_NOT_EXISTS);

	AngelScript::asIScriptFunction *func = 0;
	int r = mod->CompileFunction("addfunc", arg.c_str(), 0, AngelScript::asCOMP_ADD_TO_MODULE, &func);
	
	if ( r < 0 )
	{
		char tmp[512] = "";
		sprintf(tmp, "An error occurred while trying to add a function ('%s') to script module '%s'.", arg.c_str(), moduleName);
		SLOG(tmp);
	}
	else
	{
		// successfully added function
		// Check if we added a "special" function
		
		
		// compare the id of the newly added function with the special functions
		if (func == mod->GetFunctionByDecl("void frameStep(float)"))
		{	
			callbacks["frameStep"].push_back(func);
		}
		else if (func == mod->GetFunctionByDecl("void wheelEvents(int, string, string, string)"))
		{	
			callbacks["wheelEvents"].push_back(func);
		}
		else if (func == mod->GetFunctionByDecl("void eventCallback(int, int)"))
		{
			callbacks["eventCallback"].push_back(func);
		}
		else if (func == mod->GetFunctionByDecl("void defaultEventCallback(int, string, string, int)"))
		{
			callbacks["defaultEventCallback"].push_back(func);
		}
		else if (func == mod->GetFunctionByDecl("void on_terrain_loading(string lines)"))
		{	
			callbacks["on_terrain_loading"].push_back(func);
		}
	}

	// We must release the function object
	if ( func )
		func->Release();

	return r;
}

bool ScriptEngine::functionExists(const String &moduleName, const String &arg)
{
	if (!engine) return -1;
	if (!context) context = engine->CreateContext();
	AngelScript::asIScriptModule *mod = engine->GetModule(moduleName.c_str(), AngelScript::asGM_ONLY_IF_EXISTS);

	if (mod == 0) return false;
	else return (mod->GetFunctionByDecl(arg.c_str()) != NULL);
}

int ScriptEngine::deleteFunction(const String &moduleName, const String &arg)
{
	if (!engine) return 1;
	if (!context) context = engine->CreateContext();
	AngelScript::asIScriptModule *mod = engine->GetModule(moduleName.c_str(), AngelScript::asGM_ONLY_IF_EXISTS);

	if ( mod == 0 || mod->GetFunctionCount() == 0 )
	{
		char tmp[512] = "";
		sprintf(tmp, "An error occurred while trying to remove a function ('%s') from script module '%s'.", arg.c_str(), moduleName.c_str());
		SLOG(tmp);
		return AngelScript::asNO_FUNCTION;
	}

	AngelScript::asIScriptFunction* func = mod->GetFunctionByDecl(arg.c_str());
	if (func != NULL)
	{
		// Warning: The function is not destroyed immediately, only when no more references point to it.
		mod->RemoveFunction(func);

		// Since functions can be recursive, we'll call the garbage
		// collector to make sure the object is really freed
		engine->GarbageCollect();
		
		// Check if we removed a "special" function
		for (std::map< std::string, std::vector<AngelScript::asIScriptFunction*> >::iterator it = callbacks.begin(); it != callbacks.end(); it++)
		{
			std::vector<AngelScript::asIScriptFunction*>::iterator key = std::find(it->second.begin(), it->second.end(), func);
			if (*key == func)
				it->second.erase(key);
		}

			return 1;
	}
	else
	{
		char tmp[512] = "";
		sprintf(tmp, "An error occurred while trying to remove a function ('%s') from script module '%s'.", arg.c_str(), moduleName);
		SLOG(tmp);

			return -1;

	}

	return 0;
}

int ScriptEngine::addVariable(const String &moduleName, const String &arg)
{
	if (!engine) return 1;
	if (!context) context = engine->CreateContext();
	AngelScript::asIScriptModule *mod = engine->GetModule(moduleName.c_str(), AngelScript::asGM_CREATE_IF_NOT_EXISTS);

	int r = mod->CompileGlobalVar("addvar", arg.c_str(), 0);
	if ( r < 0 )
	{
		char tmp[512] = "";
		sprintf(tmp, "An error occurred while trying to add a variable ('%s') to script module '%s'.", arg.c_str(), moduleName.c_str());
		SLOG(tmp);
	}

	return r;
}

int ScriptEngine::deleteVariable(const String &moduleName, const String &arg)
{
	if (!engine) return 1;
	if (!context) context = engine->CreateContext();
	AngelScript::asIScriptModule *mod = engine->GetModule(moduleName.c_str(), AngelScript::asGM_ONLY_IF_EXISTS);

	if ( mod == 0 || mod->GetGlobalVarCount() == 0 )
	{
		char tmp[512] = "";
		sprintf(tmp, "An error occurred while trying to remove a variable ('%s') from script module '%s'.", arg.c_str(), moduleName.c_str());
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
	if (callbacks["eventCallback"].empty()) return;
	if (eventMask & eventnum)
	{
		// script registered for that event, so sent it

		int r;
		if (!context) context = engine->CreateContext();
		// loop over all callbacks
		for (unsigned int i = 0; i<callbacks["eventCallback"].size(); ++i)
		{
			// prepare the call
			r = context->Prepare(callbacks["eventCallback"][i]);
			if (r<0)
			{
				SLOG("Failed to prepare eventCallback function.");
				continue;
			}

			// Set the arguments
			context->SetArgDWord(0, eventnum);
			context->SetArgDWord(1, value);

			// Execute it
			r = context->Execute();
			if (r != AngelScript::asEXECUTION_FINISHED)
			{
				SLOG("Failed to execute eventCallback function.");
				continue;
			}
		}
		return;
	}
}

int ScriptEngine::loadScript(String scriptName)
{

	String moduleName = scriptName + "_module";

	// use the first module that we encounter as default (for now)
	if (!defaultModuleName)
		defaultModuleName = moduleName.c_str();

	return loadScript(scriptName, moduleName);
}

int ScriptEngine::loadScript(String scriptName, String moduleName)
{
	AngelScript::asIScriptFunction* func = 0;
	AngelScript::asIScriptModule*   mod = 0;
	int result = 0;

	// The builder is a helper class that will load the script file,
	// search for #include directives, and load any included files as
	// well.
	OgreScriptBuilder builder;
	result = builder.StartNewModule(engine, moduleName.c_str());
	if (result < 0)
	{
		SLOG("Failed to start new module");
		return result;
	}

	// Get the newly created module
	mod = engine->GetModule(moduleName.c_str(), AngelScript::asGM_ONLY_IF_EXISTS);
	if (result < 0)
	{
		SLOG("Failed to get the new module");
		return result;
	}

	// Load the script

	result = builder.AddSectionFromFile(scriptName.c_str());
	if (result < 0)
	{
		SLOG("Unkown error while loading script file: " + scriptName);
		SLOG("Failed to add script file (error " + TOSTRING(result) + ")");
		return result;
	}

	// Set the module user data
	scriptModuleUserData_t *userdata = new scriptModuleUserData_t();
	userdata->scriptName = scriptName;
	userdata->scriptHash = builder.getHash();
	mod->SetUserData((void*)userdata);

	// Try to load precompiled bytecode
	bool cached = false;
	String fn = SSETTING("Cache Path", "") + "script" + builder.getHash() + "_" + scriptName + "c";
	CBytecodeStream bstream(fn, std::string("r"));
	if (bstream.Existing())
	{
		int res = mod->LoadByteCode(&bstream);
		if (res<0)
		 {
			SLOG("Failed to load the precompiled script: " + TOSTRING(res));
		}
		cached = !res;
	}

	// Compile the script if we couldn't load the bytecode
	if (!cached)
	{
		SLOG("Compiling script for first time use...");

		result = builder.BuildModule();
		if ( result < 0 )
		{
			SLOG("Failed to build the module");
			return result;
		}

		// save the bytecode
		{
			String fn = SSETTING("Cache Path", "") + "script" + builder.getHash() + "_" + scriptName + "c";
			SLOG("saving script bytecode to file " + fn);
			CBytecodeStream bstream(fn, std::string("w"));
			mod->SaveByteCode(&bstream);
		}
	}

	// get some other optional functions
	func = mod->GetFunctionByDecl("void frameStep(float)");
	if (func != NULL) callbacks["frameStep"].push_back(func);

	func = mod->GetFunctionByDecl("void wheelEvents(int, string, string, string)");
	if (func != NULL) callbacks["wheelEvents"].push_back(func);

	func = mod->GetFunctionByDecl("void eventCallback(int, int)");
	if (func != NULL) callbacks["eventCallback"].push_back(func);

	func = mod->GetFunctionByDecl("void defaultEventCallback(int, string, string, int)");
	if (func != NULL) callbacks["defaultEventCallback"].push_back(func);

	func = mod->GetFunctionByDecl("void on_terrain_loading(string lines)");
	if (func != NULL) callbacks["on_terrain_loading"].push_back(func);

	// Find the function that is to be called.
	func = mod->GetFunctionByDecl("void main()");
	if (func == NULL)
	{
		// The function couldn't be found. Instruct the script writer to include the
		// expected function in the script.
		SLOG("The script should have the function 'void main()'.");
		return 0;
	}

	// This should never happen
	if (!context) context = engine->CreateContext();
	
	// Prepare the script context with the function we wish to execute.
	result = context->Prepare(func);
	if (result < 0)
	{
		SLOG("Failed to prepare the context.");
		return -1;
	}

	// Set the timeout before executing the function. Give the function 1 sec
	// to return before we'll abort it.
	//timeOut = RoR::Application::GetOgreSubsystem()->GetTimeSinceStartup() + 1000;

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
			AngelScript::asIScriptFunction *func = context->GetExceptionFunction();
			SLOG("func: " + String(func->GetDeclaration()));
			SLOG("modl: " + String(func->GetModuleName()));
			SLOG("sect: " + String(func->GetScriptSectionName()));
			SLOG("line: " + TOSTRING(context->GetExceptionLineNumber()));
			SLOG("desc: " + String(context->GetExceptionString()));
		} else
		{
			SLOG("The script ended for some unforeseen reason " + TOSTRING(result));
		}
	} else
	{
		SLOG("The script finished successfully.");
	}

	return 0;
}

AngelScript::asIScriptModule *ScriptEngine::getCurrentModule()
{
	if (!engine) return NULL;
	//if(!context) return NULL;

	// We cannot use our ScriptEngine::context variable here.
	// While compiling, global variables get initialized outside of that context
	// asGetActiveContext will get the context that is currently active in this thread
	AngelScript::asIScriptContext *ctx = AngelScript::asGetActiveContext();
	if (!ctx) return NULL;

	AngelScript::asIScriptFunction *func = ctx->GetFunction();
	if (!func) return NULL;

	AngelScript::asIScriptModule *mod = engine->GetModule(func->GetModuleName(), AngelScript::asGM_ONLY_IF_EXISTS);
	if (!mod)
	{
		// This function isn't part of any module.
		// In this case, the module should have been set as user data
		mod = (AngelScript::asIScriptModule*)func->GetUserData();
	}

	return mod;
}

StringVector ScriptEngine::getAutoComplete(String command)
{
	StringVector result;
	if (!engine) return result;
	if (!defaultModuleName) return result;
	if (!context) context = engine->CreateContext();
	AngelScript::asIScriptModule *mod = engine->GetModule(defaultModuleName, AngelScript::asGM_CREATE_IF_NOT_EXISTS);

	for (unsigned int i = 0; i < mod->GetGlobalVarCount(); i++)
	{
		const char *name = mod->GetGlobalVarDeclaration(i);
		result.push_back(String(name));
		//SLOG(" VAR > " + String(name));
	}

	for (unsigned int i = 0; i < mod->GetFunctionCount(); i++)
	{
		AngelScript::asIScriptFunction *desc = mod->GetFunctionByIndex(i); //mod->GetFunctionDescriptorById(idx);
		result.push_back(String(desc->GetName()));
		//SLOG(" FUNCTION > " + String(desc->GetName()));
	}


	for (unsigned int i = 0; i < engine->GetGlobalPropertyCount(); i++)
	{
		const char *name;
		const char *nameSpace;
		int  typeId = 0;
		bool isConst = false;

		if (!engine->GetGlobalPropertyByIndex(i, &name, &nameSpace, &typeId, &isConst))
		{
			result.push_back(String(name));
			//SLOG(" PROPERTY > " + String(name));
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
