/*
    This source file is part of Rigs of Rods
    Copyright 2020 tritonas00

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
            m_active_buffer = keys_pressed;
            this->ApplyChanges();
        }
        else
        {
            ImGui::SetNextWindowPosCenter();
            ImGuiWindowFlags flags = ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoCollapse |
                ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysAutoResize;
            ImGui::Begin(_LC("GameControls", "Press a new key"), nullptr, flags);
            ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(m_active_event).c_str());
            ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(m_active_event).c_str());
            ImGui::NewLine();
            ImGui::Text(keys_pressed.c_str());
            ImGui::End();
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

        if (m_expert_mode)
        {
            this->DrawExpertToolbar();
        }
        else
        {
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
        }

        // right-aligned "Expert mode" checkbox
        if (!m_active_trigger) // do not display when editing in progress
        {
            ImGui::SameLine();
            std::string expertmode_text = _LC("GameControls", "Expert mode");
            ImGui::SetCursorPosX(ImGui::GetWindowContentRegionMax().x - ImGui::CalcTextSize(expertmode_text.c_str()).x - 35); // estimate
            if (ImGui::Checkbox(expertmode_text.c_str(), &m_expert_mode))
            {
                if (!m_expert_mode)
                    m_active_mapping_file = MAPFILE_ID_DEFAULT;
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

        ImGui::EndTabBar(); // GameSettingsTabs

        ImGui::End();
        if (!keep_open)
        {
            this->SetVisible(false);
        }
    }
}

void GameControls::DrawExpertToolbar()
{
    // Select mapping file to work with
    //    - General BeginCombo() API, you have full control over your selection data and display type.
    const int combo_min = MAPFILE_ID_ALL; // -2
    const int combo_max = App::GetInputEngine()->getNumJoysticks();

    if (ImGui::BeginCombo(_LC("GameSettings", "File"), this->GetFileComboLabel(m_active_mapping_file).c_str())) // The second parameter is the label previewed before opening the combo.
    {
        for (int i = combo_min; i < combo_max; i++)
        {
            const bool is_selected = (m_active_mapping_file == i);
            if (ImGui::Selectable(this->GetFileComboLabel(i).c_str(), is_selected))
            {
                m_active_mapping_file = i;
            }
            if (is_selected)
            {
                ImGui::SetItemDefaultFocus();   // Set the initial focus when opening the combo (scrolling + for keyboard navigation support in the upcoming navigation branch)
            }
        }
        ImGui::EndCombo();
    }

    if (m_active_mapping_file != MAPFILE_ID_ALL)
    {
        ImGui::SameLine();
        if (ImGui::Button(_LC("GameControls", "Reload")))
        {
            this->ReloadMapFile();
        }
        ImGui::SameLine();
        if (ImGui::Button(_LC("GameControls", "Save")))
        {
            this->SaveMapFile();
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
    for (event_trigger_t& trig: triggers)
    {
        display_count += (int)this->ShouldDisplay(trig);
    }
    if (display_count == 0 && !m_expert_mode)
    {
        return;
    }

    // Set up
    ImGui::PushID((int)ev_code);

    // Name column
    ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(ev_code).c_str());
    if (m_expert_mode)
    {
        ImGui::SameLine();
        ImGui::SetCursorPosX((cursor_x + m_colum_widths[0]) - 27); // estimate
        if (ImGui::Button("+"))
        {
            App::GetInputEngine()->addEventDefault((int)ev_code, m_active_mapping_file);
        }
    }

    ImGui::NextColumn();

    // Command column

    for (event_trigger_t& trig: triggers)
    {
        if (!this->ShouldDisplay(trig)) continue;

        ImGui::PushID(&trig);

        if (m_expert_mode)
        {
            if (m_active_trigger == &trig)
            {
                this->DrawEventEditBox();
            }
            else
            {
                if (ImGui::Button(_LC("GameSettings", "Edit")))
                {
                    // Begin editing
                    m_active_event = ev_code;
                    m_active_trigger = &trig;
                    m_selected_evtype = trig.eventtype;
                    m_active_buffer.Assign(App::GetInputEngine()->getEventConfig(ev_code).c_str());
                }
                ImGui::SameLine();
                ImGui::TextColored(theme.success_text_color, "%s",
                    InputEngine::getEventTypeName(App::GetInputEngine()->getEvents()[ev_code][0].eventtype));
                ImGui::SameLine();
                ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(ev_code).c_str());
            }
        }
        else // simple mode
        {
            if (ImGui::Button(App::GetInputEngine()->getEventCommand(ev_code).c_str()))
            {
                // Begin interactive keybind
                m_active_event = ev_code;
                m_active_trigger = &trig;
                m_selected_evtype = eventtypes::ET_Keyboard;
                m_active_buffer.Clear();
                m_interactive_keybinding_active = true;
            }
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
    if (m_active_mapping_file > MAPFILE_ID_ALL)
    {
        App::GetInputEngine()->saveConfigFile(m_active_mapping_file);
    }
    m_unsaved_changes = false;
}

void GameControls::ReloadMapFile()
{
    this->CancelChanges();
    if (m_active_mapping_file > MAPFILE_ID_ALL)
    {
        App::GetInputEngine()->clearEventsByDevice(m_active_mapping_file);
        App::GetInputEngine()->loadConfigFile(m_active_mapping_file);
    }
    m_unsaved_changes = false;
}

void GameControls::SetVisible(bool vis)
{
    m_is_visible = vis;
    if (vis)
    {
        m_text_all_active = _LC("GameSettings", "<All active values>");
    }
    else
    {
        this->CancelChanges();
        App::GetGuiManager()->SetVisible_GameMainMenu(true);
    }
}

std::string const& GameControls::GetFileComboLabel(int file_id)
{
    if (file_id == MAPFILE_ID_ALL)
        return m_text_all_active;
    else
        return App::GetInputEngine()->getLoadedConfigFile(file_id); // handles -1
}

bool GameControls::ShouldDisplay(event_trigger_t& trig)
{
    if (m_expert_mode)
    {
        // filter items by selected mapping file
        return (m_active_mapping_file == MAPFILE_ID_ALL ||
                m_active_mapping_file == trig.configDeviceID);
    }
    else
    {
        // display only keyboard items from "input.map" or defaults
        return trig.eventtype == eventtypes::ET_Keyboard &&
               (trig.configDeviceID == MAPFILE_ID_DEFAULT || // input.map
               trig.configDeviceID == MAPFILE_ID_ALL);       // builtin defaults
    }
}

