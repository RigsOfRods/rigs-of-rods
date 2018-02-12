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

#include "Application.h"
#include "GUIManager.h"
#include "RoRFrameListener.h"
#include "Terrn2Fileformat.h"

#include <MyGUI_PointerManager.h>

namespace RoR {
namespace GUI {

static const char*         HELPTEXT_USAGE           = "Click a telepoint or hold [Alt] and click anywhere on the minimap";
static const MyGUI::Colour HELPTEXT_COLOR_USAGE     = MyGUI::Colour(1.f, 0.746025f, 0.403509f);
static const MyGUI::Colour HELPTEXT_COLOR_TELEPOINT = MyGUI::Colour::White;
static const int           TELEICON_SIZE            = 24; // Pixels; make this int-divisible by 2
static const int           PERSON_ICON_WIDTH        = 20;
static const int           PERSON_ICON_HEIGHT       = 30;
static const int           MOUSEICON_SIZE           = 14;

TeleportWindow::TeleportWindow():
    GuiPanelBase(m_teleport_window),
    m_is_altmode_active(false),
    m_minimap_has_focus(false)
{
    m_teleport_window->eventWindowButtonPressed += MyGUI::newDelegate(this, &TeleportWindow::WindowButtonClicked);

    m_minimap_panel->eventChangeCoord += MyGUI::newDelegate(this, &TeleportWindow::MinimapPanelResized);

    m_minimap_image->eventMouseSetFocus    += MyGUI::newDelegate(this, &TeleportWindow::MinimapGotFocus);
    m_minimap_image->eventMouseLostFocus   += MyGUI::newDelegate(this, &TeleportWindow::MinimapLostFocus);
    m_minimap_image->eventMouseMove        += MyGUI::newDelegate(this, &TeleportWindow::MinimapMouseMoved);
    m_minimap_image->eventMouseButtonClick += MyGUI::newDelegate(this, &TeleportWindow::MinimapMouseClick);

    // Center on screen
    MyGUI::IntSize windowSize = m_teleport_window->getSize();
    MyGUI::IntSize parentSize = m_teleport_window->getParentSize();
    m_teleport_window->setPosition((parentSize.width - windowSize.width) / 2, (parentSize.height - windowSize.height) / 2);

    // Create person icon
    m_person_icon = m_minimap_image->createWidget<MyGUI::ImageBox>(
        "ImageBox", 0, 0, PERSON_ICON_WIDTH, PERSON_ICON_HEIGHT, MyGUI::Align::Default, "Teleport/PersonIcon");
    m_person_icon->setImageTexture("teleport_person_icon.png");
    m_person_icon->setEnabled(false); // Don't generate/receive events

    // Create mouse icon
    m_mouse_icon = m_minimap_image->createWidget<MyGUI::ImageBox>(
        "ImageBox", 0, 0, MOUSEICON_SIZE, MOUSEICON_SIZE, MyGUI::Align::Default, "Teleport/AltMouseIcon");
    m_mouse_icon->setImageTexture("teleport_mouse_point.png");
    m_mouse_icon->setEnabled(false); // Don't generate/receive events
    m_mouse_icon->setVisible(false);

    // Start hidden
    m_teleport_window->setVisible(false);
}

void TeleportWindow::SetVisible(bool v)
{
    if (m_teleport_window->isVisible() == v)
        return; // Nothing to do

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
    App::GetSimController()->TeleportPlayer(GetTeleIconUserdata(sender));
}

void TeleportWindow::MinimapGotFocus(MyGUI::Widget* widget, MyGUI::Widget* previous)
{
    if (m_is_altmode_active)
    {
        this->ShowAltmodeCursor(true);
    }
    m_minimap_has_focus = true;
}

void TeleportWindow::MinimapLostFocus(MyGUI::Widget* widget, MyGUI::Widget* next)
{
    this->ShowAltmodeCursor(false);
    m_minimap_has_focus = false;
}

void TeleportWindow::MinimapMouseMoved(MyGUI::Widget* minimap, int left, int top)
{
    m_minimap_last_mouse.left = left;
    m_minimap_last_mouse.top = top;
    if (m_mouse_icon->isVisible())
    {
        this->SetAltmodeCursorPos(left, top);
    }
}

void TeleportWindow::MinimapMouseClick(MyGUI::Widget* minimap)
{
    if (m_is_altmode_active)
    {
        this->SetVisible(false);
        MyGUI::IntPoint mini_mouse = m_minimap_last_mouse - m_minimap_image->getAbsolutePosition();
        float map_x = (static_cast<float>(mini_mouse.left)/static_cast<float>(m_minimap_image->getSize().width )) * m_map_size.x;
        float map_z = (static_cast<float>(mini_mouse.top) /static_cast<float>(m_minimap_image->getSize().height)) * m_map_size.z;
        App::GetSimController()->TeleportPlayerXZ(map_x, map_z);
    }
}

void TeleportWindow::SetAltmodeCursorPos(int screen_left, int screen_top)
{
    MyGUI::IntPoint minimap_abs = m_minimap_image->getAbsolutePosition();
    const int half = MOUSEICON_SIZE/2;
    m_mouse_icon->setPosition(screen_left - minimap_abs.left - half, screen_top - minimap_abs.top - half);
}

void TeleportWindow::SetupMap(Terrn2Def* def, Ogre::Vector3 map_size, std::string minimap_tex_name)
{
    m_map_size = map_size;
    if (!def->teleport_map_image.empty())
        m_minimap_image->setImageTexture(def->teleport_map_image);
    else
        m_minimap_image->setImageTexture(minimap_tex_name);
    m_info_textbox->setCaption(HELPTEXT_USAGE);
    m_info_textbox->setTextColour(HELPTEXT_COLOR_USAGE);

    size_t counter = 0;
    for (Terrn2Telepoint& telepoint: def->telepoints)
    {
        char name[50];
        sprintf(name, "Teleport/TelepointIcon_%u", ++counter);
        MyGUI::ImageBox* tp_icon = m_minimap_image->createWidget<MyGUI::ImageBox>(
            "ImageBox", 0, 0, TELEICON_SIZE, TELEICON_SIZE, MyGUI::Align::Default, name);
        tp_icon->setImageTexture("telepoint_icon.png");
        tp_icon->setUserData(&telepoint);
        tp_icon->eventMouseSetFocus += MyGUI::newDelegate(this, &TeleportWindow::TelepointIconGotFocus);
        tp_icon->eventMouseLostFocus += MyGUI::newDelegate(this, &TeleportWindow::TelepointIconLostFocus);
        tp_icon->eventMouseButtonClick += MyGUI::newDelegate(this, &TeleportWindow::TelepointIconClicked);
        m_telepoint_icons.push_back(tp_icon);
    }

    this->MinimapPanelResized(m_minimap_panel); // Update icon positions
}

void TeleportWindow::Reset()
{
    while (!m_telepoint_icons.empty())
    {
        m_minimap_image->_destroyChildWidget(m_telepoint_icons.back());
        m_telepoint_icons.pop_back();
    }
}

void TeleportWindow::MinimapPanelResized(MyGUI::Widget* panel)
{
    float panel_width  = static_cast<float>(panel->getWidth());
    float panel_height = static_cast<float>(panel->getHeight());
    // Fit the minimap into panel, preserving aspect ratio
    float mini_width = panel_width;
    float mini_height = (mini_width / m_map_size.x) * m_map_size.z;
    if (mini_height >= panel_height)
    {
        mini_height = panel_height;
        mini_width = (mini_height / m_map_size.z) * m_map_size.x;
    }

    // Update minimap widget
    MyGUI::IntCoord coord;
    coord.left   = static_cast<int>((panel_width / 2.f) - (mini_width / 2));
    coord.top    = static_cast<int>((panel_height / 2.f) - (mini_height / 2));
    coord.width  = static_cast<int>(mini_width);
    coord.height = static_cast<int>(mini_height);
    m_minimap_image->setCoord(coord);

    // Update icon positions
    for (MyGUI::ImageBox* tp_icon: m_telepoint_icons)
    {
        Terrn2Telepoint* def = GetTeleIconUserdata(tp_icon);
        int pos_x = static_cast<int>(((def->position.x / m_map_size.x) * mini_width ) - (TELEICON_SIZE/2));
        int pos_y = static_cast<int>(((def->position.z / m_map_size.z) * mini_height) - (TELEICON_SIZE/2));
        tp_icon->setPosition(pos_x, pos_y);
    }
}

void TeleportWindow::TeleportWindowFrameStep(float player_x, float player_z, bool alt_active)
{
    if (m_is_altmode_active != alt_active)
    {
        this->EnableAltMode(alt_active);
    }

    int icon_x = static_cast<int>(((player_x / m_map_size.x) * m_minimap_image->getWidth() ) - (PERSON_ICON_WIDTH /2));
    int icon_y = static_cast<int>(((player_z / m_map_size.z) * m_minimap_image->getHeight()) - (PERSON_ICON_HEIGHT/2));
    m_person_icon->setPosition(icon_x, icon_y);
}

void TeleportWindow::EnableAltMode(bool enable)
{
    m_is_altmode_active = enable;
    this->ShowAltmodeCursor(enable && m_minimap_has_focus);
    for (MyGUI::ImageBox* img: m_telepoint_icons)
    {
        img->setVisible(!enable);
    }
}

void TeleportWindow::ShowAltmodeCursor(bool show)
{
    m_mouse_icon->setVisible(show);
    this->SetAltmodeCursorPos(m_minimap_last_mouse.left, m_minimap_last_mouse.top);

    // Deal with mouse cursor
    if (show)
        App::GetGuiManager()->SetMouseCursorVisibility(GUIManager::MouseCursorVisibility::SUPRESSED);
    else
        App::GetGuiManager()->SetMouseCursorVisibility(GUIManager::MouseCursorVisibility::VISIBLE);
}

} // namespace GUI
} // namespace RoR

