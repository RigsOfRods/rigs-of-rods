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

/// @file

#pragma once

#include "Application.h"
#include "RefCountingObject.h"
#include "SurveyMapEntity.h"
#include "TerrainEditor.h"

#include <OgreVector3.h>
#include <string>

namespace RoR {

/// @addtogroup Terrain
/// @{

class Terrain : public RefCountingObject<Terrain>
{
public:
    static const int UNLIMITED_SIGHTRANGE = 4999;

    Terrain(CacheEntryPtr entry, Terrn2DocumentPtr def);
    virtual ~Terrain() override;
    bool initialize();
    void dispose();

    /// @name Terrain info
    /// @{
    std::string             getTerrainName() const;
    std::string             getTerrainFileName();
    std::string             getTerrainFileResourceGroup();
    std::string             getGUID() const;
    int                     getCategoryID() const;
    int                     getVersion() const;
    CacheEntryPtr           getCacheEntry();
    /// @}

    /// @name Terrain properties
    /// @{
    Terrn2DocumentPtr       GetDef();
    Ogre::Vector3           getSpawnPos();
    float                   getWaterHeight() const;
    bool                    isFlat();
    float                   getPagedDetailFactor() const  { return m_paged_detail_factor; }
    void                    addSurveyMapEntity(const std::string& type, const std::string& filename, const std::string& resource_group, const std::string& caption, const Ogre::Vector3& pos, float angle, int id);
    void                    delSurveyMapEntities(int id);
    SurveyMapEntityVec&     getSurveyMapEntities();
    /// @}

    /// @name Subsystems
    /// @{
    TerrainGeometryManager* getGeometryManager()          { return m_geometry_manager; }
    ProceduralManagerPtr    getProceduralManager();
    TerrainObjectManager*   getObjectManager()            { return m_object_manager; }
    HydraxWater*            getHydraxManager()            { return m_hydrax_water; }
    SkyManager*             getSkyManager();
    SkyXManager*            getSkyXManager()              { return SkyX_manager; }
    ShadowManager*          getShadowManager()            { return m_shadow_manager; }
    TerrainEditor*          GetTerrainEditor()            { return &m_terrain_editor; }
    Collisions*             GetCollisions()               { return m_collisions; }
    IWater*                 getWater()                    { return m_water.get(); }
    /// @}

    /// @name Visuals
    /// @{
    Ogre::Light*            getMainLight()                { return m_main_light; }
    int                     getFarClip() const            { return m_sight_range; }
    /// @}

    /// @name Simulation
    /// @{
    void                    setGravity(float value);
    float                   getGravity() const            { return m_cur_gravity; }
    float                   GetHeightAt(float x, float z);
    Ogre::Vector3           GetNormalAt(float x, float y, float z);
    Ogre::Vector3           getMaxTerrainSize();
    Ogre::AxisAlignedBox    getTerrainCollisionAAB();
    /// @}

    /// @name Utility
    /// @{
    void                    LoadTelepoints();
    void                    LoadPredefinedActors();
    bool                    HasPredefinedActors();
    /// @}

private:

    // internal methods
    void initCamera();
    void initTerrainCollisions();
    void initFog();
    void initLight();
    void initObjects();
    void initScripting();
    void initAiPresets();
    void initShadows();
    void initSkySubSystem();
    void initVegetation();
    void initWater();

    void fixCompositorClearColor();
    void loadTerrainObjects();

    // Managers

    TerrainObjectManager*   m_object_manager;
    TerrainGeometryManager* m_geometry_manager;
    std::unique_ptr<IWater> m_water;
    TerrainEditor           m_terrain_editor;
    Collisions*             m_collisions;
    ShadowManager*          m_shadow_manager;
    SkyManager*             m_sky_manager;
    SkyXManager*            SkyX_manager;
    HydraxWater*            m_hydrax_water;

    // Properties

    CacheEntryPtr           m_cache_entry;
    Terrn2DocumentPtr       m_def;
    float                   m_paged_detail_factor;
    int                     m_sight_range;

    // Gameplay

    Ogre::Light*            m_main_light;
    float                   m_cur_gravity;
    bool                    m_disposed = false;
};

/// @} // addtogroup Terrain

} // namespace RoR
