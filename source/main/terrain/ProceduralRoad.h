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

/// @addtogroup Terrain
/// @{

// dynamic roads
class ProceduralRoad : public ZeroedMemoryAllocator
{
public:

    ProceduralRoad(int id);
    ~ProceduralRoad();

    void addBlock(Ogre::Vector3 pos, Ogre::Quaternion rot, int type, float width, float bwidth, float bheight, int pillartype = 1);
    /**
     * @param p1 Top left point.
     * @param p2 Top right point.
     */
    void addQuad(Ogre::Vector3 p1, Ogre::Vector3 p2, Ogre::Vector3 p3, Ogre::Vector3 p4, int texfit, Ogre::Vector3 pos, Ogre::Vector3 lastpos, float width, bool flip = false);
    void addCollisionQuad(Ogre::Vector3 p1, Ogre::Vector3 p2, Ogre::Vector3 p3, Ogre::Vector3 p4, ground_model_t* gm, bool flip = false);
    void createMesh();
    void createMaterial();
    void finish();
    void setCollisionEnabled(bool v) { collision = v; }

    static const unsigned int MAX_VERTEX = 50000;
    static const unsigned int MAX_TRIS = 50000;

    enum RoadType
    {
        ROAD_AUTOMATIC,
        ROAD_FLAT,
        ROAD_LEFT,
        ROAD_RIGHT,
        ROAD_BOTH,
        ROAD_BRIDGE,
        ROAD_MONORAIL
    };

    enum
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

private:

    inline Ogre::Vector3 baseOf(Ogre::Vector3 p);
    void computePoints(Ogre::Vector3* pts, Ogre::Vector3 pos, Ogre::Quaternion rot, int type, float width, float bwidth, float bheight);
    void textureFit(Ogre::Vector3 p1, Ogre::Vector3 p2, Ogre::Vector3 p3, Ogre::Vector3 p4, int texfit, Ogre::Vector2* texc, Ogre::Vector3 pos, Ogre::Vector3 lastpos, float width);

    typedef struct
    {
        Ogre::Vector3 vertex;
        Ogre::Vector3 normal;
        Ogre::Vector2 texcoord;
    } CoVertice_t;

    Ogre::MeshPtr msh; // createMesh();
    Ogre::MaterialPtr mat; // createMaterial();

    Ogre::Vector2 tex[MAX_VERTEX];
    Ogre::Vector3 vertex[MAX_VERTEX];
    int tricount;
    int vertexcount;
    short tris[MAX_TRIS * 3];

    Ogre::Quaternion lastrot;
    Ogre::SceneNode* snode;
    Ogre::Vector3 lastpos;
    bool first;
    float lastbheight;
    float lastbwidth;
    float lastwidth;
    int lasttype;
    int mid;
    bool collision; //!< Register collision triangles?
    std::vector<int> registeredCollTris;
};

/// @} // addtogroup Terrain

} // namespace RoR

