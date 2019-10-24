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

#pragma once

#include "RoRPrerequisites.h"
#include "InterThreadStoreVector.h"

#include "mygui/BaseLayout.h"
#include "GUI_GameConsoleLayout.h"

namespace RoR {
// Special - not in namespace GUI

class Console :
    public Ogre::LogListener,
    public GUI::GameConsoleLayout,
    public ZeroedMemoryAllocator
{
public:

    Console();
    ~Console();

    void SetVisible(bool _visible);
    bool IsVisible();

    enum MessageType
    {
        CONSOLE_HELP,
        CONSOLE_TITLE,

        CONSOLE_LOCAL_SCRIPT,      // scripting in console

        CONSOLE_SYSTEM_NOTICE,
        CONSOLE_SYSTEM_ERROR,
        CONSOLE_SYSTEM_REPLY,      // reply to a commands

        CONSOLE_LOGMESSAGE,        // Echo from 'RoR.log'
        CONSOLE_LOGMESSAGE_SCRIPT, // Echo from 'AngelScript.log'

        // TODO: Dummy - no effect, TO BE REMOVED ~ only_a_ptr, 10/2019
        CONSOLE_MSGTYPE_LOG,
        CONSOLE_MSGTYPE_INFO,
        CONSOLE_MSGTYPE_SCRIPT
    };

    struct Message
    {
        MessageType cm_type;
        std::string cm_text;
    };

    void putMessage(int type, int uid, Ogre::UTFString msg, Ogre::String icon = "bullet_black.png", unsigned long ttl = 30000, bool forcevisible = false);

protected:

    void notifyWindowButtonPressed(MyGUI::WidgetPtr _sender, const std::string& _name);
    void messageUpdate(float dt);

    Ogre::String ConsoleText;
    Ogre::UTFString TextCol;
    bool angelscriptMode;

    void frameEntered(float dt);

    void messageLogged(const Ogre::String& message, Ogre::LogMessageLevel lml, bool maskDebug, const Ogre::String& logName, bool& skipThisMessage);

    void eventCommandAccept(MyGUI::Edit* _sender);
    void eventMouseButtonClickSendButton(MyGUI::WidgetPtr _sender);
    void eventButtonPressed(MyGUI::Widget* _sender, MyGUI::KeyCode _key, MyGUI::Char _char);

    Ogre::String sTextHistory[500];
    int iText;
    int HistoryCursor;
    InterThreadStoreVector<Console::Message> m_msg_queue;
};

} // namespace RoR
