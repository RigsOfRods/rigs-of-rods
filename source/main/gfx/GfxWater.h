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

#include "IGfxWater.h"
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

class GfxWater : public IGfxWater
{
public:

    GfxWater(Ogre::Vector3 terrn_size, float initial_water_height);
    ~GfxWater();

    void SetWaterBottomHeight(float value) override;
    void SetWaterVisible(bool value) override;
    void SetReflectionPlaneHeight(float centerheight) override;
    void UpdateReflectionPlane(float h) override;
    void WaterPrepareShutdown() override;
    void UpdateWater() override;
    void FrameStepWater(float dt) override;
    void SetForcedCameraTransform(Ogre::Radian fovy, Ogre::Vector3 pos, Ogre::Quaternion rot) override;
    void ClearForcedCameraTransform() override;

private:

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

    void           ShowWave(Ogre::Vector3 refpos);
    bool           IsCameraUnderWater();
    void           PrepareWater();

    bool                  m_water_visible = true;
    float                 m_visual_water_height = 0.f;
    float                 m_bottom_height = 0.f;
    float                 m_waterplane_mesh_scale = 1.f;
    int                   m_frame_counter = 0;
    Ogre::Vector3         m_map_size = Ogre::Vector3::ZERO;
    Ogre::Plane           m_water_plane;
    Ogre::MeshPtr         m_waterplane_mesh;
    Ogre::Entity*         m_waterplane_entity = nullptr;
    Ogre::SceneNode*      m_waterplane_node = nullptr;
    Ogre::HardwareVertexBufferSharedPtr  m_waterplane_vert_buf;
    float*                m_waterplane_vert_buf_local = nullptr;
    bool                  m_waterplane_force_update_pos = false;
    Ogre::Plane           m_reflect_plane;
    Ogre::Plane           m_refract_plane;
    ReflectionListener    m_reflect_listener;
    RefractionListener    m_refract_listener;
    Ogre::Camera*         m_reflect_cam = nullptr;
    Ogre::Camera*         m_refract_cam = nullptr;
    Ogre::RenderTexture*  m_refract_rtt_target = nullptr;
    Ogre::RenderTexture*  m_reflect_rtt_target = nullptr;
    Ogre::TexturePtr      m_refract_rtt_texture;
    Ogre::TexturePtr      m_reflect_rtt_texture;
    Ogre::Viewport*       m_refract_rtt_viewport = nullptr;
    Ogre::Viewport*       m_reflect_rtt_viewport = nullptr;
    Ogre::SceneNode*      m_bottomplane_node = nullptr;
    Ogre::Plane           m_bottom_plane;

    // Forced camera transforms, used by UpdateWater()
    bool                  m_cam_forced = false;
    Ogre::Radian          m_cam_forced_fovy = Ogre::Radian(0.f);
    Ogre::Vector3         m_cam_forced_position = Ogre::Vector3::ZERO;
    Ogre::Quaternion      m_cam_forced_orientation = Ogre::Quaternion::IDENTITY;
};

/// @} // addtogroup Gfx

} // namespace RoR
