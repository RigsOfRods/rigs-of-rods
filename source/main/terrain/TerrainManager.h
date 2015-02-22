/*
This source file is part of Rigs of Rods
Copyright 2005-2012 Pierre-Michel Ricordel
Copyright 2007-2012 Thomas Fischer

For more information, see http://www.rigsofrods.com/

Rigs of Rods is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License version 3, as
published by the Free Software Foundation.

Rigs of Rods is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Rigs of Rods.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include "ConfigFile.h"
#include "IManager.h"
#include "RoRFrameListener.h"

class TerrainManager : public IManager
{
public:

	TerrainManager();
	~TerrainManager();

	void loadTerrain(Ogre::String filename);
	void loadTerrainConfigBasics(Ogre::DataStreamPtr &ds);

	bool update(float dt);

	void setGravity(float value);
	float getGravity() { return gravity; };

	Ogre::String getTerrainName() { return terrain_name; };
	Ogre::String getGUID() { return guid; };
	int getCategoryID() { return category_id; };
	int getVersion() { return version; };
	int getFarClip() { return far_clip; }
	int getPagedMode() { return paged_mode; };
	float getPagedDetailFactor() { return paged_detail_factor; };
	std::vector<authorinfo_t> & GetAuthors();

	Ogre::Vector3 getMaxTerrainSize();

	// some getters
	Collisions *getCollisions() { return collisions; };
	Envmap *getEnvmap() { return envmap; };
	IHeightFinder *getHeightFinder();
	IWater *getWater() { return water; };
	Ogre::Light *getMainLight() { return main_light; };
	Ogre::Vector3 getSpawnPos() { return start_position; };

	CaelumManager *getCaelumManager() 
	{ 
		if (gEnv->frameListener->loading_state == TERRAIN_LOADED || gEnv->frameListener->loading_state == ALL_LOADED)
			return caelum_manager;
		else
			return nullptr;
	};

	TerrainGeometryManager *getGeometryManager() { return geometry_manager; };
	TerrainObjectManager *getObjectManager() { return object_manager; };

	// preloaded trucks
	void loadPreloadedTrucks();
	bool hasPreloadedTrucks();

	size_t getMemoryUsage();
	void freeResources();

	static const int UNLIMITED_SIGHTRANGE = 4999;

protected:

	RoR::ConfigFile m_terrain_config;

	// subsystems
	Character *character;
	Collisions *collisions;
	Dashboard *dashboard;
	Envmap *envmap;
	HDRListener *hdr_listener;
	SurveyMapManager *survey_map;
	ShadowManager *shadow_manager;
	CaelumManager *caelum_manager;
	TerrainGeometryManager *geometry_manager;
	TerrainObjectManager *object_manager;
	IWater *water;
	HydraxWater *hw;

	// properties
	Ogre::ColourValue ambient_color;
	Ogre::ColourValue fade_color;
	Ogre::Light *main_light;
	Ogre::String file_hash;
	Ogre::String guid;
	Ogre::String ogre_terrain_config_filename;
	Ogre::String terrain_name;
	Ogre::Vector3 start_position;
	std::vector<authorinfo_t> authors;
	bool use_caelum;
	float gravity;
	float paged_detail_factor;
	float water_line;
	int category_id;
	int far_clip;
	int paged_mode;
	int version;

	// internal methods
	void initCamera();
	void initCollisions();
	void initTerrainCollisions();
	void initDashboards();
	void initEnvironmentMap();
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
	void initSurveyMap();
	void initVegetation();
	void initWater();

	void fixCompositorClearColor();
	void loadTerrainObjects();
};
