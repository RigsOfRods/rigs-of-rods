// EXAMPLE SCRIPT - how to display hyperlink.
// ===================================================

#include "imgui_utils.as"  // ImHyperlink()

// Window [X] button handler
imgui_utils::CloseWindowPrompt closeBtnHandler;

void frameStep(float dt)
{
    if (ImGui::Begin("Example", closeBtnHandler.windowOpen, 0))
    {
        closeBtnHandler.draw();
        imgui_utils::ImHyperlink("http://developer.rigsofrods.org", "Dev portal");
        ImGui::End();
    }
    
}
