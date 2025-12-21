// Window [X] button handler
#include "imgui_utils.as"
imgui_utils::CloseWindowPrompt closeBtnHandler;

color COLOR_RED(1.f,.3f,.3f,1);



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


void main()
{
    game.log("OGRE INSPECTOR - shows how you can traverse scene and get info about displayed objects");
}

void frameStep(float dt)
{
    ImGui::Begin("OGRE inspector", closeBtnHandler.windowOpen, ImGuiWindowFlags_AlwaysAutoResize);
    closeBtnHandler.draw();
    drawHelpBanner();
    drawSceneHierarchy();
    ImGui::End();    
}

void drawHelpBanner()
{
    ImGui::Text("Traverses the OGRE scene hierarchy, as stored in memory.");
    ImGui::Text("Note that all element names are just simple strings, even if they contain '/'.");
    ImGui::Text("For details see scripting docs:");
    ImGui::SameLine();
    imgui_utils::ImHyperlink("https://developer.rigsofrods.org/d8/d55/namespace_angel_ogre.html"); 
    ImGui::Separator();
}

void drawSceneHierarchy()
{
    Ogre::Root@ root = Ogre::Root::getSingleton();
    if (@root != null)
    {
        drawTreeNodeOgreRoot(root);
    }
    else
    {
        ImGui::TextColored(COLOR_RED, "Error! Cannot retrieve OGRE `Root` object");
    }
}



void drawTreeNodeOgreRoot(Ogre::Root@ root)
{
    if (ImGui::TreeNode("(The OGRE Root)"))
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
    
    if (ImGui::TreeNode('"'+sceneMgr.getName()+'"'))
    {
        // Managed objects: Scene nodes
        Ogre::SceneNode@ rootNode = sceneMgr.getRootSceneNode();
        if (@rootNode != null)
        {
            ImGui::TextDisabled("Ogre::SceneNode [1]");
            drawTreeNodeOgreSceneNodeRecursive(rootNode);
        }
        else
        {
            ImGui::TextDisabled("Ogre::SceneNode [0]");
        }
        
        // Managed objects: Movables of type "Light"
        // NOTE: __getMovableObjectsByType() is a Rigs of Rods extensions, that's why it starts with double underscore.
        array<Ogre::MovableObject@>@ movableObjects = sceneMgr.__getMovableObjectsByType("Light");   
        ImGui::TextDisabled("Ogre::Light [" + movableObjects.length() + "]");
        for (uint i = 0; i < movableObjects.length(); i++)
        {
            drawTreeNodeOgreMovableObject(movableObjects[i]); 
            
        }        
        
        // Attribute: ambient light
        color ambientColor = sceneMgr.getAmbientLight();
        if (ImGui::ColorEdit3("Ambient Light", ambientColor))
        {
            sceneMgr.setAmbientLight(ambientColor);
        }     
        ImGui::TreePop();
    }
}

void drawTreeNodeOgreSceneNodeRecursive(Ogre::SceneNode@ snode)
{
    // Start with all nodes folded (root node can have hundreds...)
    ImGui::SetNextItemOpen(false, ImGuiCond_Once);
    
    Ogre::ChildNodeArray@ children = snode.getChildren();
    Ogre::MovableObjectArray@ movables = snode.getAttachedObjects();
    
    // The `__getUniqueName()` is a Rigs of Rods extension (that's why double leading underscores), 
    // because names are optional and usually not set, and imgui tree nodes require unique IDs.
    if (ImGui::TreeNode(snode.__getUniqueName()))
    {
        // Tree node open, draw children recursively
        ImGui::TextDisabled("Ogre::Node ["+children.length()+"]");
        for (uint i = 0; i < children.length(); i++)
        {
            Ogre::SceneNode@ child = cast<Ogre::SceneNode>(children[i]);
            if (@child != null)
            {
                drawTreeNodeOgreSceneNodeRecursive(child);
            }
        }
        
        // Draw attached movable objects
        ImGui::TextDisabled("Ogre::MovableObject [" + movables.length() + "]");
        for (uint i = 0; i < movables.length(); i++)
        {
            drawTreeNodeOgreMovableObject(movables[i]);
        }
        
        ImGui::TreePop();
    }
    else
    {
        ImGui::PushID(snode.__getUniqueName());
        
        // Tree node closed, draw info (context-sensitive)
        ImGui::SameLine();
        if (children.length() == 0 && movables.length() == 1)
        {
            //the most usual case - a node with an entity. display the mesh name.
            ImGui::TextDisabled("-->");
            ImGui::SameLine();
            ImGui::Text(movables[0].__getUniqueName());
            ImGui::SameLine();
            if (ImGui::SmallButton("go to")) 
            {
                game.setPersonPosition(snode.getPosition());
            }
        }
        else 
        {
            ImGui::Text("("+children.length()+" children, "+movables.length()+" movables)");
        }
        
        ImGui::PopID(); //snode.__getUniqueName()
    }
}

void drawTreeNodeOgreMovableObject(Ogre::MovableObject@ movable)
{
    if (ImGui::TreeNode(movable.__getUniqueName()))
    {
        bool visible = movable.isVisible();
        if (ImGui::Checkbox("Visible", visible))
        {
            movable.setVisible(visible);
        }
        
        bool castShadows = movable.getCastShadows();
        if (ImGui::Checkbox("Cast shadows", castShadows))
        {
            movable.setCastShadows(castShadows);
        }
        
        if (movable.getMovableType() == "Entity")
        {
            Ogre::Entity@ entity = cast<Ogre::Entity>(movable);
            Ogre::AnimationStateSet@ stateSet = entity.getAllAnimationStates();
            if (@stateSet != null)
            {
                Ogre::AnimationStateDict@ stateDict = stateSet.getAnimationStates(); 
                ImGui::TextDisabled("Ogre::AnimationState [" + stateDict.getSize() + "]");
                array<string> stateNames = stateDict.getKeys();
                for (uint i = 0; i < stateDict.getSize(); i++)
                {
                    drawTreeNodeOgreAnimationState(stateDict[stateNames[i]]);
                }
            }
        }
        else if (movable.getMovableType() == "Light")
        {
            Ogre::Light@ light = cast<Ogre::Light>(movable);
            if (light !is null)
            {
                // Log light information
                ImGui::BulletText("  Type: " + formatLightType(light.getType()) + " (" + light.getType() + ")");
                ImGui::BulletText("  Position: " + light.getPosition().x + ", " +       light.getPosition().y + ", " + light.getPosition().z);
                // ambient light
                color dColor = light.getDiffuseColour();
                if (ImGui::ColorEdit3("Diffuse Color", dColor))
                {
                    light.setDiffuseColour(dColor);
                }     
            }
            else
            {
                ImGui::TextColored(COLOR_RED, "Error! Cannot cast `MovableObject` to `Light`");
            }
        }
        
        ImGui::TreePop();
    }
}

void drawTreeNodeOgreAnimationState(Ogre::AnimationState@ anim)
{
    ImGui::BulletText('"' + anim.getAnimationName() + '"');
    ImGui::SameLine();
    ImGui::Text("(" + formatFloat(anim.getLength(), "", 0, 2) + " sec)");
    if (anim.getEnabled())
    {
        ImGui::SameLine();
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, vector2(0.f, 0.f));
        ImGui::ProgressBar(anim.getTimePosition() / anim.getLength());
        ImGui::PopStyleVar(1); // ImGuiStyleVar_FramePadding
    }
}




