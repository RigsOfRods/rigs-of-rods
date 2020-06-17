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

#include "AircraftSimulation.h"
#include "AppContext.h"
#include "Beam.h"
#include "BeamFactory.h"
#include "CacheSystem.h"
#include "CameraManager.h"
#include "Character.h"
#include "CharacterFactory.h"
#include "ChatSystem.h"
#include "Collisions.h"
#include "ContentManager.h"
#include "DashBoardManager.h"
#include "DiscordRpc.h"
#include "GameContext.h"
#include "GfxScene.h"
#include "GUIManager.h"
#include "GUI_SimActorStats.h"
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
#include "SkinFileformat.h"
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

#include <ctime>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>

#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
#include <Windows.h>
#else
#include <stdio.h>
#include <wchar.h>
#endif

using namespace Ogre;
using namespace RoR;

SimController::SimController() :
    m_hide_gui(false),
    m_is_pace_reset_pressed(false),
    m_last_simulation_speed(0.1f),
    m_physics_simulation_paused(false),
    m_physics_simulation_time(0.0f),
    m_pressure_pressed(false),
    m_pressure_pressed_timer(0.0f),
    m_time(0),
    m_time_until_next_toggle(0),
    m_advanced_vehicle_repair(false),
    m_advanced_vehicle_repair_timer(0.f)
{
}

void SimController::UpdateInputEvents(float dt)
{
    if (App::sim_state->GetEnum<SimState>() == SimState::PAUSED)
        return; //Stop everything when pause menu is visible

    if (App::GetGuiManager()->IsVisible_FrictionSettings() && App::GetGameContext()->GetPlayerActor())
    {
        App::GetGuiManager()->GetFrictionSettings()->setActiveCol(App::GetGameContext()->GetPlayerActor()->ar_last_fuzzy_ground_model);
    }

    const bool mp_connected = (App::mp_state->GetEnum<MpState>() == MpState::CONNECTED);
    if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_ENTER_CHATMODE, 0.5f) && !m_hide_gui && mp_connected)
    {
        App::GetGuiManager()->SetVisible_ChatBox(!App::GetGuiManager()->IsVisible_ChatBox());
    }

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

    if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_GET_NEW_VEHICLE))
    {
        App::GetGuiManager()->GetMainSelector()->Show(LT_AllBeam);
    }

    // Simulation pace adjustment (slowmotion)
    if (!App::GetGameContext()->GetRaceSystem().IsRaceInProgress())
    {
        if (App::GetInputEngine()->getEventBoolValue(EV_COMMON_ACCELERATE_SIMULATION))
        {
            float simulation_speed = App::GetGameContext()->GetActorManager()->GetSimulationSpeed() * pow(2.0f, dt / 2.0f);
            App::GetGameContext()->GetActorManager()->SetSimulationSpeed(simulation_speed);
            String ssmsg = _L("New simulation speed: ") + TOSTRING(Round(simulation_speed * 100.0f, 1)) + "%";
            App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE, ssmsg, "infromation.png", 2000, false);
        }
        if (App::GetInputEngine()->getEventBoolValue(EV_COMMON_DECELERATE_SIMULATION))
        {
            float simulation_speed = App::GetGameContext()->GetActorManager()->GetSimulationSpeed() * pow(0.5f, dt / 2.0f);
            App::GetGameContext()->GetActorManager()->SetSimulationSpeed(simulation_speed);
            String ssmsg = _L("New simulation speed: ") + TOSTRING(Round(simulation_speed * 100.0f, 1)) + "%";
            App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE, ssmsg, "infromation.png", 2000, false);
        }
        if (App::GetInputEngine()->getEventBoolValue(EV_COMMON_RESET_SIMULATION_PACE))
        {
            if (!m_is_pace_reset_pressed)
            {
                float simulation_speed = App::GetGameContext()->GetActorManager()->GetSimulationSpeed();
                if (simulation_speed != 1.0f)
                {
                    m_last_simulation_speed = simulation_speed;
                    App::GetGameContext()->GetActorManager()->SetSimulationSpeed(1.0f);
                    UTFString ssmsg = _L("Simulation speed reset.");
                    RoR::App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE, ssmsg, "infromation.png", 2000, false);
                }
                else if (m_last_simulation_speed != 1.0f)
                {
                    App::GetGameContext()->GetActorManager()->SetSimulationSpeed(m_last_simulation_speed);
                    String ssmsg = _L("New simulation speed: ") + TOSTRING(Round(m_last_simulation_speed * 100.0f, 1)) + "%";
                    RoR::App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE, ssmsg, "infromation.png", 2000, false);
                }
            }
            m_is_pace_reset_pressed = true;
        }
        else
        {
            m_is_pace_reset_pressed = false;
        }
    }

    // Frozen physics logic
    if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_TOGGLE_PHYSICS))
    {
        m_physics_simulation_paused = !m_physics_simulation_paused;

        if (m_physics_simulation_paused)
        {
            String ssmsg = _L("Physics paused");
            RoR::App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE, ssmsg, "infromation.png", 2000, false);
        }
        else
        {
            String ssmsg = _L("Physics unpaused");
            RoR::App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE, ssmsg, "infromation.png", 2000, false);
        }
    }
    if (m_physics_simulation_paused && App::GetGameContext()->GetActorManager()->GetSimulationSpeed() > 0.0f)
    {
        if (RoR::App::GetInputEngine()->getEventBoolValue(EV_COMMON_REPLAY_FAST_FORWARD) ||
            RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_REPLAY_FORWARD, 0.25f))
        {
            m_physics_simulation_time = PHYSICS_DT / App::GetGameContext()->GetActorManager()->GetSimulationSpeed();
        }
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
        if (!this->AreControlsLocked())
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
                if (!RoR::App::GetInputEngine()->getEventBoolValue(EV_COMMON_REPAIR_TRUCK))
                {
                    m_advanced_vehicle_repair_timer = 0.0f;
                }
                if (RoR::App::GetInputEngine()->getEventBoolValue(EV_COMMON_RESET_TRUCK) && !App::GetGameContext()->GetPlayerActor()->ar_replay_mode)
                {
                    ActorModifyRequest* rq = new ActorModifyRequest;
                    rq->amr_actor = App::GetGameContext()->GetPlayerActor();
                    rq->amr_type  = ActorModifyRequest::Type::RESET_ON_INIT_POS;
                    App::GetGameContext()->PushMessage(Message(MSG_SIM_MODIFY_ACTOR_REQUESTED, (void*)rq));
                }
                else if (RoR::App::GetInputEngine()->getEventBoolValue(EV_COMMON_REMOVE_CURRENT_TRUCK) && !App::GetGameContext()->GetPlayerActor()->ar_replay_mode)
                {
                    App::GetGameContext()->PushMessage(Message(MSG_SIM_DELETE_ACTOR_REQUESTED, (void*)App::GetGameContext()->GetPlayerActor()));
                }
                else if ((RoR::App::GetInputEngine()->getEventBoolValue(EV_COMMON_REPAIR_TRUCK) || m_advanced_vehicle_repair) && !App::GetGameContext()->GetPlayerActor()->ar_replay_mode)
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
                        float curRot = App::GetGameContext()->GetPlayerActor()->getRotation();
                        translation.x += 2.0f * cos(curRot - Ogre::Math::HALF_PI) * dt;
                        translation.z += 2.0f * sin(curRot - Ogre::Math::HALF_PI) * dt;
                    }
                    else if (RoR::App::GetInputEngine()->getEventBoolValue(EV_CHARACTER_BACKWARDS))
                    {
                        float curRot = App::GetGameContext()->GetPlayerActor()->getRotation();
                        translation.x -= 2.0f * cos(curRot - Ogre::Math::HALF_PI) * dt;
                        translation.z -= 2.0f * sin(curRot - Ogre::Math::HALF_PI) * dt;
                    }
                    if (RoR::App::GetInputEngine()->getEventBoolValue(EV_CHARACTER_SIDESTEP_RIGHT))
                    {
                        float curRot = App::GetGameContext()->GetPlayerActor()->getRotation();
                        translation.x += 2.0f * cos(curRot) * dt;
                        translation.z += 2.0f * sin(curRot) * dt;
                    }
                    else if (RoR::App::GetInputEngine()->getEventBoolValue(EV_CHARACTER_SIDESTEP_LEFT))
                    {
                        float curRot = App::GetGameContext()->GetPlayerActor()->getRotation();
                        translation.x -= 2.0f * cos(curRot) * dt;
                        translation.z -= 2.0f * sin(curRot) * dt;
                    }

                    if (translation != Vector3::ZERO || rotation != 0.0f)
                    {
                        float scale = RoR::App::GetInputEngine()->isKeyDown(OIS::KC_LMENU) ? 0.1f : 1.0f;
                        scale *= RoR::App::GetInputEngine()->isKeyDown(OIS::KC_LSHIFT) ? 3.0f : 1.0f;
                        scale *= RoR::App::GetInputEngine()->isKeyDown(OIS::KC_LCONTROL) ? 10.0f : 1.0f;

                        Vector3 rotation_center = App::GetGameContext()->GetPlayerActor()->GetRotationCenter();

                        rotation *= Ogre::Math::Clamp(scale, 0.1f, 10.0f);
                        translation *= scale;

                        App::GetGameContext()->GetPlayerActor()->RequestRotation(rotation, rotation_center);
                        App::GetGameContext()->GetPlayerActor()->RequestTranslation(translation);

                        if (App::sim_soft_reset_mode->GetBool())
                        {
                            for (auto actor : App::GetGameContext()->GetPlayerActor()->GetAllLinkedActors())
                            {
                                actor->RequestRotation(rotation, rotation_center);
                                actor->RequestTranslation(translation);
                            }
                        }

                        m_advanced_vehicle_repair_timer = 0.0f;
                    }
                    else if (RoR::App::GetInputEngine()->isKeyDownValueBounce(OIS::KC_SPACE))
                    {
                        App::GetGameContext()->GetPlayerActor()->RequestAngleSnap(45);
                        if (App::sim_soft_reset_mode->GetBool())
                        {
                            for (auto actor : App::GetGameContext()->GetPlayerActor()->GetAllLinkedActors())
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
                    if (App::sim_soft_reset_mode->GetBool())
                    {
                        reset_type = ActorModifyRequest::Type::SOFT_RESET;
                        for (auto actor : App::GetGameContext()->GetPlayerActor()->GetAllLinkedActors())
                        {
                            ActorModifyRequest* rq = new ActorModifyRequest;
                            rq->amr_actor = actor;
                            rq->amr_type = reset_type;
                            App::GetGameContext()->PushMessage(Message(MSG_SIM_MODIFY_ACTOR_REQUESTED, (void*)rq));
                        }
                    }

                    ActorModifyRequest* rq = new ActorModifyRequest;
                    rq->amr_actor = App::GetGameContext()->GetPlayerActor();
                    rq->amr_type = reset_type;
                    App::GetGameContext()->PushMessage(Message(MSG_SIM_MODIFY_ACTOR_REQUESTED, (void*)rq));
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
                    // replay mode
                    if (App::GetGameContext()->GetPlayerActor()->ar_replay_mode)
                    {
                        if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_REPLAY_FORWARD, 0.1f) && App::GetGameContext()->GetPlayerActor()->ar_replay_pos <= 0)
                        {
                            App::GetGameContext()->GetPlayerActor()->ar_replay_pos++;
                        }
                        if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_REPLAY_BACKWARD, 0.1f) && App::GetGameContext()->GetPlayerActor()->ar_replay_pos > -App::GetGameContext()->GetPlayerActor()->ar_replay_length)
                        {
                            App::GetGameContext()->GetPlayerActor()->ar_replay_pos--;
                        }
                        if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_REPLAY_FAST_FORWARD, 0.1f) && App::GetGameContext()->GetPlayerActor()->ar_replay_pos + 10 <= 0)
                        {
                            App::GetGameContext()->GetPlayerActor()->ar_replay_pos += 10;
                        }
                        if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_REPLAY_FAST_BACKWARD, 0.1f) && App::GetGameContext()->GetPlayerActor()->ar_replay_pos - 10 > -App::GetGameContext()->GetPlayerActor()->ar_replay_length)
                        {
                            App::GetGameContext()->GetPlayerActor()->ar_replay_pos -= 10;
                        }

                        if (RoR::App::GetInputEngine()->isKeyDown(OIS::KC_LMENU))
                        {
                            if (App::GetGameContext()->GetPlayerActor()->ar_replay_pos <= 0 && App::GetGameContext()->GetPlayerActor()->ar_replay_pos >= -App::GetGameContext()->GetPlayerActor()->ar_replay_length)
                            {
                                if (RoR::App::GetInputEngine()->isKeyDown(OIS::KC_LSHIFT) || RoR::App::GetInputEngine()->isKeyDown(OIS::KC_RSHIFT))
                                {
                                    App::GetGameContext()->GetPlayerActor()->ar_replay_pos += RoR::App::GetInputEngine()->getMouseState().X.rel * 1.5f;
                                }
                                else
                                {
                                    App::GetGameContext()->GetPlayerActor()->ar_replay_pos += RoR::App::GetInputEngine()->getMouseState().X.rel * 0.05f;
                                }
                                if (App::GetGameContext()->GetPlayerActor()->ar_replay_pos > 0)
                                {
                                    App::GetGameContext()->GetPlayerActor()->ar_replay_pos = 0;
                                }
                                if (App::GetGameContext()->GetPlayerActor()->ar_replay_pos < -App::GetGameContext()->GetPlayerActor()->ar_replay_length)
                                {
                                    App::GetGameContext()->GetPlayerActor()->ar_replay_pos = -App::GetGameContext()->GetPlayerActor()->ar_replay_length;
                                }
                            }
                        }
                    }

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
                    }
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

                    //replay mode
                    if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_TOGGLE_REPLAY_MODE))
                    {
                        if (App::GetGameContext()->GetPlayerActor()->getReplay() != nullptr)
                        {
                            App::GetGameContext()->GetPlayerActor()->setReplayMode(!App::GetGameContext()->GetPlayerActor()->ar_replay_mode);
                        }
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

                    //camera mode
                    if (RoR::App::GetInputEngine()->getEventBoolValue(EV_COMMON_PRESSURE_LESS))
                    {
                        float change = App::GetGameContext()->GetPlayerActor()->GetTyrePressure() * (1.0f - pow(2.0f, dt / 2.0f));
                        change = Math::Clamp(change, -dt * 10.0f, -dt * 1.0f);
                        if (m_pressure_pressed = App::GetGameContext()->GetPlayerActor()->AddTyrePressure(change))
                        {
                            if (RoR::App::GetOverlayWrapper())
                                RoR::App::GetOverlayWrapper()->showPressureOverlay(true);

                            SOUND_START(App::GetGameContext()->GetPlayerActor(), SS_TRIG_AIR);
                        }
                    }
                    else if (RoR::App::GetInputEngine()->getEventBoolValue(EV_COMMON_PRESSURE_MORE))
                    {
                        float change = App::GetGameContext()->GetPlayerActor()->GetTyrePressure() * (pow(2.0f, dt / 2.0f) - 1.0f);
                        change = Math::Clamp(change, +dt * 1.0f, +dt * 10.0f);
                        if (m_pressure_pressed = App::GetGameContext()->GetPlayerActor()->AddTyrePressure(change))
                        {
                            if (RoR::App::GetOverlayWrapper())
                                RoR::App::GetOverlayWrapper()->showPressureOverlay(true);

                            SOUND_START(App::GetGameContext()->GetPlayerActor(), SS_TRIG_AIR);
                        }
                    }
                    else if (m_pressure_pressed)
                    {
                        SOUND_STOP(App::GetGameContext()->GetPlayerActor(), SS_TRIG_AIR);
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

            if (App::GetInputEngine()->getEventBoolValue(EV_COMMON_ENTER_OR_EXIT_TRUCK) && m_time_until_next_toggle <= 0)
            {
                m_time_until_next_toggle = 0.5; // Some delay before trying to re-enter(exit) actor
                // perso in/out
                if (App::GetGameContext()->GetPlayerActor() == nullptr)
                {
                    // find the nearest vehicle
                    float mindist = 1000.0;
                    Actor* nearest_actor = nullptr;
                    for (auto actor : App::GetGameContext()->GetActorManager()->GetActors())
                    {
                        if (!actor->ar_driveable)
                            continue;
                        if (actor->ar_cinecam_node[0] == -1)
                        {
                            LOG("cinecam missing, cannot enter the actor!");
                            continue;
                        }
                        float len = 0.0f;
                        if (App::GetGameContext()->GetPlayerCharacter())
                        {
                            len = actor->ar_nodes[actor->ar_cinecam_node[0]].AbsPosition.distance(App::GetGameContext()->GetPlayerCharacter()->getPosition() + Vector3(0.0, 2.0, 0.0));
                        }
                        if (len < mindist)
                        {
                            mindist = len;
                            nearest_actor = actor;
                        }
                    }
                    if (mindist < 20.0)
                    {
                        App::GetGameContext()->PushMessage(Message(MSG_SIM_SEAT_PLAYER_REQUESTED, (void*)nearest_actor));
                    }
                }
                else if (App::GetGameContext()->GetPlayerActor()->ar_nodes[0].Velocity.squaredLength() < 1.0f ||
                        App::GetGameContext()->GetPlayerActor()->ar_sim_state == Actor::SimState::NETWORKED_OK)
                {
                    App::GetGameContext()->PushMessage(Message(MSG_SIM_SEAT_PLAYER_REQUESTED, nullptr));
                }
                else
                {
                    App::GetGameContext()->GetPlayerActor()->ar_brake = 0.66f;
                    m_time_until_next_toggle = 0.0; // No delay in this case: the vehicle must brake like braking normally
                }
            }
            else if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_ENTER_NEXT_TRUCK, 0.25f))
            {
                Actor* actor = App::GetGameContext()->FetchNextVehicleOnList();
                if (actor != App::GetGameContext()->GetPlayerActor())
                {
                    App::GetGameContext()->PushMessage(Message(MSG_SIM_SEAT_PLAYER_REQUESTED, (void*)actor));
                }
            }
            else if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_ENTER_PREVIOUS_TRUCK, 0.25f))
            {
                Actor* actor = App::GetGameContext()->FetchPrevVehicleOnList();
                if (actor != App::GetGameContext()->GetPlayerActor())
                {
                    App::GetGameContext()->PushMessage(Message(MSG_SIM_SEAT_PLAYER_REQUESTED, (void*)actor));
                }
            }
            else if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_RESPAWN_LAST_TRUCK, 0.25f))
            {
                App::GetGameContext()->RespawnLastActor();
            }
            else if (App::GetInputEngine()->getEventBoolValueBounce(EV_SURVEY_MAP_CYCLE))
            {
                App::GetGuiManager()->GetSurveyMap()->CycleMode();
            }
            else if (App::GetInputEngine()->getEventBoolValueBounce(EV_SURVEY_MAP_TOGGLE))
            {
                App::GetGuiManager()->GetSurveyMap()->ToggleMode();
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

        if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_TOGGLE_RENDER_MODE, 0.5f))
        {
            static int mSceneDetailIndex;
            mSceneDetailIndex = (mSceneDetailIndex + 1) % 3;
            switch (mSceneDetailIndex)
            {
            case 0:
                App::GetCameraManager()->GetCamera()->setPolygonMode(PM_SOLID);
                break;
            case 1:
                App::GetCameraManager()->GetCamera()->setPolygonMode(PM_WIREFRAME);
                break;
            case 2:
                App::GetCameraManager()->GetCamera()->setPolygonMode(PM_POINTS);
                break;
            }
        }
    }

    if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_TRUCK_INFO) && App::GetGameContext()->GetPlayerActor())
    {
        App::GetGuiManager()->SetVisible_SimActorStats(!App::GetGuiManager()->IsVisible_SimActorStats());
    }

    if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_TRUCK_DESCRIPTION) && App::GetGameContext()->GetPlayerActor())
    {
        App::GetGuiManager()->SetVisible_VehicleDescription(! App::GetGuiManager()->IsVisible_VehicleDescription());
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
            RoR::App::GetOverlayWrapper()->ToggleDashboardOverlays(App::GetGameContext()->GetPlayerActor());
        }
    }

    if ((App::sim_state->GetEnum<SimState>() == SimState::RUNNING || App::sim_state->GetEnum<SimState>() == SimState::PAUSED || App::sim_state->GetEnum<SimState>() == SimState::EDITOR_MODE) && RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_TOGGLE_STATS))
    {
        App::GetGuiManager()->SetVisible_SimPerfStats(!App::GetGuiManager()->IsVisible_SimPerfStats());
    }

    if (RoR::App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_OUTPUT_POSITION))
    {
        Vector3 position(Vector3::ZERO);
        Radian rotation(0);
        if (App::GetGameContext()->GetPlayerActor() == nullptr)
        {
            position = App::GetGameContext()->GetPlayerCharacter()->getPosition();
            rotation = App::GetGameContext()->GetPlayerCharacter()->getRotation() + Radian(Math::PI);
        }
        else
        {
            position = App::GetGameContext()->GetPlayerActor()->getPosition();
            rotation = App::GetGameContext()->GetPlayerActor()->getRotation();
        }
        String pos = StringUtil::format("%8.3f, %8.3f, %8.3f"   , position.x, position.y, position.z);
        String rot = StringUtil::format("% 6.1f, % 6.1f, % 6.1f",       0.0f, rotation.valueDegrees()  ,       0.0f);
        LOG("Position: " + pos + ", " + rot);
    }

    if (m_time_until_next_toggle >= 0)
    {
        m_time_until_next_toggle -= dt;
    }
}

void SimController::UpdateSimulation(float dt)
{
    if (App::io_outgauge_mode->GetInt() > 0)
    {
        App::GetOutGauge()->Update(dt, App::GetGameContext()->GetPlayerActor());
    }

    if (App::sim_state->GetEnum<SimState>() == SimState::RUNNING || App::sim_state->GetEnum<SimState>() == SimState::EDITOR_MODE)
    {
        App::GetCameraManager()->Update(dt, App::GetGameContext()->GetPlayerActor(), App::GetGameContext()->GetActorManager()->GetSimulationSpeed());
    }

    m_physics_simulation_time = m_physics_simulation_paused ? 0.0f : dt;

    App::GetOverlayWrapper()->update(dt);

    this->UpdateInputEvents(dt);

    RoR::App::GetGuiManager()->DrawSimulationGui(dt);

    for (auto actor : App::GetGameContext()->GetActorManager()->GetActors())
    {
        actor->GetGfxActor()->UpdateDebugView();
    }

    if (App::sim_state->GetEnum<SimState>() == SimState::RUNNING || App::sim_state->GetEnum<SimState>() == SimState::EDITOR_MODE || (App::mp_state->GetEnum<MpState>() == MpState::CONNECTED))
    {
        float simulation_speed = App::GetGameContext()->GetActorManager()->GetSimulationSpeed();
        if (App::GetGameContext()->GetRaceSystem().IsRaceInProgress() && simulation_speed != 1.0f)
        {
            m_last_simulation_speed = simulation_speed;
            App::GetGameContext()->GetActorManager()->SetSimulationSpeed(1.0f);
        }

        if (App::GetGameContext()->GetPlayerActor())
        {
            App::GetGuiManager()->GetSimActorStats()->UpdateStats(dt, App::GetGameContext()->GetPlayerActor());
        }

        App::GetGuiManager()->GetSceneMouse()->Draw(); // Touches simulation directly - must be done here

        App::GetGfxScene()->BufferSimulationData();

        if (App::sim_state->GetEnum<SimState>() != SimState::PAUSED)
        {
            App::GetGameContext()->UpdateActors(m_physics_simulation_time); // *** Start new physics tasks. No reading from Actor N/B beyond this point.
        }
    }

    if (RoR::App::sim_state->GetEnum<SimState>() != RoR::SimState::PAUSED &&
        !m_physics_simulation_paused)
    {
        m_time += dt;
    }
}

void SimController::HideGUI(bool hidden)
{
    if (App::GetGameContext()->GetPlayerActor() && App::GetGameContext()->GetPlayerActor()->getReplay())
        App::GetGameContext()->GetPlayerActor()->getReplay()->setHidden(hidden);

    if (RoR::App::GetOverlayWrapper())
        RoR::App::GetOverlayWrapper()->showDashboardOverlays(!hidden, App::GetGameContext()->GetPlayerActor());

    App::GetGuiManager()->hideGUI(hidden);
}

void SimController::RemoveActorByCollisionBox(std::string const & ev_src_instance_name, std::string const & box_name)
{
    Actor* actor = App::GetGameContext()->FindActorByCollisionBox(ev_src_instance_name, box_name);
    if (actor)
    {
        App::GetGameContext()->PushMessage(Message(MSG_SIM_DELETE_ACTOR_REQUESTED, (void*)actor));
    }
}

bool SimController::AreControlsLocked() const
{
    return App::GetCameraManager()->gameControlsLocked();
}
