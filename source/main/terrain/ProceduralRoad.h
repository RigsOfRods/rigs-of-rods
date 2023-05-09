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
#include "RefCountingObject.h"

namespace RoR {

/// @addtogroup Terrain
/// @{

enum class RoadType
{
    ROAD_AUTOMATIC,
    ROAD_FLAT,
    ROAD_LEFT,
    ROAD_RIGHT,
    ROAD_BOTH,
    ROAD_BRIDGE,
    ROAD_MONORAIL
};

enum class TextureFit
{
    TEXFIT_NONE,
    TEXFIT_BRICKWALL,
    TEXFIT_ROADS1,
    TEXFIT_ROADS2,
    TEXFIT_ROAD,
    TEXFIT_ROADS3,
    TEXFIT_ROADS4,
    TEXFIT_CONCRETEWALL,
    TEXFIT_CONCRETEWALLI,
    TEXFIT_CONCRETETOP,
    TEXFIT_CONCRETEUNDER
};

// dynamic roads
class ProceduralRoad : public RefCountingObject<ProceduralRoad>
{
public:

    ProceduralRoad();
    virtual ~ProceduralRoad() override;

    void addBlock(Ogre::Vector3 pos, Ogre::Quaternion rot, RoadType type, float width, float bwidth, float bheight, int pillartype = 1);
    /**
     * @param p1 Top left point.
     * @param p2 Top right point.
     */
    void addQuad(Ogre::Vector3 p1, Ogre::Vector3 p2, Ogre::Vector3 p3, Ogre::Vector3 p4, TextureFit texfit, Ogre::Vector3 pos, Ogre::Vector3 lastpos, float width, bool flip = false);
    void addCollisionQuad(Ogre::Vector3 p1, Ogre::Vector3 p2, Ogre::Vector3 p3, Ogre::Vector3 p4, ground_model_t* gm, bool flip = false);
    void addCollisionQuad(Ogre::Vector3 p1, Ogre::Vector3 p2, Ogre::Vector3 p3, Ogre::Vector3 p4, std::string const& gm_name, bool flip = false);
    void createMesh();
    void finish();
    void setCollisionEnabled(bool v) { collision = v; }

    static const unsigned int MAX_VERTEX = 50000;
    static const unsigned int MAX_TRIS = 50000;

private:

    inline Ogre::Vector3 baseOf(Ogre::Vector3 p);
    void computePoints(Ogre::Vector3* pts, Ogre::Vector3 pos, Ogre::Quaternion rot, RoadType type, float width, float bwidth, float bheight);
    void textureFit(Ogre::Vector3 p1, Ogre::Vector3 p2, Ogre::Vector3 p3, Ogre::Vector3 p4, TextureFit texfit, Ogre::Vector2* texc, Ogre::Vector3 pos, Ogre::Vector3 lastpos, float width);

    struct CoVertice_t
    {
        Ogre::Vector3 vertex;
        Ogre::Vector3 normal;
        Ogre::Vector2 texcoord;
    };

    Ogre::MeshPtr msh;
    Ogre::SubMesh* mainsub = nullptr;

    Ogre::Vector2 tex[MAX_VERTEX] = {};
    Ogre::Vector3 vertex[MAX_VERTEX] = {};
    int tricount = 0;
    int vertexcount = 0;
    uint16_t tris[MAX_TRIS * 3] = {};

    Ogre::Quaternion lastrot;
    Ogre::SceneNode* snode = nullptr;
    Ogre::Vector3 lastpos;
    bool first = true;
    float lastbheight = 0.f;
    float lastbwidth = 0.f;
    float lastwidth = 0.f;
    RoadType lasttype;
    int mid = 0;
    bool collision = true; //!< Register collision triangles?
    std::vector<int> registeredCollTris;
};

/// @} // addtogroup Terrain

} // namespace RoR

