/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2017-2018 Petr Ohlidal & contributors

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

#include "GUI_TopMenubar.h"

#include "Application.h"
#include "Beam.h"
#include "BeamFactory.h"
#include "CacheSystem.h"
#include "ContentManager.h"
#include "GUIManager.h"
#include "GUIUtils.h"
#include "GUI_MainSelector.h"
#include "InputEngine.h"
#include "RigEditor.h"
#include "Language.h"
#include "Network.h"
#include "PlatformUtils.h"
#include "RoRFrameListener.h"
#include "SkyManager.h"
#include "TerrainManager.h"

#include <algorithm>

void RoR::GUI::TopMenubar::Update()
{
    // ## ImGui's 'menubar' and 'menuitem' features won't quite cut it...
    // ## Let's do our own menus and menuitems using buttons and coloring tricks.

    GUIManager::GuiTheme const& theme = App::GetGuiManager()->GetTheme();

    const char* sim_title = "Simulation"; // TODO: Localize all!
    Str<50> actors_title;
    auto actors = App::GetSimController()->GetActors();
    int num_playable_actors = std::count_if(actors.begin(), actors.end(), [](Actor* a) {return !a->ar_hide_in_actor_list;});
    actors_title << "Vehicles (" << num_playable_actors << ")";
    const char* savegames_title = "Saves";
    const char* settings_title = "Settings";
    const char* tools_title = "Tools";
    const char* projects_title = "Projects";
    const int NUM_BUTTONS = 6;

    float menubar_content_width =
        (ImGui::GetStyle().ItemSpacing.x * (NUM_BUTTONS - 1)) +
        (ImGui::GetStyle().FramePadding.x * (NUM_BUTTONS * 2)) +
        ImGui::CalcTextSize(sim_title).x +
        ImGui::CalcTextSize(actors_title.ToCStr()).x +
        ImGui::CalcTextSize(savegames_title).x +
        ImGui::CalcTextSize(projects_title).x +
        ImGui::CalcTextSize(settings_title).x +
        ImGui::CalcTextSize(tools_title).x;

    ImVec2 window_target_pos = ImVec2((ImGui::GetIO().DisplaySize.x/2.f) - (menubar_content_width / 2.f), theme.screen_edge_padding.y);
    if (!this->ShouldDisplay(window_target_pos))
    {
        m_open_menu = TopMenu::TOPMENU_NONE;
        m_confirm_remove_all = false;
        this->DrawSpecialStateBox(10.f);
        return;
    }

    ImGui::PushStyleColor(ImGuiCol_WindowBg, theme.semitransparent_window_bg);
    ImGui::PushStyleColor(ImGuiCol_Button,   ImVec4(0,0,0,0)); // Fully transparent

    // The panel
    int flags = ImGuiWindowFlags_NoCollapse  | ImGuiWindowFlags_NoResize    | ImGuiWindowFlags_NoMove
              | ImGuiWindowFlags_NoTitleBar  | ImGuiWindowFlags_AlwaysAutoResize;
    ImGui::SetNextWindowContentSize(ImVec2(menubar_content_width, 0.f));
    ImGui::SetNextWindowPos(window_target_pos);
    ImGui::Begin("Top menubar", nullptr, flags);

    // The 'simulation' button
    ImVec2 window_pos = ImGui::GetWindowPos();
    ImVec2 sim_cursor = ImGui::GetCursorPos();
    ImGui::Button(sim_title);
    if ((m_open_menu != TopMenu::TOPMENU_SIM) && ImGui::IsItemHovered())
    {
        m_open_menu = TopMenu::TOPMENU_SIM;
    }

    ImGui::SameLine();

    // The 'vehicles' button
    ImVec2 actors_cursor = ImGui::GetCursorPos();
    ImGui::Button(actors_title);
    if ((m_open_menu != TopMenu::TOPMENU_ACTORS) && ImGui::IsItemHovered())
    {
        m_open_menu = TopMenu::TOPMENU_ACTORS;
    }

    ImGui::SameLine();

    // The 'projects' button
    ImVec2 projects_cursor = ImGui::GetCursorPos();
    ImGui::Button(projects_title);
    if ((m_open_menu != TopMenu::TOPMENU_PROJECTS) && ImGui::IsItemHovered())
    {
        m_open_menu = TopMenu::TOPMENU_PROJECTS;
    }

    ImGui::SameLine();

    // The 'savegames' button
    ImVec2 savegames_cursor = ImGui::GetCursorPos();
    ImGui::Button(savegames_title);
    if ((m_open_menu != TopMenu::TOPMENU_SAVEGAMES) && ImGui::IsItemHovered())
    {
        m_open_menu = TopMenu::TOPMENU_SAVEGAMES;
        m_quicksave_name = App::GetSimController()->GetBeamFactory()->GetQuicksaveFilename();
        m_quickload = FileExists(PathCombine(App::sys_savegames_dir->GetActiveStr(), m_quicksave_name));
        m_savegame_names.clear();
        for (int i = 0; i <= 9; i++)
        {
            Ogre::String filename = Ogre::StringUtil::format("quicksave-%d.sav", i);
            m_savegame_names.push_back(App::GetSimController()->GetBeamFactory()->ExtractSceneName(filename));
        }
    }

    ImGui::SameLine();

    // The 'settings' button
    ImVec2 settings_cursor = ImGui::GetCursorPos();
    ImGui::Button(settings_title);
    if ((m_open_menu != TopMenu::TOPMENU_SETTINGS) && ImGui::IsItemHovered())
    {
        m_open_menu = TopMenu::TOPMENU_SETTINGS;
#ifdef USE_CAELUM
        if (App::gfx_sky_mode->GetActiveEnum<GfxSkyMode>() == GfxSkyMode::CAELUM)
            m_daytime = App::GetSimTerrain()->getSkyManager()->GetTime();
#endif // USE_CAELUM
    }

    ImGui::SameLine();

    // The 'tools' button
    ImVec2 tools_cursor = ImGui::GetCursorPos();
    ImGui::Button(tools_title);
    if ((m_open_menu != TopMenu::TOPMENU_TOOLS) && ImGui::IsItemHovered())
    {
        m_open_menu = TopMenu::TOPMENU_TOOLS;
    }

    ImVec2 topmenu_final_size = ImGui::GetWindowSize();
    App::GetGuiManager()->RequestGuiCaptureKeyboard(ImGui::IsWindowHovered());
    ImGui::End();

    this->DrawSpecialStateBox(window_target_pos.y + topmenu_final_size.y + 10.f);

    ImVec2 menu_pos;
    Actor* current_actor = App::GetSimController()->GetPlayerActor();
    switch (m_open_menu)
    {
    case TopMenu::TOPMENU_SIM:
        menu_pos.y = window_pos.y + sim_cursor.y + MENU_Y_OFFSET;
        menu_pos.x = sim_cursor.x + window_pos.x - ImGui::GetStyle().WindowPadding.x;
        ImGui::SetNextWindowPos(menu_pos);
        if (ImGui::Begin("Sim menu", nullptr, static_cast<ImGuiWindowFlags_>(flags)))
        {
            // TODO: Display hotkeys on the right side of the menu (with different text color)

            if (ImGui::Button("Get new vehicle"))
            {
                App::GetGuiManager()->GetMainSelector()->Show(LT_AllBeam);
                m_open_menu = TopMenu::TOPMENU_NONE;
            }

            if (current_actor != nullptr)
            {
                if (ImGui::Button("Show vehicle description"))
                {
                    App::GetGuiManager()->SetVisible_VehicleDescription(true);
                }

                if (current_actor->ar_sim_state != Actor::SimState::NETWORKED_OK)
                {
                    if (ImGui::Button("Reload current vehicle"))
                    {
                        {
                            ActorModifyRequest rq;
                            rq.amr_type = ActorModifyRequest::Type::RELOAD;
                            rq.amr_actor = current_actor;
                            App::GetSimController()->QueueActorModify(rq);
                        }
                    }

                    if (ImGui::Button("Remove current vehicle"))
                    {
                        App::GetSimController()->QueueActorRemove(current_actor);
                    }
                }
            }

            if (!App::GetSimController()->GetLocalActors().empty())
            {
                if (ImGui::Button("Remove all vehicles"))
                {
                    m_confirm_remove_all = true;
                }
                if (m_confirm_remove_all)
                {
                    ImGui::PushStyleColor(ImGuiCol_Text, ORANGE_TEXT);
                    if (ImGui::Button(" [!] Confirm removal"))
                    {
                        for (auto actor : App::GetSimController()->GetLocalActors())
                        {
                            if (!actor->ar_hide_in_actor_list && !actor->isPreloadedWithTerrain() && 
                                    actor->ar_sim_state != Actor::SimState::NETWORKED_OK)
                            {
                                App::GetSimController()->QueueActorRemove(actor);
                            }
                        }
                        m_confirm_remove_all = false;
                    }
                    ImGui::PopStyleColor();
                }

                if (ImGui::Button("Activate all vehicles"))
                {
                    App::GetSimController()->GetBeamFactory()->WakeUpAllActors();
                }

                bool force_trucks_active = App::GetSimController()->GetBeamFactory()->AreTrucksForcedAwake();
                if (ImGui::Checkbox("Activated vehicles never sleep", &force_trucks_active))
                {
                    App::GetSimController()->GetBeamFactory()->SetTrucksForcedAwake(force_trucks_active);
                }

                if (ImGui::Button("Send all vehicles to sleep"))
                {
                    App::GetSimController()->GetBeamFactory()->SendAllActorsSleeping();
                }
            }

            ImGui::Separator();

            if (ImGui::Button("Back to menu"))
            {
                App::app_state->SetPendingVal((int)RoR::AppState::MAIN_MENU);
            }

            if (ImGui::Button("Exit"))
            {
                App::app_state->SetPendingVal((int)RoR::AppState::SHUTDOWN);
            }

            m_open_menu_hoverbox_min = menu_pos;
            m_open_menu_hoverbox_max.x = menu_pos.x + ImGui::GetWindowWidth();
            m_open_menu_hoverbox_max.y = menu_pos.y + ImGui::GetWindowHeight();
            App::GetGuiManager()->RequestGuiCaptureKeyboard(ImGui::IsWindowHovered());
            ImGui::End();
        }
        break;

    case TopMenu::TOPMENU_ACTORS:
        menu_pos.y = window_pos.y + actors_cursor.y + MENU_Y_OFFSET;
        menu_pos.x = actors_cursor.x + window_pos.x - ImGui::GetStyle().WindowPadding.x;
        ImGui::SetNextWindowPos(menu_pos);
        if (ImGui::Begin("Actors menu", nullptr, static_cast<ImGuiWindowFlags_>(flags)))
        {
            if (App::mp_state->GetActiveEnum<MpState>() != MpState::CONNECTED)
            {
                this->DrawActorListSinglePlayer();
            }
            else
            {
#ifdef USE_SOCKETW
                RoRnet::UserInfo net_user_info = RoR::Networking::GetLocalUserData();
                this->DrawMpUserToActorList(net_user_info);

                std::vector<RoRnet::UserInfo> remote_users = RoR::Networking::GetUserInfos();
                for (auto& user: remote_users)
                {
                    this->DrawMpUserToActorList(user);
                }
#endif // USE_SOCKETW
            }
            m_open_menu_hoverbox_min = menu_pos;
            m_open_menu_hoverbox_max.x = menu_pos.x + ImGui::GetWindowWidth();
            m_open_menu_hoverbox_max.y = menu_pos.y + ImGui::GetWindowHeight();
            ImGui::End();
        }
        break;

    case TopMenu::TOPMENU_PROJECTS:
        menu_pos.y = window_pos.y + projects_cursor.y + MENU_Y_OFFSET;
        menu_pos.x = projects_cursor.x + window_pos.x - ImGui::GetStyle().WindowPadding.x;
        ImGui::SetNextWindowPos(menu_pos);
        if (ImGui::Begin("Projects menu", nullptr, static_cast<ImGuiWindowFlags_>(flags)))
        {
            CacheSystem::ProjectEntryVec& projects = App::GetCacheSystem()->GetProjectEntries();
            int id_counter = 0; // Project names may not be unique
            for (std::unique_ptr<ProjectEntry>& project: projects)
            {
                ImGui::PushID(id_counter++);
                if (ImGui::TreeNode(project->prj_name.c_str())) // TODO: EDIT button
                {
                    for (int i = 0; i < (int)project->prj_snapshots.size(); ++i)
                    {
                        if (ImGui::Button(project->prj_snapshots[i].prs_name.c_str()))
                        {
                            App::GetSimController()->GetRigEditor().LoadSnapshot(project.get(), i);
                        }
                    }
                    ImGui::TreePop();
                }
                ImGui::PopID();
            }

            if (projects.size() == 0)
            {
                ImGui::TextColored(GRAY_HINT_TEXT, _L("There are no projects"));
            }

            Actor* src_actor = App::GetSimController()->GetPlayerActor();
            if (src_actor && src_actor->GetProjectEntry())
            {
                ImGui::Separator(); // Current project actions

                std::string example_to_add;
                if (ImGui::Button(_L("+ Add Example script"))) { example_to_add = "framestep_demo.as"; }
                if (ImGui::Button(_L("+ Add prototype dash"))) { example_to_add = "framestep_dash_demo.as"; }

                if (example_to_add != "")
                {
                    App::GetSimController()->GetRigEditor().AddExampleScriptToSnapshot(example_to_add);
                    App::GetSimController()->GetRigEditor().SaveSnapshot();

                    ActorModifyRequest rq;
                    rq.amr_type = ActorModifyRequest::Type::RELOAD;
                    rq.amr_actor = current_actor;
                    App::GetSimController()->QueueActorModify(rq);
                }
            }

            ImGui::Separator();

            if (ImGui::Button(_L("Refresh list")))
            {
                App::GetContentManager()->ReScanProjects(); // TODO: Make it happen asynchronously!
            }

            if (App::GetSimController()->GetPlayerActor() == nullptr)
            {
                ImGui::TextColored(GRAY_HINT_TEXT, _L("Import current vehicle"));
            }
            else
            {
                
                Str<200> btn_title;
                btn_title << "Import '" << src_actor->GetActorDesignName() << "'";
                if (ImGui::Button(btn_title.ToCStr()))
                {
                    // TODO: Make it happen asynchronously!
                    std::string filename = src_actor->GetActorFileName();
                    Str<200> display_name;
                    display_name << "(Imported) " << src_actor->GetActorDesignName();
                    if (App::GetSimController()->GetRigEditor().CreateProjet(filename, display_name.ToCStr()))
                    {
                        App::GetSimController()->GetRigEditor().ImportSnapshotToProject(filename, src_actor->GetDefinition()); // Logs+displays errors
                    }
                }
            }

            bool grp_loose = App::diag_import_grp_loose->GetActiveVal<bool>();
            if (ImGui::Checkbox(_L("Loose ;grp: import (respawn!)"), &grp_loose))
            {
                App::diag_import_grp_loose->SetActiveVal(grp_loose);
            }
            if (ImGui::IsItemHovered())
            {
                ImGui::BeginTooltip();
                ImGui::Text(_L("Accept all ';' comments as ';grp:' comments. Takes effect after vehicle reload."));
                ImGui::EndTooltip();
            }

            // Finalize menu panel
            m_open_menu_hoverbox_min = menu_pos;
            m_open_menu_hoverbox_max.x = menu_pos.x + ImGui::GetWindowWidth();
            m_open_menu_hoverbox_max.y = menu_pos.y + ImGui::GetWindowHeight();
            App::GetGuiManager()->RequestGuiCaptureKeyboard(ImGui::IsWindowHovered());
            ImGui::End();
        }
        break;

    case TopMenu::TOPMENU_SAVEGAMES:
        menu_pos.y = window_pos.y + savegames_cursor.y + MENU_Y_OFFSET;
        menu_pos.x = savegames_cursor.x + window_pos.x - ImGui::GetStyle().WindowPadding.x;
        ImGui::SetNextWindowPos(menu_pos);
        if (ImGui::Begin("Savegames", nullptr, static_cast<ImGuiWindowFlags_>(flags)))
        {
            if (ImGui::Button("Quicksave"))
            {
                App::GetSimController()->GetBeamFactory()->SaveScene(m_quicksave_name);
                m_open_menu = TopMenu::TOPMENU_NONE;
            }
            ImGui::SameLine();
            ImGui::TextColored(GRAY_HINT_TEXT, "('NUMPAD: /')");

            if (m_quickload)
            {
                if (ImGui::Button("Quickload"))
                {
                    App::GetSimController()->GetBeamFactory()->LoadScene(m_quicksave_name);
                    m_open_menu = TopMenu::TOPMENU_NONE;
                }
                ImGui::SameLine();
                ImGui::TextColored(GRAY_HINT_TEXT, "('NUMPAD: *')");
            }

            ImGui::Separator();

            ImGui::TextColored(GRAY_HINT_TEXT, "(Save with 'CTRL+ALT+1..5')");
            for (int i = 1; i <= 5; i++)
            {
                Ogre::String name = "Empty Slot";
                if (!m_savegame_names[i].empty())
                {
                    name = m_savegame_names[i];
                }
                Ogre::String caption = Ogre::StringUtil::format("%d. %s##Save", i, name.c_str());
                if (ImGui::Button(caption.c_str()))
                {
                    Ogre::String filename = Ogre::StringUtil::format("quicksave-%d.sav", i);
                    App::GetSimController()->GetBeamFactory()->SaveScene(filename);
                    m_open_menu = TopMenu::TOPMENU_NONE;
                }
            }

            ImGui::TextColored(GRAY_HINT_TEXT, "(Load with 'ALT+1..5')");
            for (int i = 1; i <= 5; i++)
            {
                if (!m_savegame_names[i].empty())
                {
                    Ogre::String name = m_savegame_names[i];
                    Ogre::String caption = Ogre::StringUtil::format("%d. %s##Load", i, name.c_str());
                    if (ImGui::Button(caption.c_str()))
                    {
                        Ogre::String filename = Ogre::StringUtil::format("quicksave-%d.sav", i);
                        App::GetSimController()->GetBeamFactory()->LoadScene(filename);
                        m_open_menu = TopMenu::TOPMENU_NONE;
                    }
                }
            }

            m_open_menu_hoverbox_min = menu_pos;
            m_open_menu_hoverbox_max.x = menu_pos.x + ImGui::GetWindowWidth();
            m_open_menu_hoverbox_max.y = menu_pos.y + ImGui::GetWindowHeight() * 2;
            App::GetGuiManager()->RequestGuiCaptureKeyboard(ImGui::IsWindowHovered());
            ImGui::End();
        }
        break;

    case TopMenu::TOPMENU_SETTINGS:
        menu_pos.y = window_pos.y + settings_cursor.y + MENU_Y_OFFSET;
        menu_pos.x = settings_cursor.x + window_pos.x - ImGui::GetStyle().WindowPadding.x;
        ImGui::SetNextWindowPos(menu_pos);
        if (ImGui::Begin("Settings menu", nullptr, static_cast<ImGuiWindowFlags_>(flags)))
        {
            ImGui::PushItemWidth(125.f); // Width includes [+/-] buttons
            ImGui::TextColored(GRAY_HINT_TEXT, "Audio:");
            DrawGFloatSlider(App::audio_master_volume, "Volume", 0, 1);
            ImGui::Separator();
            ImGui::TextColored(GRAY_HINT_TEXT, "Frames per second:");
            if (App::gfx_envmap_enabled->GetActiveVal<bool>())
            {
                DrawGIntSlider(App::gfx_envmap_rate, "Reflections", 0, 6);
            }
            DrawGIntSlider(App::gfx_fps_limit, "Graphics", 0, 240);
            int physics_fps = std::round(1.0f / App::diag_physics_dt->GetActiveVal<float>());
            if (ImGui::SliderInt("Physics", &physics_fps, 2000, 10000))
            {
                App::diag_physics_dt->SetActiveVal(Ogre::Math::Clamp(1.0f / physics_fps, 0.0001f, 0.0005f));
            }
            ImGui::Separator();
            ImGui::TextColored(GRAY_HINT_TEXT, "Simulation:");
            float slowmotion = std::min(App::GetSimController()->GetBeamFactory()->GetSimulationSpeed(), 1.0f);
            if (ImGui::SliderFloat("Slow motion", &slowmotion, 0.01f, 1.0f))
            {
                App::GetSimController()->GetBeamFactory()->SetSimulationSpeed(slowmotion);
            }
            float timelapse = std::max(App::GetSimController()->GetBeamFactory()->GetSimulationSpeed(), 1.0f);
            if (ImGui::SliderFloat("Time lapse", &timelapse, 1.0f, 10.0f))
            {
                App::GetSimController()->GetBeamFactory()->SetSimulationSpeed(timelapse);
            }
            if (App::GetSimController()->GetCameraBehavior() == CameraManager::CAMERA_BEHAVIOR_STATIC)
            {
                ImGui::Separator();
                ImGui::TextColored(GRAY_HINT_TEXT, "Camera:");
                DrawGFloatSlider(App::gfx_static_cam_fov_exp, "FOV", 0.8f, 1.5f);
                DrawGIntSlider(App::gfx_camera_height, "Height", 1, 50);
            }
            else
            {
                ImGui::Separator();
                ImGui::TextColored(GRAY_HINT_TEXT, "Camera:");
                if (App::GetSimController()->GetCameraBehavior() == CameraManager::CAMERA_BEHAVIOR_VEHICLE_CINECAM)
                {
                    int fov = App::gfx_fov_internal->GetActiveVal<int>();
                    if (ImGui::SliderInt("FOV", &fov, 10, 120))
                    {
                        App::gfx_fov_internal->SetActiveVal(fov);
                    }
                }
                else
                {
                    int fov = App::gfx_fov_external->GetActiveVal<int>();
                    if (ImGui::SliderInt("FOV", &fov, 10, 120))
                    {
                        App::gfx_fov_external->SetActiveVal(fov);
                    }
                }
                if (App::GetSimController()->GetCameraBehavior() == CameraManager::CAMERA_BEHAVIOR_FIXED)
                {
                    DrawGCheckbox(App::gfx_fixed_cam_tracking, "Tracking");
                }
            }
#ifdef USE_CAELUM
            if (App::gfx_sky_mode->GetActiveEnum<GfxSkyMode>() == GfxSkyMode::CAELUM)
            {
                ImGui::Separator();
                ImGui::TextColored(GRAY_HINT_TEXT, "Time of day:");
                float time = App::GetSimTerrain()->getSkyManager()->GetTime();
                if (ImGui::SliderFloat("", &time, m_daytime - 0.5f, m_daytime + 0.5f, ""))
                {
                    App::GetSimTerrain()->getSkyManager()->SetTime(time);
                }
            }       
#endif // USE_CAELUM
            
            if (current_actor != nullptr)
            {
                ImGui::Separator();
                ImGui::TextColored(GRAY_HINT_TEXT, "Vehicle control options:");
                DrawGCheckbox(App::io_hydro_coupling, "Keyboard steering speed coupling");
            }
            if (App::mp_state->GetActiveEnum<MpState>() == MpState::CONNECTED)
            {
                ImGui::Separator();
                ImGui::TextColored(GRAY_HINT_TEXT, "Multiplayer:");
                DrawGCheckbox(App::mp_pseudo_collisions, "Collisions");
                DrawGCheckbox(App::mp_hide_net_labels,   "Hide labels");
            }
            ImGui::PopItemWidth();
            m_open_menu_hoverbox_min = menu_pos;
            m_open_menu_hoverbox_max.x = menu_pos.x + ImGui::GetWindowWidth();
            m_open_menu_hoverbox_max.y = menu_pos.y + ImGui::GetWindowHeight();
            App::GetGuiManager()->RequestGuiCaptureKeyboard(ImGui::IsWindowHovered());
            ImGui::End();
        }
        break;

    case TopMenu::TOPMENU_TOOLS:
        menu_pos.y = window_pos.y + tools_cursor.y + MENU_Y_OFFSET;
        menu_pos.x = tools_cursor.x + window_pos.x - ImGui::GetStyle().WindowPadding.x;
        ImGui::SetNextWindowPos(menu_pos);
        if (ImGui::Begin("Tools menu", nullptr, static_cast<ImGuiWindowFlags_>(flags)))
        {
            if (ImGui::Button("Friction settings"))
            {
                App::GetGuiManager()->SetVisible_FrictionSettings(true);
                m_open_menu = TopMenu::TOPMENU_NONE;
            }

            if (ImGui::Button("Show console"))
            {
                App::GetGuiManager()->SetVisible_Console(! App::GetGuiManager()->IsVisible_Console());
                m_open_menu = TopMenu::TOPMENU_NONE;
            }

            if (ImGui::Button("Texture tool"))
            {
                App::GetGuiManager()->SetVisible_TextureToolWindow(true);
                m_open_menu = TopMenu::TOPMENU_NONE;
            }

            if (ImGui::Button("Game controls"))
            {
                App::GetGuiManager()->SetVisible_GameControls(true);
                m_open_menu = TopMenu::TOPMENU_NONE;
            }

            if (current_actor != nullptr)
            {
                if (ImGui::Button("Node / Beam utility"))
                {
                    App::GetGuiManager()->SetVisible_NodeBeamUtils(true);
                    m_open_menu = TopMenu::TOPMENU_NONE;
                }
            }

            ImGui::Separator();
            ImGui::TextColored(GRAY_HINT_TEXT, "Pre-spawn diag. options:");

            bool diag_mass = App::diag_truck_mass->GetActiveVal<bool>();
            if (ImGui::Checkbox("Node mass recalc. logging", &diag_mass))
            {
                App::diag_truck_mass->SetActiveVal(diag_mass);
            }
            if (ImGui::IsItemHovered())
            {
                ImGui::BeginTooltip();
                ImGui::Text("Extra logging on runtime - mass recalculation");
                ImGui::EndTooltip();
            }

            bool diag_break = App::diag_log_beam_break->GetActiveVal<bool>();
            if (ImGui::Checkbox("Beam break logging", &diag_break))
            {
                App::diag_log_beam_break->SetActiveVal(diag_break);
            }
            if (ImGui::IsItemHovered())
            {
                ImGui::BeginTooltip();
                ImGui::Text("Extra logging on runtime");
                ImGui::EndTooltip();
            }

            bool diag_deform = App::diag_log_beam_deform->GetActiveVal<bool>();
            if (ImGui::Checkbox("Beam deform. logging", &diag_deform))
            {
                App::diag_log_beam_deform->SetActiveVal(diag_deform);
            }
            if (ImGui::IsItemHovered())
            {
                ImGui::BeginTooltip();
                ImGui::Text("Extra logging on runtime");
                ImGui::EndTooltip();
            }

            bool diag_trig = App::diag_log_beam_trigger->GetActiveVal<bool>();
            if (ImGui::Checkbox("Trigger logging", &diag_trig))
            {
                App::diag_log_beam_trigger->SetActiveVal(diag_trig);
            }
            if (ImGui::IsItemHovered())
            {
                ImGui::BeginTooltip();
                ImGui::Text("Extra logging on runtime - trigger beams activity");
                ImGui::EndTooltip();
            }

            bool diag_vcam = App::diag_videocameras->GetActiveVal<bool>();
            if (ImGui::Checkbox("VideoCamera direction marker", &diag_vcam))
            {
                App::diag_videocameras->SetActiveVal(diag_vcam);
            }
            if (ImGui::IsItemHovered())
            {
                ImGui::BeginTooltip();
                ImGui::Text("Visual marker of VideoCameras direction");
                ImGui::EndTooltip();
            }

            if (current_actor != nullptr)
            {
                ImGui::Separator();

                ImGui::TextColored(GRAY_HINT_TEXT, "Live diagnostic views:");
                ImGui::TextColored(GRAY_HINT_TEXT, "(Toggle with '%s')", App::GetInputEngine()->getEventCommand(EV_COMMON_TOGGLE_DEBUG_VIEW).c_str());
                ImGui::TextColored(GRAY_HINT_TEXT, "(Cycle with '%s')", App::GetInputEngine()->getEventCommand(EV_COMMON_CYCLE_DEBUG_VIEWS).c_str());

                int debug_view_type = static_cast<int>(GfxActor::DebugViewType::DEBUGVIEW_NONE);
                if (current_actor != nullptr)
                {
                    debug_view_type = static_cast<int>(current_actor->GetGfxActor()->GetDebugView());
                }
                ImGui::RadioButton("Normal view",     &debug_view_type,  static_cast<int>(GfxActor::DebugViewType::DEBUGVIEW_NONE));
                ImGui::RadioButton("Skeleton view",   &debug_view_type,  static_cast<int>(GfxActor::DebugViewType::DEBUGVIEW_SKELETON));
                ImGui::RadioButton("Node details",    &debug_view_type,  static_cast<int>(GfxActor::DebugViewType::DEBUGVIEW_NODES));
                ImGui::RadioButton("Beam details",    &debug_view_type,  static_cast<int>(GfxActor::DebugViewType::DEBUGVIEW_BEAMS));
                if (current_actor->ar_num_wheels > 0)
                {
                    ImGui::RadioButton("Wheel details",   &debug_view_type,  static_cast<int>(GfxActor::DebugViewType::DEBUGVIEW_WHEELS));
                }
                if (current_actor->ar_num_shocks > 0)
                {
                    ImGui::RadioButton("Shock details",   &debug_view_type,  static_cast<int>(GfxActor::DebugViewType::DEBUGVIEW_SHOCKS));
                }
                if (current_actor->ar_num_rotators > 0)
                {
                    ImGui::RadioButton("Rotator details", &debug_view_type,  static_cast<int>(GfxActor::DebugViewType::DEBUGVIEW_ROTATORS));
                }
                if (current_actor->hasSlidenodes())
                {
                    ImGui::RadioButton("Slidenode details", &debug_view_type,  static_cast<int>(GfxActor::DebugViewType::DEBUGVIEW_SLIDENODES));
                }
                if (current_actor->ar_num_cabs > 0)
                {
                    ImGui::RadioButton("Submesh details", &debug_view_type,  static_cast<int>(GfxActor::DebugViewType::DEBUGVIEW_SUBMESH));
                }

                if ((current_actor != nullptr) && (debug_view_type != static_cast<int>(current_actor->GetGfxActor()->GetDebugView())))
                {
                    current_actor->GetGfxActor()->SetDebugView(static_cast<GfxActor::DebugViewType>(debug_view_type));
                }

                if (debug_view_type >= 1 && debug_view_type <= static_cast<int>(GfxActor::DebugViewType::DEBUGVIEW_BEAMS)) 
                {
                    ImGui::Separator();
                    ImGui::TextColored(GRAY_HINT_TEXT,           "Settings:");
                    DrawGCheckbox(App::diag_hide_broken_beams,   "Hide broken beams");
                    DrawGCheckbox(App::diag_hide_beam_stress,    "Hide beam stress");
                    DrawGCheckbox(App::diag_hide_wheels,         "Hide wheels");
                    DrawGCheckbox(App::diag_hide_nodes,          "Hide nodes");
                    if (debug_view_type >= 2)
                    {
                        DrawGCheckbox(App::diag_hide_wheel_info, "Hide wheel info");
                    }
                }
            }

            m_open_menu_hoverbox_min = menu_pos;
            m_open_menu_hoverbox_max.x = menu_pos.x + ImGui::GetWindowWidth();
            m_open_menu_hoverbox_max.y = menu_pos.y + ImGui::GetWindowHeight();
            App::GetGuiManager()->RequestGuiCaptureKeyboard(ImGui::IsWindowHovered());
            ImGui::End();
        }
        break;

    default:
        m_open_menu_hoverbox_min = ImVec2(0,0);
        m_open_menu_hoverbox_max = ImVec2(0,0);
    }

    ImGui::PopStyleColor(2); // WindowBg, Button
}

bool RoR::GUI::TopMenubar::ShouldDisplay(ImVec2 window_pos)
{
    ImVec2 box_min(0,0);
    ImVec2 box_max(ImGui::GetIO().DisplaySize.x, ImGui::GetStyle().WindowPadding.y + PANEL_HOVERBOX_HEIGHT);
    ImVec2 mouse_pos = ImGui::GetIO().MousePos;
    bool window_hovered ((mouse_pos.x >= box_min.x) && (mouse_pos.x <= box_max.x) &&
                            (mouse_pos.y >= box_min.y) && (mouse_pos.y <= box_max.y));

    if (m_open_menu == TopMenu::TOPMENU_NONE)
    {
        return window_hovered;
    }

    bool menu_hovered ((mouse_pos.x >= m_open_menu_hoverbox_min.x) && (mouse_pos.x <= m_open_menu_hoverbox_max.x) &&
                        (mouse_pos.y >= m_open_menu_hoverbox_min.y) && (mouse_pos.y <= m_open_menu_hoverbox_max.y));
    return (menu_hovered || window_hovered);
}

void RoR::GUI::TopMenubar::DrawMpUserToActorList(RoRnet::UserInfo &user)
{
    // Count actors owned by the player
    unsigned int num_actors_player = 0;
    for (Actor* actor : App::GetSimController()->GetActors())
    {
        if (actor->ar_net_source_id == user.uniqueid)
        {
            ++num_actors_player;
        }
    }

    // Prepare user info text
    const char* user_type_str = "";
         if (user.authstatus & RoRnet::AUTH_BOT)     { user_type_str = "Bot, ";       } // Old coloring: #0000c9
    else if (user.authstatus & RoRnet::AUTH_BANNED)  { user_type_str = "Banned, ";    } // Old coloring: none
    else if (user.authstatus & RoRnet::AUTH_RANKED)  { user_type_str = "Ranked, ";    } // Old coloring: #00c900
    else if (user.authstatus & RoRnet::AUTH_MOD)     { user_type_str = "Moderator, "; } // Old coloring: #c90000
    else if (user.authstatus & RoRnet::AUTH_ADMIN)   { user_type_str = "Admin, ";     } // Old coloring: #c97100

    char usertext_buf[400];
    snprintf(usertext_buf, 400, "%s: %u (%sVer: %s, Lang: %s)",
        user.username, num_actors_player, user_type_str, user.clientversion, user.language);

    // Display user in list
    Ogre::ColourValue player_color;
#ifdef USE_SOCKETW
    player_color = RoR::Networking::GetPlayerColor(user.colournum);
#endif
    ImVec4 player_gui_color(player_color.r, player_color.g, player_color.b, 1.f);
    ImGui::PushStyleColor(ImGuiCol_Text, player_gui_color);
    ImGui::Text("%s", usertext_buf);
    ImGui::PopStyleColor();

    // Display actor list
    int i = 0;
    for (auto actor : App::GetSimController()->GetActors())
    {
        if ((!actor->ar_hide_in_actor_list) && (actor->ar_net_source_id == user.uniqueid))
        {
            char actortext_buf[400];
            snprintf(actortext_buf, 400, "  + %s (%s) ##[%d:%d]", actor->ar_design_name.c_str(), actor->ar_filename.c_str(), i++, user.uniqueid);
            if (ImGui::Button(actortext_buf)) // Button clicked?
            {
                App::GetSimController()->SetPendingPlayerActor(actor);
            }
        }
    }
}

void RoR::GUI::TopMenubar::DrawActorListSinglePlayer()
{
    std::vector<Actor*> actor_list;
    for (auto actor : App::GetSimController()->GetActors())
    {
        if (!actor->ar_hide_in_actor_list)
        {
            actor_list.emplace_back(actor);
        }
    }
    if (actor_list.empty())
    {
        ImGui::PushStyleColor(ImGuiCol_Text, GRAY_HINT_TEXT);
        ImGui::Text("None spawned yet");
        ImGui::Text("Use [Simulation] menu");
        ImGui::PopStyleColor();
    }
    else
    {
        Actor* player_actor = App::GetSimController()->GetPlayerActor();
        int i = 0;
        for (auto actor : actor_list)
        {
            char text_buf_rem[200];
            snprintf(text_buf_rem, 200, "X" "##[%d]", i);
            ImGui::PushStyleColor(ImGuiCol_Text, RED_TEXT);
            if (ImGui::Button(text_buf_rem))
            {
                App::GetSimController()->QueueActorRemove(actor);
            }
            ImGui::PopStyleColor();
            ImGui::SameLine();

            char text_buf[200];
            snprintf(text_buf, 200, "[%d] %s", i++, actor->ar_design_name.c_str());
            auto linked_actors = actor->GetAllLinkedActors();
            if (actor == player_actor)
            {
                ImGui::PushStyleColor(ImGuiCol_Text, GREEN_TEXT);
            }
            else if (std::find(linked_actors.begin(), linked_actors.end(), player_actor) != linked_actors.end())
            {
                ImGui::PushStyleColor(ImGuiCol_Text, ORANGE_TEXT);
            }
            else if (actor->ar_sim_state == Actor::SimState::LOCAL_SIMULATED)
            {
                ImGui::PushStyleColor(ImGuiCol_Text, WHITE_TEXT);
            }
            else
            {
                ImGui::PushStyleColor(ImGuiCol_Text, GRAY_HINT_TEXT);
            }
            if (ImGui::Button(text_buf)) // Button clicked?
            {
                App::GetSimController()->SetPendingPlayerActor(actor);
            }
            ImGui::PopStyleColor();
        }
    }
}

void RoR::GUI::TopMenubar::DrawSpecialStateBox(float top_offset)
{
    std::string special_text;
    ImVec4 special_color;

    // Gather state info
    if (App::GetSimController()->GetPhysicsPaused() && !RoR::App::GetSimController()->IsGUIHidden())
    {
        special_color = ORANGE_TEXT;
        special_text = Ogre::StringUtil::replaceAll(_L("All physics paused, press '{}' to resume"),
            "{}", App::GetInputEngine()->getEventCommand(EV_COMMON_TOGGLE_PHYSICS));
    }
    else if (App::GetSimController()->GetPlayerActor() &&
             App::GetSimController()->GetPlayerActor()->ar_physics_paused
             && !RoR::App::GetSimController()->IsGUIHidden())
    {
        special_color = GREEN_TEXT;
        special_text = Ogre::StringUtil::replaceAll(_L("Vehicle physics paused, press '{}' to resume"),
            "{}", App::GetInputEngine()->getEventCommand(EV_TRUCK_TOGGLE_PHYSICS));
    }

    // Draw box if needed
    if (!special_text.empty())
    {
        ImVec2 box_pos;
        box_pos.y = top_offset;
        box_pos.x = (ImGui::GetIO().DisplaySize.x / 2) - ((ImGui::CalcTextSize(special_text.c_str()).x / 2) + ImGui::GetStyle().FramePadding.x);
        ImGui::SetNextWindowPos(box_pos);
        ImGuiWindowFlags flags = ImGuiWindowFlags_NoResize    | ImGuiWindowFlags_NoMove |
                                 ImGuiWindowFlags_NoTitleBar  | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize;
        ImGui::PushStyleColor(ImGuiCol_WindowBg, App::GetGuiManager()->GetTheme().semitransparent_window_bg);
        if (ImGui::Begin("Special state box", nullptr, flags))
        {
            ImGui::TextColored(special_color, "%s", special_text.c_str());
            ImGui::End();
        }
        ImGui::PopStyleColor(1); // WindowBg
    }
}
