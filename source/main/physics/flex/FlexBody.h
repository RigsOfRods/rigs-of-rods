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

#include "RigDef_Prerequisites.h"
#include "RoRPrerequisites.h"
#include "Flexable.h"
#include "Locator_t.h"

#include <OgreVector3.h>
#include <OgreQuaternion.h>
#include <OgreHardwareVertexBuffer.h>
#include <OgreMesh.h>

/// Flexbody = A deformable mesh; updated on CPU every frame, then uploaded to video memory
class FlexBody
{
    friend class RoR::FlexFactory;
    friend class RoR::FlexBodyFileIO;

    FlexBody( // Private, for FlexFactory
        RigDef::Flexbody* def,
        RoR::FlexBodyCacheData* preloaded_from_cache,
        RoR::GfxActor* gfx_actor,
        Ogre::Entity* entity,
        int ref, 
        int nx, 
        int ny,
        Ogre::Quaternion const & rot, 
        std::vector<unsigned int> & node_indices
    );

public:

    ~FlexBody();

    void printMeshInfo(Ogre::Mesh* mesh);
    void reset();
    void updateBlend();
    void writeBlend();

    /// Visibility control 
    /// @param mode {-2 = always, -1 = 3rdPerson only, 0+ = cinecam index}
    void setCameraMode(int mode) { m_camera_mode = mode; };
    int getCameraMode() { return m_camera_mode; };

    void ComputeFlexbody(); //!< Updates mesh deformation; works on CPU using local copy of vertex data.
    void UpdateFlexbodyVertexBuffers();

    void setVisible(bool visible);

    void SetFlexbodyCastShadow(bool val);

    int size() { return static_cast<int>(m_vertex_count); };

private:

    RoR::GfxActor*    m_gfx_actor;
    size_t            m_vertex_count;
    Ogre::Vector3     m_flexit_center; ///< Updated per frame

    Ogre::Vector3*    m_dst_pos;
    Ogre::Vector3*    m_src_normals;
    Ogre::Vector3*    m_dst_normals;
    Ogre::ARGB*       m_src_colors;
    Locator_t*        m_locators; ///< 1 loc per vertex

    int               m_node_center;
    int               m_node_x;
    int               m_node_y;
    Ogre::Vector3     m_center_offset;
    Ogre::SceneNode*  m_scene_node;
    Ogre::Entity*     m_scene_entity;
    int               m_camera_mode; ///< Visibility control {-2 = always, -1 = 3rdPerson only, 0+ = cinecam index}

    int                                 m_shared_buf_num_verts;
    Ogre::HardwareVertexBufferSharedPtr m_shared_vbuf_pos;
    Ogre::HardwareVertexBufferSharedPtr m_shared_vbuf_norm;
    Ogre::HardwareVertexBufferSharedPtr m_shared_vbuf_color;

    int                                 m_num_submesh_vbufs;
    int                                 m_submesh_vbufs_vertex_counts[16];
    Ogre::HardwareVertexBufferSharedPtr m_submesh_vbufs_pos[16];   ///< positions
    Ogre::HardwareVertexBufferSharedPtr m_submesh_vbufs_norm[16];  ///< normals
    Ogre::HardwareVertexBufferSharedPtr m_submesh_vbufs_color[16]; ///< colors

    bool m_uses_shared_vertex_data;
    bool m_has_texture;
    bool m_has_texture_blend;
    bool m_blend_changed;
};
