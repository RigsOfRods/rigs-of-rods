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

// overlay
string ovName = "ov"+thisScript;
bool ov_fail = false;
// panel
string paName = "pa"+thisScript;
bool pa_fail = false;
int paNumCreates = 0;
// misc
int framecounter = 0;
float pos_step = 50; // pixels

void frameStep(float dt)
{
    Ogre::Overlay@ ov;
    if (!ov_fail)
    { 
        // NOTE: getByName() will calmly return NULL if overlay doesn't exist.
        @ov = Ogre::OverlayManager::getSingleton().getByName(ovName);  
        // CAUTION: attempt to create the same overlay again will throw an exception, interrupting the script in between!
        if (@ov == null) 
        { 
            @ov = Ogre::OverlayManager::getSingleton().create(ovName); 
        }
        if (@ov == null)
        {
            ov_fail = true;
        }
        else
        { 
            ov.show();
        }
    }
    
    Ogre::OverlayElement@ pa;
    
    if (!pa_fail )
    {
        if ( Ogre::OverlayManager::getSingleton().hasOverlayElement(paName)) 
        {
            game.log('Looking for pa');
            // CAUTION: getOverlayElement() will throw exception if not found, always do `hasOverlayElement()` first!
            @pa = Ogre::OverlayManager::getSingleton().getOverlayElement(paName); 
            
        }
        // CAUTION: using the same name twice will throw an exception, interrupting the script in between!
        if (@pa == null )
        {
            @pa = Ogre::OverlayManager::getSingleton().createOverlayElement("Panel", paName); 
            paNumCreates++;
            game.log('paNumCreates:'+paNumCreates);
            if (@pa == null ) 
            { 
                pa_fail=true; 
            }
            else
            {
                //       game.log("adding pa to ov");
                ov.add2D(pa);
                pa.setMetricsMode(Ogre::GMM_PIXELS);
                pa.setPosition(100,100);
                pa.setDimensions(100,100);
                pa.setMaterialName("tracks/wheelface", 'OgreAutodetect');
                pa.show();
            }
        }
        
    }
    
    if (ImGui::Begin("Example", closeBtnHandler.windowOpen, 0))
    {
        closeBtnHandler.draw();
        ImGui::Text("overlays should work; ov_fail:"+ov_fail+", pa_fail:"+pa_fail  +", frames:"+framecounter);
        framecounter++;
        if (!pa_fail && @pa != null)
        {
            
            ImGui::TextDisabled("The wheel overlay:");
        if (ImGui::Button("Hide")) { pa.hide(); }
        ImGui::SameLine();  if (ImGui::Button("Show")) { pa.show(); }
            
        if (ImGui::Button("Position: Left+")) { pa.setLeft(pa.getLeft()+pos_step); }
        ImGui::SameLine();  if (ImGui::Button("Position:left-")) { pa.setLeft(pa.getLeft()-pos_step); }
            
        if (ImGui::Button("Position: Top+")) { pa.setTop(pa.getTop()+pos_step); }
        ImGui::SameLine();  if (ImGui::Button("Position:Top-")) { pa.setTop(pa.getTop()-pos_step); }
            
        if (ImGui::Button("Width+")) { pa.setWidth(pa.getWidth()+pos_step); }
        ImGui::SameLine();  if (ImGui::Button("Width-")) { pa.setWidth(pa.getWidth()-pos_step); }
            
        if (ImGui::Button("height+")) { pa.setHeight(pa.getHeight()+pos_step); }
        ImGui::SameLine();  if (ImGui::Button("height-")) { pa.setHeight(pa.getHeight()-pos_step); }
            
        }
        ImGui::End();
    }     
}
