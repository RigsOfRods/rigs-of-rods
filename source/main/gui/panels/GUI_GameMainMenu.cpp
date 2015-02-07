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

#include "GUI_GameMainMenu.h"

#include "RoRPrerequisites.h"
#include "Utils.h"
#include "RoRVersion.h"
#include "rornet.h"
#include "Language.h"
#include "SelectorWindow.h"
#include "GlobalEnvironment.h"
#include "Application.h"
#include "GUIManager.h"
#include "RoRFrameListener.h"
#include "MainThread.h"
#include "Console.h"

#include <MyGUI.h>


using namespace RoR;
using namespace GUI;

#define CLASS        GameMainMenu
#define MAIN_WIDGET  ((MyGUI::Window*)mMainWidget)

CLASS::CLASS()
{
	MyGUI::WindowPtr win = dynamic_cast<MyGUI::WindowPtr>(mMainWidget);

	m_single_player->eventMouseButtonClick      += MyGUI::newDelegate(this, &CLASS::eventMouseButtonClickSelectTerrainButton);
	m_rig_editor->eventMouseButtonClick    += MyGUI::newDelegate(this, &CLASS::eventMouseButtonClickRigEditorButton);
	m_settings->eventMouseButtonClick     += MyGUI::newDelegate(this, &CLASS::eventMouseButtonClickSettingButton);
	m_about->eventMouseButtonClick        += MyGUI::newDelegate(this, &CLASS::eventMouseButtonClickAboutButton);
	m_exit->eventMouseButtonClick        += MyGUI::newDelegate(this, &CLASS::eventMouseButtonClickExitButton);
	m_multi_player->eventMouseButtonClick += MyGUI::newDelegate(this, &CLASS::eventMouseButtonClickMultiPlayerButton);
	win->setMovable(false);

	Hide();
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
	if (RoR::Application::GetConsole()->getVisible())
		RoR::Application::GetConsole()->setVisible(false);

	MAIN_WIDGET->setVisibleSmooth(true);
	//Is this really needed?
	SelectorWindow::getSingleton().show(SelectorWindow::LT_Terrain); 
	SelectorWindow::getSingleton().mMainWidget->setVisible(false);
}

void CLASS::Hide()
{
	MAIN_WIDGET->setVisibleSmooth(false);
}

void CLASS::eventMouseButtonClickSelectTerrainButton(MyGUI::WidgetPtr _sender)
{
	SelectorWindow::getSingleton().mMainWidget->setVisible(true);
	Hide();
}

void CLASS::eventMouseButtonClickSettingButton(MyGUI::WidgetPtr _sender)
{
	Application::GetGuiManager()->ShowSettingGui(true);
	Hide();
}

void CLASS::eventMouseButtonClickAboutButton(MyGUI::WidgetPtr _sender)
{
	Application::GetGuiManager()->ShowAboutGUI(true);
	Hide();
}

void CLASS::eventMouseButtonClickExitButton(MyGUI::WidgetPtr _sender)
{
	gEnv->frameListener->shutdown_final();
	//Hide();
}

void CLASS::eventMouseButtonClickMultiPlayerButton(MyGUI::WidgetPtr _sender)
{
	Application::GetGuiManager()->ShowMultiPlayerSelector(true);
	Hide();
}

void CLASS::eventMouseButtonClickRigEditorButton(MyGUI::WidgetPtr _sender)
{
	Application::GetMainThreadLogic()->SetNextApplicationState(Application::STATE_RIG_EDITOR);
	Application::GetMainThreadLogic()->RequestExitCurrentLoop();
	Hide();
}

