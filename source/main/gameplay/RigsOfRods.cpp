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
#include "BootstrapLoadingState.h"
#include "GameState.h"
#include "LobbyState.h"
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
	
	
	new ContentManager(); // TODO: delete contentmanager on cleanup

	// dummy state to display the progress bar
	BootstrapLoadingState::create(RoR::Application::GetAppStateManager(),  "BootstrapLoadingState");
	RoR::Application::GetAppStateManager()->changeAppState(RoR::Application::GetAppStateManager()->findByName("BootstrapLoadingState"));

	// then the base content setup
	ContentManager::getSingleton().init();

	// thats the default state it chooses to start
	// GameState = default state, classic
	// LobbyState = experimental Multiplayer Lobby

	String startState = SSETTING("StartState", "GameState");

	GameState::create(RoR::Application::GetAppStateManager(),  "GameState");
	LobbyState::create(RoR::Application::GetAppStateManager(), "LobbyState");

	// select the first one
	LOG("Rigs of Rods main loop starting ...");
	RoR::Application::GetAppStateManager()->start(RoR::Application::GetAppStateManager()->findByName(startState));
	
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
