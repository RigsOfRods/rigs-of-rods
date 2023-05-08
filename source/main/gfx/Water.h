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

#include <OgreHardwareVertexBuffer.h> // Ogre::HardwareVertexBufferSharedPtr
#include <OgreMesh.h>
#include <OgrePlane.h>
#include <OgreRenderTargetListener.h>
#include <OgreTexture.h>
#include <OgreVector3.h>
#include <vector>

namespace RoR {

/// @addtogroup Gfx
/// @{

class Water : public IWater
{
public:

    Water(Ogre::Vector3 terrn_size);
    ~Water();

    // Interface `IWater`
    float          GetStaticWaterHeight() override;
    void           SetStaticWaterHeight(float value) override;
    void           SetWaterBottomHeight(float value) override;
    void           SetWavesHeight(float value) override;
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

    struct ReflectionListener: Ogre::RenderTargetListener
    {
        ReflectionListener(): scene_mgr(nullptr), waterplane_entity(nullptr) {}

        void preRenderTargetUpdate(const Ogre::RenderTargetEvent& evt) override;
        void postRenderTargetUpdate(const Ogre::RenderTargetEvent& evt) override;

        Ogre::SceneManager* scene_mgr;
        Ogre::Entity*       waterplane_entity;
    };

    struct RefractionListener: Ogre::RenderTargetListener
    {
        RefractionListener(): scene_mgr(nullptr), waterplane_entity(nullptr) {}

        void preRenderTargetUpdate(const Ogre::RenderTargetEvent& evt) override;
        void postRenderTargetUpdate(const Ogre::RenderTargetEvent& evt) override;

        Ogre::SceneManager* scene_mgr;
        Ogre::Entity*       waterplane_entity;
    };

    float          GetWaveHeight(Ogre::Vector3 pos);
    void           ShowWave(Ogre::Vector3 refpos);
    bool           IsCameraUnderWater();
    void           PrepareWater();

    bool                  m_water_visible;
    float                 m_water_height;
    float                 m_waves_height;
    float                 m_bottom_height;
    float                 m_max_ampl;
    float                 m_waterplane_mesh_scale;
    int                   m_frame_counter;
    Ogre::Vector3         m_map_size;
    Ogre::Plane           m_water_plane;
    Ogre::MeshPtr         m_waterplane_mesh;
    Ogre::Entity*         m_waterplane_entity;
    Ogre::SceneNode*      m_waterplane_node;
    Ogre::HardwareVertexBufferSharedPtr  m_waterplane_vert_buf;
    float*                m_waterplane_vert_buf_local;
    bool                  m_waterplane_force_update_pos;
    Ogre::Plane           m_reflect_plane;
    Ogre::Plane           m_refract_plane;
    ReflectionListener    m_reflect_listener;
    RefractionListener    m_refract_listener;
    Ogre::Camera*         m_reflect_cam;
    Ogre::Camera*         m_refract_cam;
    Ogre::RenderTexture*  m_refract_rtt_target;
    Ogre::RenderTexture*  m_reflect_rtt_target;
    Ogre::TexturePtr      m_refract_rtt_texture;
    Ogre::TexturePtr      m_reflect_rtt_texture;
    Ogre::Viewport*       m_refract_rtt_viewport;
    Ogre::Viewport*       m_reflect_rtt_viewport;
    Ogre::SceneNode*      m_bottomplane_node;
    Ogre::Plane           m_bottom_plane;
    std::vector<WaveTrain>  m_wavetrain_defs;

    // Forced camera transforms, used by UpdateWater()
    bool                  m_cam_forced;
    Ogre::Radian          m_cam_forced_fovy;
    Ogre::Vector3         m_cam_forced_position;
    Ogre::Quaternion      m_cam_forced_orientation;
};

/// @} // addtogroup Gfx

} // namespace RoR
