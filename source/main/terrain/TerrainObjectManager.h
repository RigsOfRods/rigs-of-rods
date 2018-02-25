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

#ifdef USE_PAGED
#include "BatchPage.h"
#include "GrassLoader.h"
#include "ImpostorPage.h"
#include "PagedGeometry.h"
#include "TreeLoader2D.h"
#include "TreeLoader3D.h"
#endif //USE_PAGED

class TerrainObjectManager : public ZeroedMemoryAllocator
{
public:

    TerrainObjectManager(TerrainManager* terrainManager);
    ~TerrainObjectManager();

    void loadObjectConfigFile(Ogre::String filename);

    void loadObject(const Ogre::String& name, const Ogre::Vector3& pos, const Ogre::Vector3& rot, Ogre::SceneNode* bakeNode, const Ogre::String& instancename, const Ogre::String& type, bool enable_collisions = true, int scripthandler = -1, bool uniquifyMaterial = false);
    void moveObjectVisuals(const Ogre::String& instancename, const Ogre::Vector3& pos);
    void unloadObject(const Ogre::String& instancename);

    void loadPreloadedTrucks();
    bool hasPreloadedTrucks() { return !truck_preload.empty(); };

    void postLoad();

    bool updateAnimatedObjects(float dt);

    typedef struct localizer_t
    {
        int type;
        Ogre::Vector3 position;
        Ogre::Quaternion rotation;
    } localizer_t;

    typedef struct object_t
    {
        Ogre::String name;
        Ogre::Vector3 position;
        Ogre::Vector3 rotation;
        Ogre::Vector3 initial_position;
        Ogre::Vector3 initial_rotation;
        Ogre::SceneNode* node;
    } object_t;

    std::vector<object_t> getObjects() { return objects; };
	std::vector<localizer_t> GetLocalizers() { return localizers; }

    bool update(float dt);

protected:

    TerrainManager* terrainManager;

    typedef struct
    {
        Ogre::Entity* ent;
        Ogre::SceneNode* node;
        Ogre::AnimationState* anim;
        float speedfactor;
    } animated_object_t;

    Ogre::StaticGeometry* bakesg;
    ProceduralManager* proceduralManager;

    Road* road;
    Ogre::SceneNode* bakeNode;

    typedef struct
    {
        float px;
        float py;
        float pz;
        Ogre::Quaternion rotation;
        char name[256];
        bool ismachine;
        bool freePosition;
    } truck_prepare_t;

    std::vector<truck_prepare_t> truck_preload;

    bool background_loading;
    bool use_rt_shader_system;

#ifdef USE_PAGED
    typedef struct
    {
        Forests::PagedGeometry* geom;
        void* loader;
    } paged_geometry_t;

    std::vector<paged_geometry_t> pagedGeometry;
    Forests::TreeLoader2D* treeLoader;
#endif //USE_PAGED

    std::vector<localizer_t> localizers;

    int objcounter;

    std::vector<animated_object_t> animatedObjects;
    std::vector<MeshObject*> meshObjects;

    typedef struct loadedObject_t
    {
        Ogre::SceneNode* sceneNode;
        Ogre::String instanceName;
        bool enabled;
        std::vector<int> collBoxes;
        std::vector<int> collTris;
    } loadedObject_t;

    std::map<std::string, loadedObject_t> loadedObjects;

    std::vector<object_t> objects;
};

