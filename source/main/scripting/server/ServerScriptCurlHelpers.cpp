#include "ServerScriptCurlHelpers.h"
#include "ServerScriptEngine.h"

#ifdef USE_CURL

void RoR::ServerScriptCurlRequestThreadFunc(CurlTaskContext context)
{
    context.ctc_script_engine->curlStatus(CURL_STATUS_START, 0, 0, context.ctc_displayname, "");
    std::string data;
    CURLcode curl_result = CURLE_OK;
    long http_response = 0;
    if (GetUrlAsString(context.ctc_url, /*out:*/curl_result, /*out:*/http_response, /*out:*/data))
    {
        context.ctc_script_engine->curlStatus(CURL_STATUS_SUCCESS, (int)curl_result, (int)http_response, context.ctc_displayname, data);
    }
    else
    {
        context.ctc_script_engine->curlStatus(CURL_STATUS_FAILURE, (int)curl_result, (int)http_response, context.ctc_displayname, curl_easy_strerror(curl_result));
    }
}

#endif // USE_CURL
