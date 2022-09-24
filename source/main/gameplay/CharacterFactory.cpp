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
#include "Character.h"
#include "GfxScene.h"
#include "Utils.h"

using namespace RoR;

CharacterFactory::CharacterFactory()
{
    // set up definitions
    // NOTE each anim is evaluated separately, there is no either-or relation,
    // so you must set each anim's conditions to avoid conflicts.

    CharacterDefPtr rorbot = std::make_shared<CharacterDef>();
    rorbot->mesh_name = "character.mesh";
    rorbot->name = "Classic RORBot";

    { // driving
        CharacterAnimDef def;
        BITMASK_SET_1(def.for_situations, Character::SITUATION_DRIVING);
        def.anim_name = "Driving";
        def.playback_time_ratio = 0.f;
        def.playback_steering_ratio = -1.f;
        def.anim_continuous = false;
        def.source_percentual = true;
        def.anim_neutral_mid = true;
        def.playback_trim = 0.01f;
        rorbot->anims.push_back(def);
    }

    { // swimming
        CharacterAnimDef def;
        BITMASK_SET_1(def.for_situations, Character::SITUATION_IN_DEEP_WATER);
        BITMASK_SET_1(def.for_actions, Character::ACTION_MOVE_FORWARD);
        def.anim_name = "Swim_loop";
        def.playback_h_speed_ratio = 1.f;
        def.playback_time_ratio = 1.f;
        rorbot->anims.push_back(def);
    }

    { // floating in water
        CharacterAnimDef def;
        BITMASK_SET_1(def.for_situations, Character::SITUATION_IN_DEEP_WATER);
        BITMASK_SET_1(def.except_actions, Character::ACTION_MOVE_FORWARD);
        def.anim_name = "Spot_swim";
        def.playback_time_ratio = 1.f;
        rorbot->anims.push_back(def);
    }

    { // running
        CharacterAnimDef def;
        BITMASK_SET_1(def.except_situations, Character::SITUATION_IN_DEEP_WATER);
        BITMASK_SET_1(def.except_situations, Character::SITUATION_DRIVING);
        BITMASK_SET_1(def.for_actions, Character::ACTION_MOVE_FORWARD);
        BITMASK_SET_1(def.for_actions, Character::ACTION_RUN);
        def.anim_name = "Run";
        def.playback_time_ratio = 1.f;
        def.playback_h_speed_ratio = 1.f;
        rorbot->anims.push_back(def);
    }

    { // walking forward
        CharacterAnimDef def;
        BITMASK_SET_1(def.except_situations, Character::SITUATION_IN_DEEP_WATER);
        BITMASK_SET_1(def.except_situations, Character::SITUATION_DRIVING);
        BITMASK_SET_1(def.for_actions, Character::ACTION_MOVE_FORWARD);
        BITMASK_SET_1(def.except_actions, Character::ACTION_RUN);
        def.anim_name = "Walk";
        def.playback_time_ratio = 1.f;
        def.playback_h_speed_ratio = 1.f;
        rorbot->anims.push_back(def);
    }

    { // walking backward
        CharacterAnimDef def;
        BITMASK_SET_1(def.except_situations, Character::SITUATION_IN_DEEP_WATER);
        BITMASK_SET_1(def.except_situations, Character::SITUATION_DRIVING);
        BITMASK_SET_1(def.for_actions, Character::ACTION_MOVE_BACKWARD);
        def.anim_name = "Walk";
        def.playback_time_ratio = -1.f;
        def.playback_h_speed_ratio = 1.f;
        rorbot->anims.push_back(def);
    }

    { // side stepping right (+time)
        CharacterAnimDef def;
        BITMASK_SET_1(def.except_situations, Character::SITUATION_IN_DEEP_WATER);
        BITMASK_SET_1(def.except_situations, Character::SITUATION_DRIVING);
        BITMASK_SET_1(def.for_actions, Character::ACTION_SIDESTEP_RIGHT);
        BITMASK_SET_1(def.except_actions, Character::ACTION_MOVE_FORWARD);
        BITMASK_SET_1(def.except_actions, Character::ACTION_MOVE_BACKWARD);
        BITMASK_SET_1(def.except_actions, Character::ACTION_TURN_RIGHT);
        BITMASK_SET_1(def.except_actions, Character::ACTION_TURN_LEFT);
        BITMASK_SET_1(def.except_actions, Character::ACTION_RUN);
        def.anim_name = "Side_step";
        def.playback_time_ratio = 1.f;
        def.anim_autorestart = true;
        rorbot->anims.push_back(def);
    }

    { // side stepping left (-time)
        CharacterAnimDef def;
        BITMASK_SET_1(def.except_situations, Character::SITUATION_IN_DEEP_WATER);
        BITMASK_SET_1(def.except_situations, Character::SITUATION_DRIVING);
        BITMASK_SET_1(def.for_actions, Character::ACTION_SIDESTEP_LEFT);
        BITMASK_SET_1(def.except_actions, Character::ACTION_MOVE_FORWARD);
        BITMASK_SET_1(def.except_actions, Character::ACTION_MOVE_BACKWARD);
        BITMASK_SET_1(def.except_actions, Character::ACTION_TURN_RIGHT);
        BITMASK_SET_1(def.except_actions, Character::ACTION_TURN_LEFT);
        BITMASK_SET_1(def.except_actions, Character::ACTION_RUN);
        def.anim_name = "Side_step";
        def.playback_time_ratio = -1.f;
        def.anim_autorestart = true;
        rorbot->anims.push_back(def);
    }

    { // turning left (+time)
        CharacterAnimDef def;
        BITMASK_SET_1(def.except_situations, Character::SITUATION_IN_DEEP_WATER);
        BITMASK_SET_1(def.except_situations, Character::SITUATION_DRIVING);
        BITMASK_SET_1(def.for_actions, Character::ACTION_TURN_LEFT);
        BITMASK_SET_1(def.except_actions, Character::ACTION_SIDESTEP_LEFT);
        BITMASK_SET_1(def.except_actions, Character::ACTION_SIDESTEP_RIGHT);
        BITMASK_SET_1(def.except_actions, Character::ACTION_MOVE_FORWARD);
        BITMASK_SET_1(def.except_actions, Character::ACTION_MOVE_BACKWARD);
        BITMASK_SET_1(def.except_actions, Character::ACTION_RUN);
        def.anim_name = "Turn";
        def.playback_time_ratio = 1.f;
        def.anim_autorestart = true;
        rorbot->anims.push_back(def);
    }

    { // turning right (-time)
        CharacterAnimDef def;
        BITMASK_SET_1(def.except_situations, Character::SITUATION_IN_DEEP_WATER);
        BITMASK_SET_1(def.except_situations, Character::SITUATION_DRIVING);
        BITMASK_SET_1(def.for_actions, Character::ACTION_TURN_RIGHT);
        BITMASK_SET_1(def.except_actions, Character::ACTION_SIDESTEP_LEFT);
        BITMASK_SET_1(def.except_actions, Character::ACTION_SIDESTEP_RIGHT);
        BITMASK_SET_1(def.except_actions, Character::ACTION_MOVE_FORWARD);
        BITMASK_SET_1(def.except_actions, Character::ACTION_MOVE_BACKWARD);
        BITMASK_SET_1(def.except_actions, Character::ACTION_RUN);
        def.anim_name = "Turn";
        def.playback_time_ratio = -1.f;
        def.anim_autorestart = true;
        rorbot->anims.push_back(def);
    }

    { // idle
        CharacterAnimDef def;
        BITMASK_SET_1(def.except_situations, Character::SITUATION_IN_DEEP_WATER);
        BITMASK_SET_1(def.except_situations, Character::SITUATION_DRIVING);
        BITMASK_SET_1(def.except_actions, Character::ACTION_TURN_RIGHT);
        BITMASK_SET_1(def.except_actions, Character::ACTION_TURN_LEFT);
        BITMASK_SET_1(def.except_actions, Character::ACTION_SIDESTEP_LEFT);
        BITMASK_SET_1(def.except_actions, Character::ACTION_SIDESTEP_RIGHT);
        BITMASK_SET_1(def.except_actions, Character::ACTION_MOVE_FORWARD);
        BITMASK_SET_1(def.except_actions, Character::ACTION_MOVE_BACKWARD);
        BITMASK_SET_1(def.except_actions, Character::ACTION_RUN);
        def.anim_name = "Idle_sway";
        def.playback_time_ratio = 1.f;
        rorbot->anims.push_back(def);
    }

    m_character_defs.push_back(rorbot);
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

    m_local_character = std::unique_ptr<Character>(new Character(m_character_defs[0], -1, 0, playerName, colourNum, false));
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

    LOG(" new character for " + TOSTRING(sourceid) + ":" + TOSTRING(streamid) + ", colour: " + TOSTRING(colour));

    Character* ch = new Character(m_character_defs[0], sourceid, streamid, name, colour, true);
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
