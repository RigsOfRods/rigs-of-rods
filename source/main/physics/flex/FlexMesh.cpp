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
#include "FlexMesh.h"

#include "ApproxMath.h"
#include "Ogre.h"
#include "ResourceBuffer.h"

using namespace Ogre;

FlexMesh::FlexMesh(char* name, node_t *nds, int n1, int n2, int nstart, int nrays, char* texface, char* texband, bool rimmed, float rimratio) :
	  is_rimmed(rimmed)
	, nbrays(nrays)
	, nodes(nds)
	, rim_ratio(rimratio)
{
	/// Create the mesh via the MeshManager
	msh = MeshManager::getSingleton().createManual(name, ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,new ResourceBuffer());

	/// Create submeshes
	subface = msh->createSubMesh();
	subband = msh->createSubMesh();

	//materials
	subface->setMaterialName(texface);
	subband->setMaterialName(texband);

	/// Define the vertices
	nVertices = 4*nrays+2;
	if (is_rimmed) nVertices+=2*nrays;
	vbufCount = (2*3+2)*nVertices;
	vertices=(float*)malloc(vbufCount*sizeof(float));
	//shadow
	shadownorvertices=(float*)malloc(nVertices*(3+2)*sizeof(float));
	shadowposvertices=(float*)malloc(nVertices*3*2*sizeof(float));
	nodeIDs=(int*)malloc(nVertices*sizeof(int));

	//define node ids
	nodeIDs[0]=n1;
	nodeIDs[1]=n2;
	int i;
	for (i=0; i<nrays; i++)
	{
		//face
		nodeIDs[2+i*2]=nstart+i*2;
		nodeIDs[2+i*2+1]=nstart+i*2+1;
		if (is_rimmed)
		{
			//band
			nodeIDs[2+2*nrays+i*2]=nstart+2*nrays+i*2;
			nodeIDs[2+2*nrays+i*2+1]=nstart+2*nrays+i*2+1;
			//face2 (outer)
			nodeIDs[2+4*nrays+i*2]=nstart+2*nrays+i*2;
			nodeIDs[2+4*nrays+i*2+1]=nstart+2*nrays+i*2+1;
		} else
		{
			//band
			nodeIDs[2+2*nrays+i*2]=nstart+i*2;
			nodeIDs[2+2*nrays+i*2+1]=nstart+i*2+1;
		}
	}
	//color fix to remove
//		for (i=0; i<(int)nVertices; i++)
//		{
//			covertices[i].color=Vector3(0.0, 0.0, 0.0);
//		};
	//textures coordinates
	covertices[0].texcoord=Vector2(0.5, 0.5);
	covertices[1].texcoord=Vector2(0.5, 0.5);
	for (i=0; i<nrays; i++)
	{
		//band
		covertices[2+2*nrays+(i/2)*4].texcoord=Vector2(0.0, 0.0);
		covertices[2+2*nrays+(i/2)*4+1].texcoord=Vector2(0.0, 1.0);
		covertices[2+2*nrays+(i/2)*4+2].texcoord=Vector2(1.0, 0.0);
		covertices[2+2*nrays+(i/2)*4+3].texcoord=Vector2(1.0, 1.0);
		//face
		if (is_rimmed)
		{
			covertices[2+i*2].texcoord=Vector2(0.5+0.5*rim_ratio*sin((float)i*2.0*3.14159/nrays), 0.5+0.5*rim_ratio*cos((float)i*2.0*3.14159/nrays));
			covertices[2+i*2+1].texcoord=covertices[2+i*2].texcoord;
			covertices[2+4*nrays+i*2].texcoord=Vector2(0.5+0.5*sin(((float)i+0.5)*2.0*3.14159/nrays), 0.5+0.5*cos(((float)i+0.5)*2.0*3.14159/nrays));
			covertices[2+4*nrays+i*2+1].texcoord=covertices[2+4*nrays+i*2].texcoord;
		} else
		{
			covertices[2+i*2].texcoord=Vector2(0.5+0.5*sin(i*2.0*3.14159/nrays), 0.5+0.5*cos(i*2.0*3.14159/nrays));
			covertices[2+i*2+1].texcoord=covertices[2+i*2].texcoord;
		}
	}

	/// Define triangles
	/// The values in this table refer to vertices in the above table
	bandibufCount = 3*2*nrays;
	faceibufCount = 3*2*nrays;
	if (is_rimmed) faceibufCount=faceibufCount*3;
	facefaces=(unsigned short*)malloc(faceibufCount*sizeof(unsigned short));
	bandfaces=(unsigned short*)malloc(bandibufCount*sizeof(unsigned short));
	for (i=0; i<nrays; i++)
	{
		//wheel sides
		facefaces[3*(i*2)]=0;   facefaces[3*(i*2)+1]=2+i*2;     facefaces[3*(i*2)+2]=2+((i+1)%nrays)*2;
		facefaces[3*(i*2+1)]=1; facefaces[3*(i*2+1)+2]=2+i*2+1; facefaces[3*(i*2+1)+1]=2+((i+1)%nrays)*2+1;
		if (is_rimmed)
		{
			facefaces[3*(i*4+0+2*nrays)]=2+i*2; facefaces[3*(i*4+0+2*nrays)+1]=2+4*nrays+i*2;             facefaces[3*(i*4+0+2*nrays)+2]=2+((i+1)%nrays)*2;
			facefaces[3*(i*4+1+2*nrays)]=2+4*nrays+i*2; facefaces[3*(i*4+1+2*nrays)+1]=2+4*nrays+((i+1)%nrays)*2; facefaces[3*(i*4+1+2*nrays)+2]=2+((i+1)%nrays)*2;
			facefaces[3*(i*4+2+2*nrays)]=2+i*2+1; facefaces[3*(i*4+2+2*nrays)+2]=2+4*nrays+i*2+1;             facefaces[3*(i*4+2+2*nrays)+1]=2+((i+1)%nrays)*2+1;
			facefaces[3*(i*4+3+2*nrays)]=2+4*nrays+i*2+1; facefaces[3*(i*4+3+2*nrays)+2]=2+4*nrays+((i+1)%nrays)*2+1; facefaces[3*(i*4+3+2*nrays)+1]=2+((i+1)%nrays)*2+1;
		}
		//wheel band
//			bandfaces[3*(i*2)]=2+2*nrays+i*2; bandfaces[3*(i*2)+1]=2+2*nrays+i*2+1; bandfaces[3*(i*2)+2]=2+2*nrays+((i+1)%nrays)*2+1;
//			bandfaces[3*(i*2+1)]=2+2*nrays+((i+1)%nrays)*2+1; bandfaces[3*(i*2+1)+2]=2+2*nrays+i*2; bandfaces[3*(i*2+1)+1]=2+2*nrays+((i+1)%nrays)*2;
		bandfaces[3*(i*2)]=2+2*nrays+i*2; bandfaces[3*(i*2)+1]=2+2*nrays+i*2+1; bandfaces[3*(i*2)+2]=2+2*nrays+((i+1)%nrays)*2;
		bandfaces[3*(i*2+1)]=2+2*nrays+((i+1)%nrays)*2; bandfaces[3*(i*2+1)+2]=2+2*nrays+((i+1)%nrays)*2+1; bandfaces[3*(i*2+1)+1]=2+2*nrays+i*2+1;
	}

	//update coords
	updateVertices();

	/// Create vertex data structure for 8 vertices shared between submeshes
	msh->sharedVertexData = new VertexData();
	msh->sharedVertexData->vertexCount = nVertices;

	/// Create declaration (memory format) of vertex data
	decl = msh->sharedVertexData->vertexDeclaration;
	size_t offset = 0;
	decl->addElement(0, offset, VET_FLOAT3, VES_POSITION);
	offset += VertexElement::getTypeSize(VET_FLOAT3);
	decl->addElement(0, offset, VET_FLOAT3, VES_NORMAL);
	offset += VertexElement::getTypeSize(VET_FLOAT3);
//        decl->addElement(0, offset, VET_FLOAT3, VES_DIFFUSE);
//        offset += VertexElement::getTypeSize(VET_FLOAT3);
	decl->addElement(0, offset, VET_FLOAT2, VES_TEXTURE_COORDINATES, 0);
	offset += VertexElement::getTypeSize(VET_FLOAT2);

	/// Allocate vertex buffer of the requested number of vertices (vertexCount)
	/// and bytes per vertex (offset)
	vbuf =
		HardwareBufferManager::getSingleton().createVertexBuffer(
			offset, msh->sharedVertexData->vertexCount, HardwareBuffer::HBU_DYNAMIC_WRITE_ONLY_DISCARDABLE);

	/// Upload the vertex data to the card
	vbuf->writeData(0, vbuf->getSizeInBytes(), vertices, true);

	/// Set vertex buffer binding so buffer 0 is bound to our vertex buffer
	VertexBufferBinding* bind = msh->sharedVertexData->vertexBufferBinding;
	bind->setBinding(0, vbuf);

	//for the face
	/// Allocate index buffer of the requested number of vertices (ibufCount)
	HardwareIndexBufferSharedPtr faceibuf = HardwareBufferManager::getSingleton().
		createIndexBuffer(
			HardwareIndexBuffer::IT_16BIT,
			faceibufCount,
			HardwareBuffer::HBU_STATIC_WRITE_ONLY);

	/// Upload the index data to the card
	faceibuf->writeData(0, faceibuf->getSizeInBytes(), facefaces, true);

	/// Set parameters of the submesh
	subface->useSharedVertices = true;
	subface->indexData->indexBuffer = faceibuf;
	subface->indexData->indexCount = faceibufCount;
	subface->indexData->indexStart = 0;

	//for the band
	/// Allocate index buffer of the requested number of vertices (ibufCount)
	HardwareIndexBufferSharedPtr bandibuf = HardwareBufferManager::getSingleton().
		createIndexBuffer(
			HardwareIndexBuffer::IT_16BIT,
			bandibufCount,
			HardwareBuffer::HBU_STATIC_WRITE_ONLY);

	/// Upload the index data to the card
	bandibuf->writeData(0, bandibuf->getSizeInBytes(), bandfaces, true);

	/// Set parameters of the submesh
	subband->useSharedVertices = true;
	subband->indexData->indexBuffer = bandibuf;
	subband->indexData->indexCount = bandibufCount;
	subband->indexData->indexStart = 0;

	/// Set bounding information (for culling)
	msh->_setBounds(AxisAlignedBox(-1,-1,0,1,1,0), true);
	//msh->_setBoundingSphereRadius(Math::Sqrt(1*1+1*1));

		/// Notify Mesh object that it has been loaded
	msh->buildEdgeList();
	//msh->buildTangentVectors();
	/*unsigned short src, dest;
	if (!msh->suggestTangentVectorBuildParams(src, dest))
	{
		msh->buildTangentVectors(src, dest);
	}
	*/

	msh->load();
	//msh->touch();
	//        msh->load();

	//msh->buildEdgeList();
}

Vector3 FlexMesh::updateVertices()
{
	Vector3 center = (nodes[nodeIDs[0]].smoothpos + nodes[nodeIDs[1]].smoothpos) / 2.0;

	//optimization possible here : just copy bands on face

	covertices[0].vertex=nodes[nodeIDs[0]].smoothpos-center;
	//normals
	covertices[0].normal=approx_normalise(nodes[nodeIDs[0]].smoothpos-nodes[nodeIDs[1]].smoothpos);

	covertices[1].vertex=nodes[nodeIDs[1]].smoothpos-center;
	//normals
	covertices[1].normal=-covertices[0].normal;

	for (int i=0; i<nbrays*2; i++)
	{
		covertices[2+i].vertex=nodes[nodeIDs[2+i]].smoothpos-center;
		//normals
		if ((i%2)==0)
		{
			covertices[2+i].normal=approx_normalise(nodes[nodeIDs[0]].smoothpos-nodes[nodeIDs[1]].smoothpos);
		} else
		{
			covertices[2+i].normal=-covertices[2+i-1].normal;
		}
		if (is_rimmed)
		{
			covertices[2+4*nbrays+i].vertex=nodes[nodeIDs[2+4*nbrays+i]].smoothpos-center;
			//normals
			if ((i%2)==0)
			{
				covertices[2+4*nbrays+i].normal=approx_normalise(nodes[nodeIDs[2+4*nbrays+i]].smoothpos-nodes[nodeIDs[2+4*nbrays+i+1]].smoothpos);
			} else
			{
				covertices[2+4*nbrays+i].normal=-covertices[2+4*nbrays+i-1].normal;
			}
			//bands
			covertices[2+2*nbrays+i].vertex=covertices[2+4*nbrays+i].vertex;
			covertices[2+2*nbrays+i].normal=approx_normalise(covertices[2+4*nbrays+i].vertex);
		} else
		{
			//bands
			covertices[2+2*nbrays+i].vertex=covertices[2+i].vertex;
			covertices[2+2*nbrays+i].normal=approx_normalise(covertices[2+i].vertex);
		}
	}
	return center;
}

Vector3 FlexMesh::updateShadowVertices()
{
	Vector3 center = (nodes[nodeIDs[0]].smoothpos + nodes[nodeIDs[1]].smoothpos) / 2.0;

	coshadowposvertices[0].vertex=nodes[nodeIDs[0]].smoothpos-center;
	//normals
	coshadownorvertices[0].normal=approx_normalise(nodes[nodeIDs[0]].smoothpos-nodes[nodeIDs[1]].smoothpos);

	coshadowposvertices[1].vertex=nodes[nodeIDs[1]].smoothpos-center;
	//normals
	coshadownorvertices[1].normal=-coshadownorvertices[0].normal;

	for (int i=0; i<nbrays*2; i++)
	{
		coshadowposvertices[2+i].vertex=nodes[nodeIDs[2+i]].smoothpos-center;

		coshadownorvertices[2+i].normal=approx_normalise(nodes[nodeIDs[2+i]].smoothpos-center);
		//normals
		if ((i%2)==0)
		{
			coshadownorvertices[2+i].normal=approx_normalise(nodes[nodeIDs[0]].smoothpos-nodes[nodeIDs[1]].smoothpos);
		} else
		{
			coshadownorvertices[2+i].normal=-coshadownorvertices[2+i-1].normal;
		}
		if (is_rimmed)
		{
			coshadowposvertices[2+4*nbrays+i].vertex=nodes[nodeIDs[2+4*nbrays+i]].smoothpos-center;

			coshadownorvertices[2+4*nbrays+i].normal=approx_normalise(nodes[nodeIDs[2+4*nbrays+i]].smoothpos-center);
			//normals
			if ((i%2)==0)
			{
				coshadownorvertices[2+4*nbrays+i].normal=approx_normalise(nodes[nodeIDs[2+4*nbrays+i]].smoothpos-nodes[nodeIDs[2+4*nbrays+i+1]].smoothpos);
			} else
			{
				coshadownorvertices[2+4*nbrays+i].normal=-coshadownorvertices[2+4*nbrays+i-1].normal;
			}
			//bands
			coshadowposvertices[2+2*nbrays+i].vertex=coshadowposvertices[2+4*nbrays+i].vertex;
			if ((i%2)==0)
			{
				coshadownorvertices[2+2*nbrays+i].normal=approx_normalise(nodes[nodeIDs[2+i]].smoothpos-nodes[nodeIDs[0]].smoothpos);
			} else
			{
				coshadownorvertices[2+2*nbrays+i].normal=approx_normalise(nodes[nodeIDs[2+i]].smoothpos-nodes[nodeIDs[1]].smoothpos);
			}
		} else
		{
			//bands
			coshadowposvertices[2+2*nbrays+i].vertex=coshadowposvertices[2+i].vertex;
			if ((i%2)==0)
			{
				coshadownorvertices[2+2*nbrays+i].normal=approx_normalise(nodes[nodeIDs[2+i]].smoothpos-nodes[nodeIDs[0]].smoothpos);
			} else
			{
				coshadownorvertices[2+2*nbrays+i].normal=approx_normalise(nodes[nodeIDs[2+i]].smoothpos-nodes[nodeIDs[1]].smoothpos);
			}
		}

	}

	if (is_rimmed)
	{
		for (int i=0; i<2+nbrays*6; i++)
		{
			coshadowposvertices[i+2+nbrays*6].vertex=coshadowposvertices[i].vertex;
			coshadownorvertices[i].texcoord=covertices[i].texcoord;
		}
	} else
	{
		for (int i=0; i<2+nbrays*4; i++)
		{
			coshadowposvertices[i+2+nbrays*4].vertex=coshadowposvertices[i].vertex;
			coshadownorvertices[i].texcoord=covertices[i].texcoord;
		}
	}

	return center;
}

/*
Vector3 FlexMesh::updateShadowVertices()
{
		int i;
	Vector3 center;
	center=(nodes[nodeIDs[0]].Position+nodes[nodeIDs[1]].Position)/2.0;

	coshadowposvertices[0].vertex=nodes[nodeIDs[0]].Position-center;
		//normals
	coshadownorvertices[0].normal=coshadowposvertices[0].vertex;
	coshadownorvertices[0].normal.normalise();

	coshadowposvertices[1].vertex=nodes[nodeIDs[1]].Position-center;
		//normals
	coshadownorvertices[1].normal=coshadowposvertices[1].vertex;
	coshadownorvertices[1].normal.normalise();
	//texcoords
	coshadownorvertices[0].texcoord=covertices[0].texcoord;
	coshadownorvertices[1].texcoord=covertices[1].texcoord;
	for (i=0; i<nbrays*2; i++)
	{
		coshadowposvertices[2+i].vertex=nodes[nodeIDs[2+i]].Position-center;
		//normals
		coshadownorvertices[2+i].normal=coshadowposvertices[2+i].vertex;
		coshadownorvertices[2+i].normal.normalise();
		//optimization
		coshadowposvertices[2+2*nbrays+i].vertex=coshadowposvertices[2+i].vertex;
		coshadownorvertices[2+2*nbrays+i].normal=coshadownorvertices[2+i].normal;

		//texcoords
		coshadownorvertices[2+i].texcoord=covertices[2+i].texcoord;
		coshadownorvertices[2+2*nbrays+i].texcoord=covertices[2+2*nbrays+i].texcoord;
	}
	return center;
}
*/

void FlexMesh::setVisible(bool visible)
{
	// nothing to do here?
}

void FlexMesh::flexitCompute()
{
	if (gEnv->sceneManager->getShadowTechnique()==SHADOWTYPE_STENCIL_MODULATIVE || gEnv->sceneManager->getShadowTechnique()==SHADOWTYPE_STENCIL_ADDITIVE)
	{
		flexit_center = updateShadowVertices();
	} else
	{
		flexit_center = updateVertices();
	}
}

Vector3 FlexMesh::flexitFinal()
{
	if (gEnv->sceneManager->getShadowTechnique()==SHADOWTYPE_STENCIL_MODULATIVE || gEnv->sceneManager->getShadowTechnique()==SHADOWTYPE_STENCIL_ADDITIVE)
	{
		//find the binding
		unsigned posbinding = msh->sharedVertexData->vertexDeclaration->findElementBySemantic(VES_POSITION)->getSource();
		HardwareVertexBufferSharedPtr pbuf=msh->sharedVertexData->vertexBufferBinding->getBuffer(posbinding);
		//pbuf->lock(HardwareBuffer::HBL_NORMAL);
		pbuf->writeData(0, pbuf->getSizeInBytes(), shadowposvertices, true);
		//pbuf->unlock();
		//find the binding
		unsigned norbinding = msh->sharedVertexData->vertexDeclaration->findElementBySemantic(VES_NORMAL)->getSource();
		HardwareVertexBufferSharedPtr nbuf=msh->sharedVertexData->vertexBufferBinding->getBuffer(norbinding);
		//nbuf->lock(HardwareBuffer::HBL_NORMAL);
		nbuf->writeData(0, nbuf->getSizeInBytes(), shadownorvertices, true);
		//nbuf->unlock();

		EdgeData* ed = msh->getEdgeList();
		ed->updateFaceNormals(0, pbuf);
	} else
	{
		//vbuf->lock(HardwareBuffer::HBL_NORMAL);
		vbuf->writeData(0, vbuf->getSizeInBytes(), vertices, true);
		//vbuf->unlock();
		//msh->sharedVertexData->vertexBufferBinding->getBuffer(0)->writeData(0, vbuf->getSizeInBytes(), vertices, true);
	}

	return flexit_center;
}
