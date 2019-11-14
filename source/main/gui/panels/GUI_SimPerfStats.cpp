/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2013-2019 Petr Ohlidal

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

#include "GUI_SimPerfStats.h"

#include "Application.h"
#include "GUIManager.h"
#include "Language.h"
#include "OgreSubsystem.h"

#include <imgui.h>
#include <Ogre.h>

using namespace RoR;
using namespace GUI;

void SimPerfStats::Draw()
{
    GUIManager::GuiTheme const& theme = App::GetGuiManager()->GetTheme();

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar;
    ImGui::SetNextWindowPos(theme.screen_edge_padding);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, theme.semitransparent_window_bg);
    ImGui::Begin("FPS", &m_is_visible, flags);

    const Ogre::RenderTarget::FrameStats& stats = App::GetOgreSubsystem()->GetRenderWindow()->getStatistics();
    ImGui::Text("%s%.2f", _L("Current FPS: "), stats.lastFPS);
    ImGui::Text("%s%.2f", _L("Average FPS: "), stats.avgFPS);
    ImGui::Text("%s%.2f", _L("Worst FPS: "),   stats.worstFPS);
    ImGui::Text("%s%.2f", _L("Best FPS: "),    stats.bestFPS);
    ImGui::Separator();
    ImGui::Text("%s%zu", _L("Triangle count: "), stats.triangleCount);
    ImGui::Text("%s%zu", _L("Batch count: "),    stats.batchCount);

    ImGui::End();
    ImGui::PopStyleColor(1); // WindowBg
}
