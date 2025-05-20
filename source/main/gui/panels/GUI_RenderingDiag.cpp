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
#include "Language.h"
#include "Terrain.h"
#include "GfxScene.h"
#include "Utils.h"

using namespace RoR;
using namespace GUI;

void RenderingDiag::Draw()
{
    ImGui::SetNextWindowPosCenter(ImGuiCond_FirstUseEver);
    ImGuiWindowFlags win_flags = ImGuiWindowFlags_NoCollapse;
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
    ImGui::Text(_LC("RenderingDiag", "Main screen"));
    ImGui::Separator();

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
    ImGui::Text(_LC("RenderingDiag", "Envmap - globals"));
    ImGui::Checkbox(_LC("RenderingDiag", "Show terrain objects"), &App::GetGfxScene()->GetEnvMap().envmap_show_terrain_objects);
    ImGui::Separator();

    ImGui::Text(_LC("RenderingDiag", "Envmap +X"));
    this->DrawEnvmapFace(0);
    ImGui::Separator();

    ImGui::Text(_LC("RenderingDiag", "Envmap -X"));
    this->DrawEnvmapFace(1);
    ImGui::Separator();

    ImGui::Text(_LC("RenderingDiag", "Envmap +Y"));
    this->DrawEnvmapFace(2);
    ImGui::Separator();

    ImGui::Text(_LC("RenderingDiag", "Envmap -Y"));
    this->DrawEnvmapFace(3);
    ImGui::Separator();

    ImGui::Text(_LC("RenderingDiag", "Envmap -Z"));
    this->DrawEnvmapFace(4);
    ImGui::Separator();

    ImGui::Text(_LC("RenderingDiag", "Envmap +Z"));
    this->DrawEnvmapFace(5);
    ImGui::Separator();
}

