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

#ifndef __HIGHSCORE_WINDOW_H__
#define __HIGHSCORE_WINDOW_H__

#include "RoRPrerequisites.h"
#include "Singleton.h"
#include "mygui/BaseLayout.h"
#include <Ogre.h>
#include "Skin.h"


ATTRIBUTE_CLASS_LAYOUT(HighScoreWindow, "HighScoreWindow.layout");

class HighScoreWindow :
	public wraps::BaseLayout,
	public RoRSingleton<HighScoreWindow>,
	public ZeroedMemoryAllocator
{
	friend class RoRSingleton<HighScoreWindow>;
	HighScoreWindow();
	~HighScoreWindow();
public:
	void show(Ogre::UTFString &str);
	void hide();

private:
	ATTRIBUTE_FIELD_WIDGET_NAME(HighScoreWindow, mTxt, "highscore_txt");
	MyGUI::TextBox* mTxt;
	
	void notifyWindowPressed(MyGUI::Window* _widget, const std::string& _name);

};

#endif // __HIGHSCORE_WINDOW_H__

#endif //MYGUI
