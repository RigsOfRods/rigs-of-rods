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

#include "FlexObj.h"

#include <Ogre.h>

using namespace Ogre;

FlexObj::FlexObj(node_t *nds, std::vector<CabTexcoord>& texcoords, int numtriangles, 
             int* triangles, std::vector<CabSubmesh>& submesh_defs, 
             char* texname, char* name, char* backtexname, char* transtexname)
{
    unsigned int i;
    triangleCount = numtriangles;

    gEnv->sceneManager=gEnv->sceneManager;
    nodes=nds;
    /// Create the mesh via the MeshManager
    msh = MeshManager::getSingleton().createManual(name, ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);

    /// Create submeshes
    m_submeshes.reserve(submesh_defs.size());
    for (size_t j=0; j<submesh_defs.size(); j++)
    {
		Ogre::SubMesh* submesh = msh->createSubMesh();
        switch (submesh_defs[j].backmesh_type)
		{
		case CabSubmesh::BACKMESH_OPAQUE:      submesh->setMaterialName(backtexname);  break;
		case CabSubmesh::BACKMESH_TRANSPARENT: submesh->setMaterialName(transtexname); break;
		default:                               submesh->setMaterialName(texname);
		}
		m_submeshes.push_back(submesh);
    };

    /// Define the vertices (8 vertices, each consisting of 3 groups of 3 floats
    nVertices = texcoords.size();
    vbufCount = (2*3+2)*nVertices;
    vertices=(float*)malloc(vbufCount*sizeof(float));
    //shadow
    shadownorvertices=(float*)malloc(nVertices*(3+2)*sizeof(float));
    shadowposvertices=(float*)malloc(nVertices*3*2*sizeof(float));
    nodeIDs=(int*)malloc(nVertices*sizeof(int));

    
    for (i=0; i<nVertices; i++)
    {
        nodeIDs[i] = texcoords[i].node_id; //define node ids
		covertices[i].texcoord=Vector2(texcoords[i].texcoord_u, texcoords[i].texcoord_v); //textures coordinates
    }

    /// Define triangles
    /// The values in this table refer to vertices in the above table
    ibufCount = 3*numtriangles;
    faces=(unsigned short*)malloc(ibufCount*sizeof(unsigned short));
    for (i=0; i<ibufCount; i++)
    {
        faces[i]=findID(i/3, triangles[i], submesh_defs);
    }

    sref=(float*)malloc(numtriangles*sizeof(float));

    for (i=0; i<(unsigned int)numtriangles;i++)
    {
        Vector3 v1, v2;
        v1=nodes[nodeIDs[faces[i*3+1]]].RelPosition-nodes[nodeIDs[faces[i*3]]].RelPosition;
        v2=nodes[nodeIDs[faces[i*3+2]]].RelPosition-nodes[nodeIDs[faces[i*3]]].RelPosition;
        v1=v1.crossProduct(v2);
        sref[i]=v1.length()*2.0;
    }


    //update coords
    updateVertices();



    /// Create vertex data structure for vertices shared between submeshes
    msh->sharedVertexData = new VertexData();
    msh->sharedVertexData->vertexCount = nVertices;


    /// Create declaration (memory format) of vertex data
    decl = msh->sharedVertexData->vertexDeclaration;
    size_t offset = 0;
    decl->addElement(0, offset, VET_FLOAT3, VES_POSITION);
    offset += VertexElement::getTypeSize(VET_FLOAT3);
    decl->addElement(0, offset, VET_FLOAT3, VES_NORMAL);
    offset += VertexElement::getTypeSize(VET_FLOAT3);
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

    /// Set parameters of the submeshes
    for (size_t j=0; j<m_submeshes.size(); j++)
    {
        size_t index_count;
		if (j == 0)
			index_count = 3*submesh_defs[j].cabs_pos;
		else
			index_count = 3*(submesh_defs[j].cabs_pos-submesh_defs[j-1].cabs_pos); // 3 indices per triangle
        m_submeshes[j]->useSharedVertices = true;
        /// Allocate index buffer of the requested number of vertices (ibufCount)
        HardwareIndexBufferSharedPtr ibuf = HardwareBufferManager::getSingleton().createIndexBuffer(
             HardwareIndexBuffer::IT_16BIT,
             index_count,
             HardwareBuffer::HBU_STATIC_WRITE_ONLY);

        /// Upload the index data to the card
		unsigned short* faces_ptr;
		if (j == 0)
			faces_ptr = &faces[0];
		else
			faces_ptr = &faces[submesh_defs[j-1].cabs_pos * 3];
        ibuf->writeData(0, ibuf->getSizeInBytes(), faces_ptr, true);
        m_submeshes[j]->indexData->indexBuffer = ibuf;
        m_submeshes[j]->indexData->indexCount = index_count;
        m_submeshes[j]->indexData->indexStart = 0;
    }


    /// Set bounding information (for culling)
    msh->_setBounds(AxisAlignedBox(-100,-100,-100,100,100,100), true);

    /// Notify Mesh object that it has been loaded
    msh->load();
}

void FlexObj::scale(float factor)
{
    for (int i=0; i<triangleCount;i++)
    {
        sref[i] *= factor;
    }
}

//find the zeroed id of the node v in the context of the tidx triangle
int FlexObj::findID(int tidx, int v, std::vector<CabSubmesh>& submeshes)
{
    //first: find the context from the triangle index tidx
    size_t context;
    for (context=0; context<submeshes.size()+1; context++) 
	{
		if (static_cast<unsigned int>(tidx) < submeshes[context].cabs_pos)
		{
			if (context>0)
				context--;
			break;
		}
	}
    //okay, now search in the vertice block corresponding to the context
	for (size_t j=submeshes[context].texcoords_pos; j<submeshes[context+1].texcoords_pos; j++)
	{
		if (nodeIDs[j]==v)
			return j;
	}
    return 0;
}

//with normals
Vector3 FlexObj::updateVertices()
{
    unsigned int i;
    Vector3 center;
    center=(nodes[nodeIDs[0]].AbsPosition+nodes[nodeIDs[1]].AbsPosition)/2.0;
    for (i=0; i<nVertices; i++)
    {
        //set position
        covertices[i].vertex=nodes[nodeIDs[i]].AbsPosition-center;
        //reset normals
        covertices[i].normal=Vector3::ZERO;
    }
    //accumulate normals per triangle
    for (i=0; i<ibufCount/3; i++)
    {
        Vector3 v1, v2;
        v1=nodes[nodeIDs[faces[i*3+1]]].AbsPosition-nodes[nodeIDs[faces[i*3]]].AbsPosition;
        v2=nodes[nodeIDs[faces[i*3+2]]].AbsPosition-nodes[nodeIDs[faces[i*3]]].AbsPosition;
        v1=v1.crossProduct(v2);
        float s=v1.length();
        //avoid large tris
        if (s>sref[i])
        {
            covertices[faces[i*3+1]].vertex=covertices[faces[i*3]].vertex+Vector3(0.1,0,0);
            covertices[faces[i*3+2]].vertex=covertices[faces[i*3]].vertex+Vector3(0,0,0.1);
        }
        //v1.normalise();
        if (s == 0)
            continue;
        v1=v1/s;
        covertices[faces[i*3]].normal+=v1;
        covertices[faces[i*3+1]].normal+=v1;
        covertices[faces[i*3+2]].normal+=v1;
    }
    //normalize
    for (i=0; i<nVertices; i++)
    {
        covertices[i].normal = approx_normalise(covertices[i].normal);
    }

    return center;
}

//with normals
Vector3 FlexObj::updateShadowVertices()
{
    Vector3 center = (nodes[nodeIDs[0]].AbsPosition + nodes[nodeIDs[1]].AbsPosition) / 2.0;

    for (unsigned int i=0; i<nVertices; i++)
    {
        //set position
        coshadowposvertices[i].vertex=nodes[nodeIDs[i]].AbsPosition-center;
        coshadowposvertices[i+nVertices].vertex=nodes[nodeIDs[i]].AbsPosition-center;
        //reset normals
        coshadownorvertices[i].normal=Vector3::ZERO;
    }
    //accumulate normals per triangle
    for (unsigned int i=0; i<ibufCount/3; i++)
    {
        Vector3 v1, v2;
        v1=nodes[nodeIDs[faces[i*3+1]]].AbsPosition-nodes[nodeIDs[faces[i*3]]].AbsPosition;
        v2=nodes[nodeIDs[faces[i*3+2]]].AbsPosition-nodes[nodeIDs[faces[i*3]]].AbsPosition;
        v1=v1.crossProduct(v2);
        float s=v1.length();
        //avoid large tris
        if (s>sref[i])
        {
            coshadowposvertices[faces[i*3+1]].vertex=coshadowposvertices[faces[i*3]].vertex+Vector3(0.1,0,0);
            coshadowposvertices[faces[i*3+2]].vertex=coshadowposvertices[faces[i*3]].vertex+Vector3(0,0,0.1);
            coshadowposvertices[faces[i*3+1]+nVertices].vertex=coshadowposvertices[faces[i*3+1]].vertex;
            coshadowposvertices[faces[i*3+2]+nVertices].vertex=coshadowposvertices[faces[i*3+2]].vertex;
        }
        //v1.normalise();
        if (s == 0)
            continue;
        v1=v1/s;

        coshadownorvertices[faces[i*3]].normal+=v1;
        coshadownorvertices[faces[i*3+1]].normal+=v1;
        coshadownorvertices[faces[i*3+2]].normal+=v1;
    }
    //normalize
    for (unsigned int i=0; i<nVertices; i++)
    {
        coshadownorvertices[i].normal = approx_normalise(coshadownorvertices[i].normal);
        //texcoords
        coshadownorvertices[i].texcoord=covertices[i].texcoord;
    }

    return center;
}

Vector3 FlexObj::flexit()
{
    Ogre::Vector3 center = this->updateVertices();
    vbuf->writeData(0, vbuf->getSizeInBytes(), vertices, true);
    return center;
}

FlexObj::~FlexObj()
{
    if (!msh.isNull())
    {
        Ogre::MeshManager::getSingleton().remove(msh->getHandle());
        msh.setNull();
    }

    if (vertices          != nullptr) { free (vertices); }
    if (shadownorvertices != nullptr) { free (shadownorvertices); }
    if (shadowposvertices != nullptr) { free (shadowposvertices); }
    if (nodeIDs           != nullptr) { free (nodeIDs); }
    if (faces             != nullptr) { free (faces); }
    if (sref              != nullptr) { free (sref); }
}
