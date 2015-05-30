/*
	This source file is part of Rigs of Rods
	Copyright 2005-2012 Pierre-Michel Ricordel
	Copyright 2007-2012 Thomas Fischer
	Copyright 2013-2015 Petr Ohlidal

	For more information, see http://www.rigsofrods.com/

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

#include "ApproxMath.h"
#include "FlexFactory.h"
#include "MaterialReplacer.h"
#include "ResourceBuffer.h"
#include "RigLoadingProfilerControl.h"
#include "Skin.h"
#include "BeamData.h"

using namespace Ogre;

#define FLEX_COMPAT_v0_38_67

FlexBody::FlexBody(
    RoR::FlexBodyCacheData* preloaded_from_cache,
	node_t *all_nodes, 
	int numnodes, 
	Ogre::String const & meshname, 
	Ogre::String const & uname, 
	int ref, 
	int nx, 
	int ny, 
	Ogre::Vector3 const & offset, 
	Ogre::Quaternion const & rot, 
	std::vector<unsigned int> & node_indices, 
	MaterialFunctionMapper *material_function_mapper, 
	Skin *usedSkin, 
	MaterialReplacer *material_replacer,
    bool enable_LODs // = false
):
	  m_camera_mode(-2)	
	, m_center_offset(offset)
	, m_node_center(ref)
	, m_node_x(nx)
	, m_node_y(ny)
	, m_is_enabled(true)
	, m_is_faulty(false)
	, m_has_texture_blend(true)
	, m_nodes(all_nodes)
	, m_scene_node(nullptr)
    , m_has_texture(true)
    , m_locators(nullptr)
    , m_src_normals(nullptr)
    , m_dst_normals(nullptr)
    , m_dst_pos(nullptr)
    , m_src_colors(nullptr)
{
    TIMER_CREATE();
    FLEXBODY_PROFILER_START("Compute pos + orientation");

	m_nodes[m_node_center].iIsSkin=true;
	m_nodes[m_node_x].iIsSkin=true;
	m_nodes[m_node_y].iIsSkin=true;

    Ogre::Vector3* vertices = nullptr;

	Vector3 normal = Vector3::UNIT_Y;
	Vector3 position = Vector3::ZERO;
	Quaternion orientation = Quaternion::ZERO;
#ifdef FLEX_COMPAT_v0_38_67
    const bool compat_mode = true;
#else
    const bool compat_mode = false;
#endif
	if (ref >= 0)
	{
        if (compat_mode)
        { // v0.38.67
            normal=(m_nodes[ny].smoothpos-m_nodes[ref].smoothpos).crossProduct(m_nodes[nx].smoothpos-m_nodes[ref].smoothpos);
		    normal.normalise();
		    //position
		    position=m_nodes[ref].smoothpos+offset.x*(m_nodes[nx].smoothpos-m_nodes[ref].smoothpos)+offset.y*(m_nodes[ny].smoothpos-m_nodes[ref].smoothpos);
		    position=(position+normal*offset.z);
		    //orientation
		    Vector3 refx=m_nodes[nx].smoothpos-m_nodes[ref].smoothpos;
		    refx.normalise();
		    Vector3 refy=refx.crossProduct(normal);
		    orientation=Quaternion(refx, normal, refy)*rot;
        }
        else
        { // v0.4.0.7
		    Vector3 diffX = m_nodes[nx].smoothpos-m_nodes[ref].smoothpos;
		    Vector3 diffY = m_nodes[ny].smoothpos-m_nodes[ref].smoothpos;

		    normal = fast_normalise(diffY.crossProduct(diffX));

		    // position
		    position = m_nodes[ref].smoothpos + offset.x*diffX + offset.y*diffY;
		    position = position + offset.z*normal;

		    // orientation
		    Vector3 refX = fast_normalise(diffX);
		    Vector3 refY = refX.crossProduct(normal);
		    orientation  = Quaternion(refX, normal, refY) * rot;
        }
	} 
    else
	{
		// special case!
		normal = Vector3::UNIT_Y;
		position=m_nodes[0].smoothpos+offset;
		orientation = rot;
	}
    FLEXBODY_PROFILER_ENTER("Find mesh resource");

	// load unique mesh (load original mesh and clone it with unique name)

	// find group that contains the mesh
	String groupname="";
	try
	{
		groupname = ResourceGroupManager::getSingleton().findGroupContainingResource(meshname);
	}catch(...)
	{
	}
	if (groupname == "")
	{
		LOG("FLEXBODY mesh not found: "+String(meshname));
		m_is_faulty=true;
		return;
	}
    FLEXBODY_PROFILER_ENTER("Build unique name");
	// build new unique mesh name
	char uname_mesh[256] = {};
	strncpy(uname_mesh, uname.c_str(), 250);
	uname_mesh[250] = '\0';
	strcat(uname_mesh, "_mesh");
    FLEXBODY_PROFILER_ENTER("Load mesh");
	MeshPtr mesh = MeshManager::getSingleton().load(meshname, groupname);
    TIMER_SNAPSHOT(stat_mesh_loaded_time);
    FLEXBODY_PROFILER_ENTER("Clone mesh");
	MeshPtr newmesh = mesh->clone(uname_mesh);
    if (enable_LODs)
    {
        // now find possible LODs
	    FLEXBODY_PROFILER_ENTER("Handle LODs >> Split mesh name");
 	    String basename, ext;
	    StringUtil::splitBaseFilename(String(meshname), basename, ext);
        FLEXBODY_PROFILER_ENTER("Handle LODs >> Run loop");
	    for (int i=0; i<4;i++)
	    {
            bool exists;
            String fn;
            {
                FLEXBODY_PROFILER_SCOPED("LOD LOOP > Find resource > call Ogre::ResGroupMan::resExistsInAnyGroup()");
		        fn = basename + "_" + TOSTRING(i) + ".mesh";
		        exists = ResourceGroupManager::getSingleton().resourceExistsInAnyGroup(fn);
            }
		    if (!exists) continue;
            {
                FLEXBODY_PROFILER_SCOPED("LOD LOOP > Create manual LOD level");
		        float distance = 3;
		        if (i == 1) distance = 20;
		        if (i == 2) distance = 50;
		        if (i == 3) distance = 200;
		        newmesh->createManualLodLevel(distance, fn);
            }
	    }
    }
    FLEXBODY_PROFILER_ENTER("Create entity");
	Entity *ent = gEnv->sceneManager->createEntity(uname, uname_mesh);
    FLEXBODY_PROFILER_ENTER("MaterialFunctionMapper::replaceSimpleMeshMaterials()");
	MaterialFunctionMapper::replaceSimpleMeshMaterials(ent, ColourValue(0.5, 0.5, 1));
	if (material_function_mapper)
    {
        FLEXBODY_PROFILER_ENTER("mat_function_mapper->replaceMeshMaterials()")
        material_function_mapper->replaceMeshMaterials(ent);
    }
	if (material_replacer)
    {
        FLEXBODY_PROFILER_ENTER("material_replacer->replaceMeshMaterials()")
        material_replacer->replaceMeshMaterials(ent);
    }
	if (usedSkin)
    {
        FLEXBODY_PROFILER_ENTER("usedSkin->replaceMeshMaterials()")
        usedSkin->replaceMeshMaterials(ent);
    }
    TIMER_SNAPSHOT(stat_mesh_ready_time);
    FLEXBODY_PROFILER_ENTER("Check texcoord presence")

	m_mesh=ent->getMesh();
    int num_submeshes = m_mesh->getNumSubMeshes();
    if (preloaded_from_cache == nullptr)
    {
        //determine if we have texture coordinates everywhere
	    if (m_mesh->sharedVertexData && m_mesh->sharedVertexData->vertexDeclaration->findElementBySemantic(VES_TEXTURE_COORDINATES)==0)
        {
            m_has_texture=false;
        }
        for (int i=0; i<num_submeshes; i++) 
        { 
            if (!m_mesh->getSubMesh(i)->useSharedVertices && m_mesh->getSubMesh(i)->vertexData->vertexDeclaration->findElementBySemantic(VES_TEXTURE_COORDINATES)==0) 
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
	    if (m_mesh->sharedVertexData && m_mesh->sharedVertexData->vertexDeclaration->findElementBySemantic(VES_NORMAL)==0)
        {
            havenormal=false;
        }
        for (int i=0; i<num_submeshes; i++) 
        { 
            if (!m_mesh->getSubMesh(i)->useSharedVertices && m_mesh->getSubMesh(i)->vertexData->vertexDeclaration->findElementBySemantic(VES_NORMAL)==0) 
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
    FLEXBODY_PROFILER_ENTER("Detect missing normals")
	
    TIMER_SNAPSHOT(stat_mesh_scanned_time);
    FLEXBODY_PROFILER_ENTER("Create vertex declaration")

	//create optimal VertexDeclaration
	VertexDeclaration* optimalVD=HardwareBufferManager::getSingleton().createVertexDeclaration();
    FLEXBODY_PROFILER_ENTER("Setup vertex declaration")
	optimalVD->addElement(0, 0, VET_FLOAT3, VES_POSITION);
	optimalVD->addElement(1, 0, VET_FLOAT3, VES_NORMAL);
	if (m_has_texture_blend) optimalVD->addElement(2, 0, VET_COLOUR_ARGB, VES_DIFFUSE);
	if (m_has_texture) optimalVD->addElement(3, 0, VET_FLOAT2, VES_TEXTURE_COORDINATES);
    FLEXBODY_PROFILER_ENTER("Sort vertex decl")
	optimalVD->sort();
    FLEXBODY_PROFILER_ENTER("Vertex decl -> closeGapsInSource()")
	optimalVD->closeGapsInSource();
    FLEXBODY_PROFILER_ENTER("Create 'optimal buffer usage' list")
	BufferUsageList optimalBufferUsages;
	for (size_t u = 0; u <= optimalVD->getMaxSource(); ++u) 
    {
        optimalBufferUsages.push_back(HardwareBuffer::HBU_DYNAMIC_WRITE_ONLY_DISCARDABLE);
    }

    FLEXBODY_PROFILER_ENTER("Create color buffers")
	//adding color buffers, well get the reference later
	if (m_has_texture_blend)
	{
		if (m_mesh->sharedVertexData)
		{
			if (m_mesh->sharedVertexData->vertexDeclaration->findElementBySemantic(VES_DIFFUSE)==0)
			{
				//add buffer
				int index=m_mesh->sharedVertexData->vertexDeclaration->getMaxSource()+1;
				m_mesh->sharedVertexData->vertexDeclaration->addElement(index, 0, VET_COLOUR_ARGB, VES_DIFFUSE);
				m_mesh->sharedVertexData->vertexDeclaration->sort();
				index=m_mesh->sharedVertexData->vertexDeclaration->findElementBySemantic(VES_DIFFUSE)->getSource();
				HardwareVertexBufferSharedPtr vbuf=HardwareBufferManager::getSingleton().createVertexBuffer(VertexElement::getTypeSize(VET_COLOUR_ARGB), m_mesh->sharedVertexData->vertexCount, HardwareBuffer::HBU_DYNAMIC_WRITE_ONLY_DISCARDABLE);
				m_mesh->sharedVertexData->vertexBufferBinding->setBinding(index, vbuf);
			}
		}
		for (int i=0; i<num_submeshes; i++)
        {
            if (!m_mesh->getSubMesh(i)->useSharedVertices)
		    {
                Ogre::VertexData* vertex_data = m_mesh->getSubMesh(i)->vertexData;
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
    FLEXBODY_PROFILER_ENTER("Reorganise vertex buffers")
    TIMER_SNAPSHOT(stat_vertexbuffers_created_time);

	//reorg
	//LOG("FLEXBODY reorganizing buffers");
	if (m_mesh->sharedVertexData)
	{
		m_mesh->sharedVertexData->reorganiseBuffers(optimalVD, optimalBufferUsages);
		m_mesh->sharedVertexData->removeUnusedBuffers();
		m_mesh->sharedVertexData->closeGapsInBindings();
	}
    Mesh::SubMeshIterator smIt = m_mesh->getSubMeshIterator();
	while (smIt.hasMoreElements())
	{
		SubMesh* sm = smIt.getNext();
		if (!sm->useSharedVertices)
		{
			sm->vertexData->reorganiseBuffers(optimalVD, optimalBufferUsages);
			sm->vertexData->removeUnusedBuffers();
			sm->vertexData->closeGapsInBindings();
		}
	}
    TIMER_SNAPSHOT(stat_buffers_reorganised_time);
    FLEXBODY_PROFILER_ENTER("Count vertices")

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
	    if (m_mesh->sharedVertexData)
	    {
		    m_vertex_count+=m_mesh->sharedVertexData->vertexCount;
		    m_uses_shared_vertex_data=true;
	    }
	    for (int i=0; i<num_submeshes; i++)
	    {
		    if (!m_mesh->getSubMesh(i)->useSharedVertices)
		    {
			    m_vertex_count+=m_mesh->getSubMesh(i)->vertexData->vertexCount;
			    m_num_submesh_vbufs++;
		    }
	    }
    }
    else
    {
        m_vertex_count            = preloaded_from_cache->header.vertex_count;
        m_uses_shared_vertex_data = preloaded_from_cache->header.UsesSharedVertexData();
        m_num_submesh_vbufs       = preloaded_from_cache->header.num_submesh_vbufs;
    }

	LOG("FLEXBODY Vertices in mesh "+String(meshname)+": "+ TOSTRING(m_vertex_count));
	
    // Profiler data
    double stat_manual_buffers_created_time = -1;
    double stat_transformed_time = -1;
    double stat_located_time = -1;
    if (preloaded_from_cache != nullptr)
    {
        FLEXBODY_PROFILER_ENTER("Copy buffers + locators from cache")

        m_dst_pos     = preloaded_from_cache->dst_pos;
        m_src_normals = preloaded_from_cache->src_normals;
        m_locators    = preloaded_from_cache->locators;
        m_dst_normals = (Vector3*)malloc(sizeof(Vector3)*m_vertex_count); // Use malloc() for compatibility

        if (m_has_texture_blend)
	    {
		    m_src_colors = preloaded_from_cache->src_colors;
	    }

        if (m_mesh->sharedVertexData)
	    {
		    m_shared_buf_num_verts=(int)m_mesh->sharedVertexData->vertexCount;

		    //vertices
		    int source=m_mesh->sharedVertexData->vertexDeclaration->findElementBySemantic(VES_POSITION)->getSource();
		    m_shared_vbuf_pos=m_mesh->sharedVertexData->vertexBufferBinding->getBuffer(source);
		    //normals
		    source=m_mesh->sharedVertexData->vertexDeclaration->findElementBySemantic(VES_NORMAL)->getSource();
		    m_shared_vbuf_norm=m_mesh->sharedVertexData->vertexBufferBinding->getBuffer(source);
		    //colors
		    if (m_has_texture_blend)
		    {
			    source=m_mesh->sharedVertexData->vertexDeclaration->findElementBySemantic(VES_DIFFUSE)->getSource();
			    m_shared_vbuf_color=m_mesh->sharedVertexData->vertexBufferBinding->getBuffer(source);
		    }
	    }
        unsigned int curr_submesh_idx = 0;
	    for (int i=0; i<num_submeshes; i++)
	    {
            const Ogre::SubMesh* submesh = m_mesh->getSubMesh(i);
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
        FLEXBODY_PROFILER_ENTER("Alloc buffers")
	    vertices=(Vector3*)malloc(sizeof(Vector3)*m_vertex_count);
	    m_dst_pos=(Vector3*)malloc(sizeof(Vector3)*m_vertex_count);
	    m_src_normals=(Vector3*)malloc(sizeof(Vector3)*m_vertex_count);
	    m_dst_normals=(Vector3*)malloc(sizeof(Vector3)*m_vertex_count);
	    if (m_has_texture_blend)
	    {
		    m_src_colors=(ARGB*)malloc(sizeof(ARGB)*m_vertex_count);
		    for (int i=0; i<(int)m_vertex_count; i++) m_src_colors[i]=0x00000000;
	    }
        FLEXBODY_PROFILER_ENTER("Fill buffers")
	    Vector3* vpt=vertices;
	    Vector3* npt=m_src_normals;
	    if (m_mesh->sharedVertexData)
	    {
		    m_shared_buf_num_verts=(int)m_mesh->sharedVertexData->vertexCount;
		    //vertices
		    int source=m_mesh->sharedVertexData->vertexDeclaration->findElementBySemantic(VES_POSITION)->getSource();
		    m_shared_vbuf_pos=m_mesh->sharedVertexData->vertexBufferBinding->getBuffer(source);
		    m_shared_vbuf_pos->readData(0, m_mesh->sharedVertexData->vertexCount*sizeof(Vector3), (void*)vpt);
		    vpt+=m_mesh->sharedVertexData->vertexCount;
		    //normals
		    source=m_mesh->sharedVertexData->vertexDeclaration->findElementBySemantic(VES_NORMAL)->getSource();
		    m_shared_vbuf_norm=m_mesh->sharedVertexData->vertexBufferBinding->getBuffer(source);
		    m_shared_vbuf_norm->readData(0, m_mesh->sharedVertexData->vertexCount*sizeof(Vector3), (void*)npt);
		    npt+=m_mesh->sharedVertexData->vertexCount;
		    //colors
		    if (m_has_texture_blend)
		    {
			    source=m_mesh->sharedVertexData->vertexDeclaration->findElementBySemantic(VES_DIFFUSE)->getSource();
			    m_shared_vbuf_color=m_mesh->sharedVertexData->vertexBufferBinding->getBuffer(source);
			    m_shared_vbuf_color->writeData(0, m_mesh->sharedVertexData->vertexCount*sizeof(ARGB), (void*)m_src_colors);
		    }
	    }
        int cursubmesh=0;
	    for (int i=0; i<num_submeshes; i++)
	    {
            const Ogre::SubMesh* submesh = m_mesh->getSubMesh(i);
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
        TIMER_SNAPSHOT_REF(stat_manual_buffers_created_time);

        FLEXBODY_PROFILER_ENTER("Transform vertices")
	    //transform
	    for (int i=0; i<(int)m_vertex_count; i++)
	    {
		    vertices[i]=(orientation*vertices[i])+position;
	    }
        TIMER_SNAPSHOT_REF(stat_transformed_time);

        FLEXBODY_PROFILER_ENTER("Locate nodes")
	    m_locators = new Locator_t[m_vertex_count];
        int vertex_count = m_vertex_count;
        if (compat_mode)
        { // v0.38.67
            for (int i = 0; i<vertex_count; ++i)
            {
                //search nearest node as the local origin
		        float mindist=100000.0;
		        int minnode=-1;

                auto end  = node_indices.end();
                auto itor = node_indices.begin();
                for (; itor != end; ++itor)
                {
                    float dist=(vertices[i]-m_nodes[*itor].smoothpos).length();
			        if (dist<mindist) 
                    {
                        mindist=dist;
                        minnode=*itor;
                    }
                }
		        if (minnode==-1) { LOG("FLEXBODY ERROR on mesh "+String(meshname)+": REF node not found"); }
		        m_locators[i].ref=minnode;
		        m_nodes[minnode].iIsSkin=true;

                //search the second nearest node as the X vector
		        mindist=100000.0;
		        minnode=-1;
                auto end_x  = node_indices.end();
                auto itor_x = node_indices.begin();
                for (; itor_x != end_x; ++itor_x)
                {
                    int node_idx = *itor_x;
                    if (node_idx==m_locators[i].ref) continue;
			        float dist=(vertices[i]-m_nodes[node_idx].smoothpos).length();
			        if (dist<mindist) 
                    {
                        mindist=dist;
                        minnode=node_idx;
                    }
                }

		        if (minnode==-1) 
                {
                    LOG("FLEXBODY ERROR on mesh "+String(meshname)+": VX node not found");
                }
		        m_locators[i].nx=minnode;
		        m_nodes[minnode].iIsSkin=true;

		        //search another close, orthogonal node as the Y vector
		        mindist=100000.0;
		        minnode=-1;
		        Vector3 vx=m_nodes[m_locators[i].nx].smoothpos-m_nodes[m_locators[i].ref].smoothpos;
		        vx.normalise();

                auto end_y  = node_indices.end();
                auto itor_y = node_indices.begin();
                for (; itor_y != end_y; ++itor_y)
                {
                    int k = *itor_y;
                    if (k==m_locators[i].ref || k==m_locators[i].nx) 
                    {
                        continue;
                    }
			        Vector3 vt=m_nodes[k].smoothpos-m_nodes[m_locators[i].ref].smoothpos;
			        vt.normalise();
			        float cost=vx.dotProduct(vt);
			        if (cost>0.707 || cost<-0.707)
                    { 
                        continue; //rejection, fails the orthogonality criterion (+-45 degree)
                    }
			        float dist=(vertices[i]-m_nodes[k].smoothpos).length();
			        if (dist<mindist) 
                    {
                        mindist=dist;
                        minnode=k;
                    }
                }

		        if (minnode==-1)
                {
                    LOG("FLEXBODY ERROR on mesh "+String(meshname)+": VY node not found");
                }
		        m_locators[i].ny=minnode;
		        m_nodes[minnode].iIsSkin=true;
        
		        Vector3 vz=(m_nodes[m_locators[i].nx].smoothpos-m_nodes[m_locators[i].ref].smoothpos).crossProduct(m_nodes[m_locators[i].ny].smoothpos-m_nodes[m_locators[i].ref].smoothpos);
		        vz.normalise();
		        Matrix3 mat;
		        mat.SetColumn(0, m_nodes[m_locators[i].nx].smoothpos-m_nodes[m_locators[i].ref].smoothpos);
		        mat.SetColumn(1, m_nodes[m_locators[i].ny].smoothpos-m_nodes[m_locators[i].ref].smoothpos);
		        mat.SetColumn(2, vz);
		        mat=mat.Inverse();

		        //compute coordinates in the newly formed euclidian basis
		        m_locators[i].coords=mat*(vertices[i]-m_nodes[m_locators[i].ref].smoothpos);

		        //thats it!

            }
        }
        else
        { // v0.4.0.7
	        for (int i=0; i<(int)m_vertex_count; i++)
	        {
		        //search nearest node as the local origin
		        float closest_node_distance = 1000000.0;
		        int closest_node_index = -1;
                auto end  = node_indices.end();
                auto itor = node_indices.begin();
                for (; itor != end; ++itor)
                {
                    float node_distance = vertices[i].squaredDistance(m_nodes[*itor].smoothpos);
                    if (node_distance < closest_node_distance)
                    {
                        closest_node_distance = node_distance;
                        closest_node_index = *itor;
                    }
                }
                if (closest_node_index==-1)
                {
                    LOG("FLEXBODY ERROR on mesh "+String(meshname)+": REF node not found");
                }
                else
                {
                    m_nodes[closest_node_index].iIsSkin=true;
                }
                m_locators[i].ref=closest_node_index;            

		        //search the second nearest node as the X vector
		        closest_node_distance=1000000.0;
		        closest_node_index=-1;
                itor = node_indices.begin();
                for (; itor != end; ++itor)
                {
                    if (*itor == m_locators[i].ref)
                    {
                        continue;
                    }
                    float node_distance = vertices[i].squaredDistance(m_nodes[*itor].smoothpos);
                    if (node_distance < closest_node_distance)
                    {
                        closest_node_distance = node_distance;
                        closest_node_index = *itor;
                    }
                }
                if (closest_node_index==-1)
                {
                    LOG("FLEXBODY ERROR on mesh "+String(meshname)+": VX node not found");
                }
                else
                {
                    m_nodes[closest_node_index].iIsSkin=true;
                }
                m_locators[i].nx=closest_node_index;

		        //search another close, orthogonal node as the Y vector
                closest_node_distance=1000000.0;
		        closest_node_index=-1;
                itor = node_indices.begin();
                Vector3 vx = fast_normalise(m_nodes[m_locators[i].nx].smoothpos - m_nodes[m_locators[i].ref].smoothpos);
                for (; itor != end; ++itor)
                {
                    if (*itor == m_locators[i].ref || *itor == m_locators[i].nx)
                    {
                        continue;
                    }
                    float node_distance = vertices[i].squaredDistance(m_nodes[*itor].smoothpos);
                    if (node_distance < closest_node_distance)
                    {
                        Vector3 vt = fast_normalise(m_nodes[*itor].smoothpos - m_nodes[m_locators[i].ref].smoothpos);
                        float cost = vx.dotProduct(vt);
                        if (cost>0.707 || cost<-0.707)
                        {
                            continue; //rejection, fails the orthogonality criterion (+-45 degree)
                        }
                        closest_node_distance = node_distance;
                        closest_node_index = *itor;
                    }
                }
                if (closest_node_index==-1)
                {
                    LOG("FLEXBODY ERROR on mesh "+String(meshname)+": VY node not found");
                }
                else
                {
                    m_nodes[closest_node_index].iIsSkin=true;
                }
                m_locators[i].ny=closest_node_index;

		        // If something unexpected happens here, then
		        // replace fast_normalise(a) with a.normalisedCopy()

		        Matrix3 mat;
		        Vector3 diffX = m_nodes[m_locators[i].nx].smoothpos-m_nodes[m_locators[i].ref].smoothpos;
		        Vector3 diffY = m_nodes[m_locators[i].ny].smoothpos-m_nodes[m_locators[i].ref].smoothpos;

		        mat.SetColumn(0, diffX);
		        mat.SetColumn(1, diffY);
		        mat.SetColumn(2, fast_normalise(diffX.crossProduct(diffY))); // Old version: mat.SetColumn(2, m_nodes[loc.nz].smoothpos-m_nodes[loc.ref].smoothpos);

		        mat = mat.Inverse();

		        //compute coordinates in the newly formed Euclidean basis
		        m_locators[i].coords= mat * (vertices[i] - m_nodes[m_locators[i].ref].smoothpos);

		        // that's it!
	        }
        }
        TIMER_SNAPSHOT_REF(stat_located_time);

    } // if (preloaded_from_cache == nullptr)

	//adjusting bounds
    FLEXBODY_PROFILER_ENTER("Adjust bounds")
	AxisAlignedBox aab=m_mesh->getBounds();
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
	m_mesh->_setBounds(aab, true);

    FLEXBODY_PROFILER_ENTER("Attach mesh to scene")
	LOG("FLEXBODY show mesh");
	//okay, show the mesh now
	m_scene_node=gEnv->sceneManager->getRootSceneNode()->createChildSceneNode();
	m_scene_node->attachObject(ent);
	m_scene_node->setPosition(position);

    TIMER_SNAPSHOT(stat_showmesh_time);
    if (preloaded_from_cache == nullptr)
    {
        FLEXBODY_PROFILER_ENTER("Transform normals")
        if (compat_mode)
        { // v0.38.67
        	for (int i=0; i<(int)m_vertex_count; i++)
	        {
		        Vector3 vz=(m_nodes[m_locators[i].nx].smoothpos-m_nodes[m_locators[i].ref].smoothpos).crossProduct(m_nodes[m_locators[i].ny].smoothpos-m_nodes[m_locators[i].ref].smoothpos);
		        vz.normalise();
		        Matrix3 mat;
		        mat.SetColumn(0, m_nodes[m_locators[i].nx].smoothpos-m_nodes[m_locators[i].ref].smoothpos);
		        mat.SetColumn(1, m_nodes[m_locators[i].ny].smoothpos-m_nodes[m_locators[i].ref].smoothpos);
		        mat.SetColumn(2, vz);
		        mat=mat.Inverse();

		        //compute coordinates in the euclidian basis
		        m_src_normals[i]=mat*(orientation*m_src_normals[i]);
	        }
        }
        else
        { // v0.4.0.7
	        // If something unexpected happens here, then
	        // replace fast_normalise(a) with a.normalisedCopy()
	        for (int i=0; i<(int)m_vertex_count; i++)
	        {
		        Matrix3 mat;
		        Vector3 diffX = m_nodes[m_locators[i].nx].smoothpos-m_nodes[m_locators[i].ref].smoothpos;
		        Vector3 diffY = m_nodes[m_locators[i].ny].smoothpos-m_nodes[m_locators[i].ref].smoothpos;

		        mat.SetColumn(0, diffX);
		        mat.SetColumn(1, diffY);
		        mat.SetColumn(2, fast_normalise(diffX.crossProduct(diffY))); // Old version: mat.SetColumn(2, m_nodes[loc.nz].smoothpos-m_nodes[loc.ref].smoothpos);

		        mat = mat.Inverse();

		        // compute coordinates in the Euclidean basis
		        m_src_normals[i] = mat*(orientation * m_src_normals[i]);
	        }
        }
    }

    TIMER_SNAPSHOT(stat_euclidean2_time);
    FLEXBODY_PROFILER_ENTER("Printing time stats");

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
#else
    LOG("FLEXBODY ready");
#endif
    FLEXBODY_PROFILER_EXIT();
}

FlexBody::~FlexBody()
{
    // Stuff using <new>
    if (m_locators != nullptr) { delete m_locators; }
    // Stuff using malloc()
    if (m_src_normals != nullptr) { free(m_src_normals); }
    if (m_dst_normals != nullptr) { free(m_dst_normals); }
    if (m_dst_pos     != nullptr) { free(m_dst_pos    ); }
    if (m_src_colors  != nullptr) { free(m_src_colors ); }
}

void FlexBody::setEnabled(bool e)
{
	if (m_is_faulty) return;
	setVisible(e);
	m_is_enabled = e;
}

void FlexBody::setVisible(bool visible)
{
	if (m_is_faulty) return;
	if (!m_is_enabled) return;
	if (m_scene_node)
		m_scene_node->setVisible(visible);
}

void FlexBody::printMeshInfo(Mesh* mesh)
{
	if (m_is_faulty) return;
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

#ifdef FLEX_COMPAT_v0_38_67
bool FlexBody::flexitPrepare(Beam* b)
{
	if (m_is_faulty) return false;
	if (!m_is_enabled) return false;
	if (m_has_texture_blend) updateBlend();
	
	// compute the local center
	Vector3 normal;
	Vector3 center;
	if(m_node_center >= 0)
	{
		normal=(m_nodes[m_node_y].smoothpos-m_nodes[m_node_center].smoothpos).crossProduct(m_nodes[m_node_x].smoothpos-m_nodes[m_node_center].smoothpos);
		normal.normalise();
		center=m_nodes[m_node_center].smoothpos
            +m_center_offset.x*(m_nodes[m_node_x].smoothpos-m_nodes[m_node_center].smoothpos)
            +m_center_offset.y*(m_nodes[m_node_y].smoothpos-m_nodes[m_node_center].smoothpos);
		center=(center+normal*m_center_offset.z);
	} 
    else
	{
		normal = Vector3::UNIT_Y;
		center = m_nodes[0].smoothpos;
	}
    flexit_center = center;

	return Flexable::flexitPrepare(b);
}

void FlexBody::flexitCompute()
{
    Ogre::Vector3 center = flexit_center;
	for (int i=0; i<(int)m_vertex_count; i++)
	{
		Locator_t *loc=&m_locators[i];
		Matrix3 mat;
		mat.SetColumn(0, m_nodes[loc->nx].smoothpos-m_nodes[loc->ref].smoothpos);
		mat.SetColumn(1, m_nodes[loc->ny].smoothpos-m_nodes[loc->ref].smoothpos);

		Vector3 vz=(m_nodes[loc->nx].smoothpos-m_nodes[loc->ref].smoothpos).crossProduct(m_nodes[loc->ny].smoothpos-m_nodes[loc->ref].smoothpos);
		vz.normalise();
		mat.SetColumn(2, vz);

		m_dst_pos[i]=mat*loc->coords+m_nodes[loc->ref].smoothpos-center;
		m_dst_normals[i]=mat*m_src_normals[i];
		m_dst_normals[i].normalise(); //painfull but necessary!
	}
}

#else // #ifndef FLEX_COMPAT_v0_38_67

bool FlexBody::flexitPrepare(Beam* b)
{
	if (m_is_faulty) return false;
	if (!m_is_enabled) return false;
	if (m_has_texture_blend) updateBlend();
	
	// compute the local center
	Ogre::Vector3 flexit_normal;

	if (m_node_center >= 0)
	{
		Vector3 diffX = m_nodes[m_node_x].smoothpos - m_nodes[m_node_center].smoothpos;
		Vector3 diffY = m_nodes[m_node_y].smoothpos - m_nodes[m_node_center].smoothpos;
		flexit_normal = diffY.crossProduct(diffX).normalisedCopy();

		flexit_center = m_nodes[m_node_center].smoothpos + m_center_offset.x*diffX + m_center_offset.y*diffY;
		flexit_center += m_center_offset.z*flexit_normal;
	} else
	{
		flexit_normal = Vector3::UNIT_Y;
		flexit_center = m_nodes[0].smoothpos;
	}

	return Flexable::flexitPrepare(b);
}

void FlexBody::flexitCompute()
{
	// If something unexpected happens here, then
	// replace approx_normalise(a) with a.normalisedCopy()
	for (int i=0; i<(int)m_vertex_count; i++)
	{
		Vector3 diffX = m_nodes[m_locators[i].nx].smoothpos - m_nodes[m_locators[i].ref].smoothpos;
		Vector3 diffY = m_nodes[m_locators[i].ny].smoothpos - m_nodes[m_locators[i].ref].smoothpos;
		Vector3 nCross = approx_normalise(diffX.crossProduct(diffY));

		m_dst_pos[i].x = diffX.x * m_locators[i].coords.x + diffY.x * m_locators[i].coords.y + nCross.x * m_locators[i].coords.z;
		m_dst_pos[i].y = diffX.y * m_locators[i].coords.x + diffY.y * m_locators[i].coords.y + nCross.y * m_locators[i].coords.z;
		m_dst_pos[i].z = diffX.z * m_locators[i].coords.x + diffY.z * m_locators[i].coords.y + nCross.z * m_locators[i].coords.z;

		m_dst_pos[i] += m_nodes[m_locators[i].ref].smoothpos - flexit_center;

		m_dst_normals[i].x = diffX.x * m_src_normals[i].x + diffY.x * m_src_normals[i].y + nCross.x * m_src_normals[i].z;
		m_dst_normals[i].y = diffX.y * m_src_normals[i].x + diffY.y * m_src_normals[i].y + nCross.y * m_src_normals[i].z;
		m_dst_normals[i].z = diffX.z * m_src_normals[i].x + diffY.z * m_src_normals[i].y + nCross.z * m_src_normals[i].z;

		m_dst_normals[i] = approx_normalise(m_dst_normals[i]);
	}
#if 0
	for (int i=0; i<(int)m_vertex_count; i++)
	{
		Matrix3 mat;
		Vector3 diffX = m_nodes[m_locators[i].nx].smoothpos - m_nodes[m_locators[i].ref].smoothpos;
		Vector3 diffY = m_nodes[m_locators[i].ny].smoothpos - m_nodes[m_locators[i].ref].smoothpos;

		mat.SetColumn(0, diffX);
		mat.SetColumn(1, diffY);
		mat.SetColumn(2, approx_normalise(diffX.crossProduct(diffY))); // Old version: mat.SetColumn(2, m_nodes[loc.nz].smoothpos-m_nodes[loc.ref].smoothpos);

		m_dst_pos[i] = mat * m_locators[i].coords + m_nodes[m_locators[i].ref].smoothpos - flexit_center;
		m_dst_normals[i] = approx_normalise(mat * m_src_normals[i]);
	}
#endif
}

#endif // #ifdef FLEX_COMPAT_v0_38_67

Vector3 FlexBody::flexitFinal()
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

	m_scene_node->setPosition(flexit_center);

	return flexit_center;
}

void FlexBody::reset()
{
	if (m_is_faulty) return;
	if (m_has_texture_blend)
	{
		for (int i=0; i<(int)m_vertex_count; i++) m_src_colors[i]=0x00000000;
		writeBlend();
	}
}

void FlexBody::writeBlend()
{
	if (!m_is_enabled) return;
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
	if (!m_is_enabled) return;
	bool changed = false;
	for (int i=0; i<(int)m_vertex_count; i++)
	{
		node_t *nd = &m_nodes[m_locators[i].ref];
		ARGB col = m_src_colors[i];
		if (nd->contacted && !(col&0xFF000000))
		{
			m_src_colors[i]=col|0xFF000000;
			changed = true;
		}
		if ((nd->wetstate!=DRY) ^ ((col&0x000000FF)>0))
		{
			m_src_colors[i]=(col&0xFFFFFF00)+0x000000FF*(nd->wetstate!=DRY);
			changed = true;
		}
	}
	if (changed) writeBlend();
}


