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

#include "GUI_TopMenubar.h"

#include "Application.h"
#include "Actor.h"
#include "ActorManager.h"
#include "CameraManager.h"
#include "GameContext.h"
#include "GfxScene.h"
#include "GUIManager.h"
#include "GUIUtils.h"
#include "GUI_MainSelector.h"
#include "GUI_SurveyMap.h"
#include "InputEngine.h"
#include "Language.h"
#include "Network.h"
#include "PlatformUtils.h"
#include "Replay.h"
#include "SkyManager.h"
#include "Terrain.h"
#include "Water.h"
#include "ScriptEngine.h"
#include "Console.h"
#include "ContentManager.h"

#include <algorithm>
#include <fmt/format.h>


#ifdef USE_CURL
#   include <curl/curl.h>
#   include <curl/easy.h>
#endif //USE_CURL

#if defined(_MSC_VER) && defined(GetObject) // This MS Windows macro from <wingdi.h> (Windows Kit 8.1) clashes with RapidJSON
#   undef GetObject
#endif

using namespace RoR;
using namespace GUI;

#if defined(USE_CURL)

static size_t CurlWriteFunc(void *ptr, size_t size, size_t nmemb, std::string* data)
{
    data->append((char*)ptr, size * nmemb);
    return size * nmemb;
}

void GetJson()
{
    std::string url = "https://raw.githubusercontent.com/RigsOfRods-Community/ai-waypoints/main/waypoints.json";
    std::string response_payload;
    long response_code = 0;

    CURL *curl = curl_easy_init();
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_IPRESOLVE, CURL_IPRESOLVE_V4);
#ifdef _WIN32
    curl_easy_setopt(curl, CURLOPT_SSL_OPTIONS, CURLSSLOPT_NATIVE_CA);
#endif // _WIN32
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, CurlWriteFunc);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_payload);

    curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);

    curl_easy_cleanup(curl);
    curl = nullptr;

    if (response_code == 200)
    {
        Message m(MSG_NET_REFRESH_AI_PRESETS);
        m.description = response_payload;
        App::GetGameContext()->PushMessage(m);
    }
}

#endif // defined(USE_CURL)

void TopMenubar::Update()
{
    // ## ImGui's 'menubar' and 'menuitem' features won't quite cut it...
    // ## Let's do our own menus and menuitems using buttons and coloring tricks.

    GUIManager::GuiTheme const& theme = App::GetGuiManager()->GetTheme();

    int num_playable_actors = 0;
    for (ActorPtr& actor: App::GetGameContext()->GetActorManager()->GetActors())
    {
        if (!actor->ar_hide_in_actor_list)
        {
            num_playable_actors++;
        }
    }

    std::string sim_title =         _LC("TopMenubar", "Simulation");
    std::string actors_title = fmt::format("{} ({})", _LC("TopMenubar", "Vehicles"), num_playable_actors);
    std::string savegames_title =   _LC("TopMenubar", "Saves");
    std::string settings_title =    _LC("TopMenubar", "Settings");
    std::string tools_title =       _LC("TopMenubar", "Tools");
    std::string ai_title =          _LC("TopMenubar", "Vehicle AI");

    int NUM_BUTTONS = 5;
    if (App::mp_state->getEnum<MpState>() != MpState::CONNECTED)
    {
        NUM_BUTTONS = 6;
    }

    float menubar_content_width =
        (ImGui::GetStyle().ItemSpacing.x * (NUM_BUTTONS - 1)) +
        (ImGui::GetStyle().FramePadding.x * (NUM_BUTTONS * 2)) +
        ImGui::CalcTextSize(sim_title.c_str()).x +
        ImGui::CalcTextSize(actors_title.c_str()).x +
        ImGui::CalcTextSize(savegames_title.c_str()).x +
        ImGui::CalcTextSize(settings_title.c_str()).x +
        ImGui::CalcTextSize(tools_title.c_str()).x;

    if (App::mp_state->getEnum<MpState>() != MpState::CONNECTED)
    {
        menubar_content_width += ImGui::CalcTextSize(ai_title.c_str()).x;
    }

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

    if (ImGui::IsWindowHovered())
    {
        ai_menu = false;
    }

    // The 'simulation' button
    ImVec2 window_pos = ImGui::GetWindowPos();
    ImVec2 sim_cursor = ImGui::GetCursorPos();
    ImGui::Button(sim_title.c_str());
    if ((m_open_menu != TopMenu::TOPMENU_SIM) && ImGui::IsItemHovered())
    {
        m_open_menu = TopMenu::TOPMENU_SIM;
    }

    // The 'AI' button
    ImVec2 ai_cursor = ImVec2(0, 0);

    if (App::mp_state->getEnum<MpState>() != MpState::CONNECTED)
    {
        ImGui::SameLine();
        ai_cursor = ImGui::GetCursorPos();
        ImGui::Button(ai_title.c_str());
        if ((m_open_menu != TopMenu::TOPMENU_AI) && ImGui::IsItemHovered())
        {
            m_open_menu = TopMenu::TOPMENU_AI;
        }
    }

    ImGui::SameLine();

    // The 'vehicles' button
    ImVec2 actors_cursor = ImGui::GetCursorPos();
    ImGui::Button(actors_title.c_str());
    if ((m_open_menu != TopMenu::TOPMENU_ACTORS) && ImGui::IsItemHovered())
    {
        m_open_menu = TopMenu::TOPMENU_ACTORS;
    }

    ImGui::SameLine();

    // The 'savegames' button
    ImVec2 savegames_cursor = ImGui::GetCursorPos();
    ImGui::Button(savegames_title.c_str());
    if ((m_open_menu != TopMenu::TOPMENU_SAVEGAMES) && ImGui::IsItemHovered())
    {
        m_open_menu = TopMenu::TOPMENU_SAVEGAMES;
        m_quicksave_name = App::GetGameContext()->GetQuicksaveFilename();
        m_quickload = FileExists(PathCombine(App::sys_savegames_dir->getStr(), m_quicksave_name));
        m_savegame_names.clear();
        for (int i = 0; i <= 9; i++)
        {
            Ogre::String filename = Ogre::StringUtil::format("quicksave-%d.sav", i);
            m_savegame_names.push_back(App::GetGameContext()->ExtractSceneName(filename));
        }
    }

    ImGui::SameLine();

    // The 'settings' button
    ImVec2 settings_cursor = ImGui::GetCursorPos();
    ImGui::Button(settings_title.c_str());
    if ((m_open_menu != TopMenu::TOPMENU_SETTINGS) && ImGui::IsItemHovered())
    {
        m_open_menu = TopMenu::TOPMENU_SETTINGS;
#ifdef USE_CAELUM
        if (App::gfx_sky_mode->getEnum<GfxSkyMode>() == GfxSkyMode::CAELUM)
            m_daytime = App::GetGameContext()->GetTerrain()->getSkyManager()->GetTime();
#endif // USE_CAELUM
    }

    ImGui::SameLine();

    // The 'tools' button
    ImVec2 tools_cursor = ImGui::GetCursorPos();
    ImGui::Button(tools_title.c_str());
    if ((m_open_menu != TopMenu::TOPMENU_TOOLS) && ImGui::IsItemHovered())
    {
        m_open_menu = TopMenu::TOPMENU_TOOLS;
    }

    ImVec2 topmenu_final_size = ImGui::GetWindowSize();
    App::GetGuiManager()->RequestGuiCaptureKeyboard(ImGui::IsWindowHovered());
    ImGui::End();

    this->DrawSpecialStateBox(window_target_pos.y + topmenu_final_size.y + 10.f);

    ImVec2 menu_pos;
    ActorPtr current_actor = App::GetGameContext()->GetPlayerActor();
    switch (m_open_menu)
    {
    case TopMenu::TOPMENU_SIM:
        menu_pos.y = window_pos.y + sim_cursor.y + MENU_Y_OFFSET;
        menu_pos.x = sim_cursor.x + window_pos.x - ImGui::GetStyle().WindowPadding.x;
        ImGui::SetNextWindowPos(menu_pos);
        if (ImGui::Begin(_LC("TopMenubar", "Sim menu"), nullptr, static_cast<ImGuiWindowFlags_>(flags)))
        {
            // TODO: Display hotkeys on the right side of the menu (with different text color)

            if (ImGui::Button(_LC("TopMenubar", "Get new vehicle")))
            {
                m_open_menu = TopMenu::TOPMENU_NONE;

                RoR::Message m(MSG_GUI_OPEN_SELECTOR_REQUESTED);
                m.payload = reinterpret_cast<void*>(new LoaderType(LT_AllBeam));
                App::GetGameContext()->PushMessage(m);
            }

            if (current_actor != nullptr)
            {
                if (ImGui::Button(_LC("TopMenubar", "Show vehicle description")))
                {
                    App::GetGuiManager()->VehicleDescription.SetVisible(true);
                }

                if (current_actor->ar_state != ActorState::NETWORKED_OK)
                {
                    if (ImGui::Button(_LC("TopMenubar", "Reload current vehicle")))
                    {
                        ActorModifyRequest* rq = new ActorModifyRequest;
                        rq->amr_type = ActorModifyRequest::Type::RELOAD;
                        rq->amr_actor = current_actor->ar_instance_id;
                        App::GetGameContext()->PushMessage(Message(MSG_SIM_MODIFY_ACTOR_REQUESTED, (void*)rq));
                    }

                    if (ImGui::Button(_LC("TopMenubar", "Remove current vehicle")))
                    {
                        App::GetGameContext()->PushMessage(Message(MSG_SIM_DELETE_ACTOR_REQUESTED, static_cast<void*>(new ActorPtr(current_actor))));
                    }
                }
            }
            else if (App::GetGameContext()->GetLastSpawnedActor())
            {
                if (ImGui::Button(_LC("TopMenubar", "Activate last spawned vehicle")))
                {
                    ActorModifyRequest* rq = new ActorModifyRequest;
                    rq->amr_type = ActorModifyRequest::Type::WAKE_UP;
                    rq->amr_actor = App::GetGameContext()->GetLastSpawnedActor()->ar_instance_id;
                    App::GetGameContext()->PushMessage(Message(MSG_SIM_MODIFY_ACTOR_REQUESTED, (void*)rq));
                }

                if (ImGui::Button(_LC("TopMenubar", "Reload last spawned vehicle")))
                {
                    ActorModifyRequest* rq = new ActorModifyRequest;
                    rq->amr_type = ActorModifyRequest::Type::RELOAD;
                    rq->amr_actor = App::GetGameContext()->GetLastSpawnedActor()->ar_instance_id;
                    App::GetGameContext()->PushMessage(Message(MSG_SIM_MODIFY_ACTOR_REQUESTED, (void*)rq));
                }

                if (ImGui::Button(_LC("TopMenubar", "Remove last spawned vehicle")))
                {
                    ActorPtr actor = App::GetGameContext()->GetLastSpawnedActor();
                    App::GetGameContext()->PushMessage(Message(MSG_SIM_DELETE_ACTOR_REQUESTED, static_cast<void*>(new ActorPtr(actor))));
                }
            }

            if (!App::GetGameContext()->GetActorManager()->GetLocalActors().empty())
            {
                if (ImGui::Button(_LC("TopMenubar", "Remove all vehicles")))
                {
                    m_confirm_remove_all = true;
                }
                if (m_confirm_remove_all)
                {
                    ImGui::PushStyleColor(ImGuiCol_Text, ORANGE_TEXT);
                    if (ImGui::Button(_LC("TopMenubar", " [!] Confirm removal")))
                    {
                        for (ActorPtr actor : App::GetGameContext()->GetActorManager()->GetLocalActors())
                        {
                            if (!actor->ar_hide_in_actor_list && !actor->isPreloadedWithTerrain() && 
                                    actor->ar_state != ActorState::NETWORKED_OK)
                            {
                                App::GetGameContext()->PushMessage(Message(MSG_SIM_DELETE_ACTOR_REQUESTED, static_cast<void*>(new ActorPtr(actor))));
                            }
                        }
                        m_confirm_remove_all = false;
                    }
                    ImGui::PopStyleColor();
                }

                if (ImGui::Button(_LC("TopMenubar", "Activate all vehicles")))
                {
                    App::GetGameContext()->GetActorManager()->WakeUpAllActors();
                }

                bool force_trucks_active = App::GetGameContext()->GetActorManager()->AreTrucksForcedAwake();
                if (ImGui::Checkbox(_LC("TopMenubar", "Activated vehicles never sleep"), &force_trucks_active))
                {
                    App::GetGameContext()->GetActorManager()->SetTrucksForcedAwake(force_trucks_active);
                }

                if (ImGui::Button(_LC("TopMenubar", "Send all vehicles to sleep")))
                {
                    App::GetGameContext()->GetActorManager()->SendAllActorsSleeping();
                }
            }

            if (App::mp_state->getEnum<MpState>() != MpState::CONNECTED)
            {
                if (ImGui::Button(_LC("TopMenubar", "Reload current terrain")))
                {
                    App::GetGameContext()->PushMessage(Message(MsgType::MSG_SIM_UNLOAD_TERRN_REQUESTED));
                    // Order is required - create chain.
                    App::GetGameContext()->ChainMessage(Message(MsgType::MSG_EDI_RELOAD_BUNDLE_REQUESTED,
                        (void*)App::GetGameContext()->GetTerrain()->getCacheEntry()));
                    App::GetGameContext()->ChainMessage(Message(MsgType::MSG_SIM_LOAD_TERRN_REQUESTED,
                        App::GetGameContext()->GetTerrain()->getCacheEntry()->fname));
                }
            }

            ImGui::Separator();

            if (ImGui::Button(_LC("TopMenubar", "Back to menu")))
            {
                if (App::mp_state->getEnum<MpState>() == MpState::CONNECTED)
                {
                    App::GetGameContext()->PushMessage(Message(MSG_NET_DISCONNECT_REQUESTED));
                }
                App::GetGameContext()->PushMessage(Message(MSG_SIM_UNLOAD_TERRN_REQUESTED));
                App::GetGameContext()->PushMessage(Message(MSG_GUI_OPEN_MENU_REQUESTED));
            }

            if (ImGui::Button(_LC("TopMenubar", "Exit")))
            {
                App::GetGameContext()->PushMessage(Message(MSG_APP_SHUTDOWN_REQUESTED));
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
        if (ImGui::Begin(_LC("TopMenubar", "Actors menu"), nullptr, static_cast<ImGuiWindowFlags_>(flags)))
        {
            if (App::mp_state->getEnum<MpState>() != MpState::CONNECTED)
            {
                this->DrawActorListSinglePlayer();
            }
            else
            {
#ifdef USE_SOCKETW
                RoRnet::UserInfo net_user_info = App::GetNetwork()->GetLocalUserData();
                this->DrawMpUserToActorList(net_user_info);

                std::vector<RoRnet::UserInfo> remote_users = App::GetNetwork()->GetUserInfos();
                for (auto& user: remote_users)
                {
                    this->DrawMpUserToActorList(user);
                }
#endif // USE_SOCKETW
            }
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
        if (ImGui::Begin(_LC("TopMenubar", "Savegames"), nullptr, static_cast<ImGuiWindowFlags_>(flags)))
        {
            if (ImGui::Button(_LC("TopMenubar", "Quicksave")))
            {
                App::GetGameContext()->GetActorManager()->SaveScene(m_quicksave_name);
                m_open_menu = TopMenu::TOPMENU_NONE;
            }
            ImGui::SameLine();
            ImGui::TextColored(GRAY_HINT_TEXT, "(NUMPAD: /)");

            if (m_quickload)
            {
                if (ImGui::Button(_LC("TopMenubar", "Quickload")))
                {
                    App::GetGameContext()->GetActorManager()->LoadScene(m_quicksave_name);
                    m_open_menu = TopMenu::TOPMENU_NONE;
                }
                ImGui::SameLine();
                ImGui::TextColored(GRAY_HINT_TEXT, "(NUMPAD: *)");
            }

            ImGui::Separator();

            ImGui::TextColored(GRAY_HINT_TEXT, "%s", _LC("TopMenubar", "(Save with CTRL+ALT+1..5)"));
            for (int i = 1; i <= 5; i++)
            {
                Ogre::String name = _LC("TopMenubar", "Empty Slot");
                if (!m_savegame_names[i].empty())
                {
                    name = m_savegame_names[i];
                }
                Ogre::String caption = Ogre::StringUtil::format("%d. %s##Save", i, name.c_str());
                if (ImGui::Button(caption.c_str()))
                {
                    Ogre::String filename = Ogre::StringUtil::format("quicksave-%d.sav", i);
                    App::GetGameContext()->GetActorManager()->SaveScene(filename);
                    m_open_menu = TopMenu::TOPMENU_NONE;
                }
            }

            ImGui::TextColored(GRAY_HINT_TEXT, "%s", _LC("TopMenubar", "(Load with ALT+1..5)"));
            for (int i = 1; i <= 5; i++)
            {
                if (!m_savegame_names[i].empty())
                {
                    Ogre::String name = m_savegame_names[i];
                    Ogre::String caption = Ogre::StringUtil::format("%d. %s##Load", i, name.c_str());
                    if (ImGui::Button(caption.c_str()))
                    {
                        Ogre::String filename = Ogre::StringUtil::format("quicksave-%d.sav", i);
                        App::GetGameContext()->PushMessage(RoR::Message(MSG_SIM_LOAD_SAVEGAME_REQUESTED, filename));
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
        if (ImGui::Begin(_LC("TopMenubar", "Settings menu"), nullptr, static_cast<ImGuiWindowFlags_>(flags)))
        {
            ImGui::PushItemWidth(125.f); // Width includes [+/-] buttons
            ImGui::TextColored(GRAY_HINT_TEXT, "%s", _LC("TopMenubar",  "Audio:"));
            DrawGFloatSlider(App::audio_master_volume, _LC("TopMenubar", "Volume"), 0, 1);
            ImGui::Separator();
            ImGui::TextColored(GRAY_HINT_TEXT, "%s", _LC("TopMenubar", "Frames per second:"));
            if (App::gfx_envmap_enabled->getBool())
            {
                DrawGIntSlider(App::gfx_envmap_rate, _LC("TopMenubar", "Reflections"), 0, 6);
            }
            DrawGIntSlider(App::gfx_fps_limit, _LC("TopMenubar", "Game"), 0, 240);

            ImGui::Separator();
            ImGui::TextColored(GRAY_HINT_TEXT, "%s", _LC("TopMenubar", "Simulation:"));
            float slowmotion = std::min(App::GetGameContext()->GetActorManager()->GetSimulationSpeed(), 1.0f);
            if (ImGui::SliderFloat(_LC("TopMenubar", "Slow motion"), &slowmotion, 0.01f, 1.0f))
            {
                App::GetGameContext()->GetActorManager()->SetSimulationSpeed(slowmotion);
            }
            float timelapse = std::max(App::GetGameContext()->GetActorManager()->GetSimulationSpeed(), 1.0f);
            if (ImGui::SliderFloat(_LC("TopMenubar", "Time lapse"), &timelapse, 1.0f, 10.0f))
            {
                App::GetGameContext()->GetActorManager()->SetSimulationSpeed(timelapse);
            }
            if (App::GetCameraManager()->GetCurrentBehavior() == CameraManager::CAMERA_BEHAVIOR_STATIC)
            {
                ImGui::Separator();
                ImGui::TextColored(GRAY_HINT_TEXT, "%s", _LC("TopMenubar", "Camera:"));
                DrawGFloatSlider(App::gfx_static_cam_fov_exp, _LC("TopMenubar", "FOV"), 0.8f, 1.5f);
                DrawGIntSlider(App::gfx_camera_height, _LC("TopMenubar", "Height"), 1, 50);
            }
            else
            {
                ImGui::Separator();
                ImGui::TextColored(GRAY_HINT_TEXT, "%s", _LC("TopMenubar", "Camera:"));
                if (App::GetCameraManager()->GetCurrentBehavior() == CameraManager::CAMERA_BEHAVIOR_VEHICLE_CINECAM)
                {
                    int fov = App::gfx_fov_internal->getInt();
                    if (ImGui::SliderInt(_LC("TopMenubar", "FOV"), &fov, 10, 120))
                    {
                        App::gfx_fov_internal->setVal(fov);
                    }
                }
                else
                {
                    int fov = App::gfx_fov_external->getInt();
                    if (ImGui::SliderInt(_LC("TopMenubar", "FOV"), &fov, 10, 120))
                    {
                        App::gfx_fov_external->setVal(fov);
                    }
                }
                if (App::GetCameraManager()->GetCurrentBehavior() == CameraManager::CAMERA_BEHAVIOR_FIXED)
                {
                    DrawGCheckbox(App::gfx_fixed_cam_tracking, _LC("TopMenubar", "Tracking"));
                }
            }
#ifdef USE_CAELUM
            if (App::gfx_sky_mode->getEnum<GfxSkyMode>() == GfxSkyMode::CAELUM)
            {
                ImGui::Separator();
                ImGui::TextColored(GRAY_HINT_TEXT, "%s", _LC("TopMenubar", "Time of day:"));
                float time = App::GetGameContext()->GetTerrain()->getSkyManager()->GetTime();
                if (ImGui::SliderFloat("", &time, m_daytime - 0.5f, m_daytime + 0.5f, ""))
                {
                    App::GetGameContext()->GetTerrain()->getSkyManager()->SetTime(time);
                }
                ImGui::SameLine();
                DrawGCheckbox(App::gfx_sky_time_cycle, _LC("TopMenubar", "Cycle"));
                if (App::gfx_sky_time_cycle->getBool())
                {
                    DrawGIntSlider(App::gfx_sky_time_speed, _LC("TopMenubar", "Speed"), 10, 2000);
                }
            }       
#endif // USE_CAELUM
            if (RoR::App::gfx_water_waves->getBool() && App::mp_state->getEnum<MpState>() != MpState::CONNECTED && App::GetGameContext()->GetTerrain()->getWater())
            {
                if (App::gfx_water_mode->getEnum<GfxWaterMode>() != GfxWaterMode::HYDRAX && App::gfx_water_mode->getEnum<GfxWaterMode>() != GfxWaterMode::NONE)
                {
                    ImGui::PushID("waves");
                    ImGui::TextColored(GRAY_HINT_TEXT, "%s", _LC("TopMenubar", "Waves Height:"));
                    if(ImGui::SliderFloat("", &m_waves_height, 0.f, 4.f, ""))
                    {
                        App::GetGameContext()->GetTerrain()->getWater()->SetWavesHeight(m_waves_height);
                    }
                    ImGui::PopID();
                }
            }    
            
            if (current_actor != nullptr)
            {
                ImGui::Separator();
                ImGui::TextColored(GRAY_HINT_TEXT, "%s", _LC("TopMenubar",  "Vehicle control options:"));
                DrawGCheckbox(App::io_hydro_coupling, _LC("TopMenubar", "Keyboard steering speed coupling"));
            }
            if (App::mp_state->getEnum<MpState>() == MpState::CONNECTED)
            {
                ImGui::Separator();
                ImGui::TextColored(GRAY_HINT_TEXT, "%s", _LC("TopMenubar", "Multiplayer:"));
                DrawGCheckbox(App::mp_pseudo_collisions, _LC("TopMenubar", "Collisions"));
                DrawGCheckbox(App::mp_hide_net_labels,   _LC("TopMenubar", "Hide labels"));
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
        if (ImGui::Begin(_LC("TopMenubar", "Tools menu"), nullptr, static_cast<ImGuiWindowFlags_>(flags)))
        {
            if (ImGui::Button(_LC("TopMenubar", "Friction settings")))
            {
                App::GetGuiManager()->FrictionSettings.SetVisible(true);
                m_open_menu = TopMenu::TOPMENU_NONE;
            }

            if (ImGui::Button(_LC("TopMenubar", "Show console")))
            {
                App::GetGuiManager()->ConsoleWindow.SetVisible(!App::GetGuiManager()->ConsoleWindow.IsVisible());
                m_open_menu = TopMenu::TOPMENU_NONE;
            }

            if (ImGui::Button(_LC("TopMenubar", "Texture tool")))
            {
                App::GetGuiManager()->TextureToolWindow.SetVisible(true);
                m_open_menu = TopMenu::TOPMENU_NONE;
            }

            if (ImGui::Button(_LC("TopMenubar", "Collisions debug")))
            {
                App::GetGuiManager()->CollisionsDebug.SetVisible(true);
                m_open_menu = TopMenu::TOPMENU_NONE;
            }

            if (current_actor != nullptr)
            {
                if (ImGui::Button(_LC("TopMenubar", "Node / Beam utility")))
                {
                    App::GetGuiManager()->NodeBeamUtils.SetVisible(true);
                    m_open_menu = TopMenu::TOPMENU_NONE;
                }

                if (ImGui::Button(_LC("TopMenubar", "FlexBody debug")))
                {
                    App::GetGuiManager()->FlexbodyDebug.SetVisible(true);
                    m_open_menu = TopMenu::TOPMENU_NONE;
                }
            }

            ImGui::Separator();
            ImGui::TextColored(GRAY_HINT_TEXT, "%s", _LC("TopMenubar", "Pre-spawn diag. options:"));

            bool diag_mass = App::diag_truck_mass->getBool();
            if (ImGui::Checkbox(_LC("TopMenubar", "Node mass recalc. logging"), &diag_mass))
            {
                App::diag_truck_mass->setVal(diag_mass);
            }
            if (ImGui::IsItemHovered())
            {
                ImGui::BeginTooltip();
                ImGui::Text("%s", _LC("TopMenubar", "Extra logging on runtime - mass recalculation"));
                ImGui::EndTooltip();
            }

            bool diag_break = App::diag_log_beam_break->getBool();
            if (ImGui::Checkbox(_LC("TopMenubar", "Beam break logging"), &diag_break))
            {
                App::diag_log_beam_break->setVal(diag_break);
            }
            if (ImGui::IsItemHovered())
            {
                ImGui::BeginTooltip();
                ImGui::Text("%s", _LC("TopMenubar", "Extra logging on runtime"));
                ImGui::EndTooltip();
            }

            bool diag_deform = App::diag_log_beam_deform->getBool();
            if (ImGui::Checkbox(_LC("TopMenubar", "Beam deform. logging"), &diag_deform))
            {
                App::diag_log_beam_deform->setVal(diag_deform);
            }
            if (ImGui::IsItemHovered())
            {
                ImGui::BeginTooltip();
                ImGui::Text("%s", _LC("TopMenubar", "Extra logging on runtime"));
                ImGui::EndTooltip();
            }

            bool diag_trig = App::diag_log_beam_trigger->getBool();
            if (ImGui::Checkbox(_LC("TopMenubar", "Trigger logging"), &diag_trig))
            {
                App::diag_log_beam_trigger->setVal(diag_trig);
            }
            if (ImGui::IsItemHovered())
            {
                ImGui::BeginTooltip();
                ImGui::Text("%s", _LC("TopMenubar", "Extra logging on runtime - trigger beams activity"));
                ImGui::EndTooltip();
            }

            bool diag_vcam = App::diag_videocameras->getBool();
            if (ImGui::Checkbox(_LC("TopMenubar", "VideoCamera direction marker"), &diag_vcam))
            {
                App::diag_videocameras->setVal(diag_vcam);
            }
            if (ImGui::IsItemHovered())
            {
                ImGui::BeginTooltip();
                ImGui::Text("%s", _LC("TopMenubar", "Visual marker of VideoCameras direction"));
                ImGui::EndTooltip();
            }

            ImGui::PushItemWidth(125.f); // Width includes [+/-] buttons
            ImGui::Separator();
            ImGui::TextColored(GRAY_HINT_TEXT, _LC("TopMenubar", "Visual options:"));
            DrawGIntSlider(App::gfx_polygon_mode, _LC("TopMenubar", "Polygon mode"), 1, 3);
            if (ImGui::IsItemHovered())
            {
                ImGui::BeginTooltip();
                ImGui::Text("%s", _LC("TopMenubar", "1 = Solid"));
                ImGui::Text("%s", _LC("TopMenubar", "2 = Wireframe"));
                ImGui::Text("%s", _LC("TopMenubar", "3 = Points"));
                ImGui::EndTooltip();
            }

            if (current_actor != nullptr)
            {
                ImGui::Separator();

                ImGui::TextColored(GRAY_HINT_TEXT, "%s", _LC("TopMenubar", "Live diagnostic views:"));
                ImGui::TextColored(GRAY_HINT_TEXT, "%s", _LC("TopMenubar", "(Toggle with %s)"), App::GetInputEngine()->getEventCommandTrimmed(EV_COMMON_TOGGLE_DEBUG_VIEW).c_str());
                ImGui::TextColored(GRAY_HINT_TEXT, "%s", _LC("TopMenubar", "(Cycle with %s)"), App::GetInputEngine()->getEventCommandTrimmed(EV_COMMON_CYCLE_DEBUG_VIEWS).c_str());

                int debug_view_type = static_cast<int>(DebugViewType::DEBUGVIEW_NONE);
                if (current_actor != nullptr)
                {
                    debug_view_type = static_cast<int>(current_actor->GetGfxActor()->GetDebugView());
                }
                ImGui::RadioButton(_LC("TopMenubar", "Normal view"),     &debug_view_type,  static_cast<int>(DebugViewType::DEBUGVIEW_NONE));
                ImGui::RadioButton(_LC("TopMenubar", "Skeleton view"),   &debug_view_type,  static_cast<int>(DebugViewType::DEBUGVIEW_SKELETON));
                ImGui::RadioButton(_LC("TopMenubar", "Node details"),    &debug_view_type,  static_cast<int>(DebugViewType::DEBUGVIEW_NODES));
                ImGui::RadioButton(_LC("TopMenubar", "Beam details"),    &debug_view_type,  static_cast<int>(DebugViewType::DEBUGVIEW_BEAMS));
                if (current_actor->ar_wheels.size() > 0)
                {
                    ImGui::RadioButton(_LC("TopMenubar", "Wheel details"),   &debug_view_type,  static_cast<int>(DebugViewType::DEBUGVIEW_WHEELS));
                }
                if (current_actor->ar_shocks.size() > 0)
                {
                    ImGui::RadioButton(_LC("TopMenubar", "Shock details"),   &debug_view_type,  static_cast<int>(DebugViewType::DEBUGVIEW_SHOCKS));
                }
                if (current_actor->ar_num_rotators > 0)
                {
                    ImGui::RadioButton(_LC("TopMenubar", "Rotator details"), &debug_view_type,  static_cast<int>(DebugViewType::DEBUGVIEW_ROTATORS));
                }
                if (current_actor->hasSlidenodes())
                {
                    ImGui::RadioButton(_LC("TopMenubar", "Slidenode details"), &debug_view_type,  static_cast<int>(DebugViewType::DEBUGVIEW_SLIDENODES));
                }
                if (current_actor->ar_num_cabs > 0)
                {
                    ImGui::RadioButton(_LC("TopMenubar", "Submesh details"), &debug_view_type,  static_cast<int>(DebugViewType::DEBUGVIEW_SUBMESH));
                }

                if ((current_actor != nullptr) && (debug_view_type != static_cast<int>(current_actor->GetGfxActor()->GetDebugView())))
                {
                    current_actor->GetGfxActor()->SetDebugView(static_cast<DebugViewType>(debug_view_type));
                }

                if (debug_view_type >= 1 && debug_view_type <= static_cast<int>(DebugViewType::DEBUGVIEW_BEAMS)) 
                {
                    ImGui::Separator();
                    ImGui::TextColored(GRAY_HINT_TEXT, "%s",     _LC("TopMenubar", "Settings:"));
                    DrawGCheckbox(App::diag_hide_broken_beams,   _LC("TopMenubar", "Hide broken beams"));
                    DrawGCheckbox(App::diag_hide_beam_stress,    _LC("TopMenubar", "Hide beam stress"));
                    DrawGCheckbox(App::diag_hide_wheels,         _LC("TopMenubar", "Hide wheels"));
                    DrawGCheckbox(App::diag_hide_nodes,          _LC("TopMenubar", "Hide nodes"));
                    if (debug_view_type >= 2)
                    {
                        DrawGCheckbox(App::diag_hide_wheel_info, _LC("TopMenubar", "Hide wheel info"));
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

    case TopMenu::TOPMENU_AI:
        menu_pos.y = window_pos.y + ai_cursor.y + MENU_Y_OFFSET;
        menu_pos.x = ai_cursor.x + window_pos.x - ImGui::GetStyle().WindowPadding.x;
        ImGui::SetNextWindowPos(menu_pos);
        if (ImGui::Begin(_LC("TopMenubar", "AI menu"), nullptr, static_cast<ImGuiWindowFlags_>(flags)))
        {
            if (ImGui::IsWindowHovered())
            {
                ai_menu = false;
            }

            ImGui::PushItemWidth(125.f); // Width includes [+/-] buttons
            ImGui::TextColored(GRAY_HINT_TEXT, "%s", _LC("TopMenubar", "General options:"));

            if (ai_num < 1)
                ai_num = 1;


            if (ai_mode == 2 || ai_mode == 3) // Drag Race or Crash driving mode
            {
                ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
                ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
            }

            ImGui::InputInt(_LC("TopMenubar", "Vehicle count"), &ai_num, 1, 100);
            if (ImGui::IsItemHovered())
            {
                ImGui::BeginTooltip();
                ImGui::Text("%s", _LC("TopMenubar", "Number of vehicles"));
                ImGui::EndTooltip();
            }

            if (ai_mode == 2 || ai_mode == 3) // Drag Race or Crash driving mode
            {
                ImGui::PopItemFlag();
                ImGui::PopStyleVar();
            }

            if (ai_num < 2)
            {
                ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
                ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
            }

            if (ai_mode == 3) // Crash driving mode
            {
                ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
                ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
            }

            ImGui::InputInt(_LC("TopMenubar", "Distance"), &ai_distance, 1, 100);
            if (ImGui::IsItemHovered())
            {
                ImGui::BeginTooltip();
                ImGui::Text("%s", _LC("TopMenubar", "Following distance in meters"));
                ImGui::EndTooltip();
            }

            if (ai_mode == 3) // Crash driving mode
            {
                ImGui::PopItemFlag();
                ImGui::PopStyleVar();
            }

            std::string label1 = "Behind";
            if (ai_position_scheme == 1)
            {
                label1 = "Parallel";
            }
            else if (ai_position_scheme == 2)
            {
                label1 = "Opposite";
            }

            if (ai_mode == 2 || ai_mode == 3) // Drag Race or Crash driving mode
            {
                ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
                ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
            }

            if (ImGui::BeginCombo("Position", label1.c_str()))
            {
                if (ImGui::Selectable("Behind"))
                {
                    ai_position_scheme = 0;
                }
                if (ImGui::Selectable("Parallel"))
                {
                    ai_position_scheme = 1;
                }
                ImGui::EndCombo();
            }
            if (ImGui::IsItemHovered())
            {
                ImGui::BeginTooltip();
                ImGui::Text("%s", _LC("TopMenubar", "Positioning scheme"));
                ImGui::Separator();
                ImGui::Text("%s", _LC("TopMenubar", "Behind: Set vehicle behind vehicle, in line"));
                ImGui::Text("%s", _LC("TopMenubar", "Parallel: Set vehicles in parallel, useful for certain scenarios like drag races"));
                ImGui::EndTooltip();
            }

            if (ai_num < 2)
            {
                ImGui::PopItemFlag();
                ImGui::PopStyleVar();
            }

            if (ai_times < 1)
                ai_times = 1;

            if (ai_mode == 4) // Chase driving mode
            {
                ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
                ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
            }

            ImGui::InputInt(_LC("TopMenubar", "Repeat times"), &ai_times, 1, 100);
            if (ImGui::IsItemHovered())
            {
                ImGui::BeginTooltip();
                ImGui::Text("%s", _LC("TopMenubar", "How many times to loop the path"));
                ImGui::EndTooltip();
            }

            if (ai_mode == 4) // Chase driving mode
            {
                ImGui::PopItemFlag();
                ImGui::PopStyleVar();
            }

            if (ai_mode == 2 || ai_mode == 3) // Drag Race or Crash driving mode
            {
                ImGui::PopItemFlag();
                ImGui::PopStyleVar();
            }

            ImGui::Separator();
            ImGui::TextColored(GRAY_HINT_TEXT, "%s", _LC("TopMenubar", "Vehicle options:"));

            std::string label2 = "Normal";
            if (ai_mode == 1)
            {
                label2 = "Race";
            }
            else if (ai_mode == 2)
            {
                label2 = "Drag Race";
            }
            else if (ai_mode == 3)
            {
                label2 = "Crash";
            }
            else if (ai_mode == 4)
            {
                label2 = "Chase";
            }

            for (auto actor : App::GetGameContext()->GetActorManager()->GetLocalActors())
            {
                if (actor->ar_driveable == AI)
                {
                    ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
                    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
                    break;
                }
            }

            if (ImGui::BeginCombo("Mode", label2.c_str()))
            {
                if (ImGui::Selectable("Normal"))
                {
                    ai_mode = 0;

                    if (ai_mode_prev == 2 || ai_mode_prev == 3)
                    {
                        ai_num = ai_num_prev;
                        ai_speed = ai_speed_prev;
                        ai_position_scheme = ai_position_scheme_prev;
                        ai_times = ai_times_prev;
                    }
                    ai_mode_prev = ai_mode;
                }
                if (ImGui::Selectable("Race"))
                {
                    ai_mode = 1;

                    if (ai_mode_prev == 2 || ai_mode_prev == 3)
                    {
                        ai_num = ai_num_prev;
                        ai_speed = ai_speed_prev;
                        ai_position_scheme = ai_position_scheme_prev;
                        ai_times = ai_times_prev;
                    }
                    ai_mode_prev = ai_mode;
                }
                if (ImGui::Selectable("Drag Race"))
                {
                    ai_mode = 2;

                    if (ai_mode_prev != 3)
                    {
                        ai_num_prev = ai_num;
                        ai_speed_prev = ai_speed;
                        ai_position_scheme_prev = ai_position_scheme;
                        ai_times_prev = ai_times;
                    }
                    ai_mode_prev = ai_mode;
                    ai_num = 2;
                    ai_speed = 1000;
                    ai_position_scheme = 1;
                    ai_times = 1;
                }
                if (ImGui::Selectable("Crash"))
                {
                    ai_mode = 3;
                    if (ai_mode_prev != 2)
                    {
                        ai_num_prev = ai_num;
                        ai_speed_prev = ai_speed;
                        ai_position_scheme_prev = ai_position_scheme;
                        ai_times_prev = ai_times;
                    }
                    ai_mode_prev = ai_mode;
                    ai_num = 2;
                    ai_speed = 100;
                    ai_position_scheme = 2;
                    ai_times = 1;
                }
                if (ImGui::Selectable("Chase"))
                {
                    ai_mode = 4;

                    if (ai_mode_prev == 2 || ai_mode_prev == 3)
                    {
                        ai_num = ai_num_prev;
                        ai_speed = ai_speed_prev;
                        ai_position_scheme = ai_position_scheme_prev;
                        ai_times = ai_times_prev;
                    }
                    ai_mode_prev = ai_mode;
                }
                ImGui::EndCombo();
            }
            if (ImGui::IsItemHovered())
            {
                ImGui::BeginTooltip();
                ImGui::Text("%s", _LC("TopMenubar", "Land vehicle driving mode"));
                ImGui::Separator();
                ImGui::Text("%s", _LC("TopMenubar", "Normal: Modify speed according to turns, other vehicles and character"));
                ImGui::Text("%s", _LC("TopMenubar", "Race: Always keep defined speed"));
                ImGui::Text("%s", _LC("TopMenubar", "Drag Race: Two vehicles performing a drag race"));
                ImGui::Text("%s", _LC("TopMenubar", "Crash: Two vehicles driving in opposite direction"));
                ImGui::Text("%s", _LC("TopMenubar", "Chase: Follow character and player vehicle"));
                ImGui::EndTooltip();
            }

            for (auto actor : App::GetGameContext()->GetActorManager()->GetLocalActors())
            {
                if (actor->ar_driveable == AI)
                {
                    ImGui::PopItemFlag();
                    ImGui::PopStyleVar();
                    break;
                }
            }

            if (ai_speed < 1)
                ai_speed = 1;

            ImGui::InputInt(_LC("TopMenubar", "Speed"), &ai_speed, 1, 100);
            if (ImGui::IsItemHovered())
            {
                ImGui::BeginTooltip();
                ImGui::Text("%s", _LC("TopMenubar", "Speed in km/h for land vehicles or knots/s for boats"));
                ImGui::EndTooltip();
            }

            if (ai_altitude < 1)
                ai_altitude = 1;

            ImGui::InputInt(_LC("TopMenubar", "Altitude"), &ai_altitude, 1, 100);
            if (ImGui::IsItemHovered())
            {
                ImGui::BeginTooltip();
                ImGui::Text("%s", _LC("TopMenubar", "Airplane maximum altitude in feet"));
                ImGui::EndTooltip();
            }

            ImGui::Separator();

            if (ImGui::Button(StripColorMarksFromText(ai_dname).c_str(), ImVec2(250, 0)))
            {
                ai_select = true;

                RoR::Message m(MSG_GUI_OPEN_SELECTOR_REQUESTED);
                m.payload = reinterpret_cast<void*>(new LoaderType(LT_AllBeam));
                App::GetGameContext()->PushMessage(m);
            }
            if (ImGui::IsItemHovered())
            {
                ImGui::BeginTooltip();
                ImGui::Text("%s", _LC("TopMenubar", "Land vehicles, boats and airplanes"));
                ImGui::EndTooltip();
            }

            if (ai_mode == 2 || ai_mode == 3) // Drag Race or Crash driving mode
            {
                ImGui::PushID("vehicle2");
                if (ImGui::Button(StripColorMarksFromText(ai_dname2).c_str(), ImVec2(250, 0)))
                {
                    ai_select2 = true;

                    RoR::Message m(MSG_GUI_OPEN_SELECTOR_REQUESTED);
                    m.payload = reinterpret_cast<void*>(new LoaderType(LT_AllBeam));
                    App::GetGameContext()->PushMessage(m);
                }
                if (ImGui::IsItemHovered())
                {
                    ImGui::BeginTooltip();
                    ImGui::Text("%s", _LC("TopMenubar", "Land vehicles, boats and airplanes"));
                    ImGui::EndTooltip();
                }
                ImGui::PopID();
            }

            ImGui::Separator();

            if (ai_rec)
            {
                ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
                ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
            }

            if (!App::GetGuiManager()->SurveyMap.ai_waypoints.empty() || ai_mode == 4) // Waypoints provided or Chase driving mode
            {
                ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyle().Colors[ImGuiCol_ButtonActive]);
            }

            if (ImGui::Button(_LC("TopMenubar", "Start"), ImVec2(80, 0)))
            {
                if (ai_mode == 4) // Chase driving mode
                {
                    App::GetGuiManager()->SurveyMap.ai_waypoints.clear();
                    if (App::GetGameContext()->GetPlayerActor()) // We are in vehicle
                    {
                        App::GetGuiManager()->SurveyMap.ai_waypoints.push_back(App::GetGameContext()->GetPlayerActor()->getPosition() + Ogre::Vector3(20, 0, 0));
                    }
                    else // We are in feet
                    {
                        App::GetGuiManager()->SurveyMap.ai_waypoints.push_back(App::GetGameContext()->GetPlayerCharacter()->getPosition() + Ogre::Vector3(20, 0, 0));
                    }
                    App::GetScriptEngine()->loadScript("AI.as", ScriptCategory::CUSTOM);
                }
                else
                {
                    if (App::GetGuiManager()->SurveyMap.ai_waypoints.empty())
                    {
                        App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE,
                                                      fmt::format(_LC("TopMenubar", "Select a preset, record or open survey map ({}) to set waypoints."),
                                                      App::GetInputEngine()->getEventCommandTrimmed(EV_SURVEY_MAP_CYCLE)), "lightbulb.png");
                    }
                    else
                    {
                        App::GetScriptEngine()->loadScript("AI.as", ScriptCategory::CUSTOM);
                    }
                }
            }

            if (!App::GetGuiManager()->SurveyMap.ai_waypoints.empty() || ai_mode == 4) // Waypoints provided or Chase driving mode
            {
                ImGui::PopStyleColor();
            }

            ImGui::SameLine();

            if (ImGui::Button(_LC("TopMenubar", "Stop"), ImVec2(80, 0)))
            {
                if (ai_mode == 4) // Chase driving mode
                {
                    App::GetGuiManager()->SurveyMap.ai_waypoints.clear();
                }

                for (ActorPtr actor : App::GetGameContext()->GetActorManager()->GetLocalActors())
                {
                    if (actor->ar_driveable == AI)
                    {
                        App::GetGameContext()->PushMessage(Message(MSG_SIM_DELETE_ACTOR_REQUESTED, static_cast<void*>(new ActorPtr(actor))));
                    }
                }
            }

            if (ai_rec)
            {
                ImGui::PopItemFlag();
                ImGui::PopStyleVar();
            }

            ImGui::SameLine();

            ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyle().Colors[ImGuiCol_Button]);
            std::string label = "Record";
            if (ai_rec)
            {
                label = "Recording";
                ImGui::PushStyleColor(ImGuiCol_Button, RED_TEXT);
            }

            if (ImGui::Button(label.c_str(), ImVec2(80, 0)))
            {
                if (!ai_rec)
                {
                    App::GetGuiManager()->SurveyMap.ai_waypoints.clear();
                    ai_rec = true;
                }
                else
                {
                   ai_rec = false;
                }
            }

            ImGui::PopStyleColor();
            ImGui::Separator();

            bool is_open = ImGui::CollapsingHeader(_LC("TopMenubar", "Presets"));
            if (ImGui::IsItemActivated() && !is_open && j_doc.Empty()) // Fetch once
            {
                if (FileExists(PathCombine(App::sys_savegames_dir->getStr(), "waypoints.json")))
                {
                    App::GetContentManager()->LoadAndParseJson("waypoints.json", RGN_SAVEGAMES, j_doc);
                }
                else
                {
                    this->GetPresets();
                }
            }

            if (is_open && j_doc.Empty())
            {
                float spinner_size = 8.f;
                ImGui::SetCursorPosX((ImGui::GetWindowSize().x / 2.f) - spinner_size);
                LoadingIndicatorCircle("spinner", spinner_size, theme.value_blue_text_color, theme.value_blue_text_color, 10, 10);
            }

            if (is_open && !j_doc.Empty())
            {
                size_t num_rows = j_doc.GetArray().Size();
                int count = 0;
                for (size_t i = 0; i < num_rows; i++)
                {
                    rapidjson::Value& j_row = j_doc[static_cast<rapidjson::SizeType>(i)];

                    if (j_row.HasMember("terrain") && App::sim_terrain_name->getStr() == j_row["terrain"].GetString())
                    {
                        count++;
                        if (ImGui::Button(j_row["preset"].GetString(), ImVec2(250, 0)))
                        {
                            App::GetGuiManager()->SurveyMap.ai_waypoints.clear();

                            for (size_t i = 0; i < j_row["waypoints"].Size(); i++)
                            {
                                float x = j_row["waypoints"][i][0].GetFloat();
                                float y = j_row["waypoints"][i][1].GetFloat();
                                float z = j_row["waypoints"][i][2].GetFloat();
                                App::GetGuiManager()->SurveyMap.ai_waypoints.push_back(Ogre::Vector3(x, y, z));
                            }
                        }
                    }
                }
                if (count == 0)
                {
                    ImGui::Text("%s", _LC("TopMenubar", "No presets found for this terrain :("));
                    ImGui::Text("%s", _LC("TopMenubar", "Supported terrains:"));
                    ImGui::Separator();

                    ImGui::BeginChild("terrains-scrolling", ImVec2(0.f, 200), false);

                    for (size_t i = 0; i < num_rows; i++)
                    {
                        rapidjson::Value& j_row_terrains = j_doc[static_cast<rapidjson::SizeType>(i)];
                        if (j_row_terrains.HasMember("terrains"))
                        {
                            for (size_t i = 0; i < j_row_terrains["terrains"].Size(); i++)
                            {
                                ImGui::Text("%s", j_row_terrains["terrains"][i].GetString());
                            }
                        }
                    }

                    ImGui::EndChild();
                }
            }

            if (ImGui::CollapsingHeader(_LC("TopMenubar", "Waypoints")))
            {
                if (App::GetGuiManager()->SurveyMap.ai_waypoints.empty())
                {
                    ImGui::Text("%s", _LC("TopMenubar", "No waypoints defined."));
                }
                else
                {
                    if (ImGui::Button(_LC("TopMenubar", "Export"), ImVec2(250, 0)))
                    {
                        std::string s;

                        for (int i = 0; i < App::GetGuiManager()->SurveyMap.ai_waypoints.size(); i++)
                        {
                            s += "\n            [" + std::to_string(App::GetGuiManager()->SurveyMap.ai_waypoints[i].x) + ", " + std::to_string(App::GetGuiManager()->SurveyMap.ai_waypoints[i].y) + ", " + std::to_string(App::GetGuiManager()->SurveyMap.ai_waypoints[i].z) + "]";
                            if (i != App::GetGuiManager()->SurveyMap.ai_waypoints.size() - 1)
                            {
                                s += ",";
                            }
                        }

                        std::string json = fmt::format("\n    {{\n        \"terrain\":\"{}\",\n        \"preset\":\"Preset name\",\n        \"waypoints\":\n        [{}\n        ]\n    }}", App::sim_terrain_name->getStr(), s);
                        RoR::Log(json.c_str());

                        App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE,
                                                      fmt::format(_LC("TopMenubar", "{} waypoints exported to RoR.log"),
                                                      App::GetGuiManager()->SurveyMap.ai_waypoints.size()), "lightbulb.png");
                    }

                    ImGui::BeginChild("waypoints-scrolling", ImVec2(0.f, 200), false);

                    for (int i = 0; i < App::GetGuiManager()->SurveyMap.ai_waypoints.size(); i++)
                    {
                        ImGui::PushID(i);
                        ImGui::AlignTextToFramePadding();
                        ImGui::Text("%d", i);
                        ImGui::SameLine();
                        std::string w = std::to_string(App::GetGuiManager()->SurveyMap.ai_waypoints[i].x) + " " + std::to_string(App::GetGuiManager()->SurveyMap.ai_waypoints[i].y) + " " + std::to_string(App::GetGuiManager()->SurveyMap.ai_waypoints[i].z);
                        if (ImGui::Button(w.c_str(), ImVec2(230, 0)))
                        {
                            Ogre::Vector3* payload = new Ogre::Vector3(App::GetGuiManager()->SurveyMap.ai_waypoints[i]);
                            App::GetGameContext()->PushMessage(Message(MSG_SIM_TELEPORT_PLAYER_REQUESTED, (void*)payload));
                        }
                        ImGui::PopID();
                    }

                    ImGui::EndChild();
                }
            }

            ImGui::PopItemWidth();
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

bool TopMenubar::ShouldDisplay(ImVec2 window_pos)
{
    if (!App::GetGuiManager()->AreStaticMenusAllowed())
    {
        return false;
    }

    if (ImGui::IsMouseDown(1))
    {
        return false;
    }

    if (ai_menu)
    {
        m_open_menu = TopMenu::TOPMENU_AI;
        return true;
    }

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

void TopMenubar::DrawMpUserToActorList(RoRnet::UserInfo &user)
{
    // Count actors owned by the player
    unsigned int num_actors_player = 0;
    for (ActorPtr& actor : App::GetGameContext()->GetActorManager()->GetActors())
    {
        if (actor->ar_net_source_id == user.uniqueid)
        {
            ++num_actors_player;
        }
    }

    // Display user in list
#ifdef USE_SOCKETW
    const Ogre::ColourValue player_color = App::GetNetwork()->GetPlayerColor(user.colournum);
    ImVec4 player_gui_color(player_color.r, player_color.g, player_color.b, 1.f);
    ImGui::PushStyleColor(ImGuiCol_Text, player_gui_color);
    ImGui::Text("%s: %u (%s, Ver: %s, Lang: %s)",
                user.username, num_actors_player,
                App::GetNetwork()->UserAuthToStringShort(user).c_str(),
                user.clientversion, user.language);
    ImGui::PopStyleColor();
#endif // USE_SOCKETW

    // Display actor list
    Ogre::TexturePtr tex1 = FetchIcon("control_pause.png");
    Ogre::TexturePtr tex2 = FetchIcon("control_play.png");
    int i = 0;
    for (ActorPtr& actor : App::GetGameContext()->GetActorManager()->GetActors())
    {
        if ((!actor->ar_hide_in_actor_list) && (actor->ar_net_source_id == user.uniqueid))
        {
            std::string id = fmt::format("{}:{}", i++, user.uniqueid);
            ImGui::PushID(id.c_str());
            if (actor->ar_state == ActorState::NETWORKED_OK)
            {
                if (ImGui::ImageButton(reinterpret_cast<ImTextureID>(tex1->getHandle()), ImVec2(16, 16)))
                {
                   App::GetGameContext()->PushMessage(Message(MSG_SIM_HIDE_NET_ACTOR_REQUESTED, static_cast<void*>(new ActorPtr(actor))));
                }
            }
            else if (actor->ar_state == ActorState::NETWORKED_HIDDEN)
            {
                if (ImGui::ImageButton(reinterpret_cast<ImTextureID>(tex2->getHandle()), ImVec2(16, 16)))
                {
                   App::GetGameContext()->PushMessage(Message(MSG_SIM_UNHIDE_NET_ACTOR_REQUESTED, static_cast<void*>(new ActorPtr(actor))));
                }
            }
            else // Our actor(s)
            {
                std::string text_buf_rem = fmt::format(" X ##[{}]", i);
                ImGui::PushStyleColor(ImGuiCol_Text, RED_TEXT);
                if (ImGui::Button(text_buf_rem.c_str()))
                {
                   App::GetGameContext()->PushMessage(Message(MSG_SIM_DELETE_ACTOR_REQUESTED, static_cast<void*>(new ActorPtr(actor))));
                }
                ImGui::PopStyleColor();
            }
            ImGui::PopID();
            ImGui::SameLine();

            std::string actortext_buf = fmt::format("{} ({}) ##[{}:{}]", StripColorMarksFromText(actor->ar_design_name).c_str(), actor->ar_filename.c_str(), i++, user.uniqueid);
            if (ImGui::Button(actortext_buf.c_str())) // Button clicked?
            {
                App::GetGameContext()->PushMessage(Message(MSG_SIM_SEAT_PLAYER_REQUESTED, static_cast<void*>(new ActorPtr(actor))));
            }
        }
    }
}

void TopMenubar::DrawActorListSinglePlayer()
{
    std::vector<ActorPtr> actor_list;
    for (ActorPtr actor : App::GetGameContext()->GetActorManager()->GetActors())
    {
        if (!actor->ar_hide_in_actor_list)
        {
            actor_list.emplace_back(actor);
        }
    }
    if (actor_list.empty())
    {
        ImGui::PushStyleColor(ImGuiCol_Text, GRAY_HINT_TEXT);
        ImGui::Text("%s", _LC("TopMenubar", "None spawned yet"));
        ImGui::Text("%s", _LC("TopMenubar", "Use [Simulation] menu"));
        ImGui::PopStyleColor();
    }
    else
    {
        ActorPtr player_actor = App::GetGameContext()->GetPlayerActor();
        int i = 0;
        for (ActorPtr actor : actor_list)
        {
            std::string text_buf_rem = fmt::format("X ##[{}]", i);
            ImGui::PushStyleColor(ImGuiCol_Text, RED_TEXT);
            if (ImGui::Button(text_buf_rem.c_str()))
            {
                App::GetGameContext()->PushMessage(Message(MSG_SIM_DELETE_ACTOR_REQUESTED, static_cast<void*>(new ActorPtr(actor))));
            }
            ImGui::PopStyleColor();
            ImGui::SameLine();

            std::string text_buf = fmt::format( "[{}] {}", i++, StripColorMarksFromText(actor->ar_design_name).c_str());
            auto linked_actors = actor->getAllLinkedActors();
            if (actor == player_actor)
            {
                ImGui::PushStyleColor(ImGuiCol_Text, GREEN_TEXT);
            }
            else if (std::find(linked_actors.begin(), linked_actors.end(), player_actor) != linked_actors.end())
            {
                ImGui::PushStyleColor(ImGuiCol_Text, ORANGE_TEXT);
            }
            else if (actor->ar_state == ActorState::LOCAL_SIMULATED)
            {
                ImGui::PushStyleColor(ImGuiCol_Text, WHITE_TEXT);
            }
            else
            {
                ImGui::PushStyleColor(ImGuiCol_Text, GRAY_HINT_TEXT);
            }
            if (ImGui::Button(text_buf.c_str())) // Button clicked?
            {
                App::GetGameContext()->PushMessage(Message(MSG_SIM_SEAT_PLAYER_REQUESTED, static_cast<void*>(new ActorPtr(actor))));
            }
            ImGui::PopStyleColor();
        }
    }
}

void TopMenubar::DrawSpecialStateBox(float top_offset)
{
    std::string special_text;
    std::string special_text_b;
    std::string special_text_c;
    std::string special_text_d;
    ImVec4 special_color = ImGui::GetStyle().Colors[ImGuiCol_Text]; // Regular color
    ImVec4 special_color_c = ImVec4(0,0,0,0);
    float content_width = 0.f;
    bool replay_box = false;
    bool race_box = false;

    // Gather state info
    if (App::GetGameContext()->GetActorManager()->IsSimulationPaused() && !App::GetGuiManager()->IsGuiHidden())
    {
        special_color = ORANGE_TEXT;
        special_text = fmt::format(_LC("TopMenubar", "All physics paused, press {} to resume"),
                                   App::GetInputEngine()->getEventCommandTrimmed(EV_COMMON_TOGGLE_PHYSICS));
        content_width = ImGui::CalcTextSize(special_text.c_str()).x;
    }
    else if (App::GetGameContext()->GetPlayerActor() &&
             App::GetGameContext()->GetPlayerActor()->ar_physics_paused &&
             !App::GetGuiManager()->IsGuiHidden())
    {
        special_color = GREEN_TEXT;
        special_text = fmt::format(_LC("TopMenubar", "Vehicle physics paused, press {} to resume"),
                                   App::GetInputEngine()->getEventCommandTrimmed(EV_TRUCK_TOGGLE_PHYSICS));
        content_width = ImGui::CalcTextSize(special_text.c_str()).x;
    }
    else if (App::GetGameContext()->GetPlayerActor() &&
            App::GetGameContext()->GetPlayerActor()->ar_state == ActorState::LOCAL_REPLAY)
    {
        content_width = 300;
        replay_box = true;
        special_text = _LC("TopMenubar", "Replay");
    }
    else if (App::GetGfxScene()->GetSimDataBuffer().simbuf_dir_arrow_visible)
    {
        race_box = true;

        // Calculate distance
        GameContextSB& data = App::GetGfxScene()->GetSimDataBuffer();
        GUIManager::GuiTheme const& theme = App::GetGuiManager()->GetTheme();
        float distance = 0.0f;
        ActorPtr player_actor = App::GetGfxScene()->GetSimDataBuffer().simbuf_player_actor;
        if (player_actor != nullptr && App::GetGameContext()->GetPlayerActor() &&
            player_actor->GetGfxActor()->GetSimDataBuffer().simbuf_actor_state == ActorState::LOCAL_SIMULATED)
        {
            distance = player_actor->GetGfxActor()->GetSimDataBuffer().simbuf_pos.distance(data.simbuf_dir_arrow_target);
        }
        else
        {
            distance = data.simbuf_character_pos.distance(data.simbuf_dir_arrow_target);
        }

        // format text
        special_text = App::GetGfxScene()->GetSimDataBuffer().simbuf_dir_arrow_text;
        special_text_b = fmt::format("{:.1f} {}", distance, _LC("DirectionArrow", "meter"));
        content_width = ImGui::CalcTextSize(special_text.c_str()).x + ImGui::CalcTextSize(special_text_b.c_str()).x;

        float time = App::GetGfxScene()->GetSimDataBuffer().simbuf_race_time;
        special_text_c = fmt::format("{:02d}.{:02d}.{:02d}", (int)(time) / 60, (int)(time) % 60, (int)(time * 100.0) % 100);
        float time_diff = App::GetGfxScene()->GetSimDataBuffer().simbuf_race_time_diff;
        special_color_c = (time_diff > 0.0f)
                          ? theme.value_red_text_color
                          : ((time_diff < 0.0f) ? theme.success_text_color : theme.value_blue_text_color);

        if (App::GetGfxScene()->GetSimDataBuffer().simbuf_race_best_time > 0.0f)
        {
            float best_time = App::GetGfxScene()->GetSimDataBuffer().simbuf_race_best_time;
            special_text_d = fmt::format("{:02d}.{:02d}.{:02d}", (int)(best_time) / 60, (int)(best_time) % 60, (int)(best_time * 100.0) % 100);
        }
    }
    else if (App::sim_state->getEnum<SimState>() == SimState::EDITOR_MODE)
    {
        special_color = GREEN_TEXT;
        special_text = fmt::format(_LC("TopMenubar", "Terrain editing mode, press {} to save and exit"),
                                   App::GetInputEngine()->getEventCommandTrimmed(EV_COMMON_TOGGLE_TERRAIN_EDITOR));
        content_width = ImGui::CalcTextSize(special_text.c_str()).x;
    }

    // Draw box if needed
    if (!special_text.empty())
    {
        ImVec2 box_pos;
        box_pos.y = top_offset;
        box_pos.x = (ImGui::GetIO().DisplaySize.x / 2) - ((content_width / 2) + ImGui::GetStyle().FramePadding.x);
        ImGui::SetNextWindowPos(box_pos);
        ImGuiWindowFlags flags = ImGuiWindowFlags_NoResize    | ImGuiWindowFlags_NoMove |
                                 ImGuiWindowFlags_NoTitleBar  | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize;
        ImGui::PushStyleColor(ImGuiCol_WindowBg, App::GetGuiManager()->GetTheme().semitransparent_window_bg);
        if (ImGui::Begin("Special state box", nullptr, flags))
        {
            ImGui::TextColored(special_color, "%s", special_text.c_str());
            if (replay_box)
            {
                ImGui::SameLine();
                
                // Progress bar with frame index/count
                Replay* replay = App::GetGameContext()->GetPlayerActor()->getReplay();
                float fraction = (float)std::abs(replay->getCurrentFrame())/(float)replay->getNumFrames();
                Str<100> pbar_text; pbar_text << replay->getCurrentFrame() << "/" << replay->getNumFrames();
                float pbar_width = content_width - (ImGui::GetStyle().ItemSpacing.x + ImGui::CalcTextSize(special_text.c_str()).x);
                ImGui::ProgressBar(fraction, ImVec2(pbar_width, ImGui::GetTextLineHeight()), pbar_text.ToCStr());

                // Game time text
                float time_sec = replay->getLastReadTime() / 1000000.0;
                char str[200];
                int str_pos = 0;
                if (time_sec > 60)
                {
                    int min = (int)time_sec / 60;
                    str_pos = snprintf(str, 200, "%dmin ", min);
                    time_sec -= (float)min * 60.f;
                }
                snprintf(str+str_pos, 200-str_pos, "%.2fsec", time_sec);
                ImGui::TextDisabled("%s: %s", _LC("TopMenubar", "Time"), str);
                
            }
            else if (race_box)
            {
                ImGui::SameLine();
                ImGui::Text(special_text_b.c_str());
                ImGui::SetCursorPosX((ImGui::GetWindowSize().x / 2) - (ImGui::CalcTextSize(special_text_c.c_str()).x / 2));
                ImGui::TextColored(special_color_c,"%s", special_text_c.c_str());

                Str<300> text;
                text << "Best Time: " << special_text_d.c_str();
                ImGui::SetCursorPosX((ImGui::GetWindowSize().x / 2) - (ImGui::CalcTextSize(text).x / 2));

                if (!special_text_d.empty())
                {
                    ImGui::TextDisabled(text);
                }
            }
            ImGui::End();
        }
        ImGui::PopStyleColor(1); // WindowBg
    }
}

void TopMenubar::Refresh(std::string payload)
{
    j_doc.Parse(payload.c_str());
}

void TopMenubar::GetPresets()
{
#if defined(USE_CURL)
    std::packaged_task<void()> task(GetJson);
    std::thread(std::move(task)).detach();
#endif // defined(USE_CURL)
}
