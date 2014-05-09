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

#include "HighScoreWindow.h"
#include "GUIManager.h"
#include "Language.h"
#include "Utils.h"

HighScoreWindow::HighScoreWindow()
{
	initialiseByAttributes(this);

	((MyGUI::Window*)mMainWidget)->setCaption(_L("Highscores"));
	((MyGUI::Window*)mMainWidget)->eventWindowButtonPressed += MyGUI::newDelegate(this, &HighScoreWindow::notifyWindowPressed);
}

HighScoreWindow::~HighScoreWindow()
{
}

void HighScoreWindow::show(Ogre::UTFString &txt)
{
	((MyGUI::Window*)mMainWidget)->setVisibleSmooth(true);
	((MyGUI::Window*)mMainWidget)->setAlpha(0.9);

	int width = 200;
	int height = MyGUI::RenderManager::getInstance().getViewSize().height - 120;
	int left = MyGUI::RenderManager::getInstance().getViewSize().width - width;
	int top = 60;

	((MyGUI::Window*)mMainWidget)->setPosition(left, top);
	((MyGUI::Window*)mMainWidget)->setSize(width, height);

	try
	{
		mTxt->setCaption(convertToMyGUIString(txt));
	} catch(...)
	{
		mTxt->setCaption("ENCODING ERROR");
	}

}

void HighScoreWindow::hide()
{
	((MyGUI::Window*)mMainWidget)->setVisibleSmooth(false);
}

void HighScoreWindow::notifyWindowPressed(MyGUI::Window* _widget, const std::string& _name)
{
	//MyGUI::WindowPtr window = _widget->castType<MyGUI::Window>();
	if (_name == "close")
		hide();
}
#endif //MYGUI

