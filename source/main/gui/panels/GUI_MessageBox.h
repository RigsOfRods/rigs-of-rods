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
	along with Rigs of Rods.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

/** 
	@file   GUI_MessageBox.h
	@author Moncef Ben Slimane
	@date   12/2014
*/

#include "ForwardDeclarations.h"
#include "GUI_MessageBoxLayout.h"

namespace RoR
{

namespace GUI
{

class gMessageBox : public MessageBoxLayout //Used gMessageBox to not make interference with MessageBox function
{

public:
	gMessageBox();
	~gMessageBox();
	
	void ShowMessageBox(Ogre::String mTitle, Ogre::String mText, bool button1, Ogre::String mButton1, bool AllowClose, bool button2, Ogre::String mButton2);
	void UpdateMessageBox(Ogre::String mTitle, Ogre::String mText, bool button1, Ogre::String mButton1, bool AllowClose, bool button2, Ogre::String mButton2, bool IsVisible);
	// return 0: no button pressed, should not happen, use AllowClose to enable.
	// return 1: button1 pressed.
	// return 2: button2 pressed.
	int getResult();

private:
	void notifyWindowButtonPressed(MyGUI::WidgetPtr _sender, const std::string& _name);
	void eventMouseButton1ClickSaveButton(MyGUI::WidgetPtr _sender);
	void eventMouseButton2ClickSaveButton(MyGUI::WidgetPtr _sender);

	void Show();
	void Hide();

	bool b_AllowClose;
	int i_Results;
};

} // namespace GUI

} // namespace RoR
