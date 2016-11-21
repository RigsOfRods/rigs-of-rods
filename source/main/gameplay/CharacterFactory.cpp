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

#include "CharacterFactory.h"

#include "Character.h"

Character* CharacterFactory::createLocal(int playerColour)
{
    Character* ch = new Character(-1, 0, playerColour, false);
    return ch;
}

void CharacterFactory::createRemoteInstance(int sourceid, int streamid)
{
#ifdef USE_SOCKETW
    user_info_t info;
    RoR::Networking::GetUserInfo(sourceid, info);
    int colour = info.colournum;

    LOG(" new character for " + TOSTRING(sourceid) + ":" + TOSTRING(streamid) + ", colour: " + TOSTRING(colour));

    m_characters.push_back(std::unique_ptr<Character>(new Character(sourceid, streamid, colour, true)));
#endif // USE_SOCKETW
}

void CharacterFactory::removeStreamSource(int sourceid)
{
    for (auto it = m_characters.begin(); it != m_characters.end(); it++)
    {
        if ((*it)->getSourceID() == sourceid)
        {
            (*it).reset();
            m_characters.erase(it);
            break;
        }
    }
}

void CharacterFactory::update(float dt)
{
    gEnv->player->update(dt);
    gEnv->player->updateLabels();

    for (auto& c : m_characters)
    {
        c->update(dt);
        c->updateLabels();
    }
}

#ifdef USE_SOCKETW
void CharacterFactory::handleStreamData(std::vector<RoR::Networking::recv_packet_t> packet_buffer)
{
    for (auto packet : packet_buffer)
    {
        if (packet.header.command == MSG2_STREAM_REGISTER)
        {
            stream_register_t* reg = (stream_register_t *)packet.buffer;
            if (reg->type == 1)
            {
                createRemoteInstance(packet.header.source, packet.header.streamid);
            }
        }
        else if (packet.header.command == MSG2_USER_LEAVE)
        {
            removeStreamSource(packet.header.source);
        }
        else
        {
            for (auto& c : m_characters)
            {
                c->receiveStreamData(packet.header.command, packet.header.source, packet.header.streamid, packet.buffer);
            }
        }
    }
}
#endif // USE_SOCKETW
