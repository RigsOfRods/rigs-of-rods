// How to use OGRE lights query
// ===================================================

// Window [X] button handler
#include "imgui_utils.as"
imgui_utils::CloseWindowPrompt closeBtnHandler;

string formatLightType(Ogre::LightTypes t)
{
    switch (t)
    {
        case Ogre::LT_POINT: return "LT_POINT";
        case Ogre::LT_DIRECTIONAL: return "LT_DIRECTIONAL";
        case Ogre::LT_SPOTLIGHT: return "LT_SPOTLIGHT";
        default: ;
    }
    return "";
}

void drawLightsExamplePanel()
{
    // Get the scene manager
    TerrainClass @terrain = game.getTerrain();
    Ogre::SceneManager@ smgr = game.getSceneManager();       
    if (@terrain == null || @smgr == null)
    {
        ImGui::TextDisabled("no terrain loaded!");
        return;
    }
    
    // Retrieve all lights in the scene
    array<Ogre::MovableObject@>@ movableObjects = smgr.__getMovableObjectsByType("Light");
    
    ImGui::TextDisabled("Found " + movableObjects.length() + " lights in the scene");
    
    // Iterate through all lights
    for (uint i = 0; i < movableObjects.length(); i++)
    {
        // Cast MovableObject to Light
        Ogre::Light@ light = cast<Ogre::Light>(movableObjects[i]);
        
        if (light !is null)
        {
            // Log light information
            ImGui::Text("Light #" + i + ": " + light.__getUniqueName());
            ImGui::BulletText("  Type: " + formatLightType(light.getType()) + " (" + light.getType() + ")");
            ImGui::BulletText("  Position: " + light.getPosition().x + ", " +       light.getPosition().y + ", " + light.getPosition().z);
            ImGui::BulletText("  Diffuse Color: R=" + light.getDiffuseColour().r +  " G=" + light.getDiffuseColour().g +  " B=" + light.getDiffuseColour().b);
        }
    }
    
}



// `frameStep()` runs every frame; `dt` is delta time in seconds.
void frameStep(float dt)
{
    // Begin drawing window
    if (ImGui::Begin("Lights query example", closeBtnHandler.windowOpen, 0))
    {
        // Draw the "Terminate this script?" prompt on the top (if not disabled by config).
        closeBtnHandler.draw();
        
        // =====example====
        drawLightsExamplePanel();
        // =====end example====
        
        // End drawing window
        ImGui::End();
    }
}
