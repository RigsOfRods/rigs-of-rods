/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2013-2019 Petr Ohlidal

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

#include "GUI_SimActorStats.h"

#include "Application.h"
#include "Actor.h"
#include "SimData.h"
#include "Language.h"
#include "GfxActor.h"
#include "GUIManager.h"
#include "Utils.h"
#include "GUIUtils.h"

using namespace RoR;
using namespace GUI;

void SimActorStats::Draw(RoR::GfxActor* actorx)
{
    GUIManager::GuiTheme& theme = App::GetGuiManager()->GetTheme();
    
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar;
    ImGui::SetNextWindowPos(ImVec2(theme.screen_edge_padding.x, (theme.screen_edge_padding.y + 150)));
    ImGui::Begin("SimActorStats", nullptr, flags);

    RoR::ImTextWrappedColorMarked(actorx->FetchActorDesignName());
    ImGui::Dummy(ImGui::GetStyle().FramePadding);

    ImGui::Separator();
    if (m_stat_health < 1.0f)
    {
        const float value = static_cast<float>( Round((1.0f - m_stat_health) * 100.0f, 2) );
        ImGui::TextColored(theme.value_blue_text_color,"%s", _LC("SimActorStats", "Vehicle health: "));
        ImGui::SameLine();
        ImGui::Text("%.2f%%", value);
    }
    else if (m_stat_health >= 1.0f) //When this condition is true, it means that health is at 0% which means 100% of destruction.
    {
        ImGui::TextColored(theme.value_blue_text_color,"%s", _LC("SimActorStats", "Vehicle destruction: "));
        ImGui::SameLine();
        ImGui::Text("100%%");
    }

    const int num_beams_i = actorx->FetchNumBeams();
    const float num_beams_f = static_cast<float>(num_beams_i);
    ImGui::TextColored(theme.value_blue_text_color,"%s", _LC("SimActorStats", "Beam count: "));
    ImGui::SameLine();
    ImGui::Text("%d", num_beams_i);

    const float broken_pct = static_cast<float>( Round((float)m_stat_broken_beams / num_beams_f, 2) * 100.0f );
    ImGui::TextColored(theme.value_blue_text_color,"%s", _LC("SimActorStats", "Broken beams count: "));
    ImGui::SameLine();
    ImGui::Text("%d (%.0f%%)", m_stat_broken_beams, broken_pct);

    const float deform_pct = static_cast<float>( Round((float)m_stat_deformed_beams / num_beams_f * 100.0f) );
    ImGui::TextColored(theme.value_blue_text_color,"%s", _LC("SimActorStats", "Deformed beams count: "));
    ImGui::SameLine();
    ImGui::Text("%d (%.0f%%)", m_stat_deformed_beams, deform_pct);

    const float avg_deform = static_cast<float>( Round((float)m_stat_avg_deform / num_beams_f, 4) * 100.0f );
    ImGui::TextColored(theme.value_blue_text_color,"%s", _LC("SimActorStats", "Average deformation: "));
    ImGui::SameLine();
    ImGui::Text("%.2f", avg_deform);

    const float avg_stress = 1.f - (float)m_stat_beam_stress / num_beams_f;
    ImGui::TextColored(theme.value_blue_text_color,"%s", _LC("SimActorStats", "Average stress: "));
    ImGui::SameLine();
    ImGui::Text("%+08.0f", avg_stress);

    ImGui::NewLine();

    const int num_nodes = actorx->FetchNumNodes();
    const int num_wheelnodes = actorx->FetchNumWheelNodes();
    ImGui::TextColored(theme.value_blue_text_color,"%s", _LC("SimActorStats", "Node count: "));
    ImGui::SameLine();
    ImGui::Text("%d (%s%d)", num_nodes, "wheels: ", num_wheelnodes);

    ImGui::TextColored(theme.value_blue_text_color,"%s", _LC("SimActorStats", "Total mass: "));
    ImGui::SameLine();
    ImGui::Text("%8.2f Kg (%.2f tons)", m_stat_mass_Kg, m_stat_mass_Kg / 1000.0f);

    ImGui::NewLine();

    const float n0_velo_len   = actorx->GetSimDataBuffer().simbuf_node0_velo.length();
    if ((actorx->GetAttributes().xa_has_engine) && (actorx->GetAttributes().xa_driveable == TRUCK))
    {
        const double PI = 3.14159265358979323846;

        const float max_rpm       = actorx->GetAttributes().xa_engine_max_rpm;
        const float torque        = actorx->GetSimDataBuffer().simbuf_engine_torque;
        const float turbo_psi     = actorx->GetSimDataBuffer().simbuf_engine_turbo_psi;
        const float cur_rpm       = actorx->GetSimDataBuffer().simbuf_engine_rpm;
        const float wheel_speed   = actorx->GetSimDataBuffer().simbuf_wheel_speed;

        ImGui::TextColored(theme.value_blue_text_color,"%s", _LC("SimActorStats", "Engine RPM: "));
        ImGui::SameLine();
        ImVec4 rpm_color = (cur_rpm > max_rpm) ? theme.value_red_text_color : ImGui::GetStyle().Colors[ImGuiCol_Text];
        ImGui::TextColored(rpm_color, "%.2f / %.2f", cur_rpm, max_rpm);

        ImGui::TextColored(theme.value_blue_text_color,"%s", _LC("SimActorStats", "Input shaft RPM: "));
        ImGui::SameLine();
        const float inputshaft_rpm = Round(std::max(0.0f, actorx->GetSimDataBuffer().simbuf_inputshaft_rpm));
        ImGui::TextColored(rpm_color, "%.0f", inputshaft_rpm);

        ImGui::TextColored(theme.value_blue_text_color,"%s", _LC("SimActorStats", "Current torque: "));
        ImGui::SameLine();
        ImGui::Text("%.0f Nm", Round(torque));

        const float currentKw = (((cur_rpm * (torque + ((turbo_psi * 6.8) * torque) / 100) * ( PI / 30)) / 1000));
        ImGui::TextColored(theme.value_blue_text_color,"%s", _LC("SimActorStats", "Current power: "));
        ImGui::SameLine();
        ImGui::Text("%.0fhp (%.0fKw)", static_cast<float>(Round(currentKw *1.34102209)), static_cast<float>(Round(currentKw)));

        ImGui::TextColored(theme.value_blue_text_color,"%s", _LC("SimActorStats", "Current gear: "));
        ImGui::SameLine();
        ImGui::Text("%d", actorx->GetSimDataBuffer().simbuf_gear);

        ImGui::TextColored(theme.value_blue_text_color,"%s", _LC("SimActorStats", "Drive ratio: "));
        ImGui::SameLine();
        ImGui::Text("%.2f:1", actorx->GetSimDataBuffer().simbuf_drive_ratio);

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

        ImGui::TextColored(theme.value_blue_text_color,"%s", _LC("SimActorStats", "Wheel speed: "));
        ImGui::SameLine();
        ImGui::Text("%.0fKm/h (%.0f mph)", Round(velocityKPH), Round(velocityMPH));

        ImGui::TextColored(theme.value_blue_text_color,"%s", _LC("SimActorStats", "Vehicle speed: "));
        ImGui::SameLine();
        ImGui::Text("%.0fKm/h (%.0f mph)", Round(carSpeedKPH), Round(carSpeedMPH));
    }
    else // Aircraft or boat
    {
        float speedKN = n0_velo_len * 1.94384449f;
        ImGui::TextColored(theme.value_blue_text_color,"%s", _LC("SimActorStats", "Current speed: "));
        ImGui::SameLine();
        ImGui::Text("%.0f kn (%.0f Km/h; %.0f mph)", Round(speedKN), Round(speedKN * 1.852), Round(speedKN * 1.151));

        if (actorx->GetAttributes().xa_driveable == AIRPLANE)
        {
            const float altitude = actorx->GetSimNodeBuffer()[0].AbsPosition.y / 30.48 * 100;
            ImGui::TextColored(theme.value_blue_text_color,"%s", _LC("SimActorStats", "Altitude: "));
            ImGui::SameLine();
            ImGui::Text("%.0f feet (%.0f meters)", Round(altitude), Round(altitude * 0.30480));

            int engine_num = 1; // UI; count from 1
            for (GfxActor::SimBuffer::AeroEngineSB& ae: actorx->GetSimDataBuffer().simbuf_aeroengines)
            {
                ImGui::TextColored(theme.value_blue_text_color, "%s #%d:", _LC("SimActorStats", "Engine "), engine_num);
                ImGui::SameLine();
                if (ae.simbuf_ae_type == AEROENGINE_TURBOPROP_PISTONPROP)
                {
                    ImGui::Text("%.2f RPM", ae.simbuf_ae_rpm);
                }
                else // Turbojet
                {
                    ImGui::Text("%.2f", ae.simbuf_ae_rpm);
                }
                ++engine_num;
            }
        }
        else if (actorx->GetAttributes().xa_driveable == BOAT)
        {
            int engine_num = 1; // UI; count from 1
            for (GfxActor::SimBuffer::ScrewPropSB& screw: actorx->GetSimDataBuffer().simbuf_screwprops)
            {
                ImGui::TextColored(theme.value_blue_text_color, "%s #%d:", _LC("SimActorStats", "Engine "), engine_num);
                ImGui::SameLine();
                ImGui::Text("%f%", screw.simbuf_sp_throttle);
                ++engine_num;
            }
        }
    }

    ImGui::NewLine();

    const float speedKPH = actorx->GetSimDataBuffer().simbuf_top_speed * 3.6f;
    const float speedMPH = actorx->GetSimDataBuffer().simbuf_top_speed * 2.23693629f;
    ImGui::TextColored(theme.value_blue_text_color,"%s", _LC("SimActorStats", "Top speed: "));
    ImGui::SameLine();
    ImGui::Text("%.0f km/h (%.0f mph)", Round(speedKPH), Round(speedMPH));

    ImGui::NewLine();

    ImGui::TextColored(theme.value_blue_text_color,"%s", _LC("SimActorStats", "G-Forces:"));
    ImGui::Text("Vertical: % 6.2fg  (%1.2fg)", m_stat_gcur_x, m_stat_gmax_x);
    ImGui::Text("Sagittal: % 6.2fg  (%1.2fg)", m_stat_gcur_y, m_stat_gmax_y);
    ImGui::Text("Lateral:  % 6.2fg  (%1.2fg)", m_stat_gcur_z, m_stat_gmax_z);

    ImGui::NewLine();
    ImGui::End();
}

void SimActorStats::UpdateStats(float dt, Actor* actor)
{
    //taken from TruckHUD.cpp (now removed)
    beam_t* beam = actor->ar_beams;
    float average_deformation = 0.0f;
    float beamstress = 0.0f;
    float mass = actor->getTotalMass();
    int beambroken = 0;
    int beamdeformed = 0;
    Ogre::Vector3 gcur = actor->GetGForcesCur();
    Ogre::Vector3 gmax = actor->GetGForcesMax();

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
