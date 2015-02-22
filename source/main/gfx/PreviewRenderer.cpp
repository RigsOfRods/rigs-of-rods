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
#include "PreviewRenderer.h"

#include "Beam.h"
#include "BeamFactory.h"
#include "PlatformUtils.h"
#include "RoRWindowEventUtilities.h"
#include "Settings.h"
#include "CaelumManager.h"
#include "Utils.h"
#include "Application.h"
#include "OgreSubsystem.h"
#include "RoRFrameListener.h"

using namespace Ogre;

PreviewRenderer::PreviewRenderer()
{
	fn = SSETTING("OPT_IMGPATH", "");
	if (fn.empty()) return;
	LOG("previewRenderer initialized");
}

PreviewRenderer::~PreviewRenderer()
{
}

void PreviewRenderer::render()
{
	if (!gEnv->cameraManager) return;

	LOG("starting previewRenderer...");
	Beam *truck = BeamFactory::getSingleton().getCurrentTruck();
	SceneManager *sceneMgr = gEnv->sceneManager;
	Viewport *vp = RoR::Application::GetOgreSubsystem()->GetViewport();

	// disable skybox
	//sceneMgr->setSkyBox(false, "");
	// disable fog
	//sceneMgr->setFog(FOG_NONE);
	// disable shadows
	//vp->setShadowsEnabled(false);
	// white background
	//vp->setBackgroundColour(ColourValue::White);
	vp->setClearEveryFrame(true);

	// better mipmapping
	MaterialManager::getSingleton().setDefaultAnisotropy(8);
	MaterialManager::getSingleton().setDefaultTextureFiltering(TFO_ANISOTROPIC);

	// now reset the truck to the scene's center
	//Vector3 tsize = AxisAlignedBox(truck->minx, truck->miny, truck->minz, truck->maxx, truck->maxy, truck->maxz).getSize();
	//Vector3 pos0 = truck->nodes[0].iPosition;
	//truck->resetPosition(pos0.x + tsize.x, pos0.z + tsize.z, true);

	// now switch on headlights and stuff
	BeamFactory::getSingleton().setCurrentTruck(truck->trucknum);
	truck->reset();
	//truck->lightsToggle();
	//truck->beaconsToggle();
	//truck->setBlinkType(BLINK_WARN);
	//truck->toggleCustomParticles();
	
	
	// DO NOT accelerate
	//if (truck->engine)
	//	truck->engine->autoSetAcc(0.2f);

	// steer a bit
	truck->hydrodircommand = -0.6f;

	// run the beam engine for ten seconds
	LOG("simulating truck for ten seconds ...");
	float time=0;
	Real dt = 0.0333; // 0.0333 seconds / frame = 30 FPS
	while(time < 10)
	{
		// run the engine for ten virtual seconds
		BeamFactory::getSingleton().calcPhysics(dt);
		time += dt;
	}
	BeamFactory::getSingleton().updateVisual(dt);
	LOG("simulation done");

	// calculate min camera radius for truck and the trucks center

	// first: average pos
	//truck->updateTruckPosition();

	// then camera radius:
	float minCameraRadius = 0;
	for (int i=0; i < truck->free_node; i++)
	{
		Real dist = truck->nodes[i].AbsPosition.distance(truck->getPosition());
		if (dist > minCameraRadius)
			minCameraRadius = dist;
	}
	minCameraRadius *= 2.1f;

	SceneNode *camNode = sceneMgr->getRootSceneNode()->createChildSceneNode();

	Camera *cam = gEnv->mainCamera;
	cam->setLodBias(1000.0f);
	cam->setAspectRatio(1.0f);

	// calculate the camera-distance
	float fov = 60;

	// now calculate the bounds with respect of the nodes and beams
	AxisAlignedBox aab = getWorldAABB(truck->getSceneNode());
	aab.merge(truck->boundingBox);

	Vector3 maxVector = aab.getMaximum();
	Vector3 minVector = aab.getMinimum();
	LOG("Object bounds: "+TOSTRING(minVector)+" - "+TOSTRING(maxVector));

	int z1 = (maxVector.z-minVector.z)/2 + (((maxVector.x-minVector.x)/2) / tan(fov / 2));
	int z2 = (maxVector.z-minVector.z)/2 + (((maxVector.y-minVector.y)/2) / tan(fov / 2));
	
	//camNode->setPosition(truckPos);
	camNode->setPosition(aab.getCenter());
	camNode->attachObject(cam);

	cam->setFOVy(Angle(fov));
	cam->setNearClipDistance(1.0f);
	cam->setPosition(Vector3(0,12,std::max(z1, z2)+1));

	Real radius = cam->getPosition().length();
	cam->setPosition(0.0,0.0,0.0);
	cam->setOrientation(Quaternion::IDENTITY);
	cam->yaw(Degree(0));
	cam->pitch(Degree(-45));
	
	cam->moveRelative(Vector3(0.0,0.0,radius));

	cam->setAspectRatio(1.0f);

	// only render our object
	//sceneMgr->clearSpecialCaseRenderQueues();
	//sceneMgr->addSpecialCaseRenderQueue(RENDER_QUEUE_6);
	//sceneMgr->setSpecialCaseRenderQueueMode(SceneManager::SCRQM_INCLUDE);

	//render2dviews(truck, cam, minCameraRadius);
	render3dpreview(truck, cam, minCameraRadius, camNode);
}

void PreviewRenderer::render2dviews(Beam *truck, Camera *cam, float minCameraRadius)
{
	float ominCameraRadius = minCameraRadius;
	Vector3 truckPos = truck->getPosition();

	for (int o=0; o<2; o++)
	{
		String oext = "ortho.";
		if     (o == 0)
		{
			cam->setProjectionType(PT_ORTHOGRAPHIC);
			minCameraRadius = ominCameraRadius * 2.1f;
		}
		else if (o == 1)
		{
			oext = "3d.";
			cam->setProjectionType(PT_PERSPECTIVE);
			minCameraRadius = ominCameraRadius * 2.4f;
		}

		for (int i=0; i<2; i++)
		{
			String ext = "normal.";
			if (i == 0)
			{
				truck->hideSkeleton(true);
			} else if (i == 1)
			{
				ext = "skeleton.";
				// now show the skeleton
				truck->showSkeleton(true, true);
				truck->updateSimpleSkeleton();
			}
			ext = oext + ext;

			// 3d first :)
			cam->setPosition(Vector3(truckPos.x - minCameraRadius, truckPos.y * 1.5f, truckPos.z - minCameraRadius));
			cam->lookAt(truckPos);
			render(ext + "perspective");

			// right
			cam->setPosition(Vector3(truckPos.x, truckPos.y, truckPos.z - minCameraRadius));
			cam->lookAt(truckPos);
			render(ext + "right");

			// left
			cam->setPosition(Vector3(truckPos.x, truckPos.y, truckPos.z + minCameraRadius));
			cam->lookAt(truckPos);
			render(ext + "left");

			// front
			cam->setPosition(Vector3(truckPos.x - minCameraRadius, truckPos.y, truckPos.z));
			cam->lookAt(truckPos);
			render(ext + "front");

			// back
			cam->setPosition(Vector3(truckPos.x + minCameraRadius, truckPos.y, truckPos.z));
			cam->lookAt(truckPos);
			render(ext + "back");

			// top
			cam->setPosition(Vector3(truckPos.x, truckPos.y + minCameraRadius, truckPos.z + 0.01f));
			cam->lookAt(truckPos);
			render(ext + "top");

			// bottom
			cam->setPosition(Vector3(truckPos.x, truckPos.y - minCameraRadius, truckPos.z + 0.01f));
			cam->lookAt(truckPos);
			render(ext + "bottom");

		}
	}
}

void PreviewRenderer::render3dpreview(Beam *truck, Camera *renderCamera, float minCameraRadius, SceneNode *camNode)
{
	int yaw_angles = 1;
	int pitch_angles = 32;
	TexturePtr renderTexture;
	uint32 textureSize = 1024;
	//renderTexture = TextureManager::getSingleton().createManual("3dpreview1", ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, TEX_TYPE_2D, textureSize * pitch_angles, textureSize * yaw_angles, 0, PF_A8R8G8B8, TU_RENDERTARGET);
	renderTexture = TextureManager::getSingleton().createManual("3dpreview1", ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, TEX_TYPE_2D, textureSize, textureSize, 32, 0, PF_A8R8G8B8, TU_RENDERTARGET, 0, false, 8, "Quality");
	
	RenderTexture *renderTarget = renderTexture->getBuffer()->getRenderTarget();
	renderTarget->setAutoUpdated(false);
	Viewport *renderViewport = renderTarget->addViewport(renderCamera);
	renderViewport->setOverlaysEnabled(false);
	renderViewport->setClearEveryFrame(true);
	//renderViewport->setShadowsEnabled(false);
	//renderViewport->setBackgroundColour(ColourValue(1, 1, 1, 0));

#ifdef USE_CAELUM
	
	if (gEnv->sky && gEnv->frameListener->loading_state == TERRAIN_LOADED)
	{
		gEnv->sky->notifyCameraChanged(renderCamera);
//		gEnv->terrainManager->getCaelumManager()->forceUpdate(0.01f);
	}
#endif // USE_CAELUM

	String skelmode = "normal";

	const float xDivFactor = 1.0f / pitch_angles;
	const float yDivFactor = 1.0f / yaw_angles;
	for (int s=0;s<2; s++)
	{
		if (s == 0)
		{
			truck->hideSkeleton(true);
			skelmode = "normal";
		} else if (s == 1)
		{
			skelmode = "skeleton";
			// now show the skeleton
			truck->showSkeleton(true, true);
			truck->updateSimpleSkeleton();
		}

		for (int o = 0; o < pitch_angles; ++o)
		{
			//4 pitch angle renders
			Radian pitch = Degree((360.0f * o) * xDivFactor);
			for (int i = 0; i < yaw_angles; ++i)
			{
				Radian yaw = Degree(-10); //Degree((20.0f * i) * yDivFactor - 10); //0, 45, 90, 135, 180, 225, 270, 315

				Real radius = minCameraRadius; //renderCamera->getPosition().length();
				renderCamera->setPosition(Vector3::ZERO);
				renderCamera->setOrientation(Quaternion::IDENTITY);
				renderCamera->yaw(Degree(pitch));
				renderCamera->pitch(Degree(yaw));
				renderCamera->moveRelative(Vector3(0.0, 0.0, radius));

 				char tmp[56];
				sprintf(tmp, "%03d_%03d.jpg", i, o); // use .png for transparancy
				String ifn = fn + skelmode + SSETTING("dirsep", "\\") + String(tmp);
    			
				if (RoR::PlatformUtils::FileExists(ifn.c_str()))
				{
					LOG("rending skipped - already existing [" + TOSTRING(yaw) + String(" / ") + TOSTRING(pitch) + String(" / ") + TOSTRING(radius) + String("] ") + ifn);
					continue;
				}

				//Render into the texture
				// only when rendering all images into one texture
				//renderViewport->setDimensions((float)(o) * xDivFactor, (float)(i) * yDivFactor, xDivFactor, yDivFactor);

				RoRWindowEventUtilities::messagePump();
				Root::getSingleton().renderOneFrame();
				renderTarget->update();
#ifdef USE_CAELUM
				
				if (gEnv->sky && gEnv->frameListener->loading_state == TERRAIN_LOADED)
				{
					gEnv->sky->forceUpdate(0.01f);
				}
#endif //USE_CAELUM

				renderTarget->writeContentsToFile(ifn);
				
				LOG("rendered [" + TOSTRING(yaw) + String(" / ") + TOSTRING(pitch) + String(" / ") + TOSTRING(radius) + String("] ") + ifn);
			}
		}
	}

	//Save RTT to file with respecting the temp dir
	//renderTarget->writeContentsToFile(fn + "all.png");

}


void PreviewRenderer::render(String ext)
{
	// create some screenshot
	RoRWindowEventUtilities::messagePump();
	Root::getSingleton().renderOneFrame();
	RoR::Application::GetOgreSubsystem()->GetRenderWindow()->update();
	RoR::Application::GetOgreSubsystem()->GetRenderWindow()->writeContentsToFile(fn+"."+ext+".jpg");
}
