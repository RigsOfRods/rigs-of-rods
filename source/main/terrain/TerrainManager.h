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

class TerrainManager : public ZeroedMemoryAllocator
{
public:

    TerrainManager(RoRFrameListener* sim_controller);
    ~TerrainManager();

    void loadTerrain(Ogre::String filename);

    bool update(float dt);

    void setGravity(float value);
    float getGravity() { return gravity; };

    Ogre::String getTerrainName() { return m_def.name; };
    Ogre::String getGUID() { return m_def.guid; };
    int getCategoryID() { return m_def.category_id; };
    int getVersion() { return m_def.version; };
    int getFarClip() { return far_clip; }
    int getPagedMode() { return paged_mode; };
    float getPagedDetailFactor() { return paged_detail_factor; };
    std::vector<authorinfo_t>& GetAuthors();

    Ogre::Vector3 getMaxTerrainSize();

    // some getters
    Collisions* getCollisions() { return collisions; };
    IHeightFinder* getHeightFinder();
    IWater* getWater() { return water; };
    Ogre::Light* getMainLight() { return main_light; };
    Ogre::Vector3 getSpawnPos() { return m_def.start_position; };
    RoR::Terrn2Def& GetDef() { return m_def; }

    SkyManager* getSkyManager();

    TerrainGeometryManager* getGeometryManager() { return geometry_manager; };
    TerrainObjectManager* getObjectManager() { return object_manager; };

    ShadowManager*     getShadowManager() { return shadow_manager; };
    std::string        GetMinimapTextureName();
    RoRFrameListener*  GetSimController() { return m_sim_controller; }

    // preloaded trucks
    void loadPreloadedTrucks();
    bool hasPreloadedTrucks();

    static const int UNLIMITED_SIGHTRANGE = 4999;

protected:

    RoRFrameListener* m_sim_controller;
    // subsystems
    Character* character;
    Collisions* collisions;
    Dashboard* dashboard;
    HDRListener* hdr_listener;
    SurveyMapManager* m_survey_map;
    ShadowManager* shadow_manager;
    SkyManager* sky_manager;
    TerrainGeometryManager* geometry_manager;
    TerrainObjectManager* object_manager;
    IWater* water;
    HydraxWater* hw;
    Ogre::Light *main_light;

    // properties
    RoR::Terrn2Def m_def;
    float gravity;

    float paged_detail_factor;
    int far_clip;
    int paged_mode;


    // internal methods
    void initCamera();
    void initTerrainCollisions();
    void initDashboards();
    void initFog();
    void initGeometry();
    void initGlow();
    void initHDR();
    void initLight();
    void initMotionBlur();
    void initObjects();
    void initScripting();
    void initShadows();
    void initSkySubSystem();
    void initSubSystems();
    void initSunburn();
    void initVegetation();
    void initWater();

    void fixCompositorClearColor();
    void loadTerrainObjects();
};
