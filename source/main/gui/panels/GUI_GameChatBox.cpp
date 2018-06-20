/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2013-2017 Petr Ohlidal & contributors

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
/// @date   2/2015

#include "GUI_GameChatBox.h"

#include <OgreRenderTarget.h>
#include <OgreRenderWindow.h>
#include <OgreViewport.h>
#include <OgreRoot.h>

#include "RoRPrerequisites.h"

#include "ChatSystem.h"
#include "Utils.h"
#include "Language.h"
#include "GUIManager.h"
#include "Application.h"
#include "OgreSubsystem.h"
#include "Settings.h"

using namespace RoR;
using namespace GUI;

#define CLASS        GameChatBox
#define MAIN_WIDGET  ((MyGUI::Window*)mMainWidget)

CLASS::CLASS() :
    alpha(1.0f)
    , newMsg(false)
{
    MyGUI::Gui::getInstance().eventFrameStart += MyGUI::newDelegate(this, &CLASS::Update);

    /* Adjust menu position */
    Ogre::Viewport* viewport = RoR::App::GetOgreSubsystem()->GetRenderWindow()->getViewport(0);
    int margin = (viewport->getActualHeight() / 6);
    MAIN_WIDGET->setPosition(
        2, // left
        viewport->getActualHeight() - MAIN_WIDGET->getHeight() - margin // top
    );

    m_Chatbox_TextBox->eventEditSelectAccept += MyGUI::newDelegate(this, &CLASS::eventCommandAccept);
    autoHide = BSETTING("ChatAutoHide", true);
    MAIN_WIDGET->setVisible(!autoHide);
}

CLASS::~CLASS()
{
}

void CLASS::Show()
{
    MAIN_WIDGET->setVisible(true);
    m_Chatbox_TextBox->setEnabled(true);
    MyGUI::InputManager::getInstance().setKeyFocusWidget(m_Chatbox_TextBox);
}

void CLASS::Hide()
{
    MAIN_WIDGET->setVisible(false);
}

bool CLASS::IsVisible()
{
    return MAIN_WIDGET->isVisible();
}

void CLASS::pushMsg(Ogre::String txt)
{
    mHistory += RoR::Color::NormalColour + txt + " \n";
    newMsg = true;
    m_Chatbox_MainBox->setCaptionWithReplacing(mHistory);
}

void CLASS::eventCommandAccept(MyGUI::Edit* _sender)
{
    Ogre::UTFString msg = convertFromMyGUIString(_sender->getCaption());
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
                Ogre::UTFString trmsg = _L("usage: /whisper username message");
                pushMsg(trmsg);
                return;
            }
            RoR::ChatSystem::SendPrivateChat(args[1], args[2]);
            return;
        }
    }

#ifdef USE_SOCKETW
    if (RoR::App::mp_state.GetActive() == RoR::MpState::CONNECTED)
    {
        RoR::ChatSystem::SendChat(msg.c_str());
        return;
    }
#endif // USE_SOCKETW

    //MyGUI::InputManager::getInstance().resetKeyFocusWidget();
    RoR::App::GetGuiManager()->UnfocusGui();
}

void CLASS::Update(float dt)
{
    if (App::mp_state.GetActive() != MpState::CONNECTED)
    {
        MAIN_WIDGET->setVisible(false);
        return;
    }
    else if (!autoHide)
    {
        MAIN_WIDGET->setVisible(true);
        return;
    }

    if (newMsg)
    {
        newMsg = false;
        pushTime = Ogre::Root::getSingleton().getTimer()->getMilliseconds();
        MAIN_WIDGET->setAlpha(1);
        MAIN_WIDGET->setVisible(true);
        return;
    }

    if (!MyGUI::InputManager::getInstance().isFocusKey())
    {
        unsigned long ot = Ogre::Root::getSingleton().getTimer()->getMilliseconds();
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
        if (alpha <= 0.0f)
        {
            MAIN_WIDGET->setVisible(false);
        }
        else
        {
            MAIN_WIDGET->setAlpha(alpha);
        }
    }
    else if (MAIN_WIDGET->getVisible())
    {
        pushTime = Ogre::Root::getSingleton().getTimer()->getMilliseconds();
        m_Chatbox_TextBox->setEnabled(true);
        MAIN_WIDGET->setAlpha(1);
    }
}

void CLASS::SetVisible(bool value)
{
    if (value)
        this->Show();
    else
        this->Hide();
}
