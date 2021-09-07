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


#include "GUI_DirectionArrow.h"

#include "AppContext.h"
#include "GfxActor.h"
#include "GfxScene.h"
#include "Language.h"
#include "GUIManager.h"

#include <Overlay/OgreOverlayManager.h>
#include <fmt/core.h>

using namespace RoR;
using namespace GUI;

void GUI::DirectionArrow::LoadOverlay()
{
    // Load overlay from .overlay file
    m_overlay = Ogre::OverlayManager::getSingleton().getByName("tracks/DirectionArrow");

    // openGL fix
    m_overlay->show();
    m_overlay->hide();

    this->CreateArrow();
}

void GUI::DirectionArrow::CreateArrow()
{
    // setup direction arrow
    Ogre::Entity* arrow_entity = App::GetGfxScene()->GetSceneManager()->createEntity("arrow2.mesh");
    arrow_entity->setRenderQueueGroup(Ogre::RENDER_QUEUE_OVERLAY);

    // Add entity to the scene node
    m_node = new Ogre::SceneNode(App::GetGfxScene()->GetSceneManager());
    m_node->attachObject(arrow_entity);
    m_node->setVisible(false);
    m_node->setScale(0.1, 0.1, 0.1);
    m_node->setPosition(Ogre::Vector3(-0.6, +0.4, -1));
    m_node->setFixedYawAxis(true, Ogre::Vector3::UNIT_Y);
    m_overlay->add3D(m_node);
}

void GUI::DirectionArrow::Update(RoR::GfxActor* player_vehicle)
{
    GfxScene::SimBuffer& data = App::GetGfxScene()->GetSimDataBuffer();

    if (data.simbuf_dir_arrow_visible)
    {
        // Set visible
        m_node->setVisible(true);
        m_overlay->show();

        // Update arrow direction
        m_node->lookAt(data.simbuf_dir_arrow_target, Ogre::Node::TS_WORLD, Ogre::Vector3::UNIT_Y);

        // Update distance
        float distance = 0.0f;
        if (player_vehicle != nullptr && player_vehicle->GetSimDataBuffer().simbuf_live_local)
        {
            distance = player_vehicle->GetSimDataBuffer().simbuf_pos.distance(data.simbuf_dir_arrow_target);
        }
        else
        {
            distance = data.simbuf_character_pos.distance(data.simbuf_dir_arrow_target);
        }

        GUIManager::GuiTheme const& theme = App::GetGuiManager()->GetTheme();

        ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar;
        std::string text_a = data.simbuf_dir_arrow_text;
        std::string text_b = fmt::format("{:.1f} {}", distance, _LC("DirectionArrow", "meter")); 
        const float content_width = ImGui::CalcTextSize(text_a.c_str()).x + ImGui::CalcTextSize(text_b.c_str()).x;
        ImGui::SetNextWindowContentWidth(content_width);
        ImGui::SetNextWindowPos(ImVec2((ImGui::GetIO().DisplaySize.x/2.f) - (content_width / 2.f), theme.screen_edge_padding.y));
        ImGui::PushStyleColor(ImGuiCol_WindowBg, theme.semitransparent_window_bg);
        ImGui::Begin("Arrow", nullptr, flags);
        ImGui::Text(text_a.c_str());
        ImGui::SameLine();

        if (distance < 50)
        {
            ImGui::TextColored(theme.success_text_color,"%s", text_b.c_str());
        }
        else if (distance > 200)
        {
            ImGui::TextColored(theme.warning_text_color,"%s", text_b.c_str());
        }
        else
        {
            ImGui::TextColored(theme.value_blue_text_color,"%s", text_b.c_str());
        }

        ImGui::End();
        ImGui::PopStyleColor(1); // WindowBg
    }
    else
    {
        m_node->setVisible(false);
        m_overlay->hide();
    }
}

// Only effective in main menu, simulation visibility is set in `Update()`
void GUI::DirectionArrow::SetVisible(bool value)
{
    if (m_node && m_overlay)
    {
        m_node->setVisible(value);
        if (value)
            m_overlay->show();
        else
            m_overlay->hide();
    }
}
