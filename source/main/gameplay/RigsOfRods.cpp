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

#include "BootstrapLoadingState.h"
#include "GameState.h"
#include "LobbyState.h"
#include "Settings.h"
#include "ContentManager.h"

using namespace Ogre;

RigsOfRods::RigsOfRods(Ogre::String name, Ogre::String hwnd, Ogre::String mainhwnd, bool embedded) :
	stateManager(0),
	hwnd(hwnd),
	mainhwnd(mainhwnd),
	name(name),
	embedded(embedded)
{
	setSingleton(this);
}

RigsOfRods::~RigsOfRods()
{
	delete stateManager;
	delete OgreFramework::getSingletonPtr();
	// TODO: delete contentmanager
	
}

void RigsOfRods::go(void)
{
	// init ogre
	new OgreFramework();
	if (!OgreFramework::getSingletonPtr()->initOgre(name, hwnd, mainhwnd, embedded))
		return;

	// now add the states
	stateManager = new AppStateManager();
	new ContentManager();

	if (!BSETTING("REPO_MODE", false))
	{
		// dummy state to display the progress bar
		BootstrapLoadingState::create(stateManager,  "BootstrapLoadingState");
		stateManager->changeAppState(stateManager->findByName("BootstrapLoadingState"));
	} else
	{
		// in repo mode, directly into the gamestate
		ContentManager::getSingleton().initBootstrap();
	}

	// then the base content setup
	ContentManager::getSingleton().init();

	// thats the default state it chooses to start
	// GameState = default state, classic
	// LobbyState = experimental Multiplayer Lobby

	String startState = SSETTING("StartState", "GameState");

	GameState::create(stateManager,  "GameState");
	LobbyState::create(stateManager, "LobbyState");

	// select the first one
	if (embedded)
	{
		LOG("Rigs of Rods embedded initialized!");
		stateManager->changeAppState(stateManager->findByName(startState));
	} else
	{
		LOG("Rigs of Rods main loop starting ...");
		stateManager->start(stateManager->findByName(startState));
	}
}

void RigsOfRods::update(double dt)
{
	stateManager->update(dt);
}

void RigsOfRods::shutdown()
{
	if (stateManager)
	{
		stateManager->shutdown();
	} else
	{
		printf("shutdown failed, no statemanager instance!\n");
	}
}

void RigsOfRods::tryShutdown()
{
	if (stateManager)
		stateManager->tryShutdown();
}

void RigsOfRods::pauseRendering()
{
	stateManager->pauseRendering();
}
