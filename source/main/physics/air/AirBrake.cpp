/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer

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

#include "AirBrake.h"

#include "Application.h"
#include "SimData.h"
#include "Actor.h"
#include "GfxActor.h"
#include "GfxScene.h"

#include <Ogre.h>

using namespace Ogre;
using namespace RoR;

Airbrake::Airbrake(ActorPtr actor, const char* basename, int num, NodeNum_t ndref, NodeNum_t ndx, NodeNum_t ndy, NodeNum_t nda, Ogre::Vector3 pos, float width, float length, float maxang, std::string const & texname, float tx1, float ty1, float tx2, float ty2, float lift_coef)
{
    m_actor = actor;
    snode = 0;
    noderef = ndref;
    nodex = ndx;
    nodey = ndy;
    nodea = nda;
    offset = pos;
    maxangle = maxang;
    area = width * length * lift_coef;
    char meshname[256];
    sprintf(meshname, "airbrakemesh-%s-%i", basename, num);
    /// Create the mesh via the MeshManager
    msh = MeshManager::getSingleton().createManual(meshname, actor->GetGfxActor()->GetResourceGroup());

    union
    {
        float* vertices;
        CoVertice_t* covertices;
    };

    /// Create submesh
    SubMesh* sub = msh->createSubMesh();

    //materials
    sub->setMaterialName(texname);

    /// Define the vertices
    size_t nVertices = 4;
    size_t vbufCount = (2 * 3 + 2) * nVertices;
    vertices = (float*)malloc(vbufCount * sizeof(float));

    //textures coordinates
    covertices[0].texcoord = Vector2(tx1, ty1);
    covertices[1].texcoord = Vector2(tx2, ty1);
    covertices[2].texcoord = Vector2(tx2, ty2);
    covertices[3].texcoord = Vector2(tx1, ty2);

    /// Define triangles
    /// The values in this table refer to vertices in the above table
    size_t ibufCount = 3 * 4;
    unsigned short* faces = (unsigned short*)malloc(ibufCount * sizeof(unsigned short));
    faces[0] = 0;
    faces[1] = 1;
    faces[2] = 2;
    faces[3] = 0;
    faces[4] = 2;
    faces[5] = 3;
    faces[6] = 0;
    faces[7] = 2;
    faces[8] = 1;
    faces[9] = 0;
    faces[10] = 3;
    faces[11] = 2;

    //set coords
    covertices[0].vertex = Vector3(0, 0, 0);
    covertices[1].vertex = Vector3(width, 0, 0);
    covertices[2].vertex = Vector3(width, 0, length);
    covertices[3].vertex = Vector3(0, 0, length);

    covertices[0].normal = Vector3(0, 1, 0);
    covertices[1].normal = Vector3(0, 1, 0);
    covertices[2].normal = Vector3(0, 1, 0);
    covertices[3].normal = Vector3(0, 1, 0);

    /// Create vertex data structure for vertices shared between submeshes
    msh->sharedVertexData = new VertexData();
    msh->sharedVertexData->vertexCount = nVertices;

    /// Create declaration (memory format) of vertex data
    VertexDeclaration* decl = msh->sharedVertexData->vertexDeclaration;
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
    HardwareVertexBufferSharedPtr vbuf =
        HardwareBufferManager::getSingleton().createVertexBuffer(
            offset, msh->sharedVertexData->vertexCount, HardwareBuffer::HBU_DYNAMIC_WRITE_ONLY_DISCARDABLE);

    /// Upload the vertex data to the card
    vbuf->writeData(0, vbuf->getSizeInBytes(), vertices, true);

    /// Set vertex buffer binding so buffer 0 is bound to our vertex buffer
    VertexBufferBinding* bind = msh->sharedVertexData->vertexBufferBinding;
    bind->setBinding(0, vbuf);

    /// Allocate index buffer of the requested number of vertices (ibufCount)
    HardwareIndexBufferSharedPtr faceibuf = HardwareBufferManager::getSingleton().
        createIndexBuffer(
            HardwareIndexBuffer::IT_16BIT,
            ibufCount,
            HardwareBuffer::HBU_STATIC_WRITE_ONLY);

    /// Upload the index data to the card
    faceibuf->writeData(0, faceibuf->getSizeInBytes(), faces, true);

    /// Set parameters of the submesh
    sub->useSharedVertices = true;
    sub->indexData->indexBuffer = faceibuf;
    sub->indexData->indexCount = ibufCount;
    sub->indexData->indexStart = 0;

    /// Set bounding information (for culling)
    msh->_setBounds(AxisAlignedBox(-1, -1, 0, 1, 1, 0), true);
    //msh->_setBoundingSphereRadius(Math::Sqrt(1*1+1*1));

    /// Notify Mesh object that it has been loaded
    msh->load();

    // create the entity and scene node
    char entname[256];
    sprintf(entname, "airbrakenode-%s-%i", basename, num);
    ec = App::GetGfxScene()->GetSceneManager()->createEntity(entname, meshname, actor->GetGfxActor()->GetResourceGroup());
    snode = App::GetGfxScene()->GetSceneManager()->getRootSceneNode()->createChildSceneNode();
    snode->attachObject(ec);

    updatePosition(0.0);

    free(vertices);
    free(faces);
}

void Airbrake::updatePosition(float amount)
{
    ratio = amount;
    // Visual update moved from here to `GfxActor::UpdateAirbrakes()`
}

void Airbrake::applyForce()
{
    //tropospheric model valid up to 11.000m (33.000ft)
    float altitude = m_actor->ar_nodes[noderef].AbsPosition.y;
    //float sea_level_temperature=273.15+15.0; //in Kelvin
    float sea_level_pressure = 101325; //in Pa
    //float airtemperature=sea_level_temperature-altitude*0.0065; //in Kelvin
    float airpressure = sea_level_pressure * pow(1.0 - 0.0065 * altitude / 288.15, 5.24947); //in Pa
    float airdensity = airpressure * 0.0000120896;//1.225 at sea level

    Vector3 wind = -m_actor->ar_nodes[noderef].Velocity;
    float wspeed = wind.length();

    Vector3 drag = (1.2 * area * sin(fabs(ratio * maxangle / 57.3)) * 0.5 * airdensity * wspeed / 4.0) * wind;
    m_actor->ar_nodes[noderef].Forces += drag;
    m_actor->ar_nodes[nodex].Forces += drag;
    m_actor->ar_nodes[nodey].Forces += drag;
    m_actor->ar_nodes[nodea].Forces += drag;
}
