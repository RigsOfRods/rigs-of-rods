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

using namespace RoR;
using namespace GUI;

void GameControls::Draw()
{
    ImGui::SetNextWindowPosCenter(ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(800.f, 600.f), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin(_LC("GameControls", "Game Controls"), &m_is_visible))
    {
        ImGui::End(); // The window is collapsed
        return;
    }

    GUIManager::GuiTheme& theme = App::GetGuiManager()->GetTheme();

    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4.f, 8.f));

    if (ImGui::Button(_LC("GameControls", "Airplane"))) { m_tab = ControlsTab::AIRPLANE; }
    ImGui::SameLine();
    if (ImGui::Button(_LC("GameControls", "Boat"))) { m_tab = ControlsTab::BOAT; }
    ImGui::SameLine();
    if (ImGui::Button(_LC("GameControls", "Camera"))) { m_tab = ControlsTab::CAMERA; }
    ImGui::SameLine();
    if (ImGui::Button(_LC("GameControls", "Sky"))) { m_tab = ControlsTab::SKY; }
    ImGui::SameLine();
    if (ImGui::Button(_LC("GameControls", "Character"))) { m_tab = ControlsTab::CHARACTER; }
    ImGui::SameLine();
    if (ImGui::Button(_LC("GameControls", "Commands"))) { m_tab = ControlsTab::COMMANDS; }
    ImGui::SameLine();
    if (ImGui::Button(_LC("GameControls", "Common"))) { m_tab = ControlsTab::COMMON; }
    ImGui::SameLine();
    if (ImGui::Button(_LC("GameControls", "Grass"))) { m_tab = ControlsTab::GRASS; }
    ImGui::SameLine();
    if (ImGui::Button(_LC("GameControls", "Map"))) { m_tab = ControlsTab::MAP; }
    ImGui::SameLine();
    if (ImGui::Button(_LC("GameControls", "Menu"))) { m_tab = ControlsTab::MENU; }
    ImGui::SameLine();
    if (ImGui::Button(_LC("GameControls", "Truck"))) { m_tab = ControlsTab::TRUCK; }

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

        this->DrawEvent(EV_AIRPLANE_AIRBRAKES_FULL);
        this->DrawEvent(EV_AIRPLANE_AIRBRAKES_LESS);
        this->DrawEvent(EV_AIRPLANE_AIRBRAKES_MORE);
        this->DrawEvent(EV_AIRPLANE_AIRBRAKES_NONE);
        this->DrawEvent(EV_AIRPLANE_BRAKE);
        this->DrawEvent(EV_AIRPLANE_ELEVATOR_DOWN);
        this->DrawEvent(EV_AIRPLANE_ELEVATOR_UP);
        this->DrawEvent(EV_AIRPLANE_FLAPS_FULL);
        this->DrawEvent(EV_AIRPLANE_FLAPS_LESS);
        this->DrawEvent(EV_AIRPLANE_FLAPS_MORE);
        this->DrawEvent(EV_AIRPLANE_FLAPS_NONE);
        this->DrawEvent(EV_AIRPLANE_PARKING_BRAKE);
        this->DrawEvent(EV_AIRPLANE_REVERSE);
        this->DrawEvent(EV_AIRPLANE_RUDDER_LEFT);
        this->DrawEvent(EV_AIRPLANE_RUDDER_RIGHT);
        this->DrawEvent(EV_AIRPLANE_STEER_LEFT);
        this->DrawEvent(EV_AIRPLANE_STEER_RIGHT);
        this->DrawEvent(EV_AIRPLANE_THROTTLE_AXIS);
        this->DrawEvent(EV_AIRPLANE_THROTTLE_DOWN);
        this->DrawEvent(EV_AIRPLANE_THROTTLE_FULL);
        this->DrawEvent(EV_AIRPLANE_THROTTLE_NO);
        this->DrawEvent(EV_AIRPLANE_THROTTLE_UP);
        this->DrawEvent(EV_AIRPLANE_TOGGLE_ENGINES);
    }
    else if (m_tab == ControlsTab::BOAT)
    {
        ImGui::Columns(3, /*id=*/nullptr, /*border=*/true);

        this->DrawEvent(EV_BOAT_CENTER_RUDDER);
        this->DrawEvent(EV_BOAT_REVERSE);
        this->DrawEvent(EV_BOAT_STEER_LEFT);
        this->DrawEvent(EV_BOAT_STEER_RIGHT);
        this->DrawEvent(EV_BOAT_THROTTLE_AXIS);
        this->DrawEvent(EV_BOAT_THROTTLE_UP);
        this->DrawEvent(EV_BOAT_THROTTLE_DOWN);
    }
    else if (m_tab == ControlsTab::CAMERA)
    {
        ImGui::Columns(3, /*id=*/nullptr, /*border=*/true);

        this->DrawEvent(EV_CAMERA_CHANGE);
        this->DrawEvent(EV_CAMERA_FREE_MODE);
        this->DrawEvent(EV_CAMERA_FREE_MODE_FIX);
        this->DrawEvent(EV_CAMERA_LOOKBACK);
        this->DrawEvent(EV_CAMERA_RESET);
        this->DrawEvent(EV_CAMERA_UP);
        this->DrawEvent(EV_CAMERA_DOWN);
        this->DrawEvent(EV_CAMERA_ROTATE_DOWN);
        this->DrawEvent(EV_CAMERA_ROTATE_LEFT);
        this->DrawEvent(EV_CAMERA_ROTATE_RIGHT);
        this->DrawEvent(EV_CAMERA_ROTATE_UP);
        this->DrawEvent(EV_CAMERA_ZOOM_IN);
        this->DrawEvent(EV_CAMERA_ZOOM_IN_FAST);
        this->DrawEvent(EV_CAMERA_ZOOM_OUT);
        this->DrawEvent(EV_CAMERA_ZOOM_OUT_FAST);
    }
    else if (m_tab == ControlsTab::SKY)
    {
        ImGui::Columns(3, /*id=*/nullptr, /*border=*/true);

        this->DrawEvent(EV_SKY_DECREASE_TIME);
        this->DrawEvent(EV_SKY_DECREASE_TIME_FAST);
        this->DrawEvent(EV_SKY_INCREASE_TIME);
        this->DrawEvent(EV_SKY_INCREASE_TIME_FAST);
    }
    else if (m_tab == ControlsTab::CHARACTER)
    {
        ImGui::Columns(3, /*id=*/nullptr, /*border=*/true);

        this->DrawEvent(EV_CHARACTER_BACKWARDS);
        this->DrawEvent(EV_CHARACTER_FORWARD);
        this->DrawEvent(EV_CHARACTER_JUMP);
        this->DrawEvent(EV_CHARACTER_LEFT);
        this->DrawEvent(EV_CHARACTER_RIGHT);
        this->DrawEvent(EV_CHARACTER_ROT_DOWN);
        this->DrawEvent(EV_CHARACTER_ROT_UP);
        this->DrawEvent(EV_CHARACTER_RUN);
        this->DrawEvent(EV_CHARACTER_SIDESTEP_LEFT);
        this->DrawEvent(EV_CHARACTER_SIDESTEP_RIGHT);

    }    
    else if (m_tab == ControlsTab::COMMANDS)
    {
        ImGui::Columns(3, /*id=*/nullptr, /*border=*/true);

        this->DrawEvent(EV_COMMANDS_01);
        this->DrawEvent(EV_COMMANDS_02);
        this->DrawEvent(EV_COMMANDS_03);
        this->DrawEvent(EV_COMMANDS_04);
        this->DrawEvent(EV_COMMANDS_05);
        this->DrawEvent(EV_COMMANDS_06);
        this->DrawEvent(EV_COMMANDS_07);
        this->DrawEvent(EV_COMMANDS_08);
        this->DrawEvent(EV_COMMANDS_09);
        this->DrawEvent(EV_COMMANDS_10);
        this->DrawEvent(EV_COMMANDS_11);
        this->DrawEvent(EV_COMMANDS_12);
        this->DrawEvent(EV_COMMANDS_13);
        this->DrawEvent(EV_COMMANDS_14);
        this->DrawEvent(EV_COMMANDS_15);
        this->DrawEvent(EV_COMMANDS_16);
        this->DrawEvent(EV_COMMANDS_17);
        this->DrawEvent(EV_COMMANDS_18);
        this->DrawEvent(EV_COMMANDS_19);
        this->DrawEvent(EV_COMMANDS_20);
        this->DrawEvent(EV_COMMANDS_21);
        this->DrawEvent(EV_COMMANDS_22);
        this->DrawEvent(EV_COMMANDS_23);
        this->DrawEvent(EV_COMMANDS_24);
        this->DrawEvent(EV_COMMANDS_25);
        this->DrawEvent(EV_COMMANDS_26);
        this->DrawEvent(EV_COMMANDS_27);
        this->DrawEvent(EV_COMMANDS_28);
        this->DrawEvent(EV_COMMANDS_29);
        this->DrawEvent(EV_COMMANDS_30);
        this->DrawEvent(EV_COMMANDS_31);
        this->DrawEvent(EV_COMMANDS_32);
        this->DrawEvent(EV_COMMANDS_33);
        this->DrawEvent(EV_COMMANDS_34);
        this->DrawEvent(EV_COMMANDS_35);
        this->DrawEvent(EV_COMMANDS_36);
        this->DrawEvent(EV_COMMANDS_37);
        this->DrawEvent(EV_COMMANDS_38);
        this->DrawEvent(EV_COMMANDS_39);
        this->DrawEvent(EV_COMMANDS_40);
        this->DrawEvent(EV_COMMANDS_41);
        this->DrawEvent(EV_COMMANDS_42);
        this->DrawEvent(EV_COMMANDS_43);
        this->DrawEvent(EV_COMMANDS_44);
        this->DrawEvent(EV_COMMANDS_45);
        this->DrawEvent(EV_COMMANDS_46);
        this->DrawEvent(EV_COMMANDS_47);
        this->DrawEvent(EV_COMMANDS_48);
        this->DrawEvent(EV_COMMANDS_49);
        this->DrawEvent(EV_COMMANDS_50);
        this->DrawEvent(EV_COMMANDS_51);
        this->DrawEvent(EV_COMMANDS_52);
        this->DrawEvent(EV_COMMANDS_53);
        this->DrawEvent(EV_COMMANDS_54);
        this->DrawEvent(EV_COMMANDS_55);
        this->DrawEvent(EV_COMMANDS_56);
        this->DrawEvent(EV_COMMANDS_57);
        this->DrawEvent(EV_COMMANDS_58);
        this->DrawEvent(EV_COMMANDS_59);
        this->DrawEvent(EV_COMMANDS_60);
        this->DrawEvent(EV_COMMANDS_61);
        this->DrawEvent(EV_COMMANDS_62);
        this->DrawEvent(EV_COMMANDS_63);
        this->DrawEvent(EV_COMMANDS_64);
        this->DrawEvent(EV_COMMANDS_65);
        this->DrawEvent(EV_COMMANDS_66);
        this->DrawEvent(EV_COMMANDS_67);
        this->DrawEvent(EV_COMMANDS_68);
        this->DrawEvent(EV_COMMANDS_69);
        this->DrawEvent(EV_COMMANDS_70);
        this->DrawEvent(EV_COMMANDS_71);
        this->DrawEvent(EV_COMMANDS_72);
        this->DrawEvent(EV_COMMANDS_73);
        this->DrawEvent(EV_COMMANDS_74);
        this->DrawEvent(EV_COMMANDS_75);
        this->DrawEvent(EV_COMMANDS_76);
        this->DrawEvent(EV_COMMANDS_77);
        this->DrawEvent(EV_COMMANDS_78);
        this->DrawEvent(EV_COMMANDS_79);
        this->DrawEvent(EV_COMMANDS_80);
        this->DrawEvent(EV_COMMANDS_81);
        this->DrawEvent(EV_COMMANDS_82);
        this->DrawEvent(EV_COMMANDS_83);
        this->DrawEvent(EV_COMMANDS_84);
    }
    else if (m_tab == ControlsTab::COMMON)
    {
        ImGui::Columns(3, /*id=*/nullptr, /*border=*/true);

        this->DrawEvent(EV_COMMON_ACCELERATE_SIMULATION);
        this->DrawEvent(EV_COMMON_DECELERATE_SIMULATION);
        this->DrawEvent(EV_COMMON_RESET_SIMULATION_PACE);
        this->DrawEvent(EV_COMMON_CONSOLE_TOGGLE);
        this->DrawEvent(EV_COMMON_ENTER_OR_EXIT_TRUCK);
        this->DrawEvent(EV_COMMON_ENTER_NEXT_TRUCK);
        this->DrawEvent(EV_COMMON_ENTER_PREVIOUS_TRUCK);
        this->DrawEvent(EV_COMMON_REMOVE_CURRENT_TRUCK);
        this->DrawEvent(EV_COMMON_RESPAWN_LAST_TRUCK);
        this->DrawEvent(EV_COMMON_FULLSCREEN_TOGGLE);
        this->DrawEvent(EV_COMMON_HIDE_GUI);
        this->DrawEvent(EV_COMMON_TOGGLE_DASHBOARD);
        this->DrawEvent(EV_COMMON_LOCK);
        this->DrawEvent(EV_COMMON_AUTOLOCK);
        this->DrawEvent(EV_COMMON_ROPELOCK);
        this->DrawEvent(EV_COMMON_OUTPUT_POSITION);
        this->DrawEvent(EV_COMMON_GET_NEW_VEHICLE);
        this->DrawEvent(EV_COMMON_PRESSURE_LESS);
        this->DrawEvent(EV_COMMON_PRESSURE_MORE);
        this->DrawEvent(EV_COMMON_QUICKLOAD);
        this->DrawEvent(EV_COMMON_QUICKSAVE);
        this->DrawEvent(EV_COMMON_QUIT_GAME);
        this->DrawEvent(EV_COMMON_REPLAY_BACKWARD);
        this->DrawEvent(EV_COMMON_REPLAY_FAST_BACKWARD);
        this->DrawEvent(EV_COMMON_REPLAY_FAST_FORWARD);
        this->DrawEvent(EV_COMMON_REPLAY_FORWARD);
        this->DrawEvent(EV_COMMON_RESCUE_TRUCK);
        this->DrawEvent(EV_COMMON_RESET_TRUCK);
        this->DrawEvent(EV_COMMON_TOGGLE_RESET_MODE);
        this->DrawEvent(EV_COMMON_SCREENSHOT);
        this->DrawEvent(EV_COMMON_SECURE_LOAD);
        this->DrawEvent(EV_COMMON_TOGGLE_DEBUG_VIEW);
        this->DrawEvent(EV_COMMON_TOGGLE_TERRAIN_EDITOR);
        this->DrawEvent(EV_COMMON_TOGGLE_CUSTOM_PARTICLES);
        this->DrawEvent(EV_COMMON_TOGGLE_MAT_DEBUG);
        this->DrawEvent(EV_COMMON_TOGGLE_RENDER_MODE);
        this->DrawEvent(EV_COMMON_TOGGLE_REPLAY_MODE);
        this->DrawEvent(EV_COMMON_TOGGLE_PHYSICS);
        this->DrawEvent(EV_COMMON_TOGGLE_STATS);
        this->DrawEvent(EV_COMMON_TOGGLE_TRUCK_BEACONS);
        this->DrawEvent(EV_COMMON_TOGGLE_TRUCK_LIGHTS);
        this->DrawEvent(EV_COMMON_TRUCK_INFO);
        this->DrawEvent(EV_COMMON_TRUCK_DESCRIPTION);
        this->DrawEvent(EV_COMMON_FOV_LESS);
        this->DrawEvent(EV_COMMON_FOV_MORE);
        this->DrawEvent(EV_COMMON_FOV_RESET);
    }
    else if (m_tab == ControlsTab::GRASS)
    {
        ImGui::Columns(3, /*id=*/nullptr, /*border=*/true);

        this->DrawEvent(EV_GRASS_LESS);
        this->DrawEvent(EV_GRASS_MORE);
        this->DrawEvent(EV_GRASS_MOST);
        this->DrawEvent(EV_GRASS_NONE);
        this->DrawEvent(EV_GRASS_SAVE);
    }
    else if (m_tab == ControlsTab::MAP)
    {
        ImGui::Columns(3, /*id=*/nullptr, /*border=*/true);

        this->DrawEvent(EV_SURVEY_MAP_ZOOM_IN);
        this->DrawEvent(EV_SURVEY_MAP_ZOOM_OUT);
    }    
    else if (m_tab == ControlsTab::MENU)
    {
        ImGui::Columns(3, /*id=*/nullptr, /*border=*/true);

        this->DrawEvent(EV_MENU_DOWN);
        this->DrawEvent(EV_MENU_LEFT);
        this->DrawEvent(EV_MENU_RIGHT);
        this->DrawEvent(EV_MENU_SELECT);
        this->DrawEvent(EV_MENU_UP);
    }
    else if (m_tab == ControlsTab::TRUCK)
    {
        ImGui::Columns(3, /*id=*/nullptr, /*border=*/true);

        this->DrawEvent(EV_TRUCK_ACCELERATE);
        this->DrawEvent(EV_TRUCK_ACCELERATE_MODIFIER_25);
        this->DrawEvent(EV_TRUCK_ACCELERATE_MODIFIER_50);
        this->DrawEvent(EV_TRUCK_ANTILOCK_BRAKE);
        this->DrawEvent(EV_TRUCK_AUTOSHIFT_DOWN);
        this->DrawEvent(EV_TRUCK_AUTOSHIFT_UP);
        this->DrawEvent(EV_TRUCK_BLINK_LEFT);
        this->DrawEvent(EV_TRUCK_BLINK_RIGHT);
        this->DrawEvent(EV_TRUCK_BLINK_WARN);
        this->DrawEvent(EV_TRUCK_BRAKE);
        this->DrawEvent(EV_TRUCK_BRAKE_MODIFIER_25);
        this->DrawEvent(EV_TRUCK_BRAKE_MODIFIER_50);
        this->DrawEvent(EV_TRUCK_CRUISE_CONTROL);
        this->DrawEvent(EV_TRUCK_CRUISE_CONTROL_READJUST);
        this->DrawEvent(EV_TRUCK_CRUISE_CONTROL_ACCL);
        this->DrawEvent(EV_TRUCK_CRUISE_CONTROL_DECL);
        this->DrawEvent(EV_TRUCK_HORN);
        this->DrawEvent(EV_TRUCK_LIGHTTOGGLE01);
        this->DrawEvent(EV_TRUCK_LIGHTTOGGLE02);
        this->DrawEvent(EV_TRUCK_LIGHTTOGGLE03);
        this->DrawEvent(EV_TRUCK_LIGHTTOGGLE04);
        this->DrawEvent(EV_TRUCK_LIGHTTOGGLE05);
        this->DrawEvent(EV_TRUCK_LIGHTTOGGLE06);
        this->DrawEvent(EV_TRUCK_LIGHTTOGGLE07);
        this->DrawEvent(EV_TRUCK_LIGHTTOGGLE08);
        this->DrawEvent(EV_TRUCK_LIGHTTOGGLE09);
        this->DrawEvent(EV_TRUCK_LIGHTTOGGLE10);
        this->DrawEvent(EV_TRUCK_MANUAL_CLUTCH);
        this->DrawEvent(EV_TRUCK_PARKING_BRAKE);
        this->DrawEvent(EV_TRUCK_TRAILER_PARKING_BRAKE);
        this->DrawEvent(EV_TRUCK_SHIFT_DOWN);
        this->DrawEvent(EV_TRUCK_SHIFT_NEUTRAL);
        this->DrawEvent(EV_TRUCK_SHIFT_UP);
        this->DrawEvent(EV_TRUCK_STARTER);
        this->DrawEvent(EV_TRUCK_STEER_LEFT);
        this->DrawEvent(EV_TRUCK_STEER_RIGHT);
        this->DrawEvent(EV_TRUCK_SWITCH_SHIFT_MODES);
        this->DrawEvent(EV_TRUCK_TOGGLE_CONTACT);
        this->DrawEvent(EV_TRUCK_TOGGLE_FORWARDCOMMANDS);
        this->DrawEvent(EV_TRUCK_TOGGLE_IMPORTCOMMANDS);
        this->DrawEvent(EV_TRUCK_TOGGLE_INTER_AXLE_DIFF);
        this->DrawEvent(EV_TRUCK_TOGGLE_INTER_WHEEL_DIFF);
        this->DrawEvent(EV_TRUCK_TOGGLE_PHYSICS);
        this->DrawEvent(EV_TRUCK_TOGGLE_TCASE_4WD_MODE);
        this->DrawEvent(EV_TRUCK_TOGGLE_TCASE_GEAR_RATIO);
        this->DrawEvent(EV_TRUCK_TOGGLE_VIDEOCAMERA);
        this->DrawEvent(EV_TRUCK_TRACTION_CONTROL);
    }    
    ImGui::End();
}

void GameControls::DrawEvent(RoR::events ev_code)
{
    GUIManager::GuiTheme& theme = App::GetGuiManager()->GetTheme();

    ImGui::TextColored(theme.value_blue_text_color, "%s", App::GetInputEngine()->eventIDToName(ev_code).c_str());
    ImGui::NextColumn();
    ImGui::TextColored(theme.success_text_color, "%s", App::GetInputEngine()->getEventCommand(ev_code).c_str());
    ImGui::NextColumn();
    ImGui::TextColored(GRAY_HINT_TEXT, "%s", App::GetInputEngine()->eventIDToDescription(ev_code).c_str());
    ImGui::NextColumn();
}
