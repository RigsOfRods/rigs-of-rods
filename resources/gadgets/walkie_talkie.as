// \title Walkie Talkie
// \brief Shows which actor is remotely receiving commands
// ===================================================

//#region WalkieTalkie UI
Ogre::TexturePtr gWalkieTalkieIcon; // loaded in main()
void drawWalkieTalkiePanel()
{
    if (!gWalkieTalkieIcon.isNull())
    {
        ImGui::Image(gWalkieTalkieIcon, vector2(50,80));
    }
    ImGui::SameLine();
    // Force the text to the right side of the image
    vector2 floatingCursor = ImGui::GetCursorPos();
    ImGui::TextDisabled("Actor remotely receiving commands:");
    BeamClass@ truck = game.getTruckRemotelyReceivingCommands();
    ImGui::SetCursorPos(floatingCursor + vector2(0, ImGui::GetTextLineHeightWithSpacing()));
    if (@truck == null)
    {
        ImGui::TextDisabled("Currently none.\nWalk or drive closer to another.");
    }
    else
    {
        ImGui::Text(truck.getTruckName());
    }
}

//#endregion

//#region Game interface

// Window [X] button handler
#include "imgui_utils.as"
imgui_utils::CloseWindowPrompt closeBtnHandler;

void main()
{
    gWalkieTalkieIcon = Ogre::TextureManager::getSingleton().load("walkie_talkie.png", "IconsRG");
}

// `frameStep()` runs every frame; `dt` is delta time in seconds.
void frameStep(float dt)
{
    // Begin drawing window
    int flags = ImGuiWindowFlags_AlwaysAutoResize;
    if (ImGui::Begin("Walkie Talkie", closeBtnHandler.windowOpen, flags))
    {
        // Draw the "Terminate this script?" prompt on the top (if not disabled by config).
        closeBtnHandler.draw();
        
        drawWalkieTalkiePanel();
        
        // End drawing window
        ImGui::End();
    }
}

//#endregion
