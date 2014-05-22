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
#include "CacheSystem.h"
#include "ContentManager.h"
#include "DustManager.h"
#include "ErrorUtils.h"
#include "GUIFriction.h"
#include "GUIManager.h"
#include "GUIMenu.h"
#include "InputEngine.h"
#include "Language.h"
#include "LoadingWindow.h"
#include "OgreSubsystem.h"
#include "OverlayWrapper.h"
#include "RoRFrameListener.h"
#include "ScriptEngine.h"
#include "SelectorWindow.h"
#include "Settings.h"
#include "StartupScreen.h"
#include "Utils.h"

#include <OgreRoot.h>

// Global instance of GlobalEnvironment used throughout the game.
GlobalEnvironment *gEnv; 

using namespace RoR;

MainThread::MainThread():
	m_no_rendering(false),
	m_shutdown(false),
	m_ror_frame_listener(nullptr)
{
	pthread_mutex_init(&m_lock, nullptr);
}

void MainThread::Go()
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

	RoR::Application::CreateCacheSystem();

	RoR::Application::GetCacheSystem()->setLocation(SSETTING("Cache Path", ""), SSETTING("Config Root", ""));

	Application::GetContentManager()->init(); // Load all resource packs

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

	new DustManager(); // setup particle manager singleton. TODO: Move under Application

	RoR::Application::CreateInputEngine();
	RoR::Application::GetInputEngine()->setupDefault(RoR::Application::GetOgreSubsystem()->GetMainHWND());

	if (BSETTING("regen-cache-only", false))
	{
		RoR::Application::GetCacheSystem()->startup(true); // true = force regeneration
		
		// Get stats
		int num_new     = RoR::Application::GetCacheSystem()->newFiles;
		int num_changed = RoR::Application::GetCacheSystem()->changedFiles;
		int num_deleted = RoR::Application::GetCacheSystem()->deletedFiles;
		
		// Report
		Ogre::UTFString str = _L("Cache regeneration done.\n");
		if (num_new > 0)
		{
			str = str + TOUTFSTRING(num_new) + _L(" new files\n");
		}
		if (num_changed > 0)
		{
			str = str + TOUTFSTRING(num_changed) + _L(" changed files\n");
		}
		if (num_deleted > 0)
		{
			str = str + TOUTFSTRING(num_deleted) + _L(" deleted files\n");
		}
		if (num_new + num_changed + num_deleted == 0)
		{
			str = str + _L("no changes");
		}
		str = str + _L("\n(These stats can be imprecise)");
		ErrorUtils::ShowError(_L("Cache regeneration done"), str);

		exit(0);
	}

	RoR::Application::GetCacheSystem()->startup();

	// --------------------------------------------------------------------------------
	// Continue with legacy GameState + RoRFrameListener

	m_ror_frame_listener = new RoRFrameListener(this);
	gEnv->frameListener = m_ror_frame_listener;
	ScriptEngine::getSingleton().SetFrameListener(m_ror_frame_listener);
	Application::GetOgreSubsystem()->GetOgreRoot()->addFrameListener(m_ror_frame_listener);

	EnterMainLoop();

	// ================================================================================
	// Cleanup
	// ================================================================================

	scene_manager->destroyCamera(camera);
    RoR::Application::GetOgreSubsystem()->GetOgreRoot()->destroySceneManager(scene_manager);

	Application::DestroyContentManager();
}

void MainThread::EnterMainLoop()
{
	unsigned long timeSinceLastFrame = 1;
	unsigned long startTime          = 0;
	unsigned long minTimePerFrame    = 0;
	unsigned long fpsLimit           = ISETTING("FPS-Limiter", 0);

	if (fpsLimit < 10 || fpsLimit >= 200)
	{
		fpsLimit = 0;
	}

	if (fpsLimit)
	{
		minTimePerFrame = 1000 / fpsLimit;
	}

	while(!m_shutdown)
	{
		startTime = RoR::Application::GetOgreSubsystem()->GetTimer()->getMilliseconds();

		// no more actual rendering?
		if (m_no_rendering)
		{
			sleepMilliSeconds(100);
			continue;
		}

		////update(timeSinceLastFrame);
		MUTEX_LOCK(&m_lock);
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32 || OGRE_PLATFORM == OGRE_PLATFORM_LINUX
		RoRWindowEventUtilities::messagePump();
#endif
		Ogre::RenderWindow* rw = RoR::Application::GetOgreSubsystem()->GetRenderWindow();
		if (rw->isClosed())
		{
			// TODO: is this locking circus needed?

			// unlock before shutdown
			MUTEX_UNLOCK(&m_lock);
			// shutdown locks the mutex itself
			
			// shutdown needs to be synced
			MUTEX_LOCK(&m_lock);
			m_shutdown = true;
			printf(">SH\n");
			MUTEX_UNLOCK(&m_lock);
			return;
		}

		RoR::Application::GetOgreSubsystem()->GetOgreRoot()->renderOneFrame();

		if (!rw->isActive() && rw->isVisible())
			rw->update(); // update even when in background !

		MUTEX_UNLOCK(&m_lock);

		if (fpsLimit && timeSinceLastFrame < minTimePerFrame)
		{
			// Sleep twice as long as we were too fast.
			sleepMilliSeconds((minTimePerFrame - timeSinceLastFrame) << 1);
		}


		timeSinceLastFrame = RoR::Application::GetOgreSubsystem()->GetTimer()->getMilliseconds() - startTime;
	}
}

void MainThread::Exit()
{
	RoR::Application::GetOgreSubsystem()->GetOgreRoot()->removeFrameListener(m_ror_frame_listener);
	delete m_ror_frame_listener;
	m_ror_frame_listener = nullptr;
}

void MainThread::Shutdown()
{
	m_shutdown = true;
}
