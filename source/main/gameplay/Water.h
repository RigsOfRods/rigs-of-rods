/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2013-2020 Petr Ohlidal

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

#include "IWater.h"
#include "Application.h"

#include <Ogre.h>
#include <vector>

namespace RoR {

class Water : public IWater, public ZeroedMemoryAllocator
{
public:

    Water(Ogre::Vector3 terrn_size);
    ~Water();

    // Interface `IWater`
    float          GetStaticWaterHeight() override;
    void           SetStaticWaterHeight(float value) override;
    void           SetWaterBottomHeight(float value) override;
    float          CalcWavesHeight(Ogre::Vector3 pos) override;
    Ogre::Vector3  CalcWavesVelocity(Ogre::Vector3 pos) override;
    void           SetWaterVisible(bool value) override;
    bool           IsUnderWater(Ogre::Vector3 pos) override;
    void           SetReflectionPlaneHeight(float centerheight) override;
    void           UpdateReflectionPlane(float h) override;
    void           WaterPrepareShutdown() override;
    void           UpdateWater() override;
    void           FrameStepWater(float dt) override;
    void           SetForcedCameraTransform(Ogre::Radian fovy, Ogre::Vector3 pos, Ogre::Quaternion rot) override;
    void           ClearForcedCameraTransform() override;

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

    float          GetWaveHeight(Ogre::Vector3 pos);
    bool           IsCameraUnderWater();
    void           PrepareWater();

    bool                  m_water_visible;
    float                 m_water_height;
    float                 m_bottom_height;
    float                 m_max_ampl;
    float                 m_waterplane_mesh_scale;
    bool                  m_waterplane_force_update_pos;
    Ogre::Vector3         m_map_size;
    Ogre::Plane           m_water_plane;
    Ogre::MeshPtr         m_waterplane_mesh;
    Ogre::v1::Entity*     m_waterplane_entity;
    Ogre::SceneNode*      m_waterplane_node;
    Ogre::SceneNode*      m_bottomplane_node;
    Ogre::Plane           m_bottom_plane;
    std::vector<WaveTrain>  m_wavetrain_defs;

    // Forced camera transforms, used by UpdateWater()
    bool                  m_cam_forced;
    Ogre::Radian          m_cam_forced_fovy;
    Ogre::Vector3         m_cam_forced_position;
    Ogre::Quaternion      m_cam_forced_orientation;
};

} // namespace RoR
