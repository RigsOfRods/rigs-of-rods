/// \title NODE HIGHLIGHT DEMO
/// \brief How to draw nodes and detect/highligt mouse interaction
/// 
/// File structure (#region-s)
/// * GAME CALLBACKS - funcs invoked by game
/// * CONFIG - vars and UI funcs
/// * NODE INTERACTION - vars and drawing/UI funcs
/// * HELPERS - funcs
/// ===================================================

//#region GAME CALLBACKS:

// `main()` runs once when script is loaded.
void main()
{
    game.registerForEvent(SE_TRUCK_ENTER);
    prepareNodeSelectionForNewActor(game.getCurrentTruck());
}

//  `frameStep()` runs every frame; `dt` is delta time in seconds.
void frameStep(float dt)
{
    ImGui::TextDisabled("::: NODE HIGHLIGHT DEMO:::");
    ImGui::Separator();
    BeamClass@ actor = game.getCurrentTruck();
    if (@actor == null) 
    {
        ImGui::Text("you are on foot");
    }
    else
    {
        ImGui::Text("Driving '"+actor.getTruckName()+"' with total "+actor.getNodeCount()+" nodes ("+actor.getWheelNodeCount()+" wheel nodes)");
        
        drawNodeHighlights(actor);
        
        // Left mouse button click = flip the selection state of the
        if (mouseHoveredNodeID != -1 && ImGui::IsMouseClicked(0))
        {
            nodeSelectedStates[mouseHoveredNodeID] = !nodeSelectedStates[mouseHoveredNodeID];
        }
        
        
        drawNodeSelectionUI();
        
        if (ImGui::CollapsingHeader("Config"))
        {
            drawConfigUI();
        }
    }
}

// Handles events registered using `game.registerForEvent()`
void eventCallback(scriptEvents ev, int arg1, int arg2ex, int arg3ex, int arg4ex, string arg5ex, string arg6ex, string arg7ex, string arg8ex)
{
    if (ev == SE_TRUCK_ENTER)
    {
        prepareNodeSelectionForNewActor(game.getTruckByNum(arg1));
    }
}

//#endregion

// #region CONFIG

color cfgNodeColor = color(0.9, 0.8, 0.5, 1);
color cfgRimNodeColor = color(0.4, 0.8, 0.5, 1);
color cfgTireNodeColor = color(0.5, 0.8, 0.8, 1);
float cfgNodeRadius = 3.f;
int cfgNodeNumSegments = 5;
bool cfgShowWheelNodes = true;
int cfgNodeHoverMaxCursorDist = 10; // Only confirm the hovered state if the cursor is less than 10 pixels away from it in any direction.
color cfgNodeHoverColor = color(1,0,0,1);
float cfgNodeHoverRadius = 5.f;
float cfgNodeHoverThickness = 2.f;
color cfgSelectedNodeColor = color(0.8f,1.f,0.1f,1.f);
float cfgSelectedNodeSize = 7.f; // drawn as rectangle

void drawConfigUI()
{
    
    ImGui::InputFloat("Node radius", cfgNodeRadius);
    ImGui::InputInt("Node num segments", cfgNodeNumSegments);
    //DOC: bool ColorEdit3(const string&in, color&inout)
    ImGui::ColorEdit3("Node color", cfgNodeColor);
    ImGui::Checkbox("Show wheel nodes", cfgShowWheelNodes);
    if (cfgShowWheelNodes)
    {
        ImGui::ColorEdit3("Rim node color", cfgRimNodeColor);
        ImGui::ColorEdit3("Tire node color", cfgTireNodeColor);
    }
    
    ImGui::ColorEdit3("Hover color", cfgNodeHoverColor);
    ImGui::InputFloat("Hover radius", cfgNodeHoverRadius);
    ImGui::InputFloat("Hover thickness", cfgNodeHoverThickness);
    ImGui::InputInt("Hover max cursor dist.", cfgNodeHoverMaxCursorDist);
    
    ImGui::ColorEdit3("Selected node color", cfgSelectedNodeColor);
    ImGui::InputFloat("Selected node size", cfgSelectedNodeSize);
}

// #endregion

// #region NODE INTERACTION:

int mouseHoveredNodeID = -1;
array<bool> nodeSelectedStates = array<bool>(); // 1:1 with nodes, resized on `SE_TRUCK_ENTER` event

void prepareNodeSelectionForNewActor(BeamClass@ actor)
{
    // should be invoked for each SE_TRUCK_ENTER event, but also in `main()`, in case this script starts when already in vehicle!
    // -------------------
    
    nodeSelectedStates.resize(0); // purge the selection array
    mouseHoveredNodeID = -1;
    
    if (@actor != null)
    {
        
        nodeSelectedStates.resize(actor.getNodeCount()); // scale the selection array, with all nodes deselected (false)
    }
}

void drawNodeSelectionUI()
{
    int totalSelected = 0;
    string strSelected = "";
    string strJoiner = "";
    for (uint i=0; i < nodeSelectedStates.length(); i++)
    {
        if (nodeSelectedStates[i])
        {
            strSelected += (strJoiner + i);
            strJoiner = ",";
            totalSelected++;
        }
    }
    ImGui::TextDisabled("Selected nodes (total: " + totalSelected + ")");
    ImGui::Text(strSelected);
    if (ImGui::Button("Select all"))
    {
        for (uint i=0; i < nodeSelectedStates.length(); i++)
        {
            nodeSelectedStates[i] = true;
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("UnSelect all"))
    {
        for (uint i=0; i < nodeSelectedStates.length(); i++)
        {
            nodeSelectedStates[i] = false;
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Flip selection"))
    {
        for (uint i=0; i < nodeSelectedStates.length(); i++)
        {
            nodeSelectedStates[i] = !nodeSelectedStates[i];
        }
    }    
}

void drawNodeHighlights(BeamClass@ actor)
{
    
    int mouseClosestNodeID = -1;
    
    int mouseClosestNodeDist = 999999;
    
    ImDrawList@ drawlist = getDummyFullscreenWindow("nodeHighlights");
    for (int i = 0; i < actor.getNodeCount(); i++)
    {
        vector3 posWorld = actor.getNodePosition(i);
        color col = cfgNodeColor;
        bool wheelNode = false;
    if (actor.isNodeWheelRim(i)) {col=cfgRimNodeColor; wheelNode=true;}
    if (actor.isNodeWheelTire(i)) {col=cfgTireNodeColor; wheelNode=true;}
        if (!wheelNode || cfgShowWheelNodes)
        {
            vector2 pos = vector2(0,0);
            bool inFrontOfCamera = game.getScreenPosFromWorldPos(posWorld, /*out:*/ pos);
            if (inFrontOfCamera)
            {
                // Draw the node
                if (nodeSelectedStates[i])
                {
                    // DOC: void AddRectFilled(const vector2&in p_min, const vector2&in p_max, const color&in col, float rounding = 0.0f, int rounding_corners = 15)
                    vector2 halfSize(cfgSelectedNodeSize/2.f, cfgSelectedNodeSize/2.f);
                    drawlist.AddRectFilled(pos-halfSize, pos+halfSize, cfgSelectedNodeColor);
                }
                else
                {            
                    //DOC: void ImDrawList::AddCircleFilled(const ImVec2& center, float radius, ImU32 col, int num_segments)
                    drawlist.AddCircleFilled(pos, cfgNodeRadius, col, cfgNodeNumSegments);
                }
                
                // Accumulate mouse hover info
                int nodeDist = getMouseShortestDistance(game.getMouseScreenPosition(), pos);
                if (nodeDist < mouseClosestNodeDist)
                {
                    mouseClosestNodeID = i;
                    mouseClosestNodeDist = nodeDist;
                }
                
                // Draw mouse hover marker
                if (i == mouseHoveredNodeID)
                {
                    // DOC: void AddCircle(const vector2&in center, float radius, const color&in col, int num_segments = 12, float thickness = 1.f)
                    drawlist.AddCircle(pos, cfgNodeHoverRadius, cfgNodeHoverColor, cfgNodeNumSegments, cfgNodeHoverThickness);
                }
            }
        }
    }
    
    // Finalize hover detection
    ImGui::Text("<DBG drawNodeHighlights> Mouse closest node: " + mouseClosestNodeID + " (dist:" + mouseClosestNodeDist + ")");
    if (mouseClosestNodeDist <= cfgNodeHoverMaxCursorDist)
    {
        mouseHoveredNodeID = mouseClosestNodeID;
    }
    else
    {
        mouseHoveredNodeID = -1;
    }
}

// #endregion

// #region HELPERS

// shamelessly copypasted from 'road_editor.as', line 787
ImDrawList@ getDummyFullscreenWindow(const string&in name)
{
    // Dummy fullscreen window to draw to
    int window_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar| ImGuiWindowFlags_NoInputs 
    | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoBringToFrontOnFocus;
    ImGui::SetNextWindowPos(vector2(0,0));
    ImGui::SetNextWindowSize(game.getDisplaySize());
    ImGui::PushStyleColor(/*ImGuiCol_WindowBg*/2, color(0.f,0.f,0.f,0.f)); // Fully transparent background!
    ImGui::Begin(name, /*open:*/true, window_flags);
    ImDrawList@ drawlist = ImGui::GetWindowDrawList();
    ImGui::End();
    ImGui::PopStyleColor(1); // WindowBg
    
    return drawlist;    
}

// shamelessly copypasted from 'road_editor.as', line 803
int getMouseShortestDistance(vector2 mouse, vector2 target)
{
    int dx = iabs(int(mouse.x) - int(target.x));
    int dy = iabs(int(mouse.y) - int(target.y));
    return imax(dx, dy);
}

int imin(int a, int b)
{
    return (a < b) ? a : b;
}

int imax(int a, int b)
{
    return (a > b) ? a : b;
}

int iabs(int a)
{
    return (a < 0) ? -a : a;
}

// #endregion
