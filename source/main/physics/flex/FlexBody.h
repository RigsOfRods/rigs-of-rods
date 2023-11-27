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

#include "RigDef_Prerequisites.h"
#include "Application.h"
#include "Locator_t.h"
#include "SimData.h"
#include "RigDef_File.h"
#include "Utils.h"

#include <Ogre.h>

namespace RoR {

/// @addtogroup Gfx
/// @{

/// @addtogroup Flex
/// @{

/// Flexbody = A deformable mesh; updated on CPU every frame, then uploaded to video memory
class FlexBody
{
    friend class RoR::FlexFactory;
    friend class RoR::FlexBodyFileIO;

    FlexBody( // Private, for FlexFactory
        RoR::FlexBodyCacheData* preloaded_from_cache,
        RoR::GfxActor* gfx_actor,
        Ogre::Entity* entity,
        NodeNum_t ref, 
        NodeNum_t nx, 
        NodeNum_t ny,
        Ogre::Vector3 offset,
        Ogre::Quaternion const & rot, 
        std::vector<unsigned int> & node_indices
    );

public:

    typedef int FlexBodyPlaceholder_t;
    static const FlexBodyPlaceholder_t TUNING_PLACEHOLDER = -11;

    FlexBody(FlexBodyPlaceholder_t, FlexbodyID_t id, const std::string& orig_meshname);
    ~FlexBody();

    void reset();
    void updateBlend();
    void writeBlend();

    /// Visibility control 
    /// @param mode {-2 = always, -1 = 3rdPerson only, 0+ = cinecam index}
    void setCameraMode(int mode) { m_camera_mode = mode; };
    int getCameraMode() { return m_camera_mode; };

    void computeFlexbody(); //!< Updates mesh deformation; works on CPU using local copy of vertex data.
    void updateFlexbodyVertexBuffers();

    bool isVisible() const;
    void setVisible(bool visible);

    void setFlexbodyCastShadow(bool val);

    int getVertexCount() { return static_cast<int>(m_vertex_count); };
    Locator_t& getVertexLocator(int vert) { ROR_ASSERT((size_t)vert < m_vertex_count); return m_locators[vert]; }
    Ogre::Vector3 getVertexPos(int vert) { ROR_ASSERT((size_t)vert < m_vertex_count); return m_dst_pos[vert] + m_flexit_center; }
    Ogre::Entity* getEntity() { return m_scene_entity; }
    const std::string& getOrigMeshName() const { return m_orig_mesh_name; }
    std::vector<NodeNum_t>& getForsetNodes() { return m_forset_nodes; };
    std::string getOrigMeshInfo() { return m_orig_mesh_info; }
    std::string getLiveMeshInfo() { return RoR::PrintMeshInfo("Live", m_scene_entity->getMesh()); }

    NodeNum_t getRefNode() { return m_node_center; }
    NodeNum_t getXNode() { return m_node_x; }
    NodeNum_t getYNode() { return m_node_y; }

    FlexbodyID_t getID() const { return m_id; }

private:

    void defragmentFlexbodyMesh();

    RoR::GfxActor*    m_gfx_actor = nullptr;
    size_t            m_vertex_count = 0;
    Ogre::Vector3     m_flexit_center = Ogre::Vector3::ZERO; //!< Updated per frame
    FlexbodyID_t      m_id = FLEXBODYID_INVALID; // Filled by FlexFactory

    Ogre::Vector3*    m_dst_pos = nullptr;
    Ogre::Vector3*    m_src_normals = nullptr;
    Ogre::Vector3*    m_dst_normals = nullptr;
    Ogre::ARGB*       m_src_colors = nullptr;
    Locator_t*        m_locators = nullptr; //!< 1 loc per vertex

    NodeNum_t         m_node_center = NODENUM_INVALID;
    NodeNum_t         m_node_x = NODENUM_INVALID;
    NodeNum_t         m_node_y = NODENUM_INVALID;
    Ogre::Vector3     m_center_offset = Ogre::Vector3::ZERO;
    Ogre::SceneNode*  m_scene_node = nullptr;
    Ogre::Entity*     m_scene_entity = nullptr;
    int               m_camera_mode = CAMERA_MODE_ALWAYS_VISIBLE; //!< Visibility control {-2 = always, -1 = 3rdPerson only, 0+ = cinecam index}

    int                                 m_shared_buf_num_verts = 0;
    Ogre::HardwareVertexBufferSharedPtr m_shared_vbuf_pos;
    Ogre::HardwareVertexBufferSharedPtr m_shared_vbuf_norm;
    Ogre::HardwareVertexBufferSharedPtr m_shared_vbuf_color;

    int                                 m_num_submesh_vbufs = 0;
    int                                 m_submesh_vbufs_vertex_counts[16];
    Ogre::HardwareVertexBufferSharedPtr m_submesh_vbufs_pos[16];   //!< positions
    Ogre::HardwareVertexBufferSharedPtr m_submesh_vbufs_norm[16];  //!< normals
    Ogre::HardwareVertexBufferSharedPtr m_submesh_vbufs_color[16]; //!< colors

    bool m_uses_shared_vertex_data = false;
    bool m_has_texture = true;
    bool m_has_texture_blend = true;
    bool m_blend_changed = false;

    // Diagnostic data, not used for calculations
    std::vector<NodeNum_t> m_forset_nodes;
    std::string m_orig_mesh_info;
    std::string m_orig_mesh_name;
};

/// @} // addtogroup Flex
/// @} // addtogroup Gfx

} // namespace RoR
