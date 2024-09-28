/// \title OGRE example - character posing (skeletal animations)
/// \brief See https://github.com/RigsOfRods/rigs-of-rods/pull/3030#issue-1639679673

// Window [X] button handler
#include "imgui_utils.as"
imgui_utils::CloseWindowPrompt closeBtnHandler;

PoseDemo poseDemo;

void main()
{
    game.log("OGRE example - character posing (skeletal animations)");
}

void frameStep(float dt)
{
    ImGui::Begin("OGRE example - character posing (skeletal animations)", closeBtnHandler.windowOpen, ImGuiWindowFlags_AlwaysAutoResize);
    closeBtnHandler.draw();
    poseDemo.draw();
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