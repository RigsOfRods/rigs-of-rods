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

#include "Actor.h"
#include "Application.h"
#include "CacheSystem.h"
#include "Character.h"
#include "Console.h"
#include "GfxScene.h"
#include "Utils.h"

using namespace RoR;

CharacterFactory::CharacterFactory()
{
}

CharacterDocumentPtr CharacterFactory::FetchCharacterDef(CacheEntryPtr cache_entry)
{
    if (!cache_entry->character_def)
    {
        try
        {
            Ogre::DataStreamPtr datastream = Ogre::ResourceGroupManager::getSingleton().openResource(cache_entry->fname, cache_entry->resource_group);
            CharacterParser character_parser;
            cache_entry->character_def = character_parser.ProcessOgreStream(datastream);
        }
        catch (Ogre::Exception& eeh)
        {
            App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_ERROR,
                fmt::format("Could not load character, message: {}", eeh.getFullDescription()));
        }
    }

    return cache_entry->character_def;
}

Character* CharacterFactory::CreateLocalCharacter()
{
    int colourNum = -1;
    Ogre::UTFString playerName = "";

#ifdef USE_SOCKETW
    if (App::mp_state->getEnum<MpState>() == MpState::CONNECTED)
    {
        RoRnet::UserInfo info = App::GetNetwork()->GetLocalUserData();
        colourNum = info.colournum;
        playerName = tryConvertUTF(info.username);
    }
#endif // USE_SOCKETW

    CacheEntryPtr cache_entry = App::GetCacheSystem()->FindEntryByFilename(LT_Character, /*partial:*/false, App::sim_player_character->getStr());
    if (!cache_entry)
    {
        App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_ERROR,
            fmt::format("Could not find character '{}' in mod cache.", App::sim_player_character->getStr()));
        return nullptr;
    }

    CharacterDocumentPtr document = this->FetchCharacterDef(cache_entry);
    if (!document)
    {
        return nullptr; // Error already reported
    }

    m_local_character = std::unique_ptr<Character>(new Character(document, -1, 0, playerName, colourNum, false));
    App::GetGfxScene()->RegisterGfxCharacter(m_local_character.get());
    return m_local_character.get();
}

void CharacterFactory::createRemoteInstance(int sourceid, int streamid)
{
#ifdef USE_SOCKETW
    RoRnet::UserInfo info;
    App::GetNetwork()->GetUserInfo(sourceid, info);
    int colour = info.colournum;
    Ogre::UTFString name = tryConvertUTF(info.username);

    std::string info_str = fmt::format("player '{}' ({}:{}), colour: {}", info.clientname, sourceid, streamid, colour);

    LOG(" new character for " + info_str);

    std::string filename = App::sim_player_character->getStr(); // TBD: transmit and use the actual character used by the player

    CacheEntryPtr cache_entry = App::GetCacheSystem()->FindEntryByFilename(LT_Character, /*partial:*/false, filename);
    if (!cache_entry)
    {
        App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_ERROR,
            fmt::format("Could not create character for {} - character '{}' not found in mod cache.", info_str, filename));
        return;
    }

    CharacterDocumentPtr document = this->FetchCharacterDef(cache_entry);
    if (!document)
    {
        App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_ERROR,
            fmt::format("Could not create character for {} - cannot load file '{}'.", info_str, cache_entry->fname));
        return;
    }

    Character* ch = new Character(document, sourceid, streamid, name, colour, true);
    App::GetGfxScene()->RegisterGfxCharacter(ch);
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

void CharacterFactory::Update(float dt)
{
    m_local_character->updateLocal(dt);

#ifdef USE_SOCKETW
    if ((App::mp_state->getEnum<MpState>() == MpState::CONNECTED) && !m_local_character->isRemote())
    {
        m_local_character->SendStreamData();
    }
#endif // USE_SOCKETW


}

void CharacterFactory::UndoRemoteActorCoupling(ActorPtr actor)
{
    for (auto& c : m_remote_characters)
    {
        if (c->GetActorCoupling() == actor)
        {
            c->SetActorCoupling(false, nullptr);
        }
    }
}

void CharacterFactory::DeleteAllCharacters()
{
    m_remote_characters.clear(); // std::unique_ptr<> will do the cleanup...
    m_local_character.reset(); // ditto
}

#ifdef USE_SOCKETW
void CharacterFactory::handleStreamData(std::vector<RoR::NetRecvPacket> packet_buffer)
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
