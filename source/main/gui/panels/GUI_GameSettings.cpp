/*
    This source file is part of Rigs of Rods
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

#include "GUI_GameSettings.h"

#include "AppContext.h"
#include "CacheSystem.h"
#include "GameContext.h"
#include "GUIManager.h"
#include "GUIUtils.h"
#include "Language.h"
#include "SoundManager.h"

#ifdef USE_OPENAL
#   include <alc.h>
#endif

using namespace RoR;
using namespace GUI;

void GameSettings::Draw()
{
    const int flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar;
    ImGui::SetNextWindowSize(ImVec2(670.f, 400.f), ImGuiCond_FirstUseEver);
    if (m_bump_height != 0)
    {
        ImGui::SetNextWindowSize(m_window_size + ImVec2(0, m_bump_height));
        m_bump_height = 0.f;
    }
    ImGui::SetNextWindowPosCenter(ImGuiCond_Appearing);
    bool keep_open = true;
    ImGui::Begin(_LC("GameSettings", "Game settings"), &keep_open, flags);

    ImGui::BeginTabBar("GameSettingsTabs");

    const float child_height = ImGui::GetWindowHeight()
        - ((2.f * ImGui::GetStyle().WindowPadding.y) + (3.f * ImGui::GetItemsLineHeightWithSpacing())
            + ImGui::GetStyle().ItemSpacing.y);

    if (ImGui::BeginTabItem(_LC("GameSettings", "Render System")))
    {
        ImGui::BeginChild("Settings-Render-scroll", ImVec2(0.f, child_height), false);
        this->DrawRenderSystemSettings();
        ImGui::EndChild();
        ImGui::EndTabItem();
    }
    if (ImGui::BeginTabItem(_LC("GameSettings", "General")))
    {
        ImGui::BeginChild("Settings-General-scroll", ImVec2(0.f, child_height), false);
        this->DrawGeneralSettings();
        ImGui::EndChild();
        ImGui::EndTabItem();
    }
    if (ImGui::BeginTabItem(_LC("GameSettings", "Gameplay")))
    {
        ImGui::BeginChild("Settings-Gameplay-scroll", ImVec2(0.f, child_height), false);
        this->DrawGameplaySettings();
        ImGui::EndChild();
        ImGui::EndTabItem();
    }
    if (ImGui::BeginTabItem(_LC("GameSettings", "UI")))
    {
        ImGui::BeginChild("Settings-UI-scroll", ImVec2(0.f, child_height), false);
        this->DrawUiSettings();
        ImGui::EndChild();
        ImGui::EndTabItem();
    }
    if (ImGui::BeginTabItem(_LC("GameSettings", "Graphics")))
    {
        ImGui::BeginChild("Settings-Graphics-scroll", ImVec2(0.f, child_height), false);
        this->DrawGraphicsSettings();
        ImGui::EndChild();
        ImGui::EndTabItem();
    }
    if (ImGui::BeginTabItem(_LC("GameSettings", "Audio")))
    {
        ImGui::BeginChild("Settings-Audio-scroll", ImVec2(0.f, child_height), false);
        this->DrawAudioSettings();
        ImGui::EndChild();
        ImGui::EndTabItem();
    }
    if (ImGui::BeginTabItem(_LC("GameSettings", "Controls")))
    {
        ImGui::BeginChild("Settings-Controls-scroll", ImVec2(0.f, child_height), false);
        this->DrawControlSettings();
        ImGui::EndChild();
        ImGui::EndTabItem();
    }
    if (ImGui::BeginTabItem(_LC("GameSettings", "Diagnostic")))
    {
        ImGui::BeginChild("Settings-Diag-scroll", ImVec2(0.f, child_height), false);
        this->DrawDiagSettings();
        ImGui::EndChild();
        ImGui::EndTabItem();
    }

    ImGui::EndTabBar();

    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 4.f);
    m_window_size = ImGui::GetWindowSize();

    ImGui::End();
    if (!keep_open)
    {
        this->SetVisible(false);
    }
}

void GameSettings::DrawRenderSystemSettings()
{
    ImGui::TextDisabled("%s", _LC("GameSettings", "Render system (changes require a restart)"));

    if (m_render_must_restart)
    {
        std::string text = _LC("GameSettings", "You must restart the game to make changes effective.");
        ImVec2 box_size = ImGui::CalcTextSize(text.c_str()) + ImGui::GetStyle().FramePadding * 2;
        ImVec2 box_pos = ImGui::GetCursorPos() + ImVec2(ImGui::GetWindowContentRegionWidth() * 0.5f - box_size.x * 0.5f, 0.f);
        ImGui::SetCursorPos(box_pos);
        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.8f, 0.7f, 0.1f, 1.f));
        ImGui::BeginChildFrame(ImGuiID(123), box_size);
        ImGui::TextColored(ImVec4(0.1f, 0.1f, 0.1f, 1.f), text.c_str());
        ImGui::EndChildFrame();
        ImGui::PopStyleColor(); // FrameBg
    }

    const auto ogre_root = App::GetAppContext()->GetOgreRoot();
    const auto render_systems = ogre_root->getAvailableRenderers();
    std::string render_system_names;
    for (auto rs : render_systems)
    {
        render_system_names += rs->getName() + '\0';
    }
    const auto ro = ogre_root->getRenderSystemByName(App::app_rendersys_override->getStr());
    const auto rs = ro ? ro : ogre_root->getRenderSystem();
    const auto it = std::find(render_systems.begin(), render_systems.end(), rs);
    int render_id = it != render_systems.end() ? std::distance(render_systems.begin(), it) : 0;
    /* Combobox for selecting the Render System*/
    if (ImGui::Combo(_LC ("GameSettings", "Render System"), &render_id, render_system_names.c_str()))
    {
        App::app_rendersys_override->setStr(render_systems[render_id]->getName());
    }

    const auto config_options = ogre_root->getRenderSystem()->getConfigOptions();
    std::set<std::string> filter = {"Allow NVPerfHUD", "Colour Depth", "Fixed Pipeline Enabled",
        "Floating-point mode", "Resource Creation Policy", "VSync Interval", "sRGB Gamma Conversion"};
    for (auto opt : config_options)
    {
        auto co = opt.second;
        if (co.immutable)
            continue;
        if (co.possibleValues.empty())
            continue;
        if (filter.find(co.name) != filter.end())
            continue;
        std::sort(co.possibleValues.rbegin(), co.possibleValues.rend());
        std::string option_values;
        for (auto value : co.possibleValues)
        {
            option_values += value + '\0';
        }
        const auto it = std::find(co.possibleValues.begin(), co.possibleValues.end(), opt.second.currentValue);
        int option_id = it != co.possibleValues.end() ? std::distance(co.possibleValues.begin(), it) : 0;
        if (ImGui::Combo(co.name.c_str(), &option_id, option_values.c_str()))
        {
            // Check the new value is different from the current one
            if (co.currentValue != co.possibleValues[option_id])
            {
                // Set the new value to the render system
                rs->setConfigOption(co.name, co.possibleValues[option_id]);
                if (rs->validateConfigOptions().empty())
                {
                    ogre_root->saveConfig();

                    // Show the "Must restart game..." box, make the window bigger to accomodate it
                    if (!m_render_must_restart)
                    {
                        m_render_must_restart = true;
                        m_bump_height = ImGui::GetTextLineHeightWithSpacing() + ImGui::GetStyle().FramePadding.y * 2;
                    }
                }
            }
        }
    }
}

void GameSettings::DrawGeneralSettings()
{
    ImGui::TextDisabled("%s", _LC("GameSettings", "Application settings"));

#ifndef NOLANG
    std::vector<std::pair<std::string, std::string>> languages = App::GetLanguageEngine()->getLanguages();
    std::string lang_values;
    for (auto value : languages)
    {
        lang_values += value.first + '\0';
    }
    const auto it = std::find_if(languages.begin(), languages.end(),
            [](const std::pair<std::string, std::string>& l) { return l.second == App::app_language->getStr(); });
    int lang_selection = it != languages.end() ? std::distance(languages.begin(), it) : 0;
    if (ImGui::Combo(_LC("GameSettings", "Language"), &lang_selection, lang_values.c_str()))
    {
        App::app_language->setStr(languages[lang_selection].second);
        App::GetLanguageEngine()->setup();
    }
#endif

    // Country selection
    static Ogre::FileInfoListPtr fl = Ogre::ResourceGroupManager::getSingleton().findResourceFileInfo("FlagsRG", "*");
    if (!fl->empty())
    {
        static std::vector<std::string> countries;
        if (countries.empty())
        {
            for (auto& file : *fl)
            {
                std::string country = Ogre::StringUtil::replaceAll(file.filename, ".png", "");
                if (country.size() == 2) // RoR protocol limitation
                {
                    countries.push_back(country);
                }
            }
            std::sort(countries.begin(), countries.end());
        }
        std::string country_values;
        for (auto value : countries)
        {
            country_values += value + '\0';
        }
        const auto it = std::find(countries.begin(), countries.end(), App::app_country->getStr());
        int country_selection = it != countries.end() ? std::distance(countries.begin(), it) : 0;
        if (ImGui::Combo(_LC("GameSettings", "Country"), &country_selection, country_values.c_str()))
        {
            App::app_country->setStr(countries[country_selection].c_str());
        }
    }

    int sshot_select = (App::app_screenshot_format->getStr() == "jpg") ? 1 : 0; // Hardcoded; TODO: list available formats.

    /* Screenshot format: Can be png or jpg*/
    if (ImGui::Combo(_LC("GameSettings", "Screenshot format"), &sshot_select, "png\0jpg\0\0"))
    {
        std::string str = (sshot_select == 1) ? "jpg" : "png";
        App::app_screenshot_format->setStr(str);
    }

    DrawGTextEdit(App::app_extra_mod_path, _LC("GameSettings", "Extra mod path"),  m_buf_app_extra_mod_dir);

    DrawGCheckbox(App::app_skip_main_menu, _LC("GameSettings", "Skip main menu"));
    DrawGCheckbox(App::app_async_physics, _LC("GameSettings", "Async physics"));
    DrawGCheckbox(App::app_disable_online_api, _LC("GameSettings", "Disable online api"));

    if (ImGui::Button(_LC("GameSettings", "Update cache")))
    {
        App::GetGuiManager()->GameSettings.SetVisible(false);
        App::GetGameContext()->PushMessage(Message(MSG_APP_MODCACHE_UPDATE_REQUESTED));
        App::GetGameContext()->PushMessage(Message(MSG_GUI_OPEN_MENU_REQUESTED));
    }
}

void GameSettings::DrawGameplaySettings()
{
    ImGui::TextDisabled("%s", _LC("GameSettings", "Simulation settings"));

    DrawGCombo(App::sim_gearbox_mode, _LC("GameSettings", "Gearbox mode"),
        m_combo_items_gearbox_mode.c_str());

    //DrawGCheckbox(App::gfx_flexbody_cache,     "Enable flexbody cache");

    DrawGCheckbox(App::sim_spawn_running, _LC("GameSettings", "Engines spawn running"));

    DrawGCheckbox(App::sim_replay_enabled, _LC("GameSettings", "Replay mode"));
    if (App::sim_replay_enabled->getBool())
    {
        DrawGIntBox(App::sim_replay_length, _LC("GameSettings", "Replay length"));
        DrawGIntBox(App::sim_replay_stepping, _LC("GameSettings", "Replay stepping"));
    }

    DrawGCheckbox(App::sim_realistic_commands, _LC("GameSettings", "Realistic forward commands"));

    DrawGCheckbox(App::sim_races_enabled, _LC("GameSettings", "Enable races"));

    DrawGCheckbox(App::sim_no_self_collisions, _LC("GameSettings", "No intra truck collisions"));
    DrawGCheckbox(App::sim_no_collisions, _LC("GameSettings", "No inter truck collisions"));

    DrawGCheckbox(App::io_discord_rpc, _LC("GameSettings", "Discord Rich Presence"));

    DrawGCheckbox(App::sim_quickload_dialog, _LC("GameSettings", "Show confirm. UI dialog for quickload"));

    DrawGCheckbox(App::sim_tuning_enabled, _LC("GameSettings", "Enable vehicle tuning"));
}

void GameSettings::DrawAudioSettings()
{
#ifdef USE_OPENAL
    ImGui::TextDisabled("%s", _LC("GameSettings", "Audio settings"));

    static const ALCchar *devices = alcGetString(NULL, ALC_ALL_DEVICES_SPECIFIER);
    const ALCchar *device = devices, *next = devices + 1;
    std::vector<std::string> audio_devices;

    while (device && *device != '\0' && next && *next != '\0')
    {
            audio_devices.push_back(device);
            size_t len = strlen(device);
            device += (len + 1);
            next += (len + 2);
    }

    const auto it = std::find(audio_devices.begin(), audio_devices.end(), App::audio_device_name->getStr());
    int device_id = it != audio_devices.end() ? std::distance(audio_devices.begin(), it) : 0;
    if (ImGui::Combo(_LC("GameSettings", "Audio device"), &device_id, devices))
    {
        App::audio_device_name->setStr(audio_devices[device_id]);
    }

    DrawGCheckbox(App::audio_enable_creak,      _LC("GameSettings", "Creak sound"));
    DrawGCheckbox(App::audio_enable_efx,        _LC("GameSettings", "Enable advanced sound effects via OpenAL EFX"));

    if (App::audio_enable_efx->getBool())
    {
        DrawGCombo(App::audio_efx_reverb_engine,         _LC("GameSettings", "OpenAL Reverb engine"), m_combo_items_efx_reverb_engine.c_str());
        DrawGCheckbox(App::audio_enable_obstruction,     _LC("GameSettings", "Sound obstruction"));
        if (App::audio_enable_obstruction->getBool())
        {
            DrawGCheckbox(App::audio_force_obstruction_inside_vehicles, _LC("GameSettings", "Force obstruction inside vehicles"));
        }
        DrawGCheckbox(App::audio_enable_occlusion,       _LC("GameSettings", "Sound occlusion"));
        DrawGCheckbox(App::audio_enable_directed_sounds, _LC("GameSettings", "Directed sounds (exhausts etc.)"));
        if (App::audio_efx_reverb_engine->getEnum<EfxReverbEngine>() == EfxReverbEngine::EAXREVERB)
        {
            DrawGCheckbox(App::audio_enable_reflection_panning, _LC("GameSettings", "Early reflections panning (experimental)"));
        }

        DrawGCheckbox(App::audio_engine_controls_environmental_audio, _LC("GameSettings", "Engine exerts automatic control over environmental audio"));

        if (App::audio_engine_controls_environmental_audio)
        {
            DrawGTextEdit(App::audio_default_efx_preset,          _LC("GameSettings", "Default EFX Reverb Preset"), m_buf_audio_default_efx_preset);
            DrawGTextEdit(App::audio_force_listener_efx_preset,   _LC("GameSettings", "Force EFX Reverb Preset"),   m_buf_audio_force_listener_efx_preset);
        }
    }

    DrawGCheckbox(App::audio_menu_music,        _LC("GameSettings", "Main menu music"));
    DrawGFloatSlider(App::audio_master_volume,  _LC("GameSettings", "Master volume"), 0, 1);
    DrawGFloatSlider(App::audio_doppler_factor, _LC("GameSettings", "Doppler factor"), 0, 10);
#endif // USE_OPENAL
}

void GameSettings::DrawUiSettings()
{
    ImGui::TextDisabled("%s", _LC("GameSettings", "UI settings"));

    this->DrawUiPresetCombo();

    this->DrawUiDefaultDashboard(m_ui_known_dash_truck, App::ui_default_truck_dash, CID_DashboardsTruck, _LC("GameSettings", "Default truck dashboard"));
    this->DrawUiDefaultDashboard(m_ui_known_dash_boat, App::ui_default_boat_dash, CID_DashboardsBoat, _LC("GameSettings", "Default boat dashboard"));

    DrawGCheckbox(App::gfx_speedo_imperial, _LC("GameSettings", "Imperial units"));

    DrawGCheckbox(App::ui_show_live_repair_controls, _LC("GameSettings", "Show controls in live repair box"));
    
    DrawGCheckbox(App::ui_show_vehicle_buttons, _LC("GameSettings", "Show vehicle buttons menu"));

    DrawGCheckbox(App::ui_always_show_fullsize, _LC("GameSettings", "Always show full size help image"));


    DrawGCheckbox(App::gfx_surveymap_icons,  _LC("GameSettings", "Overview map icons"));
    if (App::gfx_surveymap_icons->getBool())
    {
        DrawGCheckbox(App::gfx_declutter_map,  _LC("GameSettings", "Declutter overview map"));
    }

}

void GameSettings::DrawGraphicsSettings()
{
    ImGui::TextDisabled("%s", _LC("GameSettings", "Video settings"));

    DrawGCombo(App::gfx_flares_mode, _LC("GameSettings", "Light sources"),
        m_combo_items_light_sources.c_str());

    DrawGCombo(App::gfx_shadow_type, _LC("GameSettings", "Shadow type (requires restart)"),
        m_combo_items_shadow_type.c_str());

    if (App::gfx_shadow_type->getEnum<GfxShadowType>() != GfxShadowType::NONE)
    {
        DrawGCheckbox(App::gfx_reduce_shadows, _LC("GameSettings", "Shadow optimizations"));
        if (App::gfx_shadow_type->getEnum<GfxShadowType>() == GfxShadowType::PSSM)
        {
            DrawGIntSlider(App::gfx_shadow_quality, _LC("GameSettings", "Shadow quality"), 0, 3);
        }
    }

    DrawGCombo(App::gfx_sky_mode, _LC("GameSettings", "Sky gfx"),
        m_combo_items_sky_mode.c_str());

    if (App::gfx_sky_mode->getEnum<GfxSkyMode>() != GfxSkyMode::SKYX)
    {
        DrawGIntSlider(App::gfx_sight_range, _LC("GameSettings", "Sight range (meters)"), 100, 5000);
    }

    DrawGCombo(App::gfx_texture_filter , _LC("GameSettings", "Texture filtering"),
        m_combo_items_tex_filter.c_str());

    if (App::gfx_texture_filter->getEnum<GfxTexFilter>() == GfxTexFilter::ANISOTROPIC)
    {
        int anisotropy = Ogre::Math::Clamp(App::gfx_anisotropy->getInt(), 1, 16);
        int  selection = std::log2(anisotropy);
        if (ImGui::Combo(_LC("GameSettings", "Anisotropy"), &selection, "1\0""2\0""4\0""8\0""16\0\0"))
        {
            App::gfx_anisotropy->setVal(std::pow(2, selection));
        }
    }

    DrawGCombo(App::gfx_vegetation_mode, _LC("GameSettings", "Vegetation density"),
        m_combo_items_vegetation.c_str());

    DrawGCombo(App::gfx_water_mode, _LC("GameSettings", "Water gfx"),
        m_combo_items_water_mode.c_str());

    DrawGIntSlider(App::gfx_fps_limit,       _LC("GameSettings", "FPS limit"), 0, 240);

    DrawGIntCheck(App::gfx_particles_mode,   _LC("GameSettings", "Enable particle gfx"));
    DrawGIntCheck(App::gfx_skidmarks_mode,   _LC("GameSettings", "Enable skidmarks"));

    DrawGCheckbox(App::gfx_auto_lod,      _LC("GameSettings", "Enable automatic mesh LOD generator (Increases loading times)"));

    DrawGCheckbox(App::gfx_envmap_enabled,   _LC("GameSettings", "Realtime reflections"));
    if (App::gfx_envmap_enabled->getBool())
    {
        ImGui::PushItemWidth(125.f); // Width includes [+/-] buttons
        DrawGIntSlider(App::gfx_envmap_rate, _LC("GameSettings", "Realtime refl. update rate"), 0, 2);
        ImGui::PopItemWidth();
    }

    DrawGCheckbox(App::gfx_enable_videocams, _LC("GameSettings", "Render video cameras"));
    DrawGCheckbox(App::gfx_water_waves,      _LC("GameSettings", "Waves on water"));
    DrawGCheckbox(App::gfx_alt_actor_materials,      _LC("GameSettings", "Use alternate vehicle materials"));

    DrawGCombo(App::gfx_extcam_mode, "Exterior camera mode",
        m_combo_items_extcam_mode.c_str());

    DrawGIntSlider(App::gfx_camera_height, _LC("GameSettings", "Static camera height (meters)"), 1, 50);
    DrawGIntSlider(App::gfx_fov_external_default, _LC("GameSettings", "Exterior field of view"), 10, 120);
    DrawGIntSlider(App::gfx_fov_internal_default, _LC("GameSettings", "Interior field of view"), 10, 120);
}

void GameSettings::DrawDiagSettings()
{
    ImGui::TextDisabled("%s", _LC("GameSettings", "Diagnostic options"));
    ImGui::TextColored(ImVec4(0.89f,0.15f,0.21f,1.0f), "%s", _LC("GameSettings", "These settings are for advanced users only, you should only change these if you know what you're doing"));

    DrawGTextEdit(App::diag_preset_terrain,      _LC("GameSettings", "Preselected terrain"),         m_buf_diag_preset_terrain);
    DrawGTextEdit(App::diag_preset_vehicle,      _LC("GameSettings", "Preselected vehicle"),         m_buf_diag_preset_vehicle);
    DrawGTextEdit(App::diag_preset_veh_config,   _LC("GameSettings", "Presel. veh. config"),         m_buf_diag_preset_veh_config);
    DrawGCheckbox(App::diag_preset_veh_enter,    _LC("GameSettings", "Enter preselected vehicle"));
    DrawGCheckbox(App::diag_auto_spawner_report, _LC("GameSettings", "Auto actor spawner report"));
    DrawGCheckbox(App::diag_rig_log_node_import, _LC("GameSettings", "Log node import (spawn)"));
    DrawGCheckbox(App::diag_rig_log_node_stats,  _LC("GameSettings", "Log node stats (spawn)"));
    DrawGCheckbox(App::diag_camera,              _LC("GameSettings", "Debug camera (rails)"));
    DrawGCheckbox(App::diag_truck_mass,          _LC("GameSettings", "Debug actor mass"));
    DrawGCheckbox(App::diag_envmap,              _LC("GameSettings", "Debug realtime reflections"));
    DrawGCheckbox(App::diag_videocameras,        _LC("GameSettings", "Debug videocameras"));
    DrawGCheckbox(App::diag_warning_texture,     _LC("GameSettings", "Debug textures"));
    DrawGCheckbox(App::diag_hide_broken_beams,   _LC("GameSettings", "Hide broken beams"));
    DrawGCheckbox(App::diag_hide_wheel_info,     _LC("GameSettings", "Hide wheel info"));
    DrawGCheckbox(App::diag_hide_wheels,         _LC("GameSettings", "Hide wheels"));
    DrawGCheckbox(App::diag_hide_nodes,          _LC("GameSettings", "Hide nodes"));
    DrawGCheckbox(App::diag_log_console_echo,    _LC("GameSettings", "Echo log to console"));
    DrawGCheckbox(App::diag_log_beam_break,      _LC("GameSettings", "Log beam breaking"));
    DrawGCheckbox(App::diag_log_beam_deform,     _LC("GameSettings", "Log beam deforming"));
    DrawGCheckbox(App::diag_log_beam_trigger,    _LC("GameSettings", "Log beam triggers"));
    DrawGCheckbox(App::diag_allow_window_resize, _LC("GameSettings", "Allow game window resizing"));
    if (ImGui::Button(_LC("GameSettings", "Rebuild cache")))
    {
        App::GetGuiManager()->GameSettings.SetVisible(false);
        App::GetGameContext()->PushMessage(Message(MSG_APP_MODCACHE_PURGE_REQUESTED));
        App::GetGameContext()->PushMessage(Message(MSG_GUI_OPEN_MENU_REQUESTED));
    }
}

void GameSettings::DrawControlSettings()
{
    ImGui::TextDisabled("%s", _LC("GameSettings", "Controller options"));

    IoInputGrabMode io_input_grab_mode_old = App::io_input_grab_mode->getEnum<IoInputGrabMode>();
    DrawGCombo(App::io_input_grab_mode, _LC("GameSettings", "Input grab mode"), m_combo_items_input_grab.c_str());
    if (io_input_grab_mode_old != App::io_input_grab_mode->getEnum<IoInputGrabMode>())
    {
        App::GetGameContext()->PushMessage(Message(MSG_APP_REINIT_INPUT_REQUESTED));
        // This may take a second - display a 'please wait' box
        App::GetGuiManager()->LoadingWindow.SetProgress(
            App::GetGuiManager()->LoadingWindow.PERC_HIDE_PROGRESSBAR, 
            _LC("GameSettings", "Restarting input subsystem, please wait..."), /*render_frame:*/false);
    }

    DrawGFloatSlider(App::io_analog_smoothing,   _LC("GameSettings", "Analog Input Smoothing"),   0.5f, 2.0f);
    DrawGFloatSlider(App::io_analog_sensitivity, _LC("GameSettings", "Analog Input Sensitivity"), 0.5f, 2.0f);
    DrawGFloatSlider(App::io_blink_lock_range,   _LC("GameSettings", "Blinker Lock Range"),       0.1f, 1.0f);

    DrawGCheckbox(App::io_arcade_controls, _LC("GameSettings", "Use arcade controls"));
    DrawGCheckbox(App::io_invert_orbitcam, _LC("GameSettings", "Invert orbit camera"));

    DrawGCheckbox(App::io_ffb_enabled, _LC("GameSettings", "Enable ForceFeedback"));
    if (App::io_ffb_enabled->getBool())
    {
        ImGui::PushItemWidth(125.f);
        DrawGFloatBox(App::io_ffb_camera_gain, _LC("GameSettings", "FFB camera gain"));
        DrawGFloatBox(App::io_ffb_center_gain, _LC("GameSettings", "FFB center gain"));
        DrawGFloatBox(App::io_ffb_master_gain, _LC("GameSettings", "FFB master gain"));
        DrawGFloatBox(App::io_ffb_stress_gain, _LC("GameSettings", "FFB stress gain"));
        ImGui::PopItemWidth();
    }

    DrawGIntCheck(App::io_outgauge_mode, _LC("GameSettings", "Enable OutGauge protocol"));
    if (App::io_outgauge_mode->getBool())
    {
        DrawGTextEdit(App::io_outgauge_ip, _LC("GameSettings", "OutGauge IP"), m_buf_io_outgauge_ip);
        ImGui::PushItemWidth(125.f);
        DrawGIntBox(App::io_outgauge_port,    _LC("GameSettings", "OutGauge port"));
        DrawGIntBox(App::io_outgauge_id,      _LC("GameSettings", "OutGauge ID"));
        DrawGFloatBox(App::io_outgauge_delay, _LC("GameSettings", "OutGauge delay"));
        ImGui::PopItemWidth();
    }
}

void GameSettings::SetVisible(bool v)
{
    m_is_visible = v;
    if (!v && App::app_state->getEnum<AppState>() == RoR::AppState::MAIN_MENU)
    {
        App::GetGuiManager()->GameMainMenu.SetVisible(true);
    }

    // Pre-format combobox strings.
    if (m_combo_items_gearbox_mode == "")
    {
        ImAddItemToComboboxString(m_combo_items_gearbox_mode, ToLocalizedString(SimGearboxMode::AUTO));
        ImAddItemToComboboxString(m_combo_items_gearbox_mode, ToLocalizedString(SimGearboxMode::SEMI_AUTO));
        ImAddItemToComboboxString(m_combo_items_gearbox_mode, ToLocalizedString(SimGearboxMode::MANUAL));
        ImAddItemToComboboxString(m_combo_items_gearbox_mode, ToLocalizedString(SimGearboxMode::MANUAL_STICK));
        ImAddItemToComboboxString(m_combo_items_gearbox_mode, ToLocalizedString(SimGearboxMode::MANUAL_RANGES));
        ImTerminateComboboxString(m_combo_items_gearbox_mode);
    }

    if (m_combo_items_light_sources == "")
    {
        ImAddItemToComboboxString(m_combo_items_light_sources, ToLocalizedString(GfxFlaresMode::NONE));
        ImAddItemToComboboxString(m_combo_items_light_sources, ToLocalizedString(GfxFlaresMode::NO_LIGHTSOURCES));
        ImAddItemToComboboxString(m_combo_items_light_sources, ToLocalizedString(GfxFlaresMode::CURR_VEHICLE_HEAD_ONLY));
        ImAddItemToComboboxString(m_combo_items_light_sources, ToLocalizedString(GfxFlaresMode::ALL_VEHICLES_HEAD_ONLY));
        ImAddItemToComboboxString(m_combo_items_light_sources, ToLocalizedString(GfxFlaresMode::ALL_VEHICLES_ALL_LIGHTS));
        ImTerminateComboboxString(m_combo_items_light_sources);
    }

    if (m_combo_items_shadow_type == "")
    {
        ImAddItemToComboboxString(m_combo_items_shadow_type, ToLocalizedString(GfxShadowType::NONE));
        ImAddItemToComboboxString(m_combo_items_shadow_type, ToLocalizedString(GfxShadowType::PSSM));
        ImTerminateComboboxString(m_combo_items_shadow_type);
    }

    if (m_combo_items_sky_mode == "")
    {
        ImAddItemToComboboxString(m_combo_items_sky_mode, ToLocalizedString(GfxSkyMode::SANDSTORM));
        ImAddItemToComboboxString(m_combo_items_sky_mode, ToLocalizedString(GfxSkyMode::CAELUM));
        ImAddItemToComboboxString(m_combo_items_sky_mode, ToLocalizedString(GfxSkyMode::SKYX));
        ImTerminateComboboxString(m_combo_items_sky_mode);
    }

    if (m_combo_items_tex_filter == "")
    {
        ImAddItemToComboboxString(m_combo_items_tex_filter, ToLocalizedString(GfxTexFilter::NONE));
        ImAddItemToComboboxString(m_combo_items_tex_filter, ToLocalizedString(GfxTexFilter::BILINEAR));
        ImAddItemToComboboxString(m_combo_items_tex_filter, ToLocalizedString(GfxTexFilter::TRILINEAR));
        ImAddItemToComboboxString(m_combo_items_tex_filter, ToLocalizedString(GfxTexFilter::ANISOTROPIC));
        ImTerminateComboboxString(m_combo_items_tex_filter);
    }

    if (m_combo_items_vegetation == "")
    {
        ImAddItemToComboboxString(m_combo_items_vegetation, ToLocalizedString(GfxVegetation::NONE));
        ImAddItemToComboboxString(m_combo_items_vegetation, ToLocalizedString(GfxVegetation::x20PERC));
        ImAddItemToComboboxString(m_combo_items_vegetation, ToLocalizedString(GfxVegetation::x50PERC));
        ImAddItemToComboboxString(m_combo_items_vegetation, ToLocalizedString(GfxVegetation::FULL));
        ImTerminateComboboxString(m_combo_items_vegetation);
    }

    if (m_combo_items_water_mode == "")
    {
        ImAddItemToComboboxString(m_combo_items_water_mode, ToLocalizedString(GfxWaterMode::NONE));
        ImAddItemToComboboxString(m_combo_items_water_mode, ToLocalizedString(GfxWaterMode::BASIC));
        ImAddItemToComboboxString(m_combo_items_water_mode, ToLocalizedString(GfxWaterMode::REFLECT));
        ImAddItemToComboboxString(m_combo_items_water_mode, ToLocalizedString(GfxWaterMode::FULL_FAST));
        ImAddItemToComboboxString(m_combo_items_water_mode, ToLocalizedString(GfxWaterMode::FULL_HQ));
        ImAddItemToComboboxString(m_combo_items_water_mode, ToLocalizedString(GfxWaterMode::HYDRAX));
        ImTerminateComboboxString(m_combo_items_water_mode);
    }

    if (m_combo_items_extcam_mode == "")
    {
        ImAddItemToComboboxString(m_combo_items_extcam_mode, ToLocalizedString(GfxExtCamMode::NONE));
        ImAddItemToComboboxString(m_combo_items_extcam_mode, ToLocalizedString(GfxExtCamMode::STATIC));
        ImAddItemToComboboxString(m_combo_items_extcam_mode, ToLocalizedString(GfxExtCamMode::PITCHING));
        ImTerminateComboboxString(m_combo_items_extcam_mode);
    }

    if (m_combo_items_input_grab == "")
    {
        ImAddItemToComboboxString(m_combo_items_input_grab, ToLocalizedString(IoInputGrabMode::NONE));
        ImAddItemToComboboxString(m_combo_items_input_grab, ToLocalizedString(IoInputGrabMode::ALL));
        ImAddItemToComboboxString(m_combo_items_input_grab, ToLocalizedString(IoInputGrabMode::DYNAMIC));
        ImTerminateComboboxString(m_combo_items_input_grab);
    }

    if (m_combo_items_efx_reverb_engine == "")
    {
        ImAddItemToComboboxString(m_combo_items_efx_reverb_engine, ToLocalizedString(EfxReverbEngine::NONE));
        ImAddItemToComboboxString(m_combo_items_efx_reverb_engine, ToLocalizedString(EfxReverbEngine::REVERB));
        ImAddItemToComboboxString(m_combo_items_efx_reverb_engine, ToLocalizedString(EfxReverbEngine::EAXREVERB));
        ImTerminateComboboxString(m_combo_items_efx_reverb_engine);
    }

    if (m_cached_uipreset_combo_string == "")
    {
        ImAddItemToComboboxString(m_cached_uipreset_combo_string, ToLocalizedString(UiPreset::NOVICE));
        ImAddItemToComboboxString(m_cached_uipreset_combo_string, ToLocalizedString(UiPreset::REGULAR));
        ImAddItemToComboboxString(m_cached_uipreset_combo_string, ToLocalizedString(UiPreset::EXPERT));
        ImAddItemToComboboxString(m_cached_uipreset_combo_string, ToLocalizedString(UiPreset::MINIMALLIST));
        ImTerminateComboboxString(m_cached_uipreset_combo_string);
    }
}

void GameSettings::DrawUiPresetCombo()
{
    ImGui::PushID("uiPreset");

    ImGui::SetNextItemWidth(UI_SELECTOR_WIDTH);
    if (DrawGCombo(App::ui_preset, _LC("TopMenubar", "UI Preset"), m_cached_uipreset_combo_string.c_str()))
    {
        App::GetGuiManager()->ApplyUiPreset();
    }

    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        const float COLLUMNWIDTH_NAME = 175.f;
        const float COLLUMNWIDTH_VALUE = 60.f;
        // Hack to make space for the table (doesn't autoresize)
        ImGui::Dummy(ImVec2(COLLUMNWIDTH_NAME + COLLUMNWIDTH_VALUE*((int)UiPreset::Count), 1.f));

        // UiPresets table
        ImGui::Columns((int)UiPreset::Count + 1);
        ImGui::SetColumnWidth(0, COLLUMNWIDTH_NAME);
        for (int i = 0; i < (int)UiPreset::Count; i++)
        {
            ImGui::SetColumnWidth(i+1, COLLUMNWIDTH_VALUE);
        }

        // table header
        ImGui::TextDisabled("%s", "Setting");
        ImGui::NextColumn();
        for (int i = 0; i < (int)UiPreset::Count; i++)
        {
            ImGui::TextDisabled("%s", ToLocalizedString((UiPreset)i).c_str());
            ImGui::NextColumn();
        }

        // table body
        ImGui::Separator();

        int presetId = 0;
        while (UiPresets[presetId].uip_cvar != nullptr)
        {
            ImGui::Text("%s", UiPresets[presetId].uip_cvar);
            ImGui::NextColumn();
            for (int i = 0; i < (int)UiPreset::Count; i++)
            {
                ImGui::Text("%s", UiPresets[presetId].uip_values[i].c_str());
                ImGui::NextColumn();
            }

            presetId++;
        }

        // end table
        ImGui::Columns(1);
        ImGui::EndTooltip();
    }

    ImGui::PopID(); //"uiPreset"
}

void GameSettings::DrawUiDefaultDashboard(CacheEntryPtr& entry, CVar* cvar, CacheCategoryId category_id, const std::string& label)
{
    if (!entry || entry->fname != cvar->getStr())
    {
        entry = App::GetCacheSystem()->FindEntryByFilename(LT_DashBoard, /* partial: */false, cvar->getStr());
    }

    ImGui::AlignTextToFramePadding();
    std::string caption = fmt::format("{}##truck_dash", entry ? entry->dname : cvar->getStr());
    if (ImGui::Button(caption.c_str(), ImVec2(UI_SELECTOR_WIDTH, 0.f)))
    {
        default_dash_being_selected = category_id;
        LoaderType* payload = new LoaderType(LoaderType::LT_DashBoard);
        App::GetGameContext()->PushMessage(Message(MSG_GUI_OPEN_SELECTOR_REQUESTED, (void*)payload));
    }
    ImGui::SameLine();
    ImGui::Text("%s", label.c_str());
}
