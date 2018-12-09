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

#include "GUIManager.h"
#include "GUIUtils.h"
#include "OgreSubsystem.h"

void RoR::GUI::GameSettings::Draw()
{
    bool is_visible = true;
    const int flags = ImGuiWindowFlags_NoCollapse;
    ImGui::SetNextWindowSize(ImVec2(600.f, 400.f), ImGuiSetCond_FirstUseEver);
    ImGui::SetNextWindowPosCenter(ImGuiSetCond_Appearing);
    ImGui::Begin("Game settings", &is_visible, flags);
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

    if (ImGui::Button("Render System")) { m_tab = SettingsTab::RENDER_SYSTEM; }
    ImGui::SameLine();
    if (ImGui::Button("General"))       { m_tab = SettingsTab::GENERAL;       }
    ImGui::SameLine();
    if (ImGui::Button("Graphics"))      { m_tab = SettingsTab::GRAPHICS;      }
    ImGui::SameLine();
#ifdef USE_OPENAL
    if (ImGui::Button("Audio"))         { m_tab = SettingsTab::AUDIO;         }
    ImGui::SameLine();
#endif // USE_OPENAL
    if (ImGui::Button("Controls"))      { m_tab = SettingsTab::CONTROL;       }
    ImGui::SameLine();
    if (ImGui::Button("Diagnostic"))    { m_tab = SettingsTab::DIAG;          }

    ImGui::PopStyleVar(1);

    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 4.f);
    ImGui::Separator();

    if (m_tab == SettingsTab::RENDER_SYSTEM)
    {
        ImGui::TextDisabled("Render system");

        const Ogre::RenderSystemList render_systems = App::GetOgreSubsystem()->GetOgreRoot()->getAvailableRenderers();
        std::string render_system_names;
        for (auto rs : render_systems)
        {
            render_system_names += rs->getName() + '\0';
        }
        const auto rs = App::GetOgreSubsystem()->GetOgreRoot()->getRenderSystem();
        const auto it = std::find(render_systems.begin(), render_systems.end(), rs);
        int render_id = it != render_systems.end() ? std::distance(render_systems.begin(), it) : 0;
        if (ImGui::Combo("Render System", &render_id, render_system_names.c_str()))
        {
            App::GetOgreSubsystem()->GetOgreRoot()->setRenderSystem(render_systems[render_id]); // Can we do that here?
        }

        const Ogre::ConfigOptionMap config_options = rs->getConfigOptions();
        for (auto opt : config_options)
        {
            auto co = opt.second;
            if (co.immutable)
                continue;
            if (co.possibleValues.empty())
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
                    App::GetOgreSubsystem()->GetOgreRoot()->saveConfig();
                }
            }
        }
    }
    else if (m_tab == SettingsTab::GENERAL)
    {
        ImGui::TextDisabled("Application settings");

        int lang_selection = 0;
        ImGui::Combo("Language", &lang_selection, "English\0\0"); // Dummy; TODO: List available languages

        int sshot_select = (std::strcmp(App::app_screenshot_format.GetActive(),"jpg") == 0) ? 1 : 0; // Hardcoded; TODO: list available formats.
        if (ImGui::Combo("Screenshot format", &sshot_select, "png\0jpg\0\0"))
        {
            App::app_screenshot_format.SetActive((sshot_select == 1) ? "jpg" : "png");
        }

        DrawGCheckbox(App::app_skip_main_menu, "Skip main menu");
        DrawGCheckbox(App::app_async_physics,  "Async physics");

        ImGui::Separator();
        ImGui::TextDisabled("Simulation settings");

        DrawGCombo(App::sim_gearbox_mode, "Gearbox mode",
            "Automatic shift\0"
            "Manual shift - Auto clutch\0"
            "Fully Manual: sequential shift\0"
            "Fully manual: stick shift\0"
            "Fully Manual: stick shift with ranges\00");

        DrawGCheckbox(App::gfx_speedo_digital,     "Digital speedometer");
        DrawGCheckbox(App::gfx_speedo_imperial,    "Imperial speedometer");

        //DrawGCheckbox(App::gfx_flexbody_lods,      "Enable flexbody LODs");
        //DrawGCheckbox(App::gfx_flexbody_cache,     "Enable flexbody cache");

        DrawGCheckbox(App::sim_replay_enabled,     "Replay mode");
        if (App::sim_replay_enabled.GetActive())
        {
            DrawGIntBox(App::sim_replay_length,    "Replay length");
            DrawGIntBox(App::sim_replay_stepping,  "Replay stepping");
        }

        DrawGCheckbox(App::sim_position_storage,   "Use position storage");

        DrawGCheckbox(App::sim_no_self_collisions, "No intra truck collisions");
        DrawGCheckbox(App::sim_no_collisions,      "No inter truck collisions");
    }
#ifdef USE_OPENAL
    else if (m_tab == SettingsTab::AUDIO)
    {
        ImGui::TextDisabled("Audio settings");

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
        if (ImGui::Combo("Audio device", &device_id, devices))
        {
            App::audio_device_name.SetActive(audio_devices[device_id].c_str());
        }

        DrawGCheckbox(App::audio_enable_creak,     "Creak sound");
        DrawGCheckbox(App::audio_menu_music,       "Main menu music");
        DrawGFloatSlider(App::audio_master_volume, "Master volume", 0, 1);
    }
#endif // USE_OPENAL
    else if (m_tab == SettingsTab::GRAPHICS)
    {
        ImGui::TextDisabled("Video settings");

        DrawGCombo(App::gfx_flares_mode, "Lights",
            "None (fastest)\0"
            "No light sources\0"
            "Only current vehicle, main lights\0"
            "All vehicles, main lights\0"
            "All vehicles, all lights\0\0");

        DrawGCombo(App::gfx_shadow_type, "Shadow type",
            "Disabled\0"
            "Texture\0"
            "PSSM\0\0");

        if (App::gfx_shadow_type.GetActive() != GfxShadowType::NONE)
        {
            DrawGCheckbox(App::gfx_reduce_shadows, "Shadow optimizations");
            if (App::gfx_shadow_type.GetActive() == GfxShadowType::PSSM)
            {
                DrawGIntSlider(App::gfx_shadow_quality, "Shadow quality", 0, 3);
            }
        }

        DrawGCombo(App::gfx_extcam_mode, "Exterior camera mode",
            "None\0"
            "Static\0"
            "Pitching\0\0");

        DrawGCombo(App::gfx_sky_mode, "Sky gfx",
            "Sandstorm (fastest)\0"
            "Caelum (best looking, slower)\0"
            "SkyX (best looking, slower)\0\0");

        DrawGCombo(App::gfx_texture_filter , "Texture filtering",
            "None\0"
            "Bilinear\0"
            "Trilinear\0"
            "Anisotropic\0\0");

        if (App::gfx_texture_filter.GetActive() == GfxTexFilter::ANISOTROPIC)
        {
            int anisotropy = Ogre::Math::Clamp(App::gfx_anisotropy.GetActive(), 1, 16);
            int  selection = std::log2(anisotropy);
            if (ImGui::Combo("Anisotropy", &selection, "1\0""2\0""4\0""8\0""16\0\0"))
            {
                App::gfx_anisotropy.SetActive(std::pow(2, selection));
            }
        }

        DrawGCombo(App::gfx_vegetation_mode, "Vegetation density",
            "None\0"
            "20%\0"
            "50%\0"
            "Full\0\0");

        DrawGCombo(App::gfx_water_mode, "Water gfx",
            "None\0"
            "Basic (fastest)\0"
            "Reflection\0"
            "Reflection + refraction (speed optimized)\0"
            "Reflection + refraction (quality optimized)\0"
            "HydraX\0\0");

        DrawGIntCheck(App::gfx_particles_mode,   "Enable particle gfx");
        DrawGIntCheck(App::gfx_skidmarks_mode,   "Enable skidmarks");

        DrawGCheckbox(App::gfx_envmap_enabled,   "Realtime reflections");
        if (App::gfx_envmap_enabled.GetActive())
        {
            ImGui::PushItemWidth(125.f); // Width includes [+/-] buttons
            DrawGIntSlider(App::gfx_envmap_rate,     "Realtime refl. update rate", 0, 6);
            ImGui::PopItemWidth();
        }

        DrawGCheckbox(App::gfx_minimap_enabled,  "Minimap enabled");
        DrawGCheckbox(App::gfx_enable_videocams, "Render video cameras");
        DrawGCheckbox(App::gfx_water_waves,      "Waves on water");

        ImGui::PushItemWidth(125.f);
        DrawGIntBox(App::gfx_fps_limit,      "FPS limit");
        DrawGFloatBox(App::gfx_sight_range,  "Sight range (meters)");
        DrawGFloatBox(App::gfx_fov_external, "Exterior FOV (field of view)");
        DrawGFloatBox(App::gfx_fov_internal, "Interior FOV (field of view)");
        ImGui::PopItemWidth();

    }
    else if (m_tab == SettingsTab::DIAG)
    {
        ImGui::TextDisabled("Diagnostic options");

        DrawGCheckbox(App::diag_auto_spawner_report, "Auto actor spawner report");
        DrawGCheckbox(App::diag_rig_log_node_import, "Log node import (spawn)");
        DrawGCheckbox(App::diag_rig_log_node_stats,  "Log node stats (spawn)");
        DrawGCheckbox(App::diag_rig_log_messages,    "Log messages (spawn)");
        DrawGCheckbox(App::diag_camera,              "Debug camera (rails)");
        DrawGCheckbox(App::diag_collisions,          "Debug collisions");
        DrawGCheckbox(App::diag_truck_mass,          "Debug actor mass");
        DrawGCheckbox(App::diag_envmap,              "Debug realtime reflections");
        DrawGCheckbox(App::diag_videocameras,        "Debug videocameras");
        DrawGCheckbox(App::diag_preset_veh_enter,    "Enter preselected vehicle");
        DrawGCheckbox(App::diag_log_console_echo,    "Echo log to console");
        DrawGCheckbox(App::diag_log_beam_break,      "Log beam breaking");
        DrawGCheckbox(App::diag_log_beam_deform,     "Log beam deforming");
        DrawGCheckbox(App::diag_log_beam_trigger,    "Log beam triggers");
        DrawGTextEdit(App::diag_preset_terrain,      "Preselected terrain",         m_buf_diag_preset_terrain, false);
        DrawGTextEdit(App::diag_preset_vehicle,      "Preselected vehicle",         m_buf_diag_preset_vehicle);
        DrawGTextEdit(App::diag_preset_veh_config,   "Presel. veh. config",         m_buf_diag_preset_veh_config);
        DrawGTextEdit(App::diag_extra_resource_dir,  "Extra resources directory",   m_buf_diag_extra_resource_dir);
    }
    else if (m_tab == SettingsTab::CONTROL)
    {
        ImGui::TextDisabled("Controller options");

        DrawGCheckbox(App::io_ffb_enabled, "Enable ForceFeedback");
        if (App::io_ffb_enabled.GetActive())
        {
            ImGui::PushItemWidth(125.f);
            DrawGFloatBox(App::io_ffb_camera_gain, "FFB camera gain");
            DrawGFloatBox(App::io_ffb_center_gain, "FFB center gain");
            DrawGFloatBox(App::io_ffb_master_gain, "FFB master gain");
            DrawGFloatBox(App::io_ffb_stress_gain, "FFB stress gain");
            ImGui::PopItemWidth();
        }

        DrawGCombo(App::io_input_grab_mode, "Input grab mode",
            "None\0"
            "All\0"
            "Dynamic\0\0");

        DrawGCheckbox(App::io_arcade_controls, "Use arcade controls");
        DrawGIntCheck(App::io_outgauge_mode, "Enable OutGauge protocol");
        if (App::io_outgauge_mode.GetActive())
        {
            DrawGTextEdit(App::io_outgauge_ip, "OutGauge IP", m_buf_io_outgauge_ip);
            ImGui::PushItemWidth(125.f);
            DrawGIntBox(App::io_outgauge_port,    "OutGauge port");
            DrawGIntBox(App::io_outgauge_id,      "OutGauge ID");
            DrawGFloatBox(App::io_outgauge_delay, "OutGauge delay");
            ImGui::PopItemWidth();
        }
    }

    ImGui::End();
}
