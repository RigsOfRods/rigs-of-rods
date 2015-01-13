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
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Rigs of Rods. If not, see <http://www.gnu.org/licenses/>.
*/

/** 
	@file   GUIManager.h
	@author based on the basemanager code from mygui common
*/

#pragma once

#include "GUIInputManager.h"
#include "RoRPrerequisites.h"

#include <MyGUI.h>
#include <OgreFrameListener.h>
#include <OgreWindowEventUtilities.h>

//Include managed guis headers
#include "GUI_GameMainMenu.h"
#include "GUI_GameAbout.h"
#include "GUI_GameSettings.h"
#include "GUI_DebugOptions.h"
#include "GUI_SimUtils.h"
#include "GUI_MessageBox.h"
#include "GUI_MultiplayerSelector.h"

namespace RoR
{

class GUIManager :
	  public GUIInputManager
	, public Ogre::FrameListener
	, public Ogre::WindowEventListener
	, public ZeroedMemoryAllocator
{

	friend class RoR::Application; // Manages lifecycle of this class

public:

	void destroy();

	void unfocus();

	static Ogre::String getRandomWallpaperImage();

	void windowResized(Ogre::RenderWindow* rw);

	/** Set scene manager where GUI will be rendered */
	void SetSceneManager(Ogre::SceneManager* scene_manager);

	//GUI windows manager
	void ShowMainMenu(bool isVisible);
	void ShowSettingGui(bool isVisible);
	void ShowAboutGUI(bool isVisible);
	void ShowDebugOptionsGUI(bool isVisible);

	void ToggleFPSBox();
	void ToggleTruckInfoBox();

	void UpdateSimUtils(float dt, Beam *truck);
	void ShowMessageBox(Ogre::String mTitle, Ogre::String mText, bool button1, Ogre::String mButton1, bool AllowClose, bool button2, Ogre::String mButton2);
	int getMessageBoxResult(); //TODO

	void ShowMultiPlayerSelector(bool isVisible);
private:

	GUIManager();
	virtual ~GUIManager();

	bool create();
	void createGui();
	void destroyGui();

	virtual bool frameStarted(const Ogre::FrameEvent& _evt);
	virtual bool frameEnded(const Ogre::FrameEvent& _evt);
	virtual void windowClosed(Ogre::RenderWindow* rw);

	void eventRequestTag(const MyGUI::UString& _tag, MyGUI::UString& _result);

	MyGUI::Gui* mGUI;
	MyGUI::OgrePlatform* mPlatform;
	Ogre::String mResourceFileName;
	bool mExit;

	//GUI Windows pointers
    std::unique_ptr<GUI::GameMainMenu>      m_gui_GameMainMenu;
	std::unique_ptr<GUI::GameAbout>			m_gui_GameAbout;
	std::unique_ptr<GUI::GameSettings>		m_gui_GameSettings;
	std::unique_ptr<GUI::DebugOptions>		m_gui_DebugOptions;
	std::unique_ptr<GUI::SimUtils>			m_gui_SimUtils;
	bool isSimUtilsVisible;
	std::unique_ptr<GUI::gMessageBox>		m_gui_gMessageBox;

	std::unique_ptr<GUI::MultiplayerSelector>			m_gui_MultiplayerSelector;
};

} // namespace RoR

