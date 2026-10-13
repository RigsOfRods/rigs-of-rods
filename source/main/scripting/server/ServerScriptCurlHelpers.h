
#pragma once

#ifdef USE_CURL

#include "CurlHelpers.h" // RIGSOFRODS: This is actually the client's header with same name as original server header. Both implement the same functionality but with cosmetic differences.

namespace RoR { // RIGSOFRODS

enum ServerScriptCurlStatusType
{
    CURL_STATUS_INVALID,  //!< Should never be reported.
    CURL_STATUS_START,    //!< New CURL request started, n1/n2 both 0.
    CURL_STATUS_PROGRESS, //!< Download in progress, n1 = bytes downloaded, n2 = total bytes.
    CURL_STATUS_SUCCESS,  //!< CURL request finished, n1 = CURL return code, n2 = HTTP result code, message = received payload.
    CURL_STATUS_FAILURE,  //!< CURL request finished, n1 = CURL return code, n2 = HTTP result code, message = CURL error string.
};

void ServerScriptCurlRequestThreadFunc(CurlTaskContext context);

} // namespace RoR

#endif // USE_CURL
