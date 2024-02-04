void main()
{
    game.log("CAUTION: Proof of concept, very experimental!");
    game.log("This script tests the new direct bindings of OGRE rendering framework");
}

OgreInspector inspector;

void frameStep(float dt)
{
    // Begin drawing window
    ImGui::Begin("OGRE demo script", /*open:*/true, ImGuiWindowFlags_AlwaysAutoResize);

    inspector.draw();
    
    // End window
    ImGui::End();    
}

class OgreInspector
{
    void draw()
    {
        ImGui::TextDisabled("Scene inspector");
        ImGui::SameLine();
        ImGui::TextDisabled("(?)");
        if (ImGui::IsItemHovered())
        {
            ImGui::BeginTooltip();
            this.drawInspectorTooltipBody();
            ImGui::EndTooltip();
        }
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
    
    void drawInspectorTooltipBody()
    {
        ImGui::Text("Traverses the OGRE scene hierarchy, as stored in memory");
        ImGui::Text("Note that all element names are just simple strings,");
        ImGui::Text("even if they have '/' in them, like 'Ogre/SceneRoot'.");
        ImGui::Separator();
        ImGui::TextDisabled("See official OGRE API reference");
        ImGui::TextDisabled("https://ogrecave.github.io/ogre/api/latest/"); 
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
        
            ImGui::TreePop();
        }
    }

    void drawTreeNodeOgreSceneNodeRecursive(const Ogre::SceneNode@ snode)
    {
        // When drawing first time, fold nodes with many children (root node can have hundreds...)
        const Ogre::ChildNodeArray@ children = snode.getChildren();
        ImGui::SetNextItemOpen(children.length() < 10, ImGuiCond_Once);
        
        // The `__getUniqueId()` is a Rigs of Rods extension (that's why double leading underscores), 
        // because names are optional and usually not set, and imgui tree nodes require unique IDs.
        if (ImGui::TreeNode(snode.__getUniqueId()))
        {
            // Tree node open, draw children recursively
            ImGui::TextDisabled("Ogre::Node ["+children.length()+"]");
            for (uint i = 0; i < children.length(); i++)
            {
                const Ogre::SceneNode@ child = cast<Ogre::SceneNode>(children[i]);
                if (@child != null)
                {
                    drawTreeNodeOgreSceneNodeRecursive(child);
                }
            }
        
            ImGui::TreePop();
        }
        else
        {
            // Tree node closed, draw child count
            ImGui::SameLine();
            ImGui::Text("("+children.length()+" children)");
        }
    }
}


