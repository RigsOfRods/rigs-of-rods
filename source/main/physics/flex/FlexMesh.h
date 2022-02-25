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

#include <OgreString.h>
#include <OgreEntity.h>
#include <Ogre.h>
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
        int n1,
        int n2,
        int nstart,
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

    struct FlexMeshVertex
    {
        Ogre::Vector3 position;
        Ogre::Vector3 normal;
        Ogre::Vector2 texcoord;
    };

    // Wheel
    Ogre::Vector3     m_flexit_center;
    RoR::GfxActor*    m_gfx_actor;
    int               m_num_rays;
    bool              m_is_rimmed;

    // Meshes
    Ogre::MeshPtr     m_mesh;
    Ogre::SubMesh*    m_submesh_wheelface;
    Ogre::SubMesh*    m_submesh_tiretread;
    Ogre::VertexDeclaration* m_vertex_format;
    Ogre::HardwareVertexBufferSharedPtr m_hw_vbuf;

    // Vertices
    FlexMeshVertex*   m_vertices;
    int*              m_vertex_nodes;

    // Indices
    unsigned short*   m_wheelface_indices;
    unsigned short*   m_tiretread_indices;
};

/// @} // addtogroup Flex
/// @} // addtogroup Gfx

} // namespace RoR
