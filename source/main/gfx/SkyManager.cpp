/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2013-2018 Petr Ohlidal

    For more information, see http://www.rigsofrods.org/

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

#include "SkyManager.h"

#include "Actor.h"
#include "AppContext.h"
#include "CameraManager.h"
#include "GameContext.h"
#include "GfxScene.h"
#include "Terrain.h"
#include "TerrainGeometryManager.h"

#include <Caelum.h>

using namespace RoR;

SkyManager::SkyManager() : m_caelum_system(nullptr), m_last_clock(0.0)
{
    // Initialise CaelumSystem.
    m_caelum_system = new Caelum::CaelumSystem(
        RoR::App::GetAppContext()->GetOgreRoot(),
        App::GetGfxScene()->GetSceneManager(),
        Caelum::CaelumSystem::CAELUM_COMPONENTS_DEFAULT
    );

    m_caelum_system->attachViewport(RoR::App::GetAppContext()->GetViewport());

    // Register caelum as a listener.
    RoR::App::GetAppContext()->GetRenderWindow()->addListener(m_caelum_system);
    RoR::App::GetAppContext()->GetOgreRoot()->addFrameListener(m_caelum_system);
}

SkyManager::~SkyManager()
{
    RoR::App::GetAppContext()->GetRenderWindow()->removeListener(m_caelum_system);
    RoR::App::GetAppContext()->GetOgreRoot()->removeFrameListener(m_caelum_system);
    m_caelum_system->shutdown(false);
    m_caelum_system = nullptr;
}

void SkyManager::NotifySkyCameraChanged(Ogre::Camera* cam)
{
    if (m_caelum_system)
        m_caelum_system->notifyCameraChanged(cam);
}

void SkyManager::DetectSkyUpdate()
{
    if (!m_caelum_system || !App::GetGameContext()->GetTerrain())
    {
        return;
    }

    Caelum::LongReal c = m_caelum_system->getUniversalClock()->getJulianDay();

    if (c - m_last_clock > 0.001f)
    {
        TerrainGeometryManager* gm = App::GetGameContext()->GetTerrain()->getGeometryManager();
        if (gm)
            gm->updateLightMap();
    }

    m_last_clock = c;
}

void SkyManager::LoadCaelumScript(std::string script, int fogStart, int fogEnd)
{
    // load the caelum config
    try
    {
        Caelum::CaelumPlugin::getSingleton().loadCaelumSystemFromScript(m_caelum_system, script);

        // overwrite some settings
#ifdef CAELUM_VERSION_SEC
        // important: overwrite fog settings if not using infinite farclip
        if (fogStart != -1 && fogEnd != -1 && fogStart < fogEnd)
        {
            // setting farclip (hacky)
            App::GetCameraManager()->GetCamera()->setFarClipDistance(fogEnd / 0.8);
            // custom boundaries
            m_caelum_system->setManageSceneFog(Ogre::FOG_LINEAR);
            m_caelum_system->setManageSceneFogStart(fogStart);
            m_caelum_system->setManageSceneFogEnd(fogEnd);
        }
        else if (App::GetCameraManager()->GetCamera()->getFarClipDistance() > 0)
        {
            if (fogStart != -1 && fogEnd != -1)
            {
                LOG("CaelumFogStart must be smaller then CaelumFogEnd. Ignoring boundaries.");
            }
            else if (fogStart != -1 || fogEnd != -1)
            {
                LOG("You always need to define both boundaries (CaelumFogStart AND CaelumFogEnd). Ignoring boundaries.");
            }
            // non infinite farclip
            float farclip = App::GetCameraManager()->GetCamera()->getFarClipDistance();
            m_caelum_system->setManageSceneFog(Ogre::FOG_LINEAR);
            m_caelum_system->setManageSceneFogStart(farclip * 0.7);
            m_caelum_system->setManageSceneFogEnd(farclip * 0.9);
        }
        else
        {
            // no fog in infinite farclip
            m_caelum_system->setManageSceneFog(Ogre::FOG_NONE);
        }
#else
#error please use a recent Caelum version, see http://www.rigsofrods.org/wiki/pages/Compiling_3rd_party_libraries#Caelum
#endif // CAELUM_VERSION

        m_caelum_system->setEnsureSingleShadowSource(true);
        m_caelum_system->setEnsureSingleLightSource(true);

        // enforcing update, so shadows are set correctly before creating the terrain
        m_caelum_system->updateSubcomponents(0.1);
    }
    catch (Ogre::Exception& e)
    {
        RoR::LogFormat("[RoR] Exception while loading sky script: %s", e.getFullDescription().c_str());
    }
    Ogre::Vector3 lightsrc = m_caelum_system->getSun()->getMainLight()->getDerivedDirection();
    m_caelum_system->getSun()->getMainLight()->getParentSceneNode()->setDirection(lightsrc.normalisedCopy());

    // now optimize the moon a bit
    if (m_caelum_system->getMoon())
    {
        m_caelum_system->getMoon()->setAutoDisable(true);
        //m_caelum_system->getMoon()->setAutoDisableThreshold(1);
        m_caelum_system->getMoon()->setForceDisable(true);
        m_caelum_system->getMoon()->getMainLight()->setCastShadows(false);
    }
}

void SkyManager::SetSkyTimeFactor(float factor)
{
    m_caelum_system->getUniversalClock()->setTimeScale(factor);
}

Ogre::Light* SkyManager::GetSkyMainLight()
{
    if (m_caelum_system && m_caelum_system->getSun())
    {
        return m_caelum_system->getSun()->getMainLight();
    }
    return nullptr;
}

float SkyManager::GetSkyTimeFactor()
{
    return m_caelum_system->getUniversalClock()->getTimeScale();
}

std::string SkyManager::GetPrettyTime()
{
    int ignore;
    int hour;
    int minute;
    Caelum::LongReal second;
    Caelum::Astronomy::getGregorianDateTimeFromJulianDay(m_caelum_system->getJulianDay()
        , ignore, ignore, ignore, hour, minute, second);

    char buf[100];
    snprintf(buf, 100, "%02d:%02d:%02d", hour, minute, static_cast<int>(second));
    return buf;
}

#endif //USE_CAELUM
