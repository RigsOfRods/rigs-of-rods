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

#include "Actor.h"
#include "AppContext.h"
#include "GUIManager.h"
#include "Language.h"
#include <iomanip>

#include <imgui.h>
#include <Ogre.h>

using namespace RoR;
using namespace GUI;

void SimPerfStats::Draw()
{
    GUIManager::GuiTheme const& theme = App::GetGuiManager()->GetTheme();

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar;
    ImGui::SetNextWindowPos(theme.screen_edge_padding);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, theme.semitransparent_window_bg);
    ImVec2 histogram_size = ImVec2(60.f, 35.f);
    ImGui::Begin("FPS", &m_is_visible, flags);

    const Ogre::RenderTarget::FrameStats& stats = App::GetAppContext()->GetRenderWindow()->getStatistics();

    std::string title = "FPS";
    ImGui::SetCursorPosX((ImGui::GetWindowSize().x - ImGui::CalcTextSize(title.c_str()).x) * 0.5f);
    ImGui::Text(title.c_str());

    std::string a = "Current";
    ImGui::SetCursorPosX((histogram_size.x + (3 * ImGui::GetStyle().ItemSpacing.x) + ImGui::GetStyle().FramePadding.x - ImGui::CalcTextSize(a.c_str()).x) * 0.5f);
    ImGui::Text("%s", _LC("SimPerfStats", a.c_str()));
    ImGui::SameLine();

    std::string b = "Average";
    ImGui::SetCursorPosX((histogram_size.x * 3 + (5 * ImGui::GetStyle().ItemSpacing.x) + ImGui::GetStyle().FramePadding.x - ImGui::CalcTextSize(b.c_str()).x) * 0.5f);
    ImGui::Text("%s", _LC("SimPerfStats", b.c_str()));
    ImGui::SameLine();

    std::string c = "Worst";
    ImGui::SetCursorPosX((histogram_size.x * 5 + (7 * ImGui::GetStyle().ItemSpacing.x) + ImGui::GetStyle().FramePadding.x - ImGui::CalcTextSize(c.c_str()).x) * 0.5f);
    ImGui::Text("%s", _LC("SimPerfStats", c.c_str()));
    ImGui::SameLine();

    std::string d = "Best";
    ImGui::SetCursorPosX((histogram_size.x * 7 + (9 * ImGui::GetStyle().ItemSpacing.x) + ImGui::GetStyle().FramePadding.x - ImGui::CalcTextSize(d.c_str()).x) * 0.5f);
    ImGui::Text("%s", _LC("SimPerfStats", d.c_str()));

    ImGui::PlotHistogram("", &stats.lastFPS, 1, 0, this->Convert(stats.lastFPS).c_str(), 0.f, stats.bestFPS, histogram_size);
    ImGui::SameLine();
    ImGui::PlotHistogram("", &stats.avgFPS, 1, 0, this->Convert(stats.avgFPS).c_str(), 0.f, stats.bestFPS, histogram_size);
    ImGui::SameLine();
    ImGui::PlotHistogram("", &stats.worstFPS, 1, 0, this->Convert(stats.worstFPS).c_str(), 0.f, stats.bestFPS, histogram_size);
    ImGui::SameLine();
    ImGui::PlotHistogram("", &stats.bestFPS, 1, 0, this->Convert(stats.bestFPS).c_str(), 0.f, stats.bestFPS, histogram_size);

    ImGui::Separator();
    ImGui::Text("%s%zu", _LC("SimPerfStats", "Triangle count: "), stats.triangleCount);
    ImGui::Text("%s%zu", _LC("SimPerfStats", "Batch count: "),    stats.batchCount);

    ImGui::End();
    ImGui::PopStyleColor(1); // WindowBg
}

std::string SimPerfStats::Convert(const float f)
{
    std::stringstream s;
    s << std::fixed << std::setprecision(2) << f;
    return s.str();
}