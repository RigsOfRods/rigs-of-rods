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
	@file   MainThread.cpp
	@author Petr Ohlidal
	@date   05/2014
*/

#include "MainThread.h"

#include "Application.h"
#include "StartupScreen.h"
#include "AppStateManager.h"
#include "ContentManager.h"
#include "GameState.h"
#include "Language.h"
#include "LobbyState.h"
#include "OgreSubsystem.h"
#include "Settings.h"

#include <OgreRoot.h>

// Global instance of GlobalEnvironment used throughout the game.
GlobalEnvironment *gEnv; 

namespace RoR
{

void MainThread::go()
{
	// ================================================================================
	// Bootstrap
	// ================================================================================

	gEnv = new GlobalEnvironment(); // Instantiate global environment

	if (! Application::GetSettings().setupPaths() )
	{
		throw std::runtime_error("[RoR] MainThread::go(): Failed to setup file paths");
	}

	Application::StartOgreSubsystem();
	gEnv->ogreRoot     = Application::GetOgreSubsystem()->GetOgreRoot();
	gEnv->viewPort     = Application::GetOgreSubsystem()->GetViewport();
	gEnv->renderWindow = Application::GetOgreSubsystem()->GetRenderWindow();

	Application::CreateAppStateManager();

	Application::CreateContentManager();

	LanguageEngine::getSingleton().setup(); // TODO: Manage by class Application

	// Add startup resources
	Application::GetContentManager()->AddResourcePack(ContentManager::ResourcePack::OGRE_CORE);
	Application::GetContentManager()->AddResourcePack(ContentManager::ResourcePack::GUI_STARTUP_SCREEN);
	Application::GetContentManager()->AddResourcePack(ContentManager::ResourcePack::GUI_MENU_WALLPAPERS);
	Ogre::ResourceGroupManager::getSingleton().initialiseResourceGroup("Bootstrap");
	Ogre::ResourceGroupManager::getSingleton().initialiseResourceGroup("Wallpapers");

	StartupScreen bootstrap_screen;
	bootstrap_screen.InitAndShow();

	Application::GetOgreSubsystem()->GetOgreRoot()->renderOneFrame(); // Render bootstrap screen once and leave it visible.

	Application::GetContentManager()->init(); // Load all resource packs

	// Create application states and launch the default one
	// GameState = default state, classic
	// LobbyState = experimental Multiplayer Lobby
	GameState::create(Application::GetAppStateManager(),  "GameState");
	LobbyState::create(Application::GetAppStateManager(), "LobbyState");
	
	Ogre::String start_state = SSETTING("StartState", "GameState");
	bootstrap_screen.HideAndRemove();
	Application::GetAppStateManager()->start(Application::GetAppStateManager()->findByName(start_state));

	// ================================================================================
	// Cleanup
	// ================================================================================

	Application::DestroyContentManager();

	Application::DestroyAppStateManager();
}

} // namespace RoR