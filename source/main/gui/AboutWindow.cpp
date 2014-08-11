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
#ifdef USE_MYGUI

#include "AboutWindow.h"
#include "GUIManager.h"
#include "InputEngine.h"
#include "Language.h"
#include "Application.h"
#include "SkinManager.h"
#include "Utils.h"
#include "OgreSubsystem.h"
#include "RoRFrameListener.h"
#include "RoRVersion.h"

#include "MenuWindow.h"

using namespace Ogre;

AboutWindow::AboutWindow() 
{
	//TODO
	initialiseByAttributes(this);
	mMainWidget->setVisible(false);
	((MyGUI::Window*)mMainWidget)->setCaption(_L("About"));
	MyGUI::WindowPtr win = dynamic_cast<MyGUI::WindowPtr>(mMainWidget);

	mBackButton->eventMouseButtonClick      += MyGUI::newDelegate(this, &AboutWindow::eventMouseButtonClickBackButton);
	mMainWidget->setPosition((0.3 * RoR::Application::GetOgreSubsystem()->GetRenderWindow()->getViewport(0)->getActualWidth()) , (0.3 * RoR::Application::GetOgreSubsystem()->GetRenderWindow()->getViewport(0)->getActualHeight() ));
	mVersionWord->setCaption(String(ROR_VERSION_STRING));
	mNetVersionWord->setCaption(String(RORNET_VERSION));
}

AboutWindow::~AboutWindow()
{
	//TODO
}

void AboutWindow::Show()
{
	RoR::Application::GetInputEngine()->resetKeys();
	MyGUI::InputManager::getInstance().setKeyFocusWidget(mMainWidget);
	mMainWidget->setEnabledSilent(true);
    mMainWidget->castType<MyGUI::Window>()->setVisible(true);
}

void AboutWindow::Hide()
{
	RoR::Application::GetGuiManager()->unfocus();
	mMainWidget->setVisible(false);
	mMainWidget->setEnabledSilent(false);
	if (! gEnv->frameListener->loading_state == TERRAIN_LOADED)
	{
		//Return to the menu
		MenuWindow::getSingleton().Show();
	}
}

void AboutWindow::eventMouseButtonClickBackButton(MyGUI::WidgetPtr _sender)
{
	Hide();
}

#endif