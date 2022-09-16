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

    ImGuiWindowFlags win_flags = ImGuiWindowFlags_NoCollapse;
    bool keep_open = true;
    ImGui::Begin(_LC("About", "Static collision debug"), &keep_open, win_flags);

    ImGui::Text("Terrain name: %s", App::GetSimTerrain()->getTerrainName().c_str());
    ImGui::Separator();
    ImGui::Text("Num collision boxes: %d", (int)App::GetSimTerrain()->GetCollisions()->getCollisionBoxes().size());
    if (ImGui::Checkbox("Show collision boxes", &m_draw_collision_boxes))
    {
        // Initial fill
        if (m_draw_collision_boxes &&m_collision_boxes.size() == 0)
        {
            for (const collision_box_t& cbox : App::GetSimTerrain()->GetCollisions()->getCollisionBoxes())
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

    m_is_hovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows);
    App::GetGuiManager()->RequestGuiCaptureKeyboard(m_is_hovered);

    ImGui::End();

    if (!keep_open)
    {
        this->SetVisible(false);
    }

    // only draw reasonably close labels
    const Ogre::Vector3 cam_pos = App::GetCameraManager()->GetCameraNode()->getPosition();
    const float MAX_DISTANCE_METERS = 155.f;

    if (m_draw_collision_boxes)
    {
        for (const collision_box_t& cbox : App::GetSimTerrain()->GetCollisions()->getCollisionBoxes())
        {
            if (IsDistanceWithin(cam_pos, this->GetCollBoxWorldPos(cbox), MAX_DISTANCE_METERS))
            {
                this->DrawCollisionBoxDebugText(cbox);
            }
        }
    }
}


void CollisionsDebug::AddCollisionBoxDebugMesh(collision_box_t const& coll_box)
{
    int scripthandler = -1;
    if (coll_box.eventsourcenum != -1)
        scripthandler = App::GetSimTerrain()->GetCollisions()->getEventSource(coll_box.eventsourcenum).scripthandler;

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

void CollisionsDebug::DrawCollisionBoxDebugText(collision_box_t const& coll_box)
{
    if (!coll_box.virt)
        return;

    int scripthandler = -1;
    std::string boxname;
    std::string instancename;
    if (coll_box.eventsourcenum != -1)
    {
        scripthandler = App::GetSimTerrain()->GetCollisions()->getEventSource(coll_box.eventsourcenum).scripthandler;
        boxname = App::GetSimTerrain()->GetCollisions()->getEventSource(coll_box.eventsourcenum).boxname;
        instancename = App::GetSimTerrain()->GetCollisions()->getEventSource(coll_box.eventsourcenum).instancename;
    }

    String caption = "EVENTBOX\nevent:" + String(boxname) + "\ninstance:" + String(instancename) + "\nhandler:" + TOSTRING(scripthandler);

    this->DrawLabelAtWorldPos(caption, this->GetCollBoxWorldPos(coll_box));
}

void CollisionsDebug::DrawLabelAtWorldPos(std::string const& caption, Ogre::Vector3 const& world_pos)
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
        ImVec4 text_color = ImGui::GetStyle().Colors[ImGuiCol_Text];
        drawlist->AddText(g->Font, g->FontSize, text_pos, ImColor(text_color), caption.c_str());
    }
    // ---- END COPYPASTE ----
}

void CollisionsDebug::CleanUp()
{
    for (Ogre::SceneNode* snode : m_collision_boxes)
    {
        snode->removeAndDestroyAllChildren();
        App::GetGfxScene()->GetSceneManager()->destroySceneNode(snode);
    }
    m_collision_boxes.clear();
}
