/*
This source file is part of Rigs of Rods
Copyright 2005-2012 Pierre-Michel Ricordel
Copyright 2007-2012 Thomas Fischer

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
#include "RigsOfRods.h"

#include <Ogre.h>

#include "Application.h"
#include "AppStateManager.h"
#include "Settings.h"
#include "ContentManager.h"

using namespace Ogre;

RigsOfRods::RigsOfRods()
{
	setSingleton(this);
}

RigsOfRods::~RigsOfRods()
{
}

void RigsOfRods::go(void)
{

}

void RigsOfRods::update(double dt)
{
	RoR::Application::GetAppStateManager()->update(dt);
}

void RigsOfRods::shutdown()
{
	if (RoR::Application::GetAppStateManager() != nullptr)
	{
		RoR::Application::GetAppStateManager()->shutdown();
	} 
	else
	{
		printf("shutdown failed, no statemanager instance!\n");
	}
}

void RigsOfRods::tryShutdown()
{
	if (RoR::Application::GetAppStateManager() != nullptr)
	{
		RoR::Application::GetAppStateManager()->tryShutdown();
	}
}

void RigsOfRods::pauseRendering()
{
	RoR::Application::GetAppStateManager()->pauseRendering();
}
