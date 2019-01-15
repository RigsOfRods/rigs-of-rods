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


#pragma once

#include "RoRPrerequisites.h"
#include "mygui/BaseLayout.h"
#include <Ogre.h>

namespace RoR {
namespace GUI {

ATTRIBUTE_CLASS_LAYOUT(LoadingWindow, "LoadingWindow.layout");

class LoadingWindow :
    public wraps::BaseLayout,
    public ZeroedMemoryAllocator
{
public:

    LoadingWindow();
    ~LoadingWindow();

    void setProgress(int _percent, const Ogre::UTFString& _text = "", bool _updateRenderFrame = true);
    void setAutotrack(const Ogre::UTFString& _text = "", bool _updateRenderFrame = true);

    void SetVisible(bool v);
    bool IsVisible();
    void SetMustUpdateRenderwindow(bool val) { m_must_update_renderwindow = val; }

private:

    void renderOneFrame(bool force = false);

    ATTRIBUTE_FIELD_WIDGET_NAME(LoadingWindow, mBarProgress, "Bar");

    MyGUI::ProgressBar* mBarProgress;
    ATTRIBUTE_FIELD_WIDGET_NAME(LoadingWindow, mInfoStaticText, "Info");

    MyGUI::TextBox* mInfoStaticText;

    Ogre::Timer* t;

    bool m_must_update_renderwindow; //!< Must call RenderWindow::update() after rendering
};

} // namespace GUI
} // namespace RoR
