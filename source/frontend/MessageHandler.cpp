/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2013-2020 Petr Ohlidal

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

#include "MessageHandler.h"

// backend
#include "GUIManager.h"
#include "AppContext.h"

bool MessageHandler::ProcessMessage(RoR::Message& m)
{
    using namespace RoR;

    // REMEMBER this runs on the backend thread!
    switch (m.type)
    {
        case MSG_APP_SHUTDOWN_REQUESTED:
            this->SetExitRequested();
            break;

        case MSG_APP_SCREENSHOT_REQUESTED:
            App::GetGuiManager()->SetMouseCursorVisibility(GUIManager::MouseCursorVisibility::HIDDEN);
            App::GetAppContext()->CaptureScreenshot();
            App::GetGuiManager()->SetMouseCursorVisibility(GUIManager::MouseCursorVisibility::VISIBLE);
            break;

        case MSG_APP_DISPLAY_FULLSCREEN_REQUESTED:
            App::GetAppContext()->ActivateFullscreen(true);
            App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE,
                _L("Display mode changed to fullscreen"));
            break;

        case MSG_APP_DISPLAY_WINDOWED_REQUESTED:
            App::GetAppContext()->ActivateFullscreen(false);
            App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE,
                _L("Display mode changed to windowed"));
            break;
    }
    return true;
}

bool MessageHandler::WasExitRequested()
{
    std::lock_guard<std::mutex> lock(m_exit_mutex);
    return m_exit_requested;
}

void MessageHandler::SetExitRequested()
{
    std::lock_guard<std::mutex> lock(m_exit_mutex);
    m_exit_requested = true;
}