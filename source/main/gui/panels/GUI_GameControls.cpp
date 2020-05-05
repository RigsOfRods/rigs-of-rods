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

namespace RoR {
namespace GUI {

void GameControls::Draw()
{
    ImGui::SetNextWindowPosCenter(ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(800.f, 600.f), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin(_L("Game Controls"), &m_is_visible))
    {
        ImGui::End(); // The window is collapsed
        return;
    }

    ImVec4 GRAY_HINT_TEXT = ImVec4(0.62f, 0.62f, 0.61f, 1.f);
    GUIManager::GuiTheme& theme = App::GetGuiManager()->GetTheme();

    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4.f, 8.f));

    if (ImGui::Button("Airplane")) { m_tab = ControlsTab::AIRPLANE; }
    ImGui::SameLine();
    if (ImGui::Button("Boat")) { m_tab = ControlsTab::BOAT; }
    ImGui::SameLine();
    if (ImGui::Button("Camera")) { m_tab = ControlsTab::CAMERA; }
    ImGui::SameLine();
    if (ImGui::Button("Sky")) { m_tab = ControlsTab::SKY; }
    ImGui::SameLine();
    if (ImGui::Button("Character")) { m_tab = ControlsTab::CHARACTER; }
    ImGui::SameLine();
    if (ImGui::Button("Commands")) { m_tab = ControlsTab::COMMANDS; }
    ImGui::SameLine();
    if (ImGui::Button("Common")) { m_tab = ControlsTab::COMMON; }
    ImGui::SameLine();
    if (ImGui::Button("Grass")) { m_tab = ControlsTab::GRASS; }
    ImGui::SameLine();
    if (ImGui::Button("Map")) { m_tab = ControlsTab::MAP; }
    ImGui::SameLine();
    if (ImGui::Button("Menu")) { m_tab = ControlsTab::MENU; }
    ImGui::SameLine();
    if (ImGui::Button("Truck")) { m_tab = ControlsTab::TRUCK; }

    ImGui::PopStyleVar(1);

    ImGui::Columns(3, /*id=*/nullptr, /*border=*/true);
    ImGui::TextColored(GRAY_HINT_TEXT, "Name");
    ImGui::NextColumn();
    ImGui::TextColored(GRAY_HINT_TEXT, "Shortcut");
    ImGui::NextColumn();
    ImGui::TextColored(GRAY_HINT_TEXT, "Description");
    ImGui::Columns(1);

    if (m_tab == ControlsTab::AIRPLANE)
    {
        ImGui::Columns(3, /*id=*/nullptr, /*border=*/true);
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_AIRPLANE_AIRBRAKES_FULL).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_AIRPLANE_AIRBRAKES_LESS).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_AIRPLANE_AIRBRAKES_MORE).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_AIRPLANE_AIRBRAKES_NONE).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_AIRPLANE_BRAKE).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_AIRPLANE_ELEVATOR_DOWN).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_AIRPLANE_ELEVATOR_UP).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_AIRPLANE_FLAPS_FULL).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_AIRPLANE_FLAPS_LESS).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_AIRPLANE_FLAPS_MORE).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_AIRPLANE_FLAPS_NONE).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_AIRPLANE_PARKING_BRAKE).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_AIRPLANE_REVERSE).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_AIRPLANE_RUDDER_LEFT).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_AIRPLANE_RUDDER_RIGHT).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_AIRPLANE_STEER_LEFT).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_AIRPLANE_STEER_RIGHT).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_AIRPLANE_THROTTLE_AXIS).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_AIRPLANE_THROTTLE_DOWN).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_AIRPLANE_THROTTLE_FULL).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_AIRPLANE_THROTTLE_NO).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_AIRPLANE_THROTTLE_UP).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_AIRPLANE_TOGGLE_ENGINES).c_str());
        ImGui::NextColumn();
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_AIRPLANE_AIRBRAKES_FULL).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_AIRPLANE_AIRBRAKES_LESS).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_AIRPLANE_AIRBRAKES_MORE).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_AIRPLANE_AIRBRAKES_NONE).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_AIRPLANE_BRAKE).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_AIRPLANE_ELEVATOR_DOWN).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_AIRPLANE_ELEVATOR_UP).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_AIRPLANE_FLAPS_FULL).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_AIRPLANE_FLAPS_LESS).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_AIRPLANE_FLAPS_MORE).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_AIRPLANE_FLAPS_NONE).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_AIRPLANE_PARKING_BRAKE).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_AIRPLANE_REVERSE).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_AIRPLANE_RUDDER_LEFT).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_AIRPLANE_RUDDER_RIGHT).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_AIRPLANE_STEER_LEFT).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_AIRPLANE_STEER_RIGHT).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_AIRPLANE_THROTTLE_AXIS).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_AIRPLANE_THROTTLE_DOWN).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_AIRPLANE_THROTTLE_FULL).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_AIRPLANE_THROTTLE_NO).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_AIRPLANE_THROTTLE_UP).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_AIRPLANE_TOGGLE_ENGINES).c_str());
        ImGui::NextColumn();
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_AIRPLANE_AIRBRAKES_FULL).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_AIRPLANE_AIRBRAKES_LESS).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_AIRPLANE_AIRBRAKES_MORE).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_AIRPLANE_AIRBRAKES_NONE).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_AIRPLANE_BRAKE).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_AIRPLANE_ELEVATOR_DOWN).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_AIRPLANE_ELEVATOR_UP).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_AIRPLANE_FLAPS_FULL).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_AIRPLANE_FLAPS_LESS).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_AIRPLANE_FLAPS_MORE).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_AIRPLANE_FLAPS_NONE).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_AIRPLANE_PARKING_BRAKE).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_AIRPLANE_REVERSE).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_AIRPLANE_RUDDER_LEFT).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_AIRPLANE_RUDDER_RIGHT).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_AIRPLANE_STEER_LEFT).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_AIRPLANE_STEER_RIGHT).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_AIRPLANE_THROTTLE_AXIS).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_AIRPLANE_THROTTLE_DOWN).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_AIRPLANE_THROTTLE_FULL).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_AIRPLANE_THROTTLE_NO).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_AIRPLANE_THROTTLE_UP).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_AIRPLANE_TOGGLE_ENGINES).c_str());
    }
    else if (m_tab == ControlsTab::BOAT)
    {
        ImGui::Columns(3, /*id=*/nullptr, /*border=*/true);
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_BOAT_CENTER_RUDDER).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_BOAT_REVERSE).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_BOAT_STEER_LEFT).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_BOAT_STEER_RIGHT).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_BOAT_THROTTLE_AXIS).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_BOAT_THROTTLE_UP).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_BOAT_THROTTLE_DOWN).c_str());
        ImGui::NextColumn();
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_BOAT_CENTER_RUDDER).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_BOAT_REVERSE).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_BOAT_STEER_LEFT).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_BOAT_STEER_RIGHT).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_BOAT_THROTTLE_AXIS).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_BOAT_THROTTLE_UP).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_BOAT_THROTTLE_DOWN).c_str());
        ImGui::NextColumn();
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_BOAT_CENTER_RUDDER).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_BOAT_REVERSE).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_BOAT_STEER_LEFT).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_BOAT_STEER_RIGHT).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_BOAT_THROTTLE_AXIS).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_BOAT_THROTTLE_UP).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_BOAT_THROTTLE_DOWN).c_str());
    }
    else if (m_tab == ControlsTab::CAMERA)
    {
        ImGui::Columns(3, /*id=*/nullptr, /*border=*/true);
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_CAMERA_CHANGE).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_CAMERA_FREE_MODE).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_CAMERA_FREE_MODE_FIX).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_CAMERA_LOOKBACK).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_CAMERA_RESET).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_CAMERA_UP).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_CAMERA_DOWN).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_CAMERA_ROTATE_DOWN).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_CAMERA_ROTATE_LEFT).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_CAMERA_ROTATE_RIGHT).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_CAMERA_ROTATE_UP).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_CAMERA_ZOOM_IN).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_CAMERA_ZOOM_IN_FAST).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_CAMERA_ZOOM_OUT).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_CAMERA_ZOOM_OUT_FAST).c_str());
        ImGui::NextColumn();
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_CAMERA_CHANGE).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_CAMERA_FREE_MODE).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_CAMERA_FREE_MODE_FIX).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_CAMERA_LOOKBACK).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_CAMERA_RESET).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_CAMERA_UP).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_CAMERA_DOWN).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_CAMERA_ROTATE_DOWN).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_CAMERA_ROTATE_LEFT).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_CAMERA_ROTATE_RIGHT).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_CAMERA_ROTATE_UP).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_CAMERA_ZOOM_IN).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_CAMERA_ZOOM_IN_FAST).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_CAMERA_ZOOM_OUT).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_CAMERA_ZOOM_OUT_FAST).c_str());
        ImGui::NextColumn();
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_CAMERA_CHANGE).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_CAMERA_FREE_MODE).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_CAMERA_FREE_MODE_FIX).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_CAMERA_LOOKBACK).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_CAMERA_RESET).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_CAMERA_UP).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_CAMERA_DOWN).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_CAMERA_ROTATE_DOWN).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_CAMERA_ROTATE_LEFT).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_CAMERA_ROTATE_RIGHT).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_CAMERA_ROTATE_UP).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_CAMERA_ZOOM_IN).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_CAMERA_ZOOM_IN_FAST).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_CAMERA_ZOOM_OUT).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_CAMERA_ZOOM_OUT_FAST).c_str());
    }
    else if (m_tab == ControlsTab::SKY)
    {
        ImGui::Columns(3, /*id=*/nullptr, /*border=*/true);
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_SKY_DECREASE_TIME).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_SKY_DECREASE_TIME_FAST).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_SKY_INCREASE_TIME).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_SKY_INCREASE_TIME_FAST).c_str());
        ImGui::NextColumn();
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_SKY_DECREASE_TIME).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_SKY_DECREASE_TIME_FAST).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_SKY_INCREASE_TIME).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_SKY_INCREASE_TIME_FAST).c_str());
        ImGui::NextColumn();
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_SKY_DECREASE_TIME).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_SKY_DECREASE_TIME_FAST).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_SKY_INCREASE_TIME).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_SKY_INCREASE_TIME_FAST).c_str());
    }
    else if (m_tab == ControlsTab::CHARACTER)
    {
        ImGui::Columns(3, /*id=*/nullptr, /*border=*/true);
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_CHARACTER_BACKWARDS).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_CHARACTER_FORWARD).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_CHARACTER_JUMP).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_CHARACTER_LEFT).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_CHARACTER_RIGHT).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_CHARACTER_ROT_DOWN).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_CHARACTER_ROT_UP).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_CHARACTER_RUN).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_CHARACTER_SIDESTEP_LEFT).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_CHARACTER_SIDESTEP_RIGHT).c_str());
        ImGui::NextColumn();
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_CHARACTER_BACKWARDS).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_CHARACTER_FORWARD).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_CHARACTER_JUMP).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_CHARACTER_LEFT).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_CHARACTER_RIGHT).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_CHARACTER_ROT_DOWN).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_CHARACTER_ROT_UP).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_CHARACTER_RUN).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_CHARACTER_SIDESTEP_LEFT).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_CHARACTER_SIDESTEP_RIGHT).c_str());
        ImGui::NextColumn();
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_CHARACTER_BACKWARDS).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_CHARACTER_FORWARD).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_CHARACTER_JUMP).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_CHARACTER_LEFT).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_CHARACTER_RIGHT).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_CHARACTER_ROT_DOWN).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_CHARACTER_ROT_UP).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_CHARACTER_RUN).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_CHARACTER_SIDESTEP_LEFT).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_CHARACTER_SIDESTEP_RIGHT).c_str());
    }    
    else if (m_tab == ControlsTab::COMMANDS)
    {
        ImGui::Columns(3, /*id=*/nullptr, /*border=*/true);
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_COMMANDS_01).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_COMMANDS_02).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_COMMANDS_03).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_COMMANDS_04).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_COMMANDS_05).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_COMMANDS_06).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_COMMANDS_07).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_COMMANDS_08).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_COMMANDS_09).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_COMMANDS_10).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_COMMANDS_11).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_COMMANDS_12).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_COMMANDS_13).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_COMMANDS_14).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_COMMANDS_15).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_COMMANDS_16).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_COMMANDS_17).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_COMMANDS_18).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_COMMANDS_19).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_COMMANDS_20).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_COMMANDS_21).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_COMMANDS_22).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_COMMANDS_23).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_COMMANDS_24).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_COMMANDS_25).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_COMMANDS_26).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_COMMANDS_27).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_COMMANDS_28).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_COMMANDS_29).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_COMMANDS_30).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_COMMANDS_31).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_COMMANDS_32).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_COMMANDS_33).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_COMMANDS_34).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_COMMANDS_35).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_COMMANDS_36).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_COMMANDS_37).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_COMMANDS_38).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_COMMANDS_39).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_COMMANDS_40).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_COMMANDS_41).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_COMMANDS_42).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_COMMANDS_43).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_COMMANDS_44).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_COMMANDS_45).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_COMMANDS_46).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_COMMANDS_47).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_COMMANDS_48).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_COMMANDS_49).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_COMMANDS_50).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_COMMANDS_51).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_COMMANDS_52).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_COMMANDS_53).c_str());    
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_COMMANDS_54).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_COMMANDS_55).c_str());    
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_COMMANDS_56).c_str());    
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_COMMANDS_57).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_COMMANDS_58).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_COMMANDS_59).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_COMMANDS_60).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_COMMANDS_61).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_COMMANDS_62).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_COMMANDS_63).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_COMMANDS_64).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_COMMANDS_65).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_COMMANDS_66).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_COMMANDS_67).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_COMMANDS_68).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_COMMANDS_69).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_COMMANDS_70).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_COMMANDS_71).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_COMMANDS_72).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_COMMANDS_73).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_COMMANDS_74).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_COMMANDS_75).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_COMMANDS_76).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_COMMANDS_77).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_COMMANDS_78).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_COMMANDS_79).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_COMMANDS_80).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_COMMANDS_81).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_COMMANDS_82).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_COMMANDS_83).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_COMMANDS_84).c_str());
        ImGui::NextColumn();
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_COMMANDS_01).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_COMMANDS_02).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_COMMANDS_03).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_COMMANDS_04).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_COMMANDS_05).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_COMMANDS_06).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_COMMANDS_07).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_COMMANDS_08).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_COMMANDS_09).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_COMMANDS_10).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_COMMANDS_11).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_COMMANDS_12).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_COMMANDS_13).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_COMMANDS_14).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_COMMANDS_15).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_COMMANDS_16).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_COMMANDS_17).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_COMMANDS_18).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_COMMANDS_19).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_COMMANDS_20).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_COMMANDS_21).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_COMMANDS_22).c_str());    
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_COMMANDS_23).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_COMMANDS_24).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_COMMANDS_25).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_COMMANDS_26).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_COMMANDS_27).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_COMMANDS_28).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_COMMANDS_29).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_COMMANDS_30).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_COMMANDS_31).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_COMMANDS_32).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_COMMANDS_33).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_COMMANDS_34).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_COMMANDS_35).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_COMMANDS_36).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_COMMANDS_37).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_COMMANDS_38).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_COMMANDS_39).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_COMMANDS_40).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_COMMANDS_41).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_COMMANDS_42).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_COMMANDS_43).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_COMMANDS_44).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_COMMANDS_45).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_COMMANDS_46).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_COMMANDS_47).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_COMMANDS_48).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_COMMANDS_49).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_COMMANDS_50).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_COMMANDS_51).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_COMMANDS_52).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_COMMANDS_53).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_COMMANDS_54).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_COMMANDS_55).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_COMMANDS_56).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_COMMANDS_57).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_COMMANDS_58).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_COMMANDS_59).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_COMMANDS_60).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_COMMANDS_61).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_COMMANDS_62).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_COMMANDS_63).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_COMMANDS_64).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_COMMANDS_65).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_COMMANDS_66).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_COMMANDS_67).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_COMMANDS_68).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_COMMANDS_69).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_COMMANDS_70).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_COMMANDS_71).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_COMMANDS_72).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_COMMANDS_73).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_COMMANDS_74).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_COMMANDS_75).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_COMMANDS_76).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_COMMANDS_77).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_COMMANDS_78).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_COMMANDS_79).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_COMMANDS_80).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_COMMANDS_81).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_COMMANDS_82).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_COMMANDS_83).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_COMMANDS_84).c_str());
        ImGui::NextColumn();
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_COMMANDS_01).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_COMMANDS_02).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_COMMANDS_03).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_COMMANDS_04).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_COMMANDS_05).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_COMMANDS_06).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_COMMANDS_07).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_COMMANDS_08).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_COMMANDS_09).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_COMMANDS_10).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_COMMANDS_11).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_COMMANDS_12).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_COMMANDS_13).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_COMMANDS_14).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_COMMANDS_15).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_COMMANDS_16).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_COMMANDS_17).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_COMMANDS_18).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_COMMANDS_19).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_COMMANDS_20).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_COMMANDS_21).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_COMMANDS_22).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_COMMANDS_23).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_COMMANDS_24).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_COMMANDS_25).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_COMMANDS_26).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_COMMANDS_27).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_COMMANDS_28).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_COMMANDS_29).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_COMMANDS_30).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_COMMANDS_31).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_COMMANDS_32).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_COMMANDS_33).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_COMMANDS_34).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_COMMANDS_35).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_COMMANDS_36).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_COMMANDS_37).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_COMMANDS_38).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_COMMANDS_39).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_COMMANDS_40).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_COMMANDS_41).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_COMMANDS_42).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_COMMANDS_43).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_COMMANDS_44).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_COMMANDS_45).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_COMMANDS_46).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_COMMANDS_47).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_COMMANDS_48).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_COMMANDS_49).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_COMMANDS_50).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_COMMANDS_51).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_COMMANDS_52).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_COMMANDS_53).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_COMMANDS_54).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_COMMANDS_55).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_COMMANDS_56).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_COMMANDS_57).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_COMMANDS_58).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_COMMANDS_59).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_COMMANDS_60).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_COMMANDS_61).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_COMMANDS_62).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_COMMANDS_63).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_COMMANDS_64).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_COMMANDS_65).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_COMMANDS_66).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_COMMANDS_67).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_COMMANDS_68).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_COMMANDS_69).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_COMMANDS_70).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_COMMANDS_71).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_COMMANDS_72).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_COMMANDS_73).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_COMMANDS_74).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_COMMANDS_75).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_COMMANDS_76).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_COMMANDS_77).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_COMMANDS_78).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_COMMANDS_79).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_COMMANDS_80).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_COMMANDS_81).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_COMMANDS_82).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_COMMANDS_83).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_COMMANDS_84).c_str());
    }
    else if (m_tab == ControlsTab::COMMON)
    {
        ImGui::Columns(3, /*id=*/nullptr, /*border=*/true);
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_COMMON_ACCELERATE_SIMULATION).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_COMMON_DECELERATE_SIMULATION).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_COMMON_RESET_SIMULATION_PACE).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_COMMON_CONSOLE_TOGGLE).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_COMMON_ENTER_OR_EXIT_TRUCK).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_COMMON_ENTER_NEXT_TRUCK).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_COMMON_ENTER_PREVIOUS_TRUCK).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_COMMON_REMOVE_CURRENT_TRUCK).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_COMMON_RESPAWN_LAST_TRUCK).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_COMMON_FULLSCREEN_TOGGLE).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_COMMON_HIDE_GUI).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_COMMON_TOGGLE_DASHBOARD).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_COMMON_LOCK).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_COMMON_AUTOLOCK).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_COMMON_ROPELOCK).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_COMMON_OUTPUT_POSITION).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_COMMON_GET_NEW_VEHICLE).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_COMMON_PRESSURE_LESS).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_COMMON_PRESSURE_MORE).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_COMMON_QUICKLOAD).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_COMMON_QUICKSAVE).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_COMMON_QUIT_GAME).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_COMMON_REPLAY_BACKWARD).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_COMMON_REPLAY_FAST_BACKWARD).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_COMMON_REPLAY_FAST_FORWARD).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_COMMON_REPLAY_FORWARD).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_COMMON_RESCUE_TRUCK).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_COMMON_RESET_TRUCK).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_COMMON_TOGGLE_RESET_MODE).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_COMMON_SCREENSHOT).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_COMMON_SECURE_LOAD).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_COMMON_TOGGLE_DEBUG_VIEW).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_COMMON_TOGGLE_TERRAIN_EDITOR).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_COMMON_TOGGLE_CUSTOM_PARTICLES).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_COMMON_TOGGLE_MAT_DEBUG).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_COMMON_TOGGLE_RENDER_MODE).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_COMMON_TOGGLE_REPLAY_MODE).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_COMMON_TOGGLE_PHYSICS).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_COMMON_TOGGLE_STATS).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_COMMON_TOGGLE_TRUCK_BEACONS).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_COMMON_TOGGLE_TRUCK_LIGHTS).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_COMMON_TRUCK_INFO).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_COMMON_TRUCK_DESCRIPTION).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_COMMON_FOV_LESS).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_COMMON_FOV_MORE).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_COMMON_FOV_RESET).c_str());
        ImGui::NextColumn();
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_COMMON_ACCELERATE_SIMULATION).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_COMMON_DECELERATE_SIMULATION).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_COMMON_RESET_SIMULATION_PACE).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_COMMON_CONSOLE_TOGGLE).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_COMMON_ENTER_OR_EXIT_TRUCK).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_COMMON_ENTER_NEXT_TRUCK).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_COMMON_ENTER_PREVIOUS_TRUCK).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_COMMON_REMOVE_CURRENT_TRUCK).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_COMMON_RESPAWN_LAST_TRUCK).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_COMMON_FULLSCREEN_TOGGLE).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_COMMON_HIDE_GUI).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_COMMON_TOGGLE_DASHBOARD).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_COMMON_LOCK).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_COMMON_AUTOLOCK).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_COMMON_ROPELOCK).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_COMMON_OUTPUT_POSITION).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_COMMON_GET_NEW_VEHICLE).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_COMMON_PRESSURE_LESS).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_COMMON_PRESSURE_MORE).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_COMMON_QUICKLOAD).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_COMMON_QUICKSAVE).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_COMMON_QUIT_GAME).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_COMMON_REPLAY_BACKWARD).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_COMMON_REPLAY_FAST_BACKWARD).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_COMMON_REPLAY_FAST_FORWARD).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_COMMON_REPLAY_FORWARD).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_COMMON_RESCUE_TRUCK).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_COMMON_RESET_TRUCK).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_COMMON_TOGGLE_RESET_MODE).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_COMMON_SCREENSHOT).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_COMMON_SECURE_LOAD).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_COMMON_TOGGLE_DEBUG_VIEW).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_COMMON_TOGGLE_TERRAIN_EDITOR).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_COMMON_TOGGLE_CUSTOM_PARTICLES).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_COMMON_TOGGLE_MAT_DEBUG).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_COMMON_TOGGLE_RENDER_MODE).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_COMMON_TOGGLE_REPLAY_MODE).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_COMMON_TOGGLE_PHYSICS).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_COMMON_TOGGLE_STATS).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_COMMON_TOGGLE_TRUCK_BEACONS).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_COMMON_TOGGLE_TRUCK_LIGHTS).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_COMMON_TRUCK_INFO).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_COMMON_TRUCK_DESCRIPTION).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_COMMON_FOV_LESS).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_COMMON_FOV_MORE).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_COMMON_FOV_RESET).c_str());
        ImGui::NextColumn();
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_COMMON_ACCELERATE_SIMULATION).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_COMMON_DECELERATE_SIMULATION).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_COMMON_RESET_SIMULATION_PACE).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_COMMON_CONSOLE_TOGGLE).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_COMMON_ENTER_OR_EXIT_TRUCK).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_COMMON_ENTER_NEXT_TRUCK).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_COMMON_ENTER_PREVIOUS_TRUCK).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_COMMON_REMOVE_CURRENT_TRUCK).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_COMMON_RESPAWN_LAST_TRUCK).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_COMMON_FULLSCREEN_TOGGLE).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_COMMON_HIDE_GUI).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_COMMON_TOGGLE_DASHBOARD).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_COMMON_LOCK).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_COMMON_AUTOLOCK).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_COMMON_ROPELOCK).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_COMMON_OUTPUT_POSITION).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_COMMON_GET_NEW_VEHICLE).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_COMMON_PRESSURE_LESS).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_COMMON_PRESSURE_MORE).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_COMMON_QUICKLOAD).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_COMMON_QUICKSAVE).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_COMMON_QUIT_GAME).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_COMMON_REPLAY_BACKWARD).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_COMMON_REPLAY_FAST_BACKWARD).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_COMMON_REPLAY_FAST_FORWARD).c_str());    
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_COMMON_REPLAY_FORWARD).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_COMMON_RESCUE_TRUCK).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_COMMON_RESET_TRUCK).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_COMMON_TOGGLE_RESET_MODE).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_COMMON_SCREENSHOT).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_COMMON_SECURE_LOAD).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_COMMON_TOGGLE_DEBUG_VIEW).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_COMMON_TOGGLE_TERRAIN_EDITOR).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_COMMON_TOGGLE_CUSTOM_PARTICLES).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_COMMON_TOGGLE_MAT_DEBUG).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_COMMON_TOGGLE_RENDER_MODE).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_COMMON_TOGGLE_REPLAY_MODE).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_COMMON_TOGGLE_PHYSICS).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_COMMON_TOGGLE_STATS).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_COMMON_TOGGLE_TRUCK_BEACONS).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_COMMON_TOGGLE_TRUCK_LIGHTS).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_COMMON_TRUCK_INFO).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_COMMON_TRUCK_DESCRIPTION).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_COMMON_FOV_LESS).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_COMMON_FOV_MORE).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_COMMON_FOV_RESET).c_str());
    }
    else if (m_tab == ControlsTab::GRASS)
    {
        ImGui::Columns(3, /*id=*/nullptr, /*border=*/true);
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_GRASS_LESS).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_GRASS_MORE).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_GRASS_MOST).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_GRASS_NONE).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_GRASS_SAVE).c_str());
        ImGui::NextColumn();
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_GRASS_LESS).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_GRASS_MORE).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_GRASS_MOST).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_GRASS_NONE).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_GRASS_SAVE).c_str());
        ImGui::NextColumn();
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_GRASS_LESS).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_GRASS_MORE).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_GRASS_MOST).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_GRASS_NONE).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_GRASS_SAVE).c_str());
    }
    else if (m_tab == ControlsTab::MAP)
    {
        ImGui::Columns(3, /*id=*/nullptr, /*border=*/true);
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_SURVEY_MAP_ZOOM_IN).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_SURVEY_MAP_ZOOM_OUT).c_str());
        ImGui::NextColumn();
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_SURVEY_MAP_ZOOM_IN).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_SURVEY_MAP_ZOOM_OUT).c_str());
        ImGui::NextColumn();
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_SURVEY_MAP_ZOOM_IN).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_SURVEY_MAP_ZOOM_OUT).c_str());
    }    
    else if (m_tab == ControlsTab::MENU)
    {
        ImGui::Columns(3, /*id=*/nullptr, /*border=*/true);
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_MENU_DOWN).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_MENU_LEFT).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_MENU_RIGHT).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_MENU_SELECT).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_MENU_UP).c_str());
        ImGui::NextColumn();
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_MENU_DOWN).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_MENU_LEFT).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_MENU_RIGHT).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_MENU_SELECT).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_MENU_UP).c_str());
        ImGui::NextColumn();
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_MENU_DOWN).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_MENU_LEFT).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_MENU_RIGHT).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_MENU_SELECT).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_MENU_UP).c_str());
    }
    else if (m_tab == ControlsTab::TRUCK)
    {
        ImGui::Columns(3, /*id=*/nullptr, /*border=*/true);
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_TRUCK_ACCELERATE).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_TRUCK_ACCELERATE_MODIFIER_25).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_TRUCK_ACCELERATE_MODIFIER_50).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_TRUCK_ANTILOCK_BRAKE).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_TRUCK_AUTOSHIFT_DOWN).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_TRUCK_AUTOSHIFT_UP).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_TRUCK_BLINK_LEFT).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_TRUCK_BLINK_RIGHT).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_TRUCK_BLINK_WARN).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_TRUCK_BRAKE).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_TRUCK_BRAKE_MODIFIER_25).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_TRUCK_BRAKE_MODIFIER_50).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_TRUCK_CRUISE_CONTROL).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_TRUCK_CRUISE_CONTROL_READJUST).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_TRUCK_CRUISE_CONTROL_ACCL).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_TRUCK_CRUISE_CONTROL_DECL).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_TRUCK_HORN).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_TRUCK_LIGHTTOGGLE01).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_TRUCK_LIGHTTOGGLE02).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_TRUCK_LIGHTTOGGLE03).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_TRUCK_LIGHTTOGGLE04).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_TRUCK_LIGHTTOGGLE05).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_TRUCK_LIGHTTOGGLE06).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_TRUCK_LIGHTTOGGLE07).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_TRUCK_LIGHTTOGGLE08).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_TRUCK_LIGHTTOGGLE09).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_TRUCK_LIGHTTOGGLE10).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_TRUCK_MANUAL_CLUTCH).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_TRUCK_PARKING_BRAKE).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_TRUCK_TRAILER_PARKING_BRAKE).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_TRUCK_SHIFT_DOWN).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_TRUCK_SHIFT_NEUTRAL).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_TRUCK_SHIFT_UP).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_TRUCK_STARTER).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_TRUCK_STEER_LEFT).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_TRUCK_STEER_RIGHT).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_TRUCK_SWITCH_SHIFT_MODES).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_TRUCK_TOGGLE_CONTACT).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_TRUCK_TOGGLE_FORWARDCOMMANDS).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_TRUCK_TOGGLE_IMPORTCOMMANDS).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_TRUCK_TOGGLE_INTER_AXLE_DIFF).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_TRUCK_TOGGLE_INTER_WHEEL_DIFF).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_TRUCK_TOGGLE_PHYSICS).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_TRUCK_TOGGLE_TCASE_4WD_MODE).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_TRUCK_TOGGLE_TCASE_GEAR_RATIO).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_TRUCK_TOGGLE_VIDEOCAMERA).c_str());
        ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(EV_TRUCK_TRACTION_CONTROL).c_str());
        ImGui::NextColumn();
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_TRUCK_ACCELERATE).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_TRUCK_ACCELERATE_MODIFIER_25).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_TRUCK_ACCELERATE_MODIFIER_50).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_TRUCK_ANTILOCK_BRAKE).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_TRUCK_AUTOSHIFT_DOWN).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_TRUCK_AUTOSHIFT_UP).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_TRUCK_BLINK_LEFT).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_TRUCK_BLINK_RIGHT).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_TRUCK_BLINK_WARN).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_TRUCK_BRAKE).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_TRUCK_BRAKE_MODIFIER_25).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_TRUCK_BRAKE_MODIFIER_50).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_TRUCK_CRUISE_CONTROL).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_TRUCK_CRUISE_CONTROL_READJUST).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_TRUCK_CRUISE_CONTROL_ACCL).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_TRUCK_CRUISE_CONTROL_DECL).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_TRUCK_HORN).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_TRUCK_LIGHTTOGGLE01).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_TRUCK_LIGHTTOGGLE02).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_TRUCK_LIGHTTOGGLE03).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_TRUCK_LIGHTTOGGLE04).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_TRUCK_LIGHTTOGGLE05).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_TRUCK_LIGHTTOGGLE06).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_TRUCK_LIGHTTOGGLE07).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_TRUCK_LIGHTTOGGLE08).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_TRUCK_LIGHTTOGGLE09).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_TRUCK_LIGHTTOGGLE10).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_TRUCK_MANUAL_CLUTCH).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_TRUCK_PARKING_BRAKE).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_TRUCK_TRAILER_PARKING_BRAKE).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_TRUCK_SHIFT_DOWN).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_TRUCK_SHIFT_NEUTRAL).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_TRUCK_SHIFT_UP).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_TRUCK_STARTER).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_TRUCK_STEER_LEFT).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_TRUCK_STEER_RIGHT).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_TRUCK_SWITCH_SHIFT_MODES).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_TRUCK_TOGGLE_CONTACT).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_TRUCK_TOGGLE_FORWARDCOMMANDS).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_TRUCK_TOGGLE_IMPORTCOMMANDS).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_TRUCK_TOGGLE_INTER_AXLE_DIFF).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_TRUCK_TOGGLE_INTER_WHEEL_DIFF).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_TRUCK_TOGGLE_PHYSICS).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_TRUCK_TOGGLE_TCASE_4WD_MODE).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_TRUCK_TOGGLE_TCASE_GEAR_RATIO).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_TRUCK_TOGGLE_VIDEOCAMERA).c_str());
        ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(EV_TRUCK_TRACTION_CONTROL).c_str());
        ImGui::NextColumn();
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_TRUCK_ACCELERATE).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_TRUCK_ACCELERATE_MODIFIER_25).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_TRUCK_ACCELERATE_MODIFIER_50).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_TRUCK_ANTILOCK_BRAKE).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_TRUCK_AUTOSHIFT_DOWN).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_TRUCK_AUTOSHIFT_UP).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_TRUCK_BLINK_LEFT).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_TRUCK_BLINK_RIGHT).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_TRUCK_BLINK_WARN).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_TRUCK_BRAKE).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_TRUCK_BRAKE_MODIFIER_25).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_TRUCK_BRAKE_MODIFIER_50).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_TRUCK_CRUISE_CONTROL).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_TRUCK_CRUISE_CONTROL_READJUST).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_TRUCK_CRUISE_CONTROL_ACCL).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_TRUCK_CRUISE_CONTROL_DECL).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_TRUCK_HORN).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_TRUCK_LIGHTTOGGLE01).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_TRUCK_LIGHTTOGGLE02).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_TRUCK_LIGHTTOGGLE03).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_TRUCK_LIGHTTOGGLE04).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_TRUCK_LIGHTTOGGLE05).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_TRUCK_LIGHTTOGGLE06).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_TRUCK_LIGHTTOGGLE07).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_TRUCK_LIGHTTOGGLE08).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_TRUCK_LIGHTTOGGLE09).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_TRUCK_LIGHTTOGGLE10).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_TRUCK_MANUAL_CLUTCH).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_TRUCK_PARKING_BRAKE).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_TRUCK_TRAILER_PARKING_BRAKE).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_TRUCK_SHIFT_DOWN).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_TRUCK_SHIFT_NEUTRAL).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_TRUCK_SHIFT_UP).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_TRUCK_STARTER).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_TRUCK_STEER_LEFT).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_TRUCK_STEER_RIGHT).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_TRUCK_SWITCH_SHIFT_MODES).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_TRUCK_TOGGLE_CONTACT).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_TRUCK_TOGGLE_FORWARDCOMMANDS).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_TRUCK_TOGGLE_IMPORTCOMMANDS).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_TRUCK_TOGGLE_INTER_AXLE_DIFF).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_TRUCK_TOGGLE_INTER_WHEEL_DIFF).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_TRUCK_TOGGLE_PHYSICS).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_TRUCK_TOGGLE_TCASE_4WD_MODE).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_TRUCK_TOGGLE_TCASE_GEAR_RATIO).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_TRUCK_TOGGLE_VIDEOCAMERA).c_str());
        ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(EV_TRUCK_TRACTION_CONTROL).c_str());
    }    
    ImGui::End();
}

} // namespace GUI
} // namespace RoR
