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
#include "GUIManager.h"
#include "InputEngine.h"

#include <fmt/format.h>

using namespace RoR;
using namespace GUI;

void GameControls::Draw()
{
    ImGui::SetNextWindowPosCenter(ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(800.f, 600.f), ImGuiCond_FirstUseEver);
    bool keep_open = true;
    ImGui::Begin(_LC("GameControls", "Game Controls"), &keep_open);

    GUIManager::GuiTheme& theme = App::GetGuiManager()->GetTheme();

    this->DrawToolbar();

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

void GameControls::DrawToolbar()
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
    // Filter by selected map file
    if (m_active_mapping_file != MAPFILE_ID_ALL &&
        m_active_mapping_file != MAPFILE_ID_DEFAULT &&
        !App::GetInputEngine()->getEvents()[ev_code].empty() &&
        App::GetInputEngine()->getEvents()[ev_code][0].configDeviceID != m_active_mapping_file) // TODO: handle multiple mappings for one event code - currently we only check the first one.
    {
        return;
    }

    ImGui::PushID((int)ev_code);
    GUIManager::GuiTheme& theme = App::GetGuiManager()->GetTheme();

    // Name column
    ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(ev_code).c_str());
    ImGui::NextColumn();

    // Command column
    if (m_active_event == ev_code)
    {
        const int flags = ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CharsUppercase | ImGuiInputTextFlags_CharsNoBlank;
        if (ImGui::InputText("", m_active_buffer.GetBuffer(), m_active_buffer.GetCapacity(), flags))
        {
            this->ApplyChanges();
        }
        if (ImGui::Button(_LC("GameSettings", "OK")))
        {
            this->ApplyChanges();
        }
        ImGui::SameLine();
        if (ImGui::Button(_LC("GameSettings", "Cancel")))
        {
            this->CancelChanges();
        }
    }
    else
    {
        if (ImGui::Button(_LC("GameSettings", "Edit")))
        {
            // Begin editing
            m_active_event = ev_code;
            m_active_buffer.Assign(App::GetInputEngine()->getEventCommand(ev_code).c_str());
        }
        ImGui::SameLine();
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(ev_code).c_str());
    }
    ImGui::NextColumn();

    ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(ev_code).c_str());
    ImGui::NextColumn();

    ImGui::PopID(); // ev_code
}

void GameControls::DrawControlsTab(const char* prefix)
{
    ImGui::Columns(3, /*id=*/nullptr, /*border=*/true);

    for (auto& ev_pair: App::GetInputEngine()->getEvents())
    {
        // Retrieve data
        const RoR::events ev_code = RoR::events(ev_pair.first);
        const std::string ev_name = App::GetInputEngine()->eventIDToName(ev_code);

        // Filter event list by Keyboard + prefix
        if (ev_pair.second.size() > 0 &&
            ev_pair.second[0].eventtype == eventtypes::ET_Keyboard &&
            ev_name.find(prefix) == 0)
        {
            this->DrawEvent(ev_code);
        }
    }

    ImGui::Columns(1);
}

void GameControls::DrawControlsTabItem(const char* name, const char* prefix)
{
    if (ImGui::BeginTabItem(_LC("GameSettings", name)))
    {
        ImGui::PushID(prefix);

        // Table header
        ImGui::Columns(3, /*id=*/nullptr, /*border=*/true);
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

    // Erase all previous configurations
    App::GetInputEngine()->clearEvents(m_active_event);

    // Format a '.map' format line with new config
    std::string line = fmt::format("{} Keyboard {}",
        App::GetInputEngine()->eventIDToName(m_active_event),
        m_active_buffer.ToCStr());

    // Parse the line - this updates live settings.
    App::GetInputEngine()->processLine(line.c_str());

    // Reset editing context.
    m_active_event = events::EV_MODE_LAST; // Invalid
    m_active_buffer.Clear();
}

void GameControls::CancelChanges()
{
    // Reset editing context.
    m_active_event = events::EV_MODE_LAST; // Invalid
    m_active_buffer.Clear();
}

void GameControls::SaveMapFile()
{
    if (m_active_mapping_file > MAPFILE_ID_ALL)
    {
        App::GetInputEngine()->saveConfigFile(m_active_mapping_file);
    }
}

void GameControls::ReloadMapFile()
{
    if (m_active_mapping_file > MAPFILE_ID_ALL)
    {
        App::GetInputEngine()->clearEventsByDevice(m_active_mapping_file);
        App::GetInputEngine()->loadConfigFile(m_active_mapping_file);
    }
}

void GameControls::SetVisible(bool vis)
{
    m_is_visible = vis;
    if (!vis)
    {
        this->CancelChanges();
        App::GetGuiManager()->SetVisible_GameMainMenu(true);
    }
}

std::string const& GameControls::GetFileComboLabel(int file_id)
{
    if (file_id == MAPFILE_ID_ALL)
        return MAPFILE_COMBO_ALL;
    else
        return App::GetInputEngine()->getLoadedConfigFile(file_id); // handles -1
}
