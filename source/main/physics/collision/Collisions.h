/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2009      Lefteris Stamatogiannakis
    Copyright 2013-2017 Petr Ohlidal & contributors

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

#include "BeamData.h" // for collision_box_t
#include "GroundModelFileformat.h"

#include <OgrePrerequisites.h>
#include <OgreString.h>
#include <OgreEntity.h>
#include <OgreVector3.h>
#include <OgreSceneNode.h>
#include <OgreQuaternion.h>

#include <mutex>

struct eventsource_t
{
    char instancename[256];
    char boxname[256];
    Ogre::SceneNode* snode;
    Ogre::Quaternion direction;
    int scripthandler;
    int cbox;
    bool enabled;
};

typedef std::vector<int> cell_t;

// Forward decl
namespace Json { class Value; }

class Collisions : public ZeroedMemoryAllocator
{
public:

    // these are absolute maximums per terrain
    static const int MAX_COLLISION_BOXES = 5000;
    static const int MAX_COLLISION_TRIS = 100000;

private:

    struct hash_t
    {
        unsigned int cellid;
        cell_t* cell;
    };

    struct collision_tri_t
    {
        Ogre::Vector3 a;
        Ogre::Vector3 b;
        Ogre::Vector3 c;
        Ogre::Matrix3 forward;
        Ogre::Matrix3 reverse;
        RoR::GroundModelDef* gm;
        bool enabled;
    };

    static const int MAX_EVENT_SOURCE = 500;

    // this is a power of two, change with caution
    static const int HASH_POWER = 20;
    static const int HASH_SIZE = 1 << HASH_POWER;

    // how many elements per cell? power of 2 minus 2 is better
    static const int CELL_BLOCKSIZE = 126;

    // how many cells in the pool? Increase in case of sparse distribution of objects
    //static const int MAX_CELLS = 10000;
    static const int UNUSED_CELLID = 0xFFFFFFFF;
    static const int UNUSED_CELLELEMENT = 0xFFFFFFFF;

    // terrain size is limited to 327km x 327km:
    static const int CELL_SIZE = 2.0; // we divide through this
    static const int MAXIMUM_CELL = 0x7FFF;

    // collision boxes pool
    collision_box_t collision_boxes[MAX_COLLISION_BOXES];
    collision_box_t* last_called_cbox;
    int free_collision_box;

    // collision tris pool;
    collision_tri_t* collision_tris;
    int free_collision_tri;

    // collision hashtable
    hash_t hashtable[HASH_SIZE];

    // cell pool
    std::vector<cell_t*> cells;

    // ground models
    std::map<Ogre::String, RoR::GroundModelDef> ground_models;

    // event sources
    eventsource_t eventsources[MAX_EVENT_SOURCE];
    int free_eventsource;

    bool permitEvent(int filter);
    bool envokeScriptCallback(collision_box_t* cbox, node_t* node = 0);

    IHeightFinder* hFinder;
    Ogre::ManualObject* debugmo;
    bool debugMode;
    int collision_count;
    int collision_version;
    int largest_cellcount;
    long max_col_tris;
    unsigned int hashmask;

    RoR::GroundModelManager m_ground_models;
    struct LandUse /// Ground surface map, traditionally called "land use map"
    {
        RoR::GroundModelDef** gm_map;
        RoR::GroundModelDef*  gm_outside;  ///< Used outside of map boundaries
        RoR::GroundModelDef*  gm_fallback; ///< Used if all else fails
        RoR::GroundModelDef*  gm_node;     ///< Used for node/box collisions
        size_t                map_size_x;
        size_t                map_size_z;
    } m_land_use;

    void hash_add(int cell_x, int cell_z, int value);
    void hash_free(int cell_x, int cell_z, int value);
    cell_t* hash_find(int cell_x, int cell_z);
    unsigned int hashfunc(unsigned int cellid);

    Ogre::Vector3 calcCollidedSide(const Ogre::Vector3& pos, const Ogre::Vector3& lo, const Ogre::Vector3& hi);

public:

    std::mutex m_scriptcallback_mutex;

    bool forcecam;
    Ogre::Vector3 forcecampos;

    eventsource_t* getEvent(int eventID) { return &eventsources[eventID]; };

    Collisions();
    ~Collisions();

    inline RoR::GroundModelManager* GetGroundModelManager() { return &m_ground_models; }
    RoR::GroundModelDef* GetNamedOrDefaultGM(const char* name_or_null);
    Ogre::Vector3 getPosition(const Ogre::String& inst, const Ogre::String& box);
    Ogre::Quaternion getDirection(const Ogre::String& inst, const Ogre::String& box);
    collision_box_t* getBox(const Ogre::String& inst, const Ogre::String& box);

    eventsource_t* isTruckInEventBox(Beam* truck);

    bool collisionCorrect(Ogre::Vector3* refpos, bool envokeScriptCallbacks = true);
    bool groundCollision(node_t* node, float dt, RoR::GroundModelDef** gm, float* nso = 0);
    bool isInside(Ogre::Vector3 pos, const Ogre::String& inst, const Ogre::String& box, float border = 0);
    bool isInside(Ogre::Vector3 pos, collision_box_t* cbox, float border = 0);
    bool nodeCollision(node_t* node, bool contacted, float dt, float* nso, RoR::GroundModelDef** ogm);

    void clearEventCache();
    void finishLoadingTerrain();
    void printStats();

    int addCollisionBox(Ogre::SceneNode* tenode, bool rotating, bool virt, Ogre::Vector3 pos, Ogre::Vector3 rot, Ogre::Vector3 l, Ogre::Vector3 h, Ogre::Vector3 sr, const Ogre::String& eventname, const Ogre::String& instancename, bool forcecam, Ogre::Vector3 campos, Ogre::Vector3 sc = Ogre::Vector3::UNIT_SCALE, Ogre::Vector3 dr = Ogre::Vector3::ZERO, int event_filter = EVENT_ALL, int scripthandler = -1);
    int addCollisionMesh(Ogre::String meshname, Ogre::Vector3 pos, Ogre::Quaternion q, Ogre::Vector3 scale, RoR::GroundModelDef* gm = 0, std::vector<int>* collTris = 0);
    int addCollisionTri(Ogre::Vector3 p1, Ogre::Vector3 p2, Ogre::Vector3 p3, RoR::GroundModelDef* gm);
    int createCollisionDebugVisualization();
    int enableCollisionTri(int number, bool enable);
    int removeCollisionBox(int number);
    int removeCollisionTri(int number);

    // ground models things
    void LoadGroundModelsConfigFile(std::string filename);
    std::map<Ogre::String, RoR::GroundModelDef>* getGroundModels() { return &ground_models; };
    void SetupGroundModelMap(Json::Value* j_landuse); ///< A.K.A. landusemap
    RoR::GroundModelDef* QueryGroundModelMap(const int x, const int z); ///< A.K.A. landusemap

    void getMeshInformation(Ogre::Mesh* mesh, size_t& vertex_count, Ogre::Vector3* & vertices,
        size_t& index_count, unsigned* & indices,
        const Ogre::Vector3& position = Ogre::Vector3::ZERO,
        const Ogre::Quaternion& orient = Ogre::Quaternion::IDENTITY, const Ogre::Vector3& scale = Ogre::Vector3::UNIT_SCALE);
    void resizeMemory(long newSize);
};

void primitiveCollision(node_t* node, Ogre::Vector3& force, const Ogre::Vector3& velocity, const Ogre::Vector3& normal, float dt, RoR::GroundModelDef* gm, float* nso, float penetration = 0, float reaction = -1.0f);
