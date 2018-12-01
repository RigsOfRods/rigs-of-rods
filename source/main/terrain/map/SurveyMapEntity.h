/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2013-2017 Petr Ohlidal & contributors

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

#pragma once

#include "RoRPrerequisites.h"

#include "mygui/BaseLayout.h"

ATTRIBUTE_CLASS_LAYOUT(SurveyMapEntity, "MapEntity.layout");

class SurveyMapEntity : public wraps::BaseLayout, public ZeroedMemoryAllocator
{
public:

    SurveyMapEntity(Ogre::String type, MyGUI::StaticImagePtr parent);

    void setCaption(Ogre::String s);
    void setPosition(float x, float z);
    void setRotation(float r);
    void setState(int state);
    void setVisibility(bool value);

private:

    MyGUI::StaticImagePtr mParent;

    ATTRIBUTE_FIELD_WIDGET_NAME(SurveyMapEntity, mCaption, "mCaption");

    MyGUI::StaticText* mCaption;

    ATTRIBUTE_FIELD_WIDGET_NAME(SurveyMapEntity, mIcon, "mIcon");

    MyGUI::StaticImage* mIcon;
    MyGUI::RotatingSkin* mIconRotating;

    MyGUI::IntSize mIconSize;
    Ogre::Real mRotation;
    Ogre::String mFileName;
    Ogre::String mType;
    bool mIsStatic;

    void updateIcon();
};


