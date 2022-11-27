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
    ImGui::Begin(_LC("FlexbodyDebug", "Flexbody/Prop debug"), &keep_open, win_flags);

    Actor* actor = App::GetGameContext()->GetPlayerActor();
    if (!actor)
    {
        ImGui::Text("%s", _LC("FlexbodyDebug", "You are on foot."));
        ImGui::End();
        return;
    }

    if (actor->GetGfxActor()->GetFlexbodies().size() == 0
        && actor->GetGfxActor()->getProps().size() == 0)
    {
        ImGui::Text("%s", _LC("FlexbodyDebug", "This vehicle has no flexbodies or props."));
        ImGui::End();
        return;
    }

    if (ImGui::Combo(_LC("FlexbodyDebug", "Select element"), &m_combo_selection, m_combo_items.c_str()))
    {
        this->UpdateVisibility();
        show_locator.resize(0);
        if (m_combo_selection < m_combo_props_start)
        {
            show_locator.resize(actor->GetGfxActor()->GetFlexbodies()[m_combo_selection]->getVertexCount(), false);
        }
    }
    if (ImGui::Checkbox("Hide other (note: pauses reflections)", &this->hide_other_elements))
    {
        this->UpdateVisibility();
    }
    ImGui::Separator();

    // Fetch the element (prop or flexbody)
    FlexBody* flexbody = nullptr;
    Prop* prop = nullptr;
    Ogre::MaterialPtr mat; // Assume one submesh (=> subentity)
    NodeNum_t node_ref = NODENUM_INVALID, node_x = NODENUM_INVALID, node_y = NODENUM_INVALID;
    std::string mesh_name;
    if (m_combo_selection >= m_combo_props_start)
    {
        prop = &actor->GetGfxActor()->getProps()[m_combo_selection - m_combo_props_start];
        mat = prop->pp_mesh_obj->getEntity()->getSubEntity(0)->getMaterial();
        node_ref = prop->pp_node_ref;
        node_x = prop->pp_node_x;
        node_y = prop->pp_node_y;
        mesh_name = prop->pp_mesh_obj->getEntity()->getMesh()->getName();
    }
    else
    {
        flexbody = actor->GetGfxActor()->GetFlexbodies()[m_combo_selection];
        mat = flexbody->getEntity()->getSubEntity(0)->getMaterial();
        node_ref = flexbody->getRefNode();
        node_x = flexbody->getXNode();
        node_y = flexbody->getYNode();
        mesh_name = flexbody->getOrigMeshName();
    }

    ImGui::Text("Mesh: '%s'", mesh_name.c_str());
    ImGui::SameLine();
    if (ImGui::Checkbox("Wireframe (per material)", &this->draw_mesh_wireframe))
    {
        // Assume one technique and one pass
        if (mat && mat->getTechniques().size() > 0 && mat->getTechniques()[0]->getPasses().size() > 0)
        {
            Ogre::PolygonMode mode = (this->draw_mesh_wireframe) ? Ogre::PM_WIREFRAME : Ogre::PM_SOLID;
            mat->getTechniques()[0]->getPasses()[0]->setPolygonMode(mode);
        }
    }

    ImGui::Text("Base nodes: Ref=%d, X=%d, Y=%d", (int)node_ref, (int)node_x, (int)node_y);
    ImGui::SameLine();
    ImGui::Checkbox("Show##base", &this->show_base_nodes);

    bool flexbody_locators_visible = false;
    if (flexbody)
    {
        ImGui::Text("Forset nodes: (total %d)", (int)flexbody->getForsetNodes().size());
        ImGui::SameLine();
        ImGui::Checkbox("Show all##forset", &this->show_forset_nodes);

        ImGui::Text("Vertices: (total %d)", (int)flexbody->getVertexCount());
        ImGui::SameLine();
        ImGui::Checkbox("Show all (pick with mouse)##verts", &this->show_vertices);

        if (ImGui::CollapsingHeader("Vertex locators table"))
        {
            this->DrawLocatorsTable(flexbody, /*out:*/flexbody_locators_visible);
        }

        if (ImGui::CollapsingHeader("Vertex locators memory (experimental!)"))
        {
            this->DrawMemoryOrderGraph(flexbody);
        }
    }

    if (ImGui::CollapsingHeader("Mesh info"))
    {
        if (flexbody)
            this->DrawMeshInfo(flexbody);
        else
            this->DrawMeshInfo(prop);
    }

    m_is_hovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows);
    App::GetGuiManager()->RequestGuiCaptureKeyboard(m_is_hovered);
    ImGui::End();

    if (!keep_open)
    {
        this->SetVisible(false);
    }

    if (this->show_base_nodes || this->show_forset_nodes || this->show_vertices || flexbody_locators_visible)
    {
        this->DrawDebugView(flexbody, prop, node_ref, node_x, node_y);
    }
}

void FlexbodyDebug::AnalyzeFlexbodies()
{
    // Reset
    m_combo_items = "";
    m_combo_props_start = -1;
    m_combo_selection = -1;

    // Var
    int num_combo_items = 0;

    // Analyze flexbodies
    Actor* actor = App::GetGameContext()->GetPlayerActor();
    if (actor && actor->GetGfxActor()->GetFlexbodies().size() > 0)
    {
        for (FlexBody* fb : actor->GetGfxActor()->GetFlexbodies())
        {
            ImAddItemToComboboxString(m_combo_items,
                fmt::format("{} ({} verts -> {} nodes)", fb->getOrigMeshName(), fb->getVertexCount(), fb->getForsetNodes().size()));
            num_combo_items++;
        }

        m_combo_selection = 0;

        show_locator.resize(0);
        show_locator.resize(actor->GetGfxActor()->GetFlexbodies()[m_combo_selection]->getVertexCount(), false);
    }

    // Analyze props as well
    if (actor && actor->GetGfxActor()->getProps().size() > 0)
    {
        m_combo_props_start = num_combo_items;
        for (Prop const& p: actor->GetGfxActor()->getProps())
        {
            ImAddItemToComboboxString(m_combo_items, fmt::format("{} (prop)", p.pp_mesh_obj->getEntity()->getMesh()->getName()));
            num_combo_items++;
        }

        if (m_combo_selection == -1)
            m_combo_selection = 0;
    }

    ImTerminateComboboxString(m_combo_items);
}

const ImVec4 FORSETNODE_COLOR_V4(1.f, 0.87f, 0.3f, 1.f);
const ImU32 FORSETNODE_COLOR = ImColor(FORSETNODE_COLOR_V4);
const float FORSETNODE_RADIUS(2.f);
const ImU32 BASENODE_COLOR(0xff44a5ff); // ABGR format (alpha, blue, green, red)
const float BASENODE_RADIUS(3.f);
const float BEAM_THICKNESS(1.2f);
const float BLUE_BEAM_THICKNESS = BEAM_THICKNESS + 0.8f; // Blue beam looks a lot thinner for some reason
const ImU32 NODE_TEXT_COLOR(0xffcccccf); // node ID text color - ABGR format (alpha, blue, green, red)
const ImVec4 VERTEX_COLOR_V4(0.1f, 1.f, 1.f, 1.f);
const ImU32 VERTEX_COLOR = ImColor(VERTEX_COLOR_V4);
const ImU32 VERTEX_TEXT_COLOR = ImColor(171, 194, 186);
const float VERTEX_RADIUS(1.f);
const float LOCATOR_BEAM_THICKNESS(1.0f);
const ImVec4 LOCATOR_BEAM_COLOR_V4(0.05f, 1.f, 0.65f, 1.f);
const ImVec4 AXIS_X_BEAM_COLOR_V4(1.f, 0.f, 0.f, 1.f);
const ImVec4 AXIS_Y_BEAM_COLOR_V4(0.15f, 0.15f, 1.f, 1.f);
const ImU32 LOCATOR_BEAM_COLOR = ImColor(LOCATOR_BEAM_COLOR_V4);
const ImU32 AXIS_X_BEAM_COLOR = ImColor(AXIS_X_BEAM_COLOR_V4);
const ImU32 AXIS_Y_BEAM_COLOR = ImColor(AXIS_Y_BEAM_COLOR_V4);
const float VERT_HOVER_MAX_DISTANCE = 25.f;
const float MEMGRAPH_NODE_RADIUS(1.f);
const ImVec4 MEMGRAPH_NODEREF_COLOR_V4(1.f, 0.89f, 0.22f, 1.f);
const ImVec4 MEMGRAPH_NODEX_COLOR_V4(1.f, 0.21f, 0.21f, 1.f);
const ImVec4 MEMGRAPH_NODEY_COLOR_V4(0.27f, 0.76f, 1.f, 1.f);

void FlexbodyDebug::DrawDebugView(FlexBody* flexbody, Prop* prop, NodeNum_t node_ref, NodeNum_t node_x, NodeNum_t node_y)
{
    ROR_ASSERT(App::GetGameContext()->GetPlayerActor() != nullptr);
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
        Ogre::Vector3 refnode_pos = world2screen.Convert(nodes[node_ref].AbsPosition);
        Ogre::Vector3 xnode_pos = world2screen.Convert(nodes[node_x].AbsPosition);
        Ogre::Vector3 ynode_pos = world2screen.Convert(nodes[node_y].AbsPosition);
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
        drawlist->AddText(ImVec2(refnode_pos.x, refnode_pos.y), NODE_TEXT_COLOR, fmt::format("{}", node_ref).c_str());
        drawlist->AddText(ImVec2(xnode_pos.x, xnode_pos.y), NODE_TEXT_COLOR, fmt::format("{}", node_x).c_str());
        drawlist->AddText(ImVec2(ynode_pos.x, ynode_pos.y), NODE_TEXT_COLOR, fmt::format("{}", node_y).c_str());
    }

    if (flexbody && this->show_forset_nodes)
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
    if (flexbody && this->show_vertices)
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

    if (flexbody)
    {
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
    }

    drawlist->ChannelsMerge();
}

void FlexbodyDebug::UpdateVisibility()
{
    Actor* actor = App::GetGameContext()->GetPlayerActor();
    if (!actor)
    {
        return;
    }

    if (this->hide_other_elements)
    {
        // Hide everything, even meshes scattered across gameplay components (wheels, wings...).
        // Note: Environment map (dynamic reflections) is halted while the "Hide other" mode is active - see `RoR::GfxEnvmap::UpdateEnvMap()`
        actor->GetGfxActor()->SetAllMeshesVisible(false);

        // Then re-display what we need manually.
        auto& flexbody_vec = actor->GetGfxActor()->GetFlexbodies();
        const int combo_flexbody_selection = m_combo_selection;
        if (combo_flexbody_selection > 0 && combo_flexbody_selection < (int)flexbody_vec.size())
        {
            flexbody_vec[combo_flexbody_selection]->setVisible(true);
        }

        auto& prop_vec = actor->GetGfxActor()->getProps();
        const int combo_prop_selection = m_combo_selection + (int)flexbody_vec.size();
        if (combo_prop_selection > 0 && combo_prop_selection < (int)prop_vec.size())
        {
            prop_vec[combo_prop_selection].setPropMeshesVisible(true);
        }
    }
    else
    {
        // Show everything, `GfxActor::UpdateScene()` will update visibility as needed.
        actor->GetGfxActor()->SetAllMeshesVisible(true);
    }
}

void FlexbodyDebug::DrawLocatorsTable(FlexBody* flexbody, bool& locators_visible)
{
    const float content_height =
        (2.f * ImGui::GetStyle().WindowPadding.y)
        + (5.f * ImGui::GetItemsLineHeightWithSpacing())
        + ImGui::GetStyle().ItemSpacing.y * 5;
    const float child_height = ImGui::GetWindowHeight() - (content_height + 100);


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
}

void FlexbodyDebug::DrawMemoryOrderGraph(FlexBody* flexbody)
{
    // Analysis
    NodeNum_t forset_max = std::numeric_limits<NodeNum_t>::min();
    NodeNum_t forset_min = std::numeric_limits<NodeNum_t>::max();
    for (NodeNum_t n : flexbody->getForsetNodes())
    {
        if (n > forset_max) { forset_max = n; }
        if (n < forset_min) { forset_min = n; }
    }

    // Tools!
    const float SLIDER_WIDTH = 150;
    DrawGCheckbox(App::flexbody_defrag_enabled, "Enable defrag");
    ImGui::SameLine();
    if (ImGui::Button("Reload vehicle"))
    {
        ActorModifyRequest* rq = new ActorModifyRequest;
        rq->amr_type = ActorModifyRequest::Type::RELOAD;
        rq->amr_actor = App::GetGameContext()->GetPlayerActor();
        App::GetGameContext()->PushMessage(Message(MSG_SIM_MODIFY_ACTOR_REQUESTED, (void*)rq));
    }

    if (App::flexbody_defrag_enabled->getBool())
    {
        if (ImGui::CollapsingHeader("Artistic effects (keep all enabled for correct visual)."))
        {
            DrawGCheckbox(App::flexbody_defrag_reorder_texcoords, "Reorder texcoords");
            ImGui::SameLine();
            DrawGCheckbox(App::flexbody_defrag_reorder_indices, "Reorder indices");
            ImGui::SameLine();
            DrawGCheckbox(App::flexbody_defrag_invert_lookup, "Invert index lookup");
        }
    }

    if (App::flexbody_defrag_enabled->getBool())
    {
        ImGui::TextDisabled("Sorting: insert-sort by lowest penalty, start: REF=VX=VY=%d", (int)forset_min);
        ImGui::TextDisabled("Penalty calc: nodes (each x each), smalest nodes, node means");
        ImGui::SetNextItemWidth(SLIDER_WIDTH);
        DrawGIntSlider(App::flexbody_defrag_const_penalty, "Const penalty for inequality", 0, 15);
        ImGui::SetNextItemWidth(SLIDER_WIDTH);
        DrawGIntSlider(App::flexbody_defrag_prog_up_penalty, "Progressive penalty for upward direction", 0, 15);
        ImGui::SetNextItemWidth(SLIDER_WIDTH);
        DrawGIntSlider(App::flexbody_defrag_prog_down_penalty, "Progressive penalty for downward direction", 0, 15);
    }

    // Legend
    ImGui::TextDisabled("For optimal CPU cache usage, all dots should be roughly in ascending order (left->right), gaps are OK");
    ImGui::TextDisabled("X axis (left->right) = verts (total %d)", flexbody->getVertexCount());
    ImGui::TextDisabled("Y axis (bottom->top) = nodes (lowest %d, higest %d) ", (int)forset_min, (int)forset_max);
    ImGui::SameLine();
    ImGui::TextColored(MEMGRAPH_NODEREF_COLOR_V4, "REF");
    ImGui::SameLine();
    ImGui::TextColored(MEMGRAPH_NODEX_COLOR_V4, " VX");
    ImGui::SameLine();
    ImGui::TextColored(MEMGRAPH_NODEY_COLOR_V4, " VY");
    ImGui::Separator();

    // The graph
    ImVec2 size(ImGui::GetWindowWidth() - 2 * ImGui::GetStyle().WindowPadding.x, 200);
    ImVec2 top_left_pos = ImGui::GetCursorScreenPos();
    ImGui::Dummy(size);

    ImDrawList* drawlist = ImGui::GetWindowDrawList();
    int num_verts = flexbody->getVertexCount();
    const float x_step = (size.x / (float)num_verts);
    const float y_step = (size.y / (float)(forset_max - forset_min));
    for (int i = 0; i < num_verts; i++)
    {
        const int NUM_SEGMENTS = 5;
        Locator_t& loc = flexbody->getVertexLocator(i);
        ImVec2 bottom_x_pos = top_left_pos + ImVec2(i * x_step, size.y);

        drawlist->AddCircleFilled(bottom_x_pos - ImVec2(0, (loc.ref - forset_min) * y_step), MEMGRAPH_NODE_RADIUS, ImColor(MEMGRAPH_NODEREF_COLOR_V4), NUM_SEGMENTS);
        drawlist->AddCircleFilled(bottom_x_pos - ImVec2(0, (loc.nx - forset_min) * y_step), MEMGRAPH_NODE_RADIUS, ImColor(MEMGRAPH_NODEX_COLOR_V4), NUM_SEGMENTS);
        drawlist->AddCircleFilled(bottom_x_pos - ImVec2(0, (loc.ny - forset_min) * y_step), MEMGRAPH_NODE_RADIUS, ImColor(MEMGRAPH_NODEY_COLOR_V4), NUM_SEGMENTS);
    }
    ImGui::Separator();
}

void FlexbodyDebug::DrawMeshInfo(FlexBody* flexbody)
{
    ImGui::Text("For developers only; modders cannot affect this.");
    ImGui::Separator();
    ImGui::Text("%s", flexbody->getOrigMeshInfo().c_str());
    ImGui::Separator();
    ImGui::Text("%s", flexbody->getLiveMeshInfo().c_str());
}

void FlexbodyDebug::DrawMeshInfo(Prop* prop)
{
    ImGui::Text("The prop mesh files as provided by modder.");
    if (prop->pp_mesh_obj)
    {
        ImGui::Separator();
        ImGui::Text("%s", RoR::PrintMeshInfo("Prop", prop->pp_mesh_obj->getEntity()->getMesh()).c_str());
    }
    if (prop->pp_wheel_mesh_obj)
    {
        ImGui::Separator();
        ImGui::Text("%s", RoR::PrintMeshInfo("Special: steering wheel", prop->pp_wheel_mesh_obj->getEntity()->getMesh()).c_str());
    }
    for (int i = 0; i < 4; i++)
    {
        if (prop->pp_beacon_scene_node[i])
        {
            ImGui::Separator();
            Ogre::Entity* entity = static_cast<Ogre::Entity*>(prop->pp_beacon_scene_node[i]->getAttachedObject(0));
            ImGui::Text("%s", RoR::PrintMeshInfo(fmt::format("Special: beacon #{}", i), entity->getMesh()).c_str());
        }
    }
}
