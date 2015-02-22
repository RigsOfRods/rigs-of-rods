/*
	This source file is part of Rigs of Rods
	Copyright 2005-2012 Pierre-Michel Ricordel
	Copyright 2007-2012 Thomas Fischer
	Copyright 2013-2014 Petr Ohlidal

	For more information, see http://www.rigsofrods.com/

	Rigs of Rods is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License version 3, as
	published by the Free Software Foundation.

	Rigs of Rods is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Rigs of Rods. If not, see <http://www.gnu.org/licenses/>.
*/

#ifdef USE_CAELUM

#include "CaelumManager.h"

#include "Application.h"
#include "OgreSubsystem.h"
#include "Settings.h"
#include "TerrainManager.h"
#include "TerrainGeometryManager.h"

#include <Caelum.h>

using namespace Ogre;

//---------------------------------------------------------------------
CaelumManager::CaelumManager() : mCaelumSystem(0), lc(0)
{
	// Initialise CaelumSystem.
	mCaelumSystem = new Caelum::CaelumSystem (
		RoR::Application::GetOgreSubsystem()->GetOgreRoot(), 
		gEnv->sceneManager, 
		Caelum::CaelumSystem::CAELUM_COMPONENTS_DEFAULT
	);
	mCaelumSystem->attachViewport(RoR::Application::GetOgreSubsystem()->GetViewport());
	/*
	// TODO: set real time, and let the user select his true location
	mCaelumSystem->getUniversalClock()->setGregorianDateTime(2008, 4, 9, 6, 33, 0);
	mCaelumSystem->setObserverLongitude(Degree(0));
	mCaelumSystem->setObserverLatitude(Degree(0));
	mCaelumSystem->getUniversalClock()->setTimeScale(100);
	*/

	// Register caelum as a listener.
	RoR::Application::GetOgreSubsystem()->GetRenderWindow()->addListener (mCaelumSystem);
	RoR::Application::GetOgreSubsystem()->GetOgreRoot()->addFrameListener(mCaelumSystem);
}

CaelumManager::~CaelumManager()
{
	RoR::Application::GetOgreSubsystem()->GetRenderWindow()->removeListener(mCaelumSystem);
	mCaelumSystem->shutdown(false);
	mCaelumSystem = nullptr;
}

void CaelumManager::notifyCameraChanged(Camera *cam)
{
	if (mCaelumSystem)
		mCaelumSystem->notifyCameraChanged(cam);
}

void CaelumManager::forceUpdate(float dt)
{
	if (mCaelumSystem)
		mCaelumSystem->updateSubcomponents(dt);
}

void CaelumManager::detectUpdate()
{
	if (!mCaelumSystem || !gEnv->terrainManager) return;
	Caelum::LongReal c = mCaelumSystem->getUniversalClock()->getJulianDay();

	if(c - lc > 0.001f)
	{
		TerrainGeometryManager *gm = gEnv->terrainManager->getGeometryManager();
		if(gm) gm->updateLightMap();
	}

	lc = c;
}

void CaelumManager::loadScript(String script, int fogStart, int fogEnd)
{
	// load the caelum config
	try
	{
		Caelum::CaelumPlugin::getSingleton().loadCaelumSystemFromScript (mCaelumSystem, script);

		// overwrite some settings
#ifdef CAELUM_VERSION_SEC
		// important: overwrite fog settings if not using infinite farclip
		if (fogStart != -1 && fogEnd != -1 && fogStart < fogEnd)
		{
			// setting farclip (hacky)
			Settings::getSingleton().setSetting("SightRange", StringConverter::toString((float)(fogEnd/0.8)));
			float far_clip = FSETTING("SightRange",4500);
			gEnv->mainCamera->setFarClipDistance(far_clip);
			// custom boundaries
			mCaelumSystem->setManageSceneFog(FOG_LINEAR);
			mCaelumSystem->setManageSceneFogStart(fogStart);
			mCaelumSystem->setManageSceneFogEnd(fogEnd);
		}
		else if (gEnv->mainCamera->getFarClipDistance() > 0)
		{
			if(fogStart != -1 && fogEnd != -1){ LOG("CaelumFogStart must be smaller then CaelumFogEnd. Ignoring boundaries.");} 
			else if(fogStart != -1 || fogEnd != -1){ LOG("You always need to define both boundaries (CaelumFogStart AND CaelumFogEnd). Ignoring boundaries.");}
			// non infinite farclip
			Real farclip = gEnv->mainCamera->getFarClipDistance();
			mCaelumSystem->setManageSceneFog(FOG_LINEAR);
			mCaelumSystem->setManageSceneFogStart(farclip*0.7);
			mCaelumSystem->setManageSceneFogEnd(farclip*0.9);
		} else
		{
			// no fog in infinite farclip
			mCaelumSystem->setManageSceneFog(FOG_NONE);
		}
#else
#error please use a recent Caelum version, see http://www.rigsofrods.com/wiki/pages/Compiling_3rd_party_libraries#Caelum
#endif // CAELUM_VERSION
		// now optimize the moon a bit
		if (mCaelumSystem->getMoon())
		{
			mCaelumSystem->getMoon()->setAutoDisable(true);
			//mCaelumSystem->getMoon()->setAutoDisableThreshold(1);
			mCaelumSystem->getMoon()->setForceDisable(true);
			mCaelumSystem->getMoon()->getMainLight()->setCastShadows(false);
		}

		mCaelumSystem->setEnsureSingleShadowSource(true);
		mCaelumSystem->setEnsureSingleLightSource(true);

		// enforcing update, so shadows are set correctly before creating the terrain
		forceUpdate(0.1);

	} catch(Exception& e)
	{
		LOG("exception upon loading sky script: " + e.getFullDescription());
	}
}

void CaelumManager::setTimeFactor(Real factor)
{
    mCaelumSystem->getUniversalClock()->setTimeScale(factor);
}

Light *CaelumManager::getMainLight()
{
	if (mCaelumSystem && mCaelumSystem->getSun())
		return mCaelumSystem->getSun()->getMainLight();
	return 0;
}

Real CaelumManager::getTimeFactor()
{
    return mCaelumSystem->getUniversalClock()->getTimeScale();
}

String CaelumManager::getPrettyTime()
{
	int ignore;
	int hour;
	int minute;
	Caelum::LongReal second;
	Caelum::Astronomy::getGregorianDateTimeFromJulianDay(mCaelumSystem->getJulianDay()
	, ignore, ignore, ignore, hour, minute, second);
	
	return StringConverter::toString( hour, 2, '0' )
	+ ":" + StringConverter::toString( minute, 2, '0' )
	+ ":" + StringConverter::toString( (int)second, 2, '0' );
}

bool CaelumManager::update( float dt )
{
	// TODO
	return true;
}

size_t CaelumManager::getMemoryUsage()
{
	return 0;
}

void CaelumManager::freeResources()
{
	// TODO
}

#endif //USE_CAELUM
