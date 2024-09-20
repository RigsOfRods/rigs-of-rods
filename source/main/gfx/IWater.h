/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2017-2020 Petr Ohlidal

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
#include "ProjectedGrid.h" // HydraX
#include <Ogre.h>

namespace RoR {

/// @addtogroup Gfx
/// @{

class IWater //!< Common to classic water (Water.cpp, `class Water`) and HydraX water (HydraxWater.cpp, `class HydraxWater`)
{
public:
    IWater()
    {
        m_active_water_mode = App::gfx_water_mode->getEnum<GfxWaterMode>();
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
    virtual void           SetWaterSunPosition(Ogre::Vector3) {}
    virtual bool           IsUnderWater(Ogre::Vector3 pos) = 0;
    virtual void           FrameStepWater(float dt) = 0;
    virtual void           SetReflectionPlaneHeight(float centerheight) {}
    virtual void           UpdateReflectionPlane(float h) {}
    virtual void           WaterPrepareShutdown() {}
    virtual void           UpdateWater() = 0;
    virtual void           SetWaterColor(Ogre::ColourValue color) {}
    virtual void           WaterSetSunPositon(Ogre::Vector3 pos) {}
    GfxWaterMode           GetActiveWaterMode() { return m_active_water_mode; }

    // Only used by classic water (Water.cpp)
    virtual void           SetWavesHeight(float value) {};
    virtual float          GetWavesHeight() { return 0.f; };

    // Only used by classic Water for SurveyMap texture creation
    virtual void           SetForcedCameraTransform(Ogre::Radian fovy, Ogre::Vector3 pos, Ogre::Quaternion rot) {};
    virtual void           ClearForcedCameraTransform() {};

    // Only for HydraX at the moment, but will be unified
    virtual Hydrax::Module::ProjectedGrid::Options   GetWaterGridOptions() { return Hydrax::Module::ProjectedGrid::Options(); }
    virtual void           SetWaterGridOptions(Hydrax::Module::ProjectedGrid::Options options) {};

protected:
    GfxWaterMode m_active_water_mode; //!< A snapshot of cvar `gfx_water_mode` at the time of water creation - because the cvar can change (i.e. via TopMenubar or scripting, see also `MSG_EDI_REINIT_WATER_REQUESTED`)
};

/// @} // addtogroup Gfx

} // namespace RoR
