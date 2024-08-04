/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2013-2023 Petr Ohlidal

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

#include "ODefFileFormat.h"
#include "MeshObject.h"
#include "ProceduralManager.h"
#include "SurveyMapEntity.h"

#ifdef USE_PAGED
#include "PagedGeometry.h"
#include "TreeLoader2D.h"
#include "TreeLoader3D.h"
#include "BatchPage.h"
#include "GrassLoader.h"
#include "ImpostorPage.h"
#endif //USE_PAGED

#include <map>
#include <unordered_map>

namespace RoR {

/// @addtogroup Terrain
/// @{

class TerrainObjectManager
{
    friend class Terrain;
public:

    struct EditorObject
    {
        Ogre::String name;
        Ogre::String instance_name;
        Ogre::String type;
        Ogre::Vector3 position = Ogre::Vector3::ZERO;
        Ogre::Vector3 rotation = Ogre::Vector3::ZERO;
        Ogre::Vector3 initial_position = Ogre::Vector3::ZERO;
        Ogre::Vector3 initial_rotation = Ogre::Vector3::ZERO;
        Ogre::SceneNode* node = nullptr;
        bool enable_collisions = true;
        int script_handler = -1;
    };

    TerrainObjectManager(Terrain* terrainManager);
    ~TerrainObjectManager();

    std::vector<EditorObject>& GetEditorObjects() { return m_editor_objects; }
    void           LoadTObjFile(Ogre::String filename);
    bool           LoadTerrainObject(const Ogre::String& name, const Ogre::Vector3& pos, const Ogre::Vector3& rot, const Ogre::String& instancename, const Ogre::String& type, float rendering_distance = 0, bool enable_collisions = true, int scripthandler = -1, bool uniquifyMaterial = false);
    bool           LoadTerrainScript(const Ogre::String& filename);
    void           MoveObjectVisuals(const Ogre::String& instancename, const Ogre::Vector3& pos);
    void           unloadObject(const Ogre::String& instancename);
    void           LoadTelepoints();
    void           LoadPredefinedActors();
    bool           HasPredefinedActors() { return !m_predefined_actors.empty(); };
    bool           UpdateTerrainObjects(float dt);

    void ProcessTree(
        float yawfrom, float yawto,
        float scalefrom, float scaleto,
        char* ColorMap, char* DensityMap, char* treemesh, char* treeCollmesh,
        float gridspacing, float highdens,
        int minDist, int maxDist, int mapsizex, int mapsizez);

    void ProcessGrass(
        float SwaySpeed, float SwayLength, float SwayDistribution, float Density,
        float minx, float miny, float minH, float maxx, float maxy, float maxH,
        char* grassmat, char* colorMapFilename, char* densityMapFilename,
        int growtechnique, int techn, int range, int mapsizex, int mapsizez);

    struct localizer_t
    {
        LocalizerType type;
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

    std::vector<localizer_t> GetLocalizers() { return localizers; }

    ProceduralManagerPtr& getProceduralManager() { return m_procedural_manager; }
    Ogre::SceneNode* getGroupingSceneNode();

protected:

    struct AnimatedObject
    {
        Ogre::Entity* ent;
        Ogre::SceneNode* node;
        Ogre::AnimationState* anim;
        float speedfactor;
    };

    struct ParticleEffectObject
    {
        Ogre::ParticleSystem* psys = nullptr;
        Ogre::SceneNode* node = nullptr;
    };

    struct PredefinedActor
    {
        float px;
        float py;
        float pz;
        Ogre::Quaternion rotation;
        std::string name;
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

    // ODef processing functions

    RoR::ODefFile* FetchODef(std::string const & odef_name);
    void           ProcessODefCollisionBoxes(StaticObject* obj, ODefFile* odef, const EditorObject& params, bool race_event);

    // Update functions

    void           UpdateAnimatedObjects(float dt);
    void           UpdateParticleEffectObjects();

    // Variables

    std::vector<localizer_t> localizers;
    std::unordered_map<std::string, std::shared_ptr<RoR::ODefFile>> m_odef_cache;
    std::map<std::string, StaticObject>   m_static_objects;
    std::vector<EditorObject>             m_editor_objects;
    std::vector<PredefinedActor>          m_predefined_actors;
    std::vector<AnimatedObject>           m_animated_objects;
    std::vector<ParticleEffectObject>     m_particle_effect_objects;
    std::vector<MeshObject*>              m_mesh_objects;
    SurveyMapEntityVec                    m_map_entities;
    Terrain*           terrainManager;
    ProceduralManagerPtr      m_procedural_manager;
    int                       m_entity_counter = 0;
    Ogre::SceneNode*          m_terrn2_grouping_node = nullptr; //!< For a readable scene graph (via inspector script)
    Ogre::SceneNode*          m_tobj_grouping_node = nullptr; //!< For even more readable scene graph (via inspector script)
    Ogre::SceneNode*          m_angelscript_grouping_node = nullptr; //!< For even more readable scene graph (via inspector script)

#ifdef USE_PAGED
    std::vector<Forests::PagedGeometry*> m_paged_geometry;
#endif //USE_PAGED
};

/// @} // addtogroup Terrain

} // namespace RoR
