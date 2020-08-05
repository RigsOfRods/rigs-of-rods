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




    if (App::sim_state->GetEnum<SimState>() == SimState::RUNNING || App::sim_state->GetEnum<SimState>() == SimState::PAUSED)
    {
        if (App::GetCameraManager()->GetCurrentBehavior() != CameraManager::CAMERA_BEHAVIOR_FREE)
        {
            if (App::GetGameContext()->GetPlayerActor() && App::GetGameContext()->GetPlayerActor()->ar_sim_state != Actor::SimState::NETWORKED_OK) // we are in a vehicle
            {
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
                            App::GetGameContext()->UpdateTruckInputEvents(dt);
                        }
                        if (App::GetGameContext()->GetPlayerActor()->ar_driveable == AIRPLANE)
                        {
                            App::GetGameContext()->UpdateAirplaneInputEvents(dt);
                        }
                        if (App::GetGameContext()->GetPlayerActor()->ar_driveable == BOAT)
                        {
                            App::GetGameContext()->UpdateBoatInputEvents(dt);
                        } // if driveable == BOAT
                    } // if not in replay mode

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
