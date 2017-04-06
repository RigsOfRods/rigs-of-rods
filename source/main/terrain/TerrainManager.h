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

#include <json/json.h>

class TerrainManager : public ZeroedMemoryAllocator
{
public:

    TerrainManager(RoRFrameListener* sim_controller);
    ~TerrainManager();

    void LoadTerrain(std::string const & filename);

    bool update(float dt);

    void setGravity(float value);
    float getGravity() { return m_terrn_gravity; };

    Ogre::String getTerrainName() { return m_terrn_name; };
    Ogre::String getGUID() { return m_terrn_guid; };
    int getFarClip() { return far_clip; }
    int getPagedMode() { return paged_mode; };
    float getPagedDetailFactor() { return paged_detail_factor; };
    std::vector<authorinfo_t>& GetAuthors();

    Ogre::Vector3 getMaxTerrainSize();

    // some getters
    Collisions* getCollisions() { return m_terrn_collisions; };
    Envmap* getEnvmap() { return envmap; };
    IHeightFinder* getHeightFinder();
    IWater* getWater() { return water; };
    Ogre::Light* getMainLight() { return main_light; };
    Ogre::Vector3 getSpawnPos() { return m_spawn_pos; };

    SkyManager* getSkyManager();

    TerrainGeometryManager* getGeometryManager() { return geometry_manager; };
    TerrainObjectManager* getObjectManager() { return object_manager; };

    ShadowManager* getShadowManager() { return shadow_manager; };
    std::string    GetMinimapTextureName();

    // preloaded trucks
    void loadPreloadedTrucks();
    bool hasPreloadedTrucks();

    static const int UNLIMITED_SIGHTRANGE = 4999;

private:

    RoRFrameListener* m_sim_controller;

    // subsystems
    Character* character;
    Collisions* m_terrn_collisions;
    Dashboard* dashboard;
    Envmap* envmap;
    HDRListener* hdr_listener;
    SurveyMapManager* m_survey_map;
    ShadowManager* shadow_manager;
    SkyManager* sky_manager;
    TerrainGeometryManager* geometry_manager;
    TerrainObjectManager* object_manager;
    IWater* water;
    HydraxWater* hw;
    Ogre::Light *main_light;

    // Terrain properties
    Ogre::Vector3   m_spawn_pos;
    std::string     m_terrn_name;
    std::string     m_terrn_guid;
    float           m_terrn_gravity;

    float paged_detail_factor;
    int far_clip;
    int paged_mode;


    // internal methods
    void InitSubSystems(Json::Value& j_terrn);
    void initCamera(Json::Value& j_terrn, Ogre::ColourValue const & ambient_color);
    void initDashboards();
    void initGeometry();
    void initGlow();
    void initHDR();
    void initLight(Ogre::ColourValue const & ambient_color);
    void initMotionBlur();
    void initObjects();
    void initScripting(Json::Value& j_terrn);
    void initShadows();
    void initSkySubSystem(Json::Value& j_terrn);
    void initSunburn();
    void initVegetation();
    void initWater(Json::Value& j_terrn);

    void fixCompositorClearColor();
};
