/*
    This source file is part of Rigs of Rods
    Copyright 2016-2017 Petr Ohlidal & contributors

    For more information, see http://www.rigsofrods.org/

    Rigs of Rods is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 3, as
    published by the Free Software Foundation.

    Rigs of Rods is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Rigs of Rods. If not, see <http://www.gnu.org/licenses/>.
*/

#include "GUIUtils.h"

void RoR::DrawImGuiSpinner(float& counter, const ImVec2 size, const float spacing, const float step_sec)
{
    // Hardcoded to 4 segments, counter is reset after full round (4 steps)
    // --------------------------------------------------------------------

    const ImU32 COLORS[] = { ImColor(255,255,255,255), ImColor(210,210,210,255), ImColor(120,120,120,255), ImColor(60,60,60,255) };

    // Update counter, determine coloring
    counter += ImGui::GetIO().DeltaTime;
    int color_start = 0; // Index to GUI_SPINNER_COLORS array for the top middle segment (segment 0)
    while (counter > (step_sec*4.f))
    {
        counter -= (step_sec*4.f);
    }

    if (counter > (step_sec*3.f))
    {
        color_start = 3;
    }
    else if (counter > (step_sec*2.f))
    {
        color_start = 2;
    }
    else if (counter > (step_sec))
    {
        color_start = 1;
    }

    // Draw segments
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    const ImVec2 pos = ImGui::GetCursorScreenPos();
    const float left = pos.x;
    const float top = pos.y;
    const float right = pos.x + size.x;
    const float bottom = pos.y + size.y;
    const float mid_x = pos.x + (size.x / 2.f);
    const float mid_y = pos.y + (size.y / 2.f);

    // NOTE: Enter vertices in clockwise order, otherwise anti-aliasing doesn't work and polygon is rasterized larger! -- Observed under OpenGL2 / OGRE 1.9

    // Top triangle, vertices: mid, left, right
    draw_list->AddTriangleFilled(ImVec2(mid_x, mid_y-spacing),   ImVec2(left + spacing, top),     ImVec2(right - spacing, top),     COLORS[color_start]);
    // Right triangle, vertices: mid, top, bottom
    draw_list->AddTriangleFilled(ImVec2(mid_x+spacing, mid_y),   ImVec2(right, top + spacing),    ImVec2(right, bottom - spacing),  COLORS[(color_start+3)%4]);
    // Bottom triangle, vertices: mid, right, left
    draw_list->AddTriangleFilled(ImVec2(mid_x, mid_y+spacing),   ImVec2(right - spacing, bottom), ImVec2(left + spacing, bottom),   COLORS[(color_start+2)%4]);
    // Left triangle, vertices: mid, bottom, top
    draw_list->AddTriangleFilled(ImVec2(mid_x-spacing, mid_y),   ImVec2(left, bottom - spacing),  ImVec2(left, top + spacing),      COLORS[(color_start+1)%4]);
}

void RoR::DrawGCheckbox(GVarPod_A<bool>& gvar, const char* label)
{
    bool val = gvar.GetActive();
    if (ImGui::Checkbox(label, &val))
    {
        gvar.SetActive(val);
    }
}

void RoR::DrawGCheckbox(GVarPod_APS<bool>& gvar, const char* label)
{
    bool val = gvar.GetStored();
    if (ImGui::Checkbox(label, &val))
    {
        gvar.SetActive(val);
        gvar.SetStored(val);
    }
}

void RoR::DrawGIntCheck(GVarPod_A<int>& gvar, const char* label)
{
    bool val = (gvar.GetActive() != 0);
    if (ImGui::Checkbox(label, &val))
    {
        gvar.SetActive(val ? 1 : 0);
    }
}

void RoR::DrawGIntBox(GVarPod_A<int>& gvar, const char* label)
{
    int val = gvar.GetActive();
    if (ImGui::InputInt(label, &val, 1, 100, ImGuiInputTextFlags_EnterReturnsTrue))
    {
        gvar.SetActive(val);
    }
}

void RoR::DrawGIntSlider(GVarPod_A<int>& gvar, const char* label, int v_min, int v_max)
{
    int val = gvar.GetActive();
    if (ImGui::SliderInt(label, &val, v_min, v_max))
    {
        gvar.SetActive(val);
    }
}

void RoR::DrawGFloatSlider(GVarPod_A<float>& gvar, const char* label, float v_min, float v_max)
{
    float val = gvar.GetActive();
    if (ImGui::SliderFloat(label, &val, v_min, v_max, "%.2f"))
    {
        gvar.SetActive(val);
    }
}

void RoR::DrawGFloatBox(GVarPod_A<float>& gvar, const char* label)
{
    float fval = gvar.GetActive();
    if (ImGui::InputFloat(label, &fval, 0.f, 0.f, -1, ImGuiInputTextFlags_EnterReturnsTrue))
    {
        gvar.SetActive(fval);
    }
}

void RoR::DrawGFloatBox(GVarPod_APS<float>& gvar, const char* label)
{
    float fval = gvar.GetStored();
    if (ImGui::InputFloat(label, &fval, 0.f, 0.f, -1, ImGuiInputTextFlags_EnterReturnsTrue))
    {
        gvar.SetActive(fval);
        gvar.SetStored(fval);
    }
}
