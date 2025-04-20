// Demonstrates new FreeeForce type: HALFBEAM
// ===================================================

// Window [X] button handler
#include "imgui_utils.as"
imgui_utils::CloseWindowPrompt closeBtnHandler;

// #region Record keeping
class FFRecord
{
    FFRecord(int _id) 
    {
        id = _id; 
    }
    
    int id;
    int lastEventType = 0;
    string lastEventActualValue;
    string lastEventThresholdValue;
};
array<FFRecord@> addedFreeforces;
// #endregion

// #region Adding and removing
int ffBaseActorId = 0, ffTargetActorId = 0;
int ffBaseNodeNum = 0, ffTargetNodeNum = 0;
bool autoAddOppositeHalfbeam=true;

void addFreeForce()
{
    int id = game.getFreeForceNextId();
    game.pushMessage(MSG_SIM_ADD_FREEFORCE_REQUESTED, {
    {'id', id},
    {'type', FREEFORCETYPE_HALFBEAM_GENERIC},
    {'force_magnitude', 0.f}, // unused by HALFBEAM but still required.
    {'base_actor', ffBaseActorId},
    {'base_node', ffBaseNodeNum},
    {'target_actor', ffTargetActorId},
    {'target_node', ffTargetNodeNum}
    });
    
    addedFreeforces.insertLast(FFRecord(id));
    
    if (autoAddOppositeHalfbeam)
    {
        int id2 = game.getFreeForceNextId();
        game.pushMessage(MSG_SIM_ADD_FREEFORCE_REQUESTED, {
        {'id', id2},
        {'type', FREEFORCETYPE_HALFBEAM_GENERIC},
        {'force_magnitude', 0.f}, // unused by HALFBEAM but still required.
        {'target_actor', ffBaseActorId},
        {'target_node', ffBaseNodeNum},
        {'base_actor', ffTargetActorId},
        {'base_node', ffTargetNodeNum}
        });
        
        addedFreeforces.insertLast(FFRecord(id2));
    }
}
void removeFreeForce(uint iToRemove)
{
    game.pushMessage(MSG_SIM_REMOVE_FREEFORCE_REQUESTED, {
    {'id', addedFreeforces[iToRemove].id}
    });
    addedFreeforces.removeAt(iToRemove);
}
// #endregion

// #region Handling game events
void handleEventFreeforcesActivity(int type, int freeforceId, string actualValue, string thresholdValue)
{
    // look up the freeforce
    for (uint i=0; i < addedFreeforces.length(); i++)
    {
        if (addedFreeforces[i].id == freeforceId)
        {
            // freeforce found; update record and exit.
            addedFreeforces[i].lastEventType = type;
            addedFreeforces[i].lastEventActualValue = actualValue;
            addedFreeforces[i].lastEventThresholdValue = thresholdValue;
            return;
        }
    }
    
    
    // report unhandled event
    game.log("Halfbeam example script: received event of unknown freeforce ID "+ freeforceId
    +"   --> Event:"+type
    + " Actual:" + actualValue
    + " Threshold:" + thresholdValue);
}
// #endregion

// #region UI - added freeforces
void drawAddedFreeforces()
{
    ImGui::TextDisabled("Added freeforces:");
    ImGui::TextDisabled("(NOTE: Not reported from game, just remembered in script)");
    ImGui::TextDisabled("(--> if script exits with freeforces not deleted, they are stuck)");
    if (addedFreeforces.length() == 0)
    {
        ImGui::Text("none");
    }
    else
    {
        int iToRemove = -1;
        for(uint i=0; i<addedFreeforces.length(); i++)
        {
            ImGui::Bullet(); 
            ImGui::SameLine(); 
            ImGui::Text("FreeForce ID: " + addedFreeforces[i].id);
            ImGui::SameLine(); 
            if (ImGui::SmallButton("Delete##"+i))
            {
                iToRemove = int(i);
            }
            drawFFActivityEventInfo(i);
        }
        if (iToRemove!=-1)
        {
            removeFreeForce(uint(iToRemove));
        }
    }
}

void drawFFActivityEventInfo(uint i)
{
    switch (addedFreeforces[i].lastEventType)
    {
        case FREEFORCESACTIVITY_ADDED: 
        {
            ImGui::Text("    --> Last game event: Freeforce was added."); 
            return;
        }
        // case FREEFORCESACTIVITY_REMOVED ~ Can never be displayed here, record already gone.
        case FREEFORCESACTIVITY_MODIFIED: 
        {
            ImGui::Text("    --> Last game event: Freeforce was modified."); 
            return;
        }
        case FREEFORCESACTIVITY_DEFORMED:
        {
            ImGui::Text("    --> Last game event: Freeforce was deformed. "
            + " Actual stress:" + addedFreeforces[i].lastEventActualValue
            + " Threshold stress:" + addedFreeforces[i].lastEventThresholdValue
            );
            return;
        }
        case FREEFORCESACTIVITY_BROKEN:
        {
            ImGui::Text("    --> Last game event: Freeforce was broken. "
            + " Actual force:" + addedFreeforces[i].lastEventActualValue
            + " Threshold force:" + addedFreeforces[i].lastEventThresholdValue
            );
            return;
        }
        default:
        {
            ImGui::Text("    --> No game events yet. "); 
            return;
        }
    }
}
// #endregion

// #region UI - add freeforce form
void drawFreeforceForm()
{
    ImGui::TextDisabled("Add FreeForce (type HALFBEAM):");
    ImGui::TextDisabled("(REMEMBER: a freebeam is 2 halfbeams in opposite directions)");
    
    ImGui::Text("Base: ");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(100);
    ImGui::InputInt("Actor ID##base", ffBaseActorId); 
    ImGui::SameLine();
    ImGui::SetNextItemWidth(100);
    ImGui::InputInt("node num##base", ffBaseNodeNum);
    
    ImGui::Text("Target: ");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(100);
    ImGui::InputInt("Actor ID##target", ffTargetActorId); 
    ImGui::SameLine();
    ImGui::SetNextItemWidth(100);
    ImGui::InputInt("node num##target", ffTargetNodeNum);
    
    ImGui::Checkbox("Auto add opposite halfbeam", autoAddOppositeHalfbeam);
    
    if (ImGui::Button("Add the FreeForce(s)"))
    {
        addFreeForce();
    }
    ImGui::SameLine();
    ImGui::Text("CAUTION: glitches out if either actor isn't activated!");
}
// #endregion

// #region UI - drawing actors
void drawActors()
{
    ImGui::TextDisabled("Available actors:");
    array<BeamClass@> actors = game.getAllTrucks();
    for (uint i=0; i<actors.length(); i++)
    {
        ImGui::Bullet(); 
        ImGui::SameLine(); 
        ImGui::Text(actors[i].getTruckName() + " (instance ID: " + actors[i].getInstanceId() + ")");
    }
}
// #endregion

// #region Setup and updates
void main()
{
    game.registerForEvent(SE_GENERIC_FREEFORCES_ACTIVITY);
    // Uncomment to close window without asking.
    //closeBtnHandler.cfgCloseImmediatelly = true;
    closeBtnHandler.cfgPromptText = "Exit now? Make sure to delete all freeforces or they're stuck!";
}

// `frameStep()` runs every frame; `dt` is delta time in seconds.
void frameStep(float dt)
{
    // Begin drawing window
    if (ImGui::Begin("FreeForce type HALFBEAM demo", closeBtnHandler.windowOpen, 0))
    {
        // Draw the "Terminate this script?" prompt on the top (if not disabled by config).
        closeBtnHandler.draw();
        
        drawActors();
        ImGui::Separator();
        drawAddedFreeforces();
        ImGui::Separator();
        drawFreeforceForm();
        
        // End drawing window
        ImGui::End();
    }
}

// EVENT HANDLING
void eventCallbackEx(scriptEvents ev, 
int arg1, int arg2, int arg3, int arg4, 
string arg5, string arg6, string arg7, string arg8)
{
    if (ev == SE_GENERIC_FREEFORCES_ACTIVITY)
    {
        handleEventFreeforcesActivity(arg1, arg2, arg5, arg6);
    }
}
// #endregion
