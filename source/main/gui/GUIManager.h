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
#ifndef __GUI_Manager_H_
#define __GUI_Manager_H_

#include "RoRPrerequisites.h"

#include "GUIInputManager.h"
#include "Ogre.h"
#include "Singleton.h"

#include <MyGUI.h>

#define GETMYGUI GUIManager::getSingleton().getGUI()

namespace MyGUI { class OgrePlatform; }

class GUIManager :
	  public RoRSingletonNoCreation<GUIManager>
	, public GUIInputManager
	, public Ogre::FrameListener
	, public Ogre::WindowEventListener
	, public ZeroedMemoryAllocator
{
public:

	GUIManager();
	virtual ~GUIManager();

	void destroy();

	MyGUI::Gui* getGUI() { return mGUI; }

	void unfocus();

	static Ogre::String getRandomWallpaperImage();

	void windowResized(Ogre::RenderWindow* rw);

private:

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

#endif // __GUI_Manager_H_
#endif // USE_MYGUI
