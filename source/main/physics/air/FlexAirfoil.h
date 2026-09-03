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
    friend FlexAirfoilMesh;
public:
    FlexAirfoil(Ogre::String const& wname, ActorPtr actor,
        NodeNum_t pnfld, NodeNum_t pnfrd, NodeNum_t pnflu, NodeNum_t pnfru, NodeNum_t pnbld, NodeNum_t pnbrd, NodeNum_t pnblu, NodeNum_t pnbru,
        char mtype, float controlratio, float mind, float maxd, Ogre::String const& afname, float lift_coef, bool break_able);

    ~FlexAirfoil();
    
    // DEV NOTE: original `updateVertices()` updated both physics state + visuals.
    void updateVerticesPhysics();
    
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
    std::string fa_name;
    ActorPtr fa_actor;

    // Exposed for simbuffer & FlexAirfoilMesh
    float airfoilpos[90];
    bool isstabilator;
    bool stabilleft;
    float deflection;

private:

    float sref;

    float chordratio;
    bool hascontrol;
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

} // namespace RoR

