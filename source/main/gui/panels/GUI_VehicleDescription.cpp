/*
This source file is part of Rigs of Rods
Copyright 2005-2012 Pierre-Michel Ricordel
Copyright 2007-2012 Thomas Fischer
Copyright 2013-2014 Petr Ohlidal

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

/**
@file   GUI_VehicleDescription.cpp
@author Moncef Ben Slimane
@date   11/2014
*/

#include "GUI_VehicleDescription.h"

#include "RoRPrerequisites.h"
#include "Beam.h"
#include "BeamFactory.h"
#include "Utils.h"
#include "RoRVersion.h"
#include "rornet.h"
#include "Language.h"
#include "GUIManager.h"
#include "Application.h"


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
	Hide();
}

CLASS::~CLASS()
{
	currTruck = nullptr;
	m_vehicle_desc->setCaptionWithReplacing("");
}

void CLASS::LoadText()
{
	m_vehicle_title->setMaxTextLength(33);
	m_vehicle_title->setCaptionWithReplacing(currTruck->getTruckName());

	Ogre::UTFString txt;

	std::vector<authorinfo_t> file_authors = currTruck->getAuthors();
	if (!file_authors.empty())
	{
		Ogre::String authors = "";
		for (std::vector<authorinfo_t>::iterator it = file_authors.begin(); it != file_authors.end(); ++it)
		{
			authors += "* " + (*it).name + " \n";
		}
		txt = txt + _L("Authors: \n") + authors;
	}
	else
		txt = txt + _L("(no author information available) ");

	std::vector<std::string> description = currTruck->getDescription();
	for (unsigned int i = 1; i < 3; i++)
	{
		if (i < description.size())
		{
			txt = txt + _L("\nDescription: \n");
			txt = txt + (ANSI_TO_UTF(description[i])) + "\n";
		}
	}

	txt = txt + _L("\nCommands: \n");

	int filledCommands = 0;
	for (int i = 1; i < MAX_COMMANDS && filledCommands < COMMANDS_VISIBLE; i += 2)
	{
		if (currTruck->commandkey[i].beams.empty() || currTruck->commandkey[i].description == "hide") continue;

		filledCommands++;
		char commandID[256] = {};
		Ogre::String keyStr = "";

		sprintf(commandID, "COMMANDS_%02d", i);
		int eventID = RoR::Application::GetInputEngine()->resolveEventName(Ogre::String(commandID));
		Ogre::String keya = RoR::Application::GetInputEngine()->getEventCommand(eventID);
		sprintf(commandID, "COMMANDS_%02d", i + 1);
		eventID = RoR::Application::GetInputEngine()->resolveEventName(Ogre::String(commandID));
		Ogre::String keyb = RoR::Application::GetInputEngine()->getEventCommand(eventID);

		// cut off expl
		if (keya.size() > 6 && keya.substr(0, 5) == "EXPL+") keya = keya.substr(5);
		if (keyb.size() > 6 && keyb.substr(0, 5) == "EXPL+") keyb = keyb.substr(5);

		keyStr = keya + "/" + keyb;

		if (currTruck->commandkey[i].description.empty())
		{
			txt = txt + "* " + keyStr + ": " + _L("unknown function");
		}
		else
		{
			txt = txt + "* " + keyStr + ": " + currTruck->commandkey[i].description;
		}

		txt = txt + "\n";
	}

	m_vehicle_desc->setCaption(Ogre::String(txt));
}

void CLASS::Show()
{
	MAIN_WIDGET->setVisible(true);
	currTruck = BeamFactory::getSingleton().getCurrentTruck();
	LoadText();
}

void CLASS::Hide()
{
	MAIN_WIDGET->setVisible(false);
}

bool CLASS::getVisible()
{
	return MAIN_WIDGET->getVisible();
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
		Hide();
}