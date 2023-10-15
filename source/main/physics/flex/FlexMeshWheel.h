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

#include "ForwardDeclarations.h"
#include "FlexMesh.h"

#include <Ogre.h>
#include <string>
#include <vector>

namespace RoR {

/// @addtogroup Gfx
/// @{

/// @addtogroup Flex
/// @{

/// Consists of static mesh, representing the rim, and dynamic mesh, representing the tire.
class FlexMeshWheel: public Flexable
{
    friend class RoR::FlexFactory;

public:

    ~FlexMeshWheel();

    Ogre::Entity* getRimEntity() { return m_rim_entity; };
    Ogre::Entity* GetTireEntity() { return m_tire_entity; }

    Ogre::Vector3 updateVertices();

    // Flexable
    bool flexitPrepare();
    void flexitCompute();
    Ogre::Vector3 flexitFinal();

    void setVisible(bool visible);

private:

    FlexMeshWheel( // Use FlexFactory
        Ogre::Entity* rim_prop_entity,
        RoR::GfxActor* gfx_actor,
        int axis_node_1_index,
        int axis_node_2_index,
        int nstart,
        int nrays,
        std::string const& tire_mesh_name,
        std::string const& tire_material_name,
        float rimradius,
        bool rimreverse
    );

    struct FlexMeshWheelVertex
    {
        Ogre::Vector3 position;
        Ogre::Vector3 normal;
        Ogre::Vector2 texcoord;
    };

    // Wheel
    size_t           m_num_rays;
    float            m_rim_radius;
    RoR::GfxActor*   m_gfx_actor;
    int              m_axis_node0_idx;
    int              m_axis_node1_idx;
    int              m_start_node_idx; //!< First node (lowest index) belonging to this wheel.

    // Meshes
    Ogre::Vector3    m_flexit_center;
    Ogre::MeshPtr    m_mesh;
    Ogre::SubMesh*   m_submesh;
    bool             m_is_rim_reverse;
    Ogre::Entity*    m_rim_entity;
    Ogre::Entity*    m_tire_entity; // Assigned by friend FlexFactory
    Ogre::SceneNode* m_rim_scene_node;

    // Vertices
    float            m_norm_y;
    size_t           m_vertex_count;
    std::vector<FlexMeshWheelVertex> m_vertices;
    Ogre::VertexDeclaration* m_vertex_format;
    Ogre::HardwareVertexBufferSharedPtr m_hw_vbuf;

    // Indices
    size_t           m_index_count;
    std::vector<unsigned short>  m_indices;
};

/// @} // addtogroup Flex
/// @} // addtogroup Gfx

} // namespace RoR
