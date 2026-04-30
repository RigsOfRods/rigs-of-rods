// \title Simulated Values on Actors
// \brief Demo of new simulated values on actors concept.
// Simulated values on actors allow scripts to send input
// events to a particular actor by simulating the input state.
// This script is based on example_game_setEventSimulatedValue.as
// ===================================================

// Window [X] button handler
#include "imgui_utils.as"
imgui_utils::CloseWindowPrompt closeBtnHandler;

void drawInputBtn(inputEvents ev, string caption, BeamClass@ vehicle)
{
  ImGui::Button(caption);
  if (ImGui::IsItemActive())   // Is button currently pressed?
  { vehicle.setEventSimulatedValue(ev, 1.f); }
  else 
  { vehicle.removeEventSimulatedValue(ev); } // IMPORTANT! The value is persistent, we must also reset it.
}

void frameStep(float dt)
{
    if (ImGui::Begin("Simulated Values on Actors", closeBtnHandler.windowOpen, 0))
    {
        closeBtnHandler.draw();
    
        drawBody();
        ImGui::End();
    }
}

void drawBody()
{
  BeamClass@ vehicle = game.getCurrentTruck();
  if (@vehicle != null)
  {
    ImGui::TextDisabled("Vehicle controls:");
    drawInputBtn(EV_TRUCK_ACCELERATE, "   ^   ", vehicle);
    drawInputBtn(EV_TRUCK_STEER_LEFT, "<", vehicle);
    ImGui::SameLine();
    drawInputBtn(EV_TRUCK_STEER_RIGHT, ">", vehicle);
    drawInputBtn(EV_TRUCK_BRAKE, "   v   ", vehicle);

    if (ImGui::Button("Clear simulated values for this vehicle"))
      vehicle.clearEventSimulatedValues();
  }
}
