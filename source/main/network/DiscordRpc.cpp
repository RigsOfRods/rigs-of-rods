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
#include <discord_rpc.h>
#endif

using namespace RoR;

#ifdef USE_DISCORD_RPC
void DiscordErrorCallback(int, const char *error)
{
    RoR::LogFormat("Discord Error: %s", error);
}

void DiscordReadyCallback(const DiscordUser *user)
{
    RoR::LogFormat("Discord Ready: %s", user->username);
}
#endif

void DiscordRpc::Init()
{
#ifdef USE_DISCORD_RPC
    if(App::io_discord_rpc->getBool())
    {
        DiscordEventHandlers handlers;
        memset(&handlers, 0, sizeof(handlers));
        handlers.ready = DiscordReadyCallback;
        handlers.errored = DiscordErrorCallback;

        // Discord_Initialize(const char* applicationId, DiscordEventHandlers* handlers, int autoRegister, const char* optionalSteamId)
        Discord_Initialize("492484203435393035", &handlers, 1, "1234");
    }
#endif
}

void DiscordRpc::UpdatePresence()
{
#ifdef USE_DISCORD_RPC
    if(App::io_discord_rpc->getBool())
    {
        char buffer[256];
        DiscordRichPresence discordPresence;
        memset(&discordPresence, 0, sizeof(discordPresence));
        if (App::mp_state->getEnum<MpState>() == MpState::CONNECTED)
        {
            discordPresence.state = "Playing online";
            sprintf(buffer, "On server: %s:%d  on terrain: %s",
                    RoR::App::mp_server_host->getStr().c_str(),
                    RoR::App::mp_server_port->getInt(),
                    RoR::App::sim_terrain_gui_name->getStr().c_str());
        }
        else
        {
            discordPresence.state = "Playing singleplayer";
            sprintf(buffer, "On terrain: %s", RoR::App::sim_terrain_gui_name->getStr().c_str());
        }
        discordPresence.details = buffer;
        discordPresence.startTimestamp = time(0);
        discordPresence.largeImageKey = "ror_logo_t";
        discordPresence.largeImageText = "Rigs of Rods";
        Discord_UpdatePresence(&discordPresence);
    }
#endif
}

void DiscordRpc::Shutdown()
{
#ifdef USE_DISCORD_RPC
    Discord_Shutdown();
#endif
}