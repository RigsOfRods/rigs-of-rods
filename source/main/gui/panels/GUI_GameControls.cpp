/*
    This source file is part of Rigs of Rods
    Copyright 2020 tritonas00
    Copyright 2021 Petr Ohlidal

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

#include "Actor.h"
#include "Application.h"
#include "Language.h"
#include "OgreImGui.h"
#include "GUI_LoadingWindow.h"
#include "GUIManager.h"
#include "InputEngine.h"

#include <fmt/format.h>

using namespace RoR;
using namespace GUI;

void GameControls::Draw()
{
    if (m_interactive_keybinding_active)
    {
        // Interactive keybind mode - controls window remains 'visible', but box is drawn instead.

        GUIManager::GuiTheme& theme = App::GetGuiManager()->GetTheme();
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
            App::GetInputEngine()->resetKeys(); // Do not leak the pressed keys to gameplay.
        }
        else
        {
            ImGui::SetNextWindowPosCenter();
            ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysAutoResize;
            bool open = true;
            ImGui::Begin(_LC("GameControls", "Press a new key"), &open, flags);
            // Title and description
            ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(m_active_event).c_str());
            ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(m_active_event).c_str());
            // Keys preview
            ImGui::NewLine();
            ImGui::Text(keys_pressed.c_str());
            // EXPL checkbox + tooltip
            ImGui::NewLine();
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
            ImGui::End();
            if (!open)
            {
                this->CancelChanges();
            }
        }
    }
    else
    {
        // regular window display

        ImGui::SetNextWindowPosCenter(ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(800.f, 600.f), ImGuiCond_FirstUseEver);
        bool keep_open = true;
        ImGui::Begin(_LC("GameControls", "Game Controls"), &keep_open);

        GUIManager::GuiTheme& theme = App::GetGuiManager()->GetTheme();

        // Toolbar

        if (m_unsaved_changes)
        {
            if (ImGui::Button(_LC("GameControls", "Save changes")))
            {
                this->SaveMapFile();
            }
            ImGui::SameLine();
            if (ImGui::Button(_LC("GameControls", "Reset changes")))
            {
                this->ReloadMapFile();
            }
        }

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

void GameControls::DrawEvent(RoR::events ev_code)
{
    // Var
    InputEngine::TriggerVec& triggers = App::GetInputEngine()->getEvents()[ev_code];
    GUIManager::GuiTheme& theme = App::GetGuiManager()->GetTheme();
    float cursor_x = ImGui::GetCursorPosX();

    // Check if we have anything to show
    int display_count = 0;
    for (event_trigger_t& trig : triggers)
    {
        display_count += (int)this->ShouldDisplay(trig);
    }
    if (display_count == 0)
    {
        return;
    }

    // Set up
    ImGui::PushID((int)ev_code);

    // Name column
    ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(ev_code).c_str());

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

        if (ImGui::Button(App::GetInputEngine()->getTriggerCommand(trig).c_str(), ImVec2(ImGui::GetColumnWidth() - 2*ImGui::GetStyle().ItemSpacing.x, 0)))
        {
            // Begin interactive keybind
            m_active_event = ev_code;
            m_active_trigger = &trig;
            m_selected_evtype = eventtypes::ET_Keyboard;
            m_active_buffer.Clear();
            m_interactive_keybinding_active = true;
            m_interactive_keybinding_expl = trig.explicite;
        }

        // If there's more than 1 commands, add numbering at the left side of the buttons
        num_drawn_commands++;
        if (num_visible_commands > 1)
        {
            ImVec2 text_pos = cursor_before_command + ImGui::GetStyle().FramePadding;
            ImU32 text_color = ImColor(ImGui::GetStyle().Colors[ImGuiCol_TextDisabled]);
            ImGui::GetWindowDrawList()->AddText(text_pos, text_color, fmt::format("{}.", num_drawn_commands).c_str());
        }

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
    if (ImGui::BeginCombo(_LC("GameSettings", "EventType"), InputEngine::getEventTypeName(m_selected_evtype))) // The second parameter is the label previewed before opening the combo.
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

    // Buttons
    if (ImGui::Button(_LC("GameSettings", "OK")))
    {
        this->ApplyChanges();
    }
    ImGui::SameLine();
    if (ImGui::Button(_LC("GameSettings", "Cancel")))
    {
        this->CancelChanges();
    }
    ImGui::SameLine();
    if (ImGui::Button(_LC("GameSettings", "Delete")))
    {
        App::GetInputEngine()->eraseEvent(m_active_event, m_active_trigger);
        this->CancelChanges();
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
    if (ImGui::BeginTabItem(_LC("GameSettings", name)))
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
    App::GetInputEngine()->processLine(line.c_str(), m_active_mapping_file);

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
    App::GetInputEngine()->saveConfigFile(m_active_mapping_file);
    m_unsaved_changes = false;
}

void GameControls::ReloadMapFile()
{
    this->CancelChanges();
    App::GetInputEngine()->clearEventsByDevice(m_active_mapping_file);
    App::GetInputEngine()->loadConfigFile(m_active_mapping_file);
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
    // display only keyboard items from "input.map" or defaults
    return trig.eventtype == eventtypes::ET_Keyboard &&
            (trig.configDeviceID == InputEngine::DEFAULT_MAPFILE_DEVICEID ||
                (trig.configDeviceID == InputEngine::BUILTIN_MAPPING_DEVICEID));
}

