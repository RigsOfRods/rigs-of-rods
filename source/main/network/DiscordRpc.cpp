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
#include "GameContext.h"
#include "Network.h"
#include <discord.h>
#endif

using namespace RoR;

size_t CurlWriteFunc(void *ptr, size_t size, size_t nmemb, std::string* data);

void DiscordRpc::Init()
{
#ifdef USE_DISCORD_RPC
    if(App::io_discord_rpc->getBool())
    {
        discord::Core* core{};
        auto result = discord::Core::Create(532207219509559326, DiscordCreateFlags_Default, &core);
        state.core.reset(core);
        if (!state.core) {
            LogFormat("[Discord] Failed to instantiate discord core! (err %i)", static_cast<int>(result));
            return;
        }
        state.core->SetLogHook(
            discord::LogLevel::Debug, [](discord::LogLevel level, const char* message) {
              LogFormat("[Discord](%i): %s", static_cast<uint32_t>(level), message);
            });

        state.core->ActivityManager().RegisterCommand("startror://run");

        state.core->ActivityManager().OnActivityJoin.Connect(
            [this](const std::string& server) {
                LogFormat("[Discord] Atemping to join server %s", server.c_str());
                if(state.requestedServer == server)
                    return;
                const int colon = static_cast<int>(server.rfind(":"));
                if (colon != std::string::npos)
                {
                    state.requestedServer = server;
                    App::mp_server_host->setStr(server.substr(0, colon));
                    App::mp_server_port->setVal(Ogre::StringConverter::parseInt(server.substr(colon + 1, server.length())));
                    App::GetGameContext()->PushMessage(Message(MSG_NET_CONNECT_REQUESTED));
                }
            });
        state.core->ActivityManager().OnActivityJoinRequest.Connect([](discord::User const& user) {
            LogFormat("Join Request %s", user.GetUsername());
        });
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
            std::string serv_name = App::GetNetwork()->GetServerName();
            activity.SetState("Playing online");
            details = fmt::format("On server: {}", serv_name.c_str());

            std::string serverlist_url = App::mp_api_url->getStr() + "/server-list?json=true";
            std::string response_payload;
            std::string response_header;
            long        response_code = 0;

            CURL *curl = curl_easy_init();
            curl_easy_setopt(curl, CURLOPT_URL,           serverlist_url.c_str());
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, CurlWriteFunc);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA,     &response_payload);
            curl_easy_setopt(curl, CURLOPT_HEADERDATA,    &response_header);

            curl_easy_perform(curl);
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);

            curl_easy_cleanup(curl);
            curl = nullptr;

            if (response_code != 200)
            {
                LogFormat( "[Discord] Failed to retrieve serverlist; HTTP status code: %i", response_code);
                return;
            }

            rapidjson::Document j_data_doc;
            j_data_doc.Parse(response_payload.c_str());
            if (j_data_doc.HasParseError() || !j_data_doc.IsArray())
            {
                Log("[Discord] Error parsing serverlist JSON");
                return;
            }

            rapidjson::Value serv;

            for (auto& val : j_data_doc.GetArray())
            {
                if(val["name"].GetString() == serv_name)
                {
                    serv = val;
                    break ;
                }
            }
            if(!serv.Empty())
            {
                activity.GetParty().GetSize().SetMaxSize(serv["max-clients"].GetInt());
                activity.GetParty().GetSize().SetCurrentSize(serv["current-users"].GetInt());
                auto server = fmt::format("{}:{}",RoR::App::mp_server_host->getStr(), RoR::App::mp_server_port->getInt());
                activity.GetParty().SetId(serv_name.c_str());
                activity.GetSecrets().SetJoin(server.c_str());
                //activity.SetInstance(true);
            }
        }
        else
        {
            activity.SetState("Playing singleplayer");
            details = fmt::format("On terrain: {}", RoR::App::sim_terrain_gui_name->getStr());
        }
        activity.SetDetails(details.c_str());
        activity.GetAssets().SetLargeImage("ror_logo_2");
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