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
#include "EnvironmentMap.h"

#include "Beam.h"
#include "Ogre.h"
#include "OgreOverlay.h"
#include "OgreOverlayManager.h"
#include "Settings.h"
#include "SkyManager.h"
#include "TerrainManager.h"
#include "MainThread.h"
#include "OgreSubsystem.h"

#include "Compositor/OgreCompositorWorkspace.h"
#include "Compositor/OgreCompositorWorkspaceDef.h"
#include "Compositor/OgreCompositorWorkspaceListener.h"
#include "Compositor/OgreCompositorChannel.h"


using namespace Ogre;

Envmap::Envmap() :
	  mInitiated(false)
	, mIsDynamic(false)
	, mRound(0)
	, updateRate(1)
{
	MovableObject::setDefaultVisibilityFlags(0x00000001 | 0x00000004);
	//RoR::Application::GetMainThreadLogic()->getMainWorkSpace()->setListener(this);

	//todo: remake ogre 2.0
	mIsDynamic = BSETTING("Envmap", false);
	updateRate = ISETTING("EnvmapUpdateRate", 1);

	// create the camera used to render to our cubemap
	mCubeCamera = gEnv->sceneManager->createCamera("CubeMapCamera", true, false);
	mCubeCamera->setFOVy(Degree(90));
	mCubeCamera->setAspectRatio(1);
	mCubeCamera->setFixedYawAxis(false);
	mCubeCamera->setNearClipDistance(5);

	//The default far clip distance is way too big for a cubemap-capable camara, which prevents
	//Ogre from better culling and prioritizing lights in a forward renderer.
	//TODO: Improve the Sky algorithm so that we don't need to use this absurd high number
	mCubeCamera->setFarClipDistance(10000);

	init();
}

Envmap::~Envmap()
{

}

void Envmap::update(Ogre::Vector3 center, Beam *beam /* = 0 */)
{
	if (mInitiated)
	{
		mCubeCamera->setPosition(center);
		for (int i = 0; i < updateRate; i++)
		{
			//mCubemapWorkspace->_update();
		}
	}
}

void Envmap::init()
{
	CompositorManager2 *compositorManager = RoR::Application::GetOgreSubsystem()->GetOgreRoot()->getCompositorManager2();

	const Ogre::IdString workspaceName("Environment_Mapping");
	if (!compositorManager->hasWorkspaceDefinition(workspaceName))
	{
		CompositorWorkspaceDef *workspaceDef = compositorManager->addWorkspaceDefinition(workspaceName);

		//Loaded from compositor file
		workspaceDef->connectOutput("EnvironmentRendererNode", 0);
	}

	tex = TextureManager::getSingleton().createManual("EnvironmentTexture", ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, TEX_TYPE_CUBE_MAP, 256, 256, 0, PF_R8G8B8, TU_RENDERTARGET);

	CompositorChannel channel;
	channel.target = tex->getBuffer(0)->getRenderTarget(); //Any of the render targets will do
	channel.textures.push_back(tex);

	mCubemapWorkspace = compositorManager->addWorkspace(gEnv->sceneManager, channel, mCubeCamera, workspaceName, true);
	mInitiated = true;
}
