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
#include "CameraManager.h"
#include "Character.h"
#include "CharacterFactory.h"
#include "ChatSystem.h"
#include "Console.h"
#include "ContentManager.h"
#include "DepthOfFieldEffect.h"
#include "DustManager.h"
#include "ErrorUtils.h"
#include "ForceFeedback.h"
#include "GlobalEnvironment.h"
#include "GUIFriction.h"
#include "GUIManager.h"
#include "GUIMenu.h"
#include "GUIMp.h"
#include "Heathaze.h"
#include "InputEngine.h"
#include "Language.h"
#include "LoadingWindow.h"
#include "MumbleIntegration.h"
#include "Network.h"
#include "OgreSubsystem.h"
#include "OverlayWrapper.h"
#include "OutProtocol.h"
#include "PlayerColours.h"
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
	m_start_time(0),
	m_race_start_time(0),
	m_race_in_progress(false),
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

	Application::CreateContentManager();

	LanguageEngine::getSingleton().setup(); // TODO: Manage by class Application

	// Add startup resources
	RoR::Application::GetContentManager()->AddResourcePack(ContentManager::ResourcePack::OGRE_CORE);
	RoR::Application::GetContentManager()->AddResourcePack(ContentManager::ResourcePack::GUI_STARTUP_SCREEN);
	RoR::Application::GetContentManager()->AddResourcePack(ContentManager::ResourcePack::GUI_MENU_WALLPAPERS);
	Ogre::ResourceGroupManager::getSingleton().initialiseResourceGroup("Bootstrap");
	Ogre::ResourceGroupManager::getSingleton().initialiseResourceGroup("Wallpapers");

	StartupScreen bootstrap_screen;
	bootstrap_screen.InitAndShow();

	Application::GetOgreSubsystem()->GetOgreRoot()->renderOneFrame(); // Render bootstrap screen once and leave it visible.

	RoR::Application::CreateCacheSystem();

	RoR::Application::GetCacheSystem()->setLocation(SSETTING("Cache Path", ""), SSETTING("Config Root", ""));

	Application::GetContentManager()->init(); // Load all resource packs

	bootstrap_screen.HideAndRemove();

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

#ifdef USE_MYGUI
	
	Application::CreateGuiManagerIfNotExists();

	// create console, must be done early
	Application::CreateConsoleIfNotExists();

	// Init singletons. TODO: Move under Application
	LoadingWindow::getSingleton();
	SelectorWindow::getSingleton();
	new GUI_MainMenu();
	GUI_Friction::getSingleton();

	MyGUI::VectorWidgetPtr v = MyGUI::LayoutManager::getInstance().loadLayout("wallpaper.layout");
	if (!v.empty())
	{
		MyGUI::Widget *mainw = v.at(0);
		if (mainw)
		{
			MyGUI::ImageBox *img = (MyGUI::ImageBox *)(mainw->getChildAt(0));
			if (img) img->setImageTexture(bootstrap_screen.GetWallpaperTextureName());
		}
	}

#endif // USE_MYGUI

#ifdef USE_ANGELSCRIPT

	new ScriptEngine(); // Init singleton. TODO: Move under Application

#endif

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

	// Create legacy RoRFrameListener

	gEnv->frameListener = new RoRFrameListener();
	ScriptEngine::getSingleton().SetFrameListener(gEnv->frameListener);

	gEnv->frameListener->enablePosStor = BSETTING("Position Storage", false);

#ifdef USE_MPLATFORM
	gEnv->frameListener->mplatform = new MPlatform_FD();
	if (gEnv->frameListener->mplatform) 
	{
		mplatform->connect();
	}
#endif

	// setup direction arrow overlay
	gEnv->frameListener->dirvisible = false;
	gEnv->frameListener->dirArrowPointed = Vector3::ZERO;

	gEnv->frameListener->windowResized(RoR::Application::GetOgreSubsystem()->GetRenderWindow());
	RoRWindowEventUtilities::addWindowEventListener(RoR::Application::GetOgreSubsystem()->GetRenderWindow(), gEnv->frameListener);

	// get lights mode
	String lightsMode = SSETTING("Lights", "Only current vehicle, main lights");
	if (lightsMode == "None (fastest)")
		gEnv->frameListener->flaresMode = 0;
	else if (lightsMode == "No light sources")
		gEnv->frameListener->flaresMode = 1;
	else if (lightsMode == "Only current vehicle, main lights")
		gEnv->frameListener->flaresMode = 2;
	else if (lightsMode == "All vehicles, main lights")
		gEnv->frameListener->flaresMode = 3;
	else if (lightsMode == "All vehicles, all lights")
		gEnv->frameListener->flaresMode = 4;

	// force feedback
	if (BSETTING("Force Feedback", true))
	{
		//check if a device has been detected
		if (RoR::Application::GetInputEngine()->getForceFeedbackDevice())
		{
			//retrieve gain values
			float ogain   = FSETTING("Force Feedback Gain",      100) / 100.0f;
			float stressg = FSETTING("Force Feedback Stress",    100) / 100.0f;
			float centg   = FSETTING("Force Feedback Centering", 0  ) / 100.0f;
			float camg    = FSETTING("Force Feedback Camera",    100) / 100.0f;

			gEnv->frameListener->forcefeedback = new ForceFeedback(RoR::Application::GetInputEngine()->getForceFeedbackDevice(), ogain, stressg, centg, camg);
		}
	}

	String screenshotFormatString = SSETTING("Screenshot Format", "jpg (smaller, default)");
	if     (screenshotFormatString == "jpg (smaller, default)")
		strcpy(gEnv->frameListener->screenshotformat, "jpg");
	else if (screenshotFormatString =="png (bigger, no quality loss)")
		strcpy(gEnv->frameListener->screenshotformat, "png");
	else
		strncpy(gEnv->frameListener->screenshotformat, screenshotFormatString.c_str(), 10);

	// check command line args
	String cmd = SSETTING("cmdline CMD", "");
	String cmdAction = "";
	String cmdServerIP = "";
	String modName = "";
	long cmdServerPort = 0;
	Vector3 spawnLocation = Vector3::ZERO;
	if (!cmd.empty())
	{
		StringVector str = StringUtil::split(cmd, "/");
		// process args now
		for (StringVector::iterator it = str.begin(); it!=str.end(); it++)
		{

			String argstr = *it;
			StringVector args = StringUtil::split(argstr, ":");
			if (args.size()<2) continue;
			if (args[0] == "action" && args.size() == 2) cmdAction = args[1];
			if (args[0] == "serverpass" && args.size() == 2) SETTINGS.setSetting("Server password", args[1]);
			if (args[0] == "modname" && args.size() == 2) modName = args[1];
			if (args[0] == "ipport" && args.size() == 3)
			{
				cmdServerIP = args[1];
				cmdServerPort = StringConverter::parseLong(args[2]);
			}
			if (args[0] == "loc" && args.size() == 4)
			{
				spawnLocation = Vector3(StringConverter::parseInt(args[1]), StringConverter::parseInt(args[2]), StringConverter::parseInt(args[3]));
				SETTINGS.setSetting("net spawn location", TOSTRING(spawnLocation));
			}
		}
	}

	if (cmdAction == "regencache") SETTINGS.setSetting("regen-cache-only", "Yes");
	if (cmdAction == "installmod")
	{
		// use modname!
	}
	bool enable_network = BSETTING("Network enable", false);
	// check if we enable netmode based on cmdline
	if (!enable_network && cmdAction == "joinserver")
		enable_network = true;

	String preselected_map = SSETTING("Preselected Map", "");

	// initiate player colours
	PlayerColours::getSingleton();

	// you always need that, even if you are not using the network
	NetworkStreamManager::getSingleton();

	// new factory for characters, net is INVALID, will be set later
	new CharacterFactory();
	new ChatSystemFactory();

	// notice: all factories must be available before starting the network!
#ifdef USE_SOCKETW
	
	if (enable_network)
	{
		// cmdline overrides config
		std::string server_name = SSETTING("Server name", "").c_str();
		if (cmdAction == "joinserver" && !cmdServerIP.empty())
			server_name = cmdServerIP;

		long server_port = ISETTING("Server port", 1337);
		if (cmdAction == "joinserver" && cmdServerPort)
			server_port = cmdServerPort;

		if (server_port==0)
		{
			ErrorUtils::ShowError(_L("A network error occured"), _L("Bad server port"));
			exit(123);
			return;
		}
		LOG("trying to join server '" + String(server_name) + "' on port " + TOSTRING(server_port) + "'...");

#ifdef USE_MYGUI
		LoadingWindow::getSingleton().setAutotrack(_L("Trying to connect to server ..."));
#endif // USE_MYGUI
		// important note: all new network code is written in order to allow also the old network protocol to further exist.
		// at some point you need to decide with what type of server you communicate below and choose the correct class

		gEnv->network = new Network(server_name, server_port, gEnv->frameListener);

		bool connres = gEnv->network->connect();
#ifdef USE_MYGUI
		LoadingWindow::getSingleton().hide();

#ifdef USE_SOCKETW
		new GUI_Multiplayer();
		GUI_Multiplayer::getSingleton().update();
#endif //USE_SOCKETW

#endif //USE_MYGUI
		if (!connres)
		{
			LOG("connection failed. server down?");
			ErrorUtils::ShowError(_L("Unable to connect to server"), _L("Unable to connect to the server. It is certainly down or you have network problems."));
			//fatal
			exit(1);
		}
		char *terrn = gEnv->network->getTerrainName();
		bool isAnyTerrain = (terrn && !strcmp(terrn, "any"));
		if (preselected_map.empty() && isAnyTerrain)
		{
			// so show the terrain selection
			preselected_map = "";
		} else if (!isAnyTerrain)
		{
			preselected_map = getASCIIFromCharString(terrn, 255);
		}

		// create player _AFTER_ network, important
		int colourNum = 0;
		if (gEnv->network->getLocalUserData()) colourNum = gEnv->network->getLocalUserData()->colournum;
		gEnv->player = (Character *)CharacterFactory::getSingleton().createLocal(colourNum);

		// network chat stuff
		gEnv->frameListener->netChat = ChatSystemFactory::getSingleton().createLocal(colourNum);

#ifdef USE_MYGUI
		Console *c = RoR::Application::GetConsole();
		if (c)
		{
			c->setVisible(true);
			c->setNetChat(gEnv->frameListener->netChat);
			wchar_t tmp[255] = L"";
			UTFString format = _L("Press %ls to start chatting");
			swprintf(tmp, 255, format.asWStr_c_str(), ANSI_TO_WCHAR(RoR::Application::GetInputEngine()->getKeyForCommand(EV_COMMON_ENTER_CHATMODE)).c_str());
			c->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_HELP, UTFString(tmp), "information.png");
		}
#endif //USE_MYGUI

#ifdef USE_MUMBLE
		new MumbleIntegration();
#endif // USE_MUMBLE

	} 
#endif //SOCKETW	

	// new beam factory
	new BeamFactory();

	// ================================================================================
	// Terrain selection
	// ================================================================================
	
	if (preselected_map.empty())
	{
#ifdef USE_MYGUI
		// show terrain selector
		ShowSurveyMap(false);
		SelectorWindow::getSingleton().show(SelectorWindow::LT_Terrain);
#endif // USE_MYGUI

		EnterMenuLoop(); // TODO: Doesn't really make sense without USE_MYGUI
	}
	else
	{
		LOG("Preselected Map: " + (preselected_map));
	}

	CacheEntry* selected_map = SelectorWindow::getSingleton().getSelection();
	if (selected_map == nullptr)
	{
		RequestShutdown();
	}

	// ================================================================================
	// Game
	// ================================================================================

	if (! m_shutdown_requested)
	{
		// ============================================================================
		// Loading base resources
		// ============================================================================

		LoadingWindow::getSingleton().setProgress(0, _L("Loading base resources"));

		RoR::Application::GetContentManager()->AddResourcePack(ContentManager::ResourcePack::AIRFOILS);
		RoR::Application::GetContentManager()->AddResourcePack(ContentManager::ResourcePack::BEAM_OBJECTS);
		RoR::Application::GetContentManager()->AddResourcePack(ContentManager::ResourcePack::MATERIALS);
		RoR::Application::GetContentManager()->AddResourcePack(ContentManager::ResourcePack::MESHES);
		RoR::Application::GetContentManager()->AddResourcePack(ContentManager::ResourcePack::OVERLAYS);
		RoR::Application::GetContentManager()->AddResourcePack(ContentManager::ResourcePack::PARTICLES);
		RoR::Application::GetContentManager()->AddResourcePack(ContentManager::ResourcePack::SCRIPTS);
		RoR::Application::GetContentManager()->AddResourcePack(ContentManager::ResourcePack::TEXTURES);
		RoR::Application::GetContentManager()->AddResourcePack(ContentManager::ResourcePack::FLAGS);
		RoR::Application::GetContentManager()->AddResourcePack(ContentManager::ResourcePack::ICONS);
		RoR::Application::GetContentManager()->AddResourcePack(ContentManager::ResourcePack::FAMICONS);
		RoR::Application::GetContentManager()->AddResourcePack(ContentManager::ResourcePack::HYDRAX);

		if (SSETTING("Sky effects", "Caelum (best looking, slower)") == "Caelum (best looking, slower)")
			RoR::Application::GetContentManager()->AddResourcePack(ContentManager::ResourcePack::CAELUM);

		if (SSETTING("Vegetation", "None (fastest)") != "None (fastest)")
			RoR::Application::GetContentManager()->AddResourcePack(ContentManager::ResourcePack::PAGED);

		if (BSETTING("HDR", false))
			RoR::Application::GetContentManager()->AddResourcePack(ContentManager::ResourcePack::HDR);

		if (BSETTING("DOF", false))
			RoR::Application::GetContentManager()->AddResourcePack(ContentManager::ResourcePack::DEPTH_OF_FIELD);

		if (BSETTING("Glow", false))
			RoR::Application::GetContentManager()->AddResourcePack(ContentManager::ResourcePack::GLOW);

		if (BSETTING("Motion blur", false))
			RoR::Application::GetContentManager()->AddResourcePack(ContentManager::ResourcePack::BLUR);

		if (BSETTING("HeatHaze", false))
			RoR::Application::GetContentManager()->AddResourcePack(ContentManager::ResourcePack::HEATHAZE);

		if (BSETTING("Sunburn", false))
			RoR::Application::GetContentManager()->AddResourcePack(ContentManager::ResourcePack::SUNBURN);

		if (SSETTING("Shadow technique", "") == "Parallel-split Shadow Maps")
			RoR::Application::GetContentManager()->AddResourcePack(ContentManager::ResourcePack::PSSM);

		Ogre::ResourceGroupManager::getSingleton().initialiseResourceGroup("LoadBeforeMap");

		// ============================================================================
		// Setup
		// ============================================================================

		Application::CreateOverlayWrapper();
		Application::GetOverlayWrapper()->SetupDirectionArrow();

		new DustManager(); // setup particle manager singleton. TODO: Move under Application

		if (! enable_network)
		{
			gEnv->player = (Character *)CharacterFactory::getSingleton().createLocal(-1);
			if (gEnv->player != nullptr)
			{
				gEnv->player->setVisible(false);
			}
		}

		// heathaze effect
		if (BSETTING("HeatHaze", false))
		{
			gEnv->frameListener->heathaze = new HeatHaze();
			gEnv->frameListener->heathaze->setEnable(true);
		}

		// depth of field effect
		if (BSETTING("DOF", false))
		{
			gEnv->frameListener->dof = new DOFManager();
		}

		// init camera manager after mygui and after we have a character
		new CameraManager(gEnv->frameListener->dof);

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
				gEnv->frameListener->loading_state = TERRAIN_LOADED;
				gEnv->frameListener->initTrucks(true, preselected_truck, "", &truckConfig, enterTruck);
			}
			else if (gEnv->terrainManager->hasPreloadedTrucks())
			{
				Skin* selected_skin = SelectorWindow::getSingleton().getSelectedSkin();
				gEnv->frameListener->initTrucks(false, map_file_name, "", 0, false, selected_skin);
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

			Application::CreateSceneMouse();
			gEnv->frameListener->initialized = true;
			Application::GetOgreSubsystem()->GetOgreRoot()->addFrameListener(gEnv->frameListener);

			EnterGameplayLoop();
		}	
	}

	// ================================================================================
	// Cleanup
	// ================================================================================

#ifdef USE_MYGUI
	LoadingWindow::freeSingleton();
	SelectorWindow::freeSingleton();
#endif //MYGUI

#ifdef USE_SOCKETW
	if (gEnv->network) delete (gEnv->network);
#endif //SOCKETW

	//TODO: we should destroy OIS here
	//TODO: we could also try to destroy SoundScriptManager, but we don't care!

#ifdef USE_MPLATFORM
	if (gEnv->frameListener->mplatform != nullptr)
	{
		if (gEnv->frameListener->mplatform->disconnect())
		{
			delete(gEnv->frameListener->mplatform);
			gEnv->frameListener->mplatform = nullptr;
		}
	}
#endif

	scene_manager->destroyCamera(camera);
    RoR::Application::GetOgreSubsystem()->GetOgreRoot()->destroySceneManager(scene_manager);

	Application::DestroyContentManager();

	delete gEnv->frameListener;
	gEnv->frameListener = nullptr;

	delete gEnv;
	gEnv = nullptr;
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
	RoR::Application::GetOgreSubsystem()->GetOgreRoot()->removeFrameListener(gEnv->frameListener);
	delete gEnv->frameListener;
	gEnv->frameListener = nullptr;
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

	if (RoR::Application::GetOgreSubsystem()->GetRenderWindow()->isClosed())
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
		gEnv->frameListener->netcheckGUITimer += seconds_since_last_frame;
		if (gEnv->frameListener->netcheckGUITimer > 2)
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
			gEnv->frameListener->netcheckGUITimer=0;
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
		RoR::Application::GetOverlayWrapper()->update(seconds_since_last_frame); // TODO: What's the meaning of this? It only updates some internal timer. ~ only_a_ptr
	}

#ifdef USE_MYGUI
	if (RoR::Application::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_ENTER_CHATMODE, 0.5f) && !gEnv->frameListener->hidegui)
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

	gEnv->frameListener->loading_state=TERRAIN_LOADED;
	
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

void MainThread::StartRaceTimer()
{
	m_race_start_time = RoR::Application::GetOgreSubsystem()->GetTimer()->getMilliseconds();
	m_race_in_progress = true;
	OverlayWrapper* ow = RoR::Application::GetOverlayWrapper();
	if (ow)
	{
		ow->racing->show();
		ow->laptimes->show();
		ow->laptimems->show();
		ow->laptimemin->show();
	}
}

float MainThread::StopRaceTimer()
{
	float time = static_cast<float>(RoR::Application::GetOgreSubsystem()->GetTimer()->getMilliseconds() - m_race_start_time);
	// let the display on
	OverlayWrapper* ow = RoR::Application::GetOverlayWrapper();
	if (ow)
	{
		wchar_t txt[256] = L"";
		UTFString fmt = _L("Last lap: %.2i'%.2i.%.2i");
		swprintf(txt, 256, fmt.asWStr_c_str(), ((int)(time))/60,((int)(time))%60, ((int)(time*100.0))%100);
		ow->lasttime->setCaption(UTFString(txt));
		//ow->racing->hide();
		ow->laptimes->hide();
		ow->laptimems->hide();
		ow->laptimemin->hide();
	}
	m_race_start_time = 0;
	m_race_in_progress = false;
	return time;
}

void MainThread::UpdateRacingGui()
{
	OverlayWrapper* ow = RoR::Application::GetOverlayWrapper();
	if (!ow) return;
	// update racing gui if required
	float time = static_cast<float>(RoR::Application::GetOgreSubsystem()->GetTimer()->getMilliseconds() - m_race_start_time);
	wchar_t txt[10];
	swprintf(txt, 10, L"%.2i", ((int)(time*100.0))%100);
	ow->laptimems->setCaption(txt);
	swprintf(txt, 10, L"%.2i", ((int)(time))%60);
	ow->laptimes->setCaption(txt);
	swprintf(txt, 10, L"%.2i'", ((int)(time))/60);
	ow->laptimemin->setCaption(UTFString(txt));
}
