/// \title SHOCK TUNING SCRIPT
/// \brief Reads diagnostic values from the game and renders a graph.
/// To run in game, open console (hotkey ~) and say  `loadscript example_game_shockTuning.as`
/// ---------------------------------------------------------------------------

// Window [X] button handler
#include "imgui_utils.as"
imgui_utils::CloseWindowPrompt closeBtnHandler;

// #region CONFIG

const int DEFAULT_PLOT_MAX_SAMPLES = 2 * 60; // 2 seconds in wall time (because buffering is done per-FPS, so slowmo=more samples)
int cfgPlotMaxSamples = DEFAULT_PLOT_MAX_SAMPLES;

float cfgSpringPlotMax = 300*1000.f; // N
float cfgSpringPlotMin = 0.f;
float cfgSpringPlotWidth = 100;
float cfgSpringPlotHeight = 45;

float cfgDampPlotMax = 80*1000.f; // N
float cfgDampPlotMin = 0.f;
float cfgDampPlotWidth = 80;
float cfgDampPlotHeight = 35;

float cfgVeloPlotMax = 2.f; // m/s
float cfgVeloPlotMin = 0.f;
float cfgVeloPlotWidth = 80;
float cfgVeloPlotHeight = 35;

float cfgUiCfgItemWidth = 150.f;
float cfgUiSpringBoxWidth = 200.f;
float cfgUiSpringBoxHeight = 166.f;

// in-scene highlight; // shamelessly gutted from 'example_ImGui_nodeHighlight.as'
float cfgNodeRadius = 3.f;
int cfgNodeNumSegments = 5;
color cfgNodeColor = color(0.9, 0.9, 0.3, 1); 
// additions for shock
color cfgSpringHoverColor = color(0.5, 1.0, 0.9, 1); 
color cfgSpringColor = color(0.9, 0.8, 0.5, 1); 
float cfgSpringThickness = 4.f;
float cfgSpringHoverThickness = 8.f;
color cfgBrightTextColor=color(1.f,1.f,0.7f,1.f);

void drawConfigUI()
{
    // cfgPlotBuffeLenSec
    ImGui::SetNextItemWidth(cfgUiCfgItemWidth);
    if (ImGui::InputInt("cfgPlotMaxSamples", cfgPlotMaxSamples))
    {
        for (uint i = 0; i < g_shock_buffers.length(); i++)
        {
            g_shock_buffers[i].resizeBuffers();
        }
    }
    
    ImGui::TextDisabled("UI:");
    ImGui::SetNextItemWidth(cfgUiCfgItemWidth);    ImGui::InputFloat("cfgSpringPlotMax", cfgSpringPlotMax);
    ImGui::SetNextItemWidth(cfgUiCfgItemWidth);    ImGui::InputFloat("cfgSpringPlotMin", cfgSpringPlotMin);
    ImGui::SetNextItemWidth(cfgUiCfgItemWidth);    ImGui::InputFloat("cfgSpringPlotWidth (0=auto)", cfgSpringPlotWidth);
    ImGui::SetNextItemWidth(cfgUiCfgItemWidth);    ImGui::InputFloat("cfgSpringPlotHeight (0=auto)", cfgSpringPlotHeight);    
    ImGui::Separator();
    ImGui::SetNextItemWidth(cfgUiCfgItemWidth);    ImGui::InputFloat("cfgDampPlotMax", cfgDampPlotMax);
    ImGui::SetNextItemWidth(cfgUiCfgItemWidth);    ImGui::InputFloat("cfgDampPlotMin", cfgDampPlotMin);
    ImGui::SetNextItemWidth(cfgUiCfgItemWidth);    ImGui::InputFloat("cfgDampPlotWidth (0=auto)", cfgDampPlotWidth);
    ImGui::SetNextItemWidth(cfgUiCfgItemWidth);    ImGui::InputFloat("cfgDampPlotHeight (0=auto)", cfgDampPlotHeight);    
    ImGui::Separator();
    ImGui::SetNextItemWidth(cfgUiCfgItemWidth);    ImGui::InputFloat("cfgVeloPlotMax", cfgVeloPlotMax);
    ImGui::SetNextItemWidth(cfgUiCfgItemWidth);    ImGui::InputFloat("cfgVeloPlotMin", cfgVeloPlotMin);
    ImGui::SetNextItemWidth(cfgUiCfgItemWidth);    ImGui::InputFloat("cfgVeloPlotWidth (0=auto)", cfgVeloPlotWidth);
    ImGui::SetNextItemWidth(cfgUiCfgItemWidth);    ImGui::InputFloat("cfgVeloPlotHeight (0=auto)", cfgVeloPlotHeight);    
    ImGui::Separator();
    ImGui::SetNextItemWidth(cfgUiCfgItemWidth);    ImGui::InputFloat("cfgUiSpringBoxWidth", cfgUiSpringBoxWidth);
    ImGui::SetNextItemWidth(cfgUiCfgItemWidth);    ImGui::InputFloat("cfgUiSpringBoxHeight", cfgUiSpringBoxHeight);    
    
    ImGui::TextDisabled("highlights:");
    //DOC: bool ColorEdit3(const string&in, color&inout)
    ImGui::SetNextItemWidth(cfgUiCfgItemWidth);    ImGui::ColorEdit3("Node color", cfgNodeColor);
    ImGui::SetNextItemWidth(cfgUiCfgItemWidth);    ImGui::ColorEdit3("Spring color", cfgSpringColor);
    ImGui::SetNextItemWidth(cfgUiCfgItemWidth);    ImGui::ColorEdit3("Spring hover col", cfgSpringHoverColor);
    ImGui::SetNextItemWidth(cfgUiCfgItemWidth);    ImGui::InputFloat("cfgSpringThickness", cfgSpringThickness);
    ImGui::SetNextItemWidth(cfgUiCfgItemWidth);    ImGui::InputFloat("cfgSpringHoverThickness", cfgSpringHoverThickness);
    ImGui::SetNextItemWidth(cfgUiCfgItemWidth);    ImGui::ColorEdit3("cfgBrightTextColor", cfgBrightTextColor);
}

// #endregion

// #region SHOCK DATA
class SpringData
{
    // intialize arrays to size of cfgPlotMaxSamples and zero out values.
    array<float> spring(DEFAULT_PLOT_MAX_SAMPLES, 0.f);
    array<float> damping(DEFAULT_PLOT_MAX_SAMPLES, 0.f);
    array<float> velocity(DEFAULT_PLOT_MAX_SAMPLES, 0.f);
    float scenePlacementBetweenNodes= 0.5f; // 0.5= in between, 2.0 = extrapolated behind node2 etc,,,
    bool drawInScene = false;
    
    void addSample(array<float>& samples, float value)
    {
        
        // Overflow - erase the oldest value and append the new.
        samples.removeAt(0);
        samples.insertLast(value);
    }
    
    void resizeBuffers()
    {
        this.spring.resize(cfgPlotMaxSamples);    
        this.damping.resize(cfgPlotMaxSamples);    
        this.velocity.resize(cfgPlotMaxSamples);   
    }
    
    void drawSpringUiBox()
    {
        
        // DOC: PlotLines(const string&in label, array<float>&in values, int values_count, int values_offset = 0, const string&in overlay_text = string(), float scale_min = FLT_MAX, float scale_max = FLT_MAX, vector2 graph_size = vector2(0,0))
        vector2 cursor = ImGui::GetCursorScreenPos();
        ImGui::PlotLines("Spring rate", this.spring, cfgPlotMaxSamples, 0, "",  cfgSpringPlotMin, cfgSpringPlotMax, vector2(cfgSpringPlotWidth, cfgSpringPlotHeight));
        ImGui::GetWindowDrawList().AddText(cursor +vector2(cfgSpringPlotWidth + 10, 18), cfgBrightTextColor, ''+ this.spring[this.spring.length() - 1]);
        
        cursor = ImGui::GetCursorScreenPos();
        ImGui::PlotLines("Damping", this.damping, cfgPlotMaxSamples,0, "",  cfgDampPlotMin, cfgDampPlotMax, vector2(cfgDampPlotWidth, cfgDampPlotHeight));
        ImGui::GetWindowDrawList().AddText(cursor +vector2(cfgDampPlotWidth + 8, 18), cfgBrightTextColor, ''+ this.velocity[this.spring.length() - 1]);
        
        cursor = ImGui::GetCursorScreenPos();
        ImGui::PlotLines("Velocity", this.velocity, cfgPlotMaxSamples,0, "",  cfgVeloPlotMin, cfgVeloPlotMax, vector2(cfgVeloPlotWidth, cfgVeloPlotHeight));
        ImGui::GetWindowDrawList().AddText(cursor +vector2(cfgVeloPlotWidth + 8, 18), cfgBrightTextColor, ''+ this.velocity[this.spring.length() - 1]);
    }
}

array<SpringData@> g_shock_buffers;
// #endregion

// #region GAME CALLBACKS
CVarClass@  g_app_state = console.cVarFind("app_state"); // 0=bootstrap, 1=main menu, 2=simulation, see AppState in Application.h
BeamClass@ g_prev_actor = null;

void main()
{
    log("Spring tuning script loaded!");
}

//Script update function - invoked once every rendered frame, with elapsed time (delta time, in seconds) as parameter.
void frameStep(float dt)
{
    if (g_app_state.getInt() != 2)
    {
        return; // Don't draw anything in main menu.
    }
    
    // Open window
    ImGui::Begin("*ALPHA!* Spring tuning", closeBtnHandler.windowOpen, /*flags:*/0);
    closeBtnHandler.draw();
    ImGui::Dummy(vector2(300,1)); // force some minimum width
    ImGui::TextDisabled("::: This script is a stub :::");
    ImGui::TextDisabled("::: Please give feedback to improve it :::");
    ImGui::Separator();
    
    if (ImGui::CollapsingHeader("Config"))
    {
        drawConfigUI();
    }
    
    // Get current vehicle
    BeamClass@ actor = game.getCurrentTruck();
    if (@actor != null)
    {
        ImGui::Text("You are driving " + actor.getTruckName() + " (" + actor.getShockCount() + " shocks)");
        ImGui::Text("Hover the checkboxes for in-scene highlight");
        
        if (@actor != @g_prev_actor)
        {
            // Actor changed - reset the data buffers!
            g_shock_buffers.removeRange(0, g_shock_buffers.length() - 1); // remove all
            for (int i = 0; i < actor.getShockCount(); i++)
            {
                g_shock_buffers.insertLast(SpringData()); // add new empty objects
            }
        }
        @g_prev_actor = @actor;
        
        // accumulate values
        for (int i = 0; i < actor.getShockCount(); i++)
        {
            SpringData@ buffer = g_shock_buffers[i];
            buffer.addSample(buffer.spring,  actor.getShockSpringRate(i));
            buffer.addSample(buffer.damping,  actor.getShockDamping(i));
            buffer.addSample(buffer.velocity,  actor.getShockVelocity(i));
        }
        
        // draw the graphs
        for (int i = 0; i < actor.getShockCount(); i++)
        {
            ImGui::PushID(i);
            SpringData@ shockdata = g_shock_buffers[i];
            ImGui::Checkbox("Spring #"+i, shockdata.drawInScene);
            if (ImGui::IsItemHovered())
            {
                // drawNodeHighlight(actor, actor.getShockNode1(i));
                drawSpringHighlight(actor, i, cfgSpringHoverColor, cfgSpringHoverThickness);
            }
            if (shockdata.drawInScene)
            {
                ImGui::SameLine(); ImGui::SetNextItemWidth(75.f);
                ImGui::SliderFloat("placement", shockdata.scenePlacementBetweenNodes, -5.f, 5.f);
            }
            ImGui::PopID(); // i
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
    
    if (@actor != null)
    {
        // Draw the individual shock in-scene windows
        for (int i = 0; i < actor.getShockCount(); i++)
        {
            SpringData@ shockdata = g_shock_buffers[i];
            if (shockdata.drawInScene)
            {
                drawSpringHighlight(actor, i, cfgSpringColor, cfgSpringThickness);
                drawSpringSceneHud(actor, i);
            }
        }
    }
}

// #endregion

// #region SCENE HUD

void drawSpringSceneHud(BeamClass@ actor, int shockID)
{
    // determine where the window is on screen from scene position
    vector3 n1world = actor.getNodePosition(actor.getShockNode1(shockID));
    vector3 n2world = actor.getNodePosition(actor.getShockNode2(shockID));
    
    vector2 n1pos = vector2(0,0);
    vector2 n2pos = vector2(0,0);
    bool inFrontOfCamera = game.getScreenPosFromWorldPos(n1world, /*out:*/ n1pos) && game.getScreenPosFromWorldPos(n2world, /*out:*/ n2pos);
    
    if (!inFrontOfCamera)
    {
        return;
    }
    
    // calculate placement based on user input
    vector2 pivot = n1pos;
    vector2 beamVec = n2pos - n1pos;
    ImGui::SetNextWindowPos(pivot + (beamVec * g_shock_buffers[shockID]. scenePlacementBetweenNodes));
    ImGui::SetNextWindowSize(vector2(cfgUiSpringBoxWidth, cfgUiSpringBoxHeight));
    int window_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize;
    bool boxOpen=true;
    if (    ImGui::Begin("shock #"+shockID, boxOpen, window_flags))
    {
        ImGui::SetCursorPosY(ImGui::GetCursorPosY()-5);
        ImGui::TextDisabled("Shock #"+shockID);
        g_shock_buffers[shockID].drawSpringUiBox();
        ImGui::End();
    }
}

// #endregion

// #region SCENE HIGHLIGHT
// shamelessly gutted from 'example_ImGui_nodeHighlight.as' `drawNodeHighlight()`
void drawSpringHighlight(BeamClass@ actor, int shockID, color beamColor, float beamThickness)
{
    // determine where the shock is on screen
    vector3 n1world = actor.getNodePosition(actor.getShockNode1(shockID));
    vector3 n2world = actor.getNodePosition(actor.getShockNode2(shockID));
    
    vector2 n1pos = vector2(0,0);
    vector2 n2pos = vector2(0,0);
    bool inFrontOfCamera = game.getScreenPosFromWorldPos(n1world, /*out:*/ n1pos) && game.getScreenPosFromWorldPos(n2world, /*out:*/ n2pos);
    
    if (inFrontOfCamera)
    {
        // Draw the nodes
        ImDrawList@ drawlist = imgui_utils::ImGetDummyFullscreenWindow("nodeHighlights");
        
        //DOC: void ImDrawList::AddCircleFilled(const ImVec2& center, float radius, ImU32 col, int num_segments)
        drawlist.AddCircleFilled(n1pos, cfgNodeRadius, cfgNodeColor, cfgNodeNumSegments);
        drawlist.AddCircleFilled(n2pos, cfgNodeRadius, cfgNodeColor, cfgNodeNumSegments);
        
        // Draw the beam
        drawlist.AddLine(n1pos, n2pos, beamColor, beamThickness);
    }
}

// #endregion

