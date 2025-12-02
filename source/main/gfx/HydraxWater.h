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
#include "IGfxWater.h"
#include "ProjectedGrid.h"

#include <Ogre.h>
#include <OgreTerrainGroup.h>

namespace RoR {

/// @addtogroup Gfx
/// @{

static const std::string HYDRAX_DEFAULT_CONFIG_FILE = "HydraxDefault.hdx"; // Fallback, in game resources   
static const std::string HYDRAX_USER_CONFIG_FILE = "hydrax.cfg"; // Primary, in user config dir

class HydraxWater : public IGfxWater
{
public:

    HydraxWater(Hydrax::Noise::Noise* noise, float waterHeight, Ogre::TerrainGroup* terrain_grp, Ogre::String configFile);
    ~HydraxWater();

    void SetWaterSunPosition(Ogre::Vector3 pos) override;
    void SetWaterVisible(bool value) override;
    void FrameStepWater(float dt) override;
    void UpdateWater() override;
    void SetWaterColor(Ogre::ColourValue color) override;

protected:

    void InitHydrax();
    Hydrax::Hydrax* mHydrax;
    float waveHeight;
    float waterHeight;
    Hydrax::Noise::Noise* waternoise;
    Hydrax::Module::ProjectedGrid* mModule;
    Ogre::String CurrentConfigFile;
};

/// @} // addtogroup Gfx

} // namespace RoR
