/*
This source file is part of Rigs of Rods
Copyright 2005,2006,2007,2008,2009 Pierre-Michel Ricordel
Copyright 2007,2008,2009 Thomas Fischer

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

#include "RoRPrerequisites.h"
#include "HydraxWater.h"
//#include "OgreTerrainSceneManager.h" // for some cast in order to get the terrain material = ILLEGAL to link to a plugin!
#include "RadialGrid.h"

HydraxWater::HydraxWater() :
	  waternoise(0)
	, mHydrax(0)
	, wheight(5)
{
	//Vector3 mapsize = gEnv->terrainManager->getMaxTerrainSize();
	LOG("* loading Hydrax...");
	waveStrength = 30.5f;
	mHydrax = new Hydrax::Hydrax(gEnv->sceneManager, gEnv->mainCamera, gEnv->viewPort);
}

int HydraxWater::loadConfig(Ogre::String configfile)
{
	waternoise = new Hydrax::Noise::Perlin(/*Generic one*/);

	// Create our projected grid module  
	Hydrax::Module::RadialGrid *module 
		= new Hydrax::Module::RadialGrid(// Hydrax parent pointer
		mHydrax,
		// Noise module
		waternoise,
		//Ogre::Plane(Ogre::Vector3(0,1,0), Ogre::Vector3(0,0,0)),
		// Normal mode
		Hydrax::MaterialManager::NM_VERTEX);

		/*
		= new Hydrax::Module::ProjectedGrid(// Hydrax parent pointer
		mHydrax,
		// Noise module
		waternoise,
		// Base plane
		Ogre::Plane(Ogre::Vector3::UNIT_Y, Ogre::Real(0.0f)),
		//Ogre::Plane(Ogre::Vector3(0,1,0), Ogre::Vector3(0,0,0)),
		// Normal mode
		Hydrax::MaterialManager::NM_VERTEX,
		// Projected grid options
		Hydrax::Module::ProjectedGrid::Options(264));
		*/
	// Set our module
	mHydrax->setModule(static_cast<Hydrax::Module::Module*>(module));

	// Load all parameters from config file
	// Remarks: The config file must be in Hydrax resource group.
	// All parameters can be set/updated directly by code(Like previous versions),
	// but due to the high number of customizable parameters, Hydrax 0.4 allows save/load config files.
	mHydrax->loadCfg(configfile);

	// Set our shader mode
	//mHydrax->setShaderMode(Hydrax::MaterialManager::SM_CG);

#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
	// directx
	mHydrax->setShaderMode(Hydrax::MaterialManager::SM_HLSL);
#elif OGRE_PLATFORM == OGRE_PLATFORM_LINUX
	// opengl
	mHydrax->setShaderMode(Hydrax::MaterialManager::SM_CG);
#elif OGRE_PLATFORM == OGRE_PLATFORM_APPLE
	// opengl
	mHydrax->setShaderMode(Hydrax::MaterialManager::SM_CG);
#endif

	// water height is always overwritten
	mHydrax->setPosition(Ogre::Vector3(0, wheight, 0));

	// Create water
    mHydrax->create();
	

/*
	// Adjust some options
	mHydrax->setPosition(Ogre::Vector3(0, wheight, 0));
	mHydrax->setPlanesError(waveStrength*0.33);
	mHydrax->setDepthLimit(10);	//30m
	mHydrax->setNormalDistortion(0.025);
//	mHydrax->setDepthColor(Ogre::Vector3(0.04,0.135,0.185));
	mHydrax->setSmoothPower(30);
	mHydrax->setCausticsScale(12);
	mHydrax->setGlobalTransparency(0);
	mHydrax->setFullReflectionDistance(99999997952.0);
	//mHydrax->setPolygonMode(Ogre::PM_WIREFRAME);
	//mHydrax->setFoamScale(0.1);    // Or something else
	//mHydrax->setFoamStart(-0.5);    // Must be negative
	//mHydrax->setFoamMaxDistance(100);
*/
	// XXX: TODO: check for custom terrain material
	
//	MaterialPtr mat = ((TerrainSceneManager*)mSceneMgr)->getTerrainMaterial(); //can't do that
	
	
	// Add our hydrax depth technique to island material
	// (Because the terrain mesh is not an Ogre::Entity)
	/*
	MaterialPtr mat = (MaterialPtr)(MaterialManager::getSingleton().getByName("TerrainSceneManager/Terrain"));
	if(!mat.isNull())
	{
		mHydrax->getMaterialManager()->addDepthTechnique(mat->createTechnique());
	}
	*/

	// fog override for the water!
	/*
	MaterialPtr mathydrax = mHydrax->getMaterialManager()->getMaterial(Hydrax::MaterialManager::MAT_WATER);
	if(!mathydrax.isNull())
		for(int i=0; i<mathydrax->getNumTechniques();i++)
			for(int j=0; j<mathydrax->getTechnique(i)->getNumPasses();j++)
				mathydrax->getTechnique(i)->getPass(j)->setFog(true);
				*/
	/*
	// test for decals
	for(int x=0; x<1000; x+=10)
	{
		Hydrax::Decal* mDecal = mHydrax->getDecalsManager()->add("searose.png");
		mDecal->setPosition(Ogre::Vector2(x,x));
		mDecal->setSize(Vector2(0.001, 0.001));
		mDecal->setTransparency(0.8);
	}
	*/
	LogManager::getSingleton().logMessage("* Hydrax loaded with config "+configfile);
	return 0;
}

void HydraxWater::registerDust(DustPool* dp)
{
}

void HydraxWater::moveTo(Camera *cam, float centerheight)
{
}

bool HydraxWater::allowUnderWater()
{
	return true;
}

void HydraxWater::showWave(Vector3 refpos)
{
}

void HydraxWater::update()
{
}

void HydraxWater::prepareShutdown()
{
}

float HydraxWater::getHeight()
{
	return wheight;
}

void HydraxWater::setHeight(float value)
{
	wheight = value;
}

void HydraxWater::setVisible(bool value)
{
	if(mHydrax)
		mHydrax->setVisible(value);
}

float HydraxWater::getHeightWaves(Vector3 pos)
{
	// TODO: fix hydrax crash!
	/*
	if(waternoise)
	{
		float nv = waternoise->getValue(pos.x, pos.z);
		return this->wheight + nv * waveStrength;
	}
	*/
	return wheight;
}

Vector3 HydraxWater::getVelocity(Vector3 pos)
{
	// TODO: FIX THIS!
	return Vector3(0, 0, 0);
}

void HydraxWater::updateReflectionPlane(float h)
{
}

void HydraxWater::setFadeColour(ColourValue ambient)
{
	if(mHydrax)
		mHydrax->setSunColor(Vector3(ambient.r, ambient.g, ambient.b));
}


void HydraxWater::setSunPosition(Ogre::Vector3 pos)
{
	if(mHydrax)
		mHydrax->setSunPosition(pos);
}

void HydraxWater::framestep(float dt)
{
	if(mHydrax)
		mHydrax->update(dt);
}

