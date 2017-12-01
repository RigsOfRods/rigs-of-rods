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

#include "GUI_GameMainMenu.h"

#include "Application.h"
#include "GUIManager.h"
#include "GUI_MainSelector.h"
#include "MainMenu.h"

#include <MyGUI.h>
#include <utils/Language.h>

using namespace RoR;
using namespace GUI;

#define CLASS        GameMainMenu
#define MAIN_WIDGET  ((MyGUI::Window*)mMainWidget)

CLASS::CLASS()
{
    MyGUI::WindowPtr win = dynamic_cast<MyGUI::WindowPtr>(mMainWidget);

    m_single_player->eventMouseButtonClick += MyGUI::newDelegate(this, &CLASS::eventMouseButtonClickSelectTerrainButton);
    m_settings->eventMouseButtonClick += MyGUI::newDelegate(this, &CLASS::eventMouseButtonClickSettingButton);
    m_about->eventMouseButtonClick += MyGUI::newDelegate(this, &CLASS::eventMouseButtonClickAboutButton);
    m_exit->eventMouseButtonClick += MyGUI::newDelegate(this, &CLASS::eventMouseButtonClickExitButton);
    m_multi_player->eventMouseButtonClick += MyGUI::newDelegate(this, &CLASS::eventMouseButtonClickMultiPlayerButton);

    m_single_player->setCaption(_L("Singleplayer"));
    m_settings->setCaption(_L("Settings"));
    m_about->setCaption(_L("About"));
    m_exit->setCaption(_L("Quit"));
    m_multi_player->setCaption(_L("Multiplayer"));

    win->setCaption(_L("Main Menu"));
    win->setMovable(false);

#ifndef USE_SOCKETW
    m_multi_player->setEnabled(false);
#endif

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

void CLASS::eventMouseButtonClickSelectTerrainButton(MyGUI::WidgetPtr _sender)
{
    this->Hide();
    if (App::diag_preset_terrain.IsActiveEmpty())
    {
        App::GetGuiManager()->GetMainSelector()->Show(LT_Terrain);
    }
    else
    {
        App::app_state.SetPending(RoR::AppState::SIMULATION);
    }
}

void CLASS::eventMouseButtonClickSettingButton(MyGUI::WidgetPtr _sender)
{
    App::GetGuiManager()->SetVisible_GameSettings(true);
    this->Hide();
}

void CLASS::eventMouseButtonClickAboutButton(MyGUI::WidgetPtr _sender)
{
    App::GetGuiManager()->SetVisible_GameAbout(true);
    Hide();
}

void CLASS::eventMouseButtonClickExitButton(MyGUI::WidgetPtr _sender)
{
    Hide();
    App::app_state.SetPending(RoR::AppState::SHUTDOWN);
}

void CLASS::eventMouseButtonClickMultiPlayerButton(MyGUI::WidgetPtr _sender)
{
    App::GetGuiManager()->SetVisible_MultiplayerSelector(true);
    Hide();
}

void CLASS::SetVisible(bool v)
{
    MAIN_WIDGET->setVisible(v);
}

bool CLASS::IsVisible() { return MAIN_WIDGET->getVisible(); }
