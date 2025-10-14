/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2013-2016 Petr Ohlidal

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

#include "Hydrax.h"
#include "IWater.h"
#include "ProjectedGrid.h"

#include <Ogre.h>

namespace RoR {

/// @addtogroup Gfx
/// @{

class HydraxWater : public IWater
{
public:

    HydraxWater(float waterHeight, Ogre::String configFile = "HydraxDefault.hdx");
    ~HydraxWater();

    // Interface IWater
    float          GetStaticWaterHeight() override;
    void           SetStaticWaterHeight(float value) override;
    float          CalcWavesHeight(Ogre::Vector3 pos, float timeshift = 0.f) override;
    Ogre::Vector3  CalcWavesVelocity(Ogre::Vector3 pos, float timeshift = 0.f) override;
    void           SetWaterVisible(bool value) override;
    void           WaterSetSunPosition(Ogre::Vector3) override;
    bool           IsUnderWater(Ogre::Vector3 pos) override;
    void           FrameStepWater(float dt) override;
    void           UpdateWater() override;

    Hydrax::Hydrax* GetHydrax() { return mHydrax; }

protected:

    void InitHydrax();
    Hydrax::Hydrax* mHydrax;
    float waveHeight;
    float waterHeight;
    Hydrax::Noise::Perlin* waternoise;
    Hydrax::Module::ProjectedGrid* mModule;
    Ogre::String CurrentConfigFile;
};

/// @} // addtogroup Gfx

} // namespace RoR
