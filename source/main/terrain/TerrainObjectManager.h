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

#include "RoRPrerequisites.h"

#include "ODefFileFormat.h"

#ifdef USE_PAGED
#include "BatchPage.h"
#include "GrassLoader.h"
#include "ImpostorPage.h"
#include "PagedGeometry.h"
#include "TreeLoader2D.h"
#include "TreeLoader3D.h"
#endif //USE_PAGED

// Forward
namespace Json { class Value; }

class TerrainObjectManager : public ZeroedMemoryAllocator
{
public:

    TerrainObjectManager(TerrainManager* terrainManager);
    ~TerrainObjectManager();

    void ProcessTerrainObjects(Json::Value* j_terrn);

    void loadObject(const Ogre::String& name, const Ogre::Vector3& pos, const Ogre::Vector3& rot, Ogre::SceneNode* bakeNode, const Ogre::String& instancename, const Ogre::String& type, bool enable_collisions = true, int scripthandler = -1, bool uniquifyMaterial = false);
    void moveObjectVisuals(const Ogre::String& instancename, const Ogre::Vector3& pos);
    void unloadObject(const Ogre::String& instancename);

    void loadPreloadedTrucks();
    bool hasPreloadedTrucks() { return !truck_preload.empty(); };

    void postLoad();

    bool updateAnimatedObjects(float dt);

    void ProcessTreePage(Json::Value* j_tree_page, int mapsizex, int mapsizez);
    //OLD    float yawfrom, float yawto,
    //OLD    float scalefrom, float scaleto,
    //OLD    char* ColorMap, char* DensityMap, char* treemesh, char* treeCollmesh,
    //OLD    float gridspacing, float highdens,
    //OLD    int minDist, int maxDist, int mapsizex, int mapsizez);

    void ProcessGrass(
        float SwaySpeed, float SwayLength, float SwayDistribution, float Density,
        float minx, float miny, float minH, float maxx, float maxy, float maxH,
        char* grassmat, char* colorMapFilename, char* densityMapFilename,
        int growtechnique, int techn, int range, int mapsizex, int mapsizez);

    struct localizer_t
    {
        int type;
        Ogre::Vector3 position;
        Ogre::Quaternion rotation;
    };

    struct object_t
    {
        Ogre::String name;
        Ogre::Vector3 position;
        Ogre::Vector3 rotation;
        Ogre::Vector3 initial_position;
        Ogre::Vector3 initial_rotation;
        Ogre::SceneNode* node;
    };

    std::vector<object_t> getObjects() { return objects; };

    bool update(float dt);

protected:

    ODefFile* FetchODef(std::string const & odef_name);

    TerrainManager* terrainManager;

    struct animated_object_t
    {
        Ogre::Entity* ent;
        Ogre::SceneNode* node;
        Ogre::AnimationState* anim;
        float speedfactor;
    };

    Ogre::StaticGeometry* bakesg;
    ProceduralManager* proceduralManager;

    Road* road;
    Ogre::SceneNode* bakeNode;

    struct truck_prepare_t
    {
        float px;
        float py;
        float pz;
        Ogre::Quaternion rotation;
        char name[256];
        bool ismachine;
        bool freePosition;
    };

    std::vector<truck_prepare_t> truck_preload;

    bool background_loading;
    bool use_rt_shader_system;

#ifdef USE_PAGED
    struct paged_geometry_t
    {
        Forests::PagedGeometry* geom;
        void* loader;
    };

    std::vector<paged_geometry_t> pagedGeometry;
    Forests::TreeLoader2D* treeLoader;
#endif //USE_PAGED

    localizer_t localizers[64];

    int objcounter;
    int free_localizer;

    std::vector<animated_object_t> animatedObjects;
    std::vector<MeshObject*> meshObjects;

    struct loadedObject_t
    {
        Ogre::SceneNode* sceneNode;
        Ogre::String instanceName;
        bool enabled;
        std::vector<int> collBoxes;
        std::vector<int> collTris;
    };

    std::map<std::string, loadedObject_t> loadedObjects;
    std::unordered_map<std::string, std::shared_ptr<ODefFile>> m_odef_cache;
    std::vector<object_t> objects;
};

