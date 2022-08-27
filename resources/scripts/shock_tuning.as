/*
    ---------------------------------------------------------------------------
                  Project Rigs of Rods (www.rigsofrods.org)
                            
                             SHOCK TUNING SCRIPT
                                
    Reads diagnostic values from the game and renders a graph.
     
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

// assume 60FPS = circa 3 sec
const int MAX_SAMPLES = 3*60;

class ShockData
{
    // intialize arrays to size of MAX_SAMPLES and zero out values.
    array<float> spring(MAX_SAMPLES, 0.f);
    array<float> damping(MAX_SAMPLES, 0.f);
    array<float> velocity(MAX_SAMPLES, 0.f);
    
    void addSample(array<float>& samples, float value)
    {

            // Overflow - erase the oldest value and append the new.
            samples.removeAt(0);
            samples.insertLast(value);

    }    
}

/*
    ---------------------------------------------------------------------------
    Global variables
*/
CVarClass@  g_app_state = console.cVarFind("app_state"); // 0=bootstrap, 1=main menu, 2=simulation, see AppState in Application.h
array<ShockData@> g_shock_buffers;
BeamClass@ g_prev_actor = null;

/*
    ---------------------------------------------------------------------------
    Script setup function - invoked once when script is loaded.
*/
void main()
{
    log("Shock tuning script loaded!");
}

/*
    ---------------------------------------------------------------------------
    Script update function - invoked once every rendered frame,
    with elapsed time (delta time, in seconds) as parameter.
*/
void frameStep(float dt)
{
    if (g_app_state.getInt() != 2)
    {
        return; // Don't draw anything in main menu.
    }

    // Open window
    ImGui::Begin("Shock tuning", /*open:*/true, /*flags:*/0);
    
    // Get current vehicle
    BeamClass@ actor = game.getCurrentTruck();
    if (actor != null)
    {
        ImGui::Text("You are driving " + actor.getTruckName() + " (" + actor.getShockCount() + " shocks)");
        
        if (@actor != @g_prev_actor)
        {
            // Actor changed - reset the data buffers!
            g_shock_buffers.removeRange(0, g_shock_buffers.length() - 1); // remove all
            for (int i = 0; i < actor.getShockCount(); i++)
            {
                g_shock_buffers.insertLast(ShockData()); // add new empty objects
            }
        }
        @g_prev_actor = @actor;
        
        // accumulate values
        for (int i = 0; i < actor.getShockCount(); i++)
        {
            ShockData@ buffer = g_shock_buffers[i];
            buffer.addSample(buffer.spring,  actor.getShockSpringRate(i));
            buffer.addSample(buffer.damping,  actor.getShockDamping(i));
            buffer.addSample(buffer.velocity,  actor.getShockVelocity(i));
        }
        
        // draw the graphs
        for (int i = 0; i < actor.getShockCount(); i++)
        {
            ImGui::Separator();
            ImGui::Text("Shock #" + i);
            ShockData@ buffer = g_shock_buffers[i];
            ImGui::PlotLines("Spring rate", buffer.spring, MAX_SAMPLES);
            ImGui::PlotLines("Damping", buffer.damping, MAX_SAMPLES);
            ImGui::PlotLines("Velocity", buffer.velocity, MAX_SAMPLES);
        }        
    }
    else
    {
        ImGui::Text("You are on foot. Press "
            + inputs.getEventCommandTrimmed(EV_COMMON_GET_NEW_VEHICLE)
            + " to select and spawn a vehicle.");
    }
    
    // End window
    ImGui::End();
}

