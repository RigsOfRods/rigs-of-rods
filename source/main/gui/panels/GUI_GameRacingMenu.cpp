/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2013+     Petr Ohlidal & contributors

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
/// @author Moncef Ben Slimane
/// @date   11/2014

#include "GUI_GameRacingMenu.h"

#include "Application.h"
#include "GUIManager.h"
#include "GUI_MainSelector.h"

#include <MyGUI.h>
#include <utils/Language.h>

using namespace RoR;
using namespace GUI;

#define CLASS        GameRacingMenu
#define MAIN_WIDGET  ((MyGUI::Window*)mMainWidget)

CLASS::CLASS()
{
    MyGUI::WindowPtr win = dynamic_cast<MyGUI::WindowPtr>(mMainWidget);

    m_load_player->eventMouseButtonClick += MyGUI::newDelegate(this, &CLASS::eventMouseButtonClickLoadPlayerButton);
    m_save_player->eventMouseButtonClick += MyGUI::newDelegate(this, &CLASS::eventMouseButtonClickSavePlayerButton);
    m_single_race->eventMouseButtonClick += MyGUI::newDelegate(this, &CLASS::eventMouseButtonClickSingleRaceButton);
    m_championship->eventMouseButtonClick += MyGUI::newDelegate(this, &CLASS::eventMouseButtonClickChampionshipButton);
    m_main_menu->eventMouseButtonClick += MyGUI::newDelegate(this, &CLASS::eventMouseButtonClickMainMenuButton);

    m_load_player->setCaption(_L("Load Player"));
    m_save_player->setCaption(_L("Save Player"));
    m_single_race->setCaption(_L("Single Race"));
    //m_settings->setCaption(_L("Race Settings"));
    m_championship->setCaption(_L("Championship"));
    m_main_menu->setCaption(_L("Main Menu"));

    win->setCaption(_L("Racing Menu"));
    win->setMovable(false);

    MAIN_WIDGET->setVisible(false);
}

CLASS::~CLASS()
{
}

void CLASS::SetPosition(int pixels_left, int pixels_top)
{
    MAIN_WIDGET->setPosition(pixels_left, pixels_top);
}

int CLASS::GetHeight()
{
    return MAIN_WIDGET->getHeight();
}

void CLASS::Show()
{
    MAIN_WIDGET->setVisibleSmooth(true);
}

void CLASS::Hide()
{
    MAIN_WIDGET->setVisibleSmooth(false);
}

void CLASS::eventMouseButtonClickLoadPlayerButton(MyGUI::WidgetPtr _sender)
{
    this->Hide();
    // To be implemented
}

void CLASS::eventMouseButtonClickSavePlayerButton(MyGUI::WidgetPtr _sender)
{
    // To be implemented
    Hide();
}

void CLASS::eventMouseButtonClickSingleRaceButton(MyGUI::WidgetPtr _sender)
{
    // To be implemented
    Hide();
}

void CLASS::eventMouseButtonClickChampionshipButton(MyGUI::WidgetPtr _sender)
{
    Hide();
    // To be implemented
}

void CLASS::eventMouseButtonClickMainMenuButton(MyGUI::WidgetPtr _sender)
{
    App::GetGuiManager()->GameMainMenu.SetVisible(true);
    Hide();
}

void CLASS::SetVisible(bool v)
{
    MAIN_WIDGET->setVisible(v);
}

bool CLASS::IsVisible() { return MAIN_WIDGET->getVisible(); }
