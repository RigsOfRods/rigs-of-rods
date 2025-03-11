// TEST of new `game.getMousePointedMovableObjects()`
// ===================================================

// Window [X] button handler
#include "imgui_utils.as"
imgui_utils::CloseWindowPrompt closeBtnHandler;

// total time in seconds
float tt = 0.f;

void main()
{
    // Uncomment to close window without asking.
    //closeBtnHandler.cfgCloseImmediatelly = true;
}

// `frameStep()` runs every frame; `dt` is delta time in seconds.
void frameStep(float dt)
{
    // Begin drawing window
    if (ImGui::Begin("Mouse pointed object info", closeBtnHandler.windowOpen, 0))
    {
        // Draw the "Terminate this script?" prompt on the top (if not disabled by config).
        closeBtnHandler.draw();
        
        array<Ogre::MovableObject@>@ movables = game.getMousePointedMovableObjects();
        if (@movables == null)
        {
            ImGui::Text("<no results>");
        }
        else
        {
            ImGui::Text("N U M   R E S U L T S   :   " + movables.length());
            ImGui::Separator();
            for (uint i=0; i<movables.length(); i++)
            {
                if (i > 0)
                {
                    ImGui::Separator();
                }
                Ogre::MovableObject@ movable = movables[i];
                drawMovableObjectInfo(movable);
                
            }
        }
        
        
        // End drawing window
        ImGui::End();
    }
}

void drawMovableObjectInfo(Ogre::MovableObject@ movable)
{
    ImGui::TextDisabled('Movable:' + movable.__getUniqueName());
    Ogre::SceneNode@ snode = movable.getParentSceneNode();
    ImGui::TextDisabled('SceneNode: ' + snode.__getUniqueName());
    
    bool visible = movable.isVisible();
    ImGui::Text("Visible: "+ visible);
    
    bool castShadows = movable.getCastShadows();
    ImGui::Text("Cast shadows: "+ castShadows);
}

