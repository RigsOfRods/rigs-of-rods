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
	@file   GUI_MultiplayerSelector.h
	@author Moncef Ben Slimane
	@date   11/2014
*/

#include "ForwardDeclarations.h"
#include "GUI_MultiplayerSelectorLayout.h"

namespace RoR
{

namespace GUI
{

class MultiplayerSelector : public MultiplayerSelectorLayout
{

public:
	MultiplayerSelector();
	~MultiplayerSelector();

	void Show();
	void Hide();

private:
	void eventMouseButtonClickJoinButton(MyGUI::WidgetPtr _sender);
	void eventMouseButtonClickConfigButton(MyGUI::WidgetPtr _sender);

	void notifyWindowButtonPressed(MyGUI::WidgetPtr _sender, const std::string& _name);
	void CenterToScreen();
	bool IsVisible();
	void init();
};

} // namespace GUI

} // namespace RoR
