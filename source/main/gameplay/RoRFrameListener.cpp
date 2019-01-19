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
#include "Collisions.h"
#include "ContentManager.h"
#include "DashBoardManager.h"
#include "DustManager.h"
#include "EnvironmentMap.h"
#include "ForceFeedback.h"
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
#include "SkyManager.h"
#include "SkyXManager.h"
#include "SoundScriptManager.h"
#include "TerrainManager.h"
#include "TerrainObjectManager.h"
#include "Utils.h"
#include "Water.h"

#include "GUIManager.h"
#include "GUI_FrictionSettings.h"
#include "GUI_GameConsole.h"
#include "GUI_LoadingWindow.h"
#include "GUI_MainSelector.h"
#include "GUI_MultiplayerClientList.h"
#include "GUI_SimUtils.h"
#include "GUI_TopMenubar.h"

#include "SurveyMapManager.h"

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
    m_physics_simulation_paused(false),
    m_physics_simulation_time(0.0f),
    m_pressure_pressed(false),
    m_pressure_pressed_timer(0.0f),
    m_race_bestlap_time(0),
    m_race_id(-1),
    m_race_start_time(0),
    m_reload_dir(Quaternion::IDENTITY),
    m_reload_pos(Vector3::ZERO),
    m_stats_on(0),
    m_time(0),
    m_time_until_next_toggle(0),
    m_soft_reset_mode(false),
    m_advanced_vehicle_repair(false),
    m_advanced_vehicle_repair_timer(0.f),
    m_actor_info_gui_visible(false)
{
}

void SimController::UpdateForceFeedback()
{
    if (!App::io_ffb_enabled.GetActive()) { return; }

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

void SimController::StartRaceTimer(int id)
{
    m_race_start_time = m_time;
    m_race_id = id;
}

float SimController::StopRaceTimer()
{
    if (m_race_id != -1)
    {
        m_race_bestlap_time = m_time - m_race_start_time;
        m_race_start_time = 0.0f;
        m_race_id = -1;
    }

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

    if (App::sim_state.GetActive() == SimState::PAUSED)
        return; //Stop everything when pause menu is visible

    if (gui_man->IsVisible_FrictionSettings() && m_player_actor)
    {
        gui_man->GetFrictionSettings()->setActiveCol(m_player_actor->ar_last_fuzzy_ground_model);
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

        String fn_prefix = PathCombine(App::sys_screenshot_dir.GetActive(), "screenshot_");
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
        RoR::App::GetOgreSubsystem()->GetRenderWindow()->update();

        // TODO: Find a way to show ImGUI on the screenshot

        if (std::strcmp(App::app_screenshot_format.GetActive(), "png") == 0)
        {
            // add some more data into the image
            AdvancedScreen* as = new AdvancedScreen(RoR::App::GetOgreSubsystem()->GetRenderWindow(), tmpfn);
            //as->addData("terrain_Name", loadedTerrain);
            //as->addData("terrain_ModHash", terrainModHash);
            //as->addData("terrain_FileHash", terrainFileHash);
            if (m_player_actor)
            {
                as->addData("Truck_Num", TOSTRING(m_player_actor->ar_instance_id));
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
        (m_player_actor->ar_sim_state != Actor::SimState::NETWORKED_OK) &&
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
    if (this->GetCameraBehavior() != CameraManager::CAMERA_BEHAVIOR_STATIC) // the static camera has its own fov logic
    {
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
                RoR::App::GetGuiManager()->PushNotification("Notice:", _L("FOV: Limit reached"));
            }
        }
        if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_FOV_RESET))
        {
            float fov = gEnv->mainCamera->getFOVy().valueDegrees();
            if (this->GetCameraBehavior() == CameraManager::CAMERA_BEHAVIOR_VEHICLE_CINECAM)
            {
                fov = App::gfx_fov_internal.GetStored();
                App::gfx_fov_internal.SetActive(fov);
            }
            else
            {
                fov = App::gfx_fov_external.GetStored();
                App::gfx_fov_external.SetActive(fov);
            }
            gEnv->mainCamera->setFOVy(Degree(fov));
            RoR::App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE, _L("FOV: ") + TOSTRING(fov), "camera_edit.png", 2000);
            RoR::App::GetGuiManager()->PushNotification("Notice:", _L("FOV: ") + TOSTRING(fov));
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

    static auto& object_list = App::GetSimTerrain()->getObjectManager()->GetEditorObjects();
    static bool terrain_editing_track_object = true;
    static bool terrain_editing_mode = false;
    static int terrain_editing_rotation_axis = 1;
    static int object_count = object_list.size();
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
            std::string path = PathCombine(RoR::App::sys_config_dir.GetActive(), "editor_out.cfg");
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
        if (object_count != object_list.size())
        {
            object_count = object_list.size();
            object_index = -1;
        }
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
                    gEnv->player->setPosition(sn->getPosition());
                }
            }
            else if (terrain_editing_track_object && gEnv->player->getPosition() != sn->getPosition())
            {
                object_list[object_index].position = gEnv->player->getPosition();
                sn->setPosition(gEnv->player->getPosition());
            }
            if (RoR::App::GetInputEngine()->getEventBoolValue(EV_COMMON_REMOVE_CURRENT_TRUCK))
            {
                App::GetSimTerrain()->getObjectManager()->unloadObject(object_list[object_index].instance_name);
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
                if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_TOGGLE_PHYSICS))
                {
                    for (auto actor : m_player_actor->GetAllLinkedActors())
                    {
                        actor->ar_physics_paused = !m_player_actor->ar_physics_paused;
                    }
                    m_player_actor->ar_physics_paused = !m_player_actor->ar_physics_paused;
                }
                if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_TOGGLE_RESET_MODE))
                {
                    m_soft_reset_mode = !m_soft_reset_mode;
                    if (m_soft_reset_mode)
                        RoR::App::GetGuiManager()->PushNotification("Notice:", _L("Enabled soft reset mode"));
                    else
                        RoR::App::GetGuiManager()->PushNotification("Notice:", _L("Enabled hard reset mode"));
                }
                if (!RoR::App::GetInputEngine()->getEventBoolValue(EV_COMMON_REPAIR_TRUCK))
                {
                    m_advanced_vehicle_repair_timer = 0.0f;
                }
                if (RoR::App::GetInputEngine()->getEventBoolValue(EV_COMMON_RESET_TRUCK) && !m_player_actor->ar_replay_mode)
                {
                    ActorModifyRequest rq;
                    rq.amr_actor = m_player_actor;
                    rq.amr_type  = ActorModifyRequest::Type::RESET_ON_INIT_POS;
                    this->QueueActorModify(rq);
                }
                else if (RoR::App::GetInputEngine()->getEventBoolValue(EV_COMMON_REMOVE_CURRENT_TRUCK) && !m_player_actor->ar_replay_mode)
                {
                    this->QueueActorRemove(m_player_actor);
                }
                else if ((RoR::App::GetInputEngine()->getEventBoolValue(EV_COMMON_REPAIR_TRUCK) || m_advanced_vehicle_repair) && !m_player_actor->ar_replay_mode)
                {
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

                        Vector3 rotation_center = m_player_actor->GetRotationCenter();

                        rotation *= Ogre::Math::Clamp(scale, 0.1f, 10.0f);
                        translation *= scale;

                        m_player_actor->RequestRotation(rotation, rotation_center);
                        m_player_actor->RequestTranslation(translation);

                        if (m_soft_reset_mode)
                        {
                            for (auto actor : m_player_actor->GetAllLinkedActors())
                            {
                                actor->RequestRotation(rotation, rotation_center);
                                actor->RequestTranslation(translation);
                            }
                        }

                        m_advanced_vehicle_repair_timer = 0.0f;
                    }
                    else if (RoR::App::GetInputEngine()->isKeyDownValueBounce(OIS::KC_SPACE))
                    {
                        m_player_actor->RequestAngleSnap(45);
                        if (m_soft_reset_mode)
                        {
                            for (auto actor : m_player_actor->GetAllLinkedActors())
                            {
                                actor->RequestAngleSnap(45);
                            }
                        }
                    }
                    else
                    {
                        m_advanced_vehicle_repair_timer += dt;
                    }

                    auto reset_type = ActorModifyRequest::Type::RESET_ON_SPOT;
                    if (m_soft_reset_mode)
                    {
                        reset_type = ActorModifyRequest::Type::SOFT_RESET;
                        for (auto actor : m_player_actor->GetAllLinkedActors())
                        {
                            ActorModifyRequest rq;
                            rq.amr_actor = actor;
                            rq.amr_type = reset_type;
                            this->QueueActorModify(rq);
                        }
                    }

                    ActorModifyRequest rq;
                    rq.amr_actor = m_player_actor;
                    rq.amr_type = reset_type;
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
                    } else if (m_physics_simulation_paused && m_actor_manager.GetSimulationSpeed() > 0.0f)
                    {
                        if (RoR::App::GetInputEngine()->getEventBoolValue(EV_COMMON_REPLAY_FAST_FORWARD) ||
                            RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_REPLAY_FORWARD, 0.25f))
                        {
                            m_physics_simulation_time = PHYSICS_DT / m_actor_manager.GetSimulationSpeed();
                        }
                    }

                    if (m_player_actor->ar_driveable == TRUCK)
                    {
                        LandVehicleSimulation::UpdateInputEvents(m_player_actor, dt);
                    }
                    if (m_player_actor->ar_driveable == AIRPLANE)
                    {
                        AircraftSimulation::UpdateInputEvents(m_player_actor, dt);
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

                    if (m_race_id == -1 && RoR::App::GetInputEngine()->getEventBoolValue(EV_COMMON_ACCELERATE_SIMULATION))
                    {
                        float simulation_speed = m_actor_manager.GetSimulationSpeed() * pow(2.0f, dt / 2.0f);
                        m_actor_manager.SetSimulationSpeed(simulation_speed);
                        String ssmsg = _L("New simulation speed: ") + TOSTRING(Round(simulation_speed * 100.0f, 1)) + "%";
                        RoR::App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE, ssmsg, "infromation.png", 2000, false);
                        RoR::App::GetGuiManager()->PushNotification("Notice:", ssmsg);
                    }
                    if (m_race_id == -1 && RoR::App::GetInputEngine()->getEventBoolValue(EV_COMMON_DECELERATE_SIMULATION))
                    {
                        float simulation_speed = m_actor_manager.GetSimulationSpeed() * pow(0.5f, dt / 2.0f);
                        m_actor_manager.SetSimulationSpeed(simulation_speed);
                        String ssmsg = _L("New simulation speed: ") + TOSTRING(Round(simulation_speed * 100.0f, 1)) + "%";
                        RoR::App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE, ssmsg, "infromation.png", 2000, false);
                        RoR::App::GetGuiManager()->PushNotification("Notice:", ssmsg);
                    }
                    if (m_race_id == -1 && RoR::App::GetInputEngine()->getEventBoolValue(EV_COMMON_RESET_SIMULATION_PACE))
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
                        m_player_actor->ar_toggle_ropes = true;
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
                        m_player_actor->ar_toggle_ties = true;
                    }

                    //replay mode
                    if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_TOGGLE_REPLAY_MODE))
                    {
                        if (m_player_actor->getReplay() != nullptr)
                        {
                            m_player_actor->setReplayMode(!m_player_actor->ar_replay_mode);
                        } else
                        {
                            m_physics_simulation_paused = !m_physics_simulation_paused;
                        }
                    }

                    if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_TOGGLE_CUSTOM_PARTICLES))
                    {
                        m_player_actor->ToggleCustomParticles();
                    }

                    if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_TOGGLE_DEBUG_VIEW))
                    {
                        m_player_actor->GetGfxActor()->ToggleDebugView();
                        for (auto actor : m_player_actor->GetAllLinkedActors())
                        {
                            actor->GetGfxActor()->SetDebugView(m_player_actor->GetGfxActor()->GetDebugView());
                        }
                    }

                    if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_CYCLE_DEBUG_VIEWS))
                    {
                        m_player_actor->GetGfxActor()->CycleDebugViews();
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
                        float change = m_player_actor->GetTyrePressure() * (1.0f - pow(2.0f, dt / 2.0f));
                        change = Math::Clamp(change, -dt * 10.0f, -dt * 1.0f);
                        if (m_pressure_pressed = m_player_actor->AddTyrePressure(change))
                        {
                            if (RoR::App::GetOverlayWrapper())
                                RoR::App::GetOverlayWrapper()->showPressureOverlay(true);

                            SOUND_START(m_player_actor, SS_TRIG_AIR);
                        }
                    }
                    else if (RoR::App::GetInputEngine()->getEventBoolValue(EV_COMMON_PRESSURE_MORE))
                    {
                        float change = m_player_actor->GetTyrePressure() * (pow(2.0f, dt / 2.0f) - 1.0f);
                        change = Math::Clamp(change, +dt * 1.0f, +dt * 10.0f);
                        if (m_pressure_pressed = m_player_actor->AddTyrePressure(change))
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
                        m_pressure_pressed_timer = 1.5f;
                    }
                    else if (m_pressure_pressed_timer > 0.0f)
                    {
                        m_pressure_pressed_timer -= dt;
                    }
                    else
                    {
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
        SkyManager* sky_mgr = App::GetSimTerrain()->getSkyManager();
        if (caelum_enabled && (sky_mgr != nullptr) && (simRUNNING(s) || simPAUSED(s) || simEDITOR(s)))
        {
            Real time_factor = 1.0f;

            if (RoR::App::GetInputEngine()->getEventBoolValue(EV_SKY_INCREASE_TIME))
            {
                time_factor = 1000.0f;
            }
            else if (RoR::App::GetInputEngine()->getEventBoolValue(EV_SKY_INCREASE_TIME_FAST))
            {
                time_factor = 10000.0f;
            }
            else if (RoR::App::GetInputEngine()->getEventBoolValue(EV_SKY_DECREASE_TIME))
            {
                time_factor = -1000.0f;
            }
            else if (RoR::App::GetInputEngine()->getEventBoolValue(EV_SKY_DECREASE_TIME_FAST))
            {
                time_factor = -10000.0f;
            }

            if (sky_mgr->GetSkyTimeFactor() != time_factor)
            {
                sky_mgr->SetSkyTimeFactor(time_factor);
                RoR::App::GetGuiManager()->PushNotification("Notice:", _L("Time set to ") + sky_mgr->GetPrettyTime());
                RoR::App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE, _L("Time set to ") + sky_mgr->GetPrettyTime(), "weather_sun.png", 1000);
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
                m_player_actor->ar_brake = 0.66f;
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
                ActorSpawnRequest rq;
                rq.asr_cache_entry     = m_last_cache_selection;
                rq.asr_config          = m_last_vehicle_configs;
                rq.asr_skin            = m_last_skin_selection;
                rq.asr_origin          = ActorSpawnRequest::Origin::USER;
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

    if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_TOGGLE_DASHBOARD))
    {
        if (RoR::App::GetOverlayWrapper())
        {
            RoR::App::GetOverlayWrapper()->ToggleDashboardOverlays(m_player_actor);
        }
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

void SimController::TeleportPlayerXZ(float x, float z)
{
    Real y = gEnv->collisions->getSurfaceHeight(x, z);
    if (!m_player_actor)
    {
        gEnv->player->setPosition(Vector3(x, y, z));
        return;
    }

    TRIGGER_EVENT(SE_TRUCK_TELEPORT, m_player_actor->ar_instance_id);

    Vector3 translation = Vector3(x, y, z) - m_player_actor->ar_nodes[0].AbsPosition;

    auto actors = m_player_actor->GetAllLinkedActors();
    actors.push_back(m_player_actor);

    float src_agl = std::numeric_limits<float>::max(); 
    float dst_agl = std::numeric_limits<float>::max(); 
    for (auto actor : actors)
    {
        for (int i = 0; i < actor->ar_num_nodes; i++)
        {
            Vector3 pos = actor->ar_nodes[i].AbsPosition;
            src_agl = std::min(pos.y - gEnv->collisions->getSurfaceHeight(pos.x, pos.z), src_agl);
            pos += translation;
            dst_agl = std::min(pos.y - gEnv->collisions->getSurfaceHeight(pos.x, pos.z), dst_agl);
        }
    }

    translation += Vector3::UNIT_Y * (std::max(0.0f, src_agl) - dst_agl);

    for (auto actor : actors)
    {
        actor->ResetPosition(actor->ar_nodes[0].AbsPosition + translation, false);
    }
}

void SimController::FinalizeActorSpawning(Actor* actor, ActorSpawnRequest rq)
{
    if (rq.asr_spawnbox == nullptr)
    {
        // Calculate translational offset for node[0] to align the actor's rotation center with m_reload_pos
        Vector3 translation = rq.asr_position - actor->GetRotationCenter();
        translation.y = 0;
        // and to avoid ground collisions
        float bgl = 0.0f; 
        for (int i = 0; i < actor->ar_num_nodes; i++)
        {
            float h = actor->GetMaxHeight(true);
            Vector3 pos = actor->ar_nodes[i].AbsPosition + translation;
            bgl = std::max(bgl, gEnv->collisions->getSurfaceHeightBelow(pos.x, pos.z, h) - pos.y);
        }
        translation.y += bgl;

        actor->ResetPosition(actor->ar_nodes[0].AbsPosition + translation, true);
    }

    if (actor->ar_engine != nullptr && App::sim_spawn_running.GetActive())
    {
        actor->ar_engine->StartEngine();
    }

    if (actor->ar_driveable != NOT_DRIVEABLE)
    {
        this->SetPendingPlayerActor(actor);
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
        
        // Remove modify-requests for this actor
        m_actor_modify_queue.erase(
            std::remove_if(
                m_actor_modify_queue.begin(), m_actor_modify_queue.end(),
                [actor](ActorModifyRequest& rq) -> bool { return rq.amr_actor == actor; }),
            m_actor_modify_queue.end());

        m_actor_manager.DeleteActorInternal(actor);
    }
    m_actor_remove_queue.clear();

    for (ActorModifyRequest& rq: m_actor_modify_queue)
    {
        if (rq.amr_type == ActorModifyRequest::Type::SOFT_RESET)
        {
            rq.amr_actor->SoftReset();
        }
        if ((rq.amr_type == ActorModifyRequest::Type::RESET_ON_SPOT) ||
            (rq.amr_type == ActorModifyRequest::Type::RESET_ON_INIT_POS))
        {
            rq.amr_actor->SyncReset(rq.amr_type == ActorModifyRequest::Type::RESET_ON_INIT_POS);
        }
        if (rq.amr_type == ActorModifyRequest::Type::RELOAD)
        {
            if (m_player_actor == rq.amr_actor) // Check if the request is up-to-date
            {
                String filename = m_player_actor->ar_filename;
                auto reload_pos = m_player_actor->getPosition();
                auto reload_dir = Quaternion(Degree(270) - Radian(m_player_actor->getRotation()), Vector3::UNIT_Y);
                auto debug_view = m_player_actor->GetGfxActor()->GetDebugView();

                reload_pos.y = m_player_actor->GetMinHeight();

                m_prev_player_actor = nullptr;
                this->ChangePlayerActor(nullptr);
                m_gfx_scene.RemoveGfxActor(rq.amr_actor->GetGfxActor());
                m_actor_manager.DeleteActorInternal(rq.amr_actor);
                App::GetCacheSystem()->UnloadActorDefFromMemory(filename); // Force reload from filesystem

                ActorSpawnRequest srq;
                srq.asr_position = reload_pos;
                srq.asr_rotation = reload_dir;
                srq.asr_filename = filename;
                Actor* new_actor = this->SpawnActorDirectly(srq); // try to load the same actor again
                if (new_actor)
                {
                    this->FinalizeActorSpawning(new_actor, srq);
                    new_actor->GetGfxActor()->SetDebugView(debug_view);
                }
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
                    float h = m_player_actor->GetMaxHeight(true);
                    rq.asr_rotation = Quaternion(Degree(270) - Radian(m_player_actor->getRotation()), Vector3::UNIT_Y);
                    rq.asr_position = m_player_actor->GetRotationCenter();
                    rq.asr_position.y = gEnv->collisions->getSurfaceHeightBelow(rq.asr_position.x, rq.asr_position.z, h);
                    rq.asr_position.y += m_player_actor->GetHeightAboveGroundBelow(h, true); // retain height above ground
                }
                else
                {
                    rq.asr_rotation = Quaternion(Degree(180) - gEnv->player->getRotation(), Vector3::UNIT_Y);
                    rq.asr_position = gEnv->player->getPosition();
                }
            }

            Actor* fresh_actor = this->SpawnActorDirectly(rq);
            if (fresh_actor != nullptr)
            {
                this->FinalizeActorSpawning(fresh_actor, rq);
                if (rq.asr_spawnbox == nullptr)
                {
                    // Try to resolve collisions with other actors
                    fresh_actor->resolveCollisions(50.0f, m_player_actor == nullptr);
                }
            }
        }
        else if (rq.asr_origin == ActorSpawnRequest::Origin::CONFIG_FILE)
        {
            Actor* fresh_actor = this->SpawnActorDirectly(rq);
            if (fresh_actor != nullptr)
            {
                this->FinalizeActorSpawning(fresh_actor, rq);
                if (App::diag_preset_veh_enter.GetActive() && fresh_actor->ar_num_nodes > 0)
                {
                    this->SetPendingPlayerActor(fresh_actor);
                }
            }
        }
        else if (rq.asr_origin == ActorSpawnRequest::Origin::TERRN_DEF)
        {
            Actor* fresh_actor = this->SpawnActorDirectly(rq);
            if (fresh_actor != nullptr)
            {
                if (rq.asr_terrn_machine)
                {
                    fresh_actor->ar_driveable = MACHINE;
                }

                String type = SurveyMapManager::getTypeByDriveable(fresh_actor->ar_driveable);
                App::GetSimController()->GetGfxScene().GetSurveyMap()->createMapEntity(type);
            }
        }
        else
        {
            Actor* fresh_actor = this->SpawnActorDirectly(rq);
            if (fresh_actor != nullptr)
            {
                this->FinalizeActorSpawning(fresh_actor, rq);
            }
        }
    }
    m_actor_spawn_queue.clear();

    if (m_pending_player_actor != m_player_actor)
    {
        m_prev_player_actor = m_player_actor ? m_player_actor : m_prev_player_actor;
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
#ifdef USE_OPENAL
            // update audio listener position
            static Vector3 lastCameraPosition;
            Vector3 cameraSpeed = (gEnv->mainCamera->getPosition() - lastCameraPosition) / dt;
            lastCameraPosition = gEnv->mainCamera->getPosition();

            SoundScriptManager::getSingleton().setCamera(gEnv->mainCamera->getPosition(), gEnv->mainCamera->getDirection(), gEnv->mainCamera->getUp(), cameraSpeed);
#endif // USE_OPENAL
        }
    }

    m_physics_simulation_time = m_physics_simulation_paused ? 0.0f : dt;

    this->UpdateInputEvents(dt);

    RoR::App::GetGuiManager()->DrawSimulationGui(dt);

#ifdef USE_ANGELSCRIPT
    ScriptEngine::getSingleton().framestep(dt);
#endif

    for (auto actor : GetActors())
    {
        actor->GetGfxActor()->UpdateDebugView();
    }

    if (simRUNNING(s) || simEDITOR(s))
    {
        float simulation_speed = m_actor_manager.GetSimulationSpeed();
        if (m_race_id != -1 && simulation_speed != 1.0f)
        {
            m_last_simulation_speed = simulation_speed;
            m_actor_manager.SetSimulationSpeed(1.0f);
        }

        this->UpdateForceFeedback();

        RoR::App::GetGuiManager()->UpdateSimUtils(dt, m_player_actor);

        m_scene_mouse.UpdateSimulation();

        m_gfx_scene.BufferSimulationData();

        if (App::sim_state.GetPending() != SimState::PAUSED)
        {
            m_actor_manager.UpdateActors(m_player_actor, m_physics_simulation_time); // *** Start new physics tasks. No reading from Actor N/B beyond this point.
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
                    return;
                }
            }
        }
    }
    
    App::sim_state.SetActive(SimState::SELECTING); // TODO: use 'pending' mechanism

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
        App::GetGuiManager()->SetVisible_MpClientList(!hidden);
#endif // USE_SOCKETW

    if (RoR::App::GetOverlayWrapper())
        RoR::App::GetOverlayWrapper()->showDashboardOverlays(!hidden, m_player_actor);

    m_gfx_scene.GetSurveyMap()->hideGUI(hidden); // TODO: we shouldn't update GfxScene-owned objects from simulation, but this whole HideGUI() function will likely end up being invoked by GfxActor in the future, so it's OK for now ~ only_a_ptr, 05/2018

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

        spawn_pos.y = gEnv->collisions->getSurfaceHeightBelow(spawn_pos.x, spawn_pos.z, spawn_pos.y + 1.8f);

        gEnv->player->setPosition(spawn_pos);
        gEnv->player->setRotation(Degree(spawn_rot));
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

    App::GetGuiManager()->SetVisible_LoadingWindow(false);
}

bool SimController::SetupGameplayLoop()
{
    RoR::Log("[RoR] Loading resources...");

    App::GetGuiManager()->GetLoadingWindow()->setProgress(0, _L("Loading resources"));
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
    }

    // ========================================================================
    // Extra setup
    // ========================================================================

    if (App::io_outgauge_mode.GetActive() > 0)
    {
        m_out_protocol = std::unique_ptr<OutProtocol>(new OutProtocol());
    }

    App::CreateOverlayWrapper();
    App::GetOverlayWrapper()->SetupDirectionArrow();

    if (App::audio_menu_music.GetActive())
    {
        SOUND_KILL(-1, SS_TRIG_MAIN_MENU);
    }

    m_scene_mouse.InitializeVisuals(); // TODO: Move to GfxScene ~ only_a_ptr, 06/2018

    gEnv->sceneManager->setAmbientLight(Ogre::ColourValue(0.3f, 0.3f, 0.3f));

    return true;
}

void SimController::EnterGameplayLoop()
{
    OgreBites::WindowEventUtilities::addWindowEventListener(App::GetOgreSubsystem()->GetRenderWindow(), this);

    Ogre::RenderWindow* rw = RoR::App::GetOgreSubsystem()->GetRenderWindow();

    auto start_time = std::chrono::high_resolution_clock::now();

    while (App::app_state.GetPending() == AppState::SIMULATION)
    {
        OgreBites::WindowEventUtilities::messagePump();
        if (rw->isClosed())
        {
            App::app_state.SetPending(AppState::SHUTDOWN);
            continue;
        }

        // Check FPS limit
        if (App::gfx_fps_limit.GetActive() > 0)
        {
            const float min_frame_time = 1.0f / Ogre::Math::Clamp(App::gfx_fps_limit.GetActive(), 5, 240);
            float dt = std::chrono::duration<float>(std::chrono::high_resolution_clock::now() - start_time).count();
            while (dt < min_frame_time)
            {
                dt = std::chrono::duration<float>(std::chrono::high_resolution_clock::now() - start_time).count();
            }
        }

        const auto now = std::chrono::high_resolution_clock::now();
        const float dt_sec = std::chrono::duration<float>(now - start_time).count();
        start_time = now;

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
            const char* text = RoR::Networking::GetErrorMessage().asUTF8_c_str();
            App::GetGuiManager()->ShowMessageBox(_L("Network fatal error: "), text);
            App::app_state.SetPending(AppState::MAIN_MENU);
        }
#endif

        if (!rw->isActive() && rw->isVisible())
        {
            rw->update(); // update even when in background !
        }
    }

    m_actor_manager.SyncWithSimThread(); // Wait for background tasks to finish
    App::sim_state.SetActive(SimState::OFF);
    App::GetGuiManager()->GetLoadingWindow()->setProgress(50, _L("Unloading Terrain"), !m_was_app_window_closed); // Renders a frame
    this->CleanupAfterSimulation();
    OgreBites::WindowEventUtilities::removeWindowEventListener(App::GetOgreSubsystem()->GetRenderWindow(), this);
}

void SimController::ChangePlayerActor(Actor* actor)
{
    Actor* prev_player_actor = m_player_actor;
    m_player_actor = actor;

    // hide any old dashes
    if (prev_player_actor && prev_player_actor->ar_dashboard)
    {
        prev_player_actor->ar_dashboard->setVisible3d(false);
    }
    // show new
    if (m_player_actor && m_player_actor->ar_dashboard)
    {
        m_player_actor->ar_dashboard->setVisible3d(true);
    }

    if (prev_player_actor)
    {
        if (RoR::App::GetOverlayWrapper())
        {
            RoR::App::GetOverlayWrapper()->showDashboardOverlays(false, prev_player_actor);
        }

        SOUND_STOP(prev_player_actor, SS_TRIG_AIR);
        SOUND_STOP(prev_player_actor, SS_TRIG_PUMP);
    }

    if (m_player_actor == nullptr)
    {
        // getting outside

        if (prev_player_actor)
        {
            if (prev_player_actor->GetGfxActor()->GetVideoCamState() == GfxActor::VideoCamState::VCSTATE_ENABLED_ONLINE)
            {
                prev_player_actor->GetGfxActor()->SetVideoCamState(GfxActor::VideoCamState::VCSTATE_ENABLED_OFFLINE);
            }

            prev_player_actor->prepareInside(false);

            // get player out of the vehicle
            float h = prev_player_actor->getMinCameraRadius();
            float rotation = prev_player_actor->getRotation() - Math::HALF_PI;
            Vector3 position = prev_player_actor->getPosition();
            if (prev_player_actor->ar_cinecam_node[0] != -1)
            {
                // actor has a cinecam (find optimal exit position)
                Vector3 l = position - 2.0f * prev_player_actor->GetCameraRoll();
                Vector3 r = position + 2.0f * prev_player_actor->GetCameraRoll();
                float l_h = gEnv->collisions->getSurfaceHeightBelow(l.x, l.z, l.y + h);
                float r_h = gEnv->collisions->getSurfaceHeightBelow(r.x, r.z, r.y + h);
                position  = std::abs(r.y - r_h) * 1.2f < std::abs(l.y - l_h) ? r : l;
            }
            position.y = gEnv->collisions->getSurfaceHeightBelow(position.x, position.z, position.y + h);

            if (gEnv->player)
            {
                gEnv->player->SetActorCoupling(false);
                gEnv->player->setRotation(Radian(rotation));
                gEnv->player->setPosition(position);
            }
        }

        m_force_feedback->SetEnabled(false);

        TRIGGER_EVENT(SE_TRUCK_EXIT, prev_player_actor?prev_player_actor->ar_instance_id:-1);
    }
    else
    {
        // getting inside
        if (RoR::App::GetOverlayWrapper() != nullptr)
        {
            RoR::App::GetOverlayWrapper()->showDashboardOverlays(!m_hide_gui, m_player_actor);
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

    if (prev_player_actor != nullptr || m_player_actor != nullptr)
    {
        m_camera_manager.NotifyVehicleChanged(prev_player_actor, m_player_actor);
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

