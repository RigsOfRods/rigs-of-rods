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
#include "Beam.h"
#include "BeamEngine.h"
#include "BeamFactory.h"
#include "CacheSystem.h"
#include "CameraManager.h"
#include "Character.h"
#include "CharacterFactory.h"
#include "ChatSystem.h"
#include "Console.h"
#include "ContentManager.h"
#include "DashBoardManager.h"
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
#include "Scripting.h"
#include "Settings.h"
#include "Skin.h"
#include "SoundScriptManager.h"
#include "SurveyMapManager.h"
#include "TerrainManager.h"
#include "TruckHUD.h"
#include "Utils.h"
#include "SkyManager.h"

#include <OgreRoot.h>
#include <OgreString.h>

#ifdef USE_ANGELSCRIPT
#    include "ScriptEngine.h"
#endif

// Global instance of GlobalEnvironment used throughout the game.
GlobalEnvironment *gEnv; 

using namespace RoR;
using namespace Ogre; // The _L() macro won't compile without.

MainThread::MainThread():
	m_no_rendering(false),
	m_shutdown_requested(false),
	m_restart_requested(false),
	m_start_time(0),
	m_exit_loop_requested(false),
	m_base_resource_loaded(false),
	m_application_state(Application::STATE_NONE),
	m_next_application_state(Application::STATE_NONE)
{
	RoR::Application::SetMainThreadLogic(this);
}

void MainThread::Go()
{
	// ================================================================================
	// Bootstrap
	// ================================================================================

	m_application_state = Application::STATE_BOOTSTRAP;

	gEnv = new GlobalEnvironment(); // Instantiate global environment

	if (! Application::GetSettings().setupPaths() )
	{
		throw std::runtime_error("[RoR] MainThread::go(): Failed to setup file paths");
	}

	Application::StartOgreSubsystem();
#ifdef ROR_USE_OGRE_1_9
	Ogre::OverlaySystem* overlay_system = new OverlaySystem(); //Overlay init
#endif

	Application::CreateContentManager();

	LanguageEngine::getSingleton().setup();

	// Add startup resources
	RoR::Application::GetContentManager()->AddResourcePack(ContentManager::ResourcePack::OGRE_CORE);
	RoR::Application::GetContentManager()->AddResourcePack(ContentManager::ResourcePack::GUI_STARTUP_SCREEN);
	RoR::Application::GetContentManager()->AddResourcePack(ContentManager::ResourcePack::GUI_MENU_WALLPAPERS);
	Ogre::ResourceGroupManager::getSingleton().initialiseResourceGroup("Bootstrap");
	Ogre::ResourceGroupManager::getSingleton().initialiseResourceGroup("Wallpapers");

	// Setup rendering (menu + simulation)
	Ogre::SceneManager* scene_manager = RoR::Application::GetOgreSubsystem()->GetOgreRoot()->createSceneManager(Ogre::ST_EXTERIOR_CLOSE, "main_scene_manager");
	gEnv->sceneManager = scene_manager;
#ifdef ROR_USE_OGRE_1_9
	if (overlay_system) {
		scene_manager->addRenderQueueListener(overlay_system);
	}
#endif

	Ogre::Camera* camera = scene_manager->createCamera("PlayerCam");
	camera->setPosition(Ogre::Vector3(128,25,128)); // Position it at 500 in Z direction
	camera->lookAt(Ogre::Vector3(0,0,-300)); // Look back along -Z
	camera->setNearClipDistance( 0.5 );
	camera->setFarClipDistance( 1000.0*1.733 );
	camera->setFOVy(Ogre::Degree(60));
	camera->setAutoAspectRatio(true);
	RoR::Application::GetOgreSubsystem()->GetViewport()->setCamera(camera);
	gEnv->mainCamera = camera;

	// SHOW BOOTSTRAP SCREEN

	// Create rendering overlay
	Ogre::OverlayManager& overlay_manager = Ogre::OverlayManager::getSingleton();
	Ogre::Overlay* startup_screen_overlay = static_cast<Ogre::Overlay*>( overlay_manager.getByName("RoR/StartupScreen") );
	if (!startup_screen_overlay)
	{
		OGRE_EXCEPT(Ogre::Exception::ERR_ITEM_NOT_FOUND, "Cannot find loading overlay for startup screen", "MainThread::Go");
	}

	// Set random wallpaper image
	//is this still needed? As things load so fast that it's rendred for a fraction of a second.
	Ogre::MaterialPtr mat = Ogre::MaterialManager::getSingleton().getByName("RoR/StartupScreenWallpaper");
	Ogre::String menu_wallpaper_texture_name = GUIManager::getRandomWallpaperImage(); // TODO: manage by class Application
	if (! menu_wallpaper_texture_name.empty() && ! mat.isNull())
	{
		if (mat->getNumTechniques() > 0 && mat->getTechnique(0)->getNumPasses() > 0 && mat->getTechnique(0)->getPass(0)->getNumTextureUnitStates() > 0)
		{
			mat->getTechnique(0)->getPass(0)->getTextureUnitState(0)->setTextureName(menu_wallpaper_texture_name);
		}
	}

	startup_screen_overlay->show();
	
	scene_manager->clearSpecialCaseRenderQueues();
	scene_manager->addSpecialCaseRenderQueue(Ogre::RENDER_QUEUE_OVERLAY);
	scene_manager->setSpecialCaseRenderQueueMode(Ogre::SceneManager::SCRQM_INCLUDE);

	Application::GetOgreSubsystem()->GetOgreRoot()->renderOneFrame(); // Render bootstrap screen once and leave it visible.

	RoR::Application::CreateCacheSystem();

	RoR::Application::GetCacheSystem()->setLocation(SSETTING("Cache Path", ""), SSETTING("Config Root", ""));

	Application::GetContentManager()->init();

	// HIDE BOOTSTRAP SCREEN

	startup_screen_overlay->hide();

	// Back to full rendering
	scene_manager->clearSpecialCaseRenderQueues();
	scene_manager->setSpecialCaseRenderQueueMode(Ogre::SceneManager::SCRQM_EXCLUDE);
	
	Application::CreateGuiManagerIfNotExists();

	// create console, must be done early
	Application::CreateConsoleIfNotExists();

	// Load and show menu wallpaper
	MyGUI::VectorWidgetPtr v = MyGUI::LayoutManager::getInstance().loadLayout("wallpaper.layout");
	MyGUI::Widget* menu_wallpaper_widget = nullptr;
	if (!v.empty())
	{
		MyGUI::Widget *mainw = v.at(0);
		if (mainw)
		{
			MyGUI::ImageBox *img = (MyGUI::ImageBox *)(mainw->getChildAt(0));
			if (img) img->setImageTexture(menu_wallpaper_texture_name);
			menu_wallpaper_widget = mainw;
		}
	}

#ifdef USE_ANGELSCRIPT

	new ScriptEngine(); // Init singleton. TODO: Move under Application

#endif

	RoR::Application::CreateInputEngine();
	RoR::Application::GetInputEngine()->setupDefault(RoR::Application::GetOgreSubsystem()->GetMainHWND());

	// Initialize "managed materials"
	// These are base materials referenced by user content
	// They must be initialized before any content is loaded, including mod-cache update.
	// Otherwise material links are unresolved and loading ends with an exception
	// TODO: Study Ogre::ResourceLoadingListener and implement smarter solution (not parsing materials on cache refresh!)
	Application::GetContentManager()->InitManagedMaterials();

	if (BSETTING("regen-cache-only", false)) //Can be usefull so we will leave it here -max98
		MainThread::RegenCache(); 

	RoR::Application::GetCacheSystem()->Startup();

	// Init singletons. TODO: Move under Application
	LoadingWindow::getSingleton();
	RoR::Application::GetGuiManager()->InitMainSelector(Application::GetContentManager()->GetSkinManager());
	GUI_Friction::getSingleton();

	// Create legacy RoRFrameListener

	gEnv->frameListener = new RoRFrameListener();

#ifdef USE_ANGELSCRIPT
	ScriptEngine::getSingleton().SetFrameListener(gEnv->frameListener);
#endif

#ifdef USE_MPLATFORM
	gEnv->frameListener->m_platform = new MPlatform_FD();
	if (gEnv->frameListener->m_platform) 
	{
		m_platform->connect();
	}
#endif

	new GUI_MainMenu(Application::GetGuiManagerInterface()); /* Top menubar */
	gEnv->frameListener->windowResized(RoR::Application::GetOgreSubsystem()->GetRenderWindow());
	RoRWindowEventUtilities::addWindowEventListener(RoR::Application::GetOgreSubsystem()->GetRenderWindow(), gEnv->frameListener);

#ifdef _WIN32
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

			gEnv->frameListener->m_forcefeedback = new ForceFeedback(RoR::Application::GetInputEngine()->getForceFeedbackDevice(), ogain, stressg, centg, camg);
		}
	}
#endif // _WIN32

	String screenshotFormatString = SSETTING("Screenshot Format", "jpg (smaller, default)");
	if     (screenshotFormatString == "jpg (smaller, default)")
		strcpy(gEnv->frameListener->m_screenshot_format, "jpg");
	else if (screenshotFormatString =="png (bigger, no quality loss)")
		strcpy(gEnv->frameListener->m_screenshot_format, "png");
	else
		strncpy(gEnv->frameListener->m_screenshot_format, screenshotFormatString.c_str(), 10);

	bool enable_network = BSETTING("Network enable", false);

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
		RoR::Application::GetContentManager()->AddResourcePack(ContentManager::ResourcePack::MESHES);
		RoR::Application::GetContentManager()->AddResourcePack(ContentManager::ResourcePack::MATERIALS);

		std::string server_name = SSETTING("Server name", "").c_str();

		long server_port = ISETTING("Server port", 1337);

		if (server_port==0)
		{
			ErrorUtils::ShowError(_L("A network error occured"), _L("Bad server port"));
			exit(123);
			return;
		}
		LOG("trying to join server '" + String(server_name) + "' on port " + TOSTRING(server_port) + "'...");

		LoadingWindow::getSingleton().setAutotrack(_L("Trying to connect to server ..."));

		// important note: all new network code is written in order to allow also the old network protocol to further exist.
		// at some point you need to decide with what type of server you communicate below and choose the correct class

		gEnv->network = new Network(server_name, server_port);

		bool connres = gEnv->network->connect();

		LoadingWindow::getSingleton().hide();

		new GUI_Multiplayer();
		GUI_Multiplayer::getSingleton().update();

		if (!connres)
		{
			LOG("connection failed. server down?");
			ErrorUtils::ShowError(_L("Unable to connect to server"), _L("Unable to connect to the server. It may be offline or you have network problems."));
			//fatal
			exit(1);
		}
		char *terrn = gEnv->network->getTerrainName();
		bool isAnyTerrain = (terrn && !strcmp(terrn, "any"));
		if (preselected_map.empty() && isAnyTerrain)
		{
			// so show the terrain selection
			preselected_map = "";
		} 
		else if (!isAnyTerrain)
		{
			preselected_map = getASCIIFromCharString(terrn, 255);
		}

		// --------------------------------------------------------------------
		// network chat stuff
		int colourNum = 0;
		if (gEnv->network->getLocalUserData())
		{
			colourNum = gEnv->network->getLocalUserData()->colournum;
		}

		ChatSystem* net_chat = ChatSystemFactory::getSingleton().createLocal(colourNum);

		// TODO: separate console and chatbox.

		Application::GetGuiManager()->SetNetChat(net_chat);
		wchar_t tmp[255] = L"";
		UTFString format = _L("Press %ls to start chatting");
		swprintf(tmp, 255, format.asWStr_c_str(), ANSI_TO_WCHAR(RoR::Application::GetInputEngine()->getKeyForCommand(EV_COMMON_ENTER_CHATMODE)).c_str());
		Application::GetGuiManager()->pushMessageChatBox(UTFString(tmp));

#ifdef USE_MUMBLE
		new MumbleIntegration();
#endif // USE_MUMBLE

	} 
#endif //SOCKETW	

	new BeamFactory();
	// ========================================================================
	// Main loop (switches application states)
	// ========================================================================

	Application::State previous_application_state(m_application_state);
	m_next_application_state = Application::STATE_MAIN_MENU;
	if (! preselected_map.empty())
	{
		LOG("Preselected Map: " + (preselected_map));
		m_next_application_state = Application::STATE_SIMULATION;
	}
	m_base_resource_loaded = false;
	while (! m_shutdown_requested)
	{
		if (m_next_application_state == Application::STATE_MAIN_MENU)
		{
			// ================================================================
			// Main menu
			// ================================================================

			m_application_state = Application::STATE_MAIN_MENU;
			m_next_application_state = Application::STATE_MAIN_MENU;

			OgreSubsystem* ror_ogre_subsystem = RoR::Application::GetOgreSubsystem();
			assert(ror_ogre_subsystem != nullptr);

			if (previous_application_state == Application::STATE_SIMULATION)
			{
				Application::GetGuiManager()->killSimUtils();
				UnloadTerrain();
				m_base_resource_loaded = true;
				gEnv->cameraManager->OnReturnToMainMenu();
				/* Hide top menu */
				GUI_MainMenu* top_menu = GUI_MainMenu::getSingletonPtr();
				if (top_menu != nullptr)
				{
					top_menu->setVisible(false);
				}
				/* Restore wallpaper */
				menu_wallpaper_widget->setVisible(true);

				/* Set Mumble to non-positional audio */
				#ifdef USE_MUMBLE
					  MumbleIntegration::getSingleton().update(Vector3::ZERO, Ogre::Vector3(0.0f, 0.0f, 1.0f), Ogre::Vector3(0.0f, 1.0f, 0.0f),
							  	  	  	  	  	  	  	  	  	  Vector3::ZERO, Ogre::Vector3(0.0f, 0.0f, 1.0f), Ogre::Vector3(0.0f, 1.0f, 0.0f));
				#endif // USE_MUMBLE
			}

			if (BSETTING("MainMenuMusic", true))
			{
				SoundScriptManager::getSingleton().createInstance("tracks/main_menu_tune", -1, nullptr);
				SoundScriptManager::getSingleton().trigStart(-1, SS_TRIG_MAIN_MENU);
			}

			//if (!RoR::Application::GetGuiManager()->getMainSelector()->IsVisible())
			RoR::Application::GetGuiManager()->ShowMainMenu(true);

			if (gEnv->network != nullptr || BSETTING("SkipMainMenu", false))
			{
				// Multiplayer started from configurator / MainMenu disabled -> go directly to map selector (traditional behavior)
				RoR::Application::GetGuiManager()->getMainSelector()->Show(LT_Terrain);
				RoR::Application::GetGuiManager()->ShowMainMenu(false);
			}
			else
			{
				RoR::Application::GetGuiManager()->ShowMainMenu(true);
			}
		
			EnterMainMenuLoop();
			
			previous_application_state = Application::STATE_MAIN_MENU;
			m_application_state = Application::STATE_NONE;
		}
		if (m_next_application_state == Application::STATE_SIMULATION)
		{
			// ================================================================
			// Simulation
			// ================================================================

			if (SetupGameplayLoop(enable_network, preselected_map))
			{
				previous_application_state = Application::STATE_SIMULATION;
				EnterGameplayLoop();	
			}
			else
			{
				m_next_application_state = Application::STATE_MAIN_MENU;
			}
		}
		else if (m_next_application_state == Application::STATE_CHANGEMAP)
		{
			//Sim -> change map -> sim
			//                  -> back to menu

			if (previous_application_state == Application::STATE_SIMULATION)
			{
				Application::GetGuiManager()->killSimUtils();
				UnloadTerrain();
				m_base_resource_loaded = true;	
				
			}
			menu_wallpaper_widget->setVisible(true);
			previous_application_state = Application::STATE_CHANGEMAP;
			m_next_application_state = Application::STATE_CHANGEMAP;

			RoR::Application::GetGuiManager()->getMainSelector()->Show(LT_Terrain);
			//It's the same thing so..
			EnterMainMenuLoop();
		}
	}

	// ========================================================================
	// Cleanup
	// ========================================================================

	LoadingWindow::freeSingleton();
	RoR::Application::GetGuiManager()->getMainSelector()->~MainSelector();

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
#ifdef ROR_USE_OGRE_1_9
	// Produces a segfault
	// delete overlay_system;
#endif

	Application::DestroyContentManager();

	delete gEnv->frameListener;
	gEnv->frameListener = nullptr;

	delete gEnv->cameraManager;
	gEnv->cameraManager = nullptr;

	delete gEnv;
	gEnv = nullptr;

}

bool MainThread::SetupGameplayLoop(bool enable_network, Ogre::String preselected_map)
{
	if (!m_base_resource_loaded)
	{
		LOG("Loading base resources");

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
	}

	// ============================================================================
	// Loading settings resources
	// ============================================================================

	if (SSETTING("Water effects", "Reflection + refraction (speed optimized)") == "Hydrax" && !RoR::Application::GetContentManager()->isLoaded(ContentManager::ResourcePack::HYDRAX.mask))
		RoR::Application::GetContentManager()->AddResourcePack(ContentManager::ResourcePack::HYDRAX);

	if (SSETTING("Sky effects", "Caelum (best looking, slower)") == "Caelum (best looking, slower)" && !RoR::Application::GetContentManager()->isLoaded(ContentManager::ResourcePack::CAELUM.mask))
		RoR::Application::GetContentManager()->AddResourcePack(ContentManager::ResourcePack::CAELUM);

	if (SSETTING("Vegetation", "None (fastest)") != "None (fastest)" && !RoR::Application::GetContentManager()->isLoaded(ContentManager::ResourcePack::PAGED.mask))
		RoR::Application::GetContentManager()->AddResourcePack(ContentManager::ResourcePack::PAGED);

	if (BSETTING("HDR", false) && !RoR::Application::GetContentManager()->isLoaded(ContentManager::ResourcePack::HDR.mask))
		RoR::Application::GetContentManager()->AddResourcePack(ContentManager::ResourcePack::HDR);

	if (BSETTING("DOF", false) && !RoR::Application::GetContentManager()->isLoaded(ContentManager::ResourcePack::DEPTH_OF_FIELD.mask))
		RoR::Application::GetContentManager()->AddResourcePack(ContentManager::ResourcePack::DEPTH_OF_FIELD);

	if (BSETTING("Glow", false) && !RoR::Application::GetContentManager()->isLoaded(ContentManager::ResourcePack::GLOW.mask))
		RoR::Application::GetContentManager()->AddResourcePack(ContentManager::ResourcePack::GLOW);

	if (BSETTING("Motion blur", false) && !RoR::Application::GetContentManager()->isLoaded(ContentManager::ResourcePack::BLUR.mask))
		RoR::Application::GetContentManager()->AddResourcePack(ContentManager::ResourcePack::BLUR);

	if (BSETTING("HeatHaze", false) && !RoR::Application::GetContentManager()->isLoaded(ContentManager::ResourcePack::HEATHAZE.mask))
		RoR::Application::GetContentManager()->AddResourcePack(ContentManager::ResourcePack::HEATHAZE);

	if (BSETTING("Sunburn", false) && !RoR::Application::GetContentManager()->isLoaded(ContentManager::ResourcePack::SUNBURN.mask))
		RoR::Application::GetContentManager()->AddResourcePack(ContentManager::ResourcePack::SUNBURN);

	/*if (SSETTING("Shadow technique", "") == "Parallel-split Shadow Maps" && !RoR::Application::GetContentManager()->isLoaded(ContentManager::ResourcePack::PSSM.mask))
		RoR::Application::GetContentManager()->AddResourcePack(ContentManager::ResourcePack::PSSM);
		*/

	Ogre::ResourceGroupManager::getSingleton().initialiseResourceGroup("LoadBeforeMap");

	// ============================================================================
	// Setup
	// ============================================================================

	Application::CreateOverlayWrapper();
	Application::GetOverlayWrapper()->SetupDirectionArrow();

	gEnv->sceneManager->setAmbientLight(Ogre::ColourValue(0.3f, 0.3f, 0.3f));

	if (!m_base_resource_loaded)
	{
		new DustManager(); // setup particle manager singleton. TODO: Move under Application
	}

	if (enable_network)
	{
		// NOTE: create player _AFTER_ network, important
		int colourNum = 0;
		if (gEnv->network->getLocalUserData())
		{
			colourNum = gEnv->network->getLocalUserData()->colournum;
		}
		gEnv->player = (Character *)CharacterFactory::getSingleton().createLocal(colourNum);
	}
	else
	{
		gEnv->player = (Character *)CharacterFactory::getSingleton().createLocal(-1);
		if (gEnv->player != nullptr)
		{
			gEnv->player->setVisible(false);
		}
	}

	// heathaze effect
	if (BSETTING("HeatHaze", false) && RoR::Application::GetContentManager()->isLoaded(ContentManager::ResourcePack::HEATHAZE.mask))
	{
		gEnv->frameListener->m_heathaze = new HeatHaze();
		gEnv->frameListener->m_heathaze->setEnable(true);
	}

	// depth of field effect
	if (BSETTING("DOF", false) && RoR::Application::GetContentManager()->isLoaded(ContentManager::ResourcePack::DEPTH_OF_FIELD.mask))
	{
		gEnv->frameListener->m_dof = new DOFManager();
	}

	if (!m_base_resource_loaded)
	{
		// init camera manager after mygui and after we have a character
		gEnv->cameraManager = new CameraManager(gEnv->frameListener->m_dof);
	}
	
	// ============================================================================
	// Loading map
	// ============================================================================

	Ogre::String map_file_name;
	if (preselected_map.empty())
	{
		CacheEntry* selected_map = RoR::Application::GetGuiManager()->getMainSelector()->GetSelectedEntry();
		if (selected_map != nullptr)
		{
			map_file_name = selected_map->fname;
			RoR::Application::GetCacheSystem()->checkResourceLoaded(*selected_map);
			LOG("Loading map resources");
		}
		else
		{
			LOG("No map selected. Exit.");
			return false;
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
		
	if (map_file_name.empty())
	{
		LOG("No map selected. Exit.");
		return false;
	}
	
	LoadTerrain(map_file_name);

	// ========================================================================
	// Loading vehicle
	// ========================================================================

	Ogre::String preselected_truck = SSETTING("Preselected Truck", "");
	if (!preselected_truck.empty() && preselected_truck != "none")
	{
		Ogre::String preselected_truck_config = SSETTING("Preselected TruckConfig", "");
		if (preselected_truck_config != "")
		{
			LOG("Preselected Truck Config: " + (preselected_truck_config));
		}
		LOG("Preselected Truck: " + (preselected_truck));

		const std::vector<Ogre::String> truckConfig = std::vector<Ogre::String>(1, preselected_truck_config);
		bool enterTruck = (BSETTING("Enter Preselected Truck", false));

		Vector3 pos = gEnv->terrainManager->getSpawnPos();
		Quaternion rot = Quaternion::ZERO;

		Beam* b = BeamFactory::getSingleton().CreateLocalRigInstance(pos, rot, preselected_truck, -1, nullptr, false, &truckConfig);

		if (enterTruck && b && b->free_node > 0)
		{
			BeamFactory::getSingleton().setCurrentTruck(b->trucknum);
		}
		if (b && b->engine)
		{
			b->engine->start();
		}
	}

	gEnv->terrainManager->loadPreloadedTrucks();

	if (ISETTING("OutGauge Mode", 0) > 0)
	{
		new OutProtocol();
	}

	if (BSETTING("MainMenuMusic", true))
		SoundScriptManager::getSingleton().trigKill(-1, SS_TRIG_MAIN_MENU);
		//SoundScriptManager::getSingleton().modulate(nullptr, SS_MOD_MUSIC_VOLUME, 0);

	Application::CreateSceneMouse();
	Application::GetGuiManager()->initSimUtils();

	return true;
}

void MainThread::EnterMainMenuLoop()
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

		MainMenuLoopUpdate(timeSinceLastFrame);

		if (RoR::Application::GetGuiManager()->getMainSelector()->IsFinishedSelecting())
		{
			CacheEntry* selected_map = RoR::Application::GetGuiManager()->getMainSelector()->GetSelectedEntry();
			if (selected_map != nullptr)
			{
				SetNextApplicationState(Application::STATE_SIMULATION);
				RequestExitCurrentLoop();
			}
		}

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
	/* SETUP */
	Application::GetOgreSubsystem()->GetOgreRoot()->addFrameListener(gEnv->frameListener);

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

	/* LOOP */

	while(! m_exit_loop_requested)
	{
		startTime = RoR::Application::GetOgreSubsystem()->GetTimer()->getMilliseconds();

		// no more actual rendering?
		if (m_no_rendering)
		{
			sleepMilliSeconds(100);
			continue;
		}

#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32 || OGRE_PLATFORM == OGRE_PLATFORM_LINUX
		RoRWindowEventUtilities::messagePump();
#endif
		Ogre::RenderWindow* rw = RoR::Application::GetOgreSubsystem()->GetRenderWindow();
		if (rw->isClosed())
		{
			m_shutdown_requested = true;
			printf(">SH\n");
			return;
		}

		RoR::Application::GetOgreSubsystem()->GetOgreRoot()->renderOneFrame();

		if (!rw->isActive() && rw->isVisible())
			rw->update(); // update even when in background !

		if (fpsLimit && timeSinceLastFrame < minTimePerFrame)
		{
			// Sleep twice as long as we were too fast.
			sleepMilliSeconds((minTimePerFrame - timeSinceLastFrame) << 1);
		}


		timeSinceLastFrame = RoR::Application::GetOgreSubsystem()->GetTimer()->getMilliseconds() - startTime;
	}

	/* RESTORE ENVIRONMENT */

	m_exit_loop_requested = false;
	Application::GetOgreSubsystem()->GetOgreRoot()->removeFrameListener(gEnv->frameListener);
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

void MainThread::RequestRestart()
{
	m_shutdown_requested = true;
	Go();
}

void MainThread::RequestExitCurrentLoop()
{
	m_exit_loop_requested = true;
}

void MainThread::MainMenuLoopUpdate(float seconds_since_last_frame)
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

/*	if (RoR::Application::GetGuiManager()->getMainSelector()->IsFinishedSelecting())
	{
		RequestExitCurrentLoop();
	}*/

	// update GUI
	RoR::Application::GetInputEngine()->Capture();

	// update network gui if required, at most every 2 seconds
	if (gEnv->network)
	{
		netcheck_gui_timer += seconds_since_last_frame;
		if (netcheck_gui_timer > 2.0f)
		{
#ifdef USE_MYGUI
#ifdef USE_SOCKETW
			if (BeamFactory::getSingleton().checkStreamsResultsChanged() || gEnv->network->getNetQualityChanged())
			{
				GUI_Multiplayer::getSingleton().update();
			}
#endif // USE_SOCKETW
#endif // USE_MYGUI
			netcheck_gui_timer = 0.0f;
		}

#ifdef USE_SOCKETW
		// update net quality icon
		if (gEnv->network->getNetQualityChanged())
		{
			GUI_Multiplayer::getSingleton().update();
		}
#endif // USE_SOCKETW
	}

	MainMenuLoopUpdateEvents(seconds_since_last_frame);

#ifdef USE_ANGELSCRIPT
	ScriptEngine::getSingleton().framestep(seconds_since_last_frame);
#endif
}

void MainThread::MainMenuLoopUpdateEvents(float seconds_since_last_frame)
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

	if (RoR::Application::GetOverlayWrapper() != nullptr)
	{
		RoR::Application::GetOverlayWrapper()->update(seconds_since_last_frame); // TODO: What's the meaning of this? It only updates some internal timer. ~ only_a_ptr
	}

	if (RoR::Application::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_ENTER_CHATMODE, 0.5f) && !gEnv->frameListener->m_hide_gui)
	{
		//TODO: Separate Chat and console
	}

	if (RoR::Application::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_QUIT_GAME))
	{
		//TODO: Go back to menu 
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

	LoadingWindow::getSingleton().setProgress(0, _L("Loading Terrain"));

	LOG("Loading new terrain format: " + terrain_file);

	if (gEnv->terrainManager != nullptr)
	{
		// remove old terrain
		delete(gEnv->terrainManager);
	}

	gEnv->terrainManager = new TerrainManager();
	gEnv->terrainManager->loadTerrain(terrain_file);

#ifdef USE_MYGUI
	if (GUI_Friction::getSingletonPtr())
	{
		GUI_Friction::getSingleton().setCollisions(gEnv->collisions);
	}
#endif //USE_MYGUI

	gEnv->frameListener->m_loading_state = TERRAIN_LOADED;
	
	if (gEnv->player != nullptr)
	{
		gEnv->player->setVisible(true);
		gEnv->player->setPosition(gEnv->terrainManager->getSpawnPos());

		// Classic behavior, retained for compatibility.
		// Required for maps like N-Labs or F1 Track.
		if (!gEnv->terrainManager->hasPreloadedTrucks())
		{
			gEnv->player->setRotation(Degree(180));
		}
	}

	// hide loading window
	LoadingWindow::getSingleton().hide();
	// hide wallpaper
	MyGUI::Window *w = MyGUI::Gui::getInstance().findWidget<MyGUI::Window>("wallpaper");
	if (w != nullptr)
	{
		w->setVisibleSmooth(false);
	}
}

void MainThread::BackToMenu()
{
	RoR::Application::GetMainThreadLogic()->SetNextApplicationState(Application::STATE_MAIN_MENU);
	RoR::Application::GetMainThreadLogic()->RequestExitCurrentLoop();
}

void MainThread::ChangeMap()
{
	RoR::Application::GetMainThreadLogic()->SetNextApplicationState(Application::STATE_CHANGEMAP);
	RoR::Application::GetMainThreadLogic()->RequestExitCurrentLoop();

}
void MainThread::UnloadTerrain()
{
#ifdef USE_MYGUI
	if (gEnv->surveyMap) gEnv->surveyMap->setVisibility(false);
#endif //USE_MYGUI

	gEnv->frameListener->m_loading_state = NONE_LOADED;
	LoadingWindow::getSingleton().setProgress(0, _L("Unloading Terrain"));
	
	RoR::Application::GetGuiManager()->getMainSelector()->Reset();

	//First of all..
	OverlayWrapper* ow = RoR::Application::GetOverlayWrapper();
	gEnv->frameListener->StopRaceTimer();
	ow->HideRacingOverlay();
	ow->HideDirectionOverlay();

	LoadingWindow::getSingleton().setProgress(15, _L("Unloading Terrain"));

	//Unload all vehicules
	BeamFactory::getSingleton().removeAllTrucks();
	LoadingWindow::getSingleton().setProgress(30, _L("Unloading Terrain"));

	if (gEnv->player != nullptr)
	{
		gEnv->player->setVisible(false);
		delete(gEnv->player);
		gEnv->player = nullptr;
	}
	LoadingWindow::getSingleton().setProgress(45, _L("Unloading Terrain"));

	if (gEnv->terrainManager != nullptr)
	{
		// remove old terrain
		delete(gEnv->terrainManager);
		gEnv->terrainManager = nullptr;
	}
	LoadingWindow::getSingleton().setProgress(60, _L("Unloading Terrain"));

	Application::DeleteSceneMouse();
	LoadingWindow::getSingleton().setProgress(75, _L("Unloading Terrain"));

	//Reinit few things
	gEnv->player = (Character *)CharacterFactory::getSingleton().createLocal(-1);
	if (gEnv->player != nullptr)
	{
		gEnv->player->setVisible(false);
	}
	LoadingWindow::getSingleton().setProgress(100, _L("Unloading Terrain"));
	// hide loading window
	LoadingWindow::getSingleton().hide();
}

void MainThread::ShowSurveyMap(bool be_visible)
{
	if (gEnv->surveyMap != nullptr)
	{
		gEnv->surveyMap->setVisibility(be_visible);
	}
}

void MainThread::ChangedCurrentVehicle(Beam *previous_vehicle, Beam *current_vehicle)
{
	// hide any old dashes
	if (previous_vehicle && previous_vehicle->dash)
	{
		previous_vehicle->dash->setVisible3d(false);
	}
	// show new
	if (current_vehicle && current_vehicle->dash)
	{
		current_vehicle->dash->setVisible3d(true);
	}
	
	// normal workflow
	if (current_vehicle == nullptr)
	{
		// get player out of the vehicle
		if (previous_vehicle && gEnv->player)
		{
			float rotation = previous_vehicle->getRotation() - Math::HALF_PI;
			Vector3 position = previous_vehicle->nodes[0].AbsPosition;
			if (previous_vehicle->cinecameranodepos[0] != -1 && previous_vehicle->cameranodepos[0] != -1 && previous_vehicle->cameranoderoll[0] != -1)
			{
				// truck has a cinecam
				position  = previous_vehicle->nodes[previous_vehicle->cinecameranodepos[0]].AbsPosition;
				position += -2.0 * ((previous_vehicle->nodes[previous_vehicle->cameranodepos[0]].RelPosition - previous_vehicle->nodes[previous_vehicle->cameranoderoll[0]].RelPosition).normalisedCopy());
				position += Vector3(0.0, -1.0, 0.0);
			}
			gEnv->player->setBeamCoupling(false);
			gEnv->player->setRotation(Radian(rotation));
			gEnv->player->setPosition(position);
		}

		//force feedback
		if (gEnv->frameListener->m_forcefeedback)
		{
			gEnv->frameListener->m_forcefeedback->setEnabled(false);
		}

		// hide truckhud
		if (RoR::Application::GetOverlayWrapper()) RoR::Application::GetOverlayWrapper()->truckhud->show(false);

		//getting outside
		if (previous_vehicle)
		{
			previous_vehicle->prepareInside(false);

			if (previous_vehicle->dash)
			{
				previous_vehicle->dash->setVisible(false);
			}
		}

		if (RoR::Application::GetOverlayWrapper())
		{
			RoR::Application::GetOverlayWrapper()->showDashboardOverlays(false, current_vehicle);
		}

#ifdef USE_OPENAL
		SoundScriptManager::getSingleton().trigStop(previous_vehicle, SS_TRIG_AIR);
		SoundScriptManager::getSingleton().trigStop(previous_vehicle, SS_TRIG_PUMP);
#endif // OPENAL

		TRIGGER_EVENT(SE_TRUCK_EXIT, previous_vehicle?previous_vehicle->trucknum:-1);
	}
	else
	{
		//getting inside
		current_vehicle->desactivate();

		if (RoR::Application::GetOverlayWrapper() && ! gEnv->frameListener->m_hide_gui)
		{
			RoR::Application::GetOverlayWrapper()->showDashboardOverlays(true, current_vehicle);
		}

		current_vehicle->activate();
		
		//hide unused items
		if (RoR::Application::GetOverlayWrapper() && current_vehicle->free_active_shock==0)
		{
			(OverlayManager::getSingleton().getOverlayElement("tracks/rollcorneedle"))->hide();
		}
		
		//force feedback
		if (gEnv->frameListener->m_forcefeedback)
		{
			gEnv->frameListener->m_forcefeedback->setEnabled(current_vehicle->driveable==TRUCK); //only for trucks so far
		}

		// attach player to truck
		if (gEnv->player)
		{
			gEnv->player->setBeamCoupling(true, current_vehicle);
		}

		if (RoR::Application::GetOverlayWrapper())
		{
			try
			{
				if (current_vehicle->hashelp)
				{
					OverlayManager::getSingleton().getOverlayElement("tracks/helppanel")->setMaterialName(current_vehicle->helpmat);
					OverlayManager::getSingleton().getOverlayElement("tracks/machinehelppanel")->setMaterialName(current_vehicle->helpmat);
				}
				else
				{
					OverlayManager::getSingleton().getOverlayElement("tracks/helppanel")->setMaterialName("tracks/black");
					OverlayManager::getSingleton().getOverlayElement("tracks/machinehelppanel")->setMaterialName("tracks/black");
				}
			} 
			catch(Ogre::Exception& ex)
			{
				// Report the error
				std::stringstream msg;
				msg << "Error, help panel material (defined in 'help' or 'guisettings/helpMaterial') could not be loaded.\n"
					"Exception occured, file:" << __FILE__ << ", line:" << __LINE__ << ", message:" << ex.what();
				LOG(msg.str());

				// Do not retry
				current_vehicle->hashelp = 0;
				current_vehicle->helpmat[0] = '\0';
			}

			// enable gui mods
			if (! current_vehicle->speedomat.empty())
			{
				OverlayManager::getSingleton().getOverlayElement("tracks/speedo")->setMaterialName(current_vehicle->speedomat);
			}
			else
			{
				OverlayManager::getSingleton().getOverlayElement("tracks/speedo")->setMaterialName("tracks/Speedo");
			}

			if (! current_vehicle->tachomat.empty())
			{
				OverlayManager::getSingleton().getOverlayElement("tracks/tacho")->setMaterialName(current_vehicle->tachomat);
			}
			else
			{
				OverlayManager::getSingleton().getOverlayElement("tracks/tacho")->setMaterialName("tracks/Tacho");
			}
		}
		
		TRIGGER_EVENT(SE_TRUCK_ENTER, current_vehicle?current_vehicle->trucknum:-1);
	}

	if (previous_vehicle != nullptr || current_vehicle != nullptr)
	{
		gEnv->cameraManager->NotifyVehicleChanged(previous_vehicle, current_vehicle);
	}
}

void MainThread::RegenCache()
{
	//TODO: Move to somewhere else than MainThread maybe? -max98
	RoR::Application::GetCacheSystem()->Startup(true); // true = force regeneration

	if (BSETTING("regen-cache-only", false)) //Only if game run just for regen
	{
		// Get stats
		int num_new = RoR::Application::GetCacheSystem()->newFiles;
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
}

