/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2013-2020 Petr Ohlidal

    For more information, see http://www.rigsofrods.org/

    Rigs of Rods is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 3, as
    published by the Free Software Foundation.

    Rigs of Rods is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Rigs of Rods. If not, see <http://www.gnu.org/licenses/>.
*/

/// Mouse interaction with 3D scene: node grab (LMB), player actor/camera node select (MMB)
/// author: Thomas Fischer <thomas@thomasfischer.biz> 11th of March 2011
/// author: Remade with DearIMGUI by Petr Ohlidal, 2020

#include "GUI_SceneMouse.h"

#include "Application.h"
#include "Actor.h"
#include "CameraManager.h"
#include "GameContext.h"
#include "OgreImGui.h"
#include "GUIUtils.h"

using namespace Ogre;
using namespace RoR;

#define MOUSE_GRAB_FORCE 30000.0f

void GUI::SceneMouse::Draw()
{
    if (App::sim_state->GetEnum<SimState>() == SimState::PAUSED)
    {
        return; // Do nothing when paused
    }

    if (ImGui::IsMouseDown(0)) // LMB button is pressed
    {
        Ogre::Ray mouse_ray = this->CastMouseRay();
        if (grab_truck)
        {
            // get values
            lastgrabpos = mouse_ray.getPoint(mindist);

            // add forces
            grab_truck->HandleMouseMove(minnode, lastgrabpos, MOUSE_GRAB_FORCE);
        }
        else
        {
            // walk all trucks
            minnode = -1;
            for (auto actor : App::GetGameContext()->GetActorManager()->GetActors())
            {
                if (actor->ar_sim_state != Actor::SimState::LOCAL_SIMULATED)
                    continue;
                
                // check if our ray intersects with the bounding box of the truck
                std::pair<bool, Real> pair = mouse_ray.intersects(actor->ar_bounding_box);
                if (!pair.first)
                    continue;

                for (int j = 0; j < actor->ar_num_nodes; j++)
                {
                    if (actor->ar_nodes[j].nd_no_mouse_grab)
                        continue;

                    // check if our ray intersects with the node
                    std::pair<bool, Real> pair = mouse_ray.intersects(Sphere(actor->ar_nodes[j].AbsPosition, 0.1f));
                    if (pair.first)
                    {
                        // we hit it, check if its the nearest node
                        if (pair.second < mindist)
                        {
                            mindist = pair.second;
                            minnode = j;
                            grab_truck = actor;
                            lastgrabpos = actor->ar_nodes[j].AbsPosition; // For drawing only
                        }
                    }
                }
                
            }

            // check if we hit a node
            if (grab_truck)
            {
                for (std::vector<hook_t>::iterator it = grab_truck->ar_hooks.begin(); it != grab_truck->ar_hooks.end(); it++)
                {
                    if (it->hk_hook_node->pos == minnode)
                    {
                        grab_truck->ToggleHooks(it->hk_group, MOUSE_HOOK_TOGGLE, minnode);
                    }
                }
            }
        }
    }
    else // LMB not pressed
    {
        // remove forces
        if (grab_truck)
            grab_truck->HandleMouseMove(minnode, Vector3::ZERO, 0);

        // reset the variables
        minnode = -1;
        grab_truck = nullptr;
        mindist = 99999;
        lastgrabpos = Vector3::ZERO;
    }

    if (ImGui::IsMouseClicked(1)) // Middle mouse button
    {
        Ogre::Ray mouse_ray = this->CastMouseRay();
        Real nearest_ray_distance = std::numeric_limits<float>::max();
        Actor* player_actor = App::GetGameContext()->GetPlayerActor();
        for (auto actor : App::GetGameContext()->GetActorManager()->GetActors())
        {
            if (actor != player_actor)
            {
                Vector3 pos = actor->getPosition();
                std::pair<bool, Real> pair = mouse_ray.intersects(Sphere(pos, actor->getMinCameraRadius()));
                if (pair.first)
                {
                    Real ray_distance = mouse_ray.getDirection().crossProduct(pos - mouse_ray.getOrigin()).length();
                    if (ray_distance < nearest_ray_distance)
                    {
                        nearest_ray_distance = ray_distance;
                        App::GetGameContext()->PushMessage(Message(MSG_SIM_SEAT_PLAYER_REQUESTED, (void*)actor));
                    }
                }
            }
        }

        if (player_actor && App::GetCameraManager()->GetCurrentBehavior() == CameraManager::CAMERA_BEHAVIOR_VEHICLE)
        {
            Real nearest_camera_distance = std::numeric_limits<float>::max();
            Real nearest_ray_distance = std::numeric_limits<float>::max();
            int nearest_node_index = -1;

            // Reselect the player actor
            for (int i = 0; i < player_actor->ar_num_nodes; i++)
            {
                Vector3 pos = player_actor->ar_nodes[i].AbsPosition;
                std::pair<bool, Real> pair = mouse_ray.intersects(Sphere(pos, 0.25f));
                if (pair.first)
                {
                    Real ray_distance = mouse_ray.getDirection().crossProduct(pos - mouse_ray.getOrigin()).length();
                    if (ray_distance < nearest_ray_distance || (ray_distance == nearest_ray_distance && pair.second < nearest_camera_distance))
                    {
                        nearest_camera_distance = pair.second;
                        nearest_ray_distance = ray_distance;
                        nearest_node_index = i;
                    }
                }
            }
            if (player_actor->ar_custom_camera_node != nearest_node_index)
            {
                player_actor->ar_custom_camera_node = nearest_node_index;
                player_actor->calculateAveragePosition();
                App::GetCameraManager()->NotifyContextChange(); // Reset last 'look at' pos
            }
        }
    }

    if (grab_truck)
    {
        // Draw the grab-line
        World2ScreenConverter world2screen;
        const Vector3 mouse_screen_xyz = world2screen.Convert(lastgrabpos);
        const Vector3 node_screen_xyz = world2screen.Convert(grab_truck->ar_nodes[minnode].AbsPosition);

        const ImVec2 mouse_screen(mouse_screen_xyz.x, mouse_screen_xyz.y);
        const ImVec2 node_screen(node_screen_xyz.x, node_screen_xyz.y);

        ImDrawList* drawlist = RoR::ObtainImGuiDrawList("RoR-SceneMouse");

        drawlist->AddLine(mouse_screen, node_screen, ImColor(0.2f, 0.2f, 0.9f), 2.f);
        drawlist->AddCircleFilled(mouse_screen, 4.f, ImColor(0.2f, 0.2f, 0.9f));
        drawlist->AddCircleFilled(node_screen, 4.f, ImColor(0.2f, 0.2f, 0.9f));
    }
}

Ogre::Ray GUI::SceneMouse::CastMouseRay()
{
    ImVec2 mouse_screen = ImGui::GetIO().MousePos / ImGui::GetIO().DisplaySize;
    return App::GetCameraManager()->GetCamera()->getCameraToViewportRay(mouse_screen.x, mouse_screen.y);    
}

