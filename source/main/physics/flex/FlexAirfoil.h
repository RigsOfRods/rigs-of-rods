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

#include "Application.h"
#include "SimData.h" // For MAX_AEROENGINES

namespace RoR {

/// @addtogroup Physics
/// @{

class FlexAirfoil
{
public:
    FlexAirfoil(Ogre::String const& wname, ActorPtr actor,
        NodeNum_t pnfld, NodeNum_t pnfrd, NodeNum_t pnflu, NodeNum_t pnfru, NodeNum_t pnbld, NodeNum_t pnbrd, NodeNum_t pnblu, NodeNum_t pnbru,
        std::string const & texname,
        Ogre::Vector2 texlf, Ogre::Vector2 texrf, Ogre::Vector2 texlb, Ogre::Vector2 texrb,
        char mtype, float controlratio, float mind, float maxd, Ogre::String const& afname, float lift_coef, bool break_able);

    ~FlexAirfoil();

    // DEV NOTE: original `updateVertices()` updated both physics state + visuals.
    void updateVerticesPhysics();
    Ogre::Vector3 updateVerticesGfx(RoR::GfxActor* gfx_actor);
    void uploadVertices();

    void setControlDeflection(float val);

    void enableInducedDrag(float span, float area, bool l);

    void addwash(int propid, float ratio);

    void updateForces();

    float aoa;
    char type;
    NodeNum_t nfld;
    NodeNum_t nfrd;
    NodeNum_t nflu;
    NodeNum_t nfru;
    NodeNum_t nbld;
    NodeNum_t nbrd;
    NodeNum_t nblu;
    NodeNum_t nbru;

    bool broken;
    bool breakable;
    float liftcoef;

private:

    float airfoilpos[90];

    typedef struct
    {
        Ogre::Vector3 vertex;
        Ogre::Vector3 normal;
        //	Ogre::Vector3 color;
        Ogre::Vector2 texcoord;
    } CoVertice_t;

    Ogre::MeshPtr msh;
    Ogre::SubMesh* subface;
    Ogre::SubMesh* subband;

    Ogre::SubMesh* subcup;
    Ogre::SubMesh* subcdn;

    Ogre::VertexDeclaration* decl;
    Ogre::HardwareVertexBufferSharedPtr vbuf;

    size_t nVertices;
    size_t vbufCount;

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
    ActorPtr m_actor;

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
    int free_wash;
    int washpropnum[MAX_AEROENGINES];
    float washpropratio[MAX_AEROENGINES];
};

/// @} // addtogroup Physics

} // namespace RoRs
