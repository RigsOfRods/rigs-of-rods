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

#include "GeneralSimulation.h"

#include "AircraftSimulation.h"
#include "AppContext.h"
#include "Actor.h"
#include "ActorManager.h"
#include "CacheSystem.h"
#include "CameraManager.h"
#include "Character.h"
#include "CharacterFactory.h"
#include "ChatSystem.h"
#include "CameraManager.h"
#include "Collisions.h"
#include "ContentManager.h"
#include "DashBoardManager.h"
#include "DiscordRpc.h"
#include "GameContext.h"
#include "GfxScene.h"
#include "GUIManager.h"
#include "GUI_SimActorStats.h"
#include "GUI_SceneMouse.h"
#include "GUI_SurveyMap.h"
#include "ForceFeedback.h"
#include "InputEngine.h"
#include "LandVehicleSimulation.h"
#include "Language.h"
#include "MumbleIntegration.h"
#include "OutGauge.h"
#include "OverlayWrapper.h"
#include "PlatformUtils.h"
#include "RaceSystem.h"
#include "Replay.h"
#include "RigDef_File.h"
#include "RoRVersion.h"
#include "ScrewProp.h"
#include "ScriptEngine.h"
#include "SkinFileFormat.h"
#include "SkyManager.h"
#include "SkyXManager.h"
#include "SoundScriptManager.h"
#include "TerrainManager.h"
#include "TerrainObjectManager.h"
#include "Utils.h"

#include "GUIManager.h"
#include "GUI_FrictionSettings.h"
#include "Console.h"
#include "GUI_LoadingWindow.h"
#include "GUI_MainSelector.h"
#include "GUI_MultiplayerClientList.h"

using namespace Ogre;
using namespace RoR;

void GeneralSimulation::UpdateInputEvents(float dt)
{
    // TODO: Move outside
    if (App::sim_state->GetEnum<SimState>() == SimState::PAUSED)
        return; //Stop everything when pause menu is visible

    // TODO: put to GUI::Frictionsettings::Draw() via scene simbuffer
    if (App::GetGuiManager()->IsVisible_FrictionSettings() && App::GetGameContext()->GetPlayerActor())
    {
        App::GetGuiManager()->GetFrictionSettings()->setActiveCol(App::GetGameContext()->GetPlayerActor()->ar_last_fuzzy_ground_model);
    }

    const bool mp_connected = (App::mp_state->GetEnum<MpState>() == MpState::CONNECTED);
    if ((App::GetGameContext()->GetPlayerActor() != nullptr) &&
        (App::GetGameContext()->GetPlayerActor()->ar_sim_state != Actor::SimState::NETWORKED_OK) &&
        App::GetInputEngine()->getEventBoolValueBounce(EV_TRUCKEDIT_RELOAD, 0.5f))
    {
        ActorModifyRequest* rq = new ActorModifyRequest;
        rq->amr_type = ActorModifyRequest::Type::RELOAD;
        rq->amr_actor = App::GetGameContext()->GetPlayerActor();
        App::GetGameContext()->PushMessage(Message(MSG_SIM_MODIFY_ACTOR_REQUESTED, (void*)rq));
        return;
    }

    bool toggle_editor = (App::GetGameContext()->GetPlayerActor() && App::sim_state->GetEnum<SimState>() == SimState::EDITOR_MODE) ||
        (!App::GetGameContext()->GetPlayerActor() && RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_TOGGLE_TERRAIN_EDITOR));

    if (toggle_editor)
    {
        Message m(App::sim_state->GetEnum<SimState>() == SimState::EDITOR_MODE ?
                  MSG_EDI_LEAVE_TERRN_EDITOR_REQUESTED : MSG_EDI_ENTER_TERRN_EDITOR_REQUESTED);
        App::GetGameContext()->PushMessage(m);
    }

    if (App::sim_state->GetEnum<SimState>() == SimState::EDITOR_MODE)
    {
        App::GetSimTerrain()->GetTerrainEditor()->UpdateInputEvents(dt);
    }
    else if (App::sim_state->GetEnum<SimState>() == SimState::RUNNING || App::sim_state->GetEnum<SimState>() == SimState::PAUSED)
    {
        App::GetGameContext()->GetCharacterFactory()->Update(dt);
        if (App::GetCameraManager()->GetCurrentBehavior() != CameraManager::CAMERA_BEHAVIOR_FREE)
        {
            if (App::GetGameContext()->GetPlayerActor() && App::GetGameContext()->GetPlayerActor()->ar_sim_state != Actor::SimState::NETWORKED_OK) // we are in a vehicle
            {
                if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_TOGGLE_PHYSICS))
                {
                    for (auto actor : App::GetGameContext()->GetPlayerActor()->GetAllLinkedActors())
                    {
                        actor->ar_physics_paused = !App::GetGameContext()->GetPlayerActor()->ar_physics_paused;
                    }
                    App::GetGameContext()->GetPlayerActor()->ar_physics_paused = !App::GetGameContext()->GetPlayerActor()->ar_physics_paused;
                }
                if (App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_TOGGLE_RESET_MODE))
                {
                    App::sim_soft_reset_mode->SetVal(!App::sim_soft_reset_mode->GetBool());
                    App::GetConsole()->putMessage(
                        Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE,
                        (App::sim_soft_reset_mode->GetBool()) ? _L("Enabled soft reset mode") : _L("Enabled hard reset mode"));
                }

                if (App::GetInputEngine()->getEventBoolValue(EV_COMMON_RESET_TRUCK) &&
                    App::GetGameContext()->GetPlayerActor()->ar_sim_state != Actor::SimState::LOCAL_REPLAY)
                {
                    ActorModifyRequest* rq = new ActorModifyRequest;
                    rq->amr_actor = App::GetGameContext()->GetPlayerActor();
                    rq->amr_type  = ActorModifyRequest::Type::RESET_ON_INIT_POS;
                    App::GetGameContext()->PushMessage(Message(MSG_SIM_MODIFY_ACTOR_REQUESTED, (void*)rq));
                }
                else if (App::GetInputEngine()->getEventBoolValue(EV_COMMON_REMOVE_CURRENT_TRUCK) &&
                         App::GetGameContext()->GetPlayerActor()->ar_sim_state != Actor::SimState::LOCAL_REPLAY)
                {
                    App::GetGameContext()->PushMessage(Message(MSG_SIM_DELETE_ACTOR_REQUESTED, (void*)App::GetGameContext()->GetPlayerActor()));
                }
                else
                {
                    // get commands
                    // -- here we should define a maximum numbers per actor. Some actors does not have that much commands
                    // -- available, so why should we iterate till MAX_COMMANDS?
                    for (int i = 1; i <= MAX_COMMANDS + 1; i++)
                    {
                        int eventID = EV_COMMANDS_01 + (i - 1);

                        App::GetGameContext()->GetPlayerActor()->ar_command_key[i].playerInputValue = RoR::App::GetInputEngine()->getEventValue(eventID);
                    }

                    if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_TOGGLE_FORWARDCOMMANDS))
                    {
                        App::GetGameContext()->GetPlayerActor()->ar_forward_commands = !App::GetGameContext()->GetPlayerActor()->ar_forward_commands;
                        if (App::GetGameContext()->GetPlayerActor()->ar_forward_commands)
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
                        App::GetGameContext()->GetPlayerActor()->ar_import_commands = !App::GetGameContext()->GetPlayerActor()->ar_import_commands;
                        if (App::GetGameContext()->GetPlayerActor()->ar_import_commands)
                        {
                            RoR::App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE, _L("importcommands enabled"), "information.png", 3000);
                        }
                        else
                        {
                            RoR::App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE, _L("importcommands disabled"), "information.png", 3000);
                        }
                    }
                    if (App::GetGameContext()->GetPlayerActor()->GetReplay())
                    {
                        App::GetGameContext()->GetPlayerActor()->GetReplay()->UpdateInputEvents();
                    }

                    if (App::GetGameContext()->GetPlayerActor()->ar_sim_state != Actor::SimState::LOCAL_REPLAY)
                    {
                        if (App::GetGameContext()->GetPlayerActor()->ar_driveable == TRUCK)
                        {
                            LandVehicleSimulation::UpdateInputEvents(App::GetGameContext()->GetPlayerActor(), dt);
                        }
                        if (App::GetGameContext()->GetPlayerActor()->ar_driveable == AIRPLANE)
                        {
                            AircraftSimulation::UpdateInputEvents(App::GetGameContext()->GetPlayerActor(), dt);
                        }
                        if (App::GetGameContext()->GetPlayerActor()->ar_driveable == BOAT)
                        {
                            //BOAT SPECIFICS

                            //throttle
                            if (RoR::App::GetInputEngine()->isEventDefined(EV_BOAT_THROTTLE_AXIS))
                            {
                                float f = RoR::App::GetInputEngine()->getEventValue(EV_BOAT_THROTTLE_AXIS);
                                // use negative values also!
                                f = f * 2 - 1;
                                for (int i = 0; i < App::GetGameContext()->GetPlayerActor()->ar_num_screwprops; i++)
                                    App::GetGameContext()->GetPlayerActor()->ar_screwprops[i]->setThrottle(-f);
                            }
                            if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_BOAT_THROTTLE_DOWN, 0.1f))
                            {
                                //throttle down
                                for (int i = 0; i < App::GetGameContext()->GetPlayerActor()->ar_num_screwprops; i++)
                                    App::GetGameContext()->GetPlayerActor()->ar_screwprops[i]->setThrottle(App::GetGameContext()->GetPlayerActor()->ar_screwprops[i]->getThrottle() - 0.05);
                            }
                            if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_BOAT_THROTTLE_UP, 0.1f))
                            {
                                //throttle up
                                for (int i = 0; i < App::GetGameContext()->GetPlayerActor()->ar_num_screwprops; i++)
                                    App::GetGameContext()->GetPlayerActor()->ar_screwprops[i]->setThrottle(App::GetGameContext()->GetPlayerActor()->ar_screwprops[i]->getThrottle() + 0.05);
                            }

                            // steer
                            float tmp_steer_left = RoR::App::GetInputEngine()->getEventValue(EV_BOAT_STEER_LEFT);
                            float tmp_steer_right = RoR::App::GetInputEngine()->getEventValue(EV_BOAT_STEER_RIGHT);
                            float stime = RoR::App::GetInputEngine()->getEventBounceTime(EV_BOAT_STEER_LEFT) + RoR::App::GetInputEngine()->getEventBounceTime(EV_BOAT_STEER_RIGHT);
                            float sum_steer = (tmp_steer_left - tmp_steer_right) * dt;
                            // do not center the rudder!
                            if (fabs(sum_steer) > 0 && stime <= 0)
                            {
                                for (int i = 0; i < App::GetGameContext()->GetPlayerActor()->ar_num_screwprops; i++)
                                    App::GetGameContext()->GetPlayerActor()->ar_screwprops[i]->setRudder(App::GetGameContext()->GetPlayerActor()->ar_screwprops[i]->getRudder() + sum_steer);
                            }
                            if (RoR::App::GetInputEngine()->isEventDefined(EV_BOAT_STEER_LEFT_AXIS) && RoR::App::GetInputEngine()->isEventDefined(EV_BOAT_STEER_RIGHT_AXIS))
                            {
                                tmp_steer_left = RoR::App::GetInputEngine()->getEventValue(EV_BOAT_STEER_LEFT_AXIS);
                                tmp_steer_right = RoR::App::GetInputEngine()->getEventValue(EV_BOAT_STEER_RIGHT_AXIS);
                                sum_steer = (tmp_steer_left - tmp_steer_right);
                                for (int i = 0; i < App::GetGameContext()->GetPlayerActor()->ar_num_screwprops; i++)
                                    App::GetGameContext()->GetPlayerActor()->ar_screwprops[i]->setRudder(sum_steer);
                            }
                            if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_BOAT_CENTER_RUDDER, 0.1f))
                            {
                                for (int i = 0; i < App::GetGameContext()->GetPlayerActor()->ar_num_screwprops; i++)
                                    App::GetGameContext()->GetPlayerActor()->ar_screwprops[i]->setRudder(0);
                            }

                            if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_BOAT_REVERSE))
                            {
                                for (int i = 0; i < App::GetGameContext()->GetPlayerActor()->ar_num_screwprops; i++)
                                    App::GetGameContext()->GetPlayerActor()->ar_screwprops[i]->toggleReverse();
                            }
                        } // if driveable == BOAT
                    } // if not in replay mode
                    //COMMON KEYS

                    if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_TRUCK_REMOVE))
                    {
                        App::GetGameContext()->PushMessage(Message(MSG_SIM_DELETE_ACTOR_REQUESTED, (void*)App::GetGameContext()->GetPlayerActor()));
                    }
                    if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_ROPELOCK))
                    {
                        App::GetGameContext()->GetPlayerActor()->ar_toggle_ropes = true;
                    }
                    if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_LOCK))
                    {
                        App::GetGameContext()->GetPlayerActor()->ToggleHooks(-1, HOOK_TOGGLE, -1);
                        //SlideNodeLock
                        App::GetGameContext()->GetPlayerActor()->ToggleSlideNodeLock();
                    }
                    if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_AUTOLOCK))
                    {
                        //unlock all autolocks
                        App::GetGameContext()->GetPlayerActor()->ToggleHooks(-2, HOOK_UNLOCK, -1);
                    }
                    //strap
                    if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_SECURE_LOAD))
                    {
                        App::GetGameContext()->GetPlayerActor()->ar_toggle_ties = true;
                    }

                    if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_TOGGLE_CUSTOM_PARTICLES))
                    {
                        App::GetGameContext()->GetPlayerActor()->ToggleCustomParticles();
                    }

                    if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_TOGGLE_DEBUG_VIEW))
                    {
                        App::GetGameContext()->GetPlayerActor()->GetGfxActor()->ToggleDebugView();
                        for (auto actor : App::GetGameContext()->GetPlayerActor()->GetAllLinkedActors())
                        {
                            actor->GetGfxActor()->SetDebugView(App::GetGameContext()->GetPlayerActor()->GetGfxActor()->GetDebugView());
                        }
                    }

                    if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_CYCLE_DEBUG_VIEWS))
                    {
                        App::GetGameContext()->GetPlayerActor()->GetGfxActor()->CycleDebugViews();
                        for (auto actor : App::GetGameContext()->GetPlayerActor()->GetAllLinkedActors())
                        {
                            actor->GetGfxActor()->SetDebugView(App::GetGameContext()->GetPlayerActor()->GetGfxActor()->GetDebugView());
                        }
                    }

                    if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_TOGGLE_TRUCK_LIGHTS))
                    {
                        App::GetGameContext()->GetPlayerActor()->ToggleLights();
                    }

                    if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_TOGGLE_TRUCK_BEACONS))
                    {
                        App::GetGameContext()->GetPlayerActor()->ToggleBeacons();
                    }

                    if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_RESCUE_TRUCK, 0.5f) && !mp_connected && App::GetGameContext()->GetPlayerActor()->ar_driveable != AIRPLANE)
                    {
                        Actor* rescuer = nullptr;
                        for (auto actor : App::GetGameContext()->GetActorManager()->GetActors())
                        {
                            if (actor->ar_rescuer_flag)
                            {
                                rescuer = actor;
                            }
                        }
                        if (rescuer == nullptr)
                        {
                            RoR::App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE, _L("No rescue truck found!"), "warning.png");
                        }
                        else
                        {
                            App::GetGameContext()->PushMessage(Message(MSG_SIM_SEAT_PLAYER_REQUESTED, (void*)rescuer));
                        }
                    }

                    if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_TRAILER_PARKING_BRAKE))
                    {
                        if (App::GetGameContext()->GetPlayerActor()->ar_driveable == TRUCK)
                            App::GetGameContext()->GetPlayerActor()->ar_trailer_parking_brake = !App::GetGameContext()->GetPlayerActor()->ar_trailer_parking_brake;
                        else if (App::GetGameContext()->GetPlayerActor()->ar_driveable == NOT_DRIVEABLE)
                            App::GetGameContext()->GetPlayerActor()->ToggleParkingBrake();
                    }

                    if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_TOGGLE_VIDEOCAMERA, 0.5f))
                    {
                        if (App::GetGameContext()->GetPlayerActor()->GetGfxActor()->GetVideoCamState() == GfxActor::VideoCamState::VCSTATE_DISABLED)
                        {
                            App::GetGameContext()->GetPlayerActor()->GetGfxActor()->SetVideoCamState(GfxActor::VideoCamState::VCSTATE_ENABLED_ONLINE);
                        }
                        else
                        {
                            App::GetGameContext()->GetPlayerActor()->GetGfxActor()->SetVideoCamState(GfxActor::VideoCamState::VCSTATE_DISABLED);
                        }
                    }

                    if (!RoR::App::GetInputEngine()->getEventBoolValue(EV_COMMON_RESPAWN_LAST_TRUCK))
                    {
                        if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_BLINK_LEFT))
                            App::GetGameContext()->GetPlayerActor()->toggleBlinkType(BLINK_LEFT);
                        if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_BLINK_RIGHT))
                            App::GetGameContext()->GetPlayerActor()->toggleBlinkType(BLINK_RIGHT);
                        if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_BLINK_WARN))
                            App::GetGameContext()->GetPlayerActor()->toggleBlinkType(BLINK_WARN);
                    }
                }
            }
            else if (!App::GetGameContext()->GetPlayerActor())
            {
                // Find nearest actor
                const Ogre::Vector3 position = App::GetGameContext()->GetPlayerCharacter()->getPosition();
                Actor* nearest_actor = nullptr;
                float min_squared_distance = std::numeric_limits<float>::max();
                for (auto actor : App::GetGameContext()->GetActorManager()->GetActors())
                {
                    float squared_distance = position.squaredDistance(actor->ar_nodes[0].AbsPosition);
                    if (squared_distance < min_squared_distance)
                    {
                        min_squared_distance = squared_distance;
                        nearest_actor = actor;
                    }
                }
                
                // Evaluate
                if (nearest_actor != nullptr &&
                    nearest_actor->ar_import_commands &&
                    min_squared_distance < (nearest_actor->getMinCameraRadius()*nearest_actor->getMinCameraRadius()))
                {
                    // get commands
                    // -- here we should define a maximum numbers per actor. Some actors does not have that much commands
                    // -- available, so why should we iterate till MAX_COMMANDS?
                    for (int i = 1; i <= MAX_COMMANDS + 1; i++)
                    {
                        int eventID = EV_COMMANDS_01 + (i - 1);

                        nearest_actor->ar_command_key[i].playerInputValue = RoR::App::GetInputEngine()->getEventValue(eventID);
                    }
                }
            }

            // EV_COMMON_ENTER_OR_EXIT_TRUCK - Without a delay: the vehicle must brake like braking normally
            if (App::GetInputEngine()->getEventBoolValue(EV_COMMON_ENTER_OR_EXIT_TRUCK) &&
                App::GetGameContext()->GetPlayerActor())
            {
                App::GetGameContext()->GetPlayerActor()->ar_brake = 0.66f;
            }
            
        } // AreControlsLocked()

#ifdef USE_CAELUM

        const bool caelum_enabled = App::gfx_sky_mode->GetEnum<GfxSkyMode>() == GfxSkyMode::CAELUM;
        SkyManager* sky_mgr = App::GetSimTerrain()->getSkyManager();
        if (caelum_enabled && (sky_mgr != nullptr) && (App::sim_state->GetEnum<SimState>() == SimState::RUNNING || App::sim_state->GetEnum<SimState>() == SimState::PAUSED || App::sim_state->GetEnum<SimState>() == SimState::EDITOR_MODE))
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
                RoR::App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE, _L("Time set to ") + sky_mgr->GetPrettyTime(), "weather_sun.png", 1000);
            }
        }

#endif // USE_CAELUM


        const bool skyx_enabled = App::gfx_sky_mode->GetEnum<GfxSkyMode>() == GfxSkyMode::SKYX;
        SkyXManager* skyx_mgr = App::GetSimTerrain()->getSkyXManager();
        if (skyx_enabled && (skyx_mgr != nullptr) && (App::sim_state->GetEnum<SimState>() == SimState::RUNNING || App::sim_state->GetEnum<SimState>() == SimState::PAUSED || App::sim_state->GetEnum<SimState>() == SimState::EDITOR_MODE))
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
    }


}
