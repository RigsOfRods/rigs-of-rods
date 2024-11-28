// BUS STACKING SCRIPT for performance testing
// requested from Discord#dev:
//   https://discord.com/channels/136544456244461568/189904947649708032/1242063394601828503
// ===================================================

// Window [X] button handler
#include "imgui_utils.as"
imgui_utils::CloseWindowPrompt closeBtnHandler;

const vector3 Y_AXIS(0,1,0);

int cfgXCount = 4;
int cfgYCount = 2; // up axis
int cfgZCount = 4;
float cfgXStepMeters = 12.2f;
float cfgYStepMeters = 3.5f;  // up axis
float cfgZStepMeters = 4.f;

void frameStep(float dt)
{
    if (ImGui::Begin("::: GRID SPAWN :::", closeBtnHandler.windowOpen, 0))
    {
        // Draw the "Terminate this script?" prompt on the top (if not disabled by config).
        closeBtnHandler.draw();
        
        drawMainPanel();
        
        ImGui::End();
    }
}

void drawMainPanel()
{
    
if (ImGui::InputInt("X count", cfgXCount)) { if (cfgXCount < 1) {cfgXCount=1;} }
if (ImGui::InputInt("Y count (up)", cfgYCount)) { if (cfgYCount < 1) {cfgYCount=1;} }
if (ImGui::InputInt("Z count", cfgZCount)) { if (cfgZCount < 1) {cfgZCount=1;} }
    ImGui::InputFloat("X step (m)", cfgXStepMeters); 
    ImGui::InputFloat("Y (up) step (m)", cfgYStepMeters); 
    ImGui::InputFloat("Z step (m)", cfgZStepMeters); 
    
    ImGui::Separator();
    if (ImGui::Button("stack busses! (total "+cfgXCount*cfgYCount*cfgZCount+")"))
    {
        stackBusses();
    }
}

void stackBusses()
{
    bool freePosition  = false;
    quaternion gridRot(game.getPersonRotation(), Y_AXIS);
    for (int iy = 0; iy < cfgYCount; iy++)
    {
        for (int ix = 0; ix < cfgXCount; ix++)
        {
            for (int iz = 0; iz < cfgZCount; iz++)
            {
                vector3 gridPos( ix * cfgXStepMeters,  iy * cfgYStepMeters, iz * cfgZStepMeters);
                vector3 spawnPos = game.getPersonPosition() + (gridRot * gridPos);
                
                game.pushMessage(MSG_SIM_SPAWN_ACTOR_REQUESTED,  { 
                {'filename', '95bbUID-agoras.truck'},
                {'free_position', freePosition},
                {'position', spawnPos},
                {'rotation',  quaternion(game.getPersonRotation(), Y_AXIS)}
                });
            }
        }
    }
}
