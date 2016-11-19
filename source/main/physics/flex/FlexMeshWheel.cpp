/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2013-2015 Petr Ohlidal

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

#include "MaterialReplacer.h"
#include "Skin.h"
#include "MaterialFunctionMapper.h"
#include "BeamData.h"

#include <Ogre.h>

using namespace Ogre;

FlexMeshWheel::FlexMeshWheel(
    Ogre::String const & name,
    node_t *nds, 
    int axis_node_1_index, 
    int axis_node_2_index, 
    int nstart, 
    int nrays, 
    Ogre::String const & mesh_name,
    Ogre::String const & material_name,//char* texband, 
    float rimradius, 
    bool rimreverse, 
    MaterialFunctionMapper *material_function_mapper, // *mfm
    Skin *used_skin, // *usedSkin, 
    MaterialReplacer *material_replacer // *mr
) :
      id0(axis_node_1_index)
    , id1(axis_node_2_index)
    , idstart(nstart)
    , mr(material_replacer)
    , nbrays(nrays)
    , nodes(nds)
    , revrim(rimreverse)
    , rim_radius(rimradius)
{

    //the rim object
    Ogre::String rim_name = "rim-" + name;
    rimEnt = gEnv->sceneManager->createEntity(rim_name, mesh_name);
    MaterialFunctionMapper::replaceSimpleMeshMaterials(rimEnt, ColourValue(0, 0.5, 0.8));
    if (material_function_mapper != nullptr)
    {
        material_function_mapper->replaceMeshMaterials(rimEnt);
    }
    if (material_replacer != nullptr)
    {
        material_replacer->replaceMeshMaterials(rimEnt);
    }
    if (used_skin != nullptr)
    {
        used_skin->replaceMeshMaterials(rimEnt);
    }
    rnode=gEnv->sceneManager->getRootSceneNode()->createChildSceneNode();
    rnode->attachObject(rimEnt);

    /// Create the mesh via the MeshManager
    msh = MeshManager::getSingleton().createManual(name, ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);

    /// Create submeshes
    sub = msh->createSubMesh();

    //materials
    sub->setMaterialName(material_name);

    /// Define the vertices
    nVertices = 6*(nrays+1);
    vbufCount = (2*3+2)*nVertices;
    vertices=(float*)malloc(vbufCount*sizeof(float));
    //shadow
    shadownorvertices=(float*)malloc(nVertices*(3+2)*sizeof(float));
    shadowposvertices=(float*)malloc(nVertices*3*2*sizeof(float));

    int i;
    //textures coordinates
    for (i=0; i<nrays+1; i++)
    {
        covertices[i*6   ].texcoord=Vector2((float)i/(float)nrays, 0.00f);
        covertices[i*6+1 ].texcoord=Vector2((float)i/(float)nrays, 0.23f);
        covertices[i*6+2 ].texcoord=Vector2((float)i/(float)nrays, 0.27f);
        covertices[i*6+3 ].texcoord=Vector2((float)i/(float)nrays, 0.73f);
        covertices[i*6+4 ].texcoord=Vector2((float)i/(float)nrays, 0.77f);
        covertices[i*6+5 ].texcoord=Vector2((float)i/(float)nrays, 1.00f);
    }

    /// Define triangles
    /// The values in this table refer to vertices in the above table
    ibufCount = 3*10*nrays;
    faces=(unsigned short*)malloc(ibufCount*sizeof(unsigned short));
    for (i=0; i<nrays; i++)
    {
        faces[3*(i*10  )]=i*6;   faces[3*(i*10  )+1]=i*6+1;     faces[3*(i*10  )+2]=(i+1)*6;
        faces[3*(i*10+1)]=i*6+1; faces[3*(i*10+1)+1]=(i+1)*6+1; faces[3*(i*10+1)+2]=(i+1)*6;

        faces[3*(i*10+2)]=i*6+1; faces[3*(i*10+2)+1]=i*6+2;     faces[3*(i*10+2)+2]=(i+1)*6+1;
        faces[3*(i*10+3)]=i*6+2; faces[3*(i*10+3)+1]=(i+1)*6+2; faces[3*(i*10+3)+2]=(i+1)*6+1;

        faces[3*(i*10+4)]=i*6+2; faces[3*(i*10+4)+1]=i*6+3;     faces[3*(i*10+4)+2]=(i+1)*6+2;
        faces[3*(i*10+5)]=i*6+3; faces[3*(i*10+5)+1]=(i+1)*6+3; faces[3*(i*10+5)+2]=(i+1)*6+2;

        faces[3*(i*10+6)]=i*6+3; faces[3*(i*10+6)+1]=i*6+4;     faces[3*(i*10+6)+2]=(i+1)*6+3;
        faces[3*(i*10+7)]=i*6+4; faces[3*(i*10+7)+1]=(i+1)*6+4; faces[3*(i*10+7)+2]=(i+1)*6+3;

        faces[3*(i*10+8)]=i*6+4; faces[3*(i*10+8)+1]=i*6+5;     faces[3*(i*10+8)+2]=(i+1)*6+4;
        faces[3*(i*10+9)]=i*6+5; faces[3*(i*10+9)+1]=(i+1)*6+5; faces[3*(i*10+9)+2]=(i+1)*6+4;
    }

    normy=1.0;
    //update coords
    updateVertices();
    //compute normy;
    normy=((covertices[0].vertex-covertices[1].vertex).crossProduct(covertices[1].vertex-covertices[6+1].vertex)).length();
    //recompute for normals
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
    HardwareIndexBufferSharedPtr ibuf = HardwareBufferManager::getSingleton().
     createIndexBuffer(
         HardwareIndexBuffer::IT_16BIT,
            ibufCount,
            HardwareBuffer::HBU_STATIC_WRITE_ONLY);

    /// Upload the index data to the card
    ibuf->writeData(0, ibuf->getSizeInBytes(), faces, true);

    /// Set parameters of the submesh
    sub->useSharedVertices = true;
    sub->indexData->indexBuffer = ibuf;
    sub->indexData->indexCount = ibufCount;
    sub->indexData->indexStart = 0;


    /// Set bounding information (for culling)
    msh->_setBounds(AxisAlignedBox(-1,-1,0,1,1,0), true);
    //msh->_setBoundingSphereRadius(Math::Sqrt(1*1+1*1));

    /// Notify Mesh object that it has been loaded
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

FlexMeshWheel::~FlexMeshWheel()
{
    if (vertices          != nullptr) { free (vertices); }
    if (shadownorvertices != nullptr) { free (shadownorvertices); }
    if (shadowposvertices != nullptr) { free (shadowposvertices); }
    if (faces             != nullptr) { free (faces); }
}

Vector3 FlexMeshWheel::updateVertices()
{
    Vector3 center = (nodes[id0].AbsPosition + nodes[id1].AbsPosition) / 2.0;
    Vector3 ray = nodes[idstart].AbsPosition - nodes[id0].AbsPosition;
    Vector3 axis = nodes[id0].AbsPosition - nodes[id1].AbsPosition;

    axis.normalise();
    
    for (int i=0; i<nbrays; i++)
    {
        Plane pl=Plane(axis, nodes[id0].AbsPosition);
        ray=nodes[idstart+i*2].AbsPosition-nodes[id0].AbsPosition;
        ray=pl.projectVector(ray);
        ray.normalise();
        covertices[i*6  ].vertex=nodes[id0].AbsPosition+rim_radius*ray-center;

        covertices[i*6+1].vertex=nodes[idstart+i*2].AbsPosition-0.05*(nodes[idstart+i*2].AbsPosition-nodes[id0].AbsPosition)-center;
        covertices[i*6+2].vertex=nodes[idstart+i*2].AbsPosition-0.1*(nodes[idstart+i*2].AbsPosition-nodes[idstart+i*2+1].AbsPosition)-center;
        covertices[i*6+3].vertex=nodes[idstart+i*2+1].AbsPosition-0.1*(nodes[idstart+i*2+1].AbsPosition-nodes[idstart+i*2].AbsPosition)-center;
        covertices[i*6+4].vertex=nodes[idstart+i*2+1].AbsPosition-0.05*(nodes[idstart+i*2+1].AbsPosition-nodes[id1].AbsPosition)-center;

        pl=Plane(-axis, nodes[id1].AbsPosition);
        ray=nodes[idstart+i*2+1].AbsPosition-nodes[id1].AbsPosition;
        ray=pl.projectVector(ray);
        ray.normalise();
        covertices[i*6+5].vertex=nodes[id1].AbsPosition+rim_radius*ray-center;

        //normals
        covertices[i*6  ].normal=axis;
        covertices[i*6+1].normal=(covertices[i*6].vertex-covertices[i*6+1].vertex).crossProduct(covertices[i*6].vertex-covertices[((i+1)%nbrays)*6+1].vertex)/normy;
        covertices[i*6+2].normal=ray;
        covertices[i*6+3].normal=ray;
        covertices[i*6+4].normal=(covertices[i*6+4].vertex-covertices[i*6+5].vertex).crossProduct(covertices[i*6+4].vertex-covertices[((i+1)%nbrays)*6+4].vertex)/normy;
        covertices[i*6+5].normal=-axis;
    }
    for (int i=0; i<6; i++)
    {
        covertices[nbrays*6+i].vertex=covertices[i].vertex;
        covertices[nbrays*6+i].normal=covertices[i].normal;
    }

    return center;
}

Vector3 FlexMeshWheel::updateShadowVertices()
{
    Vector3 center = (nodes[id0].AbsPosition + nodes[id1].AbsPosition) / 2.0;
    Vector3 ray = nodes[idstart].AbsPosition - nodes[id0].AbsPosition;
    Vector3 axis = nodes[id0].AbsPosition - nodes[id1].AbsPosition;
    
    axis.normalise();

    for (int i=0; i<nbrays; i++)
    {
        Plane pl=Plane(axis, nodes[id0].AbsPosition);
        ray=nodes[idstart+i*2].AbsPosition-nodes[id0].AbsPosition;
        ray=pl.projectVector(ray);
        ray.normalise();
        coshadowposvertices[i*6  ].vertex=nodes[id0].AbsPosition+rim_radius*ray-center;

        coshadowposvertices[i*6+1].vertex=nodes[idstart+i*2].AbsPosition-0.05*(nodes[idstart+i*2].AbsPosition-nodes[id0].AbsPosition)-center;
        coshadowposvertices[i*6+2].vertex=nodes[idstart+i*2].AbsPosition-0.1*(nodes[idstart+i*2].AbsPosition-nodes[idstart+i*2+1].AbsPosition)-center;
        coshadowposvertices[i*6+3].vertex=nodes[idstart+i*2+1].AbsPosition-0.1*(nodes[idstart+i*2+1].AbsPosition-nodes[idstart+i*2].AbsPosition)-center;
        coshadowposvertices[i*6+4].vertex=nodes[idstart+i*2+1].AbsPosition-0.05*(nodes[idstart+i*2+1].AbsPosition-nodes[id1].AbsPosition)-center;

        pl=Plane(-axis, nodes[id1].AbsPosition);
        ray=nodes[idstart+i*2+1].AbsPosition-nodes[id1].AbsPosition;
        ray=pl.projectVector(ray);
        ray.normalise();
        coshadowposvertices[i*6+5].vertex=nodes[id1].AbsPosition+rim_radius*ray-center;

        //normals
        coshadownorvertices[i*6  ].normal=axis;
        coshadownorvertices[i*6+1].normal=(coshadowposvertices[i*6].vertex-coshadowposvertices[i*6+1].vertex).crossProduct(coshadowposvertices[i*6].vertex-coshadowposvertices[((i+1)%nbrays)*6+1].vertex)/normy;
        coshadownorvertices[i*6+2].normal=ray;
        coshadownorvertices[i*6+3].normal=ray;
        coshadownorvertices[i*6+4].normal=(coshadowposvertices[i*6+4].vertex-coshadowposvertices[i*6+5].vertex).crossProduct(coshadowposvertices[i*6+4].vertex-coshadowposvertices[((i+1)%nbrays)*6+4].vertex)/normy;
        coshadownorvertices[i*6+5].normal=-axis;

        coshadownorvertices[i*6  ].texcoord=covertices[i*6  ].texcoord;
        coshadownorvertices[i*6+1].texcoord=covertices[i*6+1].texcoord;
        coshadownorvertices[i*6+2].texcoord=covertices[i*6+2].texcoord;
        coshadownorvertices[i*6+3].texcoord=covertices[i*6+3].texcoord;
        coshadownorvertices[i*6+4].texcoord=covertices[i*6+4].texcoord;
        coshadownorvertices[i*6+5].texcoord=covertices[i*6+5].texcoord;

    }
    for (int i=0; i<6; i++)
    {
        coshadowposvertices[nbrays*6+i].vertex=coshadowposvertices[i].vertex;
        coshadownorvertices[nbrays*6+i].normal=coshadownorvertices[i].normal;
        coshadownorvertices[nbrays*6+i].texcoord=coshadownorvertices[i].texcoord;
    }

    return center;
}

void FlexMeshWheel::setVisible(bool visible)
{
    if (rnode) rnode->setVisible(visible);
}

bool FlexMeshWheel::flexitPrepare()
{
    Vector3 center = (nodes[id0].AbsPosition + nodes[id1].AbsPosition) / 2.0;
    rnode->setPosition(center);

    Vector3 axis = nodes[id0].AbsPosition - nodes[id1].AbsPosition;
    axis.normalise();

    if (revrim) axis = -axis;
    Vector3 ray = nodes[idstart].AbsPosition - nodes[id0].AbsPosition;
    Vector3 onormal = axis.crossProduct(ray);
    onormal.normalise();
    ray = axis.crossProduct(onormal);
    rnode->setOrientation(Quaternion(axis, onormal, ray));

    return true;
}

void FlexMeshWheel::flexitCompute()
{
    flexit_center = updateVertices();
}

Vector3 FlexMeshWheel::flexitFinal()
{

    //vbuf->lock(HardwareBuffer::HBL_NORMAL);
    vbuf->writeData(0, vbuf->getSizeInBytes(), vertices, true);
    //vbuf->unlock();
    //msh->sharedVertexData->vertexBufferBinding->getBuffer(0)->writeData(0, vbuf->getSizeInBytes(), vertices, true);

    return flexit_center;
}
