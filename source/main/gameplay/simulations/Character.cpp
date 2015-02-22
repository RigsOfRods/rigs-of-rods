/*/
This source file is part of Rigs of Rods
Copyright 2005-2012 Pierre-Michel Ricordel
Copyright 2007-2012 Thomas Fischer

For more information, see http://www.rigsofrods.com/

Rigs of Rods is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License version 3, as
published by the Free Software Foundation.

Rigs of Rods is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Rigs of Rods.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "Character.h"

#include "Application.h"
#include "BeamFactory.h"
#include "CameraManager.h"
#include "Collisions.h"
#include "IHeightFinder.h"
#include "InputEngine.h"
#include "SurveyMapManager.h"
#include "SurveyMapEntity.h"
#include "Network.h"
#include "NetworkStreamManager.h"
#include "PlayerColours.h"
#include "TerrainManager.h"
#include "Utils.h"
#include "Water.h"

using namespace Ogre;


unsigned int Character::characterCounter = 0;

Character::Character(int source, unsigned int streamid, int colourNumber, bool remote) :
	  beamCoupling(0)
	, canJump(false)
	, characterRotation(0.0f)
	, characterSpeed(2.0f)
	, characterVSpeed(0.0f)
	, colourNumber(colourNumber)
	, last_net_time(0)
	, mAnimState(0)
	, mCamera(gEnv->mainCamera)
	, mCharacterNode(0)
	, mLastPosition(Vector3::ZERO)
	, mMoveableText(0)
	, networkAuthLevel(0)
	, networkUsername("")
	, physicsEnabled(true)
	, remote(remote)
	, source(source)
	, streamid(streamid)
	, isCoupled(0)
{
	myNumber = characterCounter++;
	myName   = "Character" + TOSTRING(myNumber);

	Entity *entity = gEnv->sceneManager->createEntity(myName+"_mesh", "character.mesh");
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

	if (gEnv->network)
	{
		sendStreamSetup();
	}
	if (remote)
	{
		setVisible(true);
	}
	// setup colour
	MaterialPtr mat1 = MaterialManager::getSingleton().getByName("tracks/character");
	MaterialPtr mat2 = mat1->clone("tracks/"+myName);
	entity->setMaterialName("tracks/"+myName);

	updateNetLabel();
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
		MaterialManager::getSingleton().unload("tracks/"+myName);
	} catch(...) {}
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

void Character::setUID(int uid)
{
	this->source = uid;
}

void Character::updateNetLabel()
{
#ifdef USE_SOCKETW
	// label above head
	if (!gEnv->network) return;
	if (remote)
	{
		client_t *info = gEnv->network->getClientInfo(this->source);
		if (!info) return;
		if (tryConvertUTF(info->user.username).empty()) return;
		this->colourNumber = info->user.colournum;
		networkUsername = tryConvertUTF(info->user.username);
		networkAuthLevel = info->user.authstatus;
	} else
	{
		user_info_t *info = gEnv->network->getLocalUserData();
		if (!info) return;
		if (String(info->username).empty()) return;
		this->colourNumber = info->colournum;
		networkUsername = tryConvertUTF(info->username);
		networkAuthLevel = info->authstatus;
	}

	//LOG(" * updateNetLabel : " + TOSTRING(this->source));
	if (!mMoveableText)
	{
		mMoveableText = new MovableText("netlabel-"+myName, networkUsername);
		mCharacterNode->attachObject(mMoveableText);
		mMoveableText->setFontName("CyberbitEnglish");
		mMoveableText->setTextAlignment(MovableText::H_CENTER, MovableText::V_ABOVE);
		mMoveableText->setAdditionalHeight(2);
		mMoveableText->showOnTop(false);
		mMoveableText->setCharacterHeight(8);
		mMoveableText->setColor(ColourValue::Black);
	}

	//LOG(" *label caption: " + String(networkUsername));
	try
	{
		mMoveableText->setCaption(networkUsername);
	} catch (...) {}
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

	// update character colour
	updateCharacterColour();
#endif //SOCKETW
}

void Character::setPosition(Vector3 position)
{
	mCharacterNode->setPosition(position);
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
	if (gEnv->surveyMap && gEnv->surveyMap->getMapEntityByName(myName))
	{
		gEnv->surveyMap->getMapEntityByName(myName)->setVisibility(visible);
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
		while(it.hasMoreElements())
		{
			AnimationState *as = it.getNext();
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
	} else
	{
		mAnimState->getAnimationState(mode)->addTime(time);
	}
}

void Character::update(float dt)
{
	if (physicsEnabled && !remote)
	{
		// disable character movement when using the free camera mode or when the menu is opened
		// TODO: check for menu being opened
		if (gEnv->cameraManager && gEnv->cameraManager->gameControlsLocked()) return;

		// small hack: if not visible do not apply physics
		Vector3 position = mCharacterNode->getPosition();

		// gravity force is always on
		position.y += characterVSpeed * dt;
		characterVSpeed += dt * -9.8f;

		// object contact
		Vector3 position2 = position + Vector3(0.0f, 0.075f, 0.0f);
		Vector3 position3 = position + Vector3(0.0f, 0.25, 0.0f);

		if (gEnv->collisions->collisionCorrect(&position))
		{
			characterVSpeed = std::max(0.0f, characterVSpeed);
			if (gEnv->collisions->collisionCorrect(&position2) && !gEnv->collisions->collisionCorrect(&position3))
			{
				characterVSpeed = 2.0f; // autojump
			} else
			{
				canJump = true;
			}
		} else
		{
			// double check
			if (mLastPosition.squaredDistance(position) < 25.0f)
			{
				const int numstep = 100;
				Vector3 dvec = (position - mLastPosition);
				for (int i=1; i <= numstep; i++)
				{
					Vector3 cposition = mLastPosition + dvec * ((float)i / numstep);
					if (gEnv->collisions->collisionCorrect(&cposition))
					{
						position = cposition;
						characterVSpeed = std::max(0.0f, characterVSpeed);
						canJump = true;
						break;
					}
				}
			}

		}

		mLastPosition = position;

		// ground contact
		float pheight = gEnv->terrainManager->getHeightFinder()->getHeightAt(position.x,position.z);

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
			if (RoR::Application::GetInputEngine()->getEventBoolValue(EV_CHARACTER_JUMP))
			{
				characterVSpeed = 2.0f;
				canJump = false;
			}
		}

		bool idleanim = true;

		tmpJoy = RoR::Application::GetInputEngine()->getEventValue(EV_CHARACTER_RIGHT);
		if (tmpJoy > 0.0f)
		{
			setRotation(characterRotation + dt * 2.0f * Radian(tmpJoy));
			if (!isswimming)
			{
				setAnimationMode("Turn", -dt);
				idleanim = false;
			}
		}

		tmpJoy = RoR::Application::GetInputEngine()->getEventValue(EV_CHARACTER_LEFT);
		if (tmpJoy > 0.0f)
		{
			setRotation(characterRotation - dt * 2.0f * Radian(tmpJoy));
			if (!isswimming)
			{
				setAnimationMode("Turn", dt);
				idleanim = false;
			}
		}
				
		float tmpRun = RoR::Application::GetInputEngine()->getEventValue(EV_CHARACTER_RUN);
		float accel = 1.0f;

		tmpJoy = accel = RoR::Application::GetInputEngine()->getEventValue(EV_CHARACTER_SIDESTEP_LEFT);
		if (tmpJoy > 0.0f)
		{
			if (tmpRun > 0.0f) accel = 3.0f * tmpRun;
			// animation missing for that
			position += dt * characterSpeed *1.5f * accel * Vector3(cos(characterRotation.valueRadians() - Math::HALF_PI), 0.0f, sin(characterRotation.valueRadians() - Math::HALF_PI));
		}

		tmpJoy = accel = RoR::Application::GetInputEngine()->getEventValue(EV_CHARACTER_SIDESTEP_RIGHT);
		if (tmpJoy > 0.0f)
		{
			if (tmpRun > 0.0f) accel = 3.0f * tmpRun;
			// animation missing for that
			position += dt * characterSpeed * 1.5f * accel * Vector3(cos(characterRotation.valueRadians() + Math::HALF_PI), 0.0f, sin(characterRotation.valueRadians() + Math::HALF_PI));
		}

		tmpJoy = accel = RoR::Application::GetInputEngine()->getEventValue(EV_CHARACTER_FORWARD) + RoR::Application::GetInputEngine()->getEventValue(EV_CHARACTER_ROT_UP);
		float tmpBack  = RoR::Application::GetInputEngine()->getEventValue(EV_CHARACTER_BACKWARDS) + RoR::Application::GetInputEngine()->getEventValue(EV_CHARACTER_ROT_DOWN);
		
		tmpJoy  = std::min(tmpJoy, 1.0f);
		tmpBack = std::min(tmpBack, 1.0f);

		if (tmpJoy > 0.0f || tmpRun > 0.0f)
		{
			if (tmpRun > 0.0f) accel = 3.0f * tmpRun;
			
			float time = dt * tmpJoy * characterSpeed;

			if (isswimming)
			{
				setAnimationMode("Swim_loop", time);
				idleanim = false;
			} else
			{
				if (tmpRun > 0.0f)
				{
					setAnimationMode("Run", time);
					idleanim = false;
				} else
				{
					setAnimationMode("Walk", time);
					idleanim = false;
				}
			}
			// 0.005f fixes character getting stuck on meshes
			position += dt * characterSpeed * 1.5f * accel * Vector3(cos(characterRotation.valueRadians()), 0.01f, sin(characterRotation.valueRadians()));
		} else if (tmpBack > 0.0f)
		{
			float time = -dt * characterSpeed;
			if (isswimming)
			{
				setAnimationMode("Spot_swim", time);
				idleanim = false;
			} else
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
				setAnimationMode("Spot_swim", dt * 0.5f);
			} else
			{
				setAnimationMode("Idle_sway", dt * 0.05f);
			}
		}

		mCharacterNode->setPosition(position);
		updateMapIcon();
	} else if (beamCoupling)
	{
		// if physics is disabled, its attached to a beam truck maybe?
		Vector3 pos;
		Quaternion rot;
		float angle = beamCoupling->hydrodirwheeldisplay * -1.0f; // not getSteeringAngle(), but this, as its smoothed
		int res = beamCoupling->calculateDriverPos(pos, rot);
		if (!res)
		{
			setPosition(pos + rot * Vector3(0.0f,-0.6f,-0.1f)); // hack to position the character right perfect on the default seat
			mCharacterNode->setOrientation(rot);
			setAnimationMode("driving");
			Real lenght = mAnimState->getAnimationState("driving")->getLength();
			float timePos = ((angle + 1.0f) * 0.5f) * lenght;
			//LOG("angle: " + TOSTRING(angle) + " / " + TOSTRING(timePos));
			// prevent animation flickering on the borders:
			if (timePos < 0.01f)
			{
				timePos = 0.01f;
			}
			if (timePos > lenght - 0.01f)
			{
				timePos = lenght - 0.01f;
			}
			mAnimState->getAnimationState("driving")->setTimePosition(timePos);
		}

	}

#ifdef USE_SOCKETW
	if (gEnv->network && !remote)
	{
		sendStreamData();
	}
#endif // USE_SOCKETW
}

void Character::updateMapIcon()
{
#ifdef USE_MYGUI
	if (!gEnv->surveyMap) return;
	SurveyMapEntity* e = gEnv->surveyMap->getMapEntityByName(myName);
	if (e)
	{
		e->setPosition(mCharacterNode->getPosition());
		e->setRotation(mCharacterNode->getOrientation());
		e->setVisibility(!beamCoupling);
	} else
	{
		createMapEntity();
	}
#endif // USE_MYGUI
}

void Character::move(Vector3 offset)
{
	mCharacterNode->translate(offset);
}

void Character::sendStreamSetup()
{
	if (remote) return;
	// new local stream
	stream_register_t reg;
	memset(&reg, 0, sizeof(reg));
	reg.status = 1;
	strcpy(reg.name, "default");
	reg.type = 1;
	reg.data[0] = 2;

	NetworkStreamManager::getSingleton().addLocalStream(this, &reg);
}

void Character::sendStreamData()
{
	int t = netTimer.getMilliseconds();
	if (t-last_net_time < 100)
		return;

	// do not send position data if coupled to a truck already
	if (beamCoupling) return;

	last_net_time = t;

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

	//LOG("sending character stream data: " + TOSTRING(net->getUserID()) + ":"+ TOSTRING(streamid));
	this->addPacket(MSG2_STREAM_DATA, sizeof(pos_netdata_t), (char*)&data);
}

void Character::receiveStreamData(unsigned int &type, int &source, unsigned int &streamid, char *buffer, unsigned int &len)
{
	if (type == MSG2_STREAM_DATA && this->source == source && this->streamid == streamid)
	{
		header_netdata_t *header = (header_netdata_t *)buffer;
		if (header->command == CHARCMD_POSITION)
		{
			// position
			pos_netdata_t *data = (pos_netdata_t *)buffer;
			Vector3 pos(data->posx, data->posy, data->posz);
			this->setPosition(pos);
			Quaternion rot(data->rotw, data->rotx, data->roty, data->rotz);
			mCharacterNode->setOrientation(rot);
			setAnimationMode(getASCIIFromCharString(data->animationMode, 255), data->animationTime);
		} else if (header->command == CHARCMD_ATTACH)
		{
			// attach
			attach_netdata_t *data = (attach_netdata_t *)buffer;
			if (data->enabled)
			{
				Beam *beam = BeamFactory::getSingleton().getBeam(data->source_id, data->stream_id);
				setBeamCoupling(true, beam);
			} else
			{
				setBeamCoupling(false);
			}
		}
	}
}

void Character::updateNetLabelSize()
{
	if (!this || !gEnv->network || !mMoveableText) return;

	mMoveableText->setVisible(getVisible());

	if (!mMoveableText->isVisible()) return; // why not getVisible()?

	float camDist = (mCharacterNode->getPosition() - mCamera->getPosition()).length();
	float h = std::max(9.0f, camDist * 1.2f);

	mMoveableText->setCharacterHeight(h);

	if (camDist > 1000.0f)
		mMoveableText->setCaption(networkUsername + "  (" + TOSTRING((float)(ceil(camDist / 100) / 10.0f))+ " km)");
	else if (camDist > 20.0f && camDist <= 1000.0f)
		mMoveableText->setCaption(networkUsername + "  (" + TOSTRING((int)camDist)+ " m)");
	else
		mMoveableText->setCaption(networkUsername);
}

void Character::setBeamCoupling(bool enabled, Beam *truck /* = 0 */)
{
	if (enabled)
	{
		if (!truck) return;
		beamCoupling = truck;
		setPhysicsEnabled(false);
		if (mMoveableText && mMoveableText->isVisible())
		{
			mMoveableText->setVisible(false);
		}
		if (gEnv->network && !remote)
		{
			attach_netdata_t data;
			data.command = CHARCMD_ATTACH;
			data.enabled = true;
			data.source_id = beamCoupling->getSourceID();
			data.stream_id = beamCoupling->getStreamID();
			this->addPacket(MSG2_STREAM_DATA, sizeof(attach_netdata_t), (char*)&data);
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
	} else
	{
		isCoupled = false;
		setPhysicsEnabled(true);
		beamCoupling = 0;
		if (mMoveableText && !mMoveableText->isVisible())
		{
			mMoveableText->setVisible(true);
		}
		if (gEnv->network && !remote)
		{
			attach_netdata_t data;
			data.command = CHARCMD_ATTACH;
			data.enabled = false;
			this->addPacket(MSG2_STREAM_DATA, sizeof(attach_netdata_t), (char*)&data);
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
