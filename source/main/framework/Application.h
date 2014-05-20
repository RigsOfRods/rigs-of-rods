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

/** 
	@file   Application.h
	@author Petr Ohlidal
	@date   05/2014
	@brief  Central object manager and communications hub.
*/

#pragma once

#include "RoRPrerequisites.h"
#include "Settings.h"

namespace RoR
{

/** Central object manager and communications hub.
*/
class Application
{
	friend class MainThread; // Manages lifecycle of this class.

public:

	static OgreSubsystem* GetOgreSubsystem()
	{
		assert(ms_ogre_subsystem != nullptr && "OgreSubsystem not created");
		return ms_ogre_subsystem;
	};

	static Settings& GetSettings()
	{
		return Settings::getSingleton(); // Temporary solution
	};

private:

	static void StartOgreSubsystem();

	static void ShutdownOgreSubsystem();

	static OgreSubsystem * ms_ogre_subsystem;
};

} // namespace RoR