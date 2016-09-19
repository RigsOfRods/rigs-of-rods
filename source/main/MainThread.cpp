/*
	This source file is part of Rigs of Rods
	Copyright 2005-2012 Pierre-Michel Ricordel
	Copyright 2007-2012 Thomas Fischer
	Copyright 2013-2014 Petr Ohlidal

	For more information, see http://www.rigsofrods.org/

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
#include "ContentManager.h"
#include "DashBoardManager.h"
#include "DepthOfFieldEffect.h"
#include "DustManager.h"
#include "ErrorUtils.h"
#include "ForceFeedback.h"
#include "GlobalEnvironment.h"
#include "GUIManager.h"
#include "GUI_LoadingWindow.h"
#include "GUI_MainSelector.h"
#include "GUI_MultiplayerClientList.h"
#include "Heathaze.h"
#include "InputEngine.h"
#include "Language.h"
#include "MumbleIntegration.h"
#include "Mirrors.h"
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

#include <chrono>
#include <thread>

using namespace RoR;
using namespace Ogre; // The _L() macro won't compile without.

MainThread::MainThread():
	m_no_rendering(false),
	m_restart_requested(false),
	m_start_time(0),
	m_base_resource_loaded(false),
    m_is_mumble_created(false),
    m_frame_listener(nullptr)
{
	RoR::Application::SetMainThreadLogic(this);
}

void MainThread::Go()
{
	// ================================================================================
	// Bootstrap
	// ================================================================================

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

	RoR::Application::GetCacheSystem()->setLocation(Application::GetSysCacheDir() + PATH_SLASH, Application::GetSysConfigDir() + PATH_SLASH);

	Application::GetContentManager()->init();

	// HIDE BOOTSTRAP SCREEN

	startup_screen_overlay->hide();

	// Back to full rendering
	scene_manager->clearSpecialCaseRenderQueues();
	scene_manager->setSpecialCaseRenderQueueMode(Ogre::SceneManager::SCRQM_EXCLUDE);
	
	Application::CreateGuiManagerIfNotExists();

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

	// Create legacy RoRFrameListener
	m_frame_listener = new RoRFrameListener();

#ifdef USE_ANGELSCRIPT
	ScriptEngine::getSingleton().SetFrameListener(m_frame_listener);
#endif

#ifdef USE_MPLATFORM
	m_frame_listener->m_platform = new MPlatform_FD();
	if (m_frame_listener->m_platform) 
	{
		m_platform->connect();
	}
#endif

	m_frame_listener->windowResized(RoR::Application::GetOgreSubsystem()->GetRenderWindow());
	RoRWindowEventUtilities::addWindowEventListener(RoR::Application::GetOgreSubsystem()->GetRenderWindow(), m_frame_listener);

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

			m_frame_listener->m_forcefeedback = new ForceFeedback(RoR::Application::GetInputEngine()->getForceFeedbackDevice(), ogain, stressg, centg, camg);
		}
	}
#endif // _WIN32

	String screenshotFormatString = SSETTING("Screenshot Format", "jpg (smaller, default)");
	if     (screenshotFormatString == "jpg (smaller, default)")
		strcpy(m_frame_listener->m_screenshot_format, "jpg");
	else if (screenshotFormatString =="png (bigger, no quality loss)")
		strcpy(m_frame_listener->m_screenshot_format, "png");
	else
		strncpy(m_frame_listener->m_screenshot_format, screenshotFormatString.c_str(), 10);

	// initiate player colours
	PlayerColours::getSingleton();

	// new factory for characters, net is INVALID, will be set later
	new CharacterFactory();

	new BeamFactory();

	// ========================================================================
	// Main loop (switches application states)
	// ========================================================================

    Application::State previous_application_state = Application::GetActiveAppState();
    Application::SetActiveAppState(Application::APP_STATE_MAIN_MENU);

    if (! Application::GetPendingTerrain().empty())
    {
        Application::SetPendingAppState(Application::APP_STATE_SIMULATION);
    }
    if (Application::GetPendingMpState() == Application::MP_STATE_CONNECTED)
    {
        this->JoinMultiplayerServer();
        if (Application::GetActiveMpState() == Application::MP_STATE_CONNECTED)
        {
            Application::SetPendingAppState(Application::APP_STATE_SIMULATION);
        }
    }
    m_base_resource_loaded = false;
    for (;;)
    {
        if (Application::GetPendingAppState() == Application::APP_STATE_SHUTDOWN)
        {
            break;
        }
        else if (Application::GetPendingAppState() == Application::APP_STATE_MAIN_MENU)
        {
            Application::SetActiveAppState(Application::APP_STATE_MAIN_MENU);
            Application::SetPendingAppState(Application::APP_STATE_NONE);

			OgreSubsystem* ror_ogre_subsystem = RoR::Application::GetOgreSubsystem();
			assert(ror_ogre_subsystem != nullptr);

			if (previous_application_state == Application::APP_STATE_SIMULATION)
			{
                if (Application::GetActiveMpState() == Application::MP_STATE_CONNECTED)
                {
                    RoR::Networking::Disconnect();
                    Application::GetGuiManager()->SetVisible_MpClientList(false);
                }
				UnloadTerrain();
				m_base_resource_loaded = true;
				gEnv->cameraManager->OnReturnToMainMenu();
				/* Hide top menu */
                Application::GetGuiManager()->SetVisible_TopMenubar(false);
				/* Restore wallpaper */
				menu_wallpaper_widget->setVisible(true);

				/* Set Mumble to non-positional audio */
				#ifdef USE_MUMBLE
                    MumbleIntegration::getSingleton().update(
                        Ogre::Vector3::ZERO,
                        Ogre::Vector3(0.0f, 0.0f, 1.0f),
                        Ogre::Vector3(0.0f, 1.0f, 0.0f),
                        Ogre::Vector3::ZERO,
                        Ogre::Vector3(0.0f, 0.0f, 1.0f),
                        Ogre::Vector3(0.0f, 1.0f, 0.0f));
				#endif // USE_MUMBLE
			}

			if (BSETTING("MainMenuMusic", true))
			{
				SoundScriptManager::getSingleton().createInstance("tracks/main_menu_tune", -1, nullptr);
				SoundScriptManager::getSingleton().trigStart(-1, SS_TRIG_MAIN_MENU);
			}

            Application::GetGuiManager()->ReflectGameState();
			if (Application::GetPendingMpState() == Application::MP_STATE_CONNECTED || BSETTING("SkipMainMenu", false))
			{
				// Multiplayer started from configurator / MainMenu disabled -> go directly to map selector (traditional behavior)
                RoR::Application::GetGuiManager()->SetVisible_GameMainMenu(false);
				RoR::Application::GetGuiManager()->GetMainSelector()->Show(LT_Terrain);
			}

			EnterMainMenuLoop();
		}
		else if (Application::GetPendingAppState() == Application::APP_STATE_SIMULATION)
		{
			if (SetupGameplayLoop())
			{
                Application::SetActiveAppState(Application::APP_STATE_SIMULATION);
                Application::SetPendingAppState(Application::APP_STATE_NONE);
                Application::GetGuiManager()->ReflectGameState();
                EnterGameplayLoop();
			}
			else
			{
                Application::SetPendingAppState(Application::APP_STATE_MAIN_MENU);
			}
		}
		else if (Application::GetPendingAppState() == Application::APP_STATE_CHANGE_MAP)
		{
			//Sim -> change map -> sim
			//                  -> back to menu

            Application::SetActiveAppState(Application::APP_STATE_CHANGE_MAP);
            Application::SetPendingAppState(Application::APP_STATE_NONE);
			if (previous_application_state == Application::APP_STATE_SIMULATION)
			{
				UnloadTerrain();
				m_base_resource_loaded = true;
				
			}
			menu_wallpaper_widget->setVisible(true);

			RoR::Application::GetGuiManager()->GetMainSelector()->Show(LT_Terrain);
			//It's the same thing so..
			EnterMainMenuLoop();
		}
        previous_application_state = Application::GetActiveAppState();
	} // End of app state loop

	// ========================================================================
	// Cleanup
	// ========================================================================

	RoR::Application::GetGuiManager()->GetMainSelector()->~MainSelector();

#ifdef USE_SOCKETW
	if (Application::GetActiveMpState() == Application::MP_STATE_CONNECTED)
	{
		RoR::Networking::Disconnect();
	}
#endif //SOCKETW

	//TODO: we should destroy OIS here
	//TODO: we could also try to destroy SoundScriptManager, but we don't care!

#ifdef USE_MPLATFORM
	if (m_frame_listener->mplatform != nullptr)
	{
		if (m_frame_listener->mplatform->disconnect())
		{
			delete(m_frame_listener->mplatform);
			m_frame_listener->mplatform = nullptr;
		}
	}
#endif

	scene_manager->destroyCamera(camera);
	RoR::Application::GetOgreSubsystem()->GetOgreRoot()->destroySceneManager(scene_manager);
#ifdef ROR_USE_OGRE_1_9
	// Produces a segfault
	// delete overlay_system;
#endif

	Application::DestroyOverlayWrapper();

	Application::DestroyContentManager();

	delete m_frame_listener;
    m_frame_listener = nullptr;

	delete gEnv->cameraManager;
	gEnv->cameraManager = nullptr;

	delete gEnv;
	gEnv = nullptr;

}

bool MainThread::SetupGameplayLoop()
{
    auto* loading_window = Application::GetGuiManager()->GetLoadingWindow();
	if (!m_base_resource_loaded)
	{
		LOG("Loading base resources");

		// ============================================================================
		// Loading base resources
		// ============================================================================

		loading_window->setProgress(0, _L("Loading base resources"));

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

    BeamFactory::getSingleton().GetParticleManager().CheckAndInit();

	int colourNum = -1;

#ifdef USE_SOCKETW
	if (Application::GetActiveMpState() == Application::MP_STATE_CONNECTED)
	{
		wchar_t tmp[255] = L"";
		UTFString format = _L("Press %ls to start chatting");
		swprintf(tmp, 255, format.asWStr_c_str(), ANSI_TO_WCHAR(RoR::Application::GetInputEngine()->getKeyForCommand(EV_COMMON_ENTER_CHATMODE)).c_str());
		Application::GetGuiManager()->pushMessageChatBox(UTFString(tmp));

		user_info_t info = RoR::Networking::GetLocalUserData();
		colourNum = info.colournum;
	}
#endif // USE_SOCKETW

	// NOTE: create player _AFTER_ network, important
	gEnv->player = CharacterFactory::getSingleton().createLocal(colourNum);

	// heathaze effect
	if (BSETTING("HeatHaze", false) && RoR::Application::GetContentManager()->isLoaded(ContentManager::ResourcePack::HEATHAZE.mask))
	{
		m_frame_listener->m_heathaze = new HeatHaze();
		m_frame_listener->m_heathaze->setEnable(true);
	}

	// depth of field effect
	if (BSETTING("DOF", false) && RoR::Application::GetContentManager()->isLoaded(ContentManager::ResourcePack::DEPTH_OF_FIELD.mask))
	{
		m_frame_listener->m_dof = new DOFManager();
	}

	if (!m_base_resource_loaded)
	{
		// init camera manager after mygui and after we have a character
		gEnv->cameraManager = new CameraManager(m_frame_listener->m_dof);
	}
	
	// ============================================================================
	// Loading map
	// ============================================================================

	if (Application::GetPendingTerrain().empty())
	{
		CacheEntry* selected_map = RoR::Application::GetGuiManager()->GetMainSelector()->GetSelectedEntry();
		if (selected_map != nullptr)
		{
            Application::SetPendingTerrain(selected_map->fname);
		}
		else
		{
			LOG("No map selected. Returning to menu.");
            Application::GetGuiManager()->SetVisible_LoadingWindow(false);
			return false;
		}
	}

    if(! LoadTerrain())
    {
        LOG("Could not load map. Returning to menu.");
        Application::GetGuiManager()->SetVisible_LoadingWindow(false);
        return false;
    }

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

		Vector3 pos = gEnv->player->getPosition();
		Quaternion rot = Quaternion(Degree(180) - gEnv->player->getRotation(), Vector3::UNIT_Y);

		Beam* b = BeamFactory::getSingleton().CreateLocalRigInstance(pos, rot, preselected_truck, -1, nullptr, false, &truckConfig);

		if (b != nullptr)
		{
			// Calculate translational offset for node[0] to align the trucks rotation center with m_reload_pos
			Vector3 translation = pos - b->getRotationCenter();
			b->resetPosition(b->nodes[0].AbsPosition + Vector3(translation.x, 0.0f, translation.z), true);

			b->updateFlexbodiesPrepare();
			b->updateFlexbodiesFinal();
			b->updateVisual();

			if (enterTruck && b->free_node > 0)
			{
				BeamFactory::getSingleton().setCurrentTruck(b->trucknum);
			}
			if (b->engine)
			{
				b->engine->start();
			}
		}
	}

	gEnv->terrainManager->loadPreloadedTrucks();

	// ========================================================================
	// Extra setup
	// ========================================================================

	if (ISETTING("OutGauge Mode", 0) > 0)
	{
		new OutProtocol();
	}

    Application::CreateOverlayWrapper();
    Application::GetOverlayWrapper()->SetupDirectionArrow();

	if (BSETTING("MainMenuMusic", true))
    {
		SoundScriptManager::getSingleton().trigKill(-1, SS_TRIG_MAIN_MENU);
    }

	Application::CreateSceneMouse();

    gEnv->sceneManager->setAmbientLight(Ogre::ColourValue(0.3f, 0.3f, 0.3f));

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

	while (Application::GetPendingAppState() == Application::APP_STATE_NONE)
	{
		startTime = RoR::Application::GetOgreSubsystem()->GetTimer()->getMilliseconds();

		// no more actual rendering?
		if (m_no_rendering)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
			continue;
		}

		MainMenuLoopUpdate(timeSinceLastFrame);

		if (RoR::Application::GetGuiManager()->GetMainSelector()->IsFinishedSelecting())
		{
			CacheEntry* selected_map = RoR::Application::GetGuiManager()->GetMainSelector()->GetSelectedEntry();
			if (selected_map != nullptr)
			{
                RoR::Application::GetGuiManager()->GetMainSelector()->Reset(); // TODO: Eliminate this mechanism ~ only_a_ptr 09/2016
                Application::SetPendingAppState(Application::APP_STATE_SIMULATION);
                Application::SetPendingTerrain(selected_map->fname);
			}
		}

#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32 || OGRE_PLATFORM == OGRE_PLATFORM_LINUX
		RoRWindowEventUtilities::messagePump();
#endif
		Ogre::RenderWindow* rw = RoR::Application::GetOgreSubsystem()->GetRenderWindow();
		if (rw->isClosed())
		{
			Application::SetPendingAppState(Application::APP_STATE_SHUTDOWN);
			continue;
		}

		RoR::Application::GetOgreSubsystem()->GetOgreRoot()->renderOneFrame();

		if (!rw->isActive() && rw->isVisible())
			rw->update(); // update even when in background !

		
		// FPS-limiter. TODO: Is this necessary in menu?
		if (fpsLimit && timeSinceLastFrame < minTimePerFrame)
		{
			// Sleep twice as long as we were too fast.
			int ms = static_cast<int>((minTimePerFrame - timeSinceLastFrame) << 1);
			std::this_thread::sleep_for(std::chrono::milliseconds(ms));
		}

		timeSinceLastFrame = RoR::Application::GetOgreSubsystem()->GetTimer()->getMilliseconds() - startTime;
	}
}

void MainThread::EnterGameplayLoop()
{
	/* SETUP */
	RoR::Mirrors::Init();

	Application::GetOgreSubsystem()->GetOgreRoot()->addFrameListener(m_frame_listener);

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

	while(Application::GetPendingAppState() == Application::APP_STATE_NONE)
	{
		startTime = RoR::Application::GetOgreSubsystem()->GetTimer()->getMilliseconds();

		// no more actual rendering?
		if (m_no_rendering)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
			continue;
		}

#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32 || OGRE_PLATFORM == OGRE_PLATFORM_LINUX
		RoRWindowEventUtilities::messagePump();
#endif
		Ogre::RenderWindow* rw = RoR::Application::GetOgreSubsystem()->GetRenderWindow();
		if (rw->isClosed())
		{
            Application::SetPendingAppState(Application::APP_STATE_SHUTDOWN);
			continue;
		}

		RoR::Application::GetOgreSubsystem()->GetOgreRoot()->renderOneFrame();

        if ((Application::GetActiveMpState() == Application::MP_STATE_CONNECTED) && RoR::Networking::CheckError())
        {
            Ogre::String title = Ogre::UTFString(_L("Network fatal error: ")).asUTF8();
            Ogre::String msg = RoR::Networking::GetErrorMessage().asUTF8();
            Application::GetGuiManager()->ShowMessageBox(title, msg, true, "OK", true, false, "");
            Application::SetPendingAppState(Application::APP_STATE_MAIN_MENU);
        }

		if (!rw->isActive() && rw->isVisible())
			rw->update(); // update even when in background !

		if (fpsLimit && timeSinceLastFrame < minTimePerFrame)
		{
			// Sleep twice as long as we were too fast.
			int ms = static_cast<int>((minTimePerFrame - timeSinceLastFrame) << 1);
			std::this_thread::sleep_for(std::chrono::milliseconds(ms));
		}

		timeSinceLastFrame = RoR::Application::GetOgreSubsystem()->GetTimer()->getMilliseconds() - startTime;
	}

	/* RESTORE ENVIRONMENT */

	Application::GetOgreSubsystem()->GetOgreRoot()->removeFrameListener(m_frame_listener);
}

void MainThread::Exit()
{
	RoR::Application::GetOgreSubsystem()->GetOgreRoot()->removeFrameListener(m_frame_listener);
	delete m_frame_listener;
	m_frame_listener = nullptr;
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
		Application::SetPendingAppState(Application::APP_STATE_SHUTDOWN);
		return;
	}

#ifdef USE_SOCKETW
	if (Application::GetActiveMpState() == Application::MP_STATE_CONNECTED)
	{
        Application::GetGuiManager()->GetMpClientList()->update();
	}
#endif // USE_SOCKETW

	RoR::Application::GetInputEngine()->Capture();

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

	if (RoR::Application::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_ENTER_CHATMODE, 0.5f) && !m_frame_listener->m_hide_gui)
	{
		//TODO: Separate Chat and console
	}

	if (RoR::Application::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_QUIT_GAME))
	{
		//TODO: Go back to menu 
		Application::SetPendingAppState(Application::APP_STATE_SHUTDOWN);
		return;
	}

    auto gui_man = Application::GetGuiManager();
    if (RoR::Application::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_CONSOLE_TOGGLE, 5.f))
    {
        gui_man->SetVisible_Console(!gui_man->IsVisible_Console());
    }

	// TODO: screenshot

	// FOV settings disabled in menu

	// FIXME: full screen/windowed screen switching
	//if (RoR::Application::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_FULLSCREEN_TOGGLE, 2.0f)) {}
}

bool MainThread::LoadTerrain()
{
	// check if the resource is loaded
	Ogre::String terrain_file = Application::GetPendingTerrain();
    Application::SetPendingTerrain("");
	if (! RoR::Application::GetCacheSystem()->checkResourceLoaded(terrain_file)) // Input-output argument.
	{
		// fallback for terrains, add .terrn if not found and retry
		terrain_file = Ogre::StringUtil::replaceAll(terrain_file, ".terrn2", "");
		terrain_file = Ogre::StringUtil::replaceAll(terrain_file, ".terrn", "");
		terrain_file = terrain_file + ".terrn2";
		if (!RoR::Application::GetCacheSystem()->checkResourceLoaded(terrain_file))
		{
			LOG("Terrain not found: " + terrain_file);
            Ogre::UTFString title(_L("Terrain loading error"));
            Ogre::UTFString msg(_L("Terrain not found: ") + terrain_file);
            Application::GetGuiManager()->ShowMessageBox(title.asUTF8(), msg.asUTF8(), true, "OK", true, false, "");
            return false;
		}
	}

	Application::GetGuiManager()->GetLoadingWindow()->setProgress(0, _L("Loading Terrain"));

	LOG("Loading terrain: " + terrain_file);

	if (gEnv->terrainManager != nullptr)
	{
		// remove old terrain
		delete(gEnv->terrainManager); // TODO: do it when leaving simulation.
	}

	gEnv->terrainManager = new TerrainManager();
	gEnv->terrainManager->loadTerrain(terrain_file);
    Application::SetActiveTerrain(terrain_file);

#ifdef USE_MYGUI
    Application::GetGuiManager()->FrictionSettingsUpdateCollisions();
#endif //USE_MYGUI
	
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

		// Small hack to prevent spawning the Character in mid-air
		for (int i=0; i<100; i++)
		{
			gEnv->player->update(0.05f);
		}
	}

	// hide loading window
	Application::GetGuiManager()->SetVisible_LoadingWindow(false);
	// hide wallpaper
	MyGUI::Window *w = MyGUI::Gui::getInstance().findWidget<MyGUI::Window>("wallpaper");
	if (w != nullptr)
	{
		w->setVisibleSmooth(false);
	}
    return true;
}

void MainThread::UnloadTerrain()
{
#ifdef USE_MYGUI
	if (gEnv->surveyMap) gEnv->surveyMap->setVisibility(false);
#endif //USE_MYGUI
    auto loading_window = Application::GetGuiManager()->GetLoadingWindow();

	loading_window->setProgress(0, _L("Unloading Terrain"));
	
	RoR::Application::GetGuiManager()->GetMainSelector()->Reset();

	//First of all..
	OverlayWrapper* ow = RoR::Application::GetOverlayWrapper();
	m_frame_listener->StopRaceTimer();
	ow->HideRacingOverlay();
	ow->HideDirectionOverlay();

	loading_window->setProgress(15, _L("Unloading Terrain"));

	//Unload all vehicules
	BeamFactory::getSingleton().removeAllTrucks();
	loading_window->setProgress(30, _L("Unloading Terrain"));

	if (gEnv->player != nullptr)
	{
		gEnv->player->setVisible(false);
		delete(gEnv->player);
		gEnv->player = nullptr;
	}
	loading_window->setProgress(45, _L("Unloading Terrain"));

	if (gEnv->terrainManager != nullptr)
	{
		// remove old terrain
		delete(gEnv->terrainManager);
		gEnv->terrainManager = nullptr;
	}
	loading_window->setProgress(60, _L("Unloading Terrain"));

	Application::DeleteSceneMouse();
	loading_window->setProgress(75, _L("Unloading Terrain"));

	//Reinit few things
	loading_window->setProgress(100, _L("Unloading Terrain"));
	// hide loading window
    Application::GetGuiManager()->SetVisible_LoadingWindow(false);
}

void MainThread::ShowSurveyMap(bool be_visible)
{
	if (gEnv->surveyMap != nullptr)
	{
		gEnv->surveyMap->setVisibility(be_visible);
	}
}

void MainThread::JoinMultiplayerServer()
{
#ifdef USE_SOCKETW

    RoR::Application::GetGuiManager()->SetVisible_MultiplayerSelector(false);
    RoR::Application::GetGuiManager()->SetVisible_GameMainMenu(false);

    Application::GetGuiManager()->GetLoadingWindow()->setAutotrack(_L("Trying to connect to server ..."));

    if (!RoR::Networking::Connect())
    {
        LOG("connection failed. server down?");
        Application::GetGuiManager()->SetVisible_LoadingWindow(false);
        Application::GetGuiManager()->ShowMessageBox("Connection failed",
            RoR::Networking::GetErrorMessage().asUTF8_c_str(), true, "OK", true, false, "");

        RoR::Application::GetGuiManager()->SetVisible_GameMainMenu(true);
        return;
    }

    Application::GetGuiManager()->SetVisible_LoadingWindow(false);
    RoR::Application::GetGuiManager()->SetVisible_MpClientList(true);
    Application::GetGuiManager()->GetMpClientList()->update();

    RoR::ChatSystem::SendStreamSetup();

#ifdef USE_MUMBLE
    if (! m_is_mumble_created)
    {
        new MumbleIntegration();
        m_is_mumble_created = true;
    }
#endif // USE_MUMBLE

    String terrain_name = RoR::Networking::GetTerrainName();
    if (terrain_name != "any")
    {
        Application::SetPendingTerrain(terrain_name);
        Application::SetPendingAppState(Application::APP_STATE_SIMULATION);
    }
    else
    {
        // Connected -> go directly to map selector
        RoR::Application::GetGuiManager()->GetMainSelector()->Reset();
        RoR::Application::GetGuiManager()->GetMainSelector()->Show(LT_Terrain);
    }
#endif //SOCKETW
}

void MainThread::LeaveMultiplayerServer()
{
#ifdef USE_SOCKETW
    if (Application::GetActiveMpState() == Application::MP_STATE_CONNECTED)
    {
        Application::GetGuiManager()->GetLoadingWindow()->setAutotrack(_L("Disconnecting, wait 10 seconds ..."));
        RoR::Networking::Disconnect();
        Application::GetGuiManager()->SetVisible_MpClientList(false);
        Application::GetGuiManager()->SetVisible_LoadingWindow(false);
    }
#endif //SOCKETW
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

	if (previous_vehicle)
	{
		if (RoR::Application::GetOverlayWrapper())
		{
			RoR::Application::GetOverlayWrapper()->showDashboardOverlays(false, previous_vehicle);
		}

#ifdef USE_OPENAL
		SoundScriptManager::getSingleton().trigStop(previous_vehicle, SS_TRIG_AIR);
		SoundScriptManager::getSingleton().trigStop(previous_vehicle, SS_TRIG_PUMP);
#endif // OPENAL
	}

    auto frame_listener = Application::GetMainThreadLogic()->GetFrameListener();

	if (current_vehicle == nullptr)
	{
		// getting outside
		if (previous_vehicle && gEnv->player)
		{
			previous_vehicle->prepareInside(false);

			// get player out of the vehicle
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

		// force feedback
		if (frame_listener->m_forcefeedback)
		{
			frame_listener->m_forcefeedback->setEnabled(false);
		}

		// hide truckhud
		if (RoR::Application::GetOverlayWrapper()) RoR::Application::GetOverlayWrapper()->truckhud->show(false);

		TRIGGER_EVENT(SE_TRUCK_EXIT, previous_vehicle?previous_vehicle->trucknum:-1);
	}
    else
	{
		// getting inside
		if (RoR::Application::GetOverlayWrapper() && ! frame_listener->m_hide_gui)
		{
			RoR::Application::GetOverlayWrapper()->showDashboardOverlays(true, current_vehicle);
		}

		// hide unused items
		if (RoR::Application::GetOverlayWrapper() && current_vehicle->free_active_shock==0)
		{
			(OverlayManager::getSingleton().getOverlayElement("tracks/rollcorneedle"))->hide();
		}

		// force feedback
		if (frame_listener->m_forcefeedback)
		{
			frame_listener->m_forcefeedback->setEnabled(current_vehicle->driveable==TRUCK); //only for trucks so far
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
				} else
				{
					OverlayManager::getSingleton().getOverlayElement("tracks/helppanel")->setMaterialName("tracks/black");
					OverlayManager::getSingleton().getOverlayElement("tracks/machinehelppanel")->setMaterialName("tracks/black");
				}
			} catch(Ogre::Exception& ex)
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
			} else
			{
				OverlayManager::getSingleton().getOverlayElement("tracks/speedo")->setMaterialName("tracks/Speedo");
			}

			if (! current_vehicle->tachomat.empty())
			{
				OverlayManager::getSingleton().getOverlayElement("tracks/tacho")->setMaterialName(current_vehicle->tachomat);
			} else
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

