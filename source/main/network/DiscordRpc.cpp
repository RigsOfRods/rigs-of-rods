/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2013-2020 Petr Ohlidal

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

#include "DiscordRpc.h"

#include "AppContext.h"

#ifdef USE_DISCORD_RPC
#include <discord.h>
#include "Network.h"
#endif

using namespace RoR;

void DiscordRpc::Init()
{
#ifdef USE_DISCORD_RPC
    if(App::io_discord_rpc->getBool())
    {
        discord::Core* core{};
        auto result = discord::Core::Create(492484203435393035, DiscordCreateFlags_Default, &core);
        state.core.reset(core);
        if (!state.core) {
            LogFormat("[Discord] Failed to instantiate discord core! (err %i)", static_cast<int>(result));
            return;
        }
        state.core->SetLogHook(
            discord::LogLevel::Debug, [](discord::LogLevel level, const char* message) {
              LogFormat("[Discord](%i): %s", static_cast<uint32_t>(level), message);
            });

        state.core->ActivityManager().RegisterCommand("RoR -joinserver=");

        state.core->ActivityManager().OnActivityInvite.Connect(
            [](discord::ActivityActionType t, discord::User const& user, discord::Activity const& ac) {
              LogFormat("[Discord]: Invite %i %s", static_cast<int>(t), user.GetUsername());
            });
        state.core->ActivityManager().OnActivityJoin.Connect(
            [](const char* secret) {  LogFormat("[Discord]: %s", secret); });
    }
#endif
}

void DiscordRpc::UpdatePresence()
{
#ifdef USE_DISCORD_RPC
    if(App::io_discord_rpc->getBool() && state.core)
    {
        discord::Activity activity{};
        std::string details;

        if (App::mp_state->getEnum<MpState>() == MpState::CONNECTED)
        {
            activity.SetState("Playing online");
            details = fmt::format("On server: {}:{}  on terrain: {}",
                    RoR::App::mp_server_host->getStr(),
                    RoR::App::mp_server_port->getInt(),
                    RoR::App::sim_terrain_gui_name->getStr()
            );
            auto server = fmt::format("{}:{}",RoR::App::mp_server_host->getStr(), RoR::App::mp_server_port->getInt());

            activity.GetParty().SetId(App::GetNetwork()->GetServerName().c_str());
            activity.GetParty().GetSize().SetMaxSize(16);
            activity.GetParty().GetSize().SetCurrentSize(2);
            activity.GetSecrets().SetJoin(server.c_str());
            activity.SetInstance(true);
        }
        else
        {
            activity.SetState("Playing singleplayer");
            details = fmt::format("On terrain: {}", RoR::App::sim_terrain_gui_name->getStr());
        }
        activity.SetDetails(details.c_str());
        activity.GetAssets().SetLargeImage("ror_logo_t");
        activity.GetAssets().SetLargeText("Rigs of Rods");
        activity.SetType(discord::ActivityType::Playing);
        state.core->ActivityManager().UpdateActivity(activity, [](discord::Result result) {
          LogFormat("[Discord] %s updating activity! (%i)", ((result == discord::Result::Ok) ? "Succeeded" : "Failed"), static_cast<int>(result));
        });
    }
#endif
}

void DiscordRpc::RunCallbacks()
{
#ifdef USE_DISCORD_RPC
  if (state.core)
    state.core->RunCallbacks();
#endif
}

void DiscordRpc::Shutdown()
{
#ifdef USE_DISCORD_RPC
    state.core.reset();
#endif
}