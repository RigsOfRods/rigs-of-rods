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
#include "TerrainManager.h"

#include "BeamData.h"
#include "BeamFactory.h"
#include "Dashboard.h"
#include "DustManager.h"
#include "EnvironmentMap.h"
#include "ErrorUtils.h"
#include "GUIFriction.h"
#include "GlowMaterialListener.h"
#include "HDRListener.h"
#include "Language.h"
#include "RoRFrameListener.h"
#include "ScriptEngine.h"
#include "Settings.h"
#include "ShadowManager.h"
#include "SkyManager.h"
#include "SoundScriptManager.h"
#include "SurveyMapManager.h"
#include "TerrainGeometryManager.h"
#include "TerrainObjectManager.h"
#include "Utils.h"
#include "Water.h"

using namespace Ogre;

TerrainManager::TerrainManager() : 
	  geometry_manager(0)
	, object_manager(0)
	, sky_manager(0)
	, shadow_manager(0)
	, survey_map(0)
	, hdr_listener(0)
	, envmap(0)
	, dashboard(0)
	, collisions(0)
	, character(0)
	, water(0)

{
	gravity = DEFAULT_GRAVITY;
}

TerrainManager::~TerrainManager()
{

}

// some shortcut to remove ugly code
#ifdef USE_MYGUI
#include "LoadingWindow.h"
#define PROGRESS_WINDOW(x, y) { LOG(Ogre::String("  ## ") + y); LoadingWindow::getSingleton().setProgress(x, y); }
#else
#define PROGRESS_WINDOW(x, y) { LOG(Ogre::String("  ## ") + y) }
#endif //USE_MYGUI

void TerrainManager::loadTerrainConfigBasics(Ogre::DataStreamPtr &ds)
{
	// now generate the hash of it
	generateHashFromDataStream(ds, fileHash);

	mTerrainConfig.load(ds, "\t:=", true);

	// read in the settings
	terrain_name = mTerrainConfig.getSetting("Name", "General");
	if (terrain_name.empty())
	{
		showError(_L("Terrain loading error"), _L("the terrain name cannot be empty"));
		exit(125);
	}

	ogre_terrain_config_filename = mTerrainConfig.getSetting("GeometryConfig", "General");
	// otc = ogre terrain config
	if (ogre_terrain_config_filename.find(".otc") == String::npos)
	{
		showError(_L("Terrain loading error"), _L("the new terrain mode only supports .otc configurations"));
		exit(125);
	}

	ambient_color = StringConverter::parseColourValue(mTerrainConfig.getSetting("AmbientColor", "General"));
	start_position = StringConverter::parseVector3(mTerrainConfig.getSetting("StartPosition", "General"));

	guid = mTerrainConfig.getSetting("GUID", "General");
	categoryID = StringConverter::parseInt(mTerrainConfig.getSetting("CategoryID", "General"));
	version = StringConverter::parseInt(mTerrainConfig.getSetting("Version", "General"));

}

void TerrainManager::loadTerrain(String filename)
{
	DataStreamPtr ds;

	try
	{
		String group = ResourceGroupManager::getSingleton().findGroupContainingResource(filename);
		ds = ResourceGroupManager::getSingleton().openResource(filename, group);
	} catch(...)
	{
		LOG("Terrain not found: " + String(filename));
		showError(_L("Terrain loading error"), _L("Terrain not found: ") + filename);
		exit(125);
	}

	PROGRESS_WINDOW(10, _L("Loading Terrain Configuration"));

	LOG(" ===== LOADING TERRAIN " + filename);

	loadTerrainConfigBasics(ds);

	// then, init the subsystems, order is important :)
	initSubSystems();

	fixCompositorClearColor();

	LOG(" ===== LOADING TERRAIN GEOMETRY " + filename);

	// load the terrain geometry
	PROGRESS_WINDOW(80, _L("Loading Terrain Geometry"));
	geometry_manager->loadOgreTerrainConfig(ogre_terrain_config_filename);

	LOG(" ===== LOADING TERRAIN WATER " + filename);
	// must happen here
	initWater();

	LOG(" ===== LOADING TERRAIN OBJECTS " + filename);

	PROGRESS_WINDOW(90, _L("Loading Terrain Objects"));
	loadTerrainObjects();

	collisions->printStats();

	if (gEnv->frameListener) gEnv->frameListener->loading_state = TERRAIN_LOADED;
	
	// bake the decals
	//finishTerrainDecal();

	collisions->finishLoadingTerrain();
	LOG(" ===== TERRAIN LOADING DONE " + filename);
}

void TerrainManager::initSubSystems()
{
	// geometry - ogre terrain things
	PROGRESS_WINDOW(15, _L("Initializing Geometry Subsystem"));
	initGeometry();

	// objects  - .odef support
	PROGRESS_WINDOW(17, _L("Initializing Object Subsystem"));
	initObjects();
	
	// collisions
	PROGRESS_WINDOW(19, _L("Initializing Collision Subsystem"));
	initCollisions();

	// scripting
	PROGRESS_WINDOW(19, _L("Initializing Script Subsystem"));
	initScripting();

	// shadows
	PROGRESS_WINDOW(21, _L("Initializing Shadow Subsystem"));
	initShadows();

	PROGRESS_WINDOW(25, _L("Initializing Camera Subsystem"));
	initCamera();

	// sky, must come after camera due to farclip
	PROGRESS_WINDOW(23, _L("Initializing Sky Subsystem"));
	initSkySubSystem();

	PROGRESS_WINDOW(27, _L("Initializing Light Subsystem"));
	initLight();

	PROGRESS_WINDOW(29, _L("Initializing Fog Subsystem"));
	initFog();

	PROGRESS_WINDOW(31, _L("Initializing Vegetation Subsystem"));
	initVegetation();

	// water must be done later on
	//PROGRESS_WINDOW(33, _L("Initializing Water Subsystem"));
	//initWater();

	if (BSETTING("HDR", false))
	{
		PROGRESS_WINDOW(35, _L("Initializing HDR Subsystem"));
		initHDR();
	}
	if (BSETTING("Glow", false))
	{
		PROGRESS_WINDOW(37, _L("Initializing Glow Subsystem"));
		initGlow();
	}
	if (BSETTING("Motion blur", false))
	{
		PROGRESS_WINDOW(39, _L("Initializing Motion Blur Subsystem"));
		initMotionBlur();
	}
	if (BSETTING("Sunburn", false))
	{
		PROGRESS_WINDOW(41, _L("Initializing Sunburn Subsystem"));
		initSunburn();
	}
	// environment map
	if (!BSETTING("Envmapdisable", false))
	{
		PROGRESS_WINDOW(43, _L("Initializing Environment Map Subsystem"));
		initEnvironmentMap();
	}
	// init the map
	if (!BSETTING("disableOverViewMap", false))
	{
		PROGRESS_WINDOW(45, _L("Initializing Overview Map Subsystem"));
		initSurveyMap();
	}

	PROGRESS_WINDOW(47, _L("Initializing Dashboards Subsystem"));
	initDashboards();
}

void TerrainManager::initCamera()
{
	gEnv->mainCamera->getViewport()->setBackgroundColour(ambient_color);

	farclip = FSETTING("SightRange", 4500);
	if(farclip == 5000)
	{
		gEnv->mainCamera->setFarClipDistance(0);
	} else
	{
		gEnv->mainCamera->setFarClipDistance(farclip);
	}


	gEnv->mainCamera->setPosition(start_position);
}

void TerrainManager::initSkySubSystem()
{
#ifdef USE_CAELUM
	// Caelum skies
	bool useCaelum = SSETTING("Sky effects", "Caelum (best looking, slower)")=="Caelum (best looking, slower)";

	if (useCaelum)
	{
		sky_manager = new SkyManager();
		gEnv->sky = sky_manager;

		// try to load caelum config
		String caelumConfig = mTerrainConfig.getSetting("CaelumConfigFile", "General");

		if (!caelumConfig.empty() && ResourceGroupManager::getSingleton().resourceExistsInAnyGroup(caelumConfig))
		{
			// config provided and existing, use it :)
			sky_manager->loadScript(caelumConfig);
		} else
		{
			// no config provided, fall back to the default one
			sky_manager->loadScript("ror_default_sky");
		}

	} else
#endif //USE_CAELUM
	{
		String sandStormConfig = mTerrainConfig.getSetting("SandStormCubeMap", "General");

		if (!sandStormConfig.empty())
		{
			// use custom
			gEnv->sceneManager->setSkyBox(true, sandStormConfig, 100, true);
		} else
		{
			// use default
			gEnv->sceneManager->setSkyBox(true, "tracks/skyboxcol", 100, true);
		}
	}
}

void TerrainManager::initLight()
{
	if (use_caelum)
	{
		main_light = sky_manager->getMainLight();
	} else
	{
		// screw caelum, we will roll our own light

		// Create a light
		main_light = gEnv->sceneManager->createLight("MainLight");
		//directional light for shadow
		main_light->setType(Light::LT_DIRECTIONAL);
		main_light->setDirection(0.785, -0.423, 0.453);

		main_light->setDiffuseColour(ambient_color);
		main_light->setSpecularColour(ambient_color);
	}
}

void TerrainManager::initFog()
{
	if(farclip == 5000)
		gEnv->sceneManager->setFog(FOG_NONE);
	else
		gEnv->sceneManager->setFog(FOG_LINEAR, ambient_color,  0, farclip * 0.7, farclip * 0.9);
}

void TerrainManager::initVegetation()
{
	// get vegetation mode
	pagedMode = 0; //None
	pagedDetailFactor = 0;
	String vegetationMode = SSETTING("Vegetation", "None (fastest)");
	if     (vegetationMode == "None (fastest)")
	{
		pagedMode = 0;
		pagedDetailFactor = 0.001;
	}
	else if (vegetationMode == "20%")
	{
		pagedMode = 1;
		pagedDetailFactor = 0.2;
	}
	else if (vegetationMode == "50%")
	{
		pagedMode = 2;
		pagedDetailFactor = 0.5;
	}
	else if (vegetationMode == "Full (best looking, slower)")
	{
		pagedMode = 3;
		pagedDetailFactor = 1;
	}
}

void TerrainManager::initHDR()
{
	Viewport *vp = gEnv->mainCamera->getViewport();
	CompositorInstance *instance = CompositorManager::getSingleton().addCompositor(vp, "HDR", 0);
	CompositorManager::getSingleton().setCompositorEnabled(vp, "HDR", true);

	// HDR needs a special listener
	hdr_listener = new HDRListener();
	instance->addListener(hdr_listener);
	hdr_listener->notifyViewportSize(vp->getActualWidth(), vp->getActualHeight());
	hdr_listener->notifyCompositor(instance);
}

void TerrainManager::initGlow()
{
	CompositorManager::getSingleton().addCompositor(gEnv->mainCamera->getViewport(), "Glow");
	CompositorManager::getSingleton().setCompositorEnabled(gEnv->mainCamera->getViewport(), "Glow", true);
	GlowMaterialListener *gml = new GlowMaterialListener();
	MaterialManager::getSingleton().addListener(gml);
}

void TerrainManager::initMotionBlur()
{
	/// Motion blur effect
	CompositorPtr comp3 = CompositorManager::getSingleton().create(
		"MotionBlur", ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME
		);
	{
		CompositionTechnique *t = comp3->createTechnique();
		{
			CompositionTechnique::TextureDefinition *def = t->createTextureDefinition("scene");
			def->width = 0;
			def->height = 0;
#if OGRE_VERSION>0x010602
			def->formatList.push_back(PF_R8G8B8);
#else
			def->format = PF_R8G8B8;
#endif //OGRE_VERSION
		}
		{
			CompositionTechnique::TextureDefinition *def = t->createTextureDefinition("sum");
			def->width = 0;
			def->height = 0;
#if OGRE_VERSION>0x010602
			def->formatList.push_back(PF_R8G8B8);
#else
			def->format = PF_R8G8B8;
#endif //OGRE_VERSION
		}
		{
			CompositionTechnique::TextureDefinition *def = t->createTextureDefinition("temp");
			def->width = 0;
			def->height = 0;
#if OGRE_VERSION>0x010602
			def->formatList.push_back(PF_R8G8B8);
#else
			def->format = PF_R8G8B8;
#endif //OGRE_VERSION
		}
		/// Render scene
		{
			CompositionTargetPass *tp = t->createTargetPass();
			tp->setInputMode(CompositionTargetPass::IM_PREVIOUS);
			tp->setOutputName("scene");
		}
		/// Initialisation pass for sum texture
		{
			CompositionTargetPass *tp = t->createTargetPass();
			tp->setInputMode(CompositionTargetPass::IM_PREVIOUS);
			tp->setOutputName("sum");
			tp->setOnlyInitial(true);
		}
		/// Do the motion blur
		{
			CompositionTargetPass *tp = t->createTargetPass();
			tp->setInputMode(CompositionTargetPass::IM_NONE);
			tp->setOutputName("temp");
			{ CompositionPass *pass = tp->createPass();
			pass->setType(CompositionPass::PT_RENDERQUAD);
			pass->setMaterialName("Ogre/Compositor/Combine");
			pass->setInput(0, "scene");
			pass->setInput(1, "sum");
			}
		}
		/// Copy back sum texture
		{
			CompositionTargetPass *tp = t->createTargetPass();
			tp->setInputMode(CompositionTargetPass::IM_NONE);
			tp->setOutputName("sum");
			{ CompositionPass *pass = tp->createPass();
			pass->setType(CompositionPass::PT_RENDERQUAD);
			pass->setMaterialName("Ogre/Compositor/Copyback");
			pass->setInput(0, "temp");
			}
		}
		/// Display result
		{
			CompositionTargetPass *tp = t->getOutputTargetPass();
			tp->setInputMode(CompositionTargetPass::IM_NONE);
			{ CompositionPass *pass = tp->createPass();
			pass->setType(CompositionPass::PT_RENDERQUAD);
			pass->setMaterialName("Ogre/Compositor/MotionBlur");
			pass->setInput(0, "sum");
			}
		}
	}
	CompositorManager::getSingleton().addCompositor(gEnv->mainCamera->getViewport(),"MotionBlur");
	CompositorManager::getSingleton().setCompositorEnabled(gEnv->mainCamera->getViewport(), "MotionBlur", true);
}

void TerrainManager::initSunburn()
{
	CompositorManager::getSingleton().addCompositor(gEnv->mainCamera->getViewport(),"Sunburn");
	CompositorManager::getSingleton().setCompositorEnabled(gEnv->mainCamera->getViewport(), "Sunburn", true);
}

void TerrainManager::fixCompositorClearColor()
{
	//hack
	// now with extensive error checking
	if (CompositorManager::getSingleton().hasCompositorChain(gEnv->mainCamera->getViewport()))
	{
		//	//CompositorManager::getSingleton().getCompositorChain(gEnv->ogreCamera->getViewport())->getCompositor(0)->getTechnique()->getOutputTargetPass()->getPass(0)->setClearColour(fade_color);
		CompositorInstance *co = CompositorManager::getSingleton().getCompositorChain(gEnv->mainCamera->getViewport())->_getOriginalSceneCompositor();
		if (co)
		{
			CompositionTechnique *ct = co->getTechnique();
			if (ct)
			{
				CompositionTargetPass *ctp = ct->getOutputTargetPass();
				if (ctp)
				{
					CompositionPass *p = ctp->getPass(0);
					if (p)
					{
						p->setClearColour(fade_color);
					}
				}
			}
		}
	}
}

void TerrainManager::initWater()
{
	String waterSettingsString = SSETTING("Water effects", "Reflection + refraction (speed optimized)");
	if (waterSettingsString != "None")
		water = new Water(mTerrainConfig);
}

void TerrainManager::initEnvironmentMap()
{
	envmap = new Envmap();
}

void TerrainManager::initDashboards()
{
	dashboard = new Dashboard();
}

void TerrainManager::initShadows()
{
	shadow_manager   = new ShadowManager();
	shadow_manager->loadConfiguration();
}

void TerrainManager::loadTerrainObjects()
{
	try
	{
		ConfigFile::SettingsIterator objectsIterator = mTerrainConfig.getSettingsIterator("Objects");
		String svalue, sname;
		while (objectsIterator.hasMoreElements())
		{
			sname = objectsIterator.peekNextKey();
			StringUtil::trim(sname);
			svalue = objectsIterator.getNext();
			StringUtil::trim(svalue);

			object_manager->loadObjectConfigFile(sname);
		}
	} catch(...)
	{
		// no objects found
	}

	// bakes the geometry and things
	object_manager->postLoad();
}

void TerrainManager::initCollisions()
{
	collisions = new Collisions();
	gEnv->collisions = collisions;

	String tractionMapConfig = mTerrainConfig.getSetting("TractionMap", "General");
	if (!tractionMapConfig.empty())
	{
		gEnv->collisions->setupLandUse(tractionMapConfig.c_str());
	}
}

bool TerrainManager::update(float dt)
{
	if(object_manager)
		object_manager->update(dt);

	if(geometry_manager)
		geometry_manager->update(dt);

	return true;
}


void TerrainManager::initScripting()
{
#ifdef USE_ANGELSCRIPT
	bool loaded = false;

	// only load terrain scripts while not in multiplayer
	if (!gEnv->network)
	{
		try
		{
			ConfigFile::SettingsIterator objectsIterator = mTerrainConfig.getSettingsIterator("Scripts");
			while (objectsIterator.hasMoreElements())
			{
				String sname = objectsIterator.peekNextKey();
				StringUtil::trim(sname);
				String svalue = objectsIterator.getNext();
				StringUtil::trim(svalue);

				if(!ScriptEngine::getSingleton().loadScript(sname))
					loaded = true;
			}
		} catch(...)
		{
			// simply no script section
			//LOG("Exception while trying load script: " + e.getFullDescription());
		}
	}
	
	if (!loaded)
	{
		// load a default script that does the most basic things
		ScriptEngine::getSingleton().loadScript("default.as");
	}
	// finally activate AS logging, so we dont spam the users screen with initialization messages
	ScriptEngine::getSingleton().activateLogging();
#endif //USE_ANGELSCRIPT
}

void TerrainManager::setGravity(float value)
{
	gravity = value;
	BeamFactory::getSingleton().recalcGravityMasses();
}

void TerrainManager::initSurveyMap()
{
	survey_map = new SurveyMapManager(getMaxTerrainSize());
}

void TerrainManager::initGeometry()
{
	geometry_manager = new TerrainGeometryManager(this);
}

void TerrainManager::initObjects()
{
	object_manager = new TerrainObjectManager(this);
}

Ogre::Vector3 TerrainManager::getMaxTerrainSize()
{
	if(!geometry_manager)
		return Vector3::ZERO;
	return geometry_manager->getMaxTerrainSize();
}

IHeightFinder* TerrainManager::getHeightFinder()
{
	return geometry_manager;
}

size_t TerrainManager::getMemoryUsage()
{
	// TODO: FIX
	return 0;
}

void TerrainManager::freeResources()
{
	// TODO
}
