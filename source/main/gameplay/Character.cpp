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
#include "PlayerColours.h"
#include "SurveyMapEntity.h"
#include "SurveyMapManager.h"
#include "TerrainManager.h"
#include "Utils.h"
#include "Water.h"

using namespace Ogre;
using namespace RoR;

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

    if (App::GetActiveMpState() == App::MP_STATE_CONNECTED)
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
    if ((App::GetActiveMpState() == App::MP_STATE_CONNECTED) && (remote || !mHideOwnNetLabel))
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
#ifdef USE_MYGUI
    if (gEnv->surveyMap && mapEntity)
    {
        gEnv->surveyMap->deleteMapEntity(mapEntity);
    }
#endif // USE_MYGUI
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
    String matName = "tracks/" + myName;
    PlayerColours::getSingleton().updateMaterial(colourNumber, matName, 2);
}

void Character::updateLabels()
{
    if (App::GetActiveMpState() != App::MP_STATE_CONNECTED) { return; }

#ifdef USE_SOCKETW
    user_info_t info;

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
#ifdef USE_MYGUI
    if (gEnv->surveyMap && gEnv->surveyMap->getMapEntityByName(myName))
    {
        gEnv->surveyMap->getMapEntityByName(myName)->setPosition(position);
    }
#endif // USE_MYGUI
}

void Character::setVisible(bool visible)
{
    mCharacterNode->setVisible(visible);
#ifdef USE_MYGUI
    if (gEnv->surveyMap)
    {
        SurveyMapEntity* e = gEnv->surveyMap->getMapEntityByName(myName);
        if (e)
            e->setVisibility(visible);
    }
#endif // USE_MYGUI
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
    if ((App::GetActiveMpState() == App::MP_STATE_CONNECTED) && !remote)
    {
        sendStreamData();
    }
#endif // USE_SOCKETW
}

void Character::updateMapIcon()
{
#ifdef USE_MYGUI
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
#endif // USE_MYGUI
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

void Character::sendStreamSetup()
{
#ifdef USE_SOCKETW
    if (remote)
        return;

    stream_register_t reg;
    memset(&reg, 0, sizeof(reg));
    reg.status = 1;
    strcpy(reg.name, "default");
    reg.type = 1;
    reg.data[0] = 2;

    RoR::Networking::AddLocalStream(&reg, sizeof(stream_register_t));

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

    pos_netdata_t data;
    data.command = CHARCMD_POSITION;
    data.posx = mCharacterNode->getPosition().x;
    data.posy = mCharacterNode->getPosition().y;
    data.posz = mCharacterNode->getPosition().z;
    data.rotx = mCharacterNode->getOrientation().x;
    data.roty = mCharacterNode->getOrientation().y;
    data.rotz = mCharacterNode->getOrientation().z;
    data.rotw = mCharacterNode->getOrientation().w;
    strncpy(data.animationMode, mLastAnimMode.c_str(), 254);
    data.animationTime = mAnimState->getAnimationState(mLastAnimMode)->getTimePosition();

    //LOG("sending character stream data: " + TOSTRING(RoR::Networking::GetUID()) + ":" + TOSTRING(m_stream_id));
    RoR::Networking::AddPacket(m_stream_id, MSG2_STREAM_DATA, sizeof(pos_netdata_t), (char*)&data);
#endif // USE_SOCKETW
}

void Character::receiveStreamData(unsigned int& type, int& source, unsigned int& streamid, char* buffer)
{
    if (type == MSG2_STREAM_DATA && m_source_id == source && m_stream_id == streamid)
    {
        header_netdata_t* header = (header_netdata_t *)buffer;
        if (header->command == CHARCMD_POSITION)
        {
            pos_netdata_t* data = (pos_netdata_t *)buffer;
            Vector3 pos(data->posx, data->posy, data->posz);
            setPosition(pos);
            Quaternion rot(data->rotw, data->rotx, data->roty, data->rotz);
            mCharacterNode->setOrientation(rot);
            setAnimationMode(getASCIIFromCharString(data->animationMode, 255), data->animationTime);
        }
        else if (header->command == CHARCMD_ATTACH)
        {
            attach_netdata_t* data = (attach_netdata_t *)buffer;
            if (data->enabled)
            {
                Beam* beam = BeamFactory::getSingleton().getBeam(data->source_id, data->stream_id);
                setBeamCoupling(true, beam);
            }
            else
            {
                setBeamCoupling(false);
            }
        }
    }
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
        if ((App::GetActiveMpState() == App::MP_STATE_CONNECTED) && !remote)
        {
#ifdef USE_SOCKETW
            attach_netdata_t data;
            data.command = CHARCMD_ATTACH;
            data.enabled = true;
            data.source_id = beamCoupling->m_source_id;
            data.stream_id = beamCoupling->m_stream_id;
            RoR::Networking::AddPacket(m_stream_id, MSG2_STREAM_DATA, sizeof(attach_netdata_t), (char*)&data);
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
        if ((App::GetActiveMpState() == App::MP_STATE_CONNECTED) && !remote)
        {
#ifdef USE_SOCKETW
            attach_netdata_t data;
            data.command = CHARCMD_ATTACH;
            data.enabled = false;
            RoR::Networking::AddPacket(m_stream_id, MSG2_STREAM_DATA, sizeof(attach_netdata_t), (char*)&data);
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
#ifdef USE_MYGUI
    if (gEnv->surveyMap)
    {
        mapEntity = gEnv->surveyMap->createNamedMapEntity(myName, "person");
        mapEntity->setState(0);
        mapEntity->setVisibility(true);
        mapEntity->setPosition(mCharacterNode->getPosition());
        mapEntity->setRotation(mCharacterNode->getOrientation());
    }
#endif // USE_MYGUI
}
