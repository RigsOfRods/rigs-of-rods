/*
This source file is part of Rigs of Rods
Copyright 2005-2012 Pierre-Michel Ricordel
Copyright 2007-2012 Thomas Fischer

For more information, see http://www.rigsofrods.com/

Rigs of Rods is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License version 3, as
published by the Free Software Foundation.

Rigs of Rods is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Rigs of Rods.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "CameraBehaviorIsometric.h"

#include "Application.h"
#include "Console.h"
#include "Language.h"
#include "GUIManager.h"

using namespace Ogre;

void CameraBehaviorIsometric::activate(const CameraManager::CameraContext &ctx, bool reset /* = true */)
{
#ifdef USE_MYGUI
	RoR::Application::GetConsole()->putMessage(
		RoR::Console::CONSOLE_MSGTYPE_INFO, 
		RoR::Console::CONSOLE_SYSTEM_NOTICE, 
		_L("fixed free camera"), 
		"camera_link.png", 
		3000
	);
	RoR::Application::GetGuiManager()->PushNotification("Notice:", _L("fixed free camera") + TOSTRING(""));
#endif // USE_MYGUI
}
