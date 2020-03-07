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

#include "FlexMeshWheel.h"

#include "BeamData.h"
#include "GfxActor.h"

#include <Ogre.h>

using namespace Ogre;

FlexMeshWheel::FlexMeshWheel(
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
) :
      m_axis_node0_idx(axis_node_1_index)
    , m_axis_node1_idx(axis_node_2_index)
    , m_start_node_idx(nstart)
    , m_num_rays(static_cast<size_t>(nrays))
    , m_gfx_actor(gfx_actor)
    , m_is_rim_reverse(rimreverse)
    , m_rim_radius(rimradius)
{
    m_rim_entity = rim_prop_entity;
    m_rim_scene_node=gEnv->sceneManager->getRootSceneNode()->createChildSceneNode();
    m_rim_scene_node->attachObject(m_rim_entity);

    // Create the tire mesh via the MeshManager
    m_mesh = MeshManager::getSingleton().createManual(tire_mesh_name, gfx_actor->GetResourceGroup());

    // Create submeshes
    m_submesh = m_mesh->createSubMesh();

    //materials
    m_submesh->setMaterialName(tire_material_name);

    // Define the vertices
    m_vertex_count = 6*(nrays+1);
    m_vertices = new FlexMeshWheelVertex[m_vertex_count];

    int i;
    //textures coordinates
    for (i=0; i<nrays+1; i++)
    {
        m_vertices[i*6   ].texcoord=Vector2((float)i/(float)nrays, 0.00f);
        m_vertices[i*6+1 ].texcoord=Vector2((float)i/(float)nrays, 0.23f);
        m_vertices[i*6+2 ].texcoord=Vector2((float)i/(float)nrays, 0.27f);
        m_vertices[i*6+3 ].texcoord=Vector2((float)i/(float)nrays, 0.73f);
        m_vertices[i*6+4 ].texcoord=Vector2((float)i/(float)nrays, 0.77f);
        m_vertices[i*6+5 ].texcoord=Vector2((float)i/(float)nrays, 1.00f);
    }

    // Define triangles
    // The values in this table refer to vertices in the above table
    m_index_count = 3*10*nrays;
    m_indices=(unsigned short*)malloc(m_index_count*sizeof(unsigned short));
    for (i=0; i<nrays; i++)
    {
        m_indices[3*(i*10  )]=i*6;   m_indices[3*(i*10  )+1]=i*6+1;     m_indices[3*(i*10  )+2]=(i+1)*6;
        m_indices[3*(i*10+1)]=i*6+1; m_indices[3*(i*10+1)+1]=(i+1)*6+1; m_indices[3*(i*10+1)+2]=(i+1)*6;

        m_indices[3*(i*10+2)]=i*6+1; m_indices[3*(i*10+2)+1]=i*6+2;     m_indices[3*(i*10+2)+2]=(i+1)*6+1;
        m_indices[3*(i*10+3)]=i*6+2; m_indices[3*(i*10+3)+1]=(i+1)*6+2; m_indices[3*(i*10+3)+2]=(i+1)*6+1;

        m_indices[3*(i*10+4)]=i*6+2; m_indices[3*(i*10+4)+1]=i*6+3;     m_indices[3*(i*10+4)+2]=(i+1)*6+2;
        m_indices[3*(i*10+5)]=i*6+3; m_indices[3*(i*10+5)+1]=(i+1)*6+3; m_indices[3*(i*10+5)+2]=(i+1)*6+2;

        m_indices[3*(i*10+6)]=i*6+3; m_indices[3*(i*10+6)+1]=i*6+4;     m_indices[3*(i*10+6)+2]=(i+1)*6+3;
        m_indices[3*(i*10+7)]=i*6+4; m_indices[3*(i*10+7)+1]=(i+1)*6+4; m_indices[3*(i*10+7)+2]=(i+1)*6+3;

        m_indices[3*(i*10+8)]=i*6+4; m_indices[3*(i*10+8)+1]=i*6+5;     m_indices[3*(i*10+8)+2]=(i+1)*6+4;
        m_indices[3*(i*10+9)]=i*6+5; m_indices[3*(i*10+9)+1]=(i+1)*6+5; m_indices[3*(i*10+9)+2]=(i+1)*6+4;
    }

    m_norm_y=1.0;
    //update coords
    updateVertices();
    //compute m_norm_y;
    m_norm_y=((m_vertices[0].position-m_vertices[1].position).crossProduct(m_vertices[1].position-m_vertices[6+1].position)).length();
    //recompute for normals
    updateVertices();

    // Create position data structure for 8 vertices shared between submeshes
    m_mesh->sharedVertexData = new VertexData();
    m_mesh->sharedVertexData->vertexCount = m_vertex_count;

    // Create declaration (memory format) of position data
    m_vertex_format = m_mesh->sharedVertexData->vertexDeclaration;
    size_t offset = 0;
    m_vertex_format->addElement(0, offset, VET_FLOAT3, VES_POSITION);
    offset += VertexElement::getTypeSize(VET_FLOAT3);
    m_vertex_format->addElement(0, offset, VET_FLOAT3, VES_NORMAL);
    offset += VertexElement::getTypeSize(VET_FLOAT3);
    m_vertex_format->addElement(0, offset, VET_FLOAT2, VES_TEXTURE_COORDINATES, 0);
    offset += VertexElement::getTypeSize(VET_FLOAT2);

    // Allocate position buffer of the requested number of vertices (vertexCount)
    // and bytes per position (offset)
    m_hw_vbuf =
      HardwareBufferManager::getSingleton().createVertexBuffer(
          offset, m_mesh->sharedVertexData->vertexCount, HardwareBuffer::HBU_DYNAMIC_WRITE_ONLY_DISCARDABLE);

    // Upload the position data to the card
    m_hw_vbuf->writeData(0, m_hw_vbuf->getSizeInBytes(), m_vertices, true);

    // Set position buffer binding so buffer 0 is bound to our position buffer
    VertexBufferBinding* bind = m_mesh->sharedVertexData->vertexBufferBinding;
    bind->setBinding(0, m_hw_vbuf);

    //for the face
    // Allocate index buffer of the requested number of vertices (m_index_count)
    HardwareIndexBufferSharedPtr ibuf = HardwareBufferManager::getSingleton().
     createIndexBuffer(
         HardwareIndexBuffer::IT_16BIT,
            m_index_count,
            HardwareBuffer::HBU_STATIC_WRITE_ONLY);

    // Upload the index data to the card
    ibuf->writeData(0, ibuf->getSizeInBytes(), m_indices, true);

    // Set parameters of the submesh
    m_submesh->useSharedVertices = true;
    m_submesh->indexData->indexBuffer = ibuf;
    m_submesh->indexData->indexCount = m_index_count;
    m_submesh->indexData->indexStart = 0;


    // Set bounding information (for culling)
    m_mesh->_setBounds(AxisAlignedBox(-1,-1,0,1,1,0), true);

    m_mesh->load();
}

FlexMeshWheel::~FlexMeshWheel()
{
    if (m_vertices != nullptr) { delete m_vertices; }
    if (m_indices != nullptr) { free (m_indices); }

    // Rim: we own both Entity and SceneNode
    m_rim_scene_node->detachAllObjects();
    gEnv->sceneManager->destroySceneNode(m_rim_scene_node);
    gEnv->sceneManager->destroyEntity(m_rim_entity);
    m_rim_entity = nullptr;

    // Tyre: we own the Entity, SceneNode is owned by vwheel_t
    m_tire_entity->detachFromParent();
    gEnv->sceneManager->destroyEntity(m_tire_entity);
    m_tire_entity = nullptr;

    // Delete tyre mesh
    m_mesh->unload();
    Ogre::MeshManager::getSingleton().remove(m_mesh->getHandle());
    m_mesh.setNull();
}

Vector3 FlexMeshWheel::updateVertices()
{
    RoR::GfxActor::SimBuffer::NodeSB* all_nodes = m_gfx_actor->GetSimNodeBuffer();
    Vector3 center = (all_nodes[m_axis_node0_idx].AbsPosition + all_nodes[m_axis_node1_idx].AbsPosition) / 2.0;
    Vector3 ray = all_nodes[m_start_node_idx].AbsPosition - all_nodes[m_axis_node0_idx].AbsPosition;
    Vector3 axis = all_nodes[m_axis_node0_idx].AbsPosition - all_nodes[m_axis_node1_idx].AbsPosition;

    axis.normalise();
    
    for (size_t i=0; i<m_num_rays; i++)
    {
        Plane pl=Plane(axis, all_nodes[m_axis_node0_idx].AbsPosition);
        ray=all_nodes[m_start_node_idx+i*2].AbsPosition-all_nodes[m_axis_node0_idx].AbsPosition;
        ray=pl.projectVector(ray);
        ray.normalise();
        m_vertices[i*6  ].position=all_nodes[m_axis_node0_idx].AbsPosition+m_rim_radius*ray-center;

        m_vertices[i*6+1].position=all_nodes[m_start_node_idx+i*2].AbsPosition-0.05  *(all_nodes[m_start_node_idx+i*2].AbsPosition-all_nodes[m_axis_node0_idx].AbsPosition)-center;
        m_vertices[i*6+2].position=all_nodes[m_start_node_idx+i*2].AbsPosition-0.1   *(all_nodes[m_start_node_idx+i*2].AbsPosition-all_nodes[m_start_node_idx+i*2+1].AbsPosition)-center;
        m_vertices[i*6+3].position=all_nodes[m_start_node_idx+i*2+1].AbsPosition-0.1 *(all_nodes[m_start_node_idx+i*2+1].AbsPosition-all_nodes[m_start_node_idx+i*2].AbsPosition)-center;
        m_vertices[i*6+4].position=all_nodes[m_start_node_idx+i*2+1].AbsPosition-0.05*(all_nodes[m_start_node_idx+i*2+1].AbsPosition-all_nodes[m_axis_node1_idx].AbsPosition)-center;

        pl=Plane(-axis, all_nodes[m_axis_node1_idx].AbsPosition);
        ray=all_nodes[m_start_node_idx+i*2+1].AbsPosition-all_nodes[m_axis_node1_idx].AbsPosition;
        ray=pl.projectVector(ray);
        ray.normalise();
        m_vertices[i*6+5].position=all_nodes[m_axis_node1_idx].AbsPosition+m_rim_radius*ray-center;

        //normals
        m_vertices[i*6  ].normal=axis;
        m_vertices[i*6+1].normal=(m_vertices[i*6].position-m_vertices[i*6+1].position).crossProduct(m_vertices[i*6].position-m_vertices[((i+1)%m_num_rays)*6+1].position)/m_norm_y;
        m_vertices[i*6+2].normal=ray;
        m_vertices[i*6+3].normal=ray;
        m_vertices[i*6+4].normal=(m_vertices[i*6+4].position-m_vertices[i*6+5].position).crossProduct(m_vertices[i*6+4].position-m_vertices[((i+1)%m_num_rays)*6+4].position)/m_norm_y;
        m_vertices[i*6+5].normal=-axis;
    }
    for (int i=0; i<6; i++)
    {
        m_vertices[m_num_rays*6+i].position=m_vertices[i].position;
        m_vertices[m_num_rays*6+i].normal=m_vertices[i].normal;
    }

    return center;
}

void FlexMeshWheel::setVisible(bool visible)
{
    if (m_rim_scene_node) m_rim_scene_node->setVisible(visible);
}

bool FlexMeshWheel::flexitPrepare()
{
    RoR::GfxActor::SimBuffer::NodeSB* all_nodes = m_gfx_actor->GetSimNodeBuffer();
    Vector3 center = (all_nodes[m_axis_node0_idx].AbsPosition + all_nodes[m_axis_node1_idx].AbsPosition) / 2.0;
    m_rim_scene_node->setPosition(center);

    Vector3 axis = all_nodes[m_axis_node0_idx].AbsPosition - all_nodes[m_axis_node1_idx].AbsPosition;
    axis.normalise();

    if (m_is_rim_reverse) axis = -axis;
    Vector3 ray = all_nodes[m_start_node_idx].AbsPosition - all_nodes[m_axis_node0_idx].AbsPosition;
    Vector3 onormal = axis.crossProduct(ray);
    onormal.normalise();
    ray = axis.crossProduct(onormal);
    m_rim_scene_node->setOrientation(Quaternion(axis, onormal, ray));

    return true;
}

void FlexMeshWheel::flexitCompute()
{
    m_flexit_center = updateVertices();
}

Vector3 FlexMeshWheel::flexitFinal()
{
    m_hw_vbuf->writeData(0, m_hw_vbuf->getSizeInBytes(), m_vertices, true);
    return m_flexit_center;
}
