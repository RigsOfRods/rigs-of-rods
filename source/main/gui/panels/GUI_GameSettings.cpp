/*
    This source file is part of Rigs of Rods
    Copyright 2016-2017 Petr Ohlidal & contributors

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

#include "CacheSystem.h"
#include "GUIManager.h"
#include "GUIUtils.h"
#include "OgreSubsystem.h"
#include "Language.h"

void RoR::GUI::GameSettings::Draw()
{
    bool is_visible = true;
    const int flags = ImGuiWindowFlags_NoCollapse;
    ImGui::SetNextWindowSize(ImVec2(600.f, 400.f), ImGuiSetCond_FirstUseEver);
    ImGui::SetNextWindowPosCenter(ImGuiSetCond_Appearing);
    ImGui::Begin(_LC("GameSettings", "Game settings"), &is_visible, flags);
    if (! is_visible)
    {
        this->SetVisible(false);
        if (App::app_state.GetActive() == RoR::AppState::MAIN_MENU)
        {
            App::GetGuiManager()->SetVisible_GameMainMenu(true);
        }
        ImGui::End();
        return;
    }

    // 'Tabs' buttons
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4.f, 8.f));

    if (ImGui::Button(_LC("GameSettings", "Render System"))) { m_tab = SettingsTab::RENDER_SYSTEM; }
    ImGui::SameLine();
    if (ImGui::Button(_LC("GameSettings", "General")))       { m_tab = SettingsTab::GENERAL;       }
    ImGui::SameLine();
    if (ImGui::Button(_LC("GameSettings", "Graphics")))      { m_tab = SettingsTab::GRAPHICS;      }
    ImGui::SameLine();
#ifdef USE_OPENAL
    if (ImGui::Button(_LC("GameSettings", "Audio")))         { m_tab = SettingsTab::AUDIO;         }
    ImGui::SameLine();
#endif // USE_OPENAL
    if (ImGui::Button(_LC("GameSettings", "Controls")))      { m_tab = SettingsTab::CONTROL;       }
    ImGui::SameLine();
    if (ImGui::Button(_LC("GameSettings", "Diagnostic")))    { m_tab = SettingsTab::DIAG;          }

    ImGui::PopStyleVar(1);

    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 4.f);
    ImGui::Separator();

    if (m_tab == SettingsTab::RENDER_SYSTEM)
    {
        ImGui::TextDisabled(_LC("GameSettings", "Render system (changes require a restart)"));

        const auto ogre_root = App::GetOgreSubsystem()->GetOgreRoot();
        const auto render_systems = ogre_root->getAvailableRenderers();
        std::string render_system_names;
        for (auto rs : render_systems)
        {
            render_system_names += rs->getName() + '\0';
        }
        const auto ro = ogre_root->getRenderSystemByName(App::app_rendersys_override.GetActive());
        const auto rs = ro ? ro : ogre_root->getRenderSystem();
        const auto it = std::find(render_systems.begin(), render_systems.end(), rs);
        int render_id = it != render_systems.end() ? std::distance(render_systems.begin(), it) : 0;
        /* Combobox for selecting the Render System*/
        if (ImGui::Combo(_LC ("GameSettings", "Render System"), &render_id, render_system_names.c_str()))
        {
            App::app_rendersys_override.SetActive(render_systems[render_id]->getName().c_str());
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
                rs->setConfigOption(co.name, co.possibleValues[option_id]);
                if (rs->validateConfigOptions().empty())
                {
                    ogre_root->saveConfig();
                }
            }
        }
    }
    else if (m_tab == SettingsTab::GENERAL)
    {
        ImGui::TextDisabled(_LC("GameSettings", "Application settings"));

#ifndef NOLANG
        std::vector<std::pair<std::string, std::string>> languages = LanguageEngine::getSingleton().getLanguages();
        std::string lang_values;
        for (auto value : languages)
        {
            lang_values += value.first + '\0';
        }
        const auto it = std::find_if(languages.begin(), languages.end(),
                [](const std::pair<std::string, std::string>& l) { return l.second == App::app_language.GetActive(); });
        int lang_selection = it != languages.end() ? std::distance(languages.begin(), it) : 0;
        if (ImGui::Combo(_LC("GameSettings", "Language"), &lang_selection, lang_values.c_str()))
        {
            App::app_language.SetActive(languages[lang_selection].second.c_str());
            LanguageEngine::getSingleton().setup();
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
            const auto it = std::find(countries.begin(), countries.end(), std::string(App::app_country.GetActive()));
            int country_selection = it != countries.end() ? std::distance(countries.begin(), it) : 0;
            if (ImGui::Combo(_LC("GameSettings", "Country"), &country_selection, country_values.c_str()))
            {
                App::app_country.SetActive(countries[country_selection].c_str());
            }
        }

        int sshot_select = (std::strcmp(App::app_screenshot_format.GetActive(),"jpg") == 0) ? 1 : 0; // Hardcoded; TODO: list available formats.

        /* Screenshot format: Can be png or jpg*/
        if (ImGui::Combo(_LC("GameSettings", "Screenshot format"), &sshot_select, "png\0jpg\0\0"))
        {
            App::app_screenshot_format.SetActive((sshot_select == 1) ? "jpg" : "png");
        }

        DrawGTextEdit(App::app_extra_mod_path, _LC("GameSettings", "Extra mod path"),  m_buf_app_extra_mod_dir);

        DrawGCheckbox(App::app_skip_main_menu, _LC("GameSettings", "Skip main menu"));
        DrawGCheckbox(App::app_async_physics, _LC("GameSettings", "Async physics"));

        ImGui::Separator();
        ImGui::TextDisabled(_LC("GameSettings", "Simulation settings"));

        DrawGCombo(App::sim_gearbox_mode, _LC("GameSettings", "Gearbox mode"),
            "Automatic shift\0"
            "Manual shift - Auto clutch\0"
            "Fully Manual: sequential shift\0"
            "Fully manual: stick shift\0"
            "Fully Manual: stick shift with ranges\00");

        DrawGCheckbox(App::gfx_speedo_digital, _LC("GameSettings", "Digital speedometer"));
        DrawGCheckbox(App::gfx_speedo_imperial, _LC("GameSettings", "Imperial speedometer"));

        //DrawGCheckbox(App::gfx_flexbody_lods,      "Enable flexbody LODs");
        //DrawGCheckbox(App::gfx_flexbody_cache,     "Enable flexbody cache");

        DrawGCheckbox(App::sim_spawn_running, _LC("GameSettings", "Engines spawn running"));

        DrawGCheckbox(App::sim_replay_enabled, _LC("GameSettings", "Replay mode"));
        if (App::sim_replay_enabled.GetActive())
        {
            DrawGIntBox(App::sim_replay_length, _LC("GameSettings", "Replay length"));
            DrawGIntBox(App::sim_replay_stepping, _LC("GameSettings", "Replay stepping"));
        }

        DrawGCheckbox(App::sim_position_storage, _LC("GameSettings", "Use position storage"));

        DrawGCheckbox(App::sim_races_enabled, _LC("GameSettings", "Enable races"));

        DrawGCheckbox(App::sim_no_self_collisions, _LC("GameSettings", "No intra truck collisions"));
        DrawGCheckbox(App::sim_no_collisions, _LC("GameSettings", "No inter truck collisions"));
    }
#ifdef USE_OPENAL
    else if (m_tab == SettingsTab::AUDIO)
    {
        ImGui::TextDisabled(_LC("GameSettings", "Audio settings"));

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

        const auto it = std::find(audio_devices.begin(), audio_devices.end(), App::audio_device_name.GetActive());
        int device_id = it != audio_devices.end() ? std::distance(audio_devices.begin(), it) : 0;
        if (ImGui::Combo(_LC("GameSettings", "Audio device"), &device_id, devices))
        {
            App::audio_device_name.SetActive(audio_devices[device_id].c_str());
        }

        DrawGCheckbox(App::audio_enable_creak,     _LC("GameSettings", "Creak sound"));
        DrawGCheckbox(App::audio_menu_music,       _LC("GameSettings", "Main menu music"));
        DrawGFloatSlider(App::audio_master_volume, _LC("GameSettings", "Master volume"), 0, 1);
    }
#endif // USE_OPENAL
    else if (m_tab == SettingsTab::GRAPHICS)
    {
        ImGui::TextDisabled(_LC("GameSettings", "Video settings"));

        DrawGCombo(App::gfx_flares_mode, _LC("GameSettings", "Lights"),
            "None (fastest)\0"
            "No light sources\0"
            "Only current vehicle, main lights\0"
            "All vehicles, main lights\0"
            "All vehicles, all lights\0\0");

        DrawGCombo(App::gfx_shadow_type, _LC("GameSettings", "Shadow type"),
            "Disabled\0"
            "Texture\0"
            "PSSM\0\0");

        if (App::gfx_shadow_type.GetActive() != GfxShadowType::NONE)
        {
            DrawGCheckbox(App::gfx_reduce_shadows, _LC("GameSettings", "Shadow optimizations"));
            if (App::gfx_shadow_type.GetActive() == GfxShadowType::PSSM)
            {
                DrawGIntSlider(App::gfx_shadow_quality, _LC("GameSettings", "Shadow quality"), 0, 3);
            }
        }

        DrawGCombo(App::gfx_sky_mode, "Sky gfx",
            "Sandstorm (fastest)\0"
            "Caelum (best looking, slower)\0"
            "SkyX (best looking, slower)\0\0");

        DrawGIntSlider(App::gfx_sight_range, _LC("GameSettings", "Sight range (meters)"), 100, 5000);

        DrawGCombo(App::gfx_texture_filter , _LC("GameSettings", "Texture filtering"),
            "None\0"
            "Bilinear\0"
            "Trilinear\0"
            "Anisotropic\0\0");

        if (App::gfx_texture_filter.GetActive() == GfxTexFilter::ANISOTROPIC)
        {
            int anisotropy = Ogre::Math::Clamp(App::gfx_anisotropy.GetActive(), 1, 16);
            int  selection = std::log2(anisotropy);
            if (ImGui::Combo(_LC("GameSettings", "Anisotropy"), &selection, "1\0""2\0""4\0""8\0""16\0\0"))
            {
                App::gfx_anisotropy.SetActive(std::pow(2, selection));
            }
        }

        DrawGCombo(App::gfx_vegetation_mode, _LC("GameSettings", "Vegetation density"),
            "None\0"
            "20%\0"
            "50%\0"
            "Full\0\0");

        DrawGCombo(App::gfx_water_mode, _LC("GameSettings", "Water gfx"),
            "None\0"
            "Basic (fastest)\0"
            "Reflection\0"
            "Reflection + refraction (speed optimized)\0"
            "Reflection + refraction (quality optimized)\0"
            "HydraX\0\0");

        DrawGIntSlider(App::gfx_fps_limit,       _LC("GameSettings", "FPS limit"), 0, 240);

        DrawGIntCheck(App::gfx_particles_mode,   _LC("GameSettings", "Enable particle gfx"));
        DrawGIntCheck(App::gfx_skidmarks_mode,   _LC("GameSettings", "Enable skidmarks"));

        DrawGCheckbox(App::gfx_envmap_enabled,   _LC("GameSettings", "Realtime reflections"));
        if (App::gfx_envmap_enabled.GetActive())
        {
            ImGui::PushItemWidth(125.f); // Width includes [+/-] buttons
            DrawGIntSlider(App::gfx_envmap_rate, _LC("GameSettings", "Realtime refl. update rate"), 0, 6);
            ImGui::PopItemWidth();
        }

        DrawGCheckbox(App::gfx_enable_videocams, _LC("GameSettings", "Render video cameras"));
        DrawGCheckbox(App::gfx_surveymap_icons,  _LC("GameSettings", "Overview map icons"));
        if (App::gfx_surveymap_icons.GetActive())
        {
            DrawGCheckbox(App::gfx_declutter_map,  _LC("GameSettings", "Declutter overview map"));
        }
        DrawGCheckbox(App::gfx_water_waves,      _LC("GameSettings", "Waves on water"));

        DrawGCombo(App::gfx_extcam_mode, "Exterior camera mode",
            "None\0"
            "Static\0"
            "Pitching\0\0");

        DrawGIntSlider(App::gfx_fov_external, _LC("GameSettings", "Exterior field of view"), 10, 120);
        DrawGIntSlider(App::gfx_fov_internal, _LC("GameSettings", "Interior field of view"), 10, 120);
    }
    else if (m_tab == SettingsTab::DIAG)
    {
        ImGui::TextDisabled(_LC("GameSettings", "Diagnostic options"));

        int physics_fps = std::round(1.0f / App::diag_physics_dt.GetActive());
        if (ImGui::SliderInt(_LC("GameSettings", "Physics frames per second"), &physics_fps, 2000, 10000))
        {
            App::diag_physics_dt.SetActive(Ogre::Math::Clamp(1.0f / physics_fps, 0.0001f, 0.0005f));
        }
        DrawGTextEdit(App::diag_preset_terrain,      _LC("GameSettings", "Preselected terrain"),         m_buf_diag_preset_terrain, false);
        DrawGTextEdit(App::diag_preset_vehicle,      _LC("GameSettings", "Preselected vehicle"),         m_buf_diag_preset_vehicle);
        DrawGTextEdit(App::diag_preset_veh_config,   _LC("GameSettings", "Presel. veh. config"),         m_buf_diag_preset_veh_config);
        DrawGCheckbox(App::diag_preset_veh_enter,    _LC("GameSettings", "Enter preselected vehicle"));
        DrawGCheckbox(App::diag_auto_spawner_report, _LC("GameSettings", "Auto actor spawner report"));
        DrawGCheckbox(App::diag_rig_log_node_import, _LC("GameSettings", "Log node import (spawn)"));
        DrawGCheckbox(App::diag_rig_log_node_stats,  _LC("GameSettings", "Log node stats (spawn)"));
        DrawGCheckbox(App::diag_rig_log_messages,    _LC("GameSettings", "Log messages (spawn)"));
        DrawGCheckbox(App::diag_camera,              _LC("GameSettings", "Debug camera (rails)"));
        DrawGCheckbox(App::diag_collisions,          _LC("GameSettings", "Debug collisions"));
        DrawGCheckbox(App::diag_truck_mass,          _LC("GameSettings", "Debug actor mass"));
        DrawGCheckbox(App::diag_envmap,              _LC("GameSettings", "Debug realtime reflections"));
        DrawGCheckbox(App::diag_videocameras,        _LC("GameSettings", "Debug videocameras"));
        DrawGCheckbox(App::diag_warning_texture,     _LC("GameSettings", "Debug textures"));
        DrawGCheckbox(App::diag_log_console_echo,    _LC("GameSettings", "Echo log to console"));
        DrawGCheckbox(App::diag_log_beam_break,      _LC("GameSettings", "Log beam breaking"));
        DrawGCheckbox(App::diag_log_beam_deform,     _LC("GameSettings", "Log beam deforming"));
        DrawGCheckbox(App::diag_log_beam_trigger,    _LC("GameSettings", "Log beam triggers"));
        if (ImGui::Button(_LC("GameSettings", "Trigger cache update (and quit)")))
        {
            App::app_force_cache_udpate.SetActive(true);
            App::app_state.SetPending(AppState::SHUTDOWN);
        }
        if (ImGui::Button(_LC("GameSettings", "Clear cache (and quit)")))
        {
            std::remove(App::GetCacheSystem()->getCacheConfigFilename().c_str());
            App::app_state.SetPending(AppState::SHUTDOWN);
        }
    }
    else if (m_tab == SettingsTab::CONTROL)
    {
        ImGui::TextDisabled(_LC("GameSettings", "Controller options"));

        DrawGCheckbox(App::io_ffb_enabled, _LC("GameSettings", "Enable ForceFeedback"));
        if (App::io_ffb_enabled.GetActive())
        {
            ImGui::PushItemWidth(125.f);
            DrawGFloatBox(App::io_ffb_camera_gain, _LC("GameSettings", "FFB camera gain"));
            DrawGFloatBox(App::io_ffb_center_gain, _LC("GameSettings", "FFB center gain"));
            DrawGFloatBox(App::io_ffb_master_gain, _LC("GameSettings", "FFB master gain"));
            DrawGFloatBox(App::io_ffb_stress_gain, _LC("GameSettings", "FFB stress gain"));
            ImGui::PopItemWidth();
        }

        DrawGCombo(App::io_input_grab_mode, _LC("GameSettings", "Input grab mode"),
            "None\0"
            "All\0"
            "Dynamic\0\0");

        DrawGCheckbox(App::io_arcade_controls, _LC("GameSettings", "Use arcade controls"));
        DrawGIntCheck(App::io_outgauge_mode, _LC("GameSettings", "Enable OutGauge protocol"));
        if (App::io_outgauge_mode.GetActive())
        {
            DrawGTextEdit(App::io_outgauge_ip, _LC("GameSettings", "OutGauge IP"), m_buf_io_outgauge_ip);
            ImGui::PushItemWidth(125.f);
            DrawGIntBox(App::io_outgauge_port,    _LC("GameSettings", "OutGauge port"));
            DrawGIntBox(App::io_outgauge_id,      _LC("GameSettings", "OutGauge ID"));
            DrawGFloatBox(App::io_outgauge_delay, _LC("GameSettings", "OutGauge delay"));
            ImGui::PopItemWidth();
        }
    }

    ImGui::End();
}
