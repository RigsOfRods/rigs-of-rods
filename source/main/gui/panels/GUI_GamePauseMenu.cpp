/*
	This source file is part of Rigs of Rods
	Copyright 2005-2012 Pierre-Michel Ricordel
	Copyright 2007-2012 Thomas Fischer
	Copyright 2013-2014 Petr Ohlidal

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

/** 
	@file   
	@author Moncef Ben Slimane
	@date   11/2014
*/

#include "GUI_GamePauseMenu.h"

#include "RoRPrerequisites.h"
#include "Utils.h"
#include "Language.h"
#include "GlobalEnvironment.h"
#include "Application.h"
#include "GUIManager.h"
#include "RoRFrameListener.h"
#include "MainThread.h"
#include "Character.h"
#include "CameraManager.h"
#include "BeamFactory.h"

#include <MyGUI.h>


using namespace RoR;
using namespace GUI;

#define CLASS        GamePauseMenu
#define MAIN_WIDGET  ((MyGUI::Window*)mMainWidget)

CLASS::CLASS()
{
	MyGUI::WindowPtr win = dynamic_cast<MyGUI::WindowPtr>(mMainWidget);
	win->setMovable(false);

	m_resume_game->eventMouseButtonClick += MyGUI::newDelegate(this, &CLASS::eventMouseButtonClickResumeButton);
	m_change_map->eventMouseButtonClick += MyGUI::newDelegate(this, &CLASS::eventMouseButtonClickChangeMapButton);
	m_back_to_menu->eventMouseButtonClick += MyGUI::newDelegate(this, &CLASS::eventMouseButtonClickBackToMenuButton);
	m_rig_editor->eventMouseButtonClick += MyGUI::newDelegate(this, &CLASS::eventMouseButtonClickRigEditorButton);
	m_quit_game->eventMouseButtonClick += MyGUI::newDelegate(this, &CLASS::eventMouseButtonClickQuitButton);

	win->setCaption(_L("Pause"));
	m_resume_game->setCaption(_L("Resume Game"));
	m_change_map->setCaption(_L("Change Map"));
	m_back_to_menu->setCaption(_L("Back to menu"));
	m_rig_editor->setCaption(_L("Rig Editor"));
	m_quit_game->setCaption(_L("Quit to Desktop"));
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

	if (gEnv->player->getVisible() && !gEnv->player->getBeamCoupling())
	{
		gEnv->player->setPhysicsEnabled(false);
	}

	gEnv->frameListener->setSimPaused(true);
	BeamFactory::getSingleton().MuteAllTrucks();

	m_rig_editor->setEnabled(false);

	if (gEnv->multiplayer_state == Global::MP_STATE_CONNECTED)
	{
		m_back_to_menu->setEnabled(false);
		m_change_map->setEnabled(false);
	}
}

void CLASS::Hide()
{
	// The "fade out" effect causes a visual glitch.
	// During the animation, camera doesn't follow the vehicle and position
	// of the driver isn't updated (he "floats out" of the cabin)
	// Quick workaroud: use simple hiding without animation.
	//MAIN_WIDGET->setVisibleSmooth(false);
	MAIN_WIDGET->setVisible(false);

	if (gEnv->player->getVisible() && !gEnv->player->getBeamCoupling())
	{
		gEnv->player->setPhysicsEnabled(true);
	}

	gEnv->frameListener->setSimPaused(false);
	BeamFactory::getSingleton().UnmuteAllTrucks();
}

void CLASS::eventMouseButtonClickResumeButton(MyGUI::WidgetPtr _sender)
{
	Hide();
}

void CLASS::eventMouseButtonClickChangeMapButton(MyGUI::WidgetPtr _sender)
{
	//TODO: FIXME
	Hide();
	Application::GetMainThreadLogic()->ChangeMap();
}

void CLASS::eventMouseButtonClickBackToMenuButton(MyGUI::WidgetPtr _sender)
{
	Hide();
	Application::GetMainThreadLogic()->BackToMenu();
}

void CLASS::eventMouseButtonClickRigEditorButton(MyGUI::WidgetPtr _sender)
{
}

void CLASS::eventMouseButtonClickQuitButton(MyGUI::WidgetPtr _sender)
{
	Hide();
	Application::GetMainThreadLogic()->RequestShutdown();
	Application::GetMainThreadLogic()->RequestExitCurrentLoop();
}
