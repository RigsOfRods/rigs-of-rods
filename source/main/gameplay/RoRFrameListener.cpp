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
#include "PlatformUtils.h"
#include "Replay.h"
#include "RigDef_File.h"
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

SimController::SimController(RoR::ForceFeedback* ff, RoR::SkidmarkConfig* skid_conf) :
    m_player_actor(nullptr),
    m_prev_player_actor(nullptr),
    m_pending_player_actor(nullptr),
    m_actor_manager(),
    m_character_factory(),
    m_dir_arrow_pointed(Vector3::ZERO),
    m_force_feedback(ff),
    m_skidmark_conf(skid_conf),
    m_hide_gui(false),
    m_was_app_window_closed(false),
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

void SimController::UpdateForceFeedback(float dt)
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
        Ogre::Vector3 ff_vehicle = m_player_actor->GetFFbBodyForces();
        m_force_feedback->SetForces(
            -ff_vehicle.dotProduct(m_player_actor->GetCameraRoll()) / 10000.0,
             ff_vehicle.dotProduct(m_player_actor->GetCameraDir())  / 10000.0,
            m_player_actor->ar_wheel_speed,
            m_player_actor->ar_hydro_dir_command,
            m_player_actor->GetFFbHydroForces());
    }
}

void SimController::StartRaceTimer()
{
    m_race_start_time = (int)m_time; // TODO: This adds of up to 0.9 sec to player's time! Fix it! ~ only_a_ptr, 05/2018
    m_race_in_progress = true;
}

float SimController::StopRaceTimer()
{
    float time;

    if (m_race_in_progress)
    {
        time = static_cast<float>(m_time - m_race_start_time);
        m_race_bestlap_time = time;
    }

    m_race_start_time = 0;
    m_race_in_progress = false;
    return m_race_bestlap_time;
}

void SimController::UpdateInputEvents(float dt)
{
    if (dt == 0.0f)
        return;

    auto s = App::sim_state.GetActive();
    auto gui_man = App::GetGuiManager();

    RoR::App::GetInputEngine()->updateKeyBounces(dt);
    if (!RoR::App::GetInputEngine()->getInputsChanged())
        return;

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
        return; //Stop everything when pause menu is visible

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

        String fn_prefix = std::string(App::sys_screenshot_dir.GetActive()) + RoR::PATH_SLASH + String("screenshot_");
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

    if ((m_player_actor != nullptr) &&
        (m_player_actor->ar_sim_state == Actor::SimState::NETWORKED_OK) &&
        RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_TRUCKEDIT_RELOAD, 0.5f))
    {
        ActorModifyRequest rq;
        rq.amr_type = ActorModifyRequest::Type::RELOAD;
        rq.amr_actor = m_player_actor;
        this->QueueActorModify(rq);

        return;
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
            if (this->GetCameraBehavior() == CameraManager::CAMERA_BEHAVIOR_VEHICLE_CINECAM)
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
            std::string path = std::string(RoR::App::sys_config_dir.GetActive()) + RoR::PATH_SLASH + "editor_out.cfg";
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
                Vector3 ref_pos = this->AreControlsLocked() ? gEnv->mainCamera->getPosition() : gEnv->player->getPosition();
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
        if (object_index != -1 && !this->AreControlsLocked())
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
        if (!this->AreControlsLocked())
        {
            if (m_player_actor) // we are in a vehicle
            {
                if (!RoR::App::GetInputEngine()->getEventBoolValue(EV_COMMON_REPAIR_TRUCK))
                {
                    m_advanced_vehicle_repair_timer = 0.0f;
                }
                if (RoR::App::GetInputEngine()->getEventBoolValue(EV_COMMON_RESET_TRUCK) && !m_player_actor->ar_replay_mode)
                {
                    this->StopRaceTimer();
                    ActorModifyRequest rq;
                    rq.amr_actor = m_player_actor;
                    rq.amr_type  = ActorModifyRequest::Type::RESET_ON_INIT_POS;
                    this->QueueActorModify(rq);
                }
                else if (RoR::App::GetInputEngine()->getEventBoolValue(EV_COMMON_REMOVE_CURRENT_TRUCK) && !m_player_actor->ar_replay_mode)
                {
                    this->StopRaceTimer();
                    this->QueueActorRemove(m_player_actor);
                }
                else if ((RoR::App::GetInputEngine()->getEventBoolValue(EV_COMMON_REPAIR_TRUCK) || m_advanced_vehicle_repair) && !m_player_actor->ar_replay_mode)
                {
                    this->StopRaceTimer();
                    if (RoR::App::GetInputEngine()->getEventBoolValue(EV_COMMON_REPAIR_TRUCK))
                    {
                        m_advanced_vehicle_repair = m_advanced_vehicle_repair_timer > 1.0f;
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

                        m_player_actor->RequestRotation(rotation * scale);
                        m_player_actor->RequestTranslation(translation * scale);

                        m_advanced_vehicle_repair_timer = 0.0f;
                    }
                    else
                    {
                        m_advanced_vehicle_repair_timer += dt;
                    }

                    ActorModifyRequest rq;
                    rq.amr_actor = m_player_actor;
                    rq.amr_type  = ActorModifyRequest::Type::RESET_ON_SPOT;
                    this->QueueActorModify(rq);
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
                        this->QueueActorRemove(m_player_actor);
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
                        m_player_actor->GetGfxActor()->ToggleSkeletonView();
                        for (auto actor : m_player_actor->GetAllLinkedActors())
                        {
                            actor->GetGfxActor()->SetDebugView(m_player_actor->GetGfxActor()->GetDebugView());
                        }
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
                            this->SetPendingPlayerActor(rescuer);
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
            if (this->GetPlayerActor() == nullptr)
            {
                // find the nearest vehicle
                float mindist = 1000.0;
                Actor* nearest_actor = nullptr;
                for (auto actor : GetActors())
                {
                    if (!actor->ar_driveable)
                        continue;
                    if (actor->ar_cinecam_node[0] == -1)
                    {
                        LOG("cinecam missing, cannot enter the actor!");
                        continue;
                    }
                    float len = 0.0f;
                    if (gEnv->player)
                    {
                        len = actor->ar_nodes[actor->ar_cinecam_node[0]].AbsPosition.distance(gEnv->player->getPosition() + Vector3(0.0, 2.0, 0.0));
                    }
                    if (len < mindist)
                    {
                        mindist = len;
                        nearest_actor = actor;
                    }
                }
                if (mindist < 20.0)
                {
                    this->SetPendingPlayerActor(nearest_actor);
                }
            }
            else if (m_player_actor->ar_nodes[0].Velocity.length() < 1.0f)
            {
                this->SetPendingPlayerActor(nullptr);
            }
            else
            {
                m_player_actor->ar_brake = m_player_actor->ar_brake_force * 0.66;
                m_time_until_next_toggle = 0.0; // No delay in this case: the vehicle must brake like braking normally
            }
        }
        else if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_ENTER_NEXT_TRUCK, 0.25f))
        {
            Actor* actor = m_actor_manager.FetchNextVehicleOnList(m_player_actor, m_prev_player_actor);

            if (actor != m_player_actor)
            {
                this->SetPendingPlayerActor(actor);
            }
        }
        else if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_ENTER_PREVIOUS_TRUCK, 0.25f))
        {
            Actor* actor = m_actor_manager.FetchPreviousVehicleOnList(m_player_actor, m_prev_player_actor);

            if (actor != m_player_actor)
            {
                this->SetPendingPlayerActor(actor);
            }
        }
        else if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_RESPAWN_LAST_TRUCK, 0.25f))
        {
            if (m_last_cache_selection != nullptr)
            {
                /* We load an extra actor */

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

                ActorSpawnRequest rq;
                rq.asr_position        = m_reload_pos;
                rq.asr_rotation        = m_reload_dir;
                rq.asr_filename        = m_last_cache_selection->fname;
                rq.asr_cache_entry_num = m_last_cache_selection->number;
                rq.asr_config          = m_last_vehicle_configs;
                rq.asr_skin            = m_last_skin_selection;
                m_actor_spawn_queue.push_back(rq);
            }
        }
    }
    else
    {
        //no terrain or actor loaded
        terrain_editing_mode = false;
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

    if (m_time_until_next_toggle >= 0)
    {
        m_time_until_next_toggle -= dt;
    }
}

void SimController::TeleportPlayer(RoR::Terrn2Telepoint* telepoint)
{
    if (m_player_actor != nullptr)
        return; // Player could enter an actor while Teleport-GUI is visible

    gEnv->player->setPosition(telepoint->position);
}

void SimController::TeleportPlayerXZ(float x, float z)
{
    if (m_player_actor)
        return; // Player could enter an actor while Teleport-GUI is visible

    float y = App::GetSimTerrain()->GetHeightAt(x, z);
    gEnv->player->setPosition(Ogre::Vector3(x, y, z));
}

void SimController::FinalizeActorSpawning(Actor* local_actor, Actor* prev_actor, ActorSpawnRequest rq)
{
    if (local_actor != nullptr)
    {
        if (rq.asr_spawnbox == nullptr)
        {
            // Calculate translational offset for node[0] to align the actor's rotation center with m_reload_pos
            Vector3 translation = rq.asr_position - local_actor->GetRotationCenter();
            local_actor->ResetPosition(local_actor->ar_nodes[0].AbsPosition + Vector3(translation.x, 0.0f, translation.z), true);

            if (local_actor->ar_driveable != NOT_DRIVEABLE || (prev_actor && prev_actor->ar_driveable != NOT_DRIVEABLE))
            {
                // Try to resolve collisions with other actors
                local_actor->resolveCollisions(50.0f, prev_actor == nullptr);
            }
        }

        if (local_actor->ar_driveable != NOT_DRIVEABLE)
        {
            // We are supposed to be in this vehicle, if it is a vehicle
            if (local_actor->ar_engine != nullptr)
            {
                local_actor->ar_engine->StartEngine();
            }
            this->SetPendingPlayerActor(local_actor);
        }

        local_actor->updateVisual();
    }
}

void SimController::UpdateSimulation(float dt)
{
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

    // ACTOR CHANGE REQUESTS - Handle early (so that other logic can reflect it)
    //   1. Removal requests - Done first; respective entries in 'modify' queue are erased.
    //   2. Modify requests - currently just reload, will be extended
    //   3. Spawn requests - may outdate others by changing player seating
    //   4. Player seating change requests - TODO; currently done ad-hoc
    for (Actor* actor: m_actor_remove_queue)
    {
        if (actor == m_player_actor)
        {
            Vector3 center = m_player_actor->GetRotationCenter();
            this->ChangePlayerActor(nullptr); // Get out of the vehicle
            gEnv->player->setPosition(center);
        }

        if (actor == m_prev_player_actor)
        {
            m_prev_player_actor = nullptr;
        }

        if (actor == m_pending_player_actor)
        {
            m_pending_player_actor = m_player_actor; // Reset the requested change.
        }

        m_gfx_scene.RemoveGfxActor(actor->GetGfxActor());
        m_actor_manager.DeleteActorInternal(actor);
        
        // Remove modify-requests for this actor
        m_actor_modify_queue.erase(
            std::remove_if(
                m_actor_modify_queue.begin(), m_actor_modify_queue.end(),
                [actor](ActorModifyRequest& rq) -> bool { return rq.amr_actor == actor; }),
            m_actor_modify_queue.end());
    }
    m_actor_remove_queue.clear();

    for (ActorModifyRequest& rq: m_actor_modify_queue)
    {
        if ((rq.amr_type == ActorModifyRequest::Type::RESET_ON_SPOT) ||
            (rq.amr_type == ActorModifyRequest::Type::RESET_ON_INIT_POS))
        {
            rq.amr_actor->SyncReset(rq.amr_type == ActorModifyRequest::Type::RESET_ON_INIT_POS);
        }
        if (rq.amr_type == ActorModifyRequest::Type::RELOAD)
        {
            if (m_player_actor == rq.amr_actor) // Check if the request is up-to-date
            {
                m_actor_manager.UnloadTruckfileFromMemory(m_player_actor->ar_filename.c_str()); // Force reload from filesystem
                ActorSpawnRequest srq;
                srq.asr_position = m_reload_pos;
                srq.asr_rotation = m_reload_dir;
                srq.asr_filename = m_player_actor->ar_filename;
                Actor* new_actor = this->SpawnActorDirectly(srq); // try to load the same actor again

                // copy over the most basic info
                if (m_player_actor->ar_num_nodes == new_actor->ar_num_nodes)
                {
                    for (int i = 0; i < m_player_actor->ar_num_nodes; i++)
                    {
                        // copy over nodes attributes if the amount of them didnt change
                        new_actor->ar_nodes[i].AbsPosition = m_player_actor->ar_nodes[i].AbsPosition; // TODO: makes sense? the Reset below overwrites it. ~only_a_ptr, 09/2018
                        new_actor->ar_nodes[i].RelPosition = m_player_actor->ar_nodes[i].RelPosition; // TODO: ditto
                        new_actor->ar_nodes[i].Velocity    = m_player_actor->ar_nodes[i].Velocity;    // TODO: ditto
                        new_actor->ar_nodes[i].Forces      = m_player_actor->ar_nodes[i].Forces;      // TODO: ditto
                        new_actor->ar_nodes[i].initial_pos = m_player_actor->ar_nodes[i].initial_pos;
                        new_actor->ar_origin               = m_player_actor->ar_origin;
                    }
                }

                // TODO:
                // * copy over the engine infomation
                // * commands status
                // * other minor stati

                this->QueueActorRemove(m_player_actor); // TODO: The 'reload' command is hybrid removal/modify, so doesn't fit the 1.2.3. scheme of things ~ only_a_ptr, 09/2018

                // reset the new actor (starts engine, resets gui, ...)
                new_actor->SyncReset(false); // false = Do not reset position

                // enter the new actor
                this->SetPendingPlayerActor(new_actor);
            }
        }
    }
    m_actor_modify_queue.clear();

    for (ActorSpawnRequest& rq: m_actor_spawn_queue)
    {
        if (rq.asr_origin == ActorSpawnRequest::Origin::USER)
        {
            m_last_cache_selection = rq.asr_cache_entry;
            m_last_skin_selection  = rq.asr_skin;
            m_last_vehicle_configs = rq.asr_config;

            if (rq.asr_spawnbox == nullptr)
            {
                if (m_player_actor != nullptr)
                {
                    float rotation = m_player_actor->getRotation() - Math::HALF_PI;
                    rq.asr_rotation = Quaternion(Degree(180) - Radian(rotation), Vector3::UNIT_Y);
                    rq.asr_position = m_player_actor->GetRotationCenter();
                    // TODO: Fix this by projecting m_reload_pos onto the terrain / mesh
                    rq.asr_position.y = m_player_actor->ar_nodes[m_player_actor->ar_lowest_contacting_node].AbsPosition.y;
                }
                else
                {
                    rq.asr_rotation = Quaternion(Degree(180) - gEnv->player->getRotation(), Vector3::UNIT_Y);
                    rq.asr_position = gEnv->player->getPosition();
                }
            }

            Actor* fresh_actor = this->SpawnActorDirectly(rq);
            this->FinalizeActorSpawning(fresh_actor, m_player_actor, rq);
        }
        else if (rq.asr_origin == ActorSpawnRequest::Origin::CONFIG_FILE)
        {
            Actor* fresh_actor = this->SpawnActorDirectly(rq);

            // Calculate translational offset for node[0] to align the actor's rotation center with m_reload_pos
            Vector3 translation = rq.asr_position - fresh_actor->GetRotationCenter();
            fresh_actor->ResetPosition(fresh_actor->ar_nodes[0].AbsPosition + Vector3(translation.x, 0.0f, translation.z), true);

            if (App::diag_preset_veh_enter.GetActive())
            {
                if (fresh_actor->ar_num_nodes > 0)
                {
                    this->SetPendingPlayerActor(fresh_actor);
                }
                App::diag_preset_veh_enter.SetActive(false);
            }
            if (fresh_actor->ar_engine)
            {
                fresh_actor->ar_engine->StartEngine();
            }
        }
        else if (rq.asr_origin == ActorSpawnRequest::Origin::TERRN_DEF)
        {
            Actor* fresh_actor = this->SpawnActorDirectly(rq);

            if (rq.asr_terrn_machine)
            {
                fresh_actor->ar_driveable = MACHINE;
            }

            if (App::GetSimController()->GetGfxScene().GetSurveyMap() != nullptr)
            {
                SurveyMapEntity* e = App::GetSimController()->GetGfxScene().GetSurveyMap()->createNamedMapEntity(
                    "Truck" + std::to_string(fresh_actor->ar_instance_id),
                    SurveyMapManager::getTypeByDriveable(fresh_actor->ar_driveable));
                if (e != nullptr)
                {
                    e->setState(static_cast<int>(Actor::SimState::LOCAL_SIMULATED));
                    e->setVisibility(true);
                    e->setPosition(rq.asr_position.x, rq.asr_position.z);
                    e->setRotation(-Radian(fresh_actor->getRotation()));
                }
            }
        }
        else
        {
            Actor* fresh_actor = this->SpawnActorDirectly(rq);

            this->FinalizeActorSpawning(fresh_actor, m_player_actor, rq);
        }
    }
    m_actor_spawn_queue.clear();

    if (m_pending_player_actor != m_player_actor)
    {
        this->ChangePlayerActor(m_pending_player_actor); // 'Pending' remains the same until next change is queued
    }

    RoR::App::GetInputEngine()->Capture();
    auto s = App::sim_state.GetActive();

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
        if ((!simPAUSED(s)) && (dt != 0.f))
        {
            m_camera_manager.Update(dt, m_player_actor, m_actor_manager.GetSimulationSpeed());
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

    this->UpdateInputEvents(dt);

    RoR::App::GetGuiManager()->DrawSimulationGui(dt);

    const bool is_altkey_pressed =  App::GetInputEngine()->isKeyDown(OIS::KeyCode::KC_LMENU) || App::GetInputEngine()->isKeyDown(OIS::KeyCode::KC_RMENU);
    App::GetGuiManager()->GetTeleport()->TeleportWindowFrameStep(
        gEnv->player->getPosition().x, gEnv->player->getPosition().z, is_altkey_pressed);

#ifdef USE_ANGELSCRIPT
    ScriptEngine::getSingleton().framestep(dt);
#endif

    if (simRUNNING(s) || simPAUSED(s) || simEDITOR(s))
    {
        this->UpdateForceFeedback(dt);

        RoR::App::GetGuiManager()->UpdateSimUtils(dt, m_player_actor);

        for (auto actor : GetActors())
        {
            actor->GetGfxActor()->UpdateDebugView();
        }

        if (!simPAUSED(s))
        {
            if (m_player_actor != nullptr)
            {
                m_scene_mouse.UpdateSimulation();
            }

            m_gfx_scene.BufferSimulationData();

            m_actor_manager.UpdateActors(m_player_actor, dt); // *** Start new physics tasks. No reading from Actor N/B beyond this point.
        }
    }
}

void SimController::ShowLoaderGUI(int type, const Ogre::String& instance, const Ogre::String& box)
{
    // first, test if the place if clear, BUT NOT IN MULTIPLAYER
    if (!(App::mp_state.GetActive() == MpState::CONNECTED))
    {
        collision_box_t* spawnbox = gEnv->collisions->getBox(instance, box);
        for (auto actor : GetActors())
        {
            for (int i = 0; i < actor->ar_num_nodes; i++)
            {
                if (gEnv->collisions->isInside(actor->ar_nodes[i].AbsPosition, spawnbox))
                {
                    RoR::App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE, _L("Please clear the place first"), "error.png");
                    RoR::App::GetGuiManager()->PushNotification("Notice:", _L("Please clear the place first"));
                    gEnv->collisions->clearEventCache();
                    return;
                }
            }
        }
    }
    
    App::sim_state.SetActive(SimState::SELECTING); // TODO: use 'pending' mechanism
    if (m_gfx_scene.GetSurveyMap())
        m_gfx_scene.GetSurveyMap()->setVisibility(false); // TODO: we shouldn't update GfxScene-owned objects from simulation, we should queue the update ~ only_a_ptr, 05/2018

    ActorSpawnRequest rq;
    rq.asr_position = gEnv->collisions->getPosition(instance, box);
    rq.asr_rotation = gEnv->collisions->getDirection(instance, box);
    rq.asr_spawnbox = gEnv->collisions->getBox(instance, box);
    App::GetGuiManager()->GetMainSelector()->Show(LoaderType(type), rq);
}

void SimController::UpdateDirectionArrow(char* text, Vector3 position)
{
    if (RoR::App::GetOverlayWrapper() == nullptr)
        return;

    if (text == nullptr)
    {
        RoR::App::GetOverlayWrapper()->HideDirectionOverlay();
        m_dir_arrow_pointed = Vector3::ZERO;
    }
    else
    {
        RoR::App::GetOverlayWrapper()->ShowDirectionOverlay(text);
        m_dir_arrow_pointed = position;
    }
}

/* --- Window Events ------------------------------------------ */
void SimController::windowResized(Ogre::RenderWindow* rw)
{
    if (!rw)
        return;
    LOG("[RoR] Received 'window resized' notification");

    if (RoR::App::GetOverlayWrapper())
        RoR::App::GetOverlayWrapper()->windowResized();

    if (m_gfx_scene.GetSurveyMap())
        m_gfx_scene.GetSurveyMap()->windowResized(); // TODO: we shouldn't update GfxScene-owned objects from simulation, we should queue the update ~ only_a_ptr, 05/2018

    //update mouse area
    RoR::App::GetInputEngine()->windowResized(rw);

    m_actor_manager.NotifyActorsWindowResized();
}

//Unattach OIS before window shutdown (very important under Linux)
void SimController::windowClosed(Ogre::RenderWindow* rw)
{
    // No on-screen rendering must be performed after window is closed -> crashes!
    LOG("[RoR|Simulation] Received \"WindowClosed\" event. Stopping rendering.");
    m_was_app_window_closed = true;
}

void SimController::windowMoved(Ogre::RenderWindow* rw)
{
    LOG("*** windowMoved");
}

void SimController::windowFocusChange(Ogre::RenderWindow* rw)
{
    // Too verbose
    //LOG("*** windowFocusChange");
    RoR::App::GetInputEngine()->resetKeys();
}

void SimController::HideGUI(bool hidden)
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

        if (m_gfx_scene.GetSurveyMap())
            m_gfx_scene.GetSurveyMap()->setVisibility(false); // TODO: we shouldn't update GfxScene-owned objects from simulation, but this whole HideGUI() function will likely end up being invoked by GfxActor in the future, so it's OK for now ~ only_a_ptr, 05/2018
    }
    else
    {
        if (m_player_actor && (this->GetCameraBehavior() == CameraManager::CAMERA_BEHAVIOR_VEHICLE_CINECAM))
        {
            if (RoR::App::GetOverlayWrapper())
                RoR::App::GetOverlayWrapper()->showDashboardOverlays(true, m_player_actor);
        }
    }
    App::GetGuiManager()->hideGUI(hidden);
}

void SimController::RemoveActorByCollisionBox(std::string const & ev_src_instance_name, std::string const & box_name)
{
    Actor* actor = m_actor_manager.FindActorInsideBox(gEnv->collisions, ev_src_instance_name, box_name);
    if (actor != nullptr)
    {
        this->QueueActorRemove(actor);
    }
}

bool SimController::LoadTerrain()
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
            App::GetGuiManager()->ShowMessageBox(title.asUTF8_c_str(), msg.asUTF8_c_str());
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

    TerrainManager* terrain = new TerrainManager();
    App::SetSimTerrain(terrain); // The terrain preparation logic relies on it.
    if (!terrain->LoadAndPrepareTerrain(terrain_file))
    {
        App::GetGuiManager()->ShowMessageBox("Failed to load terrain", "See 'RoR.log' for more info.", true, "OK", nullptr);
        App::SetSimTerrain(nullptr);
        delete terrain;
        App::sim_terrain_name.ResetPending();
        return false;
    }
    App::sim_terrain_name.ApplyPending();

    // Init minimap
    RoR::App::GetGuiManager()->GetLoadingWindow()->setProgress(50, _L("Initializing Overview Map Subsystem"));
    App::GetSimController()->GetGfxScene().InitSurveyMap(terrain->getMaxTerrainSize());

    App::GetGuiManager()->FrictionSettingsUpdateCollisions();

    if (gEnv->player != nullptr)
    {
        Vector3 spawn_pos = App::GetSimTerrain()->getSpawnPos();
        Real spawn_rot = 0.0f;

        // Classic behavior, retained for compatibility.
        // Required for maps like N-Labs or F1 Track.
        if (!App::GetSimTerrain()->HasPredefinedActors())
        {
            spawn_rot = 180.0f;
        }

        if (!App::diag_preset_spawn_pos.IsActiveEmpty())
        {
            spawn_pos = StringConverter::parseVector3(String(App::diag_preset_spawn_pos.GetActive()), spawn_pos);
            App::diag_preset_spawn_pos.SetActive("");
        }
        if (!App::diag_preset_spawn_rot.IsActiveEmpty())
        {
            spawn_rot = StringConverter::parseReal(App::diag_preset_spawn_rot.GetActive(), spawn_rot);
            App::diag_preset_spawn_rot.SetActive("");
        }

        gEnv->player->setPosition(spawn_pos);
        gEnv->player->setRotation(Degree(spawn_rot));

        // Small hack to prevent spawning the Character in mid-air
        for (int i = 0; i < 100; i++)
        {
            gEnv->player->update(0.05f);
        }

        gEnv->player->setVisible(true);

        gEnv->mainCamera->setPosition(gEnv->player->getPosition());

        // Small hack to improve the spawn experience
        for (int i = 0; i < 100; i++)
        {
            m_camera_manager.Update(0.02f, nullptr, 1.0f);
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

void SimController::CleanupAfterSimulation()
{
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

    m_scene_mouse.DiscardVisuals(); // TODO: move this to GfxScene ~~ only_a_ptr, 06/2018
    App::GetGuiManager()->GetTeleport()->Reset();

    App::GetGuiManager()->SetVisible_LoadingWindow(false);
}

bool SimController::SetupGameplayLoop()
{
    auto* loading_window = App::GetGuiManager()->GetLoadingWindow();

    RoR::Log("[RoR] Loading resources...");

    loading_window->setProgress(0, _L("Loading resources"));
    App::GetContentManager()->LoadGameplayResources();

    // ============================================================================
    // Setup
    // ============================================================================

    m_gfx_scene.InitScene(gEnv->sceneManager); // TODO: de-globalize SceneManager

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

    // init camera manager after mygui and after we have a character
    m_camera_manager.SetCameraReady(); // TODO: get rid of this hack; see == SimCam == ~ only_a_ptr, 06/2018

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
        &App::GetSimTerrain()->GetDef(),
        App::GetSimTerrain()->getMaxTerrainSize());

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

        ActorSpawnRequest rq;
        rq.asr_filename   = App::diag_preset_vehicle.GetActive();
        rq.asr_config     = std::vector<Ogre::String>(1, App::diag_preset_veh_config.GetActive());
        rq.asr_position   = gEnv->player->getPosition();
        rq.asr_rotation   = Quaternion(Degree(180) - gEnv->player->getRotation(), Vector3::UNIT_Y);
        rq.asr_origin     = ActorSpawnRequest::Origin::CONFIG_FILE;
        this->QueueActorSpawn(rq);

        App::diag_preset_vehicle.SetActive("");
        App::diag_preset_veh_config.SetActive("");
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

    m_scene_mouse.InitializeVisuals(); // TODO: Move to GfxScene ~ only_a_ptr, 06/2018

    gEnv->sceneManager->setAmbientLight(Ogre::ColourValue(0.3f, 0.3f, 0.3f));

    if (App::gfx_enable_dof.GetActive())
    {
        m_camera_manager.ActivateDepthOfFieldEffect();
    }

    return true;
}

void SimController::EnterGameplayLoop()
{
    RoRWindowEventUtilities::addWindowEventListener(App::GetOgreSubsystem()->GetRenderWindow(), this);

    Ogre::RenderWindow* rw = RoR::App::GetOgreSubsystem()->GetRenderWindow();
    Ogre::Timer timer;
    unsigned long start_time = 0; // milliseconds
    unsigned long start_time_prev = 0;

    while (App::app_state.GetPending() == AppState::SIMULATION)
    {
        RoRWindowEventUtilities::messagePump();
        if (rw->isClosed())
        {
            App::app_state.SetPending(AppState::SHUTDOWN);
            continue;
        }

        // Query timer
        start_time_prev = start_time;
        start_time = timer.getMilliseconds();
        const unsigned long frame_time_ms = start_time - start_time_prev;

        // Check FPS limit
        if (App::gfx_fps_limit.GetActive() != 0)
        {
            if (App::gfx_fps_limit.GetActive() < 5)
            {
                App::gfx_fps_limit.SetActive(5);
            }

            // NOTE: Calculation adjusted to get required FPS.
            //       When using '1s = 1000ms' the FPS goes over the limit by cca. +75%, I'm not sure why ~ only_a_ptr, 07/2018
            const unsigned long min_frame_time = 1580 / App::gfx_fps_limit.GetActive();
            if (frame_time_ms < min_frame_time)
            {
                std::chrono::milliseconds sleep_time(min_frame_time - frame_time_ms);
                std::this_thread::sleep_for(sleep_time);
            }
        }

        // Check simulation state change
        if (App::sim_state.GetPending() != App::sim_state.GetActive())
        {
            if (App::sim_state.GetActive() == SimState::RUNNING)
            {
                if (App::sim_state.GetPending() == SimState::PAUSED)
                {
                    m_actor_manager.MuteAllActors();
                    App::sim_state.ApplyPending();
                }
            }
            else if (App::sim_state.GetActive() == SimState::PAUSED)
            {
                if (App::sim_state.GetPending() == SimState::RUNNING)
                {
                    m_actor_manager.UnmuteAllActors();
                    App::sim_state.ApplyPending();
                }                
            }
        }

        // Update gameplay and 3D scene
        const float dt_sec = static_cast<float>(frame_time_ms) * 0.001f;
        App::GetGuiManager()->NewImGuiFrame(dt_sec);

        if (dt_sec != 0.f)
        {
            m_time += dt_sec;
            this->UpdateSimulation(dt_sec);
            if (RoR::App::sim_state.GetActive() != RoR::SimState::PAUSED)
            {
                m_gfx_scene.UpdateScene(dt_sec);
            }
        }

        // TODO: Ugly! Currently it seems drawing DearIMGUI only works when invoked from `Ogre::FrameListener::frameRenderingQueued`.
        //       We only want GUI on screen, not other targets (reflections etc...), so we can't have it attached permanently.
        //       Research and find a more elegant solution.  ~ only_a_ptr, 07/2018
        RoR::App::GetOgreSubsystem()->GetOgreRoot()->addFrameListener(&App::GetGuiManager()->GetImGui());
        RoR::App::GetOgreSubsystem()->GetOgreRoot()->renderOneFrame();
        RoR::App::GetOgreSubsystem()->GetOgreRoot()->removeFrameListener(&App::GetGuiManager()->GetImGui());

        if (m_stats_on && RoR::App::GetOverlayWrapper())
        {
            RoR::App::GetOverlayWrapper()->updateStats();
        }

#ifdef USE_SOCKETW
        if ((App::mp_state.GetActive() == MpState::CONNECTED) && RoR::Networking::CheckError())
        {
            const char* title = LanguageEngine::getSingleton().lookUp("Network fatal error: ").asUTF8_c_str();
            const char* text = RoR::Networking::GetErrorMessage().asUTF8_c_str();
            App::GetGuiManager()->ShowMessageBox(title, text);
            App::app_state.SetPending(AppState::MAIN_MENU);
        }
#endif

        if (!rw->isActive() && rw->isVisible())
        {
            rw->update(); // update even when in background !
        }
    }

    App::sim_state.SetActive(SimState::OFF);
    App::GetGuiManager()->GetLoadingWindow()->setProgress(50, _L("Unloading Terrain"), !m_was_app_window_closed); // Renders a frame
    this->CleanupAfterSimulation();
    RoRWindowEventUtilities::removeWindowEventListener(App::GetOgreSubsystem()->GetRenderWindow(), this);
    // DO NOT: App::GetOverlayWrapper()->SetSimController(nullptr); -- already deleted via App::DestroyOverlayWrapper(); // TODO: de-globalize that object!
    m_camera_manager.DisableDepthOfFieldEffect();
}

void SimController::ChangePlayerActor(Actor* actor)
{
    m_prev_player_actor = m_player_actor;
    m_player_actor = actor;

    // hide any old dashes
    if (m_prev_player_actor && m_prev_player_actor->ar_dashboard)
    {
        m_prev_player_actor->ar_dashboard->setVisible3d(false);
    }
    // show new
    if (m_player_actor && m_player_actor->ar_dashboard)
    {
        m_player_actor->ar_dashboard->setVisible3d(true);
    }

    if (m_prev_player_actor)
    {
        if (RoR::App::GetOverlayWrapper())
        {
            RoR::App::GetOverlayWrapper()->showDashboardOverlays(false, m_prev_player_actor);
        }

        SOUND_STOP(m_prev_player_actor, SS_TRIG_AIR);
        SOUND_STOP(m_prev_player_actor, SS_TRIG_PUMP);
    }

    if (m_player_actor == nullptr)
    {
        // getting outside

        if (m_prev_player_actor)
        {
            if (m_prev_player_actor->GetGfxActor()->GetVideoCamState() == GfxActor::VideoCamState::VCSTATE_ENABLED_ONLINE)
            {
                m_prev_player_actor->GetGfxActor()->SetVideoCamState(GfxActor::VideoCamState::VCSTATE_ENABLED_OFFLINE);
            }

            m_prev_player_actor->prepareInside(false);

            // get player out of the vehicle
            float rotation = m_prev_player_actor->getRotation() - Math::HALF_PI;
            Vector3 position = m_prev_player_actor->ar_nodes[0].AbsPosition;
            if (m_prev_player_actor->ar_cinecam_node[0] != -1)
            {
                // actor has a cinecam
                position = m_prev_player_actor->ar_nodes[m_prev_player_actor->ar_cinecam_node[0]].AbsPosition;
                position += -2.0 * m_prev_player_actor->GetCameraRoll();
                position += Vector3(0.0, -1.0, 0.0);
            }

            if (gEnv->player)
            {
                gEnv->player->SetActorCoupling(false);
                gEnv->player->setRotation(Radian(rotation));
                gEnv->player->setPosition(position);
            }
        }

        m_force_feedback->SetEnabled(false);

        TRIGGER_EVENT(SE_TRUCK_EXIT, m_prev_player_actor?m_prev_player_actor->ar_instance_id:-1);
    }
    else
    {
        // getting inside
        if (RoR::App::GetOverlayWrapper() && ! m_hide_gui)
        {
            RoR::App::GetOverlayWrapper()->showDashboardOverlays(true, m_player_actor);
        }

        if (m_player_actor->GetGfxActor()->GetVideoCamState() == GfxActor::VideoCamState::VCSTATE_ENABLED_OFFLINE)
        {
            m_player_actor->GetGfxActor()->SetVideoCamState(GfxActor::VideoCamState::VCSTATE_ENABLED_ONLINE);
        }

        // force feedback
        m_force_feedback->SetEnabled(m_player_actor->ar_driveable == TRUCK); //only for trucks so far

        // attach player to vehicle
        if (gEnv->player)
        {
            gEnv->player->SetActorCoupling(true, m_player_actor);
        }

        if (RoR::App::GetOverlayWrapper())
        {
            try
            {
                if (!m_player_actor->ar_help_panel_material.empty())
                {
                    OverlayManager::getSingleton().getOverlayElement("tracks/machinehelppanel")->setMaterialName(m_player_actor->ar_help_panel_material);
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
                m_player_actor->ar_help_panel_material.clear();
            }
        }

        TRIGGER_EVENT(SE_TRUCK_ENTER, m_player_actor?m_player_actor->ar_instance_id:-1);
    }

    if (m_prev_player_actor != nullptr || m_player_actor != nullptr)
    {
        m_camera_manager.NotifyVehicleChanged(m_prev_player_actor, m_player_actor);
    }

    m_actor_manager.UpdateSleepingState(m_player_actor, 0.f);
}

bool SimController::AreControlsLocked() const
{
    // TODO: remove camera manager from gEnv, see == SimCam == comment in CameraManager.cpp ~ only_a_ptr
    return (m_camera_manager.IsCameraReady()
          && m_camera_manager.gameControlsLocked());
}

void SimController::ResetCamera()
{
    // Temporary function, see == SimCam == comment in CameraManager.cpp ~ only_a_ptr

    if (m_camera_manager.IsCameraReady()) // TODO: remove camera manager from gEnv, see SimCam
    {
        // TODO: Detect camera changes from sim. state, don't rely on callback; see SimCam
        m_camera_manager.NotifyContextChange();
    }
}

CameraManager::CameraBehaviors SimController::GetCameraBehavior()
{
    if (m_camera_manager.IsCameraReady())
    {
        return static_cast<CameraManager::CameraBehaviors>(
            m_camera_manager.getCurrentBehavior());
    }
    return CameraManager::CAMERA_BEHAVIOR_INVALID;
}

// Temporary interface until camera controls are refactored; see == SimCam == ~ only_a_ptr, 06/2018
bool SimController::CameraManagerMouseMoved(const OIS::MouseEvent& _arg)
{
    if (!m_camera_manager.IsCameraReady())
    {
        return true; // This is what SceneMouse expects
    }
    return m_camera_manager.mouseMoved(_arg);
}

// Temporary interface until camera controls are refactored; see == SimCam == ~ only_a_ptr, 06/2018
void SimController::CameraManagerMousePressed(const OIS::MouseEvent& _arg, OIS::MouseButtonID _id)
{
    if (m_camera_manager.IsCameraReady())
    {
        m_camera_manager.mousePressed(_arg, _id);
    }
}

Actor* SimController::SpawnActorDirectly(RoR::ActorSpawnRequest rq)
{
    if (rq.asr_cache_entry != nullptr)
    {
        rq.asr_cache_entry_num = rq.asr_cache_entry->number;
        rq.asr_filename = rq.asr_cache_entry->fname;
    }

    std::shared_ptr<RigDef::File> def = m_actor_manager.FetchActorDef(
        rq.asr_filename.c_str(), rq.asr_origin == ActorSpawnRequest::Origin::TERRN_DEF);
    if (def == nullptr)
    {
        return nullptr; // Error already reported
    }

    LOG(" ===== LOADING VEHICLE: " + rq.asr_filename);
    Actor* actor = m_actor_manager.CreateActorInstance(rq, def);

    if (rq.asr_origin != ActorSpawnRequest::Origin::NETWORK)
    {
        // lock slide nodes after spawning the actor?
        if (def->slide_nodes_connect_instantly)
        {
            actor->ToggleSlideNodeLock();
        }

        RoR::App::GetGuiManager()->GetTopMenubar()->triggerUpdateVehicleList();

        // add own username to the actor
        if (RoR::App::mp_state.GetActive() == RoR::MpState::CONNECTED)
        {
            actor->UpdateNetworkInfo();
        }
    }

    return actor;
}

