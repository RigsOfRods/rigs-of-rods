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
#include "RoRFrameListener.h" // SimController
#include "Utils.h"

using namespace RoR;

Character* CharacterFactory::createLocal(Ogre::UTFString playerName, int playerColour)
{
    Character* ch = new Character(-1, 0, playerName, playerColour, false);
    App::GetSimController()->GetGfxScene().RegisterGfxCharacter(ch->SetupGfx());
    return ch;
}

void CharacterFactory::createRemoteInstance(int sourceid, int streamid)
{
#ifdef USE_SOCKETW
    RoRnet::UserInfo info;
    RoR::Networking::GetUserInfo(sourceid, info);
    int colour = info.colournum;
    Ogre::UTFString name = tryConvertUTF(info.username);

    LOG(" new character for " + TOSTRING(sourceid) + ":" + TOSTRING(streamid) + ", colour: " + TOSTRING(colour));

    Character* ch = new Character(sourceid, streamid, name, colour, true);
    App::GetSimController()->GetGfxScene().RegisterGfxCharacter(ch->SetupGfx());
    m_remote_characters.push_back(std::unique_ptr<Character>(ch));
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

    for (auto& c : m_remote_characters)
    {
        c->update(dt);
    }
}

void CharacterFactory::UndoRemoteActorCoupling(Actor* actor)
{
    for (auto& c : m_remote_characters)
    {
        if (c->GetActorCoupling() == actor)
        {
            c->SetActorCoupling(false, nullptr);
        }
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
