// EXAMPLE - how to use `setEventSimulatedValue()` as analog input
// Created to test https://github.com/RigsOfRods/rigs-of-rods/issues/3230
// ===================================================

// Window [X] button handler
#include "imgui_utils.as"
imgui_utils::CloseWindowPrompt closeBtnHandler;

string btnDown = "";
float btnValue = 0.f;
float cfgAccellPerSec = 0.6;

void drawInputBtn(inputEvents ev, string caption)
{
    ImGui::Button(caption);
    if (ImGui::IsItemActive())   // Is button currently pressed?
    {
        btnDown = caption;
        
        inputs.setEventSimulatedValue(ev, btnValue);
    }
    else 
    {
        
    inputs.setEventSimulatedValue(ev, 0.f); } // IMPORTANT! The value is persistent, we must also reset it.
}

// `frameStep()` runs every frame; `dt` is delta time in seconds.
void frameStep(float dt)
{
    btnDown = "";
    if (ImGui::Begin("Example", closeBtnHandler.windowOpen, 0))
    {
        closeBtnHandler.draw();
        
        drawBody();
        ImGui::End();
    }
    
    if (btnDown != "")
    {
        btnValue += dt * cfgAccellPerSec;
    if (btnValue > 1.f) {btnValue = 1.f;}
    }
    else
    {
        btnValue=0.f;
    }
}

void drawBody()
{
    ImGui::InputFloat("cfgAccellPerSec", cfgAccellPerSec);
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
        if (btnDown != "")
        {
            ImGui::Text("["+btnDown + "] value: " + btnValue);
        }
        
    }
}
