/*
This source file is part of Rigs of Rods
Copyright 2005-2012 Pierre-Michel Ricordel
Copyright 2007-2012 Thomas Fischer

For more information, see http://www.rigsofrods.com/

Rigs of Rods is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License version 3, as
published by the Free Software Foundation.

Rigs of Rods is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Rigs of Rods.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "FlexBody.h"

#include "ApproxMath.h"
#include "MaterialReplacer.h"
#include "ResourceBuffer.h"
#include "Skin.h"
#include "BeamData.h" // For node_t
#include "Timer.h"

// Uncomment for time stats in RoR.log
// #define FLEXBODY_LOG_LOADING_TIMES

#ifdef FLEXBODY_LOG_LOADING_TIMES
#   define TIMER_CREATE() \
        PrecisionTimer loading_timer;

#   define TIMER_SNAPSHOT(VAR_NAME) \
        double VAR_NAME = loading_timer.elapsed(); loading_timer.restart();
#else
#   define TIMER_CREATE()
#   define TIMER_SNAPSHOT(VAR_NAME)
#endif

using namespace Ogre;

FlexBody::FlexBody(
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
	bool forceNoShadows, 
	MaterialReplacer *material_replacer
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
{
    TIMER_CREATE();

	m_nodes[m_node_center].iIsSkin=true;
	m_nodes[m_node_x].iIsSkin=true;
	m_nodes[m_node_y].iIsSkin=true;

    Ogre::Vector3* vertices = nullptr;

	Vector3 normal = Vector3::UNIT_Y;
	Vector3 position = Vector3::ZERO;
	Quaternion orientation = Quaternion::ZERO;
	if (ref >= 0)
	{
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
	} else
	{
		// special case!
		normal = Vector3::UNIT_Y;
		position=m_nodes[0].smoothpos+offset;
		orientation = rot;
	}

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
	// build new unique mesh name
	char uname_mesh[256] = {};
	strncpy(uname_mesh, uname.c_str(), 250);
	uname_mesh[250] = '\0';
	strcat(uname_mesh, "_mesh");
	MeshPtr mesh = MeshManager::getSingleton().load(meshname, groupname);
    TIMER_SNAPSHOT(stat_mesh_loaded_time);

	MeshPtr newmesh = mesh->clone(uname_mesh);
	
	// now find possible LODs
 	String basename, ext;
	StringUtil::splitBaseFilename(String(meshname), basename, ext);
	for (int i=0; i<4;i++)
	{
		String fn = basename + "_" + TOSTRING(i) + ".mesh";
		bool exists = ResourceGroupManager::getSingleton().resourceExistsInAnyGroup(fn);
		if (!exists) continue;

		float distance = 3;
		if (i == 1) distance = 20;
		if (i == 2) distance = 50;
		if (i == 3) distance = 200;
		newmesh->createManualLodLevel(distance, fn);
	}

	Entity *ent = gEnv->sceneManager->createEntity(uname, uname_mesh);
	MaterialFunctionMapper::replaceSimpleMeshMaterials(ent, ColourValue(0.5, 0.5, 1));
	if (material_function_mapper) material_function_mapper->replaceMeshMaterials(ent);
	if (material_replacer) material_replacer->replaceMeshMaterials(ent);
	if (usedSkin) usedSkin->replaceMeshMaterials(ent);
    TIMER_SNAPSHOT(stat_mesh_ready_time);

	m_mesh=ent->getMesh();

	//determine if we have texture coordinates everywhere
	if (m_mesh->sharedVertexData && m_mesh->sharedVertexData->vertexDeclaration->findElementBySemantic(VES_TEXTURE_COORDINATES)==0)
    {
        m_has_texture=false;
    }
    for (int i=0; i<m_mesh->getNumSubMeshes(); i++) 
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
    for (int i=0; i<m_mesh->getNumSubMeshes(); i++) 
    { 
        if (!m_mesh->getSubMesh(i)->useSharedVertices && m_mesh->getSubMesh(i)->vertexData->vertexDeclaration->findElementBySemantic(VES_NORMAL)==0) 
        { 
            havenormal=false; 
        } 
    }
	if (!havenormal) 
    {
        LOG("FLEXBODY Error: at least one part of this mesh does not have normal vectors, export your mesh with normal vectors! Disabling flexbody");
    }
    TIMER_SNAPSHOT(stat_mesh_scanned_time);

	//create optimal VertexDeclaration
	VertexDeclaration* optimalVD=HardwareBufferManager::getSingleton().createVertexDeclaration();
	optimalVD->addElement(0, 0, VET_FLOAT3, VES_POSITION);
	optimalVD->addElement(1, 0, VET_FLOAT3, VES_NORMAL);
	if (m_has_texture_blend) optimalVD->addElement(2, 0, VET_COLOUR_ARGB, VES_DIFFUSE);
	if (m_has_texture) optimalVD->addElement(3, 0, VET_FLOAT2, VES_TEXTURE_COORDINATES);
	optimalVD->sort();
	optimalVD->closeGapsInSource();

	BufferUsageList optimalBufferUsages;
	for (size_t u = 0; u <= optimalVD->getMaxSource(); ++u) optimalBufferUsages.push_back(HardwareBuffer::HBU_DYNAMIC_WRITE_ONLY_DISCARDABLE);

	//print mesh information
	//LOG("FLEXBODY Printing input mesh informations:");
	//printMeshInfo(ent->getMesh().getPointer());

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
		for (int i=0; i<m_mesh->getNumSubMeshes(); i++) if (!m_mesh->getSubMesh(i)->useSharedVertices)
		{
			if (m_mesh->getSubMesh(i)->vertexData->vertexDeclaration->findElementBySemantic(VES_DIFFUSE)==0)
			{
				//add buffer
				int index=m_mesh->getSubMesh(i)->vertexData->vertexDeclaration->getMaxSource()+1;
				m_mesh->getSubMesh(i)->vertexData->vertexDeclaration->addElement(index, 0, VET_COLOUR_ARGB, VES_DIFFUSE);
				m_mesh->getSubMesh(i)->vertexData->vertexDeclaration->sort();
				index=m_mesh->getSubMesh(i)->vertexData->vertexDeclaration->findElementBySemantic(VES_DIFFUSE)->getSource();
				HardwareVertexBufferSharedPtr vbuf=HardwareBufferManager::getSingleton().createVertexBuffer(VertexElement::getTypeSize(VET_COLOUR_ARGB), m_mesh->getSubMesh(i)->vertexData->vertexCount, HardwareBuffer::HBU_DYNAMIC_WRITE_ONLY_DISCARDABLE);
				m_mesh->getSubMesh(i)->vertexData->vertexBufferBinding->setBinding(index, vbuf);
			}
		}
	}
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

	//print mesh information
	//LOG("FLEXBODY Printing modififed mesh informations:");
	//printMeshInfo(ent->getMesh().getPointer());

	//get the buffers
	//getMeshInformation(ent->getMesh().getPointer(),m_vertex_count,vertices,index_count,indices, position, orientation, Vector3(1,1,1));

	//getting vertex counts
	m_vertex_count=0;
	m_uses_shared_vertex_data=false;
	m_num_submesh_vbufs=0;
	if (m_mesh->sharedVertexData)
	{
		m_vertex_count+=m_mesh->sharedVertexData->vertexCount;
		m_uses_shared_vertex_data=true;
	}
	for (int i=0; i<m_mesh->getNumSubMeshes(); i++)
	{
		if (!m_mesh->getSubMesh(i)->useSharedVertices)
		{
			m_vertex_count+=m_mesh->getSubMesh(i)->vertexData->vertexCount;
			m_num_submesh_vbufs++;
		}
	}

	LOG("FLEXBODY Vertices in mesh "+String(meshname)+": "+ TOSTRING(m_vertex_count));
	//LOG("Triangles in mesh: %u",index_count / 3);

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
	int cursubmesh=0;
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
	for (int i=0; i<m_mesh->getNumSubMeshes(); i++) if (!m_mesh->getSubMesh(i)->useSharedVertices)
	{
		m_submesh_vbufs_vertex_counts[cursubmesh]=(int)m_mesh->getSubMesh(i)->vertexData->vertexCount;
		//vertices
		int source=m_mesh->getSubMesh(i)->vertexData->vertexDeclaration->findElementBySemantic(VES_POSITION)->getSource();
		m_submesh_vbufs_pos[cursubmesh]=m_mesh->getSubMesh(i)->vertexData->vertexBufferBinding->getBuffer(source);
		m_submesh_vbufs_pos[cursubmesh]->readData(0, m_mesh->getSubMesh(i)->vertexData->vertexCount*sizeof(Vector3), (void*)vpt);
		vpt+=m_mesh->getSubMesh(i)->vertexData->vertexCount;
		//normals
		source=m_mesh->getSubMesh(i)->vertexData->vertexDeclaration->findElementBySemantic(VES_NORMAL)->getSource();
		m_submesh_vbufs_norm[cursubmesh]=m_mesh->getSubMesh(i)->vertexData->vertexBufferBinding->getBuffer(source);
		m_submesh_vbufs_norm[cursubmesh]->readData(0, m_mesh->getSubMesh(i)->vertexData->vertexCount*sizeof(Vector3), (void*)npt);
		npt+=m_mesh->getSubMesh(i)->vertexData->vertexCount;
		//colors
		if (m_has_texture_blend)
		{
			source=m_mesh->getSubMesh(i)->vertexData->vertexDeclaration->findElementBySemantic(VES_DIFFUSE)->getSource();
			m_submesh_vbufs_color[cursubmesh]=m_mesh->getSubMesh(i)->vertexData->vertexBufferBinding->getBuffer(source);
			m_submesh_vbufs_color[cursubmesh]->writeData(0, m_mesh->getSubMesh(i)->vertexData->vertexCount*sizeof(ARGB), (void*)m_src_colors);
		}
		cursubmesh++;
	}
    TIMER_SNAPSHOT(stat_manual_buffers_created_time);

	//transform
	for (int i=0; i<(int)m_vertex_count; i++)
	{
		vertices[i]=(orientation*vertices[i])+position;
	}
    TIMER_SNAPSHOT(stat_transformed_time);

	m_locators=(Locator_t*)malloc(sizeof(Locator_t)*m_vertex_count);
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
    TIMER_SNAPSHOT(stat_located_time);

	//adjusting bounds
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

	LOG("FLEXBODY show mesh");
	//okay, show the mesh now
	m_scene_node=gEnv->sceneManager->getRootSceneNode()->createChildSceneNode();
	m_scene_node->attachObject(ent);
	m_scene_node->setPosition(position);

    TIMER_SNAPSHOT(stat_showmesh_time);

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

    TIMER_SNAPSHOT(stat_euclidean2_time);

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


