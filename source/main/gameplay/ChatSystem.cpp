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

#include "ChatSystem.h"

#include "Actor.h"
#include "Console.h"
#include "GUIManager.h"
#include "Language.h"
#include "Utils.h"

namespace RoR {
namespace ChatSystem {

using namespace RoRnet;

#ifdef USE_SOCKETW
void SendStreamSetup()
{
    RoRnet::StreamRegister reg;
    memset(&reg, 0, sizeof(RoRnet::StreamRegister));
    reg.status = 1;
    strcpy(reg.name, "chat");
    reg.type = 3;

    App::GetNetwork()->AddLocalStream(&reg, sizeof(RoRnet::StreamRegister));
}
#endif // USE_SOCKETW

#ifdef USE_SOCKETW
void ReceiveStreamData(unsigned int type, int source, char* buffer)
{
    if (type != MSG2_UTF8_CHAT && type != MSG2_UTF8_PRIVCHAT)
        return;

    std::string text = SanitizeUtf8CString(buffer);
    if (type == MSG2_UTF8_PRIVCHAT)
    {
        text = _L(" [whispered] ") + text;
    }

    App::GetConsole()->putNetMessage(source, Console::CONSOLE_SYSTEM_NETCHAT, text.c_str());
}
#endif // USE_SOCKETW

#ifdef USE_SOCKETW
void HandleStreamData(std::vector<RoR::NetRecvPacket> packet_buffer)
{
    for (auto packet : packet_buffer)
    {
        ReceiveStreamData(packet.header.command, packet.header.source, packet.buffer);
    }
}
#endif // USE_SOCKETW

} // namespace ChatSystem
} // namespace RoR
