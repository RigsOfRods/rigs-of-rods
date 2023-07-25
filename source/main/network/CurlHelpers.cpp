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

#include "Application.h"
#include "CurlHelpers.h"
#include "GameContext.h"
#include "RoRVersion.h"
#include "ScriptEvents.h"

#include <fmt/format.h>

using namespace RoR;

#ifdef USE_CURL
#   include <curl/curl.h>
#   include <curl/easy.h>

static size_t CurlStringWriteFunc(void *ptr, size_t size, size_t nmemb, std::string* data)
{
    data->append((char*)ptr, size * nmemb);
    return size * nmemb;
}

static size_t CurlProgressFunc(void* ptr, double filesize_B, double downloaded_B)
{
    // Ensure that the file to be downloaded is not empty because that would cause a division by zero error later on
    if (filesize_B <= 0.0)
    {
        return 0;
    }

    CurlTaskContext* context = (CurlTaskContext*)ptr;

    double perc = (downloaded_B / filesize_B) * 100;

    if (perc > context->ctc_old_perc && context->ctc_msg_progress != MSG_INVALID)
    {
        ScriptEventArgs* args = new ScriptEventArgs;
        args->arg1 = ASTHREADSTATUS_CURLSTRING_PROGRESS;
        args->arg2ex = perc;
        args->arg5ex = fmt::format("{} {}\n{}: {:.2f}{}\n{}: {:.2f}{}", "Downloading", 
            context->ctc_displayname, "File size", filesize_B/(1024 * 1024), "MB", "Downloaded", downloaded_B/(1024 * 1024), "MB");
        
        App::GetGameContext()->PushMessage(Message(context->ctc_msg_progress, args));
    }

    context->ctc_old_perc = perc;

    // If you don't return 0, the transfer will be aborted - see the documentation
    return 0;
}

bool RoR::GetUrlAsString(const std::string& url, CURLcode& curl_result, long& response_code, std::string& response_payload)
{
    std::string response_header;
    std::string user_agent = fmt::format("{}/{}", "Rigs of Rods Client", ROR_VERSION_STRING);

    CURL *curl = curl_easy_init();
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_IPRESOLVE, CURL_IPRESOLVE_V4);
#ifdef _WIN32
    curl_easy_setopt(curl, CURLOPT_SSL_OPTIONS, CURLSSLOPT_NATIVE_CA);
#endif // _WIN32
    curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "gzip");
    curl_easy_setopt(curl, CURLOPT_USERAGENT, user_agent.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, CurlStringWriteFunc);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_payload);
    curl_easy_setopt(curl, CURLOPT_HEADERDATA, &response_header);

    curl_result = curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);

    curl_easy_cleanup(curl);
    curl = nullptr;

    if (curl_result != CURLE_OK || response_code != 200)
    {
        Ogre::LogManager::getSingleton().stream()
            << "[RoR|CURL] Failed to retrieve url '"<<url<<"' as string;"
            << " Error: '" << curl_easy_strerror(curl_result) << "'; HTTP status code: " << response_code;
        response_payload = curl_easy_strerror(curl_result);
        return false;
    }

    return true;
}

bool RoR::GetUrlAsStringMQ(CurlTaskContext task)
{
    std::string data;
    CURLcode curl_result = CURLE_OK;
    long http_response = 0;
    bool result = GetUrlAsString(task.ctc_url, /*out:*/curl_result, /*out:*/http_response, /*out:*/data);
      
    if (!result && task.ctc_msg_failure != MSG_INVALID)
    {
        // `data` contains the `curl_easy_strerror()` message (already logged)
        ScriptEventArgs* args = new ScriptEventArgs();
        args->arg1 = ASTHREADSTATUS_CURLSTRING_FAILURE;
        args->arg2ex = http_response;
        args->arg3ex = curl_result;
        args->arg5ex = data;

        App::GetGameContext()->PushMessage(Message(task.ctc_msg_failure, args));
    }
    else if (result && task.ctc_msg_failure != MSG_INVALID)
    {
        // `data` contains the `CURL_WRITEDATA` payload.
        ScriptEventArgs* args = new ScriptEventArgs();
        args->arg1 = ASTHREADSTATUS_CURLSTRING_SUCCESS;
        args->arg2ex = http_response;
        args->arg3ex = curl_result;
        args->arg5ex = data;

        App::GetGameContext()->PushMessage(Message(task.ctc_msg_failure, args));
    }

    return result;
}

#endif //USE_CURL