/*
	This source file is part of Rigs of Rods
	Copyright 2005-2012 Pierre-Michel Ricordel
	Copyright 2007-2012 Thomas Fischer
	Copyright 2013-2014 Petr Ohlidal

	For more information, see http://www.rigsofrods.com/

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
	sCameraMode = gEnv->cameraManager->getCurrentBehavior();

	if (gEnv->player->getVisible() && !gEnv->player->getBeamCoupling())
		gEnv->player->setPhysicsEnabled(false);
	else
		gEnv->cameraManager->switchBehavior(1);

	gEnv->frameListener->setSimPaused(true);
	BeamFactory::getSingleton().MuteAllTrucks();

	if (gEnv->network)
	{
		m_back_to_menu->setEnabled(false);
		m_rig_editor->setEnabled(false);
		m_change_map->setEnabled(false);
	}
}

void CLASS::Hide()
{
	MAIN_WIDGET->setVisibleSmooth(false);

	if (gEnv->player->getVisible() && !gEnv->player->getBeamCoupling())
		gEnv->player->setPhysicsEnabled(true);
	else
		gEnv->cameraManager->switchBehavior(sCameraMode);

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
	Application::GetMainThreadLogic()->BackToMenu();
}

void CLASS::eventMouseButtonClickBackToMenuButton(MyGUI::WidgetPtr _sender)
{
	Hide();
	Application::GetMainThreadLogic()->BackToMenu();
}

void CLASS::eventMouseButtonClickRigEditorButton(MyGUI::WidgetPtr _sender)
{
	Hide();
	Application::GetMainThreadLogic()->SetNextApplicationState(Application::STATE_RIG_EDITOR);
	Application::GetMainThreadLogic()->RequestExitCurrentLoop();
}

void CLASS::eventMouseButtonClickQuitButton(MyGUI::WidgetPtr _sender)
{
	Hide();
	Application::GetMainThreadLogic()->RequestShutdown();
	Application::GetMainThreadLogic()->RequestExitCurrentLoop();
}