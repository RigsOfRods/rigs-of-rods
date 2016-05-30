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

#include "Mirror.h"

#include "Application.h"
#include "Beam.h"

#include <Ogre.h>
#include <OgreMaterial.h>
#include <OgreSubsystem.h>

using namespace Ogre;

Mirror::Mirror(SceneNode *sceneNode, int mirrorType) :
	  mMat(MaterialManager::getSingleton().getByName("mirror"))
	, mMirrorCam(gEnv->sceneManager->createCamera("MirrorCam"))
	, mMirrorNode(sceneNode)
	, mMirrorType(mirrorType)
	, mRttTex(nullptr)
{
	TexturePtr rttTexPtr = TextureManager::getSingleton().createManual("mirrortexture"
			, ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME
			, TEX_TYPE_2D
			, 128
			, 256
			, 0
			, PF_R8G8B8
			, TU_RENDERTARGET);

	mRttTex = rttTexPtr->getBuffer()->getRenderTarget();
	
	mMirrorCam->setNearClipDistance(0.2f);
	mMirrorCam->setFarClipDistance(gEnv->mainCamera->getFarClipDistance());
	mMirrorCam->setFOVy(Degree(50));
	mMirrorCam->setAspectRatio(
		((Real)RoR::Application::GetOgreSubsystem()->GetRenderWindow()->getViewport(0)->getActualWidth() / 
		 (Real)RoR::Application::GetOgreSubsystem()->GetRenderWindow()->getViewport(0)->getActualHeight()) / 2.0f);

	Viewport *v = mRttTex->addViewport(mMirrorCam);
	v->setClearEveryFrame(true);
	v->setBackgroundColour(gEnv->mainCamera->getViewport()->getBackgroundColour());

	mMat->getTechnique(0)->getPass(0)->getTextureUnitState(0)->setTextureName("mirrortexture");
	mMat->getTechnique(0)->getPass(0)->setLightingEnabled(false);
	v->setOverlaysEnabled(false);
}

Mirror::~Mirror()
{
}

void Mirror::update(Beam* truck)
{
	Vector3 normal = Vector3::ZERO;
	Vector3 center = Vector3::ZERO;
	Radian roll = Degree(360) - Radian(asin(truck->getDirection().dotProduct(Vector3::UNIT_Y)));

	if (mMirrorType == +1)
	{
		normal = mMirrorNode->getOrientation() * Vector3(cos(truck->leftMirrorAngle), sin(truck->leftMirrorAngle), 0.0f);
		center = mMirrorNode->getPosition() + mMirrorNode->getOrientation() * Vector3(0.07f,-0.22f,0);
	}
	if (mMirrorType == -1)
	{
		normal = mMirrorNode->getOrientation() * Vector3(cos(truck->rightMirrorAngle), sin(truck->rightMirrorAngle), 0.0f);
		center = mMirrorNode->getPosition() + mMirrorNode->getOrientation() * Vector3(0.07f, +0.22f, 0);
	}

	Plane plane = Plane(normal, center);
	Vector3 project = plane.projectVector(gEnv->mainCamera->getPosition() - center);

	mMirrorCam->setPosition(center); 
	mMirrorCam->lookAt(gEnv->mainCamera->getPosition() - 2.0f * project); 
	mMirrorCam->roll(roll);
}
