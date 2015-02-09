/*
	This source file is part of Rigs of Rods
	Copyright 2005-2012 Pierre-Michel Ricordel
	Copyright 2007-2012 Thomas Fischer
	Copyright 2013-2015 Petr Ohlidal

	For more information, see http://www.rigsofrods.com/

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
	@file   
	@author Petr Ohlidal
	@date   02/2015
	@brief  Command interface for GAME/GUI_PANEL -> GUI_MANAGER communication. Reduces compile-time dependencies.
*/

#pragma once

#include "RoRPrerequisites.h"

namespace RoR
{

class GuiManagerInterface
{
public:

	// The manager
	virtual void UnfocusGui() = 0;

	// Rig spawner window
	virtual void AddRigLoadingReport(std::string const & vehicle_name, std::string const & text, int num_errors, int num_warnings, int num_other) = 0;
	virtual void ShowRigSpawnerReportWindow() = 0;
	virtual void HideRigSpawnerReportWindow() = 0;

	// TODO: Use this class to eliminate Singleton access to GUI windows
};

} // namespace RoR
