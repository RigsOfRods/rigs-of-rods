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
#include "BatchPage.h"
#include "GrassLoader.h"
#include "ImpostorPage.h"
#include "PagedGeometry.h"
#include "TreeLoader2D.h"
#include "TreeLoader3D.h"

class TerrainObjectManager : public ZeroedMemoryAllocator
{
public:

    struct EditorObject
    {
        Ogre::String name;
        Ogre::Vector3 position;
        Ogre::Vector3 rotation;
        Ogre::Vector3 initial_position;
        Ogre::Vector3 initial_rotation;
        Ogre::SceneNode* node;
    };

    TerrainObjectManager(TerrainManager* terrainManager);
    ~TerrainObjectManager();

    std::vector<EditorObject> GetEditorObjects() const { return m_editor_objects; }
    void           LoadTObjFile(Ogre::String filename);
    void           LoadTerrainObject(const Ogre::String& name, const Ogre::Vector3& pos, const Ogre::Vector3& rot, Ogre::SceneNode* m_staticgeometry_bake_node, const Ogre::String& instancename, const Ogre::String& type, bool enable_collisions = true, int scripthandler = -1, bool uniquifyMaterial = false);
    void           MoveObjectVisuals(const Ogre::String& instancename, const Ogre::Vector3& pos);
    void           unloadObject(const Ogre::String& instancename);
    void           LoadPredefinedActors();
    bool           HasPredefinedActors() { return !m_predefined_actors.empty(); };
    void           PostLoadTerrain();
    bool           UpdateTerrainObjects(float dt);

    typedef struct localizer_t
    {
        int type;
        Ogre::Vector3 position;
        Ogre::Quaternion rotation;
    } localizer_t;

    std::vector<localizer_t> GetLocalizers() { return localizers; }
private:

    struct AnimatedObject
    {
        Ogre::Entity* ent;
        Ogre::SceneNode* node;
        Ogre::AnimationState* anim;
        float speedfactor;
    };

    struct PredefinedActor
    {
        float px;
        float py;
        float pz;
        Ogre::Quaternion rotation;
        char name[256];
        bool ismachine;
        bool freePosition;
    };

    struct StaticObject
    {
        Ogre::SceneNode* sceneNode;
        Ogre::String instanceName;
        bool enabled;
        std::vector<int> collBoxes;
        std::vector<int> collTris;
    };

    struct PaGeomInstance
    {
        Forests::PagedGeometry* geom;
        void* loader;
    };

    bool           UpdateAnimatedObjects(float dt);
    std::vector<localizer_t> localizers;

    std::map<std::string, StaticObject>   m_static_objects;
    std::vector<EditorObject>             m_editor_objects;
    std::vector<PredefinedActor>          m_predefined_actors;
    std::vector<AnimatedObject>           m_animated_objects;
    std::vector<MeshObject*>              m_mesh_objects;
    TerrainManager*           terrainManager;
    Ogre::StaticGeometry*     m_staticgeometry;
    ProceduralManager*        m_procedural_mgr;
    Road*                     m_road;
    Ogre::SceneNode*          m_staticgeometry_bake_node;
    bool                      m_background_loading;
    bool                      m_use_rtshadersystem;

#ifdef USE_PAGED
    std::vector<PaGeomInstance> m_paged_geometry;
    Forests::TreeLoader2D*      m_tree_loader;
#endif //USE_PAGED
};

