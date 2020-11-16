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

#include "imgui_internal.h" // ImTextCharFromUtf8
#include <regex>
#include <stdio.h> // sscanf

static const std::regex  TEXT_COLOR_REGEX = std::regex(R"(#[a-fA-F\d]{6})");
static const int         TEXT_COLOR_MAX_LEN = 5000;

// --------------------------------
// ImTextFeeder

void RoR::ImTextFeeder::AddInline(ImU32 color, ImVec2 text_size, const char* text_begin, const char* text_end)
{
    drawlist->AddText(this->cursor, color, text_begin, text_end);
    this->cursor.x += text_size.x;
    this->size.x = std::max(this->cursor.x - this->origin.x, this->size.x);
    this->size.y = std::max(this->size.y, text_size.y);
}

void RoR::ImTextFeeder::AddWrapped(ImU32 color, float wrap_width, const char* text_begin, const char* text_end)
{
    ImVec2 size = ImGui::CalcTextSize(text_begin, text_end);
    const float cur_width = this->cursor.x - this->origin.x;
    if (wrap_width > 0.f && cur_width + size.x > wrap_width)
    {
        this->NextLine();
    }

    // Trim blanks at the start of new line (not first line), like ImGui::TextWrapped() does.
    bool recalc_size = false;
    if (this->cursor.x == this->origin.x && this->cursor.y != this->origin.y)
    {
        while ((*text_begin == ' ' || *text_begin == '\t'))
        {
            ++text_begin;
            recalc_size = true;
            if (text_begin == text_end)
                return; // Nothing more to draw
        }
    }

    if (recalc_size)
        size = ImGui::CalcTextSize(text_begin, text_end);

    this->AddInline(color, size, text_begin, text_end);
}

void RoR::ImTextFeeder::AddMultiline(ImU32 color, float wrap_width, const char* text_begin, const char* text_end)
{
    const char* text_pos = text_begin;
    const char* tok_begin = nullptr;
    while (text_pos != text_end)
    {
        // Decode and advance one character
        unsigned int c = (unsigned int)*text_pos;
        int bytes_advance = 1;
        if (c >= 0x80)
            bytes_advance += ImTextCharFromUtf8(&c, text_pos, text_end);

        if (c == '\r')
        {} // Ignore carriage return
        else if (c == '\n')
        {
            if (tok_begin != nullptr)
            {
                this->AddWrapped(color, wrap_width, tok_begin, text_pos); // Print the token
            }
            tok_begin = nullptr;
            this->NextLine();
        }
        else if (c == ' ' || c == '\t')
        {
            if (tok_begin != nullptr)
            {
                this->AddWrapped(color, wrap_width, tok_begin, text_pos); // Print the token
                tok_begin = nullptr;
            }
            this->AddWrapped(color, wrap_width, text_pos, text_pos + bytes_advance); // Print the blank
        }
        else
        {
            if (tok_begin == nullptr)
                tok_begin = text_pos;
        }
        text_pos += bytes_advance;
    }

    if (tok_begin != nullptr)
    {
        this->AddWrapped(color, wrap_width, tok_begin, text_end); // Print last token
    }
}

void RoR::ImTextFeeder::NextLine()
{
    const float height = ImGui::GetTextLineHeight();
    this->size.y += height;
    this->cursor.x = this->origin.x;
    this->cursor.y += height;
}

// --------------------------------
// Global functions

// Internal helper
inline void ColorToInts(ImVec4 v, int&r, int&g, int&b) { r=(int)(v.x*255); g=(int)(v.y*255); b=(int)(v.z*255); }

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

ImVec2 RoR::DrawColorMarkedText(ImDrawList* drawlist, ImVec2 text_cursor, ImVec4 default_color, float override_alpha, float wrap_width, std::string const& line)
{
    ImTextFeeder feeder(drawlist, text_cursor);
    int r,g,b, dark_r,dark_g,dark_b;
    ColorToInts(default_color, r,g,b);
    ColorToInts(App::GetGuiManager()->GetTheme().color_mark_max_darkness, dark_r, dark_g, dark_b);
    std::smatch color_match;
    std::string::const_iterator seg_start = line.begin();
    while (std::regex_search(seg_start, line.end(), color_match, TEXT_COLOR_REGEX)) // Find next marker
    {
        // Print segment before the color marker (if any)
        std::string::const_iterator seg_end = color_match[0].first;
        if (seg_start != seg_end)
        {
            feeder.AddMultiline(ImColor(r,g,b,(int)(override_alpha*255)), wrap_width, &*seg_start, &*seg_end);
        }
        // Prepare for printing segment after color marker
        sscanf(color_match.str(0).c_str(), "#%2x%2x%2x", &r, &g, &b);
        if (r==0 && g==0 && b==0)
        {
            ColorToInts(default_color, r,g,b);
        }
        else if (r < dark_r && g < dark_g && b < dark_b)
        {
            // If below darkness limit, invert the color to lighen it up.
            r = 255-r;
            g = 255-g;
            b = 255-b;
        }
        seg_start = color_match[0].second;
    }

    // Print final segment (if any)
    if (seg_start != line.begin() + line.length())
    {
        feeder.AddMultiline(ImColor(r,g,b,(int)(override_alpha*255)), wrap_width, &*seg_start, &*line.end());
    }

    return feeder.size;
}

void RoR::DrawGCheckbox(CVar* cvar, const char* label)
{
    bool val = cvar->GetBool();
    if (ImGui::Checkbox(label, &val))
    {
        cvar->SetVal(val);
    }
}

void RoR::DrawGIntCheck(CVar* cvar, const char* label)
{
    bool val = (cvar->GetInt() != 0);
    if (ImGui::Checkbox(label, &val))
    {
        cvar->SetVal(val ? 1 : 0);
    }
}

void RoR::DrawGIntBox(CVar* cvar, const char* label)
{
    int val = cvar->GetInt();
    if (ImGui::InputInt(label, &val, 1, 100, ImGuiInputTextFlags_EnterReturnsTrue))
    {
        cvar->SetVal(val);
    }
}

void RoR::DrawGIntSlider(CVar* cvar, const char* label, int v_min, int v_max)
{
    int val = cvar->GetInt();

    if (ImGui::SliderInt(label, &val, v_min, v_max))
    {
        cvar->SetVal(val);
    }
}

void RoR::DrawGFloatSlider(CVar* cvar, const char* label, float v_min, float v_max)
{
    float val = cvar->GetFloat();

    if (ImGui::SliderFloat(label, &val, v_min, v_max, "%.2f"))
    {
        cvar->SetVal(val);
    }
}

void RoR::DrawGFloatBox(CVar* cvar, const char* label)
{
    float fval = cvar->GetFloat();
    if (ImGui::InputFloat(label, &fval, 0.f, 0.f, "%.3f", ImGuiInputTextFlags_EnterReturnsTrue))
    {
        cvar->SetVal(fval);
    }
}

void RoR::DrawGTextEdit(CVar* cvar, const char* label, Str<1000>& buf)
{
    if (ImGui::InputText(label, buf.GetBuffer(), buf.GetCapacity(), ImGuiInputTextFlags_EnterReturnsTrue))
    {
        cvar->SetStr(buf.GetBuffer());
    }
    if (ImGui::IsItemActive())
    {
        ImGui::TextDisabled("(hit Enter key to submit)");
    }
    else
    {
        buf.Assign(cvar->GetStr().c_str());
    }
}

void RoR::DrawGCombo(CVar* cvar, const char* label, const char* values)
{
    int selection = cvar->GetInt();
    if (ImGui::Combo(label, &selection, values))
    {
        cvar->SetVal(selection);
    }
}
