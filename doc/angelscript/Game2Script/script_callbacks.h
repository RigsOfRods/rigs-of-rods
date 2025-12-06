
namespace Game2Script {

/** \addtogroup ScriptSideAPIs
 *  @{
 */    

/** \addtogroup Game2Script
 *  @{
 */   

/** Required; Script setup function - invoked once when script is loaded.
*   If not present, the game will report error and abandon the script.
*/
void main();

/** Optional; Script update function - invoked once every rendered frame.
*   @param dt Elapsed time (delta time) in seconds.
*/
void frameStep(float dt);

/** Optional; Invoked if a registered event is triggered, see `GameScriptClass::registerForEvent()`.
*   @param event Event code.
*   @param param Event-specific parameter, see docs for the event codes.
*/
void eventCallback(Script2Game::scriptEvents event, int param);

/** Optional; if present, will be used instead of `Game2Script::eventCallback()`.
*   Invoked if a registered event is triggered, see `GameScriptClass::registerForEvent()`.
*   The first argument is the same as `eventCallback()` gets, other are extras, see descriptions at `Script2Game::scriptEvents`.
*   @param event Event code.
*/
void eventCallbackEx(Script2Game::scriptEvents event, int arg1, int arg2ex, int arg3ex, int arg4ex, string arg5ex, string arg6ex, string arg7ex, string arg8ex);

/** OBSOLETE, ONLY WORKS WITH TERRAIN SCRIPTS - Use `eventCallbackEx()` with event `SE_EVENTBOX_ENTER` instead, it does the same job and works with any script.
*    Optional; Invoked when a vehicle touches an eventbox which has no custom handler function.
*    This is a legacy feature which pre-dates generic events `SE_EVENTBOX_ENTER` and `SE_EVENTBOX_EXIT` and bundled objects like truckshop/spawners still rely on it.
*   @param trigger_type Unused, always 0.
*   @param inst Unique ID of the terrain object instance which created the eventbox.
*   @param box Name of the eventbox as defined by the terrain object's ODEF file.
*   @param nodeid Number of the node which triggered the event, or -1 if not known.
*/
void defaultEventCallback(int trigger_type, string inst, string box, int nodeid);

/** Optional; Invoked before terminating the script.
*   Allows the script to release objects and resources used during runtime.
*/
void dispose();

/** @}*/   //addtogroup Game2Script
/** @}*/   //addtogroup ScriptSideAPIs

} // namespace Game2Script