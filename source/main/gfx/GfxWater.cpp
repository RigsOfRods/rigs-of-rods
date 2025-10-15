/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2013-2020 Petr Ohlidal

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

#include "GfxWater.h"

#include "Actor.h"
#include "AppContext.h"
#include "CameraManager.h"
#include "GfxScene.h"
#include "PlatformUtils.h" // PathCombine
#include "Terrain.h"

#include <Ogre.h>

using namespace Ogre;
using namespace RoR;

static const int WAVEREZ = 100;

GfxWater::GfxWater(Ogre::Vector3 terrn_size, float initial_water_height) :
    m_map_size(terrn_size),
    m_visual_water_height(initial_water_height)
{
    m_reflect_listener.scene_mgr = App::GetGfxScene()->GetSceneManager();
    m_refract_listener.scene_mgr = App::GetGfxScene()->GetSceneManager();

    if (m_map_size.x < 1500 && m_map_size.z < 1500)
        m_waterplane_mesh_scale = 1.5f;

    this->PrepareWater();
}

GfxWater::~GfxWater()
{
    if (m_refract_cam != nullptr)
    {
        App::GetGfxScene()->GetSceneManager()->destroyCamera(m_refract_cam);
        m_refract_cam = nullptr;
    }

    if (m_reflect_cam != nullptr)
    {
        App::GetGfxScene()->GetSceneManager()->destroyCamera(m_reflect_cam);
        m_reflect_cam = nullptr;
    }

    if (m_waterplane_entity != nullptr)
    {
        App::GetGfxScene()->GetSceneManager()->destroyEntity("plane");
        m_waterplane_entity = nullptr;
    }

    if (m_waterplane_node != nullptr)
    {
        App::GetGfxScene()->GetSceneManager()->getRootSceneNode()->removeAndDestroyChild("WaterPlane");
        m_waterplane_node = nullptr;
    }

    if (m_bottomplane_node != nullptr)
    {
        App::GetGfxScene()->GetSceneManager()->destroyEntity("bplane");
        App::GetGfxScene()->GetSceneManager()->getRootSceneNode()->removeAndDestroyChild("BottomWaterPlane");
        m_bottomplane_node = nullptr;
    }

    if (m_refract_rtt_target)
    {
        m_refract_rtt_target->removeAllListeners();
        m_refract_rtt_target->removeAllViewports();
        m_refract_rtt_target = nullptr;
    }

#if 0 // Doesn't cut it with Ogre 1.11
    if (!m_refract_rtt_texture.isNull())
    {
        m_refract_rtt_texture->unload();
        Ogre::TextureManager::getSingleton().remove(m_refract_rtt_texture->getHandle());
        m_refract_rtt_texture.reset();
    }
#endif

    if (m_reflect_rtt_target)
    {
        m_reflect_rtt_target->removeAllListeners();
        m_reflect_rtt_target->removeAllViewports();
        m_reflect_rtt_target = nullptr;
    }

#if 0 // Doesn't cut it with Ogre 1.11
    if (!m_reflect_rtt_texture.isNull())
    {
        m_reflect_rtt_texture->unload();
        Ogre::TextureManager::getSingleton().remove(m_reflect_rtt_texture->getHandle());
        m_reflect_rtt_texture.reset();
    }
#endif

    if (m_waterplane_vert_buf_local)
    {
        free(m_waterplane_vert_buf_local);
        m_waterplane_vert_buf_local = nullptr;
    }

    Ogre::MeshManager::getSingleton().remove("ReflectPlane");
    Ogre::MeshManager::getSingleton().remove("BottomPlane");
    Ogre::MeshManager::getSingleton().remove("WaterPlane");
}

void GfxWater::PrepareWater()
{
    m_water_plane.normal = Vector3::UNIT_Y;
    m_water_plane.d = 0;

    const auto type = App::gfx_water_mode->getEnum<GfxWaterMode>();
    const bool full_gfx = type == GfxWaterMode::FULL_HQ || type == GfxWaterMode::FULL_FAST;

    if (full_gfx || type == GfxWaterMode::REFLECT)
    {
        // Check prerequisites first
        const RenderSystemCapabilities* caps = Root::getSingleton().getRenderSystem()->getCapabilities();
        if (!caps->hasCapability(RSC_VERTEX_PROGRAM) || !(caps->hasCapability(RSC_FRAGMENT_PROGRAM)))
        {
            OGRE_EXCEPT(Exception::ERR_RENDERINGAPI_ERROR, "Your card does not support vertex and fragment programs, so cannot "
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
                OGRE_EXCEPT(Exception::ERR_RENDERINGAPI_ERROR, "Your card does not support advanced fragment programs, "
                    "so cannot run Water effects. Sorry!",
                    "Water effects");
            }
        }
        // Ok
        // Define a floor plane mesh
        m_reflect_plane.normal = Vector3::UNIT_Y;
        m_reflect_plane.d = - 0.15;
        m_refract_plane.normal = -Vector3::UNIT_Y;
        m_refract_plane.d = 0.15;

        if (full_gfx)
        {
            TexturePtr m_refract_rtt_targetPtr = Ogre::TextureManager::getSingleton ().getByName ("Refraction");
            m_refract_rtt_texture = m_refract_rtt_targetPtr;
            m_refract_rtt_target = m_refract_rtt_targetPtr->getBuffer()->getRenderTarget();
            {
                m_refract_cam = App::GetGfxScene()->GetSceneManager()->createCamera("RefractCam");
                m_refract_cam->setNearClipDistance(App::GetCameraManager()->GetCamera()->getNearClipDistance());
                m_refract_cam->setFarClipDistance(App::GetCameraManager()->GetCamera()->getFarClipDistance());
                m_refract_cam->setAspectRatio(
                    (Real)RoR::App::GetAppContext()->GetRenderWindow()->getViewport(0)->getActualWidth() /
                    (Real)RoR::App::GetAppContext()->GetRenderWindow()->getViewport(0)->getActualHeight());

                m_refract_rtt_viewport = m_refract_rtt_target->addViewport(m_refract_cam);
                m_refract_rtt_viewport->setClearEveryFrame(true);
                m_refract_rtt_viewport->setBackgroundColour(App::GetGfxScene()->GetSceneManager()->getFogColour());

                MaterialPtr mat = MaterialManager::getSingleton().getByName("Examples/FresnelReflectionRefraction");
                mat->getTechnique(0)->getPass(0)->getTextureUnitState(2)->setTextureName("Refraction");

                mat = MaterialManager::getSingleton().getByName("Examples/FresnelReflectionRefractioninverted");
                mat->getTechnique(0)->getPass(0)->getTextureUnitState(2)->setTextureName("Refraction");

                m_refract_rtt_viewport->setOverlaysEnabled(false);

                m_refract_rtt_target->addListener(&m_refract_listener);

                //optimisation
                m_refract_rtt_target->setAutoUpdated(false);

                // Also clip
                m_refract_cam->enableCustomNearClipPlane(m_refract_plane);
            }
        }

        TexturePtr m_reflect_rtt_targetPtr = Ogre::TextureManager::getSingleton ().getByName ("Reflection");
        m_reflect_rtt_texture = m_reflect_rtt_targetPtr;
        m_reflect_rtt_target = m_reflect_rtt_targetPtr->getBuffer()->getRenderTarget();
        {
            m_reflect_cam = App::GetGfxScene()->GetSceneManager()->createCamera("ReflectCam");
            m_reflect_cam->setNearClipDistance(App::GetCameraManager()->GetCamera()->getNearClipDistance());
            m_reflect_cam->setFarClipDistance(App::GetCameraManager()->GetCamera()->getFarClipDistance());
            m_reflect_cam->setAspectRatio(
                (Real)RoR::App::GetAppContext()->GetRenderWindow()->getViewport(0)->getActualWidth() /
                (Real)RoR::App::GetAppContext()->GetRenderWindow()->getViewport(0)->getActualHeight());

            m_reflect_rtt_viewport = m_reflect_rtt_target->addViewport(m_reflect_cam);
            m_reflect_rtt_viewport->setClearEveryFrame(true);
            m_reflect_rtt_viewport->setBackgroundColour(App::GetGfxScene()->GetSceneManager()->getFogColour());

            MaterialPtr mat;
            if (full_gfx)
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

            m_reflect_rtt_viewport->setOverlaysEnabled(false);

            m_reflect_rtt_target->addListener(&m_reflect_listener);

            //optimisation
            m_reflect_rtt_target->setAutoUpdated(false);

            // set up linked reflection
            m_reflect_cam->enableReflection(m_water_plane);
            // Also clip
            m_reflect_cam->enableCustomNearClipPlane(m_reflect_plane);
        }

        m_waterplane_mesh = MeshManager::getSingleton().createPlane("ReflectPlane",
            ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
            m_water_plane,
            m_map_size.x * m_waterplane_mesh_scale, m_map_size.z * m_waterplane_mesh_scale, WAVEREZ, WAVEREZ, true, 1, 50, 50, Vector3::UNIT_Z, HardwareBuffer::HBU_DYNAMIC_WRITE_ONLY_DISCARDABLE);

        m_waterplane_entity = App::GetGfxScene()->GetSceneManager()->createEntity("plane", "ReflectPlane");
        if (full_gfx)
            m_waterplane_entity->setMaterialName("Examples/FresnelReflectionRefraction");
        else
            m_waterplane_entity->setMaterialName("Examples/FresnelReflection");
    }
    else
    {
        m_waterplane_mesh = MeshManager::getSingleton().createPlane("WaterPlane",
            ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
            m_water_plane,
            m_map_size.x * m_waterplane_mesh_scale, m_map_size.z * m_waterplane_mesh_scale, WAVEREZ, WAVEREZ, true, 1, 50, 50, Vector3::UNIT_Z, HardwareBuffer::HBU_DYNAMIC_WRITE_ONLY_DISCARDABLE);
        m_waterplane_entity = App::GetGfxScene()->GetSceneManager()->createEntity("plane", "WaterPlane");
        m_waterplane_entity->setMaterialName("tracks/basicwater");
    }

    m_waterplane_entity->setCastShadows(false);
    m_reflect_listener.waterplane_entity = m_waterplane_entity;
    m_refract_listener.waterplane_entity = m_waterplane_entity;
    //position
    m_waterplane_node = App::GetGfxScene()->GetSceneManager()->getRootSceneNode()->createChildSceneNode("WaterPlane");
    m_waterplane_node->attachObject(m_waterplane_entity);
    m_waterplane_node->setPosition(Vector3((m_map_size.x * m_waterplane_mesh_scale) / 2, m_visual_water_height, (m_map_size.z * m_waterplane_mesh_scale) / 2));

    //bottom
    m_bottom_plane.normal = Vector3::UNIT_Y;
    m_bottom_plane.d = -m_bottom_height; //30m below waterline
    MeshManager::getSingleton().createPlane("BottomPlane",
        ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
        m_bottom_plane,
        m_map_size.x * m_waterplane_mesh_scale, m_map_size.z * m_waterplane_mesh_scale, 1, 1, true, 1, 1, 1, Vector3::UNIT_Z);
    Entity* pE = App::GetGfxScene()->GetSceneManager()->createEntity("bplane", "BottomPlane");
    pE->setMaterialName("tracks/seabottom");
    pE->setCastShadows(false);

    //position
    m_bottomplane_node = App::GetGfxScene()->GetSceneManager()->getRootSceneNode()->createChildSceneNode("BottomWaterPlane");
    m_bottomplane_node->attachObject(pE);
    m_bottomplane_node->setPosition(Vector3((m_map_size.x * m_waterplane_mesh_scale) / 2, 0, (m_map_size.z * m_waterplane_mesh_scale) / 2));

    //setup for waves
    m_waterplane_vert_buf = m_waterplane_mesh->sharedVertexData->vertexBufferBinding->getBuffer(0);

    if (m_waterplane_vert_buf->getSizeInBytes() == (WAVEREZ + 1) * (WAVEREZ + 1) * 32)
    {
        m_waterplane_vert_buf_local = (float*)malloc(m_waterplane_vert_buf->getSizeInBytes());
        m_waterplane_vert_buf->readData(0, m_waterplane_vert_buf->getSizeInBytes(), m_waterplane_vert_buf_local);
    }
    else
        m_waterplane_vert_buf_local = 0;
}

void GfxWater::SetWaterVisible(bool value)
{
    m_water_visible = value;
    if (m_waterplane_entity)
        m_waterplane_entity->setVisible(value);
    if (m_waterplane_node)
        m_waterplane_node->setVisible(value);
    if (m_bottomplane_node)
        m_bottomplane_node->setVisible(value);
}

void GfxWater::SetReflectionPlaneHeight(float centerheight)
{
    const auto type = App::gfx_water_mode->getEnum<GfxWaterMode>();
    if (type == GfxWaterMode::FULL_HQ || type == GfxWaterMode::FULL_FAST || type == GfxWaterMode::REFLECT)
    {
        this->UpdateReflectionPlane(centerheight);
    }
}

void GfxWater::ShowWave(Vector3 refpos)
{
    if (!m_waterplane_vert_buf_local)
        return;

    float xScaled = m_map_size.x * m_waterplane_mesh_scale;
    float zScaled = m_map_size.z * m_waterplane_mesh_scale;

    for (int pz = 0; pz < WAVEREZ + 1; pz++)
    {
        for (int px = 0; px < WAVEREZ + 1; px++)
        {
            m_waterplane_vert_buf_local[(pz * (WAVEREZ + 1) + px) * 8 + 1] = App::GetGameContext()->GetTerrain()->getWater()->CalcWavesHeight(refpos + Vector3(xScaled * 0.5 - (float)px * xScaled / WAVEREZ, 0, (float)pz * zScaled / WAVEREZ - zScaled * 0.5)) - App::GetGameContext()->GetTerrain()->getWater()->GetStaticWaterHeight();
        }
    }

    //normals
    for (int pz = 0; pz < WAVEREZ + 1; pz++)
    {
        for (int px = 0; px < WAVEREZ + 1; px++)
        {
            int left = std::max(0, px - 1);
            int right = std::min(px + 1, WAVEREZ);
            int up = std::max(0, pz - 1);
            int down = std::min(pz + 1, WAVEREZ);

            Vector3 normal = (Vector3(m_waterplane_vert_buf_local + ((pz * (WAVEREZ + 1) + left) * 8)) - Vector3(m_waterplane_vert_buf_local + ((pz * (WAVEREZ + 1) + right) * 8))).crossProduct(Vector3(m_waterplane_vert_buf_local + ((up * (WAVEREZ + 1) + px) * 8)) - Vector3(m_waterplane_vert_buf_local + ((down * (WAVEREZ + 1) + px) * 8)));
            normal.normalise();

            m_waterplane_vert_buf_local[(pz * (WAVEREZ + 1) + px) * 8 + 3] = normal.x;
            m_waterplane_vert_buf_local[(pz * (WAVEREZ + 1) + px) * 8 + 4] = normal.y;
            m_waterplane_vert_buf_local[(pz * (WAVEREZ + 1) + px) * 8 + 5] = normal.z;
        }
    }

    m_waterplane_vert_buf->writeData(0, (WAVEREZ + 1) * (WAVEREZ + 1) * 32, m_waterplane_vert_buf_local, true);
}

bool GfxWater::IsCameraUnderWater()
{
    Wavefield* waveField = App::GetGameContext()->GetTerrain()->getWater();
    Ogre::SceneNode* camNode = App::GetCameraManager()->GetCameraNode();
    return camNode->getPosition().y < waveField->CalcWavesHeight(camNode->getPosition());
}

void GfxWater::UpdateWater()
{
    if (!m_water_visible)
        return;

    const Ogre::Vector3    camera_pos = (m_cam_forced) ? m_cam_forced_position : App::GetCameraManager()->GetCameraNode()->getPosition();
    const Ogre::Quaternion camera_rot = (m_cam_forced) ? m_cam_forced_orientation : App::GetCameraManager()->GetCameraNode()->getOrientation();
    const Ogre::Radian     camera_fov = (m_cam_forced) ? m_cam_forced_fovy : App::GetCameraManager()->GetCamera()->getFOVy();

    if (m_waterplane_node)
    {
        const Vector3 water_cam_pos(camera_pos.x, m_visual_water_height, camera_pos.z);
        Vector3 sightPos(water_cam_pos);
        // Direction points down -Z by default (adapted from Ogre::Camera)
        Ogre::Vector3 cameraDir = camera_rot * -Ogre::Vector3::UNIT_Z;

        Ray lineOfSight(camera_pos, cameraDir);
        Plane waterPlane(Vector3::UNIT_Y, Vector3::UNIT_Y * m_visual_water_height);

        std::pair<bool, Real> intersection = lineOfSight.intersects(waterPlane);

        if (intersection.first && intersection.second > 0.0f)
            sightPos = lineOfSight.getPoint(intersection.second);

        Real offset = std::min(water_cam_pos.distance(sightPos), std::min(m_map_size.x, m_map_size.z) * 0.5f);

        Vector3 waterPos = water_cam_pos + (sightPos - water_cam_pos).normalisedCopy() * offset;
        Vector3 bottomPos = Vector3(waterPos.x, m_bottom_height, waterPos.z);

        if (waterPos.distance(m_waterplane_node->getPosition()) > 200.0f || m_waterplane_force_update_pos)
        {
            m_waterplane_node->setPosition(Vector3(waterPos.x, m_visual_water_height, waterPos.z));
            m_bottomplane_node->setPosition(bottomPos);
        }
        if (RoR::App::gfx_water_waves->getBool() && RoR::App::mp_state->getEnum<MpState>() == RoR::MpState::DISABLED)
            this->ShowWave(m_waterplane_node->getPosition());
    }

    m_frame_counter++;
    if (App::gfx_water_mode->getEnum<GfxWaterMode>() == GfxWaterMode::FULL_FAST)
    {
        if (m_frame_counter % 2 == 1 || m_waterplane_force_update_pos)
        {
            m_reflect_cam->setOrientation(camera_rot);
            m_reflect_cam->setPosition(camera_pos);
            m_reflect_cam->setFOVy(camera_fov);
            m_reflect_rtt_target->update();
        }
        if (m_frame_counter % 2 == 0 || m_waterplane_force_update_pos)
        {
            m_refract_cam->setOrientation(camera_rot);
            m_refract_cam->setPosition(camera_pos);
            m_refract_cam->setFOVy(camera_fov);
            m_refract_rtt_target->update();
        }
    }
    else if (App::gfx_water_mode->getEnum<GfxWaterMode>() == GfxWaterMode::FULL_HQ)
    {
        m_reflect_cam->setOrientation(camera_rot);
        m_reflect_cam->setPosition(camera_pos);
        m_reflect_cam->setFOVy(camera_fov);
        m_reflect_rtt_target->update();

        m_refract_cam->setOrientation(camera_rot);
        m_refract_cam->setPosition(camera_pos);
        m_refract_cam->setFOVy(camera_fov);
        m_refract_rtt_target->update();
    }
    else if (App::gfx_water_mode->getEnum<GfxWaterMode>() == GfxWaterMode::REFLECT)
    {
        m_reflect_cam->setOrientation(camera_rot);
        m_reflect_cam->setPosition(camera_pos);
        m_reflect_cam->setFOVy(camera_fov);
        m_reflect_rtt_target->update();
    }

    m_waterplane_force_update_pos = false;
}

void GfxWater::WaterPrepareShutdown()
{
    if (m_refract_rtt_target)
        m_refract_rtt_target->removeListener(&m_refract_listener);
    if (m_reflect_rtt_target)
        m_reflect_rtt_target->removeListener(&m_reflect_listener);
}

void GfxWater::SetWaterBottomHeight(float value)
{
    m_bottom_height = value;
}

void GfxWater::UpdateReflectionPlane(float h)
{
    if (this->IsCameraUnderWater())
    {
        m_water_plane.d = -h;
        if (m_reflect_cam)
        {
            m_reflect_cam->disableReflection();
            m_reflect_cam->disableCustomNearClipPlane();
        }
    }
    else
    {
        m_reflect_plane.normal = Vector3::UNIT_Y;
        m_refract_plane.normal = -Vector3::UNIT_Y;
        m_reflect_plane.d = -h + 0.15;
        m_refract_plane.d = h + 0.15;
        m_water_plane.d = -h;
        if (m_reflect_cam)
        {
            m_reflect_cam->enableReflection(m_water_plane);
            m_reflect_cam->enableCustomNearClipPlane(m_reflect_plane);
        }
    }

    if (m_refract_cam)
    {
        m_refract_cam->enableCustomNearClipPlane(m_refract_plane);
    }
}

void GfxWater::FrameStepWater(float dt)
{
    // Update even if game paused to account for camera movement (important for reflections).
    // --------------------------------------------------------------------------------------
    const float water_height = App::GetGameContext()->GetTerrain()->getWater()->GetStaticWaterHeight();
    if (water_height != m_visual_water_height)
    {
        m_visual_water_height = water_height;
        m_waterplane_force_update_pos = true;
    }
    this->UpdateWater();
}

void GfxWater::SetForcedCameraTransform(Ogre::Radian fovy, Ogre::Vector3 pos, Ogre::Quaternion rot)
{
    m_cam_forced = true;
    m_cam_forced_fovy = fovy;
    m_cam_forced_position = pos;
    m_cam_forced_orientation = rot;
}

void GfxWater::ClearForcedCameraTransform()
{
    m_cam_forced = false;
    m_cam_forced_fovy = 0;
    m_cam_forced_position = Ogre::Vector3::ZERO;
    m_cam_forced_orientation = Ogre::Quaternion::IDENTITY;
}


// ------------------------- The listeners -------------------------


void GfxWater::RefractionListener::preRenderTargetUpdate(const Ogre::RenderTargetEvent& evt)
{
    this->scene_mgr->getRenderQueue()->getQueueGroup(RENDER_QUEUE_MAIN)->setShadowsEnabled(false);
    this->waterplane_entity->setVisible(false);
    App::GetGfxScene()->SetParticlesVisible(false); // Hide water spray
}

void GfxWater::RefractionListener::postRenderTargetUpdate(const Ogre::RenderTargetEvent& evt)
{
    this->scene_mgr->getRenderQueue()->getQueueGroup(RENDER_QUEUE_MAIN)->setShadowsEnabled(true);
    this->waterplane_entity->setVisible(true);
    App::GetGfxScene()->SetParticlesVisible(true); // Restore water spray
}

void GfxWater::ReflectionListener::preRenderTargetUpdate(const Ogre::RenderTargetEvent& evt)
{
    this->scene_mgr->getRenderQueue()->getQueueGroup(RENDER_QUEUE_MAIN)->setShadowsEnabled(false);
    this->waterplane_entity->setVisible(false);
}

void GfxWater::ReflectionListener::postRenderTargetUpdate(const Ogre::RenderTargetEvent& evt)
{
    this->scene_mgr->getRenderQueue()->getQueueGroup(RENDER_QUEUE_MAIN)->setShadowsEnabled(true);
    this->waterplane_entity->setVisible(true);
}
