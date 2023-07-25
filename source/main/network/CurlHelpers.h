/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2013-2023 Petr Ohlidal

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

#pragma once

#include "GameContext.h"

#ifdef USE_CURL
#   include <curl/curl.h>
#   include <curl/easy.h>

#include <string>

namespace RoR {

struct CurlTaskContext
{
    std::string ctc_displayname;
    std::string ctc_url;
    // Send no messages by default
    MsgType ctc_msg_progress = MSG_INVALID;
    MsgType ctc_msg_success = MSG_INVALID;
    MsgType ctc_msg_failure = MSG_INVALID; //!< Sent on failure; Payload: `RoR::ScriptEventArgs` (see 'gameplay/ScriptEvents.h') with args for `RoR::SE_ANGELSCRIPT_THREAD_STATUS`
    // Status
    double ctc_old_perc = 0; // not threadsafe but whatever
};

bool GetUrlAsString(const std::string& url, CURLcode& curl_result, long& response_code, std::string& response_payload);

bool GetUrlAsStringMQ(CurlTaskContext task);

} // namespace RoR

#endif // USE_CURL