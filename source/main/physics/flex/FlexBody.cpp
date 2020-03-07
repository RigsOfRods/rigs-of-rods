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

#include "FlexBody.h"

#include "Application.h"
#include "ApproxMath.h"
#include "BeamData.h"
#include "FlexFactory.h"
#include "GfxActor.h"
#include "RigDef_File.h"

#include <Ogre.h>

using namespace Ogre;

FlexBody::FlexBody(
    RigDef::Flexbody* def,
    RoR::FlexBodyCacheData* preloaded_from_cache,
    RoR::GfxActor* gfx_actor,
    Ogre::Entity* ent,
    int ref,
    int nx,
    int ny,
    Ogre::Quaternion const & rot,
    std::vector<unsigned int> & node_indices
):
      m_camera_mode(-2)
    , m_center_offset(def->offset)
    , m_node_center(ref)
    , m_node_x(nx)
    , m_node_y(ny)
    , m_has_texture_blend(true)
    , m_scene_node(nullptr)
    , m_scene_entity(ent)
    , m_shared_buf_num_verts(0)
    , m_has_texture(true)
    , m_blend_changed(false)
    , m_locators(nullptr)
    , m_src_normals(nullptr)
    , m_dst_normals(nullptr)
    , m_dst_pos(nullptr)
    , m_src_colors(nullptr)
    , m_gfx_actor(gfx_actor)
{

    Ogre::Vector3* vertices = nullptr;

    Vector3 normal = Vector3::UNIT_Y;
    Vector3 position = Vector3::ZERO;
    Quaternion orientation = Quaternion::ZERO;

    RoR::GfxActor::SimBuffer::NodeSB* nodes = m_gfx_actor->GetSimNodeBuffer();

    if (ref >= 0)
    {
        Vector3 diffX = nodes[nx].AbsPosition-nodes[ref].AbsPosition;
        Vector3 diffY = nodes[ny].AbsPosition-nodes[ref].AbsPosition;

        normal = (diffY.crossProduct(diffX)).normalisedCopy();

        // position
        position = nodes[ref].AbsPosition + def->offset.x * diffX + def->offset.y * diffY;
        position = position + def->offset.z * normal;

        // orientation
        Vector3 refX = diffX.normalisedCopy();
        Vector3 refY = refX.crossProduct(normal);
        orientation  = Quaternion(refX, normal, refY) * rot;
    }
    else
    {
        // special case!
        normal = Vector3::UNIT_Y;
        position = nodes[0].AbsPosition + def->offset;
        orientation = rot;
    }

    Ogre::MeshPtr mesh=ent->getMesh();
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
        m_has_texture        = preloaded_from_cache->header.HasTexture();
        m_has_texture_blend  = preloaded_from_cache->header.HasTextureBlend();
    }

    //create optimal VertexDeclaration
    VertexDeclaration* optimalVD=HardwareBufferManager::getSingleton().createVertexDeclaration();
    optimalVD->addElement(0, 0, VET_FLOAT3, VES_POSITION);
    optimalVD->addElement(1, 0, VET_FLOAT3, VES_NORMAL);
    if (m_has_texture_blend) optimalVD->addElement(2, 0, VET_COLOUR_ARGB, VES_DIFFUSE);
    if (m_has_texture) optimalVD->addElement(3, 0, VET_FLOAT2, VES_TEXTURE_COORDINATES);
    optimalVD->sort();
    optimalVD->closeGapsInSource();
    BufferUsageList optimalBufferUsages;
    for (size_t u = 0; u <= optimalVD->getMaxSource(); ++u)
    {
        optimalBufferUsages.push_back(HardwareBuffer::HBU_DYNAMIC_WRITE_ONLY_DISCARDABLE);
    }

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
        mesh->sharedVertexData->reorganiseBuffers(optimalVD, optimalBufferUsages);
        mesh->sharedVertexData->removeUnusedBuffers();
        mesh->sharedVertexData->closeGapsInBindings();
    }
    Mesh::SubMeshIterator smIt = mesh->getSubMeshIterator();
    while (smIt.hasMoreElements())
    {
        SubMesh* sm = smIt.getNext();
        if (!sm->useSharedVertices)
        {
            sm->vertexData->reorganiseBuffers(optimalVD->clone(), optimalBufferUsages);
            sm->vertexData->removeUnusedBuffers();
            sm->vertexData->closeGapsInBindings();
        }
    }

    //print mesh information
    //LOG("FLEXBODY Printing modififed mesh informations:");
    //printMeshInfo(ent->getMesh().getPointer());

    //get the buffers
    //getMeshInformation(ent->getMesh().getPointer(),m_vertex_count,vertices,index_count,indices, position, orientation, Vector3(1,1,1));

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
        m_uses_shared_vertex_data = preloaded_from_cache->header.UsesSharedVertexData();
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
                LOG("FLEXBODY ERROR on mesh "+def->mesh_name+": REF node not found");
                closest_node_index = 0;
            }
            m_locators[i].ref=closest_node_index;            

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
                LOG("FLEXBODY ERROR on mesh "+def->mesh_name+": VX node not found");
                closest_node_index = 0;
            }
            m_locators[i].nx=closest_node_index;

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
                LOG("FLEXBODY ERROR on mesh "+def->mesh_name+": VY node not found");
                closest_node_index = 0;
            }
            m_locators[i].ny=closest_node_index;

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
    m_scene_node=gEnv->sceneManager->getRootSceneNode()->createChildSceneNode();
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

#ifdef FLEXBODY_LOG_LOADING_TIMES
    char stats[1000];
    sprintf(stats, "FLEXBODY (%s) ready, stats:"
        "\n\tmesh loaded:  %f sec"
        "\n\tmesh ready:   %f sec"
        "\n\tmesh scanned: %f sec"
        "\n\tOgre vertexbuffers created:       %f sec"
        "\n\tOgre vertexbuffers reorganised:   %f sec"
        "\n\tmanual vertexbuffers created:     %f sec"
        "\n\tmanual vertexbuffers transformed: %f sec"
        "\n\tnodes located:      %f sec"
        "\n\tmesh displayed:     %f sec"
        "\n\tnormals calculated: %f sec",
        meshname.c_str(), stat_mesh_loaded_time, stat_mesh_ready_time, stat_mesh_scanned_time, 
        stat_vertexbuffers_created_time, stat_buffers_reorganised_time,
        stat_manual_buffers_created_time, stat_transformed_time, stat_located_time, 
        stat_showmesh_time, stat_euclidean2_time);
    LOG(stats);
#endif
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

    // OGRE resource - scene node
    m_scene_node->getParentSceneNode()->removeChild(m_scene_node);
    gEnv->sceneManager->destroySceneNode(m_scene_node);
    m_scene_node = nullptr;

    // OGRE resource - scene entity
    Ogre::MeshPtr mesh = m_scene_entity->getMesh();
    gEnv->sceneManager->destroyEntity(m_scene_entity);
    m_scene_entity = nullptr;

    // OGRE resource - mesh (unique copy - should be destroyed)
    Ogre::MeshManager::getSingleton().remove(mesh->getHandle());
}

void FlexBody::setVisible(bool visible)
{
    if (m_scene_node)
        m_scene_node->setVisible(visible);
}

void FlexBody::SetFlexbodyCastShadow(bool val)
{
    m_scene_entity->setCastShadows(val);
}

void FlexBody::printMeshInfo(Mesh* mesh)
{
    if (mesh->sharedVertexData)
    {
        LOG("FLEXBODY Mesh has Shared Vertices:");
        VertexData* vt=mesh->sharedVertexData;
        LOG("FLEXBODY element count:"+TOSTRING(vt->vertexDeclaration->getElementCount()));
        for (int j=0; j<(int)vt->vertexDeclaration->getElementCount(); j++)
        {
            const VertexElement* ve=vt->vertexDeclaration->getElement(j);
            LOG("FLEXBODY element "+TOSTRING(j)+" source "+TOSTRING(ve->getSource()));
            LOG("FLEXBODY element "+TOSTRING(j)+" offset "+TOSTRING(ve->getOffset()));
            LOG("FLEXBODY element "+TOSTRING(j)+" type "+TOSTRING(ve->getType()));
            LOG("FLEXBODY element "+TOSTRING(j)+" semantic "+TOSTRING(ve->getSemantic()));
            LOG("FLEXBODY element "+TOSTRING(j)+" size "+TOSTRING(ve->getSize()));
        }
    }
    LOG("FLEXBODY Mesh has "+TOSTRING(mesh->getNumSubMeshes())+" submesh(es)");
    for (int i=0; i<mesh->getNumSubMeshes(); i++)
    {
        SubMesh* submesh = mesh->getSubMesh(i);
        LOG("FLEXBODY SubMesh "+TOSTRING(i)+": uses shared?:"+TOSTRING(submesh->useSharedVertices));
        if (!submesh->useSharedVertices)
        {
            VertexData* vt=submesh->vertexData;
            LOG("FLEXBODY element count:"+TOSTRING(vt->vertexDeclaration->getElementCount()));
            for (int j=0; j<(int)vt->vertexDeclaration->getElementCount(); j++)
            {
                const VertexElement* ve=vt->vertexDeclaration->getElement(j);
                LOG("FLEXBODY element "+TOSTRING(j)+" source "+TOSTRING(ve->getSource()));
                LOG("FLEXBODY element "+TOSTRING(j)+" offset "+TOSTRING(ve->getOffset()));
                LOG("FLEXBODY element "+TOSTRING(j)+" type "+TOSTRING(ve->getType()));
                LOG("FLEXBODY element "+TOSTRING(j)+" semantic "+TOSTRING(ve->getSemantic()));
                LOG("FLEXBODY element "+TOSTRING(j)+" size "+TOSTRING(ve->getSize()));
            }
        }
    }
}

void FlexBody::ComputeFlexbody()
{
    if (m_has_texture_blend) updateBlend();

    RoR::GfxActor::SimBuffer::NodeSB* nodes = m_gfx_actor->GetSimNodeBuffer();

    // compute the local center
    Ogre::Vector3 flexit_normal;

    if (m_node_center >= 0)
    {
        Vector3 diffX = nodes[m_node_x].AbsPosition - nodes[m_node_center].AbsPosition;
        Vector3 diffY = nodes[m_node_y].AbsPosition - nodes[m_node_center].AbsPosition;
        flexit_normal = fast_normalise(diffY.crossProduct(diffX));

        m_flexit_center = nodes[m_node_center].AbsPosition + m_center_offset.x * diffX + m_center_offset.y * diffY;
        m_flexit_center += m_center_offset.z * flexit_normal;
    }
    else
    {
        flexit_normal = Vector3::UNIT_Y;
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

void FlexBody::UpdateFlexbodyVertexBuffers()
{
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
    RoR::GfxActor::SimBuffer::NodeSB* nodes = m_gfx_actor->GetSimNodeBuffer();
    for (int i=0; i<(int)m_vertex_count; i++)
    {
        RoR::GfxActor::SimBuffer::NodeSB *nd = &nodes[m_locators[i].ref];
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

