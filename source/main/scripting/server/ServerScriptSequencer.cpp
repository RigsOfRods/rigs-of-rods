
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

#include "ServerScriptSequencer.h"

#include "ServerScriptLogger.h"
#include "ServerScriptConfig.h"
#include "ServerScriptEngine.h"
#include "ScriptEngine.h" // RIGSOFRODS: for QueueStringForExecution()
#include "Utils.h"

#include <stdio.h>
#include <time.h>
#include <chrono>
#include <string>
#include <iostream>
#include <stdexcept>
#include <sstream>
#include <memory>

using namespace RoR;


ServerScriptSequencer::ServerScriptSequencer() {
    m_start_time = static_cast<int>(time(nullptr));
}

ServerScriptSequencer::~ServerScriptSequencer() {
    this->Close();
}

int ServerScriptSequencer::Initialize(const std::string& script_filename) {
    m_script_engine = std::unique_ptr<ServerScriptEngine>(new ServerScriptEngine(this));
    return m_script_engine->loadScript(script_filename);
}

void ServerScriptSequencer::Close() {
    // stop the script engine
    m_script_engine->StopTimerThread();
    m_script_engine.reset();
    // delete all clients
    for (unsigned int i = 0; i < m_clients.size(); i++) {
        delete m_clients[i];
    }
    m_clients.clear();
}

bool ServerScriptSequencer::CheckNickIsUnique(std::string &nick) {
    // WARNING: be sure that this is only called within a clients_mutex lock!

    // check for duplicate names
    for (unsigned int i = 0; i < m_clients.size(); i++) {
        if (nick == SanitizeUtf8String(m_clients[i]->user.username)) {
            return true;
        }
    }
    return false;
}


int ServerScriptSequencer::GetFreePlayerColour() {
    // WARNING: be sure that this is only called within a clients_mutex lock!

    int col = 0;
    for (;;) // TODO: How many colors ARE there?
    {
        bool collision = false;
        for (unsigned int i = 0; i < m_clients.size(); i++) {
            if (m_clients[i]->user.colournum == col) {
                collision = true;
                break;
            }
        }
        if (!collision) {
            return col;
        }
        col++;
    }
}

void ServerScriptSequencer::createClient(RoRnet::UserInfo& user) {
    //we have a confirmed client that wants to play
    //try to find a place for him
    Logger::Log(LOG_DEBUG, "got instance in createClient()");

    std::lock_guard<std::mutex> scoped_lock(m_clients_mutex);

	std::string nick = SanitizeUtf8String(user.username);

    if (nick.empty())
    {
        nick = "Anonymous";
        strncpy(user.username, nick.c_str(), RORNET_MAX_USERNAME_LEN - 1);
    }
    if (ServerScriptSequencer::CheckNickIsUnique(nick)) {
        Logger::Log(LOG_WARN, std::string("found duplicate nick, getting new one: ") + nick);

        // shorten username so the number will fit (only if its too long already)
        std::string new_nick_base = nick.substr(0, RORNET_MAX_USERNAME_LEN - 4) + "-";

        for (int i = 2; i < 99; i++) {
            nick = new_nick_base + std::to_string(i);
            if (!ServerScriptSequencer::CheckNickIsUnique(nick)) {
                Logger::Log(LOG_WARN, std::string("New username was composed: ") + nick);
                strncpy(user.username, nick.c_str(), RORNET_MAX_USERNAME_LEN - 1);
                break;
            }
        }
    }

    // assign unique userid
    user.uniqueid = m_free_user_id;
    m_free_user_id++;

    // assign color
    user.colournum = ServerScriptSequencer::GetFreePlayerColour();

    //okay, create the client slot
    ServerScriptClient *to_add = new ServerScriptClient();
    to_add->user = user;
    m_clients.push_back(to_add);

    // Do script callback
#ifdef WITH_ANGELSCRIPT
    if (m_script_engine != nullptr) {
        m_script_engine->playerAdded(client_id);
    }
#endif //WITH_ANGELSCRIPT

    // done!
    Logger::Log(LOG_VERBOSE, "ServerScriptSequencer: New client added");
}

int ServerScriptSequencer::getNumClients() {
    std::lock_guard<std::mutex> scoped_lock(m_clients_mutex);
    return (int) m_clients.size();
}

void ServerScriptSequencer::QueueClientForDisconnect(int uid) {

    ServerScriptClient *client = this->FindClientById(static_cast<unsigned int>(uid));
    if (client == nullptr) {
        Logger::Log(LOG_DEBUG,
            "ServerScriptSequencer::QueueClientForDisconnect() Internal error, got non-existent user ID: %d", uid);
        return;
    }

    // RIGSOFRODS: We always call 'playerDeleted' callback externally - even rorserver does it mostly so.

    EraseIf(m_clients, [uid](ServerScriptClient* client) { return client->user.uniqueid == uid; });
}

void ServerScriptSequencer::sendGameCommand(int uid, std::string cmd) {
    const char *data = cmd.c_str();
    int size = cmd.size();

    // RIGSOFRODS: only forward commands for the player, bots currently don't have any means to deal with them.

    if (uid == TO_ALL || uid == App::GetNetwork()->GetLocalUserData().uniqueid)
    {
        App::GetScriptEngine()->queueStringForExecution(cmd);
    }
}

// this does not lock the clients_mutex, make sure it is locked before hand
// note: uid==-1==TO_ALL = broadcast your message to all players
void ServerScriptSequencer::serverSay(std::string msg, int uid, int type) {
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

    std::string msg_valid = SanitizeUtf8String(msg);
    auto itor = m_clients.begin();
    auto endi = m_clients.end();
    for (; itor != endi; ++itor) {
        ServerScriptClient *client = *itor;
        if ((uid == TO_ALL || ((int) client->user.uniqueid) == uid)) {
            // RIGSOFRODS: for the local player (AUTH_ADMIN), just
        }
    }
}

bool ServerScriptSequencer::Kick(int kuid) {
    ServerScriptClient *kicked_client = this->FindClientById(static_cast<unsigned int>(kuid));
    if (kicked_client == nullptr) {
        return false;
    }

    this->QueueClientForDisconnect(kicked_client->user.uniqueid);
    return true;
}

// RIGSOFRODS: chops of `queueMessage()` in rorserver.

void ServerScriptSequencer::queueMessageStreamRegister(int uid, RoRnet::StreamRegister *reg)
{
    std::lock_guard<std::mutex> scoped_lock(m_clients_mutex);
    ServerScriptClient *client = this->FindClientById(static_cast<unsigned int>(uid));
    if (client == nullptr) {
        return;
    }    
    
    int publishMode = BROADCAST_NORMAL;

    // Do a script callback
    if (m_script_engine) {
        int scriptpub = m_script_engine->streamAdded(client->user.uniqueid, reg);

        // We only support blocking and normal at the moment. Other modes are not supported.
        switch (scriptpub) {
            case BROADCAST_AUTO:
                break;

            case BROADCAST_BLOCK:
                publishMode = BROADCAST_BLOCK;
                break;

            case BROADCAST_NORMAL:
                publishMode = BROADCAST_NORMAL;
                break;

            default:
                Logger::Log(LOG_ERROR, "Stream broadcasting mode not supported.");
                break;
        }
    }

    int streamid = reg->origin_streamid;
    if (publishMode != BROADCAST_BLOCK) {
        // Add the stream
        reg->name[127] = 0;
        Logger::Log(LOG_VERBOSE, " * new stream registered: %d:%d, type: %d, name: '%s', status: %d",
                    client->user.uniqueid, streamid, reg->type, reg->name, reg->status);
        client->streams[streamid] = *reg;
    }
    else
    {
        Logger::Log(LOG_INFO, " * stream reg blocked: %d:%d, type: %d, name: '%s', status: %d",
                    client->user.uniqueid, streamid, reg->type, reg->name, reg->status);
    }
}

void ServerScriptSequencer::queueMessageStreamUnregister(int uid, unsigned int streamid) {
    std::lock_guard<std::mutex> scoped_lock(m_clients_mutex);

    ServerScriptClient *client = this->FindClientById(static_cast<unsigned int>(uid));
    if (client == nullptr) {
        return;
    }
    
    int publishMode = BROADCAST_NORMAL;
    
    // Remove the stream
    if (client->streams.erase(streamid) > 0) {
        Logger::Log(LOG_VERBOSE, " * stream deregistered: %d:%d", client->user.uniqueid, streamid);
        publishMode = BROADCAST_ALL;
    }
}

void ServerScriptSequencer::queueMessageUserLeave(int uid) {
    std::lock_guard<std::mutex> scoped_lock(m_clients_mutex);

    ServerScriptClient *client = this->FindClientById(static_cast<unsigned int>(uid));
    if (client == nullptr) {
        return;
    }

    int publishMode = BROADCAST_NORMAL;

    // from client
    Logger::Log(LOG_INFO, "User disconnects on request: " + SanitizeUtf8String(client->user.username));
    QueueClientForDisconnect(client->user.uniqueid);
    // RIGSOFRODS: RoRserver doesn't seem to invoke 'playerDeleted' on clean disconnect by player, but we do it for consistency.
    m_script_engine->playerDeleted(client->user.uniqueid, 0);
}

void ServerScriptSequencer::queueMessagePlayerChat(int uid, const std::string & data) {
    std::lock_guard<std::mutex> scoped_lock(m_clients_mutex);

    ServerScriptClient *client = this->FindClientById(static_cast<unsigned int>(uid));
    if (client == nullptr) {
        return;
    }

    int publishMode = BROADCAST_NORMAL;

    std::string str = SanitizeUtf8String(data);
    Logger::Log(LOG_INFO, "CHAT| %s: %s", SanitizeUtf8String(client->user.username).c_str(), str.c_str());

    publishMode = BROADCAST_ALL;
    if (str[0] == '!') {
        publishMode = BROADCAST_BLOCK; // no broadcast of server commands!
    }

    // Script callback
    if (m_script_engine) {
        int scriptpub = m_script_engine->playerChat(client->user.uniqueid, str);
        if (scriptpub != BROADCAST_AUTO) publishMode = scriptpub;
    }

    if (str == "!help") {
        serverSay(std::string("builtin commands:"), uid);
        serverSay(std::string("!help !version, !list, !say, !kick"), uid);
    }

    if (str == "!version") {; 
        serverSay("Local script", uid);
    } else if (str == "!list") {
        if (client->user.authstatus & RoRnet::AUTH_MOD || client->user.authstatus & RoRnet::AUTH_ADMIN)
        {
            serverSay(std::string(" uid | auth   | nick   | ip"), uid);
        }
        else
        {
            serverSay(std::string(" uid | auth   | nick"), uid);
        }

        for (unsigned int i = 0; i < m_clients.size(); i++) {
            if (i >= m_clients.size())
                break;
            char authst[10] = "";
            if (m_clients[i]->user.authstatus & RoRnet::AUTH_ADMIN) strcat(authst, "A");
            if (m_clients[i]->user.authstatus & RoRnet::AUTH_MOD) strcat(authst, "M");
            if (m_clients[i]->user.authstatus & RoRnet::AUTH_RANKED) strcat(authst, "R");
            if (m_clients[i]->user.authstatus & RoRnet::AUTH_BOT) strcat(authst, "B");
            if (m_clients[i]->user.authstatus & RoRnet::AUTH_BANNED) strcat(authst, "X");\

            char tmp2[256] = "";

            sprintf(tmp2, "% 3d | %-6s | %-20s", m_clients[i]->user.uniqueid, authst,
                    SanitizeUtf8String(m_clients[i]->user.username).c_str());
            
            serverSay(std::string(tmp2), uid);
        }
 
    } else if (str.substr(0, 6) == "!kick ") {
        if (client->user.authstatus & RoRnet::AUTH_MOD || client->user.authstatus & RoRnet::AUTH_ADMIN) {
            int kuid = -1;
            char kickmsg_tmp[256] = "";
            int res = sscanf(str.substr(6).c_str(), "%d %s", &kuid, kickmsg_tmp);
            std::string kickMsg = std::string(kickmsg_tmp);
            Ogre::StringUtil::trim(kickMsg);
            if (res != 2 || kuid == -1 || !kickMsg.size()) {
                serverSay(std::string("usage: !kick <uid> <message>"), uid);
                serverSay(std::string("example: !kick 3 bye!"), uid);
            } else {
                bool kicked = Kick(kuid);
                if (!kicked)
                    serverSay(std::string("kick not successful: uid not found!"), uid);
            }
        } else {
            // not allowed
            serverSay(std::string("You are not authorized to kick people!"), uid);
        }

    } else if (str.substr(0, 5) == "!say ") {
        if (client->user.authstatus & RoRnet::AUTH_MOD || client->user.authstatus & RoRnet::AUTH_ADMIN) {
            int kuid = -2;
            char saymsg_tmp[256] = "";
            int res = sscanf(str.substr(5).c_str(), "%d %s", &kuid, saymsg_tmp);
            std::string sayMsg = std::string(saymsg_tmp);
            Ogre::StringUtil::trim(sayMsg);
            if (res != 2 || kuid < -1 || !sayMsg.size()) {
                serverSay(std::string("usage: !say <uid> <message> (use uid -1 for general broadcast)"), uid);
                serverSay(std::string("example: !say 3 Wecome to this server!"), uid);
            } else {
                serverSay(sayMsg, kuid, FROM_HOST);
            }

        } else {
            // not allowed
            serverSay(std::string("You are not authorized to use this command!"), uid);
        }
    }
}

void ServerScriptSequencer::queueMessageGameCmd(int uid, const std::string & data) {
    std::lock_guard<std::mutex> scoped_lock(m_clients_mutex);

    ServerScriptClient *client = this->FindClientById(static_cast<unsigned int>(uid));
    if (client == nullptr) {
        return;
    }

    // script message
    if (m_script_engine) m_script_engine->gameCmd(client->user.uniqueid, std::string(data));
}

int ServerScriptSequencer::getStartTime() {
    return m_start_time;
}

ServerScriptClient *ServerScriptSequencer::getClient(int uid) {
    return this->FindClientById(static_cast<unsigned int>(uid));
}


void ServerScriptSequencer::frameStepScripts(float dt)
{

    // All script callbacks must be invoked while clients-mutex is locked
    std::lock_guard<std::mutex> scoped_lock(m_clients_mutex);
    m_script_engine->frameStep(dt);

}

// clients_mutex needs to be locked wen calling this method
// Invoked either from ServerScriptSequencer or ServerScript
ServerScriptClient *ServerScriptSequencer::FindClientById(unsigned int client_id) {
    auto itor = m_clients.begin();
    auto endi = m_clients.end();
    for (; itor != endi; ++itor) {
        ServerScriptClient *client = *itor;
        if (client->user.uniqueid == client_id) {
            return client;
        }
    }
    return nullptr;
}

