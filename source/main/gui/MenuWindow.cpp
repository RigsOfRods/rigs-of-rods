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

#include "MenuWindow.h"
#include "GUIManager.h"
#include "InputEngine.h"
#include "Language.h"
#include "Application.h"
#include "SkinManager.h"
#include "Utils.h"
#include "OgreSubsystem.h"
#include "RoRFrameListener.h"

#include "SelectorWindow.h"
#include "SettingsWindow.h"
#include "AboutWindow.h"

using namespace Ogre;

MenuWindow::MenuWindow() 
{
	//TODO
	initialiseByAttributes(this);
	mMainWidget->setVisible(false);
	((MyGUI::Window*)mMainWidget)->setCaption(_L("Menu"));
	MyGUI::WindowPtr win = dynamic_cast<MyGUI::WindowPtr>(mMainWidget);

	mSelTerrButton->eventMouseButtonClick      += MyGUI::newDelegate(this, &MenuWindow::eventMouseButtonClickSelectButton);
	mSettingsButton->eventMouseButtonClick     += MyGUI::newDelegate(this, &MenuWindow::eventMouseButtonClickSettingButton);
	mAboutButton->eventMouseButtonClick        += MyGUI::newDelegate(this, &MenuWindow::eventMouseButtonClickAboutButton);
	mCloseButton->eventMouseButtonClick        += MyGUI::newDelegate(this, &MenuWindow::eventMouseButtonClickExitButton);

	mMainWidget->setPosition((0.05 * RoR::Application::GetOgreSubsystem()->GetRenderWindow()->getViewport(0)->getActualWidth()) , (0.6 * RoR::Application::GetOgreSubsystem()->GetRenderWindow()->getViewport(0)->getActualHeight() ));
	//mMainWidget->setRealSize(0.8, 0.8);
	//SelectorWindow::getSingleton().show(SelectorWindow::LT_Terrain); 
}

MenuWindow::~MenuWindow()
{
	//TODO
}

void MenuWindow::Show()
{
	RoR::Application::GetInputEngine()->resetKeys();
	// focus main mMainWidget (for key input)
	MyGUI::InputManager::getInstance().setKeyFocusWidget(mMainWidget);
	mMainWidget->setEnabledSilent(true);

    mMainWidget->castType<MyGUI::Window>()->setVisible(true);
	SelectorWindow::getSingleton().show(SelectorWindow::LT_Terrain); 
	SelectorWindow::getSingleton().mMainWidget->setVisible(false);
	//SelectorWindow::getSingleton().
}

void MenuWindow::Hide()
{
	RoR::Application::GetGuiManager()->unfocus();
	mMainWidget->setVisible(false);
	mMainWidget->setEnabledSilent(false);
}

void MenuWindow::eventMouseButtonClickSelectButton(MyGUI::WidgetPtr _sender)
{
	//SelectorWindow::getSingleton().show(SelectorWindow::LT_AllBeam);
	SelectorWindow::getSingleton().mMainWidget->setVisible(true);
	Hide();
}

void MenuWindow::eventMouseButtonClickSettingButton(MyGUI::WidgetPtr _sender)
{
	SettingsWindow::getSingleton().Show();
	//Hide();
}

void MenuWindow::eventMouseButtonClickAboutButton(MyGUI::WidgetPtr _sender)
{
	AboutWindow::getSingleton().Show();
	//Hide();
}

void MenuWindow::eventMouseButtonClickExitButton(MyGUI::WidgetPtr _sender)
{
	gEnv->frameListener->shutdown_final();
	//Hide();
}
#endif