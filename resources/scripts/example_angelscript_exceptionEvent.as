// TEST SCRIPT for Ogre Exceptions forwarded as game event, 
// based on Overlays code because those throw a lot.
// ===================================================

// Window [X] button handler
#include "imgui_utils.as"
imgui_utils::CloseWindowPrompt closeBtnHandler;

dictionary exception_stats; // fullDesc -> num occurences

// SETUP
void main() { game.registerForEvent(SE_GENERIC_EXCEPTION_CAUGHT); }

// EVENT HANDLING
void eventCallbackEx(scriptEvents ev, 
    int arg1, int arg2, int arg3, int arg4, 
    string arg5, string arg6, string arg7, string arg8)
{
    if (ev == SE_GENERIC_EXCEPTION_CAUGHT)
    {
       // format: FROM >> MSG (TYPE)
        string desc=arg5+' --> '+arg7 +' ('+arg6+')';
        exception_stats[desc] = int(        exception_stats[desc])+1;
    }
}

void frameStep(float dt)
{
    Ogre::Overlay@ ov;
    Ogre::OverlayElement@ pa;

    // CAUTION: attempt to create the same overlay again will throw an exception, 
    @ ov =  Ogre::OverlayManager::getSingleton().create("ov");

    // CAUTION: getOverlayElement() will throw exception if not found, 
    @pa = Ogre::OverlayManager::getSingleton().getOverlayElement("pa"); 

    // CAUTION: using the same name twice will throw an exception,
    @ pa = Ogre::OverlayManager::getSingleton().createOverlayElement("Panel", "pa"); 

    // ---- the exceptions ----
    if (ImGui::Begin("Exception events", closeBtnHandler.windowOpen, 0))
    {
        closeBtnHandler.draw();
        
        array<string>@tag_keys = exception_stats.getKeys();
        ImGui::Text("total exception types:"+tag_keys.length());
        for (uint i=0; i < tag_keys.length(); i++) 
        { 
            int tagcount = int(exception_stats[tag_keys[i]]);
            ImGui::Bullet(); 
            ImGui::SameLine(); ImGui::TextDisabled('['+tagcount+']');
            ImGui::SameLine(); ImGui::Text(tag_keys[i]+": "+tagcount);
        }
        ImGui::End();
    }
    // ------ END exceptions ----
}
