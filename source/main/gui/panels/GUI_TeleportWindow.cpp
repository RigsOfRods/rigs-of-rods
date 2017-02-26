/*
    This source file is part of Rigs of Rods
    Copyright 2017 Petr Ohlidal & contributors

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

#include "GUI_TeleportWindow.h"
#include "RoRFrameListener.h"
#include "Terrn2Fileformat.h"

namespace RoR {
namespace GUI {

static const char*         HELPTEXT_USAGE = "Click a telepoint."; // TODO: or hold [Alt] and click anywhere on the minimap
static const MyGUI::Colour HELPTEXT_COLOR_USAGE(1.f, 0.746025f, 0.403509f);
static const MyGUI::Colour HELPTEXT_COLOR_TELEPOINT = MyGUI::Colour::White;
static const int           TELEICON_SIZE = 24; // Pixels; make this int-divisible by 2

TeleportWindow::TeleportWindow():
    GuiPanelBase(m_teleport_window),
    m_sim_controller(nullptr)
{
    m_teleport_window->eventWindowButtonPressed += MyGUI::newDelegate(this, &TeleportWindow::WindowButtonClicked);

    // Center on screen
    MyGUI::IntSize windowSize = m_teleport_window->getSize();
    MyGUI::IntSize parentSize = m_teleport_window->getParentSize();
    m_teleport_window->setPosition((parentSize.width - windowSize.width) / 2, (parentSize.height - windowSize.height) / 2);

    // Start hidden
    m_teleport_window->setVisible(false);
}

void TeleportWindow::SetVisible(bool v)
{
    m_teleport_window->setVisible(v);
}

bool TeleportWindow::IsVisible()
{
    return m_teleport_window->getVisible();
}

void TeleportWindow::WindowButtonClicked(MyGUI::Widget* sender, const std::string& name)
{
    this->SetVisible(false); // There's only the [X] close button on the window.
}

Terrn2Telepoint* GetTeleIconUserdata(MyGUI::Widget* widget)
{
    Terrn2Telepoint** userdata = widget->getUserData<Terrn2Telepoint*>(false);
    assert(userdata != nullptr);
    return *userdata;
}

void TeleportWindow::TelepointIconGotFocus(MyGUI::Widget* cur_widget, MyGUI::Widget* prev_widget)
{
    m_info_textbox->setCaption(GetTeleIconUserdata(cur_widget)->name);
    m_info_textbox->setTextColour(HELPTEXT_COLOR_TELEPOINT);
    static_cast<MyGUI::ImageBox*>(cur_widget)->setImageTexture("telepoint_icon_hover.png");
}

void TeleportWindow::TelepointIconLostFocus(MyGUI::Widget* cur_widget, MyGUI::Widget* new_widget)
{
    m_info_textbox->setCaption(HELPTEXT_USAGE);
    m_info_textbox->setTextColour(HELPTEXT_COLOR_USAGE);
    static_cast<MyGUI::ImageBox*>(cur_widget)->setImageTexture("telepoint_icon.png");
}

void TeleportWindow::TelepointIconClicked(MyGUI::Widget* sender)
{
    this->SetVisible(false);
    static_cast<MyGUI::ImageBox*>(sender)->setImageTexture("telepoint_icon.png");
    m_sim_controller->TeleportPlayer(GetTeleIconUserdata(sender));
}

void TeleportWindow::SetupMap(RoRFrameListener* sim_controller, Terrn2Def* def, Ogre::Vector3 map_size, std::string minimap_tex_name)
{
    m_sim_controller = sim_controller;
    m_minimap_image->setImageTexture(minimap_tex_name);
    m_info_textbox->setCaption(HELPTEXT_USAGE);
    m_info_textbox->setTextColour(HELPTEXT_COLOR_USAGE);
    float mini_width  = static_cast<float>(m_minimap_image->getSize().width);
    float mini_height = static_cast<float>(m_minimap_image->getSize().height);
    for (Terrn2Telepoint& telepoint: def->telepoints)
    {
        int pos_x = static_cast<int>(((telepoint.position.x / map_size.x) * mini_width ) - (TELEICON_SIZE/2));
        int pos_y = static_cast<int>(((telepoint.position.z / map_size.z) * mini_height) - (TELEICON_SIZE/2));
        MyGUI::ImageBox* tp_icon = m_minimap_image->createWidget<MyGUI::ImageBox>("ImageBox", pos_x, pos_y, TELEICON_SIZE, TELEICON_SIZE, MyGUI::Align::Default);
        tp_icon->setImageTexture("telepoint_icon.png");
        tp_icon->setUserData(&telepoint);
        tp_icon->eventMouseSetFocus += MyGUI::newDelegate(this, &TeleportWindow::TelepointIconGotFocus);
        tp_icon->eventMouseLostFocus += MyGUI::newDelegate(this, &TeleportWindow::TelepointIconLostFocus);
        tp_icon->eventMouseButtonClick += MyGUI::newDelegate(this, &TeleportWindow::TelepointIconClicked);
        m_telepoint_icons.push_back(tp_icon);
    }
}

void TeleportWindow::Reset()
{
    m_sim_controller = nullptr;
    while (!m_telepoint_icons.empty())
    {
        m_minimap_image->_destroyChildWidget(m_telepoint_icons.back());
        m_telepoint_icons.pop_back();
    }
}

} // namespace GUI
} // namespace RoR

