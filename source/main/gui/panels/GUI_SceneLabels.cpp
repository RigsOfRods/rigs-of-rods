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
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Rigs of Rods. If not, see <http://www.gnu.org/licenses/>.
*/

#include "GUI_SceneLabels.h"

#include "Application.h"
#include "Actor.h"
#include "Console.h"
#include "EngineSim.h"
#include "GameContext.h"
#include "GfxActor.h"
#include "GUIManager.h"
#include "GUIUtils.h"
#include "InputEngine.h"
#include "SoundScriptManager.h"
#include "Utils.h"

#include <Ogre.h>
#include <imgui_internal.h>

using namespace RoR;
using namespace GUI;
using namespace Ogre;

void SceneLabels::Update()
{
    if (!m_icons_cached)
        this->CacheIcons();

    // Evaluate mouse hover
    m_hovered_actor = this->FindHoveredActor();

    // Draw hovered actor box
    if (m_hovered_actor)
    {
        float vlen = m_hovered_actor->getPosition().distance(App::GetCameraManager()->GetCameraNode()->getPosition());
        this->DrawInstance(m_hovered_actor->getPosition() + Vector3(0.f, 1.4f, 0.f), vlen, nullptr);
    }

    // NOTE: walkietalkie labels are drawn from `GfxScene::UpdateWalkieTalkieLabels()`
}

ActorPtr SceneLabels::FindHoveredActor()
{
    // get mouse ray
    Ogre::Viewport* vp = App::GetCameraManager()->GetCamera()->getViewport();
    Ogre::Ray mouseRay = App::GetCameraManager()->GetCamera()->getCameraToViewportRay(
        (float)ImGui::GetIO().MousePos.x / (float)vp->getActualWidth(),
        (float)ImGui::GetIO().MousePos.y / (float)vp->getActualHeight());

    // walk all trucks
    ActorPtr grab_truck;
    float mindist = 99999;
    for (const ActorPtr& actor : App::GetGameContext()->GetActorManager()->GetActors())
    {
        if (actor->ar_state == ActorState::LOCAL_SIMULATED)
        {
            // check if our ray intersects with the bounding box of the truck
            std::pair<bool, Real> pair = mouseRay.intersects(actor->ar_bounding_box);
            if (pair.first)
            {
                // we hit it, check if its the nearest sphere
                if (pair.second < mindist)
                {
                    mindist = pair.second;
                    grab_truck = actor;
                }
            }
            
        }
    }

    return grab_truck;
}

void DrawRow(events ev, const Ogre::TexturePtr& icon)
{
    ImDrawEventHighlighted(ev);
    ImGui::SameLine();
    if (icon)
    {
        ImGui::ImageButton(reinterpret_cast<ImTextureID>(icon->getHandle()), ImVec2(24, 24));
        ImGui::SameLine();
    }
    ImGui::TextDisabled(App::GetInputEngine()->eventIDToDescription(ev).c_str());
    ImGui::NextColumn();
}

void SceneLabels::DrawInstance(Ogre::Vector3 scene_pos, float cam_dist, const ActorPtr& actor)
{
    // Draw a context-sensitive label:
    // * Character: walkietalkie icon
    // * Actor: icon and usable light+command keys
    // * Actor (mouse hover): name and mouse hints
    // ----------------------------------------------

    if (!m_icons_cached)
        this->CacheIcons();

    ImVec2 screen_size = ImGui::GetIO().DisplaySize;
    World2ScreenConverter world2screen(
        App::GetCameraManager()->GetCamera()->getViewMatrix(true), App::GetCameraManager()->GetCamera()->getProjectionMatrix(), Ogre::Vector2(screen_size.x, screen_size.y));

    Ogre::Vector3 pos_xyz = world2screen.Convert(scene_pos);

    // Do not draw when behind camera
    if (pos_xyz.z > 0.f)
        return;

    // Align position to whole pixels, to minimize jitter.
    ImVec2 pos((int)pos_xyz.x+0.5, (int)pos_xyz.y+0.5);

    Ogre::TexturePtr icon = FetchIcon("walkie_talkie.png", "IconsRG");
    ImVec2 icon_size(24.f, 24.f);//(icon->getWidth(), icon->getHeight());

    const float ICON_PAD_RIGHT = 5.f;
    ImVec2 total_size = icon_size;

    std::string caption;
    std::string id_str = "walkie talkie for character";
    if (actor)
    {
        caption = FormatLabelWithDistance(actor->ar_design_name, cam_dist);

        // Place name+commands next to the icon
        ImVec2 namesize = ImGui::CalcTextSize(caption.c_str());
        total_size.x += ICON_PAD_RIGHT + std::max(namesize.x, actor->m_walkietalkie_commandkeys_screensize.x);
        total_size.y = std::max(total_size.y, actor->m_walkietalkie_commandkeys_screensize.y);

        // Account for space between event-button and description
        total_size += ImGui::GetStyle().ItemSpacing * ImVec2(1.f, 0.f)
            + ImVec2(ImGui::GetStyle().FramePadding.x*2, 0.f);

        id_str = fmt::format("walkie talkie for actor {}", actor->ar_instance_id).c_str();
    }
    else if (m_hovered_actor)
    {
        caption = FormatLabelWithDistance(m_hovered_actor->ar_design_name, cam_dist);
        id_str = fmt::format("mouse hover box for actor {}", m_hovered_actor->ar_instance_id).c_str();
        icon.reset(); // omit icon

        ImVec2 namesize = ImGui::CalcTextSize(caption.c_str());
        total_size = ImVec2(std::max(namesize.x, 250.f), 55);
    }

    // Because we want buttons in the label, it must be a window, not a fullscreen dummy for drawing
    const float PADDING = 4.f;
    const ImVec2 BOXPAD2(PADDING, PADDING);
    const ImVec2 BTNPAD2(2.f, 0.f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, BOXPAD2);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, BTNPAD2);
    ImVec2 rect_min = (pos - total_size/2) - BOXPAD2;
    ImGui::SetNextWindowPos(rect_min);
    ImGui::SetNextWindowSize(total_size + BOXPAD2*2);
    int window_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar 
                     | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoBringToFrontOnFocus;
    if (ImGui::Begin(id_str.c_str(), nullptr, window_flags))
    {
        GUIManager::GuiTheme const& theme = App::GetGuiManager()->GetTheme();
   
        if (actor)
        {
            // Draw actor name (dark text)
            ImGui::SetCursorPosX(icon_size.x + ICON_PAD_RIGHT);
            ImGui::TextDisabled(caption.c_str());
            ImGui::Separator();

            // Draw light controls (with activity highlight)
            for (auto& pair: actor->m_walkietalkie_lights_cache)
            {
                ImGui::SetCursorPosX(icon_size.x + ICON_PAD_RIGHT);
                DrawRow(pair.first, pair.second);
            } 

            // Draw the commands (with activity highlight)
            for (auto& pair: actor->m_walkietalkie_commandkeys_cache)
            {
                const bool force_active = (actor->ar_command_key[pair.first].playerInputValue != 0);
                ImGui::SetCursorPosX(icon_size.x + ICON_PAD_RIGHT);
                ImDrawEventHighlighted(events(pair.first), force_active);
                ImGui::SameLine();
                ImGui::Text(pair.second.c_str());
            }
        }
        else if (m_hovered_actor)
        {
            // Draw actor name (dark text)
            ImGui::TextDisabled(caption.c_str());
            ImGui::Separator();

            // Bottom area - Draw mouse hints
            float orig_y = ImGui::GetCursorPosY();

            // Left mouse button = grab
            ImGui::Image(reinterpret_cast<ImTextureID>(rc_left_mouse_button->getHandle()), ImVec2(28, 24));
            ImGui::SameLine();
            ImGui::SetCursorPosY(ImGui::GetCursorPosY() + ImGui::GetStyle().ItemSpacing.y);
            ImGui::Text("%s", _LC("ActorMouseHoverBox", "Grab"));

            // Middle mouse button = enter
            ImGui::SameLine();
            ImGui::SetCursorPosY(orig_y);
            ImGui::Image(reinterpret_cast<ImTextureID>(rc_middle_mouse_button->getHandle()), ImVec2(28, 24));
            ImGui::SameLine();
            ImGui::SetCursorPosY(ImGui::GetCursorPosY() + ImGui::GetStyle().ItemSpacing.y);
            ImGui::Text("%s", _LC("ActorMouseHoverBox", "Enter"));

            // Right mouse button = menu
            ImGui::SameLine();
            ImGui::SetCursorPosY(orig_y);
            ImGui::Image(reinterpret_cast<ImTextureID>(rc_middle_mouse_button->getHandle()), ImVec2(28, 24));
            ImGui::SameLine();
            ImGui::SetCursorPosY(ImGui::GetCursorPosY() + ImGui::GetStyle().ItemSpacing.y);

            ImGui::Text("%s", _LC("ActorMouseHoverBox", "Open menu"));
        }

        // Draw icon last
        if (icon)
        {
            ImGui::SetCursorPos(BOXPAD2);
            ImGui::Image(reinterpret_cast<ImTextureID>(icon->getHandle()), icon_size);
        }

        App::GetGuiManager()->RequestStaticMenusBlocking(ImGui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows));
        ImGui::End();
        ImGui::PopStyleVar(); //ImGuiStyleVar_WindowPadding, BOXPAD2);
        ImGui::PopStyleVar(); //ImGuiStyleVar_FramePadding, BTNPAD2);
    }
}





void SceneLabels::CacheIcons()
{
    // Icons used https://materialdesignicons.com/
    // Licence https://github.com/Templarian/MaterialDesign/blob/master/LICENSE

    rc_headlight_icon = FetchIcon("car-light-high.png");
    rc_left_blinker_icon = FetchIcon("arrow-left-bold.png");
    rc_right_blinker_icon = FetchIcon("arrow-right-bold.png");
    rc_warning_light_icon = FetchIcon("hazard-lights.png");
    rc_horn_icon = FetchIcon("bugle.png");
    rc_mirror_icon = FetchIcon("mirror-rectangle.png");
    rc_repair_icon = FetchIcon("car-wrench.png");
    rc_parking_brake_icon = FetchIcon("car-brake-alert.png");
    rc_traction_control_icon = FetchIcon("car-traction-control.png");
    rc_abs_icon = FetchIcon("car-brake-abs.png");
    rc_physics_icon = FetchIcon("pause.png");
    rc_actor_physics_icon = FetchIcon("pause-circle-outline.png");
    rc_a_icon = FetchIcon("alpha-a-circle.png");
    rc_w_icon = FetchIcon("alpha-w-circle.png");
    rc_m_icon = FetchIcon("alpha-m-circle.png");
    rc_g_icon = FetchIcon("alpha-g-circle.png");
    rc_particle_icon = FetchIcon("water.png");
    rc_shift_icon = FetchIcon("car-shift-pattern.png");
    rc_engine_icon = FetchIcon("engine.png");
    rc_beacons_icon = FetchIcon("alarm-light-outline.png");
    rc_camera_icon = FetchIcon("camera-switch-outline.png");
    rc_lock_icon = FetchIcon("alpha-l-circle.png");
    rc_secure_icon = FetchIcon("lock-outline.png");
    rc_cruise_control_icon = FetchIcon("car-cruise-control.png");

    // Thanks WillM for the icons!

    rc_left_mouse_button = FetchIcon("left-mouse-button.png");
    rc_middle_mouse_button = FetchIcon("middle-mouse-button.png");
    rc_middle_mouse_scroll_button = FetchIcon("middle-mouse-scroll-button.png");
    rc_right_mouse_button = FetchIcon("right-mouse-button.png");

    m_icons_cached = true;
}