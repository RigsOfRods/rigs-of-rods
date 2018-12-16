/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
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
#include "Terrn2Fileformat.h"

#include <OgreVector3.h>
#include <string>

class TerrainManager : public ZeroedMemoryAllocator
{
public:

    TerrainManager();
    ~TerrainManager();

    void               setGravity(float value);
    std::vector<authorinfo_t>& GetAuthors();
    TerrainGeometryManager* getGeometryManager()     { return m_geometry_manager; };
    TerrainObjectManager* getObjectManager()         { return m_object_manager; };
    float              getGravity() const            { return m_cur_gravity; };
    std::string        getTerrainName() const        { return m_def.name; };
    std::string        getGUID() const               { return m_def.guid; };
    int                getCategoryID() const         { return m_def.category_id; };
    int                getVersion() const            { return m_def.version; };
    int                getFarClip() const            { return m_sight_range; }
    float              getPagedDetailFactor() const  { return m_paged_detail_factor; };
    Ogre::Vector3      getMaxTerrainSize();
    Collisions*        getCollisions()               { return m_collisions; };
    IWater*            getWater()                    { return m_water.get(); };
    Ogre::Light*       getMainLight()                { return m_main_light; };
    Ogre::Vector3      getSpawnPos()                 { return m_def.start_position; };
    RoR::Terrn2Def&    GetDef()                      { return m_def; }
    HydraxWater*       getHydraxManager()            { return m_hydrax_water; }
    SkyManager*        getSkyManager();
    SkyXManager*       getSkyXManager()              { return SkyX_manager; };
    ShadowManager*     getShadowManager()            { return m_shadow_manager; };
    void               LoadTelepoints();
    void               LoadPredefinedActors();
    bool               HasPredefinedActors();
    bool               LoadAndPrepareTerrain(std::string terrn2_filename);
    void               HandleException(const char* summary);
    float              GetHeightAt(float x, float z);
    Ogre::Vector3      GetNormalAt(float x, float y, float z);

    static const int UNLIMITED_SIGHTRANGE = 4999;

private:

    SkyXManager *SkyX_manager;
    // internal methods
    void initCamera();
    void initTerrainCollisions();
    void initDashboards();
    void initFog();
    void initLight();
    void initMotionBlur();
    void initObjects();
    void initScripting();
    void initShadows();
    void initSkySubSystem();
    void initSunburn();
    void initVegetation();
    void initWater();

    void fixCompositorClearColor();
    void loadTerrainObjects();

    TerrainObjectManager*   m_object_manager;
    TerrainGeometryManager* m_geometry_manager;
    std::unique_ptr<IWater> m_water;
    Collisions*    m_collisions;
    Dashboard*     m_dashboard;
    HDRListener*   m_hdr_listener;
    ShadowManager* m_shadow_manager;
    SkyManager*    m_sky_manager;
    HydraxWater*   m_hydrax_water;
    Ogre::Light*   m_main_light;
    RoR::Terrn2Def m_def;
    float          m_cur_gravity;
    float          m_paged_detail_factor;
    int            m_sight_range;
};
