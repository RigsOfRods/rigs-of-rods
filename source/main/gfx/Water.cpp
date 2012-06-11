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

#include "ResourceBuffer.h"
#include "RoRFrameListener.h"
#include "Settings.h"
#include "TerrainManager.h"

using namespace Ogre;

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

Water::Water(const Ogre::ConfigFile &mTerrainConfig)
{
	Vector3 mapsize = gEnv->terrainManager->getMaxTerrainSize();
	vRtt1 = vRtt2 = 0;
	mScale = 1.0f;
	if (mapsize.x < 1500)
		mScale = 1.5f;
	//reading wavefield
	visible=true;
	free_wavetrain=0;
	maxampl=0;

	haswaves = BSETTING("Waves", false);

	// disable waves in multiplayer
	if(gEnv->network)
		haswaves = false;

	// parse height
	float wheight = PARSEREAL(mTerrainConfig.getSetting("WaterLine", "General"));

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
	//theCam=camera;
	pTestNode=0;
	waterSceneMgr=gEnv->sceneManager;
	framecounter=0;
	height=wheight;
	orgheight=wheight;
	rttTex1=0;
	rttTex2=0;
	MeshPtr mprt;
	mReflectCam=0;
	mRefractCam=0;
	//wbuf=0;
	//ColourValue fade=camera->getViewport()->getBackgroundColour();
	ColourValue fade=gEnv->sceneManager->getFogColour();

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
		reflectionPlane.d = -wheight+0.15;
		refractionPlane.normal = -Vector3::UNIT_Y;
		refractionPlane.d = wheight+0.15;
		waterPlane.normal = Vector3::UNIT_Y;
		waterPlane.d = -wheight;

		if (mType == WATER_FULL_QUALITY || mType == WATER_FULL_SPEED)
		{
			TexturePtr rttTex1Ptr = TextureManager::getSingleton().createManual("Refraction", ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, TEX_TYPE_2D, 512, 512, 0, PF_R8G8B8, TU_RENDERTARGET, new ResourceBuffer());
			rttTex1 = rttTex1Ptr->getBuffer()->getRenderTarget();
			{
				mRefractCam = gEnv->sceneManager->createCamera("RefractCam");
				mRefractCam->setNearClipDistance(gEnv->mainCamera->getNearClipDistance());
				mRefractCam->setFarClipDistance(gEnv->mainCamera->getFarClipDistance());
				mRefractCam->setAspectRatio(
					(Real)gEnv->renderWindow->getViewport(0)->getActualWidth() /
					(Real)gEnv->renderWindow->getViewport(0)->getActualHeight());

				vRtt1 = rttTex1->addViewport( mRefractCam );
				vRtt1->setClearEveryFrame( true );
				vRtt1->setBackgroundColour( fade );
				//            v->setBackgroundColour( ColourValue::Black );


				MaterialPtr mat = MaterialManager::getSingleton().getByName("Examples/FresnelReflectionRefraction");
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
			mReflectCam->setNearClipDistance(gEnv->mainCamera->getNearClipDistance());
			mReflectCam->setFarClipDistance(gEnv->mainCamera->getFarClipDistance());
			mReflectCam->setAspectRatio(
				(Real)gEnv->renderWindow->getViewport(0)->getActualWidth() /
				(Real)gEnv->renderWindow->getViewport(0)->getActualHeight());

			vRtt2 = rttTex2->addViewport( mReflectCam );
			vRtt2->setClearEveryFrame( true );
			vRtt2->setBackgroundColour( fade );
			//            v->setBackgroundColour( ColourValue::Black );


			MaterialPtr mat ;
			if (mType==WATER_FULL_QUALITY || mType==WATER_FULL_SPEED) mat = MaterialManager::getSingleton().getByName("Examples/FresnelReflectionRefraction");
			else mat = MaterialManager::getSingleton().getByName("Examples/FresnelReflection");
			mat->getTechnique(0)->getPass(0)->getTextureUnitState(1)->setTextureName("Reflection");

			vRtt2->setOverlaysEnabled(false);

			rttTex2->addListener(&mReflectionListener);

			//optimisation
			rttTex2->setAutoUpdated(false);

			// set up linked reflection
			mReflectCam->enableReflection(waterPlane);
			// Also clip
			mReflectCam->enableCustomNearClipPlane(reflectionPlane);
		}

		mprt=MeshManager::getSingleton().createPlane("ReflectPlane",
			ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
			waterPlane,
			mapsize.x * mScale,mapsize.z * mScale,WAVEREZ,WAVEREZ,true,1,50,50,Vector3::UNIT_Z, HardwareBuffer::HBU_DYNAMIC_WRITE_ONLY_DISCARDABLE);
		pPlaneEnt = gEnv->sceneManager->createEntity( "plane", "ReflectPlane" );
		if (mType==WATER_FULL_QUALITY || mType==WATER_FULL_SPEED) pPlaneEnt->setMaterialName("Examples/FresnelReflectionRefraction");
		else pPlaneEnt->setMaterialName("Examples/FresnelReflection");
		//        gEnv->ogreSceneManager->getRootSceneNode()->createChildSceneNode()->attachObject(pPlaneEnt);
		//position
		pTestNode = gEnv->sceneManager->getRootSceneNode()->createChildSceneNode("WaterPlane");
		pTestNode->attachObject(pPlaneEnt);
		pTestNode->setPosition( Vector3((mapsize.x * mScale)/2,0,(mapsize.z * mScale)/2) );
	}
	else
	{
		//basic Water
		waterPlane.normal = Vector3::UNIT_Y;
		waterPlane.d = -wheight;
		mprt=MeshManager::getSingleton().createPlane("WaterPlane",
			ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
			waterPlane,
			mapsize.x * mScale,mapsize.z * mScale,WAVEREZ,WAVEREZ,true,1,50,50,Vector3::UNIT_Z, HardwareBuffer::HBU_DYNAMIC_WRITE_ONLY_DISCARDABLE);
		pPlaneEnt = gEnv->sceneManager->createEntity( "plane", "WaterPlane" );
		pPlaneEnt->setMaterialName("tracks/basicwater");
		//position
		pTestNode = gEnv->sceneManager->getRootSceneNode()->createChildSceneNode("WaterPlane");
		pTestNode->attachObject(pPlaneEnt);
		pTestNode->setPosition( Vector3((mapsize.x * mScale)/2,0,(mapsize.z * mScale)/2) );
	}
	//bottom
	bottomPlane.normal = Vector3::UNIT_Y;
	bottomPlane.d = -wheight+30.0; //30m below waterline
	MeshManager::getSingleton().createPlane("BottomPlane",
		ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
		bottomPlane,
		mapsize.x * mScale,mapsize.z * mScale,1,1,true,1,1,1,Vector3::UNIT_Z);
	Entity *pE = gEnv->sceneManager->createEntity( "bplane", "BottomPlane" );
	pE->setMaterialName("tracks/seabottom");
	//position
	pBottomNode = gEnv->sceneManager->getRootSceneNode()->createChildSceneNode("BottomWaterPlane");
	pBottomNode->attachObject(pE);
	pBottomNode->setPosition( Vector3((mapsize.x * mScale)/2,0,(mapsize.z * mScale)/2) );
	//setup for waves
	wbuf=mprt->sharedVertexData->vertexBufferBinding->getBuffer(0);
	if (wbuf->getSizeInBytes()==(WAVEREZ+1)*(WAVEREZ+1)*32)
	{
		wbuffer=(float*)malloc(wbuf->getSizeInBytes());
		wbuf->readData(0, wbuf->getSizeInBytes(), wbuffer);
	} else wbuffer=0;
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
	if (pTestNode)
		pTestNode->setVisible(value);
	if (pBottomNode)
		pBottomNode->setVisible(value);

}

void Water::setFadeColour(ColourValue ambient)
{
	// update the viewports background colour!
	if (vRtt1) vRtt1->setBackgroundColour(ambient);
	if (vRtt2) vRtt2->setBackgroundColour(ambient);
}


void Water::moveTo(Camera *cam, float centerheight)
{
	Vector3 mapsize = gEnv->terrainManager->getMaxTerrainSize();
	if (pTestNode)
	{
		Vector3 pos=cam->getPosition();
		Vector3 offset=cam->getDirection();
		offset.y=0;
		offset.normalise();
		pos = pos + offset * mapsize.x * mScale * 0.46666;
		pos.y=orgheight - height;
		pos.x=((int)pos.x/60)*60;
		pos.z=((int)pos.z/60)*60;
		pTestNode->setPosition(pos);
		pBottomNode->setPosition(pos);
		if (haswaves) showWave(pos);
		if (mType==WATER_FULL_QUALITY || mType==WATER_FULL_SPEED || mType==WATER_REFLECT) updateReflectionPlane(centerheight);
	}
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

void Water::update()
{
	if (!visible)
		return;
	framecounter++;
	if (mType==WATER_FULL_SPEED)
	{
		if (framecounter%2)
		{
			mReflectCam->setOrientation(gEnv->mainCamera->getOrientation());
			mReflectCam->setPosition(gEnv->mainCamera->getPosition());
			mReflectCam->setFOVy(gEnv->mainCamera->getFOVy());
			rttTex2->update();
		}
		else
		{
			mRefractCam->setOrientation(gEnv->mainCamera->getOrientation());
			mRefractCam->setPosition(gEnv->mainCamera->getPosition());
			mRefractCam->setFOVy(gEnv->mainCamera->getFOVy());
			rttTex1->update();
		}
	} else if (mType==WATER_FULL_QUALITY)
	{
		mReflectCam->setOrientation(gEnv->mainCamera->getOrientation());
		mReflectCam->setPosition(gEnv->mainCamera->getPosition());
		mReflectCam->setFOVy(gEnv->mainCamera->getFOVy());
		rttTex2->update();
		mRefractCam->setOrientation(gEnv->mainCamera->getOrientation());
		mRefractCam->setPosition(gEnv->mainCamera->getPosition());
		mRefractCam->setFOVy(gEnv->mainCamera->getFOVy());
		rttTex1->update();
	}
	else if (mType==WATER_REFLECT)
	{
		mReflectCam->setOrientation(gEnv->mainCamera->getOrientation());
		mReflectCam->setPosition(gEnv->mainCamera->getPosition());
		mReflectCam->setFOVy(gEnv->mainCamera->getFOVy());
		rttTex2->update();
	}
}

void Water::prepareShutdown()
{
	if (rttTex1) rttTex1->removeListener(&mRefractionListener);
	if (rttTex2) rttTex2->removeListener(&mReflectionListener);
}

float Water::getHeight() {return height;};

void Water::setHeight(float value)
{
	height = value;
	update();
}

float Water::getHeightWaves(Vector3 pos)
{
	// no waves?
	if (!haswaves)
	{
		// constant height, sea is flat as pancake
		return height;
	}

	// uh, some upper limit?!
	if (pos.y > height + maxampl)
		return height;

	// calculate how high the waves should be at this point
	//  (mapsize.x * mScale) / 2 = terrain width / 2
	//  (mapsize.z * mScale) / 2 = terrain height / 2
	// calculates the distance to the center of the terrain and dives it through 3.000.000
	Vector3 mapsize = gEnv->terrainManager->getMaxTerrainSize();
	float waveheight = (pos - Vector3((mapsize.x * mScale) / 2, height, (mapsize.z * mScale) / 2)).squaredLength() / 3000000.0;
	// we will store the result in this variable, init it with the default height
	float result = height;
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
											(mrtime * wavetrains[i].wavespeed \
											+ sin(wavetrains[i].direction) * pos.x \
											+ cos(wavetrains[i].direction) * pos.z \
											) \
											/ wavetrains[i].wavelength) \
											);
	}
	// return the summed up waves
	return result;
}

Vector3 Water::getVelocity(Vector3 pos)
{
	if (!haswaves) return Vector3::ZERO;

	if (pos.y>height+maxampl) return Vector3::ZERO;
	int i;
	Vector3 mapsize = gEnv->terrainManager->getMaxTerrainSize();
	float waveheight=(pos-Vector3((mapsize.x * mScale)/2, height, (mapsize.z * mScale)/2)).squaredLength()/3000000.0;
	Vector3 result=Vector3::ZERO;
	for (i=0; i<free_wavetrain; i++)
	{
		float amp=wavetrains[i].amplitude*waveheight;
		if (amp>wavetrains[i].maxheight) amp=wavetrains[i].maxheight;
		float speed=6.28318*amp/(wavetrains[i].wavelength/wavetrains[i].wavespeed);
		result.y+=speed*cos(6.28318*((mrtime*wavetrains[i].wavespeed+sin(wavetrains[i].direction)*pos.x+cos(wavetrains[i].direction)*pos.z)/wavetrains[i].wavelength));
		result+=Vector3(sin(wavetrains[i].direction), 0, cos(wavetrains[i].direction))*speed*sin(6.28318*((mrtime*wavetrains[i].wavespeed+sin(wavetrains[i].direction)*pos.x+cos(wavetrains[i].direction)*pos.z)/wavetrains[i].wavelength));
	}
	return result;
}



void Water::updateReflectionPlane(float h)
{
	//Ray ra=gEnv->ogreCamera->getCameraToViewportRay(0.5,0.5);
	//std::pair<bool, Real> mpair=ra.intersects(Plane(Vector3::UNIT_Y, -height));
	//if (mpair.first) h=ra.getPoint(mpair.second).y;
	reflectionPlane.d = -h+0.15;
	refractionPlane.d = h+0.15;
	waterPlane.d = -h;
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

void Water::framestep(float dt)
{
	update();
}
