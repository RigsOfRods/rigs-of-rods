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
#include "IHeightFinder.h"
#include "InputEngine.h"
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

unsigned int Character::characterCounter = 0;

Character::Character(int source, unsigned int streamid, int colourNumber, bool remote) :
    beamCoupling(0)
    , canJump(false)
    , characterRotation(0.0f)
    , characterSpeed(2.0f)
    , characterVSpeed(0.0f)
    , colourNumber(colourNumber)
    , mAnimState(0)
    , mCamera(gEnv->mainCamera)
    , mCharacterNode(0)
    , mHideOwnNetLabel(BSETTING("HideOwnNetLabel", false))
    , mMoveableText(0)
    , networkAuthLevel(0)
    , networkUsername("")
    , physicsEnabled(true)
    , remote(remote)
    , m_source_id(source)
    , m_stream_id(streamid)
    , isCoupled(0)
    , m_sim_controller(nullptr)
{
    myNumber = characterCounter++;
    myName = "Character" + TOSTRING(myNumber);

    Entity* entity = gEnv->sceneManager->createEntity(myName + "_mesh", "character.mesh");
#if OGRE_VERSION<0x010602
	entity->setNormaliseNormals(true);
#endif //OGRE_VERSION

    // fix disappearing mesh
    AxisAlignedBox aabb;
    aabb.setInfinite();
    entity->getMesh()->_setBounds(aabb);

    // add entity to the scene node
    mCharacterNode = gEnv->sceneManager->getRootSceneNode()->createChildSceneNode();
    mCharacterNode->attachObject(entity);
    mCharacterNode->setScale(0.02f, 0.02f, 0.02f);
    mAnimState = entity->getAllAnimationStates();

    if (App::mp_state.GetActive() == MpState::CONNECTED)
    {
        sendStreamSetup();
    }
    if (remote)
    {
        setVisible(true);
    }
    // setup colour
    MaterialPtr mat1 = MaterialManager::getSingleton().getByName("tracks/character");
    MaterialPtr mat2 = mat1->clone("tracks/" + myName);
    entity->setMaterialName("tracks/" + myName);

#ifdef USE_SOCKETW
    if ((App::mp_state.GetActive() == MpState::CONNECTED) && (remote || !mHideOwnNetLabel))
    {
        mMoveableText = new MovableText("netlabel-" + myName, "");
        mCharacterNode->attachObject(mMoveableText);
        mMoveableText->setFontName("CyberbitEnglish");
        mMoveableText->setTextAlignment(MovableText::H_CENTER, MovableText::V_ABOVE);
        mMoveableText->setAdditionalHeight(2);
        mMoveableText->showOnTop(false);
        mMoveableText->setCharacterHeight(8);
        mMoveableText->setColor(ColourValue::Black);

        updateLabels();
    }
#endif //SOCKETW
}

Character::~Character()
{
    setVisible(false);
    if (mMoveableText)
    {
        delete mMoveableText;
    }
    if (mCharacterNode)
    {
        mCharacterNode->detachAllObjects();
    }
    if (gEnv->surveyMap && mapEntity)
    {
        gEnv->surveyMap->deleteMapEntity(mapEntity);
    }
    // try to unload some materials
    try
    {
        MaterialManager::getSingleton().unload("tracks/" + myName);
    }
    catch (...)
    {
    }
}

void Character::updateCharacterRotation()
{
    setRotation(characterRotation);
}

void Character::updateCharacterColour()
{
    const String materialName = "tracks/" + myName;
    const int textureUnitStateNum = 2;

    MaterialPtr mat = MaterialManager::getSingleton().getByName(materialName);
    if (mat.isNull())
        return;

    if (mat->getNumTechniques() > 0 && mat->getTechnique(0)->getNumPasses() > 0 && textureUnitStateNum < mat->getTechnique(0)->getPass(0)->getNumTextureUnitStates())
    {
        auto state = mat->getTechnique(0)->getPass(0)->getTextureUnitState(textureUnitStateNum);
        auto color = Networking::GetPlayerColor(colourNumber);
        state->setAlphaOperation(LBX_BLEND_CURRENT_ALPHA, LBS_MANUAL, LBS_CURRENT, 0.8);
        state->setColourOperationEx(LBX_BLEND_CURRENT_ALPHA, LBS_MANUAL, LBS_CURRENT, color, color, 1);
    }
}

void Character::updateLabels()
{
    if (App::mp_state.GetActive() != MpState::CONNECTED) { return; }

#ifdef USE_SOCKETW
    RoRnet::UserInfo info;

    if (remote)
    {
        if (!RoR::Networking::GetUserInfo(m_source_id, info))
            return;
    }
    else
    {
        info = RoR::Networking::GetLocalUserData();
    }

    networkAuthLevel = info.authstatus;

    colourNumber = info.colournum;
    updateCharacterColour();

    if (String(info.username).empty())
        return;
    networkUsername = tryConvertUTF(info.username);

    if (mMoveableText)
    {
        mMoveableText->setCaption(networkUsername);
    }

    /*
    if (networkAuthLevel & AUTH_ADMIN)
    {
        mMoveableText->setFontName("highcontrast_red");
    } else if (networkAuthLevel & AUTH_RANKED)
    {
        mMoveableText->setFontName("highcontrast_green");
    } else
    {
        mMoveableText->setFontName("highcontrast_black");
    }
    */

    updateNetLabelSize();
#endif //SOCKETW
}

void Character::setPosition(Vector3 position)
{
    mCharacterNode->setPosition(position);
    mLastPosition.clear();
    if (gEnv->surveyMap && gEnv->surveyMap->getMapEntityByName(myName))
    {
        gEnv->surveyMap->getMapEntityByName(myName)->setPosition(position);
    }
}

void Character::setVisible(bool visible)
{
    mCharacterNode->setVisible(visible);
    if (gEnv->surveyMap)
    {
        SurveyMapEntity* e = gEnv->surveyMap->getMapEntityByName(myName);
        if (e)
            e->setVisibility(visible);
    }
}

Vector3 Character::getPosition()
{
    return mCharacterNode->getPosition();
}

bool Character::getVisible()
{
    return mCharacterNode->getAttachedObject(0)->getVisible();
}

void Character::setRotation(Radian rotation)
{
    characterRotation = rotation;
    mCharacterNode->resetOrientation();
    mCharacterNode->yaw(-characterRotation);
}

void Character::setAnimationMode(String mode, float time)
{
    if (!mAnimState)
        return;
    if (mLastAnimMode != mode)
    {
        AnimationStateIterator it = mAnimState->getAnimationStateIterator();
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
        mLastAnimMode = mode;
    }
    else
    {
        mAnimState->getAnimationState(mode)->addTime(time);
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
    if (physicsEnabled && !remote)
    {
        // disable character movement when using the free camera mode or when the menu is opened
        // TODO: check for menu being opened
        if (gEnv->cameraManager && gEnv->cameraManager->gameControlsLocked())
            return;

        Vector3 position = mCharacterNode->getPosition();

        // gravity force is always on
        position.y += characterVSpeed * dt;
        characterVSpeed += dt * -9.8f;

        // Auto compensate minor height differences
        float depth = calculate_collision_depth(position);
        if (depth > 0.0f)
        {
            characterVSpeed = std::max(0.0f, characterVSpeed);
            canJump = true;
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
        if (mLastPosition.size() > 0)
        {
            Vector3 lastPosition = mLastPosition.front();
            Vector3 diff = mCharacterNode->getPosition() - lastPosition;
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

        mLastPosition.push_front(position);

        if (mLastPosition.size() > 10)
        {
            mLastPosition.pop_back();
        }

        // ground contact
        float pheight = gEnv->terrainManager->getHeightFinder()->getHeightAt(position.x, position.z);

        if (position.y < pheight)
        {
            position.y = pheight;
            characterVSpeed = 0.0f;
            canJump = true;
        }

        // water stuff
        bool isswimming = false;
        float wheight = -99999;

        if (gEnv->terrainManager->getWater())
        {
            wheight = gEnv->terrainManager->getWater()->getHeightWaves(position);
            if (position.y < wheight - 1.8f)
            {
                position.y = wheight - 1.8f;
                characterVSpeed = 0.0f;
            }
        }

        // 0.1 due to 'jumping' from waves -> not nice looking
        if (gEnv->terrainManager->getWater() && (wheight - pheight > 1.8f) && (position.y + 0.1f <= wheight))
        {
            isswimming = true;
        }

        float tmpJoy = 0.0f;
        if (canJump)
        {
            if (RoR::App::GetInputEngine()->getEventBoolValue(EV_CHARACTER_JUMP))
            {
                characterVSpeed = 2.0f;
                canJump = false;
            }
        }

        bool idleanim = true;

        tmpJoy = RoR::App::GetInputEngine()->getEventValue(EV_CHARACTER_RIGHT);
        if (tmpJoy > 0.0f)
        {
            float scale = RoR::App::GetInputEngine()->isKeyDown(OIS::KC_LMENU) ? 0.1f : 1.0f;
            setRotation(characterRotation + dt * 2.0f * scale * Radian(tmpJoy));
            if (!isswimming)
            {
                setAnimationMode("Turn", -dt);
                idleanim = false;
            }
        }

        tmpJoy = RoR::App::GetInputEngine()->getEventValue(EV_CHARACTER_LEFT);
        if (tmpJoy > 0.0f)
        {
            float scale = RoR::App::GetInputEngine()->isKeyDown(OIS::KC_LMENU) ? 0.1f : 1.0f;
            setRotation(characterRotation - dt * scale * 2.0f * Radian(tmpJoy));
            if (!isswimming)
            {
                setAnimationMode("Turn", dt);
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
            position += dt * characterSpeed * 0.5f * accel * Vector3(cos(characterRotation.valueRadians() - Math::HALF_PI), 0.0f, sin(characterRotation.valueRadians() - Math::HALF_PI));
            if (!isswimming)
            {
                setAnimationMode("Side_step", -dt);
                idleanim = false;
            }
        }

        tmpJoy = accel = RoR::App::GetInputEngine()->getEventValue(EV_CHARACTER_SIDESTEP_RIGHT);
        if (tmpJoy > 0.0f)
        {
            if (tmpRun > 0.0f)
                accel = 3.0f * tmpRun;
            // animation missing for that
            position += dt * characterSpeed * 0.5f * accel * Vector3(cos(characterRotation.valueRadians() + Math::HALF_PI), 0.0f, sin(characterRotation.valueRadians() + Math::HALF_PI));
            if (!isswimming)
            {
                setAnimationMode("Side_step", dt);
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

            float time = dt * tmpJoy * characterSpeed;

            if (isswimming)
            {
                setAnimationMode("Swim_loop", time);
                idleanim = false;
            }
            else
            {
                if (tmpRun > 0.0f)
                {
                    setAnimationMode("Run", time);
                    idleanim = false;
                }
                else
                {
                    setAnimationMode("Walk", time);
                    idleanim = false;
                }
            }
            // 0.005f fixes character getting stuck on meshes
            position += dt * characterSpeed * 1.5f * accel * Vector3(cos(characterRotation.valueRadians()), 0.01f, sin(characterRotation.valueRadians()));
        }
        else if (tmpBack > 0.0f)
        {
            float time = -dt * characterSpeed;
            if (isswimming)
            {
                setAnimationMode("Spot_swim", time);
                idleanim = false;
            }
            else
            {
                setAnimationMode("Walk", time);
                idleanim = false;
            }
            // 0.005f fixes character getting stuck on meshes
            position -= dt * characterSpeed * tmpBack * Vector3(cos(characterRotation.valueRadians()), 0.01f, sin(characterRotation.valueRadians()));
        }

        if (idleanim)
        {
            if (isswimming)
            {
                setAnimationMode("Spot_swim", dt * 2.0f);
            }
            else
            {
                setAnimationMode("Idle_sway", dt * 1.0f);
            }
        }

        mCharacterNode->setPosition(position);
        updateMapIcon();
    }
    else if (beamCoupling && beamCoupling->hasDriverSeat()) // beamCoupling = The vehicle or machine which the character occupies
    {
        Vector3 pos;
        Quaternion rot;
        beamCoupling->calculateDriverPos(pos, rot);
        float angle = beamCoupling->hydrodirwheeldisplay * -1.0f; // not getSteeringAngle(), but this, as its smoothed
        mCharacterNode->setOrientation(rot);
        setPosition(pos + (rot * Vector3(0.f, -0.6f, 0.f))); // hack to position the character right perfect on the default seat

        // Animation
        this->setAnimationMode("Driving");
        Real anim_length = mAnimState->getAnimationState("Driving")->getLength();
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
        mAnimState->getAnimationState("Driving")->setTimePosition(anim_time_pos);
    }

#ifdef USE_SOCKETW
    if ((App::mp_state.GetActive() == MpState::CONNECTED) && !remote)
    {
        sendStreamData();
    }
#endif // USE_SOCKETW
}

void Character::updateMapIcon()
{
    if (!gEnv->surveyMap)
        return;
    SurveyMapEntity* e = gEnv->surveyMap->getMapEntityByName(myName);
    if (e)
    {
        e->setPosition(mCharacterNode->getPosition());
        e->setRotation(mCharacterNode->getOrientation());
        e->setVisibility(!beamCoupling);
    }
    else
    {
        createMapEntity();
    }
}

void Character::unwindMovement(float distance)
{
    if (mLastPosition.size() == 0)
        return;

    Vector3 curPos = getPosition();
    Vector3 oldPos = curPos;

    for (Vector3 pos : mLastPosition)
    {
        oldPos = pos;
        if (oldPos.distance(curPos) > distance)
            break;
    }

    setPosition(oldPos);
}

void Character::move(Vector3 offset)
{
    mCharacterNode->translate(offset);
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
        "[RoR|Networking] ERROR on remote character (User: '%s', SourceID: %d, StreamID: %d): ",
        username.asUTF8_c_str(), m_source_id, m_stream_id);

    LOGSTREAM << msg_buf << detail;
#endif
}

void Character::sendStreamSetup()
{
#ifdef USE_SOCKETW
    if (remote)
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

void Character::sendStreamData()
{
#ifdef USE_SOCKETW
    // do not send position data if coupled to a truck already
    if (beamCoupling)
        return;

    Networking::CharacterMsgPos msg;
    msg.command = Networking::CHARACTER_CMD_POSITION;
    msg.pos_x = mCharacterNode->getPosition().x;
    msg.pos_y = mCharacterNode->getPosition().y;
    msg.pos_z = mCharacterNode->getPosition().z;
    msg.rot_angle = characterRotation.valueRadians();
    strncpy(msg.anim_name, mLastAnimMode.c_str(), CHARACTER_ANIM_NAME_LEN);
    msg.anim_time = mAnimState->getAnimationState(mLastAnimMode)->getTimePosition();

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
            this->setAnimationMode(Utils::SanitizeUtf8String(pos_msg->anim_name), pos_msg->anim_time);
        }
        else if (msg->command == Networking::CHARACTER_CMD_DETACH)
        {
            if (beamCoupling != nullptr)
                this->setBeamCoupling(false);
            else
                this->ReportError("Received command `DETACH`, but not currently attached to a vehicle. Ignoring command.");
        }
        else if (msg->command == Networking::CHARACTER_CMD_ATTACH)
        {
            auto* attach_msg = reinterpret_cast<Networking::CharacterMsgAttach*>(buffer);
            Beam* beam = m_sim_controller->GetBeamFactory()->getBeam(attach_msg->source_id, attach_msg->stream_id);
            if (beam != nullptr)
            {
                this->setBeamCoupling(true, beam);
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

void Character::updateNetLabelSize()
{
    if (!mMoveableText)
        return;
    if (networkUsername.empty())
        return;

    float camDist = (mCharacterNode->getPosition() - mCamera->getPosition()).length();
    float h = std::max(9.0f, camDist * 1.2f);

    mMoveableText->setCharacterHeight(h);

    if (camDist > 1000.0f)
        mMoveableText->setCaption(networkUsername + "  (" + TOSTRING((float)(ceil(camDist / 100) / 10.0f)) + " km)");
    else if (camDist > 20.0f && camDist <= 1000.0f)
        mMoveableText->setCaption(networkUsername + "  (" + TOSTRING((int)camDist) + " m)");
    else
        mMoveableText->setCaption(networkUsername);
}

void Character::setBeamCoupling(bool enabled, Beam* truck /* = 0 */)
{
    if (enabled)
    {
        if (!truck)
            return;
        beamCoupling = truck;
        setPhysicsEnabled(false);
        if (mMoveableText && mMoveableText->isVisible())
        {
            mMoveableText->setVisible(false);
        }
        if ((App::mp_state.GetActive() == MpState::CONNECTED) && !remote)
        {
#ifdef USE_SOCKETW
            Networking::CharacterMsgAttach msg;
            msg.command = Networking::CHARACTER_CMD_ATTACH;
            msg.source_id = beamCoupling->m_source_id;
            msg.stream_id = beamCoupling->m_stream_id;
            RoR::Networking::AddPacket(m_stream_id, RoRnet::MSG2_STREAM_DATA, sizeof(Networking::CharacterMsgAttach), (char*)&msg);
#endif // USE_SOCKETW
        }

        // do not cast shadows inside of a truck
        mCharacterNode->getAttachedObject(0)->setCastShadows(false);
        isCoupled = true;
        // check if there is a seat, if not, hide our character
        if (!beamCoupling->hasDriverSeat())
        {
            // driver seat not found
            setVisible(false);
            isCoupled = false;
        }
    }
    else
    {
        isCoupled = false;
        setPhysicsEnabled(true);
        beamCoupling = 0;
        if (mMoveableText && !mMoveableText->isVisible())
        {
            mMoveableText->setVisible(true);
        }
        if ((App::mp_state.GetActive() == MpState::CONNECTED) && !remote)
        {
#ifdef USE_SOCKETW
            Networking::CharacterMsgGeneric msg;
            msg.command = Networking::CHARACTER_CMD_DETACH;
            RoR::Networking::AddPacket(m_stream_id, RoRnet::MSG2_STREAM_DATA, sizeof(Networking::CharacterMsgGeneric), (char*)&msg);
#endif // USE_SOCKETW
        }

        // show character
        setVisible(true);

        mCharacterNode->resetOrientation();

        // cast shadows when using it on the outside
        mCharacterNode->getAttachedObject(0)->setCastShadows(true);
    }
}

void Character::createMapEntity()
{
    if (gEnv->surveyMap)
    {
        mapEntity = gEnv->surveyMap->createNamedMapEntity(myName, "person");
        mapEntity->setState(0);
        mapEntity->setVisibility(true);
        mapEntity->setPosition(mCharacterNode->getPosition());
        mapEntity->setRotation(mCharacterNode->getOrientation());
    }
}
