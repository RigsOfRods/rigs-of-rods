/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2014-2017 Petr Ohlidal & contributors

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

#include "GUI_GamePauseMenu.h"

#include "Application.h"
#include "BeamFactory.h"
#include "Character.h"
#include "GlobalEnvironment.h"
#include "Language.h"
#include "OgreSubsystem.h"
#include "RoRPrerequisites.h"

#include <MyGUI.h>
#include <OgreRenderWindow.h>
#include <OgreViewport.h>

using namespace RoR;
using namespace GUI;

#define CLASS        GamePauseMenu
#define MAIN_WIDGET  ((MyGUI::Window*)mMainWidget)

CLASS::CLASS()
{
    MyGUI::WindowPtr win = dynamic_cast<MyGUI::WindowPtr>(mMainWidget);
    win->setMovable(false);

    m_resume_game ->eventMouseButtonClick += MyGUI::newDelegate(this, &CLASS::eventMouseButtonClickResumeButton);
    m_change_map  ->eventMouseButtonClick += MyGUI::newDelegate(this, &CLASS::eventMouseButtonClickChangeMapButton);
    m_back_to_menu->eventMouseButtonClick += MyGUI::newDelegate(this, &CLASS::eventMouseButtonClickBackToMenuButton);
    m_quit_game   ->eventMouseButtonClick += MyGUI::newDelegate(this, &CLASS::eventMouseButtonClickQuitButton);

    win->setCaption(_L("Pause"));
    m_resume_game ->setCaption(_L("Resume Game"));
    m_change_map  ->setCaption(_L("Change Map"));
    m_back_to_menu->setCaption(_L("Back to menu"));
    m_quit_game   ->setCaption(_L("Quit to Desktop"));

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

    const bool online = RoR::App::mp_state.GetActive() == RoR::MpState::CONNECTED;
    m_change_map->setEnabled(!online);

    // Adjust screen position
    Ogre::Viewport* viewport = RoR::App::GetOgreSubsystem()->GetRenderWindow()->getViewport(0);
    int margin = (viewport->getActualHeight() / 15);
    int top = viewport->getActualHeight() - this->GetHeight() - margin;
    this->SetPosition(margin, top);
}

void CLASS::Hide()
{
    // The "fade out" effect causes a visual glitch.
    // During the animation, camera doesn't follow the vehicle and position
    // of the driver isn't updated (he "floats out" of the cabin)
    // Quick workaroud: use simple hiding without animation.
    //MAIN_WIDGET->setVisibleSmooth(false);
    MAIN_WIDGET->setVisible(false);
}

void CLASS::eventMouseButtonClickResumeButton(MyGUI::WidgetPtr _sender)
{
    App::sim_state.SetPending(SimState::RUNNING);
    Hide();
}

void CLASS::eventMouseButtonClickChangeMapButton(MyGUI::WidgetPtr _sender)
{
    Hide();
    App::app_state.SetPending(RoR::AppState::CHANGE_MAP);
}

void CLASS::eventMouseButtonClickBackToMenuButton(MyGUI::WidgetPtr _sender)
{
    Hide();
    App::app_state.SetPending(RoR::AppState::MAIN_MENU);
}

void CLASS::eventMouseButtonClickQuitButton(MyGUI::WidgetPtr _sender)
{
    Hide();
    App::app_state.SetPending(RoR::AppState::SHUTDOWN);
}

void CLASS::SetVisible(bool v)
{
    if (v) { this->Show(); }
    else { this->Hide(); }
}

bool CLASS::IsVisible() { return MAIN_WIDGET->getVisible(); }
