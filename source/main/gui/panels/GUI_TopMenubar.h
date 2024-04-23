/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2016-2020 Petr Ohlidal

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

/// @file
/// @author Petr Ohlidal
/// @date   06/2017

#pragma once

#include "AddonPartFileFormat.h"
#include "CacheSystem.h"
#include "RoRnet.h"

#include <imgui.h>
#include <imgui_internal.h>
#include <vector>
#include <rapidjson/document.h>

struct ai_events
{
    Ogre::Vector3 position;
    int speed = -1;
};

namespace RoR {
namespace GUI {

class TopMenubar
{
public:
    const float   MENU_Y_OFFSET         = 40.f;
    const float   PANEL_HOVERBOX_HEIGHT = 50.f;
    const ImVec4  GRAY_HINT_TEXT        = ImVec4(0.62f, 0.62f, 0.61f, 1.f);
    const ImVec4  WHITE_TEXT            = ImVec4(0.9f, 0.9f, 0.9f, 1.f);
    const ImVec4  GREEN_TEXT            = ImVec4(0.0f, 0.9f, 0.0f, 1.f);
    const ImVec4  ORANGE_TEXT           = ImVec4(0.9f, 0.6f, 0.0f, 1.f);
    const ImVec4  RED_TEXT              = ImVec4(1.00f, 0.00f, 0.00f, 1.f);
    const ImVec2  MENU_HOVERBOX_PADDING = ImVec2(25.f, 10.f);

    enum class TopMenu { TOPMENU_NONE, TOPMENU_SIM, TOPMENU_ACTORS, TOPMENU_SAVEGAMES, TOPMENU_SETTINGS, TOPMENU_TOOLS, TOPMENU_AI, TOPMENU_TUNING };
    enum class StateBox { STATEBOX_NONE, STATEBOX_REPLAY, STATEBOX_RACE, STATEBOX_LIVE_REPAIR, STATEBOX_QUICK_REPAIR };

    TopMenubar();
    ~TopMenubar();

    void Draw(float dt);
    bool ShouldDisplay(ImVec2 window_pos);

    bool IsVisible() { return m_open_menu != TopMenu::TOPMENU_NONE; };

    // Vehicle AI
    std::vector<ai_events> ai_waypoints;
    int ai_num = 1;
    int ai_speed = 50;
    int ai_times = 1;
    int ai_altitude = 1000;
    int ai_distance = 20;
    int ai_position_scheme = 0;
    Ogre::String ai_fname = "95bbUID-agoras.truck";
    Ogre::String ai_dname = "Bus RVI Agora S";
    Ogre::String ai_sectionconfig = "";
    std::string ai_skin = "";
    bool ai_select = false;
    bool ai_rec = false;
    int ai_mode = 0;
    bool ai_menu = false;

    // Secondary selections for Drag Race and Crash driving modes
    bool ai_select2 = false;
    Ogre::String ai_fname2 = "95bbUID-agoras.truck";
    Ogre::String ai_dname2 = "Bus RVI Agora S";
    Ogre::String ai_sectionconfig2 = "";
    std::string ai_skin2 = "";

    int ai_num_prev = 1;
    int ai_speed_prev = 50;
    int ai_position_scheme_prev = 0;
    int ai_times_prev = 1;
    int ai_mode_prev = 0;

    // AI waypoint presets
    void DownloadAiPresets(); //!< Initiate threaded download of 'extern' waypoints from GitHub repo.
    void RefreshAiPresets();  //!< Refresh the list of presets, used for display. Needs to be called when terrain is loaded.
    rapidjson::Document ai_presets_all; //!< The full list of presets, used for display. Needs to be refreshed when terrain is loaded.
    rapidjson::Document ai_presets_extern; //!< Externally provided presets (GitHub repo or local 'savegames/waypoints.json' file).
    rapidjson::Document ai_presets_bundled; //!< Presets bundled with the terrain, see `[AI Presets]` section in .terrn2 file format.

    // Sky
    float sun_size = 1.0;
    float cloud_density = 0.0;
    float sky_light = 0.7;

    // Tuning menu
    ActorPtr tuning_actor;          //!< Detecting actor change to update cached values.
    std::vector<CacheEntryPtr> tuning_addonparts;   //!< Addonparts eligible for current actor, both matched by GUID and force-installed by user via [browse all] button.
    std::vector<bool> tuning_addonparts_conflict_w_used; //!< 1:1 with `tuning_addonparts`; True means the eligible (not used) addonpart conflicts with an used addonpart.
    AddonPartConflictVec tuning_conflicts; //!< Conflicts between eligible addonparts tweaking the same element.
    CacheQuery tuning_saves;        //!< Tuneups saved by user, with category ID `RoR::CID_AddonpartUser`
    Str<200> tuning_savebox_buf;    //!< Buffer for tuneup name to be saved
    bool tuning_savebox_visible = false;   //!< User pressed 'save active' to open savebox.
    bool tuning_savebox_overwrite = false; //!< Status of "Overwrite?" checkbox
    const ImVec4 TUNING_HOLDTOCONFIRM_COLOR = ImVec4(0.25f, 0.15f, 0.2f, 0.5f);
    const float TUNING_HOLDTOCONFIRM_TIMELIMIT = 1.5f; //!< Delete button must be held for several sec to confirm.
    float tuning_rwidget_cursorx_min = 0.f; //!< Avoid drawing right-side widgets ('Delete' button or 'Protected' chk) over saved tuneup names.
    CacheEntryPtr tuning_hovered_addonpart;
    void RefreshTuningMenu();

private:
    bool IsMenuEnabled(TopMenu which);

    void DrawActorListSinglePlayer();
    void DrawMpUserToActorList(RoRnet::UserInfo &user); // Multiplayer
    void DrawSpecialStateBox(float top_offset);

    // Tuning menu helpers
    void DrawTuningBoxedSubjectIdInline(int subject_id);
    void DrawTuningProtectedChkRightAligned(int subject_id, bool is_protected, ModifyProjectRequestType request_type_set,  ModifyProjectRequestType request_type_reset);
    void DrawTuningForceRemoveControls(const int subject_id, const std::string& name, const bool is_unwanted, const bool is_force_removed, ModifyProjectRequestType request_type_set,  ModifyProjectRequestType request_type_reset);

    ImVec2  m_open_menu_hoverbox_min;
    ImVec2  m_open_menu_hoverbox_max;
    TopMenu m_open_menu = TopMenu::TOPMENU_NONE;

    ImVec2  m_state_box_hoverbox_min;
    ImVec2  m_state_box_hoverbox_max;
    StateBox m_state_box = StateBox::STATEBOX_NONE;

    bool    m_confirm_remove_all = false;

    float   m_daytime = 0.f;
    float   m_waves_height;
    bool    m_quickload = false;
    std::string m_quicksave_name;
    std::vector<std::string> m_savegame_names;
};

} // namespace GUI
} // namespace RoR
