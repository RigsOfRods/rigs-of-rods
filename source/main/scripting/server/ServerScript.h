
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

#include "ServerScriptCurlHelpers.h"
#include "angelscript.h"
#include "RoRnet.h"

namespace RoR { // RIGSOFRODS

class ServerScript {
protected:
    ServerScriptEngine *mse;              //!< local script engine pointer, used as proxy mostly
    ServerScriptSequencer *seq;

public:
    ServerScript(ServerScriptEngine *se, ServerScriptSequencer* sequencer);

    ~ServerScript();

    void log(std::string &msg);

    void say(std::string &msg, int uid = -1, int type = 0);

    void kick(int kuid, std::string &msg);

    void ban(int kuid, std::string &msg);

    bool unban(int kuid);

    int playerChat(int uid, char *str);

    std::string getServerTerrain();

    int sendGameCommand(int uid, std::string cmd);

    std::string getUserName(int uid);

    void setUserName(int uid, const std::string &username);

    std::string getUserAuth(int uid);

    int getUserAuthRaw(int uid);

    void setUserAuthRaw(int uid, int authmode);

    int getUserColourNum(int uid);

    void setUserColourNum(int uid, int num);

    std::string getUserToken(int uid);

    std::string getUserVersion(int uid);

    std::string getUserIPAddress(int uid);

    int getUserPosition(int uid, Ogre::Vector3 &v);

    int getNumClients();

    int getStartTime();

    int getTime();

    std::string get_version();

    std::string get_asVersion();

    std::string get_protocolVersion();

    void setCallback(const std::string &type, const std::string &func, void *obj, int refTypeId);

    void deleteCallback(const std::string &type, const std::string &func, void *obj, int refTypeId);

    void throwException(const std::string &message);

    unsigned int get_maxClients();

    std::string get_serverName();

    std::string get_IPAddr();

    unsigned int get_listenPort();

    int get_serverMode();

    std::string get_owner();

    std::string get_website();

    std::string get_ircServ();

    std::string get_voipServ();

    int rangeRandomInt(int from, int to);

    void broadcastUserInfo(int uid);

    /**
     * Launches a background task, use `curlStatus` callback to monitor progress and receive result.
     * @param displayname The "correlation ID" - the label passed to the callback to identify the transfer.
     * @remark Callback signature: `curlStatus(curlStatusType, int n1, int n2, string displayname, string message)`
     * - CURL_STATUS_PROGRESS: n1 = bytes downloaded, n2 = total bytes, message = empty
     * - CURL_STATUS_SUCCESS: n1 = CURL return code, n2 = HTTP result code, message = payload as string
     * - CURL_STATUS_FAILURE: n1 = CURL return code, n2 = HTTP result code, message = CURL error string
     */
    void curlRequestAsync(std::string url, std::string displayname);
};

} // namespace RoR

#endif // USE_ANGELSCRIPT
