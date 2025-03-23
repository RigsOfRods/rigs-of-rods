/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2017-2018 Petr Ohlidal

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

#include "Character.h"

#include "Application.h"
#include "Actor.h"
#include "ActorManager.h"
#include "CameraManager.h"
#include "Collisions.h"
#include "GameContext.h"
#include "GfxScene.h"
#include "InputEngine.h"
#include "MovableText.h"
#include "Network.h"
#include "Terrain.h"
#include "Utils.h"
#include "Water.h"

using namespace Ogre;
using namespace RoR;

#define LOGSTREAM Ogre::LogManager::getSingleton().stream()

Character::Character(int source, unsigned int streamid, UTFString player_name, int color_number, bool is_remote) :
      m_actor_coupling(nullptr)
    , m_can_jump(false)
    , m_character_rotation(0.0f)
    , m_character_h_speed(2.0f)
    , m_character_v_speed(0.0f)
    , m_color_number(color_number)
    , m_anim_time(0.f)
    , m_net_last_anim_time(0.f)
    , m_net_last_update_time(0.f)
    , m_net_username(player_name)
    , m_is_remote(is_remote)
    , m_source_id(source)
    , m_stream_id(streamid)
    , m_gfx_character(nullptr)
    , m_driving_anim_length(0.f)
    , m_anim_name("Idle_sway")
{
    static int id_counter = 0;
    m_instance_name = "Character#" + TOSTRING(id_counter);
    ++id_counter;

    if (App::mp_state->getEnum<MpState>() == MpState::CONNECTED)
    {
        this->SendStreamSetup();
    }
}

Character::~Character()
{
    if (m_gfx_character != nullptr)
    {
        App::GetGfxScene()->RemoveGfxCharacter(m_gfx_character);
        delete m_gfx_character;
    }
}

void Character::updateCharacterRotation()
{
    setRotation(m_character_rotation);
}

void Character::setPosition(Vector3 position) // TODO: updates OGRE objects --> belongs to GfxScene ~ only_a_ptr, 05/2018
{
    //ASYNCSCENE OLD m_character_scenenode->setPosition(position);
    m_character_position = position;
    m_prev_position = position;
}

Vector3 Character::getPosition()
{
    //ASYNCSCENE OLDreturn m_character_scenenode->getPosition();
    return m_character_position;
}

void Character::setRotation(Radian rotation)
{
    m_character_rotation = rotation;
}

void Character::SetAnimState(std::string mode, float time)
{
    if (m_anim_name != mode)
    {
        m_anim_name = mode;
        m_anim_time = time;
        m_net_last_anim_time = 0.0f;
    }
    else
    {
        m_anim_time += time;
    }
}

float calculate_collision_depth(Vector3 pos)
{
    Vector3 query = pos + 0.3f * Vector3::UNIT_Y;
    while (query.y > pos.y)
    {
        if (App::GetGameContext()->GetTerrain()->GetCollisions()->collisionCorrect(&query, false))
            break;
        query.y -= 0.001f;
    }
    return query.y - pos.y;
}

void Character::update(float dt)
{
    if (!m_is_remote && (m_actor_coupling == nullptr) && (App::sim_state->getEnum<SimState>() != SimState::PAUSED))
    {
        // disable character movement when using the free camera mode or when the menu is opened
        // TODO: check for menu being opened
        if (App::GetCameraManager()->GetCurrentBehavior() == CameraManager::CAMERA_BEHAVIOR_FREE)
        {
            return;
        }

        Vector3 position = m_character_position; //ASYNCSCENE OLD m_character_scenenode->getPosition();

        // gravity force is always on
        position.y += m_character_v_speed * dt;
        m_character_v_speed += dt * -9.8f;

        // Trigger script events and handle mesh (ground) collision
        Vector3 query = position;
        App::GetGameContext()->GetTerrain()->GetCollisions()->collisionCorrect(&query);

        // Auto compensate minor height differences
        float depth = calculate_collision_depth(position);
        if (depth > 0.0f)
        {
            m_can_jump = true;
            m_character_v_speed = std::max(0.0f, m_character_v_speed);
            position.y += std::min(depth, 2.0f * dt);
        }

        // Submesh "collision"
        {
            float depth = 0.0f;
            for (ActorPtr& actor : App::GetGameContext()->GetActorManager()->GetActors())
            {
                if (actor->ar_bounding_box.contains(position))
                {
                    for (int i = 0; i < actor->ar_num_collcabs; i++)
                    {
                        int tmpv = actor->ar_collcabs[i] * 3;
                        Vector3 a = actor->ar_nodes[actor->ar_cabs[tmpv + 0]].AbsPosition;
                        Vector3 b = actor->ar_nodes[actor->ar_cabs[tmpv + 1]].AbsPosition;
                        Vector3 c = actor->ar_nodes[actor->ar_cabs[tmpv + 2]].AbsPosition;
                        auto result = Math::intersects(Ray(position, Vector3::UNIT_Y), a, b, c);
                        if (result.first && result.second < 1.8f)
                        {
                            depth = std::max(depth, result.second);
                        }
                    }
                }
            }
            if (depth > 0.0f)
            {
                m_can_jump = true;
                m_character_v_speed = std::max(0.0f, m_character_v_speed);
                position.y += std::min(depth, 0.05f);
            }
        }

        // Obstacle detection
        if (position != m_prev_position)
        {
            const int numstep = 100;
            Vector3 diff = position - m_prev_position;
            Vector3 base = m_prev_position + Vector3::UNIT_Y * 0.25f;
            for (int i = 1; i < numstep; i++)
            {
                Vector3 query = base + diff * ((float)i / numstep);
                if (App::GetGameContext()->GetTerrain()->GetCollisions()->collisionCorrect(&query, false))
                {
                    m_character_v_speed = std::max(0.0f, m_character_v_speed);
                    position = m_prev_position + diff * ((float)(i - 1) / numstep);
                    position.y += 0.025f;
                    break;
                }
            }
        }

        m_prev_position = position;

        // ground contact
        float pheight = App::GetGameContext()->GetTerrain()->getHeightAt(position.x, position.z);

        if (position.y < pheight)
        {
            position.y = pheight;
            m_character_v_speed = 0.0f;
            m_can_jump = true;
        }

        // water stuff
        bool isswimming = false;
        float wheight = -99999;

        if (App::GetGameContext()->GetTerrain()->getWater())
        {
            wheight = App::GetGameContext()->GetTerrain()->getWater()->CalcWavesHeight(position);
            if (position.y < wheight - 1.8f)
            {
                position.y = wheight - 1.8f;
                m_character_v_speed = 0.0f;
            }
        }

        // 0.1 due to 'jumping' from waves -> not nice looking
        if (App::GetGameContext()->GetTerrain()->getWater() && (wheight - pheight > 1.8f) && (position.y + 0.1f <= wheight))
        {
            isswimming = true;
        }

        float tmpJoy = 0.0f;
        if (m_can_jump)
        {
            if (RoR::App::GetInputEngine()->getEventBoolValue(EV_CHARACTER_JUMP))
            {
                m_character_v_speed = 2.0f;
                m_can_jump = false;
            }
        }

        bool idleanim = true;
        float tmpGoForward = RoR::App::GetInputEngine()->getEventValue(EV_CHARACTER_FORWARD)
                             + RoR::App::GetInputEngine()->getEventValue(EV_CHARACTER_ROT_UP);
        float tmpGoBackward = RoR::App::GetInputEngine()->getEventValue(EV_CHARACTER_BACKWARDS)
                             + RoR::App::GetInputEngine()->getEventValue(EV_CHARACTER_ROT_DOWN);
        bool not_walking = (tmpGoForward == 0.f && tmpGoBackward == 0.f);

        tmpJoy = RoR::App::GetInputEngine()->getEventValue(EV_CHARACTER_RIGHT);
        if (tmpJoy > 0.0f)
        {
            float scale = RoR::App::GetInputEngine()->isKeyDown(OIS::KC_LMENU) ? 0.1f : 1.0f;
            setRotation(m_character_rotation + dt * 2.0f * scale * Radian(tmpJoy));
            if (!isswimming && not_walking)
            {
                this->SetAnimState("Turn", -dt);
                idleanim = false;
            }
        }

        tmpJoy = RoR::App::GetInputEngine()->getEventValue(EV_CHARACTER_LEFT);
        if (tmpJoy > 0.0f)
        {
            float scale = RoR::App::GetInputEngine()->isKeyDown(OIS::KC_LMENU) ? 0.1f : 1.0f;
            setRotation(m_character_rotation - dt * scale * 2.0f * Radian(tmpJoy));
            if (!isswimming && not_walking)
            {
                this->SetAnimState("Turn", dt);
                idleanim = false;
            }
        }

        float tmpRun = RoR::App::GetInputEngine()->getEventValue(EV_CHARACTER_RUN);
        float accel = 1.0f;

        tmpJoy = accel = RoR::App::GetInputEngine()->getEventValue(EV_CHARACTER_SIDESTEP_LEFT);
        if (tmpJoy > 0.0f)
        {
            if (tmpRun > 0.0f)
                accel = 3.0f * tmpRun;
            // animation missing for that
            position += dt * m_character_h_speed * 0.5f * accel * Vector3(cos(m_character_rotation.valueRadians() - Math::HALF_PI), 0.0f, sin(m_character_rotation.valueRadians() - Math::HALF_PI));
            if (!isswimming && not_walking)
            {
                this->SetAnimState("Side_step", -dt);
                idleanim = false;
            }
        }

        tmpJoy = accel = RoR::App::GetInputEngine()->getEventValue(EV_CHARACTER_SIDESTEP_RIGHT);
        if (tmpJoy > 0.0f)
        {
            if (tmpRun > 0.0f)
                accel = 3.0f * tmpRun;
            // animation missing for that
            position += dt * m_character_h_speed * 0.5f * accel * Vector3(cos(m_character_rotation.valueRadians() + Math::HALF_PI), 0.0f, sin(m_character_rotation.valueRadians() + Math::HALF_PI));
            if (!isswimming && not_walking)
            {
                this->SetAnimState("Side_step", dt);
                idleanim = false;
            }
        }

        tmpJoy = accel = tmpGoForward;
        float tmpBack = tmpGoBackward;

        tmpJoy = std::min(tmpJoy, 1.0f);
        tmpBack = std::min(tmpBack, 1.0f);

        if (tmpJoy > 0.0f || tmpRun > 0.0f)
        {
            if (tmpRun > 0.0f)
                accel = 3.0f * tmpRun;

            float time = dt * tmpJoy * m_character_h_speed;

            if (isswimming)
            {
                this->SetAnimState("Swim_loop", time);
                idleanim = false;
            }
            else
            {
                if (tmpRun > 0.0f)
                {
                    this->SetAnimState("Run", time);
                    idleanim = false;
                }
                else
                {
                    this->SetAnimState("Walk", time);
                    idleanim = false;
                }
            }
            position += dt * m_character_h_speed * 1.5f * accel * Vector3(cos(m_character_rotation.valueRadians()), 0.0f, sin(m_character_rotation.valueRadians()));
        }
        else if (tmpBack > 0.0f)
        {
            float time = -dt * m_character_h_speed;
            if (isswimming)
            {
                this->SetAnimState("Spot_swim", time);
                idleanim = false;
            }
            else
            {
                this->SetAnimState("Walk", time);
                idleanim = false;
            }
            position -= dt * m_character_h_speed * tmpBack * Vector3(cos(m_character_rotation.valueRadians()), 0.0f, sin(m_character_rotation.valueRadians()));
        }

        if (idleanim)
        {
            if (isswimming)
            {
                this->SetAnimState("Spot_swim", dt * 2.0f);
            }
            else
            {
                this->SetAnimState("Idle_sway", dt * 1.0f);
            }
        }

        m_character_position = position;
    }
    else if (m_actor_coupling) // The character occupies a vehicle or machine
    {
        // Animation
        float angle = m_actor_coupling->ar_hydro_dir_wheel_display * -1.0f; // not getSteeringAngle(), but this, as its smoothed
        float anim_time_pos = ((angle + 1.0f) * 0.5f) * m_driving_anim_length;
        // prevent animation flickering on the borders:
        if (anim_time_pos < 0.01f)
        {
            anim_time_pos = 0.01f;
        }
        if (anim_time_pos > m_driving_anim_length - 0.01f)
        {
            anim_time_pos = m_driving_anim_length - 0.01f;
        }
        m_anim_name = "Driving";
        m_anim_time = anim_time_pos;
        m_net_last_anim_time = 0.0f;
    }

#ifdef USE_SOCKETW
    if ((App::mp_state->getEnum<MpState>() == MpState::CONNECTED) && !m_is_remote)
    {
        this->SendStreamData();
    }
#endif // USE_SOCKETW
}

void Character::move(Vector3 offset)
{
    m_character_position += offset;  //ASYNCSCENE OLD m_character_scenenode->translate(offset);
}

// Helper function
void Character::ReportError(const char* detail)
{
#ifdef USE_SOCKETW
    Ogre::UTFString username;
    RoRnet::UserInfo info;
    if (!App::GetNetwork()->GetUserInfo(m_source_id, info))
        username = "~~ERROR getting username~~";
    else
        username = info.username;

    char msg_buf[300];
    snprintf(msg_buf, 300,
        "[RoR|Networking] ERROR on m_is_remote character (User: '%s', SourceID: %d, StreamID: %d): ",
        username.asUTF8_c_str(), m_source_id, m_stream_id);

    LOGSTREAM << msg_buf << detail;
#endif
}

void Character::SendStreamSetup()
{
#ifdef USE_SOCKETW
    if (m_is_remote)
        return;

    RoRnet::StreamRegister reg;
    memset(&reg, 0, sizeof(reg));
    reg.status = 1;
    strcpy(reg.name, "default");
    reg.type = 1;
    reg.data[0] = 2;

    App::GetNetwork()->AddLocalStream(&reg, sizeof(RoRnet::StreamRegister));

    m_source_id = reg.origin_sourceid;
    m_stream_id = reg.origin_streamid;
#endif // USE_SOCKETW
}

void Character::SendStreamData()
{
#ifdef USE_SOCKETW
    if (m_net_timer.getMilliseconds() - m_net_last_update_time < 100)
        return;

    // do not send position data if coupled to an actor already
    if (m_actor_coupling)
        return;

    m_net_last_update_time = m_net_timer.getMilliseconds();

    NetCharacterMsgPos msg;
    msg.command = CHARACTER_CMD_POSITION;
    msg.pos_x = m_character_position.x;
    msg.pos_y = m_character_position.y;
    msg.pos_z = m_character_position.z;
    msg.rot_angle = m_character_rotation.valueRadians();
    strncpy(msg.anim_name, m_anim_name.c_str(), CHARACTER_ANIM_NAME_LEN);
    msg.anim_time = m_anim_time - m_net_last_anim_time;

    m_net_last_anim_time = m_anim_time;

    App::GetNetwork()->AddPacket(m_stream_id, RoRnet::MSG2_STREAM_DATA_DISCARDABLE, sizeof(NetCharacterMsgPos), (char*)&msg);
#endif // USE_SOCKETW
}

void Character::receiveStreamData(unsigned int& type, int& source, unsigned int& streamid, char* buffer)
{
#ifdef USE_SOCKETW
    if (type == RoRnet::MSG2_STREAM_DATA && m_source_id == source && m_stream_id == streamid)
    {
        auto* msg = reinterpret_cast<NetCharacterMsgGeneric*>(buffer);
        if (msg->command == CHARACTER_CMD_POSITION)
        {
            auto* pos_msg = reinterpret_cast<NetCharacterMsgPos*>(buffer);
            this->setPosition(Ogre::Vector3(pos_msg->pos_x, pos_msg->pos_y, pos_msg->pos_z));
            this->setRotation(Ogre::Radian(pos_msg->rot_angle));
            if (strnlen(pos_msg->anim_name, CHARACTER_ANIM_NAME_LEN) < CHARACTER_ANIM_NAME_LEN)
            {
                this->SetAnimState(pos_msg->anim_name, pos_msg->anim_time);
            }
        }
        else if (msg->command == CHARACTER_CMD_DETACH)
        {
            if (m_actor_coupling != nullptr)
                this->SetActorCoupling(false, nullptr);
            else
                this->ReportError("Received command `DETACH`, but not currently attached to a vehicle. Ignoring command.");
        }
        else if (msg->command == CHARACTER_CMD_ATTACH)
        {
            auto* attach_msg = reinterpret_cast<NetCharacterMsgAttach*>(buffer);
            ActorPtr beam = App::GetGameContext()->GetActorManager()->GetActorByNetworkLinks(attach_msg->source_id, attach_msg->stream_id);
            if (beam != nullptr)
            {
                this->SetActorCoupling(true, beam);
            }
            else
            {
                char err_buf[200];
                snprintf(err_buf, 200, "Received command `ATTACH` with target{SourceID: %d, StreamID: %d}, "
                    "but corresponding vehicle doesn't exist. Ignoring command.",
                    attach_msg->source_id, attach_msg->stream_id);
                this->ReportError(err_buf);
            }
        }
        else
        {
            char err_buf[100];
            snprintf(err_buf, 100, "Received invalid command: %d. Cannot process.", msg->command);
            this->ReportError(err_buf);
        }
    }
#endif
}

void Character::SetActorCoupling(bool enabled, ActorPtr actor)
{
    m_actor_coupling = actor;
#ifdef USE_SOCKETW
    if (App::mp_state->getEnum<MpState>() == MpState::CONNECTED && !m_is_remote)
    {
        if (enabled)
        {
            NetCharacterMsgAttach msg;
            msg.command = CHARACTER_CMD_ATTACH;
            msg.source_id = m_actor_coupling->ar_net_source_id;
            msg.stream_id = m_actor_coupling->ar_net_stream_id;
            App::GetNetwork()->AddPacket(m_stream_id, RoRnet::MSG2_STREAM_DATA, sizeof(NetCharacterMsgAttach), (char*)&msg);
        }
        else
        {
            NetCharacterMsgGeneric msg;
            msg.command = CHARACTER_CMD_DETACH;
            App::GetNetwork()->AddPacket(m_stream_id, RoRnet::MSG2_STREAM_DATA, sizeof(NetCharacterMsgGeneric), (char*)&msg);
        }
    }
#endif // USE_SOCKETW
}

ActorPtr Character::GetActorCoupling() { return m_actor_coupling; }

// --------------------------------
// GfxCharacter

GfxCharacter* Character::SetupGfx()
{
    Entity* entity = App::GetGfxScene()->GetSceneManager()->createEntity(m_instance_name + "_mesh", "character.mesh");
    m_driving_anim_length = entity->getAnimationState("Driving")->getLength();

    // fix disappearing mesh
    AxisAlignedBox aabb;
    aabb.setInfinite();
    entity->getMesh()->_setBounds(aabb);

    // add entity to the scene node
    Ogre::SceneNode* scenenode = App::GetGfxScene()->GetSceneManager()->getRootSceneNode()->createChildSceneNode(m_instance_name);
    scenenode->attachObject(entity);
    scenenode->setScale(0.02f, 0.02f, 0.02f);
    scenenode->setVisible(false);

    // setup colour
    MaterialPtr mat1 = MaterialManager::getSingleton().getByName("tracks/character");
    MaterialPtr mat2 = mat1->clone("tracks/" + m_instance_name);
    entity->setMaterialName("tracks/" + m_instance_name);

    m_gfx_character = new GfxCharacter();
    m_gfx_character->xc_scenenode = scenenode;
    m_gfx_character->xc_character = this;
    m_gfx_character->xc_instance_name = m_instance_name;

    return m_gfx_character;
}

RoR::GfxCharacter::~GfxCharacter()
{
    Entity* ent = static_cast<Ogre::Entity*>(xc_scenenode->getAttachedObject(0));
    xc_scenenode->detachAllObjects();
    App::GetGfxScene()->GetSceneManager()->destroySceneNode(xc_scenenode);
    App::GetGfxScene()->GetSceneManager()->destroyEntity(ent);
    MaterialManager::getSingleton().unload("tracks/" + xc_instance_name);
}

void RoR::GfxCharacter::BufferSimulationData()
{
    xc_simbuf_prev = xc_simbuf;

    xc_simbuf.simbuf_character_pos          = xc_character->getPosition();
    xc_simbuf.simbuf_character_rot          = xc_character->getRotation();
    xc_simbuf.simbuf_color_number           = xc_character->GetColorNum();
    xc_simbuf.simbuf_net_username           = xc_character->GetNetUsername();
    xc_simbuf.simbuf_is_remote              = xc_character->GetIsRemote();
    xc_simbuf.simbuf_actor_coupling         = xc_character->GetActorCoupling();
    xc_simbuf.simbuf_anim_name              = xc_character->GetAnimName();
    xc_simbuf.simbuf_anim_time              = xc_character->GetAnimTime();
}

void RoR::GfxCharacter::UpdateCharacterInScene()
{
    // Actor coupling
    if (xc_simbuf.simbuf_actor_coupling != xc_simbuf_prev.simbuf_actor_coupling)
    {
        if (xc_simbuf.simbuf_actor_coupling != nullptr)
        {
            // Entering/switching vehicle
            xc_scenenode->getAttachedObject(0)->setCastShadows(false);
            xc_scenenode->setVisible(xc_simbuf.simbuf_actor_coupling->GetGfxActor()->HasDriverSeatProp());
        }
        else if (xc_simbuf_prev.simbuf_actor_coupling != nullptr)
        {
            // Leaving vehicle
            xc_scenenode->getAttachedObject(0)->setCastShadows(true);
            xc_scenenode->resetOrientation();
        }
    }

    // Position + Orientation
    Ogre::Entity* entity = static_cast<Ogre::Entity*>(xc_scenenode->getAttachedObject(0));
    if (xc_simbuf.simbuf_actor_coupling != nullptr)
    {
        // We're in vehicle
        GfxActor* gfx_actor = xc_simbuf.simbuf_actor_coupling->GetGfxActor();

        // Update character visibility first
        switch (gfx_actor->GetSimDataBuffer().simbuf_actor_state)
        {
        case ActorState::NETWORKED_HIDDEN:
            entity->setVisible(false);
            break;
        case ActorState::NETWORKED_OK:
            entity->setVisible(gfx_actor->HasDriverSeatProp());
            break;
        default:
            break; // no change.
        }

        // If visible, update position
        if (entity->isVisible())
        {
            Ogre::Vector3 pos;
            Ogre::Quaternion rot;
            xc_simbuf.simbuf_actor_coupling->GetGfxActor()->CalculateDriverPos(pos, rot);
            xc_scenenode->setOrientation(rot);
            // hack to position the character right perfect on the default seat (because the mesh has decentered origin)
            xc_scenenode->setPosition(pos + (rot * Vector3(0.f, -0.6f, 0.f)));
        }
    }
    else
    {
        xc_scenenode->resetOrientation();
        xc_scenenode->yaw(-xc_simbuf.simbuf_character_rot);
        xc_scenenode->setPosition(xc_simbuf.simbuf_character_pos);
        xc_scenenode->setVisible(true);
    }

    // Animation
    if (xc_simbuf.simbuf_anim_name != xc_simbuf_prev.simbuf_anim_name)
    {
        // 'Classic' method - enable one anim, exterminate the others ~ only_a_ptr, 06/2018
        AnimationStateIterator it = entity->getAllAnimationStates()->getAnimationStateIterator();

        while (it.hasMoreElements())
        {
            AnimationState* as = it.getNext();

            if (as->getAnimationName() == xc_simbuf.simbuf_anim_name)
            {
                as->setEnabled(true);
                as->setWeight(1);
                as->addTime(xc_simbuf.simbuf_anim_time);
            }
            else
            {
                as->setEnabled(false);
                as->setWeight(0);
            }
        }
    }
    else if (xc_simbuf.simbuf_anim_name != "") // Just do nothing if animation name is empty. May happen during networked play.
    {
        auto* as_cur = entity->getAnimationState(xc_simbuf.simbuf_anim_name);
        as_cur->setTimePosition(xc_simbuf.simbuf_anim_time);
    }

    // Multiplayer label
#ifdef USE_SOCKETW
    if (App::mp_state->getEnum<MpState>() == MpState::CONNECTED && !xc_simbuf.simbuf_actor_coupling)
    {
        // From 'updateCharacterNetworkColor()'
        const String materialName = "tracks/" + xc_instance_name;

        MaterialPtr mat = MaterialManager::getSingleton().getByName(materialName);
        if (!mat.isNull() && mat->getNumTechniques() > 0 && mat->getTechnique(0)->getNumPasses() > 1 &&
                mat->getTechnique(0)->getPass(1)->getNumTextureUnitStates() > 1)
        {
            const auto& state = mat->getTechnique(0)->getPass(1)->getTextureUnitState(1);
            Ogre::ColourValue color = App::GetNetwork()->GetPlayerColor(xc_simbuf.simbuf_color_number);
            state->setColourOperationEx(LBX_BLEND_CURRENT_ALPHA, LBS_MANUAL, LBS_CURRENT, color);
        }

        if ((!xc_simbuf.simbuf_is_remote && !App::mp_hide_own_net_label->getBool()) ||
            (xc_simbuf.simbuf_is_remote && !App::mp_hide_net_labels->getBool()))
        {
            float camDist = (xc_scenenode->getPosition() - App::GetCameraManager()->GetCameraNode()->getPosition()).length();
            Ogre::Vector3 scene_pos = xc_scenenode->getPosition();
            scene_pos.y += (1.9f + camDist / 100.0f);

            App::GetGfxScene()->DrawNetLabel(scene_pos, camDist, xc_simbuf.simbuf_net_username, xc_simbuf.simbuf_color_number);
        }
    }
#endif // USE_SOCKETW
}
