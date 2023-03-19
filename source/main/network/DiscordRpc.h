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

#pragma once

namespace RoR {

/// @addtogroup Network
/// @{

#ifdef USE_DISCORD_RPC
struct DiscordState {
    discord::User currentUser;
    std::string requestedServer;
    std::unique_ptr<discord::Core> core;
};
#endif

/// Wrapper class, just for code consistency with other subsystems
class DiscordRpc
{
public:
    void Init();
    void UpdatePresence();
    void RunCallbacks();
    void Shutdown();

  private:
#ifdef USE_DISCORD_RPC
    DiscordState state{};
#endif
};
/// @}   //addtogroup Network

} // namespace RoR