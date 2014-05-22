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

#ifdef USE_MYGUI

#include "GUIInputManager.h"
#include "RoRPrerequisites.h"

#include <MyGUI.h>
#include <OgreFrameListener.h>
#include <OgreWindowEventUtilities.h>


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

	MyGUI::Gui* getGUI() { return mGUI; }

	void unfocus();

	static Ogre::String getRandomWallpaperImage();

	void windowResized(Ogre::RenderWindow* rw);

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
};

} // namespace RoR

#endif // USE_MYGUI
