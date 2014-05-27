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
#include "BeamFactory.h"
#include "CacheSystem.h"
#include "Character.h"
#include "CharacterFactory.h"
#include "Console.h"
#include "ContentManager.h"
#include "DustManager.h"
#include "ErrorUtils.h"
#include "GlobalEnvironment.h"
#include "GUIFriction.h"
#include "GUIManager.h"
#include "GUIMenu.h"
#include "GUIMp.h"
#include "InputEngine.h"
#include "Language.h"
#include "LoadingWindow.h"
#include "MumbleIntegration.h"
#include "Network.h"
#include "OgreSubsystem.h"
#include "OverlayWrapper.h"
#include "OutProtocol.h"
#include "RoRFrameListener.h"
#include "ScriptEngine.h"
#include "SelectorWindow.h"
#include "Settings.h"
#include "Skin.h"
#include "StartupScreen.h"
#include "SurveyMapManager.h"
#include "TerrainManager.h"
#include "Utils.h"

#include <OgreRoot.h>
#include <OgreString.h>

// Global instance of GlobalEnvironment used throughout the game.
GlobalEnvironment *gEnv; 

using namespace RoR;
using namespace Ogre; // The _L() macro won't compile without.

MainThread::MainThread():
	m_no_rendering(false),
	m_shutdown_requested(false),
	m_ror_frame_listener(nullptr),
	m_start_time(0),
	m_exit_loop_requested(false)
{
	pthread_mutex_init(&m_lock, nullptr);
}

void MainThread::Go()
{
	// ================================================================================
	// Bootstrap
	// ================================================================================

	gEnv = new GlobalEnvironment(); // Instantiate global environment
	gEnv->main_thread_control = this;

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

	// ================================================================================
	// Terrain selection
	// ================================================================================
	
	Ogre::String preselected_map = SSETTING("Preselected Map", "");
	if (preselected_map.empty())
	{
#ifdef USE_MYGUI
		// show terrain selector
		ShowSurveyMap(false);
		SelectorWindow::getSingleton().show(SelectorWindow::LT_Terrain);
#endif // USE_MYGUI
		EnterMenuLoop(); // TODO: Ddoesn't really make sense without USE_MYGUI
	}
	else
	{
		LOG("Preselected Map: " + (preselected_map));
	}

	// ================================================================================
	// Game
	// ================================================================================

	if (! m_shutdown_requested)
	{
		// ============================================================================
		// Loading map
		// ============================================================================

		Ogre::String map_file_name;
		if (preselected_map.empty())
		{
			CacheEntry* selected_map = SelectorWindow::getSingleton().getSelection();
			if (selected_map != nullptr)
			{
				map_file_name = selected_map->fname;
			}
		}
		else
		{
			if (!RoR::Application::GetCacheSystem()->checkResourceLoaded(preselected_map))
			{
				preselected_map = Ogre::StringUtil::replaceAll(preselected_map, ".terrn2", "");
				preselected_map = Ogre::StringUtil::replaceAll(preselected_map, ".terrn", "");
				preselected_map = preselected_map + ".terrn2";
				// fallback to old terrain name with .terrn
				if (! RoR::Application::GetCacheSystem()->checkResourceLoaded(preselected_map))
				{
					LOG("Terrain not found: " + preselected_map);
					ErrorUtils::ShowError(_L("Terrain loading error"), _L("Terrain not found: ") + preselected_map);
					exit(123);
				}
			}
			// set the terrain cache entry
			CacheEntry ce = RoR::Application::GetCacheSystem()->getResourceInfo(preselected_map);
			map_file_name = preselected_map;
		}
		
		if (! map_file_name.empty())
		{
			LoadTerrain(map_file_name);

			// ========================================================================
			// Loading vehicle
			// ========================================================================

			Ogre::String preselected_truck = SSETTING("Preselected Truck", "");
			Ogre::String preselected_truckConfig = SSETTING("Preselected TruckConfig", "");
			bool enterTruck = (BSETTING("Enter Preselected Truck", false));
			if (preselected_truckConfig != "")
			{
				LOG("Preselected Truck Config: " + (preselected_truckConfig));
			}
			if (! preselected_truck.empty() && preselected_truck != "none")
			{
				LOG("Preselected Truck: " + (preselected_truck));

				// load preselected truck
				const std::vector<Ogre::String> truckConfig = std::vector<Ogre::String>(1, preselected_truckConfig);
				m_ror_frame_listener->loading_state = TERRAIN_LOADED;
				m_ror_frame_listener->initTrucks(true, preselected_truck, "", &truckConfig, enterTruck);
			}
			else if (gEnv->terrainManager->hasPreloadedTrucks())
			{
				Skin* selected_skin = SelectorWindow::getSingleton().getSelectedSkin();
				m_ror_frame_listener->initTrucks(false, map_file_name, "", 0, false, selected_skin);
			}
			else
			{
#ifdef USE_MYGUI
				// show truck selector
				if (gEnv->terrainManager->getWater())
				{
					ShowSurveyMap(false);
					SelectorWindow::getSingleton().show(SelectorWindow::LT_NetworkWithBoat);
				} 
				else
				{
					ShowSurveyMap(false);
					SelectorWindow::getSingleton().show(SelectorWindow::LT_Network);
				}
#endif // USE_MYGUI
			}

			// ========================================================================
			// Game loop
			// ========================================================================

			m_ror_frame_listener->initialized=true;

			EnterGameplayLoop();
		}	
	}

	// ================================================================================
	// Cleanup
	// ================================================================================

	scene_manager->destroyCamera(camera);
    RoR::Application::GetOgreSubsystem()->GetOgreRoot()->destroySceneManager(scene_manager);

	Application::DestroyContentManager();
}

void MainThread::EnterMenuLoop()
{
	// ==== FPS-limiter ====
	// TODO: Is this necessary in menu?

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

	while (!m_exit_loop_requested)
	{
		startTime = RoR::Application::GetOgreSubsystem()->GetTimer()->getMilliseconds();

		// no more actual rendering?
		if (m_no_rendering)
		{
			sleepMilliSeconds(100);
			continue;
		}

		////update(timeSinceLastFrame);
		MenuLoopUpdate(timeSinceLastFrame);
		
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32 || OGRE_PLATFORM == OGRE_PLATFORM_LINUX
		RoRWindowEventUtilities::messagePump();
#endif
		Ogre::RenderWindow* rw = RoR::Application::GetOgreSubsystem()->GetRenderWindow();
		if (rw->isClosed())
		{
			RequestExitCurrentLoop();
			RequestShutdown();
			continue;
		}

		RoR::Application::GetOgreSubsystem()->GetOgreRoot()->renderOneFrame();

		if (!rw->isActive() && rw->isVisible())
			rw->update(); // update even when in background !

		
		// FPS-limiter. TODO: Is this necessary in menu?
		if (fpsLimit && timeSinceLastFrame < minTimePerFrame)
		{
			// Sleep twice as long as we were too fast.
			sleepMilliSeconds((minTimePerFrame - timeSinceLastFrame) << 1);
		}


		timeSinceLastFrame = RoR::Application::GetOgreSubsystem()->GetTimer()->getMilliseconds() - startTime;
	}

	/* CLEANUP */

	m_exit_loop_requested = false;
}

void MainThread::EnterGameplayLoop()
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

	while(!m_shutdown_requested)
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
			m_shutdown_requested = true;
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

void MainThread::RequestShutdown()
{
	m_shutdown_requested = true;
}

void MainThread::RequestExitCurrentLoop()
{
	m_exit_loop_requested = true;
}

void MainThread::MenuLoopUpdate(float seconds_since_last_frame)
{
	if (seconds_since_last_frame==0) 
	{
		return;
	}
	if (seconds_since_last_frame>1.0/20.0) // TODO: Does this have any sense for menu?
	{
		seconds_since_last_frame=1.0/20.0; 
	}

	if (gEnv->renderWindow->isClosed())
	{
		RequestExitCurrentLoop();
		RequestShutdown();
		return;
	}

	if (SelectorWindow::getSingleton().isFinishedSelecting())
	{
		RequestExitCurrentLoop();
	}

	// update GUI
	RoR::Application::GetInputEngine()->Capture();

	// update network gui if required, at most every 2 seconds
	if (gEnv->network)
	{
		m_ror_frame_listener->netcheckGUITimer += seconds_since_last_frame;
		if (m_ror_frame_listener->netcheckGUITimer > 2)
		{
			//// RoRFrameListener::checkRemoteStreamResultsChanged()

#ifdef USE_MYGUI
#ifdef USE_SOCKETW
			if (BeamFactory::getSingleton().checkStreamsResultsChanged())
			{
				GUI_Multiplayer::getSingleton().update();
			}
#endif // USE_SOCKETW
#endif // USE_MYGUI
			m_ror_frame_listener->netcheckGUITimer=0;
		}

#ifdef USE_SOCKETW
#ifdef USE_MYGUI
		// update net quality icon
		if (gEnv->network->getNetQualityChanged())
		{
			GUI_Multiplayer::getSingleton().update();
		}
#endif // USE_MYGUI
#endif // USE_SOCKETW

		// now update mumble 3d audio things
#ifdef USE_MUMBLE
		if (gEnv->player)
		{
			MumbleIntegration::getSingleton().update(gEnv->mainCamera->getPosition(), gEnv->player->getPosition() + Vector3(0, 1.8f, 0));
		}
#endif // USE_MUMBLE
	}

	MenuLoopUpdateEvents(seconds_since_last_frame);

#ifdef USE_ANGELSCRIPT
	ScriptEngine::getSingleton().framestep(seconds_since_last_frame);
#endif

	// update network labels
	if (gEnv->network) // TODO: Does this have any sense for menu?
	{
		CharacterFactory::getSingleton().updateLabels();
	}

}

void MainThread::MenuLoopUpdateEvents(float seconds_since_last_frame)
{
	if (seconds_since_last_frame==0.0f)
	{
		return;
	}

	RoR::Application::GetInputEngine()->updateKeyBounces(seconds_since_last_frame);

	if (! RoR::Application::GetInputEngine()->getInputsChanged())
	{
		return;
	}

	bool dirty = false;

	if (RoR::Application::GetOverlayWrapper() != nullptr)
	{
		RoR::Application::GetOverlayWrapper()->update(seconds_since_last_frame); // TODO: What's the meaning of this? It only updates some internal timer.
	}

#ifdef USE_MYGUI
	if (RoR::Application::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_ENTER_CHATMODE, 0.5f) && !m_ror_frame_listener->hidegui)
	{
		Console *c = RoR::Application::GetConsole();
		if (c)
		{
			RoR::Application::GetInputEngine()->resetKeys();
			c->setVisible(true);
			c->select();
		}
	}
#endif //USE_MYGUI

	if (RoR::Application::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_QUIT_GAME))
	{
		RequestExitCurrentLoop();
		RequestShutdown();
		return;
	}

	// TODO: screenshot

	// FOV settings disabled in menu

	// FIXME: full screen/windowed screen switching
	//if (RoR::Application::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_FULLSCREEN_TOGGLE, 2.0f)) {}

	#ifdef USE_MYGUI
		if (SelectorWindow::getSingleton().isFinishedSelecting())
		{
			
		}
#endif //MYGUI

}

void MainThread::LoadTerrain(Ogre::String const & a_terrain_file)
{
	// check if the resource is loaded
	Ogre::String terrain_file = a_terrain_file;
	if (! RoR::Application::GetCacheSystem()->checkResourceLoaded(terrain_file)) // Input-output argument.
	{
		// fallback for terrains, add .terrn if not found and retry
		terrain_file = Ogre::StringUtil::replaceAll(terrain_file, ".terrn2", "");
		terrain_file = Ogre::StringUtil::replaceAll(terrain_file, ".terrn", "");
		terrain_file = terrain_file + ".terrn2";
		if (!RoR::Application::GetCacheSystem()->checkResourceLoaded(terrain_file))
		{
			LOG("Terrain not found: " + terrain_file);
			ErrorUtils::ShowError(_L("Terrain loading error"), _L("Terrain not found: ") + terrain_file);
			exit(123);
		}
	}

#ifdef USE_MYGUI
	LoadingWindow::getSingleton().setProgress(0, _L("Loading Terrain"));
#endif //USE_MYGUI

	LOG("Loading new terrain format: " + terrain_file);

	if (gEnv->terrainManager != nullptr)
	{
		// remove old terrain
		delete(gEnv->terrainManager);
	}

	gEnv->terrainManager = new TerrainManager();
	gEnv->terrainManager->loadTerrain(terrain_file);

	m_ror_frame_listener->loading_state=TERRAIN_LOADED;
	
	if (gEnv->player != nullptr)
	{
		gEnv->player->setVisible(true);
		gEnv->player->setPosition(gEnv->terrainManager->getSpawnPos());
	}

#ifdef USE_MYGUI

	// hide loading window
	LoadingWindow::getSingleton().hide();
	// hide wallpaper
	MyGUI::Window *w = MyGUI::Gui::getInstance().findWidget<MyGUI::Window>("wallpaper");
	if (w != nullptr)
	{
		w->setVisibleSmooth(false);
	}

#endif //USE_MYGUI
}

void MainThread::ShowSurveyMap(bool be_visible)
{
#ifdef USE_MYGUI
	if (gEnv->surveyMap != nullptr)
	{
		gEnv->surveyMap->setVisibility(be_visible);
	}
#endif //USE_MYGUI
}
