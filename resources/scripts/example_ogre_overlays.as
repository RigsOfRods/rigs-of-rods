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
bool ov_buttonpressed=false;
bool ov_fail = false;
// panel
string paName = "pa"+thisScript;
bool pa_buttonpressed=false;
bool pa_fail = false;
bool pa_ready=false;
// misc
int framecounter = 0;
float pos_step = 50; // pixels

void frameStep(float dt)
{
    if (ImGui::Begin("Example", closeBtnHandler.windowOpen, 0))
    {
        closeBtnHandler.draw();
        
        ImGui::TextDisabled("A very crude test of creating OGRE overlays in code");
        ImGui::TextDisabled("Creates one overlay '"+ovName+"' + one panel '"+paName+"'"); 
        ImGui::TextDisabled("and leaves them dangling until you restart the game :)");
        ImGui::TextDisabled("(Note: Names contain scriptUnitID (NID) to avoid clashes and interferences).");
        
        if (!ov_buttonpressed)
        {
            // there's no such API like "does overlay exist? --> boolean", we must retrieve, fortunately it behaves calmly.
            Ogre::Overlay@ ovCheck = Ogre::OverlayManager::getSingleton().getByName(ovName);  
            if (@ovCheck == null)
            {
                if (ImGui::SmallButton("create overlay")) 
                { 
                    ov_buttonpressed=true;
                }
            }
            else
            {
                ImGui::Text("It seems the overlay already exists ?!");
            }
        }
        
        Ogre::Overlay@ ov;
        if (ov_buttonpressed && !ov_fail)
        { 
            // NOTE: getByName() will calmly return NULL if overlay doesn't exist.
            @ov = Ogre::OverlayManager::getSingleton().getByName(ovName);  
            // CAUTION: attempt to create the same overlay again will throw an exception, interrupting the script in between!
            if (@ov == null) 
            { 
                if (Ogre::OverlayManager::getSingleton().create(ovName) == null)  // intentionally not assigning - we want to retrieve from OGRE
                {
                    ov_fail = true;
                    game.log("OGRE overlaymanager create() returned null");
                }
                else
                {
                    Ogre::Overlay@ ovTest = Ogre::OverlayManager::getSingleton().getByName(ovName);  
                    if (@ovTest == null)
                    {
                        game.log("Overlay not created by OGRE");
                        ov_fail = true;
                    }
                }
            }
            else 
            { 
                ov.show();
            }
        }
        
        Ogre::OverlayElement@ pa;
        
        if (!pa_buttonpressed)
        {
            ImGui::Text("OGRE OverlayManager  .hasOverlayElement(paName)--> " + Ogre::OverlayManager::getSingleton().hasOverlayElement(paName));
        if (ImGui::SmallButton("create panel")) { pa_buttonpressed=true; }
        }
        
        if (pa_buttonpressed && !ov_fail && !pa_fail)
        {
            if ( Ogre::OverlayManager::getSingleton().hasOverlayElement(paName)) 
            {
                // CAUTION: getOverlayElement() will throw exception if not found, always do `hasOverlayElement()` first!
                
                @pa = Ogre::OverlayManager::getSingleton().getOverlayElement(paName); 
                if (@pa == null)
                {
                    game.log("OGRE reports panel exists, but cannot be retrieved");
                    pa_fail = true;
                }
                else if (@ov != null && !pa_ready && ImGui::SmallButton("setup the panel"))
                {
                    game.log("adding pa to ov");
                    ov.add2D(pa);
                    pa.setMetricsMode(Ogre::GMM_PIXELS);
                    pa.setPosition(100,100);
                    pa.setDimensions(100,100);
                    pa.setMaterialName("tracks/wheelface", 'OgreAutodetect');
                    pa.show();
                    pa_ready=true;
                }
                
            }
            // CAUTION: using the same name twice will throw an exception, interrupting the script in between!
            else if (ImGui::SmallButton("create panel"))
            {
                game.log("creating pa");
                Ogre::OverlayManager::getSingleton().createOverlayElement("Panel", paName);  // intentionally not assigning, we want to look up.
                Ogre::OverlayElement@ paTest = Ogre::OverlayManager::getSingleton().getOverlayElement(paName); 
                if (@paTest == null ) 
                { 
                    game.log("Panel not created by OGRE");
                    pa_fail=true; 
                }
                
            }
            
        }
        
        
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
