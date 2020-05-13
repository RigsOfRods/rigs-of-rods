/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2013-2014 Petr Ohlidal

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

/// @file   SceneMouse.h
/// @author Thomas Fischer <thomas@thomasfischer.biz>
/// @date   11th of March 2011

#include "SceneMouse.h"

#include "Application.h"
#include "Beam.h"
#include "RoRFrameListener.h"

#include <OgreSceneManager.h>
#include <OgreMaterialManager.h>
#include <OgreResourceGroupManager.h>
#include <OgreTechnique.h>
#include <OgreManualObject.h>

using namespace Ogre;
using namespace RoR;

#define MOUSE_GRAB_FORCE 30000.0f

SceneMouse::SceneMouse() :
    mouseGrabState(0),
    grab_truck(nullptr),
    pickLine(nullptr),
    pickLineNode(nullptr)
{
    releaseMousePick(); // TODO: Yuck! Rewrite in proper code!! ~ only_a_ptr, 06/2018
}

void SceneMouse::InitializeVisuals()
{
    // load 3d line for mouse picking
    pickLine = App::GetGfxScene()->GetSceneManager()->createManualObject("PickLineObject");
    pickLineNode = App::GetGfxScene()->GetSceneManager()->getRootSceneNode()->createChildSceneNode("PickLineNode");

    MaterialPtr pickLineMaterial = MaterialManager::getSingleton().getByName("PickLineMaterial", ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
    if (pickLineMaterial.isNull())
    {
        pickLineMaterial = MaterialManager::getSingleton().create("PickLineMaterial", ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
    }
    pickLineMaterial->setReceiveShadows(false);
    pickLineMaterial->getTechnique(0)->setLightingEnabled(true);
    pickLineMaterial->getTechnique(0)->getPass(0)->setDiffuse(0, 0, 1, 0);
    pickLineMaterial->getTechnique(0)->getPass(0)->setAmbient(0, 0, 1);
    pickLineMaterial->getTechnique(0)->getPass(0)->setSelfIllumination(0, 0, 1);

    pickLine->begin("PickLineMaterial", RenderOperation::OT_LINE_LIST);
    pickLine->position(0, 0, 0);
    pickLine->position(0, 0, 0);
    pickLine->end();
    pickLineNode->attachObject(pickLine);
    pickLineNode->setVisible(false);
}

void SceneMouse::DiscardVisuals()
{
    if (pickLineNode != nullptr)
    {
        App::GetGfxScene()->GetSceneManager()->getRootSceneNode()->removeAndDestroyChild("PickLineNode");
        pickLineNode = nullptr;
    }

    if (pickLine != nullptr)
    {
        App::GetGfxScene()->GetSceneManager()->destroyManualObject("PickLineObject");
        pickLine = nullptr;
    }
}

void SceneMouse::releaseMousePick()
{
    if (App::sim_state->GetActiveEnum<SimState>() == SimState::PAUSED) { return; } // Do nothing when paused

    // remove forces
    if (grab_truck)
        grab_truck->HandleMouseMove(minnode, Vector3::ZERO, 0);

    // reset the variables
    minnode = -1;
    grab_truck = 0;
    mindist = 99999;
    mouseGrabState = 0;
    lastgrabpos = Vector3::ZERO;
    lastMouseX = 0;
    lastMouseY = 0;

    mouseGrabState = 0;
}

bool SceneMouse::mouseMoved(const OIS::MouseEvent& _arg)
{
    const OIS::MouseState ms = _arg.state;

    // experimental mouse hack
    if (ms.buttonDown(OIS::MB_Left) && mouseGrabState == 0)
    {
        lastMouseY = ms.Y.abs;
        lastMouseX = ms.X.abs;

        Ray mouseRay = getMouseRay();

        // walk all trucks
        minnode = -1;
        grab_truck = NULL;
        for (auto actor : App::GetSimController()->GetActors())
        {
            if (actor->ar_sim_state == Actor::SimState::LOCAL_SIMULATED)
            {
                // check if our ray intersects with the bounding box of the truck
                std::pair<bool, Real> pair = mouseRay.intersects(actor->ar_bounding_box);
                if (!pair.first)
                    continue;

                for (int j = 0; j < actor->ar_num_nodes; j++)
                {
                    if (actor->ar_nodes[j].nd_no_mouse_grab)
                        continue;

                    // check if our ray intersects with the node
                    std::pair<bool, Real> pair = mouseRay.intersects(Sphere(actor->ar_nodes[j].AbsPosition, 0.1f));
                    if (pair.first)
                    {
                        // we hit it, check if its the nearest node
                        if (pair.second < mindist)
                        {
                            mindist = pair.second;
                            minnode = j;
                            grab_truck = actor;
                        }
                    }
                }
            }
        }

        // check if we hit a node
        if (grab_truck && minnode != -1)
        {
            mouseGrabState = 1;

            for (std::vector<hook_t>::iterator it = grab_truck->ar_hooks.begin(); it != grab_truck->ar_hooks.end(); it++)
            {
                if (it->hk_hook_node->pos == minnode)
                {
                    grab_truck->ToggleHooks(it->hk_group, MOUSE_HOOK_TOGGLE, minnode);
                }
            }
        }
    }
    else if (ms.buttonDown(OIS::MB_Left) && mouseGrabState == 1)
    {
        // force applying and so forth happens in update()
        lastMouseY = ms.Y.abs;
        lastMouseX = ms.X.abs;
        // not fixed
        return false;
    }
    else if (!ms.buttonDown(OIS::MB_Left) && mouseGrabState == 1)
    {
        releaseMousePick();
        // not fixed
        return false;
    }

    return false;
}

void SceneMouse::UpdateSimulation()
{
    if (mouseGrabState == 1 && grab_truck)
    {
        // get values
        Ray mouseRay = getMouseRay(); // TODO: Touches OGRE Camera+Viewport which shouldn't happen here ~ only_a_ptr, 06/2018
        lastgrabpos = mouseRay.getPoint(mindist);

        // add forces
        grab_truck->HandleMouseMove(minnode, lastgrabpos, MOUSE_GRAB_FORCE);
    }
}

void SceneMouse::UpdateVisuals()
{
    if (grab_truck == nullptr)
    {
        pickLineNode->setVisible(false);   // Hide the line     
    }
    else
    {
        pickLineNode->setVisible(true);   // Show the line
        // update visual line
        pickLine->beginUpdate(0);
        pickLine->position(grab_truck->GetGfxActor()->GetSimNodeBuffer()[minnode].AbsPosition);
        pickLine->position(lastgrabpos);
        pickLine->end();
    }
}

bool SceneMouse::mousePressed(const OIS::MouseEvent& _arg, OIS::MouseButtonID _id)
{
    if (App::sim_state->GetActiveEnum<SimState>() == SimState::PAUSED) { return true; } // Do nothing when paused

    const OIS::MouseState ms = _arg.state;

    if (ms.buttonDown(OIS::MB_Middle))
    {
        lastMouseY = ms.Y.abs;
        lastMouseX = ms.X.abs;
        Ray mouseRay = getMouseRay();

        if (App::sim_state->GetActiveEnum<SimState>() == SimState::EDITOR_MODE)
        {
            App::GetSimController()->SetTerrainEditorMouseRay(mouseRay);
            return true;
        }

        Actor* player_actor = App::GetSimController()->GetPlayerActor();

        // Reselect the player actor
        {
            Real nearest_ray_distance = std::numeric_limits<float>::max();

            for (auto actor : App::GetSimController()->GetActors())
            {
                if (actor != player_actor)
                {
                    Vector3 pos = actor->getPosition();
                    std::pair<bool, Real> pair = mouseRay.intersects(Sphere(pos, actor->getMinCameraRadius()));
                    if (pair.first)
                    {
                        Real ray_distance = mouseRay.getDirection().crossProduct(pos - mouseRay.getOrigin()).length();
                        if (ray_distance < nearest_ray_distance)
                        {
                            nearest_ray_distance = ray_distance;
                            App::GetSimController()->SetPendingPlayerActor(actor);
                        }
                    }
                }
            }
        }

        // Reselect the vehicle orbit camera center
        if (player_actor && App::GetCameraManager()->GetCurrentBehavior() == CameraManager::CAMERA_BEHAVIOR_VEHICLE)
        {
            Real nearest_camera_distance = std::numeric_limits<float>::max();
            Real nearest_ray_distance = std::numeric_limits<float>::max();
            int nearest_node_index = -1;

            for (int i = 0; i < player_actor->ar_num_nodes; i++)
            {
                Vector3 pos = player_actor->ar_nodes[i].AbsPosition;
                std::pair<bool, Real> pair = mouseRay.intersects(Sphere(pos, 0.25f));
                if (pair.first)
                {
                    Real ray_distance = mouseRay.getDirection().crossProduct(pos - mouseRay.getOrigin()).length();
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

    return true;
}

bool SceneMouse::mouseReleased(const OIS::MouseEvent& _arg, OIS::MouseButtonID _id)
{
    if (App::sim_state->GetActiveEnum<SimState>() == SimState::PAUSED) { return true; } // Do nothing when paused

    App::GetSimController()->SetTerrainEditorMouseRay(Ray(Vector3::ZERO, Vector3::ZERO));

    if (mouseGrabState == 1)
    {
        releaseMousePick();
    }

    return true;
}

bool SceneMouse::keyPressed(const OIS::KeyEvent& _arg)
{
    return false;
}

bool SceneMouse::keyReleased(const OIS::KeyEvent& _arg)
{
    return false;
}

Ray SceneMouse::getMouseRay()
{
    Viewport* vp = App::GetCameraManager()->GetCamera()->getViewport();

    return App::GetCameraManager()->GetCamera()->getCameraToViewportRay((float)lastMouseX / (float)vp->getActualWidth(), (float)lastMouseY / (float)vp->getActualHeight());
}
