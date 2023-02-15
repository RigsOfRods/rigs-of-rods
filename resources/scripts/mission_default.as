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
    game.log("default mission script loaded");
}

/* A mandatory callback for setting up a mission */
bool loadMission(string filename, string resource_group)
{
    game.log("loading mission file '"+filename+"' (resource group '"+resource_group+"')");
    bool result = g_missions.loadMission(filename, resource_group);
    game.log("finished loading mission file '"+filename+"' (resource group '"+resource_group+"'), result: " + result);
    return result;
}

bool unloadMission()
{
    return false; // TBD
}


