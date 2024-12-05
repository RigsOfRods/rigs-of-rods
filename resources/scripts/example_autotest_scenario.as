/// \title automated test script
/// \brief originally written to investigate https://github.com/RigsOfRods/rigs-of-rods/pull/3042, left around as example
/// =====================================================
/// PRIMARY SCENARIO (now fixed):
///   I test like this: load a map with AI waypoints preset,
///   load it and Start 3 agoras, then Stop. after 3-4 tries, kaboom
/// SECONDARY SCENARIO (investigating):
///   Set `cfgStopAfterNumDeletes = 1`, wait for the script to interrupt
///   and then wait some 10 seconds - you get a segfault
///   (tested on Win10, VisualStudio2022, Debug)
// --------------------------------------------------------------

// Global vars - config
string cfgTerrnFilename = "simple2.terrn2";
string cfgVehicleFilename = "b6b0UID-semi.truck";
uint cfgNumSpawns = 3;
int cfgStopAfterNumDeletes = 1; // Optional, Use -1 to disable; this seems to reproduce a dangling pointer crash in SoundScriptManager.

// Global vars - test step tracking
typedef int StepNum_t;
StepNum_t gStep = 0;
StepNum_t gStepLogged = -1;
StepNum_t gStepSpawned = -1;

// Global vars - event tracking
int gTotalSpawns = 0;
int gTotalDeletes = 0;
array<int> gVehicleIDs;

// Global vars - console variable aliases
CVarClass@ gCvarAppState; // 0=bootstrap, 1=main menu, 2=simulation, see AppState in Application.h
CVarClass@ gCvarSimState; // 0=off, 1=running, 2=paused, 3=terrain editor, see SimState in Application.h

void main()
{
    @gCvarAppState = console.cVarFind("app_state");
    @gCvarSimState = console.cVarFind("sim_state");
    game.registerForEvent(SE_GENERIC_NEW_TRUCK); // a vehicle was spawned
    game.registerForEvent(SE_GENERIC_DELETED_TRUCK); // a vehicle was removed
    
    //  AI setup (see https://github.com/RigsOfRods/rigs-of-rods/pull/3045)
    //  -------------------------------------------------------------------
    
    // the filename
    // first parameter is index - only use 0/1 for drag race and crash modes. Otherwise use 0.
    game.setAIVehicleName(0, cfgVehicleFilename);

    // 0) Normal driving mode
    // 1) Race
    // 2) Drag Race
    // 3) Crash driving mode
    // 4) Chase the player mode
    game.setAIMode(4);

 
    
    game.log("Automated test script: enters preconfigured terrain ("+cfgTerrnFilename+") and keeps loading/unloading preconfigured vehicle ("+cfgTerrnFilename+")");
}

void eventCallbackEx(scriptEvents ev,   int arg1, int arg2ex, int arg3ex, int arg4ex,   string arg5ex, string arg6ex, string arg7ex, string arg8ex)
{
    if (ev == SE_GENERIC_NEW_TRUCK)
    {
        gVehicleIDs.insertLast(arg1);
        if (gStepSpawned != gStep)
        {
            gStepSpawned = gStep;
        }
    }
    else if (ev == SE_GENERIC_DELETED_TRUCK)
    {
        game.log("Autotest: vehicle deleted, ID: " + arg1);
        gVehicleIDs.removeAt(gVehicleIDs.find(arg1));
        
    }
}

void frameStep(float dt)
{
    // The test actions
    switch (gStep)
    {
        case 0:
            testLogStep("Waiting for main menu (to load preconfigured terrain "+cfgTerrnFilename+")...");
            // this isn't really needed now since `frameStep()` updates are halted during bootstrap, but let's pretend we do async rendering :)
            // UPDATE: since this became an example it's useful to let users return to main menu.
            if (gCvarAppState.getInt() == 1)
            {
                gStep++;
            }
            break;
            
        case 1:
            testLogStep("Loading terrain " + cfgTerrnFilename);
            game.pushMessage(MSG_SIM_LOAD_TERRN_REQUESTED, { {"filename", cfgTerrnFilename} });
            gStep++;
            break;
            
        case 2:
            testLogStep("Waiting for terrain to load...");
            // this isn't really needed now since `frameStep()` updates are halted during terrain loading, but let's pretend we do async rendering :)
            if (gCvarAppState.getInt() == 2)
            {
                gStep++;
            }
            break;
            
        case 3:
            // this was dropped
            //testLogStep("Freezing physics");
            //game.pushMessage(MSG_SIM_FREEZE_PHYSICS_REQUESTED, {});
            gStep++;
            break;
            
        case 4:
            testLogStep("Adding waypoints");
            // define the start position by inserting initial waypoint.
            game.addWaypoint(game.getPersonPosition() + vector3(6, 0, 6)); // 6 meters away from player

            // define the start direction by inserting another waypoint
            game.addWaypoint(game.getPersonPosition()); // look at player!   
            gStep++;
            break;
            
        case 5:
            testLogStep("Spawning AI vehicle ");
            // See: https://github.com/RigsOfRods/rigs-of-rods/pull/3045
            // Request loading the AI script (asynchronously) - it will spawn the vehicle.
            // WARNING: this doesn't save off the setup values above - you can still modify them below and change what the AI will do!
            //          If you want to launch multiple AIs in sequence, register for SE_GENERIC_NEW_TRUCK event - when it arrives, it's safe to setup and launch new AI script.
            game.pushMessage(MSG_APP_LOAD_SCRIPT_REQUESTED, { {"filename", "AI.as"} });
            gTotalSpawns++;
            gStep++;
            break;
            
        case 6:
            testLogStep("Waiting for AI vehicle to spawn");
            if (gStepSpawned == gStep)
            {
                gStep++;
            }
            break;
            
        case 7:
            testLogStep("Checking spawn count: "+gVehicleIDs.length()+"/"+cfgNumSpawns);
            if (gVehicleIDs.length() == cfgNumSpawns)
                gStep++;
            else
                gStep = 5; // go back
            break;
            
        case 8:
            testLogStep("Removing AI vehicles");
            for (uint i = 0; i < gVehicleIDs.length(); i++)
            {
                game.pushMessage(MSG_SIM_DELETE_ACTOR_REQUESTED, { {'instance_id', gVehicleIDs[i]} });
                gTotalDeletes++;
                
                // Optional abort: this seems to reproduce a dangling pointer crash in SoundScriptManager.
                if (cfgStopAfterNumDeletes > 0 && gTotalDeletes == cfgStopAfterNumDeletes)
                {
                    game.log("Autotest: interrupting, reached stop count of deletes: " + cfgStopAfterNumDeletes);
                    gStep = -100; // Interrupt test
                    break;
                }
            }
            gStep++;
            break;
            
        case 9:
            testLogStep("Waiting for AI vehicles to disappear");
            if (gVehicleIDs.length() == 0)
                gStep++;
            break;
            
        case 10:
            testLogStep("repeating from step 5");
            gStep = 5;
            break;
            
        default:
            break;
    }
}

// Log helper - log only once per step
void testLogStep(string s)
{
    if (gStepLogged != gStep)
    {
        game.log("Automated test step "+gStep+": " + s);
        gStepLogged = gStep;
    }
}

