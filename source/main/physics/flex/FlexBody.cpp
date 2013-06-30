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

using namespace Ogre;

FlexBody::FlexBody(node_t *nds, int numnds, char* meshname, char* uname, int ref, int nx, int ny, Vector3 offset, Quaternion rot, char* setdef, MaterialFunctionMapper *mfm, Skin *usedSkin, bool enableShadows, MaterialReplacer *mr) :
	  cameramode(-2)	
	, coffset(offset)
	, cref(ref)
	, cx(nx)
	, cy(ny)
	, enabled(true)
	, faulty(false)
	, freenodeset(0)
	, hasblend(true)
	, hastangents(false)
	, mr(mr)
	, nodes(nds)
	, numnodes(numnds)
	, snode(0)
{
	nodes[cref].iIsSkin=true;
	nodes[cx].iIsSkin=true;
	nodes[cy].iIsSkin=true;

	hasshadows=(gEnv->sceneManager->getShadowTechnique()==SHADOWTYPE_STENCIL_MODULATIVE || gEnv->sceneManager->getShadowTechnique()==SHADOWTYPE_STENCIL_ADDITIVE);

	//parsing set definition
	char* pos=setdef;
	char* end=pos;
	char endwas='G';
	while (endwas!=0)
	{
		unsigned int val1, val2;
		end=pos;
		while (*end!='-' && *end!=',' && *end!=0) end++;
		endwas=*end;
		*end=0;
		val1=strtoul(pos, 0, 10);
		if (endwas=='-')
		{
			pos=end+1;
			end=pos;
			while (*end!=',' && *end!=0) end++;
			endwas=*end;
			*end=0;
			val2=strtoul(pos, 0, 10);
			addinterval(val1, val2);
		}
		else addinterval(val1, val1);
		pos=end+1;
	}

	/*
	// too verbose, removed
	for (int i=0; i < freenodeset; i++)
		LOG("FLEXBODY node interval "+TOSTRING(i)+": "+TOSTRING(nodeset[i].from)+"-"+TOSTRING(nodeset[i].to));
	*/

	Vector3 normal = Vector3::UNIT_Y;
	Vector3 position = Vector3::ZERO;
	Quaternion orientation = Quaternion::ZERO;
	if (ref >= 0)
	{
		Vector3 diffX = nodes[nx].smoothpos-nodes[ref].smoothpos;
		Vector3 diffY = nodes[ny].smoothpos-nodes[ref].smoothpos;

		normal = fast_normalise(diffY.crossProduct(diffX));

		// position
		position = nodes[ref].smoothpos + offset.x*diffX + offset.y*diffY;
		position = position + offset.z*normal;

		// orientation
		Vector3 refX = fast_normalise(diffX);
		Vector3 refY = refX.crossProduct(normal);
		orientation  = Quaternion(refX, normal, refY) * rot;
	} else
	{
		// special case!
		normal = Vector3::UNIT_Y;
		position=nodes[0].smoothpos+offset;
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
		faulty=true;
		return;
	}
	// build new unique mesh name
	char uname_mesh[256] = {};
	strncpy(uname_mesh, uname, 250);
	uname_mesh[250] = '\0';
	strcat(uname_mesh, "_mesh");
	MeshPtr mesh = MeshManager::getSingleton().load(meshname, groupname);
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
	if (mfm) mfm->replaceMeshMaterials(ent);
	if (mr) mr->replaceMeshMaterials(ent);
	if (usedSkin) usedSkin->replaceMeshMaterials(ent);
	//LOG("FLEXBODY unique mesh created: "+String(meshname)+" -> "+String(uname_mesh));

	msh=ent->getMesh();

	//determine if we have texture coordinates everywhere
	hastexture=true;
	if (msh->sharedVertexData && msh->sharedVertexData->vertexDeclaration->findElementBySemantic(VES_TEXTURE_COORDINATES)==0) hastexture=false;
	for (int i=0; i<msh->getNumSubMeshes(); i++) if (!msh->getSubMesh(i)->useSharedVertices && msh->getSubMesh(i)->vertexData->vertexDeclaration->findElementBySemantic(VES_TEXTURE_COORDINATES)==0) hastexture=false;
	if (!hastexture) LOG("FLEXBODY Warning: at least one part of this mesh does not have texture coordinates, switching off texturing!");
	if (!hastexture) {hastangents=false;hasblend=false;}; //we can't do this

	//detect the anomalous case where a mesh is exported without normal vectors
	bool havenormal=true;
	if (msh->sharedVertexData && msh->sharedVertexData->vertexDeclaration->findElementBySemantic(VES_NORMAL)==0) havenormal=false;
	for (int i=0; i<msh->getNumSubMeshes(); i++) if (!msh->getSubMesh(i)->useSharedVertices && msh->getSubMesh(i)->vertexData->vertexDeclaration->findElementBySemantic(VES_NORMAL)==0) havenormal=false;
	if (!havenormal) LOG("FLEXBODY Error: at least one part of this mesh does not have normal vectors, export your mesh with normal vectors! THIS WILL CRASH IN 3.2.1...");

	//create optimal VertexDeclaration
	VertexDeclaration* optimalVD=HardwareBufferManager::getSingleton().createVertexDeclaration();
	optimalVD->addElement(0, 0, VET_FLOAT3, VES_POSITION);
	optimalVD->addElement(1, 0, VET_FLOAT3, VES_NORMAL);
	if (hasblend) optimalVD->addElement(2, 0, VET_COLOUR_ARGB, VES_DIFFUSE);
	if (hastexture) optimalVD->addElement(3, 0, VET_FLOAT2, VES_TEXTURE_COORDINATES);
	if (hastangents) optimalVD->addElement(4, 0, VET_FLOAT3, VES_TANGENT);
	optimalVD->sort();
	optimalVD->closeGapsInSource();

	BufferUsageList optimalBufferUsages;
	for (size_t u = 0; u <= optimalVD->getMaxSource(); ++u) optimalBufferUsages.push_back(HardwareBuffer::HBU_DYNAMIC_WRITE_ONLY_DISCARDABLE);

	//print mesh information
	//LOG("FLEXBODY Printing input mesh informations:");
	//printMeshInfo(ent->getMesh().getPointer());

	//adding color buffers, well get the reference later
	if (hasblend)
	{
		if (msh->sharedVertexData)
		{
			if (msh->sharedVertexData->vertexDeclaration->findElementBySemantic(VES_DIFFUSE)==0)
			{
				//add buffer
				int index=msh->sharedVertexData->vertexDeclaration->getMaxSource()+1;
				msh->sharedVertexData->vertexDeclaration->addElement(index, 0, VET_COLOUR_ARGB, VES_DIFFUSE);
				msh->sharedVertexData->vertexDeclaration->sort();
				index=msh->sharedVertexData->vertexDeclaration->findElementBySemantic(VES_DIFFUSE)->getSource();
				HardwareVertexBufferSharedPtr vbuf=HardwareBufferManager::getSingleton().createVertexBuffer(VertexElement::getTypeSize(VET_COLOUR_ARGB), msh->sharedVertexData->vertexCount, HardwareBuffer::HBU_DYNAMIC_WRITE_ONLY_DISCARDABLE);
				msh->sharedVertexData->vertexBufferBinding->setBinding(index, vbuf);
			}
		}
		for (int i=0; i<msh->getNumSubMeshes(); i++) if (!msh->getSubMesh(i)->useSharedVertices)
		{
			if (msh->getSubMesh(i)->vertexData->vertexDeclaration->findElementBySemantic(VES_DIFFUSE)==0)
			{
				//add buffer
				int index=msh->getSubMesh(i)->vertexData->vertexDeclaration->getMaxSource()+1;
				msh->getSubMesh(i)->vertexData->vertexDeclaration->addElement(index, 0, VET_COLOUR_ARGB, VES_DIFFUSE);
				msh->getSubMesh(i)->vertexData->vertexDeclaration->sort();
				index=msh->getSubMesh(i)->vertexData->vertexDeclaration->findElementBySemantic(VES_DIFFUSE)->getSource();
				HardwareVertexBufferSharedPtr vbuf=HardwareBufferManager::getSingleton().createVertexBuffer(VertexElement::getTypeSize(VET_COLOUR_ARGB), msh->getSubMesh(i)->vertexData->vertexCount, HardwareBuffer::HBU_DYNAMIC_WRITE_ONLY_DISCARDABLE);
				msh->getSubMesh(i)->vertexData->vertexBufferBinding->setBinding(index, vbuf);
			}
		}
	}

	//tangents for envmapping
	if (hastangents)
	{
		LOG("FLEXBODY preparing for tangents");
		unsigned short srcTex, destTex;
		bool existing = msh->suggestTangentVectorBuildParams(VES_TANGENT, srcTex, destTex);
		if (!existing) msh->buildTangentVectors(VES_TANGENT, srcTex, destTex);
	}

	//reorg
	//LOG("FLEXBODY reorganizing buffers");
	if (msh->sharedVertexData)
	{
		msh->sharedVertexData->reorganiseBuffers(optimalVD, optimalBufferUsages);
		msh->sharedVertexData->removeUnusedBuffers();
		msh->sharedVertexData->closeGapsInBindings();
	}
    Mesh::SubMeshIterator smIt = msh->getSubMeshIterator();
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

	//print mesh information
	//LOG("FLEXBODY Printing modififed mesh informations:");
	//printMeshInfo(ent->getMesh().getPointer());

	//get the buffers
	//getMeshInformation(ent->getMesh().getPointer(),vertex_count,vertices,index_count,indices, position, orientation, Vector3(1,1,1));

	//getting vertex counts
	vertex_count=0;
	hasshared=false;
	numsubmeshbuf=0;
	if (msh->sharedVertexData)
	{
		vertex_count+=msh->sharedVertexData->vertexCount;
		hasshared=true;
	}
	for (int i=0; i<msh->getNumSubMeshes(); i++)
	{
		if (!msh->getSubMesh(i)->useSharedVertices)
		{
			vertex_count+=msh->getSubMesh(i)->vertexData->vertexCount;
			numsubmeshbuf++;
		}
	}

	LOG("FLEXBODY Vertices in mesh "+String(meshname)+": "+ TOSTRING(vertex_count));
	//LOG("Triangles in mesh: %u",index_count / 3);

	//getting buffers bindings and data
	if (numsubmeshbuf>0)
	{
		submeshnums=(int*)malloc(sizeof(int)*numsubmeshbuf);
		subnodecounts=(int*)malloc(sizeof(int)*numsubmeshbuf);
		//C++ is just dumb!
		//How can they manage to break such a fundamental programming mechanisms?
		//They invented the un-initializable and un-attribuable objects you can't allocate dynamically!
		//I'm sure they have a fancy way to do that but they won't pry my precious malloc() from my cold, dead hands! goddamit!
		//subpbufs=(HardwareVertexBufferSharedPtr*)malloc(sizeof(HardwareVertexBufferSharedPtr)*numsubmeshbuf);
		//subpbufs[0]=HardwareVertexBufferSharedPtr(); //crash!
		//subnbufs=(HardwareVertexBufferSharedPtr*)malloc(sizeof(HardwareVertexBufferSharedPtr)*numsubmeshbuf);
		//subnbufs[0]=HardwareVertexBufferSharedPtr(); //crash!
		if (numsubmeshbuf>=16) 	LOG("FLEXBODY You have more than 16 submeshes! Blame Bjarne for this crash.");
	}
	vertices=(Vector3*)malloc(sizeof(Vector3)*vertex_count);
	dstpos=(Vector3*)malloc(sizeof(Vector3)*vertex_count);
	srcnormals=(Vector3*)malloc(sizeof(Vector3)*vertex_count);
	dstnormals=(Vector3*)malloc(sizeof(Vector3)*vertex_count);
	if (hasblend)
	{
		srccolors=(ARGB*)malloc(sizeof(ARGB)*vertex_count);
		for (int i=0; i<(int)vertex_count; i++) srccolors[i]=0x00000000;
	}
	Vector3* vpt=vertices;
	Vector3* npt=srcnormals;
	int cursubmesh=0;
	if (msh->sharedVertexData)
	{
		sharedcount=(int)msh->sharedVertexData->vertexCount;
		//vertices
		int source=msh->sharedVertexData->vertexDeclaration->findElementBySemantic(VES_POSITION)->getSource();
		sharedpbuf=msh->sharedVertexData->vertexBufferBinding->getBuffer(source);
		sharedpbuf->readData(0, msh->sharedVertexData->vertexCount*sizeof(Vector3), (void*)vpt);
		vpt+=msh->sharedVertexData->vertexCount;
		//normals
		source=msh->sharedVertexData->vertexDeclaration->findElementBySemantic(VES_NORMAL)->getSource();
		sharednbuf=msh->sharedVertexData->vertexBufferBinding->getBuffer(source);
		sharednbuf->readData(0, msh->sharedVertexData->vertexCount*sizeof(Vector3), (void*)npt);
		npt+=msh->sharedVertexData->vertexCount;
		//colors
		if (hasblend)
		{
			source=msh->sharedVertexData->vertexDeclaration->findElementBySemantic(VES_DIFFUSE)->getSource();
			sharedcbuf=msh->sharedVertexData->vertexBufferBinding->getBuffer(source);
			sharedcbuf->writeData(0, msh->sharedVertexData->vertexCount*sizeof(ARGB), (void*)srccolors);
		}
	}
	for (int i=0; i<msh->getNumSubMeshes(); i++) if (!msh->getSubMesh(i)->useSharedVertices)
	{
		submeshnums[cursubmesh]=i;
		subnodecounts[cursubmesh]=(int)msh->getSubMesh(i)->vertexData->vertexCount;
		//vertices
		int source=msh->getSubMesh(i)->vertexData->vertexDeclaration->findElementBySemantic(VES_POSITION)->getSource();
		subpbufs[cursubmesh]=msh->getSubMesh(i)->vertexData->vertexBufferBinding->getBuffer(source);
		subpbufs[cursubmesh]->readData(0, msh->getSubMesh(i)->vertexData->vertexCount*sizeof(Vector3), (void*)vpt);
		vpt+=msh->getSubMesh(i)->vertexData->vertexCount;
		//normals
		source=msh->getSubMesh(i)->vertexData->vertexDeclaration->findElementBySemantic(VES_NORMAL)->getSource();
		subnbufs[cursubmesh]=msh->getSubMesh(i)->vertexData->vertexBufferBinding->getBuffer(source);
		subnbufs[cursubmesh]->readData(0, msh->getSubMesh(i)->vertexData->vertexCount*sizeof(Vector3), (void*)npt);
		npt+=msh->getSubMesh(i)->vertexData->vertexCount;
		//colors
		if (hasblend)
		{
			source=msh->getSubMesh(i)->vertexData->vertexDeclaration->findElementBySemantic(VES_DIFFUSE)->getSource();
			subcbufs[cursubmesh]=msh->getSubMesh(i)->vertexData->vertexBufferBinding->getBuffer(source);
			subcbufs[cursubmesh]->writeData(0, msh->getSubMesh(i)->vertexData->vertexCount*sizeof(ARGB), (void*)srccolors);
		}
		cursubmesh++;
	}

	//transform
	for (int i=0; i<(int)vertex_count; i++)
	{
		vertices[i]=(orientation*vertices[i])+position;
	}

	locs=(Locator_t*)malloc(sizeof(Locator_t)*vertex_count);
	for (int i=0; i<(int)vertex_count; i++)
	{
		//search nearest node as the local origin
		float mindist=1000000.0;
		int minnode=-1;
		for (int k=0; k<numnodes; k++)
		{
			//if (nodes[k].iswheel) continue;
			float dist = vertices[i].squaredDistance(nodes[k].smoothpos);
			if (dist < mindist && isinset(k))
			{
				mindist = dist;
				minnode = k;
			}
		}
		if (minnode==-1) LOG("FLEXBODY ERROR on mesh "+String(meshname)+": REF node not found");
		locs[i].ref=minnode;
		nodes[minnode].iIsSkin=true;

//	LOG("FLEXBODY distance to "+TOSTRING(minnode)+" "+TOSTRING(mindist));

		//search the second nearest node as the X vector
		mindist=1000000.0;
		minnode=-1;
		for (int k=0; k<numnodes; k++)
		{
			//if (nodes[k].iswheel) continue;
			if (k==locs[i].ref) continue;
			float dist = vertices[i].squaredDistance(nodes[k].smoothpos);
			if (dist < mindist && isinset(k))
			{
				mindist = dist;
				minnode = k;
			}
		}
		if (minnode==-1) LOG("FLEXBODY ERROR on mesh "+String(meshname)+": VX node not found");
		locs[i].nx=minnode;
		nodes[minnode].iIsSkin=true;

		//search another close, orthogonal node as the Y vector
		mindist=1000000.0;
		minnode=-1;
		Vector3 vx = fast_normalise(nodes[locs[i].nx].smoothpos - nodes[locs[i].ref].smoothpos);
		for (int k=0; k<numnodes; k++)
		{
			//if (nodes[k].iswheel) continue;
			if (k==locs[i].ref) continue;
			if (k==locs[i].nx) continue;
			float dist = vertices[i].squaredDistance(nodes[k].smoothpos);
			if (dist < mindist && isinset(k))
			{
				Vector3 vt = fast_normalise(nodes[k].smoothpos - nodes[locs[i].ref].smoothpos);
				float cost = vx.dotProduct(vt);
				if (cost>0.707 || cost<-0.707) continue; //rejection, fails the orthogonality criterion (+-45 degree)
				mindist = dist;
				minnode = k;
			}
		}
		if (minnode==-1) LOG("FLEXBODY ERROR on mesh "+String(meshname)+": VY node not found");
		locs[i].ny=minnode;
		nodes[minnode].iIsSkin=true;

#if 0
		//search the final close, orthogonal node as the Z vector
		mindist=1000000.0;
		minnode=-1;
		Vector3 vy=nodes[locs[i].ny].smoothpos-nodes[locs[i].ref].smoothpos;
		vy.normalise();
		for (int k=0; k<numnodes; k++)
		{
			//if (nodes[k].iswheel) continue;
			if (k==locs[i].ref) continue;
			if (k==locs[i].nx) continue;
			if (k==locs[i].ny) continue;
			float dist=vertices[i].squaredDistance(nodes[k].smoothpos);
			if (dist < mindist)
			{
				Vector3 vt=approx_normalise(nodes[k].smoothpos-nodes[locs[i].ref].smoothpos);
				float cost=vx.dotProduct(vt);
				if (cost>0.707 || cost<-0.707) continue; //rejection, fails the orthogonality criterion (+-45 degree)
				cost=vy.dotProduct(vt);
				if (cost>0.707 || cost<-0.707) continue; //rejection, fails the orthogonality criterion (+-45 degree)
				mindist = dist;
				minnode = k;
			}
		}
		if (minnode==-1) LOG("FLEXBODY ERROR on mesh "+String(meshname)+": VZ node not found");
		locs[i].nz=minnode;

		//rright, check orientation
		Vector3 xyn=vx.crossProduct(vy);
		if (xyn.dotProduct(nodes[locs[i].nz].smoothpos-nodes[locs[i].ref].smoothpos)<0)
		{
			//the base is messed up
			int t=locs[i].nz;
			locs[i].nz=locs[i].ny;
			locs[i].ny=t;
		}
#endif // 0

		// If something unexpected happens here, then
		// replace fast_normalise(a) with a.normalisedCopy()

		Matrix3 mat;
		Vector3 diffX = nodes[locs[i].nx].smoothpos-nodes[locs[i].ref].smoothpos;
		Vector3 diffY = nodes[locs[i].ny].smoothpos-nodes[locs[i].ref].smoothpos;

		mat.SetColumn(0, diffX);
		mat.SetColumn(1, diffY);
		mat.SetColumn(2, fast_normalise(diffX.crossProduct(diffY))); // Old version: mat.SetColumn(2, nodes[loc.nz].smoothpos-nodes[loc.ref].smoothpos);

		mat = mat.Inverse();

		//compute coordinates in the newly formed Euclidean basis
		locs[i].coords= mat * (vertices[i] - nodes[locs[i].ref].smoothpos);

		// that's it!
	}

	//shadow
	if (hasshadows)
	{
		LOG("FLEXBODY preparing for shadow volume");
		msh->prepareForShadowVolume(); // we do this always so we have only one data structure format to manage
		msh->buildEdgeList();
	}

	//adjusting bounds
	AxisAlignedBox aab=msh->getBounds();
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
	msh->_setBounds(aab, true);

	LOG("FLEXBODY show mesh");
	//okay, show the mesh now
	snode=gEnv->sceneManager->getRootSceneNode()->createChildSceneNode();
	snode->attachObject(ent);
	snode->setPosition(position);
	//ent->setCastShadows(enableShadows);

#if 0
	// XXX TODO: fix 1.7 LODs
	if (enable_truck_lod)
	{
		String lodstr = "FLEXBODY LODs: ";
		for (int i=0;i<msh->getNumLodLevels();i++)
		{
			if (i) lodstr += ", ";
			lodstr += TOSTRING(Real(sqrt(msh->getLodLevel(i).fromDepthSquared))) + "m";

			if (msh->getLodLevel(i).edgeData)
			{
				lodstr += "(" + TOSTRING(msh->getLodLevel(i).edgeData->triangles.size()) + " triangles)";
			} else
			{
				if (msh->getEdgeList(i))
					lodstr += "(" + TOSTRING(msh->getEdgeList(i)->triangles.size()) +" triangles)";
			}
		}
		LOG(lodstr);
	}
#endif //0

	// If something unexpected happens here, then
	// replace fast_normalise(a) with a.normalisedCopy()
	for (int i=0; i<(int)vertex_count; i++)
	{
		Matrix3 mat;
		Vector3 diffX = nodes[locs[i].nx].smoothpos-nodes[locs[i].ref].smoothpos;
		Vector3 diffY = nodes[locs[i].ny].smoothpos-nodes[locs[i].ref].smoothpos;

		mat.SetColumn(0, diffX);
		mat.SetColumn(1, diffY);
		mat.SetColumn(2, fast_normalise(diffX.crossProduct(diffY))); // Old version: mat.SetColumn(2, nodes[loc.nz].smoothpos-nodes[loc.ref].smoothpos);

		mat = mat.Inverse();

		// compute coordinates in the Euclidean basis
		srcnormals[i] = mat*(orientation * srcnormals[i]);
	}

	LOG("FLEXBODY ready");
}

void FlexBody::setEnabled(bool e)
{
	if (faulty) return;
	setVisible(e);
	enabled = e;
}

void FlexBody::setVisible(bool visible)
{
	if (faulty) return;
	if (!enabled) return;
	if (snode)
		snode->setVisible(visible);
}

void FlexBody::printMeshInfo(Mesh* mesh)
{
	if (faulty) return;
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

void FlexBody::addinterval(int from, int to)
{
	if (freenodeset < MAX_SET_INTERVALS)
	{
		nodeset[freenodeset].from=from;
		nodeset[freenodeset].to=to;
		freenodeset++;
	}
}

bool FlexBody::isinset(int n)
{
	for (int i=0; i < freenodeset; i++)
	{
		if (n >= nodeset[i].from && n <= nodeset[i].to)
			return true;
	}
	return false;
}

bool FlexBody::flexitPrepare(Beam* b)
{
	if (faulty) return false;
	if (!enabled) return false;
	if (hasblend) updateBlend();
	
	// compute the local center
	Ogre::Vector3 flexit_normal;

	if (cref >= 0)
	{
		Vector3 diffX = nodes[cx].smoothpos - nodes[cref].smoothpos;
		Vector3 diffY = nodes[cy].smoothpos - nodes[cref].smoothpos;
		flexit_normal = diffY.crossProduct(diffX).normalisedCopy();

		flexit_center = nodes[cref].smoothpos + coffset.x*diffX + coffset.y*diffY;
		flexit_center += coffset.z*flexit_normal;
	} else
	{
		flexit_normal = Vector3::UNIT_Y;
		flexit_center = nodes[0].smoothpos;
	}

	return Flexable::flexitPrepare(b);
}

void FlexBody::flexitCompute()
{
	// If something unexpected happens here, then
	// replace approx_normalise(a) with a.normalisedCopy()
	for (int i=0; i<(int)vertex_count; i++)
	{
		Vector3 diffX = nodes[locs[i].nx].smoothpos - nodes[locs[i].ref].smoothpos;
		Vector3 diffY = nodes[locs[i].ny].smoothpos - nodes[locs[i].ref].smoothpos;
		Vector3 nCross = approx_normalise(diffX.crossProduct(diffY));

		dstpos[i].x = diffX.x * locs[i].coords.x + diffY.x * locs[i].coords.y + nCross.x * locs[i].coords.z;
		dstpos[i].y = diffX.y * locs[i].coords.x + diffY.y * locs[i].coords.y + nCross.y * locs[i].coords.z;
		dstpos[i].z = diffX.z * locs[i].coords.x + diffY.z * locs[i].coords.y + nCross.z * locs[i].coords.z;

		dstpos[i] += nodes[locs[i].ref].smoothpos - flexit_center;

		dstnormals[i].x = diffX.x * srcnormals[i].x + diffY.x * srcnormals[i].y + nCross.x * srcnormals[i].z;
		dstnormals[i].y = diffX.y * srcnormals[i].x + diffY.y * srcnormals[i].y + nCross.y * srcnormals[i].z;
		dstnormals[i].z = diffX.z * srcnormals[i].x + diffY.z * srcnormals[i].y + nCross.z * srcnormals[i].z;

		dstnormals[i] = approx_normalise(dstnormals[i]);
	}
#if 0
	for (int i=0; i<(int)vertex_count; i++)
	{
		Matrix3 mat;
		Vector3 diffX = nodes[locs[i].nx].smoothpos - nodes[locs[i].ref].smoothpos;
		Vector3 diffY = nodes[locs[i].ny].smoothpos - nodes[locs[i].ref].smoothpos;

		mat.SetColumn(0, diffX);
		mat.SetColumn(1, diffY);
		mat.SetColumn(2, approx_normalise(diffX.crossProduct(diffY))); // Old version: mat.SetColumn(2, nodes[loc.nz].smoothpos-nodes[loc.ref].smoothpos);

		dstpos[i] = mat * locs[i].coords + nodes[locs[i].ref].smoothpos - flexit_center;
		dstnormals[i] = approx_normalise(mat * srcnormals[i]);
	}
#endif
}

Vector3 FlexBody::flexitFinal()
{
	Vector3 *ppt = dstpos;
	Vector3 *npt = dstnormals;
	if (hasshared)
	{
		sharedpbuf->writeData(0, sharedcount*sizeof(Vector3), ppt, true);
		ppt += sharedcount;
		sharednbuf->writeData(0, sharedcount*sizeof(Vector3), npt, true);
		npt += sharedcount;
	}
	for (int i=0; i<numsubmeshbuf; i++)
	{
		subpbufs[i]->writeData(0, subnodecounts[i]*sizeof(Vector3), ppt, true);
		ppt += subnodecounts[i];
		subnbufs[i]->writeData(0, subnodecounts[i]*sizeof(Vector3), npt, true);
		npt += subnodecounts[i];
	}

	snode->setPosition(flexit_center);

	return flexit_center;
}

void FlexBody::reset()
{
	if (faulty) return;
	if (hasblend)
	{
		for (int i=0; i<(int)vertex_count; i++) srccolors[i]=0x00000000;
		writeBlend();
	}
}

void FlexBody::writeBlend()
{
	if (!enabled) return;
	if (!hasblend) return;
	ARGB *cpt = srccolors;
	if (hasshared)
	{
		sharedcbuf->writeData(0, sharedcount*sizeof(ARGB), (void*)cpt, true);
		cpt+=sharedcount;
	}
	for (int i=0; i<numsubmeshbuf; i++)
	{
		subcbufs[i]->writeData(0, subnodecounts[i]*sizeof(ARGB), (void*)cpt, true);
		cpt+=subnodecounts[i];
	}
}

void FlexBody::updateBlend() //so easy!
{
	if (!enabled) return;
	bool changed = false;
	for (int i=0; i<(int)vertex_count; i++)
	{
		node_t *nd = &nodes[locs[i].ref];
		ARGB col = srccolors[i];
		if (nd->contacted && !(col&0xFF000000))
		{
			srccolors[i]=col|0xFF000000;
			changed = true;
		}
		if ((nd->wetstate!=DRY) ^ ((col&0x000000FF)>0))
		{
			srccolors[i]=(col&0xFFFFFF00)+0x000000FF*(nd->wetstate!=DRY);
			changed = true;
		}
	}
	if (changed) writeBlend();
}
