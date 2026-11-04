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

namespace RoR {

/// @addtogroup Gfx
/// @{
    
/// @addtogroup Flex
/// @{    

class FlexAirfoilMesh
{
public:
    FlexAirfoilMesh(ActorPtr& actor, WingID_t wingid,
        std::string const & texband,
        Ogre::Vector2 texlf, Ogre::Vector2 texrf, Ogre::Vector2 texlb, Ogre::Vector2 texrb);

    ~FlexAirfoilMesh();
    
    // DEV NOTE: original `FlexAirfoil::updateVertices()` updated both physics state + visuals.
    Ogre::Vector3 updateVerticesGfx();
    
    
private:

    ActorPtr m_actor;
    WingID_t m_wingid;
    
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
};

/// @} // addtogroup Flex

/// @} // addtogroup Gfx

} // namespace RoR