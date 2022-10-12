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
#include <fmt/format.h>

#include "Actor.h"
#include "GUIManager.h"
#include "GUIUtils.h"
#include "OgreImGui.h"
#include "Language.h"

using namespace RoR;
using namespace GUI;

void LoadingWindow::SetProgress(int percent, std::string const& text, bool render_frame/*=true*/)
{
    if (render_frame && m_timer.getMilliseconds() > 10)
    {
        App::GetGuiManager()->NewImGuiFrame(m_timer.getMilliseconds() * 0.001);
        m_timer.reset();
    }
    else
    {
        render_frame = false;
    }

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
        this->Draw();
        Ogre::Root::getSingleton().renderOneFrame();
    }

    // Echo status to log
    Str<200> msg;
    msg << "[RoR|PleaseWaitUI] ";
    if      (percent == PERC_SHOW_SPINNER)     { msg << "<spinner>"; }
    else if (percent != PERC_HIDE_PROGRESSBAR) { msg << "<" << percent << "%>"; }
    msg << " " << text;
    RoR::Log(msg);
}

void LoadingWindow::SetProgressNetConnect(const std::string& net_status)
{
    this->SetProgress(PERC_SHOW_SPINNER,
                    fmt::format( "{} [{}:{}]\n{}", _LC("LoadingWindow", "Joining"),
                                App::mp_server_host->getStr(), App::mp_server_port->getInt(), net_status));
}

void LoadingWindow::Draw()
{
    GUIManager::GuiTheme const& theme = App::GetGuiManager()->GetTheme();

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
    ImGui::Begin(_LC("LoadingWindow", "Please wait"), nullptr, flags);
    ImGui::Text("%s", m_text.c_str());
    
    if (m_percent == PERC_SHOW_SPINNER)
    {
        float spinner_size = 8.f;
        LoadingIndicatorCircle("spinner", spinner_size, theme.value_blue_text_color, theme.value_blue_text_color, 10, 10);
    }
    else if (m_percent == PERC_HIDE_PROGRESSBAR)
    {
        ImGui::NewLine();
    }
    else
    {
        ImGui::NewLine();
        ImGui::ProgressBar(static_cast<float>(m_percent)/100.f);
    }
    ImGui::End();
}
