/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2013-2017 Petr Ohlidal & contributors

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

/// @file
/// @author Moncef Ben Slimane
/// @date   12/2014

#include "GUI_MessageBox.h"

#include "RoRPrerequisites.h"
#include "Utils.h"
#include "RoRVersion.h"
#include "RoRnet.h"
#include "Language.h"
#include "GUIManager.h"
#include "ScriptEvents.h"
#include "Scripting.h"

#include <MyGUI.h>

using namespace RoR;
using namespace GUI;

#define CLASS        gMessageBox
#define MAIN_WIDGET  ((MyGUI::Window*)mMainWidget)

CLASS::CLASS()
{
    MyGUI::WindowPtr win = dynamic_cast<MyGUI::WindowPtr>(mMainWidget);
    win->eventWindowButtonPressed += MyGUI::newDelegate(this, &CLASS::notifyWindowButtonPressed); //The "X" button thing

    MyGUI::IntSize gui_area = MyGUI::RenderManager::getInstance().getViewSize();
    mMainWidget->setPosition(gui_area.width / 2 - mMainWidget->getWidth() / 2, gui_area.height / 2 - mMainWidget->getHeight() / 2);

    b_AllowClose = false;

    m_button1->eventMouseButtonClick += MyGUI::newDelegate(this, &CLASS::eventMouseButton1ClickSaveButton);
    m_button2->eventMouseButtonClick += MyGUI::newDelegate(this, &CLASS::eventMouseButton2ClickSaveButton);

    MAIN_WIDGET->setVisible(false);
}

CLASS::~CLASS()
{
}

void CLASS::Show()
{
    MAIN_WIDGET->setVisibleSmooth(true);
}

void CLASS::UpdateMessageBox(Ogre::String mTitle, Ogre::String mText, bool button1, Ogre::String mButton1, bool AllowClose, bool button2, Ogre::String mButton2, bool IsVisible)
{
    ((MyGUI::Window*)mMainWidget)->setCaptionWithReplacing(mTitle);
    m_text->setCaptionWithReplacing(mText);

    if (button1)
        m_button1->setCaptionWithReplacing(mButton1);

    if (button2)
        m_button2->setCaptionWithReplacing(mButton2);

    if (!MAIN_WIDGET->getVisible())
        Show();
    else if (IsVisible == false)
        Hide();
}

void CLASS::ShowMessageBox(Ogre::String mTitle, Ogre::String mText, bool button1, Ogre::String mButton1, bool AllowClose, bool button2, Ogre::String mButton2)
{
    i_Results = 0; //Reinit
    ((MyGUI::Window*)mMainWidget)->setCaptionWithReplacing(mTitle);
    m_text->setCaptionWithReplacing(mText);

    if (button1)
        m_button1->setCaptionWithReplacing(mButton1);

    if (button2)
        m_button2->setCaptionWithReplacing(mButton2);

    Show(); //Last thing to be done
}

bool CLASS::IsVisible()
{
    return MAIN_WIDGET->getVisible();
}

int CLASS::getResult()
{
    if (i_Results > 0)
        return i_Results;

    return 0;
}

void CLASS::Hide()
{
    MAIN_WIDGET->setVisibleSmooth(false);
}

void CLASS::notifyWindowButtonPressed(MyGUI::WidgetPtr _sender, const std::string& _name)
{
    if (_name == "close" && !b_AllowClose)
    {
        TRIGGER_EVENT(SE_GENERIC_MESSAGEBOX_CLICK, 0);
        Hide();
    }
}

void CLASS::eventMouseButton1ClickSaveButton(MyGUI::WidgetPtr _sender)
{
    TRIGGER_EVENT(SE_GENERIC_MESSAGEBOX_CLICK, 1);
    i_Results = 1;
    Hide();
}

void CLASS::eventMouseButton2ClickSaveButton(MyGUI::WidgetPtr _sender)
{
    TRIGGER_EVENT(SE_GENERIC_MESSAGEBOX_CLICK, 2);
    i_Results = 2;
    Hide();
}
