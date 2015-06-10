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
#include "SurveyMapTextureCreator.h"

#include "BeamFactory.h"
#include "IWater.h"
#include "ResourceBuffer.h"
#include "SurveyMapManager.h"
#include "TerrainManager.h"

using namespace Ogre;

int SurveyMapTextureCreator::mCounter = 0;

SurveyMapTextureCreator::SurveyMapTextureCreator() :
	  mCamera(NULL)
	, mRttTex(NULL)
	, mStatics(NULL)
	, mTextureUnitState(NULL)
	, mViewport(NULL)
	, mMapCenter(Vector2::ZERO)
	, mMapSize(Vector3::ZERO)
	, mMapZoom(0.0f)
{
	mCounter++;
	init();
}

bool SurveyMapTextureCreator::init()
{
	//todo ogre 2.0
	return false;
}

void SurveyMapTextureCreator::setStaticGeometry(StaticGeometry *staticGeometry)
{
	mStatics = staticGeometry;
}

void SurveyMapTextureCreator::update()
{

}

String SurveyMapTextureCreator::getMaterialName()
{
	return "MapRttMat" + TOSTRING(mCounter);
}

String SurveyMapTextureCreator::getCameraName()
{
	return "MapRttCam" + TOSTRING(mCounter);
}

String SurveyMapTextureCreator::getTextureName()
{
	return "MapRttTex" + TOSTRING(mCounter);
}

void SurveyMapTextureCreator::preRenderTargetUpdate()
{

}

void SurveyMapTextureCreator::postRenderTargetUpdate()
{

}
