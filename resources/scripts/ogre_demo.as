void main()
{
    game.log("CAUTION: Proof of concept, very experimental!");
    game.log("This script tests the new direct bindings of OGRE rendering framework");
}

OgreInspector inspector;
PoseDemo poseDemo;

void frameStep(float dt)
{
    // Begin drawing window
    ImGui::Begin("OGRE demo script", /*open:*/true, ImGuiWindowFlags_AlwaysAutoResize);
    
    if (ImGui::CollapsingHeader("Inspector"))
    {
        inspector.draw();
    }

    if (ImGui::CollapsingHeader("Pose demo"))
    {
        poseDemo.draw();
    }
    
    // End window
    ImGui::End();    
}

class PoseDemo
{
    Ogre::Entity@ ent;
    Ogre::SceneNode@ snode;
    string error;
    Ogre::Entity@ stand_ent;
    Ogre::SceneNode@ stand_snode;    
    
    void draw()
    {
        this.drawSetupButton();
        
        if (@this.ent != null && @this.ent.getAllAnimationStates() != null)
        {
            this.drawAnimationControls(this.ent.getAllAnimationStates());
        }
    }
    
    void drawAnimationControls(Ogre::AnimationStateSet@ stateSet)
    {
        Ogre::AnimationStateDict@ stateDict = stateSet.getAnimationStates(); 
        array<string> stateNames = stateDict.getKeys();
        for (uint i = 0; i < stateDict.getSize(); i++)
        {
            Ogre::AnimationState@ anim = stateDict[stateNames[i]];
            ImGui::BulletText('"' + anim.getAnimationName() + '"');
            
            ImGui::PushID(anim.getAnimationName());
            
            ImGui::Dummy(vector2(25.f, 0.f)); // indent
            ImGui::SameLine();
            bool enabled = anim.getEnabled();
            if (ImGui::Checkbox("Enabled", enabled))
            {
                anim.setEnabled(enabled);
            }
            
            ImGui::SameLine();
            float weight = anim.getWeight();
            ImGui::SetNextItemWidth(75.f);
            if (ImGui::SliderFloat("BlendWeight", weight, 0.f, 1.f))
            {
                anim.setWeight(weight);
            }
 
            ImGui::Dummy(vector2(25.f, 0.f)); // Indent
            ImGui::SameLine();
            float timepos = anim.getTimePosition();
            if (ImGui::SliderFloat("TimePos", timepos, 0.f, anim.getLength()))
            {
                anim.setTimePosition(timepos);
            }
                
            ImGui::PopID(); // anim.getAnimationName()
        }
    }
    
    void drawSetupButton()
    {
        if (this.error != "")
        {
            ImGui::Text("Error: " + this.error);
        }
        else if (@this.ent == null)
        {
            if (ImGui::Button("Show example animated entity"))
            {
                // Load the rorbot
                @this.ent = game.getSceneManager().createEntity("PoseDemoCharacter", "character.mesh" );
                if (@this.ent == null)
                {
                    error = "Could not load 'character.mesh'";
                    return;
                }

                // Put to scene
                @this.snode = game.getSceneManager().getRootSceneNode().createChildSceneNode("PoseDemoNode");
                this.snode.attachObject(this.ent);
                //at the same place as the player character, slightly raised to the air
                this.snode.setPosition(game.getPersonPosition() + vector3(0.f, 0.3f, 0.f));
                // rotated 180 degrees
                this.snode.yaw((game.getPersonRotation() * -1.f) + degree(180));
                // 'character.mesh' has wrong scale, the game scales it down to <0.02, 0.02, 0.02>, we go even a bit smaller.
                this.snode.setScale(vector3(0.014, 0.014, 0.014));
                // dress up directly with the shared material, we won't be changing shirt color
                this.ent.setMaterialName("tracks/character");
                
                // the stand
                @this.stand_ent = game.getSceneManager().createEntity("PoseDemoStand", "trucktrigger.mesh" );
                @this.stand_snode = game.getSceneManager().getRootSceneNode().createChildSceneNode("PoseDemoStandNode");
                this.stand_snode.setPosition(game.getPersonPosition());
                this.stand_snode.setDirection(vector3(0.f, -1.f, 0.f)); // Place on side
                this.stand_snode.setScale(vector3(1.f, 1.f, 3.f)); // triple height
                this.stand_snode.attachObject(this.stand_ent);
            }
        }
        else
        {
            if (ImGui::Button("Remove example animated entity"))
            {
                this.ent.detachFromParent();
                game.getSceneManager().destroyEntity(this.ent);
                @this.ent = null;
                
                game.getSceneManager().destroySceneNode(this.snode);
                @this.snode = null;
                
                // the stand
                game.getSceneManager().destroyEntity(this.stand_ent);
                @this.stand_ent = null;
                
                game.getSceneManager().destroySceneNode(this.stand_snode);
                @this.stand_snode = null;                
            }
        }
    }
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

    void drawTreeNodeOgreSceneNodeRecursive(Ogre::SceneNode@ snode)
    {
        // Start with all nodes folded (root node can have hundreds...)
        ImGui::SetNextItemOpen(false, ImGuiCond_Once);
        
        Ogre::ChildNodeArray@ children = snode.getChildren();
        
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
            Ogre::MovableObjectArray@ movables = snode.getAttachedObjects();
            ImGui::TextDisabled("Ogre::MovableObject [" + movables.length() + "]");
            for (uint i = 0; i < movables.length(); i++)
            {
                drawTreeNodeOgreMovableObject(movables[i]);
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


