// TEST SCRIPT for Ogre Overlays
// builtin element types are listed in RoR.log on startup:
// *  Panel registered.
// *  BorderPanel registered.
// *  TextArea registered.
// ===================================================
//       #### WARNING - DANGEROUS API ####
//   OGRE objects don't use reference counting 
//   - do not keep pointers longer than absolutely necessary!
//   Prefer always looking up the resource from OGRE - slower but safer.
// ===================================================

// Window [X] button handler
#include "imgui_utils.as"
imgui_utils::CloseWindowPrompt closeBtnHandler;

void frameStep(float dt)
{
    if (ImGui::Begin("OGRE overlays", closeBtnHandler.windowOpen, 0))
    {
        closeBtnHandler.draw();
        drawOverlayList();
        ImGui::End();
    }
}

bool drawOverlayTreeNode( Ogre::Overlay@ ov)
{
    ImGui::PushID(ov.getName()+"-treenode");
    bool open = ImGui::TreeNode(ov.getName());
    
    ImGui::SameLine();
    if (!ov.isVisible())
    {
        if (ImGui::SmallButton("Show"))
        {
            ov.show();
        }
    }
    else
    {
        if (ImGui::SmallButton("Hide"))
        {
            ov.hide();
        }
    }
    
    ImGui::PopID();
    return open;
    
}


void drawOverlayElementDetails(Ogre::OverlayElement@ el)
{
    ImGui::PushStyleColor(ImGuiCol_Text, color(0.8, 0.8, 0.8, 1.0));
    ImGui::Bullet(); ImGui::Text("Left: " + el.getLeft()); 
    ImGui::Bullet(); ImGui::Text("Top: " + el.getTop());
    ImGui::Bullet(); ImGui::Text("Width:" + el.getWidth()); 
    ImGui::Bullet(); ImGui::Text("Height: " + el.getHeight());
    ImGui::PopStyleColor(); // Text
}

void drawOverlayElementControls(Ogre::OverlayElement@ el)
{
    ImGui::PushID(el.getName() + "-controls");
    ImGui::PushStyleColor(ImGuiCol_Text, color(0.8, 0.8, 0.8, 1.0));
float left=el.getLeft(); if (ImGui::InputFloat("Left", left)) { el.setLeft(left); }
float top=el.getTop(); if (ImGui::InputFloat("Top", top)) { el.setTop(top); }
float width=el.getWidth(); if (ImGui::InputFloat("Width", width) ) { el.setWidth(width); }
float height=el.getHeight(); if (ImGui::InputFloat("Height", height)) { el.setHeight(height); }
    ImGui::PushID("-metric");
    ImGui::TextDisabled("MetricsMode:");
if (ImGui::RadioButton("Pixels", el.getMetricsMode() == Ogre::GMM_PIXELS)) { el.setMetricsMode(Ogre::GMM_PIXELS); }
if (ImGui::RadioButton("Relative", el.getMetricsMode() == Ogre::GMM_RELATIVE)) { el.setMetricsMode(Ogre::GMM_RELATIVE); }
if (ImGui::RadioButton("Relative (aspect adjusted)", el.getMetricsMode() == Ogre::GMM_RELATIVE_ASPECT_ADJUSTED)) { el.setMetricsMode(Ogre::GMM_RELATIVE_ASPECT_ADJUSTED); }
    ImGui::PopID();
    ImGui::PopID();    
    ImGui::PopStyleColor(); // Text
}

void drawOverlayElementList( Ogre::Overlay@ ov)
{
    ImGui::PushID(ov.getName()+"-elements");
    ImGui::PushStyleColor(ImGuiCol_Text, color(0.9, 0.8, 0.6, 1.0));
    
    array<Ogre::OverlayElement@>@ elems = ov.get2DElements();
    for (uint i=0; i < elems.length(); i++)
    {
        if (drawOverlayElementTreeNode(elems[i]))
        {
            drawOverlayElementControls(elems[i]);
            ImGui::TreePop();
        }
    }
    
    ImGui::PopStyleColor(); // Text
    ImGui::PopID();
}

bool drawOverlayElementTreeNode(Ogre::OverlayElement@ el)
{
    ImGui::PushID(el.getName()+"-treenode");
    bool open = ImGui::TreeNode(el.getName()); 
    ImGui::SameLine();
    ImGui::TextDisabled("(Material: "+el.getMaterialName()+")");
    ImGui::SameLine();
    if (!el.isVisible())
    {
        if (ImGui::SmallButton("Show"))
        {
            el.show();
        }
    }
    else
    {
        if (ImGui::SmallButton("Hide"))
        {
            el.hide();
        }
    }
    ImGui::PopID();
    return open;
}

void drawOverlayList()
{
    array<Ogre::Overlay@>@ overlays = Ogre::OverlayManager::getSingleton().getOverlays();
    ImGui::TextDisabled("There are " + overlays.length() + " overlays:");
    for (uint i=0; i<overlays.length(); i++)
    {
        if (drawOverlayTreeNode(overlays[i]))
        {
            drawOverlayElementList(overlays[i]);
            ImGui::TreePop();
        }  
    }
}

