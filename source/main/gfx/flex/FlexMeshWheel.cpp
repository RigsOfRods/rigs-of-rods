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

#include "FlexMeshWheel.h"

#include "Application.h"
#include "SimData.h"
#include "GfxActor.h"
#include "GfxScene.h"

#include <Ogre.h>

using namespace Ogre;
using namespace RoR;

FlexMeshWheel::FlexMeshWheel(
    Ogre::Entity* rim_prop_entity,
    Ogre::SceneNode* rim_scene_node,
    RoR::GfxActor* gfx_actor,
    int axis_node_1_index, 
    int axis_node_2_index, 
    int nstart, 
    int nrays, 
    std::string const& tire_mesh_name,
    std::string const& tire_mesh_rg,
    std::string const& tire_material_name,
    std::string const& tire_material_rg,
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
    // Rim entity setup
    m_rim_entity = rim_prop_entity;
    m_rim_scene_node = rim_scene_node;
    m_rim_scene_node->attachObject(m_rim_entity);
    m_rim_skelinst = m_rim_entity->getSkeleton();
    ROR_ASSERT(m_rim_skelinst);
    m_rim_boneinst = m_rim_skelinst->getBone(0);
    ROR_ASSERT(m_rim_boneinst);

    // Create the tire mesh via the MeshManager
    m_mesh = MeshManager::getSingleton().createManual(tire_mesh_name, tire_mesh_rg);

    // Create submeshes
    m_submesh = m_mesh->createSubMesh();

    //materials
    m_submesh->setMaterialName(tire_material_name, tire_material_rg);

    // Define the vertices
    m_vertex_count = 6*(nrays+1);
    m_vertices.resize(m_vertex_count);

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
    m_indices.resize(m_index_count);
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
    this->UpdateVertices();
    //compute m_norm_y;
    m_norm_y=((m_vertices[0].position-m_vertices[1].position).crossProduct(m_vertices[1].position-m_vertices[6+1].position)).length();
    //recompute for normals
    this->UpdateVertices();

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
    m_hw_vbuf->writeData(0, m_hw_vbuf->getSizeInBytes(), m_vertices.data(), true);

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
    ibuf->writeData(0, ibuf->getSizeInBytes(), m_indices.data(), true);
    m_indices.clear(); // We won't need these anymore.

    // Set parameters of the submesh
    m_submesh->useSharedVertices = true;
    m_submesh->indexData->indexBuffer = ibuf;
    m_submesh->indexData->indexCount = m_index_count;
    m_submesh->indexData->indexStart = 0;


    // Set bounding information (for culling) - 100x100 is the size of actor-local physics space (relative to `ar_origin`)
    m_mesh->_setBounds(AxisAlignedBox(-100,-100,-100,100,100,100), true);

    m_mesh->load();
}

FlexMeshWheel::~FlexMeshWheel()
{
    // Rim: we own both Entity and SceneNode
    m_rim_scene_node->detachAllObjects();
    App::GetGfxScene()->GetSceneManager()->destroySceneNode(m_rim_scene_node);
    App::GetGfxScene()->GetSceneManager()->destroyEntity(m_rim_entity);
    m_rim_entity = nullptr;

    // Tyre: we own the Entity, SceneNode is owned by vwheel_t
    m_tire_entity->detachFromParent();
    App::GetGfxScene()->GetSceneManager()->destroyEntity(m_tire_entity);
    m_tire_entity = nullptr;

    // Delete tyre mesh
    m_mesh->unload();
    Ogre::MeshManager::getSingleton().remove(m_mesh->getHandle());
    m_mesh.reset();
}

void FlexMeshWheel::UpdateVertices()
{
    RoR::NodeSB* all_nodes = m_gfx_actor->GetSimNodeBuffer();
    Vector3 ray = all_nodes[m_start_node_idx].RelPosition - all_nodes[m_axis_node0_idx].RelPosition;
    const Vector3 axis = (all_nodes[m_axis_node0_idx].RelPosition - all_nodes[m_axis_node1_idx].RelPosition).normalisedCopy();

    for (size_t i=0; i<m_num_rays; i++)
    {
        Plane pl=Plane(axis, all_nodes[m_axis_node0_idx].RelPosition);
        ray=all_nodes[m_start_node_idx+i*2].RelPosition-all_nodes[m_axis_node0_idx].RelPosition;
        ray=pl.projectVector(ray);
        ray.normalise();
        m_vertices[i*6  ].position=all_nodes[m_axis_node0_idx].RelPosition+m_rim_radius*ray;

        m_vertices[i*6+1].position=all_nodes[m_start_node_idx+i*2].RelPosition-0.05  *(all_nodes[m_start_node_idx+i*2].RelPosition-all_nodes[m_axis_node0_idx].RelPosition);
        m_vertices[i*6+2].position=all_nodes[m_start_node_idx+i*2].RelPosition-0.1   *(all_nodes[m_start_node_idx+i*2].RelPosition-all_nodes[m_start_node_idx+i*2+1].RelPosition);
        m_vertices[i*6+3].position=all_nodes[m_start_node_idx+i*2+1].RelPosition-0.1 *(all_nodes[m_start_node_idx+i*2+1].RelPosition-all_nodes[m_start_node_idx+i*2].RelPosition);
        m_vertices[i*6+4].position=all_nodes[m_start_node_idx+i*2+1].RelPosition-0.05*(all_nodes[m_start_node_idx+i*2+1].RelPosition-all_nodes[m_axis_node1_idx].RelPosition);

        pl=Plane(-axis, all_nodes[m_axis_node1_idx].RelPosition);
        ray=all_nodes[m_start_node_idx+i*2+1].RelPosition-all_nodes[m_axis_node1_idx].RelPosition;
        ray=pl.projectVector(ray);
        ray.normalise();
        m_vertices[i*6+5].position=all_nodes[m_axis_node1_idx].RelPosition+m_rim_radius*ray;

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
}

void FlexMeshWheel::FlexitSetVisible(bool visible)
{
    if (m_rim_scene_node) m_rim_scene_node->setVisible(visible);
}

void FlexMeshWheel::UpdateRimMesh()
{
    // Update the rim entity:
    //  to prevent the prop from jittering at large world distances,
    //  we fix the scenenode at physics origin and move the verts relatively using a bone.
    /// -----------------------------------------------------------------------------------

    m_rim_scene_node->setPosition(m_gfx_actor->GetSimDataBuffer().simbuf_origin);
    m_rim_boneinst->setManuallyControlled(true);

    RoR::NodeSB* all_nodes = m_gfx_actor->GetSimNodeBuffer();
    Vector3 center = (all_nodes[m_axis_node0_idx].RelPosition + all_nodes[m_axis_node1_idx].RelPosition) / 2.0;
    m_rim_boneinst->setPosition(center);

    Vector3 axis = all_nodes[m_axis_node0_idx].RelPosition - all_nodes[m_axis_node1_idx].RelPosition;
    axis.normalise();

    if (m_is_rim_reverse) axis = -axis;
    Vector3 ray = all_nodes[m_start_node_idx].RelPosition - all_nodes[m_axis_node0_idx].RelPosition;
    Vector3 onormal = axis.crossProduct(ray).normalisedCopy();
    ray = axis.crossProduct(onormal);
    m_rim_boneinst->setOrientation(Quaternion(axis, onormal, ray));

    m_rim_skelinst->_notifyManualBonesDirty();
}

void FlexMeshWheel::FlexitCompute()
{
    this->UpdateVertices();
}

void FlexMeshWheel::FlexitFinalize()
{
    m_hw_vbuf->writeData(0, m_hw_vbuf->getSizeInBytes(), m_vertices.data(), true);
    this->UpdateRimMesh();
}

