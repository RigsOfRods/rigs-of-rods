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

#include "FlexMesh.h"

#include <Ogre.h>

#include "ApproxMath.h"
#include "BeamData.h"
#include "GfxActor.h"

using namespace Ogre;

FlexMesh::FlexMesh(
    Ogre::String const & name, 
    RoR::GfxActor* gfx_actor,
    int n1, 
    int n2, 
    int nstart, 
    int nrays,
    Ogre::String const & face_material_name, 
    Ogre::String const & band_material_name, 
    bool rimmed, 
    float rim_ratio
) :
      m_is_rimmed(rimmed)
    , m_num_rays(nrays)
    , m_gfx_actor(gfx_actor)
{
    // Create the mesh via the MeshManager
    m_mesh = MeshManager::getSingleton().createManual(name, gfx_actor->GetResourceGroup());

    // Create submeshes
    m_submesh_wheelface = m_mesh->createSubMesh();
    m_submesh_tiretread = m_mesh->createSubMesh();

    //materials
    m_submesh_wheelface->setMaterialName(face_material_name);
    m_submesh_tiretread->setMaterialName(band_material_name);

    // Define the vertices
    size_t vertex_count = 4*nrays+2; // each ray needs 4 verts (2 for sidewalls and 2 for band). The axis needs an extra 2.
    if (m_is_rimmed) // For truckfile sections "[mesh]wheels2".
    {
        vertex_count+=2*nrays; // 1 extra vertex for each sidewall.
    }
    m_vertices = new FlexMeshVertex[vertex_count];
    m_vertex_nodes=(int*)malloc(vertex_count*sizeof(int));

    //define node ids
    m_vertex_nodes[0]=n1;
    m_vertex_nodes[1]=n2;
    int i;
    for (i=0; i<nrays; i++)
    {
        //face
        m_vertex_nodes[2+i*2]=nstart+i*2;
        m_vertex_nodes[2+i*2+1]=nstart+i*2+1;
        if (m_is_rimmed)
        {
            //band
            m_vertex_nodes[2+2*nrays+i*2]   = nstart+2*nrays+i*2;
            m_vertex_nodes[2+2*nrays+i*2+1] = nstart+2*nrays+i*2+1;
            //face2 (outer)
            m_vertex_nodes[2+4*nrays+i*2]   = nstart+2*nrays+i*2;
            m_vertex_nodes[2+4*nrays+i*2+1] = nstart+2*nrays+i*2+1;
        } else
        {
            //band
            m_vertex_nodes[2+2*nrays+i*2]   = nstart+i*2;
            m_vertex_nodes[2+2*nrays+i*2+1] = nstart+i*2+1;
        }
    }

    //textures coordinates
    m_vertices[0].texcoord=Vector2(0.5, 0.5); // Axis vertices - texcoord is middle of the wheelface texture.
    m_vertices[1].texcoord=Vector2(0.5, 0.5);
    const bool odd_num_rays = (nrays % 2 == 0);
    for (i=0; i<nrays; i++)
    {
        //band
        int band_vert = 2+2*nrays+i*2;
        if (i % 2 == 0) // Even index
        {
            if (odd_num_rays && ((i+1) == nrays))
            {
                // Finalize a wheel with odd number of rays like 'bombinette.load' in 'miniredmars' map.
                m_vertices[band_vert].texcoord=Vector2(0.5, 0.0); // Stretch the texture over 2 quads instead of 1... ugly, but best we can do here
                m_vertices[band_vert+1].texcoord=Vector2(0.5, 1.0);
            }
            else
            {
                m_vertices[band_vert].texcoord=Vector2(0.0, 0.0);
                m_vertices[band_vert+1].texcoord=Vector2(0.0, 1.0);
            }
        }
        else // Odd index
        {
            m_vertices[band_vert].texcoord=Vector2(1.0, 0.0);
            m_vertices[band_vert+1].texcoord=Vector2(1.0, 1.0);
        }

        //face
        if (m_is_rimmed)
        {
            m_vertices[2+i*2].texcoord=Vector2(0.5+0.5*rim_ratio*sin((float)i*2.0*3.14159/nrays), 0.5+0.5*rim_ratio*cos((float)i*2.0*3.14159/nrays));
            m_vertices[2+i*2+1].texcoord=m_vertices[2+i*2].texcoord;
            m_vertices[2+4*nrays+i*2].texcoord=Vector2(0.5+0.5*sin(((float)i+0.5)*2.0*3.14159/nrays), 0.5+0.5*cos(((float)i+0.5)*2.0*3.14159/nrays));
            m_vertices[2+4*nrays+i*2+1].texcoord=m_vertices[2+4*nrays+i*2].texcoord;
        } else
        {
            m_vertices[2+i*2].texcoord=Vector2(0.5+0.5*sin(i*2.0*3.14159/nrays), 0.5+0.5*cos(i*2.0*3.14159/nrays));
            m_vertices[2+i*2+1].texcoord=m_vertices[2+i*2].texcoord;
        }
    }

    // Define triangles
    // The values in this table refer to vertices in the above table
    size_t tiretread_num_indices = 3*2*nrays;
    size_t wheelface_num_indices = 3*2*nrays;
    if (m_is_rimmed) wheelface_num_indices=wheelface_num_indices*3;
    m_wheelface_indices=(unsigned short*)malloc(wheelface_num_indices*sizeof(unsigned short));
    m_tiretread_indices=(unsigned short*)malloc(tiretread_num_indices*sizeof(unsigned short));
    for (i=0; i<nrays; i++)
    {
        //wheel sides
        m_wheelface_indices[3*(i*2)]=0;   m_wheelface_indices[3*(i*2)+1]=2+i*2;     m_wheelface_indices[3*(i*2)+2]=2+((i+1)%nrays)*2;
        m_wheelface_indices[3*(i*2+1)]=1; m_wheelface_indices[3*(i*2+1)+2]=2+i*2+1; m_wheelface_indices[3*(i*2+1)+1]=2+((i+1)%nrays)*2+1;
        if (m_is_rimmed)
        {
            m_wheelface_indices[3*(i*4+0+2*nrays)]=2+i*2;           m_wheelface_indices[3*(i*4+0+2*nrays)+1]=2+4*nrays+i*2;               m_wheelface_indices[3*(i*4+0+2*nrays)+2]=2+((i+1)%nrays)*2;
            m_wheelface_indices[3*(i*4+1+2*nrays)]=2+4*nrays+i*2;   m_wheelface_indices[3*(i*4+1+2*nrays)+1]=2+4*nrays+((i+1)%nrays)*2;   m_wheelface_indices[3*(i*4+1+2*nrays)+2]=2+((i+1)%nrays)*2;
            m_wheelface_indices[3*(i*4+2+2*nrays)]=2+i*2+1;         m_wheelface_indices[3*(i*4+2+2*nrays)+2]=2+4*nrays+i*2+1;             m_wheelface_indices[3*(i*4+2+2*nrays)+1]=2+((i+1)%nrays)*2+1;
            m_wheelface_indices[3*(i*4+3+2*nrays)]=2+4*nrays+i*2+1; m_wheelface_indices[3*(i*4+3+2*nrays)+2]=2+4*nrays+((i+1)%nrays)*2+1; m_wheelface_indices[3*(i*4+3+2*nrays)+1]=2+((i+1)%nrays)*2+1;
        }
        //wheel band
        m_tiretread_indices[3*(i*2)]=2+2*nrays+i*2; m_tiretread_indices[3*(i*2)+1]=2+2*nrays+i*2+1; m_tiretread_indices[3*(i*2)+2]=2+2*nrays+((i+1)%nrays)*2;
        m_tiretread_indices[3*(i*2+1)]=2+2*nrays+((i+1)%nrays)*2; m_tiretread_indices[3*(i*2+1)+2]=2+2*nrays+((i+1)%nrays)*2+1; m_tiretread_indices[3*(i*2+1)+1]=2+2*nrays+i*2+1;
    }

    //update coords
    updateVertices();

    // Create vertex data structure for 8 vertices shared between submeshes
    m_mesh->sharedVertexData = new VertexData();
    m_mesh->sharedVertexData->vertexCount = vertex_count;

    // Create declaration (memory format) of vertex data
    m_vertex_format = m_mesh->sharedVertexData->vertexDeclaration;
    size_t offset = 0;
    m_vertex_format->addElement(0, offset, VET_FLOAT3, VES_POSITION);
    offset += VertexElement::getTypeSize(VET_FLOAT3);
    m_vertex_format->addElement(0, offset, VET_FLOAT3, VES_NORMAL);
    offset += VertexElement::getTypeSize(VET_FLOAT3);
//        m_vertex_format->addElement(0, offset, VET_FLOAT3, VES_DIFFUSE);
//        offset += VertexElement::getTypeSize(VET_FLOAT3);
    m_vertex_format->addElement(0, offset, VET_FLOAT2, VES_TEXTURE_COORDINATES, 0);
    offset += VertexElement::getTypeSize(VET_FLOAT2);

    // Allocate vertex buffer of the requested number of vertices (vertexCount)
    // and bytes per vertex (offset)
    m_hw_vbuf =
        HardwareBufferManager::getSingleton().createVertexBuffer(
            offset, m_mesh->sharedVertexData->vertexCount, HardwareBuffer::HBU_DYNAMIC_WRITE_ONLY_DISCARDABLE);

    // Upload the vertex data to the card
    m_hw_vbuf->writeData(0, m_hw_vbuf->getSizeInBytes(), m_vertices, true);

    // Set vertex buffer binding so buffer 0 is bound to our vertex buffer
    VertexBufferBinding* bind = m_mesh->sharedVertexData->vertexBufferBinding;
    bind->setBinding(0, m_hw_vbuf);

    //for the sideface
    // Allocate index buffer of the requested number of vertices (ibufCount)
    HardwareIndexBufferSharedPtr faceibuf = HardwareBufferManager::getSingleton().
        createIndexBuffer(
            HardwareIndexBuffer::IT_16BIT,
            wheelface_num_indices,
            HardwareBuffer::HBU_STATIC_WRITE_ONLY);

    // Upload the index data to the card
    faceibuf->writeData(0, faceibuf->getSizeInBytes(), m_wheelface_indices, true);

    // Set parameters of the submesh
    m_submesh_wheelface->useSharedVertices = true;
    m_submesh_wheelface->indexData->indexBuffer = faceibuf;
    m_submesh_wheelface->indexData->indexCount = wheelface_num_indices;
    m_submesh_wheelface->indexData->indexStart = 0;

    //for the band
    // Allocate index buffer of the requested number of vertices (ibufCount)
    HardwareIndexBufferSharedPtr bandibuf = HardwareBufferManager::getSingleton().
        createIndexBuffer(
            HardwareIndexBuffer::IT_16BIT,
            tiretread_num_indices,
            HardwareBuffer::HBU_STATIC_WRITE_ONLY);

    // Upload the index data to the card
    bandibuf->writeData(0, bandibuf->getSizeInBytes(), m_tiretread_indices, true);

    // Set parameters of the submesh
    m_submesh_tiretread->useSharedVertices = true;
    m_submesh_tiretread->indexData->indexBuffer = bandibuf;
    m_submesh_tiretread->indexData->indexCount = tiretread_num_indices;
    m_submesh_tiretread->indexData->indexStart = 0;

    // Set bounding information (for culling)
    m_mesh->_setBounds(AxisAlignedBox(-1,-1,0,1,1,0), true);

    m_mesh->load();
}

FlexMesh::~FlexMesh()
{
    if (!m_mesh.isNull())
    {
        Ogre::MeshManager::getSingleton().remove(m_mesh->getName());
        m_mesh.setNull();
    }

    if (m_vertices          != nullptr) { delete m_vertices; }
    if (m_vertex_nodes      != nullptr) { free (m_vertex_nodes); }
    if (m_wheelface_indices != nullptr) { free (m_wheelface_indices); }
    if (m_tiretread_indices != nullptr) { free (m_tiretread_indices); }
}

Vector3 FlexMesh::updateVertices()
{
    RoR::GfxActor::SimBuffer::NodeSB* all_nodes = m_gfx_actor->GetSimNodeBuffer();
    Vector3 center = (all_nodes[m_vertex_nodes[0]].AbsPosition + all_nodes[m_vertex_nodes[1]].AbsPosition) / 2.0;

    //optimization possible here : just copy bands on face

    m_vertices[0].position=all_nodes[m_vertex_nodes[0]].AbsPosition-center;
    //normals
    m_vertices[0].normal=approx_normalise(all_nodes[m_vertex_nodes[0]].AbsPosition-all_nodes[m_vertex_nodes[1]].AbsPosition);

    m_vertices[1].position=all_nodes[m_vertex_nodes[1]].AbsPosition-center;
    //normals
    m_vertices[1].normal=-m_vertices[0].normal;

    for (int i=0; i<m_num_rays*2; i++)
    {
        m_vertices[2+i].position=all_nodes[m_vertex_nodes[2+i]].AbsPosition-center;
        //normals
        if ((i%2)==0)
        {
            m_vertices[2+i].normal=approx_normalise(all_nodes[m_vertex_nodes[0]].AbsPosition-all_nodes[m_vertex_nodes[1]].AbsPosition);
        } else
        {
            m_vertices[2+i].normal=-m_vertices[2+i-1].normal;
        }
        if (m_is_rimmed)
        {
            m_vertices[2+4*m_num_rays+i].position=all_nodes[m_vertex_nodes[2+4*m_num_rays+i]].AbsPosition-center;
            //normals
            if ((i%2)==0)
            {
                m_vertices[2+4*m_num_rays+i].normal=approx_normalise(all_nodes[m_vertex_nodes[2+4*m_num_rays+i]].AbsPosition-all_nodes[m_vertex_nodes[2+4*m_num_rays+i+1]].AbsPosition);
            } else
            {
                m_vertices[2+4*m_num_rays+i].normal=-m_vertices[2+4*m_num_rays+i-1].normal;
            }
            //bands
            m_vertices[2+2*m_num_rays+i].position=m_vertices[2+4*m_num_rays+i].position;
            m_vertices[2+2*m_num_rays+i].normal=approx_normalise(m_vertices[2+4*m_num_rays+i].position);
        } else
        {
            //bands
            m_vertices[2+2*m_num_rays+i].position=m_vertices[2+i].position;
            m_vertices[2+2*m_num_rays+i].normal=approx_normalise(m_vertices[2+i].position);
        }
    }
    return center;
}

void FlexMesh::flexitCompute()
{
    m_flexit_center = updateVertices();
}

Vector3 FlexMesh::flexitFinal()
{
    m_hw_vbuf->writeData(0, m_hw_vbuf->getSizeInBytes(), m_vertices, true);
    return m_flexit_center;
}
