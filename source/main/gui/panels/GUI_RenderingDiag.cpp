/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer

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
/// @author Thomas Fischer (thomas{AT}thomasfischer{DOT}biz)
/// @date   7th of September 2009

// Remade using DearIMGUI by Petr Ohlidal, 10/2019


#include "GUI_RenderingDiag.h"

#include "Application.h"
#include "SimData.h"
#include "Collisions.h"
#include "GameContext.h"
#include "GUIManager.h"
#include "GUIUtils.h"
#include "Language.h"
#include "Terrain.h"
#include "GfxScene.h"
#include "Utils.h"

using namespace RoR;
using namespace GUI;

void RenderingDiag::Draw()
{
    ImGui::SetNextWindowPosCenter(ImGuiCond_FirstUseEver);
    ImGuiWindowFlags win_flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize;
    bool keep_open = true;
    ImGui::Begin(_LC("RenderingDiag", "Rendering Diagnostics"), &keep_open, win_flags);
    
    if (ImGui::BeginTabBar("RenderingDiagTabs", 0))
    {
        if (ImGui::BeginTabItem(_LC("RenderingDiag", "Main screen")))
        {
            this->DrawMainScreenTab();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem(_LC("RenderingDiag", "Envmap")))
        {
            this->DrawEnvmapTab();
            ImGui::EndTabItem();
        }        
        ImGui::EndTabBar();
    }

    // Common window epilogue
    m_is_hovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows);
    App::GetGuiManager()->RequestGuiCaptureKeyboard(m_is_hovered);

    ImGui::End();

    if (!keep_open)
    {
        this->SetVisible(false);
    }
}

void RenderingDiag::DrawMainScreenTab()
{
    ImGui::TextDisabled(_LC("RenderingDiag", ":: O P T I O N S ::"));

    float nearclip = App::GetCameraManager()->GetCamera()->getNearClipDistance();
    if (ImGui::InputFloat(_LC("RenderingDiag", "Camera near clip"), &nearclip, 0.01f, 2.f, "%.2f"))
    {
        App::GetCameraManager()->GetCamera()->setNearClipDistance(nearclip);
    }

    float farclip = App::GetCameraManager()->GetCamera()->getFarClipDistance();
    if (ImGui::InputFloat(_LC("RenderingDiag", "Camera far clip"), &farclip, 10.f, 100000.0f, "%.2f"))
    {
        App::GetCameraManager()->GetCamera()->setFarClipDistance(farclip);
    }

    ImGui::Separator();
    ImGui::TextDisabled(_LC("RenderingDiag", ":: S T A T S ::"));

    const Ogre::RenderTarget::FrameStats& stats = App::GetAppContext()->GetRenderWindow()->getStatistics();
    ImGui::Text("%s", fmt::format("FPS: {:5.1f}, Batch: {:5}, Tri: {:6}", stats.lastFPS, stats.batchCount, stats.triangleCount).c_str());
}

void RenderingDiag::DrawEnvmapFace(int face)
{
    Ogre::RenderTarget* render_target = App::GetGfxScene()->GetEnvMap().GetRenderTarget(face);
    if (render_target)
    {
        const Ogre::RenderTarget::FrameStats& stats = render_target->getStatistics();
        ImGui::Text("%s", fmt::format("FPS: {:5.1f}, Batch: {:5}, Tri: {:6}", stats.lastFPS, stats.batchCount, stats.triangleCount).c_str());
    }
    else
    {
        ImGui::TextDisabled(_LC("RenderingDiag", "No render target"));
    }
}

void RenderingDiag::DrawEnvmapTab()
{
    ImGui::TextDisabled(_LC("RenderingDiag", ":: O P T I O N S ::"));

    ImGui::Checkbox(_LC("RenderingDiag", "Show terrain objects"), &App::GetGfxScene()->GetEnvMap().envmap_show_terrain_objects);
    ImGui::InputFloat(_LC("RenderingDiag", "Camera near clip"), &App::GetGfxScene()->GetEnvMap().envmap_camera_nearclip_distance, 0.01f, 2.f, "%.2f");
    ImGui::InputFloat(_LC("RenderingDiag", "Camera far clip"), &App::GetGfxScene()->GetEnvMap().envmap_camera_farclip_distance, 10.f, 100000.0f, "%.2f");
    DrawGCheckbox(App::diag_envmap, _LC("RenderingDiag", "Show cubemap overlay"));
    ImGui::InputFloat(_LC("RenderingDiag", "Overlay pos X"), &App::GetGfxScene()->GetEnvMap().evmap_diag_overlay_pos_x, -10.f, 10.f, "%.2f");
    ImGui::InputFloat(_LC("RenderingDiag", "Overlay pos Y"), &App::GetGfxScene()->GetEnvMap().evmap_diag_overlay_pos_y, -10.f, 10.f, "%.2f");
    ImGui::InputFloat(_LC("RenderingDiag", "Overlay scale"), &App::GetGfxScene()->GetEnvMap().evmap_diag_overlay_scale, 0.1f, 10.f, "%.2f");
    ImGui::Separator();

    ImGui::TextDisabled(_LC("RenderingDiag", ":: S T A T S ::"));

    ImGui::TextDisabled(_LC("RenderingDiag", "Envmap +X"));
    ImGui::SameLine();
    this->DrawEnvmapFace(0);

    ImGui::TextDisabled(_LC("RenderingDiag", "Envmap -X"));
    ImGui::SameLine();
    this->DrawEnvmapFace(1);

    ImGui::TextDisabled(_LC("RenderingDiag", "Envmap +Y"));
    ImGui::SameLine();
    this->DrawEnvmapFace(2);

    ImGui::TextDisabled(_LC("RenderingDiag", "Envmap -Y"));
    ImGui::SameLine();
    this->DrawEnvmapFace(3);

    ImGui::TextDisabled(_LC("RenderingDiag", "Envmap -Z"));
    ImGui::SameLine();
    this->DrawEnvmapFace(4);

    ImGui::TextDisabled(_LC("RenderingDiag", "Envmap +Z"));
    ImGui::SameLine();
    this->DrawEnvmapFace(5);
}

