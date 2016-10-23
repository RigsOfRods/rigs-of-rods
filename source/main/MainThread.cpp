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
	RoR::App::SetMainThreadLogic(this);
}

void MainThread::Go()
{
	// ================================================================================
	// Bootstrap
	// ================================================================================

	App::StartOgreSubsystem();
#ifdef ROR_USE_OGRE_1_9
	Ogre::OverlaySystem* overlay_system = new OverlaySystem(); //Overlay init
#endif

	App::CreateContentManager();

	LanguageEngine::getSingleton().setup();

	// Add startup resources
	RoR::App::GetContentManager()->AddResourcePack(ContentManager::ResourcePack::OGRE_CORE);
	RoR::App::GetContentManager()->AddResourcePack(ContentManager::ResourcePack::GUI_STARTUP_SCREEN);
	RoR::App::GetContentManager()->AddResourcePack(ContentManager::ResourcePack::GUI_MENU_WALLPAPERS);
	Ogre::ResourceGroupManager::getSingleton().initialiseResourceGroup("Bootstrap");
	Ogre::ResourceGroupManager::getSingleton().initialiseResourceGroup("Wallpapers");

	// Setup rendering (menu + simulation)
	Ogre::SceneManager* scene_manager = RoR::App::GetOgreSubsystem()->GetOgreRoot()->createSceneManager(Ogre::ST_EXTERIOR_CLOSE, "main_scene_manager");
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
	RoR::App::GetOgreSubsystem()->GetViewport()->setCamera(camera);
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

	App::GetOgreSubsystem()->GetOgreRoot()->renderOneFrame(); // Render bootstrap screen once and leave it visible.

	RoR::App::CreateCacheSystem();

	RoR::App::GetCacheSystem()->setLocation(App::GetSysCacheDir() + PATH_SLASH, App::GetSysConfigDir() + PATH_SLASH);

	App::GetContentManager()->init();

	// HIDE BOOTSTRAP SCREEN

	startup_screen_overlay->hide();

	// Back to full rendering
	scene_manager->clearSpecialCaseRenderQueues();
	scene_manager->setSpecialCaseRenderQueueMode(Ogre::SceneManager::SCRQM_EXCLUDE);
	
	App::CreateGuiManagerIfNotExists();

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

	RoR::App::CreateInputEngine();
	RoR::App::GetInputEngine()->setupDefault(RoR::App::GetOgreSubsystem()->GetMainHWND());

	// Initialize "managed materials"
	// These are base materials referenced by user content
	// They must be initialized before any content is loaded, including mod-cache update.
	// Otherwise material links are unresolved and loading ends with an exception
	// TODO: Study Ogre::ResourceLoadingListener and implement smarter solution (not parsing materials on cache refresh!)
	App::GetContentManager()->InitManagedMaterials();

	if (BSETTING("regen-cache-only", false)) //Can be usefull so we will leave it here -max98
		MainThread::RegenCache(); 

	RoR::App::GetCacheSystem()->Startup();

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

	m_frame_listener->windowResized(RoR::App::GetOgreSubsystem()->GetRenderWindow());
	RoRWindowEventUtilities::addWindowEventListener(RoR::App::GetOgreSubsystem()->GetRenderWindow(), m_frame_listener);

#ifdef _WIN32
    if (App::GetIoFFbackEnabled()) // Force feedback
    {
        if (RoR::App::GetInputEngine()->getForceFeedbackDevice())
        {
            m_frame_listener->m_forcefeedback.Setup();
        }
        else
        {
            LOG("No force feedback device detected, disabling force feedback");
            App::SetIoFFbackEnabled(false);
        }
    }
#endif // _WIN32

	// initiate player colours
	PlayerColours::getSingleton();

	// new factory for characters, net is INVALID, will be set later
	new CharacterFactory();

	new BeamFactory();

	// ========================================================================
	// Main loop (switches application states)
	// ========================================================================

    App::State previous_application_state = App::GetActiveAppState();
    App::SetActiveAppState(App::APP_STATE_MAIN_MENU);

    if (! App::GetDiagPreselectedTerrain().empty())
    {
        App::SetPendingAppState(App::APP_STATE_SIMULATION);
    }
    if (App::GetPendingMpState() == App::MP_STATE_CONNECTED)
    {
        this->JoinMultiplayerServer();
    }
    for (;;)
    {
        if (App::GetPendingAppState() == App::APP_STATE_SHUTDOWN)
        {
            break;
        }
        else if (App::GetPendingAppState() == App::APP_STATE_MAIN_MENU)
        {
            App::SetActiveAppState(App::APP_STATE_MAIN_MENU);
            App::SetPendingAppState(App::APP_STATE_NONE);

			if (previous_application_state == App::APP_STATE_SIMULATION)
			{
                if (App::GetActiveMpState() == App::MP_STATE_CONNECTED)
                {
                    RoR::Networking::Disconnect();
                    App::GetGuiManager()->SetVisible_MpClientList(false);
                }
				UnloadTerrain();
				gEnv->cameraManager->OnReturnToMainMenu();
				/* Hide top menu */
                App::GetGuiManager()->SetVisible_TopMenubar(false);
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

			if (App::GetAudioMenuMusic())
			{
				SoundScriptManager::getSingleton().createInstance("tracks/main_menu_tune", -1, nullptr);
				SoundScriptManager::getSingleton().trigStart(-1, SS_TRIG_MAIN_MENU);
			}

            App::GetGuiManager()->ReflectGameState();
			if (App::GetPendingMpState() == App::MP_STATE_CONNECTED || BSETTING("SkipMainMenu", false))
			{
				// Multiplayer started from configurator / MainMenu disabled -> go directly to map selector (traditional behavior)
				if (App::GetDiagPreselectedTerrain() == "")
				{
					RoR::App::GetGuiManager()->SetVisible_GameMainMenu(false);
					RoR::App::GetGuiManager()->GetMainSelector()->Show(LT_Terrain);
				}
			}

			EnterMainMenuLoop();
		}
		else if (App::GetPendingAppState() == App::APP_STATE_SIMULATION)
		{
			if (SetupGameplayLoop())
			{
                App::SetActiveAppState(App::APP_STATE_SIMULATION);
                App::SetPendingAppState(App::APP_STATE_NONE);
                App::GetGuiManager()->ReflectGameState();
                EnterGameplayLoop();
			}
			else
			{
                App::SetPendingAppState(App::APP_STATE_MAIN_MENU);
			}
		}
		else if (App::GetPendingAppState() == App::APP_STATE_CHANGE_MAP)
		{
			//Sim -> change map -> sim
			//                  -> back to menu

            App::SetActiveAppState(App::APP_STATE_CHANGE_MAP);
            App::SetPendingAppState(App::APP_STATE_NONE);
			if (previous_application_state == App::APP_STATE_SIMULATION)
			{
				UnloadTerrain();
				
			}
			menu_wallpaper_widget->setVisible(true);

			if (App::GetDiagPreselectedTerrain() == "")
			{
				RoR::App::GetGuiManager()->GetMainSelector()->Show(LT_Terrain);
			}
			else
			{
                RoR::App::GetGuiManager()->SetVisible_GameMainMenu(true);
			}
			//It's the same thing so..
			EnterMainMenuLoop();
		}
        previous_application_state = App::GetActiveAppState();
	} // End of app state loop

	// ========================================================================
	// Cleanup
	// ========================================================================

    App::GetSettings().SaveSettings(); // Save RoR.cfg

	RoR::App::GetGuiManager()->GetMainSelector()->~MainSelector();

#ifdef USE_SOCKETW
	if (App::GetActiveMpState() == App::MP_STATE_CONNECTED)
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
	RoR::App::GetOgreSubsystem()->GetOgreRoot()->destroySceneManager(scene_manager);
#ifdef ROR_USE_OGRE_1_9
	// Produces a segfault
	// delete overlay_system;
#endif

	App::DestroyOverlayWrapper();

	App::DestroyContentManager();

	delete m_frame_listener;
    m_frame_listener = nullptr;

	delete gEnv->cameraManager;
	gEnv->cameraManager = nullptr;

	delete gEnv;
	gEnv = nullptr;
}

bool MainThread::SetupGameplayLoop()
{
    auto* loading_window = App::GetGuiManager()->GetLoadingWindow();
	if (!m_base_resource_loaded)
	{
		LOG("Loading base resources");

		// ============================================================================
		// Loading base resources
		// ============================================================================

		loading_window->setProgress(0, _L("Loading base resources"));

		RoR::App::GetContentManager()->AddResourcePack(ContentManager::ResourcePack::AIRFOILS);
		RoR::App::GetContentManager()->AddResourcePack(ContentManager::ResourcePack::BEAM_OBJECTS);

		RoR::App::GetContentManager()->AddResourcePack(ContentManager::ResourcePack::MATERIALS);
		RoR::App::GetContentManager()->AddResourcePack(ContentManager::ResourcePack::MESHES);
		RoR::App::GetContentManager()->AddResourcePack(ContentManager::ResourcePack::OVERLAYS);
		RoR::App::GetContentManager()->AddResourcePack(ContentManager::ResourcePack::PARTICLES);
		RoR::App::GetContentManager()->AddResourcePack(ContentManager::ResourcePack::SCRIPTS);
		RoR::App::GetContentManager()->AddResourcePack(ContentManager::ResourcePack::TEXTURES);
		RoR::App::GetContentManager()->AddResourcePack(ContentManager::ResourcePack::FLAGS);
		RoR::App::GetContentManager()->AddResourcePack(ContentManager::ResourcePack::ICONS);
		RoR::App::GetContentManager()->AddResourcePack(ContentManager::ResourcePack::FAMICONS);

		m_base_resource_loaded = true;
	}

	// ============================================================================
	// Loading settings resources
	// ============================================================================

	if (App::GetGfxWaterMode() == App::GFX_WATER_HYDRAX && !RoR::App::GetContentManager()->isLoaded(ContentManager::ResourcePack::HYDRAX.mask))
		RoR::App::GetContentManager()->AddResourcePack(ContentManager::ResourcePack::HYDRAX);

	if (App::GetGfxSkyMode() == 1 && !RoR::App::GetContentManager()->isLoaded(ContentManager::ResourcePack::CAELUM.mask))
		RoR::App::GetContentManager()->AddResourcePack(ContentManager::ResourcePack::CAELUM);

	if (App::GetGfxVegetationMode() != RoR::App::GFX_VEGETATION_NONE && !RoR::App::GetContentManager()->isLoaded(ContentManager::ResourcePack::PAGED.mask))
		RoR::App::GetContentManager()->AddResourcePack(ContentManager::ResourcePack::PAGED);

	if (App::GetGfxEnableHdr() && !RoR::App::GetContentManager()->isLoaded(ContentManager::ResourcePack::HDR.mask))
		RoR::App::GetContentManager()->AddResourcePack(ContentManager::ResourcePack::HDR);

	if (BSETTING("DOF", false) && !RoR::App::GetContentManager()->isLoaded(ContentManager::ResourcePack::DEPTH_OF_FIELD.mask))
		RoR::App::GetContentManager()->AddResourcePack(ContentManager::ResourcePack::DEPTH_OF_FIELD);

	if (App::GetGfxEnableGlow() && !RoR::App::GetContentManager()->isLoaded(ContentManager::ResourcePack::GLOW.mask))
		RoR::App::GetContentManager()->AddResourcePack(ContentManager::ResourcePack::GLOW);

	if (BSETTING("Motion blur", false) && !RoR::App::GetContentManager()->isLoaded(ContentManager::ResourcePack::BLUR.mask))
		RoR::App::GetContentManager()->AddResourcePack(ContentManager::ResourcePack::BLUR);

	if (App::GetGfxUseHeathaze() && !RoR::App::GetContentManager()->isLoaded(ContentManager::ResourcePack::HEATHAZE.mask))
		RoR::App::GetContentManager()->AddResourcePack(ContentManager::ResourcePack::HEATHAZE);

	if (App::GetGfxEnableSunburn() && !RoR::App::GetContentManager()->isLoaded(ContentManager::ResourcePack::SUNBURN.mask))
		RoR::App::GetContentManager()->AddResourcePack(ContentManager::ResourcePack::SUNBURN);

	/*if (SSETTING("Shadow technique", "") == "Parallel-split Shadow Maps" && !RoR::App::GetContentManager()->isLoaded(ContentManager::ResourcePack::PSSM.mask))
		RoR::App::GetContentManager()->AddResourcePack(ContentManager::ResourcePack::PSSM);
		*/

	Ogre::ResourceGroupManager::getSingleton().initialiseResourceGroup("LoadBeforeMap");

	// ============================================================================
	// Setup
	// ============================================================================

    BeamFactory::getSingleton().GetParticleManager().CheckAndInit();

	int colourNum = -1;

#ifdef USE_SOCKETW
	if (App::GetActiveMpState() == App::MP_STATE_CONNECTED)
	{
		wchar_t tmp[255] = L"";
		UTFString format = _L("Press %ls to start chatting");
		swprintf(tmp, 255, format.asWStr_c_str(), ANSI_TO_WCHAR(RoR::App::GetInputEngine()->getKeyForCommand(EV_COMMON_ENTER_CHATMODE)).c_str());
		App::GetGuiManager()->pushMessageChatBox(UTFString(tmp));

		user_info_t info = RoR::Networking::GetLocalUserData();
		colourNum = info.colournum;
	}
#endif // USE_SOCKETW

	// NOTE: create player _AFTER_ network, important
	gEnv->player = CharacterFactory::getSingleton().createLocal(colourNum);

	// heathaze effect
	if (BSETTING("HeatHaze", false) && RoR::App::GetContentManager()->isLoaded(ContentManager::ResourcePack::HEATHAZE.mask))
	{
		m_frame_listener->m_heathaze = new HeatHaze();
		m_frame_listener->m_heathaze->setEnable(true);
	}

	if (gEnv->cameraManager == nullptr)
	{
		// init camera manager after mygui and after we have a character
		gEnv->cameraManager = new CameraManager();
	}
	
	// ============================================================================
	// Loading map
	// ============================================================================

	if (App::GetDiagPreselectedTerrain() != "")
	{
            App::SetSimNextTerrain(App::GetDiagPreselectedTerrain());
	}

	if (App::GetSimNextTerrain().empty())
	{
		CacheEntry* selected_map = RoR::App::GetGuiManager()->GetMainSelector()->GetSelectedEntry();
		if (selected_map != nullptr)
		{
            App::SetSimNextTerrain(selected_map->fname);
		}
		else
		{
			LOG("No map selected. Returning to menu.");
            App::GetGuiManager()->SetVisible_LoadingWindow(false);
			return false;
		}
	}

    if(! LoadTerrain())
    {
        LOG("Could not load map. Returning to menu.");
        LeaveMultiplayerServer();
        App::GetGuiManager()->SetVisible_LoadingWindow(false);
        return false;
    }

	// ========================================================================
	// Loading vehicle
	// ========================================================================

	if (App::GetDiagPreselectedVehicle() != "")
	{

		if (App::GetDiagPreselectedVehConfig() != "")
		{
			LOG("Preselected Truck Config: " + App::GetDiagPreselectedVehConfig());
		}
		LOG("Preselected Truck: " + App::GetDiagPreselectedVehicle());

		const std::vector<Ogre::String> truckConfig = std::vector<Ogre::String>(1, App::GetDiagPreselectedVehConfig());

		Vector3 pos = gEnv->player->getPosition();
		Quaternion rot = Quaternion(Degree(180) - gEnv->player->getRotation(), Vector3::UNIT_Y);

		Beam* b = BeamFactory::getSingleton().CreateLocalRigInstance(pos, rot, App::GetDiagPreselectedVehicle(), -1, nullptr, false, &truckConfig);

		if (b != nullptr)
		{
			// Calculate translational offset for node[0] to align the trucks rotation center with m_reload_pos
			Vector3 translation = pos - b->getRotationCenter();
			b->resetPosition(b->nodes[0].AbsPosition + Vector3(translation.x, 0.0f, translation.z), true);

			b->updateFlexbodiesPrepare();
			b->updateFlexbodiesFinal();
			b->updateVisual();

			if (App::GetDiagPreselectedVehEnter() && b->free_node > 0)
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

    App::CreateOverlayWrapper();
    App::GetOverlayWrapper()->SetupDirectionArrow();

	if (App::GetAudioMenuMusic())
    {
		SoundScriptManager::getSingleton().trigKill(-1, SS_TRIG_MAIN_MENU);
    }

	App::CreateSceneMouse();

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
	unsigned long fpsLimit           = App::GetGfxFpsLimit();

	if (fpsLimit < 10 || fpsLimit >= 200)
	{
		fpsLimit = 0;
	}

	if (fpsLimit)
	{
		minTimePerFrame = 1000 / fpsLimit;
	}

	while (App::GetPendingAppState() == App::APP_STATE_NONE)
	{
		startTime = App::GetOgreSubsystem()->GetTimer()->getMilliseconds();

		// no more actual rendering?
		if (m_no_rendering)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
			continue;
		}

		MainMenuLoopUpdate(timeSinceLastFrame);

		if (RoR::App::GetGuiManager()->GetMainSelector()->IsFinishedSelecting())
		{
			CacheEntry* selected_map = RoR::App::GetGuiManager()->GetMainSelector()->GetSelectedEntry();
			if (selected_map != nullptr)
			{
                App::GetGuiManager()->GetMainSelector()->Reset(); // TODO: Eliminate this mechanism ~ only_a_ptr 09/2016
                App::SetPendingAppState(App::APP_STATE_SIMULATION);
                App::SetSimNextTerrain(selected_map->fname);
			}
		}

#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32 || OGRE_PLATFORM == OGRE_PLATFORM_LINUX
		RoRWindowEventUtilities::messagePump();
#endif
		Ogre::RenderWindow* rw = RoR::App::GetOgreSubsystem()->GetRenderWindow();
		if (rw->isClosed())
		{
			App::SetPendingAppState(App::APP_STATE_SHUTDOWN);
			continue;
		}

		RoR::App::GetOgreSubsystem()->GetOgreRoot()->renderOneFrame();

		if ((App::GetActiveMpState() == App::MP_STATE_CONNECTED) && RoR::Networking::CheckError())
		{
			Ogre::String title = Ogre::UTFString(_L("Network fatal error: ")).asUTF8();
			Ogre::String msg = RoR::Networking::GetErrorMessage().asUTF8();
			App::GetGuiManager()->ShowMessageBox(title, msg, true, "OK", true, false, "");
			App::SetPendingAppState(App::APP_STATE_MAIN_MENU);

			RoR::App::GetGuiManager()->GetMainSelector()->Hide();
			RoR::App::GetGuiManager()->GetMainSelector()->Show(LT_Terrain);
		}

		if (!rw->isActive() && rw->isVisible())
			rw->update(); // update even when in background !

		
		// FPS-limiter. TODO: Is this necessary in menu?
		if (fpsLimit && timeSinceLastFrame < minTimePerFrame)
		{
			// Sleep twice as long as we were too fast.
			int ms = static_cast<int>((minTimePerFrame - timeSinceLastFrame) << 1);
			std::this_thread::sleep_for(std::chrono::milliseconds(ms));
		}

		timeSinceLastFrame = RoR::App::GetOgreSubsystem()->GetTimer()->getMilliseconds() - startTime;
	}
}

void MainThread::EnterGameplayLoop()
{
	/* SETUP */
	RoR::Mirrors::Init();

	App::GetOgreSubsystem()->GetOgreRoot()->addFrameListener(m_frame_listener);

	unsigned long timeSinceLastFrame = 1;
	unsigned long startTime          = 0;
	unsigned long minTimePerFrame    = 0;
	unsigned long fpsLimit           = App::GetGfxFpsLimit();

	if (fpsLimit < 10 || fpsLimit >= 200)
	{
		fpsLimit = 0;
	}

	if (fpsLimit)
	{
		minTimePerFrame = 1000 / fpsLimit;
	}

	/* LOOP */

	while(App::GetPendingAppState() == App::APP_STATE_NONE)
	{
		startTime = RoR::App::GetOgreSubsystem()->GetTimer()->getMilliseconds();

		// no more actual rendering?
		if (m_no_rendering)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
			continue;
		}

#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32 || OGRE_PLATFORM == OGRE_PLATFORM_LINUX
		RoRWindowEventUtilities::messagePump();
#endif
		Ogre::RenderWindow* rw = RoR::App::GetOgreSubsystem()->GetRenderWindow();
		if (rw->isClosed())
		{
            App::SetPendingAppState(App::APP_STATE_SHUTDOWN);
			continue;
		}

		RoR::App::GetOgreSubsystem()->GetOgreRoot()->renderOneFrame();

        if ((App::GetActiveMpState() == App::MP_STATE_CONNECTED) && RoR::Networking::CheckError())
        {
            Ogre::String title = Ogre::UTFString(_L("Network fatal error: ")).asUTF8();
            Ogre::String msg = RoR::Networking::GetErrorMessage().asUTF8();
            App::GetGuiManager()->ShowMessageBox(title, msg, true, "OK", true, false, "");
            App::SetPendingAppState(App::APP_STATE_MAIN_MENU);
        }

		if (!rw->isActive() && rw->isVisible())
			rw->update(); // update even when in background !

		if (fpsLimit && timeSinceLastFrame < minTimePerFrame)
		{
			// Sleep twice as long as we were too fast.
			int ms = static_cast<int>((minTimePerFrame - timeSinceLastFrame) << 1);
			std::this_thread::sleep_for(std::chrono::milliseconds(ms));
		}

		timeSinceLastFrame = RoR::App::GetOgreSubsystem()->GetTimer()->getMilliseconds() - startTime;
	}

	/* RESTORE ENVIRONMENT */

	App::GetOgreSubsystem()->GetOgreRoot()->removeFrameListener(m_frame_listener);
}

void MainThread::Exit()
{
	RoR::App::GetOgreSubsystem()->GetOgreRoot()->removeFrameListener(m_frame_listener);
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

	if (RoR::App::GetOgreSubsystem()->GetRenderWindow()->isClosed())
	{
		App::SetPendingAppState(App::APP_STATE_SHUTDOWN);
		return;
	}

#ifdef USE_SOCKETW
	if (App::GetActiveMpState() == App::MP_STATE_CONNECTED)
	{
        App::GetGuiManager()->GetMpClientList()->update();
	}
#endif // USE_SOCKETW

	RoR::App::GetInputEngine()->Capture();

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

	RoR::App::GetInputEngine()->updateKeyBounces(seconds_since_last_frame);

	if (! RoR::App::GetInputEngine()->getInputsChanged())
	{
		return;
	}

	if (RoR::App::GetOverlayWrapper() != nullptr)
	{
		RoR::App::GetOverlayWrapper()->update(seconds_since_last_frame); // TODO: What's the meaning of this? It only updates some internal timer. ~ only_a_ptr
	}

	if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_ENTER_CHATMODE, 0.5f) && !m_frame_listener->m_hide_gui)
	{
		//TODO: Separate Chat and console
	}

	if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_QUIT_GAME))
	{
		//TODO: Go back to menu 
		App::SetPendingAppState(App::APP_STATE_SHUTDOWN);
		return;
	}

    auto gui_man = App::GetGuiManager();
    if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_CONSOLE_TOGGLE, 5.f))
    {
        gui_man->SetVisible_Console(!gui_man->IsVisible_Console());
    }

	// TODO: screenshot

	// FOV settings disabled in menu

	// FIXME: full screen/windowed screen switching
	//if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_FULLSCREEN_TOGGLE, 2.0f)) {}
}

bool MainThread::LoadTerrain()
{
	// check if the resource is loaded
	Ogre::String terrain_file = App::GetSimNextTerrain();
    App::SetSimNextTerrain("");
	if (! RoR::App::GetCacheSystem()->checkResourceLoaded(terrain_file)) // Input-output argument.
	{
		// fallback for terrains, add .terrn if not found and retry
		terrain_file = Ogre::StringUtil::replaceAll(terrain_file, ".terrn2", "");
		terrain_file = Ogre::StringUtil::replaceAll(terrain_file, ".terrn", "");
		terrain_file = terrain_file + ".terrn2";
		if (!RoR::App::GetCacheSystem()->checkResourceLoaded(terrain_file))
		{
			LOG("Terrain not found: " + terrain_file);
            Ogre::UTFString title(_L("Terrain loading error"));
            Ogre::UTFString msg(_L("Terrain not found: ") + terrain_file);
            App::GetGuiManager()->ShowMessageBox(title.asUTF8(), msg.asUTF8(), true, "OK", true, false, "");
            return false;
		}
	}

	App::GetGuiManager()->GetLoadingWindow()->setProgress(0, _L("Loading Terrain"));

	LOG("Loading terrain: " + terrain_file);

	if (gEnv->terrainManager != nullptr)
	{
		// remove old terrain
		delete(gEnv->terrainManager); // TODO: do it when leaving simulation.
	}

	gEnv->terrainManager = new TerrainManager();
	gEnv->terrainManager->loadTerrain(terrain_file);
    App::SetSimActiveTerrain(terrain_file);

#ifdef USE_MYGUI
    App::GetGuiManager()->FrictionSettingsUpdateCollisions();
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
	App::GetGuiManager()->SetVisible_LoadingWindow(false);
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
    auto loading_window = App::GetGuiManager()->GetLoadingWindow();

	loading_window->setProgress(0, _L("Unloading Terrain"));
	
	RoR::App::GetGuiManager()->GetMainSelector()->Reset();

	m_frame_listener->StopRaceTimer();

	RoR::App::DestroyOverlayWrapper();

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

	App::DeleteSceneMouse();
	loading_window->setProgress(75, _L("Unloading Terrain"));

	//Reinit few things
	loading_window->setProgress(100, _L("Unloading Terrain"));
	// hide loading window
    App::GetGuiManager()->SetVisible_LoadingWindow(false);
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

    auto gui = App::GetGuiManager();
    gui->SetVisible_MultiplayerSelector(false);
    gui->SetVisible_GameMainMenu(false);

    gui->GetLoadingWindow()->setAutotrack(_L("Connecting to server ..."));

    if (!Networking::Connect())
    {
        LOG("connection failed. server down?");
        gui->SetVisible_LoadingWindow(false);
        gui->SetVisible_GameMainMenu(true);

        gui->ShowMessageBox("Connection failed", Networking::GetErrorMessage().asUTF8_c_str(), true, "OK", true, false, "");
        return;
    }

    gui->SetVisible_LoadingWindow(false);
    gui->SetVisible_MpClientList(true);
    gui->GetMpClientList()->update();

    ChatSystem::SendStreamSetup();

#ifdef USE_MUMBLE
    if (! m_is_mumble_created)
    {
        new MumbleIntegration();
        m_is_mumble_created = true;
    }
#endif // USE_MUMBLE

    String terrain_name = Networking::GetTerrainName();
    if (terrain_name != "any")
    {
        App::SetSimNextTerrain(terrain_name);
        App::SetPendingAppState(App::APP_STATE_SIMULATION);
    }
    else
    {
        // Connected -> go directly to map selector
		if (App::GetDiagPreselectedTerrain() == "")
		{
			gui->GetMainSelector()->Reset();
			gui->GetMainSelector()->Show(LT_Terrain);
		}
		else
		{
			App::SetPendingAppState(App::APP_STATE_SIMULATION);
		}
    }
#endif //SOCKETW
}

void MainThread::LeaveMultiplayerServer()
{
#ifdef USE_SOCKETW
    if (App::GetActiveMpState() == App::MP_STATE_CONNECTED)
    {
        App::GetGuiManager()->GetLoadingWindow()->setAutotrack(_L("Disconnecting, wait 10 seconds ..."));
        RoR::Networking::Disconnect();
        App::GetGuiManager()->SetVisible_MpClientList(false);
        App::GetGuiManager()->SetVisible_LoadingWindow(false);
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
		if (RoR::App::GetOverlayWrapper())
		{
			RoR::App::GetOverlayWrapper()->showDashboardOverlays(false, previous_vehicle);
		}

#ifdef USE_OPENAL
		SoundScriptManager::getSingleton().trigStop(previous_vehicle, SS_TRIG_AIR);
		SoundScriptManager::getSingleton().trigStop(previous_vehicle, SS_TRIG_PUMP);
#endif // OPENAL
	}

    auto frame_listener = App::GetMainThreadLogic()->GetFrameListener();

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

		frame_listener->m_forcefeedback.SetEnabled(false);

		// hide truckhud
		if (RoR::App::GetOverlayWrapper()) RoR::App::GetOverlayWrapper()->truckhud->show(false);

		TRIGGER_EVENT(SE_TRUCK_EXIT, previous_vehicle?previous_vehicle->trucknum:-1);
	}
    else
	{
		// getting inside
		if (RoR::App::GetOverlayWrapper() && ! frame_listener->m_hide_gui)
		{
			RoR::App::GetOverlayWrapper()->showDashboardOverlays(true, current_vehicle);
		}

		// hide unused items
		if (RoR::App::GetOverlayWrapper() && current_vehicle->free_active_shock==0)
		{
			(OverlayManager::getSingleton().getOverlayElement("tracks/rollcorneedle"))->hide();
		}

		// force feedback
		frame_listener->m_forcefeedback.SetEnabled(current_vehicle->driveable==TRUCK); //only for trucks so far

		// attach player to truck
		if (gEnv->player)
		{
			gEnv->player->setBeamCoupling(true, current_vehicle);
		}

		if (RoR::App::GetOverlayWrapper())
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
	RoR::App::GetCacheSystem()->Startup(true); // true = force regeneration

	if (BSETTING("regen-cache-only", false)) //Only if game run just for regen
	{
		// Get stats
		int num_new = RoR::App::GetCacheSystem()->newFiles;
		int num_changed = RoR::App::GetCacheSystem()->changedFiles;
		int num_deleted = RoR::App::GetCacheSystem()->deletedFiles;

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

