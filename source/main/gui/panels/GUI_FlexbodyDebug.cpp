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
        show_locator.resize(0);
        show_locator.resize(actor->GetGfxActor()->GetFlexbodies()[m_combo_selection]->getVertexCount(), false);
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
    ImGui::Checkbox("Show all##forset", &this->show_forset_nodes);

    ImGui::Text("Vertices: (total %d)", (int)flexbody->getVertexCount());
    ImGui::SameLine();
    ImGui::Checkbox("Show all (pick with mouse)##verts", &this->show_vertices);

    const float content_height = 
        (2.f * ImGui::GetStyle().WindowPadding.y) 
        + (5.f * ImGui::GetItemsLineHeightWithSpacing())
        + ImGui::GetStyle().ItemSpacing.y * 3;
    const float child_height = ImGui::GetWindowHeight() - (content_height + 50);


    ImGui::BeginChild("FlexbodyDebug-scroll", ImVec2(0.f, child_height), false);

    // Begin table
    ImGui::Columns(5);
    ImGui::TextDisabled("Vert#");
    ImGui::NextColumn();
    ImGui::TextDisabled("REF node");
    ImGui::NextColumn();
    ImGui::TextDisabled("VX node");
    ImGui::NextColumn();
    ImGui::TextDisabled("VY node");
    ImGui::NextColumn();
    // show checkbox
    ImGui::NextColumn();
    ImGui::Separator();

    bool locators_visible = false;
    for (int i = 0; i < flexbody->getVertexCount(); i++)
    {
        ImGui::PushID(i);
        Locator_t& loc = flexbody->getVertexLocator(i);
        ImGui::TextDisabled("%d", i);
        ImGui::NextColumn();
        ImGui::Text("%d", (int)loc.ref);
        ImGui::NextColumn();
        ImGui::Text("%d", (int)loc.nx);
        ImGui::NextColumn();
        ImGui::Text("%d", (int)loc.ny);
        ImGui::NextColumn();
        bool show = this->show_locator[i];
        if (ImGui::Checkbox("Show", &show))
        {
            this->show_locator[i] = show;
        }
        ImGui::NextColumn();
        ImGui::PopID();

        locators_visible = locators_visible || this->show_locator[i];
    }

    // End table
    ImGui::Columns(1);
    ImGui::EndChild();

    m_is_hovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows);
    App::GetGuiManager()->RequestGuiCaptureKeyboard(m_is_hovered);
    ImGui::End();

    if (!keep_open)
    {
        this->SetVisible(false);
    }

    if (this->show_base_nodes || this->show_forset_nodes || this->show_vertices || locators_visible)
    {
        this->DrawDebugView();
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

    show_locator.resize(0);
    show_locator.resize(actor->GetGfxActor()->GetFlexbodies()[m_combo_selection]->getVertexCount(), false);
}

// All colors are in ABGR format (alpha, blue, green, red)
const ImU32 FORSETNODE_COLOR(0xff44ddff);
const float FORSETNODE_RADIUS(2.f);
const ImU32 BASENODE_COLOR(0xff44a5ff);
const float BASENODE_RADIUS(3.f);
const float BEAM_THICKNESS(1.2f);
const float BLUE_BEAM_THICKNESS = BEAM_THICKNESS + 0.8f; // Blue beam looks a lot thinner for some reason
const ImU32 NODE_TEXT_COLOR(0xffcccccf); // node ID text color
const ImU32 VERTEX_COLOR = ImColor(0.1f, 1.f, 1.f);
const ImU32 VERTEX_TEXT_COLOR = ImColor(171, 194, 186);
const float VERTEX_RADIUS(1.f);
const float LOCATOR_BEAM_THICKNESS(1.0f);
const ImU32 LOCATOR_BEAM_COLOR = ImColor(0.05f, 1.f, 0.65f);
const ImU32 AXIS_X_BEAM_COLOR = ImColor(1.f, 0.f, 0.f);
const ImU32 AXIS_Y_BEAM_COLOR = ImColor(0.1f, 0.1f, 1.f);
const float VERT_HOVER_MAX_DISTANCE = 25.f;


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
            if (xnode_pos.z < 0) { drawlist->AddLine(ImVec2(refnode_pos.x, refnode_pos.y), ImVec2(xnode_pos.x, xnode_pos.y), AXIS_X_BEAM_COLOR, BEAM_THICKNESS); }
            if (ynode_pos.z < 0) { drawlist->AddLine(ImVec2(refnode_pos.x, refnode_pos.y), ImVec2(ynode_pos.x, ynode_pos.y), AXIS_Y_BEAM_COLOR, BLUE_BEAM_THICKNESS); }
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

    int hovered_vert = -1;
    float hovered_vert_dist_squared = FLT_MAX;
    ImVec2 mouse_pos = ImGui::GetMousePos();
    ImVec2 dbg_cursor_dist(0, 0);
    if (this->show_vertices)
    {
        for (int i = 0; i < flexbody->getVertexCount(); i++)
        {
            Ogre::Vector3 vert_pos = world2screen.Convert(flexbody->getVertexPos(i));
            if (vert_pos.z < 0.f)
            {
                // Draw the visual dot
                drawlist->ChannelsSetCurrent(LAYER_NODES);
                drawlist->AddCircleFilled(ImVec2(vert_pos.x, vert_pos.y), VERTEX_RADIUS, VERTEX_COLOR);

                // Check mouse hover
                ImVec2 cursor_dist((vert_pos.x - mouse_pos.x), (vert_pos.y - mouse_pos.y));
                float dist_squared = (cursor_dist.x * cursor_dist.x) + (cursor_dist.y * cursor_dist.y);
                if (dist_squared < hovered_vert_dist_squared)
                {
                    hovered_vert = i;
                    hovered_vert_dist_squared = dist_squared;
                    dbg_cursor_dist = cursor_dist;
                }
            }
        }
    }

    // Validate mouse hover
    if (hovered_vert != -1
        && hovered_vert_dist_squared > VERT_HOVER_MAX_DISTANCE * VERT_HOVER_MAX_DISTANCE)
    {
        hovered_vert = -1;
    }

    for (int i = 0; i < flexbody->getVertexCount(); i++)
    {
        if (this->show_locator[i] || i == hovered_vert)
        {
            // The vertex
            Ogre::Vector3 vert_pos = world2screen.Convert(flexbody->getVertexPos(i));

            if (vert_pos.z < 0.f)
            {
                if (!this->show_vertices) // don't draw twice
                {
                    drawlist->ChannelsSetCurrent(LAYER_NODES);
                    drawlist->AddCircleFilled(ImVec2(vert_pos.x, vert_pos.y), VERTEX_RADIUS, VERTEX_COLOR);
                }

                drawlist->ChannelsSetCurrent(LAYER_TEXT);
                drawlist->AddText(ImVec2(vert_pos.x, vert_pos.y), VERTEX_TEXT_COLOR, fmt::format("v{}", i).c_str());
            }

            // The locator nodes
            Locator_t& loc = flexbody->getVertexLocator(i);
            Ogre::Vector3 refnode_pos = world2screen.Convert(nodes[loc.ref].AbsPosition);
            Ogre::Vector3 xnode_pos = world2screen.Convert(nodes[loc.nx].AbsPosition);
            Ogre::Vector3 ynode_pos = world2screen.Convert(nodes[loc.ny].AbsPosition);
            if (!this->show_forset_nodes) // don't draw twice
            {
                // (z < 0) means "in front of the camera"
                if (refnode_pos.z < 0.f) { drawlist->AddCircleFilled(ImVec2(refnode_pos.x, refnode_pos.y), FORSETNODE_RADIUS, FORSETNODE_COLOR); }
                if (xnode_pos.z < 0.f) { drawlist->AddCircleFilled(ImVec2(xnode_pos.x, xnode_pos.y), FORSETNODE_RADIUS, FORSETNODE_COLOR); }
                if (ynode_pos.z < 0.f) { drawlist->AddCircleFilled(ImVec2(ynode_pos.x, ynode_pos.y), FORSETNODE_RADIUS, FORSETNODE_COLOR); }
            }

            drawlist->ChannelsSetCurrent(LAYER_BEAMS);
            if (refnode_pos.z < 0)
            {
                if (xnode_pos.z < 0) { drawlist->AddLine(ImVec2(refnode_pos.x, refnode_pos.y), ImVec2(xnode_pos.x, xnode_pos.y), AXIS_X_BEAM_COLOR, BEAM_THICKNESS); }
                if (ynode_pos.z < 0) { drawlist->AddLine(ImVec2(refnode_pos.x, refnode_pos.y), ImVec2(ynode_pos.x, ynode_pos.y), AXIS_Y_BEAM_COLOR, BLUE_BEAM_THICKNESS); }
                if (vert_pos.z < 0) { drawlist->AddLine(ImVec2(refnode_pos.x, refnode_pos.y), ImVec2(vert_pos.x, vert_pos.y), LOCATOR_BEAM_COLOR, LOCATOR_BEAM_THICKNESS); }
            }

            if (!this->show_forset_nodes) // don't draw twice
            {
                drawlist->AddText(ImVec2(refnode_pos.x, refnode_pos.y), NODE_TEXT_COLOR, fmt::format("{}", loc.ref).c_str());
                drawlist->AddText(ImVec2(xnode_pos.x, xnode_pos.y), NODE_TEXT_COLOR, fmt::format("{}", loc.nx).c_str());
                drawlist->AddText(ImVec2(ynode_pos.x, ynode_pos.y), NODE_TEXT_COLOR, fmt::format("{}", loc.ny).c_str());
            }

            if (i == hovered_vert && ImGui::IsMouseClicked(0))
            {
                this->show_locator[i] = !this->show_locator[i];
            }
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
