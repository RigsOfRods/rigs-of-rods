/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2013-2025 Petr Ohlidal

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

#include "Wavefield.h"

#include "Actor.h"
#include "AppContext.h"
#include "CameraManager.h"
#include "GfxScene.h"
#include "PlatformUtils.h" // PathCombine
#include "Terrain.h"

#include <Ogre.h>

using namespace RoR;

Wavefield::Wavefield(Vec3 terrn_size) :
    Hydrax::Noise::Noise("RoR_Wavefield", false),
    m_map_size(terrn_size)
{
    if (m_map_size.x < 1500 && m_map_size.z < 1500)
        m_waterplane_mesh_scale = 1.5f;

    char line[1024] = {};
    std::string filepath = PathCombine(RoR::App::sys_config_dir->getStr(), "wavefield.cfg");
    FILE* fd = fopen(filepath.c_str(), "r");
    if (fd)
    {
        while (!feof(fd))
        {
            int res = fscanf(fd, " %[^\n\r]", line);
            if (line[0] == ';')
                continue;
            float wl, amp, mx, dir;
            res = sscanf(line, "%f, %f, %f, %f", &wl, &amp, &mx, &dir);
            if (res < 4)
                continue;

            WaveTrain wavetrain;
            wavetrain.wavelength = wl;
            wavetrain.amplitude = amp;
            wavetrain.maxheight = mx;
            wavetrain.direction = dir / 57.0;
            wavetrain.dir_sin = sin(wavetrain.direction);
            wavetrain.dir_cos = cos(wavetrain.direction);

            m_wavetrain_defs.push_back(wavetrain);
        }
        fclose(fd);
    }
    for (size_t i = 0; i < m_wavetrain_defs.size(); i++)
    {
        m_wavetrain_defs[i].wavespeed = 1.25 * sqrt(m_wavetrain_defs[i].wavelength);
        m_max_ampl += m_wavetrain_defs[i].maxheight;
    }
}

float Wavefield::GetStaticWaterHeight()
{
    return m_water_height;
};

void Wavefield::SetStaticWaterHeight(float value)
{
    m_water_height = value;
}

void Wavefield::SetWavesHeight(float value)
{
    m_waves_height = value;
}

float Wavefield::CalcWavesHeight(Vec3 pos, float timeshift_sec)
{
    // no waves?
    if (!RoR::App::gfx_water_waves->getBool() || RoR::App::mp_state->getEnum<MpState>() == RoR::MpState::CONNECTED)
    {
        // constant height, sea is flat as pancake
        return m_water_height;
    }

    // uh, some upper limit?!
    if (pos.y > m_water_height + m_max_ampl)
        return m_water_height;

    const float time_sec = m_sim_time_counter + timeshift_sec;

    float waveheight = GetWaveHeight(pos);
    // we will store the result in this variable, init it with the default height
    float result = m_water_height;
    // now walk through all the wave trains. One 'train' is one sin/cos set that will generate once wave. All the trains together will sum up, so that they generate a 'rough' sea
    for (size_t i = 0; i < m_wavetrain_defs.size(); i++)
    {
        // calculate the amplitude that this wave will have. wavetrains[i].amplitude is read from the config
        // upper limit: prevent too big waves by setting an upper limit
        float amp = std::min(m_wavetrain_defs[i].amplitude * waveheight, m_wavetrain_defs[i].maxheight);
        // now the main thing:
        // calculate the sinus with the values of the config file and add it to the result
        result += amp * sin(Ogre::Math::TWO_PI * ((time_sec * m_wavetrain_defs[i].wavespeed + m_wavetrain_defs[i].dir_sin * pos.x + m_wavetrain_defs[i].dir_cos * pos.z) / m_wavetrain_defs[i].wavelength));
    }
    // return the summed up waves
    return result;
}

bool Wavefield::IsUnderWater(Vec3 pos)
{
    float waterheight = m_water_height;

    if (RoR::App::gfx_water_waves->getBool() && RoR::App::mp_state->getEnum<MpState>() == RoR::MpState::DISABLED)
    {
        float waveheight = GetWaveHeight(pos);

        if (pos.y > m_water_height + m_max_ampl * waveheight || pos.y > m_water_height + m_max_ampl)
            return false;

        waterheight = CalcWavesHeight(pos);
    }

    return pos.y < waterheight;
}

Vec3 Wavefield::CalcWavesVelocity(Vec3 pos, float timeshift_sec)
{
    if (!RoR::App::gfx_water_waves->getBool() || RoR::App::mp_state->getEnum<MpState>() == RoR::MpState::CONNECTED)
        return Vec3();

    float waveheight = GetWaveHeight(pos);

    if (pos.y > m_water_height + m_max_ampl)
        return Vec3();

    Vec3 result;

    const float time_sec = m_sim_time_counter + timeshift_sec;

    for (size_t i = 0; i < m_wavetrain_defs.size(); i++)
    {
        float amp = std::min(m_wavetrain_defs[i].amplitude * waveheight, m_wavetrain_defs[i].maxheight);
        float speed = Ogre::Math::TWO_PI * amp / (m_wavetrain_defs[i].wavelength / m_wavetrain_defs[i].wavespeed);
        float coeff = Ogre::Math::TWO_PI * (time_sec * m_wavetrain_defs[i].wavespeed + m_wavetrain_defs[i].dir_sin * pos.x + m_wavetrain_defs[i].dir_cos * pos.z) / m_wavetrain_defs[i].wavelength;
        result.y += speed * cos(coeff);
        result += Vec3(m_wavetrain_defs[i].dir_sin, 0, m_wavetrain_defs[i].dir_cos) * speed * sin(coeff);
    }

    return result;
}

void Wavefield::FrameStepWaveField(float dt)
{
    m_sim_time_counter += dt;
}

float Wavefield::GetWaveHeight(Vec3 pos)
{
    // calculate how high the waves should be at this point
    //  (mapsize.x * m_waterplane_mesh_scale) / 2 = terrain width / 2
    //  (mapsize.z * m_waterplane_mesh_scale) / 2 = terrain height / 2
    // calculate distance to the center of the terrain and divide by 3.000.000
    float waveheight = (pos - Vec3((m_map_size.x * m_waterplane_mesh_scale) * 0.5, m_water_height, (m_map_size.z * m_waterplane_mesh_scale) * 0.5)).squaredLength() / 3000000.0;
    waveheight += m_waves_height;

    return waveheight;
}

// Implementation of Hydrax::Noise interface
float Wavefield::getValue(const float& x, const float& z)
{
    return this->CalcWavesHeight(Vec3(x, 0, z)) - m_water_height;
}

