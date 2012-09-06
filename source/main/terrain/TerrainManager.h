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
#ifndef __TerrainManager_H_
#define __TerrainManager_H_

#include "RoRPrerequisites.h"

#include "CacheSystem.h"

#include <OgreConfigFile.h>
#include "IManager.h"

class TerrainManager : public IManager
{
	friend class CacheSystem;
	friend class TerrainObjectManager;

public:

	TerrainManager();
	~TerrainManager();

	void loadTerrain(Ogre::String filename);
	void loadTerrainConfigBasics(Ogre::DataStreamPtr &ds);
	

	bool update(float dt);

	void setGravity(float value);
	float getGravity() { return gravity; };

	Ogre::String getTerrainName() { return terrain_name; };
	Ogre::Vector3 getMaxTerrainSize();

	// some getters
	Collisions *getCollisions() { return collisions; };
	Envmap *getEnvmap() { return envmap; };
	IHeightFinder *getHeightFinder();
	Ogre::Light *getMainLight() { return main_light; };
	Ogre::Vector3 getSpawnPos() { return start_position; };
	SkyManager *getSkyManager() { return sky_manager; };
	TerrainGeometryManager *getGeometryManager() { return geometry_manager; };
	TerrainObjectManager *getObjectManager() { return object_manager; };
	Water *getWater() { return water; };
	bool getTrucksLoaded() { return trucksLoaded; };

	size_t getMemoryUsage();
	void freeResources();

protected:
	// members
	Ogre::ConfigFile mTerrainConfig;

	// subsystems
	Character *character;
	Collisions *collisions;
	Dashboard *dashboard;
	Envmap *envmap;
	HDRListener *hdr_listener;
	SurveyMapManager *survey_map;
	ShadowManager *shadow_manager;
	SkyManager *sky_manager;
	TerrainGeometryManager *geometry_manager;
	TerrainObjectManager *object_manager;
	Water *water;	

	// properties
	Ogre::ColourValue ambient_color;
	Ogre::ColourValue fade_color;
	Ogre::Light *main_light;
	Ogre::String fileHash;
	Ogre::String guid;
	Ogre::String ogre_terrain_config_filename;
	Ogre::String terrain_name;
	Ogre::Vector3 start_position;
	bool trucksLoaded;
	bool use_caelum;
	float gravity;
	float pagedDetailFactor;
	float water_line;
	int categoryID;
	int farclip;
	int pagedMode;
	int version;

	// internal methods
	void initCamera();
	void initCollisions();
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

#endif // __TerrainManager_H_
