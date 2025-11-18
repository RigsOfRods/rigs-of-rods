// Window [X] button handler
#include "imgui_utils.as"
imgui_utils::CloseWindowPrompt closeBtnHandler;

void main()
{
    game.log("EXAMPLE OGRE INSPECTOR - shows how you can traverse scene and get info about displayed objects");
}

OgreInspector inspector;

void frameStep(float dt)
{
    ImGui::Begin("OGRE scene inspector example", closeBtnHandler.windowOpen, ImGuiWindowFlags_AlwaysAutoResize);
    closeBtnHandler.draw();
    inspector.draw();
    ImGui::End();    
}

class OgreInspector
{
    void draw()
    {
        ImGui::Text("Traverses the OGRE scene hierarchy, as stored in memory.");
        ImGui::Text("Note that all element names are just simple strings, even if they contain '/'.");
        ImGui::Text("For details see OGRE docs:");
        ImGui::SameLine();
        imgui_utils::ImHyperlink("https://ogrecave.github.io/ogre/api/latest/"); 
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
            // Scene nodes
            Ogre::SceneNode@ rootNode = sceneMgr.getRootSceneNode();
            if (@rootNode != null)
            {
                ImGui::TextDisabled("Ogre::SceneNode [1]");
                this.drawTreeNodeOgreSceneNodeRecursive(rootNode);
            }
            else
            {
                ImGui::TextDisabled("Ogre::SceneNode [0]");
            }
            
            // ambient light
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
                        this.drawTreeNodeOgreAnimationState(stateDict[stateNames[i]]);
                    }
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
}


