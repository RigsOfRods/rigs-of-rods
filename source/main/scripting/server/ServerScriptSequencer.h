
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

#pragma once

#ifdef USE_ANGELSCRIPT

#include "ForwardDeclarations.h"

#include <map>
#include <mutex>
#include <thread>
#include <vector>
#include "angelscript.h"
#include "RoRnet.h"

namespace RoR { // RIGSOFRODS

// This is used to define who says it, when the server says something
enum serverSayType {
    FROM_SERVER = 0,
    FROM_HOST,
    FROM_MOTD,
    FROM_RULES
};

enum broadcastType {
// order: least restrictive to most restrictive!
    BROADCAST_AUTO = -1,  // Do not edit the publishmode (for scripts only)
    BROADCAST_ALL,        // broadcast to all clients including sender
    BROADCAST_NORMAL,     // broadcast to all clients except sender
    BROADCAST_AUTHED,     // broadcast to authed users (bots)
    BROADCAST_BLOCK       // no broadcast
};

// constant for functions that receive an uid for sending something
static const int TO_ALL = -1;

struct ServerScriptClient { // RIGSOFRODS: Bare minimum from `class Client` in rorserver.
    RoRnet::UserInfo user;  //!< user information
    std::map<unsigned int, RoRnet::StreamRegister> streams;
};

class ServerScriptSequencer // RIGSOFRODS: Bare minimum from `class Sequencer` in rorserver.
{
    friend class ServerScript;
public:
    ServerScriptSequencer();
    ~ServerScriptSequencer();

    int Initialize(const std::string& script_filename); // Creates scripting engine and loads the server script. Returns 0 on success.
    bool IsRunning() { return m_script_engine != nullptr; }
    void Close();

    // Synchronized public interface
    void createClient(RoRnet::UserInfo& user); // Performs the 'playerAdded' callback.
    int getNumClients();
    void frameStepScripts(float dt);
    int getStartTime();

    // RIGSOFRODS: chops of `queueMessage()` in rorserver.
    void queueMessageStreamRegister(int uid, RoRnet::StreamRegister *reg);
    void queueMessageStreamUnregister(int uid, unsigned int streamid);
    void queueMessageUserLeave(int uid);
    void queueMessagePlayerChat(int uid, const std::string & data);
    void queueMessageGameCmd(int uid, const std::string & data);
    
private:
    // Helpers (not thread safe - only call when clients-mutex is locked!)
    ServerScriptClient*      FindClientById(unsigned int client_id);
    ServerScriptClient*      getClient(int uid);
    void                     QueueClientForDisconnect(int client_id); // RIGSOFRODS: We always call 'playerDeleted' callback externally - even rorserver does it mostly so.
    void                     serverSay(std::string msg, int uid = -1, int type = 0);
    void                     sendGameCommand(int uid, std::string cmd);
    bool                     CheckNickIsUnique(std::string &nick);
    int                      GetFreePlayerColour();
    bool                     Kick(int to_kick_uid);

    std::unique_ptr<ServerScriptEngine> m_script_engine;

    std::mutex m_clients_mutex;  //!< guards access to `m_clients` and execution of script callbacks.
    std::vector<ServerScriptClient *> m_clients;
    unsigned int m_free_user_id = 1;
    int m_start_time = 0;
};

} // namespace RoR

#endif //USE_ANGELSCRIPT
