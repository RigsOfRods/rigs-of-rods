/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2013-2022 Petr Ohlidal

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

#include "GUI_CollisionsDebug.h"

#include "Application.h"
#include "Collisions.h"
#include "GameContext.h"
#include "GfxScene.h"
#include "GUIManager.h"
#include "GUIUtils.h"
#include "Language.h"
#include "RoRVersion.h"
#include "Terrain.h"
#include "Utils.h"
#include "RoRnet.h"

#include "imgui_internal.h"

using namespace RoR;
using namespace GUI;
using namespace Ogre;

void CollisionsDebug::Draw()
{
    GUIManager::GuiTheme const& theme = App::GetGuiManager()->GetTheme();

    ImGuiWindowFlags win_flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize;
    bool keep_open = true;
    ImGui::Begin(_LC("About", "Static collision debug"), &keep_open, win_flags);

    ImGui::Text("Terrain name: %s", App::GetGameContext()->GetTerrain()->getTerrainName().c_str());
    ImGui::Text("Terrain size: %.2fx%.2f meters", App::GetGameContext()->GetTerrain()->getMaxTerrainSize().x, App::GetGameContext()->GetTerrain()->getMaxTerrainSize().z);
    ImGui::Checkbox("Draw labels", &m_draw_labels);
    ImGui::Checkbox("Draw label types", &m_labels_draw_types);
    ImGui::Checkbox("Sources on labels", &m_labels_draw_sources);
    ImGui::Separator();

    // EVENTBOX
    ImGui::PushID("EVENTBOX");
    ImGui::TextColored(COLOR_EVENTBOX, "EVENTBOX");
    ImGui::Text("Num event boxes: %d", (int)App::GetGameContext()->GetTerrain()->GetCollisions()->getCollisionBoxes().size());
    if (ImGui::Checkbox("Show event boxes", &m_draw_collision_boxes))
    {
        this->SetDrawEventBoxes(m_draw_collision_boxes);
    }
    ImGui::SetNextItemWidth(WIDTH_DRAWDIST);
    if (ImGui::InputFloat("Draw distance (meters, 0=unlimited)", &m_collision_box_draw_distance))
    {
        for (Ogre::SceneNode* snode : m_collision_boxes)
        {
            snode->getAttachedObject(0)->setRenderingDistance(m_collision_box_draw_distance);
        }
    }
    if (m_collision_boxes.size() > 0)
    {
        if (ImGui::Button("Dump debug meshes (performance)."))
        {
            this->ClearEventBoxVisuals();
        }
    }
    ImGui::Separator();
    ImGui::PopID(); // EVENTBOX

    // COLLMESH
    ImGui::PushID("COLLMESH");
    ImGui::TextColored(COLOR_COLLMESH, "COLLMESH");
    ImGui::Text("Num collision meshes: %d (%d tris)",
        (int)App::GetGameContext()->GetTerrain()->GetCollisions()->getCollisionMeshes().size(),
        (int)App::GetGameContext()->GetTerrain()->GetCollisions()->getCollisionTriangles().size());
    if (ImGui::Checkbox("Show collision meshes", &m_draw_collision_meshes))
    {
        this->SetDrawCollisionMeshes(m_draw_collision_meshes);
    }
    ImGui::SetNextItemWidth(WIDTH_DRAWDIST);
    if (ImGui::InputFloat("Draw distance (meters, 0=unlimited)", &m_collision_mesh_draw_distance))
    {
        for (Ogre::SceneNode* snode : m_collision_meshes)
        {
            snode->getAttachedObject(0)->setRenderingDistance(m_collision_mesh_draw_distance);
        }
    }
    if (m_collision_meshes.size() > 0)
    {
        if (ImGui::Button("Dump debug meshes (performance)."))
        {
            this->ClearCollisionMeshVisuals();
        }
    }
    ImGui::Separator();
    ImGui::PopID(); // COLLMESH

    // CELL
    ImGui::PushID("CELL");
    ImGui::Text("CELL");
    ImGui::Text("Occupancy: ");
    for (int i = 0; i <= 10; i+=1)
    {
        ImGui::SameLine();
        float f = i / 10.f;
        ImVec4 color(f * 2.0, 2.0 * (1.0 - f), 0.2, 0.7);
        int tris = static_cast<int>(f*Collisions::CELL_BLOCKSIZE);
        ImGui::TextColored(color, "%d ", tris);
    }
    ImGui::SetNextItemWidth(WIDTH_DRAWDIST);
    if (ImGui::InputInt("Debug area extent (around character)", &m_cell_generator_distance_limit));
    ImGui::SameLine();
    ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::Text("To save memory and FPS, only cells around the characters will be visualized");
        ImGui::EndTooltip();
    }
    if (ImGui::Checkbox("Show lookup cells (warning: slow!)", &m_draw_collision_cells))
    {
        this->SetDrawCollisionCells(m_draw_collision_cells);
    }
    ImGui::SetNextItemWidth(WIDTH_DRAWDIST);
    if (ImGui::InputFloat("Draw distance (meters, 0=unlimited)", &m_collision_cell_draw_distance))
    {
        for (Ogre::SceneNode* snode : m_collision_cells)
        {
            snode->getAttachedObject(0)->setRenderingDistance(m_collision_cell_draw_distance);
        }
    }
    if (m_collision_grid_root)
    {
        if (ImGui::Button("Dump debug meshes (performance)."))
        {
            this->ClearCollisionCellVisuals();
        }
    }
    ImGui::PopID(); // CELL

    m_is_hovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows);
    App::GetGuiManager()->RequestGuiCaptureKeyboard(m_is_hovered);

    ImGui::End();

    if (!keep_open)
    {
        this->SetVisible(false);
    }

    // only draw reasonably close labels
    const Ogre::Vector3 cam_pos = App::GetCameraManager()->GetCameraNode()->getPosition();

    if (m_draw_labels && m_draw_collision_boxes)
    {
        for (const collision_box_t& cbox : App::GetGameContext()->GetTerrain()->GetCollisions()->getCollisionBoxes())
        {
            if (IsDistanceWithin(cam_pos, this->GetCollBoxWorldPos(cbox), m_collision_box_draw_distance))
            {
                this->DrawCollisionBoxDebugText(cbox);
            }
        }
    }

    if (m_draw_labels && m_draw_collision_meshes)
    {
        for (const collision_mesh_t& cmesh : App::GetGameContext()->GetTerrain()->GetCollisions()->getCollisionMeshes())
        {
            if (IsDistanceWithin(cam_pos, cmesh.position, m_collision_mesh_draw_distance))
            {
                this->DrawCollisionMeshDebugText(cmesh);
            }
        }
    }
}

void CollisionsDebug::AddCollisionMeshDebugMesh(collision_mesh_t const& coll_mesh)
{
    // Gather data
    const CollisionTriVec& ctris = App::GetGameContext()->GetTerrain()->GetCollisions()->getCollisionTriangles();

    // Create mesh
    Ogre::ManualObject* debugmo = App::GetGfxScene()->GetSceneManager()->createManualObject();
    debugmo->begin("tracks/debug/collision/triangle", RenderOperation::OT_TRIANGLE_LIST);
    for (int i = 0; i < coll_mesh.collision_tri_count; i++)
    {
        collision_tri_t const& ctri = ctris[i + coll_mesh.collision_tri_start];
        // The collision triangle vertices are in world coords, we want local coords.
        debugmo->position(ctri.a - coll_mesh.position);
        debugmo->position(ctri.b - coll_mesh.position);
        debugmo->position(ctri.c - coll_mesh.position);
    }
    debugmo->end();
    debugmo->setRenderingDistance(m_collision_mesh_draw_distance);
    debugmo->setBoundingBox(AxisAlignedBox::BOX_INFINITE); // make infinite

    // Display mesh
    SceneNode* debugsn = App::GetGfxScene()->GetSceneManager()->getRootSceneNode()->createChildSceneNode();
    debugsn->setPosition(coll_mesh.position);
    // NOTE: orientation and scale are already "baked" to the collision triangle positions, do not re-apply it here.
    debugsn->attachObject(debugmo);

    // Submit mesh
    m_collision_meshes.push_back(debugsn);
}

void CollisionsDebug::AddCollisionBoxDebugMesh(collision_box_t const& coll_box)
{
    int scripthandler = -1;
    if (coll_box.eventsourcenum != -1)
        scripthandler = App::GetGameContext()->GetTerrain()->GetCollisions()->getEventSource(coll_box.eventsourcenum).es_script_handler;

    SceneNode* debugsn = App::GetGfxScene()->GetSceneManager()->getRootSceneNode()->createChildSceneNode();

    debugsn->setPosition(coll_box.center);
    // box content
    ManualObject* mo = App::GetGfxScene()->GetSceneManager()->createManualObject();
    String matName = "tracks/debug/collision/box";
    if (coll_box.virt && scripthandler == -1)
        matName = "tracks/debug/eventbox/unused";
    else if (coll_box.virt)
        matName = "tracks/debug/eventbox/used";
    AxisAlignedBox aa;
    for (int i = 0; i < 8; i++)
    {
        aa.merge(coll_box.debug_verts[i]);
    }
    mo->begin(matName, Ogre::RenderOperation::OT_TRIANGLE_LIST);
    mo->position(coll_box.debug_verts[0]);
    mo->position(coll_box.debug_verts[1]);
    mo->position(coll_box.debug_verts[2]);
    mo->position(coll_box.debug_verts[3]);
    mo->position(coll_box.debug_verts[4]);
    mo->position(coll_box.debug_verts[5]);
    mo->position(coll_box.debug_verts[6]);
    mo->position(coll_box.debug_verts[7]);

    // front
    mo->triangle(0, 1, 2);
    mo->triangle(1, 3, 2);
    // right side
    mo->triangle(3, 1, 5);
    mo->triangle(5, 7, 3);
    // left side
    mo->triangle(6, 4, 0);
    mo->triangle(0, 2, 6);
    // back side
    mo->triangle(7, 5, 4);
    mo->triangle(4, 6, 7);
    // bottom
    mo->triangle(5, 4, 1);
    mo->triangle(4, 0, 1);
    // top
    mo->triangle(2, 3, 6);
    mo->triangle(3, 7, 6);

    mo->end();
    mo->setBoundingBox(aa);
    mo->setRenderingDistance(200);
    debugsn->attachObject(mo);

    // the border
    mo = App::GetGfxScene()->GetSceneManager()->createManualObject();
    mo->begin(matName, Ogre::RenderOperation::OT_LINE_LIST);
    mo->position(coll_box.debug_verts[0]);
    mo->position(coll_box.debug_verts[1]);
    mo->position(coll_box.debug_verts[2]);
    mo->position(coll_box.debug_verts[3]);
    mo->position(coll_box.debug_verts[4]);
    mo->position(coll_box.debug_verts[5]);
    mo->position(coll_box.debug_verts[6]);
    mo->position(coll_box.debug_verts[7]);
    //front
    mo->index(0); mo->index(1); mo->index(1); mo->index(3); mo->index(3); mo->index(2); mo->index(2); mo->index(0);
    // right side
    mo->index(1); mo->index(5); mo->index(5); mo->index(7); mo->index(7); mo->index(3); mo->index(3); mo->index(1);
    // left side
    mo->index(0); mo->index(2); mo->index(2); mo->index(6); mo->index(6); mo->index(4); mo->index(4); mo->index(0);
    // back side
    mo->index(5); mo->index(4); mo->index(4); mo->index(6); mo->index(6); mo->index(7); mo->index(7); mo->index(5);
    // bottom and top not needed
    mo->end();
    mo->setBoundingBox(aa);
    debugsn->attachObject(mo);
    mo->setRenderingDistance(200);

    m_collision_boxes.push_back(debugsn);
}

Ogre::Vector3 CollisionsDebug::GetCollBoxWorldPos(collision_box_t const& coll_box)
{
    return Vector3(coll_box.lo + (coll_box.hi - coll_box.lo) * 0.5f);
}

void CollisionsDebug::DrawCollisionMeshDebugText(collision_mesh_t const& coll_mesh)
{
    const char* label_type_str = (m_labels_draw_types) ? "COLLMESH\n" : "";
    const char* ground_model_str = (coll_mesh.ground_model) ? coll_mesh.ground_model->name : "(multiple)";

    std::string caption = fmt::format("{}meshname:{}\ngroundmodel:{}",
        label_type_str, coll_mesh.mesh_name, ground_model_str);
    if (m_labels_draw_sources)
    {
        caption += fmt::format("\nsource:{}", coll_mesh.source_name);
    }

    this->DrawLabelAtWorldPos(caption, coll_mesh.position, COLOR_COLLMESH);
}

void CollisionsDebug::DrawCollisionBoxDebugText(collision_box_t const& coll_box)
{
    if (!coll_box.virt || coll_box.eventsourcenum == -1)
        return;

    eventsource_t const& eventsource = App::GetGameContext()->GetTerrain()->GetCollisions()->getEventSource(coll_box.eventsourcenum);
    const char* type_str = (m_labels_draw_types) ? "EVENTBOX\n" : "";

    this->DrawLabelAtWorldPos(
        fmt::format("{}event:{}\ninstance:{}\nhandler:{}", type_str,
            eventsource.es_box_name, eventsource.es_instance_name, eventsource.es_script_handler),
        this->GetCollBoxWorldPos(coll_box), COLOR_EVENTBOX);
}

void CollisionsDebug::DrawLabelAtWorldPos(std::string const& caption, Ogre::Vector3 const& world_pos, ImVec4 const& text_color)
{
    // ----- BEGIN COPYPASTE FROM GfxScene::DrawNetLabel ----

    ImVec2 screen_size = ImGui::GetIO().DisplaySize;
    World2ScreenConverter world2screen(
        App::GetCameraManager()->GetCamera()->getViewMatrix(true), App::GetCameraManager()->GetCamera()->getProjectionMatrix(), Ogre::Vector2(screen_size.x, screen_size.y));

    Ogre::Vector3 pos_xyz = world2screen.Convert(world_pos);

    // only draw when in front of camera
    if (pos_xyz.z < 0.f)
    {
        // Align position to whole pixels, to minimize jitter.
        ImVec2 pos((int)pos_xyz.x + 0.5, (int)pos_xyz.y + 0.5);

        ImVec2 text_size = ImGui::CalcTextSize(caption.c_str());
        GUIManager::GuiTheme const& theme = App::GetGuiManager()->GetTheme();

        ImDrawList* drawlist = GetImDummyFullscreenWindow();
        ImGuiContext* g = ImGui::GetCurrentContext();

        ImVec2 text_pos(pos.x - ((text_size.x / 2)), pos.y - ((text_size.y / 2)));

        // Draw background rectangle
        const float PADDING = 4.f;
        drawlist->AddRectFilled(
            text_pos - ImVec2(PADDING, PADDING),
            text_pos + text_size + ImVec2(PADDING, PADDING),
            ImColor(theme.semitransparent_window_bg),
            ImGui::GetStyle().WindowRounding);

        // draw colored text
        drawlist->AddText(g->Font, g->FontSize, text_pos, ImColor(text_color), caption.c_str());
    }
    // ---- END COPYPASTE ----
}

void CollisionsDebug::CleanUp()
{
    // EVENTBOX
    this->ClearEventBoxVisuals();

    // COLLMESH
    this->ClearCollisionMeshVisuals();

    // CELLS
    this->ClearCollisionCellVisuals();
}

void CollisionsDebug::GenerateCellDebugMaterials()
{
    if (MaterialManager::getSingleton().getByName("mat-coll-dbg-0", ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME))
    {
        return; // already generated before
    }

    // create materials
    int i = 0;
    char bname[256];
    for (i = 0; i <= 100; i++)
    {
        // register a material for skeleton view
        sprintf(bname, "mat-coll-dbg-%d", i);
        MaterialPtr mat = (MaterialPtr)(MaterialManager::getSingleton().create(bname, ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME));
        float f = fabs(((float)i) / 100);
        Pass* p = mat->getTechnique(0)->getPass(0); //
        p->createTextureUnitState()->setColourOperationEx(LBX_MODULATE, LBS_MANUAL, LBS_CURRENT, ColourValue(f * 2.0, 2.0 * (1.0 - f), 0.2, 0.7));
        p->setSceneBlending(Ogre::SBT_TRANSPARENT_ALPHA);
        p->setLightingEnabled(false);
        p->setDepthWriteEnabled(false);
        p->setDepthBias(3, 3);
        p->setCullingMode(Ogre::CULL_NONE);

        Pass* p2 = mat->getTechnique(0)->createPass();
        p2->setSceneBlending(Ogre::SBT_TRANSPARENT_ALPHA);
        p2->setLightingEnabled(false);
        p2->setDepthWriteEnabled(false);
        p2->setDepthBias(3, 3);
        p2->setCullingMode(Ogre::CULL_NONE);
        p2->setSceneBlending(Ogre::SBT_TRANSPARENT_ALPHA);
        TextureUnitState* tus2 = p2->createTextureUnitState();
        tus2->setTextureName("tile.png");


        mat->setLightingEnabled(false);
        mat->setReceiveShadows(false);
    }
}

void CollisionsDebug::SetDrawEventBoxes(bool val)
{
    m_draw_collision_boxes = val;

    // Initial fill
    if (m_draw_collision_boxes && m_collision_boxes.size() == 0)
    {
        for (const collision_box_t& cbox : App::GetGameContext()->GetTerrain()->GetCollisions()->getCollisionBoxes())
        {
            this->AddCollisionBoxDebugMesh(cbox);
        }
    }
    // Update visibility
    for (Ogre::SceneNode* snode : m_collision_boxes)
    {
        snode->setVisible(m_draw_collision_boxes);
    }
}

void CollisionsDebug::SetDrawCollisionMeshes(bool val)
{
    m_draw_collision_meshes = val;

    // Initial setup
    if (m_draw_collision_meshes && m_collision_meshes.size() == 0)
    {
        for (const collision_mesh_t& cmesh : App::GetGameContext()->GetTerrain()->GetCollisions()->getCollisionMeshes())
        {
            this->AddCollisionMeshDebugMesh(cmesh);
        }
    }
    // Update visibility
    for (Ogre::SceneNode* snode : m_collision_meshes)
    {
        snode->setVisible(m_draw_collision_meshes);
    }
}

void CollisionsDebug::SetDrawCollisionCells(bool val)
{
    m_draw_collision_cells = val;

    // Initial setup
    if (m_draw_collision_cells && !m_collision_grid_root)
    {
        this->GenerateCellDebugMaterials();
        m_collision_grid_root = App::GetGfxScene()->GetSceneManager()->getRootSceneNode()->createChildSceneNode();
        // For memory (including VRAM) and performance reasons, only generate meshes up to a certain distance from the character.
        AxisAlignedBox aabb;
        aabb.setMinimum(App::GetGameContext()->GetPlayerCharacter()->getPosition() + Ogre::Vector3(-m_cell_generator_distance_limit, 0,  -m_cell_generator_distance_limit));
        aabb.getMinimum().y = -FLT_MAX; // vertical axis
        aabb.setMaximum(App::GetGameContext()->GetPlayerCharacter()->getPosition() + Ogre::Vector3(m_cell_generator_distance_limit, 0, m_cell_generator_distance_limit));
        aabb.getMaximum().y = FLT_MAX; // vertical axis
        try
        {
            App::GetGameContext()->GetTerrain()->GetCollisions()->createCollisionDebugVisualization(m_collision_grid_root, aabb, m_collision_cells);
        }
        catch (std::bad_alloc const& allocex)
        {
            App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_ERROR,
                fmt::format("Could not create debug view, error: {}", allocex.what()));
            this->ClearCollisionCellVisuals();
        }
    }
    // Update visibility
    if (m_collision_grid_root)
    {
        m_collision_grid_root->setVisible(m_draw_collision_cells);
    }
}

void CollisionsDebug::ClearCollisionMeshVisuals()
{
    for (Ogre::SceneNode* snode : m_collision_meshes)
    {
        Entity* ent = static_cast<Entity*>(snode->getAttachedObject(0));
        App::GetGfxScene()->GetSceneManager()->destroyEntity(ent);
        App::GetGfxScene()->GetSceneManager()->destroySceneNode(snode);
    }
    m_collision_meshes.clear();
    m_draw_collision_meshes = false;
    m_collision_mesh_draw_distance = DEFAULT_DRAWDIST;
}

void CollisionsDebug::ClearEventBoxVisuals()
{
    for (Ogre::SceneNode* snode : m_collision_boxes)
    {
        Entity* ent = static_cast<Entity*>(snode->getAttachedObject(0));
        App::GetGfxScene()->GetSceneManager()->destroyEntity(ent);
        App::GetGfxScene()->GetSceneManager()->destroySceneNode(snode);
    }
    m_collision_boxes.clear();
    m_draw_collision_boxes = false;
    m_collision_box_draw_distance = DEFAULT_DRAWDIST;
}

void CollisionsDebug::ClearCollisionCellVisuals()
{
    for (Ogre::SceneNode* snode : m_collision_cells)
    {
        ManualObject* mo = static_cast<ManualObject*>(snode->getAttachedObject(0));
        App::GetGfxScene()->GetSceneManager()->destroyManualObject(mo);
    }
    m_collision_cells.clear();
    m_draw_collision_cells = false;
    m_collision_cell_draw_distance = DEFAULT_DRAWDIST;
    
    if (m_collision_grid_root)
    {
        m_collision_grid_root->removeAndDestroyAllChildren();
        App::GetGfxScene()->GetSceneManager()->destroySceneNode(m_collision_grid_root);
        m_collision_grid_root = nullptr;
    }
}

void CollisionsDebug::SetVisible(bool v) 
{ 
    if (!v)
    {
        this->SetDrawEventBoxes(false);
        this->SetDrawCollisionMeshes(false);
        this->SetDrawCollisionCells(false);
    }
    m_is_visible = v;
}
