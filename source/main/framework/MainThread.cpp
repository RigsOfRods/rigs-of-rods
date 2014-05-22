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
#include "GUIFriction.h"
#include "GUIManager.h"
#include "GUIMenu.h"
#include "Language.h"
#include "LoadingWindow.h"
#include "OgreSubsystem.h"
#include "OverlayWrapper.h"
#include "RoRFrameListener.h"
#include "ScriptEngine.h"
#include "SelectorWindow.h"
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
	GameState* legacy_game_state = static_cast<GameState*>(Application::GetAppStateManager()->findByName("GameState"));

	bootstrap_screen.HideAndRemove();

	// --------------------------------------------------------------------------------
	// Ported GameState logic

	Ogre::SceneManager* scene_manager = RoR::Application::GetOgreSubsystem()->GetOgreRoot()->createSceneManager(Ogre::ST_EXTERIOR_CLOSE);
	gEnv->sceneManager = scene_manager;

	Ogre::Camera* camera = scene_manager->createCamera("PlayerCam");
	camera->setPosition(Ogre::Vector3(128,25,128)); // Position it at 500 in Z direction
	camera->lookAt(Ogre::Vector3(0,0,-300)); // Look back along -Z
	camera->setNearClipDistance( 0.5 );
	camera->setFarClipDistance( 1000.0*1.733 );
	camera->setFOVy(Ogre::Degree(60));
	camera->setAutoAspectRatio(true);
	RoR::Application::GetOgreSubsystem()->GetViewport()->setCamera(camera);
	gEnv->mainCamera = camera;

	// --------------------------------------------------------------------------------
	// Ported RoRFrameListener logic

	Application::CreateOverlayWrapper();

#ifdef USE_MYGUI
	
	Application::CreateSceneMouseIfNotExists();
	Application::CreateGuiManagerIfNotExists();

	// create console, must be done early
	Application::CreateConsoleIfNotExists();

	// Init singletons. TODO: Move under Application
	LoadingWindow::getSingleton();
	SelectorWindow::getSingleton();
	new GUI_MainMenu();
	GUI_Friction::getSingleton();

	MyGUI::VectorWidgetPtr v = MyGUI::LayoutManager::getInstance().loadLayout("wallpaper.layout");
	// load random image in the wallpaper
	Ogre::String randomWallpaper = RoR::GUIManager::getRandomWallpaperImage();
	if (!v.empty() && !randomWallpaper.empty())
	{
		MyGUI::Widget *mainw = v.at(0);
		if (mainw)
		{
			MyGUI::ImageBox *img = (MyGUI::ImageBox *)(mainw->getChildAt(0));
			if (img) img->setImageTexture(randomWallpaper);
		}
	}

#endif // USE_MYGUI

#ifdef USE_ANGELSCRIPT

	new ScriptEngine(); // Init singleton. TODO: Move under Application

#endif

	Application::GetOverlayWrapper()->SetupDirectionArrow();

	// --------------------------------------------------------------------------------
	// Continue with legacy GameState + RoRFrameListener

	RoRFrameListener* ror_frame_listener = new RoRFrameListener(legacy_game_state, RoR::Application::GetOgreSubsystem()->GetMainHWND());
	gEnv->frameListener = ror_frame_listener;
	ScriptEngine::getSingleton().SetFrameListener(ror_frame_listener);
	Application::GetOgreSubsystem()->GetOgreRoot()->addFrameListener(ror_frame_listener);
	
	legacy_game_state->Setup(camera, scene_manager, ror_frame_listener);
	Application::GetAppStateManager()->start(legacy_game_state);

	// ================================================================================
	// Cleanup
	// ================================================================================

	scene_manager->destroyCamera(camera);
    RoR::Application::GetOgreSubsystem()->GetOgreRoot()->destroySceneManager(scene_manager);

	Application::DestroyContentManager();

	Application::DestroyAppStateManager();
}

} // namespace RoR