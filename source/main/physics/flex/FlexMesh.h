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

#include "Application.h"

#include "Flexable.h"
#include "SimData.h" // NodeNum_t

#include <OgreString.h>
#include <OgreEntity.h>
#include <OgreVector3.h>
#include <OgreMesh.h>
#include <OgreSubMesh.h>
#include <OgreHardwareBuffer.h>

namespace RoR {

/// @addtogroup Gfx
/// @{

/// @addtogroup Flex
/// @{

class FlexMesh: public Flexable
{
public:

    FlexMesh(
        Ogre::String const& name,
        RoR::GfxActor* gfx_actor,
        NodeNum_t n1,
        NodeNum_t n2,
        NodeNum_t nstart,
        int nrays,
        Ogre::String const& face_material_name,
        Ogre::String const& band_material_name,
        bool rimmed = false,
        float rimratio = 1.f
    );

    ~FlexMesh();

    Ogre::Vector3 updateVertices();

    // Flexable
    bool flexitPrepare() { return true; };
    void flexitCompute();
    Ogre::Vector3 flexitFinal();

    void setVisible(bool visible) {} // Nothing to do here

private:

    struct FlexMeshVertex // staging
    {
        Ogre::Vector3 position = Ogre::Vector3::ZERO;
        Ogre::Vector3 normal = Ogre::Vector3::ZERO;
        Ogre::Vector2 texcoord = Ogre::Vector2::ZERO;
    };

    // Wheel
    Ogre::Vector3     m_flexit_center;
    RoR::GfxActor*    m_gfx_actor;
    int               m_num_rays;
    bool              m_is_rimmed;

    // Meshes
    Ogre::MeshPtr     m_mesh;
    Ogre::SubMesh*    m_submesh_wheelface = nullptr;
    Ogre::SubMesh*    m_submesh_tiretread = nullptr;
    Ogre::VertexDeclaration* m_vertex_format;
    Ogre::HardwareVertexBufferSharedPtr m_hw_vbuf;

    // Vertices - staging
    std::vector<FlexMeshVertex> m_vertices;
    std::vector<NodeNum_t> m_vertex_nodes;

    // Indices
    std::vector<uint16_t> m_wheelface_indices;
    std::vector<uint16_t> m_tiretread_indices;

    // Skeletal animation (1 bone per node)
};

/// @} // addtogroup Flex
/// @} // addtogroup Gfx

} // namespace RoR
