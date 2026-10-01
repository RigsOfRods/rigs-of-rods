/*
    This source file is part of Rigs of Rods
    Copyright 2020 tritonas00
    Copyright 2021 - 2026 Petr Ohlidal

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


#include "GUI_GameControls.h"

#include "Application.h"
#include "Language.h"
#include "OgreImGui.h"
#include "GUIManager.h"
#include "InputEngine.h"
#include "GfxScene.h"

#include <fmt/format.h>
#include <cmath>

using namespace RoR;
using namespace GUI;

void GameControls::UpdateInteractiveKeybinding()
{
    if (!m_interactive_keybinding_active)
    {
        return;
    }

    // Interactive keybind mode - controls window remains flagged 'visible', but box is drawn instead.

    GUIManager::GuiTheme& theme = App::GetGuiManager()->GetTheme();

    ImGui::SetNextWindowPosCenter();
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize;
    ImGui::SetNextWindowContentWidth(300.f);
    std::string window_title = m_interactive_keybinding_delete_on_cancel ? _LC("GameControls", "Add new input binding") : _LC("GameControls", "Edit existing input binding");
    ImGui::Begin(window_title.c_str(), nullptr, flags);

    // Title and description (centered)
    std::string ev_name = App::GetInputEngine()->eventIDToName(m_active_event);
    ImGui::SetCursorPosX((ImGui::GetContentRegionAvail().x - ImGui::CalcTextSize(ev_name.c_str()).x) / 2);
    ImGui::TextColored(theme.value_blue_text_color, "%s", ev_name.c_str());

    std::string ev_description = App::GetInputEngine()->eventIDToDescription(m_active_event);
    ImGui::SetCursorPosX((ImGui::GetContentRegionAvail().x - ImGui::CalcTextSize(ev_description.c_str()).x) / 2);
    ImGui::TextColored(GRAY_HINT_TEXT, "%s", ev_description.c_str());

    if (m_active_mapping_deviceid == InputEngine::DEFAULT_MAPFILE_DEVICEID)
    {
        // Keyboard bindings; no analog devices applicable.
        m_interactive_keybinding_analog = false;
        ImGui::Separator();
        this->DrawInteractiveKeybindDigital();
    }
    else
    {
        // Tabs (digital vs. analog)
        // Visual choice: calculate+set paddings so that each tab takes exactly half of the window area!
        // Since we have to choose in advance, accomodate the wider text width (imgui will compensate).
        const float TAB_YPADDING = ImGui::GetStyle().FramePadding.y;
        std::string tab_digital_label = _LC("GameControls", "Digital");
        std::string tab_analog_label = _LC("GameControls", "Analog");
        float tab_xlabel = std::min(ImGui::CalcTextSize(tab_digital_label.c_str()).x, ImGui::CalcTextSize(tab_analog_label.c_str()).x);
        float tab_xpadding = (ImGui::GetContentRegionAvail().x/2 - (ImGui::GetStyle().ItemSpacing.x*2 + tab_xlabel))/2;
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(tab_xpadding, TAB_YPADDING));
        ImGui::BeginTabBar("InputBindingType");

        if (ImGui::BeginTabItem(tab_digital_label.c_str()))
        {
            m_interactive_keybinding_analog = false;
            this->DrawInteractiveButtonBinding();
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem(tab_analog_label.c_str()))
        {
            m_interactive_keybinding_analog = true;
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
        ImGui::PopStyleVar(); // FramePadding
    }

    ImGui::Separator();

    // Buttons
    ImVec2 buttons_cursor = ImGui::GetCursorPos();
    const float BTN_BIGXPADDING = 25.f;
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(BTN_BIGXPADDING, ImGui::GetStyle().FramePadding.y));
    if (ImGui::Button(_LC("GameSettings", "Cancel")))
    {
        if (m_interactive_keybinding_delete_on_cancel)
        {
            App::GetInputEngine()->eraseEvent(m_active_event, m_active_trigger);
        }
        this->CancelChanges();
    }
    // ... align 'accept' button to the right
    std::string accept_button_label = _LC("GameSettings", "Accept");
    ImGui::SetCursorPos(buttons_cursor + ImVec2(ImGui::GetContentRegionAvail().x - ImGui::CalcTextSize(accept_button_label.c_str()).x - ImGui::GetStyle().ItemSpacing.x*2 - BTN_BIGXPADDING*2, 0));
    if (ImGui::Button(accept_button_label.c_str()))
    {
        this->ApplyChanges();
    }
    ImGui::PopStyleVar();
    if (!m_interactive_keybinding_delete_on_cancel)
    {
        // ... put 'delete' button in the middle
        std::string delete_button_label = _LC("GameSettings", "Delete");
        ImGui::SetCursorPos(buttons_cursor + ImVec2((ImGui::GetContentRegionAvail().x - ImGui::CalcTextSize(delete_button_label.c_str()).x - ImGui::GetStyle().ItemSpacing.x*2) / 2, 0));
        if (ImGui::Button(delete_button_label.c_str()))
        {
            App::GetInputEngine()->eraseEvent(m_active_event, m_active_trigger);
            this->CancelChanges();
        }
    }

    ImGui::End();
}

void GameControls::DrawInteractiveKeybindDigital()
{
    Ogre::String keys_pressed;
    int num_nonmodifier_keys = App::GetInputEngine()->getCurrentKeyCombo(&keys_pressed);

    if (num_nonmodifier_keys > 0)
    {
        if (m_interactive_keybinding_expl)
        {
            m_active_buffer << "EXPL+" << keys_pressed;
        }
        else
        {
            m_active_buffer = keys_pressed;
        }
        this->ApplyChanges();
        App::GetInputEngine()->resetKeysAndMouseButtons(); // Do not leak the pressed keys to gameplay.
        return;
    }

    // Keys preview (aligned to center)
    const float PREVIEW_YSPACING = 10.f;
    ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2((ImGui::GetContentRegionAvail().x - ImGui::CalcTextSize(keys_pressed.c_str()).x) / 2, PREVIEW_YSPACING));
    ImColor flashing_imcolor(m_flashing_color.r, m_flashing_color.g, m_flashing_color.b, m_flashing_color.a);
    ImGui::TextColored(flashing_imcolor, "%s", keys_pressed.c_str());
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + PREVIEW_YSPACING);

    // EXPL checkbox + tooltip
    ImGui::Checkbox(_LC("GameControls", "EXPL"), &m_interactive_keybinding_expl);
    const bool checkbox_hovered = ImGui::IsItemHovered();
    ImGui::SameLine();
    ImGui::TextDisabled("(?)");
    const bool hint_hovered = ImGui::IsItemHovered();
    if (checkbox_hovered || hint_hovered)
    {
        ImGui::BeginTooltip();
        ImGui::Text("%s", _LC("GameControls",
            "With EXPL tag, only exactly matching key combos will be triggered.\n"
            "Without it, partial matches will trigger, too."));
        ImGui::Separator();
        ImGui::Text("%s", _LC("GameControls",
            "Example: Pressing CTRL+F1 will trigger COMMANDS_03 and COMMANDS_01\n"
            "but not COMMANDS_02 which has EXPL tag."));
        ImGui::TextDisabled("    COMMANDS_01    Keyboard    F1");
        ImGui::TextDisabled("    COMMANDS_02    Keyboard    EXPL+F1");
        ImGui::TextDisabled("    COMMANDS_03    Keyboard    CTRL+F1");
        ImGui::EndTooltip();
    }
}

void GameControls::DrawInteractiveButtonBinding()
{
    // Check for pressed joystick buttons
    OIS::JoyStickState* joy_state = App::GetInputEngine()->getCurrentJoyState(m_active_mapping_deviceid);
    for (size_t i = 0; i < joy_state->mButtons.size(); ++i)
    {
        if (joy_state->mButtons[i])
        {
            m_selected_evtype = eventtypes::ET_JoystickButton;
            m_active_buffer = fmt::format("{}", i);
            this->ApplyChanges();
            return;
        }
    }

    // Keys preview (aligned to center)
    std::string keys_pressed = _LC("GameControls", "Press a button");
    const float PREVIEW_YSPACING = 10.f;
    ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2((ImGui::GetContentRegionAvail().x - ImGui::CalcTextSize(keys_pressed.c_str()).x) / 2, PREVIEW_YSPACING));
    ImColor flashing_imcolor(m_flashing_color.r, m_flashing_color.g, m_flashing_color.b, m_flashing_color.a);
    ImGui::TextColored(flashing_imcolor, "%s", keys_pressed.c_str());
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + PREVIEW_YSPACING);
}

void GameControls::Draw(float dt)
{
    if (m_interactive_keybinding_active)
    {
        this->UpdateFlashingColor(dt);
        this->UpdateInteractiveKeybinding();
    }
    else
    {
        // regular window display

        ImGui::SetNextWindowPosCenter(ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(800.f, 600.f), ImGuiCond_FirstUseEver);
        bool keep_open = true;
        int flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoCollapse;
        ImGui::Begin(_LC("GameControls", "Game Controls"), &keep_open, flags);

        GUIManager::GuiTheme& theme = App::GetGuiManager()->GetTheme();

        // Toolbar
        this->DrawMenubar();
        this->DrawPreviewControls();

        // Tabs

        ImGui::BeginTabBar("GameSettingsTabs");

        this->DrawControlsTabItem("Airplane", "AIRPLANE");
        this->DrawControlsTabItem("Boat", "BOAT");
        this->DrawControlsTabItem("Camera", "CAMERA");
        this->DrawControlsTabItem("Sky", "SKY");
        this->DrawControlsTabItem("Character", "CHARACTER");
        this->DrawControlsTabItem("Commands", "COMMANDS");
        this->DrawControlsTabItem("Common", "COMMON");
        this->DrawControlsTabItem("Grass", "GRASS");
        this->DrawControlsTabItem("Map", "SURVEY_MAP");
        this->DrawControlsTabItem("Menu", "MENU");
        this->DrawControlsTabItem("Truck", "TRUCK");
        this->DrawControlsTabItem("Road editor", "ROAD_EDITOR");

        ImGui::EndTabBar(); // GameSettingsTabs

        m_is_hovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows);

        ImGui::End();
        if (!keep_open)
        {
            this->SetVisible(false);
        }
    }
}

void GameControls::DrawMenubar()
{
    if (ImGui::BeginMenuBar())
    {
        ImGui::SetNextItemWidth(400.f);
        // Select mapping file to work with
        //    - General BeginCombo() API, you have full control over your selection data and display type.
        const int combo_min = InputEngine::DEFAULT_MAPFILE_DEVICEID;
        const int combo_max = App::GetInputEngine()->getNumJoysticks();
        std::string active_label = App::GetInputEngine()->getLoadedConfigFile(m_active_mapping_deviceid);
        if (ImGui::BeginCombo(_LC("GameControls", "File (per Device)"), active_label.c_str())) // The second parameter is the label previewed before opening the combo.
        {
            for (int i = combo_min; i < combo_max; i++)
            {
                const bool is_selected = (m_active_mapping_deviceid == i);
                if (ImGui::Selectable(App::GetInputEngine()->getLoadedConfigFile(i).c_str(), is_selected))
                {
                    m_active_mapping_deviceid = i;
                }
                if (is_selected)
                {
                    ImGui::SetItemDefaultFocus();   // Set the initial focus when opening the combo (scrolling + for keyboard navigation support in the upcoming navigation branch)
                }
            }
            ImGui::EndCombo();
        }

        if (m_active_mapping_deviceid != InputEngine::BUILTIN_MAPPING_DEVICEID)
        {
            ImGui::SameLine();
            if (ImGui::SmallButton(_LC("GameControls", "Reload")))
            {
                this->ReloadMapFile();
            }
            ImGui::SameLine();
            if (ImGui::SmallButton(_LC("GameControls", "Save")))
            {
                this->SaveMapFile();
            }
        }

        // make small checkbox (no padding)
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 2.5f); // align checkbox to the text
        ImGui::Checkbox(_LC("GameControls", "Preview controls"), &m_preview_controls);
        ImGui::PopStyleVar(); // FramePadding

        ImGui::EndMenuBar();
    }
}

static const ImU32 PREVIEW_COLOR_BORDER = ImColor(145, 145, 142, 255);
static const ImU32 PREVIEW_COLOR_BORDER_SHADOW = ImColor(92, 91, 89, 255);
static const ImU32 PREVIEW_COLOR_BG = ImColor(55, 24, 69, 255);
static const ImU32 PREVIEW_COLOR_ACTIVE = ImColor(163, 38, 136, 255);
static const ImU32 PREVIEW_COLOR_MARKER = ImColor(92, 91, 89, 255);
static const float PREVIEW_LEFT_MARGIN = 13.f; // Space for the index label

void GameControls::DrawPreviewControls()
{
    if (!m_preview_controls)
    {
        return;
    }

    const float PREVIEW_YSPACING = 10.f;
    if (m_active_mapping_deviceid == InputEngine::DEFAULT_MAPFILE_DEVICEID)
    {
        // ~~keyboard~~
        Ogre::String keys_pressed;
        int num_nonmodifier_keys = App::GetInputEngine()->getCurrentKeyCombo(&keys_pressed);

        // Keys preview (aligned to center)
        
        ImGui::TextDisabled("%s:", _LC("GameControls", "Keyboard state"));
        ImGui::SameLine();
        ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2((ImGui::GetContentRegionAvail().x - ImGui::CalcTextSize(keys_pressed.c_str()).x) / 2, PREVIEW_YSPACING));        
        ImGui::Text("%s", keys_pressed.c_str());
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + PREVIEW_YSPACING);
    }
    else
    {
        // ~~controller~~
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + PREVIEW_YSPACING);

        // Draw button previews as ImGui::ProgressBar() with button ID in the name, and percentage = 0 or 100.
        OIS::JoyStickState* joy_state = App::GetInputEngine()->getCurrentJoyState(m_active_mapping_deviceid);
        ImGui::TextDisabled("%s:", _LC("GameControls", "Button states"));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.f);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.5f, 0.5f));
        ImGui::PushStyleColor(ImGuiCol_Border, PREVIEW_COLOR_BORDER);
        ImGui::PushStyleColor(ImGuiCol_BorderShadow, PREVIEW_COLOR_BORDER_SHADOW);
        ImGui::PushStyleColor(ImGuiCol_FrameBg, PREVIEW_COLOR_BG);
        ImGui::PushStyleColor(ImGuiCol_PlotHistogram, PREVIEW_COLOR_ACTIVE);
        for (size_t i = 0; i < joy_state->mButtons.size(); ++i)
        {
            // Draw each button preview here
            const ImVec2 joybutton_size(25.f, 0.f);
            ImGui::SameLine(); // all on single line
            ImGui::ProgressBar(joy_state->mButtons[i] ? 1.0f : 0.0f, joybutton_size, std::to_string(i).c_str());
        }
        ImGui::PopStyleColor(4); // Border, BorderShadow, FrameBg, PlotHistogram
        ImGui::PopStyleVar(2); // FramePadding, FrameBorderSize

        // Draw axis previews as horizontal bars filled from the center towards the current value.
        if (joy_state->mAxes.size() > 0)
        {
            ImGui::TextDisabled("%s:", _LC("GameControls", "Axis states"));
            for (size_t i = 0; i < joy_state->mAxes.size(); ++i)
            {
                ImGui::SameLine();
                this->DrawAxisPreview((int)i, joy_state->mAxes[i].abs);
            }
        }

        // Draw slider previews as pairs of axis-style bars (X and Y).
        const int num_sliders = std::min(App::GetInputEngine()->getJoyComponentCount(OIS::OIS_Slider, m_active_mapping_deviceid), (int)IM_ARRAYSIZE(joy_state->mSliders));
        if (num_sliders > 0)
        {
            ImGui::TextDisabled("%s:", _LC("GameControls", "Slider states"));
            for (int i = 0; i < num_sliders; ++i)
            {
                ImGui::SameLine();
                this->DrawSliderPreview(i, joy_state->mSliders[i].abX, joy_state->mSliders[i].abY);
            }
        }

        // Draw POV hat previews as small square widgets with an arrow pointing to the active direction.
        const int num_povs = std::min(App::GetInputEngine()->getJoyComponentCount(OIS::OIS_POV, m_active_mapping_deviceid), (int)IM_ARRAYSIZE(joy_state->mPOV));
        if (num_povs > 0)
        {
            ImGui::TextDisabled("%s:", _LC("GameControls", "POV states"));
            for (int i = 0; i < num_povs; ++i)
            {
                ImGui::SameLine();
                this->DrawPovPreview(i, joy_state->mPOV[i].direction);
            }
        }

        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + PREVIEW_YSPACING);
    }
}

// Reserves space for a preview widget and draws the framed background with index label; returns top-left corner.
static ImVec2 DrawPreviewFrame(ImVec2 size, int index)
{
    const ImVec2 top_left = ImGui::GetCursorScreenPos();
    const ImVec2 bottom_right = top_left + size;
    ImGui::Dummy(size);

    ImDrawList* drawlist = ImGui::GetWindowDrawList();
    drawlist->AddRectFilled(top_left, bottom_right, PREVIEW_COLOR_BG);
    const ImVec2 SHADOW_OFFSET(0.8f, 0.8f);
    drawlist->AddRect(top_left+SHADOW_OFFSET, bottom_right+SHADOW_OFFSET, PREVIEW_COLOR_BORDER_SHADOW);
    drawlist->AddRect(top_left, bottom_right, PREVIEW_COLOR_BORDER);
    drawlist->AddText(top_left + ImVec2(2.f, 0.f), ImColor(ImGui::GetStyle().Colors[ImGuiCol_Text]), std::to_string(index).c_str());
    return top_left;
}

// Draws a horizontal bar filled from the center towards the value (in OIS axis range), with a center tick.
static void DrawPreviewBarFill(ImVec2 track_top_left, ImVec2 track_size, float padding, int ois_value)
{
    const float track_halfwidth = track_size.x * 0.5f - padding;
    const float center_x = track_top_left.x + track_size.x * 0.5f;
    const float value = std::max(-1.f, std::min(1.f, (float)ois_value / (float)OIS::JoyStick::MAX_AXIS));
    const float value_x = center_x + value * track_halfwidth;

    ImDrawList* drawlist = ImGui::GetWindowDrawList();
    drawlist->AddRectFilled(
        ImVec2(std::min(center_x, value_x), track_top_left.y + padding),
        ImVec2(std::max(center_x, value_x), track_top_left.y + track_size.y - padding),
        PREVIEW_COLOR_ACTIVE);
    drawlist->AddLine(ImVec2(center_x, track_top_left.y + 1.f), ImVec2(center_x, track_top_left.y + track_size.y - 1.f), PREVIEW_COLOR_MARKER);
}

void GameControls::DrawAxisPreview(int axis_index, int axis_value)
{
    const float BAR_WIDTH = 80.f;
    const float BAR_HEIGHT = ImGui::GetTextLineHeight();
    const float BAR_PADDING = 3.f;

    const ImVec2 top_left = DrawPreviewFrame(ImVec2(BAR_WIDTH + PREVIEW_LEFT_MARGIN, BAR_HEIGHT), axis_index);
    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("%s %d: %d", _LC("GameControls", "Axis"), axis_index, axis_value);
    }

    DrawPreviewBarFill(top_left + ImVec2(PREVIEW_LEFT_MARGIN, 0.f), ImVec2(BAR_WIDTH, BAR_HEIGHT), BAR_PADDING, axis_value);
}

void GameControls::DrawSliderPreview(int slider_index, int slider_x, int slider_y)
{
    // Like axis preview, but with 2 bars stacked (X on top, Y on bottom).
    const float BAR_WIDTH = 80.f;
    const float BAR_HEIGHT = ImGui::GetTextLineHeight();
    const float BAR_PADDING = 3.f;

    const ImVec2 top_left = DrawPreviewFrame(ImVec2(BAR_WIDTH + PREVIEW_LEFT_MARGIN, BAR_HEIGHT * 2.f), slider_index);
    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("%s %d: X=%d, Y=%d", _LC("GameControls", "Slider"), slider_index, slider_x, slider_y);
    }

    DrawPreviewBarFill(top_left + ImVec2(PREVIEW_LEFT_MARGIN, 0.f), ImVec2(BAR_WIDTH, BAR_HEIGHT), BAR_PADDING, slider_x);
    DrawPreviewBarFill(top_left + ImVec2(PREVIEW_LEFT_MARGIN, BAR_HEIGHT), ImVec2(BAR_WIDTH, BAR_HEIGHT), BAR_PADDING, slider_y);
}

void GameControls::DrawPovPreview(int pov_index, int pov_direction)
{
    const float WIDGET_SIZE = ImGui::GetTextLineHeight() * 2.0f;

    const ImVec2 top_left = DrawPreviewFrame(ImVec2(WIDGET_SIZE + PREVIEW_LEFT_MARGIN, WIDGET_SIZE), pov_index);
    const ImVec2 center = top_left + ImVec2(PREVIEW_LEFT_MARGIN,0.f) + ImVec2(WIDGET_SIZE, WIDGET_SIZE) * 0.5f;
    const float radius = WIDGET_SIZE * 0.5f - 4.f;

    ImDrawList* drawlist = ImGui::GetWindowDrawList();

    // Markers for all 8 directions, so the layout is visible even when centered.
    for (int i = 0; i < 8; ++i)
    {
        const float angle = i * (3.14159265f / 4.f);
        drawlist->AddCircleFilled(center + ImVec2(std::cos(angle), std::sin(angle)) * radius, 1.5f, PREVIEW_COLOR_MARKER);
    }

    // Resolve direction vector (screen Y points down).
    ImVec2 dir(0.f, 0.f);
    if (pov_direction & OIS::Pov::North) { dir.y -= 1.f; }
    if (pov_direction & OIS::Pov::South) { dir.y += 1.f; }
    if (pov_direction & OIS::Pov::East)  { dir.x += 1.f; }
    if (pov_direction & OIS::Pov::West)  { dir.x -= 1.f; }
    const float dir_len = std::sqrt(dir.x * dir.x + dir.y * dir.y);

    if (dir_len > 0.f)
    {
        // Arrow from the center to the active direction
        dir = dir / dir_len;
        const ImVec2 perp(-dir.y, dir.x);
        const float ARROWHEAD_LEN = radius * 0.7f;
        const float ARROWHEAD_HALFWIDTH = radius * 0.55f;
        const ImVec2 tip = center + dir * radius;
        const ImVec2 base = center + dir * (radius - ARROWHEAD_LEN);
        drawlist->AddTriangleFilled(tip, base + perp * ARROWHEAD_HALFWIDTH, base - perp * ARROWHEAD_HALFWIDTH, PREVIEW_COLOR_ACTIVE);
    }
    else
    {
        drawlist->AddCircleFilled(center, 2.5f, PREVIEW_COLOR_BORDER);
    }
}

void GameControls::DrawEvent(RoR::events ev_code)
{
    // Var
    InputEngine::TriggerVec& triggers = App::GetInputEngine()->getEvents()[ev_code];
    GUIManager::GuiTheme& theme = App::GetGuiManager()->GetTheme();
    float cursor_x = ImGui::GetCursorPosX();

    // Set up
    ImGui::PushID((int)ev_code);

    // Name column
    ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(ev_code).c_str());

    ImGui::SameLine();
    ImGui::SetCursorPosX((cursor_x + m_colum_widths[0]) - 27); // estimate
    if (ImGui::SmallButton("+"))
    {
        App::GetInputEngine()->addEventDefault((int)ev_code, m_active_mapping_deviceid);

        // Begin interactive binding
        m_active_event = ev_code;
        event_trigger_t& trig = triggers.back();
        m_active_trigger = &trig;
        m_selected_evtype = m_active_mapping_deviceid == InputEngine::DEFAULT_MAPFILE_DEVICEID ? eventtypes::ET_Keyboard : trig.eventtype;
        m_active_buffer.Assign(App::GetInputEngine()->getEventConfig(ev_code).c_str());
        m_interactive_keybinding_active = true;
        m_interactive_keybinding_expl = trig.explicite;
        m_interactive_keybinding_delete_on_cancel = true; // <-- new trigger, so canceling should delete it.
    }
    

    ImGui::NextColumn();

    // Command column

    int num_visible_commands = 0; // Count visible commands ahead of time
    for (event_trigger_t& trig : triggers)
    {
        num_visible_commands += this->ShouldDisplay(trig);
    }

    int num_drawn_commands = 0;
    for (event_trigger_t& trig: triggers)
    {
        if (!this->ShouldDisplay(trig)) continue;

        ImGui::PushID(&trig);

        ImVec2 cursor_before_command = ImGui::GetCursorScreenPos();
        // Do a `SmallButton()` by hand so we can specify width.
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
        std::string full_command = fmt::format("({})  {}", 
            InputEngine::getEventTypeName(trig.eventtype), App::GetInputEngine()->getTriggerCommand(trig));
        if (ImGui::Button(full_command.c_str(), ImVec2(ImGui::GetColumnWidth() - 2*ImGui::GetStyle().ItemSpacing.x, 0)))
        {
            // Begin interactive binding
            m_active_event = ev_code;
            m_active_trigger = &trig;
            m_selected_evtype = m_active_mapping_deviceid == InputEngine::DEFAULT_MAPFILE_DEVICEID ? eventtypes::ET_Keyboard : trig.eventtype;
            m_active_buffer.Assign(App::GetInputEngine()->getEventConfig(ev_code).c_str());
            m_interactive_keybinding_active = true;
            m_interactive_keybinding_expl = trig.explicite;
            m_interactive_keybinding_delete_on_cancel = false; // <-- existing trigger, so canceling should not delete it.
        }

        // If there's more than 1 commands, add numbering at the left side of the buttons
        num_drawn_commands++;
        if (num_visible_commands > 1)
        {
            ImVec2 text_pos = cursor_before_command + ImGui::GetStyle().FramePadding;
            ImU32 text_color = ImColor(ImGui::GetStyle().Colors[ImGuiCol_TextDisabled]);
            ImGui::GetWindowDrawList()->AddText(text_pos, text_color, fmt::format("{}.", num_drawn_commands).c_str());
        }
        ImGui::PopStyleVar(); // FramePadding        

        ImGui::PopID(); // &trig
    }
    ImGui::NextColumn();

    // Description column
    ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(ev_code).c_str());
    ImGui::NextColumn();

    // Clean up.
    ImGui::PopID(); // ev_code
}

void GameControls::DrawEventEditBox()
{
    // Device type selector
    //    - General BeginCombo() API, you have full control over your selection data and display type.
    if (ImGui::BeginCombo(_LC("GameControls", "EventType"), InputEngine::getEventTypeName(m_selected_evtype))) // The second parameter is the label previewed before opening the combo.
    {
        for (int i = ET_Keyboard; i < ET_END; i++)
        {
            switch (i)
            {
                case ET_MouseButton:
                case ET_MouseAxisX:
                case ET_MouseAxisY:
                case ET_MouseAxisZ:
                case ET_JoystickAxisRel: // Configured as "JoystickAxis RELATIVE".
                case ET_JoystickSliderX: // X/Y is determined by special parameter.
                    continue; // Not available
                default:
                    break;
            }
            const bool is_selected = (m_selected_evtype == i);
            if (ImGui::Selectable(InputEngine::getEventTypeName((eventtypes)i), is_selected))
            {
                m_selected_evtype = (eventtypes)i;
            }
            if (is_selected)
            {
                ImGui::SetItemDefaultFocus();   // Set the initial focus when opening the combo (scrolling + for keyboard navigation support in the upcoming navigation branch)
            }
        }
        ImGui::EndCombo();
    }

    // Combo text input
    int flags = ImGuiInputTextFlags_EnterReturnsTrue;
    if (m_selected_evtype == ET_Keyboard || m_selected_evtype == ET_JoystickButton)
    {
        flags |= ImGuiInputTextFlags_CharsNoBlank;
    }
    if (m_selected_evtype != ET_JoystickPov)
    {
        flags |= ImGuiInputTextFlags_CharsUppercase; // POV options are case-sensitive.
    }
    if (ImGui::InputText("", m_active_buffer.GetBuffer(), m_active_buffer.GetCapacity(), flags))
    {
        this->ApplyChanges();
    }


}

void GameControls::DrawControlsTab(const char* prefix)
{
    ImGui::Columns(3, /*id=*/nullptr, /*border=*/true);

    for (auto& ev_pair: App::GetInputEngine()->getEvents())
    {
        // Retrieve data
        const RoR::events ev_code = RoR::events(ev_pair.first);
        const std::string ev_name = App::GetInputEngine()->eventIDToName(ev_code);

        // Filter event list by prefix
        if (ev_name.find(prefix) == 0)
        {
            this->DrawEvent(ev_code);
        }
    }

    m_colum_widths[0] = ImGui::GetColumnWidth(0);
    m_colum_widths[1] = ImGui::GetColumnWidth(1);
    m_colum_widths[2] = ImGui::GetColumnWidth(2);

    ImGui::Columns(1);
}

void GameControls::DrawControlsTabItem(const char* name, const char* prefix)
{
    if (ImGui::BeginTabItem(_LC("GameControls", name)))
    {
        ImGui::PushID(prefix);

        // Table header
        ImGui::Columns(3, /*id=*/nullptr, /*border=*/true);
        ImGui::SetColumnWidth(0, m_colum_widths[0] + ImGui::GetStyle().FramePadding.x + 1);
        ImGui::SetColumnWidth(1, m_colum_widths[1]);
        ImGui::SetColumnWidth(2, m_colum_widths[2]);
        ImGui::TextColored(GRAY_HINT_TEXT, "Name");
        ImGui::NextColumn();
        ImGui::TextColored(GRAY_HINT_TEXT, "Shortcut");
        ImGui::NextColumn();
        ImGui::TextColored(GRAY_HINT_TEXT, "Description");
        ImGui::NextColumn();
        ImGui::Separator();
        ImGui::Columns(1); // Cannot cross with child window.

        // Scroll region
        ImGui::BeginChild("scroll");

        // Actual controls table
        this->DrawControlsTab(prefix);

        // Cleanup
        ImGui::EndChild();    // "scroll"
        ImGui::PopID();       // `prefix`
        ImGui::EndTabItem();  // `name`
    }
}

void GameControls::ApplyChanges()
{
    // Validate input (the '_CharsNoBlank' flag ensures there is no whitespace)
    if (m_active_buffer.GetLength() == 0)
    {
        this->CancelChanges();
        return;
    }

    // Erase the old trigger.
    App::GetInputEngine()->eraseEvent(m_active_event, m_active_trigger);

    // Format a '.map' format line with new config
    std::string format_string;
    if (m_selected_evtype == ET_Keyboard)
    {
        format_string = "{} {} {}"; // Name, Type, Binding
    }
    else // Joystick
    {
        format_string = "{} {} 0 {}"; // Name, Type, DeviceNumber (unused), Binding
    }
    std::string line = fmt::format(format_string,
        App::GetInputEngine()->eventIDToName(m_active_event),
        InputEngine::getEventTypeName(m_selected_evtype),
        m_active_buffer.ToCStr());

    // Parse the line - this creates new trigger.
    App::GetInputEngine()->processLine(line.c_str(), m_active_mapping_deviceid);

    // Reset editing context.
    m_active_event = events::EV_MODE_LAST; // Invalid
    m_active_trigger = nullptr;
    m_active_buffer.Clear();
    m_interactive_keybinding_active = false;
    m_unsaved_changes = true;
}

void GameControls::CancelChanges()
{
    // Reset editing context.
    m_active_event = events::EV_MODE_LAST; // Invalid
    m_active_trigger = nullptr;
    m_active_buffer.Clear();
    m_interactive_keybinding_active = false;
}

void GameControls::SaveMapFile()
{
    this->CancelChanges();
    App::GetInputEngine()->saveConfigFile(m_active_mapping_deviceid);
    m_unsaved_changes = false;
}

void GameControls::ReloadMapFile()
{
    this->CancelChanges();
    App::GetInputEngine()->clearEventsByDevice(m_active_mapping_deviceid);
    App::GetInputEngine()->loadConfigFile(m_active_mapping_deviceid);
    m_unsaved_changes = false;
}

void GameControls::SetVisible(bool vis)
{
    m_is_visible = vis;
    m_is_hovered = false;
    if (!vis)
    {
        this->CancelChanges();
        App::GetGuiManager()->GameMainMenu.SetVisible(true);
    }
}

bool GameControls::ShouldDisplay(event_trigger_t& trig)
{
    // filter items by selected mapping file
    return (trig.configDeviceID == InputEngine::BUILTIN_MAPPING_DEVICEID ||
            trig.configDeviceID == m_active_mapping_deviceid);
}

void GameControls::UpdateFlashingColor(float dt)
{
    // Flashing color for the interactive keybind box
    float speed = 0.5f; // seconds for full cycle
    m_flashing_timer += dt;
    float t = fmodf(m_flashing_timer / speed, 1.f);
    float alpha = 0.5f + 0.3f * sinf(t * 2.f * 3.14);
    m_flashing_color = Ogre::ColourValue(0.95f, 0.9f, 0.7f, alpha);
}