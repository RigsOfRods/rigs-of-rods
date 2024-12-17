// NEW API `game.getEditorObjects()` test
// ===================================================

// Window [X] button handler
#include "imgui_utils.as"
imgui_utils::CloseWindowPrompt closeBtnHandler;

void frameStep(float dt)
{
    // Begin drawing window
    if (ImGui::Begin("Terrain Editor Objects", closeBtnHandler.windowOpen, 0))
    {
        ImGui::TextDisabled("Enumerates all terrain objects\n using new API `game.getEditorObjects()`");
        // Draw the "Terminate this script?" prompt on the top (if not disabled by config).
        closeBtnHandler.draw();
        
        // Direct access
        ImGui::Text("Object count: " + game.getEditorObjects().length());
        ImGui::Separator();
        
        // Local variable access
        array<TerrainEditorObjectClassPtr@>@ ediObjects = game.getEditorObjects();
        for (uint i = 0; i < ediObjects.length(); i++)
        {
            TerrainEditorObjectClass@ curObj = ediObjects[i];
            string label = "["+i+"/"+ediObjects.length()+"] " + curObj.getName(); 
            
            if (curObj.getInstanceName() != "") { 
                label += " ~ instance name: '"+curObj.getInstanceName()+"'"; 
            }
            if (curObj.getType() != "") { 
                label+= " (type: " + curObj.getType() + ")";
            }
            ImGui::TreeNode(label);
        }
        
        // End drawing window
        ImGui::End();
    }
}

