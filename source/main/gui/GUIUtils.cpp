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

#include "Actor.h"
#include "Utils.h"

#include "imgui_internal.h" // ImTextCharFromUtf8
#include <regex>
#include <stdio.h> // sscanf

static const std::regex  TEXT_COLOR_REGEX = std::regex(R"(#[a-fA-F\d]{6})");
static const int         TEXT_COLOR_MAX_LEN = 5000;

// --------------------------------
// ImTextFeeder

RoR::ImTextFeeder::ImTextFeeder(ImDrawList* _drawlist, ImVec2 _origin)
    : drawlist(_drawlist), origin(_origin), cursor(_origin)
{
    line_height = ImGui::GetTextLineHeight();
}

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
            bytes_advance = ImTextCharFromUtf8(&c, text_pos, text_end);

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
    this->size.y += this->line_height;
    this->cursor.x = this->origin.x;
    this->cursor.y += this->line_height;
    this->line_height = ImGui::GetTextLineHeight();
}

// --------------------------------
// Global functions

// Internal helper
inline void ColorToInts(ImVec4 v, int&r, int&g, int&b) { r=(int)(v.x*255); g=(int)(v.y*255); b=(int)(v.z*255); }

// A nice spinner https://github.com/ocornut/imgui/issues/1901#issuecomment-444929973
void RoR::LoadingIndicatorCircle(const char* label, const float indicator_radius, const ImVec4& main_color, const ImVec4& backdrop_color, const int circle_count, const float speed)
{
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems)
    {
        return;
    }

    ImGuiContext& g = *GImGui;
    const ImGuiID id = window->GetID(label);

    const ImVec2 pos = window->DC.CursorPos;
    const float circle_radius = indicator_radius / 10.0f;
    const ImRect bb(pos, ImVec2(pos.x + indicator_radius * 2.0f, pos.y + indicator_radius * 2.0f));
    ImGui::ItemSize(bb, ImGui::GetStyle().FramePadding.y);
    if (!ImGui::ItemAdd(bb, id))
    {
        return;
    }

    const float t = g.Time;
    const auto degree_offset = 2.0f * IM_PI / circle_count;

    for (int i = 0; i < circle_count; ++i)
    {
        const auto x = indicator_radius * std::sin(degree_offset * i);
        const auto y = indicator_radius * std::cos(degree_offset * i);
        const auto growth = std::max(0.0f, std::sin(t * speed - i * degree_offset));
        ImVec4 color;
        color.x = main_color.x * growth + backdrop_color.x * (1.0f - growth);
        color.y = main_color.y * growth + backdrop_color.y * (1.0f - growth);
        color.z = main_color.z * growth + backdrop_color.z * (1.0f - growth);
        color.w = 1.0f;

        window->DrawList->AddCircleFilled(ImVec2(pos.x + indicator_radius + x,
                                                         pos.y + indicator_radius - y),
                                                         circle_radius + growth * circle_radius,
                                                         ImGui::GetColorU32(color));

    }
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

std::string RoR::StripColorMarksFromText(std::string const& text)
{
    return std::regex_replace(text, TEXT_COLOR_REGEX, "");
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
        const char* end_ptr = &line.c_str()[line.length()];
        feeder.AddMultiline(ImColor(r,g,b,(int)(override_alpha*255)), wrap_width, &*seg_start, end_ptr);
    }

    return feeder.size;
}

void RoR::ImTextWrappedColorMarked(std::string const& text)
{
    ImVec2 text_pos = ImGui::GetCursorScreenPos();
    ImVec2 text_size = RoR::DrawColorMarkedText(ImGui::GetWindowDrawList(),
                                                text_pos,
                                                ImGui::GetStyle().Colors[ImGuiCol_Text],
                                                /*override_alpha=*/1.f,
                                                ImGui::GetWindowContentRegionWidth() - ImGui::GetCursorPosX(),
                                                text);
    // From `ImGui::TextEx()` ...
    ImRect bb(text_pos, text_pos + text_size);
    ImGui::ItemSize(text_size);
    ImGui::ItemAdd(bb, 0);
}

bool RoR::DrawGCheckbox(CVar* cvar, const char* label)
{
    bool val = cvar->getBool();
    if (ImGui::Checkbox(label, &val))
    {
        cvar->setVal(val);
        return true;
    }
    return false;
}

void RoR::DrawGIntCheck(CVar* cvar, const char* label)
{
    bool val = (cvar->getInt() != 0);
    if (ImGui::Checkbox(label, &val))
    {
        cvar->setVal(val ? 1 : 0);
    }
}

void RoR::DrawGIntBox(CVar* cvar, const char* label)
{
    int val = cvar->getInt();
    if (ImGui::InputInt(label, &val, 1, 100, ImGuiInputTextFlags_EnterReturnsTrue))
    {
        cvar->setVal(val);
    }
}

void RoR::DrawGIntSlider(CVar* cvar, const char* label, int v_min, int v_max)
{
    int val = cvar->getInt();

    if (ImGui::SliderInt(label, &val, v_min, v_max))
    {
        cvar->setVal(val);
    }
}

void RoR::DrawGFloatSlider(CVar* cvar, const char* label, float v_min, float v_max)
{
    float val = cvar->getFloat();

    if (ImGui::SliderFloat(label, &val, v_min, v_max, "%.2f"))
    {
        cvar->setVal(val);
    }
}

void RoR::DrawGFloatBox(CVar* cvar, const char* label)
{
    float fval = cvar->getFloat();
    if (ImGui::InputFloat(label, &fval, 0.f, 0.f, "%.3f", ImGuiInputTextFlags_EnterReturnsTrue))
    {
        cvar->setVal(fval);
    }
}

void RoR::DrawGTextEdit(CVar* cvar, const char* label, Str<1000>& buf)
{
    if (ImGui::InputText(label, buf.GetBuffer(), buf.GetCapacity(), ImGuiInputTextFlags_EnterReturnsTrue))
    {
        cvar->setStr(buf.GetBuffer());
    }
    if (ImGui::IsItemActive())
    {
        ImGui::TextDisabled("(hit Enter key to submit)");
    }
    else
    {
        buf.Assign(cvar->getStr().c_str());
    }
}

bool RoR::DrawGCombo(CVar* cvar, const char* label, const char* values)
{
    int selection = cvar->getInt();
    if (ImGui::Combo(label, &selection, values))
    {
        cvar->setVal(selection);
        return true;
    }
    return false;
}

Ogre::TexturePtr RoR::FetchIcon(const char* name)
{
    try
    {
        return Ogre::static_pointer_cast<Ogre::Texture>(
            Ogre::TextureManager::getSingleton().createOrRetrieve(name, "FlagsRG").first);
    }
    catch (...) {}

    return Ogre::TexturePtr(); // null
}

ImDrawList* RoR::GetImDummyFullscreenWindow(const std::string& name /* = "RoR_TransparentFullscreenWindow"*/)
{
    ImVec2 screen_size = ImGui::GetIO().DisplaySize;

    // Dummy fullscreen window to draw to
    int window_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar| ImGuiWindowFlags_NoInputs 
                     | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoBringToFrontOnFocus;
    ImGui::SetNextWindowPos(ImVec2(0,0));
    ImGui::SetNextWindowSize(screen_size);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0,0,0,0)); // Fully transparent background!
    ImGui::Begin(name.c_str(), NULL, window_flags);
    ImDrawList* drawlist = ImGui::GetWindowDrawList();
    ImGui::End();
    ImGui::PopStyleColor(1); // WindowBg

    return drawlist;
}

void RoR::ImAddItemToComboboxString(std::string& target, std::string const& item)
{
    // Items must be separated by single NUL character ('\0').
    // -------------------------------------------------------

    if (target == "")
    {
        target += item;
    }
    else
    {
        // Get current size (not counting trailing NUL)
        size_t prev_size = target.size();

        // Make space for 1 separating NUL
        target.resize(prev_size + 1, '\0');

        // Insert new item after the separator
        target.insert(target.begin() + prev_size + 1, item.begin(), item.end());
    }
}

void RoR::ImTerminateComboboxString(std::string& target)
{
    // Items must be separated by double NUL character ("\0\0").
    // ---------------------------------------------------------

    // Get current size (not counting trailing NUL)
    size_t prev_size = target.size();

    // Make space for 2 trailing with NULs
    target.resize(prev_size + 2, '\0');
}

void RoR::ImDrawEventHighlighted(events input_event)
{
    ImVec4 col = ImGui::GetStyle().Colors[ImGuiCol_Text];
    if (App::GetInputEngine()->getEventValue(input_event))
    {
        col = App::GetGuiManager()->GetTheme().highlight_text_color;
    }
    std::string text = App::GetInputEngine()->getEventCommandTrimmed(input_event);
    const ImVec2 PAD = ImVec2(2.f, 0.f);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, PAD);
    ImGui::BeginChildFrame(ImGuiID(input_event), ImGui::CalcTextSize(text.c_str()) + PAD*2);
    ImGui::TextColored(col, "%s", text.c_str());
    ImGui::EndChildFrame();
    ImGui::PopStyleVar(); // FramePadding
}

bool RoR::ImDrawEventHighlightedButton(events input_event, bool* btn_hovered /*=nullptr*/, bool* btn_active /*=nullptr*/)
{
    ImVec4 col = ImGui::GetStyle().Colors[ImGuiCol_Text];
    if (App::GetInputEngine()->getEventValue(input_event))
    {
        col = App::GetGuiManager()->GetTheme().highlight_text_color;
    }
    std::string text = App::GetInputEngine()->getEventCommandTrimmed(input_event);
    const ImVec2 PAD = ImVec2(2.f, 0.f);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, PAD);
    ImGui::PushStyleColor(ImGuiCol_Text, col);
    ImGui::PushID(input_event);
    const bool retval = ImGui::Button(text.c_str());
    if (btn_hovered != nullptr)
    {
        *btn_hovered = ImGui::IsItemHovered();
    }
    if (btn_active != nullptr)
    {
        *btn_active = ImGui::IsItemActive();
    }
    ImGui::PopID(); // input_event
    ImGui::PopStyleColor(); // Text
    ImGui::PopStyleVar(); // FramePadding
    return retval;
}

void RoR::ImDrawModifierKeyHighlighted(OIS::KeyCode key)
{
    ImVec4 col = ImGui::GetStyle().Colors[ImGuiCol_Text];
    if (App::GetInputEngine()->isKeyDown(key))
    {
        col = App::GetGuiManager()->GetTheme().highlight_text_color;
    }
    std::string text = App::GetInputEngine()->getModifierKeyName(key);
    const ImVec2 PAD = ImVec2(2.f, 0.f);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, PAD);
    ImGui::BeginChildFrame(ImGuiID(key), ImGui::CalcTextSize(text.c_str()) + PAD*2);
    ImGui::TextColored(col, "%s", text.c_str());
    ImGui::EndChildFrame();
    ImGui::PopStyleVar(); // FramePadding
}

ImVec2 RoR::ImCalcEventHighlightedSize(events input_event)
{
    std::string text = App::GetInputEngine()->getEventCommandTrimmed(input_event);
    const ImVec2 PAD = ImVec2(2.f, 0.f);
    return ImGui::CalcTextSize(text.c_str()) + PAD*2;
}

bool RoR::ImButtonHoldToConfirm(const std::string& btn_idstr, const bool smallbutton, const float time_limit)
{
    if (smallbutton)
        ImGui::SmallButton(btn_idstr.c_str());
    else
        ImGui::Button(btn_idstr.c_str());

    // When recording the active button, take full ID stack into account
    static const ImGuiID IMGUIID_INVALID = 0u;
    static ImGuiID active_id = IMGUIID_INVALID;
    static float active_time_left = 0.f;
    const ImGuiID btn_id = ImGui::GetCurrentWindow()->GetID(btn_idstr.c_str());

    if (ImGui::IsItemActive())
    {
        if (active_id != btn_id)
        {
            active_id = btn_id;
            active_time_left = time_limit;
        }
        else
        {
            active_time_left -= ImGui::GetIO().DeltaTime;
            if (active_time_left <= 0.f)
            {
                active_id = IMGUIID_INVALID;
                return true;
            }
        }

        ImGui::BeginTooltip();
        std::string text = _L("Hold to confirm");
        ImGui::TextDisabled(text.c_str());
        ImGui::ProgressBar(active_time_left/time_limit, ImVec2(ImGui::CalcTextSize(text.c_str()).x, 8.f), "");
        ImGui::EndTooltip();
    }
    else if (btn_id == active_id)
    {
        active_id = IMGUIID_INVALID;
    }

    return false;
}

bool RoR::ImMoveTextInputCursorToEnd(const char* label)
{
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    const ImGuiID id = window->GetID(label);
    ImGuiContext& g = *GImGui;

    // NB: we are only allowed to access 'edit_state' if we are the active widget.
    ImGuiInputTextState* state = NULL;
    if (g.InputTextState.ID != id)
    {
        return false;
    }

    state = &g.InputTextState;
    // based on `ImGuiInputTextState::CursorClamp()`
    state->Stb.cursor = state->CurLenW;
    state->Stb.select_start = 0;
    state->Stb.select_end = 0;

    return true;
}

bool RoR::GetScreenPosFromWorldPos(Ogre::Vector3 const& world_pos, ImVec2& out_screen)
{
    ImVec2 screen_size = ImGui::GetIO().DisplaySize;
    World2ScreenConverter world2screen(
        App::GetCameraManager()->GetCamera()->getViewMatrix(true), App::GetCameraManager()->GetCamera()->getProjectionMatrix(), Ogre::Vector2(screen_size.x, screen_size.y));
    Ogre::Vector3 pos_xyz = world2screen.Convert(world_pos);
    if (pos_xyz.z < 0.f)
    {
        out_screen.x = pos_xyz.x;
        out_screen.y = pos_xyz.y;
        return true;
    }
    return false;
}
