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

#include "GUI_NodeBeamUtils.h"

#include "Application.h"
#include "Beam.h"
#include "GUIManager.h"
#include "RoRFrameListener.h"

void RoR::GUI::NodeBeamUtils::Draw()
{
    Actor* actor = App::GetSimController()->GetPlayerActor();
    if (!actor)
    {
        this->SetVisible(false);
        return;
    }

    const int flags = ImGuiWindowFlags_NoCollapse;
    ImGui::SetNextWindowSize(ImVec2(600.f, 675.f), ImGuiCond_FirstUseEver);
    bool keep_open = true;
    ImGui::Begin("Node/Beam Utils", &keep_open, flags);

    ImGui::PushItemWidth(500.f); // Width includes [+/-] buttons
    float ref_mass = actor->ar_initial_total_mass;
    float cur_mass = actor->getTotalMass(false);
    if (ImGui::SliderFloat("Total mass", &cur_mass, ref_mass * 0.5f, ref_mass * 2.0f, "%.2f kg"))
    {
        actor->ar_nb_mass_scale = cur_mass / ref_mass;
        actor->ar_nb_initialized = false;
        actor->ApplyNodeBeamScales();
    }
    ImGui::Separator();
    ImGui::TextColored(GRAY_HINT_TEXT, "Beams:");
    if (ImGui::SliderFloat("Spring##Beams", &actor->ar_nb_beams_scale.first, 0.1f, 10.0f, "%.5f"))
    {
        actor->ApplyNodeBeamScales();
    }
    if (ImGui::SliderFloat("Damping##Beams", &actor->ar_nb_beams_scale.second, 0.1f, 10.0f, "%.5f"))
    {
        actor->ApplyNodeBeamScales();
    }
    ImGui::Separator();
    ImGui::TextColored(GRAY_HINT_TEXT, "Shocks:");
    if (ImGui::SliderFloat("Spring##Shocks", &actor->ar_nb_shocks_scale.first, 0.1f, 10.0f, "%.5f"))
    {
        actor->ApplyNodeBeamScales();
    }
    if (ImGui::SliderFloat("Damping##Shocks", &actor->ar_nb_shocks_scale.second, 0.1f, 10.0f, "%.5f"))
    {
        actor->ApplyNodeBeamScales();
    }
    ImGui::Separator();
    ImGui::TextColored(GRAY_HINT_TEXT, "Wheels:");
    if (ImGui::SliderFloat("Spring##Wheels", &actor->ar_nb_wheels_scale.first, 0.1f, 10.0f, "%.5f"))
    {
        actor->ApplyNodeBeamScales();
    }
    if (ImGui::SliderFloat("Damping##Wheels", &actor->ar_nb_wheels_scale.second, 0.1f, 10.0f, "%.5f"))
    {
        actor->ApplyNodeBeamScales();
    }
    ImGui::Separator();
    ImGui::Spacing();
    if (ImGui::Button("Reset to default settings", ImVec2(280.f, 25.f)))
    {
        actor->ar_nb_mass_scale = 1.0f;
        actor->ar_nb_beams_scale = {1.0f, 1.0f};
        actor->ar_nb_shocks_scale = {1.0f, 1.0f};
        actor->ar_nb_wheels_scale = {1.0f, 1.0f};
        actor->SyncReset(true);
    }
    ImGui::SameLine();
    if (ImGui::Button("Update initial node positions", ImVec2(280.f, 25.f)))
    {
        actor->UpdateInitPosition();
    }
    ImGui::PopItemWidth();

    ImGui::PushItemWidth(235.f); // Width includes [+/-] buttons
    ImGui::TextColored(GRAY_HINT_TEXT, "Physics steps:");
    ImGui::SliderInt("Skip##BeamsInt",         &actor->ar_nb_skip_steps, 0, 2000);
    ImGui::SameLine();
    ImGui::SliderInt("Measure##BeamsInt",      &actor->ar_nb_measure_steps, 2, 6000);
    ImGui::PopItemWidth();
    ImGui::PushItemWidth(138.f); // Width includes [+/-] buttons
    ImGui::Separator();
    ImGui::TextColored(GRAY_HINT_TEXT, "Beams (spring & damping search interval):");
    ImGui::SliderFloat("##BSL", &actor->ar_nb_beams_k_interval.first,   0.1f, actor->ar_nb_beams_k_interval.second);
    ImGui::SameLine();
    ImGui::SliderFloat("##BSU", &actor->ar_nb_beams_k_interval.second,  actor->ar_nb_beams_k_interval.first, 10.0f);
    ImGui::SameLine();
    ImGui::SliderFloat("##BDL", &actor->ar_nb_beams_d_interval.first,   0.1f, actor->ar_nb_beams_d_interval.second);
    ImGui::SameLine();
    ImGui::SliderFloat("##BDU", &actor->ar_nb_beams_d_interval.second,  actor->ar_nb_beams_d_interval.first, 10.0f);
    ImGui::TextColored(GRAY_HINT_TEXT, "Shocks (spring & damping search interval):");
    ImGui::SliderFloat("##SSL", &actor->ar_nb_shocks_k_interval.first,  0.1f, actor->ar_nb_shocks_k_interval.second);
    ImGui::SameLine();
    ImGui::SliderFloat("##SSU", &actor->ar_nb_shocks_k_interval.second, actor->ar_nb_shocks_k_interval.first, 10.0f);
    ImGui::SameLine();
    ImGui::SliderFloat("##SDL", &actor->ar_nb_shocks_d_interval.first,  0.1f, actor->ar_nb_shocks_d_interval.second);
    ImGui::SameLine();
    ImGui::SliderFloat("##SDU", &actor->ar_nb_shocks_d_interval.second, actor->ar_nb_shocks_d_interval.first, 10.0f);
    ImGui::TextColored(GRAY_HINT_TEXT, "Wheels (spring & damping search interval):");
    ImGui::SliderFloat("##WSL", &actor->ar_nb_wheels_k_interval.first,  0.1f, actor->ar_nb_wheels_k_interval.second);
    ImGui::SameLine();
    ImGui::SliderFloat("##WSU", &actor->ar_nb_wheels_k_interval.second, actor->ar_nb_wheels_k_interval.first, 10.0f);
    ImGui::SameLine();
    ImGui::SliderFloat("##WDL", &actor->ar_nb_wheels_d_interval.first,  0.1f, actor->ar_nb_wheels_d_interval.second);
    ImGui::SameLine();
    ImGui::SliderFloat("##WDU", &actor->ar_nb_wheels_d_interval.second, actor->ar_nb_wheels_d_interval.first, 10.0f);
    ImGui::PopItemWidth();
    ImGui::Separator();
    ImGui::Spacing();
    auto txt = m_is_searching ? "Stop searching" : actor->ar_nb_initialized ? "Continue searching" : "Start searching";
    if (ImGui::Button(txt, ImVec2(280.f, 25.f)))
    {
        m_is_searching = !m_is_searching;
        if (!m_is_searching)
        {
            actor->SyncReset(true);
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Reset search", ImVec2(280.f, 25.f)))
    {
        actor->ar_nb_initialized = false;
        m_is_searching = false;
    }
    ImGui::Separator();
    ImGui::Spacing();
    if (actor->ar_nb_initialized)
    {
        ImGui::Columns(2, "Search results");
        ImGui::SetColumnOffset(1, 290.f);
        ImGui::Text("Reference");
        ImGui::NextColumn();
        ImGui::Text("Optimum");
        ImGui::NextColumn();
        ImGui::Separator();
        ImGui::Text("Movement: %f (%f)",   actor->ar_nb_reference[5] / actor->ar_num_nodes, actor->ar_nb_reference[4]);
        ImGui::Text("Stress: %.2f (%.2f)", actor->ar_nb_reference[1] / actor->ar_num_beams, actor->ar_nb_reference[0]);
        ImGui::Text("Yitter:   %f (%f)",   actor->ar_nb_reference[3] / actor->ar_num_beams, actor->ar_nb_reference[2]);
        ImGui::NextColumn();
        ImGui::Text("Movement: %f (%f)",   actor->ar_nb_optimum[5] / actor->ar_num_nodes, actor->ar_nb_optimum[4]);
        ImGui::Text("Stress: %.2f (%.2f)", actor->ar_nb_optimum[1] / actor->ar_num_beams, actor->ar_nb_optimum[0]);
        ImGui::Text("Yitter:   %f (%f)",   actor->ar_nb_optimum[3] / actor->ar_num_beams, actor->ar_nb_optimum[2]);
        ImGui::Columns(1);
    }

    if (m_is_searching)
    {
        actor->SearchBeamDefaults();
    }

    App::GetGuiManager()->RequestGuiCaptureKeyboard(ImGui::IsWindowHovered());
    ImGui::End();
    if (!keep_open)
    {
        this->SetVisible(false);
    }
}

void RoR::GUI::NodeBeamUtils::SetVisible(bool v)
{
    m_is_visible = v;
    if (!v)
    {
        m_is_searching = false;
    }
}
