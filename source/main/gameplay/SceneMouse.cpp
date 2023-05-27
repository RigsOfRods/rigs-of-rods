/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2013-2023 Petr Ohlidal

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

#include "Actor.h"
#include "Application.h"
#include "GameContext.h"
#include "GfxScene.h"
#include "GUIUtils.h"
#include "InputEngine.h"
#include "ScriptEngine.h"

#include <Ogre.h>

using namespace Ogre;
using namespace RoR;

#define MOUSE_GRAB_FORCE 30000.0f

SceneMouse::SceneMouse() :
    mouseGrabState(0),
    grab_truck(nullptr)
{
    this->reset();
}

void SceneMouse::InitializeVisuals()
{

}

void SceneMouse::DiscardVisuals()
{

}

void SceneMouse::activateMousePick()
{
    grab_truck = mintruck;
    mouseGrabState = 1;
    TRIGGER_EVENT(SE_TRUCK_MOUSE_GRAB, grab_truck->ar_instance_id);

    for (hook_t& hook: grab_truck->ar_hooks)
    {
        if (hook.hk_hook_node->pos == minnode)
        {
            grab_truck->hookToggle(hook.hk_group, MOUSE_HOOK_TOGGLE, minnode);
        }
    }

    AddAffectorRequest* areq = new AddAffectorRequest();
    areq->aar_actor = grab_truck->ar_instance_id;
    areq->aar_set_mouseforce = true; // Assign us the AffectorID when available.
    areq->aar_affector.af_nodes.push_back(minnode);
    areq->aar_affector.af_type = AffectorType::PINNED_FORCE;
    areq->aar_affector.af_force_vector = lastgrabpos;
    areq->aar_affector.af_force_min = 0.f;
    areq->aar_affector.af_force_max = MOUSE_GRAB_FORCE;
    areq->aar_affector.af_input_ratio = 1.f; // Constant.
    App::GetGameContext()->PushMessage(Message(MSG_EDI_ADD_AFFECTOR_REQUESTED, areq));
}

void SceneMouse::releaseMousePick()
{
    if (App::sim_state->getEnum<SimState>() == SimState::PAUSED) { return; } // Do nothing when paused

    // remove forces
    if (grab_truck)
    {
        ROR_ASSERT(grab_affectorid != AFFECTORID_INVALID);
        ROR_ASSERT(grab_truck->affectorExists(grab_affectorid));

        App::GetGameContext()->PushMessage(
            Message(MSG_EDI_REMOVE_AFFECTOR_REQUESTED,
                new RemoveAffectorRequest(grab_truck->ar_instance_id, grab_affectorid)));
    }

    this->reset();
}

void SceneMouse::reset()
{
    minnode = NODENUM_INVALID;
    grab_affectorid = AFFECTORID_INVALID;
    grab_truck = nullptr;
    mindist = std::numeric_limits<float>::max();
    mouseGrabState = 0;
    lastgrabpos = Vector3::ZERO;
}

bool SceneMouse::mouseMoved(const OIS::MouseEvent& _arg)
{
    const OIS::MouseState ms = _arg.state;

    if (ms.buttonDown(OIS::MB_Left) && mouseGrabState == 0)
    {

        // mouse selection is updated every frame in `updateMouse*Highlights()`
        // check if we hit a node
        if (mintruck && minnode != NODENUM_INVALID)
        {
            this->activateMousePick();
        }
    }
    else if (ms.buttonDown(OIS::MB_Left) && mouseGrabState == 1)
    {
        // force applying and so forth happens in update()

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

void SceneMouse::updateMouseNodeHighlights(ActorPtr& actor)
{
    ROR_ASSERT(actor != nullptr);
    ROR_ASSERT(actor->ar_state == ActorState::LOCAL_SIMULATED);

    ImGui::Text("DBG updateMouseHighlights()");

    Ray mouseRay = getMouseRay();

    // check if our ray intersects with the bounding box of the truck
    std::pair<bool, Real> pair = mouseRay.intersects(actor->ar_bounding_box);
    if (!pair.first)
    {
        ImGui::Text("DBG AABB intersection fail!");
        return;
    }

    int numGrabHits = 0;
    int numHighlightHits = 0;
    for (int j = 0; j < actor->ar_num_nodes; j++)
    {
        // skip nodes with grabbing disabled
        if (actor->ar_nodes[j].nd_no_mouse_grab)
        {
            continue;
        }

        // check if our ray intersects with the node
        std::pair<bool, Real> pair = mouseRay.intersects(Sphere(actor->ar_nodes[j].AbsPosition, GRAB_SPHERE_SIZE));
        if (pair.first)
        {
            numGrabHits++;

            // we hit it, check if its the nearest node
            if (pair.second < mindist)
            {
                mindist = pair.second;
                minnode = static_cast<NodeNum_t>(j);
                mintruck = actor;
            }
        }

        // check if the node is close enough to be highlighted
        std::pair<bool, Real> highlight_result = mouseRay.intersects(Sphere(actor->ar_nodes[j].AbsPosition, this->HIGHLIGHT_SPHERE_SIZE));
        if (highlight_result.first)
        {
            numHighlightHits++;
            highlightedTruck = actor;

            highlightedNodes.push_back({ highlight_result.second, static_cast<NodeNum_t>(j) });
            highlightedNodesTopDistance = std::max(highlightedNodesTopDistance, highlight_result.second);
        }
    }
    ImGui::Text("DBG numGrabHits: %4d, numHighlightHits: %4d", numGrabHits, numHighlightHits);
}

void SceneMouse::updateMouseEffectHighlights(ActorPtr& actor)
{
    Ray mouseRay = getMouseRay();

    for (const affector_t& affector: actor->ar_affectors)
    {
        // The affected node
        for (NodeNum_t nodenum : affector.af_nodes)
        {
            std::pair<bool, Real> result = mouseRay.intersects(Sphere(actor->ar_nodes[nodenum].AbsPosition, this->FORCE_UNPIN_SPHERE_SIZE));
            if (result.first && result.second < aff_nodes_mindist)
            {
                aff_nodes_minnode = nodenum;
                aff_nodes_minaffector = affector.af_pos;
                aff_nodes_mindist = result.second;
                aff_nodes_mintruck = actor;
            }
        }

        // The pin point
        if (affector.af_type == AffectorType::PINNED_FORCE)
        {
            std::pair<bool, Real> result = mouseRay.intersects(Sphere(affector.af_force_vector, this->FORCE_UNPIN_SPHERE_SIZE));
            if (result.first && result.second < aff_pins_mindist)
            {
                aff_pins_minaffector = affector.af_pos;
                aff_pins_mindist = result.second;
                aff_pins_mintruck = actor;
            }
        }
    }
}

void SceneMouse::UpdateInputEvents()
{
    if (App::GetInputEngine()->getEventBoolValueBounce(EV_ROAD_EDITOR_POINT_INSERT))
    {
        if (mouseGrabState == 1 && grab_truck)
        {
            // Reset mouse grab but do not remove node forces
            this->reset();
        }
    }
}

void SceneMouse::UpdateSimulation()
{
    Ray mouseRay = getMouseRay();
    highlightedNodes.clear(); // clear every frame - highlights are not displayed when grabbing
    highlightedTruck = nullptr;

    if (mouseGrabState == 1 && grab_truck)
    {
        // get values
        lastgrabpos = mouseRay.getPoint(mindist);

        // update forces when the affector becomes available
        if (grab_affectorid != AFFECTORID_INVALID)
        {
            grab_truck->setAffectorForceVector(grab_affectorid, lastgrabpos);
        }
    }
    else
    {
        // refresh mouse highlight of nodes
        mintruck = nullptr;
        minnode = NODENUM_INVALID;
        mindist = std::numeric_limits<float>::max();
        for (ActorPtr& actor : App::GetGameContext()->GetActorManager()->GetActors())
        {
            if (actor->ar_state == ActorState::LOCAL_SIMULATED)
            {
                this->updateMouseNodeHighlights(actor);
            }
        }

        // refresh mouse highlight of effects
        aff_nodes_mintruck = nullptr;
        aff_nodes_minnode = NODENUM_INVALID;
        aff_nodes_minaffector = AFFECTORID_INVALID;
        aff_nodes_mindist = std::numeric_limits<float>::max();
        aff_pins_mintruck = nullptr;
        aff_pins_minaffector = AFFECTORID_INVALID;
        aff_pins_mindist = std::numeric_limits<float>::max();
        for (ActorPtr& actor : App::GetGameContext()->GetActorManager()->GetActors())
        {
            if (actor->ar_state == ActorState::LOCAL_SIMULATED)
            {
                this->updateMouseEffectHighlights(actor);
            }
        }
    }
}

void SceneMouse::drawMouseHighlights()
{
    ActorPtr actor = (mintruck != nullptr) ? mintruck : highlightedTruck;
    if (!actor)
        return; // Nothing to draw

    ImDrawList* drawlist = GetImDummyFullscreenWindow("Mouse-grab node highlights");
    const int LAYER_HIGHLIGHTS = 0;
    const int LAYER_MINNODE = 1;
    drawlist->ChannelsSplit(2);

    Vector2 screenPos;
    const GUIManager::GuiTheme& theme = App::GetGuiManager()->GetTheme();
    for (const HighlightedNode& hnode : highlightedNodes)
    {
        if (GetScreenPosFromWorldPos(actor->ar_nodes[hnode.nodenum].AbsPosition, /*out:*/screenPos))
        {
            float animRatio = 1.f - (hnode.distance / highlightedNodesTopDistance); // the closer the bigger
            float radius = (theme.mouse_highlighted_node_radius_max - theme.mouse_highlighted_node_radius_min) * animRatio;

            if (hnode.nodenum == minnode)
            {
                drawlist->ChannelsSetCurrent(LAYER_MINNODE);
                drawlist->AddCircle(ImVec2(screenPos.x, screenPos.y),
                    radius, ImColor(theme.mouse_minnode_color),
                    theme.node_circle_num_segments, theme.mouse_minnode_thickness);
            }

            ImVec4 color = theme.mouse_highlighted_node_color * animRatio;
            color.w = 1.f; // no transparency
            drawlist->ChannelsSetCurrent(LAYER_HIGHLIGHTS);
            drawlist->AddCircleFilled(ImVec2(screenPos.x, screenPos.y), radius, ImColor(color), theme.node_circle_num_segments);
        }
    }

    drawlist->ChannelsMerge();
}

void SceneMouse::drawNodeEffects()
{
    const GUIManager::GuiTheme& theme = App::GetGuiManager()->GetTheme();
    ImDrawList* drawlist = GetImDummyFullscreenWindow("Node effect view");
    const int LAYER_LINES = 0;
    const int LAYER_CIRCLES = 1;
    drawlist->ChannelsSplit(2);

    for (ActorPtr& actor : App::GetGameContext()->GetActorManager()->GetActors())
    {
        if (actor->ar_state != ActorState::LOCAL_SIMULATED)
            continue;

        for (affector_t& affector: actor->ar_affectors)
        {
            for (NodeNum_t nodenum : affector.af_nodes)
            {
                Vector2 nodeScreenPos, pointScreenPos;
                Vector3 pointWorldPos;
                if (affector.af_type == AffectorType::PINNED_FORCE)
                {
                    // Point position specified externally
                    pointWorldPos = affector.af_force_vector;
                }
                else
                {
                    // Point position calculated based on force
                    pointWorldPos = actor->ar_nodes[nodenum].AbsPosition + (affector.curForce() * FORCE_NEWTONS_TO_LINE_LENGTH_RATIO);
                }
                if (GetScreenPosFromWorldPos(actor->ar_nodes[nodenum].AbsPosition, nodeScreenPos)
                    && GetScreenPosFromWorldPos(pointWorldPos, pointScreenPos))
                {
                    drawlist->ChannelsSetCurrent(LAYER_LINES);
                    const bool highlight = (nodenum == aff_nodes_minnode) && actor == aff_nodes_mintruck;
                    const ImColor color = (highlight)
                        ? ImColor(theme.node_effect_highlight_line_color)
                        : ImColor(theme.node_effect_force_line_color);
                    drawlist->AddLine(ImVec2(nodeScreenPos.x, nodeScreenPos.y), ImVec2(pointScreenPos.x, pointScreenPos.y),
                        color, theme.node_effect_force_line_thickness);

                    if (affector.af_pos == aff_pins_minaffector)
                    {
                        // This is a PINNED force, draw the extra pin circle
                        drawlist->ChannelsSetCurrent(LAYER_CIRCLES);
                        drawlist->AddCircleFilled(ImVec2(pointScreenPos.x, pointScreenPos.y),
                            theme.node_effect_force_circle_radius, color);
                    }
                }
            }
        }
    }

    drawlist->ChannelsMerge();
}

void SceneMouse::UpdateVisuals()
{
    if (grab_truck == nullptr)
    {
        this->drawMouseHighlights();
    }

    this->drawNodeEffects();
}

bool SceneMouse::mousePressed(const OIS::MouseEvent& _arg, OIS::MouseButtonID _id)
{
    if (App::sim_state->getEnum<SimState>() == SimState::PAUSED) { return true; } // Do nothing when paused

    const OIS::MouseState ms = _arg.state;

    if (ms.buttonDown(OIS::MB_Middle))
    {
        Ray mouseRay = getMouseRay();

        if (App::sim_state->getEnum<SimState>() == SimState::EDITOR_MODE)
        {
            return true;
        }

        ActorPtr player_actor = App::GetGameContext()->GetPlayerActor();

        // Reselect the player actor
        {
            Real nearest_ray_distance = std::numeric_limits<float>::max();

            for (ActorPtr& actor : App::GetGameContext()->GetActorManager()->GetActors())
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
                            App::GetGameContext()->PushMessage(Message(MSG_SIM_SEAT_PLAYER_REQUESTED, static_cast<void*>(new ActorPtr(actor))));
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
            NodeNum_t nearest_node_index = NODENUM_INVALID;

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
                        nearest_node_index = (NodeNum_t)i;
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

    // RMB unpins forces
    if (ms.buttonDown(OIS::MB_Right) && aff_nodes_minnode != NODENUM_INVALID)
    {
        ROR_ASSERT(aff_nodes_minaffector != AFFECTORID_INVALID);
        App::GetGameContext()->PushMessage(
            Message(MSG_EDI_REMOVE_AFFECTOR_REQUESTED,
                new RemoveAffectorRequest(aff_nodes_mintruck->ar_instance_id, aff_nodes_minaffector)));
        aff_nodes_minaffector = AFFECTORID_INVALID;
        aff_nodes_minnode = NODENUM_INVALID;
        aff_nodes_mintruck = nullptr;
    }
    else if (ms.buttonDown(OIS::MB_Right) && aff_pins_minaffector != AFFECTORID_INVALID)
    {
        App::GetGameContext()->PushMessage(
            Message(MSG_EDI_REMOVE_AFFECTOR_REQUESTED,
                new RemoveAffectorRequest(aff_pins_mintruck->ar_instance_id, aff_pins_minaffector)));
        aff_pins_minaffector = AFFECTORID_INVALID;
        aff_pins_mintruck = nullptr;
    }

    return true;
}

bool SceneMouse::mouseReleased(const OIS::MouseEvent& _arg, OIS::MouseButtonID _id)
{
    if (App::sim_state->getEnum<SimState>() == SimState::PAUSED) { return true; } // Do nothing when paused

    if (mouseGrabState == 1)
    {
        releaseMousePick();
    }

    return true;
}

Ray SceneMouse::getMouseRay()
{
    int lastMouseX = App::GetInputEngine()->getMouseState().X.abs;
    int lastMouseY = App::GetInputEngine()->getMouseState().Y.abs;

    Viewport* vp = App::GetCameraManager()->GetCamera()->getViewport();

    return App::GetCameraManager()->GetCamera()->getCameraToViewportRay((float)lastMouseX / (float)vp->getActualWidth(), (float)lastMouseY / (float)vp->getActualHeight());
}
