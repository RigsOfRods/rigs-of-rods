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
#include "Heathaze.h"

#include "ResourceBuffer.h"

using namespace Ogre;

HeatHazeListener::HeatHazeListener() :
	RenderTargetListener()
{
}

void HeatHazeListener::preRenderTargetUpdate(const RenderTargetEvent& evt)
{
	// Hide everything
	//gEnv->ogreSceneManager->setFindVisibleObjects(false);

	// TODO: hide objects between the heathaze and the viewer, so no heathaze is visible on them

}
void HeatHazeListener::postRenderTargetUpdate(const RenderTargetEvent& evt)
{
	// Show everything
	//gEnv->ogreSceneManager->setFindVisibleObjects(true);
}


HeatHaze::HeatHaze() : rttTex(0), listener(0)
{
	TexturePtr rttTexPtr = TextureManager::getSingleton().createManual("heathaze_rtt", ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, TEX_TYPE_2D, gEnv->mainCamera->getViewport()->getWidth(), gEnv->mainCamera->getViewport()->getHeight(), 0, PF_R8G8B8, TU_RENDERTARGET, new ResourceBuffer());
	rttTex = rttTexPtr->getBuffer()->getRenderTarget();
	{
		/*
		// we use the main camera now
		mHazeCam = gEnv->ogreSceneManager->createCamera("Hazecam");
		mHazeCam->setNearClipDistance(1.0);
		mHazeCam->setFarClipDistance(1000.0);
		mHazeCam->setPosition(Vector3(0, 0, 0));
		*/

		//mHazeCam->setAspectRatio(2.0);

		// setup viewport
		Viewport *v = rttTex->addViewport(gEnv->mainCamera);
		//v->setClearEveryFrame(true);
		//v->setBackgroundColour(ColourValue::Black);
		v->setOverlaysEnabled(false);
		

		// setup projected material
		MaterialPtr mat = MaterialManager::getSingleton().getByName("tracks/HeatHazeMat");
		tex = mat->getTechnique(0)->getPass(0)->getTextureUnitState(1);
		tex->setTextureName("heathaze_rtt");
		tex->setProjectiveTexturing(true, gEnv->mainCamera);

		listener = new HeatHazeListener();
		rttTex->addListener(listener);
		rttTex->setAutoUpdated(false);
	}
}

void HeatHaze::setEnable(bool en)
{
	rttTex->setActive(en);
}

void HeatHaze::update()
{
	if (rttTex)
		rttTex->update();
}

void HeatHaze::prepareShutdown()
{
	if (rttTex) rttTex->removeListener(listener);
	if (listener)
	{
		delete listener;
		listener=0;
	}
}
