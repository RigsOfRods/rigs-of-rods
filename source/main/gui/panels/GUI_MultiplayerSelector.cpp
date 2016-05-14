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
	@file   GUI_MultiplayerSelector.cpp
	@author Moncef Ben Slimane
	@date   11/2014
*/

#include "GUI_MultiplayerSelector.h"

#include "RoRPrerequisites.h"
#include "Utils.h"
#include "RoRVersion.h"
#include "rornet.h"
#include "Language.h"
#include "GUIManager.h"
#include "Application.h"

#include <MyGUI.h>

using namespace RoR;
using namespace GUI;

#define CLASS        MultiplayerSelector
#define MAIN_WIDGET  ((MyGUI::Window*)mMainWidget)

CLASS::CLASS()
{
	MyGUI::WindowPtr win = dynamic_cast<MyGUI::WindowPtr>(mMainWidget);
	win->eventWindowButtonPressed += MyGUI::newDelegate(this, &CLASS::notifyWindowButtonPressed); //The "X" button thing
	
	m_joinbutton->eventMouseButtonClick += MyGUI::newDelegate(this, &CLASS::eventMouseButtonClickJoinButton);
	m_settingsbutton->eventMouseButtonClick += MyGUI::newDelegate(this, &CLASS::eventMouseButtonClickConfigButton);

	m_ror_net_ver->setCaptionWithReplacing(RORNET_VERSION);

	CenterToScreen();

	init();

	Hide();
}

CLASS::~CLASS()
{

}

void CLASS::init()
{

}

void CLASS::Show()
{
	MAIN_WIDGET->setVisibleSmooth(true);
}

void CLASS::Hide()
{
	MAIN_WIDGET->setVisibleSmooth(false);
	Application::GetGuiManager()->ShowMainMenu(true);
}

void CLASS::CenterToScreen()
{
	MyGUI::IntSize windowSize = MAIN_WIDGET->getSize();
	MyGUI::IntSize parentSize = MAIN_WIDGET->getParentSize();

	MAIN_WIDGET->setPosition((parentSize.width - windowSize.width) / 2, (parentSize.height - windowSize.height) / 2);
}

bool CLASS::IsVisible()
{
	return MAIN_WIDGET->getVisible();
}

void CLASS::eventMouseButtonClickJoinButton(MyGUI::WidgetPtr _sender)
{

}

void CLASS::eventMouseButtonClickConfigButton(MyGUI::WidgetPtr _sender)
{

}

void CLASS::notifyWindowButtonPressed(MyGUI::WidgetPtr _sender, const std::string& _name)
{
	if (_name == "close")
		Hide();
}
