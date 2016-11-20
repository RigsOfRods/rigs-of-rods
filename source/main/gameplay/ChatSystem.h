/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2013-2016 Petr Ohlidal

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

#include "RoRPrerequisites.h"

#include "Network.h"

namespace RoR {
namespace ChatSystem {

void SendChat(Ogre::UTFString chatline);
void SendPrivateChat(Ogre::UTFString target_username, Ogre::UTFString chatline);

void SendStreamSetup();

#ifdef USE_SOCKETW
void HandleStreamData(std::vector<RoR::Networking::recv_packet_t> packet);
#endif // USE_SOCKETW

Ogre::UTFString GetColouredName(Ogre::UTFString nick, int colour_number);

} // namespace Chatsystem
} // namespace RoR
