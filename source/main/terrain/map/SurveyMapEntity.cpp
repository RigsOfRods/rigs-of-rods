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

#include "SurveyMapEntity.h"

#include "Application.h"
#include "Beam.h"

using namespace Ogre;

SurveyMapEntity::SurveyMapEntity(Vector2 terrain_size, String type, MyGUI::StaticImagePtr parent) :
      mFileName("icon_" + type + ".dds")
    , mMapSize(terrain_size)
    , mParent(parent)
    , mRotation(0)
    , mType(type)
{
    initialiseByAttributes(this, parent);

    mIconRotating = mIcon->getSubWidgetMain()->castType<MyGUI::RotatingSkin>(false);

    // check if static only icon
    String group = ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME;
    mIsStatic = ResourceGroupManager::getSingleton().resourceExists(group, mFileName);

    updateIcon();
}

void SurveyMapEntity::setPosition(Vector3 pos)
{
    setPosition(pos.x, pos.z);
}

void SurveyMapEntity::setPosition(float x, float z)
{
    mMainWidget->setPosition(
        x / mMapSize.x * mParent->getWidth()  - mMainWidget->getWidth()  / 2,
        z / mMapSize.y * mParent->getHeight() - mMainWidget->getHeight() / 2
    );
    mIcon->setCoord(
        mMainWidget->getWidth() / 2 - mIconSize.width / 2,
        mMainWidget->getHeight() / 2 - mIconSize.height / 2,
        mIconSize.width,
        mIconSize.height
    );
}

void SurveyMapEntity::setRotation(float r)
{
    mRotation = r;
    if (mIconRotating)
    {
        mIconRotating->setCenter(MyGUI::IntPoint(mIcon->getWidth() / 2, mIcon->getHeight() / 2));
        mIconRotating->setAngle(mRotation);
    }
}

void SurveyMapEntity::setVisibility(bool v)
{
    mMainWidget->setVisible(v);
    mCaption->setVisible(v);
    mIcon->setVisible(v);
}

void SurveyMapEntity::setState(int truckstate)
{
    if (mIsStatic)
        return;

    String fileName = "icon_" + mType + "_sleeping" + ".dds";  // orange icon | "_deactivated" -> yellow icon

    switch (truckstate)
    {
    case static_cast<int>(Actor::SimState::LOCAL_SIMULATED):
        fileName = "icon_" + mType + "_activated" + ".dds"; // green icon
        break;
    case static_cast<int>(Actor::SimState::NETWORKED_OK):
        fileName = "icon_" + mType + "_networked" + ".dds"; // blue icon
        break;
    default:
        fileName = "icon_" + mType + ".dds"; // grey icon
        break;
    }

    if (mFileName != fileName)
    {
        mFileName = fileName;
        updateIcon();
    }
}

void SurveyMapEntity::setCaption(String s)
{
    mCaption->setCaption(s);
}

void SurveyMapEntity::updateIcon()
{
    // set image texture to load it into memory, so TextureManager::getByName will have it loaded if files exist
    mIcon->setImageTexture(mFileName);

    TexturePtr texture = (TexturePtr)(TextureManager::getSingleton().getByName(mFileName));
    if (texture.isNull())
    {
        mFileName = "icon_missing.dds";
        texture = (TexturePtr)(TextureManager::getSingleton().getByName(mFileName));
    }

    if (!texture.isNull())
    {
        mIconSize.width = (int)texture->getWidth();
        mIconSize.height = (int)texture->getHeight();
        mIcon->setSize(mIconSize);
    }
}
