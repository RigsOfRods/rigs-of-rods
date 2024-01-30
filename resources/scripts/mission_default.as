/*
* Rigs of Rods mission system
* https://forum.rigsofrods.org/threads/mission-system-prototype.3846/
*
* This is a mission handler script. It's responsible for:
* 1) Loading the scenario from "*.mission" file and setting up
*    the terrain objects and/or graphics as defined in the file.
* 2) Monitoring all necessary events while the scenario is loaded.
* 3) Unloading the scenario, undoing all changes on the terrain.
*/

#include "missions.as"

MissionManager g_missions();

/* A mandatory startup function */
void main()
{
    game.registerForEvent(SE_ANGELSCRIPT_MANIPULATIONS); // Necessary to receive mission setup info
    game.log("default mission script loaded");
}

/* Event handler callback */
void eventCallbackEx(scriptEvents ev,   int arg1, int arg2ex, int arg3ex, int arg4ex,   string arg5ex, string arg6ex, string arg7ex, string arg8ex)
{
    if (ev == SE_ANGELSCRIPT_MANIPULATIONS)
    {
        // args: #1 angelScriptManipulationType, #2 ScriptUnitId_t, #3 RoR::ScriptCategory, #4 unused, #5 script file name (*.as), #6 associated file name (i.e. *.mission), #7 associated file resource group (i.e. *.mission).
        angelScriptManipulationType manip = angelScriptManipulationType(arg1);
        int nid = arg2ex;
        
        if (manip == ASMANIP_SCRIPT_LOADED && nid == thisScript)
        {
            bool result = g_missions.loadMission(arg6ex, arg7ex);
            //game.log("DBG mission_default.as: finished loading mission file '"+arg6ex+"' (resource group '"+arg7ex+"'), result: " + result);
        }
        else if (manip == ASMANIP_SCRIPT_UNLOADING && nid == thisScript)
        {
            g_missions.unloadMission();
            //game.log("DBG mission_default.as: finished unloading mission");
        }
    }
}



