/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2013-2018 Petr Ohlidal

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

#include "CharacterFactory.h"

#include "Application.h"
#include "Character.h"

using namespace RoR;

Character* CharacterFactory::createLocal(int playerColour)
{
    Character* ch = new Character(-1, 0, playerColour, false);
    return ch;
}

void CharacterFactory::createRemoteInstance(int sourceid, int streamid)
{
#ifdef USE_SOCKETW
    RoRnet::UserInfo info;
    RoR::Networking::GetUserInfo(sourceid, info);
    int colour = info.colournum;

    LOG(" new character for " + TOSTRING(sourceid) + ":" + TOSTRING(streamid) + ", colour: " + TOSTRING(colour));

    m_remote_characters.push_back(std::unique_ptr<Character>(new Character(sourceid, streamid, colour, true)));
#endif // USE_SOCKETW
}

void CharacterFactory::removeStreamSource(int sourceid)
{
    for (auto it = m_remote_characters.begin(); it != m_remote_characters.end(); it++)
    {
        if ((*it)->getSourceID() == sourceid)
        {
            (*it).reset();
            m_remote_characters.erase(it);
            return;
        }
    }
}

void CharacterFactory::update(float dt)
{
    gEnv->player->update(dt);
    gEnv->player->updateLabels();

    for (auto& c : m_remote_characters)
    {
        c->update(dt);
        c->updateLabels();
    }
}

void CharacterFactory::DeleteAllRemoteCharacters()
{
    m_remote_characters.clear(); // std::unique_ptr<> will do the cleanup...
}

#ifdef USE_SOCKETW
void CharacterFactory::handleStreamData(std::vector<RoR::Networking::recv_packet_t> packet_buffer)
{
    for (auto packet : packet_buffer)
    {
        if (packet.header.command == RoRnet::MSG2_STREAM_REGISTER)
        {
            RoRnet::StreamRegister* reg = (RoRnet::StreamRegister *)packet.buffer;
            if (reg->type == 1)
            {
                createRemoteInstance(packet.header.source, packet.header.streamid);
            }
        }
        else if (packet.header.command == RoRnet::MSG2_USER_LEAVE)
        {
            removeStreamSource(packet.header.source);
        }
        else
        {
            for (auto& c : m_remote_characters)
            {
                c->receiveStreamData(packet.header.command, packet.header.source, packet.header.streamid, packet.buffer);
            }
        }
    }
}
#endif // USE_SOCKETW
