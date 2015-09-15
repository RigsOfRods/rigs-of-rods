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
	@file   GUI_GameChatBox.cpp
	@author Moncef Ben Slimane
	@date   2/2015
*/

#include "GUI_GameChatBox.h"

#include <OgreRenderTarget.h>
#include <OgreRenderWindow.h>
#include <OgreRoot.h>

#include "RoRPrerequisites.h"

#include "ChatSystem.h"
#include "Network.h"
#include "Utils.h"
#include "rornet.h"
#include "Language.h"
#include "GUIManager.h"
#include "Application.h"
#include "OgreSubsystem.h"

using namespace RoR;
using namespace GUI;

#define CLASS        GameChatBox
#define MAIN_WIDGET  ((MyGUI::Window*)mMainWidget)

CLASS::CLASS():
 netChat(0)
{
	MyGUI::WindowPtr win = dynamic_cast<MyGUI::WindowPtr>(mMainWidget);
	MyGUI::Gui::getInstance().eventFrameStart += MyGUI::newDelegate(this, &CLASS::Update);

	alpha = 1.0f;
	isTyping = false;
	newMsg = false;

	/* Adjust menu position */
	Ogre::Viewport* viewport = RoR::Application::GetOgreSubsystem()->GetRenderWindow()->getViewport(0);
	int margin = (viewport->getActualHeight() / 6);
	MAIN_WIDGET->setPosition(
		2, // left
		viewport->getActualHeight() - MAIN_WIDGET->getHeight() - margin // top
		);

	m_Chatbox_TextBox->eventEditSelectAccept += MyGUI::newDelegate(this, &CLASS::eventCommandAccept);
	autoHide = BSETTING("ChatAutoHide", true);

	if (!autoHide)
		Show();


	Hide();
}

CLASS::~CLASS()
{

}

void CLASS::setNetChat(ChatSystem *c)
{
	netChat = c;
}

void CLASS::Show()
{
	if (!MAIN_WIDGET->getVisible())
		MAIN_WIDGET->setVisibleSmooth(true);

	isTyping = true;
	m_Chatbox_TextBox->setEnabled(true);
	MyGUI::InputManager::getInstance().setKeyFocusWidget(m_Chatbox_TextBox);
}

void CLASS::Hide()
{
	MAIN_WIDGET->setVisibleSmooth(false);
}

bool CLASS::IsVisible()
{
	return MAIN_WIDGET->isVisible();
}

void CLASS::pushMsg(Ogre::String txt)
{
	mHistory += txt + " \n";
	newMsg = true;
	pushTime = Ogre::Root::getSingleton().getTimer()->getMilliseconds();
	m_Chatbox_MainBox->setCaptionWithReplacing(mHistory);
}

void CLASS::eventCommandAccept(MyGUI::Edit* _sender)
{
	Ogre::UTFString msg = convertFromMyGUIString(_sender->getCaption());
	isTyping = false;
	_sender->setCaption("");

	if (autoHide)
		_sender->setEnabled(false);

	if (msg.empty())
	{
		// discard the empty message
		return;
	}

	if (msg[0] == '/' || msg[0] == '\\')
	{
		Ogre::StringVector args = Ogre::StringUtil::split(msg, " ");
		if (args[0] == "/whisper")
		{
			if (args.size() != 3)
			{
				pushMsg("usage: /whisper username message");
				return;
			}
			netChat->sendPrivateChat(args[1], args[2]);
			return;
		}
	}

	if (gEnv->network && netChat)
	{
		netChat->sendChat(msg.c_str());
		return;
	}

	//MyGUI::InputManager::getInstance().resetKeyFocusWidget();
	RoR::Application::GetGuiManager()->UnfocusGui();
}

void CLASS::Update(float dt)
{
	if (autoHide)
	{
		unsigned long ot = Ogre::Root::getSingleton().getTimer()->getMilliseconds();
		if (newMsg || !isTyping)
		{
			if (!MAIN_WIDGET->getVisible())
				MAIN_WIDGET->setVisible(true);

			unsigned long endTime = pushTime + 5000;
			unsigned long startTime = endTime - (long)1000.0f;
			if (ot < startTime)
			{
				alpha = 1.0f;
			}
			else
			{
				alpha = 1 - ((ot - startTime) / 1000.0f);
			}

			MAIN_WIDGET->setAlpha(alpha);

			if (alpha <= 0.1)
			{
				newMsg = false;
				MAIN_WIDGET->setVisible(false);
			}
		}
		else if (isTyping)
			MAIN_WIDGET->setAlpha(1);

	} else if (!MAIN_WIDGET->getVisible())
		MAIN_WIDGET->setVisible(true);
}
