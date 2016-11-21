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

#pragma once

#include "RoRPrerequisites.h"

#include "FlexMesh.h"

#include <OgreString.h>
#include <OgreEntity.h>
#include <OgreVector3.h>
#include <OgreMesh.h>
#include <OgreSubMesh.h>
#include <OgreHardwareBuffer.h>

class FlexMeshWheel: public Flexable
{
public:

    FlexMeshWheel(
        Ogre::String const& name,
        node_t* nds,
        int axis_node_1_index,
        int axis_node_2_index,
        int nstart,
        int nrays,
        Ogre::String const& mesh_name,
        Ogre::String const& material_name,
        float rimradius,
        bool rimreverse,
        MaterialFunctionMapper* material_function_mapper,
        Skin* used_skin,
        MaterialReplacer* material_replacer
    );

    ~FlexMeshWheel();

    Ogre::Entity* getRimEntity() { return rimEnt; };

    Ogre::Vector3 updateVertices();
    Ogre::Vector3 updateShadowVertices();

    // Flexable
    bool flexitPrepare();
    void flexitCompute();
    Ogre::Vector3 flexitFinal();

    void setVisible(bool visible);

private:

    Ogre::Vector3 flexit_center;

    MaterialReplacer* mr;

    struct CoVertice_t
    {
        Ogre::Vector3 vertex;
        Ogre::Vector3 normal;
        //Ogre::Vector3 color;
        Ogre::Vector2 texcoord;
    };

    struct posVertice_t
    {
        Ogre::Vector3 vertex;
    };

    struct norVertice_t
    {
        Ogre::Vector3 normal;
        Ogre::Vector2 texcoord;
    };

    Ogre::MeshPtr msh;
    Ogre::SubMesh* sub;
    Ogre::VertexDeclaration* decl;
    Ogre::HardwareVertexBufferSharedPtr vbuf;

    size_t nVertices;
    size_t vbufCount;

    //shadow
    union
    {
        float* shadowposvertices;
        posVertice_t* coshadowposvertices;
    };

    union
    {
        float* shadownorvertices;
        norVertice_t* coshadownorvertices;
    };

    union
    {
        float* vertices;
        CoVertice_t* covertices;
    };

    //nodes
    //int *nodeIDs;
    int id0;
    int id1;
    int idstart;

    size_t ibufCount;
    unsigned short* faces;
    node_t* nodes;
    int nbrays;
    float rim_radius;
    Ogre::SceneNode* rnode;
    float normy;
    bool revrim;
    Ogre::Entity* rimEnt;
};
