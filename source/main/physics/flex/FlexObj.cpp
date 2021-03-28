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

#include "FlexObj.h"

#include "ApproxMath.h" // fast_normalise()
#include "GfxActor.h"

#include <Ogre.h>

using namespace Ogre;
using namespace RoR;

FlexObj::FlexObj(RoR::GfxActor* gfx_actor, node_t* all_nodes, std::vector<CabTexcoord>& texcoords, int numtriangles, 
                 int* triangles, std::vector<CabSubmesh>& submesh_defs, 
                 char* texname, const char* name, char* backtexname, char* transtexname):
    m_gfx_actor(gfx_actor)
{
    m_triangle_count = numtriangles;

    // Create the mesh via the MeshManager
    m_mesh = MeshManager::getSingleton().createManual(name, gfx_actor->GetResourceGroup());

    // Create submeshes
    m_submeshes.reserve(submesh_defs.size());
    for (size_t j=0; j<submesh_defs.size(); j++)
    {
        Ogre::SubMesh* submesh = m_mesh->createSubMesh();
        switch (submesh_defs[j].backmesh_type)
        {
        case BACKMESH_OPAQUE:      submesh->setMaterialName(backtexname);  break;
        case BACKMESH_TRANSPARENT: submesh->setMaterialName(transtexname); break;
        default:                   submesh->setMaterialName(texname);
        }
        m_submeshes.push_back(submesh);
    };

    // Define the m_vertices_raw (8 vertices, each consisting of 3 groups of 3 floats
    m_vertex_count = texcoords.size();
    m_vertices_raw=(float*)malloc(((2*3+2)*m_vertex_count)*sizeof(float));
    m_vertex_nodes=(int*)malloc(m_vertex_count*sizeof(int));
    
    for (size_t i=0; i<m_vertex_count; i++)
    {
        m_vertex_nodes[i] = texcoords[i].node_id; //define node ids
        m_vertices[i].texcoord=Vector2(texcoords[i].texcoord_u, texcoords[i].texcoord_v); //textures coordinates
    }

    // Define triangles
    // The values in this table refer to vertices in the above table
    m_index_count = 3*numtriangles;
    m_indices=(unsigned short*)malloc(m_index_count*sizeof(unsigned short));
    for (size_t i=0; i<m_index_count; i++)
    {
        m_indices[i]=ComputeVertexPos(static_cast<int>(i/3), triangles[i], submesh_defs);
    }

    m_s_ref=(float*)malloc(numtriangles*sizeof(float));

    for (size_t i=0; i<(unsigned int)numtriangles;i++)
    {
        Ogre::Vector3 base_pos = all_nodes[m_vertex_nodes[m_indices[i*3]]].RelPosition;
        Ogre::Vector3 v1       = all_nodes[m_vertex_nodes[m_indices[i*3+1]]].RelPosition - base_pos;
        Ogre::Vector3 v2       = all_nodes[m_vertex_nodes[m_indices[i*3+2]]].RelPosition - base_pos;
        m_s_ref[i]=v1.crossProduct(v2).length()*2.0;
    }

    this->UpdateMesh(); // Initialize the dynamic mesh

    // Create vertex data structure for vertices shared between submeshes
    m_mesh->sharedVertexData = new VertexData();
    m_mesh->sharedVertexData->vertexCount = m_vertex_count;

    // Create declaration (memory format) of vertex data
    m_vertex_format = m_mesh->sharedVertexData->vertexDeclaration;
    size_t offset = 0;
    m_vertex_format->addElement(0, offset, VET_FLOAT3, VES_POSITION);
    offset += VertexElement::getTypeSize(VET_FLOAT3);
    m_vertex_format->addElement(0, offset, VET_FLOAT3, VES_NORMAL);
    offset += VertexElement::getTypeSize(VET_FLOAT3);
    m_vertex_format->addElement(0, offset, VET_FLOAT2, VES_TEXTURE_COORDINATES, 0);
    offset += VertexElement::getTypeSize(VET_FLOAT2);

    // Allocate vertex buffer of the requested number of vertices (vertexCount)
    // and bytes per vertex (offset)
    m_hw_vbuf = HardwareBufferManager::getSingleton().createVertexBuffer(
        offset, m_mesh->sharedVertexData->vertexCount, HardwareBuffer::HBU_DYNAMIC_WRITE_ONLY_DISCARDABLE);

    // Upload the vertex data to the card
    m_hw_vbuf->writeData(0, m_hw_vbuf->getSizeInBytes(), m_vertices_raw, true);

    // Set vertex buffer binding so buffer 0 is bound to our vertex buffer
    VertexBufferBinding* bind = m_mesh->sharedVertexData->vertexBufferBinding;
    bind->setBinding(0, m_hw_vbuf);

    // Set parameters of the submeshes
    for (size_t j=0; j<m_submeshes.size(); j++)
    {
        size_t index_count;
        if (j == 0)
            index_count = 3*submesh_defs[j].cabs_pos;
        else
            index_count = 3*(submesh_defs[j].cabs_pos-submesh_defs[j-1].cabs_pos); // 3 indices per triangle

        m_submeshes[j]->useSharedVertices = true;
        HardwareIndexBufferSharedPtr ibuf = HardwareBufferManager::getSingleton().createIndexBuffer(
             HardwareIndexBuffer::IT_16BIT,
             index_count,
             HardwareBuffer::HBU_STATIC_WRITE_ONLY);

        // Upload the index data to the card
        unsigned short* faces_ptr;
        if (j == 0)
            faces_ptr = &m_indices[0];
        else
            faces_ptr = &m_indices[submesh_defs[j-1].cabs_pos * 3];

        ibuf->writeData(0, ibuf->getSizeInBytes(), faces_ptr, true);
        m_submeshes[j]->indexData->indexBuffer = ibuf;
        m_submeshes[j]->indexData->indexCount = index_count;
        m_submeshes[j]->indexData->indexStart = 0;
    }

    // Set bounding information (for culling)
    m_mesh->_setBounds(AxisAlignedBox(-100,-100,-100,100,100,100), true);

    // Notify Mesh object that it has been loaded
    m_mesh->load();
}

void FlexObj::ScaleFlexObj(float factor)
{
    for (int i=0; i<m_triangle_count;i++)
    {
        m_s_ref[i] *= factor;
    }
}

/// Compute vertex position in the vertexbuffer (0-based offset) for node `v` of triangle `tidx`
int FlexObj::ComputeVertexPos(int tidx, int v, std::vector<CabSubmesh>& submeshes)
{
    // --- NOTE: The following logic is arcane, I'm keeping it mostly like I found it for stability ~ only_a_ptr, 08/2017

    // Find the 'context' - a submesh (and respective span of vertices) which the vertex belong to.
    int context;
    int num_submeshes = static_cast<int>(submeshes.size());
    for (context = 0; context < num_submeshes; ++context)
    {
        if (tidx < submeshes[context].cabs_pos)
        {
            --context;
            break;
        }
    }

    // Find the vertex itself
    int i_min = (context < 0) ? 0 : static_cast<int>(submeshes[context].texcoords_pos); // Fix for single-submesh case... It seems to have worked with a negative index until now ~ only_a_ptr, 08/2017
    int i_max = static_cast<int>(submeshes[context + 1].texcoords_pos);
    for (int i = i_min; i < i_max; ++i)
    {
        if (m_vertex_nodes[i] == v)
            return i;
    }

    return 0;
}

Vector3 FlexObj::UpdateMesh()
{
    RoR::GfxActor::SimBuffer::NodeSB* all_nodes = m_gfx_actor->GetSimNodeBuffer();
    Ogre::Vector3 center=(all_nodes[m_vertex_nodes[0]].AbsPosition+all_nodes[m_vertex_nodes[1]].AbsPosition)/2.0;
    for (size_t i=0; i<m_vertex_count; i++)
    {
        //set position
        m_vertices[i].position=all_nodes[m_vertex_nodes[i]].AbsPosition-center;
        //reset normals
        m_vertices[i].normal=Vector3::ZERO;
    }
    //accumulate normals per triangle
    for (size_t i=0; i<m_index_count/3; i++)
    {
        Vector3 v1, v2;
        v1=all_nodes[m_vertex_nodes[m_indices[i*3+1]]].AbsPosition-all_nodes[m_vertex_nodes[m_indices[i*3]]].AbsPosition;
        v2=all_nodes[m_vertex_nodes[m_indices[i*3+2]]].AbsPosition-all_nodes[m_vertex_nodes[m_indices[i*3]]].AbsPosition;
        v1=v1.crossProduct(v2);
        float s=v1.length();

        //avoid large tris
        if (s>m_s_ref[i])
        {
            m_vertices[m_indices[i*3+1]].position=m_vertices[m_indices[i*3]].position+Vector3(0.1,0,0);
            m_vertices[m_indices[i*3+2]].position=m_vertices[m_indices[i*3]].position+Vector3(0,0,0.1);
        }

        if (s == 0)
            continue;

        v1=v1/s;
        m_vertices[m_indices[i*3]].normal+=v1;
        m_vertices[m_indices[i*3+1]].normal+=v1;
        m_vertices[m_indices[i*3+2]].normal+=v1;
    }
    //normalize
    for (size_t i=0; i<m_vertex_count; i++)
    {
        m_vertices[i].normal = approx_normalise(m_vertices[i].normal);
    }

    return center;
}

Vector3 FlexObj::UpdateFlexObj()
{
    Ogre::Vector3 center = this->UpdateMesh();
    m_hw_vbuf->writeData(0, m_hw_vbuf->getSizeInBytes(), m_vertices_raw, true);
    return center;
}

FlexObj::~FlexObj()
{
    if (!m_mesh.isNull())
    {
        Ogre::MeshManager::getSingleton().remove(m_mesh->getHandle());
        m_mesh.setNull();
    }

    if (m_vertices_raw != nullptr) { free (m_vertices_raw); }
    if (m_vertex_nodes != nullptr) { free (m_vertex_nodes); }
    if (m_indices      != nullptr) { free (m_indices);      }
    if (m_s_ref        != nullptr) { free (m_s_ref);        }
}
