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
#include "Vec3.h"

namespace RoR {

/// @addtogroup Physics
/// @{

class Wavefield //!< Water physics, see 'wavefield.cfg' in your config directory.
{
public:
    Wavefield(Vec3 terrn_size);

    float GetStaticWaterHeight(); //!< Returns static water level configured in 'terrn2'
    void  SetStaticWaterHeight(float value);
    void  SetWavesHeight(float);
    float CalcWavesHeight(Vec3 pos, float timeshift_sec = 0.f);
    Vec3  CalcWavesVelocity(Vec3 pos, float timeshift_sec = 0.f);
    void  FrameStepWaveField(float dt);
    bool  IsUnderWater(Vec3 pos);
    float GetWaveHeight(Vec3 pos);

private:

    struct WaveTrain
    {
        float amplitude;
        float maxheight;
        float wavelength;
        float wavespeed;
        float direction;
        float dir_sin;
        float dir_cos;
    };
    std::vector<WaveTrain>  m_wavetrain_defs;

    float  m_waterplane_mesh_scale = 1.f;
    float  m_water_height = 0.f;
    float  m_waves_height = 0.f;
    float  m_bottom_height = 0.f;
    float  m_max_ampl = 0.f;
    float  m_sim_time_counter = 0.f; //!< Elapsed simulation time in seconds.
    Vec3   m_map_size;
};

/// @} // addtogroup Physics

} // namespace RoR
