
// =============================================================================
// This file is adopted from rorserver at commit 4a7109ae2d9a081ccfdad8cc696dc54efe49acb3
// Changes from the original are marked with "//RIGSOFRODS"
// =============================================================================

/*
This file is part of "Rigs of Rods Server" (Relay mode)

Copyright 2007   Pierre-Michel Ricordel
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

#include "ServerScript.h"

#include "Console.h" // RIGSOFRODS: For putNetMessage()
#include "ServerScriptEngine.h"
#include "ServerScriptLogger.h"
#include "ServerScriptSequencer.h"
#include "Utils.h" // RIGSOFRODS: For SanitizeUtf8String

using namespace RoR;
using namespace AngelScript;

/* class that implements the interface for the scripts */
ServerScript::ServerScript(ServerScriptEngine *se, ServerScriptSequencer* sequencer) : mse(se), seq(sequencer) {
}

ServerScript::~ServerScript() {
}

void ServerScript::log(std::string &msg) {
    Logger::Log(LOG_INFO, "SCRIPT|%s", msg.c_str());
}

void ServerScript::say(std::string &msg, int uid, int type) {
    seq->serverSay(msg, uid, type);
    // RIGSOFRODS: Post message directly to console (thread safe)
    App::GetConsole()->putNetMessage(uid, Console::CONSOLE_SYSTEM_NETCHAT, msg.c_str());
}

void ServerScript::kick(int kuid, std::string &msg) {

    seq->QueueClientForDisconnect(kuid);
    mse->playerDeleted(kuid, 0);
}

void ServerScript::ban(int buid, std::string &msg) {

    this->kick(buid, msg);

}

bool ServerScript::unban(int buid) {
    return true; //RIGSOFRODS STUB // return seq->UnBan(buid);
}

std::string ServerScript::getUserName(int uid) {

    ServerScriptClient *c = seq->getClient(uid);
    if (!c) return "";

    return SanitizeUtf8String(c->user.username);
}

void ServerScript::setUserName(int uid, const std::string &username) {

    ServerScriptClient *c = seq->getClient(uid);
    if (!c) return;
    std::string username_sane = SanitizeUtf8String(username);
    strncpy(c->user.username, username_sane.c_str(), RORNET_MAX_USERNAME_LEN);
}

std::string ServerScript::getUserAuth(int uid) {

    ServerScriptClient *c = seq->getClient(uid);
    if (!c) return "none";
    if (c->user.authstatus & RoRnet::AUTH_ADMIN) return "admin";
    else if (c->user.authstatus & RoRnet::AUTH_MOD) return "moderator";
    else if (c->user.authstatus & RoRnet::AUTH_RANKED) return "ranked";
    else if (c->user.authstatus & RoRnet::AUTH_BOT) return "bot";

    return "none";
}

int ServerScript::getUserAuthRaw(int uid) {

    ServerScriptClient *c = seq->getClient(uid);
    if (!c) return RoRnet::AUTH_NONE;
    return c->user.authstatus;

    return 0;
}

void ServerScript::setUserAuthRaw(int uid, int authmode) {

    ServerScriptClient *c = seq->getClient(uid);
    if (!c) return;
    c->user.authstatus = authmode & ~(RoRnet::AUTH_RANKED | RoRnet::AUTH_BANNED);

}

int ServerScript::getUserColourNum(int uid) {

    ServerScriptClient *c = seq->getClient(uid);
    if (!c) return 0;
    return c->user.colournum;

    return 0;
}

void ServerScript::setUserColourNum(int uid, int num) {

    ServerScriptClient *c = seq->getClient(uid);
    if (!c) return;
    c->user.colournum = num;

}

std::string ServerScript::getUserToken(int uid) {

    ServerScriptClient *c = seq->getClient(uid);
    if (!c) return "";
    return std::string(c->user.usertoken, 40);
}

std::string ServerScript::getUserVersion(int uid) {

    ServerScriptClient *c = seq->getClient(uid);
    if (!c) return "";
    return std::string(c->user.clientversion, 25);

    return "";
}

std::string ServerScript::getUserIPAddress(int uid) {
    /* RIGSOFRODS: STUB
    ServerScriptClient *client = seq->getClient(uid);
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
    seq->sendGameCommand(uid, cmd);
    return 0;
}

void ServerScript::curlRequestAsync(std::string url, std::string displayname) {
#if USE_CURL
    CurlTaskContext context;
    context.ctc_url = url;
    context.ctc_displayname = displayname;
    context.ctc_script_engine = this->mse;

    std::packaged_task<void(CurlTaskContext)> pktask(ServerScriptCurlRequestThreadFunc);
    std::thread(std::move(pktask), context).detach();
#endif
}

int ServerScript::getNumClients() {
    return seq->getNumClients();
}

int ServerScript::getStartTime() {
    return seq->getStartTime();
}

int ServerScript::getTime() {
    return (int) time(NULL);
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

