/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2017-2018 Petr Ohlidal & contributors

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

#include "ForwardDeclarations.h"
#include <OgreVector3.h>

class IWater //!< TODO: Mixed gfx+physics (waves) - must be separated ~ only_a_ptr, 02/2018
{
public:
    IWater()
    {
    }

    virtual ~IWater()
    {
    }

    virtual float          GetStaticWaterHeight() = 0; //!< Returns static water level configured in 'terrn2'
    virtual void           SetStaticWaterHeight(float value) = 0;
    virtual void           SetWaterBottomHeight(float value) {};
    virtual float          CalcWavesHeight(Ogre::Vector3 pos) = 0;
    virtual Ogre::Vector3  CalcWavesVelocity(Ogre::Vector3 pos) = 0;
    virtual void           SetWaterVisible(bool value) = 0;
    virtual void           WaterSetCamera(Ogre::Camera* cam) {}
    virtual void           WaterSetSunPosition(Ogre::Vector3) {}
    virtual bool           IsUnderWater(Ogre::Vector3 pos) = 0;
    virtual void           FrameStepWater(float dt) = 0;
    virtual void           SetReflectionPlaneHeight(float centerheight) {}
    virtual void           UpdateReflectionPlane(float h) {}
    virtual void           WaterPrepareShutdown() {}
    virtual void           UpdateWater() = 0;
};
