/*
    ---------------------------------------------------------------------------
                  Project Rigs of Rods (www.rigsofrods.org)
                            
                                DEMO SCRIPT
                                
    This program showcases all the various things you can do using scripting:
     * Use DearIMGUI to draw UI of any kind, including diagnostic views.
     * Collect and show stats (i.e. frame count, total time)
     * Read/Write cvars (RoR.cfg values, cli args, game state...)
     * View and update game state (current vehicle...)
     
    There are 3 ways of invoking a script:
     1. By defining it with terrain, see terrn2 fileformat, section '[Scripts]':
        https://docs.rigsofrods.org/terrain-creation/terrn2-subsystem/.
        This is the classic old method, used for i.e. races.
     2. By using command line parameter '-runscript <filename>'.
        You can use this command multiple times at once. Added in 2022.
     3. By setting 'app_custom_scripts' in RoR.cfg to a comma-separated list
        of filenames. Spaces in filename are acceptable. Added in 2022.
    
    For introduction to game events, read
    https://docs.rigsofrods.org/terrain-creation/scripting/.

    For reference manual of the interface, see
    https://github.com/RigsOfRods/rigs-of-rods/tree/master/doc/angelscript.
    ---------------------------------------------------------------------------
*/


/*
    ---------------------------------------------------------------------------
    Global variables
*/
int         g_total_frames = 0;
float       g_total_seconds = 0;
CVarClass@  g_app_state = console.cVarFind("app_state"); // 0=bootstrap, 1=main menu, 2=simulation, see AppState in Application.h
CVarClass@  g_sim_state = console.cVarFind("sim_state"); // 0=off, 1=running, 2=paused, 3=terrain editor, see SimState in Application.h
CVarClass@  g_mp_state = console.cVarFind("mp_state"); // 0=disabled, 1=connecting, 2=connected, see MpState in Application.h

/*
    ---------------------------------------------------------------------------
    Script setup function - invoked once when script is loaded.
*/
void main()
{
    log("Hello Rigs of Rods!");
}

/*
    ---------------------------------------------------------------------------
    Script update function - invoked once every rendered frame,
    with elapsed time (delta time, in seconds) as parameter.
*/
void frameStep(float dt)
{
    // Open demo window
    ImGui::SetNextWindowSize(vector2(300, 150));
    ImGui::Begin("Demo Script", /*open:*/true, /*flags:*/0);
    
    // show some stats
    ImGui::Text("Total frames: " + g_total_frames);
    ImGui::Text("Total time: " + int(g_total_seconds / 60) + "min, " 
                               + int(g_total_seconds % 60) + "sec");
    
    // Show some game context
    if (g_app_state.getInt() == 1)
    {
        ImGui::Text("Game state: main menu");
    }
    else
    {
        if (g_mp_state.getInt() == 2)
        {
            ImGui::Text("Game state: multiplayer");
        }
        else
        {
            ImGui::Text("Game state: single player");
        }
        
        if (g_sim_state.getInt() == 2)
        {
            ImGui::SameLine();
            ImGui::Text("(paused)");
        }
        else if (g_sim_state.getInt() == 3)
        {
            ImGui::SameLine();
            ImGui::Text("(terrain edit)");
        }
        
        BeamClass@ actor = game.getCurrentTruck();
        if (actor != null)
        {
            ImGui::Text("You are driving " + actor.getTruckName());
        }
        else
        {
            ImGui::Text("You are on foot");
        }        
    }
    
    // End window
    ImGui::End();

    // Update global counters
    g_total_frames++;
    g_total_seconds += dt;
}