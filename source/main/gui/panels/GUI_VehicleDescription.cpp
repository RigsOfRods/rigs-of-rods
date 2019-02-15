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
/// @date   11/2014

#include "GUI_VehicleDescription.h"

#include "Application.h"
#include "Beam.h"
#include "BeamFactory.h"
#include "GUIManager.h"
#include "InputEngine.h"
#include "Utils.h"
#include "RoRnet.h"
#include "Language.h"
#include "RoRFrameListener.h"

#include <MyGUI.h>

using namespace RoR;
using namespace GUI;

#define CLASS        VehicleDescription
#define MAIN_WIDGET  ((MyGUI::Window*)mMainWidget)

CLASS::CLASS()
{
    MyGUI::WindowPtr win = dynamic_cast<MyGUI::WindowPtr>(mMainWidget);
    win->eventWindowButtonPressed += MyGUI::newDelegate(this, &CLASS::notifyWindowButtonPressed); //The "X" button thing

    CenterToScreen();
    MAIN_WIDGET->setVisible(false);
}

CLASS::~CLASS()
{
    m_vehicle_desc->setCaptionWithReplacing("");
}

void CLASS::LoadText()
{
    Actor* currTruck = App::GetSimController()->GetPlayerActor();

    if (currTruck == nullptr)
        return;

    m_vehicle_title->setMaxTextLength(33);
    m_vehicle_title->setCaptionWithReplacing(currTruck->GetActorDesignName());

    Ogre::UTFString txt = _L("(no author information available) ");

    std::vector<authorinfo_t> authors = currTruck->getAuthors();
    if (!authors.empty())
    {
        txt = _L("Authors: \n");
        for (auto author : authors)
        {
            txt = txt + "* " + author.name + " \n";
        }
    }

    std::vector<std::string> description = currTruck->getDescription();
    if (!description.empty())
    {
        txt = txt + _L("\nDescription: \n");
        for (auto line : description)
        {
            txt = txt + ANSI_TO_UTF(line) + "\n";
        }
    }

    txt = txt + _L("\nCommands: \n");

    int filledCommands = 0;
    for (int i = 1; i < MAX_COMMANDS && filledCommands < COMMANDS_VISIBLE; i += 2)
    {
        if (currTruck->ar_command_key[i].description == "hide")
            continue;
        if (currTruck->ar_command_key[i].beams.empty() && currTruck->ar_command_key[i].rotators.empty())
            continue;

        filledCommands++;
        char commandID[256] = {};
        Ogre::String keyStr = "";

        sprintf(commandID, "COMMANDS_%02d", i);
        int eventID = RoR::App::GetInputEngine()->resolveEventName(Ogre::String(commandID));
        Ogre::String keya = RoR::App::GetInputEngine()->getEventCommand(eventID);
        sprintf(commandID, "COMMANDS_%02d", i + 1);
        eventID = RoR::App::GetInputEngine()->resolveEventName(Ogre::String(commandID));
        Ogre::String keyb = RoR::App::GetInputEngine()->getEventCommand(eventID);

        // cut off expl
        if (keya.size() > 6 && keya.substr(0, 5) == "EXPL+")
            keya = keya.substr(5);
        if (keyb.size() > 6 && keyb.substr(0, 5) == "EXPL+")
            keyb = keyb.substr(5);

        keyStr = keya + "/" + keyb;

        if (currTruck->ar_command_key[i].description.empty())
        {
            txt = txt + "* " + keyStr + ": " + _L("unknown function");
        }
        else
        {
            txt = txt + "* " + keyStr + ": " + currTruck->ar_command_key[i].description;
        }

        txt = txt + "\n";
    }

    m_vehicle_desc->setCaption(Ogre::String(txt));
}

bool CLASS::IsVisible()
{
    return MAIN_WIDGET->getVisible();
}

void CLASS::SetVisible(bool vis)
{
    if (vis && !IsVisible())
        LoadText();

    MAIN_WIDGET->setVisible(vis);
}

void CLASS::CenterToScreen()
{
    MyGUI::IntSize windowSize = MAIN_WIDGET->getSize();
    MyGUI::IntSize parentSize = MAIN_WIDGET->getParentSize();

    MAIN_WIDGET->setPosition((parentSize.width - windowSize.width) / 2, (parentSize.height - windowSize.height) / 2);
}

void CLASS::notifyWindowButtonPressed(MyGUI::WidgetPtr _sender, const std::string& _name)
{
    if (_name == "close")
        SetVisible(false);
}
