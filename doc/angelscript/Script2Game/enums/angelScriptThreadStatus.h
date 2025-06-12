
  // =================================================== //
  // THIS IS NOT A C++ HEADER! Only a dummy for Doxygen. //
  // =================================================== //

/** \addtogroup ScriptSideAPIs
 *  @{
 */    

/** \addtogroup Script2Game
 *  @{
 */   

namespace Script2Game {

/**
* Binding of RoR::angelScriptThreadStatus
*/
enum angelScriptThreadStatus
{
    ASTHREADSTATUS_NONE,
    ASTHREADSTATUS_CURLSTRING_PROGRESS, //!< Args of `RoR::SE_ANGELSCRIPT_THREAD_STATUS`: arg#1 type, arg#2 percentage, arg#3 unused, arg#4 unused, arg#5 progress message (formatted by RoR)
    ASTHREADSTATUS_CURLSTRING_SUCCESS,  //!< Args of `RoR::SE_ANGELSCRIPT_THREAD_STATUS`: arg#1 type, arg#2 HTTP code, arg#3 CURLcode, arg#4 unused, arg#5 payload
    ASTHREADSTATUS_CURLSTRING_FAILURE,  //!< Args of `RoR::SE_ANGELSCRIPT_THREAD_STATUS`: arg#1 type, arg#2 HTTP code, arg#3 CURLcode, arg#4 unused, arg#5 message from `curl_easy_strerror()`
};

} // namespace Script2Game

/// @}    //addtogroup Script2Game
/// @}    //addtogroup ScriptSideAPIs