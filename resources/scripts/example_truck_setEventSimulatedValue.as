// \title Simulated Input Events on Actors
// \brief Demo of new simulated input events on actors concept.
// This allows scripts to send input events to a particular actor
// by simulating the input state.
// This script is based on example_game_setEventSimulatedValue.as
// ===================================================

// Window [X] button handler
#include "imgui_utils.as"
imgui_utils::CloseWindowPrompt closeBtnHandler;
BeamClass@ selectedVehicle;

void drawVehicleSelection()
{
    array<BeamClass@> actors = game.getAllTrucks();
    ImGui::TextDisabled("Spawned vehicles (" + actors.length() + ")");
    int i_selected = -1;
    for (uint i=0; i<actors.length(); i++)
    {
        string rbCaption = "["+i+"] " + actors[i].getTruckName();
        i_selected = (selectedVehicle is actors[i]) ? int(i) : -1;
        if ( ImGui::RadioButton(rbCaption, i_selected, int(i)))
        {
            @selectedVehicle = actors[i];
        }
    }
}

void drawInputBtn(inputEvents ev, string caption, BeamClass@ vehicle)
{
    ImGui::Button(caption);
    if (ImGui::IsItemActive())   // Is button currently pressed?
{ vehicle.setEventSimulatedValue(ev, 1.f); }
    else 
{ vehicle.resetEventSimulatedValue(ev); } // IMPORTANT! The value is persistent, we must also reset it.
}

void frameStep(float dt)
{
    if (ImGui::Begin("Simulated Values on Actors", closeBtnHandler.windowOpen, 0))
    {
        closeBtnHandler.draw();
        drawVehicleSelection();
        ImGui::Separator();
        
        drawVehicleControls(selectedVehicle);
        ImGui::End();
    }
}

void drawVehicleControls( BeamClass@ vehicle)
{
    if (vehicle is null)
    {
        ImGui::Text("No vehicle selected.");
        return;
    }
    
    ImGui::TextDisabled("Vehicle '"+selectedVehicle.getTruckName()+"' controls:");
    drawInputBtn(EV_TRUCK_ACCELERATE, "   ^   ", vehicle);
    drawInputBtn(EV_TRUCK_STEER_LEFT, "<", vehicle);
    ImGui::SameLine();
    drawInputBtn(EV_TRUCK_STEER_RIGHT, ">", vehicle);
    drawInputBtn(EV_TRUCK_BRAKE, "   v   ", vehicle);
    
    if (ImGui::Button("Clear simulated values for this vehicle"))
    {
        vehicle.clearEventSimulatedValues();
    }
}
