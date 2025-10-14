/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2013-2024 Petr Ohlidal

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

#include "GUI_VehicleInfoTPanel.h"

#include "Application.h"
#include "Actor.h"
#include "SimData.h"
#include "Language.h"
#include "Engine.h"
#include "GameContext.h"
#include "GfxActor.h"
#include "GUIManager.h"
#include "Utils.h"
#include "GUIUtils.h"

using namespace RoR;
using namespace GUI;

const float HELP_TEXTURE_WIDTH = 512.f;
const float HELP_TEXTURE_HEIGHT = 80.f;
const float HELP_TEXTURE_HEIGHT_FULL = 128.f;
const ImVec2 MAX_PREVIEW_SIZE(100.f, 100.f);
const float MIN_PANEL_WIDTH = 230.f;

float VehicleInfoTPanel::GetPanelWidth()
{
    return m_helptext_fullsize ? std::max(MIN_PANEL_WIDTH, HELP_TEXTURE_WIDTH) : MIN_PANEL_WIDTH;
}

void VehicleInfoTPanel::Draw(RoR::GfxActor* actorx)
{
    // === DETERMINE VISIBILITY ===

    // Show only once for 5 sec, with a notice
    bool show_translucent = false;
    if (App::ui_show_vehicle_buttons->getBool() && actorx && !m_startupdemo_init)
    {
        m_startupdemo_timer.reset();
        App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE,
                                      fmt::format(_LC("VehicleButtons", "Hover the mouse on the left to see controls")), "lightbulb.png");
        m_startupdemo_init = true;
    }
    if (App::ui_show_vehicle_buttons->getBool() && m_startupdemo_timer.getMilliseconds() < 5000)
    {
        show_translucent = true;
    }

    // Show when mouse is on the left of screen
    if (m_visibility_mode != TPANELMODE_OPAQUE 
        && App::ui_show_vehicle_buttons->getBool()
        && App::GetGuiManager()->AreStaticMenusAllowed()
        && (ImGui::GetIO().MousePos.x <= MIN_PANEL_WIDTH + ImGui::GetStyle().WindowPadding.x*2))
    {
        show_translucent = true;
    }

    if (show_translucent && m_visibility_mode != TPANELMODE_OPAQUE)
    {
        m_visibility_mode = TPANELMODE_TRANSLUCENT;
    }
    else if (!show_translucent && m_visibility_mode != TPANELMODE_OPAQUE)
    {
        m_visibility_mode = TPANELMODE_HIDDEN;
    }

    if (m_visibility_mode == TPANELMODE_HIDDEN)
    {
        return;
    }

    // === OPEN IMGUI WINDOW ===

    GUIManager::GuiTheme& theme = App::GetGuiManager()->GetTheme();
    
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBringToFrontOnFocus;

    ImGui::SetNextWindowPos(ImVec2(theme.screen_edge_padding.x, (theme.screen_edge_padding.y + 110)));

    if (m_current_focus == TPANELFOCUS_COMMANDS)
    {
      ImGui::SetNextWindowContentWidth(this->GetPanelWidth());
    }
    else
    {
      ImGui::SetNextWindowContentWidth(MIN_PANEL_WIDTH);
    }

    switch (m_visibility_mode)
    {
        case TPANELMODE_OPAQUE:
            ImGui::PushStyleColor(ImGuiCol_WindowBg, theme.semitransparent_window_bg);
            ImGui::PushStyleColor(ImGuiCol_TextDisabled, ImGui::GetStyle().Colors[ImGuiCol_TextDisabled]);
            break;

        case TPANELMODE_TRANSLUCENT:
            ImGui::PushStyleColor(ImGuiCol_WindowBg, m_panel_translucent_color);
            ImGui::PushStyleColor(ImGuiCol_TextDisabled, m_transluc_textdis_color);
            break;

        default:
            break;
    }
    ImGui::Begin("VehicleInfoTPanel", nullptr, flags);

    // === DECIDE WHAT THE WINDOW WILL DISPLAY ===

    int tabflags_basics = ImGuiTabItemFlags_None;
    int tabflags_stats = ImGuiTabItemFlags_None;
    int tabflags_commands = ImGuiTabItemFlags_None;
    int tabflags_diag = ImGuiTabItemFlags_None;
    if (m_requested_focus != TPANELFOCUS_NONE)
    {
        switch (m_requested_focus)
        {
            case TPANELFOCUS_BASICS: tabflags_basics = ImGuiTabItemFlags_SetSelected; break;
            case TPANELFOCUS_STATS:  tabflags_stats = ImGuiTabItemFlags_SetSelected; break;
            case TPANELFOCUS_DIAG:   tabflags_diag = ImGuiTabItemFlags_SetSelected; break;
            case TPANELFOCUS_COMMANDS: tabflags_commands = ImGuiTabItemFlags_SetSelected; break;
            default:;
        }
        
        // Reset the request
        m_requested_focus = TPANELFOCUS_NONE;
    }

    // === DRAW THE WINDOW HEADER - MINI IMAGE (if available) AND VEHICLE NAME ===

    ImVec2 name_pos = ImGui::GetCursorPos();
    ImVec2 tabs_pos;
    if (actorx->GetActor()->getUsedActorEntry()->filecachename != "")
    {
        Ogre::TexturePtr preview_tex = Ogre::TextureManager::getSingleton().load(
            actorx->GetActor()->getUsedActorEntry()->filecachename, RGN_CACHE);
        // Scale the image
        ImVec2 MAX_PREVIEW_SIZE(100.f, 100.f);
        ImVec2 size(preview_tex->getWidth(), preview_tex->getHeight());
        size *= MAX_PREVIEW_SIZE.x / size.x; // Fit size along X
        if (size.y > MAX_PREVIEW_SIZE.y) // Reduce size along Y if needed
        {
            size *= MAX_PREVIEW_SIZE.y / size.y;
        }
        // Draw the image
        ImGui::Image(reinterpret_cast<ImTextureID>(preview_tex->getHandle()), size);
        tabs_pos = ImGui::GetCursorPos();
        // Move name to the right
        name_pos.x += size.x + ImGui::GetStyle().ItemSpacing.x;
        ImGui::SetCursorPos(name_pos);
    }
    
    RoR::ImTextWrappedColorMarked(actorx->GetActor()->getTruckName());

    // === DRAW TAB BAR ===
    
    if (actorx->GetActor()->getUsedActorEntry()->filecachename != "")
    {
        ImGui::SetCursorPos(tabs_pos);
    }
    ImGui::BeginTabBar("VehicleInfoTPanelTabs", ImGuiTabBarFlags_None);
    if (ImGui::BeginTabItem(_LC("TPanel", "Basics"), nullptr, tabflags_basics))
    {
        m_current_focus = TPANELFOCUS_BASICS;
        this->DrawVehicleBasicsUI(actorx);
    
        ImGui::EndTabItem();
    }
    if (ImGui::BeginTabItem(_LC("TPanel", "Stats"), nullptr, tabflags_stats))
    {
        m_current_focus = TPANELFOCUS_STATS;
        this->DrawVehicleStatsUI(actorx);

        ImGui::EndTabItem();
    }
    if (ImGui::BeginTabItem(_LC("TPanel", "Commands"), nullptr, tabflags_commands))
    {
        m_current_focus = TPANELFOCUS_COMMANDS;
        this->DrawVehicleCommandsUI(actorx);

        ImGui::EndTabItem();
    }
    if (ImGui::BeginTabItem(_LC("TPanel", "Diag"), nullptr, tabflags_diag))
    {
        m_current_focus = TPANELFOCUS_DIAG;
        this->DrawVehicleDiagUI(actorx);

        ImGui::EndTabItem();
    }

    ImGui::EndTabBar();
    
    ImGui::End();
    ImGui::PopStyleColor(2); // WindowBg, TextDisabled

    this->DrawVehicleCommandHighlights(actorx);
}

void VehicleInfoTPanel::DrawVehicleCommandsUI(RoR::GfxActor* actorx)
{
    // === DRAW DESCRIPTION (if available) ===

    if (!actorx->GetActor()->getDescription().empty())
    {
        ImGui::TextDisabled("%s", _LC("VehicleDescription", "Description text:"));
        for (auto line : actorx->GetActor()->getDescription())
        {
            ImGui::TextWrapped("%s", line.c_str());
        }     
    }

    // === DRAW HELP TEXTURE (if available) ===

    if (actorx->GetHelpTex())
    {
        ImGui::TextDisabled("%s", _LC("VehicleDescription", "Help image:"));
        if (!App::ui_always_show_fullsize->getBool())
        {
            ImGui::SameLine();
            ImGui::SetCursorPosX(this->GetPanelWidth() - (ImGui::CalcTextSize(_LC("VehicleDescription", "Full size")).x + 25.f));
            ImGui::Checkbox(_LC("VehicleDescription", "Full size"), &m_helptext_fullsize);
        }
        else
        {
            m_helptext_fullsize = true;
        }
        if (m_helptext_fullsize)
        {
            m_helptext_fullsize_screenpos = ImGui::GetCursorScreenPos();
            ImGui::Dummy(ImVec2(MIN_PANEL_WIDTH, HELP_TEXTURE_HEIGHT_FULL));
            this->DrawVehicleHelpTextureFullsize(actorx);
        }
        else
        {
            ImTextureID im_tex = reinterpret_cast<ImTextureID>(actorx->GetHelpTex()->getHandle());
            ImGui::Image(im_tex, ImVec2(MIN_PANEL_WIDTH, HELP_TEXTURE_HEIGHT));
        }
    }

    // === DRAW COMMAND KEYS, WITH HIGHLIGHT ===

    m_active_commandkey = COMMANDKEYID_INVALID;
    m_hovered_commandkey = COMMANDKEYID_INVALID;

    if (actorx->GetActor()->ar_unique_commandkey_pairs.size() > 0)
    {
        ImGui::TextDisabled("%s", _LC("VehicleDescription", "Command controls:"));
        ImGui::PushStyleColor(ImGuiCol_Text, m_cmdbeam_highlight_color);
        ImGui::Text("%s", _LC("VehicleDescription", "Hover controls for on-vehicle highlight"));
        ImGui::PopStyleColor(1); // Text

        for (const UniqueCommandKeyPair& qpair: actorx->GetActor()->ar_unique_commandkey_pairs)
        {
            // Calc key-button sizes for right alignment (also to check if they fit on the description line)
            ImVec2 initial_cursor = ImGui::GetCursorScreenPos();
            const RoR::events event1 = (RoR::events)RoR::InputEngine::resolveEventName(fmt::format("COMMANDS_{:02d}", qpair.uckp_key1));
            const RoR::events event2 = (RoR::events)RoR::InputEngine::resolveEventName(fmt::format("COMMANDS_{:02d}", qpair.uckp_key2));
            std::string desc = qpair.uckp_description;
            if (qpair.uckp_description == "")
            {
                desc = _LC("VehicleDescription", "~unlabeled~");
            }
            ImVec2 key1_size = ImCalcEventHighlightedSize(event1);
            ImVec2 key2_size = ImCalcEventHighlightedSize(event2);
            ImVec2 desc_size = ImGui::CalcTextSize(desc.c_str());
            const bool single_line = ImGui::GetWindowContentRegionMax().x > desc_size.x + key1_size.x + key2_size.x;
            static const float BUMP_HEIGHT = 3.f;
            static const float MOUSEHIGHLIGHT_MAGICPADRIGHT = 4.f;

            // The line-highlighting: Done manually because `ImGui::Selectable()` blocks buttons under it (or gets blocked by buttons with the `_AllowItemOverlap` flag).
            ImVec2 highlight_mouse_min = initial_cursor - ImGui::GetStyle().ItemSpacing/2;
            ImVec2 highlight_mouse_max = highlight_mouse_min + ImVec2(ImGui::GetWindowContentRegionWidth()+MOUSEHIGHLIGHT_MAGICPADRIGHT, ImGui::GetTextLineHeightWithSpacing());
            if (!single_line)
            {
                highlight_mouse_max.y += ImGui::GetTextLineHeightWithSpacing() - BUMP_HEIGHT;
            }
            
            if ((ImGui::GetMousePos().x > highlight_mouse_min.x && ImGui::GetMousePos().y > highlight_mouse_min.y) 
                && (ImGui::GetMousePos().x < highlight_mouse_max.x && ImGui::GetMousePos().y < highlight_mouse_max.y))
            {
                // This is only for the command-highlight HUD; key1/key2 both point to the same command beams.
                m_hovered_commandkey = qpair.uckp_key1; 

                // Draw the highlight
                ImVec4 col = m_cmdbeam_highlight_color;
                col.w = 0.55f;
                ImDrawList* draw_list = ImGui::GetWindowDrawList();
                draw_list->AddRectFilled(highlight_mouse_min, highlight_mouse_max, ImColor(col));

                // Description comes first - colored for contrast
                ImGui::TextColored(m_command_hovered_text_color, "%s", desc.c_str());
            }
            else
            {
                // Description comes first
                ImGui::Text("%s", desc.c_str());
            }


            if (single_line)
            {
                ImGui::SameLine(); // They fit!
            }
            else
            {
                ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2(0.f, -BUMP_HEIGHT)); // They must go below, but bump them up a bit
            }

            // Key 1
            bool key1_hovered = false;
            bool key1_active = false;
            ImGui::SetCursorPosX((ImGui::GetWindowContentRegionMax().x) - key1_size.x - key2_size.x - ImGui::GetStyle().ItemSpacing.x);
            ImDrawEventHighlightedButton(event1, &key1_hovered, &key1_active);
            if (key1_active)
            {
                m_active_commandkey = qpair.uckp_key1;
            }
            if (key1_hovered)
            {
                m_hovered_commandkey = qpair.uckp_key1;
            }
            ImGui::SameLine();

            // Key 2
            ImGui::SetCursorPosX((ImGui::GetWindowContentRegionMax().x) - key2_size.x);
            bool key2_hovered = false;
            bool key2_active = false;
            ImDrawEventHighlightedButton(event2, &key2_hovered, &key2_active);
            if (key2_active)
            {
                m_active_commandkey = qpair.uckp_key2;
            }
            if (key2_hovered)
            {
                m_hovered_commandkey = qpair.uckp_key2;
            }
        }
    }
}

void DrawStatsLineColored(const char* name, const std::string& value, ImVec4 value_color)
{
    GUIManager::GuiTheme& theme = App::GetGuiManager()->GetTheme();
    ImGui::TextColored(theme.value_blue_text_color, "%s", name);
    // If the value is too long, move it to next line
    ImVec2 label_size = ImGui::CalcTextSize(name);
    ImVec2 value_size = ImGui::CalcTextSize(value.c_str());
    float cursor_x_desired =  - ImGui::CalcTextSize(value.c_str()).x;
    if (label_size.x + value_size.x + ImGui::GetStyle().ItemSpacing.x < ImGui::GetWindowContentRegionWidth())
    {
        ImGui::SameLine();
    }
    ImGui::SetCursorPosX(ImGui::GetWindowContentRegionMax().x - value_size.x);    
    ImGui::TextColored(value_color, "%s", value.c_str());
}

void DrawStatsLine(const char* name, const std::string& value)
{
    DrawStatsLineColored(name, value, ImGui::GetStyle().Colors[ImGuiCol_Text]);
}

void DrawStatsBullet(const char* name, const std::string& value)
{
    GUIManager::GuiTheme& theme = App::GetGuiManager()->GetTheme();
    ImGui::PushStyleColor(ImGuiCol_Text, theme.value_blue_text_color);
    ImGui::Bullet();
    ImGui::PopStyleColor(); // Text
    ImGui::SameLine();
    DrawStatsLineColored(name, value, ImGui::GetStyle().Colors[ImGuiCol_Text]);
}

void VehicleInfoTPanel::DrawVehicleStatsUI(RoR::GfxActor* actorx)
{
    GUIManager::GuiTheme& theme = App::GetGuiManager()->GetTheme();
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(ImGui::GetStyle().ItemSpacing.x, 0));

    if (m_stat_health < 1.0f)
    {
        const float value = static_cast<float>( Round((1.0f - m_stat_health) * 100.0f, 2) );
        DrawStatsLine(_LC("SimActorStats", "Vehicle health: "), fmt::format("{:.2f}%", value));
    }
    else if (m_stat_health >= 1.0f) //When this condition is true, it means that health is at 0% which means 100% of destruction.
    {
        DrawStatsLine(_LC("SimActorStats", "Vehicle destruction: "), "100%");
    }

    const int num_beams = actorx->GetActor()->ar_num_beams;
    DrawStatsLine(_LC("SimActorStats", "Beam count: "), fmt::format("{}", num_beams));

    const float broken_pct = static_cast<float>( Round((float)m_stat_broken_beams / (float)num_beams, 2) * 100.0f );
    DrawStatsLine(_LC("SimActorStats", "Broken beams count: "), fmt::format("{} ({:.0f}%)", m_stat_broken_beams, broken_pct));

    const float deform_pct = static_cast<float>( Round((float)m_stat_deformed_beams / (float)num_beams * 100.0f) );
    DrawStatsLine(_LC("SimActorStats", "Deformed beams count: "), fmt::format("{} ({:.0f}%)", m_stat_deformed_beams, deform_pct));

    const float avg_deform = static_cast<float>( Round((float)m_stat_avg_deform / (float)num_beams, 4) * 100.0f );
    DrawStatsLine(_LC("SimActorStats", "Average deformation: "), fmt::format("{:.2f}", avg_deform));

    const float avg_stress = 1.f - (float)m_stat_beam_stress / (float)num_beams;
    DrawStatsLine(_LC("SimActorStats", "Average stress: "), fmt::format("{:+08.0f}", avg_stress));

    ImGui::NewLine();

    const int num_nodes = actorx->GetActor()->ar_num_nodes;
    const int num_wheelnodes = actorx->GetActor()->getWheelNodeCount();
    DrawStatsLine(_LC("SimActorStats", "Node count: "), fmt::format("{} (wheels: {})", num_nodes, num_wheelnodes));
    if (App::gfx_speedo_imperial->getBool())
    {
        DrawStatsLine(_LC("SimActorStats", "Total mass: "), fmt::format("{:8.2f} lb", m_stat_mass_Kg * 2.205));
        DrawStatsLine(_LC("SimActorStats", ""), fmt::format("{:8.2f} kg",m_stat_mass_Kg));
        DrawStatsLine(_LC("SimActorStats", ""), fmt::format("{:.2f} tons", m_stat_mass_Kg / 1000.0f));
    }
    else
    {
        DrawStatsLine(_LC("SimActorStats", "Total mass: "), fmt::format("{:8.2f} kg", m_stat_mass_Kg));
        DrawStatsLine(_LC("SimActorStats", ""), fmt::format("{:8.2f} lb", m_stat_mass_Kg * 2.205));
        DrawStatsLine(_LC("SimActorStats", ""), fmt::format("{:.2f} tons", m_stat_mass_Kg / 1000.0f));
    }

    ImGui::NewLine();

    const float n0_velo_len   = actorx->GetSimDataBuffer().simbuf_node0_velo.length();
    if ((actorx->GetSimDataBuffer().simbuf_has_engine) && (actorx->GetSimDataBuffer().simbuf_driveable == TRUCK))
    {
        const double PI = 3.14159265358979323846;

        const float max_rpm       = actorx->GetSimDataBuffer().simbuf_engine_max_rpm;
        const float torque        = actorx->GetSimDataBuffer().simbuf_engine_torque;
        const float turbo_psi     = actorx->GetSimDataBuffer().simbuf_engine_turbo_psi;
        const float cur_rpm       = actorx->GetSimDataBuffer().simbuf_engine_rpm;
        const float wheel_speed   = actorx->GetSimDataBuffer().simbuf_wheel_speed;

        const ImVec4 rpm_color = (cur_rpm > max_rpm) ? theme.value_red_text_color : ImGui::GetStyle().Colors[ImGuiCol_Text];
        DrawStatsLineColored(_LC("SimActorStats", "Engine RPM: "), fmt::format("{:.2f} / {:.2f}", cur_rpm, max_rpm), rpm_color);

        const float inputshaft_rpm = Round(std::max(0.0f, actorx->GetSimDataBuffer().simbuf_inputshaft_rpm));
        DrawStatsLineColored(_LC("SimActorStats", "Input shaft RPM: "), fmt::format("{:.0f}", inputshaft_rpm), rpm_color);

        DrawStatsLine(_LC("SimActorStats", "Current torque: "), fmt::format("{:.0f} Nm", Round(torque)));

        const float currentKw = (((cur_rpm * (torque + ((turbo_psi * 6.8) * torque) / 100) * ( PI / 30)) / 1000));
        if (App::gfx_speedo_imperial->getBool())
        {
            DrawStatsLine(_LC("SimActorStats", "Current power: "), fmt::format("{:.0f}hp ({:.0f}Kw)", Round(currentKw * 1.34102209), Round(currentKw)));
        }
        else
        {
            DrawStatsLine(_LC("SimActorStats", "Current power: "), fmt::format("{:.0f}Kw ({:.0f}hp)", Round(currentKw), Round(currentKw * 1.34102209)));
        }

        DrawStatsLine(_LC("SimActorStats", "Current gear: "), fmt::format("{}", actorx->GetSimDataBuffer().simbuf_gear));

        DrawStatsLine(_LC("SimActorStats", "Drive ratio: "), fmt::format("{:.2f}:1", actorx->GetSimDataBuffer().simbuf_drive_ratio));

        float velocityKPH = wheel_speed * 3.6f;
        float velocityMPH = wheel_speed * 2.23693629f;
        float carSpeedKPH = n0_velo_len * 3.6f;
        float carSpeedMPH = n0_velo_len * 2.23693629f;

        // apply a deadzone ==> no flickering +/-
        if (fabs(wheel_speed) < 1.0f)
        {
            velocityKPH = velocityMPH = 0.0f;
        }
        if (fabs(n0_velo_len) < 1.0f)
        {
            carSpeedKPH = carSpeedMPH = 0.0f;
        }

        if (App::gfx_speedo_imperial->getBool())
        {
            DrawStatsLine(_LC("SimActorStats", "Wheel speed: "), fmt::format("{:.0f} mph ({:.0f} km/h)", Round(velocityMPH), Round(velocityKPH)));

            DrawStatsLine(_LC("SimActorStats", "Vehicle speed: "), fmt::format("{:.0f} mph ({:.0f} km/h)", Round(carSpeedMPH), Round(carSpeedKPH)));
        }
        else
        {
            DrawStatsLine(_LC("SimActorStats", "Wheel speed: "), fmt::format("{:.0f} km/h ({:.0f} mph)", Round(velocityKPH), Round(velocityMPH)));

            DrawStatsLine(_LC("SimActorStats", "Vehicle speed: "), fmt::format("{:.0f} km/h ({:.0f} mph)", Round(carSpeedKPH), Round(carSpeedMPH)));
        }
    }
    else // Aircraft or boat
    {
        float speedKN = n0_velo_len * 1.94384449f;
        if (App::gfx_speedo_imperial->getBool())
        {
            DrawStatsLine(_LC("SimActorStats", "Current speed: "), fmt::format("{:.0f} kn", Round(speedKN)));
            DrawStatsLine(_LC("SimActorStats", ""), fmt::format("{:.0f} mph", Round(speedKN * 1.151)));
            DrawStatsLine(_LC("SimActorStats", ""), fmt::format("{:.0f} km/h", Round(speedKN * 1.852)));
        }
        else
        {
            DrawStatsLine(_LC("SimActorStats", "Current speed: "), fmt::format("{:.0f} kn", Round(speedKN)));
            DrawStatsLine(_LC("SimActorStats", ""), fmt::format("{:.0f} km/h", Round(speedKN * 1.852)));
            DrawStatsLine(_LC("SimActorStats", ""), fmt::format("{:.0f} mph", Round(speedKN * 1.151)));
        }

        ImGui::NewLine();

        if (actorx->GetSimDataBuffer().simbuf_driveable == AIRPLANE)
        {
            const float altitude = actorx->GetSimNodeBuffer()[0].AbsPosition.y / 30.48 * 100;
            DrawStatsLine(_LC("SimActorStats", "Altitude: "), fmt::format("{:.0f} feet ({:.0f} meters)", Round(altitude), Round(altitude * 0.30480)));

            int engine_num = 1; // UI; count from 1
            for (AeroEngineSB& ae: actorx->GetSimDataBuffer().simbuf_aeroengines)
            {
                std::string label = fmt::format("{} #{}:", _LC("SimActorStats", "Engine "), engine_num);
                if (ae.simbuf_ae_type == AeroEngineType::AE_XPROP) // Turboprop/pistonprop
                {
                    DrawStatsLine(label.c_str(), fmt::format("{:.2f} RPM", ae.simbuf_ae_rpm));
                }
                else // Turbojet
                {
                    DrawStatsLine(label.c_str(), fmt::format("{:.2f}", ae.simbuf_ae_rpm));
                }
                ++engine_num;
            }
        }
        else if (actorx->GetSimDataBuffer().simbuf_driveable == BOAT)
        {
            int engine_num = 1; // UI; count from 1
            for (ScrewpropSB& screw: actorx->GetSimDataBuffer().simbuf_screwprops)
            {
                std::string label = fmt::format("{} #{}:", _LC("SimActorStats", "Engine "), engine_num);
                DrawStatsLine(label.c_str(), fmt::format("{:.2f}", screw.simbuf_sp_throttle));
                ++engine_num;
            }
        }
    }

    ImGui::NewLine();

    const float speedKPH = actorx->GetSimDataBuffer().simbuf_top_speed * 3.6f;
    const float speedMPH = actorx->GetSimDataBuffer().simbuf_top_speed * 2.23693629f;
    if (App::gfx_speedo_imperial->getBool())
    {
        DrawStatsLine(_LC("SimActorStats", "Top speed: "), fmt::format("{:.0f} mph ({:.0f} km/h)", Round(speedMPH), Round(speedKPH)));
    }
    else
    {
        DrawStatsLine(_LC("SimActorStats", "Top speed: "), fmt::format("{:.0f} km/h ({:.0f} mph)", Round(speedKPH), Round(speedMPH)));
    }

    ImGui::NewLine();

    DrawStatsLine(_LC("SimActorStats", "G-Forces:"), "");
    DrawStatsBullet("Vertical:", fmt::format("{: 6.2f}g  ({:1.2f}g)", m_stat_gcur_x, m_stat_gmax_x));
    DrawStatsBullet("Sagittal:", fmt::format("{: 6.2f}g  ({:1.2f}g)", m_stat_gcur_y, m_stat_gmax_y));
    DrawStatsBullet("Lateral:", fmt::format("{: 6.2f}g  ({:1.2f}g)", m_stat_gcur_z, m_stat_gmax_z));

    ImGui::PopStyleVar(); // ItemSpacing
}

void VehicleInfoTPanel::DrawVehicleDiagUI(RoR::GfxActor* actorx)
{
    ImGui::TextDisabled("%s", _LC("TopMenubar", "Live diagnostic views:"));
    ImGui::TextDisabled("%s", fmt::format(_LC("TopMenubar", "(Toggle with {})"), App::GetInputEngine()->getEventCommandTrimmed(EV_COMMON_TOGGLE_DEBUG_VIEW)).c_str());
    ImGui::TextDisabled("%s", fmt::format(_LC("TopMenubar", "(Cycle with {})"), App::GetInputEngine()->getEventCommandTrimmed(EV_COMMON_CYCLE_DEBUG_VIEWS)).c_str());

    int debug_view_type = static_cast<int>(actorx->GetDebugView());
    ImGui::RadioButton(_LC("TopMenubar", "Normal view"),     &debug_view_type,  static_cast<int>(DebugViewType::DEBUGVIEW_NONE));
    ImGui::RadioButton(_LC("TopMenubar", "Skeleton view"),   &debug_view_type,  static_cast<int>(DebugViewType::DEBUGVIEW_SKELETON));
    ImGui::RadioButton(_LC("TopMenubar", "Node details"),    &debug_view_type,  static_cast<int>(DebugViewType::DEBUGVIEW_NODES));
    ImGui::RadioButton(_LC("TopMenubar", "Beam details"),    &debug_view_type,  static_cast<int>(DebugViewType::DEBUGVIEW_BEAMS));
    ActorPtr current_actor = actorx->GetActor();
    if (current_actor->ar_num_wheels > 0)
    {
        ImGui::RadioButton(_LC("TopMenubar", "Wheel details"),   &debug_view_type,  static_cast<int>(DebugViewType::DEBUGVIEW_WHEELS));
    }
    if (current_actor->ar_num_shocks > 0)
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
    if (current_actor->ar_num_buoycabs > 0)
    {
        ImGui::RadioButton(_LC("TopMenubar", "Buoyancy details"), &debug_view_type,  static_cast<int>(DebugViewType::DEBUGVIEW_BUOYANCY));
    }

    if ((current_actor != nullptr) && (debug_view_type != static_cast<int>(current_actor->GetGfxActor()->GetDebugView())))
    {
        current_actor->GetGfxActor()->SetDebugView(static_cast<DebugViewType>(debug_view_type));
    }

    if (debug_view_type >= 1 && debug_view_type <= static_cast<int>(DebugViewType::DEBUGVIEW_BEAMS)) 
    {
        ImGui::Separator();
        ImGui::TextDisabled("%s",     _LC("TopMenubar", "Settings:"));
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

void VehicleInfoTPanel::SetVisible(TPanelMode mode, TPanelFocus focus)
{
    m_visibility_mode = mode;
    m_requested_focus = focus; // Cannot be handled here, must be handled in Draw() while window is open.
}

void VehicleInfoTPanel::UpdateStats(float dt, ActorPtr actor)
{
    //taken from TruckHUD.cpp (now removed)
    beam_t* beam = actor->ar_beams;
    float average_deformation = 0.0f;
    float beamstress = 0.0f;
    float mass = actor->getTotalMass();
    int beambroken = 0;
    int beamdeformed = 0;
    Ogre::Vector3 gcur = actor->getGForces();
    Ogre::Vector3 gmax = actor->getMaxGForces();

    for (int i = 0; i < actor->ar_num_beams; i++ , beam++)
    {
        if (beam->bm_broken != 0)
        {
            beambroken++;
        }
        beamstress += std::abs(beam->stress);
        float current_deformation = fabs(beam->L - beam->refL);
        if (fabs(current_deformation) > 0.0001f && beam->bm_type != BEAM_HYDRO)
        {
            beamdeformed++;
        }
        average_deformation += current_deformation;
    }

    m_stat_health = ((float)beambroken / (float)actor->ar_num_beams) * 10.0f + ((float)beamdeformed / (float)actor->ar_num_beams);
    m_stat_broken_beams = beambroken;
    m_stat_deformed_beams = beamdeformed;
    m_stat_beam_stress = beamstress;
    m_stat_mass_Kg = mass;
    m_stat_avg_deform = average_deformation;
    m_stat_gcur_x = gcur.x;
    m_stat_gcur_y = gcur.y;
    m_stat_gcur_z = gcur.z;
    m_stat_gmax_x = gmax.x;
    m_stat_gmax_y = gmax.y;
    m_stat_gmax_z = gmax.z;
}

const ImVec2 BUTTON_SIZE(18, 18);
const ImVec2 BUTTON_OFFSET(0, 3.f);
const float BUTTON_Y_OFFSET = 0.f;
const ImVec2 BUTTONDUMMY_SIZE(18, 1);

void DrawSingleBulletRow(const char* name, RoR::events ev)
{
    ImGui::Dummy(BUTTONDUMMY_SIZE); ImGui::SameLine(); ImGui::Bullet(); ImGui::Text("%s", name);
    ImGui::SameLine();
    const ImVec2 btn_size = ImCalcEventHighlightedSize(ev);
    ImGui::SetCursorPosX(ImGui::GetWindowContentRegionWidth() - btn_size.x);
    ImDrawEventHighlighted(ev);
}

void VehicleInfoTPanel::DrawVehicleBasicsUI(RoR::GfxActor* actorx)
{
    GUIManager::GuiTheme& theme = App::GetGuiManager()->GetTheme();

    if (!m_icons_cached)
    {
        this->CacheIcons();
    }

    ImGui::TextDisabled("Simulation:");
    this->DrawRepairButton(actorx);    
    this->DrawActorPhysicsButton(actorx);

    int num_headlights = actorx->GetActor()->countFlaresByType(FlareType::HEADLIGHT);
    int num_taillights = actorx->GetActor()->countFlaresByType(FlareType::TAIL_LIGHT);
    int num_blinkleft = actorx->GetActor()->countFlaresByType(FlareType::BLINKER_LEFT);
    int num_blinkright = actorx->GetActor()->countFlaresByType(FlareType::BLINKER_RIGHT);
    int num_beacons = actorx->countBeaconProps();
    bool has_horn = actorx->GetActor()->getTruckType() == TRUCK;
    if (num_headlights || num_taillights || num_blinkleft || num_blinkright || num_beacons || has_horn)
    {
        ImGui::TextDisabled("Lights and signals:");
        if (num_headlights || num_taillights)
        {
            this->DrawHeadLightButton(actorx);
        }
        if (num_blinkleft)
        {
            this->DrawLeftBlinkerButton(actorx);
        }
        if (num_blinkright)
        {
            this->DrawRightBlinkerButton(actorx);
        }
        if (num_blinkright || num_blinkleft)
        {
            this->DrawWarnBlinkerButton(actorx);
        }
        if (num_beacons)
        {
            this->DrawBeaconButton(actorx);
        }
        if (has_horn)
        {
            this->DrawHornButton(actorx);
        }
    }

    int total_customlights = 0;
    for (int i = 0; i < MAX_CLIGHTS; i++)
    {
        total_customlights += actorx->GetActor()->countCustomLights(i);
    }
    if (total_customlights > 0)
    {
        this->DrawCustomLightButtons(actorx);
    }

    const bool has_engine = actorx->GetActor()->ar_engine != nullptr;
    const bool engine_running = has_engine && actorx->GetActor()->ar_engine->isRunning();
    const bool has_transfercase = actorx->GetActor()->getTransferCaseMode() != nullptr;
    const bool has_4wd = has_transfercase && actorx->GetActor()->getTransferCaseMode()->tr_ax_2 != -1;
    const bool has_2wd = has_transfercase && actorx->GetActor()->getTransferCaseMode()->tr_2wd;
    const size_t num_tc_gears = (has_transfercase) ? actorx->GetActor()->getTransferCaseMode()->tr_gear_ratios.size() : 0u;
    if (has_engine)
    {
        ImGui::TextDisabled("Engine:");
        this->DrawEngineButton(actorx);
        if (!engine_running)
        {
            DrawSingleBulletRow("Starter", EV_TRUCK_STARTER);
        }
        if (has_transfercase && has_4wd && has_2wd)
        {
            this->DrawTransferCaseModeButton(actorx);
        }        
        if (has_transfercase && num_tc_gears > 1)
        {
            this->DrawTransferCaseGearRatioButton(actorx);
        }

        this->DrawShiftModeButton(actorx);

        switch (actorx->GetActor()->ar_engine->getAutoMode())
        {
        case SimGearboxMode::AUTO:
            DrawSingleBulletRow("Shift Up", EV_TRUCK_AUTOSHIFT_UP);
            DrawSingleBulletRow("Shift Down", EV_TRUCK_AUTOSHIFT_DOWN);
            break;
        case SimGearboxMode::SEMI_AUTO:
            DrawSingleBulletRow("Shift Up", EV_TRUCK_AUTOSHIFT_UP);
            DrawSingleBulletRow("Shift Down", EV_TRUCK_AUTOSHIFT_DOWN);
            DrawSingleBulletRow("Shift Neutral", EV_TRUCK_SHIFT_NEUTRAL);
            break;
        case SimGearboxMode::MANUAL:
            DrawSingleBulletRow("Shift Up", EV_TRUCK_SHIFT_UP);
            DrawSingleBulletRow("Shift Down", EV_TRUCK_SHIFT_DOWN);
            DrawSingleBulletRow("Shift Neutral", EV_TRUCK_SHIFT_NEUTRAL);
            DrawSingleBulletRow("Clutch", EV_TRUCK_MANUAL_CLUTCH);
            break;
        case SimGearboxMode::MANUAL_STICK:
            break;
        case SimGearboxMode::MANUAL_RANGES:
            break;
        }
        
    }
       
    const int num_axlediffs = actorx->GetActor()->getAxleDiffMode();
    const int num_wheeldiffs = actorx->GetActor()->getWheelDiffMode();
    const bool tc_visible = !actorx->GetActor()->tc_nodash;
    const bool alb_visible = !actorx->GetActor()->alb_nodash;
    const bool has_parkingbrake = actorx->GetActor()->getTruckType() != NOT_DRIVEABLE && actorx->GetActor()->getTruckType() != BOAT;
    if (num_axlediffs || num_wheeldiffs || tc_visible || alb_visible || has_parkingbrake || has_engine)
    {
        ImGui::TextDisabled("Traction:");
        if (num_axlediffs)
        {
            this->DrawAxleDiffButton(actorx);
        }
        if (num_wheeldiffs)
        {
            this->DrawWheelDiffButton(actorx);
        }
        if (tc_visible)
        {
            this->DrawTractionControlButton(actorx);
        }
        if (alb_visible)
        {
            this->DrawAntiLockBrakeButton(actorx);
        }
        if (has_parkingbrake)
        {
            this->DrawParkingBrakeButton(actorx);
        }
        if (has_engine)
        {
            this->DrawCruiseControlButton(actorx);
        }
    }
    
    const size_t num_locks = actorx->GetActor()->ar_hooks.size();
    const size_t num_ties = actorx->GetActor()->ar_ties.size();
    if (num_locks || num_ties)
    {
        ImGui::TextDisabled("Connections:");
        if (num_locks)
        {
            this->DrawLockButton(actorx);
        }
        if (num_ties)
        {
            this->DrawSecureButton(actorx);
        }
    }

    const size_t num_cparticles = actorx->getCParticles().size();
    const size_t num_videocams = actorx->getVideoCameras().size();
    ImGui::TextDisabled("View:");
    if (num_cparticles)
    {
        this->DrawParticlesButton(actorx);
    }
    if (num_videocams)
    {
        this->DrawMirrorButton(actorx);
    }
    
    this->DrawCameraButton();
}

void VehicleInfoTPanel::DrawVehicleCommandHighlights(RoR::GfxActor* actorx)
{
    if (m_hovered_commandkey == COMMANDKEYID_INVALID)
    {
        return;
    }

    ImDrawList* draw_list = GetImDummyFullscreenWindow("RoR_VehicleCommandHighlights");
    for (const commandbeam_t& cmdbeam: actorx->GetActor()->ar_command_key[m_hovered_commandkey].beams)
    {
        const beam_t& beam = actorx->GetActor()->ar_beams[cmdbeam.cmb_beam_index];
        ImVec2 p1_pos, p2_pos;
        if (GetScreenPosFromWorldPos(beam.p1->AbsPosition, p1_pos) && GetScreenPosFromWorldPos(beam.p2->AbsPosition, p2_pos))
        {
            draw_list->AddLine(p1_pos, p2_pos, ImColor(m_cmdbeam_highlight_color), m_cmdbeam_highlight_thickness);
        }    
    }
}

void VehicleInfoTPanel::DrawVehicleHelpTextureFullsize(RoR::GfxActor* actorx)
{
    // In order to draw the image on top of the T-panel, the window must be focusable, 
    // so we can't simply use `GetImDummyFullscreenWindow()`
    // ===============================================================================

    int window_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar
        | ImGuiWindowFlags_NoSavedSettings ;
    ImGui::SetNextWindowPos(m_helptext_fullsize_screenpos - ImGui::GetStyle().WindowPadding);
    ImGui::SetNextWindowSize(ImVec2(HELP_TEXTURE_WIDTH, HELP_TEXTURE_HEIGHT_FULL) + ImGui::GetStyle().WindowPadding);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0)); // Fully transparent background!
    ImGui::Begin("T-Panel help tex fullsize", NULL, window_flags);
    ImDrawList* drawlist = ImGui::GetWindowDrawList();
    ImTextureID im_tex = reinterpret_cast<ImTextureID>(actorx->GetHelpTex()->getHandle());
    drawlist->AddImage(im_tex, m_helptext_fullsize_screenpos,
        m_helptext_fullsize_screenpos + ImVec2(HELP_TEXTURE_WIDTH, HELP_TEXTURE_HEIGHT_FULL));
    ImGui::End();
    ImGui::PopStyleColor(1); // WindowBg
}

bool DrawSingleButtonRow(bool active, const Ogre::TexturePtr& icon, const char* name, RoR::events ev, bool* btn_active = nullptr)
{
    if (active)
    {
        ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyle().Colors[ImGuiCol_ButtonActive]);
    }
    else
    {
        ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyle().Colors[ImGuiCol_Button]);
    }

    ImGui::GetWindowDrawList()->AddRectFilled(
        ImGui::GetCursorScreenPos() - BUTTON_OFFSET, ImGui::GetCursorScreenPos() + BUTTON_SIZE, 
        ImColor(ImGui::GetStyle().Colors[ImGuiCol_Button]), ImGui::GetStyle().FrameRounding);
    ImGui::GetWindowDrawList()->AddImage(reinterpret_cast<ImTextureID>(icon->getHandle()),
        ImGui::GetCursorScreenPos() - BUTTON_OFFSET, ImGui::GetCursorScreenPos() + BUTTON_SIZE);
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + BUTTON_SIZE.x + 2*ImGui::GetStyle().ItemSpacing.x);
    ImGui::PopStyleColor();
    
    ImGui::Text("%s", name);
    ImGui::SameLine();
    const ImVec2 btn_size = ImCalcEventHighlightedSize(ev);
    ImGui::SetCursorPosX(ImGui::GetWindowContentRegionMax().x - btn_size.x);
    return ImDrawEventHighlightedButton(ev, nullptr, btn_active);
}

void VehicleInfoTPanel::DrawHeadLightButton(RoR::GfxActor* actorx)
{
    if (DrawSingleButtonRow(actorx->GetActor()->getHeadlightsVisible(), m_headlight_icon, "Head Lights", EV_COMMON_TOGGLE_TRUCK_LOW_BEAMS))
    {
        actorx->GetActor()->toggleHeadlights();
    }
}

void VehicleInfoTPanel::DrawLeftBlinkerButton(RoR::GfxActor* actorx)
{
    if (DrawSingleButtonRow(actorx->GetActor()->getBlinkType() == BLINK_LEFT, m_left_blinker_icon, "Left Blinker", EV_TRUCK_BLINK_LEFT))
    {
        actorx->GetActor()->toggleBlinkType(BLINK_LEFT);
    }
}

void VehicleInfoTPanel::DrawRightBlinkerButton(RoR::GfxActor* actorx)
{
    if (DrawSingleButtonRow(actorx->GetActor()->getBlinkType() == BLINK_RIGHT, m_right_blinker_icon, "Right Blinker", EV_TRUCK_BLINK_RIGHT))
    {
        actorx->GetActor()->toggleBlinkType(BLINK_RIGHT);
    }
}

void VehicleInfoTPanel::DrawWarnBlinkerButton(RoR::GfxActor* actorx)
{
    if (DrawSingleButtonRow(actorx->GetActor()->getBlinkType() == BLINK_WARN, m_warning_light_icon, "Warning Lights", EV_TRUCK_BLINK_WARN))
    {
        actorx->GetActor()->toggleBlinkType(BLINK_WARN);
    }
}

void VehicleInfoTPanel::DrawHornButton(RoR::GfxActor* actorx)
{
    if (actorx->GetActor()->ar_is_police) // Police siren
    {
        if (DrawSingleButtonRow(SOUND_GET_STATE(actorx->GetActor()->ar_instance_id, SS_TRIG_HORN), m_horn_icon, "Horn", EV_TRUCK_HORN))
        {
            SOUND_TOGGLE(actorx->GetActor(), SS_TRIG_HORN);
        }
    }
    else
    {
        // Triggering continuous command every frame is sloppy
        // Set state and read it in GameContext via GetHornButtonState()
        DrawSingleButtonRow(SOUND_GET_STATE(actorx->GetActor()->ar_instance_id, SS_TRIG_HORN), m_horn_icon, "Horn", EV_TRUCK_HORN, &m_horn_btn_active);
    }
}

void VehicleInfoTPanel::DrawMirrorButton(RoR::GfxActor* actorx)
{
    if (DrawSingleButtonRow(actorx->GetVideoCamState() == VideoCamState::VCSTATE_ENABLED_ONLINE, m_mirror_icon, "Mirrors", EV_TRUCK_TOGGLE_VIDEOCAMERA))
    {
        if (actorx->GetVideoCamState() == VideoCamState::VCSTATE_DISABLED)
        {
            actorx->SetVideoCamState(VideoCamState::VCSTATE_ENABLED_ONLINE);
        }
        else
        {
            actorx->SetVideoCamState(VideoCamState::VCSTATE_DISABLED);
        }
    }
}

void VehicleInfoTPanel::DrawRepairButton(RoR::GfxActor* actorx)
{
    if (DrawSingleButtonRow(App::GetInputEngine()->getEventBoolValue(EV_COMMON_REPAIR_TRUCK), m_repair_icon, "Repair", EV_COMMON_REPAIR_TRUCK))
    {
        ActorModifyRequest* rq = new ActorModifyRequest;
        rq->amr_actor = actorx->GetActor()->ar_instance_id;
        rq->amr_type  = ActorModifyRequest::Type::RESET_ON_SPOT;
        App::GetGameContext()->PushMessage(Message(MSG_SIM_MODIFY_ACTOR_REQUESTED, (void*)rq));
    }
}

void VehicleInfoTPanel::DrawParkingBrakeButton(RoR::GfxActor* actorx)
{
    if (DrawSingleButtonRow(actorx->GetActor()->getParkingBrake(), m_parking_brake_icon, "Parking Brake", EV_TRUCK_PARKING_BRAKE))
    {
        actorx->GetActor()->parkingbrakeToggle();
    }
}

void VehicleInfoTPanel::DrawTractionControlButton(RoR::GfxActor* actorx)
{
    if (DrawSingleButtonRow(actorx->GetActor()->tc_mode, m_traction_control_icon, "Traction Control", EV_TRUCK_TRACTION_CONTROL))
    {
        actorx->GetActor()->tractioncontrolToggle();
    }
}

void VehicleInfoTPanel::DrawAntiLockBrakeButton(RoR::GfxActor* actorx)
{
    if (DrawSingleButtonRow(actorx->GetActor()->alb_mode, m_abs_icon, "ABS", EV_TRUCK_ANTILOCK_BRAKE))
    {
        actorx->GetActor()->antilockbrakeToggle();
    }
}

void VehicleInfoTPanel::DrawActorPhysicsButton(RoR::GfxActor* actorx)
{
    if (DrawSingleButtonRow(actorx->GetActor()->ar_physics_paused, m_actor_physics_icon, "Pause Actor Physics", EV_TRUCK_TOGGLE_PHYSICS))
    {
        actorx->GetActor()->ar_physics_paused = !actorx->GetActor()->ar_physics_paused;
    }
}

void VehicleInfoTPanel::DrawAxleDiffButton(RoR::GfxActor* actorx)
{
    if (DrawSingleButtonRow(App::GetInputEngine()->getEventBoolValue(EV_TRUCK_TOGGLE_INTER_AXLE_DIFF), m_a_icon, "Axle Differential", EV_TRUCK_TOGGLE_INTER_AXLE_DIFF))
    {
        actorx->GetActor()->toggleAxleDiffMode();
        actorx->GetActor()->displayAxleDiffMode();
    }
}

void VehicleInfoTPanel::DrawWheelDiffButton(RoR::GfxActor* actorx)
{
    if (DrawSingleButtonRow(App::GetInputEngine()->getEventBoolValue(EV_TRUCK_TOGGLE_INTER_WHEEL_DIFF), m_w_icon, "Wheel Differential", EV_TRUCK_TOGGLE_INTER_WHEEL_DIFF))
    {
        actorx->GetActor()->toggleWheelDiffMode();
        actorx->GetActor()->displayWheelDiffMode();
    }
}

void VehicleInfoTPanel::DrawTransferCaseModeButton(RoR::GfxActor* actorx)
{
    if (DrawSingleButtonRow(App::GetInputEngine()->getEventBoolValue(EV_TRUCK_TOGGLE_TCASE_4WD_MODE), m_m_icon, "Transfer Case 4WD", EV_TRUCK_TOGGLE_TCASE_4WD_MODE))
    {
        actorx->GetActor()->toggleTransferCaseMode();
        actorx->GetActor()->displayTransferCaseMode();
    }
}

void VehicleInfoTPanel::DrawTransferCaseGearRatioButton(RoR::GfxActor* actorx)
{
    if (DrawSingleButtonRow(App::GetInputEngine()->getEventBoolValue(EV_TRUCK_TOGGLE_TCASE_GEAR_RATIO), m_g_icon, "Transfer Case Gear Ratio", EV_TRUCK_TOGGLE_TCASE_GEAR_RATIO))
    {
        actorx->GetActor()->toggleTransferCaseGearRatio();
        actorx->GetActor()->displayTransferCaseMode();
    }
}

void VehicleInfoTPanel::DrawParticlesButton(RoR::GfxActor* actorx)
{
    if (DrawSingleButtonRow(actorx->GetActor()->getCustomParticleMode(), m_particle_icon, "Particles", EV_COMMON_TOGGLE_CUSTOM_PARTICLES))
    {
        actorx->GetActor()->toggleCustomParticles();
    }
}

void VehicleInfoTPanel::DrawBeaconButton(RoR::GfxActor* actorx)
{
    if (DrawSingleButtonRow(actorx->GetActor()->getBeaconMode(), m_beacons_icon, "Beacons", EV_COMMON_TOGGLE_TRUCK_BEACONS))
    {
        actorx->GetActor()->beaconsToggle();
    }
}

void VehicleInfoTPanel::DrawShiftModeButton(RoR::GfxActor* actorx)
{
    if (DrawSingleButtonRow(App::GetInputEngine()->getEventBoolValue(EV_TRUCK_SWITCH_SHIFT_MODES), m_shift_icon, "Shift Mode", EV_TRUCK_SWITCH_SHIFT_MODES))
    {
        actorx->GetActor()->ar_engine->toggleAutoMode();
        // force gui update
        actorx->GetActor()->RequestUpdateHudFeatures();

        // Inform player via chatbox
        const char* msg = nullptr;
        switch (actorx->GetActor()->ar_engine->getAutoMode())
        {
        case SimGearboxMode::AUTO: msg = "Automatic shift";
            break;
        case SimGearboxMode::SEMI_AUTO: msg = "Manual shift - Auto clutch";
            break;
        case SimGearboxMode::MANUAL: msg = "Fully Manual: sequential shift";
            break;
        case SimGearboxMode::MANUAL_STICK: msg = "Fully manual: stick shift";
            break;
        case SimGearboxMode::MANUAL_RANGES: msg = "Fully Manual: stick shift with ranges";
            break;
        }
        App::GetConsole()->putMessage(RoR::Console::CONSOLE_MSGTYPE_INFO, RoR::Console::CONSOLE_SYSTEM_NOTICE, _L(msg), "cog.png");
    }
}

void VehicleInfoTPanel::DrawEngineButton(RoR::GfxActor* actorx)
{
    if (DrawSingleButtonRow(actorx->GetActor()->ar_engine->isRunning(), m_engine_icon, "Ignition", EV_TRUCK_TOGGLE_CONTACT))
    {
        if (actorx->GetActor()->ar_engine && actorx->GetActor()->ar_engine->isRunning())
        {
            actorx->GetActor()->ar_engine->toggleContact();
        }
        else if (actorx->GetActor()->ar_engine)
        {
            actorx->GetActor()->ar_engine->startEngine();
        }
    }
}

void VehicleInfoTPanel::DrawCustomLightButton(RoR::GfxActor* actorx)
{
    int num_custom_flares = 0;

    for (int i = 0; i < MAX_CLIGHTS; i++)
    {
        if (actorx->GetActor()->countCustomLights(i) > 0)
        {
            ImGui::PushID(i);
            num_custom_flares++;

            if (i == 5 || i == 9) // Add new line every 4 buttons
            {
                ImGui::NewLine();
            }

            std::string label = "L" + std::to_string(i + 1);

            if (actorx->GetActor()->getCustomLightVisible(i))
            {
                ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyle().Colors[ImGuiCol_ButtonActive]);
            }
            else
            {
                ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyle().Colors[ImGuiCol_Button]);
            }

            if (ImGui::Button(label.c_str(), ImVec2(32, 0)))
            {
                actorx->GetActor()->setCustomLightVisible(i, !actorx->GetActor()->getCustomLightVisible(i));
            }
            if (ImGui::IsItemHovered())
            {
                ImGui::BeginTooltip();
                ImGui::TextDisabled("%s %d (%s)", "Custom Light", i + 1, App::GetInputEngine()->getEventCommandTrimmed(EV_TRUCK_LIGHTTOGGLE01 + i).c_str());
                ImGui::EndTooltip();
            }
            ImGui::SameLine();

            ImGui::PopStyleColor();
            ImGui::PopID();
        }
    }
    if (num_custom_flares > 0)
    {
        ImGui::NewLine();
    }
}



void VehicleInfoTPanel::DrawCameraButton()
{
    if (DrawSingleButtonRow(false, m_camera_icon, "Switch Camera", EV_CAMERA_CHANGE))
    {
        if (App::GetCameraManager()->EvaluateSwitchBehavior())
        {
            App::GetCameraManager()->switchToNextBehavior();
        }
    }
}

void VehicleInfoTPanel::DrawLockButton(RoR::GfxActor* actorx)
{
    if (DrawSingleButtonRow(actorx->GetActor()->isLocked(), m_lock_icon, "Lock", EV_COMMON_LOCK))
    {
        //actorx->GetActor()->hookToggle(-1, HOOK_TOGGLE, -1);
        ActorLinkingRequest* hook_rq = new ActorLinkingRequest();
        hook_rq->alr_type = ActorLinkingRequestType::HOOK_TOGGLE;
        hook_rq->alr_actor_instance_id = actorx->GetActor()->ar_instance_id;
        App::GetGameContext()->PushMessage(Message(MSG_SIM_ACTOR_LINKING_REQUESTED, hook_rq));

        //actorx->GetActor()->toggleSlideNodeLock();
        ActorLinkingRequest* slidenode_rq = new ActorLinkingRequest();
        slidenode_rq->alr_type = ActorLinkingRequestType::SLIDENODE_TOGGLE;
        slidenode_rq->alr_actor_instance_id = actorx->GetActor()->ar_instance_id;
        App::GetGameContext()->PushMessage(Message(MSG_SIM_ACTOR_LINKING_REQUESTED, slidenode_rq));
    }
}

void VehicleInfoTPanel::DrawSecureButton(RoR::GfxActor* actorx)
{
    if (DrawSingleButtonRow(actorx->GetActor()->isTied(), m_secure_icon, "Secure", EV_COMMON_SECURE_LOAD))
    {
        //actorx->GetActor()->tieToggle(-1, TIE_TOGGLE, -1);
        ActorLinkingRequest* tie_rq = new ActorLinkingRequest();
        tie_rq->alr_type = ActorLinkingRequestType::TIE_TOGGLE;
        tie_rq->alr_actor_instance_id = actorx->GetActor()->ar_instance_id;
        App::GetGameContext()->PushMessage(Message(MSG_SIM_ACTOR_LINKING_REQUESTED, tie_rq));
    }
}

void VehicleInfoTPanel::DrawCruiseControlButton(RoR::GfxActor* actorx)
{
    if (DrawSingleButtonRow(actorx->GetActor()->cc_mode, m_cruise_control_icon, "Cruise Control", EV_TRUCK_CRUISE_CONTROL))
    {
        actorx->GetActor()->cruisecontrolToggle();
    }
}

void VehicleInfoTPanel::DrawCustomLightButtons(RoR::GfxActor* actorx)
{
    // Special element - a hint-only (non-button/highlight) hotkey + 10 inline buttons
    // ===============================================================================

    ImGui::GetWindowDrawList()->AddImage(reinterpret_cast<ImTextureID>(m_beacons_icon->getHandle()),
        ImGui::GetCursorScreenPos() - BUTTON_OFFSET, ImGui::GetCursorScreenPos() + BUTTON_SIZE);
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + BUTTON_SIZE.x + 2 * ImGui::GetStyle().ItemSpacing.x);

    ImGui::Text("Custom lights");
    ImGui::SameLine();
    const char* hotkey = "CTRL+[1...]";
    ImVec2 keysize = ImGui::CalcTextSize(hotkey);
    ImGui::SetCursorPosX((ImGui::GetWindowContentRegionMax().x) - keysize.x - ImGui::GetStyle().ItemSpacing.x);
    ImGui::Text(hotkey);

    const ImVec2 BTN_PADDING = ImVec2(1, 1);
    const ImVec2 btn_size = ImGui::CalcTextSize("10") + (BTN_PADDING * 2);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, BTN_PADDING);
    for (int i = 0; i < MAX_CLIGHTS; i++)
    {
        if (actorx->GetActor()->countCustomLights(i) > 0)
        {
            ImGui::PushID(i);

            std::string label = std::to_string(i + 1);

            if (actorx->GetActor()->getCustomLightVisible(i))
            {
                ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyle().Colors[ImGuiCol_ButtonActive]);
            }
            else
            {
                ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyle().Colors[ImGuiCol_Button]);
            }

            if (ImGui::Button(label.c_str(), btn_size))
            {
                actorx->GetActor()->setCustomLightVisible(i, !actorx->GetActor()->getCustomLightVisible(i));
            }
            if (ImGui::IsItemHovered())
            {
                ImGui::BeginTooltip();
                ImGui::TextDisabled("%s", App::GetInputEngine()->getEventCommandTrimmed(EV_TRUCK_LIGHTTOGGLE01 + i).c_str());
                ImGui::EndTooltip();
            }
            ImGui::PopStyleColor(); // Button
            ImGui::SameLine();
            ImGui::PopID();
        }
    }
    ImGui::PopStyleVar(); // FramePadding

    ImGui::NewLine();
}

void VehicleInfoTPanel::CacheIcons()
{
    // Icons used https://materialdesignicons.com/
    // Licence https://github.com/Templarian/MaterialDesign/blob/master/LICENSE

    m_headlight_icon = FetchIcon("car-light-high.png");
    m_left_blinker_icon = FetchIcon("arrow-left-bold.png");
    m_right_blinker_icon = FetchIcon("arrow-right-bold.png");
    m_warning_light_icon = FetchIcon("hazard-lights.png");
    m_horn_icon = FetchIcon("bugle.png");
    m_mirror_icon = FetchIcon("mirror-rectangle.png");
    m_repair_icon = FetchIcon("car-wrench.png");
    m_parking_brake_icon = FetchIcon("car-brake-alert.png");
    m_traction_control_icon = FetchIcon("car-traction-control.png");
    m_abs_icon = FetchIcon("car-brake-abs.png");
    m_physics_icon = FetchIcon("pause.png");
    m_actor_physics_icon = FetchIcon("pause-circle-outline.png");
    m_a_icon = FetchIcon("alpha-a-circle.png");
    m_w_icon = FetchIcon("alpha-w-circle.png");
    m_m_icon = FetchIcon("alpha-m-circle.png");
    m_g_icon = FetchIcon("alpha-g-circle.png");
    m_particle_icon = FetchIcon("water.png");
    m_shift_icon = FetchIcon("car-shift-pattern.png");
    m_engine_icon = FetchIcon("engine.png");
    m_beacons_icon = FetchIcon("alarm-light-outline.png");
    m_camera_icon = FetchIcon("camera-switch-outline.png");
    m_lock_icon = FetchIcon("alpha-l-circle.png");
    m_secure_icon = FetchIcon("lock-outline.png");
    m_cruise_control_icon = FetchIcon("car-cruise-control.png");

    m_icons_cached = true;
}
