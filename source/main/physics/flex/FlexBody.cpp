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

#include "FlexBody.h"

#include "Application.h"
#include "ApproxMath.h"
#include "Console.h"
#include "SimData.h"
#include "FlexFactory.h"
#include "GfxActor.h"
#include "GfxScene.h"
#include "RigDef_File.h"

#include <Ogre.h>

using namespace Ogre;
using namespace RoR;

FlexBody::FlexBody(
    RoR::FlexBodyCacheData* preloaded_from_cache,
    RoR::GfxActor* gfx_actor,
    Ogre::Entity* ent,
    NodeNum_t ref,
    NodeNum_t nx,
    NodeNum_t ny,
    Ogre::Vector3 offset,
    Ogre::Quaternion const & rot,
    std::vector<unsigned int> & node_indices,
    std::vector<ForvertTempData>& forvert_data
):
      m_center_offset(offset)
    , m_node_center(ref)
    , m_node_x(nx)
    , m_node_y(ny)
    , m_scene_entity(ent)
    , m_gfx_actor(gfx_actor)
{
    ROR_ASSERT(m_node_x != NODENUM_INVALID);
    ROR_ASSERT(m_node_y != NODENUM_INVALID);

    Ogre::Vector3* vertices = nullptr;
    std::string mesh_name = ent->getMesh()->getName();

    Vector3 normal = Vector3::UNIT_Y;
    Vector3 position = Vector3::ZERO;
    Quaternion orientation = Quaternion::ZERO;

    RoR::NodeSB* nodes = m_gfx_actor->GetSimNodeBuffer();

    if (m_node_center != NODENUM_INVALID)
    {
        Vector3 diffX = nodes[nx].AbsPosition-nodes[ref].AbsPosition;
        Vector3 diffY = nodes[ny].AbsPosition-nodes[ref].AbsPosition;

        normal = (diffY.crossProduct(diffX)).normalisedCopy();

        // position
        position = nodes[ref].AbsPosition + offset.x * diffX + offset.y * diffY;
        position = position + offset.z * normal;

        // orientation
        Vector3 refX = diffX.normalisedCopy();
        Vector3 refY = refX.crossProduct(normal);
        orientation  = Quaternion(refX, normal, refY) * rot;
    }
    else
    {
        // special case!
        normal = Vector3::UNIT_Y;
        position = nodes[0].AbsPosition + offset;
        orientation = rot;
    }

    Ogre::MeshPtr mesh=ent->getMesh();
    m_orig_mesh_info = RoR::PrintMeshInfo("Original", mesh); // For diagnostics only
    int num_submeshes = static_cast<int>(mesh->getNumSubMeshes());
    if (preloaded_from_cache == nullptr)
    {
        //determine if we have texture coordinates everywhere
        if (mesh->sharedVertexData && mesh->sharedVertexData->vertexDeclaration->findElementBySemantic(VES_TEXTURE_COORDINATES)==0)
        {
            m_has_texture=false;
        }
        for (int i=0; i<num_submeshes; i++)
        {
            if (!mesh->getSubMesh(i)->useSharedVertices && mesh->getSubMesh(i)->vertexData->vertexDeclaration->findElementBySemantic(VES_TEXTURE_COORDINATES)==0) 
            {
                m_has_texture=false;
            }
        }
        if (!m_has_texture)
        {
            LOG("FLEXBODY Warning: at least one part of this mesh does not have texture coordinates, switching off texturing!");
            m_has_texture_blend=false;
        }

        //detect the anomalous case where a mesh is exported without normal vectors
        bool havenormal=true;
        if (mesh->sharedVertexData && mesh->sharedVertexData->vertexDeclaration->findElementBySemantic(VES_NORMAL)==0)
        {
            havenormal=false;
        }
        for (int i=0; i<num_submeshes; i++)
        {
            if (!mesh->getSubMesh(i)->useSharedVertices && mesh->getSubMesh(i)->vertexData->vertexDeclaration->findElementBySemantic(VES_NORMAL)==0) 
            {
                havenormal=false;
            }
        }
        if (!havenormal)
        {
            LOG("FLEXBODY Error: at least one part of this mesh does not have normal vectors, export your mesh with normal vectors! Disabling flexbody");
            // NOTE: Intentionally not disabling, for compatibility with v0.4.0.7
        }
    }
    else
    {
        m_has_texture        = BITMASK_IS_1(preloaded_from_cache->header.flags, FlexBodyRecordHeader::HAS_TEXTURE);
        m_has_texture_blend  = BITMASK_IS_1(preloaded_from_cache->header.flags, FlexBodyRecordHeader::HAS_TEXTURE_BLEND);
    }

    //create optimal VertexDeclaration
    VertexDeclaration* optimalVD=HardwareBufferManager::getSingleton().createVertexDeclaration();
    optimalVD->addElement(0, 0, VET_FLOAT3, VES_POSITION);
    optimalVD->addElement(1, 0, VET_FLOAT3, VES_NORMAL);
    if (m_has_texture_blend) optimalVD->addElement(2, 0, VET_COLOUR_ARGB, VES_DIFFUSE);
    if (m_has_texture) optimalVD->addElement(3, 0, VET_FLOAT2, VES_TEXTURE_COORDINATES);
    optimalVD->sort();
    optimalVD->closeGapsInSource();

    //adding color buffers, well get the reference later
    if (m_has_texture_blend)
    {
        if (mesh->sharedVertexData)
        {
            if (mesh->sharedVertexData->vertexDeclaration->findElementBySemantic(VES_DIFFUSE)==0)
            {
                //add buffer
                int index=mesh->sharedVertexData->vertexDeclaration->getMaxSource()+1;
                mesh->sharedVertexData->vertexDeclaration->addElement(index, 0, VET_COLOUR_ARGB, VES_DIFFUSE);
                mesh->sharedVertexData->vertexDeclaration->sort();
                index=mesh->sharedVertexData->vertexDeclaration->findElementBySemantic(VES_DIFFUSE)->getSource();
                HardwareVertexBufferSharedPtr vbuf=HardwareBufferManager::getSingleton().createVertexBuffer(VertexElement::getTypeSize(VET_COLOUR_ARGB), mesh->sharedVertexData->vertexCount, HardwareBuffer::HBU_DYNAMIC_WRITE_ONLY_DISCARDABLE);
                mesh->sharedVertexData->vertexBufferBinding->setBinding(index, vbuf);
            }
        }
        for (int i=0; i<num_submeshes; i++)
        {
            if (!mesh->getSubMesh(i)->useSharedVertices)
            {
                Ogre::VertexData* vertex_data = mesh->getSubMesh(i)->vertexData;
                Ogre::VertexDeclaration* vertex_decl = vertex_data->vertexDeclaration;
                if (vertex_decl->findElementBySemantic(VES_DIFFUSE)==0)
                {
                    //add buffer
                    int index = vertex_decl->getMaxSource()+1;
                    vertex_decl->addElement(index, 0, VET_COLOUR_ARGB, VES_DIFFUSE);
                    vertex_decl->sort();
                    vertex_decl->findElementBySemantic(VES_DIFFUSE)->getSource();
                    HardwareVertexBufferSharedPtr vbuf = HardwareBufferManager::getSingleton().createVertexBuffer(
                        VertexElement::getTypeSize(VET_COLOUR_ARGB), vertex_data->vertexCount, HardwareBuffer::HBU_DYNAMIC_WRITE_ONLY_DISCARDABLE);
                    vertex_data->vertexBufferBinding->setBinding(index, vbuf);
                }
            }
        }
    }

    //reorg
    //LOG("FLEXBODY reorganizing buffers");
    if (mesh->sharedVertexData)
    {
        mesh->sharedVertexData->reorganiseBuffers(optimalVD);
        mesh->sharedVertexData->removeUnusedBuffers();
        mesh->sharedVertexData->closeGapsInBindings();
    }
    Mesh::SubMeshIterator smIt = mesh->getSubMeshIterator();
    while (smIt.hasMoreElements())
    {
        SubMesh* sm = smIt.getNext();
        if (!sm->useSharedVertices)
        {
            sm->vertexData->reorganiseBuffers(optimalVD->clone());
            sm->vertexData->removeUnusedBuffers();
            sm->vertexData->closeGapsInBindings();
        }
    }

    //print mesh information
    //LOG("FLEXBODY Printing modififed mesh informations:");
    //printMeshInfo(ent->getMesh().get());

    //get the buffers
    //getMeshInformation(ent->getMesh().get(),m_vertex_count,vertices,index_count,indices, position, orientation, Vector3(1,1,1));

    //getting vertex counts
    if (preloaded_from_cache == nullptr)
    {
        m_vertex_count=0;
        m_uses_shared_vertex_data=false;
        m_num_submesh_vbufs=0;
        if (mesh->sharedVertexData)
        {
            m_vertex_count+=mesh->sharedVertexData->vertexCount;
            m_uses_shared_vertex_data=true;
        }
        for (int i=0; i<num_submeshes; i++)
        {
            if (!mesh->getSubMesh(i)->useSharedVertices)
            {
                m_vertex_count+=mesh->getSubMesh(i)->vertexData->vertexCount;
                m_num_submesh_vbufs++;
            }
        }
    } else
    {
        m_vertex_count            = preloaded_from_cache->header.vertex_count;
        m_uses_shared_vertex_data = BITMASK_IS_1(preloaded_from_cache->header.flags, FlexBodyRecordHeader::USES_SHARED_VERTEX_DATA);
        m_num_submesh_vbufs       = preloaded_from_cache->header.num_submesh_vbufs;
    }
    
    // Profiler data
    double stat_manual_buffers_created_time = -1;
    double stat_transformed_time = -1;
    double stat_located_time = -1;
    if (preloaded_from_cache != nullptr)
    {
        m_dst_pos     = preloaded_from_cache->dst_pos;
        m_src_normals = preloaded_from_cache->src_normals;
        m_locators    = preloaded_from_cache->locators;
        m_dst_normals = (Vector3*)malloc(sizeof(Vector3)*m_vertex_count); // Use malloc() for compatibility

        if (m_has_texture_blend)
        {
            m_src_colors = preloaded_from_cache->src_colors;
        }

        if (mesh->sharedVertexData)
        {
            m_shared_buf_num_verts=(int)mesh->sharedVertexData->vertexCount;

            //vertices
            int source=mesh->sharedVertexData->vertexDeclaration->findElementBySemantic(VES_POSITION)->getSource();
            m_shared_vbuf_pos=mesh->sharedVertexData->vertexBufferBinding->getBuffer(source);
            //normals
            source=mesh->sharedVertexData->vertexDeclaration->findElementBySemantic(VES_NORMAL)->getSource();
            m_shared_vbuf_norm=mesh->sharedVertexData->vertexBufferBinding->getBuffer(source);
            //colors
            if (m_has_texture_blend)
            {
                source=mesh->sharedVertexData->vertexDeclaration->findElementBySemantic(VES_DIFFUSE)->getSource();
                m_shared_vbuf_color=mesh->sharedVertexData->vertexBufferBinding->getBuffer(source);
            }
        }
        unsigned int curr_submesh_idx = 0;
        for (int i=0; i<num_submeshes; i++)
        {
            const Ogre::SubMesh* submesh = mesh->getSubMesh(i);
            if (submesh->useSharedVertices)
            {
                continue;
            }
            const Ogre::VertexData* vertex_data = submesh->vertexData;
            m_submesh_vbufs_vertex_counts[curr_submesh_idx] = (int)vertex_data->vertexCount;

            int source_pos  = vertex_data->vertexDeclaration->findElementBySemantic(VES_POSITION)->getSource();
            int source_norm = vertex_data->vertexDeclaration->findElementBySemantic(VES_NORMAL)->getSource();
            m_submesh_vbufs_pos [curr_submesh_idx] = vertex_data->vertexBufferBinding->getBuffer(source_pos);
            m_submesh_vbufs_norm[curr_submesh_idx] = vertex_data->vertexBufferBinding->getBuffer(source_norm);

            if (m_has_texture_blend)
            {
                int source_color = vertex_data->vertexDeclaration->findElementBySemantic(VES_DIFFUSE)->getSource();
                m_submesh_vbufs_color[curr_submesh_idx] = vertex_data->vertexBufferBinding->getBuffer(source_color);
            }
            curr_submesh_idx++;
        }
    }
    else
    {
        vertices=(Vector3*)malloc(sizeof(Vector3)*m_vertex_count);
        m_dst_pos=(Vector3*)malloc(sizeof(Vector3)*m_vertex_count);
        m_src_normals=(Vector3*)malloc(sizeof(Vector3)*m_vertex_count);
        m_dst_normals=(Vector3*)malloc(sizeof(Vector3)*m_vertex_count);
        if (m_has_texture_blend)
        {
            m_src_colors=(ARGB*)malloc(sizeof(ARGB)*m_vertex_count);
            for (int i=0; i<(int)m_vertex_count; i++) m_src_colors[i]=0x00000000;
        }
        Vector3* vpt=vertices;
        Vector3* npt=m_src_normals;
        if (mesh->sharedVertexData)
        {
            m_shared_buf_num_verts=(int)mesh->sharedVertexData->vertexCount;
            //vertices
            int source=mesh->sharedVertexData->vertexDeclaration->findElementBySemantic(VES_POSITION)->getSource();
            m_shared_vbuf_pos=mesh->sharedVertexData->vertexBufferBinding->getBuffer(source);
            m_shared_vbuf_pos->readData(0, mesh->sharedVertexData->vertexCount*sizeof(Vector3), (void*)vpt);
            vpt+=mesh->sharedVertexData->vertexCount;
            //normals
            source=mesh->sharedVertexData->vertexDeclaration->findElementBySemantic(VES_NORMAL)->getSource();
            m_shared_vbuf_norm=mesh->sharedVertexData->vertexBufferBinding->getBuffer(source);
            m_shared_vbuf_norm->readData(0, mesh->sharedVertexData->vertexCount*sizeof(Vector3), (void*)npt);
            npt+=mesh->sharedVertexData->vertexCount;
            //colors
            if (m_has_texture_blend)
            {
                source=mesh->sharedVertexData->vertexDeclaration->findElementBySemantic(VES_DIFFUSE)->getSource();
                m_shared_vbuf_color=mesh->sharedVertexData->vertexBufferBinding->getBuffer(source);
                m_shared_vbuf_color->writeData(0, mesh->sharedVertexData->vertexCount*sizeof(ARGB), (void*)m_src_colors);
            }
        }
        int cursubmesh=0;
        for (int i=0; i<num_submeshes; i++)
        {
            const Ogre::SubMesh* submesh = mesh->getSubMesh(i);
            if (submesh->useSharedVertices)
            {
                continue;
            }
            const Ogre::VertexData* vertex_data = submesh->vertexData;
            int vertex_count = (int)vertex_data->vertexCount;
            m_submesh_vbufs_vertex_counts[cursubmesh] = vertex_count;
            //vertices
            int source = vertex_data->vertexDeclaration->findElementBySemantic(VES_POSITION)->getSource();
            m_submesh_vbufs_pos[cursubmesh]=vertex_data->vertexBufferBinding->getBuffer(source);
            m_submesh_vbufs_pos[cursubmesh]->readData(0, vertex_count*sizeof(Vector3), (void*)vpt);
            vpt += vertex_count;
            //normals
            source = vertex_data->vertexDeclaration->findElementBySemantic(VES_NORMAL)->getSource();
            m_submesh_vbufs_norm[cursubmesh]=vertex_data->vertexBufferBinding->getBuffer(source);
            m_submesh_vbufs_norm[cursubmesh]->readData(0, vertex_count*sizeof(Vector3), (void*)npt);
            npt += vertex_count;
            //colors
            if (m_has_texture_blend)
            {
                source = vertex_data->vertexDeclaration->findElementBySemantic(VES_DIFFUSE)->getSource();
                m_submesh_vbufs_color[cursubmesh] = vertex_data->vertexBufferBinding->getBuffer(source);
                m_submesh_vbufs_color[cursubmesh]->writeData(0, vertex_count*sizeof(ARGB), (void*)m_src_colors);
            }
            cursubmesh++;
        }

        //transform
        for (int i=0; i<(int)m_vertex_count; i++)
        {
            vertices[i]=(orientation*vertices[i])+position;
        }

        m_locators = new Locator_t[m_vertex_count];
        for (int i=0; i<(int)m_vertex_count; i++)
        {
            // Is this locator is manually specified via directive 'forvert'?
            auto forvert_itor = std::find_if(forvert_data.begin(), forvert_data.end(),
                [i](const ForvertTempData& data) { return data.vert_index == i; });
            if (forvert_itor != forvert_data.end())
            {
                m_locators[i].ref = forvert_itor->nref;
                m_locators[i].nx = forvert_itor->nx;
                m_locators[i].ny = forvert_itor->ny;
                m_locators[i].is_forvert = true;
                LOG(fmt::format("FLEXBODY vertex {} overriden for nodes REF:{}, VX:{}, VY:{}",
                    i, m_locators[i].ref, m_locators[i].nx, m_locators[i].ny));
            }
            else
            {

                //search nearest node as the local origin
                float closest_node_distance = std::numeric_limits<float>::max();
                int closest_node_index = -1;
                for (auto node_index : node_indices)
                {
                    float node_distance = vertices[i].squaredDistance(nodes[node_index].AbsPosition);
                    if (node_distance < closest_node_distance)
                    {
                        closest_node_distance = node_distance;
                        closest_node_index = node_index;
                    }
                }
                if (closest_node_index == -1)
                {
                    LOG("FLEXBODY ERROR on mesh " + mesh_name + ": REF node not found");
                    closest_node_index = 0;
                }
                m_locators[i].ref = closest_node_index;

                //search the second nearest node as the X vector
                closest_node_distance = std::numeric_limits<float>::max();
                closest_node_index = -1;
                for (auto node_index : node_indices)
                {
                    if (node_index == m_locators[i].ref)
                    {
                        continue;
                    }
                    float node_distance = vertices[i].squaredDistance(nodes[node_index].AbsPosition);
                    if (node_distance < closest_node_distance)
                    {
                        closest_node_distance = node_distance;
                        closest_node_index = node_index;
                    }
                }
                if (closest_node_index == -1)
                {
                    LOG("FLEXBODY ERROR on mesh " + mesh_name + ": VX node not found");
                    closest_node_index = 0;
                }
                m_locators[i].nx = closest_node_index;

                //search another close, orthogonal node as the Y vector
                closest_node_distance = std::numeric_limits<float>::max();
                closest_node_index = -1;
                Vector3 vx = (nodes[m_locators[i].nx].AbsPosition - nodes[m_locators[i].ref].AbsPosition).normalisedCopy();
                for (auto node_index : node_indices)
                {
                    if (node_index == m_locators[i].ref || node_index == m_locators[i].nx)
                    {
                        continue;
                    }
                    float node_distance = vertices[i].squaredDistance(nodes[node_index].AbsPosition);
                    if (node_distance < closest_node_distance)
                    {
                        Vector3 vt = (nodes[node_index].AbsPosition - nodes[m_locators[i].ref].AbsPosition).normalisedCopy();
                        float cost = vx.dotProduct(vt);
                        if (std::abs(cost) > std::sqrt(2.0f) / 2.0f)
                        {
                            continue; //rejection, fails the orthogonality criterion (+-45 degree)
                        }
                        closest_node_distance = node_distance;
                        closest_node_index = node_index;
                    }
                }
                if (closest_node_index == -1)
                {
                    LOG("FLEXBODY ERROR on mesh " + mesh_name + ": VY node not found");
                    closest_node_index = 0;
                }
                m_locators[i].ny = closest_node_index;
            }

            Matrix3 mat;
            Vector3 diffX = nodes[m_locators[i].nx].AbsPosition-nodes[m_locators[i].ref].AbsPosition;
            Vector3 diffY = nodes[m_locators[i].ny].AbsPosition-nodes[m_locators[i].ref].AbsPosition;

            mat.SetColumn(0, diffX);
            mat.SetColumn(1, diffY);
            mat.SetColumn(2, (diffX.crossProduct(diffY)).normalisedCopy()); // Old version: mat.SetColumn(2, nodes[loc.nz].AbsPosition-nodes[loc.ref].AbsPosition);

            mat = mat.Inverse();

            //compute coordinates in the newly formed Euclidean basis
            m_locators[i].coords = mat * (vertices[i] - nodes[m_locators[i].ref].AbsPosition);

            // that's it!
        }

    } // if (preloaded_from_cache == nullptr)

    //adjusting bounds
    AxisAlignedBox aab=mesh->getBounds();
    Vector3 v=aab.getMinimum();
    float mi=v.x;
    if (v.y<mi) mi=v.y;
    if (v.z<mi) mi=v.z;
    mi=fabs(mi);
    v=aab.getMaximum();
    float ma=v.x;
    if (ma<v.y) ma=v.y;
    if (ma<v.z) ma=v.z;
    ma=fabs(ma);
    if (mi>ma) ma=mi;
    aab.setMinimum(Vector3(-ma,-ma,-ma));
    aab.setMaximum(Vector3(ma,ma,ma));
    mesh->_setBounds(aab, true);

    //okay, show the mesh now
    m_scene_node=App::GetGfxScene()->GetSceneManager()->getRootSceneNode()->createChildSceneNode();
    m_scene_node->attachObject(ent);
    m_scene_node->setPosition(position);

    if (preloaded_from_cache == nullptr)
    {
        for (int i=0; i<(int)m_vertex_count; i++)
        {
            Matrix3 mat;
            Vector3 diffX = nodes[m_locators[i].nx].AbsPosition-nodes[m_locators[i].ref].AbsPosition;
            Vector3 diffY = nodes[m_locators[i].ny].AbsPosition-nodes[m_locators[i].ref].AbsPosition;

            mat.SetColumn(0, diffX);
            mat.SetColumn(1, diffY);
            mat.SetColumn(2, diffX.crossProduct(diffY).normalisedCopy()); // Old version: mat.SetColumn(2, nodes[loc.nz].AbsPosition-nodes[loc.ref].AbsPosition);

            mat = mat.Inverse();

            // compute coordinates in the Euclidean basis
            m_src_normals[i] = mat*(orientation * m_src_normals[i]);
        }
    }

    if (vertices != nullptr) { free(vertices); }

    // Keep the forset nodes for diagnostics
    for (unsigned int nodenum : node_indices)
    {
        m_forset_nodes.push_back((NodeNum_t)nodenum);
    }

    if (App::GetConsole()->cVarGet("flexbody_defrag_enabled", CVAR_TYPE_BOOL)->getBool()
        // For simplicity, only take 1-submesh meshes (almost always the case anyway)
        && m_scene_entity->getMesh()->getNumSubMeshes() == 1)
    {
        this->defragmentFlexbodyMesh();
    }
}

FlexBody::FlexBody(PlaceholderType p_type, FlexbodyID_t id, const std::string& orig_meshname)
{
    m_camera_mode = CAMERA_MODE_ALWAYS_HIDDEN;
    m_id = id;
    m_orig_mesh_name = orig_meshname;
    m_placeholder_type = p_type;
}

FlexBody::~FlexBody()
{
    // Stuff using <new>
    if (m_locators != nullptr) { delete[] m_locators; }
    // Stuff using malloc()
    if (m_src_normals != nullptr) { free(m_src_normals); }
    if (m_dst_normals != nullptr) { free(m_dst_normals); }
    if (m_dst_pos     != nullptr) { free(m_dst_pos    ); }
    if (m_src_colors  != nullptr) { free(m_src_colors ); }

    this->destroyOgreObjects();
}

/*static*/ const char* FlexBody::PlaceholderTypeToString(PlaceholderType type)
{
    switch (type)
    {
    case PlaceholderType::NOT_A_PLACEHOLDER: return "NOT_A_PLACEHOLDER";
    case PlaceholderType::TUNING_REMOVED_PLACEHOLDER: return "TUNING_REMOVED_PLACEHOLDER";
    case PlaceholderType::FAULTY_FORSET_PLACEHOLDER: return "FAULTY_FORSET_PLACEHOLDER";
    case PlaceholderType::FAULTY_MESH_PLACEHOLDER: return "FAULTY_MESH_PLACEHOLDER";
    default: return "";
    }
}

void FlexBody::destroyOgreObjects()
{
    // Separated out from destructor so that exceptions can be handled separately (C++ destructor cannot propagate exceptions)
    // -----------------------------------------------------------------------------------------------------------------------

    // OGRE resource - scene node
    if (m_scene_node != nullptr)
    {
        m_scene_node->getParentSceneNode()->removeChild(m_scene_node);
        App::GetGfxScene()->GetSceneManager()->destroySceneNode(m_scene_node);
    }
    m_scene_node = nullptr;

    // OGRE resource - scene entity
    if (m_scene_entity != nullptr)
    {
        Ogre::MeshPtr mesh = m_scene_entity->getMesh();
        App::GetGfxScene()->GetSceneManager()->destroyEntity(m_scene_entity);

        // OGRE resource - mesh (unique copy - should be destroyed)
        Ogre::MeshManager::getSingleton().remove(mesh->getHandle());
    }
    m_scene_entity = nullptr;
}

bool FlexBody::isVisible() const
{
    // Scene node is NULL if disabled via addonpart/tuneup.
    return m_scene_node
        && m_scene_node->isInSceneGraph()
        && m_scene_node->getAttachedObject(0)->isVisible();
        
}

void FlexBody::setVisible(bool visible)
{
    // Scene node is NULL if disabled via addonpart/tuneup.
    if (m_scene_node)
        m_scene_node->setVisible(visible);
}

void FlexBody::setFlexbodyCastShadow(bool val)
{
    // Scene entity is NULL if disabled via addonpart/tuneup.
    if (m_scene_entity)
        m_scene_entity->setCastShadows(val);
}

void FlexBody::computeFlexbody()
{
    if (m_has_texture_blend) updateBlend();

    RoR::NodeSB* nodes = m_gfx_actor->GetSimNodeBuffer();

    // compute the local center
    if (m_node_center >= 0)
    {
        Vector3 diffX = nodes[m_node_x].AbsPosition - nodes[m_node_center].AbsPosition;
        Vector3 diffY = nodes[m_node_y].AbsPosition - nodes[m_node_center].AbsPosition;
        Ogre::Vector3 flexit_normal = fast_normalise(diffY.crossProduct(diffX));

        m_flexit_center = nodes[m_node_center].AbsPosition + m_center_offset.x * diffX + m_center_offset.y * diffY;
        m_flexit_center += m_center_offset.z * flexit_normal;
    }
    else
    {
        m_flexit_center = nodes[0].AbsPosition;
    }

    for (int i=0; i<(int)m_vertex_count; i++)
    {
        Vector3 diffX = nodes[m_locators[i].nx].AbsPosition - nodes[m_locators[i].ref].AbsPosition;
        Vector3 diffY = nodes[m_locators[i].ny].AbsPosition - nodes[m_locators[i].ref].AbsPosition;
        Vector3 nCross = fast_normalise(diffX.crossProduct(diffY)); //nCross.normalise();

        m_dst_pos[i].x = diffX.x * m_locators[i].coords.x + diffY.x * m_locators[i].coords.y + nCross.x * m_locators[i].coords.z;
        m_dst_pos[i].y = diffX.y * m_locators[i].coords.x + diffY.y * m_locators[i].coords.y + nCross.y * m_locators[i].coords.z;
        m_dst_pos[i].z = diffX.z * m_locators[i].coords.x + diffY.z * m_locators[i].coords.y + nCross.z * m_locators[i].coords.z;

        m_dst_pos[i] += nodes[m_locators[i].ref].AbsPosition - m_flexit_center;

        m_dst_normals[i].x = diffX.x * m_src_normals[i].x + diffY.x * m_src_normals[i].y + nCross.x * m_src_normals[i].z;
        m_dst_normals[i].y = diffX.y * m_src_normals[i].x + diffY.y * m_src_normals[i].y + nCross.y * m_src_normals[i].z;
        m_dst_normals[i].z = diffX.z * m_src_normals[i].x + diffY.z * m_src_normals[i].y + nCross.z * m_src_normals[i].z;

        m_dst_normals[i] = fast_normalise(m_dst_normals[i]);
    }
}

void FlexBody::updateFlexbodyVertexBuffers()
{
    if (!m_scene_node) // Disabled via addonpart/tuneup
        return;

    Vector3 *ppt = m_dst_pos;
    Vector3 *npt = m_dst_normals;
    if (m_uses_shared_vertex_data)
    {
        m_shared_vbuf_pos->writeData(0, m_shared_buf_num_verts*sizeof(Vector3), ppt, true);
        ppt += m_shared_buf_num_verts;
        m_shared_vbuf_norm->writeData(0, m_shared_buf_num_verts*sizeof(Vector3), npt, true);
        npt += m_shared_buf_num_verts;
    }
    for (int i=0; i<m_num_submesh_vbufs; i++)
    {
        m_submesh_vbufs_pos[i]->writeData(0, m_submesh_vbufs_vertex_counts[i]*sizeof(Vector3), ppt, true);
        ppt += m_submesh_vbufs_vertex_counts[i];
        m_submesh_vbufs_norm[i]->writeData(0, m_submesh_vbufs_vertex_counts[i]*sizeof(Vector3), npt, true);
        npt += m_submesh_vbufs_vertex_counts[i];
    }

    if (m_blend_changed)
    {
        writeBlend();
        m_blend_changed = false;
    }

    m_scene_node->setPosition(m_flexit_center);
}

void FlexBody::reset()
{
    if (m_has_texture_blend)
    {
        for (int i=0; i<(int)m_vertex_count; i++) m_src_colors[i]=0x00000000;
        writeBlend();
    }
}

void FlexBody::writeBlend()
{
    if (!m_has_texture_blend) return;
    ARGB *cpt = m_src_colors;
    if (m_uses_shared_vertex_data)
    {
        m_shared_vbuf_color->writeData(0, m_shared_buf_num_verts*sizeof(ARGB), (void*)cpt, true);
        cpt+=m_shared_buf_num_verts;
    }
    for (int i=0; i<m_num_submesh_vbufs; i++)
    {
        m_submesh_vbufs_color[i]->writeData(0, m_submesh_vbufs_vertex_counts[i]*sizeof(ARGB), (void*)cpt, true);
        cpt+=m_submesh_vbufs_vertex_counts[i];
    }
}

void FlexBody::updateBlend() //so easy!
{
    RoR::NodeSB* nodes = m_gfx_actor->GetSimNodeBuffer();
    for (int i=0; i<(int)m_vertex_count; i++)
    {
        RoR::NodeSB *nd = &nodes[m_locators[i].ref];
        ARGB col = m_src_colors[i];
        if (nd->nd_has_contact && !(col&0xFF000000))
        {
            m_src_colors[i]=col|0xFF000000;
            m_blend_changed = true;
        }
        if (nd->nd_is_wet ^ ((col&0x000000FF)>0))
        {
            m_src_colors[i]=(col&0xFFFFFF00)+0x000000FF*nd->nd_is_wet;
            m_blend_changed = true;
        }
    }
}

int evalNodeDistance(NodeNum_t a, NodeNum_t b)
{
    if (a > b)
    {
        return App::flexbody_defrag_const_penalty->getInt()
            + App::flexbody_defrag_prog_down_penalty->getInt() * (a - b);
    }
    else if (a < b)
    {
        return App::flexbody_defrag_const_penalty->getInt()
            + App::flexbody_defrag_prog_down_penalty->getInt() * (b - a);
    }
    else
    {
        return 0;
    }
}

int evalMemoryDistance(Locator_t& a, Locator_t& b)
{
    return 0
        + evalNodeDistance(a.ref, b.ref) + evalNodeDistance(a.nx, b.ref) + evalNodeDistance(a.ny, b.ref)
        + evalNodeDistance(a.ref, b.nx) + evalNodeDistance(a.nx, b.nx) + evalNodeDistance(a.ny, b.nx)
        + evalNodeDistance(a.ref, b.ny) + evalNodeDistance(a.nx, b.ny) + evalNodeDistance(a.ny, b.ny)
        + evalNodeDistance(a.getSmallestNode(), b.getSmallestNode())
        + evalNodeDistance(a.getMean(), b.getMean());
}

template<typename uint_T> void reorderIndexBuffer(Ogre::IndexData* idx_data, std::vector<int> const& new_index_lookup)
{
    uint_T* workibuf = new uint_T[idx_data->indexCount];
    idx_data->indexBuffer->readData(0, idx_data->indexBuffer->getSizeInBytes(), workibuf);
    for (size_t i = 0; i < idx_data->indexCount; i++)
    {
        workibuf[i] = new_index_lookup[workibuf[i]];
    }
    idx_data->indexBuffer->writeData(0, idx_data->indexBuffer->getSizeInBytes(), workibuf);
    delete[] workibuf;
}

void reorderVertexBuffer(Ogre::HardwareVertexBufferSharedPtr vert_buf, const Ogre::VertexElement* vert_elem, std::vector<int> const& new_index_lookup)
{
    char* workbuf_src = new char[vert_buf->getSizeInBytes()];
    char* workbuf_dst = new char[vert_buf->getSizeInBytes()];
    vert_buf->readData(0, vert_buf->getSizeInBytes(), workbuf_src);
    for (size_t i = 0; i < vert_buf->getNumVertices(); i++)
    {
        void* src = workbuf_src + (i * vert_elem->getSize());
        void* dst = workbuf_dst + (new_index_lookup[i] * vert_elem->getSize());
        std::memcpy(dst, src, vert_elem->getSize());
    }
    vert_buf->writeData(0, vert_buf->getSizeInBytes(), workbuf_dst);
    delete[] workbuf_src;
    delete[] workbuf_dst;
}

void FlexBody::defragmentFlexbodyMesh()
{
    // Analysis
    NodeNum_t forset_max = std::numeric_limits<NodeNum_t>::min();
    NodeNum_t forset_min = std::numeric_limits<NodeNum_t>::max();
    for (NodeNum_t n : this->getForsetNodes())
    {
        if (n > forset_max) { forset_max = n; }
        if (n < forset_min) { forset_min = n; }
    }

    std::vector<int> new_index_lookup(m_vertex_count);
    for (int i = 0; i < (int)m_vertex_count; i++)
    {
        new_index_lookup[i] = i;
    }

    Locator_t prev_loc;
    // edge values to start with
    prev_loc.ref =  forset_min;
    prev_loc.nx =  forset_min;
    prev_loc.ny =  forset_min;

    // SELECTION SORT (https://www.geeksforgeeks.org/selection-sort/)
    for (int i = 0; i < m_vertex_count; i++)
    {
        // Find the next locator closest in memory
        int closest_loc = i;
        int closest_loc_penalty = INT_MAX;
        for (int j = i; j < m_vertex_count; j++)
        {
            int penalty = evalMemoryDistance(prev_loc, m_locators[j]);
            if (penalty < closest_loc_penalty)
            {
                closest_loc_penalty = penalty;
                closest_loc = j;
            }
        }

        // Swap locators+normals in memory, update lookup
        Locator_t loc_tmp = m_locators[closest_loc];
        Ogre::Vector3 norm_tmp = m_src_normals[closest_loc];
        int idx_tmp = new_index_lookup[closest_loc];

        m_locators[closest_loc] = m_locators[i];
        m_src_normals[closest_loc] = m_src_normals[i];
        new_index_lookup[closest_loc] = new_index_lookup[i];

        m_locators[i] = loc_tmp;
        m_src_normals[i] = norm_tmp;
        new_index_lookup[i] = idx_tmp;    

        // Go next
        prev_loc = m_locators[i];
    }

    if (App::flexbody_defrag_invert_lookup->getBool())
    {
        std::vector<int> inverted_lookup(m_vertex_count);
        for (int i = 0; i < (int)m_vertex_count; i++)
        {
            inverted_lookup[new_index_lookup[i]] = i;
        }
        for (int i = 0; i < (int)m_vertex_count; i++)
        {
            new_index_lookup[i] = inverted_lookup[i];
        }
    }

    // REORDERING VERTICES
    // * positions/normals are calculated, no action needed.
    // * texcoords (aka UV-coords) must be fixed.
    if (App::flexbody_defrag_reorder_texcoords->getBool())
    {
        Ogre::VertexData* vert_data = nullptr;
        if (m_scene_entity->getMesh()->sharedVertexData)
        {
            vert_data = m_scene_entity->getMesh()->sharedVertexData;
        }
        else
        {
            // for simplicity we only support single submesh
            vert_data = m_scene_entity->getMesh()->getSubMesh(0)->vertexData;
        }
        const Ogre::VertexElement* uv_elem = vert_data->vertexDeclaration->findElementBySemantic(Ogre::VES_TEXTURE_COORDINATES);
        Ogre::HardwareVertexBufferSharedPtr uv_buf = vert_data->vertexBufferBinding->getBuffer(uv_elem->getSource());
        reorderVertexBuffer(uv_buf, uv_elem, new_index_lookup);
    }

    // REORDERING INDICES
    if (App::flexbody_defrag_reorder_indices->getBool())
    {
        Ogre::IndexData* idx_data = m_scene_entity->getMesh()->getSubMesh(0)->indexData;
        // Index can be 16-bit or 32-bit!
        if (idx_data->indexBuffer->getType() == Ogre::HardwareIndexBuffer::IT_16BIT)
        {
            reorderIndexBuffer<uint16_t>(idx_data, new_index_lookup);
        }
        else
        {
            reorderIndexBuffer<uint32_t>(idx_data, new_index_lookup);
        }
    }
}
