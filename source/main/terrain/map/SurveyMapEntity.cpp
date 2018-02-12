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



#include <Ogre.h>

#include "Beam.h"
#include "SurveyMapEntity.h"
#include "SurveyMapManager.h"
#include "TerrainManager.h"

using namespace Ogre;

String SurveyMapEntity::entityStates[MaxEntityStates] = {"activated", "deactivated", "sleeping", "networked"};

SurveyMapEntity::SurveyMapEntity(SurveyMapManager* ctrl, String type, MyGUI::StaticImagePtr parent) :
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
    }
    else
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
    bool needsUpdate = false;

    if (fabs(x - mX) > 0.00001f || fabs(z - mZ) > 0.00001f)
    {
        needsUpdate = true;
    }

    mX = x;
    mZ = z;

    if (needsUpdate)
    {
        update();
    }
}

void SurveyMapEntity::setRotation(Quaternion q)
{
    setRotation(Math::HALF_PI - q.getYaw().valueRadians());
}

void SurveyMapEntity::setRotation(Radian r)
{
    setRotation(r.valueRadians());
}

void SurveyMapEntity::setRotation(Real r)
{
    mRotation = r;
    if (mIconRotating)
    {
        mIconRotating->setAngle(-r);
    }
    updateIcon();
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
    if (mIsStatic)
        return;

    EntityStates mapstate = Sleeping;

    switch (truckstate)
    {
    case static_cast<int>(Actor::SimState::LOCAL_SIMULATED):
        mapstate = Activated;
        break;
    case static_cast<int>(Actor::SimState::NETWORKED_OK):
        mapstate = Networked;
    default:
        mapstate = Sleeping;
        break;
    }

    if (mState != mapstate)
    {
        mState = mapstate;
        updateIcon();
        update();
    }
}

int SurveyMapEntity::getState()
{
    return mState;
}

void SurveyMapEntity::update()
{
    if (!mMainWidget->getVisible())
        return;

    if (!mMapControl->getMapEntitiesVisible())
    {
        mMainWidget->setVisible(false);
        mIcon->setVisible(false);
        return;
    }

    Ogre::Vector3 max_size = RoR::App::GetSimTerrain()->getMaxTerrainSize();
    Vector2 m_terrain_page_size = Vector2(max_size.x, max_size.z);
    float wscale = mMapControl->getWindowSize().length() / m_terrain_page_size.length();

    // TODO: Fix the icon positions based on the overview map size and zoom value
    // TODO: Split visibility calculation and position update into two functions

    mMainWidget->setPosition(
        mX / mMapControl->getMapSize().x * mParent->getWidth() - mMainWidget->getWidth() / 2,
        mZ / mMapControl->getMapSize().z * mParent->getHeight() - mMainWidget->getHeight() / 2
    );
    mIcon->setCoord(
        mMainWidget->getWidth() / 2 - mIconSize.width * wscale / 2,
        mMainWidget->getHeight() / 2 - mIconSize.height * wscale / 2,
        mIconSize.width * wscale,
        mIconSize.height * wscale
    );

    mIcon->setVisible(true);
    mCaption->setVisible(wscale > 0.5f);
    mMainWidget->setVisible(wscale > 0.5f);
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
        mIconSize.width = (int)texture->getWidth();
        mIconSize.height = (int)texture->getHeight();
        mIcon->setSize(mIconSize);
    }

    if (mIconRotating)
    {
        mIconRotating->setCenter(MyGUI::IntPoint(mIcon->getWidth() / 2, mIcon->getHeight() / 2));
        mIconRotating->setAngle(mRotation);
    }
}


