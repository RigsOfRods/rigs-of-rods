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

#include "Character.h"

#include "Application.h"
#include "BeamFactory.h"
#include "CameraManager.h"
#include "Collisions.h"
#include "InputEngine.h"
#include "MovableText.h"
#include "Network.h"
#include "RoRFrameListener.h"
#include "Settings.h"
#include "SurveyMapEntity.h"
#include "SurveyMapManager.h"
#include "TerrainManager.h"
#include "Utils.h"
#include "Water.h"

using namespace Ogre;
using namespace RoR;

#define LOGSTREAM Ogre::LogManager::getSingleton().stream()

Character::Character(int source, unsigned int streamid, int color_number, bool is_remote) :
    m_actor_coupling(nullptr)
    , m_can_jump(false)
    , m_character_rotation(0.0f)
    , m_character_h_speed(2.0f)
    , m_character_v_speed(0.0f)
    , m_color_number(color_number)
    , m_anim_state(0)
    , m_character_scenenode(0)
    , m_hide_own_net_label(BSETTING("HideOwnNetLabel", false))
    , m_movable_text(0)
    , m_net_username("")
    , m_is_remote(is_remote)
    , m_source_id(source)
    , m_stream_id(streamid)
    , m_have_coupling_seat(false)
{
    static int id_counter = 0;
    m_instance_name = "Character" + TOSTRING(id_counter);
    ++id_counter;

    Entity* entity = gEnv->sceneManager->createEntity(m_instance_name + "_mesh", "character.mesh");
#if OGRE_VERSION<0x010602
	entity->setNormaliseNormals(true);
#endif //OGRE_VERSION

    // fix disappearing mesh
    AxisAlignedBox aabb;
    aabb.setInfinite();
    entity->getMesh()->_setBounds(aabb);

    // add entity to the scene node
    m_character_scenenode = gEnv->sceneManager->getRootSceneNode()->createChildSceneNode();
    m_character_scenenode->attachObject(entity);
    m_character_scenenode->setScale(0.02f, 0.02f, 0.02f);
    m_anim_state = entity->getAllAnimationStates();

    if (App::mp_state.GetActive() == MpState::CONNECTED)
    {
        this->SendStreamSetup();
    }
    if (m_is_remote)
    {
        setVisible(true);
    }
    // setup colour
    MaterialPtr mat1 = MaterialManager::getSingleton().getByName("tracks/character");
    MaterialPtr mat2 = mat1->clone("tracks/" + m_instance_name);
    entity->setMaterialName("tracks/" + m_instance_name);

#ifdef USE_SOCKETW
    if ((App::mp_state.GetActive() == MpState::CONNECTED) && (m_is_remote || !m_hide_own_net_label))
    {
        m_movable_text = new MovableText("netlabel-" + m_instance_name, "");
        m_character_scenenode->attachObject(m_movable_text);
        m_movable_text->setFontName("CyberbitEnglish");
        m_movable_text->setTextAlignment(MovableText::H_CENTER, MovableText::V_ABOVE);
        m_movable_text->setAdditionalHeight(2);
        m_movable_text->showOnTop(false);
        m_movable_text->setCharacterHeight(8);
        m_movable_text->setColor(ColourValue::Black);

        updateLabels();
    }
#endif //SOCKETW
}

Character::~Character()
{
    setVisible(false);
    if (m_movable_text)
    {
        delete m_movable_text;
    }
    if (m_character_scenenode)
    {
        m_character_scenenode->detachAllObjects();
    }
    if (gEnv->surveyMap && m_survey_map_entity)
    {
        gEnv->surveyMap->deleteMapEntity(m_survey_map_entity);
    }
    // try to unload some materials
    try
    {
        MaterialManager::getSingleton().unload("tracks/" + m_instance_name);
    }
    catch (...)
    {
    }
}

void Character::updateCharacterRotation()
{
    setRotation(m_character_rotation);
}

void Character::updateCharacterNetworkColour()
{
#ifdef USE_SOCKETW
    const String materialName = "tracks/" + m_instance_name;
    const int textureUnitStateNum = 2;

    MaterialPtr mat = MaterialManager::getSingleton().getByName(materialName);
    if (mat.isNull())
        return;

    if (mat->getNumTechniques() > 0 && mat->getTechnique(0)->getNumPasses() > 0 && textureUnitStateNum < mat->getTechnique(0)->getPass(0)->getNumTextureUnitStates())
    {
        auto state = mat->getTechnique(0)->getPass(0)->getTextureUnitState(textureUnitStateNum);
        auto color = Networking::GetPlayerColor(m_color_number);
        state->setAlphaOperation(LBX_BLEND_CURRENT_ALPHA, LBS_MANUAL, LBS_CURRENT, 0.8);
        state->setColourOperationEx(LBX_BLEND_CURRENT_ALPHA, LBS_MANUAL, LBS_CURRENT, color, color, 1);
    }
#endif // USE_SOCKETW
}

void Character::updateLabels()
{
    if (App::mp_state.GetActive() != MpState::CONNECTED) { return; }

#ifdef USE_SOCKETW
    RoRnet::UserInfo info;

    if (m_is_remote)
    {
        if (!RoR::Networking::GetUserInfo(m_source_id, info))
            return;
    }
    else
    {
        info = RoR::Networking::GetLocalUserData();
    }

    m_color_number = info.colournum;
    this->updateCharacterNetworkColour();

    if (String(info.username).empty())
        return;
    m_net_username = tryConvertUTF(info.username);

    if (m_movable_text)
    {
        m_movable_text->setCaption(m_net_username);
    }

    this->ResizePersonNetLabel();
#endif //SOCKETW
}

void Character::setPosition(Vector3 position)
{
    m_character_scenenode->setPosition(position);
    m_prev_positions.clear();
    if (gEnv->surveyMap && gEnv->surveyMap->getMapEntityByName(m_instance_name))
    {
        gEnv->surveyMap->getMapEntityByName(m_instance_name)->setPosition(position);
    }
}

void Character::setVisible(bool visible)
{
    m_character_scenenode->setVisible(visible);
    if (gEnv->surveyMap)
    {
        SurveyMapEntity* e = gEnv->surveyMap->getMapEntityByName(m_instance_name);
        if (e)
            e->setVisibility(visible);
    }
}

Vector3 Character::getPosition()
{
    return m_character_scenenode->getPosition();
}

bool Character::getVisible()
{
    return m_character_scenenode->getAttachedObject(0)->getVisible();
}

void Character::setRotation(Radian rotation)
{
    m_character_rotation = rotation;
    m_character_scenenode->resetOrientation();
    m_character_scenenode->yaw(-m_character_rotation);
}

void Character::SetAnimState(std::string mode, float time)
{
    if (!m_anim_state)
        return;
    if (m_last_anim != mode)
    {
        AnimationStateIterator it = m_anim_state->getAnimationStateIterator();
        while (it.hasMoreElements())
        {
            AnimationState* as = it.getNext();
            if (as->getAnimationName() == mode)
            {
                as->setEnabled(true);
                as->setWeight(1);
                as->addTime(time);
            }
            else
            {
                as->setEnabled(false);
                as->setWeight(0);
            }
        }
        m_last_anim = mode;
    }
    else
    {
        m_anim_state->getAnimationState(mode)->addTime(time);
    }
}

float calculate_collision_depth(Vector3 pos)
{
    Vector3 query = pos + Vector3::UNIT_Y;
    while (query.y > pos.y)
    {
        if (gEnv->collisions->collisionCorrect(&query, false))
            break;
        query.y -= 0.01f;
    }
    return query.y - pos.y;
}

void Character::update(float dt)
{
    if (!m_is_remote && (m_actor_coupling == nullptr) && (App::sim_state.GetActive() != SimState::PAUSED))
    {
        // disable character movement when using the free camera mode or when the menu is opened
        // TODO: check for menu being opened
        if (gEnv->cameraManager && gEnv->cameraManager->gameControlsLocked())
            return;

        Vector3 position = m_character_scenenode->getPosition();

        // gravity force is always on
        position.y += m_character_v_speed * dt;
        m_character_v_speed += dt * -9.8f;

        // Auto compensate minor height differences
        float depth = calculate_collision_depth(position);
        if (depth > 0.0f)
        {
            m_character_v_speed = std::max(0.0f, m_character_v_speed);
            m_can_jump = true;
            if (depth < 0.3f)
            {
                position.y += depth;
                if (depth > 0.01f && gEnv->cameraManager)
                {
                    gEnv->cameraManager->NotifyContextChange();
                }
            }
        }

        // Trigger script events and handle mesh (ground) collision
        {
            Vector3 query = position;
            gEnv->collisions->collisionCorrect(&query);
            if (std::abs(position.y - query.y) > 0.1f && gEnv->cameraManager)
            {
                gEnv->cameraManager->NotifyContextChange();
            }
            position.y = query.y;
        }

        // Obstacle detection
        if (m_prev_positions.size() > 0)
        {
            Vector3 lastPosition = m_prev_positions.front();
            Vector3 diff = m_character_scenenode->getPosition() - lastPosition;
            Vector3 h_diff = Vector3(diff.x, 0.0f, diff.z);
            if (depth <= 0.0f || h_diff.squaredLength() > 0.0f)
            {
                const int numstep = 100;
                Vector3 base = lastPosition + Vector3::UNIT_Y * 0.5f;
                for (int i = 1; i < numstep; i++)
                {
                    Vector3 query = base + diff * ((float)i / numstep);
                    if (gEnv->collisions->collisionCorrect(&query, false))
                    {
                        position = lastPosition + diff * ((float)(i - 1) / numstep);;
                        break;
                    }
                }
            }
        }

        m_prev_positions.push_front(position);

        if (m_prev_positions.size() > 10)
        {
            m_prev_positions.pop_back();
        }

        // ground contact
        float pheight = App::GetSimTerrain()->GetHeightAt(position.x, position.z);

        if (position.y < pheight)
        {
            position.y = pheight;
            m_character_v_speed = 0.0f;
            m_can_jump = true;
        }

        // water stuff
        bool isswimming = false;
        float wheight = -99999;

        if (App::GetSimTerrain()->getWater())
        {
            wheight = App::GetSimTerrain()->getWater()->CalcWavesHeight(position);
            if (position.y < wheight - 1.8f)
            {
                position.y = wheight - 1.8f;
                m_character_v_speed = 0.0f;
            }
        }

        // 0.1 due to 'jumping' from waves -> not nice looking
        if (App::GetSimTerrain()->getWater() && (wheight - pheight > 1.8f) && (position.y + 0.1f <= wheight))
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

        tmpJoy = RoR::App::GetInputEngine()->getEventValue(EV_CHARACTER_RIGHT);
        if (tmpJoy > 0.0f)
        {
            float scale = RoR::App::GetInputEngine()->isKeyDown(OIS::KC_LMENU) ? 0.1f : 1.0f;
            setRotation(m_character_rotation + dt * 2.0f * scale * Radian(tmpJoy));
            if (!isswimming)
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
            if (!isswimming)
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
            if (!isswimming)
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
            if (!isswimming)
            {
                this->SetAnimState("Side_step", dt);
                idleanim = false;
            }
        }

        tmpJoy = accel = RoR::App::GetInputEngine()->getEventValue(EV_CHARACTER_FORWARD) + RoR::App::GetInputEngine()->getEventValue(EV_CHARACTER_ROT_UP);
        float tmpBack = RoR::App::GetInputEngine()->getEventValue(EV_CHARACTER_BACKWARDS) + RoR::App::GetInputEngine()->getEventValue(EV_CHARACTER_ROT_DOWN);

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
            // 0.005f fixes character getting stuck on meshes
            position += dt * m_character_h_speed * 1.5f * accel * Vector3(cos(m_character_rotation.valueRadians()), 0.01f, sin(m_character_rotation.valueRadians()));
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
            // 0.005f fixes character getting stuck on meshes
            position -= dt * m_character_h_speed * tmpBack * Vector3(cos(m_character_rotation.valueRadians()), 0.01f, sin(m_character_rotation.valueRadians()));
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

        m_character_scenenode->setPosition(position);
        updateMapIcon();
    }
    else if (m_actor_coupling && m_actor_coupling->hasDriverSeat()) // The character occupies a vehicle or machine
    {
        Vector3 pos;
        Quaternion rot;
        m_actor_coupling->calculateDriverPos(pos, rot);
        float angle = m_actor_coupling->ar_hydro_dir_wheel_display * -1.0f; // not getSteeringAngle(), but this, as its smoothed
        m_character_scenenode->setOrientation(rot);
        setPosition(pos + (rot * Vector3(0.f, -0.6f, 0.f))); // hack to position the character right perfect on the default seat

        // Animation
        this->SetAnimState("Driving");
        Real anim_length = m_anim_state->getAnimationState("Driving")->getLength();
        float anim_time_pos = ((angle + 1.0f) * 0.5f) * anim_length;
        // prevent animation flickering on the borders:
        if (anim_time_pos < 0.01f)
        {
            anim_time_pos = 0.01f;
        }
        if (anim_time_pos > anim_length - 0.01f)
        {
            anim_time_pos = anim_length - 0.01f;
        }
        m_anim_state->getAnimationState("Driving")->setTimePosition(anim_time_pos);
    }

#ifdef USE_SOCKETW
    if ((App::mp_state.GetActive() == MpState::CONNECTED) && !m_is_remote)
    {
        this->SendStreamData();
    }
#endif // USE_SOCKETW
}

void Character::updateMapIcon()
{
    if (!gEnv->surveyMap)
        return;
    SurveyMapEntity* e = gEnv->surveyMap->getMapEntityByName(m_instance_name);
    if (e)
    {
        e->setPosition(m_character_scenenode->getPosition());
        e->setRotation(m_character_scenenode->getOrientation());
        e->setVisibility(!m_actor_coupling);
    }
    else
    {
        this->AddPersonToSurveyMap();
    }
}

void Character::unwindMovement(float distance)
{
    if (m_prev_positions.size() == 0)
        return;

    Vector3 curPos = getPosition();
    Vector3 oldPos = curPos;

    for (Vector3 pos : m_prev_positions)
    {
        oldPos = pos;
        if (oldPos.distance(curPos) > distance)
            break;
    }

    setPosition(oldPos);
}

void Character::move(Vector3 offset)
{
    m_character_scenenode->translate(offset);
}

// Helper function
void Character::ReportError(const char* detail)
{
#ifdef USE_SOCKETW
    Ogre::UTFString username;
    RoRnet::UserInfo info;
    if (!RoR::Networking::GetUserInfo(m_source_id, info))
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

    RoR::Networking::AddLocalStream(&reg, sizeof(RoRnet::StreamRegister));

    m_source_id = reg.origin_sourceid;
    m_stream_id = reg.origin_streamid;
#endif // USE_SOCKETW
}

void Character::SendStreamData()
{
#ifdef USE_SOCKETW
    // do not send position data if coupled to an actor already
    if (m_actor_coupling)
        return;

    Networking::CharacterMsgPos msg;
    msg.command = Networking::CHARACTER_CMD_POSITION;
    msg.pos_x = m_character_scenenode->getPosition().x;
    msg.pos_y = m_character_scenenode->getPosition().y;
    msg.pos_z = m_character_scenenode->getPosition().z;
    msg.rot_angle = m_character_rotation.valueRadians();
    strncpy(msg.anim_name, m_last_anim.c_str(), CHARACTER_ANIM_NAME_LEN);
    msg.anim_time = m_anim_state->getAnimationState(m_last_anim)->getTimePosition();

    RoR::Networking::AddPacket(m_stream_id, RoRnet::MSG2_STREAM_DATA, sizeof(Networking::CharacterMsgPos), (char*)&msg);
#endif // USE_SOCKETW
}

void Character::receiveStreamData(unsigned int& type, int& source, unsigned int& streamid, char* buffer)
{
#ifdef USE_SOCKETW
    if (type == RoRnet::MSG2_STREAM_DATA && m_source_id == source && m_stream_id == streamid)
    {
        auto* msg = reinterpret_cast<Networking::CharacterMsgGeneric*>(buffer);
        if (msg->command == Networking::CHARACTER_CMD_POSITION)
        {
            auto* pos_msg = reinterpret_cast<Networking::CharacterMsgPos*>(buffer);
            this->setPosition(Ogre::Vector3(pos_msg->pos_x, pos_msg->pos_y, pos_msg->pos_z));
            this->setRotation(Ogre::Radian(pos_msg->rot_angle));
            this->SetAnimState(Utils::SanitizeUtf8String(pos_msg->anim_name), pos_msg->anim_time);
        }
        else if (msg->command == Networking::CHARACTER_CMD_DETACH)
        {
            if (m_actor_coupling != nullptr)
                this->SetActorCoupling(false);
            else
                this->ReportError("Received command `DETACH`, but not currently attached to a vehicle. Ignoring command.");
        }
        else if (msg->command == Networking::CHARACTER_CMD_ATTACH)
        {
            auto* attach_msg = reinterpret_cast<Networking::CharacterMsgAttach*>(buffer);
            Actor* beam = App::GetSimController()->GetBeamFactory()->GetActorByNetworkLinks(attach_msg->source_id, attach_msg->stream_id);
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

void Character::ResizePersonNetLabel()
{
    if (!m_movable_text)
        return;
    if (m_net_username.empty())
        return;

    float camDist = (m_character_scenenode->getPosition() - gEnv->mainCamera->getPosition()).length();
    float h = std::max(9.0f, camDist * 1.2f);

    m_movable_text->setCharacterHeight(h);

    if (camDist > 1000.0f)
        m_movable_text->setCaption(m_net_username + "  (" + TOSTRING((float)(ceil(camDist / 100) / 10.0f)) + " km)");
    else if (camDist > 20.0f && camDist <= 1000.0f)
        m_movable_text->setCaption(m_net_username + "  (" + TOSTRING((int)camDist) + " m)");
    else
        m_movable_text->setCaption(m_net_username);
}

void Character::SetActorCoupling(bool enabled, Actor* actor /* = nullptr */)
{
    if (enabled)
    {
        if (!actor)
            return;
        m_actor_coupling = actor;
        if (m_movable_text && m_movable_text->isVisible())
        {
            m_movable_text->setVisible(false);
        }
        if ((App::mp_state.GetActive() == MpState::CONNECTED) && !m_is_remote)
        {
#ifdef USE_SOCKETW
            Networking::CharacterMsgAttach msg;
            msg.command = Networking::CHARACTER_CMD_ATTACH;
            msg.source_id = m_actor_coupling->ar_net_source_id;
            msg.stream_id = m_actor_coupling->ar_net_stream_id;
            RoR::Networking::AddPacket(m_stream_id, RoRnet::MSG2_STREAM_DATA, sizeof(Networking::CharacterMsgAttach), (char*)&msg);
#endif // USE_SOCKETW
        }

        // do not cast shadows inside of an actor
        m_character_scenenode->getAttachedObject(0)->setCastShadows(false);
        m_have_coupling_seat = true;
        // check if there is a seat, if not, hide our character
        if (!m_actor_coupling->hasDriverSeat())
        {
            // driver seat not found
            setVisible(false);
            m_have_coupling_seat = false;
        }
    }
    else
    {
        m_have_coupling_seat = false;
        m_actor_coupling = nullptr;
        if (m_movable_text && !m_movable_text->isVisible())
        {
            m_movable_text->setVisible(true);
        }
        if ((App::mp_state.GetActive() == MpState::CONNECTED) && !m_is_remote)
        {
#ifdef USE_SOCKETW
            Networking::CharacterMsgGeneric msg;
            msg.command = Networking::CHARACTER_CMD_DETACH;
            RoR::Networking::AddPacket(m_stream_id, RoRnet::MSG2_STREAM_DATA, sizeof(Networking::CharacterMsgGeneric), (char*)&msg);
#endif // USE_SOCKETW
        }

        // show character
        setVisible(true);

        m_character_scenenode->resetOrientation();

        // cast shadows when using it on the outside
        m_character_scenenode->getAttachedObject(0)->setCastShadows(true);
    }
}

void Character::AddPersonToSurveyMap()
{
    if (gEnv->surveyMap)
    {
        m_survey_map_entity = gEnv->surveyMap->createNamedMapEntity(m_instance_name, "person");
        m_survey_map_entity->setState(0);
        m_survey_map_entity->setVisibility(true);
        m_survey_map_entity->setPosition(m_character_scenenode->getPosition());
        m_survey_map_entity->setRotation(m_character_scenenode->getOrientation());
    }
}
