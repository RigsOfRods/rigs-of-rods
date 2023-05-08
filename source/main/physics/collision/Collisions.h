/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2009      Lefteris Stamatogiannakis
    Copyright 2013-2022 Petr Ohlidal

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

#include "Application.h"
#include "SimData.h" // for collision_box_t

#include <mutex>
#include <Ogre.h>
#include <string>

namespace RoR {

/// @addtogroup Physics
/// @{

/// @addtogroup Collisions
/// @{

struct eventsource_t //!< Scripting
{
    std::string       es_instance_name;  //!< Specified by user when calling "GameScript::spawnObject()"
    std::string       es_box_name;       //!< Specified in ODEF file as "event"
    Ogre::Quaternion  es_direction;
    int               es_script_handler; //!< AngelScript function ID
    int               es_cbox;           //!< Collision box ID
    bool              es_enabled;
};

struct collision_tri_t
{
    Ogre::Vector3 a;
    Ogre::Vector3 b;
    Ogre::Vector3 c;
    Ogre::AxisAlignedBox aab;
    Ogre::Matrix3 forward;
    Ogre::Matrix3 reverse;
    ground_model_t* gm;
    bool enabled;
};
typedef std::vector<collision_tri_t> CollisionTriVec;

/// Records which collision triangles belong to which mesh.
struct collision_mesh_t
{
    std::string mesh_name;
    std::string source_name;
    Ogre::Vector3 position = Ogre::Vector3::ZERO;
    Ogre::Quaternion orientation = Ogre::Quaternion::IDENTITY;
    Ogre::Vector3 scale = Ogre::Vector3::ZERO;
    Ogre::AxisAlignedBox bounding_box;
    ground_model_t* ground_model = nullptr;
    int collision_tri_start = -1;
    int collision_tri_count = 0;
    int num_verts = 0;
    int num_indices = 0;
};
typedef std::vector<collision_mesh_t> CollisionMeshVec;

class Collisions
{
public:

    typedef std::vector<collision_box_t> CollisionBoxVec;

    enum SurfaceType
    {
        FX_NONE,
        FX_HARD, // hard surface: rubber burning and sparks
        FX_DUSTY, // dusty surface (with dust colour)
        FX_CLUMPY, // throws clumps (e.g. snow, grass) with colour
        FX_PARTICLE
    };

    Collisions(Ogre::Vector3 terrn_size);
    ~Collisions();

private:

    /// Static collision object lookup system
    /// -------------------------------------
    /// Terrain is split into equal-size 'cells' of dimension CELL_SIZE, identified by CellID
    /// A hash table aggregates elements from multiple cells in one entry
    struct hash_coll_element_t
    {
        static const int ELEMENT_TRI_BASE_INDEX = 1000000; // Effectively a maximum number of collision boxes

        inline hash_coll_element_t(unsigned int cell_id_, int value): cell_id(cell_id_), element_index(value) {}

        inline bool IsCollisionBox() const { return element_index < ELEMENT_TRI_BASE_INDEX; }
        inline bool IsCollisionTri() const { return element_index >= ELEMENT_TRI_BASE_INDEX; }

        unsigned int cell_id;

        /// Values below ELEMENT_TRI_BASE_INDEX are collision box indices (Collisions::m_collision_boxes),
        ///    values above are collision tri indices (Collisions::m_collision_tris).
        int element_index;
    };

    static const int LATEST_GROUND_MODEL_VERSION = 3;
    static const int MAX_EVENT_SOURCE = 500;

    // this is a power of two, change with caution
    static const int HASH_POWER = 20;
    static const int HASH_SIZE = 1 << HASH_POWER;

    // terrain size is limited to 327km x 327km:
    static const int CELL_SIZE = 2.0; // we divide through this
    static const int MAXIMUM_CELL = 0x7FFF;

    // collision boxes pool
    CollisionBoxVec m_collision_boxes; // Formerly MAX_COLLISION_BOXES = 5000
    std::vector<collision_box_t*> m_last_called_cboxes; // Only used for character, actors have their own cache `Actor::m_active_eventboxes`

    // collision tris pool
    CollisionTriVec m_collision_tris; // Formerly MAX_COLLISION_TRIS = 100000
    CollisionMeshVec m_collision_meshes; // For diagnostics/editing only.

    Ogre::AxisAlignedBox m_collision_aab; // Tight bounding box around all collision meshes

    // collision hashtable
    Ogre::Real hashtable_height[HASH_SIZE];
    std::vector<hash_coll_element_t> hashtable[HASH_SIZE];

    // ground models
    std::map<Ogre::String, ground_model_t> ground_models;

    // event sources
    eventsource_t eventsources[MAX_EVENT_SOURCE];
    int free_eventsource;

    bool permitEvent(ActorPtr actor, CollisionEventFilter filter);

    Landusemap* landuse;
    int collision_version;
    unsigned int hashmask;

    const Ogre::Vector3 m_terrain_size;

    void hash_add(int cell_x, int cell_z, int value, float h);
    int hash_find(int cell_x, int cell_z); /// Returns index to 'hashtable'
    unsigned int hashfunc(unsigned int cellid);
    void parseGroundConfig(Ogre::ConfigFile* cfg, Ogre::String groundModel = "");

    Ogre::Vector3 calcCollidedSide(const Ogre::Vector3& pos, const Ogre::Vector3& lo, const Ogre::Vector3& hi);

public:

    // how many elements per cell? power of 2 minus 2 is better
    static const int CELL_BLOCKSIZE = 126;

    std::mutex m_scriptcallback_mutex;

    bool forcecam;
    Ogre::Vector3 forcecampos;
    ground_model_t *defaultgm, *defaultgroundgm;

    Ogre::Vector3 getPosition(const Ogre::String& inst, const Ogre::String& box);
    Ogre::Quaternion getDirection(const Ogre::String& inst, const Ogre::String& box);
    collision_box_t* getBox(const Ogre::String& inst, const Ogre::String& box);

    std::pair<bool, Ogre::Real> intersectsTris(Ogre::Ray ray);

    float getSurfaceHeight(float x, float z);
    float getSurfaceHeightBelow(float x, float z, float height);
    bool collisionCorrect(Ogre::Vector3* refpos, bool envokeScriptCallbacks = true);
    bool groundCollision(node_t* node, float dt);
    bool isInside(Ogre::Vector3 pos, const Ogre::String& inst, const Ogre::String& box, float border = 0);
    bool isInside(Ogre::Vector3 pos, collision_box_t* cbox, float border = 0);
    bool nodeCollision(node_t* node, float dt);
    void envokeScriptCallback(collision_box_t* cbox, node_t* node = 0);
    void findPotentialEventBoxes(Actor* actor, CollisionBoxPtrVec& out_boxes);

    void finishLoadingTerrain();

    int addCollisionBox(bool rotating, bool virt, Ogre::Vector3 pos, Ogre::Vector3 rot, Ogre::Vector3 l, Ogre::Vector3 h, Ogre::Vector3 sr, const Ogre::String& eventname, const Ogre::String& instancename, bool forcecam, Ogre::Vector3 campos, Ogre::Vector3 sc = Ogre::Vector3::UNIT_SCALE, Ogre::Vector3 dr = Ogre::Vector3::ZERO, CollisionEventFilter event_filter = EVENT_ALL, int scripthandler = -1);
    void addCollisionMesh(Ogre::String const& srcname, Ogre::String const& meshname, Ogre::Vector3 const& pos, Ogre::Quaternion const& q, Ogre::Vector3 const& scale, ground_model_t* gm = 0, std::vector<int>* collTris = 0); //!< generate collision tris from existing mesh resource
    void registerCollisionMesh(Ogre::String const& srcname, Ogre::String const& meshname, Ogre::Vector3 const& pos, Ogre::AxisAlignedBox bounding_box, ground_model_t* gm, int ctri_start, int ctri_count); //!< Mark already generated collision tris as belonging to (virtual) mesh.
    int addCollisionTri(Ogre::Vector3 p1, Ogre::Vector3 p2, Ogre::Vector3 p3, ground_model_t* gm);
    void createCollisionDebugVisualization(Ogre::SceneNode* root_node, Ogre::AxisAlignedBox const& area_limit, std::vector<Ogre::SceneNode*>& out_nodes);
    void removeCollisionBox(int number);
    void removeCollisionTri(int number);
    void clearEventCache() { m_last_called_cboxes.clear(); }

    Ogre::AxisAlignedBox getCollisionAAB() { return m_collision_aab; };

    // ground models things
    int loadDefaultModels();
    int loadGroundModelsConfigFile(Ogre::String filename);
    std::map<Ogre::String, ground_model_t>* getGroundModels() { return &ground_models; };
    void setupLandUse(const char* configfile);
    ground_model_t* getGroundModelByString(const Ogre::String name);

    void getMeshInformation(Ogre::Mesh* mesh, size_t& vertex_count, Ogre::Vector3* & vertices,
        size_t& index_count, unsigned* & indices,
        const Ogre::Vector3& position = Ogre::Vector3::ZERO,
        const Ogre::Quaternion& orient = Ogre::Quaternion::IDENTITY, const Ogre::Vector3& scale = Ogre::Vector3::UNIT_SCALE);

    // Read-only (const) getters.
    eventsource_t const& getEventSource(int pos) const { ROR_ASSERT(pos < free_eventsource); return eventsources[pos]; }
    CollisionBoxVec const& getCollisionBoxes() const { return m_collision_boxes; }
    CollisionMeshVec const& getCollisionMeshes() const { return m_collision_meshes; }
    CollisionTriVec const& getCollisionTriangles() const { return m_collision_tris; }
};

Ogre::Vector3 primitiveCollision(node_t* node, Ogre::Vector3 velocity, float mass, Ogre::Vector3 normal, float dt, ground_model_t* gm, float penetration = 0);

/// @} // addtogroup Collisions
/// @} // addtogroup Physics

} // namespace RoR
