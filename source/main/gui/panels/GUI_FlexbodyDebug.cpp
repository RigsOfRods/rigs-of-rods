/*
    This source file is part of Rigs of Rods
    Copyright 2022 Petr Ohlidal

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


#include "GUI_FlexbodyDebug.h"

#include "Application.h"
#include "SimData.h"
#include "Collisions.h"
#include "FlexBody.h"
#include "GameContext.h"
#include "GUIManager.h"
#include "GUIUtils.h"
#include "Language.h"
#include "Terrain.h"
#include "Utils.h"

using namespace RoR;
using namespace GUI;

void FlexbodyDebug::Draw()
{
    ImGuiWindowFlags win_flags = ImGuiWindowFlags_NoCollapse;
    bool keep_open = true;
    ImGui::Begin(_LC("FlexbodyDebug", "Flexbody debug"), &keep_open, win_flags);

    Actor* actor = App::GetGameContext()->GetPlayerActor();
    if (!actor)
    {
        ImGui::Text("%s", _LC("FlexbodyDebug", "You are on foot."));
        ImGui::End();
        return;
    }

    if (actor->GetGfxActor()->GetFlexbodies().size() == 0)
    {
        ImGui::Text("%s", _LC("FlexbodyDebug", "This vehicle has no flexbodies."));
        ImGui::End();
        return;
    }

    if (ImGui::Combo(_LC("FlexbodyDebug", "Select flexbody"), &m_combo_selection, m_combo_items.c_str()))
    {
        this->UpdateVisibility();
    }
    if (ImGui::Checkbox("Hide other", &this->hide_other_flexbodies))
    {
        this->UpdateVisibility();
    }
    ImGui::Separator();
    FlexBody* flexbody = actor->GetGfxActor()->GetFlexbodies()[m_combo_selection];
    
    ImGui::Text("Mesh: '%s'", flexbody->getOrigMeshName().c_str());
    ImGui::SameLine();
    if (ImGui::Checkbox("Wireframe (per material)", &this->draw_mesh_wireframe))
    {
        // Assume one submesh (=> subentity), one technique and one pass
        Ogre::MaterialPtr mat = flexbody->getEntity()->getSubEntity(0)->getMaterial();
        if (mat && mat->getTechniques().size() > 0 && mat->getTechniques()[0]->getPasses().size() > 0)
        {
            Ogre::PolygonMode mode = (this->draw_mesh_wireframe) ? Ogre::PM_WIREFRAME : Ogre::PM_SOLID;
            mat->getTechniques()[0]->getPasses()[0]->setPolygonMode(mode);
        }
    }

    ImGui::Text("Base nodes: Ref=%d, X=%d, Y=%d", (int)flexbody->getRefNode(), (int)flexbody->getXNode(), (int)flexbody->getYNode());
    ImGui::SameLine();
    ImGui::Checkbox("Show##base", &this->show_base_nodes);

    ImGui::Text("Forset nodes: (total %d)", (int)flexbody->getForsetNodes().size());
    ImGui::SameLine();
    ImGui::Checkbox("Show##forset", &this->show_forset_nodes);
    
    if (this->show_base_nodes || this->show_forset_nodes)
    {
        this->DrawDebugView();
    }

    m_is_hovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows);
    App::GetGuiManager()->RequestGuiCaptureKeyboard(m_is_hovered);
    ImGui::End();

    if (!keep_open)
    {
        this->SetVisible(false);
    }
}

void FlexbodyDebug::AnalyzeFlexbodies()
{
    m_combo_items = "";
    Actor* actor = App::GetGameContext()->GetPlayerActor();
    if (actor)
    {
        for (FlexBody* fb : actor->GetGfxActor()->GetFlexbodies())
        {
            ImAddItemToComboboxString(m_combo_items,
                fmt::format("{} ({} verts -> {} nodes)", fb->getOrigMeshName(), fb->getVertexCount(), fb->getForsetNodes().size()));
        }
        ImTerminateComboboxString(m_combo_items);
    }
}

// All colors are in ABGR format (alpha, blue, green, red)
const ImU32 FORSETNODE_COLOR(0xff44ddff);
const float FORSETNODE_RADIUS(2.f);
const ImU32 BASENODE_COLOR(0xff44a5ff);
const float BASENODE_RADIUS(3.f);
const float BEAM_THICKNESS(1.2f);
const float BLUE_BEAM_THICKNESS = BEAM_THICKNESS + 0.8f; // Blue beam looks a lot thinner for some reason
const ImU32 NODE_TEXT_COLOR(0xffcccccf); // node ID text color

void FlexbodyDebug::DrawDebugView()
{
    ROR_ASSERT(App::GetGameContext()->GetPlayerActor() != nullptr);
    FlexBody* flexbody = App::GetGameContext()->GetPlayerActor()->GetGfxActor()->GetFlexbodies()[m_combo_selection];
    NodeSB* nodes = App::GetGameContext()->GetPlayerActor()->GetGfxActor()->GetSimNodeBuffer();

    // Var
    ImVec2 screen_size = ImGui::GetIO().DisplaySize;
    World2ScreenConverter world2screen(
        App::GetCameraManager()->GetCamera()->getViewMatrix(true), App::GetCameraManager()->GetCamera()->getProjectionMatrix(), Ogre::Vector2(screen_size.x, screen_size.y));

    ImDrawList* drawlist = GetImDummyFullscreenWindow();

    const int LAYER_BEAMS = 0;
    const int LAYER_NODES = 1;
    const int LAYER_TEXT = 2;
    drawlist->ChannelsSplit(3);

    if (this->show_base_nodes)
    {
        drawlist->ChannelsSetCurrent(LAYER_NODES);
        Ogre::Vector3 refnode_pos = world2screen.Convert(nodes[flexbody->getRefNode()].AbsPosition);
        Ogre::Vector3 xnode_pos = world2screen.Convert(nodes[flexbody->getXNode()].AbsPosition);
        Ogre::Vector3 ynode_pos = world2screen.Convert(nodes[flexbody->getYNode()].AbsPosition);
        // (z < 0) means "in front of the camera"
        if (refnode_pos.z < 0.f) {drawlist->AddCircleFilled(ImVec2(refnode_pos.x, refnode_pos.y), BASENODE_RADIUS, BASENODE_COLOR); }
        if (xnode_pos.z < 0.f) { drawlist->AddCircleFilled(ImVec2(xnode_pos.x, xnode_pos.y), BASENODE_RADIUS, BASENODE_COLOR); }
        if (ynode_pos.z < 0.f) { drawlist->AddCircleFilled(ImVec2(ynode_pos.x, ynode_pos.y), BASENODE_RADIUS, BASENODE_COLOR); }

        drawlist->ChannelsSetCurrent(LAYER_BEAMS);
        if (refnode_pos.z < 0)
        {
            if (xnode_pos.z < 0) { drawlist->AddLine(ImVec2(refnode_pos.x, refnode_pos.y), ImVec2(xnode_pos.x, xnode_pos.y), ImColor(1.f, 0.f, 0.f), BEAM_THICKNESS); }
            if (ynode_pos.z < 0) { drawlist->AddLine(ImVec2(refnode_pos.x, refnode_pos.y), ImVec2(ynode_pos.x, ynode_pos.y), ImColor(0.1f, 0.1f, 1.f), BLUE_BEAM_THICKNESS); }
        }

        drawlist->ChannelsSetCurrent(LAYER_TEXT);
        drawlist->AddText(ImVec2(refnode_pos.x, refnode_pos.y), NODE_TEXT_COLOR, fmt::format("{}", flexbody->getRefNode()).c_str());
        drawlist->AddText(ImVec2(xnode_pos.x, xnode_pos.y), NODE_TEXT_COLOR, fmt::format("{}", flexbody->getXNode()).c_str());
        drawlist->AddText(ImVec2(ynode_pos.x, ynode_pos.y), NODE_TEXT_COLOR, fmt::format("{}", flexbody->getYNode()).c_str());
    }

    if (this->show_forset_nodes)
    {
        for (NodeNum_t node : flexbody->getForsetNodes())
        {
            drawlist->ChannelsSetCurrent(LAYER_NODES);
            Ogre::Vector3 pos = world2screen.Convert(nodes[node].AbsPosition);
            if (pos.z < 0.f) { drawlist->AddCircleFilled(ImVec2(pos.x, pos.y), FORSETNODE_RADIUS, FORSETNODE_COLOR); }

            drawlist->ChannelsSetCurrent(LAYER_TEXT);
            drawlist->AddText(ImVec2(pos.x, pos.y), NODE_TEXT_COLOR, fmt::format("{}", node).c_str());
        }
    }

    drawlist->ChannelsMerge();
}

void FlexbodyDebug::UpdateVisibility()
{
    if (!App::GetGameContext()->GetPlayerActor())
    {
        return;
    }

    auto& flexbody_vec = App::GetGameContext()->GetPlayerActor()->GetGfxActor()->GetFlexbodies();
    for (int i = 0; i < flexbody_vec.size(); i++)
    {
        flexbody_vec[i]->setVisible(!this->hide_other_flexbodies || i == m_combo_selection);
    }
}
