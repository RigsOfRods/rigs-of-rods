/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2013-2020 Petr Ohlidal

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
#include "RefCountingObject.h"
#include "TerrainEditor.h"
#include "Terrn2FileFormat.h"

#include <OgreVector3.h>
#include <string>

namespace RoR {

/// @addtogroup Terrain
/// @{

class Terrain : public ZeroedMemoryAllocator, public RefCountingObject<Terrain>
{
public:
    static const int UNLIMITED_SIGHTRANGE = 4999;

    Terrain(CacheEntry* entry, Terrn2Def def);
    virtual ~Terrain() override;
    bool initialize();
    void dispose();

    /// @name Terrain info
    /// @{
    std::string             getTerrainName() const        { return m_def.name; }
    std::string             getTerrainFileName();
    std::string             getTerrainFileResourceGroup();
    std::string             getGUID() const               { return m_def.guid; }
    int                     getCategoryID() const         { return m_def.category_id; }
    int                     getVersion() const            { return m_def.version; }
    const CacheEntry*       getCacheEntry()               { return m_cache_entry; }
    /// @}

    /// @name Terrain properties
    /// @{
    Terrn2Def&              GetDef()                      { return m_def; }
    Ogre::Vector3           getSpawnPos()                 { return m_def.start_position; }
    float                   getWaterHeight() const        { return m_def.water_height; }
    bool                    isFlat();
    float                   getPagedDetailFactor() const  { return m_paged_detail_factor; }
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
    void                    HandleException(const char* summary);
    /// @}

private:

    // internal methods
    void initCamera();
    void initTerrainCollisions();
    void initFog();
    void initLight();
    void initObjects();
    void initScripting();
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

    const CacheEntry*       m_cache_entry;
    RoR::Terrn2Def          m_def;
    float                   m_paged_detail_factor;
    int                     m_sight_range;

    // Gameplay

    Ogre::Light*            m_main_light;
    float                   m_cur_gravity;
    bool                    m_disposed = false;
};

/// @} // addtogroup Terrain

} // namespace RoR
