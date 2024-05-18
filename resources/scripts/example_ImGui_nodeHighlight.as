// NODE HIGHLIGHT DEMO
// ===================================================

//#region GAME CALLBACKS:

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
        //DOC: bool ImGui::SliderFloat(const char* label, float* v, float v_min, float v_max, const char* format, float power)
        ImGui::SliderFloat("Node radius", cfgNodeRadius, 0.5f, 10.f);
        ImGui::SliderInt("Node num segments", cfgNodeNumSegments, 3, 20);
        //DOC: bool ColorEdit3(const string&in, color&inout)
        ImGui::ColorEdit3("Node color", cfgNodeColor);
        ImGui::Checkbox("Show wheel nodes", cfgShowWheelNodes);
        if (cfgShowWheelNodes)
        {
            ImGui::ColorEdit3("Rim node color", cfgRimNodeColor);
            ImGui::ColorEdit3("Tire node color", cfgTireNodeColor);
        }
        drawNodeHighlights(actor);
    }
}

//#endregion

color cfgNodeColor = color(0.9, 0.8, 0.5, 1);
color cfgRimNodeColor = color(0.4, 0.8, 0.5, 1);
color cfgTireNodeColor = color(0.5, 0.8, 0.8, 1);
float cfgNodeRadius = 3.f;
int cfgNodeNumSegments = 5;
bool cfgShowWheelNodes = true;

// #region CUSTOM FUNCTIONS:

void drawNodeHighlights(BeamClass@ actor)
{
    
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
                //DOC: void ImDrawList::AddCircleFilled(const ImVec2& center, float radius, ImU32 col, int num_segments)
                drawlist.AddCircleFilled(vector2(pos.x, pos.y), cfgNodeRadius, col, cfgNodeNumSegments);
            }
        }
    }
}

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

// #endregion
