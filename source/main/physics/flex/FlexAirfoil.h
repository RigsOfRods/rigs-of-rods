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

/// WARNING: gutted during OGRE2x port
class FlexAirfoil : public ZeroedMemoryAllocator
{
public:
    FlexAirfoil(Ogre::String const& wname, Actor* actor,
        int pnfld, int pnfrd, int pnflu, int pnfru, int pnbld, int pnbrd, int pnblu, int pnbru,
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
    int nfld;
    int nfrd;
    int nflu;
    int nfru;
    int nbld;
    int nbrd;
    int nblu;
    int nbru;

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

} // namespace RoRs
