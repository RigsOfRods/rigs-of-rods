// Demonstrates new FreeeForce type: HALFBEAM
// ===================================================

// Window [X] button handler
#include "imgui_utils.as"
imgui_utils::CloseWindowPrompt closeBtnHandler;

const string DEFAULT_MESH="beam.mesh";
const string DEFAULT_MATERIAL="tracks/beam";

// constants from the game source, see <SimConstants.h>
const float DEFAULT_SPRING               = 9000000.0f;
const float DEFAULT_DAMP                 = 12000.0f;
const float DEFAULT_BEAM_DIAMETER        = 0.05f;         //!< 5 centimeters default beam width
const float BEAM_BREAK                   = 1000000.0f;
const float BEAM_DEFORM                  = 400000.0f;
const float BEAM_PLASTIC_COEF_DEFAULT    = 0.f;

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
    int freebeamGfxId = -1; // common for both freeforces that make up the freebeam
};
array<FFRecord@> addedFreeforces;
// #endregion

// #region Adding and removing
int ffBaseActorId = 0, ffTargetActorId = 0;
int ffBaseNodeNum = 0, ffTargetNodeNum = 0;
bool autoAddOppositeHalfbeam=true;
// gfx args
bool autoCreateGfx=true;
string gfxMeshName = "beam.mesh";
string gfxMaterialName = "tracks/beam";
// halfbeam optional args (fine tuning)
float halfbSpring=DEFAULT_SPRING;
float halfbDamp=DEFAULT_DAMP;
float halfbBreak=BEAM_BREAK;
float halfbDeform=BEAM_DEFORM;
float halfbDiameter=DEFAULT_BEAM_DIAMETER; 
float halfbPlasticCoef=BEAM_PLASTIC_COEF_DEFAULT;

void addSingleFreeForce(int id, int bActorId, int bNodeNum, int tActorId, int tNodeNum)
{
    game.pushMessage(MSG_SIM_ADD_FREEFORCE_REQUESTED, {
    {'id', id},
    {'type', FREEFORCETYPE_HALFBEAM_GENERIC},
    {'force_magnitude', 0.f}, // unused by HALFBEAM but still required.
    {'base_actor', bActorId},
    {'base_node', bNodeNum},
    {'target_actor', tActorId},
    {'target_node', tNodeNum},
        // set_beam_defaults...
    {'halfb_spring', halfbSpring},
    {'halfb_damp', halfbDamp},
    {'halfb_strength', halfbBreak},
    {'halfb_deform', halfbDeform},
    {'halfb_diameter', halfbDiameter},
    {'halfb_plastic_coef', halfbPlasticCoef}
    });
}

void addFreeBeam()
{
    int id = game.getFreeForceNextId();
    addSingleFreeForce(id, ffBaseActorId, ffBaseNodeNum, ffTargetActorId, ffTargetNodeNum);    
    addedFreeforces.insertLast(FFRecord(id));
    
    int id2 = -1;
    if (autoAddOppositeHalfbeam)
    {
        id2 =  game.getFreeForceNextId();
        addSingleFreeForce(id2,  ffTargetActorId, ffTargetNodeNum, ffBaseActorId, ffBaseNodeNum);            
        addedFreeforces.insertLast(FFRecord(id2));
    }
    
    if (autoCreateGfx)
    {
        int xid = game.getFreeBeamGfxNextId();
        game.pushMessage(MSG_EDI_ADD_FREEBEAMGFX_REQUESTED, {
        {'id', xid},
        {'freeforce_primary', id},
        {'freeforce_secondary', id2},
        {'mesh_name', gfxMeshName},
        {'material_name', gfxMaterialName}
        });
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
void inputSbd(string name, float&inout val, float defaultVal)
{
    ImGui::SetNextItemWidth(100);
    ImGui::InputFloat(name, val);
    ImGui::SameLine();
    if (ImGui::SmallButton("reset to default##"+name))
    {
        val = defaultVal;
    }
}

void inputGfx(string name, string&inout val, string defaultVal)
{
    ImGui::SetNextItemWidth(200);
    ImGui::InputText(name, val);
    ImGui::SameLine();
    if (ImGui::SmallButton("reset to default##"+name))
    {
        val = defaultVal;
    }
}

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
    
    if (ImGui::CollapsingHeader("Fine tuning (set_beam_defaults)"))
    {
        inputSbd("Spring", halfbSpring, DEFAULT_SPRING);
        inputSbd("Damp", halfbDamp, DEFAULT_DAMP);
        inputSbd("Breaking threshold", halfbBreak, BEAM_BREAK);
        inputSbd("Deform threshold", halfbDeform, BEAM_DEFORM);
        inputSbd("Visual diameter", halfbDiameter, DEFAULT_BEAM_DIAMETER);
        inputSbd("Plastic coef", halfbPlasticCoef, BEAM_PLASTIC_COEF_DEFAULT);
    }
    
    ImGui::Checkbox("Auto add opposite halfbeam", autoAddOppositeHalfbeam);
    ImGui::Checkbox("Create gfx (auto-removed if either freeforce breaks)", autoCreateGfx);
    if (autoCreateGfx)
    {
        inputGfx("mesh", gfxMeshName, DEFAULT_MESH);
        inputGfx("material", gfxMaterialName, DEFAULT_MATERIAL);
    }
    
    if (ImGui::Button("Add the FreeForce(s)"))
    {
        addFreeBeam();
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
