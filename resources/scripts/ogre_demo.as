void main()
{
    game.log("EXPERIMENTAL! This script tests the new direct bindings of OGRE rendering framework");
}

void frameStep(float dt)
{
    // Begin drawing window
    ImGui::Begin("OGRE inspector", /*open:*/true, ImGuiWindowFlags_AlwaysAutoResize);
    ImGui::TextDisabled("See official OGRE API reference");
    ImGui::TextDisabled("https://ogrecave.github.io/ogre/api/latest/");
    ImGui::Separator();
    
    Ogre::Root@ root = Ogre::Root::getSingleton();
    if (@root != null)
    {
        drawTreeNodeOgreRoot(root);
    }
    else
    {
        ImGui::TextDisabled("Cannot retrieve OGRE `Root` object");
    }
    
    // End window
    ImGui::End();    
}

void drawTreeNodeOgreRoot(Ogre::Root@ root)
{
    if (ImGui::TreeNode("Ogre::Root"))
    {
        // Scenemanagers
        Ogre::SceneManagerInstanceDict@ sceneManagers = root.getSceneManagers();
        ImGui::TextDisabled("Ogre::SceneManager [" + sceneManagers.getSize() + "]");
        array<string> sceneMgrNames = sceneManagers.getKeys();
        for (uint i = 0; i < sceneManagers.getSize(); i++)
        {
            drawTreeNodeOgreSceneManager(sceneManagers[sceneMgrNames[i]]);
        }        
        
        ImGui::TreePop();
    }
}

void drawTreeNodeOgreSceneManager(Ogre::SceneManager@ sceneMgr)
{
    if (ImGui::TreeNode(sceneMgr.getName()))
    {        
        ImGui::TreePop();
    }
}
