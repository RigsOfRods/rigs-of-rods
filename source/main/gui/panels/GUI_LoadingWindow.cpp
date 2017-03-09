/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer

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


#include "GUI_LoadingWindow.h"

#include "Application.h"
#include "GUIManager.h"
#include "Language.h"
#include "RoRWindowEventUtilities.h"
#include "Settings.h"
#include "Utils.h"

namespace RoR {
namespace GUI {

LoadingWindow::LoadingWindow()
{
    initialiseByAttributes(this);

    MyGUI::IntSize gui_area = MyGUI::RenderManager::getInstance().getViewSize();
    mMainWidget->setPosition(gui_area.width / 2 - mMainWidget->getWidth() / 2, gui_area.height / 2 - mMainWidget->getHeight() / 2);
    ((MyGUI::Window*)mMainWidget)->setCaption(_L("Loading ..."));
    t = new Ogre::Timer();
    mMainWidget->setVisible(false);
}

LoadingWindow::~LoadingWindow()
{
    delete(t);
    t = NULL;
}

void LoadingWindow::setProgress(int _percent, const Ogre::UTFString& _text, bool _updateRenderFrame)
{
    mMainWidget->setVisible(true);
    mInfoStaticText->setCaption(convertToMyGUIString(_text));

    mBarProgress->setProgressAutoTrack(false);
    mBarProgress->setProgressPosition(_percent);

    if (_updateRenderFrame)
    {
        renderOneFrame();
    }
}

void LoadingWindow::setAutotrack(const Ogre::UTFString& _text, bool _updateRenderFrame)
{
    mMainWidget->setVisible(true);
    mInfoStaticText->setCaption(convertToMyGUIString(_text));
    mBarProgress->setProgressPosition(0);
    mBarProgress->setProgressAutoTrack(true);

    if (_updateRenderFrame)
    {
        renderOneFrame(true);
    }
}

void LoadingWindow::renderOneFrame(bool force)
{
    if (t->getMilliseconds() > 200 || force)
    {
        // we must pump the window messages, otherwise the window will get white on Vista ...
        RoRWindowEventUtilities::messagePump();
        Ogre::Root::getSingleton().renderOneFrame();
        t->reset();
    }
}

bool LoadingWindow::IsVisible() { return mMainWidget->getVisible(); }
void LoadingWindow::SetVisible(bool v) { mMainWidget->setVisible(v); }

} // namespace GUI
} // namespace RoR
