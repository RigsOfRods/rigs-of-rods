// TERRAIN EDITOR HITS UI
// ===================================================

// Window [X] button handler
#include "imgui_utils.as"
imgui_utils::CloseWindowPrompt closeBtnHandler;

const color TITLEART_COLOR (0.7f, 0.9f, 0.8f, 1.f);
const color ACTIVE_KEYBIND_COL(0.8f, 1.0f, 0.2f, 1.f);
CVarClass@ cvar_sim_state = console.cVarFind("sim_state");  // built in

// `frameStep()` runs every frame; `dt` is delta time in seconds.
void frameStep(float dt)
{
    
    
    ImGui::Begin("terrnEditorUI *!* ALPHA> *!*", closeBtnHandler.windowOpen, 0);
    ImGui::Dummy(vector2(400, 1));
    drawTerrnEditTitleArt(  );
    
    ImGui::Separator();
    
    drawTerrnEditHelpUI(); 
    
    ImGui::End();  //"terrnEditorUI"
    
}

// #region TerrnEditor title art
void drawTerrnEditTitleArt()
{
    string title="   T E R R N    E D I T O R   ";
    string art=" ^ ^ ^ ~ ^ ^ ^ ^ ~ ^ ^ ^  ";
    vector2 title_size = ImGui::CalcTextSize(title);
    vector2 art_size = ImGui::CalcTextSize(art);
    
    vector2 pos = ImGui::GetCursorScreenPos();
    ImGui::GetWindowDrawList().AddText(pos+vector2(0,-4), TITLEART_COLOR, art);
    ImGui::GetWindowDrawList().AddText(pos+vector2(0,0), TITLEART_COLOR, art);
    ImGui::GetWindowDrawList().AddText(pos+vector2(0,4), TITLEART_COLOR, art);
    ImGui::SetCursorPos(ImGui::GetCursorPos() + vector2(art_size.x, 0));
    pos = ImGui::GetCursorScreenPos();
    ImGui::GetWindowDrawList().AddText(pos+vector2(0,-3), TITLEART_COLOR, title);
    ImGui::SetCursorPos(ImGui::GetCursorPos() + vector2( title_size.x + 5, 0));
    pos = ImGui::GetCursorScreenPos();
    ImGui::GetWindowDrawList().AddText(pos+vector2(0,-4), TITLEART_COLOR, art);
    ImGui::GetWindowDrawList().AddText(pos+vector2(0,0), TITLEART_COLOR, art);
    ImGui::GetWindowDrawList().AddText(pos+vector2(0,4), TITLEART_COLOR, art);
    ImGui::Text("");//spacer
}
//#endregion title art

// #region TerrnEditor info
void drawTerrnEditHelpUI()
{
    if (cvar_sim_state.getInt() != 3)
    {
        ImGui::Text("Activate terrain editing mode with");
        ImGui::SameLine(); 
        ImDrawEventKeybindHighlighted(EV_COMMON_TOGGLE_TERRAIN_EDITOR);
    }
    else
    {
        ImGui::TextDisabled(" ^ GLOBAL HOTKEYS ^");
        ImGui::AlignTextToFramePadding();
        ImGui::Bullet();
        ImGui::SameLine(); ImDrawEventKeybindHighlighted(EV_COMMON_TOGGLE_TERRAIN_EDITOR);
        ImGui::SameLine(); ImGui::Text("Exit terrain editor");
    }
}
//#endregion TerrnEditor info

//#region imgui_utils < -- ImDrawEventKeybindHighlighted(inputEvents ev)
void  ImDrawEventKeybindHighlighted(inputEvents ev)
{
    bool active = inputs.getEventBoolValue(ev);
    string keybind = "" +  inputs.getEventCommandTrimmed(ev);
    if (active) 
    { 
        ImGui::PushStyleColor(ImGuiCol_Text, ACTIVE_KEYBIND_COL); 
    }
    ImGui::BeginChildFrame(ev, ImGui::CalcTextSize(keybind) + vector2(8,6));    
    ImGui::Text(keybind);
    ImGui::EndChildFrame();
    if (active) 
    {
        ImGui::PopStyleColor(); // ImGuiCol_Text  
    }  
}
//#endregion draw ev keybind highlighted

