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
#include "Water.h"

#include "Application.h"
#include "OgreSubsystem.h"
#include "ResourceBuffer.h"
#include "RoRFrameListener.h"
#include "Settings.h"
#include "TerrainManager.h"

#include <OgreBillboard.h>

using namespace Ogre;

/**********************************************************************
Static declarations
**********************************************************************/
// Lights
#define NUM_LIGHTS 1
// Colours for the lights
Ogre::ColourValue mDiffuseLightColours[NUM_LIGHTS] =
{
	Ogre::ColourValue(0.6, 0.6, 0.6)
};

Ogre::ColourValue mSpecularLightColours[NUM_LIGHTS] =
{
	Ogre::ColourValue(0.5, 0.5, 0.5)
};

// the light nodes
Ogre::SceneNode* mLightNodes[NUM_LIGHTS];
// the light node pivots
Ogre::SceneNode* mLightPivots[NUM_LIGHTS];

Ogre::SceneNode*      mMainNode;
Ogre::Entity*         mOceanSurfaceEnt;

Ogre::MaterialPtr     mActiveMaterial;
Ogre::Pass*           mActivePass;
Ogre::GpuProgramPtr   mActiveFragmentProgram;
Ogre::GpuProgramPtr   mActiveVertexProgram;
Ogre::GpuProgramParametersSharedPtr mActiveFragmentParameters;
Ogre::GpuProgramParametersSharedPtr mActiveVertexParameters;

Water::Water(const Ogre::ConfigFile &mTerrainConfig) 
{
	mapSize = gEnv->terrainManager->getMaxTerrainSize();
	wHeight = PARSEREAL(mTerrainConfig.getSetting("WaterLine", "General"));;
	wbHeight = PARSEREAL(mTerrainConfig.getSetting("WaterBottomLine", "General"));

	if (mapSize.x < 1500 && mapSize.z < 1500)
		mScale = 1.5f;
	else
		mScale = 1.0f;

	processWater(1);
}

Water::~Water()
{


}

void Water::processWater(int mType)
{
	mActiveMaterial = Ogre::MaterialManager::getSingleton().getByName("Ocean2_HLSL_GLSL");
	mActiveMaterial->load();

	// Define a plane mesh that will be used for the ocean surface
	Ogre::Plane oceanSurface;
	oceanSurface.normal = Ogre::Vector3::UNIT_Y;
	oceanSurface.d = 0;
	Ogre::MeshManager::getSingleton().createPlane("OceanSurface",
		Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
		oceanSurface,
		1000, 1000, 50, 50, true, 1, 1, 1, Ogre::Vector3::UNIT_Z);

	mOceanSurfaceEnt = gEnv->sceneManager->createEntity("OceanSurface",
		ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
		SCENE_STATIC);

	mOceanSurfaceEnt->setMaterialName("Ocean2_HLSL_GLSL");

	if (!mActiveMaterial.isNull() && mActiveMaterial->getNumSupportedTechniques())
	{
		Ogre::Technique* currentTechnique = mActiveMaterial->getSupportedTechnique(0);
		if (currentTechnique)
		{
			mActivePass = currentTechnique->getPass(0);
			if (mActivePass)
			{
				if (mActivePass->hasFragmentProgram())
				{
					mActiveFragmentProgram = mActivePass->getFragmentProgram();
					mActiveFragmentParameters = mActivePass->getFragmentProgramParameters();
				}
				if (mActivePass->hasVertexProgram())
				{
					mActiveVertexProgram = mActivePass->getVertexProgram();
					mActiveVertexParameters = mActivePass->getVertexProgramParameters();
				}

				mActiveVertexParameters->setNamedConstant("waveFreq", 0.025f);
				mActiveVertexParameters->setNamedConstant("waveAmp", 0.0f);

				mActivePass->setDiffuse(gEnv->terrainManager->getMainLight()->getDiffuseColour());
				mActivePass->setAmbient(gEnv->sceneManager->getAmbientLight());
			}
		}
	}
	pWaterNode = gEnv->sceneManager->getRootSceneNode()->createChildSceneNode(SCENE_STATIC);
	pWaterNode->attachObject(mOceanSurfaceEnt);
	pWaterNode->setPosition(Vector3((mapSize.x * mScale) / 2, wHeight, (mapSize.z * mScale) / 2));
}

bool Water::allowUnderWater()
{
	return false;
}

void Water::setVisible(bool value)
{

}

void Water::setFadeColour(ColourValue ambient)
{
}


void Water::moveTo(float centerheight)
{

}

void Water::showWave(Vector3 refpos)
{
}

bool Water::isCameraUnderWater()
{
	return false;
}

void Water::update()
{

}

void Water::prepareShutdown()
{

}

float Water::getHeight()
{
	return wHeight;
};

void Water::setHeight(float value)
{
	wHeight = value;
	ForceUpdate = true;
}

float Water::getHeightWaves(Vector3 pos)
{
	return 0;
}

bool Water::isUnderWater(Vector3 pos)
{
	return false;
}

Vector3 Water::getVelocity(Vector3 pos)
{
	return Ogre::Vector3(0,0,0);
}

void Water::updateReflectionPlane(float h)
{

}

void Water::setSunPosition(Vector3)
{
	// not used here!
}

void Water::setCamera(Ogre::Camera *cam)
{
	mRenderCamera = cam;
}

void Water::framestep(float dt)
{
	if (dt)
		update();
}

float Water::getWaveHeight(Vector3 pos)
{
	return 0;
}
