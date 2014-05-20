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
	@file   Application.cpp
	@author Petr Ohlidal
	@date   05/2014
*/

#include "Application.h"

#include "OgreSubsystem.h"

#include <OgreException.h>

namespace RoR
{

// Init static members
OgreSubsystem* Application::ms_ogre_subsystem = nullptr;

void Application::StartOgreSubsystem()
{
	ms_ogre_subsystem = new OgreSubsystem();
	if (ms_ogre_subsystem == nullptr)
	{
		throw std::runtime_error("[RoR] Failed to create OgreSubsystem");
	}

	if (! ms_ogre_subsystem->StartOgre("RoR", "", ""))
	{
		throw std::runtime_error("[RoR] Failed to start up OGRE 3D engine");
	}
}

void Application::ShutdownOgreSubsystem()
{
	assert(ms_ogre_subsystem != nullptr && "Application::ShutdownOgreSubsystem(): Ogre subsystem was not started");
	delete ms_ogre_subsystem;
}

} // namespace RoR