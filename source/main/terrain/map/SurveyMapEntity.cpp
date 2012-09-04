/*
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
#ifdef USE_MYGUI

#include "Beam.h"
#include "SurveyMapEntity.h"
#include "SurveyMapManager.h"
#include "Ogre.h"

using namespace Ogre;

String SurveyMapEntity::entityStates[MaxEntityStates] = {"activated", "deactivated", "sleeping", "networked"};

SurveyMapEntity::SurveyMapEntity(SurveyMapManager *ctrl, String type, MyGUI::StaticImagePtr parent) :
	  mMapControl(ctrl)
	, mType(type)
	, mParent(parent)
	, mRotation(0)
	, mState(Sleeping)
	, mX(0)
	, mZ(0)
{
	initialiseByAttributes(this, parent);

	if (mIcon)
		mIconRotating = mIcon->getSubWidgetMain()->castType<MyGUI::RotatingSkin>(false);
	else
		mIconRotating = nullptr;

	init();
}

void SurveyMapEntity::init()
{
	// check if static only icon
	String imageFile = "icon_" + mType + ".dds";
	String group = ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME;

	if (ResourceGroupManager::getSingleton().resourceExists(group, imageFile))
	{
		//LOG("static map icon found: " + imageFile);
		mIsStatic = true;
	} else
	{
		LOG("static map icon not found: " + imageFile);
		mIsStatic = false;
	}

	setVisibility(false);

	updateIcon();
	update();
}

void SurveyMapEntity::setPosition(Vector3 pos)
{
	setPosition(pos.x, pos.z);
}

void SurveyMapEntity::setPosition(float x, float z)
{
	bool needUpdate = false;

	if (fabs(x - mX) > 0.00001f || fabs(z - mZ) > 0.00001f)
	{
		needUpdate = true;
	}

	mX = x;
	mZ = z;

	if (needUpdate)
	{
		update();
	}
}

void SurveyMapEntity::setRotation(Quaternion q)
{
	mRotation = q.getYaw().valueRadians() - Math::HALF_PI;
	if (mIconRotating) mIconRotating->setAngle(-mRotation);
}

void SurveyMapEntity::setRotation(Radian _r)
{
	mRotation = _r.valueRadians();
	if (mIconRotating) mIconRotating->setAngle(-mRotation);
}

bool SurveyMapEntity::getVisibility()
{
	return mMainWidget->getVisible();
}

void SurveyMapEntity::setVisibility(bool value)
{
	mMainWidget->setVisible(value);
}

void SurveyMapEntity::setState(int truckstate)
{
	if (mIsStatic) return;

	EntityStates mapstate = Sleeping;

	switch (truckstate)
	{
	case ACTIVATED:
		mapstate = Activated;
		break;
	case DESACTIVATED:
	case MAYSLEEP:
	case GOSLEEP:
		mapstate = Deactivated;
		break;
	case SLEEPING:
		mapstate = Sleeping;
		break;
	case NETWORKED:
		mapstate = Networked;
		break;
	default:
		mapstate = Sleeping;
	}

	if (mState != mapstate)
	{
		mState = mapstate;
		update();
	}
}

int SurveyMapEntity::getState()
{
	return mState;
}

void SurveyMapEntity::update()
{
	float wscale = 1.0f - mMapControl->getMapSize().length() / mMapControl->getWorldSize().length();

	mCaption->setVisible(wscale > 0.5f);

	Vector3 mapSize = mMapControl->getMapSize();
	mMainWidget->setPosition(
		mX / mapSize.x * mParent->getWidth() - mMainWidget->getWidth() / 2,
		mZ / mapSize.z * mParent->getHeight() - mMainWidget->getHeight() / 2
	);
	mIcon->setCoord(
		mMainWidget->getWidth() / 2 - mIconSize.width * wscale / 2,
		mMainWidget->getHeight() / 2 - mIconSize.height * wscale / 2,
		mIconSize.width * wscale,
		mIconSize.height * wscale
	);
	mIcon->setVisible(true);
}

void SurveyMapEntity::setDescription(String s)
{
	mDescription = s;
	mCaption->setCaption(mDescription);
}

String SurveyMapEntity::getDescription()
{
	return mDescription;
}

void SurveyMapEntity::updateIcon()
{
	// check if static only icon
	String imageFile = "icon_" + mType + "_" + entityStates[mState] + ".dds";

	if (mIsStatic)
	{
		imageFile = "icon_" + mType + ".dds";
	}

	// set image texture to load it into memory, so TextureManager::getByName will have it loaded if files exist
	mIcon->setImageTexture(imageFile);

	TexturePtr texture = (TexturePtr)(TextureManager::getSingleton().getByName(imageFile));
	if (texture.isNull())
	{
		imageFile = "icon_missing.dds";
		texture = (TexturePtr)(TextureManager::getSingleton().getByName(imageFile));
	}

	if (!texture.isNull())
	{
		mIconSize.width  = (int)texture->getWidth();
		mIconSize.height = (int)texture->getHeight();
		mIcon->setSize(mIconSize);
	}
	
	if (mIconRotating)
	{
		mIconRotating->setCenter(MyGUI::IntPoint(mIcon->getWidth()/2, mIcon->getHeight()/2));
		mIconRotating->setAngle(mRotation);
	}
}

#endif // USE_MYGUI
