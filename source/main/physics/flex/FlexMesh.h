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

#include "Flexable.h"

#include <OgreString.h>
#include <OgreEntity.h>
#include <OgreVector3.h>
#include <OgreMesh.h>
#include <OgreSubMesh.h>
#include <OgreHardwareBuffer.h>

class FlexMesh: public Flexable
{
public:

    FlexMesh(
        Ogre::String const& name,
        node_t* nds,
        int n1,
        int n2,
        int nstart,
        int nrays,
        Ogre::String const& face_material_name,
        Ogre::String const& band_material_name,
        bool rimmed = false,
        float rimratio = 1.f
    );

    ~FlexMesh();

    Ogre::Vector3 updateVertices();
    Ogre::Vector3 updateShadowVertices();

    // Flexable
    bool flexitPrepare() { return true; };
    void flexitCompute();
    Ogre::Vector3 flexitFinal();

    void setVisible(bool visible);

private:

    Ogre::Vector3 flexit_center;

    struct CoVertice_t
    {
        Ogre::Vector3 vertex;
        Ogre::Vector3 normal;
        //	Ogre::Vector3 color;
        Ogre::Vector2 texcoord;
    };

    struct posVertice_t
    {
        Ogre::Vector3 vertex;
    };

    struct norVertice_t
    {
        Ogre::Vector3 normal;
        //	Ogre::Vector3 color;
        Ogre::Vector2 texcoord;
    };

    Ogre::MeshPtr msh;
    Ogre::SubMesh* subface;
    Ogre::SubMesh* subband;
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
    int* nodeIDs;

    size_t bandibufCount;
    size_t faceibufCount;
    unsigned short* facefaces;
    unsigned short* bandfaces;
    node_t* nodes;
    int nbrays;
    bool is_rimmed;
    float rim_ratio;
};
