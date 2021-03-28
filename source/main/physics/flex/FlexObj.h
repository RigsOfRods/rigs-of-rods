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
#include "SimData.h"

#include <Ogre.h>

namespace RoR {

/// Texture coordinates for old-style actor body (the "cab")
struct CabTexcoord
{
    NodeIdx_t    node_id;
    float  texcoord_u;
    float  texcoord_v;
};

enum CabBackmeshType { BACKMESH_NONE, BACKMESH_OPAQUE, BACKMESH_TRANSPARENT };

/// Submesh for old-style actor body (the "cab")
struct CabSubmesh
{
    CabSubmesh(): backmesh_type(BACKMESH_NONE), texcoords_pos(0), cabs_pos(0) {}

    CabBackmeshType  backmesh_type;
    size_t        texcoords_pos;
    size_t        cabs_pos;
};

/// A visual mesh, forming a chassis for softbody actor
/// At most one instance is created per actor.
class FlexObj : public ZeroedMemoryAllocator
{
public:

    FlexObj(
        RoR::GfxActor* gfx_actor,
        node_t* all_nodes, // For initial setup only, pointer is not stored
        std::vector<CabTexcoord>& texcoords,
        int numtriangles,
        int* triangles,
        std::vector<CabSubmesh>& submeshes,
        char* texname,
        const char* name,
        char* backtexname,
        char* transtexname);

    ~FlexObj();

    Ogre::Vector3   UpdateFlexObj();
    void            ScaleFlexObj(float factor);

private:

    struct FlexObjVertex
    {
        Ogre::Vector3 position;
        Ogre::Vector3 normal;
        Ogre::Vector2 texcoord;
    };

    /// Compute vertex position in the vertexbuffer (0-based offset) for node `v` of triangle `tidx`
    int             ComputeVertexPos(int tidx, int v, std::vector<CabSubmesh>& submeshes);
    Ogre::Vector3   UpdateMesh();

    Ogre::MeshPtr               m_mesh;
    std::vector<Ogre::SubMesh*> m_submeshes;
    RoR::GfxActor*              m_gfx_actor;
    float*                      m_s_ref;

    size_t                      m_vertex_count;
    int*                        m_vertex_nodes;
    Ogre::VertexDeclaration*    m_vertex_format;
    Ogre::HardwareVertexBufferSharedPtr m_hw_vbuf;
    union
    {
        float*              m_vertices_raw;
        FlexObjVertex*      m_vertices;
    };

    size_t                      m_index_count;
    unsigned short*             m_indices;
    int                         m_triangle_count;	
};

} // namespace RoRs
