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


#include "GUI_LoadingWindow.h"

#include "Language.h"
#include <imgui.h>

void RoR::GUI::LoadingWindow::setProgress(int percent, std::string const& text, bool render_frame/*=true*/)
{
    this->SetVisible(true); // Traditional behavior
    m_percent = percent;
    m_text = text;

    // Count lines
    Ogre::StringUtil::trim(m_text); // Remove leading/trailing whitespace, incl. newlines
    m_text_num_lines = 1; // First or single line
    size_t pos = 0;
    while ((pos = m_text.find("\n", pos+1)) != std::string::npos) // Count newlines
    {
        ++m_text_num_lines; // New line
    }

    if (render_frame)
    {
        Ogre::Root::getSingleton().renderOneFrame();
    }
}

void RoR::GUI::LoadingWindow::Draw()
{
    // Height calc
    float text_h = ImGui::CalcTextSize("A").y;
    float statusbar_h = text_h + (ImGui::GetStyle().FramePadding.y * 2);
    float titlebar_h = text_h + (ImGui::GetStyle().FramePadding.y * 2);

    float height = titlebar_h +
                   ImGui::GetStyle().WindowPadding.y +
                   (text_h * (m_text_num_lines + 1)) + // + One blank line
                   (ImGui::GetStyle().ItemSpacing.y * 2) + // Blank line, statusbar
                   statusbar_h +
                   ImGui::GetStyle().WindowPadding.y;

    ImGui::SetNextWindowSize(ImVec2(500.f, height));
    ImGui::SetNextWindowPosCenter();
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove;
    ImGui::Begin(_L("Please wait"), nullptr, flags);
    ImGui::Text("%s", m_text.c_str());
    ImGui::NewLine();
    if (m_percent >= 0)
    {
        ImGui::ProgressBar(static_cast<float>(m_percent)/100.f);
    }
    ImGui::End();
}
