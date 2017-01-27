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
#include "IHeightFinder.h"
#include "IWater.h"
#include "InputEngine.h"
#include "LandVehicleSimulation.h"
#include "Language.h"
#include "MainMenu.h"
#include "Mirrors.h"
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
#include "TruckHUD.h"
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

#define simRUNNING(_S_) (_S_ == App::SIM_STATE_RUNNING    )
#define  simPAUSED(_S_) (_S_ == App::SIM_STATE_PAUSED     )
#define  simSELECT(_S_) (_S_ == App::SIM_STATE_SELECTING  )
#define  simEDITOR(_S_) (_S_ == App::SIM_STATE_EDITOR_MODE)

RoRFrameListener::RoRFrameListener(RoR::ForceFeedback* ff) :
    m_dir_arrow_pointed(Vector3::ZERO),
    m_heathaze(nullptr),
    m_force_feedback(ff),
    m_hide_gui(false),
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
    m_advanced_truck_repair(false),
    m_advanced_truck_repair_timer(0.f),
    m_truck_info_on(false)
{
}

RoRFrameListener::~RoRFrameListener()
{
}

void RoRFrameListener::UpdateForceFeedback(float dt)
{
    if (!App::GetIoFFbackEnabled()) { return; }
    if (!RoR::App::GetInputEngine()->getForceFeedbackDevice())
    {
        LOG("No force feedback device detected, disabling force feedback");
        App::SetIoFFbackEnabled(false);
        return;
    }

    Beam* current_truck = BeamFactory::getSingleton().getCurrentTruck();
    if (current_truck && current_truck->driveable == TRUCK)
    {
        int cameranodepos = 0;
        int cameranodedir = 0;
        int cameranoderoll = 0;

        if (current_truck->cameranodepos[0] >= 0 && current_truck->cameranodepos[0] < MAX_NODES)
            cameranodepos = current_truck->cameranodepos[0];
        if (current_truck->cameranodedir[0] >= 0 && current_truck->cameranodedir[0] < MAX_NODES)
            cameranodedir = current_truck->cameranodedir[0];
        if (current_truck->cameranoderoll[0] >= 0 && current_truck->cameranoderoll[0] < MAX_NODES)
            cameranoderoll = current_truck->cameranoderoll[0];

        Vector3 udir = current_truck->nodes[cameranodepos].RelPosition - current_truck->nodes[cameranodedir].RelPosition;
        Vector3 uroll = current_truck->nodes[cameranodepos].RelPosition - current_truck->nodes[cameranoderoll].RelPosition;

        udir.normalise();
        uroll.normalise();

        m_force_feedback->SetForces(-current_truck->ffforce.dotProduct(uroll) / 10000.0,
            current_truck->ffforce.dotProduct(udir) / 10000.0,
            current_truck->WheelSpeed,
            current_truck->hydrodircommand,
            current_truck->ffhydro);
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

    auto s = App::GetActiveSimState();
    auto gui_man = App::GetGuiManager();

    RoR::App::GetInputEngine()->updateKeyBounces(dt);
    if (!RoR::App::GetInputEngine()->getInputsChanged())
        return true;

    // update overlays if enabled
    if (RoR::App::GetOverlayWrapper())
        RoR::App::GetOverlayWrapper()->update(dt);

    Beam* curr_truck = BeamFactory::getSingleton().getCurrentTruck();

    if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_QUIT_GAME))
    {
        if (gui_man->IsVisible_MainSelector())
        {
            gui_man->GetMainSelector()->Cancel();
        }
        else if (simRUNNING(s))
        {
            App::SetPendingSimState(App::SIM_STATE_PAUSED);
        }
        else if (simPAUSED(s))
        {
            App::SetPendingSimState(App::SIM_STATE_RUNNING);
        }
    }

    if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_CONSOLE_TOGGLE))
    {
        gui_man->SetVisible_Console(! gui_man->IsVisible_Console());
    }

    if ((curr_truck == nullptr) && RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_TELEPORT_TOGGLE))
    {
        gui_man->SetVisible_TeleportWindow(! gui_man->IsVisible_TeleportWindow());
    }

    if (gui_man->IsVisible_GamePauseMenu())
        return true; //Stop everything when pause menu is visible

    if (gui_man->IsVisible_FrictionSettings() && curr_truck)
    {
        ground_model_t* gm = curr_truck->getLastFuzzyGroundModel();

        gui_man->GetFrictionSettings()->setActiveCol(gm);
    }

    const bool mp_connected = (App::GetActiveMpState() == App::MP_STATE_CONNECTED);
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

        String fn_prefix = App::GetSysScreenshotDir() + PATH_SLASH + String("screenshot_");
        String fn_name = date.str() + String("_");
        String fn_suffix = String(".") + App::GetAppScreenshotFormat();

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
        MyGUI::PointerManager::getInstance().setVisible(false);

        BeamFactory::getSingleton().updateFlexbodiesFinal(); // Waits until all flexbody tasks are finished

        if (App::GetAppScreenshotFormat() == "png")
        {
            // add some more data into the image
            AdvancedScreen* as = new AdvancedScreen(RoR::App::GetOgreSubsystem()->GetRenderWindow(), tmpfn);
            //as->addData("terrain_Name", loadedTerrain);
            //as->addData("terrain_ModHash", terrainModHash);
            //as->addData("terrain_FileHash", terrainFileHash);
            as->addData("Truck_Num", TOSTRING(BeamFactory::getSingleton().getCurrentTruckNumber()));
            if (curr_truck)
            {
                as->addData("Truck_fname", curr_truck->realtruckfilename);
                as->addData("Truck_name", curr_truck->getTruckName());
                as->addData("Truck_beams", TOSTRING(curr_truck->getBeamCount()));
                as->addData("Truck_nodes", TOSTRING(curr_truck->getNodeCount()));
            }
            as->addData("User_NickName", App::GetMpPlayerName());
            as->addData("User_Language", App::GetAppLanguage());
            as->addData("RoR_VersionString", String(ROR_VERSION_STRING));
            as->addData("RoR_ProtocolVersion", String(RORNET_VERSION));
            as->addData("RoR_BinaryHash", "");
            as->addData("MP_ServerName", App::GetMpServerHost());
            as->addData("MP_ServerPort", TOSTRING(App::GetMpServerPort()));
            as->addData("MP_NetworkEnabled", (App::GetActiveMpState() == App::MP_STATE_CONNECTED) ? "Yes" : "No");
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

        MyGUI::PointerManager::getInstance().setVisible(true);

        // show new flash message
        String ssmsg = _L("Screenshot:") + String(" ") + fn_name + fn_suffix;
        LOG(ssmsg);
        RoR::App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE, ssmsg, "camera.png", 10000, false);
        RoR::App::GetGuiManager()->PushNotification("Notice:", ssmsg);
    }

    if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_TRUCKEDIT_RELOAD, 0.5f) && curr_truck)
    {
        this->ReloadCurrentTruck();
        return true;
    }

    // position storage
    if (curr_truck && App::GetSimPositionStorage())
    {
        int res = -10, slot = -1;
        if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_SAVE_POS01, 0.5f))
        {
            slot = 0;
            res = curr_truck->savePosition(slot);
        };
        if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_SAVE_POS02, 0.5f))
        {
            slot = 1;
            res = curr_truck->savePosition(slot);
        };
        if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_SAVE_POS03, 0.5f))
        {
            slot = 2;
            res = curr_truck->savePosition(slot);
        };
        if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_SAVE_POS04, 0.5f))
        {
            slot = 3;
            res = curr_truck->savePosition(slot);
        };
        if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_SAVE_POS05, 0.5f))
        {
            slot = 4;
            res = curr_truck->savePosition(slot);
        };
        if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_SAVE_POS06, 0.5f))
        {
            slot = 5;
            res = curr_truck->savePosition(slot);
        };
        if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_SAVE_POS07, 0.5f))
        {
            slot = 6;
            res = curr_truck->savePosition(slot);
        };
        if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_SAVE_POS08, 0.5f))
        {
            slot = 7;
            res = curr_truck->savePosition(slot);
        };
        if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_SAVE_POS09, 0.5f))
        {
            slot = 8;
            res = curr_truck->savePosition(slot);
        };
        if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_SAVE_POS10, 0.5f))
        {
            slot = 9;
            res = curr_truck->savePosition(slot);
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
                res = curr_truck->loadPosition(slot);
            };
            if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_LOAD_POS02, 0.5f))
            {
                slot = 1;
                res = curr_truck->loadPosition(slot);
            };
            if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_LOAD_POS03, 0.5f))
            {
                slot = 2;
                res = curr_truck->loadPosition(slot);
            };
            if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_LOAD_POS04, 0.5f))
            {
                slot = 3;
                res = curr_truck->loadPosition(slot);
            };
            if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_LOAD_POS05, 0.5f))
            {
                slot = 4;
                res = curr_truck->loadPosition(slot);
            };
            if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_LOAD_POS06, 0.5f))
            {
                slot = 5;
                res = curr_truck->loadPosition(slot);
            };
            if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_LOAD_POS07, 0.5f))
            {
                slot = 6;
                res = curr_truck->loadPosition(slot);
            };
            if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_LOAD_POS08, 0.5f))
            {
                slot = 7;
                res = curr_truck->loadPosition(slot);
            };
            if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_LOAD_POS09, 0.5f))
            {
                slot = 8;
                res = curr_truck->loadPosition(slot);
            };
            if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_LOAD_POS10, 0.5f))
            {
                slot = 9;
                res = curr_truck->loadPosition(slot);
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
                App::SetGfxFovInternal(fov);
            }
            else
            {
                App::SetGfxFovExternal(fov);
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

    static std::vector<TerrainObjectManager::object_t> object_list;
    static bool terrain_editing_track_object = true;
    static bool terrain_editing_mode = false;
    static int terrain_editing_rotation_axis = 1;
    static int object_index = -1;

    if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_TOGGLE_TERRAIN_EDITOR))
    {
        terrain_editing_mode = !terrain_editing_mode;
        App::SetActiveSimState(terrain_editing_mode ? App::SIM_STATE_EDITOR_MODE : App::SIM_STATE_RUNNING);
        UTFString ssmsg = terrain_editing_mode ? _L("Entered terrain editing mode") : _L("Left terrain editing mode");
        RoR::App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE, ssmsg, "infromation.png", 2000, false);
        RoR::App::GetGuiManager()->PushNotification("Notice:", ssmsg);

        if (terrain_editing_mode)
        {
            object_list = gEnv->terrainManager->getObjectManager()->getObjects();
            object_index = -1;
        }
        else
        {
            std::string path = App::GetSysConfigDir() + PATH_SLASH + "editor_out.cfg";
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
            CharacterFactory::getSingleton().update(dt);
        }
    }
    else if (simRUNNING(s) || simPAUSED(s))
    //else if (m_loading_state == ALL_LOADED)
    {
        CharacterFactory::getSingleton().update(dt);
        if (gEnv->cameraManager && !gEnv->cameraManager->gameControlsLocked())
        {
            if (!curr_truck)
            {
                if (gEnv->player)
                {
                    if (!App::GetGuiManager()->IsVisible_GamePauseMenu())
                        gEnv->player->setPhysicsEnabled(true);
                }
            }
            else // we are in a vehicle
            {
                if (RoR::App::GetInputEngine()->getEventBoolValue(EV_COMMON_RESET_TRUCK) && !curr_truck->replaymode)
                {
                    this->StopRaceTimer();
                    curr_truck->reset();
                }
                else if (RoR::App::GetInputEngine()->getEventBoolValue(EV_COMMON_REMOVE_CURRENT_TRUCK) && !curr_truck->replaymode)
                {
                    this->StopRaceTimer();
                    Vector3 center = curr_truck->getRotationCenter();
                    BeamFactory::getSingleton().removeCurrentTruck();
                    gEnv->player->setPosition(center);
                }
                else if ((RoR::App::GetInputEngine()->getEventBoolValue(EV_COMMON_REPAIR_TRUCK) || m_advanced_truck_repair) && !curr_truck->replaymode)
                {
                    this->StopRaceTimer();
                    if (RoR::App::GetInputEngine()->getEventBoolValue(EV_COMMON_REPAIR_TRUCK))
                    {
                        m_advanced_truck_repair = m_advanced_truck_repair_timer > 1.0f;
                    }
                    else
                    {
                        m_advanced_truck_repair_timer = 0.0f;
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
                        float curRot = curr_truck->getRotation();
                        translation.x += 2.0f * cos(curRot - Ogre::Math::HALF_PI) * dt;
                        translation.z += 2.0f * sin(curRot - Ogre::Math::HALF_PI) * dt;
                    }
                    else if (RoR::App::GetInputEngine()->getEventBoolValue(EV_CHARACTER_BACKWARDS))
                    {
                        float curRot = curr_truck->getRotation();
                        translation.x -= 2.0f * cos(curRot - Ogre::Math::HALF_PI) * dt;
                        translation.z -= 2.0f * sin(curRot - Ogre::Math::HALF_PI) * dt;
                    }
                    if (RoR::App::GetInputEngine()->getEventBoolValue(EV_CHARACTER_SIDESTEP_RIGHT))
                    {
                        float curRot = curr_truck->getRotation();
                        translation.x += 2.0f * cos(curRot) * dt;
                        translation.z += 2.0f * sin(curRot) * dt;
                    }
                    else if (RoR::App::GetInputEngine()->getEventBoolValue(EV_CHARACTER_SIDESTEP_LEFT))
                    {
                        float curRot = curr_truck->getRotation();
                        translation.x -= 2.0f * cos(curRot) * dt;
                        translation.z -= 2.0f * sin(curRot) * dt;
                    }

                    if (translation != Vector3::ZERO || rotation != 0.0f)
                    {
                        float scale = RoR::App::GetInputEngine()->isKeyDown(OIS::KC_LMENU) ? 0.1f : 1.0f;
                        scale *= RoR::App::GetInputEngine()->isKeyDown(OIS::KC_LSHIFT) ? 3.0f : 1.0f;
                        scale *= RoR::App::GetInputEngine()->isKeyDown(OIS::KC_LCONTROL) ? 10.0f : 1.0f;

                        curr_truck->updateFlexbodiesFinal();
                        curr_truck->displace(translation * scale, rotation * scale);

                        m_advanced_truck_repair_timer = 0.0f;
                    }
                    else
                    {
                        m_advanced_truck_repair_timer += dt;
                    }

                    curr_truck->reset(true);
                }
                else
                {
                    // get commands
                    // -- here we should define a maximum numbers per trucks. Some trucks does not have that much commands
                    // -- available, so why should we iterate till MAX_COMMANDS?
                    for (int i = 1; i <= MAX_COMMANDS + 1; i++)
                    {
                        int eventID = EV_COMMANDS_01 + (i - 1);

                        curr_truck->commandkey[i].playerInputValue = RoR::App::GetInputEngine()->getEventValue(eventID);
                    }

                    if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_TOGGLE_FORWARDCOMMANDS))
                    {
                        curr_truck->forwardcommands = !curr_truck->forwardcommands;
                        if (curr_truck->forwardcommands)
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
                        curr_truck->importcommands = !curr_truck->importcommands;
                        if (curr_truck->importcommands)
                        {
                            RoR::App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE, _L("importcommands enabled"), "information.png", 3000);
                        }
                        else
                        {
                            RoR::App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE, _L("importcommands disabled"), "information.png", 3000);
                        }
                    }
                    // replay mode
                    if (curr_truck->replaymode)
                    {
                        if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_REPLAY_FORWARD, 0.1f) && curr_truck->replaypos <= 0)
                        {
                            curr_truck->replaypos++;
                        }
                        if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_REPLAY_BACKWARD, 0.1f) && curr_truck->replaypos > -curr_truck->replaylen)
                        {
                            curr_truck->replaypos--;
                        }
                        if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_REPLAY_FAST_FORWARD, 0.1f) && curr_truck->replaypos + 10 <= 0)
                        {
                            curr_truck->replaypos += 10;
                        }
                        if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_REPLAY_FAST_BACKWARD, 0.1f) && curr_truck->replaypos - 10 > -curr_truck->replaylen)
                        {
                            curr_truck->replaypos -= 10;
                        }
                        if (RoR::App::GetInputEngine()->isKeyDown(OIS::KC_LMENU) && RoR::App::GetInputEngine()->isKeyDown(OIS::KC_V))
                        {
                        }

                        if (RoR::App::GetInputEngine()->isKeyDown(OIS::KC_LMENU))
                        {
                            if (curr_truck->replaypos <= 0 && curr_truck->replaypos >= -curr_truck->replaylen)
                            {
                                if (RoR::App::GetInputEngine()->isKeyDown(OIS::KC_LSHIFT) || RoR::App::GetInputEngine()->isKeyDown(OIS::KC_RSHIFT))
                                {
                                    curr_truck->replaypos += RoR::App::GetInputEngine()->getMouseState().X.rel * 1.5f;
                                }
                                else
                                {
                                    curr_truck->replaypos += RoR::App::GetInputEngine()->getMouseState().X.rel * 0.05f;
                                }
                                if (curr_truck->replaypos > 0)
                                {
                                    curr_truck->replaypos = 0;
                                }
                                if (curr_truck->replaypos < -curr_truck->replaylen)
                                {
                                    curr_truck->replaypos = -curr_truck->replaylen;
                                }
                            }
                        }
                    }

                    if (curr_truck->driveable == TRUCK)
                    {
                        LandVehicleSimulation::UpdateVehicle(curr_truck, dt);
                    }
                    if (curr_truck->driveable == AIRPLANE)
                    {
                        AircraftSimulation::UpdateVehicle(curr_truck, dt);
                    }
                    if (curr_truck->driveable == BOAT)
                    {
                        //BOAT SPECIFICS

                        //throttle
                        if (RoR::App::GetInputEngine()->isEventDefined(EV_BOAT_THROTTLE_AXIS))
                        {
                            float f = RoR::App::GetInputEngine()->getEventValue(EV_BOAT_THROTTLE_AXIS);
                            // use negative values also!
                            f = f * 2 - 1;
                            for (int i = 0; i < curr_truck->free_screwprop; i++)
                                curr_truck->screwprops[i]->setThrottle(-f);
                        }
                        if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_BOAT_THROTTLE_DOWN, 0.1f))
                        {
                            //throttle down
                            for (int i = 0; i < curr_truck->free_screwprop; i++)
                                curr_truck->screwprops[i]->setThrottle(curr_truck->screwprops[i]->getThrottle() - 0.05);
                        }
                        if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_BOAT_THROTTLE_UP, 0.1f))
                        {
                            //throttle up
                            for (int i = 0; i < curr_truck->free_screwprop; i++)
                                curr_truck->screwprops[i]->setThrottle(curr_truck->screwprops[i]->getThrottle() + 0.05);
                        }

                        // steer
                        float tmp_steer_left = RoR::App::GetInputEngine()->getEventValue(EV_BOAT_STEER_LEFT);
                        float tmp_steer_right = RoR::App::GetInputEngine()->getEventValue(EV_BOAT_STEER_RIGHT);
                        float stime = RoR::App::GetInputEngine()->getEventBounceTime(EV_BOAT_STEER_LEFT) + RoR::App::GetInputEngine()->getEventBounceTime(EV_BOAT_STEER_RIGHT);
                        float sum_steer = (tmp_steer_left - tmp_steer_right) * dt;
                        // do not center the rudder!
                        if (fabs(sum_steer) > 0 && stime <= 0)
                        {
                            for (int i = 0; i < curr_truck->free_screwprop; i++)
                                curr_truck->screwprops[i]->setRudder(curr_truck->screwprops[i]->getRudder() + sum_steer);
                        }
                        if (RoR::App::GetInputEngine()->isEventDefined(EV_BOAT_STEER_LEFT_AXIS) && RoR::App::GetInputEngine()->isEventDefined(EV_BOAT_STEER_RIGHT_AXIS))
                        {
                            tmp_steer_left = RoR::App::GetInputEngine()->getEventValue(EV_BOAT_STEER_LEFT_AXIS);
                            tmp_steer_right = RoR::App::GetInputEngine()->getEventValue(EV_BOAT_STEER_RIGHT_AXIS);
                            sum_steer = (tmp_steer_left - tmp_steer_right);
                            for (int i = 0; i < curr_truck->free_screwprop; i++)
                                curr_truck->screwprops[i]->setRudder(sum_steer);
                        }
                        if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_BOAT_CENTER_RUDDER, 0.1f))
                        {
                            for (int i = 0; i < curr_truck->free_screwprop; i++)
                                curr_truck->screwprops[i]->setRudder(0);
                        }

                        if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_BOAT_REVERSE))
                        {
                            for (int i = 0; i < curr_truck->free_screwprop; i++)
                                curr_truck->screwprops[i]->toggleReverse();
                        }
                    }
                    //COMMON KEYS

                    if (RoR::App::GetInputEngine()->getEventBoolValue(EV_COMMON_ACCELERATE_SIMULATION))
                    {
                        float simulation_speed = BeamFactory::getSingleton().getSimulationSpeed() * pow(2.0f, dt / 2.0f);
                        BeamFactory::getSingleton().setSimulationSpeed(simulation_speed);
                        String ssmsg = _L("New simulation speed: ") + TOSTRING(Round(simulation_speed * 100.0f, 1)) + "%";
                        RoR::App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE, ssmsg, "infromation.png", 2000, false);
                        RoR::App::GetGuiManager()->PushNotification("Notice:", ssmsg);
                    }
                    if (RoR::App::GetInputEngine()->getEventBoolValue(EV_COMMON_DECELERATE_SIMULATION))
                    {
                        float simulation_speed = BeamFactory::getSingleton().getSimulationSpeed() * pow(0.5f, dt / 2.0f);
                        BeamFactory::getSingleton().setSimulationSpeed(simulation_speed);
                        String ssmsg = _L("New simulation speed: ") + TOSTRING(Round(simulation_speed * 100.0f, 1)) + "%";
                        RoR::App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE, ssmsg, "infromation.png", 2000, false);
                        RoR::App::GetGuiManager()->PushNotification("Notice:", ssmsg);
                    }
                    if (RoR::App::GetInputEngine()->getEventBoolValue(EV_COMMON_RESET_SIMULATION_PACE))
                    {
                        if (!m_is_pace_reset_pressed)
                        {
                            float simulation_speed = BeamFactory::getSingleton().getSimulationSpeed();
                            if (simulation_speed != 1.0f)
                            {
                                m_last_simulation_speed = simulation_speed;
                                BeamFactory::getSingleton().setSimulationSpeed(1.0f);
                                UTFString ssmsg = _L("Simulation speed reset.");
                                RoR::App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE, ssmsg, "infromation.png", 2000, false);
                                RoR::App::GetGuiManager()->PushNotification("Notice:", ssmsg);
                            }
                            else if (m_last_simulation_speed != 1.0f)
                            {
                                BeamFactory::getSingleton().setSimulationSpeed(m_last_simulation_speed);
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
                        BeamFactory::getSingleton().removeCurrentTruck();
                    }
                    if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_ROPELOCK))
                    {
                        curr_truck->ropeToggle(-1);
                    }
                    if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_LOCK))
                    {
                        curr_truck->hookToggle(-1, HOOK_TOGGLE, -1);
                        //SlideNodeLock
                        curr_truck->toggleSlideNodeLock();
                    }
                    if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_AUTOLOCK))
                    {
                        //unlock all autolocks
                        curr_truck->hookToggle(-2, HOOK_UNLOCK, -1);
                    }
                    //strap
                    if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_SECURE_LOAD))
                    {
                        curr_truck->tieToggle(-1);
                    }

                    //replay mode
                    if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_TOGGLE_REPLAY_MODE))
                    {
                        this->StopRaceTimer();
                        curr_truck->setReplayMode(!curr_truck->replaymode);
                    }

                    if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_TOGGLE_CUSTOM_PARTICLES))
                    {
                        curr_truck->toggleCustomParticles();
                    }

                    if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_SHOW_SKELETON))
                    {
                        if (curr_truck->m_skeletonview_is_active)
                            curr_truck->hideSkeleton();
                        else
                            curr_truck->showSkeleton(true);
                    }

                    if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_TOGGLE_TRUCK_LIGHTS))
                    {
                        curr_truck->lightsToggle();
                    }

                    if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_TOGGLE_TRUCK_BEACONS))
                    {
                        curr_truck->beaconsToggle();
                    }

                    //camera mode
                    if (RoR::App::GetInputEngine()->getEventBoolValue(EV_COMMON_PRESSURE_LESS))
                    {
                        if (m_pressure_pressed = curr_truck->addPressure(dt * -10.0))
                        {
                            if (RoR::App::GetOverlayWrapper())
                                RoR::App::GetOverlayWrapper()->showPressureOverlay(true);
#ifdef USE_OPENAL
                            SoundScriptManager::getSingleton().trigStart(curr_truck, SS_TRIG_AIR);
#endif // OPENAL
                        }
                    }
                    else if (RoR::App::GetInputEngine()->getEventBoolValue(EV_COMMON_PRESSURE_MORE))
                    {
                        if (m_pressure_pressed = curr_truck->addPressure(dt * 10.0))
                        {
                            if (RoR::App::GetOverlayWrapper())
                                RoR::App::GetOverlayWrapper()->showPressureOverlay(true);
#ifdef USE_OPENAL
                            SoundScriptManager::getSingleton().trigStart(curr_truck, SS_TRIG_AIR);
#endif // OPENAL
                        }
                    }
                    else if (m_pressure_pressed)
                    {
#ifdef USE_OPENAL
                        SoundScriptManager::getSingleton().trigStop(curr_truck, SS_TRIG_AIR);
#endif // OPENAL
                        m_pressure_pressed = false;
                        if (RoR::App::GetOverlayWrapper())
                            RoR::App::GetOverlayWrapper()->showPressureOverlay(false);
                    }

                    if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_RESCUE_TRUCK, 0.5f) && !mp_connected && curr_truck->driveable != AIRPLANE)
                    {
                        if (!BeamFactory::getSingleton().enterRescueTruck())
                        {
                            RoR::App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE, _L("No rescue truck found!"), "warning.png");
                            RoR::App::GetGuiManager()->PushNotification("Notice:", _L("No rescue truck found!"));
                        }
                    }

                    if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_TOGGLE_VIDEOCAMERA, 0.5f))
                    {
                        curr_truck->m_is_videocamera_disabled = !curr_truck->m_is_videocamera_disabled;
                        LOG("m_is_videocamera_disabled: " + TOSTRING(curr_truck->m_is_videocamera_disabled));
                    }

                    if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_BLINK_LEFT))
                    {
                        if (curr_truck->getBlinkType() == BLINK_LEFT)
                            curr_truck->setBlinkType(BLINK_NONE);
                        else
                            curr_truck->setBlinkType(BLINK_LEFT);
                    }

                    if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_BLINK_RIGHT))
                    {
                        if (curr_truck->getBlinkType() == BLINK_RIGHT)
                            curr_truck->setBlinkType(BLINK_NONE);
                        else
                            curr_truck->setBlinkType(BLINK_RIGHT);
                    }

                    if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_BLINK_WARN))
                    {
                        if (curr_truck->getBlinkType() == BLINK_WARN)
                            curr_truck->setBlinkType(BLINK_NONE);
                        else
                            curr_truck->setBlinkType(BLINK_WARN);
                    }
                }
            } // end of truck!=-1
        }

#ifdef USE_CAELUM

        static const bool caelum_enabled = App::GetGfxSkyMode() == 1;
        if (caelum_enabled && (simRUNNING(s) || simPAUSED(s) || simEDITOR(s)))
        {
            Real time_factor = 1000.0f;
            Real multiplier = 10;
            bool update_time = false;

            if (RoR::App::GetInputEngine()->getEventBoolValue(EV_SKY_INCREASE_TIME) && gEnv->terrainManager->getSkyManager())
            {
                update_time = true;
            }
            else if (RoR::App::GetInputEngine()->getEventBoolValue(EV_SKY_INCREASE_TIME_FAST) && gEnv->terrainManager->getSkyManager())
            {
                time_factor *= multiplier;
                update_time = true;
            }
            else if (RoR::App::GetInputEngine()->getEventBoolValue(EV_SKY_DECREASE_TIME) && gEnv->terrainManager->getSkyManager())
            {
                time_factor = -time_factor;
                update_time = true;
            }
            else if (RoR::App::GetInputEngine()->getEventBoolValue(EV_SKY_DECREASE_TIME_FAST) && gEnv->terrainManager->getSkyManager())
            {
                time_factor *= -multiplier;
                update_time = true;
            }
            else
            {
                time_factor = 1.0f;
                update_time = gEnv->terrainManager->getSkyManager()->getTimeFactor() != 1.0f;
            }

            if (update_time)
            {
                gEnv->terrainManager->getSkyManager()->setTimeFactor(time_factor);
                RoR::App::GetGuiManager()->PushNotification("Notice:", _L("Time set to ") + gEnv->terrainManager->getSkyManager()->getPrettyTime());
                RoR::App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE, _L("Time set to ") + gEnv->terrainManager->getSkyManager()->getPrettyTime(), "weather_sun.png", 1000);
            }
        }

#endif // USE_CAELUM

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
            m_time_until_next_toggle = 0.5; // Some delay before trying to re-enter(exit) truck
            // perso in/out
            int current_truck = BeamFactory::getSingleton().getCurrentTruckNumber();
            int free_truck = BeamFactory::getSingleton().getTruckCount();
            Beam** trucks = BeamFactory::getSingleton().getTrucks();
            if (current_truck == -1)
            {
                // find the nearest truck
                float mindist = 1000.0;
                int minindex = -1;
                for (int i = 0; i < free_truck; i++)
                {
                    if (!trucks[i])
                        continue;
                    if (!trucks[i]->driveable)
                        continue;
                    if (trucks[i]->cinecameranodepos[0] == -1)
                    {
                        LOG("cinecam missing, cannot enter truck!");
                        continue;
                    }
                    float len = 0.0f;
                    if (gEnv->player)
                    {
                        len = trucks[i]->nodes[trucks[i]->cinecameranodepos[0]].AbsPosition.distance(gEnv->player->getPosition() + Vector3(0.0, 2.0, 0.0));
                    }
                    if (len < mindist)
                    {
                        mindist = len;
                        minindex = i;
                    }
                }
                if (mindist < 20.0)
                {
                    BeamFactory::getSingleton().setCurrentTruck(minindex);
                }
            }
            else if (curr_truck->nodes[0].Velocity.length() < 1.0f)
            {
                BeamFactory::getSingleton().setCurrentTruck(-1);
            }
            else
            {
                curr_truck->brake = curr_truck->brakeforce * 0.66;
                m_time_until_next_toggle = 0.0; // No delay in this case: the truck must brake like braking normally
            }
        }
        else if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_ENTER_NEXT_TRUCK, 0.25f))
        {
            BeamFactory::getSingleton().enterNextTruck();
        }
        else if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_ENTER_PREVIOUS_TRUCK, 0.25f))
        {
            BeamFactory::getSingleton().enterPreviousTruck();
        }
        else if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_RESPAWN_LAST_TRUCK, 0.25f))
        {
            if (m_last_cache_selection != nullptr)
            {
                /* We load an extra truck */
                std::vector<String>* config_ptr = nullptr;
                if (m_last_vehicle_configs.size() > 0)
                {
                    config_ptr = & m_last_vehicle_configs;
                }

                Beam* current_truck = BeamFactory::getSingleton().getCurrentTruck();
                if (current_truck != nullptr)
                {
                    m_reload_dir = Quaternion(Degree(270) - Radian(current_truck->getRotation()), Vector3::UNIT_Y);
                    m_reload_pos = current_truck->getRotationCenter();

                    // TODO: Fix this by projecting m_reload_pos onto the terrain / mesh
                    m_reload_pos.y = current_truck->nodes[current_truck->lowestcontactingnode].AbsPosition.y;
                }
                else
                {
                    m_reload_dir = Quaternion(Degree(180) - gEnv->player->getRotation(), Vector3::UNIT_Y);
                    m_reload_pos = gEnv->player->getPosition();
                }

                Beam* local_truck = BeamFactory::getSingleton().CreateLocalRigInstance(m_reload_pos, m_reload_dir, m_last_cache_selection->fname, m_last_cache_selection->number, 0, false, config_ptr, m_last_skin_selection);

                this->FinalizeTruckSpawning(local_truck, current_truck);
            }
        }
    }
    else
    {
        //no terrain or truck loaded
        terrain_editing_mode = false;

        if (App::GetGuiManager()->GetMainSelector()->IsFinishedSelecting())
        {
            if (simSELECT(s))
            {
                CacheEntry* selection = App::GetGuiManager()->GetMainSelector()->GetSelectedEntry();
                Skin* skin = App::GetGuiManager()->GetMainSelector()->GetSelectedSkin();
                if (selection != nullptr)
                {
                    /* We load an extra truck */
                    std::vector<String>* config_ptr = nullptr;
                    std::vector<String> config = App::GetGuiManager()->GetMainSelector()->GetVehicleConfigs();
                    if (config.size() > 0)
                    {
                        config_ptr = & config;
                    }

                    m_last_cache_selection = selection;
                    m_last_skin_selection = skin;
                    m_last_vehicle_configs = config;

                    Beam* current_truck = BeamFactory::getSingleton().getCurrentTruck();

                    if (m_reload_box == nullptr)
                    {
                        if (current_truck != nullptr)
                        {
                            float rotation = current_truck->getRotation() - Math::HALF_PI;
                            m_reload_dir = Quaternion(Degree(180) - Radian(rotation), Vector3::UNIT_Y);
                            m_reload_pos = current_truck->getRotationCenter();
                            // TODO: Fix this by projecting m_reload_pos onto the terrain / mesh
                            m_reload_pos.y = current_truck->nodes[current_truck->lowestcontactingnode].AbsPosition.y;
                        }
                        else
                        {
                            m_reload_dir = Quaternion(Degree(180) - gEnv->player->getRotation(), Vector3::UNIT_Y);
                            m_reload_pos = gEnv->player->getPosition();
                        }
                    }

                    Beam* local_truck = BeamFactory::getSingleton().CreateLocalRigInstance(m_reload_pos, m_reload_dir, selection->fname, selection->number, m_reload_box, false, config_ptr, skin);

                    this->FinalizeTruckSpawning(local_truck, current_truck);
                }
                else if (gEnv->player)
                {
                    gEnv->player->unwindMovement(0.1f);
                }

                m_reload_box = 0;
            }
            App::GetGuiManager()->GetMainSelector()->Hide();
            RoR::App::GetGuiManager()->UnfocusGui();
            App::SetActiveSimState(App::SIM_STATE_RUNNING); // TODO: use pending mechanism
        }
    }

    if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_GET_NEW_VEHICLE))
    {
        if ((simRUNNING(s) || simPAUSED(s) || simEDITOR(s)) && gEnv->player != nullptr)
        {
            App::SetActiveSimState(App::SIM_STATE_SELECTING); // TODO: use pending mechanism

            App::GetGuiManager()->GetMainSelector()->Show(LT_AllBeam);
        }
    }

    if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_TRUCK_INFO) && curr_truck)
    {
        m_truck_info_on = ! m_truck_info_on;
        gui_man->GetSimUtils()->SetTruckInfoBoxVisible(m_truck_info_on);
    }

    if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_TRUCK_DESCRIPTION) && curr_truck)
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
        if (BeamFactory::getSingleton().getCurrentTruckNumber() == -1)
        {
            if (gEnv->player)
            {
                position = gEnv->player->getPosition();
                rotation = gEnv->player->getRotation() + Radian(Math::PI);
            }
        }
        else
        {
            position = curr_truck->getPosition();
            Vector3 idir = curr_truck->getDirection();
            rotation = atan2(idir.dotProduct(Vector3::UNIT_X), idir.dotProduct(-Vector3::UNIT_Z));
        }
        LOG("Position: " + TOSTRING(position.x) + ", "+ TOSTRING(position.y) + ", " + TOSTRING(position.z) + ", 0, " + TOSTRING(rotation.valueDegrees()) + ", 0");
    }

    // Return true to continue rendering
    return true;
}

void RoRFrameListener::TeleportPlayer(RoR::Terrn2Telepoint* telepoint)
{
    if (BeamFactory::getSingleton().getCurrentTruck() != nullptr)
        return; // Player could enter truck while Teleport-GUI is visible

    gEnv->player->setPosition(telepoint->position);
}

void RoRFrameListener::TeleportPlayerXZ(float x, float z)
{
    if (BeamFactory::getSingleton().getCurrentTruck() != nullptr)
        return; // Player could enter truck while Teleport-GUI is visible

    float y = gEnv->terrainManager->getHeightFinder()->getHeightAt(x, z);
    gEnv->player->setPosition(Ogre::Vector3(x, y, z));
}

void RoRFrameListener::FinalizeTruckSpawning(Beam* local_truck, Beam* previous_truck)
{
    if (local_truck != nullptr)
    {
        if (m_reload_box == nullptr)
        {
            // Calculate translational offset for node[0] to align the trucks rotation center with m_reload_pos
            Vector3 translation = m_reload_pos - local_truck->getRotationCenter();
            local_truck->resetPosition(local_truck->nodes[0].AbsPosition + Vector3(translation.x, 0.0f, translation.z), true);

            if (local_truck->driveable != NOT_DRIVEABLE || (previous_truck && previous_truck->driveable != NOT_DRIVEABLE))
            {
                // Try to resolve collisions with other trucks
                local_truck->resolveCollisions(50.0f, previous_truck == nullptr);
            }
        }

        if (gEnv->surveyMap)
        {
            gEnv->surveyMap->createNamedMapEntity("Truck" + TOSTRING(local_truck->trucknum), SurveyMapManager::getTypeByDriveable(local_truck->driveable));
        }

        if (local_truck->driveable != NOT_DRIVEABLE)
        {
            /* We are supposed to be in this truck, if it is a truck */
            if (local_truck->engine != nullptr)
            {
                local_truck->engine->start();
            }
            BeamFactory::getSingleton().setCurrentTruck(local_truck->trucknum);
        }

        local_truck->updateFlexbodiesPrepare();
        local_truck->updateFlexbodiesFinal();
        local_truck->updateVisual();
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

    BeamFactory::getSingleton().SyncWithSimThread();

    const bool mp_connected = (App::GetActiveMpState() == App::MP_STATE_CONNECTED);
#ifdef USE_SOCKETW
    if (mp_connected)
    {
        RoR::Networking::HandleStreamData();
        m_netcheck_gui_timer += dt;
        if (m_netcheck_gui_timer > 2.0f)
        {
            App::GetGuiManager()->GetMpClientList()->update();
            m_netcheck_gui_timer = 0.0f;
        }
    }
#endif //SOCKETW

    RoR::App::GetInputEngine()->Capture();
    const bool is_altkey_pressed =  App::GetInputEngine()->isKeyDown(OIS::KeyCode::KC_LMENU) || App::GetInputEngine()->isKeyDown(OIS::KeyCode::KC_RMENU);
    auto s = App::GetActiveSimState();

    //if (gEnv->collisions) 	printf("> ground model used: %s\n", gEnv->collisions->last_used_ground_model->name);
    //
    if ((simRUNNING(s) || simEDITOR(s)) && !simPAUSED(s))
    {
        BeamFactory::getSingleton().updateFlexbodiesPrepare(); // Pushes all flexbody tasks into the thread pool
    }

    if (OutProtocol::getSingletonPtr())
    {
        OutProtocol::getSingleton().update(dt);
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

            SoundScriptManager::getSingleton().GetMumble()->update(gEnv->mainCamera->getPosition(), gEnv->mainCamera->getDirection(), gEnv->mainCamera->getUp(),
                gEnv->player->getPosition() + Vector3(0, 1.8f, 0), avatarDir, Ogre::Vector3(0.0f, 1.0f, 0.0f));
        }
#endif // USE_MUMBLE
    }

    if (simRUNNING(s) || simPAUSED(s) || simEDITOR(s))
    {
        if (gEnv->cameraManager != nullptr)
        {
            gEnv->cameraManager->update(dt);
        }
        if (gEnv->surveyMap != nullptr)
        {
            gEnv->surveyMap->update(dt);
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

    Beam* curr_truck = BeamFactory::getSingleton().getCurrentTruck();

    if (curr_truck)
    {
        curr_truck->updateVideocameras(dt);
    }

    // terrain updates
    if (gEnv->terrainManager)
    {
        // update animated objects
        gEnv->terrainManager->update(dt);

        // env map update
        if (gEnv->terrainManager->getEnvmap())
        {
            if (curr_truck)
            {
                gEnv->terrainManager->getEnvmap()->update(curr_truck->getPosition(), curr_truck);
            }
            else
            {
                Vector3 maxTerrainSize = gEnv->terrainManager->getMaxTerrainSize();
                float height = maxTerrainSize.y;
                if (gEnv->terrainManager->getHeightFinder())
                {
                    height = gEnv->terrainManager->getHeightFinder()->getHeightAt(maxTerrainSize.x / 2.0f, maxTerrainSize.z / 2.0f);
                }
                gEnv->terrainManager->getEnvmap()->update(Vector3(maxTerrainSize.x / 2.0f, height + 50.0f, maxTerrainSize.z / 2.0f));
            }
        }

        // water
        if (simRUNNING(s) || simPAUSED(s) || simEDITOR(s))
        {
            IWater* water = gEnv->terrainManager->getWater();
            if (water)
            {
                water->setCamera(gEnv->mainCamera);
                if (curr_truck)
                {
                    water->moveTo(water->getHeightWaves(curr_truck->getPosition()));
                }
                else
                {
                    water->moveTo(water->getHeight());
                }
                water->framestep(dt);
            }
        }

        // trigger updating of shadows etc
#ifdef USE_CAELUM
        SkyManager* sky = gEnv->terrainManager->getSkyManager();
        if ((sky != nullptr) && (simRUNNING(s) || simPAUSED(s) || simEDITOR(s)))
        {
            sky->detectUpdate();
        }
#endif
    }

    if (simRUNNING(s) || simPAUSED(s) || simEDITOR(s))
    {
        BeamFactory::getSingleton().GetParticleManager().update();

        if (m_heathaze)
            m_heathaze->update();
    }

    if ((simRUNNING(s) || simEDITOR(s)) && !simPAUSED(s))
    {
        BeamFactory::getSingleton().updateVisual(dt); // update visual - antishaking
    }

    if (! this->UpdateInputEvents(dt))
    {
        LOG("exiting...");
        return false;
    }
    // update 'curr_truck', since 'updateEvents' might have changed it
    curr_truck = BeamFactory::getSingleton().getCurrentTruck();

    // update gui 3d arrow
    if (RoR::App::GetOverlayWrapper() && m_is_dir_arrow_visible && (simRUNNING(s) || simPAUSED(s) || simEDITOR(s)))
    {
        RoR::App::GetOverlayWrapper()->UpdateDirectionArrow(curr_truck, m_dir_arrow_pointed);
    }

    // one of the input modes is immediate, so setup what is needed for immediate mouse/key movement
    if (m_time_until_next_toggle >= 0)
    {
        m_time_until_next_toggle -= dt;
    }

    RoR::App::GetGuiManager()->framestep(dt);

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
                Beam** vehicles = BeamFactory::getSingleton().getTrucks();
                int num_vehicles = BeamFactory::getSingleton().getTruckCount();

                gEnv->surveyMap->Update(vehicles, num_vehicles);
            }


            if (curr_truck != nullptr)
            {
                // update the truck info gui (also if not displayed!)
                RoR::App::GetOverlayWrapper()->truckhud->update(dt, curr_truck, m_truck_info_on);
#ifdef FEAT_TIMING
				BES.updateGUI(dt);
#endif // FEAT_TIMING

                // update mouse picking lines, etc
                RoR::App::GetSceneMouse()->update(dt);

                if (m_pressure_pressed)
                {
                    RoR::App::GetOverlayWrapper()->UpdatePressureTexture(curr_truck->getPressure());
                }

                if (m_race_in_progress && !RoR::App::GetGuiManager()->IsVisible_GamePauseMenu())
                {
                    UpdateRacingGui(); //I really think that this should stay here.
                }

                if (curr_truck->driveable == TRUCK && curr_truck->engine != nullptr)
                {
                    RoR::App::GetOverlayWrapper()->UpdateLandVehicleHUD(curr_truck);
                }
                else if (curr_truck->driveable == AIRPLANE)
                {
                    RoR::App::GetOverlayWrapper()->UpdateAerialHUD(curr_truck);
                }
            }
            RoR::App::GetGuiManager()->UpdateSimUtils(dt, curr_truck);
        }

        if (!simPAUSED(s))
        {
            BeamFactory::getSingleton().joinFlexbodyTasks(); // Waits until all flexbody tasks are finished
            BeamFactory::getSingleton().update(dt);
            BeamFactory::getSingleton().updateFlexbodiesFinal(); // Updates the harware buffers
        }

        if (simRUNNING(s) && (App::GetPendingSimState() == App::SIM_STATE_PAUSED))
        {
            App::GetGuiManager()->SetVisible_GamePauseMenu(true);
            BeamFactory::getSingleton().MuteAllTrucks();
            gEnv->player->setPhysicsEnabled(false);

            App::SetActiveSimState(App::SIM_STATE_PAUSED);
            App::SetPendingSimState(App::SIM_STATE_NONE);
        }
        else if (simPAUSED(s) && (App::GetPendingSimState() == App::SIM_STATE_RUNNING))
        {
            App::GetGuiManager()->SetVisible_GamePauseMenu(false);
            BeamFactory::getSingleton().UnmuteAllTrucks();
            if (gEnv->player->getVisible() && !gEnv->player->getBeamCoupling())
            {
                gEnv->player->setPhysicsEnabled(true);
            }

            App::SetActiveSimState(App::SIM_STATE_RUNNING);
            App::SetPendingSimState(App::SIM_STATE_NONE);
        }
    }

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
    int free_truck = BeamFactory::getSingleton().getTruckCount();
    Beam** trucks = BeamFactory::getSingleton().getTrucks();

    // first, test if the place if clear, BUT NOT IN MULTIPLAYER
    if (!(App::GetActiveMpState() == App::MP_STATE_CONNECTED))
    {
        collision_box_t* spawnbox = gEnv->collisions->getBox(instance, box);
        for (int t = 0; t < free_truck; t++)
        {
            if (!trucks[t])
                continue;
            for (int i = 0; i < trucks[t]->free_node; i++)
            {
                if (gEnv->collisions->isInside(trucks[t]->nodes[i].AbsPosition, spawnbox))
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
    App::SetActiveSimState(App::SIM_STATE_SELECTING); // TODO: use 'pending' mechanism
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
    LOG("*** windowResized");

    if (RoR::App::GetOverlayWrapper())
        RoR::App::GetOverlayWrapper()->windowResized();
    if (gEnv->surveyMap)
        gEnv->surveyMap->windowResized();

    //update mouse area
    RoR::App::GetInputEngine()->windowResized(rw);
}

//Unattach OIS before window shutdown (very important under Linux)
void RoRFrameListener::windowClosed(Ogre::RenderWindow* rw)
{
    LOG("*** windowClosed");
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
    Beam* curr_truck = BeamFactory::getSingleton().getCurrentTruck();

    if (curr_truck && curr_truck->getReplay())
        curr_truck->getReplay()->setHidden(hidden);

#ifdef USE_SOCKETW
    if (App::GetActiveMpState() == App::MP_STATE_CONNECTED)
    {
        App::GetGuiManager()->SetVisible_MpClientList(!hidden);
    }
#endif // USE_SOCKETW

    if (hidden)
    {
        if (RoR::App::GetOverlayWrapper())
            RoR::App::GetOverlayWrapper()->showDashboardOverlays(false, curr_truck);
        if (RoR::App::GetOverlayWrapper())
            RoR::App::GetOverlayWrapper()->truckhud->show(false);
        if (gEnv->surveyMap)
            gEnv->surveyMap->setVisibility(false);
    }
    else
    {
        if (curr_truck
            && gEnv->cameraManager
            && gEnv->cameraManager->hasActiveBehavior()
            && gEnv->cameraManager->getCurrentBehavior() != RoR::PerVehicleCameraContext::CAMCTX_BEHAVIOR_VEHICLE_CINECAM)
        {
            if (RoR::App::GetOverlayWrapper())
                RoR::App::GetOverlayWrapper()->showDashboardOverlays(true, curr_truck);
        }
    }
    App::GetGuiManager()->hideGUI(hidden);
}

void RoRFrameListener::ReloadCurrentTruck()
{
    Beam* curr_truck = BeamFactory::getSingleton().getCurrentTruck();
    if (!curr_truck)
        return;
    if (curr_truck->state == NETWORKED)
        return;

    // try to load the same truck again
    Beam* newBeam = BeamFactory::getSingleton().CreateLocalRigInstance(m_reload_pos, m_reload_dir, curr_truck->realtruckfilename, -1);

    if (!newBeam)
    {
        RoR::App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_ERROR, _L("unable to load new truck: limit reached. Please restart RoR"), "error.png");
        return;
    }

    // copy over the most basic info
    if (curr_truck->free_node == newBeam->free_node)
    {
        for (int i = 0; i < curr_truck->free_node; i++)
        {
            // copy over nodes attributes if the amount of them didnt change
            newBeam->nodes[i].AbsPosition = curr_truck->nodes[i].AbsPosition;
            newBeam->nodes[i].RelPosition = curr_truck->nodes[i].RelPosition;
            newBeam->nodes[i].Velocity = curr_truck->nodes[i].Velocity;
            newBeam->nodes[i].Forces = curr_truck->nodes[i].Forces;
            newBeam->initial_node_pos[i] = curr_truck->initial_node_pos[i];
            newBeam->origin = curr_truck->origin;
        }
    }

    // TODO:
    // * copy over the engine infomation
    // * commands status
    // * other minor stati

    // notice the user about the amount of possible reloads
    String msg = TOSTRING(newBeam->trucknum) + String(" of ") + TOSTRING(MAX_TRUCKS) + String(" possible reloads.");
    RoR::App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE, msg, "information.png");
    RoR::App::GetGuiManager()->PushNotification("Notice:", msg);

    BeamFactory::getSingleton().removeCurrentTruck();

    // reset the new truck (starts engine, resets gui, ...)
    newBeam->reset();

    // enter the new truck
    BeamFactory::getSingleton().setCurrentTruck(newBeam->trucknum);
}

void RoRFrameListener::ChangedCurrentVehicle(Beam* previous_vehicle, Beam* current_vehicle)
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

    if (current_vehicle == nullptr)
    {
        // getting outside
        if (previous_vehicle && gEnv->player)
        {
            previous_vehicle->prepareInside(false);
            m_legacy_mirrors.SetActive(false);

            // get player out of the vehicle
            float rotation = previous_vehicle->getRotation() - Math::HALF_PI;
            Vector3 position = previous_vehicle->nodes[0].AbsPosition;
            if (previous_vehicle->cinecameranodepos[0] != -1 && previous_vehicle->cameranodepos[0] != -1 && previous_vehicle->cameranoderoll[0] != -1)
            {
                // truck has a cinecam
                position = previous_vehicle->nodes[previous_vehicle->cinecameranodepos[0]].AbsPosition;
                position += -2.0 * ((previous_vehicle->nodes[previous_vehicle->cameranodepos[0]].RelPosition - previous_vehicle->nodes[previous_vehicle->cameranoderoll[0]].RelPosition).normalisedCopy());
                position += Vector3(0.0, -1.0, 0.0);
            }
            gEnv->player->setBeamCoupling(false);
            gEnv->player->setRotation(Radian(rotation));
            gEnv->player->setPosition(position);
        }

        m_force_feedback->SetEnabled(false);

        // hide truckhud
        if (RoR::App::GetOverlayWrapper())
            RoR::App::GetOverlayWrapper()->truckhud->show(false);

        TRIGGER_EVENT(SE_TRUCK_EXIT, previous_vehicle?previous_vehicle->trucknum:-1);
    }
    else
    {
        // getting inside
        if (RoR::App::GetOverlayWrapper() && ! m_hide_gui)
        {
            RoR::App::GetOverlayWrapper()->showDashboardOverlays(true, current_vehicle);
        }

        // hide unused items
        if (RoR::App::GetOverlayWrapper() && current_vehicle->free_active_shock == 0)
        {
            (OverlayManager::getSingleton().getOverlayElement("tracks/rollcorneedle"))->hide();
        }

        // force feedback
        m_force_feedback->SetEnabled(current_vehicle->driveable == TRUCK); //only for trucks so far

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
                }
                else
                {
                    OverlayManager::getSingleton().getOverlayElement("tracks/helppanel")->setMaterialName("tracks/black");
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

bool RoRFrameListener::LoadTerrain()
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

    App::GetGuiManager()->FrictionSettingsUpdateCollisions();

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
        gEnv->surveyMap->setVisibility(false);
    auto loading_window = App::GetGuiManager()->GetLoadingWindow();

    loading_window->setProgress(0, _L("Unloading Terrain"));

    RoR::App::GetGuiManager()->GetMainSelector()->Reset();

    this->StopRaceTimer();
    m_legacy_mirrors.Shutdown(gEnv->sceneManager);

    RoR::App::DestroyOverlayWrapper();

    loading_window->setProgress(15, _L("Unloading Terrain"));

    //Unload all vehicules
    BeamFactory::getSingleton().CleanUpAllTrucks();
    loading_window->setProgress(30, _L("Unloading Terrain"));

    delete gEnv->player;
    gEnv->player = nullptr;
    CharacterFactory::getSingleton().DeleteAllRemoteCharacters();

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
    App::GetGuiManager()->GetTeleport()->Reset();
}

bool RoRFrameListener::SetupGameplayLoop()
{
#ifdef USE_ANGELSCRIPT
    ScriptEngine::getSingleton().SetFrameListener(this);
#endif

    auto* loading_window = App::GetGuiManager()->GetLoadingWindow();

    LOG("Loading base resources");

    loading_window->setProgress(0, _L("Loading base resources"));
    App::GetContentManager()->CheckAndLoadBaseResources();


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

        RoRnet::UserInfo info = RoR::Networking::GetLocalUserData();
        colourNum = info.colournum;
    }
#endif // USE_SOCKETW

    // NOTE: create player _AFTER_ network, important
    gEnv->player = CharacterFactory::getSingleton().createLocal(colourNum);

    // heathaze effect
    if (BSETTING("HeatHaze", false) && RoR::App::GetContentManager()->isLoaded(ContentManager::ResourcePack::HEATHAZE.mask))
    {
        m_heathaze = new HeatHaze();
        m_heathaze->setEnable(true);
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

    if (! this->LoadTerrain())
    {
        LOG("Could not load map. Returning to menu.");
        App::GetMainMenu()->LeaveMultiplayerServer();
        App::GetGuiManager()->SetVisible_LoadingWindow(false);
        return false;
    }

    App::GetGuiManager()->GetTeleport()->SetupMap(
        this,
        &gEnv->terrainManager->GetDef(),
        gEnv->terrainManager->getMaxTerrainSize(),
        gEnv->terrainManager->GetMinimapTextureName());

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

void RoRFrameListener::EnterGameplayLoop()
{
    /* SETUP */
    m_legacy_mirrors.Init(gEnv->sceneManager, gEnv->mainCamera);

    App::GetOgreSubsystem()->GetOgreRoot()->addFrameListener(this);
    RoRWindowEventUtilities::addWindowEventListener(App::GetOgreSubsystem()->GetRenderWindow(), this);
    App::GetGuiManager()->GetTopMenubar()->SetSimController(this);
    BeamFactory::getSingleton().SetSimController(this);

    unsigned long timeSinceLastFrame = 1;
    unsigned long startTime = 0;
    unsigned long minTimePerFrame = 0;
    unsigned long fpsLimit = App::GetGfxFpsLimit();

    if (fpsLimit < 10 || fpsLimit >= 200)
    {
        fpsLimit = 0;
    }

    if (fpsLimit)
    {
        minTimePerFrame = 1000 / fpsLimit;
    }

    /* LOOP */

    while (App::GetPendingAppState() == App::APP_STATE_NONE)
    {
        startTime = RoR::App::GetOgreSubsystem()->GetTimer()->getMilliseconds();

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
#ifdef USE_SOCKETW
        if ((App::GetActiveMpState() == App::MP_STATE_CONNECTED) && RoR::Networking::CheckError())
        {
            Ogre::String title = Ogre::UTFString(_L("Network fatal error: ")).asUTF8();
            Ogre::String msg = RoR::Networking::GetErrorMessage().asUTF8();
            App::GetGuiManager()->ShowMessageBox(title, msg, true, "OK", true, false, "");
            App::SetPendingAppState(App::APP_STATE_MAIN_MENU);
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

    App::SetActiveSimState(App::SIM_STATE_NONE);
    App::SetPendingSimState(App::SIM_STATE_NONE);
    this->CleanupAfterSimulation();

    /* RESTORE ENVIRONMENT */

    App::GetOgreSubsystem()->GetOgreRoot()->removeFrameListener(this);
    RoRWindowEventUtilities::removeWindowEventListener(App::GetOgreSubsystem()->GetRenderWindow(), this);
    App::GetGuiManager()->GetTopMenubar()->SetSimController(nullptr);
    BeamFactory::getSingleton().SetSimController(nullptr);
}
