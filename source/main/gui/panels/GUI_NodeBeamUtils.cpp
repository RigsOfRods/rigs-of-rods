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
    ImGui::SetNextWindowPosCenter(ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(600.f, 675.f), ImGuiCond_FirstUseEver);
    const int flags = ImGuiWindowFlags_NoCollapse;
    bool keep_open = true;
    ImGui::Begin(_LC("NodeBeamUtils", "Node/Beam Utils"), &keep_open, flags);

    if (actor->getUsedActorEntry()->resource_bundle_type == "Zip")
    {
        this->DrawCreateProjectBanner(actor, keep_open);
    }

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
        ImGui::Text("%s: %f (%f)", _LC("NodeBeamUtils", "Movement"),   actor->ar_nb_reference[5] / actor->ar_num_nodes, actor->ar_nb_reference[4]);
        ImGui::Text("%s: %.2f (%.2f)", _LC("NodeBeamUtils", "Stress"), actor->ar_nb_reference[1] / actor->ar_num_beams, actor->ar_nb_reference[0]);
        ImGui::Text("%s:   %f (%f)", _LC("NodeBeamUtils", "Yitter"),   actor->ar_nb_reference[3] / actor->ar_num_beams, actor->ar_nb_reference[2]);
        ImGui::NextColumn();
        ImGui::Text("%s: %f (%f)", _LC("NodeBeamUtils", "Movement"),   actor->ar_nb_optimum[5] / actor->ar_num_nodes, actor->ar_nb_optimum[4]);
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

void NodeBeamUtils::DrawCreateProjectBanner(ActorPtr actor, bool& window_open)
{
    // Show [[ "read only files - create writeable project?" ]] banner.
    // If [yes], unpack the project files, unload current actor and show hint box.
    // ---------------------------------------------------------------------------

    GUIManager::GuiTheme& theme = App::GetGuiManager()->GetTheme();

    // Draw a banner
    const ImVec2 PAD(3, 3);
    ImVec2 cursor = ImGui::GetCursorScreenPos();
    ImVec2 rect_min =  cursor - PAD;
    ImVec2 rect_max = cursor + PAD + ImVec2(ImGui::GetWindowContentRegionMax().x, ImGui::GetTextLineHeightWithSpacing());
    ImGui::GetWindowDrawList()->AddRectFilled(rect_min, rect_max, ImColor(theme.tip_panel_bg_color));
    ImGui::AlignTextToFramePadding();
    ImGui::Text(_LC("NodeBeamUtils", "This mod is read only (ZIP archive)"));
    ImGui::SameLine();
    if (ImGui::Button(_LC("NodeBeamUtils", "Create writable project (if not existing)")))
    {
        // Unzip the mod
        RoR::CreateProjectRequest* req = new RoR::CreateProjectRequest();
        std::string basefname, ext, path;
        Ogre::StringUtil::splitFullFilename(actor->getUsedActorEntry()->fname_without_uid, basefname, ext, path);
        req->cpr_name = basefname + "_nbutil." + actor->getUsedActorEntry()->fext;
        req->cpr_description = "Node/Beam Utils project for " + actor->getUsedActorEntry()->dname;
        req->cpr_source_entry = actor->getUsedActorEntry();
        req->cpr_type = RoR::CreateProjectRequestType::DEFAULT;
        App::GetGameContext()->PushMessage(Message(MSG_EDI_CREATE_PROJECT_REQUESTED, req));

        // Show a message box "please load the project"
        // - it cannot be loaded automatically because it's not in modcache yet so there's no way to locate it.
        RoR::GUI::MessageBoxConfig* box = new RoR::GUI::MessageBoxConfig();
        box->mbc_title = _LC("NodeBeamUtils", "Project created");
        box->mbc_text = fmt::format(_LC("NodeBeamUtils", "Project created successfully as \n\"{}\"\n\nPlease load it and open the N/B utility again"), req->cpr_name);
        box->mbc_allow_close = true;
        App::GetGameContext()->ChainMessage(Message(MSG_GUI_SHOW_MESSAGE_BOX_REQUESTED, box));

        // Unload current actor
        App::GetGameContext()->ChainMessage(Message(MSG_SIM_DELETE_ACTOR_REQUESTED, new ActorPtr(actor)));

        window_open = false;
    }
    ImGui::Dummy(ImVec2(1.f, 6.f));
}
