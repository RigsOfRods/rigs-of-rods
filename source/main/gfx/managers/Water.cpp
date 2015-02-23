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

using namespace Ogre;

//Some ugly code here..
Entity* pPlaneEnt;

Plane waterPlane;
Plane bottomPlane;
Plane reflectionPlane;
Plane refractionPlane;
SceneManager *waterSceneMgr;

class RefractionTextureListener : public RenderTargetListener, public ZeroedMemoryAllocator
{
public:

	void preRenderTargetUpdate(const RenderTargetEvent& evt)
	{
		waterSceneMgr->getRenderQueue()->getQueueGroup(RENDER_QUEUE_MAIN)->setShadowsEnabled(false);
		// Hide plane
		pPlaneEnt->setVisible(false);
		//hide Water spray
		if (gEnv->frameListener) gEnv->frameListener->showspray(false);
	}

	void postRenderTargetUpdate(const RenderTargetEvent& evt)
	{
		// Show plane
		pPlaneEnt->setVisible(true);
		waterSceneMgr->getRenderQueue()->getQueueGroup(RENDER_QUEUE_MAIN)->setShadowsEnabled(true);
		//restore Water spray
		if (gEnv->frameListener) gEnv->frameListener->showspray(true);
	}
};

class ReflectionTextureListener : public RenderTargetListener, public ZeroedMemoryAllocator
{
public:
	void preRenderTargetUpdate(const RenderTargetEvent& evt)
	{
		waterSceneMgr->getRenderQueue()->getQueueGroup(RENDER_QUEUE_MAIN)->setShadowsEnabled(false);
		// Hide plane
		pPlaneEnt->setVisible(false);

	}
	void postRenderTargetUpdate(const RenderTargetEvent& evt)
	{
		// Show plane
		pPlaneEnt->setVisible(true);
		waterSceneMgr->getRenderQueue()->getQueueGroup(RENDER_QUEUE_MAIN)->setShadowsEnabled(true);
	}
};

RefractionTextureListener mRefractionListener;
ReflectionTextureListener mReflectionListener;
//End ugly code

Water::Water(const Ogre::ConfigFile &mTerrainConfig) :
	maxampl(0),
	free_wavetrain(0),
	visible(true),
	haswaves(BSETTING("Waves", false)),
	mScale(1.0f),
	vRtt1(0),
	vRtt2(0), 
	mRenderCamera(gEnv->mainCamera),
	pWaterNode(0),
	framecounter(0),
	rttTex1(0),
	rttTex2(0),
	mReflectCam(0),
	mRefractCam(0)
{
	//Ugh.. Why so ugly and hard to read
	mapSize = gEnv->terrainManager->getMaxTerrainSize();
	fade = gEnv->sceneManager->getFogColour();
	waterSceneMgr = gEnv->sceneManager;

	wHeight = PARSEREAL(mTerrainConfig.getSetting("WaterLine", "General"));;
	wbHeight = PARSEREAL(mTerrainConfig.getSetting("WaterBottomLine", "General"));

	if (mapSize.x < 1500 && mapSize.z < 1500)
		mScale = 1.5f;

	// disable waves in multiplayer
	if(gEnv->network)
		haswaves = false;

	// and the type
	String waterSettingsString = SSETTING("Water effects", "Reflection + refraction (speed optimized)");
	if (waterSettingsString == "Basic (fastest)")
		mType = WATER_BASIC;
	if (waterSettingsString == "Reflection")
		mType = WATER_REFLECT;
	else if (waterSettingsString == "Reflection + refraction (speed optimized)")
		mType = WATER_FULL_SPEED;
	else if (waterSettingsString == "Reflection + refraction (quality optimized)")
		mType = WATER_FULL_QUALITY;

	if (haswaves)
	{
		char line[1024] = {};
		FILE *fd = fopen((SSETTING("Config Root", "")+"wavefield.cfg").c_str(), "r");
		if (fd)
		{
			while (!feof(fd))
			{
				int res = fscanf(fd," %[^\n\r]",line);
				if (line[0] == ';') continue;
				float wl,amp,mx,dir;
				res = sscanf(line,"%f, %f, %f, %f",&wl,&amp,&mx,&dir);
				if (res < 4) continue;
				wavetrains[free_wavetrain].wavelength=wl;
				wavetrains[free_wavetrain].amplitude=amp;
				wavetrains[free_wavetrain].maxheight=mx;
				wavetrains[free_wavetrain].direction=dir/57.0;
				free_wavetrain++;
			}
			fclose(fd);
		}
		for (int i=0; i<free_wavetrain; i++)
		{
			wavetrains[i].wavespeed=1.25*sqrt(wavetrains[i].wavelength);
			maxampl+=wavetrains[i].maxheight;
		}
	}

	processWater(mType);
}

Water::~Water()
{

	if (mRefractCam != nullptr)
	{
		gEnv->sceneManager->destroyCamera(mRefractCam);
		mRefractCam = nullptr;
	}

	if (mReflectCam != nullptr)
	{
		gEnv->sceneManager->destroyCamera(mReflectCam);
		mReflectCam = nullptr;
	}

	if (pPlaneEnt != nullptr)
	{
		gEnv->sceneManager->destroyEntity(pPlaneEnt);
		pPlaneEnt = nullptr;
	}

	if (pWaterNode != nullptr)
	{
		gEnv->sceneManager->getRootSceneNode()->removeAndDestroyChild("WaterPlane");
		pWaterNode = nullptr;
	}

	if (pBottomNode != nullptr)
	{
		gEnv->sceneManager->destroyEntity("bplane");
		gEnv->sceneManager->getRootSceneNode()->removeAndDestroyChild("BottomWaterPlane");
		pBottomNode = nullptr;
	}

	wHeight = wbHeight = 0;
	mRenderCamera = nullptr;
	waterSceneMgr = nullptr;

	if (rttTex1)
	{
		//vRtt1->clear();
		rttTex1->removeAllListeners();
		rttTex1 = nullptr;
	}

	if (rttTex2)
	{
		//vRtt2->clear();
		rttTex2->removeAllListeners();
		rttTex2 = nullptr;
	}
}

void Water::processWater(int mType)
{
	waterPlane.normal = Vector3::UNIT_Y;
	waterPlane.d = 0;

	if (mType == WATER_FULL_QUALITY || mType == WATER_FULL_SPEED || mType == WATER_REFLECT)
	{
		// Check prerequisites first
		const RenderSystemCapabilities* caps = Root::getSingleton().getRenderSystem()->getCapabilities();
		if (!caps->hasCapability(RSC_VERTEX_PROGRAM) || !(caps->hasCapability(RSC_FRAGMENT_PROGRAM)))
		{
			OGRE_EXCEPT(1, "Your card does not support vertex and fragment programs, so cannot "
				"run Water effects. Sorry!",
				"Water effects");
		}
		else
		{
			if (!GpuProgramManager::getSingleton().isSyntaxSupported("arbfp1") &&
				!GpuProgramManager::getSingleton().isSyntaxSupported("ps_2_0") &&
				!GpuProgramManager::getSingleton().isSyntaxSupported("ps_1_4")
				)
			{
				OGRE_EXCEPT(1, "Your card does not support advanced fragment programs, "
					"so cannot run Water effects. Sorry!",
					"Water effects");
			}
		}
		// Ok
		// Define a floor plane mesh
		reflectionPlane.normal = Vector3::UNIT_Y;
		reflectionPlane.d = - 0.15;
		refractionPlane.normal = -Vector3::UNIT_Y;
		refractionPlane.d =  0.15;

		if (mType == WATER_FULL_QUALITY || mType == WATER_FULL_SPEED)
		{
			TexturePtr rttTex1Ptr = TextureManager::getSingleton().createManual("Refraction", ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, TEX_TYPE_2D, 512, 512, 0, PF_R8G8B8, TU_RENDERTARGET, new ResourceBuffer());
			rttTex1 = rttTex1Ptr->getBuffer()->getRenderTarget();
			{
				mRefractCam = gEnv->sceneManager->createCamera("RefractCam");
				mRefractCam->setNearClipDistance(mRenderCamera->getNearClipDistance());
				mRefractCam->setFarClipDistance(mRenderCamera->getFarClipDistance());
				mRefractCam->setAspectRatio(
					(Real)RoR::Application::GetOgreSubsystem()->GetRenderWindow()->getViewport(0)->getActualWidth() /
					(Real)RoR::Application::GetOgreSubsystem()->GetRenderWindow()->getViewport(0)->getActualHeight());

				vRtt1 = rttTex1->addViewport(mRefractCam);
				vRtt1->setClearEveryFrame(true);
				vRtt1->setBackgroundColour(fade);
				//            v->setBackgroundColour( ColourValue::Black );


				MaterialPtr mat = MaterialManager::getSingleton().getByName("Examples/FresnelReflectionRefraction");
				mat->getTechnique(0)->getPass(0)->getTextureUnitState(2)->setTextureName("Refraction");

				mat = MaterialManager::getSingleton().getByName("Examples/FresnelReflectionRefractioninverted");
				mat->getTechnique(0)->getPass(0)->getTextureUnitState(2)->setTextureName("Refraction");

				vRtt1->setOverlaysEnabled(false);

				rttTex1->addListener(&mRefractionListener);

				//optimisation
				rttTex1->setAutoUpdated(false);

				// Also clip
				mRefractCam->enableCustomNearClipPlane(refractionPlane);
			}
		}

		TexturePtr rttTex2Ptr = TextureManager::getSingleton().createManual("Reflection", ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, TEX_TYPE_2D, 512, 512, 0, PF_R8G8B8, TU_RENDERTARGET, new ResourceBuffer());
		rttTex2 = rttTex2Ptr->getBuffer()->getRenderTarget();
		{
			mReflectCam = gEnv->sceneManager->createCamera("ReflectCam");
			mReflectCam->setNearClipDistance(mRenderCamera->getNearClipDistance());
			mReflectCam->setFarClipDistance(mRenderCamera->getFarClipDistance());
			mReflectCam->setAspectRatio(
				(Real)RoR::Application::GetOgreSubsystem()->GetRenderWindow()->getViewport(0)->getActualWidth() /
				(Real)RoR::Application::GetOgreSubsystem()->GetRenderWindow()->getViewport(0)->getActualHeight());

			vRtt2 = rttTex2->addViewport(mReflectCam);
			vRtt2->setClearEveryFrame(true);
			vRtt2->setBackgroundColour(fade);

			MaterialPtr mat;
			if (mType == WATER_FULL_QUALITY || mType == WATER_FULL_SPEED)
			{
				mat = MaterialManager::getSingleton().getByName("Examples/FresnelReflectionRefraction");
				mat->getTechnique(0)->getPass(0)->getTextureUnitState(1)->setTextureName("Reflection");

				mat = MaterialManager::getSingleton().getByName("Examples/FresnelReflectionRefractioninverted");
				mat->getTechnique(0)->getPass(0)->getTextureUnitState(1)->setTextureName("Reflection");
			}
			else
			{
				mat = MaterialManager::getSingleton().getByName("Examples/FresnelReflection");
				mat->getTechnique(0)->getPass(0)->getTextureUnitState(1)->setTextureName("Reflection");
			}

			vRtt2->setOverlaysEnabled(false);

			rttTex2->addListener(&mReflectionListener);

			//optimisation
			rttTex2->setAutoUpdated(false);

			// set up linked reflection
			mReflectCam->enableReflection(waterPlane);
			// Also clip
			mReflectCam->enableCustomNearClipPlane(reflectionPlane);
		}

		mprt = MeshManager::getSingleton().createPlane("ReflectPlane",
			ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
			waterPlane,
			mapSize.x * mScale, mapSize.z * mScale, WAVEREZ, WAVEREZ, true, 1, 50, 50, Vector3::UNIT_Z, HardwareBuffer::HBU_DYNAMIC_WRITE_ONLY_DISCARDABLE);

		pPlaneEnt = gEnv->sceneManager->createEntity("plane", "ReflectPlane");
		if (mType == WATER_FULL_QUALITY || mType == WATER_FULL_SPEED)
			pPlaneEnt->setMaterialName("Examples/FresnelReflectionRefraction");
		else
			pPlaneEnt->setMaterialName("Examples/FresnelReflection");
	}
	else
	{
		mprt = MeshManager::getSingleton().createPlane("WaterPlane",
			ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
			waterPlane,
			mapSize.x * mScale, mapSize.z * mScale, WAVEREZ, WAVEREZ, true, 1, 50, 50, Vector3::UNIT_Z, HardwareBuffer::HBU_DYNAMIC_WRITE_ONLY_DISCARDABLE);
		pPlaneEnt = gEnv->sceneManager->createEntity("plane", "WaterPlane");
		pPlaneEnt->setMaterialName("tracks/basicwater");
	}

	//position
	pWaterNode = gEnv->sceneManager->getRootSceneNode()->createChildSceneNode("WaterPlane");
	pWaterNode->attachObject(pPlaneEnt);
	pWaterNode->setPosition(Vector3((mapSize.x * mScale) / 2, wHeight, (mapSize.z * mScale) / 2));

	//bottom
	bottomPlane.normal = Vector3::UNIT_Y;
	bottomPlane.d = -wbHeight; //30m below waterline
	MeshManager::getSingleton().createPlane("BottomPlane",
		ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
		bottomPlane,
		mapSize.x * mScale, mapSize.z * mScale, 1, 1, true, 1, 1, 1, Vector3::UNIT_Z);
	Entity *pE = gEnv->sceneManager->createEntity("bplane", "BottomPlane");
	pE->setMaterialName("tracks/seabottom");

	//position
	pBottomNode = gEnv->sceneManager->getRootSceneNode()->createChildSceneNode("BottomWaterPlane");
	pBottomNode->attachObject(pE);
	pBottomNode->setPosition(Vector3((mapSize.x * mScale) / 2, 0, (mapSize.z * mScale) / 2));

	//setup for waves
	wbuf = mprt->sharedVertexData->vertexBufferBinding->getBuffer(0);

	if (wbuf->getSizeInBytes() == (WAVEREZ + 1)*(WAVEREZ + 1) * 32)
	{
		wbuffer = (float*)malloc(wbuf->getSizeInBytes());
		wbuf->readData(0, wbuf->getSizeInBytes(), wbuffer);
	}
	else wbuffer = 0;
}

bool Water::allowUnderWater()
{
	return false;
}

void Water::setVisible(bool value)
{
	visible = value;
	if (pPlaneEnt)
		pPlaneEnt->setVisible(value);
	if (pWaterNode)
		pWaterNode->setVisible(value);
	if (pBottomNode)
		pBottomNode->setVisible(value);

}

void Water::setFadeColour(ColourValue ambient)
{
	// update the viewports background colour!
	if (vRtt1) vRtt1->setBackgroundColour(ambient);
	if (vRtt2) vRtt2->setBackgroundColour(ambient);
}


void Water::moveTo(float centerheight)
{
	if (mType==WATER_FULL_QUALITY || mType==WATER_FULL_SPEED || mType==WATER_REFLECT)
		updateReflectionPlane(centerheight);
}

void Water::showWave(Vector3 refpos)
{
	int px,pz;
	if (!wbuffer) return;
	for (px=0; px<WAVEREZ+1; px++)
	{
		for (pz=0; pz<WAVEREZ+1; pz++)
		{
			Vector3 mapsize = gEnv->terrainManager->getMaxTerrainSize();
			wbuffer[(pz*(WAVEREZ+1)+px)*8+1]=getHeightWaves(refpos+Vector3((mapsize.x * mScale)/2-(float)px*(mapsize.x * mScale)/WAVEREZ, 0, (float)pz*(mapsize.z * mScale)/WAVEREZ-(mapsize.z * mScale)/2));
		}
	}
	//normals
	for (px=0; px<WAVEREZ+1; px++)
	{
		for (pz=0; pz<WAVEREZ+1; pz++)
		{
			int left=px-1; if (left<0) left=0;
			int right=px+1; if (right>WAVEREZ) right=WAVEREZ;
			int up=pz-1; if (up<0) up=0;
			int down=pz+1; if (down>WAVEREZ) down=WAVEREZ;
			Vector3 normal=(Vector3(wbuffer+((pz*(WAVEREZ+1)+left)*8))-Vector3(wbuffer+((pz*(WAVEREZ+1)+right)*8))).crossProduct(Vector3(wbuffer+((up*(WAVEREZ+1)+px)*8))-Vector3(wbuffer+((down*(WAVEREZ+1)+px)*8)));
			normal.normalise();
			wbuffer[(pz*(WAVEREZ+1)+px)*8+3]=normal.x;
			wbuffer[(pz*(WAVEREZ+1)+px)*8+4]=normal.y;
			wbuffer[(pz*(WAVEREZ+1)+px)*8+5]=normal.z;
		}
	}
//	wbuf->lock(HardwareBuffer::HBL_DISCARD);
	wbuf->writeData(0, (WAVEREZ+1)*(WAVEREZ+1)*32, wbuffer, true);
//	if (wbuf->isLocked())
//		wbuf->unlock();
}

bool Water::isCameraUnderWater()
{
	if (mRenderCamera)
	{
		return (mRenderCamera->getPosition().y < getHeightWaves(mRenderCamera->getPosition()));
	}
	return false;
}

void Water::update()
{
	if (!visible || !mRenderCamera)
		return;
	
	if (pWaterNode)
	{
		Vector3 mapSize = gEnv->terrainManager->getMaxTerrainSize();
		Vector3 cameraPos(mRenderCamera->getPosition().x, wHeight, mRenderCamera->getPosition().z);
		Vector3 sightPos(cameraPos);

		Ray lineOfSight(mRenderCamera->getPosition(), mRenderCamera->getDirection());
		Plane waterPlane(Vector3::UNIT_Y, Vector3::UNIT_Y * wHeight);

		std::pair< bool, Real > intersection = lineOfSight.intersects(waterPlane);

		if (intersection.first && intersection.second > 0.0f)
			sightPos = lineOfSight.getPoint(intersection.second);

		Real offset = std::min(cameraPos.distance(sightPos), std::min(mapSize.x, mapSize.z) / 2.0f);
		
		Vector3 waterPos = cameraPos + (sightPos - cameraPos).normalisedCopy() * offset;
		Vector3 bottomPos = Vector3(waterPos.x, wbHeight, waterPos.z);

		if (waterPos.distance(pWaterNode->getPosition()) > 200.0f || ForceUpdate)
		{
			pWaterNode->setPosition(Vector3(waterPos.x, wHeight, waterPos.z));
			pBottomNode->setPosition(bottomPos);
			ForceUpdate = false; //Happens only once
		}
		if (haswaves) showWave(pWaterNode->getPosition());
	}

	bool underwater = isCameraUnderWater();
	static bool lastWaterMode = false;
	bool underWaterModeChanged = false;
	if (underwater != lastWaterMode)
	{
		underWaterModeChanged = true;
		lastWaterMode = underwater;
	}

	static ColourValue savedFogColor;
	static int savedFogMode=0;
	static float savedFogStart, savedFogEnd, savedFogDensity;
	if (underwater && underWaterModeChanged)
	{
		/*
		// TODO!
		savedFogColor = gEnv->sceneManager->getFogColour();
		savedFogMode  = gEnv->sceneManager->getFogMode();
		savedFogStart = gEnv->sceneManager->getFogStart();
		savedFogEnd   = gEnv->sceneManager->getFogEnd();
		savedFogDensity = gEnv->sceneManager->getFogDensity();
		ColourValue fogColour = ColourValue(0.2,0.28,0.8);
		gEnv->sceneManager->setFog(FOG_LINEAR, fogColour, 0, 5, 10);
		*/
	} else if(!underwater && underWaterModeChanged)
	{
		// TODO!
		//gEnv->sceneManager->setFog((FogMode)savedFogMode, savedFogColor, savedFogDensity, savedFogStart, savedFogEnd);
	}

	framecounter++;
	if (mType==WATER_FULL_SPEED)
	{
		if (framecounter%2)
		{
			mReflectCam->setOrientation(mRenderCamera->getOrientation());
			mReflectCam->setPosition(mRenderCamera->getPosition());
			mReflectCam->setFOVy(mRenderCamera->getFOVy());
			rttTex2->update();
		}
		else
		{
			mRefractCam->setOrientation(mRenderCamera->getOrientation());
			mRefractCam->setPosition(mRenderCamera->getPosition());
			mRefractCam->setFOVy(mRenderCamera->getFOVy());
			rttTex1->update();
		}

		//if(underwater && underWaterModeChanged)
		//	pPlaneEnt->setMaterialName("Examples/FresnelReflectionRefractioninverted");
		//else if(!underwater && underWaterModeChanged)
		//	pPlaneEnt->setMaterialName("Examples/FresnelReflectionRefraction");
	} else if (mType==WATER_FULL_QUALITY)
	{
		mReflectCam->setOrientation(mRenderCamera->getOrientation());
		mReflectCam->setPosition(mRenderCamera->getPosition());
		mReflectCam->setFOVy(mRenderCamera->getFOVy());
		rttTex2->update();
		mRefractCam->setOrientation(mRenderCamera->getOrientation());
		mRefractCam->setPosition(mRenderCamera->getPosition());
		mRefractCam->setFOVy(mRenderCamera->getFOVy());
		rttTex1->update();

		//if(underwater && underWaterModeChanged)
		//	pPlaneEnt->setMaterialName("Examples/FresnelReflectionRefractioninverted");
		//else if(!underwater && underWaterModeChanged)
		//	pPlaneEnt->setMaterialName("Examples/FresnelReflectionRefraction");

	}
	else if (mType==WATER_REFLECT)
	{
		mReflectCam->setOrientation(mRenderCamera->getOrientation());
		mReflectCam->setPosition(mRenderCamera->getPosition());
		mReflectCam->setFOVy(mRenderCamera->getFOVy());
		rttTex2->update();
	}
}

void Water::prepareShutdown()
{
	if (rttTex1) rttTex1->removeListener(&mRefractionListener);
	if (rttTex2) rttTex2->removeListener(&mReflectionListener);
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
	// no waves?
	if (!haswaves)
	{
		// constant height, sea is flat as pancake
		return wHeight;
	}

	// uh, some upper limit?!
	if (pos.y > wHeight + maxampl)
		return wHeight;

	float waveheight = getWaveHeight(pos);
	// we will store the result in this variable, init it with the default height
	float result = wHeight;
	// now walk through all the wave trains. One 'train' is one sin/cos set that will generate once wave. All the trains together will sum up, so that they generate a 'rough' sea
	for (int i=0; i<free_wavetrain; i++)
	{
		// calculate the amplitude that this wave will have. wavetrains[i].amplitude is read from the config
		float amp = wavetrains[i].amplitude * waveheight;
		// upper limit: prevent too big waves by setting an upper limit
		if (amp > wavetrains[i].maxheight)
			amp = wavetrains[i].maxheight;
		// now the main thing:
		// calculate the sinus with the values of the config file and add it to the result
		result += amp * sin(Math::TWO_PI * ( \
											(gEnv->mrTime * wavetrains[i].wavespeed \
											+ sin(wavetrains[i].direction) * pos.x \
											+ cos(wavetrains[i].direction) * pos.z \
											) \
											/ wavetrains[i].wavelength) \
											);
	}
	// return the summed up waves
	return result;
}

bool Water::isUnderWater(Vector3 pos)
{
	float waterheight = wHeight;

	if (haswaves)
	{
		float waveheight = getWaveHeight(pos);

		if (pos.y > wHeight + maxampl * waveheight || pos.y > wHeight + maxampl)
			return false;

		waterheight = getHeightWaves(pos);
	}

	return pos.y < waterheight;
}

Vector3 Water::getVelocity(Vector3 pos)
{
	if (!haswaves) return Vector3::ZERO;

	float waveheight = getWaveHeight(pos);

	if (pos.y > wHeight + maxampl) return Vector3::ZERO;
	
	Vector3 result(Vector3::ZERO);

	for (int i=0; i<free_wavetrain; i++)
	{
		float amp=wavetrains[i].amplitude*waveheight;
		if (amp>wavetrains[i].maxheight) amp=wavetrains[i].maxheight;
		float speed=6.28318*amp/(wavetrains[i].wavelength/wavetrains[i].wavespeed);
		result.y+=speed*cos(6.28318*((gEnv->mrTime*wavetrains[i].wavespeed+sin(wavetrains[i].direction)*pos.x+cos(wavetrains[i].direction)*pos.z)/wavetrains[i].wavelength));
		result+=Vector3(sin(wavetrains[i].direction), 0, cos(wavetrains[i].direction))*speed*sin(6.28318*((gEnv->mrTime*wavetrains[i].wavespeed+sin(wavetrains[i].direction)*pos.x+cos(wavetrains[i].direction)*pos.z)/wavetrains[i].wavelength));
	}

	return result;
}

void Water::updateReflectionPlane(float h)
{
	//Ray ra=gEnv->ogreCamera->getCameraToViewportRay(0.5,0.5);
	//std::pair<bool, Real> mpair=ra.intersects(Plane(Vector3::UNIT_Y, -height));
	//if (mpair.first) h=ra.getPoint(mpair.second).y;

	bool underwater = isCameraUnderWater();
	if(underwater)
	{
		reflectionPlane.normal = -Vector3::UNIT_Y;
		refractionPlane.normal = Vector3::UNIT_Y;
		reflectionPlane.d = h+0.15;
		refractionPlane.d = -h+0.15;
		waterPlane.d = -h;
	} else
	{
		reflectionPlane.normal = Vector3::UNIT_Y;
		refractionPlane.normal = -Vector3::UNIT_Y;
		reflectionPlane.d = -h+0.15;
		refractionPlane.d = h+0.15;
		waterPlane.d = -h;
	}

	if (mRefractCam) mRefractCam->enableCustomNearClipPlane(refractionPlane);
	if (mReflectCam)
	{
		mReflectCam->enableReflection(waterPlane);
		mReflectCam->enableCustomNearClipPlane(reflectionPlane);
	};

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
	// calculate how high the waves should be at this point
	//  (mapsize.x * mScale) / 2 = terrain width / 2
	//  (mapsize.z * mScale) / 2 = terrain height / 2
	// calculates the distance to the center of the terrain and dives it through 3.000.000
	float waveheight = (pos - Vector3((mapSize.x * mScale) / 2, wHeight, (mapSize.z * mScale) / 2)).squaredLength() / 3000000.0;
	
	return waveheight;
}
