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


#include "GUI_NodeBeamUtils.h"

#include "Application.h"
#include "Actor.h"
#include "GameContext.h"
#include "GUIManager.h"
#include "Language.h"

using namespace RoR;
using namespace GUI;

void NodeBeamUtils::Draw()
{
    ActorPtr actor = App::GetGameContext()->GetPlayerActor();
    if (!actor)
    {
        this->SetVisible(false);
        return;
    }

    const int flags = ImGuiWindowFlags_NoCollapse;
    ImGui::SetNextWindowSize(ImVec2(600.f, 675.f), ImGuiCond_FirstUseEver);
    bool keep_open = true;
    ImGui::Begin(_LC("NodeBeamUtils", "Node/Beam Utils"), &keep_open, flags);

    ImGui::PushItemWidth(500.f); // Width includes [+/-] buttons
    float ref_mass = actor->ar_initial_total_mass;
    float cur_mass = actor->getTotalMass(false);
    if (ImGui::SliderFloat(_LC("NodeBeamUtils", "Total mass"), &cur_mass, ref_mass * 0.5f, ref_mass * 2.0f, "%.2f kg"))
    {
        actor->ar_nb_mass_scale = cur_mass / ref_mass;
        actor->ar_nb_initialized = false;
        actor->applyNodeBeamScales();
    }
    ImGui::Separator();
    ImGui::TextColored(GRAY_HINT_TEXT, _LC("NodeBeamUtils", "Beams:"));
    if (ImGui::SliderFloat("Spring##Beams", &actor->ar_nb_beams_scale.first, 0.1f, 10.0f, "%.5f"))
    {
        actor->applyNodeBeamScales();
    }
    if (ImGui::SliderFloat("Damping##Beams", &actor->ar_nb_beams_scale.second, 0.1f, 10.0f, "%.5f"))
    {
        actor->applyNodeBeamScales();
    }
    ImGui::Separator();
    ImGui::TextColored(GRAY_HINT_TEXT, _LC("NodeBeamUtils", "Shocks:"));
    if (ImGui::SliderFloat("Spring##Shocks", &actor->ar_nb_shocks_scale.first, 0.1f, 10.0f, "%.5f"))
    {
        actor->applyNodeBeamScales();
    }
    if (ImGui::SliderFloat("Damping##Shocks", &actor->ar_nb_shocks_scale.second, 0.1f, 10.0f, "%.5f"))
    {
        actor->applyNodeBeamScales();
    }
    ImGui::Separator();
    ImGui::TextColored(GRAY_HINT_TEXT, _LC("NodeBeamUtils", "Wheels:"));
    if (ImGui::SliderFloat("Spring##Wheels", &actor->ar_nb_wheels_scale.first, 0.1f, 10.0f, "%.5f"))
    {
        actor->applyNodeBeamScales();
    }
    if (ImGui::SliderFloat("Damping##Wheels", &actor->ar_nb_wheels_scale.second, 0.1f, 10.0f, "%.5f"))
    {
        actor->applyNodeBeamScales();
    }
    ImGui::Separator();
    ImGui::Spacing();
    if (ImGui::Button(_LC("NodeBeamUtils", "Reset to default settings"), ImVec2(280.f, 25.f)))
    {
        actor->ar_nb_mass_scale = 1.0f;
        actor->ar_nb_beams_scale = {1.0f, 1.0f};
        actor->ar_nb_shocks_scale = {1.0f, 1.0f};
        actor->ar_nb_wheels_scale = {1.0f, 1.0f};
        actor->SyncReset(true);
    }
    ImGui::SameLine();
    if (ImGui::Button(_LC("NodeBeamUtils", "Update initial node positions"), ImVec2(280.f, 25.f)))
    {
        actor->updateInitPosition();
    }
    ImGui::PopItemWidth();

    ImGui::PushItemWidth(235.f); // Width includes [+/-] buttons
    ImGui::TextColored(GRAY_HINT_TEXT,"%s", _LC("NodeBeamUtils", "Physics steps:"));
    ImGui::SliderInt("Skip##BeamsInt",         &actor->ar_nb_skip_steps, 0, 2000);
    ImGui::SameLine();
    ImGui::SliderInt("Measure##BeamsInt",      &actor->ar_nb_measure_steps, 2, 6000);
    ImGui::PopItemWidth();
    ImGui::PushItemWidth(138.f); // Width includes [+/-] buttons
    ImGui::Separator();
    ImGui::TextColored(GRAY_HINT_TEXT,"%s", _LC("NodeBeamUtils", "Beams (spring & damping search interval):"));
    ImGui::SliderFloat("##BSL", &actor->ar_nb_beams_k_interval.first,   0.1f, actor->ar_nb_beams_k_interval.second);
    ImGui::SameLine();
    ImGui::SliderFloat("##BSU", &actor->ar_nb_beams_k_interval.second,  actor->ar_nb_beams_k_interval.first, 10.0f);
    ImGui::SameLine();
    ImGui::SliderFloat("##BDL", &actor->ar_nb_beams_d_interval.first,   0.1f, actor->ar_nb_beams_d_interval.second);
    ImGui::SameLine();
    ImGui::SliderFloat("##BDU", &actor->ar_nb_beams_d_interval.second,  actor->ar_nb_beams_d_interval.first, 10.0f);
    ImGui::TextColored(GRAY_HINT_TEXT,"%s", _LC("NodeBeamUtils", "Shocks (spring & damping search interval):"));
    ImGui::SliderFloat("##SSL", &actor->ar_nb_shocks_k_interval.first,  0.1f, actor->ar_nb_shocks_k_interval.second);
    ImGui::SameLine();
    ImGui::SliderFloat("##SSU", &actor->ar_nb_shocks_k_interval.second, actor->ar_nb_shocks_k_interval.first, 10.0f);
    ImGui::SameLine();
    ImGui::SliderFloat("##SDL", &actor->ar_nb_shocks_d_interval.first,  0.1f, actor->ar_nb_shocks_d_interval.second);
    ImGui::SameLine();
    ImGui::SliderFloat("##SDU", &actor->ar_nb_shocks_d_interval.second, actor->ar_nb_shocks_d_interval.first, 10.0f);
    ImGui::TextColored(GRAY_HINT_TEXT,"%s", _LC("NodeBeamUtils", "Wheels (spring & damping search interval):"));
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
    if (ImGui::Button(m_is_searching ? _LC("NodeBeamUtils", "Stop searching") : actor->ar_nb_initialized ? _LC("NodeBeamUtils", "Continue searching") : _LC("NodeBeamUtils", "Start searching"),
                      ImVec2(280.f, 25.f)))
    {
        m_is_searching = !m_is_searching;
        if (!m_is_searching)
        {
            actor->SyncReset(true);
        }
    }
    ImGui::SameLine();
    if (ImGui::Button(_LC("NodeBeamUtils", "Reset search"), ImVec2(280.f, 25.f)))
    {
        actor->ar_nb_initialized = false;
        m_is_searching = false;
    }
    ImGui::Separator();
    ImGui::Spacing();
    if (actor->ar_nb_initialized)
    {
        ImGui::Columns(2, _LC("NodeBeamUtils", "Search results"));
        ImGui::SetColumnOffset(1, 290.f);
        ImGui::Text("%s", _LC("NodeBeamUtils", "Reference"));
        ImGui::NextColumn();
        ImGui::Text("%s", _LC("NodeBeamUtils", "Optimum"));
        ImGui::NextColumn();
        ImGui::Separator();
        ImGui::Text("%s: %f (%f)", _LC("NodeBeamUtils", "Movement"),   actor->ar_nb_reference[5] / static_cast<int>(actor->ar_nodes.size()), actor->ar_nb_reference[4]);
        ImGui::Text("%s: %.2f (%.2f)", _LC("NodeBeamUtils", "Stress"), actor->ar_nb_reference[1] / actor->ar_num_beams, actor->ar_nb_reference[0]);
        ImGui::Text("%s:   %f (%f)", _LC("NodeBeamUtils", "Yitter"),   actor->ar_nb_reference[3] / actor->ar_num_beams, actor->ar_nb_reference[2]);
        ImGui::NextColumn();
        ImGui::Text("%s: %f (%f)", _LC("NodeBeamUtils", "Movement"),   actor->ar_nb_optimum[5] / static_cast<int>(actor->ar_nodes.size()), actor->ar_nb_optimum[4]);
        ImGui::Text("%s: %.2f (%.2f)", _LC("NodeBeamUtils", "Stress"), actor->ar_nb_optimum[1] / actor->ar_num_beams, actor->ar_nb_optimum[0]);
        ImGui::Text("%s:   %f (%f)", _LC("NodeBeamUtils", "Yitter"),   actor->ar_nb_optimum[3] / actor->ar_num_beams, actor->ar_nb_optimum[2]);
        ImGui::Columns(1);
    }

    if (m_is_searching)
    {
        actor->searchBeamDefaults();
    }

    m_is_hovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows);
    App::GetGuiManager()->RequestGuiCaptureKeyboard(m_is_hovered);

    ImGui::End();
    if (!keep_open)
    {
        this->SetVisible(false);
    }
}

void NodeBeamUtils::SetVisible(bool v)
{
    m_is_visible = v;
    m_is_hovered = false;
    if (!v)
    {
        m_is_searching = false;
    }
}
