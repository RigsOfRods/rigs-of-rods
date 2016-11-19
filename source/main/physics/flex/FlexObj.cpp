/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2013+     Petr Ohlidal & contributors

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

FlexObj::FlexObj(node_t *nds, int numtexcoords, Vector3* texcoords, int numtriangles, int* triangles, int numsubmeshes, int* subtexindex, int* subtriindex, char* texname, char* name, int* subisback, char* backtexname, char* transtexname)
{
    unsigned int i;
    int j;
    triangleCount = numtriangles;

    gEnv->sceneManager=gEnv->sceneManager;
    nodes=nds;
    /// Create the mesh via the MeshManager
    msh = MeshManager::getSingleton().createManual(name, ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);

    /// Create submeshes
    subs=(SubMesh**)malloc(numsubmeshes*sizeof(SubMesh*));
    for (j=0; j<numsubmeshes; j++)
    {
        subs[j] = msh->createSubMesh();
        //materials
        if (j<numsubmeshes-1)
        {
            if (subisback[j+1]==0) subs[j]->setMaterialName(texname);
            if (subisback[j+1]==1) subs[j]->setMaterialName(backtexname);
            if (subisback[j+1]==2) subs[j]->setMaterialName(transtexname);
        }
        else
            subs[j]->setMaterialName(texname);
    };

    /// Define the vertices (8 vertices, each consisting of 3 groups of 3 floats
    nVertices = numtexcoords;
    vbufCount = (2*3+2)*nVertices;
    vertices=(float*)malloc(vbufCount*sizeof(float));
    //shadow
    shadownorvertices=(float*)malloc(nVertices*(3+2)*sizeof(float));
    shadowposvertices=(float*)malloc(nVertices*3*2*sizeof(float));
    nodeIDs=(int*)malloc(nVertices*sizeof(int));

    //define node ids
    for (i=0; i<nVertices; i++)
    {
        nodeIDs[i]=(int)(texcoords[i].x);
    }

    //textures coordinates
    for (i=0; i<nVertices; i++)
    {
        covertices[i].texcoord=Vector2(texcoords[i].y,texcoords[i].z);
    }

    /// Define triangles
    /// The values in this table refer to vertices in the above table
    ibufCount = 3*numtriangles;
    faces=(unsigned short*)malloc(ibufCount*sizeof(unsigned short));
    for (i=0; i<ibufCount; i++)
    {
        faces[i]=findID(i/3, triangles[i], numsubmeshes, subtexindex, subtriindex);
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




    /// Set parameters of the submeshes
    for (j=0; j<numsubmeshes; j++)
    {
        int smcount=3*(subtriindex[j+1]-subtriindex[j]);
        subs[j]->useSharedVertices = true;
        /// Allocate index buffer of the requested number of vertices (ibufCount)
        HardwareIndexBufferSharedPtr ibuf = HardwareBufferManager::getSingleton().
         createIndexBuffer(
             HardwareIndexBuffer::IT_16BIT,
                smcount,
                HardwareBuffer::HBU_STATIC_WRITE_ONLY);

        /// Upload the index data to the card
        ibuf->writeData(0, ibuf->getSizeInBytes(), &faces[subtriindex[j]*3], true);
        subs[j]->indexData->indexBuffer = ibuf;
        subs[j]->indexData->indexCount = smcount;
        subs[j]->indexData->indexStart = 0;
    }


    /// Set bounding information (for culling)
    msh->_setBounds(AxisAlignedBox(-100,-100,-100,100,100,100), true);
    //msh->_setBoundingSphereRadius(100);


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
int FlexObj::findID(int tidx, int v, int numsubmeshes, int* subtexindex, int* subtriindex)
{
    int j;
    //first: find the context from the triangle index tidx
    int context;
    for (context=0; context<numsubmeshes+1; context++) if (tidx<subtriindex[context]) {context--;break;};
    //okay, now search in the vertice block corresponding to the context
    for (j=subtexindex[context]; j<subtexindex[context+1]; j++) if (nodeIDs[j]==v) return j;
    return 0;
}

/*Vector3 FlexObj::updateVertices()
{
    unsigned int i;
    Vector3 center;
    center=(nodes[nodeIDs[0]].Position+nodes[nodeIDs[1]].Position)/2.0;
    for (i=0; i<nVertices; i++)
    {
        covertices[i].vertex=nodes[nodeIDs[i]].Position-center;
        //normals
        covertices[i].normal=covertices[i].vertex;
        covertices[i].normal.normalise();
    }

    return center;
}
*/

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
    Vector3 center(Vector3::ZERO);

    center=updateVertices();
    //vbuf->lock(HardwareBuffer::HBL_NORMAL);
    vbuf->writeData(0, vbuf->getSizeInBytes(), vertices, true);
    //vbuf->unlock();
    //msh->sharedVertexData->vertexBufferBinding->getBuffer(0)->writeData(0, vbuf->getSizeInBytes(), vertices, true);

    return center;
}

FlexObj::~FlexObj()
{
    if (!msh.isNull()) msh->unload();

    if (subs              != nullptr) { free (subs); }
    if (vertices          != nullptr) { free (vertices); }
    if (shadownorvertices != nullptr) { free (shadownorvertices); }
    if (shadowposvertices != nullptr) { free (shadowposvertices); }
    if (nodeIDs           != nullptr) { free (nodeIDs); }
    if (faces             != nullptr) { free (faces); }
    if (sref              != nullptr) { free (sref); }
}
