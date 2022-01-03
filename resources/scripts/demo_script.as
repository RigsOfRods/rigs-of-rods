/*
    ---------------------------------------------------------------------------
                  Project Rigs of Rods (www.rigsofrods.org)
                            
                                DEMO SCRIPT
                                
    This program showcases all the various things you can do using scripting:
     * Use DearIMGUI to draw UI of any kind, including diagnostic views.
     * Collect and show stats (i.e. frame count, total time)
     
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
int   g_total_frames;
float g_total_seconds;

/*
    ---------------------------------------------------------------------------
    Script setup function - invoked once when script is loaded.
*/
void main()
{
    log("demo_script: invoked `main()`");
    g_total_frames = 0;
    g_total_seconds = 0;
}

/*
    ---------------------------------------------------------------------------
    Script update function - invoked once every rendered frame,
    with elapsed time (delta time, in seconds) as parameter.
*/
void frameStep(float dt)
{
    // Open demo window
    ImGui::Begin("Demo Script", /*open:*/true, /*flags:*/0);
    
    // show some stats
    ImGui::TextDisabled("-- Example stats --");
    ImGui::Text("Total frames: " + g_total_frames);
    int minutes = g_total_seconds / 60;
    int seconds = g_total_seconds % 60;
    ImGui::Text("Total time: " + minutes + "min, " + seconds + "sec");
    
    // End window
    ImGui::End();

    // Update global counters
    g_total_frames++;
    g_total_seconds += dt;
}