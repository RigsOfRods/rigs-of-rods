
// =============================================================================
// This file is adopted from rorserver at commit 4a7109ae2d9a081ccfdad8cc696dc54efe49acb3
// Changes from the original are marked with "//RIGSOFRODS"
// =============================================================================

/*
This file is part of "Rigs of Rods Server" (Relay mode)

Copyright 2009   Thomas Fischer
Copyright 2014+  Rigs of Rods Community

"Rigs of Rods Server" is free software: you can redistribute it
and/or modify it under the terms of the GNU General Public License
as published by the Free Software Foundation, either version 3
of the License, or (at your option) any later version.

"Rigs of Rods Server" is distributed in the hope that it will
be useful, but WITHOUT ANY WARRANTY; without even the implied
warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Foobar. If not, see <http://www.gnu.org/licenses/>.
*/

// created on 22th of June 2009 by Thomas Fischer
#ifdef USE_ANGELSCRIPT

#include "ServerScriptEngine.h"

#include "scriptstdstring/scriptstdstring.h" // angelscript addon
#include "scriptmath/scriptmath.h" // angelscript addon
#include "scriptmath3d/scriptmath3d.h" // angelscript addon
#include "scriptarray/scriptarray.h" // angelscript addon
#include "scriptdictionary/scriptdictionary.h" // angelscript addon
#include "scriptany/scriptany.h" // angelscript addon
#include "SocketW.h"

#include "CurlHelpers.h" // RIGSOFRODS: This is actually the client's header with same name as server header. Both implement the same functionality but with cosmetic differences.
#include "OgreScriptBuilder.h" // RIGSOFRODS: Use OGRE to load files.
#include "PlatformUtils.h" // RIGSOFRODS: For PathCombine
#include "Application.h" // RIGSOFRODS: For App::sys_logs_dir
#include "Console.h" // RIGSOFRODS: For putNetMessage()

#include <cstdio>
#include <stdlib.h>


using namespace std;
using namespace AngelScript; // RIGSOFRODS: Added to match rorserver's style
using namespace RoR;

#include <assert.h>
# define assert_net(expr) ROR_ASSERT(expr)


#ifdef __GNUC__

#include <string.h>

#endif

#include <thread>
#include <future>

static Ogre::Log* sServerLog = nullptr;

void Logger::Log(LogLevel level, const char *format, ...)
{
    // Format the message
    const int BUF_LEN = 4000; // hard limit
    char buffer[BUF_LEN] = {}; // zeroed memory
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, BUF_LEN, format, args);
    va_end(args);
    // Log the message
    sServerLog->logMessage(std::string(buffer));
}

void Logger::Log(LogLevel level, std::string const& msg)
{
    sServerLog->logMessage(msg);
}


// Stream_register_t wrapper
std::string stream_register_get_name(RoRnet::StreamRegister *reg) {
    return std::string(reg->name);
}



ServerScriptEngine::ServerScriptEngine() :
                                             engine(0),
                                             context(0)
{
    sServerLog = Ogre::LogManager::getSingleton().createLog(RoR::PathCombine(App::sys_logs_dir->getStr(), "RoRServerScript.log"));
    init();
}

ServerScriptEngine::~ServerScriptEngine() {
    // Stop thread first
    this->StopTimerThread();

    // Clean up
    deleteAllCallbacks();
    if (engine) engine->Release();
    if (context) context->Release();
}

void ServerScriptEngine::deleteAllCallbacks() {
    if (!engine) return;

    for (std::map<std::string, callbackList>::iterator typeit = callbacks.begin();
         typeit != callbacks.end(); ++typeit) {
        for (callbackList::iterator it = typeit->second.begin(); it != typeit->second.end(); ++it) {
            if (it->obj)
                it->obj->Release();
        }
        typeit->second.clear();
    }
    callbacks.clear();
}

int ServerScriptEngine::loadScript(std::string scriptname) {
    if (scriptname.empty()) return 0;

    int r;
    OgreScriptBuilder builder;
    builder.SetResourceGroup(RGN_SERVER_SCRIPTS); // RIGSOFRODS: Server scripts only load from restricted location to avoid mixups with client scripts.

    r = builder.StartNewModule(engine, "script");
    if (r < 0) {
        Logger::Log(LOG_ERROR, "ScriptEngine: Unknown error while starting a new script module.");
        return 1;
    }

    r = builder.AddSectionFromFile(scriptname.c_str());
    if (r < 0) {
        Logger::Log(LOG_ERROR, "ScriptEngine: Unknown error while adding a new section from file.");
        return 1;
    }

    r = builder.BuildModule();
    if (r < 0) {
        if (r == asINVALID_CONFIGURATION)
            Logger::Log(LOG_ERROR, "ScriptEngine: The engine configuration is invalid.");

        else if (r == asERROR)
            Logger::Log(LOG_ERROR, "ScriptEngine: The script failed to build.");

        else if (r == asBUILD_IN_PROGRESS)
            Logger::Log(LOG_ERROR, "ScriptEngine: Another thread is currently building.");

        else if (r == asINIT_GLOBAL_VARS_FAILED)
            Logger::Log(LOG_ERROR,
                        "ScriptEngine: It was not possible to initialize at least one of the global variables.");

        else
            Logger::Log(LOG_ERROR, "ScriptEngine: Unknown error while building the script.");

        return 1;
    }

    // Get the newly created module
    asIScriptModule *mod = builder.GetModule();

    // get some other optional functions
    asIScriptFunction *func;

    func = mod->GetFunctionByDecl("void frameStep(float)");
    if (func) addCallback("frameStep", func, NULL);

    func = mod->GetFunctionByDecl("void playerDeleted(int, int)");
    if (func) addCallback("playerDeleted", func, NULL);

    func = mod->GetFunctionByDecl("void playerAdded(int)");
    if (func) addCallback("playerAdded", func, NULL);

    func = mod->GetFunctionByDecl("int streamAdded(int, StreamRegister@)");
    if (func) addCallback("streamAdded", func, NULL);

    func = mod->GetFunctionByDecl("int playerChat(int, string msg)");
    if (func) addCallback("playerChat", func, NULL);

    func = mod->GetFunctionByDecl("void gameCmd(int, string)");
    if (func) addCallback("gameCmd", func, NULL);

    func = mod->GetFunctionByDecl("void curlStatus(curlStatusType, int, int, string, string)");
    if (func) addCallback("curlStatus", func, NULL);

    // Create and configure our context
    context = engine->CreateContext();
    //context->SetLineCallback(asMETHOD(ScriptEngine,LineCallback), this, asCALL_THISCALL);
    context->SetExceptionCallback(asMETHOD(ServerScriptEngine, ExceptionCallback), this, asCALL_THISCALL);

    // Find the function that is to be called.
    func = mod->GetFunctionByDecl("void main()");
    if (!func) {
        // The function couldn't be found. Instruct the script writer to include the
        // expected function in the script.
        Logger::Log(LOG_WARN,
                    "ScriptEngine: The script must have the function 'void main()'. Please add it and try again.");
        return 1;
    }

    // prepare and execute the main function
    context->Prepare(func);
    Logger::Log(LOG_INFO, "ScriptEngine: Executing main()");
    r = context->Execute();
    if (r != asEXECUTION_FINISHED) {
        // The execution didn't complete as expected. Determine what happened.
        if (r == asEXECUTION_EXCEPTION) {
            // An exception occurred, let the script writer know what happened so it can be corrected.
            Logger::Log(LOG_ERROR,
                        "ScriptEngine: An exception '%s' occurred. Please correct the code in file '%s' and try again.",
                        context->GetExceptionString(), scriptname.c_str());
        }
    }

    return 0;
}

void ServerScriptEngine::ExceptionCallback(asIScriptContext *ctx, void *param) {
    const asIScriptFunction *function = ctx->GetExceptionFunction();
    Logger::Log(LOG_INFO, "--- exception ---");
    Logger::Log(LOG_INFO, "desc: %s", ctx->GetExceptionString());
    Logger::Log(LOG_INFO, "func: %s", function->GetDeclaration());
    Logger::Log(LOG_INFO, "modl: %s", function->GetModuleName());
    Logger::Log(LOG_INFO, "sect: %s", function->GetScriptSectionName());
    int col, line = ctx->GetExceptionLineNumber(&col);
    Logger::Log(LOG_INFO, "line: %d,%d", line, col);

    // Show the call stack with the variables
    Logger::Log(LOG_INFO, "--- call stack ---");
    char tmp[2048] = "";
    for (asUINT n = 0; n < ctx->GetCallstackSize(); n++) {
        function = ctx->GetFunction(n);
        sprintf(tmp, "%s (%d): %s", function->GetScriptSectionName(), ctx->GetLineNumber(n),
                function->GetDeclaration());
        Logger::Log(LOG_INFO, tmp);
        PrintVariables(ctx, n);
    }
    Logger::Log(LOG_INFO, "--- end of script exception message ---");
}

void ServerScriptEngine::LineCallback(asIScriptContext *ctx, void *param) {
    char tmp[1024] = "";
    asIScriptEngine *engine = ctx->GetEngine();
    int col;
    const char *sectionName;
    int line = ctx->GetLineNumber(0, &col, &sectionName);
    int indent = ctx->GetCallstackSize();
    for (int n = 0; n < indent; n++)
        sprintf(tmp + n, " ");
    const asIScriptFunction *function = engine->GetFunctionById(0);
    sprintf(tmp + indent, "%s:%s:%s:%d,%d", function->GetModuleName(), sectionName,
            function->GetDeclaration(),
            line, col);

    strcat(tmp, "");
    Logger::Log(LOG_INFO, tmp);

//	PrintVariables(ctx, -1);
}

void ServerScriptEngine::PrintVariables(asIScriptContext *ctx, int stackLevel) {
    asIScriptEngine *engine = ctx->GetEngine();

    // First print the this pointer if this is a class method
    int typeId = ctx->GetThisTypeId(stackLevel);
    void *varPointer = ctx->GetThisPointer(stackLevel);
    if (typeId) {
        Logger::Log(LOG_INFO, " this = 0x%x", varPointer);
    }

    // Print the value of each variable, including parameters
    int numVars = ctx->GetVarCount(stackLevel);
    for (int n = 0; n < numVars; n++) {
        // RIFGSOFRODS: Latest angelscript doesn't have `GetVar()` anymore
        int typeId = ctx->GetVarTypeId(n, stackLevel);
        const char* varName = ctx->GetVarName(n, stackLevel);
        void *varPointer = ctx->GetAddressOfVar(n, stackLevel);
        if (typeId == asTYPEID_INT32) {
            Logger::Log(LOG_INFO, " %s = %d", ctx->GetVarDeclaration(n, stackLevel), *(int *) varPointer);
        } else if (typeId == asTYPEID_FLOAT) {
            Logger::Log(LOG_INFO, " %s = %f", ctx->GetVarDeclaration(n, stackLevel), *(float *) varPointer);
        } else if (typeId & asTYPEID_SCRIPTOBJECT) {
            asIScriptObject *obj = (asIScriptObject *) varPointer;
            if (obj)
                Logger::Log(LOG_INFO, " %s = {...}", ctx->GetVarDeclaration(n, stackLevel));
            else
                Logger::Log(LOG_INFO, " %s = <null>", ctx->GetVarDeclaration(n, stackLevel));
        } else if (typeId == engine->GetTypeIdByDecl("string")) {
            std::string *str = (std::string *) varPointer;
            if (str)
                Logger::Log(LOG_INFO, " %s = '%s'", ctx->GetVarDeclaration(n, stackLevel), str->c_str());
            else
                Logger::Log(LOG_INFO, " %s = <null>", ctx->GetVarDeclaration(n, stackLevel));
        } else {
            Logger::Log(LOG_INFO, " %s = {...}", ctx->GetVarDeclaration(n, stackLevel));
        }
    }
}

// continue with initializing everything
void ServerScriptEngine::init() {
    int result;

    // Create the script engine
    engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);

    // Set the message callback to receive information on errors in human readable form.
    // It's recommended to do this right after the creation of the engine, because if
    // some registration fails the engine may send valuable information to the message
    // stream.
    result = engine->SetMessageCallback(asMETHOD(ServerScriptEngine, msgCallback), this, asCALL_THISCALL);
    if (result < 0) {
        if (result == asINVALID_ARG) {
            Logger::Log(LOG_ERROR,
                        "ScriptEngine: One of the arguments is incorrect, e.g. obj is null for a class method.");
            return;
        } else if (result == asNOT_SUPPORTED) {
            Logger::Log(LOG_ERROR, "ScriptEngine: 	The arguments are not supported, e.g. asCALL_GENERIC.");
            return;
        }
        Logger::Log(LOG_ERROR, "ScriptEngine: Unknown error while setting up message callback");
        return;
    }

    // AngelScript doesn't have a built-in string type, as there is no definite standard
    // string type for C++ applications. Every developer is free to register it's own string type.
    // The SDK do however provide a standard add-on for registering a string type, so it's not
    // necessary to register your own string type if you don't want to.
    RegisterStdString(engine);
    RegisterScriptArray(engine,
                        true); // true = allow arrays to be registered as type[] (e.g. int[]). Needed for backwards compatibillity.
    RegisterStdStringUtils(engine); // depends on string and array
    RegisterScriptMath3D(engine); // depends on string
    RegisterScriptMath(engine);
    RegisterScriptDictionary(engine);
    //RIGSOFRODS: FIXME //RegisterScriptFile(engine);
    RegisterScriptAny(engine);

    Logger::Log(LOG_INFO, "ScriptEngine: Registration of libs done, now custom things");

    // Register ServerScript class
    result = engine->RegisterObjectType("ServerScriptClass", sizeof(ServerScript), asOBJ_REF | asOBJ_NOCOUNT);
    assert_net(result >= 0);
    result = engine->RegisterObjectMethod("ServerScriptClass", "void Log(const string &in)",
                                          asMETHOD(ServerScript, log), asCALL_THISCALL);
    assert_net(result >= 0);
    result = engine->RegisterObjectMethod("ServerScriptClass", "void say(const string &in, int uid, int type)",
                                          asMETHOD(ServerScript, say), asCALL_THISCALL);
    assert_net(result >= 0);
    result = engine->RegisterObjectMethod("ServerScriptClass", "void kick(int kuid, const string &in)",
                                          asMETHOD(ServerScript, kick), asCALL_THISCALL);
    assert_net(result >= 0);
    result = engine->RegisterObjectMethod("ServerScriptClass", "void ban(int buid, const string &in)",
                                          asMETHOD(ServerScript, ban), asCALL_THISCALL);
    assert_net(result >= 0);
    result = engine->RegisterObjectMethod("ServerScriptClass", "bool unban(int buid)", asMETHOD(ServerScript, unban),
                                          asCALL_THISCALL);
    assert_net(result >= 0);
    result = engine->RegisterObjectMethod("ServerScriptClass", "int cmd(int uid, string cmd)",
                                          asMETHOD(ServerScript, sendGameCommand), asCALL_THISCALL);
    assert_net(result >= 0);
    result = engine->RegisterObjectMethod("ServerScriptClass", "void curlRequestAsync(string url, string displayname)",
                                          asMETHOD(ServerScript, curlRequestAsync), asCALL_THISCALL);

    assert_net(result >= 0);
    result = engine->RegisterObjectMethod("ServerScriptClass", "int getNumClients()",
                                          asMETHOD(ServerScript, getNumClients), asCALL_THISCALL);
    assert_net(result >= 0);
    result = engine->RegisterObjectMethod("ServerScriptClass", "string getUserName(int uid)",
                                          asMETHOD(ServerScript, getUserName), asCALL_THISCALL);
    assert_net(result >= 0);
    result = engine->RegisterObjectMethod("ServerScriptClass", "void setUserName(int uid, const string &in)",
                                          asMETHOD(ServerScript, setUserName), asCALL_THISCALL);
    assert_net(result >= 0);
    result = engine->RegisterObjectMethod("ServerScriptClass", "string getUserAuth(int uid)",
                                          asMETHOD(ServerScript, getUserAuth), asCALL_THISCALL);
    assert_net(result >= 0);
    result = engine->RegisterObjectMethod("ServerScriptClass", "int getUserAuthRaw(int uid)",
                                          asMETHOD(ServerScript, getUserAuthRaw), asCALL_THISCALL);
    assert_net(result >= 0);
    result = engine->RegisterObjectMethod("ServerScriptClass", "void setUserAuthRaw(int uid, int)",
                                          asMETHOD(ServerScript, setUserAuthRaw), asCALL_THISCALL);
    assert_net(result >= 0);
    result = engine->RegisterObjectMethod("ServerScriptClass", "int getUserColourNum(int uid)",
                                          asMETHOD(ServerScript, getUserColourNum), asCALL_THISCALL);
    assert_net(result >= 0);
    result = engine->RegisterObjectMethod("ServerScriptClass", "void setUserColourNum(int uid, int)",
                                          asMETHOD(ServerScript, setUserColourNum), asCALL_THISCALL);
    assert_net(result >= 0);
    result = engine->RegisterObjectMethod("ServerScriptClass", "void broadcastUserInfo(int)",
                                          asMETHOD(ServerScript, broadcastUserInfo), asCALL_THISCALL);
    assert_net(result >= 0);
    result = engine->RegisterObjectMethod("ServerScriptClass", "string getUserToken(int uid)",
                                          asMETHOD(ServerScript, getUserToken), asCALL_THISCALL);
    assert_net(result >= 0);
    result = engine->RegisterObjectMethod("ServerScriptClass", "string getUserVersion(int uid)",
                                          asMETHOD(ServerScript, getUserVersion), asCALL_THISCALL);
    assert_net(result >= 0);
    result = engine->RegisterObjectMethod("ServerScriptClass", "string getUserIPAddress(int uid)",
                                          asMETHOD(ServerScript, getUserIPAddress), asCALL_THISCALL);
    assert_net(result >= 0);
    result = engine->RegisterObjectMethod("ServerScriptClass", "string getServerTerrain()",
                                          asMETHOD(ServerScript, getServerTerrain), asCALL_THISCALL);
    assert_net(result >= 0);
    result = engine->RegisterObjectMethod("ServerScriptClass", "int getTime()", asMETHOD(ServerScript, getTime),
                                          asCALL_THISCALL);
    assert_net(result >= 0);
    result = engine->RegisterObjectMethod("ServerScriptClass", "int getStartTime()",
                                          asMETHOD(ServerScript, getStartTime), asCALL_THISCALL);
    assert_net(result >= 0);
    result = engine->RegisterObjectMethod("ServerScriptClass",
                                          "void setCallback(const string &in, const string &in, ?&in)",
                                          asMETHOD(ServerScript, setCallback), asCALL_THISCALL);
    assert_net(result >= 0);
    result = engine->RegisterObjectMethod("ServerScriptClass",
                                          "void deleteCallback(const string &in, const string &in, ?&in)",
                                          asMETHOD(ServerScript, deleteCallback), asCALL_THISCALL);
    assert_net(result >= 0);
    result = engine->RegisterObjectMethod("ServerScriptClass", "void throwException(const string &in)",
                                          asMETHOD(ServerScript, throwException), asCALL_THISCALL);
    assert_net(result >= 0);
    result = engine->RegisterObjectMethod("ServerScriptClass", "string get_version()",
                                          asMETHOD(ServerScript, get_version), asCALL_THISCALL);
    assert_net(result >= 0);
    result = engine->RegisterObjectMethod("ServerScriptClass", "string get_asVersion()",
                                          asMETHOD(ServerScript, get_asVersion), asCALL_THISCALL);
    assert_net(result >= 0);
    result = engine->RegisterObjectMethod("ServerScriptClass", "string get_protocolVersion()",
                                          asMETHOD(ServerScript, get_protocolVersion), asCALL_THISCALL);
    assert_net(result >= 0);
    result = engine->RegisterObjectMethod("ServerScriptClass", "uint get_maxClients()",
                                          asMETHOD(ServerScript, get_maxClients), asCALL_THISCALL);
    assert_net(result >= 0);
    result = engine->RegisterObjectMethod("ServerScriptClass", "string get_serverName()",
                                          asMETHOD(ServerScript, get_serverName), asCALL_THISCALL);
    assert_net(result >= 0);
    result = engine->RegisterObjectMethod("ServerScriptClass", "string get_IPAddr()",
                                          asMETHOD(ServerScript, get_IPAddr), asCALL_THISCALL);
    assert_net(result >= 0);
    result = engine->RegisterObjectMethod("ServerScriptClass", "uint get_listenPort()",
                                          asMETHOD(ServerScript, get_listenPort), asCALL_THISCALL);
    assert_net(result >= 0);
    result = engine->RegisterObjectMethod("ServerScriptClass", "int get_serverMode()",
                                          asMETHOD(ServerScript, get_serverMode), asCALL_THISCALL);
    assert_net(result >= 0);
    result = engine->RegisterObjectMethod("ServerScriptClass", "string get_owner()", asMETHOD(ServerScript, get_owner),
                                          asCALL_THISCALL);
    assert_net(result >= 0);
    result = engine->RegisterObjectMethod("ServerScriptClass", "string get_website()",
                                          asMETHOD(ServerScript, get_website), asCALL_THISCALL);
    assert_net(result >= 0);
    result = engine->RegisterObjectMethod("ServerScriptClass", "string get_ircServ()",
                                          asMETHOD(ServerScript, get_ircServ), asCALL_THISCALL);
    assert_net(result >= 0);
    result = engine->RegisterObjectMethod("ServerScriptClass", "string get_voipServ()",
                                          asMETHOD(ServerScript, get_voipServ), asCALL_THISCALL);
    assert_net(result >= 0);
    result = engine->RegisterObjectMethod("ServerScriptClass", "int rangeRandomInt(int, int)",
                                          asMETHOD(ServerScript, rangeRandomInt), asCALL_THISCALL);
    assert_net(result >= 0);
    ServerScript *serverscript = new ServerScript(this);
    result = engine->RegisterGlobalProperty("ServerScriptClass server", serverscript);
    assert_net(result >= 0);

    // Register RoRnet::StreamRegister class
    result = engine->RegisterObjectType("StreamRegister", sizeof(RoRnet::StreamRegister),
                                        asOBJ_REF | asOBJ_NOCOUNT);
    assert_net(result >= 0);
    result = engine->RegisterObjectMethod("StreamRegister", "string getName()",
                                          asFUNCTION(stream_register_get_name), asCALL_CDECL_OBJFIRST);
    assert_net(result >= 0); // (No property accessor on purpose)
    result = engine->RegisterObjectProperty("StreamRegister", "int type",
                                            offsetof(RoRnet::StreamRegister, type));
    assert_net(result >= 0);
    result = engine->RegisterObjectProperty("StreamRegister", "int status",
                                            offsetof(RoRnet::StreamRegister, status));
    assert_net(result >= 0);
    result = engine->RegisterObjectProperty("StreamRegister", "int origin_sourceid",
                                            offsetof(RoRnet::StreamRegister, origin_sourceid));
    assert_net(result >= 0);
    result = engine->RegisterObjectProperty("StreamRegister", "int origin_streamid",
                                            offsetof(RoRnet::StreamRegister, origin_streamid));
    assert_net(result >= 0);


    // Register ServerType enum for the server.serverMode attribute
    result = engine->RegisterEnum("ServerType");
    assert_net(result >= 0);
    result = engine->RegisterEnumValue("ServerType", "SERVER_LAN", SERVER_LAN);
    assert_net(result >= 0);
    result = engine->RegisterEnumValue("ServerType", "SERVER_INET", SERVER_INET);
    assert_net(result >= 0);
    result = engine->RegisterEnumValue("ServerType", "SERVER_AUTO", SERVER_AUTO);
    assert_net(result >= 0);

    // Register publish mode enum for the playerChat callback
    result = engine->RegisterEnum("broadcastType");
    assert_net(result >= 0);
    result = engine->RegisterEnumValue("broadcastType", "BROADCAST_AUTO", BROADCAST_AUTO);
    assert_net(result >= 0);
    result = engine->RegisterEnumValue("broadcastType", "BROADCAST_BLOCK", BROADCAST_BLOCK);
    assert_net(result >= 0);
    result = engine->RegisterEnumValue("broadcastType", "BROADCAST_NORMAL", BROADCAST_NORMAL);
    assert_net(result >= 0);
    result = engine->RegisterEnumValue("broadcastType", "BROADCAST_AUTHED", BROADCAST_AUTHED);
    assert_net(result >= 0);
    result = engine->RegisterEnumValue("broadcastType", "BROADCAST_ALL", BROADCAST_ALL);
    assert_net(result >= 0);

    // Register authorizations
    result = engine->RegisterEnum("authType");
    assert_net(result >= 0);

    result = engine->RegisterEnumValue("authType", "AUTH_NONE", RoRnet::AUTH_NONE);
    assert_net(result >= 0);
    result = engine->RegisterEnumValue("authType", "AUTH_ADMIN", RoRnet::AUTH_ADMIN);
    assert_net(result >= 0);
    result = engine->RegisterEnumValue("authType", "AUTH_RANKED", RoRnet::AUTH_RANKED);
    assert_net(result >= 0);
    result = engine->RegisterEnumValue("authType", "AUTH_MOD", RoRnet::AUTH_MOD);
    assert_net(result >= 0);
    result = engine->RegisterEnumValue("authType", "AUTH_BOT", RoRnet::AUTH_BOT);
    assert_net(result >= 0);
    result = engine->RegisterEnumValue("authType", "AUTH_BANNED", RoRnet::AUTH_BANNED);
    assert_net(result >= 0);
    result = engine->RegisterEnumValue("authType", "AUTH_ALL", 0xFFFFFFFF);
    assert_net(result >= 0);

    // Register serverSayType
    result = engine->RegisterEnum("serverSayType");
    assert_net(result >= 0);
    result = engine->RegisterEnumValue("serverSayType", "FROM_SERVER", FROM_SERVER);
    assert_net(result >= 0);
    result = engine->RegisterEnumValue("serverSayType", "FROM_HOST", FROM_HOST);
    assert_net(result >= 0);
    result = engine->RegisterEnumValue("serverSayType", "FROM_MOTD", FROM_MOTD);
    assert_net(result >= 0);
    result = engine->RegisterEnumValue("serverSayType", "FROM_RULES", FROM_RULES);
    assert_net(result >= 0);

    // Register curl update type for `curlStatus` callback
    result = engine->RegisterEnum("curlStatusType");
    assert_net(result >= 0);
    result = engine->RegisterEnumValue("curlStatusType", "CURL_STATUS_INVALID", CURL_STATUS_INVALID);
    assert_net(result >= 0);
    result = engine->RegisterEnumValue("curlStatusType", "CURL_STATUS_START", CURL_STATUS_START);
    assert_net(result >= 0);
    result = engine->RegisterEnumValue("curlStatusType", "CURL_STATUS_PROGRESS", CURL_STATUS_PROGRESS);
    assert_net(result >= 0);
    result = engine->RegisterEnumValue("curlStatusType", "CURL_STATUS_SUCCESS", CURL_STATUS_SUCCESS);
    assert_net(result >= 0);
    result = engine->RegisterEnumValue("curlStatusType", "CURL_STATUS_FAILURE", CURL_STATUS_FAILURE);
    assert_net(result >= 0);

    // register constants
    result = engine->RegisterGlobalProperty("const int TO_ALL", (void *) &TO_ALL);
    assert_net(result >= 0);


    Logger::Log(LOG_INFO, "ScriptEngine: Registration done");
}

void ServerScriptEngine::msgCallback(const asSMessageInfo *msg) {
    const char *type = "Error";
    if (msg->type == asMSGTYPE_INFORMATION)
        type = "Info";
    else if (msg->type == asMSGTYPE_WARNING)
        type = "Warning";

    Logger::Log(LOG_INFO, "ScriptEngine: %s (%d, %d): %s = %s", msg->section, msg->row, msg->col, type, msg->message);
}

// unused method
int ServerScriptEngine::loadScriptFile(const char *fileName, string &script) {
    FILE *f = fopen(fileName, "rb");
    if (!f) return 1;

    // Determine the size of the file
    fseek(f, 0, SEEK_END);
    int len = ftell(f);
    fseek(f, 0, SEEK_SET);

    // Load the entire file in one call
    script.resize(len);
    fread(&script[0], len, 1, f);

    fclose(f);
    return 0;
}

int ServerScriptEngine::frameStep(float dt) {
    if (!engine) return 0;
    if (!context) context = engine->CreateContext();
    int r;

    // Copy the callback list, because the callback list itself may get changed while executing the script
    callbackList queue(callbacks["frameStep"]);

    // loop over all callbacks
    for (unsigned int i = 0; i < queue.size(); ++i) {
        // prepare the call
        r = context->Prepare(queue[i].func);
        if (r < 0) continue;

        // Set the object if present (if we don't set it, then we call a global function)
        if (queue[i].obj != NULL) {
            context->SetObject(queue[i].obj);
            if (r < 0) continue;
        }

        // Set the arguments
        context->SetArgFloat(0, dt);

        // Execute it
        r = context->Execute();
    }

    // Collect garbage
    engine->GarbageCollect(asGC_ONE_STEP);

    return 0;
}

void ServerScriptEngine::playerDeleted(int uid, int crash, bool doNestedCall /*= false*/) {
    if (!engine) return;
    if (!context) context = engine->CreateContext();
    int r;

    // Push the state of the context if this is a nested call
    if (doNestedCall) {
        r = context->PushState();
        if (r < 0) return;
    }

    // Copy the callback list, because the callback list itself may get changed while executing the script
    callbackList queue(callbacks["playerDeleted"]);

    // loop over all callbacks
    for (unsigned int i = 0; i < queue.size(); ++i) {
        // prepare the call
        r = context->Prepare(queue[i].func);
        if (r < 0) continue;

        // Set the object if present (if we don't set it, then we call a global function)
        if (queue[i].obj != NULL) {
            context->SetObject(queue[i].obj);
            if (r < 0) continue;
        }

        // Set the arguments
        context->SetArgDWord(0, uid);
        context->SetArgDWord(1, crash);

        // Execute it
        r = context->Execute();
    }

    // Pop the state of the context if this is was a nested call
    if (doNestedCall) {
        r = context->PopState();
        if (r < 0) return;
    }


    return;
}

void ServerScriptEngine::playerAdded(int uid) {
    if (!engine) return;
    if (!context) context = engine->CreateContext();
    int r;

    // Copy the callback list, because the callback list itself may get changed while executing the script
    callbackList queue(callbacks["playerAdded"]);

    // loop over all callbacks
    for (unsigned int i = 0; i < queue.size(); ++i) {
        // prepare the call
        r = context->Prepare(queue[i].func);
        if (r < 0) continue;

        // Set the object if present (if we don't set it, then we call a global function)
        if (queue[i].obj != NULL) {
            context->SetObject(queue[i].obj);
            if (r < 0) continue;
        }

        // Set the arguments
        context->SetArgDWord(0, uid);

        // Execute it
        r = context->Execute();
    }
    return;
}

int ServerScriptEngine::streamAdded(int uid, RoRnet::StreamRegister *reg) {
    if (!engine) return 0;
    if (!context) context = engine->CreateContext();
    int r;
    int ret = BROADCAST_AUTO;

    // Copy the callback list, because the callback list itself may get changed while executing the script
    callbackList queue(callbacks["streamAdded"]);

    // loop over all callbacks
    for (unsigned int i = 0; i < queue.size(); ++i) {
        // prepare the call
        r = context->Prepare(queue[i].func);
        if (r < 0) continue;

        // Set the object if present (if we don't set it, then we call a global function)
        if (queue[i].obj != NULL) {
            context->SetObject(queue[i].obj);
            if (r < 0) continue;
        }

        // Set the arguments
        context->SetArgDWord(0, uid);
        context->SetArgObject(1, (void *) reg);

        // Execute it
        r = context->Execute();
        if (r == asEXECUTION_FINISHED) {
            int newRet = context->GetReturnDWord();

            // Only use the new result if it's more restrictive than what we already had
            if (newRet > ret)
                ret = newRet;
        }
    }

    return ret;
}

int ServerScriptEngine::playerChat(int uid, std::string msg) {
    if (!engine) return 0;
    if (!context) context = engine->CreateContext();
    int r;
    int ret = BROADCAST_AUTO;

    // Copy the callback list, because the callback list itself may get changed while executing the script
    callbackList queue(callbacks["playerChat"]);

    // loop over all callbacks
    for (unsigned int i = 0; i < queue.size(); ++i) {
        // prepare the call
        r = context->Prepare(queue[i].func);
        if (r < 0) continue;

        // Set the object if present (if we don't set it, then we call a global function)
        if (queue[i].obj != NULL) {
            context->SetObject(queue[i].obj);
            if (r < 0) continue;
        }

        // Set the arguments
        context->SetArgDWord(0, uid);
        context->SetArgObject(1, (void *) &msg);

        // Execute it
        r = context->Execute();
        if (r == asEXECUTION_FINISHED) {
            int newRet = context->GetReturnDWord();

            // Only use the new result if it's more restrictive than what we already had
            if (newRet > ret)
                ret = newRet;
        }
    }

    return ret;
}

void ServerScriptEngine::gameCmd(int uid, const std::string &cmd) {
    if (!engine) return;
    if (!context) context = engine->CreateContext();
    int r;

    // Copy the callback list, because the callback list itself may get changed while executing the script
    callbackList queue(callbacks["gameCmd"]);

    // loop over all callbacks
    for (unsigned int i = 0; i < queue.size(); ++i) {
        // prepare the call
        r = context->Prepare(queue[i].func);
        if (r < 0) continue;

        // Set the object if present (if we don't set it, then we call a global function)
        if (queue[i].obj != NULL) {
            context->SetObject(queue[i].obj);
            if (r < 0) continue;
        }

        // Set the arguments
        context->SetArgDWord(0, uid);
        context->SetArgObject(1, (void *) &cmd);

        // Execute it
        r = context->Execute();
    }

    return;
}

void ServerScriptEngine::curlStatus(RoRServerCurlStatusType type, int n1, int n2, string displayname, string message)
{
    // Params `n1` and `n2` depend on status type :
    // - for CURL_STATUS_PROGRESS, n1 = bytes downloaded, n2 = total bytes,
    // - otherwise, n1 = CURL return code, n2 = HTTP result code.
    // -------------------------------------------------------------------

    if (!engine) return;
    if (!context) context = engine->CreateContext();
    int r;

    // Copy the callback list, because the callback list itself may get changed while executing the script
    callbackList queue(callbacks["curlStatus"]);

    // loop over all callbacks
    for (unsigned int i = 0; i < queue.size(); ++i) {
        // prepare the call
        r = context->Prepare(queue[i].func);
        if (r < 0) continue;

        // Set the object if present (if we don't set it, then we call a global function)
        if (queue[i].obj != NULL) {
            context->SetObject(queue[i].obj);
            if (r < 0) continue;
        }

        // Set the arguments
        context->SetArgDWord(0, (asDWORD)type);
        context->SetArgDWord(1, (asDWORD)n1);
        context->SetArgDWord(2, (asDWORD)n2);
        context->SetArgObject(3, (void*)&displayname);
        context->SetArgObject(4, (void*)&message);

        // Execute it
        r = context->Execute();
    }
}

void ServerScriptEngine::TimerThreadMain() {
    while (this->GetTimerThreadState() == ThreadState::RUNNING) {
        // sleep 200 miliseconds
#ifndef _WIN32
        usleep(200000);
#else // _WIN32
        Sleep(200);
#endif // _WIN32

        // call script
        this->frameStep(200.f); //RIGSOFRODS //seq->frameStepScripts(200);
    }
}

void ServerScriptEngine::EnsureTimerThreadRunning() {
    std::lock_guard<std::mutex> scoped_lock(m_timer_thread_mutex);
    if (m_timer_thread_state == ThreadState::NOT_RUNNING) {
        Logger::Log(LOG_DEBUG, "ScriptEngine: starting framestep thread");
        m_timer_thread = std::thread(&ServerScriptEngine::TimerThreadMain, this);
        m_timer_thread_state = ThreadState::RUNNING;
    }    
}

ServerScriptEngine::ThreadState ServerScriptEngine::GetTimerThreadState() {
    std::lock_guard<std::mutex> scoped_lock(m_timer_thread_mutex);
    return m_timer_thread_state;
}

void ServerScriptEngine::StopTimerThread() {
    {
        std::lock_guard<std::mutex> scoped_lock(m_timer_thread_mutex);
        if (m_timer_thread_state != ThreadState::RUNNING)
            return;
        m_timer_thread_state = ThreadState::STOP_REQUESTED;
    }

    m_timer_thread.join();
    
    {
        std::lock_guard<std::mutex> scoped_lock(m_timer_thread_mutex);
        m_timer_thread_state = ThreadState::NOT_RUNNING;
    }
}

void ServerScriptEngine::setException(const std::string &message) {
    if (!engine || !context) {
        // There's not much we can do, except for logging the message
        Logger::Log(LOG_INFO, "--- script exception ---");
        Logger::Log(LOG_INFO, " desc: %s", (message.c_str()));
        Logger::Log(LOG_INFO, "--- end of script exception message ---");
    } else
        context->SetException(message.c_str());
}

void ServerScriptEngine::addCallbackScript(const std::string &type, const std::string &_func, asIScriptObject *obj) {
    if (!engine) return;

    // get the function declaration and check the type at the same time
    std::string funcDecl = "";
    if (type == "frameStep")
        funcDecl = "void " + _func + "(float)";
    else if (type == "playerChat")
        funcDecl = "int " + _func + "(int, const string &in)";
    else if (type == "gameCmd")
        funcDecl = "void " + _func + "(int, const string &in)";
    else if (type == "playerAdded")
        funcDecl = "void " + _func + "(int)";
    else if (type == "playerDeleted")
        funcDecl = "void " + _func + "(int, int)";
    else if (type == "streamAdded")
        funcDecl = "int " + _func + "(int, StreamRegister@)";
    else if (type == "curlStatus")
        funcDecl = "void " + _func + "(curlStatusType, int, int, string, string)";
    else {
        setException("Type " + type +
                     " does not exist! Possible type strings: 'frameStep', 'playerChat', 'gameCmd', 'playerAdded', 'playerDeleted', 'streamAdded'.");
        return;
    }

    asIScriptFunction *func;
    if (obj) {
        // search for a method in the class
        asITypeInfo *objType = obj->GetObjectType();
        func = objType->GetMethodByDecl(funcDecl.c_str());
        if (!func) {
            // give a nice error message that says that the method was not found.
            func = objType->GetMethodByName(_func.c_str());
            if (func)
                setException("Method '" + std::string(func->GetDeclaration(false)) + "' was found in '" +
                             objType->GetName() + "' but the correct declaration is: '" + funcDecl + "'.");
            else
                setException("Method '" + funcDecl + "' was not found in '" + objType->GetName() + "'.");
            return;
        }
    } else {
        // search for a global function
        asIScriptModule *mod = engine->GetModule("script");
        func = mod->GetFunctionByDecl(funcDecl.c_str());
        if (!func) {
            // give a nice error message that says that the function was not found.
            func = mod->GetFunctionByName(_func.c_str());
            if (func)
                setException("Function '" + std::string(func->GetDeclaration(false)) +
                             "' was found, but the correct declaration is: '" + funcDecl + "'.");
            else
                setException("Function '" + funcDecl + "' was not found.");
            return;
        }
    }

    if (callbackExists(type, func, obj))
        Logger::Log(LOG_INFO, "ScriptEngine: error: Function '" + std::string(func->GetDeclaration(false)) +
                              "' is already a callback for '" + type + "'.");
    else
        addCallback(type, func, obj);
}

void ServerScriptEngine::addCallback(const std::string &type, asIScriptFunction *func, asIScriptObject *obj) {
    if (!engine) return;

    if (obj) {
        // We're about to store a reference to the object, so let's tell the script engine about that
        // This avoids the object from going out of scope while we're still trying to access it.
        // BUT: it prevents local objects from being destroyed automatically....
        engine->AddRefScriptObject(obj, obj->GetObjectType());
    }

    // Add the function to the list
    rorserver_callback_t tmp;
    tmp.obj = obj;
    tmp.func = func;
    callbacks[type].push_back(tmp);

    // Do we need to start the frameStep thread?
    if (type == "frameStep") {
        this->EnsureTimerThreadRunning();
    }

    // finished :)
    Logger::Log(LOG_INFO, "ScriptEngine: success: Added a '" + type + "' callback for: " +
                          std::string(func->GetDeclaration(true)));
}

void ServerScriptEngine::deleteCallbackScript(const std::string &type, const std::string &_func, asIScriptObject *obj) {
    if (!engine) return;

    // get the function declaration and check the type at the same time
    std::string funcDecl = "";
    if (type == "frameStep")
        funcDecl = "void " + _func + "(float)";
    else if (type == "playerChat")
        funcDecl = "int " + _func + "(int, const string &in)";
    else if (type == "gameCmd")
        funcDecl = "void " + _func + "(int, const string &in)";
    else if (type == "playerAdded")
        funcDecl = "void " + _func + "(int)";
    else if (type == "playerDeleted")
        funcDecl = "void " + _func + "(int, int)";
    else if (type == "streamAdded")
        funcDecl = "int " + _func + "(int, StreamRegister@)";
    else if (type == "curlStatus")
        funcDecl = "void " + _func + "(curlStatusType, int, int, string, string)";
    else {
        setException("Type " + type +
                     " does not exist! Possible type strings: 'frameStep', 'playerChat', 'gameCmd', 'playerAdded', 'playerDeleted', 'streamAdded'.");
        Logger::Log(LOG_INFO, "ScriptEngine: error: Failed to remove callback: " + _func);
        return;
    }

    asIScriptFunction *func;
    if (obj) {
        // search for a method in the class
        asITypeInfo *objType = obj->GetObjectType();
        func = objType->GetMethodByDecl(funcDecl.c_str());
        if (!func) {
            // give a nice error message that says that the method was not found.
            func = objType->GetMethodByName(_func.c_str());
            if (func)
                setException("Method '" + std::string(func->GetDeclaration(false)) + "' was found in '" +
                             objType->GetName() + "' but the correct declaration is: '" + funcDecl + "'.");
            else
                setException("Method '" + funcDecl + "' was not found in '" + objType->GetName() + "'.");
            Logger::Log(LOG_INFO, "ScriptEngine: error: Failed to remove callback: " + funcDecl);
            return;
        }
    } else {
        // search for a global function
        asIScriptModule *mod = engine->GetModule("script");
        func = mod->GetFunctionByDecl(funcDecl.c_str());
        if (!func) {
            // give a nice error message that says that the function was not found.
            func = mod->GetFunctionByName(_func.c_str());
            if (func)
                setException("Function '" + std::string(func->GetDeclaration(false)) +
                             "' was found, but the correct declaration is: '" + funcDecl + "'.");
            else
                setException("Function '" + funcDecl + "' was not found.");
            Logger::Log(LOG_INFO, "ScriptEngine: error: Failed to remove callback: " + funcDecl);
            return;
        }
    }
    deleteCallback(type, func, obj);
}

void ServerScriptEngine::deleteCallback(const std::string &type, asIScriptFunction *func, asIScriptObject *obj) {
    if (!engine) return;

    for (callbackList::iterator it = callbacks[type].begin(); it != callbacks[type].end(); ++it) {
        if (it->obj == obj && it->func == func) {
            callbacks[type].erase(it);
            Logger::Log(LOG_INFO, "ScriptEngine: success: removed a '" + type + "' callback: " +
                                  std::string(func->GetDeclaration(true)));
            if (obj)
                engine->ReleaseScriptObject(obj, obj->GetObjectType());
            return;
        }
    }
    Logger::Log(LOG_INFO, "ScriptEngine: error: failed to remove callback: " + std::string(func->GetDeclaration(true)));
}

bool ServerScriptEngine::callbackExists(const std::string &type, asIScriptFunction *func, asIScriptObject *obj) {
    if (!engine) return false;

    for (callbackList::iterator it = callbacks[type].begin(); it != callbacks[type].end(); ++it) {
        if (it->obj == obj && it->func == func)
            return true;
    }
    return false;
}

/* class that implements the interface for the scripts */
ServerScript::ServerScript(ServerScriptEngine *se) : mse(se) {
}

ServerScript::~ServerScript() {
}

void ServerScript::log(std::string &msg) {
    Logger::Log(LOG_INFO, "SCRIPT|%s", msg.c_str());
}

void ServerScript::say(std::string &msg, int uid, int type) {
    // RIGSOFRODS: Do what rorserver's `Sequencer::serverSay()` does.
    switch (type) {
        case FROM_SERVER:
            msg = std::string("SERVER: ") + msg;
            break;

        case FROM_HOST:
            if (uid == -1) {
                msg = std::string("Host(general): ") + msg;
            } else {
                msg = std::string("Host(private): ") + msg;
            }
            break;

        case FROM_RULES:
            msg = std::string("Rules: ") + msg;
            break;

        case FROM_MOTD:
            msg = std::string("MOTD: ") + msg;
            break;
    }
    // RIGSOFRODS: Post message directly to console (thread safe)
    App::GetConsole()->putNetMessage(uid, Console::CONSOLE_SYSTEM_NETCHAT, msg.c_str());
}

void ServerScript::kick(int kuid, std::string &msg) {
    /* RIGSOFRODS: STUB
    seq->QueueClientForDisconnect(kuid, msg.c_str(), false, false);
    mse->playerDeleted(kuid, 0, true);
    */
}

void ServerScript::ban(int buid, std::string &msg) {
    /* RIGSOFRODS: STUB
    seq->SilentBan(buid, msg.c_str(), false);
    mse->playerDeleted(buid, 0, true);
    */
}

bool ServerScript::unban(int buid) {
    return true; //RIGSOFRODS STUB // return seq->UnBan(buid);
}

std::string ServerScript::getUserName(int uid) {
    /* RIGSOFRODS: STUB
    Client *c = seq->getClient(uid);
    if (!c) return "";


    return Str::SanitizeUtf8(c->user.username);
    */
    return "";
}

void ServerScript::setUserName(int uid, const string &username) {
    /* RIGSOFRODS: STUB
    Client *c = seq->getClient(uid);
    if (!c) return;
    std::string username_sane = Str::SanitizeUtf8(username.begin(), username.end());
    strncpy(c->user.username, username_sane.c_str(), RORNET_MAX_USERNAME_LEN);
    */
}

std::string ServerScript::getUserAuth(int uid) {
    /* RIGSOFRODS: STUB
    Client *c = seq->getClient(uid);
    if (!c) return "none";
    if (c->user.authstatus & RoRnet::AUTH_ADMIN) return "admin";
    else if (c->user.authstatus & RoRnet::AUTH_MOD) return "moderator";
    else if (c->user.authstatus & RoRnet::AUTH_RANKED) return "ranked";
    else if (c->user.authstatus & RoRnet::AUTH_BOT) return "bot";
    //else if(c->user.authstatus & RoRnet::AUTH_NONE)
    */
    return "none";
}

int ServerScript::getUserAuthRaw(int uid) {
    /* RIGSOFRODS: STUB
    Client *c = seq->getClient(uid);
    if (!c) return RoRnet::AUTH_NONE;
    return c->user.authstatus;
    */
    return 0;
}

void ServerScript::setUserAuthRaw(int uid, int authmode) {
    /* RIGSOFRODS: STUB
    Client *c = seq->getClient(uid);
    if (!c) return;
    c->user.authstatus = authmode & ~(RoRnet::AUTH_RANKED | RoRnet::AUTH_BANNED);
    */
}

int ServerScript::getUserColourNum(int uid) {
    /* RIGSOFRODS: STUB
    Client *c = seq->getClient(uid);
    if (!c) return 0;
    return c->user.colournum;
    */
    return 0;
}

void ServerScript::setUserColourNum(int uid, int num) {
    /* RIGSOFRODS: STUB
    Client *c = seq->getClient(uid);
    if (!c) return;
    c->user.colournum = num;
    */
}

std::string ServerScript::getUserToken(int uid) {
    /* RIGSOFRODS: STUB
    Client *c = seq->getClient(uid);
    if (!c) return "";
    return std::string(c->user.usertoken, 40);
    */
    return "";
}

std::string ServerScript::getUserVersion(int uid) {
    /* RIGSOFRODS: STUB
    Client *c = seq->getClient(uid);
    if (!c) return "";
    return std::string(c->user.clientversion, 25);
    */
    return "";
}

std::string ServerScript::getUserIPAddress(int uid) {
    /* RIGSOFRODS: STUB
    Client *client = seq->getClient(uid);
    if (client != nullptr) {
        return client->GetIpAddress();
    }
    */
    return "";
}

std::string ServerScript::getServerTerrain() {
    return ""; // RIGSOFRODS: STUB // return Config::getTerrainName();
}

int ServerScript::sendGameCommand(int uid, std::string cmd) {
    return 0; // RIGSOFRODS: STUB // return seq->sendGameCommand(uid, cmd);
}

void ServerScript::curlRequestAsync(std::string url, string displayname) {
#if WITH_CURL
    CurlTaskContext context;
    context.ctc_url = url;
    context.ctc_displayname = displayname;
    context.ctc_script_engine = this->mse;

    std::packaged_task<void(CurlTaskContext)> pktask(CurlRequestThreadFunc);
    std::thread(std::move(pktask), context).detach();
#endif
}

int ServerScript::getNumClients() {
    return 0; // RIGSOFRODS: STUB //  seq->getNumClients();
}

int ServerScript::getStartTime() {
    return 0; // RIGSOFRODS: STUB //  seq->getStartTime();
}

int ServerScript::getTime() {
    return 0; // RIGSOFRODS: STUB //  Messaging::getTime();
}

void ServerScript::deleteCallback(const std::string &type, const std::string &func, void *obj, int refTypeId) {
    if (refTypeId & asTYPEID_SCRIPTOBJECT && (refTypeId & asTYPEID_OBJHANDLE)) {
        mse->deleteCallbackScript(type, func, *(asIScriptObject **) obj);
    } else if (refTypeId == asTYPEID_VOID) {
        mse->deleteCallbackScript(type, func, NULL);
    } else if (refTypeId & asTYPEID_SCRIPTOBJECT) {
        // We received an object instead of a handle of the object.
        // We cannot allow this because this will crash if the deleteCallback is called from inside a constructor of a global variable.
        mse->setException(
                "server.deleteCallback should be called with a handle of the object! (that is: put an @ sign in front of the object)");

        // uncomment to enable anyway:
        //mse->deleteCallbackScript(type, func, (asIScriptObject*)obj);
    } else {
        mse->setException("The object for the callback has to be a script-class or null!");
    }
}

void ServerScript::setCallback(const std::string &type, const std::string &func, void *obj, int refTypeId) {
    if (refTypeId & asTYPEID_SCRIPTOBJECT && (refTypeId & asTYPEID_OBJHANDLE)) {
        mse->addCallbackScript(type, func, *(asIScriptObject **) obj);
    } else if (refTypeId == asTYPEID_VOID) {
        mse->addCallbackScript(type, func, NULL);
    } else if (refTypeId & asTYPEID_SCRIPTOBJECT) {
        // We received an object instead of a handle of the object.
        // We cannot allow this because this will crash if the setCallback is called from inside a constructor of a global variable.
        mse->setException(
                "server.setCallback should be called with a handle of the object! (that is: put an @ sign in front of the object)");

        // uncomment to enable anyway:
        //mse->addCallbackScript(type, func, (asIScriptObject*)obj);
    } else {
        mse->setException("The object for the callback has to be a script-class or null!");
    }
}

void ServerScript::throwException(const std::string &message) {
    mse->setException(message);
}

std::string ServerScript::get_version() {
    return "";  // RIGSOFRODS: STUB // std::string(VERSION);
}

std::string ServerScript::get_asVersion() {
    return std::string(ANGELSCRIPT_VERSION_STRING);
}

std::string ServerScript::get_protocolVersion() {
    return std::string(RORNET_VERSION);
}

unsigned int ServerScript::get_maxClients() { return 0; } // RIGSOFRODS: STUB // return Config::getMaxClients(); }

std::string ServerScript::get_serverName() { return ""; } // RIGSOFRODS: STUB // Config::getServerName(); }

std::string ServerScript::get_IPAddr() { return ""; } // RIGSOFRODS: STUB // Config::getIPAddr(); }

unsigned int ServerScript::get_listenPort() { return 0u; } // RIGSOFRODS: STUB // return Config::getListenPort(); }

int ServerScript::get_serverMode() { return 0; } // RIGSOFRODS: STUB //  (int)Config::getServerMode(); }

std::string ServerScript::get_owner() { return ""; } // RIGSOFRODS: STUB // Config::getOwner(); }

std::string ServerScript::get_website() { return ""; } // RIGSOFRODS: STUB // Config::getWebsite(); }

std::string ServerScript::get_ircServ() { return ""; } // RIGSOFRODS: STUB // Config::getIRC(); }

std::string ServerScript::get_voipServ() { return ""; } // RIGSOFRODS: STUB // Config::getVoIP(); }

int ServerScript::rangeRandomInt(int from, int to) {
    return (int) (from + (to - from) * ((float) rand() / (float) RAND_MAX));
}

void ServerScript::broadcastUserInfo(int uid) {
    // RIGSOFRODS: STUB // seq->broadcastUserInfo(uid);
}

#endif //USE_ANGELSCRIPT
