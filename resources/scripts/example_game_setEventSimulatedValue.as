// EXAMPLE - how to use 
// ===================================================

// Window [X] button handler
#include "imgui_utils.as"
imgui_utils::CloseWindowPrompt closeBtnHandler;

void drawInputBtn(inputEvents ev, string caption)
{
  ImGui::Button(caption);
  if (ImGui::IsItemActive())   // Is button currently pressed?
  { inputs.setEventSimulatedValue(ev, 1.f); }
  else 
  { inputs.setEventSimulatedValue(ev, 0.f); } // IMPORTANT! The value is persistent, we must also reset it.
}

// `frameStep()` runs every frame; `dt` is delta time in seconds.
void frameStep(float dt)
{
    if (ImGui::Begin("Example", closeBtnHandler.windowOpen, 0))
    {
        closeBtnHandler.draw();
    
        drawBody();
        ImGui::End();
    }
}

void drawBody()
{
    if (@game.getCurrentTruck() == null)
  {
    ImGui::TextDisabled("Character controls:");
    drawInputBtn(EV_CHARACTER_FORWARD, "   ^   ");
    drawInputBtn(EV_CHARACTER_LEFT, "<");
    ImGui::SameLine();
    drawInputBtn(EV_CHARACTER_RIGHT, ">");
    drawInputBtn(EV_CHARACTER_BACKWARDS, "   v   ");
  }
  else
  {
    ImGui::TextDisabled("Vehicle controls:");
    drawInputBtn(EV_TRUCK_ACCELERATE, "   ^   ");
    drawInputBtn(EV_TRUCK_STEER_LEFT, "<");
    ImGui::SameLine();
    drawInputBtn(EV_TRUCK_STEER_RIGHT, ">");
    drawInputBtn(EV_TRUCK_BRAKE, "   v   ");
  }
}
