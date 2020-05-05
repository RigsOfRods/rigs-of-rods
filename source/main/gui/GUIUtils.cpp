/*
    This source file is part of Rigs of Rods
    Copyright 2013-2020 Petr Ohlidal

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

// Internal helper
static inline ImVec2 ImRotate(const ImVec2& v, float cos_a, float sin_a) 
{ 
    return ImVec2(v.x * cos_a - v.y * sin_a, v.x * sin_a + v.y * cos_a);
}

//source: https://github.com/ocornut/imgui/issues/1982#issuecomment-408834301
void RoR::DrawImageRotated(ImTextureID tex_id, ImVec2 center, ImVec2 size, float angle)
{
    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    float cos_a = cosf(angle);
    float sin_a = sinf(angle);
    ImVec2 pos[4] =
    {
        center + ImRotate(ImVec2(-size.x * 0.5f, -size.y * 0.5f), cos_a, sin_a),
        center + ImRotate(ImVec2(+size.x * 0.5f, -size.y * 0.5f), cos_a, sin_a),
        center + ImRotate(ImVec2(+size.x * 0.5f, +size.y * 0.5f), cos_a, sin_a),
        center + ImRotate(ImVec2(-size.x * 0.5f, +size.y * 0.5f), cos_a, sin_a)
    };
    ImVec2 uvs[4] = 
    { 
        ImVec2(0.0f, 0.0f), 
        ImVec2(1.0f, 0.0f), 
        ImVec2(1.0f, 1.0f), 
        ImVec2(0.0f, 1.0f) 
    };

    draw_list->AddImageQuad(tex_id, pos[0], pos[1], pos[2], pos[3], uvs[0], uvs[1], uvs[2], uvs[3], IM_COL32_WHITE);
}

void RoR::DrawGCheckbox(CVar* cvar, const char* label)
{
    bool val = cvar->GetActiveVal<bool>();
    if (ImGui::Checkbox(label, &val))
    {
        cvar->SetActiveVal(val);
        if (!cvar->HasFlags(CVAR_AUTO_STORE))
        {
            cvar->SetStoredVal(val);
        }
    }
}

void RoR::DrawGIntCheck(CVar* cvar, const char* label)
{
    bool val = (cvar->GetActiveVal<int>() != 0);
    if (ImGui::Checkbox(label, &val))
    {
        cvar->SetActiveVal(val ? 1 : 0);
        if (!cvar->HasFlags(CVAR_AUTO_STORE))
        {
            cvar->SetStoredVal(val);
        }
    }
}

void RoR::DrawGIntBox(CVar* cvar, const char* label)
{
    int val = cvar->GetActiveVal<int>();
    if (ImGui::InputInt(label, &val, 1, 100, ImGuiInputTextFlags_EnterReturnsTrue))
    {
        cvar->SetActiveVal(val);
        if (!cvar->HasFlags(CVAR_AUTO_STORE))
        {
            cvar->SetStoredVal(val);
        }
    }
}

void RoR::DrawGIntSlider(CVar* cvar, const char* label, int v_min, int v_max)
{
    int val = (cvar->HasFlags(CVAR_ALLOW_STORE) && !cvar->HasFlags(CVAR_AUTO_STORE))
        ? cvar->GetStoredVal<int>() : cvar->GetActiveVal<int>();

    if (ImGui::SliderInt(label, &val, v_min, v_max))
    {
        cvar->SetActiveVal(val);
        if (!cvar->HasFlags(CVAR_AUTO_STORE))
        {
            cvar->SetStoredVal(val);
        }
    }
}

void RoR::DrawGFloatSlider(CVar* cvar, const char* label, float v_min, float v_max)
{
    float val = (cvar->HasFlags(CVAR_ALLOW_STORE) && !cvar->HasFlags(CVAR_AUTO_STORE))
        ? cvar->GetStoredVal<float>() : cvar->GetActiveVal<float>();

    if (ImGui::SliderFloat(label, &val, v_min, v_max, "%.2f"))
    {
        cvar->SetActiveVal(val);
        if (!cvar->HasFlags(CVAR_AUTO_STORE))
        {
            cvar->SetStoredVal(val);
        }
    }
}

void RoR::DrawGFloatBox(CVar* cvar, const char* label)
{
    float fval = cvar->GetActiveVal<float>();
    if (ImGui::InputFloat(label, &fval, 0.f, 0.f, "%.3f", ImGuiInputTextFlags_EnterReturnsTrue))
    {
        cvar->SetActiveVal(fval);
        if (!cvar->HasFlags(CVAR_AUTO_STORE))
        {
            cvar->SetStoredVal(fval);
        }
    }
}

void RoR::DrawGTextEdit(CVar* cvar, const char* label, Str<1000>& buf)
{
    if (ImGui::InputText(label, buf.GetBuffer(), buf.GetCapacity(), ImGuiInputTextFlags_EnterReturnsTrue))
    {
        cvar->SetActiveStr(buf.GetBuffer());
        if (!cvar->HasFlags(CVAR_AUTO_STORE))
        {
            cvar->SetStoredStr(buf.GetBuffer());
        }
    }
    if (ImGui::IsItemActive())
    {
        ImGui::TextDisabled("(hit Enter key to submit)");
    }
    else
    {
        buf.Assign(cvar->GetActiveStr().c_str());
    }
}

void RoR::DrawGCombo(CVar* cvar, const char* label, const char* values)
{
    int selection = cvar->GetActiveVal<int>();
    if (ImGui::Combo(label, &selection, values))
    {
        cvar->SetActiveVal(selection);
        if (!cvar->HasFlags(CVAR_AUTO_STORE))
        {
            cvar->SetStoredVal(selection);
        }
    }
}
