/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2013-2018 Petr Ohlidal & contributors

    For more information, see http://www.rigsofrods.org/

    Rigs of Rods is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 3, as
    published by the Free Software Foundation.

    Rigs of Rods is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Rigs of Rods. If not, see <http://www.gnu.org/licenses/>.
*/

#include "DustManager.h"

#include "Application.h"
#include "DustPool.h"
#include "Heathaze.h"
#include "OverlayWrapper.h"
#include "RoRFrameListener.h" // SimController
#include "Settings.h"
#include "SkyManager.h"
#include "SurveyMapManager.h"
#include "TerrainGeometryManager.h"
#include "TerrainManager.h"
#include "TerrainObjectManager.h"

using namespace Ogre;

RoR::GfxScene::GfxScene()
    : m_ogre_scene(nullptr)
    , m_heathaze(nullptr)
{}

void RoR::GfxScene::InitScene(Ogre::SceneManager* sm)
{
    m_dustpools["dust"]   = new DustPool(sm, "tracks/Dust",   20);
    m_dustpools["clump"]  = new DustPool(sm, "tracks/Clump",  20);
    m_dustpools["sparks"] = new DustPool(sm, "tracks/Sparks", 10);
    m_dustpools["drip"]   = new DustPool(sm, "tracks/Drip",   50);
    m_dustpools["splash"] = new DustPool(sm, "tracks/Splash", 20);
    m_dustpools["ripple"] = new DustPool(sm, "tracks/Ripple", 20);

    m_ogre_scene = sm;

    m_envmap.SetupEnvMap();

    // heathaze effect
    if (App::gfx_enable_heathaze.GetActive())
    {
        m_heathaze = new HeatHaze();
        m_heathaze->setEnable(true);
    }
}

void RoR::GfxScene::InitSurveyMap(Ogre::Vector3 terrain_size)
{
    if (!RoR::App::gfx_minimap_disabled.GetActive())
    {
        m_survey_map = new SurveyMapManager(terrain_size);
    }
}

void RoR::GfxScene::DiscardScene()
{
    for (auto itor : m_dustpools)
    {
        itor.second->Discard(m_ogre_scene);
        delete itor.second;
    }
    m_dustpools.clear();

    if (m_heathaze != nullptr)
    {
        delete m_heathaze;
        m_heathaze = nullptr;
    }

    if (m_survey_map != nullptr)
    {
        delete m_survey_map;
        m_survey_map = nullptr;
    }
}

void RoR::GfxScene::UpdateScene(float dt_sec)
{
    // Actors - start threaded tasks
    for (GfxActor* gfx_actor: m_live_gfx_actors)
    {
        gfx_actor->UpdateWheelVisuals(); // Push flexwheel tasks to threadpool
    }

    // Var
    GfxActor* player_gfx_actor = nullptr;
    if (m_simbuf.simbuf_player_actor != nullptr)
    {
        player_gfx_actor = m_simbuf.simbuf_player_actor->GetGfxActor();
    }

    // Particles
    if (RoR::App::gfx_particles_mode.GetActive() == 1)
    {
        for (auto itor : m_dustpools)
        {
            itor.second->update();
        }
    }

    if (m_heathaze != nullptr)
    {
        m_heathaze->update(); // TODO: The effect seems quite badly broken at the moment ~ only_a_ptr, 05/2018
    }

    // Terrain - animated meshes and paged geometry
    App::GetSimTerrain()->getObjectManager()->UpdateTerrainObjects(dt_sec);

    // Terrain - lightmap; TODO: ported as-is from TerrainManager::update(), is it needed? ~ only_a_ptr, 05/2018
    App::GetSimTerrain()->getGeometryManager()->UpdateMainLightPosition(); // TODO: Is this necessary? I'm leaving it here just in case ~ only_a_ptr, 04/2017

    // Terrain - water
    IWater* water = App::GetSimTerrain()->getWater();
    if (water)
    {
        water->WaterSetCamera(gEnv->mainCamera);
        if (player_gfx_actor != nullptr)
        {
            water->SetReflectionPlaneHeight(water->CalcWavesHeight(player_gfx_actor->GetSimDataBuffer().simbuf_pos));
        }
        else
        {
            water->SetReflectionPlaneHeight(water->GetStaticWaterHeight());
        }
        water->FrameStepWater(dt_sec);
    }

    // Terrain - sky
#ifdef USE_CAELUM
    SkyManager* sky = App::GetSimTerrain()->getSkyManager();
    if (sky != nullptr)
    {
        sky->DetectSkyUpdate();
    }
#endif

    SkyXManager* skyx_man = App::GetSimTerrain()->getSkyXManager();
    if (skyx_man != nullptr)
    {
       skyx_man->update(dt_sec); // Light update
    }

    // GUI - Direction arrow
    if (App::GetOverlayWrapper() && App::GetOverlayWrapper()->IsDirectionArrowVisible())
    {
        App::GetOverlayWrapper()->UpdateDirectionArrowHud(
            player_gfx_actor, m_simbuf.simbuf_dir_arrow_target, m_simbuf.simbuf_character_pos);
    }

    // GUI - Survey map
    if (m_survey_map != nullptr)
    {
        for (GfxActor* gfx_actor: m_all_gfx_actors)
        {
            auto& simbuf = gfx_actor->GetSimDataBuffer();
            m_survey_map->UpdateActorMapEntry(
                gfx_actor->GetActorId(), simbuf.simbuf_pos, simbuf.simbuf_heading_angle);
        }
        m_survey_map->Update(dt_sec, m_simbuf.simbuf_player_actor);
    }

    // Actors - update misc visuals
    for (GfxActor* gfx_actor: m_live_gfx_actors)
    {
        gfx_actor->UpdateCabMesh();
        gfx_actor->UpdateAirbrakes();
        gfx_actor->UpdateCParticles();
    }
    if (player_gfx_actor != nullptr)
    {
        Ogre::Vector3 pos = player_gfx_actor->GetSimDataBuffer().simbuf_pos;
        m_envmap.UpdateEnvMap(pos, m_simbuf.simbuf_player_actor); // Safe to be called here, only modifies OGRE objects, doesn't read any physics state.
        player_gfx_actor->UpdateVideoCameras(dt_sec);
    }

    // Actors - finalize threaded tasks
    for (GfxActor* gfx_actor: m_live_gfx_actors)
    {
        gfx_actor->FinishWheelUpdates();
    }
}

void RoR::GfxScene::SetParticlesVisible(bool visible)
{
    for (auto itor : m_dustpools)
    {
        itor.second->setVisible(visible);
    }
}

DustPool* RoR::GfxScene::GetDustPool(const char* name)
{
    auto found = m_dustpools.find(name);
    if (found != m_dustpools.end())
    {
        return found->second;
    }
    else
    {
        return nullptr;
    }
}

void RoR::GfxScene::RegisterGfxActor(RoR::GfxActor* gfx_actor)
{
    m_all_gfx_actors.push_back(gfx_actor);

    if (m_survey_map != nullptr)
    {
        m_survey_map->createNamedMapEntity( // TODO: just use pointers! ~ 05/2018
            "Truck" + TOSTRING(gfx_actor->GetActorId()),
            SurveyMapManager::getTypeByDriveable(gfx_actor->GetActorDriveable()));
    }
}

void RoR::GfxScene::BufferSimulationData()
{
    m_simbuf.simbuf_player_actor = App::GetSimController()->GetPlayerActor();
    m_simbuf.simbuf_character_pos = gEnv->player->getPosition();
    m_simbuf.simbuf_dir_arrow_target = App::GetSimController()->GetDirArrowTarget();

    m_live_gfx_actors.clear();
    for (GfxActor* a: m_all_gfx_actors)
    {
        if (a->IsActorLive())
        {
            a->UpdateSimDataBuffer();
            m_live_gfx_actors.push_back(a);
        }
    }
}

void RoR::GfxScene::RemoveGfxActor(RoR::GfxActor* remove_me)
{
    auto itor = m_all_gfx_actors.begin();
    auto endi = m_all_gfx_actors.end();
    for (; itor != endi; ++itor)
    {
        if (*itor == remove_me)
        {
            m_all_gfx_actors.erase(itor);
            return;
        }
    }
}

RoR::GfxScene::SimBuffer::SimBuffer():
    simbuf_player_actor(nullptr),
    simbuf_character_pos(Ogre::Vector3::ZERO),
    simbuf_dir_arrow_target(Ogre::Vector3::ZERO)
{}
