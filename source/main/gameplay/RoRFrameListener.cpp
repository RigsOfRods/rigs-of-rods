/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer

    For more information, see http://www.rigsofrods.org/

    Rigs of Rods is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 3, as
    published by the Free Software Foundation.

    Rigs of Rods is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Rigs of Rods. If not, see <http://www.gnu.org/licenses/>.
*/

#include "RoRFrameListener.h"

#include "AdvancedScreen.h"
#include "AircraftSimulation.h"
#include "Application.h"
#include "Beam.h"
#include "BeamEngine.h"
#include "BeamFactory.h"
#include "CacheSystem.h"
#include "CameraManager.h"
#include "Character.h"
#include "CharacterFactory.h"
#include "ChatSystem.h"
#include "CollisionTools.h"
#include "Collisions.h"
#include "ContentManager.h"
#include "DashBoardManager.h"
#include "DepthOfFieldEffect.h"
#include "DustManager.h"
#include "EnvironmentMap.h"
#include "ForceFeedback.h"
#include "GUI_LoadingWindow.h"
#include "GUI_TeleportWindow.h"
#include "GUI_TopMenubar.h"
#include "GUIManager.h"
#include "Heathaze.h"
#include "IWater.h"
#include "InputEngine.h"
#include "LandVehicleSimulation.h"
#include "Language.h"
#include "MainMenu.h"

#include "MumbleIntegration.h"
#include "OgreSubsystem.h"
#include "OutProtocol.h"
#include "OverlayWrapper.h"
#include "Replay.h"
#include "RoRVersion.h"
#include "SceneMouse.h"
#include "ScrewProp.h"
#include "Scripting.h"
#include "Settings.h"
#include "SkyManager.h"
#include "SoundScriptManager.h"
#include "TerrainManager.h"
#include "TerrainObjectManager.h"
#include "Utils.h"
#include "Water.h"

#include "GUIManager.h"
#include "GUI_GameConsole.h"
#include "GUI_FrictionSettings.h"
#include "GUI_MultiplayerClientList.h"
#include "GUI_MainSelector.h"
#include "GUI_SimUtils.h"

#include "SurveyMapManager.h"
#include "SurveyMapEntity.h"

#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>

#ifdef USE_MPLATFORM
#include "MPlatformFD.h"
#endif //USE_MPLATFORM

#ifdef FEAT_TIMING
#include "BeamStats.h"
#endif //FEAT_TIMING

#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
#include <Windows.h>
#else
#include <stdio.h>
#include <wchar.h>
#endif

// some gcc fixes
#if OGRE_PLATFORM == OGRE_PLATFORM_LINUX
#pragma GCC diagnostic ignored "-Wfloat-equal"
#endif //OGRE_PLATFORM_LINUX

#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE
//#include <CFUserNotification.h>
#endif

using namespace Ogre;
using namespace RoR;

#define simRUNNING(_S_) (_S_ == SimState::RUNNING    )
#define  simPAUSED(_S_) (_S_ == SimState::PAUSED     )
#define  simSELECT(_S_) (_S_ == SimState::SELECTING  )
#define  simEDITOR(_S_) (_S_ == SimState::EDITOR_MODE)

RoRFrameListener::RoRFrameListener(RoR::ForceFeedback* ff, RoR::SkidmarkConfig* skid_conf) :
    m_player_actor(nullptr),
    m_prev_player_actor(nullptr),
    m_actor_manager(this),
    m_character_factory(),
    m_dir_arrow_pointed(Vector3::ZERO),
    m_heathaze(nullptr),
    m_force_feedback(ff),
    m_skidmark_conf(skid_conf),
    m_hide_gui(false),
    m_was_app_window_closed(false),
    m_is_dir_arrow_visible(false),
    m_is_pace_reset_pressed(false),
    m_last_cache_selection(nullptr),
    m_last_screenshot_date(""),
    m_last_screenshot_id(1),
    m_last_simulation_speed(0.1f),
    m_last_skin_selection(nullptr),
    m_netcheck_gui_timer(0.0f),
    m_pressure_pressed(false),
    m_race_bestlap_time(0),
    m_race_in_progress(false),
    m_race_start_time(0),
    m_reload_box(0),
    m_reload_dir(Quaternion::IDENTITY),
    m_reload_pos(Vector3::ZERO),
    m_stats_on(0),
    m_time(0),
    m_time_until_next_toggle(0),
    m_advanced_vehicle_repair(false),
    m_advanced_vehicle_repair_timer(0.f),
    m_actor_info_gui_visible(false)
{
}

void RoRFrameListener::UpdateForceFeedback(float dt)
{
    if (!App::io_ffb_enabled.GetActive()) { return; }

    if (!RoR::App::GetInputEngine()->getForceFeedbackDevice())
    {
         // TODO: <rant> Per-frame check? How about on-session-start check? </rant> ~only_a_ptr, 02/2017
        LOG("No force feedback device detected, disabling force feedback");
        App::io_ffb_enabled.SetActive(false);
        return;
    }

    if (m_player_actor && m_player_actor->ar_driveable == TRUCK)
    {
        int ar_camera_node_pos = 0;
        int ar_camera_node_dir = 0;
        int ar_camera_node_roll = 0;

        // TODO: <rant> Per-frame validity check? How about on-spawn check? </rant>
        // If the camera node is invalid, the FF should be disabled right away, not try to fall back to node0
        // TODO: Check cam. nodes once on spawn! They never change --> no reason to repeat the check. ~only_a_ptr, 06/2017
                // ~only_a_ptr, 02/2017
        if (m_player_actor->IsNodeIdValid(m_player_actor->ar_camera_node_pos[0]))
            ar_camera_node_pos = m_player_actor->ar_camera_node_pos[0];
        if (m_player_actor->IsNodeIdValid(m_player_actor->ar_camera_node_dir[0]))
            ar_camera_node_dir = m_player_actor->ar_camera_node_dir[0];
        if (m_player_actor->IsNodeIdValid(m_player_actor->ar_camera_node_roll[0]))
            ar_camera_node_roll = m_player_actor->ar_camera_node_roll[0];

        Vector3 udir = m_player_actor->ar_nodes[ar_camera_node_pos].RelPosition - m_player_actor->ar_nodes[ar_camera_node_dir].RelPosition;
        Vector3 uroll = m_player_actor->ar_nodes[ar_camera_node_pos].RelPosition - m_player_actor->ar_nodes[ar_camera_node_roll].RelPosition;

        udir.normalise();
        uroll.normalise();

        Ogre::Vector3 ff_vehicle = m_player_actor->GetFFbBodyForces();
        m_force_feedback->SetForces(
            -ff_vehicle.dotProduct(uroll) / 10000.0,
             ff_vehicle.dotProduct(udir)  / 10000.0,
            m_player_actor->ar_wheel_speed,
            m_player_actor->ar_hydro_dir_command,
            m_player_actor->GetFFbHydroForces());
    }
}

void RoRFrameListener::StartRaceTimer()
{
    m_race_start_time = (int)m_time;
    m_race_in_progress = true;
    OverlayWrapper* ow = RoR::App::GetOverlayWrapper();
    if (ow)
    {
        ow->ShowRacingOverlay();
        ow->laptimes->show();
        ow->laptimems->show();
        ow->laptimemin->show();
    }
}

float RoRFrameListener::StopRaceTimer()
{
    float time;

    if (m_race_in_progress)
    {
        time = static_cast<float>(m_time - m_race_start_time);
        m_race_bestlap_time = time;
    }

    // let the display on
    OverlayWrapper* ow = RoR::App::GetOverlayWrapper();
    if (ow)
    {
        wchar_t txt[256] = L"";
        UTFString fmt = _L("Last lap: %.2i'%.2i.%.2i");
        swprintf(txt, 256, fmt.asWStr_c_str(), ((int)(m_race_bestlap_time)) / 60, ((int)(m_race_bestlap_time)) % 60, ((int)(m_race_bestlap_time * 100.0)) % 100);
        ow->lasttime->setCaption(UTFString(txt));
        //ow->m_racing_overlay->hide();
        ow->laptimes->hide();
        ow->laptimems->hide();
        ow->laptimemin->hide();
    }
    m_race_start_time = 0;
    m_race_in_progress = false;
    return m_race_bestlap_time;
}

void RoRFrameListener::UpdateRacingGui()
{
    OverlayWrapper* ow = RoR::App::GetOverlayWrapper();
    if (!ow)
        return;
    // update m_racing_overlay gui if required
    float time = static_cast<float>(m_time - m_race_start_time);
    wchar_t txt[10];
    swprintf(txt, 10, L"%.2i", ((int)(time * 100.0)) % 100);
    ow->laptimems->setCaption(txt);
    swprintf(txt, 10, L"%.2i", ((int)(time)) % 60);
    ow->laptimes->setCaption(txt);
    swprintf(txt, 10, L"%.2i'", ((int)(time)) / 60);
    ow->laptimemin->setCaption(UTFString(txt));
}

bool RoRFrameListener::UpdateInputEvents(float dt)
{
    if (dt == 0.0f)
        return true;

    auto s = App::sim_state.GetActive();
    auto gui_man = App::GetGuiManager();

    RoR::App::GetInputEngine()->updateKeyBounces(dt);
    if (!RoR::App::GetInputEngine()->getInputsChanged())
        return true;

    // update overlays if enabled
    if (RoR::App::GetOverlayWrapper())
        RoR::App::GetOverlayWrapper()->update(dt);

    if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_QUIT_GAME))
    {
        if (gui_man->IsVisible_MainSelector())
        {
            gui_man->GetMainSelector()->Cancel();
        }
        else if (simRUNNING(s))
        {
            App::sim_state.SetPending(SimState::PAUSED);
        }
        else if (simPAUSED(s))
        {
            App::sim_state.SetPending(SimState::RUNNING);
        }
    }

    if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_CONSOLE_TOGGLE))
    {
        gui_man->SetVisible_Console(! gui_man->IsVisible_Console());
    }

    if ((m_player_actor == nullptr) && RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_TELEPORT_TOGGLE))
    {
        gui_man->SetVisible_TeleportWindow(! gui_man->IsVisible_TeleportWindow());
    }

    if (App::sim_state.GetActive() == SimState::PAUSED)
        return true; //Stop everything when pause menu is visible

    if (gui_man->IsVisible_FrictionSettings() && m_player_actor)
    {
        ground_model_t* gm = m_player_actor->getLastFuzzyGroundModel();

        gui_man->GetFrictionSettings()->setActiveCol(gm);
    }

    const bool mp_connected = (App::mp_state.GetActive() == MpState::CONNECTED);
    if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_ENTER_CHATMODE, 0.5f) && !m_hide_gui && mp_connected)
    {
        RoR::App::GetInputEngine()->resetKeys();
        gui_man->SetVisible_ChatBox(true);
    }

    if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_SCREENSHOT, 0.25f))
    {
        std::time_t t = std::time(nullptr);
        std::stringstream date;
#if defined(__GNUC__) && (__GNUC__ < 5)
		date << std::asctime(std::localtime(&t));
#else
        date << std::put_time(std::localtime(&t), "%Y-%m-%d_%H-%M-%S");
#endif

        String fn_prefix = std::string(App::sys_screenshot_dir.GetActive()) + PATH_SLASH + String("screenshot_");
        String fn_name = date.str() + String("_");
        String fn_suffix = String(".") + App::app_screenshot_format.GetActive();

        if (m_last_screenshot_date == date.str())
        {
            m_last_screenshot_id++;
        }
        else
        {
            m_last_screenshot_id = 1;
        }
        m_last_screenshot_date = date.str();

        fn_name = fn_name + TOSTRING(m_last_screenshot_id);

        String tmpfn = fn_prefix + fn_name + fn_suffix;

        RoR::App::GetGuiManager()->HideNotification();
        RoR::App::GetGuiManager()->SetMouseCursorVisibility(RoR::GUIManager::MouseCursorVisibility::HIDDEN);

        m_actor_manager.UpdateFlexbodiesFinal(); // Waits until all flexbody tasks are finished

        if (App::app_screenshot_format.GetActive() == "png")
        {
            // add some more data into the image
            AdvancedScreen* as = new AdvancedScreen(RoR::App::GetOgreSubsystem()->GetRenderWindow(), tmpfn);
            //as->addData("terrain_Name", loadedTerrain);
            //as->addData("terrain_ModHash", terrainModHash);
            //as->addData("terrain_FileHash", terrainFileHash);
            as->addData("Truck_Num", TOSTRING(m_player_actor->ar_instance_id));
            if (m_player_actor)
            {
                as->addData("Truck_fname", m_player_actor->ar_filename);
                as->addData("Truck_name", m_player_actor->GetActorDesignName());
                as->addData("Truck_beams", TOSTRING(m_player_actor->ar_num_beams));
                as->addData("Truck_nodes", TOSTRING(m_player_actor->ar_num_nodes));
            }
            as->addData("User_NickName", App::mp_player_name.GetActive());
            as->addData("User_Language", App::app_language.GetActive());
            as->addData("RoR_VersionString", String(ROR_VERSION_STRING));
            as->addData("RoR_ProtocolVersion", String(RORNET_VERSION));
            as->addData("RoR_BinaryHash", "");
            as->addData("MP_ServerName", App::mp_server_host.GetActive());
            as->addData("MP_ServerPort", TOSTRING(App::mp_server_port.GetActive()));
            as->addData("MP_NetworkEnabled", (App::mp_state.GetActive() == MpState::CONNECTED) ? "Yes" : "No");
            as->addData("Camera_Mode", gEnv->cameraManager ? TOSTRING(gEnv->cameraManager->getCurrentBehavior()) : "None");
            as->addData("Camera_Position", TOSTRING(gEnv->mainCamera->getPosition()));

            const RenderTarget::FrameStats& stats = RoR::App::GetOgreSubsystem()->GetRenderWindow()->getStatistics();
            as->addData("AVGFPS", TOSTRING(stats.avgFPS));

            as->write();
            delete(as);
        }
        else
        {
            RoR::App::GetOgreSubsystem()->GetRenderWindow()->update();
            RoR::App::GetOgreSubsystem()->GetRenderWindow()->writeContentsToFile(tmpfn);
        }

        App::GetGuiManager()->SetMouseCursorVisibility(GUIManager::MouseCursorVisibility::VISIBLE);

        // show new flash message
        String ssmsg = _L("Screenshot:") + String(" ") + fn_name + fn_suffix;
        LOG(ssmsg);
        RoR::App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE, ssmsg, "camera.png", 10000, false);
        RoR::App::GetGuiManager()->PushNotification("Notice:", ssmsg);
    }

    if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_TRUCKEDIT_RELOAD, 0.5f) && m_player_actor)
    {
        this->ReloadPlayerActor();
        return true;
    }

    // position storage
    if (m_player_actor && App::sim_position_storage.GetActive())
    {
        int res = -10, slot = -1;
        if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_SAVE_POS01, 0.5f))
        {
            slot = 0;
            res = m_player_actor->savePosition(slot);
        };
        if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_SAVE_POS02, 0.5f))
        {
            slot = 1;
            res = m_player_actor->savePosition(slot);
        };
        if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_SAVE_POS03, 0.5f))
        {
            slot = 2;
            res = m_player_actor->savePosition(slot);
        };
        if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_SAVE_POS04, 0.5f))
        {
            slot = 3;
            res = m_player_actor->savePosition(slot);
        };
        if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_SAVE_POS05, 0.5f))
        {
            slot = 4;
            res = m_player_actor->savePosition(slot);
        };
        if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_SAVE_POS06, 0.5f))
        {
            slot = 5;
            res = m_player_actor->savePosition(slot);
        };
        if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_SAVE_POS07, 0.5f))
        {
            slot = 6;
            res = m_player_actor->savePosition(slot);
        };
        if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_SAVE_POS08, 0.5f))
        {
            slot = 7;
            res = m_player_actor->savePosition(slot);
        };
        if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_SAVE_POS09, 0.5f))
        {
            slot = 8;
            res = m_player_actor->savePosition(slot);
        };
        if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_SAVE_POS10, 0.5f))
        {
            slot = 9;
            res = m_player_actor->savePosition(slot);
        };
        if (slot != -1 && !res)
        {
            RoR::App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE, _L("Position saved under slot ") + TOSTRING(slot + 1), "infromation.png");
            RoR::App::GetGuiManager()->PushNotification("Notice:", _L("Position saved under slot ") + TOSTRING(slot + 1));
        }
        else if (slot != -1 && res)
        {
            RoR::App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE, _L("Error while saving position saved under slot ") + TOSTRING(slot + 1), "error.png");
            RoR::App::GetGuiManager()->PushNotification("Notice:", _L("Error while saving position saved under slot ") + TOSTRING(slot + 1));
        }

        if (res == -10)
        {
            if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_LOAD_POS01, 0.5f))
            {
                slot = 0;
                res = m_player_actor->loadPosition(slot);
            };
            if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_LOAD_POS02, 0.5f))
            {
                slot = 1;
                res = m_player_actor->loadPosition(slot);
            };
            if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_LOAD_POS03, 0.5f))
            {
                slot = 2;
                res = m_player_actor->loadPosition(slot);
            };
            if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_LOAD_POS04, 0.5f))
            {
                slot = 3;
                res = m_player_actor->loadPosition(slot);
            };
            if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_LOAD_POS05, 0.5f))
            {
                slot = 4;
                res = m_player_actor->loadPosition(slot);
            };
            if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_LOAD_POS06, 0.5f))
            {
                slot = 5;
                res = m_player_actor->loadPosition(slot);
            };
            if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_LOAD_POS07, 0.5f))
            {
                slot = 6;
                res = m_player_actor->loadPosition(slot);
            };
            if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_LOAD_POS08, 0.5f))
            {
                slot = 7;
                res = m_player_actor->loadPosition(slot);
            };
            if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_LOAD_POS09, 0.5f))
            {
                slot = 8;
                res = m_player_actor->loadPosition(slot);
            };
            if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_LOAD_POS10, 0.5f))
            {
                slot = 9;
                res = m_player_actor->loadPosition(slot);
            };
            if (slot != -1 && res == 0)
            {
                RoR::App::GetGuiManager()->PushNotification("Notice:", _L("Loaded position from slot ") + TOSTRING(slot + 1));
                RoR::App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE, _L("Loaded position from slot ") + TOSTRING(slot + 1), "infromation.png");
            }
            else if (slot != -1 && res != 0)
            {
                RoR::App::GetGuiManager()->PushNotification("Notice:", _L("Could not load position from slot ") + TOSTRING(slot + 1));
                RoR::App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE, _L("Could not load position from slot ") + TOSTRING(slot + 1), "error.png");
            }
        }
    }

    // camera FOV settings
    const bool fov_less = RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_FOV_LESS, 0.1f);
    const bool fov_more = RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_FOV_MORE, 0.1f);
    if (fov_less || fov_more)
    {
        float fov = gEnv->mainCamera->getFOVy().valueDegrees();
        fov = (fov_less) ? (fov - 1.f) : (fov + 1.f);
        fov = Round(fov);

        if (fov >= 10 && fov <= 160)
        {
            gEnv->mainCamera->setFOVy(Degree(fov));

            RoR::App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE, _L("FOV: ") + TOSTRING(fov), "camera_edit.png", 2000);
            RoR::App::GetGuiManager()->PushNotification("Notice:", _L("FOV: ") + TOSTRING(fov));

            // save the settings
            if (gEnv->cameraManager &&
                gEnv->cameraManager->hasActiveBehavior() &&
                gEnv->cameraManager->getCurrentBehavior() == RoR::PerVehicleCameraContext::CAMCTX_BEHAVIOR_VEHICLE_CINECAM)
            {
                App::gfx_fov_internal.SetActive(fov);
            }
            else
            {
                App::gfx_fov_external.SetActive(fov);
            }
        }
        else
        {
            RoR::App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE, _L("FOV: Limit reached"), "camera_edit.png", 2000);
            RoR::App::GetGuiManager()->PushNotification("Notice:", _L("FOV: Limit reached") + TOSTRING(""));
        }
    }

    // full screen/windowed screen switching
    if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_FULLSCREEN_TOGGLE, 2.0f))
    {
        static int org_width = -1, org_height = -1;
        int width = RoR::App::GetOgreSubsystem()->GetRenderWindow()->getWidth();
        int height = RoR::App::GetOgreSubsystem()->GetRenderWindow()->getHeight();
        if (org_width == -1)
            org_width = width;
        if (org_height == -1)
            org_height = height;
        bool mode = RoR::App::GetOgreSubsystem()->GetRenderWindow()->isFullScreen();
        if (!mode)
        {
            RoR::App::GetOgreSubsystem()->GetRenderWindow()->setFullscreen(true, org_width, org_height);
            LOG(" ** switched to fullscreen: "+TOSTRING(width)+"x"+TOSTRING(height));
        }
        else
        {
            RoR::App::GetOgreSubsystem()->GetRenderWindow()->setFullscreen(false, 640, 480);
            RoR::App::GetOgreSubsystem()->GetRenderWindow()->setFullscreen(false, org_width, org_height);
            LOG(" ** switched to windowed mode: ");
        }
    }

    static std::vector<TerrainObjectManager::EditorObject> object_list;
    static bool terrain_editing_track_object = true;
    static bool terrain_editing_mode = false;
    static int terrain_editing_rotation_axis = 1;
    static int object_index = -1;

    if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_TOGGLE_TERRAIN_EDITOR))
    {
        terrain_editing_mode = !terrain_editing_mode;
        App::sim_state.SetActive(terrain_editing_mode ? SimState::EDITOR_MODE : SimState::RUNNING);
        UTFString ssmsg = terrain_editing_mode ? _L("Entered terrain editing mode") : _L("Left terrain editing mode");
        RoR::App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE, ssmsg, "infromation.png", 2000, false);
        RoR::App::GetGuiManager()->PushNotification("Notice:", ssmsg);

        if (terrain_editing_mode)
        {
            object_list = App::GetSimTerrain()->getObjectManager()->GetEditorObjects();
            object_index = -1;
        }
        else
        {
            std::string path = std::string(RoR::App::sys_config_dir.GetActive()) + PATH_SLASH + "editor_out.cfg";
            std::ofstream file(path);
            if (file.is_open())
            {
                for (auto object : object_list)
                {
                    SceneNode* sn = object.node;
                    if (sn != nullptr)
                    {
                        String pos = TOSTRING(object.position.x) + ", " + TOSTRING(object.position.y) + ", " + TOSTRING(object.position.z);
                        String rot = TOSTRING(object.rotation.x) + ", " + TOSTRING(object.rotation.y) + ", " + TOSTRING(object.rotation.z);

                        file << pos + ", " + rot + ", " + object.name + "\n";
                    }
                }
                file.close();
            }
            else
            {
                LOG("Cannot write '" + path + "'");
            }
        }
    }

    //OLD m_loading_state == ALL_LOADED
    if (simEDITOR(s) && object_list.size() > 0)
    {
        bool update = false;
        if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_ENTER_OR_EXIT_TRUCK))
        {
            if (object_index == -1)
            {
                // Select nearest object
                Vector3 ref_pos = gEnv->cameraManager->gameControlsLocked() ? gEnv->mainCamera->getPosition() : gEnv->player->getPosition();
                float min_dist = std::numeric_limits<float>::max();
                for (int i = 0; i < (int)object_list.size(); i++)
                {
                    float dist = ref_pos.squaredDistance(object_list[i].node->getPosition());
                    if (dist < min_dist)
                    {
                        object_index = i;
                        min_dist = dist;
                        update = true;
                    }
                }
            }
            else
            {
                object_index = -1;
            }
        }
        else if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_ENTER_NEXT_TRUCK, 0.25f))
        {
            object_index = (object_index + 1 + (int)object_list.size()) % object_list.size();
            update = true;
        }
        else if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_ENTER_PREVIOUS_TRUCK, 0.25f))
        {
            object_index = (object_index - 1 + (int)object_list.size()) % object_list.size();
            update = true;
        }
        if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_RESCUE_TRUCK))
        {
            UTFString axis = _L("ry");
            if (terrain_editing_rotation_axis == 0)
            {
                axis = _L("ry");
                terrain_editing_rotation_axis = 1;
            }
            else if (terrain_editing_rotation_axis == 1)
            {
                axis = _L("rz");
                terrain_editing_rotation_axis = 2;
            }
            else if (terrain_editing_rotation_axis == 2)
            {
                axis = _L("rx");
                terrain_editing_rotation_axis = 0;
            }
            UTFString ssmsg = _L("Rotating: ") + axis;
            RoR::App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE, ssmsg, "infromation.png", 2000, false);
            RoR::App::GetGuiManager()->PushNotification("Notice:", ssmsg);
        }
        if (RoR::App::GetInputEngine()->isKeyDownValueBounce(OIS::KC_SPACE))
        {
            terrain_editing_track_object = !terrain_editing_track_object;
            UTFString ssmsg = terrain_editing_track_object ? _L("Enabled object tracking") : _L("Disabled object tracking");
            RoR::App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE, ssmsg, "infromation.png", 2000, false);
            RoR::App::GetGuiManager()->PushNotification("Notice:", ssmsg);
        }
        if (object_index != -1 && update)
        {
            String ssmsg = _L("Selected object: [") + TOSTRING(object_index) + "/" + TOSTRING(object_list.size()) + "] (" + object_list[object_index].name + ")";
            RoR::App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE, ssmsg, "infromation.png", 2000, false);
            RoR::App::GetGuiManager()->PushNotification("Notice:", ssmsg);
            if (terrain_editing_track_object)
            {
                gEnv->player->setPosition(object_list[object_index].node->getPosition());
                //gEnv->cameraManager->NotifyContextChange();
            }
        }
        if (object_index != -1 && RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_RESET_TRUCK))
        {
            SceneNode* sn = object_list[object_index].node;

            object_list[object_index].position = object_list[object_index].initial_position;
            sn->setPosition(object_list[object_index].position);

            object_list[object_index].rotation = object_list[object_index].initial_rotation;
            Vector3 rot = object_list[object_index].rotation;
            sn->setOrientation(Quaternion(Degree(rot.x), Vector3::UNIT_X) * Quaternion(Degree(rot.y), Vector3::UNIT_Y) * Quaternion(Degree(rot.z), Vector3::UNIT_Z));
            sn->pitch(Degree(-90));
        }
        if (object_index != -1 && gEnv->cameraManager && !gEnv->cameraManager->gameControlsLocked())
        {
            SceneNode* sn = object_list[object_index].node;

            Vector3 translation = Vector3::ZERO;
            float rotation = 0.0f;

            if (RoR::App::GetInputEngine()->getEventBoolValue(EV_TRUCK_STEER_LEFT))
            {
                rotation += 2.0f;
            }
            else if (RoR::App::GetInputEngine()->getEventBoolValue(EV_TRUCK_STEER_RIGHT))
            {
                rotation -= 2.0f;
            }
            if (RoR::App::GetInputEngine()->getEventBoolValue(EV_TRUCK_ACCELERATE))
            {
                translation.y += 0.5f;
            }
            else if (RoR::App::GetInputEngine()->getEventBoolValue(EV_TRUCK_BRAKE))
            {
                translation.y -= 0.5f;
            }
            if (RoR::App::GetInputEngine()->getEventBoolValue(EV_CHARACTER_FORWARD))
            {
                translation.x += 0.5f;
            }
            else if (RoR::App::GetInputEngine()->getEventBoolValue(EV_CHARACTER_BACKWARDS))
            {
                translation.x -= 0.5f;
            }
            if (RoR::App::GetInputEngine()->getEventBoolValue(EV_CHARACTER_SIDESTEP_RIGHT))
            {
                translation.z += 0.5f;
            }
            else if (RoR::App::GetInputEngine()->getEventBoolValue(EV_CHARACTER_SIDESTEP_LEFT))
            {
                translation.z -= 0.5f;
            }

            if (translation != Vector3::ZERO || rotation != 0.0f)
            {
                float scale = RoR::App::GetInputEngine()->isKeyDown(OIS::KC_LMENU) ? 0.1f : 1.0f;
                scale *= RoR::App::GetInputEngine()->isKeyDown(OIS::KC_LSHIFT) ? 3.0f : 1.0f;
                scale *= RoR::App::GetInputEngine()->isKeyDown(OIS::KC_LCONTROL) ? 10.0f : 1.0f;

                object_list[object_index].position += translation * scale * dt;
                sn->setPosition(object_list[object_index].position);

                object_list[object_index].rotation[terrain_editing_rotation_axis] += rotation * scale * dt;
                Vector3 rot = object_list[object_index].rotation;
                sn->setOrientation(Quaternion(Degree(rot.x), Vector3::UNIT_X) * Quaternion(Degree(rot.y), Vector3::UNIT_Y) * Quaternion(Degree(rot.z), Vector3::UNIT_Z));
                sn->pitch(Degree(-90));

                if (terrain_editing_track_object)
                {
                    gEnv->player->setPosition(object_list[object_index].node->getPosition());
                }
            }
        }
        else
        {
            m_character_factory.update(dt);
        }
    }
    else if (simRUNNING(s) || simPAUSED(s))
    {
        m_character_factory.update(dt);
        if (gEnv->cameraManager && !gEnv->cameraManager->gameControlsLocked())
        {
            if (m_player_actor) // we are in a vehicle
            {
                if (RoR::App::GetInputEngine()->getEventBoolValue(EV_COMMON_RESET_TRUCK) && !m_player_actor->ar_replay_mode)
                {
                    this->StopRaceTimer();
                    m_player_actor->RequestActorReset();
                }
                else if (RoR::App::GetInputEngine()->getEventBoolValue(EV_COMMON_REMOVE_CURRENT_TRUCK) && !m_player_actor->ar_replay_mode)
                {
                    this->StopRaceTimer();
                    Vector3 center = m_player_actor->GetRotationCenter();
                    this->RemovePlayerActor();
                    gEnv->player->setPosition(center);
                }
                else if ((RoR::App::GetInputEngine()->getEventBoolValue(EV_COMMON_REPAIR_TRUCK) || m_advanced_vehicle_repair) && !m_player_actor->ar_replay_mode)
                {
                    this->StopRaceTimer();
                    if (RoR::App::GetInputEngine()->getEventBoolValue(EV_COMMON_REPAIR_TRUCK))
                    {
                        m_advanced_vehicle_repair = m_advanced_vehicle_repair_timer > 1.0f;
                    }
                    else
                    {
                        m_advanced_vehicle_repair_timer = 0.0f;
                    }

                    Vector3 translation = Vector3::ZERO;
                    float rotation = 0.0f;

                    if (RoR::App::GetInputEngine()->getEventBoolValue(EV_TRUCK_ACCELERATE))
                    {
                        translation += 2.0f * Vector3::UNIT_Y * dt;
                    }
                    else if (RoR::App::GetInputEngine()->getEventBoolValue(EV_TRUCK_BRAKE))
                    {
                        translation -= 2.0f * Vector3::UNIT_Y * dt;
                    }
                    if (RoR::App::GetInputEngine()->getEventBoolValue(EV_TRUCK_STEER_LEFT))
                    {
                        rotation += 0.5f * dt;
                    }
                    else if (RoR::App::GetInputEngine()->getEventBoolValue(EV_TRUCK_STEER_RIGHT))
                    {
                        rotation -= 0.5f * dt;
                    }
                    if (RoR::App::GetInputEngine()->getEventBoolValue(EV_CHARACTER_FORWARD))
                    {
                        float curRot = m_player_actor->getRotation();
                        translation.x += 2.0f * cos(curRot - Ogre::Math::HALF_PI) * dt;
                        translation.z += 2.0f * sin(curRot - Ogre::Math::HALF_PI) * dt;
                    }
                    else if (RoR::App::GetInputEngine()->getEventBoolValue(EV_CHARACTER_BACKWARDS))
                    {
                        float curRot = m_player_actor->getRotation();
                        translation.x -= 2.0f * cos(curRot - Ogre::Math::HALF_PI) * dt;
                        translation.z -= 2.0f * sin(curRot - Ogre::Math::HALF_PI) * dt;
                    }
                    if (RoR::App::GetInputEngine()->getEventBoolValue(EV_CHARACTER_SIDESTEP_RIGHT))
                    {
                        float curRot = m_player_actor->getRotation();
                        translation.x += 2.0f * cos(curRot) * dt;
                        translation.z += 2.0f * sin(curRot) * dt;
                    }
                    else if (RoR::App::GetInputEngine()->getEventBoolValue(EV_CHARACTER_SIDESTEP_LEFT))
                    {
                        float curRot = m_player_actor->getRotation();
                        translation.x -= 2.0f * cos(curRot) * dt;
                        translation.z -= 2.0f * sin(curRot) * dt;
                    }

                    if (translation != Vector3::ZERO || rotation != 0.0f)
                    {
                        float scale = RoR::App::GetInputEngine()->isKeyDown(OIS::KC_LMENU) ? 0.1f : 1.0f;
                        scale *= RoR::App::GetInputEngine()->isKeyDown(OIS::KC_LSHIFT) ? 3.0f : 1.0f;
                        scale *= RoR::App::GetInputEngine()->isKeyDown(OIS::KC_LCONTROL) ? 10.0f : 1.0f;

                        m_player_actor->UpdateFlexbodiesFinal();
                        m_player_actor->displace(translation * scale, rotation * scale);

                        m_advanced_vehicle_repair_timer = 0.0f;
                    }
                    else
                    {
                        m_advanced_vehicle_repair_timer += dt;
                    }

                    m_player_actor->RequestActorReset(true);
                }
                else
                {
                    // get commands
                    // -- here we should define a maximum numbers per actor. Some actors does not have that much commands
                    // -- available, so why should we iterate till MAX_COMMANDS?
                    for (int i = 1; i <= MAX_COMMANDS + 1; i++)
                    {
                        int eventID = EV_COMMANDS_01 + (i - 1);

                        m_player_actor->ar_command_key[i].playerInputValue = RoR::App::GetInputEngine()->getEventValue(eventID);
                    }

                    if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_TOGGLE_FORWARDCOMMANDS))
                    {
                        m_player_actor->ar_forward_commands = !m_player_actor->ar_forward_commands;
                        if (m_player_actor->ar_forward_commands)
                        {
                            RoR::App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE, _L("forwardcommands enabled"), "information.png", 3000);
                        }
                        else
                        {
                            RoR::App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE, _L("forwardcommands disabled"), "information.png", 3000);
                        }
                    }
                    if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_TOGGLE_IMPORTCOMMANDS))
                    {
                        m_player_actor->ar_import_commands = !m_player_actor->ar_import_commands;
                        if (m_player_actor->ar_import_commands)
                        {
                            RoR::App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE, _L("importcommands enabled"), "information.png", 3000);
                        }
                        else
                        {
                            RoR::App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE, _L("importcommands disabled"), "information.png", 3000);
                        }
                    }
                    // replay mode
                    if (m_player_actor->ar_replay_mode)
                    {
                        if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_REPLAY_FORWARD, 0.1f) && m_player_actor->ar_replay_pos <= 0)
                        {
                            m_player_actor->ar_replay_pos++;
                        }
                        if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_REPLAY_BACKWARD, 0.1f) && m_player_actor->ar_replay_pos > -m_player_actor->ar_replay_length)
                        {
                            m_player_actor->ar_replay_pos--;
                        }
                        if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_REPLAY_FAST_FORWARD, 0.1f) && m_player_actor->ar_replay_pos + 10 <= 0)
                        {
                            m_player_actor->ar_replay_pos += 10;
                        }
                        if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_REPLAY_FAST_BACKWARD, 0.1f) && m_player_actor->ar_replay_pos - 10 > -m_player_actor->ar_replay_length)
                        {
                            m_player_actor->ar_replay_pos -= 10;
                        }
                        if (RoR::App::GetInputEngine()->isKeyDown(OIS::KC_LMENU) && RoR::App::GetInputEngine()->isKeyDown(OIS::KC_V))
                        {
                        }

                        if (RoR::App::GetInputEngine()->isKeyDown(OIS::KC_LMENU))
                        {
                            if (m_player_actor->ar_replay_pos <= 0 && m_player_actor->ar_replay_pos >= -m_player_actor->ar_replay_length)
                            {
                                if (RoR::App::GetInputEngine()->isKeyDown(OIS::KC_LSHIFT) || RoR::App::GetInputEngine()->isKeyDown(OIS::KC_RSHIFT))
                                {
                                    m_player_actor->ar_replay_pos += RoR::App::GetInputEngine()->getMouseState().X.rel * 1.5f;
                                }
                                else
                                {
                                    m_player_actor->ar_replay_pos += RoR::App::GetInputEngine()->getMouseState().X.rel * 0.05f;
                                }
                                if (m_player_actor->ar_replay_pos > 0)
                                {
                                    m_player_actor->ar_replay_pos = 0;
                                }
                                if (m_player_actor->ar_replay_pos < -m_player_actor->ar_replay_length)
                                {
                                    m_player_actor->ar_replay_pos = -m_player_actor->ar_replay_length;
                                }
                            }
                        }
                    }

                    if (m_player_actor->ar_driveable == TRUCK)
                    {
                        LandVehicleSimulation::UpdateVehicle(m_player_actor, dt);
                    }
                    if (m_player_actor->ar_driveable == AIRPLANE)
                    {
                        AircraftSimulation::UpdateVehicle(m_player_actor, dt);
                    }
                    if (m_player_actor->ar_driveable == BOAT)
                    {
                        //BOAT SPECIFICS

                        //throttle
                        if (RoR::App::GetInputEngine()->isEventDefined(EV_BOAT_THROTTLE_AXIS))
                        {
                            float f = RoR::App::GetInputEngine()->getEventValue(EV_BOAT_THROTTLE_AXIS);
                            // use negative values also!
                            f = f * 2 - 1;
                            for (int i = 0; i < m_player_actor->ar_num_screwprops; i++)
                                m_player_actor->ar_screwprops[i]->setThrottle(-f);
                        }
                        if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_BOAT_THROTTLE_DOWN, 0.1f))
                        {
                            //throttle down
                            for (int i = 0; i < m_player_actor->ar_num_screwprops; i++)
                                m_player_actor->ar_screwprops[i]->setThrottle(m_player_actor->ar_screwprops[i]->getThrottle() - 0.05);
                        }
                        if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_BOAT_THROTTLE_UP, 0.1f))
                        {
                            //throttle up
                            for (int i = 0; i < m_player_actor->ar_num_screwprops; i++)
                                m_player_actor->ar_screwprops[i]->setThrottle(m_player_actor->ar_screwprops[i]->getThrottle() + 0.05);
                        }

                        // steer
                        float tmp_steer_left = RoR::App::GetInputEngine()->getEventValue(EV_BOAT_STEER_LEFT);
                        float tmp_steer_right = RoR::App::GetInputEngine()->getEventValue(EV_BOAT_STEER_RIGHT);
                        float stime = RoR::App::GetInputEngine()->getEventBounceTime(EV_BOAT_STEER_LEFT) + RoR::App::GetInputEngine()->getEventBounceTime(EV_BOAT_STEER_RIGHT);
                        float sum_steer = (tmp_steer_left - tmp_steer_right) * dt;
                        // do not center the rudder!
                        if (fabs(sum_steer) > 0 && stime <= 0)
                        {
                            for (int i = 0; i < m_player_actor->ar_num_screwprops; i++)
                                m_player_actor->ar_screwprops[i]->setRudder(m_player_actor->ar_screwprops[i]->getRudder() + sum_steer);
                        }
                        if (RoR::App::GetInputEngine()->isEventDefined(EV_BOAT_STEER_LEFT_AXIS) && RoR::App::GetInputEngine()->isEventDefined(EV_BOAT_STEER_RIGHT_AXIS))
                        {
                            tmp_steer_left = RoR::App::GetInputEngine()->getEventValue(EV_BOAT_STEER_LEFT_AXIS);
                            tmp_steer_right = RoR::App::GetInputEngine()->getEventValue(EV_BOAT_STEER_RIGHT_AXIS);
                            sum_steer = (tmp_steer_left - tmp_steer_right);
                            for (int i = 0; i < m_player_actor->ar_num_screwprops; i++)
                                m_player_actor->ar_screwprops[i]->setRudder(sum_steer);
                        }
                        if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_BOAT_CENTER_RUDDER, 0.1f))
                        {
                            for (int i = 0; i < m_player_actor->ar_num_screwprops; i++)
                                m_player_actor->ar_screwprops[i]->setRudder(0);
                        }

                        if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_BOAT_REVERSE))
                        {
                            for (int i = 0; i < m_player_actor->ar_num_screwprops; i++)
                                m_player_actor->ar_screwprops[i]->toggleReverse();
                        }
                    }
                    //COMMON KEYS

                    if (RoR::App::GetInputEngine()->getEventBoolValue(EV_COMMON_ACCELERATE_SIMULATION))
                    {
                        float simulation_speed = m_actor_manager.GetSimulationSpeed() * pow(2.0f, dt / 2.0f);
                        m_actor_manager.SetSimulationSpeed(simulation_speed);
                        String ssmsg = _L("New simulation speed: ") + TOSTRING(Round(simulation_speed * 100.0f, 1)) + "%";
                        RoR::App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE, ssmsg, "infromation.png", 2000, false);
                        RoR::App::GetGuiManager()->PushNotification("Notice:", ssmsg);
                    }
                    if (RoR::App::GetInputEngine()->getEventBoolValue(EV_COMMON_DECELERATE_SIMULATION))
                    {
                        float simulation_speed = m_actor_manager.GetSimulationSpeed() * pow(0.5f, dt / 2.0f);
                        m_actor_manager.SetSimulationSpeed(simulation_speed);
                        String ssmsg = _L("New simulation speed: ") + TOSTRING(Round(simulation_speed * 100.0f, 1)) + "%";
                        RoR::App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE, ssmsg, "infromation.png", 2000, false);
                        RoR::App::GetGuiManager()->PushNotification("Notice:", ssmsg);
                    }
                    if (RoR::App::GetInputEngine()->getEventBoolValue(EV_COMMON_RESET_SIMULATION_PACE))
                    {
                        if (!m_is_pace_reset_pressed)
                        {
                            float simulation_speed = m_actor_manager.GetSimulationSpeed();
                            if (simulation_speed != 1.0f)
                            {
                                m_last_simulation_speed = simulation_speed;
                                m_actor_manager.SetSimulationSpeed(1.0f);
                                UTFString ssmsg = _L("Simulation speed reset.");
                                RoR::App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE, ssmsg, "infromation.png", 2000, false);
                                RoR::App::GetGuiManager()->PushNotification("Notice:", ssmsg);
                            }
                            else if (m_last_simulation_speed != 1.0f)
                            {
                                m_actor_manager.SetSimulationSpeed(m_last_simulation_speed);
                                String ssmsg = _L("New simulation speed: ") + TOSTRING(Round(m_last_simulation_speed * 100.0f, 1)) + "%";
                                RoR::App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE, ssmsg, "infromation.png", 2000, false);
                                RoR::App::GetGuiManager()->PushNotification("Notice:", ssmsg);
                            }
                        }
                        m_is_pace_reset_pressed = true;
                    }
                    else
                    {
                        m_is_pace_reset_pressed = false;
                    }
                    if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_TRUCK_REMOVE))
                    {
                        this->RemovePlayerActor();
                    }
                    if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_ROPELOCK))
                    {
                        m_player_actor->ToggleRopes(-1);
                    }
                    if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_LOCK))
                    {
                        m_player_actor->ToggleHooks(-1, HOOK_TOGGLE, -1);
                        //SlideNodeLock
                        m_player_actor->ToggleSlideNodeLock();
                    }
                    if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_AUTOLOCK))
                    {
                        //unlock all autolocks
                        m_player_actor->ToggleHooks(-2, HOOK_UNLOCK, -1);
                    }
                    //strap
                    if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_SECURE_LOAD))
                    {
                        m_player_actor->ToggleTies(-1);
                    }

                    //replay mode
                    if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_TOGGLE_REPLAY_MODE))
                    {
                        this->StopRaceTimer();
                        m_player_actor->setReplayMode(!m_player_actor->ar_replay_mode);
                    }

                    if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_TOGGLE_CUSTOM_PARTICLES))
                    {
                        m_player_actor->ToggleCustomParticles();
                    }

                    if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_SHOW_SKELETON))
                    {
                        if (m_player_actor->ar_skeletonview_is_active)
                            m_player_actor->HideSkeleton();
                        else
                            m_player_actor->ShowSkeleton(true);
                    }

                    if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_TOGGLE_TRUCK_LIGHTS))
                    {
                        m_player_actor->ToggleLights();
                    }

                    if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_TOGGLE_TRUCK_BEACONS))
                    {
                        m_player_actor->ToggleBeacons();
                    }

                    //camera mode
                    if (RoR::App::GetInputEngine()->getEventBoolValue(EV_COMMON_PRESSURE_LESS))
                    {
                        if (m_pressure_pressed = m_player_actor->AddTyrePressure(dt * -10.0))
                        {
                            if (RoR::App::GetOverlayWrapper())
                                RoR::App::GetOverlayWrapper()->showPressureOverlay(true);

                            SOUND_START(m_player_actor, SS_TRIG_AIR);
                        }
                    }
                    else if (RoR::App::GetInputEngine()->getEventBoolValue(EV_COMMON_PRESSURE_MORE))
                    {
                        if (m_pressure_pressed = m_player_actor->AddTyrePressure(dt * 10.0))
                        {
                            if (RoR::App::GetOverlayWrapper())
                                RoR::App::GetOverlayWrapper()->showPressureOverlay(true);

                            SOUND_START(m_player_actor, SS_TRIG_AIR);
                        }
                    }
                    else if (m_pressure_pressed)
                    {
                        SOUND_STOP(m_player_actor, SS_TRIG_AIR);
                        m_pressure_pressed = false;
                        if (RoR::App::GetOverlayWrapper())
                            RoR::App::GetOverlayWrapper()->showPressureOverlay(false);
                    }

                    if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_RESCUE_TRUCK, 0.5f) && !mp_connected && m_player_actor->ar_driveable != AIRPLANE)
                    {
                        Actor* rescuer = m_actor_manager.FetchRescueVehicle();
                        if (rescuer == nullptr)
                        {
                            RoR::App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE, _L("No rescue truck found!"), "warning.png");
                            RoR::App::GetGuiManager()->PushNotification("Notice:", _L("No rescue truck found!"));
                        }
                        else
                        {
                            this->SetPlayerActor(rescuer);
                        }
                    }

                    if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_TOGGLE_VIDEOCAMERA, 0.5f))
                    {
                        if (m_player_actor->GetGfxActor()->GetVideoCamState() == GfxActor::VideoCamState::VCSTATE_DISABLED)
                        {
                            m_player_actor->GetGfxActor()->SetVideoCamState(GfxActor::VideoCamState::VCSTATE_ENABLED_ONLINE);
                        }
                        else
                        {
                            m_player_actor->GetGfxActor()->SetVideoCamState(GfxActor::VideoCamState::VCSTATE_DISABLED);
                        }
                    }

                    if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_BLINK_LEFT))
                    {
                        if (m_player_actor->getBlinkType() == BLINK_LEFT)
                            m_player_actor->setBlinkType(BLINK_NONE);
                        else
                            m_player_actor->setBlinkType(BLINK_LEFT);
                    }

                    if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_BLINK_RIGHT))
                    {
                        if (m_player_actor->getBlinkType() == BLINK_RIGHT)
                            m_player_actor->setBlinkType(BLINK_NONE);
                        else
                            m_player_actor->setBlinkType(BLINK_RIGHT);
                    }

                    if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_BLINK_WARN))
                    {
                        if (m_player_actor->getBlinkType() == BLINK_WARN)
                            m_player_actor->setBlinkType(BLINK_NONE);
                        else
                            m_player_actor->setBlinkType(BLINK_WARN);
                    }
                }
            }
        }

#ifdef USE_CAELUM

        const bool caelum_enabled = App::gfx_sky_mode.GetActive() == GfxSkyMode::CAELUM;
        if (caelum_enabled && (simRUNNING(s) || simPAUSED(s) || simEDITOR(s)))
        {
            Real time_factor = 1000.0f;
            Real multiplier = 10;
            bool update_time = false;

            if (RoR::App::GetInputEngine()->getEventBoolValue(EV_SKY_INCREASE_TIME) && App::GetSimTerrain()->getSkyManager())
            {
                update_time = true;
            }
            else if (RoR::App::GetInputEngine()->getEventBoolValue(EV_SKY_INCREASE_TIME_FAST) && App::GetSimTerrain()->getSkyManager())
            {
                time_factor *= multiplier;
                update_time = true;
            }
            else if (RoR::App::GetInputEngine()->getEventBoolValue(EV_SKY_DECREASE_TIME) && App::GetSimTerrain()->getSkyManager())
            {
                time_factor = -time_factor;
                update_time = true;
            }
            else if (RoR::App::GetInputEngine()->getEventBoolValue(EV_SKY_DECREASE_TIME_FAST) && App::GetSimTerrain()->getSkyManager())
            {
                time_factor *= -multiplier;
                update_time = true;
            }
            else
            {
                time_factor = 1.0f;
                update_time = App::GetSimTerrain()->getSkyManager()->GetSkyTimeFactor() != 1.0f;
            }

            if (update_time)
            {
                App::GetSimTerrain()->getSkyManager()->SetSkyTimeFactor(time_factor);
                RoR::App::GetGuiManager()->PushNotification("Notice:", _L("Time set to ") + App::GetSimTerrain()->getSkyManager()->GetPrettyTime());
                RoR::App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE, _L("Time set to ") + App::GetSimTerrain()->getSkyManager()->GetPrettyTime(), "weather_sun.png", 1000);
            }
        }

#endif // USE_CAELUM


        const bool skyx_enabled = App::gfx_sky_mode.GetActive() == GfxSkyMode::SKYX;
        SkyXManager* skyx_mgr = App::GetSimTerrain()->getSkyXManager();
        if (skyx_enabled && (skyx_mgr != nullptr) && (simRUNNING(s) || simPAUSED(s) || simEDITOR(s)))
        {

            if (RoR::App::GetInputEngine()->getEventBoolValue(EV_SKY_INCREASE_TIME))
            {
                skyx_mgr->GetSkyX()->setTimeMultiplier(1.0f);
            }
            else if (RoR::App::GetInputEngine()->getEventBoolValue(EV_SKY_INCREASE_TIME_FAST))
            {
                skyx_mgr->GetSkyX()->setTimeMultiplier(2.0f);
            }
            else if (RoR::App::GetInputEngine()->getEventBoolValue(EV_SKY_DECREASE_TIME))
            {
                skyx_mgr->GetSkyX()->setTimeMultiplier(-1.0f);
            }
            else if (RoR::App::GetInputEngine()->getEventBoolValue(EV_SKY_DECREASE_TIME_FAST))
            {
                skyx_mgr->GetSkyX()->setTimeMultiplier(-2.0f);
            }
            else
            {
                skyx_mgr->GetSkyX()->setTimeMultiplier(0.01f);
            }
        }

        if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_TOGGLE_RENDER_MODE, 0.5f))
        {
            static int mSceneDetailIndex;
            mSceneDetailIndex = (mSceneDetailIndex + 1) % 3;
            switch (mSceneDetailIndex)
            {
            case 0:
                gEnv->mainCamera->setPolygonMode(PM_SOLID);
                break;
            case 1:
                gEnv->mainCamera->setPolygonMode(PM_WIREFRAME);
                break;
            case 2:
                gEnv->mainCamera->setPolygonMode(PM_POINTS);
                break;
            }
        }

        if (RoR::App::GetInputEngine()->getEventBoolValue(EV_COMMON_ENTER_OR_EXIT_TRUCK) && m_time_until_next_toggle <= 0)
        {
            m_time_until_next_toggle = 0.5; // Some delay before trying to re-enter(exit) actor
            // perso in/out
            int num_actor_slots = m_actor_manager.GetNumUsedActorSlots();
            Actor** actor_slots = m_actor_manager.GetInternalActorSlots();

            if (this->GetPlayerActor() == nullptr)
            {
                // find the nearest vehicle
                float mindist = 1000.0;
                Actor* nearest_actor = nullptr;
                for (int i = 0; i < num_actor_slots; i++)
                {
                    if (!actor_slots[i])
                        continue;
                    if (!actor_slots[i]->ar_driveable)
                        continue;
                    if (actor_slots[i]->ar_cinecam_node[0] == -1)
                    {
                        LOG("cinecam missing, cannot enter the actor!");
                        continue;
                    }
                    float len = 0.0f;
                    if (gEnv->player)
                    {
                        len = actor_slots[i]->ar_nodes[actor_slots[i]->ar_cinecam_node[0]].AbsPosition.distance(gEnv->player->getPosition() + Vector3(0.0, 2.0, 0.0));
                    }
                    if (len < mindist)
                    {
                        mindist = len;
                        nearest_actor = actor_slots[i];
                    }
                }
                if (mindist < 20.0)
                {
                    this->SetPlayerActor(nearest_actor);
                }
            }
            else if (m_player_actor->ar_nodes[0].Velocity.length() < 1.0f)
            {
                this->SetPlayerActor(nullptr);
            }
            else
            {
                m_player_actor->ar_brake = m_player_actor->ar_brake_force * 0.66;
                m_time_until_next_toggle = 0.0; // No delay in this case: the vehicle must brake like braking normally
            }
        }
        else if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_ENTER_NEXT_TRUCK, 0.25f))
        {
            this->SetPlayerActor(m_actor_manager.FetchNextVehicleOnList(m_player_actor, m_prev_player_actor));
        }
        else if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_ENTER_PREVIOUS_TRUCK, 0.25f))
        {
            this->SetPlayerActor(m_actor_manager.FetchPreviousVehicleOnList(m_player_actor, m_prev_player_actor));
        }
        else if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_RESPAWN_LAST_TRUCK, 0.25f))
        {
            if (m_last_cache_selection != nullptr)
            {
                /* We load an extra actor */
                std::vector<String>* config_ptr = nullptr;
                if (m_last_vehicle_configs.size() > 0)
                {
                    config_ptr = & m_last_vehicle_configs;
                }

                if (m_player_actor != nullptr)
                {
                    m_reload_dir = Quaternion(Degree(270) - Radian(m_player_actor->getRotation()), Vector3::UNIT_Y);
                    m_reload_pos = m_player_actor->GetRotationCenter();

                    // TODO: Fix this by projecting m_reload_pos onto the terrain / mesh
                    m_reload_pos.y = m_player_actor->ar_nodes[m_player_actor->ar_lowest_contacting_node].AbsPosition.y;
                }
                else
                {
                    m_reload_dir = Quaternion(Degree(180) - gEnv->player->getRotation(), Vector3::UNIT_Y);
                    m_reload_pos = gEnv->player->getPosition();
                }

                Actor* local_actor = m_actor_manager.CreateLocalActor(m_reload_pos, m_reload_dir, m_last_cache_selection->fname, m_last_cache_selection->number, 0, config_ptr, m_last_skin_selection);

                this->FinalizeActorSpawning(local_actor, m_player_actor);
            }
        }
    }
    else
    {
        //no terrain or actor loaded
        terrain_editing_mode = false;

        if (App::GetGuiManager()->GetMainSelector()->IsFinishedSelecting())
        {
            if (simSELECT(s))
            {
                CacheEntry* selection = App::GetGuiManager()->GetMainSelector()->GetSelectedEntry();
                RoR::SkinDef* skin = App::GetGuiManager()->GetMainSelector()->GetSelectedSkin();
                if (selection != nullptr)
                {
                    // We load an extra actor
                    std::vector<String>* config_ptr = nullptr;
                    std::vector<String> config = App::GetGuiManager()->GetMainSelector()->GetVehicleConfigs();
                    if (config.size() > 0)
                    {
                        config_ptr = & config;
                    }

                    m_last_cache_selection = selection;
                    m_last_skin_selection = skin;
                    m_last_vehicle_configs = config;

                    if (m_reload_box == nullptr)
                    {
                        if (m_player_actor != nullptr)
                        {
                            float rotation = m_player_actor->getRotation() - Math::HALF_PI;
                            m_reload_dir = Quaternion(Degree(180) - Radian(rotation), Vector3::UNIT_Y);
                            m_reload_pos = m_player_actor->GetRotationCenter();
                            // TODO: Fix this by projecting m_reload_pos onto the terrain / mesh
                            m_reload_pos.y = m_player_actor->ar_nodes[m_player_actor->ar_lowest_contacting_node].AbsPosition.y;
                        }
                        else
                        {
                            m_reload_dir = Quaternion(Degree(180) - gEnv->player->getRotation(), Vector3::UNIT_Y);
                            m_reload_pos = gEnv->player->getPosition();
                        }
                    }

                    Actor* local_actor = m_actor_manager.CreateLocalActor(m_reload_pos, m_reload_dir, selection->fname, selection->number, m_reload_box, config_ptr, skin);

                    this->FinalizeActorSpawning(local_actor, m_player_actor);
                }
                else if (gEnv->player)
                {
                    gEnv->player->unwindMovement(0.1f);
                }

                m_reload_box = 0;
            }
            App::GetGuiManager()->GetMainSelector()->Hide();
            RoR::App::GetGuiManager()->UnfocusGui();
            App::sim_state.SetActive(SimState::RUNNING); // TODO: use pending mechanism
        }
    }

    if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_GET_NEW_VEHICLE))
    {
        if ((simRUNNING(s) || simPAUSED(s) || simEDITOR(s)) && gEnv->player != nullptr)
        {
            App::sim_state.SetActive(SimState::SELECTING); // TODO: use pending mechanism

            App::GetGuiManager()->GetMainSelector()->Show(LT_AllBeam);
        }
    }

    if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_TRUCK_INFO) && m_player_actor)
    {
        m_actor_info_gui_visible = ! m_actor_info_gui_visible;
        gui_man->GetSimUtils()->SetActorInfoBoxVisible(m_actor_info_gui_visible);
    }

    if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_TRUCK_DESCRIPTION) && m_player_actor)
    {
        gui_man->SetVisible_VehicleDescription(! gui_man->IsVisible_VehicleDescription());
    }

    if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_HIDE_GUI))
    {
        m_hide_gui = !m_hide_gui;
        this->HideGUI(m_hide_gui);
    }

    if ((simRUNNING(s) || simPAUSED(s) || simEDITOR(s)) && RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_TOGGLE_STATS))
    {
        gui_man->GetSimUtils()->SetFPSBoxVisible(! gui_man->GetSimUtils()->IsFPSBoxVisible());
    }

    if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_TOGGLE_MAT_DEBUG))
    {
        if (m_stats_on == 0)
            m_stats_on = 2;
        else if (m_stats_on == 1)
            m_stats_on = 2;
        else if (m_stats_on == 2)
            m_stats_on = 0;
        if (RoR::App::GetOverlayWrapper())
            RoR::App::GetOverlayWrapper()->showDebugOverlay(m_stats_on);
    }

    if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_OUTPUT_POSITION) && (simRUNNING(s) || simPAUSED(s) || simEDITOR(s)))
    {
        Vector3 position(Vector3::ZERO);
        Radian rotation(0);
        if (m_player_actor == nullptr)
        {
            if (gEnv->player)
            {
                position = gEnv->player->getPosition();
                rotation = gEnv->player->getRotation() + Radian(Math::PI);
            }
        }
        else
        {
            position = m_player_actor->getPosition();
            Vector3 idir = m_player_actor->getDirection();
            rotation = atan2(idir.dotProduct(Vector3::UNIT_X), idir.dotProduct(-Vector3::UNIT_Z));
        }
        LOG("Position: " + TOSTRING(position.x) + ", "+ TOSTRING(position.y) + ", " + TOSTRING(position.z) + ", 0, " + TOSTRING(rotation.valueDegrees()) + ", 0");
    }

    // Return true to continue rendering
    return true;
}

void RoRFrameListener::TeleportPlayer(RoR::Terrn2Telepoint* telepoint)
{
    if (m_player_actor != nullptr)
        return; // Player could enter an actor while Teleport-GUI is visible

    gEnv->player->setPosition(telepoint->position);
}

void RoRFrameListener::TeleportPlayerXZ(float x, float z)
{
    if (m_player_actor)
        return; // Player could enter an actor while Teleport-GUI is visible

    float y = App::GetSimTerrain()->GetHeightAt(x, z);
    gEnv->player->setPosition(Ogre::Vector3(x, y, z));
}

void RoRFrameListener::FinalizeActorSpawning(Actor* local_actor, Actor* prev_actor)
{
    if (local_actor != nullptr)
    {
        if (m_reload_box == nullptr)
        {
            // Calculate translational offset for node[0] to align the actor's rotation center with m_reload_pos
            Vector3 translation = m_reload_pos - local_actor->GetRotationCenter();
            local_actor->ResetPosition(local_actor->ar_nodes[0].AbsPosition + Vector3(translation.x, 0.0f, translation.z), true);

            if (local_actor->ar_driveable != NOT_DRIVEABLE || (prev_actor && prev_actor->ar_driveable != NOT_DRIVEABLE))
            {
                // Try to resolve collisions with other actors
                local_actor->resolveCollisions(50.0f, prev_actor == nullptr);
            }
        }

        if (gEnv->surveyMap)
        {
            gEnv->surveyMap->createNamedMapEntity("Truck" + TOSTRING(local_actor->ar_instance_id), SurveyMapManager::getTypeByDriveable(local_actor->ar_driveable));
        }

        if (local_actor->ar_driveable != NOT_DRIVEABLE)
        {
            // We are supposed to be in this vehicle, if it is a vehicle
            if (local_actor->ar_engine != nullptr)
            {
                local_actor->ar_engine->StartEngine();
            }
            this->SetPlayerActor(local_actor);
        }

        local_actor->UpdateFlexbodiesPrepare();
        local_actor->UpdateFlexbodiesFinal();
        local_actor->updateVisual();
    }
}

// Override frameStarted event to process that (don't care about frameEnded)
bool RoRFrameListener::frameStarted(const FrameEvent& evt)
{
    float dt = evt.timeSinceLastFrame;
    if (dt == 0.0f)
        return true;
    dt = std::min(dt, 0.05f);
    m_time += dt;

    m_actor_manager.SyncWithSimThread();

    const bool mp_connected = (App::mp_state.GetActive() == MpState::CONNECTED);
#ifdef USE_SOCKETW
    if (mp_connected)
    {
        std::vector<Networking::recv_packet_t> packets = RoR::Networking::GetIncomingStreamData();

        RoR::ChatSystem::HandleStreamData(packets);
        m_actor_manager.HandleActorStreamData(packets);
        m_character_factory.handleStreamData(packets); // Update characters last (or else beam coupling might fail)

        m_netcheck_gui_timer += dt;
        if (m_netcheck_gui_timer > 2.0f)
        {
            App::GetGuiManager()->GetMpClientList()->update();
            m_netcheck_gui_timer = 0.0f;
        }
    }
#endif //SOCKETW

    RoR::App::GetInputEngine()->Capture();
    App::GetGuiManager()->NewImGuiFrame(dt);
    const bool is_altkey_pressed =  App::GetInputEngine()->isKeyDown(OIS::KeyCode::KC_LMENU) || App::GetInputEngine()->isKeyDown(OIS::KeyCode::KC_RMENU);
    auto s = App::sim_state.GetActive();

    //if (gEnv->collisions) 	printf("> ground model used: %s\n", gEnv->collisions->last_used_ground_model->name);
    //
    if ((simRUNNING(s) || simEDITOR(s)) && !simPAUSED(s))
    {
        m_actor_manager.UpdateFlexbodiesPrepare(); // Pushes all flexbody tasks into the thread pool 
    }

    if (OutProtocol::getSingletonPtr())
    {
        OutProtocol::getSingleton().Update(dt, m_player_actor);
    }

    // update network gui if required, at most every 2 seconds
    if (mp_connected)
    {
        // now update mumble 3d audio things
#ifdef USE_MUMBLE
        if (gEnv->player)
        {
            // calculate orientation of avatar first
            Ogre::Vector3 avatarDir = Ogre::Vector3(Math::Cos(gEnv->player->getRotation()), 0.0f, Math::Sin(gEnv->player->getRotation()));

            App::GetMumble()->update(gEnv->mainCamera->getPosition(), gEnv->mainCamera->getDirection(), gEnv->mainCamera->getUp(),
                gEnv->player->getPosition() + Vector3(0, 1.8f, 0), avatarDir, Ogre::Vector3(0.0f, 1.0f, 0.0f));
        }
#endif // USE_MUMBLE
    }

    if (simRUNNING(s) || simPAUSED(s) || simEDITOR(s))
    {
        if (gEnv->cameraManager != nullptr)
        {
            gEnv->cameraManager->Update(dt, m_player_actor, m_actor_manager.GetSimulationSpeed());
        }
        if (gEnv->surveyMap != nullptr)
        {
            gEnv->surveyMap->Update(dt, m_player_actor);
        }
    }

#ifdef USE_OPENAL
    // update audio listener position
    static Vector3 lastCameraPosition;
    Vector3 cameraSpeed = (gEnv->mainCamera->getPosition() - lastCameraPosition) / dt;
    lastCameraPosition = gEnv->mainCamera->getPosition();

    if (simRUNNING(s) || simPAUSED(s) || simEDITOR(s))
    {
        SoundScriptManager::getSingleton().setCamera(gEnv->mainCamera->getPosition(), gEnv->mainCamera->getDirection(), gEnv->mainCamera->getUp(), cameraSpeed);
    }
#endif // USE_OPENAL

    if ((m_player_actor != nullptr) && (!simPAUSED(s)))
    {
        m_player_actor->GetGfxActor()->UpdateVideoCameras(dt);
    }

    // --- terrain updates ---

    // update animated objects
    App::GetSimTerrain()->update(dt);

    // env map update
    if (m_player_actor)
    {
        m_gfx_envmap.UpdateEnvMap(m_player_actor->getPosition(), m_player_actor);
    }
    // NOTE: Removed `else` branch which captured the middle of the map (height: ground+50m) - what was that for?? ~ only_a_ptr, 08/2017

    // water
    if (simRUNNING(s) || simPAUSED(s) || simEDITOR(s))
    {
        IWater* water = App::GetSimTerrain()->getWater();
        if (water)
        {
            water->WaterSetCamera(gEnv->mainCamera);
            if (m_player_actor)
            {
                water->SetReflectionPlaneHeight(water->CalcWavesHeight(m_player_actor->getPosition()));
            }
            else
            {
                water->SetReflectionPlaneHeight(water->GetStaticWaterHeight());
            }
            water->FrameStepWater(dt);
        }
    }

    // trigger updating of shadows etc
#ifdef USE_CAELUM
    SkyManager* sky = App::GetSimTerrain()->getSkyManager();
    if ((sky != nullptr) && (simRUNNING(s) || simPAUSED(s) || simEDITOR(s)))
    {
        sky->DetectSkyUpdate();
    }
#endif

    SkyXManager* skyx_man = App::GetSimTerrain()->getSkyXManager();
    if ((skyx_man != nullptr) && (simRUNNING(s) || simPAUSED(s) || simEDITOR(s)))
    {
       skyx_man->update(dt); // Light update
    }

    if (simRUNNING(s) || simPAUSED(s) || simEDITOR(s))
    {
        m_actor_manager.GetParticleManager().update();

        if (m_heathaze)
            m_heathaze->update();
    }

    if ((simRUNNING(s) || simEDITOR(s)) && !simPAUSED(s))
    {
        m_actor_manager.UpdateActorVisuals(dt, m_player_actor); // update visual - antishaking
    }

    if (! this->UpdateInputEvents(dt))
    {
        LOG("exiting...");
        return false;
    }
    // CAUTION: 'updateEvents' might have changed 'm_player_actor'
    //          TODO: This is a mess - actor updates from misc. inputs should be buffered, evaluated and executed at once, not ad-hoc ~ only_a_ptr, 07/2017

    // update gui 3d arrow
    // TODO: This is most definitely NOT necessary to do right here ~ only_a_ptr, 07/2017
    if (RoR::App::GetOverlayWrapper() && m_is_dir_arrow_visible && (simRUNNING(s) || simPAUSED(s) || simEDITOR(s)))
    {
        RoR::App::GetOverlayWrapper()->UpdateDirectionArrow(m_player_actor, m_dir_arrow_pointed);
    }

    // one of the input modes is immediate, so setup what is needed for immediate mouse/key movement
    if (m_time_until_next_toggle >= 0)
    {
        m_time_until_next_toggle -= dt;
    }

    RoR::App::GetGuiManager()->FrameStepGui(dt);

    // CAUTION: 'FrameStepGui()' might have changed 'm_player_actor'
    //           TODO: This is a mess - actor updates from misc. inputs should be buffered, evaluated and executed at once, not ad-hoc ~ only_a_ptr, 07/2017

    App::GetGuiManager()->GetTeleport()->TeleportWindowFrameStep(
        gEnv->player->getPosition().x, gEnv->player->getPosition().z, is_altkey_pressed);

#ifdef USE_ANGELSCRIPT
    ScriptEngine::getSingleton().framestep(dt);
#endif

    // one of the input modes is immediate, so update the movement vector
    if (simRUNNING(s) || simPAUSED(s) || simEDITOR(s))
    {
        this->UpdateForceFeedback(dt);

        if (RoR::App::GetOverlayWrapper() != nullptr)
        {
            // update survey map
            if (gEnv->surveyMap != nullptr && gEnv->surveyMap->getVisibility())
            {
                Actor** vehicles = m_actor_manager.GetInternalActorSlots();
                int num_vehicles = m_actor_manager.GetNumUsedActorSlots();

                gEnv->surveyMap->UpdateVehicles(vehicles, num_vehicles);
            }


            if (m_player_actor != nullptr)
            {
#ifdef FEAT_TIMING
				BES.updateGUI(dt);
#endif // FEAT_TIMING

                // update mouse picking lines, etc
                RoR::App::GetSceneMouse()->update(dt);

                if (m_pressure_pressed)
                {
                    RoR::App::GetOverlayWrapper()->UpdatePressureTexture(m_player_actor->GetTyrePressure());
                }

                if (m_race_in_progress && (App::sim_state.GetActive() != SimState::PAUSED))
                {
                    UpdateRacingGui(); //I really think that this should stay here.
                }

                if (m_player_actor->ar_driveable == TRUCK && m_player_actor->ar_engine != nullptr)
                {
                    RoR::App::GetOverlayWrapper()->UpdateLandVehicleHUD(m_player_actor);
                }
                else if (m_player_actor->ar_driveable == AIRPLANE)
                {
                    RoR::App::GetOverlayWrapper()->UpdateAerialHUD(m_player_actor);
                }
            }
            RoR::App::GetGuiManager()->UpdateSimUtils(dt, m_player_actor);
        }

        if (!simPAUSED(s))
        {
            m_actor_manager.JoinFlexbodyTasks(); // Waits until all flexbody tasks are finished
            m_actor_manager.UpdateActors(m_player_actor, dt);
            m_actor_manager.UpdateFlexbodiesFinal(); // Updates the harware buffers
        }

        if (simRUNNING(s) && (App::sim_state.GetPending() == SimState::PAUSED))
        {
            App::GetGuiManager()->SetVisible_GamePauseMenu(true);
            m_actor_manager.MuteAllActors();

            App::sim_state.ApplyPending();
        }
        else if (simPAUSED(s) && (App::sim_state.GetPending() == SimState::RUNNING))
        {
            App::GetGuiManager()->SetVisible_GamePauseMenu(false);
            m_actor_manager.UnmuteAllActors();

            App::sim_state.ApplyPending();
        }
    }

    App::GetGuiManager()->GetImGui().Render();

    return true;
}

bool RoRFrameListener::frameRenderingQueued(const FrameEvent& evt)
{
    App::GetGuiManager()->GetImGui().Render();
    return true;
}

bool RoRFrameListener::frameEnded(const FrameEvent& evt)
{
    // TODO: IMPROVE STATS
    if (m_stats_on && RoR::App::GetOverlayWrapper())
    {
        RoR::App::GetOverlayWrapper()->updateStats();
    }

    return true;
}

void RoRFrameListener::ShowLoaderGUI(int type, const Ogre::String& instance, const Ogre::String& box)
{
    int num_actor_slots = m_actor_manager.GetNumUsedActorSlots();
    Actor** actor_slots = m_actor_manager.GetInternalActorSlots();

    // first, test if the place if clear, BUT NOT IN MULTIPLAYER
    if (!(App::mp_state.GetActive() == MpState::CONNECTED))
    {
        collision_box_t* spawnbox = gEnv->collisions->getBox(instance, box);
        for (int t = 0; t < num_actor_slots; t++)
        {
            if (!actor_slots[t])
                continue;
            for (int i = 0; i < actor_slots[t]->ar_num_nodes; i++)
            {
                if (gEnv->collisions->isInside(actor_slots[t]->ar_nodes[i].AbsPosition, spawnbox))
                {
                    RoR::App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE, _L("Please clear the place first"), "error.png");
                    RoR::App::GetGuiManager()->PushNotification("Notice:", _L("Please clear the place first"));
                    gEnv->collisions->clearEventCache();
                    return;
                }
            }
        }
    }
    m_reload_pos = gEnv->collisions->getPosition(instance, box);
    m_reload_dir = gEnv->collisions->getDirection(instance, box);
    m_reload_box = gEnv->collisions->getBox(instance, box);
    App::sim_state.SetActive(SimState::SELECTING); // TODO: use 'pending' mechanism
    if (gEnv->surveyMap)
        gEnv->surveyMap->setVisibility(false);

    App::GetGuiManager()->GetMainSelector()->Show(LoaderType(type));
}

void RoRFrameListener::UpdateDirectionArrow(char* text, Vector3 position)
{
    if (RoR::App::GetOverlayWrapper() == nullptr)
        return;

    if (text == nullptr)
    {
        RoR::App::GetOverlayWrapper()->HideDirectionOverlay();
        m_is_dir_arrow_visible = false;
        m_dir_arrow_pointed = Vector3::ZERO;
    }
    else
    {
        RoR::App::GetOverlayWrapper()->ShowDirectionOverlay(text);
        m_is_dir_arrow_visible = true;
        m_dir_arrow_pointed = position;
    }
}

/* --- Window Events ------------------------------------------ */
void RoRFrameListener::windowResized(Ogre::RenderWindow* rw)
{
    if (!rw)
        return;
    LOG("[RoR] Received 'window resized' notification");

    if (RoR::App::GetOverlayWrapper())
        RoR::App::GetOverlayWrapper()->windowResized();
    if (gEnv->surveyMap)
        gEnv->surveyMap->windowResized();

    //update mouse area
    RoR::App::GetInputEngine()->windowResized(rw);

    m_actor_manager.NotifyActorsWindowResized();
}

//Unattach OIS before window shutdown (very important under Linux)
void RoRFrameListener::windowClosed(Ogre::RenderWindow* rw)
{
    // No on-screen rendering must be performed after window is closed -> crashes!
    LOG("[RoR|Simulation] Received \"WindowClosed\" event. Stopping rendering.");
    m_was_app_window_closed = true;
}

void RoRFrameListener::windowMoved(Ogre::RenderWindow* rw)
{
    LOG("*** windowMoved");
}

void RoRFrameListener::windowFocusChange(Ogre::RenderWindow* rw)
{
    // Too verbose
    //LOG("*** windowFocusChange");
    RoR::App::GetInputEngine()->resetKeys();
}

void RoRFrameListener::HideGUI(bool hidden)
{
    if (m_player_actor && m_player_actor->getReplay())
        m_player_actor->getReplay()->setHidden(hidden);

#ifdef USE_SOCKETW
    if (App::mp_state.GetActive() == MpState::CONNECTED)
    {
        App::GetGuiManager()->SetVisible_MpClientList(!hidden);
    }
#endif // USE_SOCKETW

    if (hidden)
    {
        if (RoR::App::GetOverlayWrapper())
            RoR::App::GetOverlayWrapper()->showDashboardOverlays(false, m_player_actor);
        if (gEnv->surveyMap)
            gEnv->surveyMap->setVisibility(false);
    }
    else
    {
        if (m_player_actor
            && gEnv->cameraManager
            && gEnv->cameraManager->hasActiveBehavior()
            && gEnv->cameraManager->getCurrentBehavior() != RoR::PerVehicleCameraContext::CAMCTX_BEHAVIOR_VEHICLE_CINECAM)
        {
            if (RoR::App::GetOverlayWrapper())
                RoR::App::GetOverlayWrapper()->showDashboardOverlays(true, m_player_actor);
        }
    }
    App::GetGuiManager()->hideGUI(hidden);
}

void RoRFrameListener::RemovePlayerActor()
{
    Actor* actor = m_player_actor;
    this->SetPlayerActor(nullptr);
    m_actor_manager.RemoveActorInternal(actor->ar_instance_id);
}

void RoRFrameListener::RemoveActorByCollisionBox(std::string const & ev_src_instance_name, std::string const & box_name)
{
    m_actor_manager.RemoveActorByCollisionBox(gEnv->collisions, ev_src_instance_name, box_name);
}

void RoRFrameListener::ReloadPlayerActor()
{
    if (!m_player_actor)
        return;
    if (m_player_actor->ar_sim_state == Actor::SimState::NETWORKED_OK)
        return;

    // try to load the same actor again
    Actor* new_actor = m_actor_manager.CreateLocalActor(m_reload_pos, m_reload_dir, m_player_actor->ar_filename, -1);

    if (!new_actor)
    {
        RoR::App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_ERROR, _L("unable to load new actor: limit reached. Please restart RoR"), "error.png");
        return;
    }

    // copy over the most basic info
    if (m_player_actor->ar_num_nodes == new_actor->ar_num_nodes)
    {
        for (int i = 0; i < m_player_actor->ar_num_nodes; i++)
        {
            // copy over nodes attributes if the amount of them didnt change
            new_actor->ar_nodes[i].AbsPosition = m_player_actor->ar_nodes[i].AbsPosition;
            new_actor->ar_nodes[i].RelPosition = m_player_actor->ar_nodes[i].RelPosition;
            new_actor->ar_nodes[i].Velocity    = m_player_actor->ar_nodes[i].Velocity;
            new_actor->ar_nodes[i].Forces      = m_player_actor->ar_nodes[i].Forces;
            new_actor->ar_nodes[i].initial_pos = m_player_actor->ar_nodes[i].initial_pos;
            new_actor->ar_origin               = m_player_actor->ar_origin;
        }
    }

    // TODO:
    // * copy over the engine infomation
    // * commands status
    // * other minor stati

    // notice the user about the amount of possible reloads
    String msg = TOSTRING(new_actor->ar_instance_id) + String(" of ") + TOSTRING(MAX_ACTORS) + String(" possible reloads.");
    RoR::App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE, msg, "information.png");
    RoR::App::GetGuiManager()->PushNotification("Notice:", msg);

    this->RemovePlayerActor();

    // reset the new actor (starts engine, resets gui, ...)
    new_actor->RequestActorReset();

    // enter the new actor
    this->SetPlayerActor(new_actor);
}

void RoRFrameListener::OnPlayerActorChange(Actor* previous_vehicle, Actor* current_vehicle)
{
    // hide any old dashes
    if (previous_vehicle && previous_vehicle->ar_dashboard)
    {
        previous_vehicle->ar_dashboard->setVisible3d(false);
    }
    // show new
    if (current_vehicle && current_vehicle->ar_dashboard)
    {
        current_vehicle->ar_dashboard->setVisible3d(true);
    }

    if (previous_vehicle)
    {
        if (RoR::App::GetOverlayWrapper())
        {
            RoR::App::GetOverlayWrapper()->showDashboardOverlays(false, previous_vehicle);
        }

        SOUND_STOP(previous_vehicle, SS_TRIG_AIR);
        SOUND_STOP(previous_vehicle, SS_TRIG_PUMP);
    }

    if (current_vehicle == nullptr)
    {
        // getting outside
        if (previous_vehicle->GetGfxActor()->GetVideoCamState() == GfxActor::VideoCamState::VCSTATE_ENABLED_ONLINE)
        {
            previous_vehicle->GetGfxActor()->SetVideoCamState(GfxActor::VideoCamState::VCSTATE_ENABLED_OFFLINE);
        }

        if (previous_vehicle && gEnv->player)
        {
            previous_vehicle->prepareInside(false);

            // get player out of the vehicle
            float rotation = previous_vehicle->getRotation() - Math::HALF_PI;
            Vector3 position = previous_vehicle->ar_nodes[0].AbsPosition;
            if (previous_vehicle->ar_cinecam_node[0] != -1 && previous_vehicle->ar_camera_node_pos[0] != -1 && previous_vehicle->ar_camera_node_roll[0] != -1)
            {
                // actor has a cinecam
                position = previous_vehicle->ar_nodes[previous_vehicle->ar_cinecam_node[0]].AbsPosition;
                position += -2.0 * ((previous_vehicle->ar_nodes[previous_vehicle->ar_camera_node_pos[0]].RelPosition - previous_vehicle->ar_nodes[previous_vehicle->ar_camera_node_roll[0]].RelPosition).normalisedCopy());
                position += Vector3(0.0, -1.0, 0.0);
            }
            gEnv->player->SetActorCoupling(false);
            gEnv->player->setRotation(Radian(rotation));
            gEnv->player->setPosition(position);
        }

        m_force_feedback->SetEnabled(false);

        TRIGGER_EVENT(SE_TRUCK_EXIT, previous_vehicle?previous_vehicle->ar_instance_id:-1);
    }
    else
    {
        // getting inside
        if (RoR::App::GetOverlayWrapper() && ! m_hide_gui)
        {
            RoR::App::GetOverlayWrapper()->showDashboardOverlays(true, current_vehicle);
        }

        if (current_vehicle->GetGfxActor()->GetVideoCamState() == GfxActor::VideoCamState::VCSTATE_ENABLED_OFFLINE)
        {
            current_vehicle->GetGfxActor()->SetVideoCamState(GfxActor::VideoCamState::VCSTATE_ENABLED_ONLINE);
        }

        // force feedback
        m_force_feedback->SetEnabled(current_vehicle->ar_driveable == TRUCK); //only for trucks so far

        // attach player to vehicle
        if (gEnv->player)
        {
            gEnv->player->SetActorCoupling(true, current_vehicle);
        }

        if (RoR::App::GetOverlayWrapper())
        {
            try
            {
                if (!current_vehicle->ar_help_panel_material.empty())
                {
                    OverlayManager::getSingleton().getOverlayElement("tracks/machinehelppanel")->setMaterialName(current_vehicle->ar_help_panel_material);
                }
                else
                {
                    OverlayManager::getSingleton().getOverlayElement("tracks/machinehelppanel")->setMaterialName("tracks/black");
                }
            }
            catch (Ogre::Exception& ex)
            {
                // Report the error
                std::stringstream msg;
                msg << "Error, help panel material (defined in 'help' or 'guisettings/helpMaterial') could not be loaded.\n"
                    "Exception occured, file:" << __FILE__ << ", line:" << __LINE__ << ", message:" << ex.what();
                LOG(msg.str());

                // Do not retry
                current_vehicle->ar_help_panel_material.clear();
            }
        }

        TRIGGER_EVENT(SE_TRUCK_ENTER, current_vehicle?current_vehicle->ar_instance_id:-1);
    }

    if (previous_vehicle != nullptr || current_vehicle != nullptr)
    {
        gEnv->cameraManager->NotifyVehicleChanged(previous_vehicle, current_vehicle);
    }
}

bool RoRFrameListener::LoadTerrain()
{
    // check if the resource is loaded
    Ogre::String terrain_file = App::sim_terrain_name.GetPending();
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

    if (App::GetSimTerrain() != nullptr)
    {
        // remove old terrain
        delete(App::GetSimTerrain()); // TODO: do it when leaving simulation.
    }

    App::SetSimTerrain(new TerrainManager());
    App::GetSimTerrain()->loadTerrain(terrain_file);
    App::sim_terrain_name.ApplyPending();

    App::GetGuiManager()->FrictionSettingsUpdateCollisions();

    if (gEnv->player != nullptr)
    {
        gEnv->player->setVisible(true);
        gEnv->player->setPosition(App::GetSimTerrain()->getSpawnPos());

        // Classic behavior, retained for compatibility.
        // Required for maps like N-Labs or F1 Track.
        if (!App::GetSimTerrain()->HasPredefinedActors())
        {
            gEnv->player->setRotation(Degree(180));
        }

        // Small hack to prevent spawning the Character in mid-air
        for (int i = 0; i < 100; i++)
        {
            gEnv->player->update(0.05f);
        }
    }

    // hide loading window
    App::GetGuiManager()->SetVisible_LoadingWindow(false);
    // hide wallpaper
    MyGUI::Window* w = MyGUI::Gui::getInstance().findWidget<MyGUI::Window>("wallpaper");
    if (w != nullptr)
    {
        w->setVisibleSmooth(false);
    }
    return true;
}

void RoRFrameListener::CleanupAfterSimulation()
{
#ifdef USE_ANGELSCRIPT
    ScriptEngine::getSingleton().SetFrameListener(nullptr);
#endif

    if (gEnv->surveyMap)
    {
        gEnv->surveyMap->setVisibility(false);
    }

    App::GetGuiManager()->GetMainSelector()->Reset();

    this->StopRaceTimer();

    App::DestroyOverlayWrapper();

    //Unload all vehicules
    m_actor_manager.CleanUpAllActors();

    delete gEnv->player;
    gEnv->player = nullptr;
    m_character_factory.DeleteAllRemoteCharacters();

    if (App::GetSimTerrain() != nullptr)
    {
        // remove old terrain
        delete(App::GetSimTerrain());
        App::SetSimTerrain(nullptr);
    }

    App::DeleteSceneMouse();
    App::GetGuiManager()->GetTeleport()->Reset();

    App::GetGuiManager()->SetVisible_LoadingWindow(false);
}

bool RoRFrameListener::SetupGameplayLoop()
{
#ifdef USE_ANGELSCRIPT
    ScriptEngine::getSingleton().SetFrameListener(this);
#endif

    auto* loading_window = App::GetGuiManager()->GetLoadingWindow();

    RoR::Log("[RoR] Loading resources...");

    loading_window->setProgress(0, _L("Loading resources"));
    App::GetContentManager()->LoadGameplayResources();

    // ============================================================================
    // Setup
    // ============================================================================

    m_actor_manager.GetParticleManager().DustManCheckAndInit(gEnv->sceneManager); // TODO: de-globalize SceneManager

    int colourNum = -1;

#ifdef USE_SOCKETW
    if (App::mp_state.GetActive() == MpState::CONNECTED)
    {
        wchar_t tmp[255] = L"";
        UTFString format = _L("Press %ls to start chatting");
        swprintf(tmp, 255, format.asWStr_c_str(), ANSI_TO_WCHAR(RoR::App::GetInputEngine()->getKeyForCommand(EV_COMMON_ENTER_CHATMODE)).c_str());
        App::GetGuiManager()->pushMessageChatBox(UTFString(tmp));

        RoRnet::UserInfo info = RoR::Networking::GetLocalUserData();
        colourNum = info.colournum;
    }
#endif // USE_SOCKETW

    gEnv->player = m_character_factory.createLocal(colourNum);

    // heathaze effect
    if (App::gfx_enable_heathaze.GetActive())
    {
        m_heathaze = new HeatHaze();
        m_heathaze->setEnable(true);
    }

    if (gEnv->cameraManager == nullptr)
    {
        // init camera manager after mygui and after we have a character
        gEnv->cameraManager = new CameraManager();
    }

    m_gfx_envmap.SetupEnvMap();

    // ============================================================================
    // Loading map
    // ============================================================================

    if (!App::diag_preset_terrain.IsActiveEmpty())
    {
        App::sim_terrain_name.SetPending(App::diag_preset_terrain.GetActive());
        App::diag_preset_terrain.SetActive("");
    }

    if (App::sim_terrain_name.IsPendingEmpty())
    {
        CacheEntry* selected_map = RoR::App::GetGuiManager()->GetMainSelector()->GetSelectedEntry();
        if (selected_map != nullptr)
        {
            App::sim_terrain_name.SetPending(selected_map->fname.c_str());
        }
        else
        {
            LOG("No map selected. Returning to menu.");
            App::GetGuiManager()->SetVisible_LoadingWindow(false);
            return false;
        }
    }

    if (! this->LoadTerrain())
    {
        LOG("Could not load map. Returning to menu.");
        App::GetMainMenu()->LeaveMultiplayerServer();
        App::GetGuiManager()->SetVisible_LoadingWindow(false);
        return false;
    }

    App::GetGuiManager()->GetTeleport()->SetupMap(
        this,
        &App::GetSimTerrain()->GetDef(),
        App::GetSimTerrain()->getMaxTerrainSize(),
        App::GetSimTerrain()->GetMinimapTextureName());

    // ========================================================================
    // Loading vehicle
    // ========================================================================

    if (!App::diag_preset_vehicle.IsActiveEmpty())
    {
        RoR::LogFormat("[RoR|Diag] Preselected Truck: %s", App::diag_preset_vehicle.GetActive());
        if (!App::diag_preset_veh_config.IsActiveEmpty())
        {
            RoR::LogFormat("[RoR|Diag] Preselected Truck Config: %s", App::diag_preset_veh_config.GetActive());
        }

        const std::vector<Ogre::String> actor_config = std::vector<Ogre::String>(1, App::diag_preset_veh_config.GetActive());

        Vector3 pos = gEnv->player->getPosition();
        Quaternion rot = Quaternion(Degree(180) - gEnv->player->getRotation(), Vector3::UNIT_Y);

        Actor* actor = m_actor_manager.CreateLocalActor(pos, rot, App::diag_preset_vehicle.GetActive(), -1, nullptr, &actor_config);

        if (actor != nullptr)
        {
            // Calculate translational offset for node[0] to align the actor's rotation center with m_reload_pos
            Vector3 translation = pos - actor->GetRotationCenter();
            actor->ResetPosition(actor->ar_nodes[0].AbsPosition + Vector3(translation.x, 0.0f, translation.z), true);

            actor->UpdateFlexbodiesPrepare();
            actor->UpdateFlexbodiesFinal();
            actor->updateVisual();

            if (App::diag_preset_veh_enter.GetActive() && actor->ar_num_nodes > 0)
            {
                this->SetPlayerActor(actor);
            }
            if (actor->ar_engine)
            {
                actor->ar_engine->StartEngine();
            }
        }
    }

    App::GetSimTerrain()->LoadPredefinedActors();

    // ========================================================================
    // Extra setup
    // ========================================================================

    if (ISETTING("OutGauge Mode", 0) > 0)
    {
        new OutProtocol();
    }

    App::CreateOverlayWrapper();
    App::GetOverlayWrapper()->SetupDirectionArrow();

    if (App::audio_menu_music.GetActive())
    {
        SOUND_KILL(-1, SS_TRIG_MAIN_MENU);
    }

    App::CreateSceneMouse();

    gEnv->sceneManager->setAmbientLight(Ogre::ColourValue(0.3f, 0.3f, 0.3f));

    if (App::gfx_enable_dof.GetActive())
    {
        gEnv->cameraManager->ActivateDepthOfFieldEffect();
    }

    return true;
}

void RoRFrameListener::EnterGameplayLoop()
{
    /* SETUP */

    App::GetOgreSubsystem()->GetOgreRoot()->addFrameListener(this);
    RoRWindowEventUtilities::addWindowEventListener(App::GetOgreSubsystem()->GetRenderWindow(), this);

    unsigned long timeSinceLastFrame = 1;
    unsigned long startTime = 0;
    unsigned long minTimePerFrame = 0;
    unsigned long fpsLimit = App::gfx_fps_limit.GetActive();

    if (fpsLimit < 10 || fpsLimit >= 200)
    {
        fpsLimit = 0;
    }

    if (fpsLimit)
    {
        minTimePerFrame = 1000 / fpsLimit;
    }

    /* LOOP */

    while (App::app_state.GetPending() == AppState::SIMULATION)
    {
        startTime = RoR::App::GetOgreSubsystem()->GetTimer()->getMilliseconds();

#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32 || OGRE_PLATFORM == OGRE_PLATFORM_LINUX
        RoRWindowEventUtilities::messagePump();
#endif
        Ogre::RenderWindow* rw = RoR::App::GetOgreSubsystem()->GetRenderWindow();
        if (rw->isClosed())
        {
            App::app_state.SetPending(AppState::SHUTDOWN);
            continue;
        }

        RoR::App::GetOgreSubsystem()->GetOgreRoot()->renderOneFrame();
#ifdef USE_SOCKETW
        if ((App::mp_state.GetActive() == MpState::CONNECTED) && RoR::Networking::CheckError())
        {
            Ogre::String title = Ogre::UTFString(_L("Network fatal error: ")).asUTF8();
            Ogre::String msg = RoR::Networking::GetErrorMessage().asUTF8();
            App::GetGuiManager()->ShowMessageBox(title, msg, true, "OK", true, false, "");
            App::app_state.SetPending(AppState::MAIN_MENU);
        }
#endif

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

    App::sim_state.SetActive(SimState::OFF);
    App::GetOgreSubsystem()->GetOgreRoot()->removeFrameListener(this); // IMPORTANT: Stop receiving render events during cleanups.
    App::GetGuiManager()->GetLoadingWindow()->setProgress(50, _L("Unloading Terrain"), !m_was_app_window_closed); // Renders a frame
    this->CleanupAfterSimulation();
    RoRWindowEventUtilities::removeWindowEventListener(App::GetOgreSubsystem()->GetRenderWindow(), this);
    // DO NOT: App::GetSceneMouse()    ->SetSimController(nullptr); -- already deleted via App::DeleteSceneMouse();      // TODO: de-globalize that object!
    // DO NOT: App::GetOverlayWrapper()->SetSimController(nullptr); -- already deleted via App::DestroyOverlayWrapper(); // TODO: de-globalize that object!
    gEnv->cameraManager->DisableDepthOfFieldEffect(); // TODO: de-globalize the CameraManager
}

void RoRFrameListener::SetPlayerActor(Actor* actor)
{
    m_prev_player_actor = m_player_actor;
    m_player_actor = actor;
    this->OnPlayerActorChange(m_prev_player_actor, m_player_actor); // TODO: Eliminate the arguments or the whole `OnPlayerActorChange` method. It's a leftover callback from era where 'cur/prev player actor' state was kept in ActorManager ~ only_a_ptr, 02/2018
    m_actor_manager.UpdateSleepingState(m_player_actor, 0.f);
}

void RoRFrameListener::SetPlayerActorById(int actor_id)
{
    Actor* actor = m_actor_manager.GetActorByIdInternal(actor_id);
    if (actor != nullptr)
    {
        this->SetPlayerActor(actor);
    }
}
