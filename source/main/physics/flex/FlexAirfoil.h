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

#pragma once

#include <Ogre.h>

#include "RoRPrerequisites.h"
#include "BeamData.h" // For MAX_AEROENGINES

class FlexAirfoil : public ZeroedMemoryAllocator
{
    friend class RigInspector; // Debug utility class

public:

    FlexAirfoil(Ogre::String const& wname, node_t* nds,
        int pnfld, int pnfrd, int pnflu, int pnfru, int pnbld, int pnbrd, int pnblu, int pnbru,
        std::string const & texname,
        Ogre::Vector2 texlf, Ogre::Vector2 texrf, Ogre::Vector2 texlb, Ogre::Vector2 texrb,
        char mtype, float controlratio, float mind, float maxd, Ogre::String const& afname, float lift_coef, AeroEngine** tps, bool break_able);

    ~FlexAirfoil();

    Ogre::Vector3 updateVertices();
    Ogre::Vector3 updateShadowVertices();
    void setControlDeflection(float val);

    Ogre::Vector3 flexit();

    void enableInducedDrag(float span, float area, bool l);

    void addwash(int propid, float ratio);

    void updateForces();

    float aoa;
    char type;
    int nfld;
    int nfrd;
    int nflu;
    int nfru;
    int nbld;
    int nbrd;
    int nblu;
    int nbru;

    //	int innan;
    bool broken;
    bool breakable;
    float liftcoef;

    char debug[256];

private:

    float airfoilpos[90];

    typedef struct
    {
        Ogre::Vector3 vertex;
        Ogre::Vector3 normal;
        //	Ogre::Vector3 color;
        Ogre::Vector2 texcoord;
    } CoVertice_t;

    typedef struct
    {
        Ogre::Vector3 vertex;
    } posVertice_t;

    typedef struct
    {
        Ogre::Vector3 normal;
        //	Ogre::Vector3 color;
        Ogre::Vector2 texcoord;
    } norVertice_t;

    Ogre::MeshPtr msh;
    Ogre::SubMesh* subface;
    Ogre::SubMesh* subband;

    Ogre::SubMesh* subcup;
    Ogre::SubMesh* subcdn;

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

    size_t faceibufCount;
    size_t bandibufCount;
    size_t cupibufCount;
    size_t cdnibufCount;
    unsigned short* facefaces;
    unsigned short* bandfaces;
    unsigned short* cupfaces;
    unsigned short* cdnfaces;
    node_t* nodes;

    float sref;

    float deflection;
    float chordratio;
    bool hascontrol;
    bool isstabilator;
    bool stabilleft;
    float lratio;
    float rratio;
    float mindef;
    float maxdef;
    float thickness;
    bool useInducedDrag;
    float idSpan;
    float idArea;
    bool idLeft;

    Airfoil* airfoil;
    AeroEngine** aeroengines;
    int free_wash;
    int washpropnum[MAX_AEROENGINES];
    float washpropratio[MAX_AEROENGINES];
};
