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

#include "Water.h"

#include "OgreSubsystem.h"
#include "PlatformUtils.h" // PathCombine
#include "RoRFrameListener.h"
#include "TerrainManager.h"

#include <Ogre.h>

using namespace Ogre;
using namespace RoR;

static const int WAVEREZ = 100;

Water::Water() :
    m_max_ampl(0),
    m_water_visible(true),
    m_waterplane_mesh_scale(1.0f),
    m_refract_rtt_viewport(0),
    m_reflect_rtt_viewport(0),
    m_render_cam(gEnv->mainCamera),
    m_bottom_height(0),
    m_water_height(0),
    m_waterplane_node(0),
    m_waterplane_force_update_pos(false),
    m_frame_counter(0),
    m_refract_rtt_target(0),
    m_reflect_rtt_target(0),
    m_reflect_cam(0),
    m_refract_cam(0)
{
    //Ugh.. Why so ugly and hard to read
    m_map_size = App::GetSimTerrain()->getMaxTerrainSize();
    m_reflect_listener.scene_mgr = gEnv->sceneManager;
    m_refract_listener.scene_mgr = gEnv->sceneManager;

    if (m_map_size.x < 1500 && m_map_size.z < 1500)
        m_waterplane_mesh_scale = 1.5f;

    char line[1024] = {};
    std::string filepath = PathCombine(RoR::App::sys_config_dir.GetActive(), "wavefield.cfg");
    FILE* fd = fopen(filepath.c_str(), "r");
    if (fd)
    {
        while (!feof(fd))
        {
            int res = fscanf(fd, " %[^\n\r]", line);
            if (line[0] == ';')
                continue;
            float wl, amp, mx, dir;
            res = sscanf(line, "%f, %f, %f, %f", &wl, &amp, &mx, &dir);
            if (res < 4)
                continue;

            WaveTrain wavetrain;
            wavetrain.wavelength = wl;
            wavetrain.amplitude = amp;
            wavetrain.maxheight = mx;
            wavetrain.direction = dir / 57.0;
            wavetrain.dir_sin = sin(wavetrain.direction);
            wavetrain.dir_cos = cos(wavetrain.direction);

            m_wavetrain_defs.push_back(wavetrain);
        }
        fclose(fd);
    }
    for (size_t i = 0; i < m_wavetrain_defs.size(); i++)
    {
        m_wavetrain_defs[i].wavespeed = 1.25 * sqrt(m_wavetrain_defs[i].wavelength);
        m_max_ampl += m_wavetrain_defs[i].maxheight;
    }

    this->PrepareWater();
}

Water::~Water()
{
    if (m_refract_cam != nullptr)
    {
        gEnv->sceneManager->destroyCamera(m_refract_cam);
        m_refract_cam = nullptr;
    }

    if (m_reflect_cam != nullptr)
    {
        gEnv->sceneManager->destroyCamera(m_reflect_cam);
        m_reflect_cam = nullptr;
    }

    if (m_waterplane_entity != nullptr)
    {
        gEnv->sceneManager->destroyEntity("plane");
        m_waterplane_entity = nullptr;
    }

    if (m_waterplane_node != nullptr)
    {
        gEnv->sceneManager->getRootSceneNode()->removeAndDestroyChild("WaterPlane");
        m_waterplane_node = nullptr;
    }

    if (m_bottomplane_node != nullptr)
    {
        gEnv->sceneManager->destroyEntity("bplane");
        gEnv->sceneManager->getRootSceneNode()->removeAndDestroyChild("BottomWaterPlane");
        m_bottomplane_node = nullptr;
    }

    m_water_height = m_bottom_height = 0;
    m_render_cam = nullptr;

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
        m_refract_rtt_texture.setNull();
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
        m_reflect_rtt_texture.setNull();
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

void Water::PrepareWater()
{
    m_water_plane.normal = Vector3::UNIT_Y;
    m_water_plane.d = 0;

    const auto type = App::gfx_water_mode.GetActive();
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
                m_refract_cam = gEnv->sceneManager->createCamera("RefractCam");
                m_refract_cam->setNearClipDistance(m_render_cam->getNearClipDistance());
                m_refract_cam->setFarClipDistance(m_render_cam->getFarClipDistance());
                m_refract_cam->setAspectRatio(
                    (Real)RoR::App::GetOgreSubsystem()->GetRenderWindow()->getViewport(0)->getActualWidth() /
                    (Real)RoR::App::GetOgreSubsystem()->GetRenderWindow()->getViewport(0)->getActualHeight());

                m_refract_rtt_viewport = m_refract_rtt_target->addViewport(m_refract_cam);
                m_refract_rtt_viewport->setClearEveryFrame(true);
                m_refract_rtt_viewport->setBackgroundColour(gEnv->sceneManager->getFogColour());

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
            m_reflect_cam = gEnv->sceneManager->createCamera("ReflectCam");
            m_reflect_cam->setNearClipDistance(m_render_cam->getNearClipDistance());
            m_reflect_cam->setFarClipDistance(m_render_cam->getFarClipDistance());
            m_reflect_cam->setAspectRatio(
                (Real)RoR::App::GetOgreSubsystem()->GetRenderWindow()->getViewport(0)->getActualWidth() /
                (Real)RoR::App::GetOgreSubsystem()->GetRenderWindow()->getViewport(0)->getActualHeight());

            m_reflect_rtt_viewport = m_reflect_rtt_target->addViewport(m_reflect_cam);
            m_reflect_rtt_viewport->setClearEveryFrame(true);
            m_reflect_rtt_viewport->setBackgroundColour(gEnv->sceneManager->getFogColour());

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

        m_waterplane_entity = gEnv->sceneManager->createEntity("plane", "ReflectPlane");
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
        m_waterplane_entity = gEnv->sceneManager->createEntity("plane", "WaterPlane");
        m_waterplane_entity->setMaterialName("tracks/basicwater");
    }

    m_waterplane_entity->setCastShadows(false);
    m_reflect_listener.waterplane_entity = m_waterplane_entity;
    m_refract_listener.waterplane_entity = m_waterplane_entity;
    //position
    m_waterplane_node = gEnv->sceneManager->getRootSceneNode()->createChildSceneNode("WaterPlane");
    m_waterplane_node->attachObject(m_waterplane_entity);
    m_waterplane_node->setPosition(Vector3((m_map_size.x * m_waterplane_mesh_scale) / 2, m_water_height, (m_map_size.z * m_waterplane_mesh_scale) / 2));

    //bottom
    m_bottom_plane.normal = Vector3::UNIT_Y;
    m_bottom_plane.d = -m_bottom_height; //30m below waterline
    MeshManager::getSingleton().createPlane("BottomPlane",
        ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
        m_bottom_plane,
        m_map_size.x * m_waterplane_mesh_scale, m_map_size.z * m_waterplane_mesh_scale, 1, 1, true, 1, 1, 1, Vector3::UNIT_Z);
    Entity* pE = gEnv->sceneManager->createEntity("bplane", "BottomPlane");
    pE->setMaterialName("tracks/seabottom");
    pE->setCastShadows(false);

    //position
    m_bottomplane_node = gEnv->sceneManager->getRootSceneNode()->createChildSceneNode("BottomWaterPlane");
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

void Water::SetWaterVisible(bool value)
{
    m_water_visible = value;
    if (m_waterplane_entity)
        m_waterplane_entity->setVisible(value);
    if (m_waterplane_node)
        m_waterplane_node->setVisible(value);
    if (m_bottomplane_node)
        m_bottomplane_node->setVisible(value);
}

void Water::SetReflectionPlaneHeight(float centerheight)
{
    const auto type = App::gfx_water_mode.GetActive();
    if (type == GfxWaterMode::FULL_HQ || type == GfxWaterMode::FULL_FAST || type == GfxWaterMode::REFLECT)
    {
        this->UpdateReflectionPlane(centerheight);
    }
}

void Water::ShowWave(Vector3 refpos)
{
    if (!m_waterplane_vert_buf_local)
        return;

    float xScaled = m_map_size.x * m_waterplane_mesh_scale;
    float zScaled = m_map_size.z * m_waterplane_mesh_scale;

    for (int pz = 0; pz < WAVEREZ + 1; pz++)
    {
        for (int px = 0; px < WAVEREZ + 1; px++)
        {
            m_waterplane_vert_buf_local[(pz * (WAVEREZ + 1) + px) * 8 + 1] = CalcWavesHeight(refpos + Vector3(xScaled * 0.5 - (float)px * xScaled / WAVEREZ, 0, (float)pz * zScaled / WAVEREZ - zScaled * 0.5)) - m_water_height;
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

bool Water::IsCameraUnderWater()
{
    if (m_render_cam)
    {
        return (m_render_cam->getPosition().y < CalcWavesHeight(m_render_cam->getPosition()));
    }
    return false;
}

void Water::UpdateWater()
{
    if (!m_water_visible || !m_render_cam)
        return;

    if (m_waterplane_node)
    {
        Vector3 cameraPos(m_render_cam->getPosition().x, m_water_height, m_render_cam->getPosition().z);
        Vector3 sightPos(cameraPos);

        Ray lineOfSight(m_render_cam->getPosition(), m_render_cam->getDirection());
        Plane waterPlane(Vector3::UNIT_Y, Vector3::UNIT_Y * m_water_height);

        std::pair<bool, Real> intersection = lineOfSight.intersects(waterPlane);

        if (intersection.first && intersection.second > 0.0f)
            sightPos = lineOfSight.getPoint(intersection.second);

        Real offset = std::min(cameraPos.distance(sightPos), std::min(m_map_size.x, m_map_size.z) * 0.5f);

        Vector3 waterPos = cameraPos + (sightPos - cameraPos).normalisedCopy() * offset;
        Vector3 bottomPos = Vector3(waterPos.x, m_bottom_height, waterPos.z);

        if (waterPos.distance(m_waterplane_node->getPosition()) > 200.0f || m_waterplane_force_update_pos)
        {
            m_waterplane_node->setPosition(Vector3(waterPos.x, m_water_height, waterPos.z));
            m_bottomplane_node->setPosition(bottomPos);
        }
        if (RoR::App::gfx_water_waves.GetActive() && RoR::App::mp_state.GetActive() == RoR::MpState::DISABLED)
            this->ShowWave(m_waterplane_node->getPosition());
    }

    m_frame_counter++;
    if (App::gfx_water_mode.GetActive() == GfxWaterMode::FULL_FAST)
    {
        if (m_frame_counter % 2 == 1 || m_waterplane_force_update_pos)
        {
            m_reflect_cam->setOrientation(m_render_cam->getOrientation());
            m_reflect_cam->setPosition(m_render_cam->getPosition());
            m_reflect_cam->setFOVy(m_render_cam->getFOVy());
            m_reflect_rtt_target->update();
        }
        if (m_frame_counter % 2 == 0 || m_waterplane_force_update_pos)
        {
            m_refract_cam->setOrientation(m_render_cam->getOrientation());
            m_refract_cam->setPosition(m_render_cam->getPosition());
            m_refract_cam->setFOVy(m_render_cam->getFOVy());
            m_refract_rtt_target->update();
        }
    }
    else if (App::gfx_water_mode.GetActive() == GfxWaterMode::FULL_HQ)
    {
        m_reflect_cam->setOrientation(m_render_cam->getOrientation());
        m_reflect_cam->setPosition(m_render_cam->getPosition());
        m_reflect_cam->setFOVy(m_render_cam->getFOVy());
        m_reflect_rtt_target->update();
        m_refract_cam->setOrientation(m_render_cam->getOrientation());
        m_refract_cam->setPosition(m_render_cam->getPosition());
        m_refract_cam->setFOVy(m_render_cam->getFOVy());
        m_refract_rtt_target->update();
    }
    else if (App::gfx_water_mode.GetActive() == GfxWaterMode::REFLECT)
    {
        m_reflect_cam->setOrientation(m_render_cam->getOrientation());
        m_reflect_cam->setPosition(m_render_cam->getPosition());
        m_reflect_cam->setFOVy(m_render_cam->getFOVy());
        m_reflect_rtt_target->update();
    }

    m_waterplane_force_update_pos = false;
}

void Water::WaterPrepareShutdown()
{
    if (m_refract_rtt_target)
        m_refract_rtt_target->removeListener(&m_refract_listener);
    if (m_reflect_rtt_target)
        m_reflect_rtt_target->removeListener(&m_reflect_listener);
}

float Water::GetStaticWaterHeight()
{
    return m_water_height;
};

void Water::SetStaticWaterHeight(float value)
{
    m_water_height = value;
    m_waterplane_force_update_pos = true;
}

float Water::CalcWavesHeight(Vector3 pos)
{
    // no waves?
    if (!RoR::App::gfx_water_waves.GetActive() || RoR::App::mp_state.GetActive() == RoR::MpState::CONNECTED)
    {
        // constant height, sea is flat as pancake
        return m_water_height;
    }

    // uh, some upper limit?!
    if (pos.y > m_water_height + m_max_ampl)
        return m_water_height;

    float waveheight = GetWaveHeight(pos);
    // we will store the result in this variable, init it with the default height
    float result = m_water_height;
    // now walk through all the wave trains. One 'train' is one sin/cos set that will generate once wave. All the trains together will sum up, so that they generate a 'rough' sea
    for (size_t i = 0; i < m_wavetrain_defs.size(); i++)
    {
        // calculate the amplitude that this wave will have. wavetrains[i].amplitude is read from the config
        // upper limit: prevent too big waves by setting an upper limit
        float amp = std::min(m_wavetrain_defs[i].amplitude * waveheight, m_wavetrain_defs[i].maxheight);
        // now the main thing:
        // calculate the sinus with the values of the config file and add it to the result
        result += amp * sin(Math::TWO_PI * ((gEnv->mrTime * m_wavetrain_defs[i].wavespeed + m_wavetrain_defs[i].dir_sin * pos.x + m_wavetrain_defs[i].dir_cos * pos.z) / m_wavetrain_defs[i].wavelength));
    }
    // return the summed up waves
    return result;
}

bool Water::IsUnderWater(Vector3 pos)
{
    float waterheight = m_water_height;

    if (RoR::App::gfx_water_waves.GetActive() && RoR::App::mp_state.GetActive() == RoR::MpState::DISABLED)
    {
        float waveheight = GetWaveHeight(pos);

        if (pos.y > m_water_height + m_max_ampl * waveheight || pos.y > m_water_height + m_max_ampl)
            return false;

        waterheight = CalcWavesHeight(pos);
    }

    return pos.y < waterheight;
}

Vector3 Water::CalcWavesVelocity(Vector3 pos)
{
    if (!RoR::App::gfx_water_waves.GetActive() || RoR::App::mp_state.GetActive() == RoR::MpState::CONNECTED)
        return Vector3::ZERO;

    float waveheight = GetWaveHeight(pos);

    if (pos.y > m_water_height + m_max_ampl)
        return Vector3::ZERO;

    Vector3 result(Vector3::ZERO);

    for (size_t i = 0; i < m_wavetrain_defs.size(); i++)
    {
        float amp = std::min(m_wavetrain_defs[i].amplitude * waveheight, m_wavetrain_defs[i].maxheight);
        float speed = Math::TWO_PI * amp / (m_wavetrain_defs[i].wavelength / m_wavetrain_defs[i].wavespeed);
        float coeff = Math::TWO_PI * (gEnv->mrTime * m_wavetrain_defs[i].wavespeed + m_wavetrain_defs[i].dir_sin * pos.x + m_wavetrain_defs[i].dir_cos * pos.z) / m_wavetrain_defs[i].wavelength;
        result.y += speed * cos(coeff);
        result += Vector3(m_wavetrain_defs[i].dir_sin, 0, m_wavetrain_defs[i].dir_cos) * speed * sin(coeff);
    }

    return result;
}

void Water::UpdateReflectionPlane(float h)
{
    bool underwater = this->IsCameraUnderWater();
    if (underwater)
    {
        m_reflect_plane.normal = -Vector3::UNIT_Y;
        m_refract_plane.normal = Vector3::UNIT_Y;
        m_reflect_plane.d = h + 0.15;
        m_refract_plane.d = -h + 0.15;
        m_water_plane.d = -h;
    }
    else
    {
        m_reflect_plane.normal = Vector3::UNIT_Y;
        m_refract_plane.normal = -Vector3::UNIT_Y;
        m_reflect_plane.d = -h + 0.15;
        m_refract_plane.d = h + 0.15;
        m_water_plane.d = -h;
    }

    if (m_refract_cam)
    {
        m_refract_cam->enableCustomNearClipPlane(m_refract_plane);
    }

    if (m_reflect_cam)
    {
        m_reflect_cam->enableReflection(m_water_plane);
        m_reflect_cam->enableCustomNearClipPlane(m_reflect_plane);
    };
}

void Water::WaterSetCamera(Ogre::Camera* cam)
{
    m_render_cam = cam;
}

void Water::FrameStepWater(float dt)
{
    if (dt)
    {
        this->UpdateWater();
    }
}

float Water::GetWaveHeight(Vector3 pos)
{
    // calculate how high the waves should be at this point
    //  (mapsize.x * m_waterplane_mesh_scale) / 2 = terrain width / 2
    //  (mapsize.z * m_waterplane_mesh_scale) / 2 = terrain height / 2
    // calculate distance to the center of the terrain and divide by 3.000.000
    float waveheight = (pos - Vector3((m_map_size.x * m_waterplane_mesh_scale) * 0.5, m_water_height, (m_map_size.z * m_waterplane_mesh_scale) * 0.5)).squaredLength() / 3000000.0;

    return waveheight;
}


// ------------------------- The listeners -------------------------


void Water::RefractionListener::preRenderTargetUpdate(const Ogre::RenderTargetEvent& evt)
{
    this->scene_mgr->getRenderQueue()->getQueueGroup(RENDER_QUEUE_MAIN)->setShadowsEnabled(false);
    this->waterplane_entity->setVisible(false);
    App::GetSimController()->GetGfxScene().SetParticlesVisible(false); // Hide water spray
}

void Water::RefractionListener::postRenderTargetUpdate(const Ogre::RenderTargetEvent& evt)
{
    this->scene_mgr->getRenderQueue()->getQueueGroup(RENDER_QUEUE_MAIN)->setShadowsEnabled(true);
    this->waterplane_entity->setVisible(true);
    App::GetSimController()->GetGfxScene().SetParticlesVisible(true); // Restore water spray
}

void Water::ReflectionListener::preRenderTargetUpdate(const Ogre::RenderTargetEvent& evt)
{
    this->scene_mgr->getRenderQueue()->getQueueGroup(RENDER_QUEUE_MAIN)->setShadowsEnabled(false);
    this->waterplane_entity->setVisible(false);
}

void Water::ReflectionListener::postRenderTargetUpdate(const Ogre::RenderTargetEvent& evt)
{
    this->scene_mgr->getRenderQueue()->getQueueGroup(RENDER_QUEUE_MAIN)->setShadowsEnabled(true);
    this->waterplane_entity->setVisible(true);
}
