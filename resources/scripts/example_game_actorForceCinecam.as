// EXAMPLE - how to use the new 'force cinecam' API:
//    void              setForcedCinecam(CineCameraID_t cinecam_id, BitMask_t flags);
//    void              clearForcedCinecam();
//    bool              getForcedCinecam(CineCameraID_t& cinecam_id, BitMask_t& flags);
//    int               getNumCinecams();
// ===================================================

// #region Camera controls
int inputFlags = 0;
void drawForceCinecamPanel()
{
    BeamClass@ actor = game.getCurrentTruck();
    if (@actor == null)
    {
        ImGui::Text("You are on foot. Enter a vehicle first.");
        ImGui::TextDisabled("It's technically possible to control actor at any time,\nbut this script only shows it where you can see it");
        
        return; // nothing more to do.
        
    }
    
    ImGui::Text("You are driving '"+actor.getTruckName()+"' ("+actor.getNumCinecams()+" cinecams)");
    int forcedCamId = -1;
    int forcedCamFlags = 0;
    ImGui::Separator();
    if (actor.getForcedCinecam(forcedCamId, forcedCamFlags))
    {
        ImGui::Text("Forced cinecam: "+forcedCamId + " (flags: " + forcedCamFlags + ")");
        ImGui::Text("Camera hotkeys are blocked now.");
        if (ImGui::SmallButton("Reset"))
        {
            actor.clearForcedCinecam();
        }
    }
    else
    {
        ImGui::Text("No cinecam forced at the moment");
        
        ImGui::InputInt("Flags (reserved for future use)", inputFlags);
        for (int i = 0; i < actor.getNumCinecams(); i++)
        {
            if (    ImGui::Button("Force cinecam #"+i)    )
            {
                actor.setForcedCinecam(i, inputFlags);
            }
        }
    }
    
}

// #endregion

//#region Game integration

// Window [X] button handler
#include "imgui_utils.as"
imgui_utils::CloseWindowPrompt closeBtnHandler;

void main()
{
    // Uncomment to close window without asking.
    //closeBtnHandler.cfgCloseImmediatelly = true;
}

// `frameStep()` runs every frame; `dt` is delta time in seconds.
void frameStep(float dt)
{
    // Begin drawing window
    if (ImGui::Begin("Demo of 'forced cinecam' feature", closeBtnHandler.windowOpen, 0))
    {
        // Draw the "Terminate this script?" prompt on the top (if not disabled by config).
        closeBtnHandler.draw();
        
        drawForceCinecamPanel();
        
        // End drawing window
        ImGui::End();
    }
}

// #endregion

