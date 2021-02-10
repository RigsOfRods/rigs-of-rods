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

#include "Water.h"

#include "AppContext.h"
#include "CameraManager.h"
#include "GfxScene.h"
#include "PlatformUtils.h" // PathCombine
#include "TerrainManager.h"

#include <Ogre.h>

using namespace Ogre;
using namespace RoR;

static const int WAVEREZ = 100;

Water::Water(Ogre::Vector3 terrn_size) :
    m_map_size(terrn_size),
    m_max_ampl(0),
    m_water_visible(true),
    m_waterplane_mesh_scale(1.0f),
    m_waterplane_force_update_pos(false),
    m_bottom_height(0),
    m_water_height(0),
    m_waterplane_node(0),
    m_frame_counter(0)
{
    if (m_map_size.x < 1500 && m_map_size.z < 1500)
        m_waterplane_mesh_scale = 1.5f;

    char line[1024] = {};
    std::string filepath = PathCombine(RoR::App::sys_config_dir->GetStr(), "wavefield.cfg");
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
// scene manager is cleared anyway
}

void Water::PrepareWater()
{
    m_water_plane.normal = Vector3::UNIT_Y;
    m_water_plane.d = 0;

    const auto type = App::gfx_water_mode->GetEnum<GfxWaterMode>();
    const bool full_gfx = type == GfxWaterMode::FULL_HQ || type == GfxWaterMode::FULL_FAST;


    
    m_waterplane_mesh = v1::MeshManager::getSingleton().createPlane("WaterPlane",
        ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
        m_water_plane,
        m_map_size.x * m_waterplane_mesh_scale, m_map_size.z * m_waterplane_mesh_scale, WAVEREZ, WAVEREZ, true, 1, 50, 50, Vector3::UNIT_Z);
    m_waterplane_entity = App::GetGfxScene()->GetSceneManager()->createEntity("plane", "WaterPlane");
    m_waterplane_entity->setMaterialName("tracks/basicwater");
    

    m_waterplane_entity->setCastShadows(false);

    //position
    m_waterplane_node = App::GetGfxScene()->GetSceneManager()->getRootSceneNode()->createChildSceneNode();
    m_waterplane_node->attachObject(m_waterplane_entity);
    m_waterplane_node->setPosition(Vector3((m_map_size.x * m_waterplane_mesh_scale) / 2, m_water_height, (m_map_size.z * m_waterplane_mesh_scale) / 2));

    //bottom
    m_bottom_plane.normal = Vector3::UNIT_Y;
    m_bottom_plane.d = -m_bottom_height; //30m below waterline
    v1::MeshManager::getSingleton().createPlane("BottomPlane",
        ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
        m_bottom_plane,
        m_map_size.x * m_waterplane_mesh_scale, m_map_size.z * m_waterplane_mesh_scale, 1, 1, true, 1, 1, 1, Vector3::UNIT_Z);
    v1::Entity* pE = App::GetGfxScene()->GetSceneManager()->createEntity("BottomPlane");
    pE->setMaterialName("tracks/seabottom");
    pE->setCastShadows(false);

    //position
    m_bottomplane_node = App::GetGfxScene()->GetSceneManager()->getRootSceneNode()->createChildSceneNode();
    m_bottomplane_node->attachObject(pE);
    m_bottomplane_node->setPosition(Vector3((m_map_size.x * m_waterplane_mesh_scale) / 2, 0, (m_map_size.z * m_waterplane_mesh_scale) / 2));


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
    const auto type = App::gfx_water_mode->GetEnum<GfxWaterMode>();
    if (type == GfxWaterMode::FULL_HQ || type == GfxWaterMode::FULL_FAST || type == GfxWaterMode::REFLECT)
    {
        this->UpdateReflectionPlane(centerheight);
    }
}

bool Water::IsCameraUnderWater()
{
    return (App::GetCameraManager()->GetCameraNode()->getPosition().y < CalcWavesHeight(App::GetCameraManager()->GetCameraNode()->getPosition()));
}

void Water::UpdateWater()
{
    if (!m_water_visible)
        return;

    const Ogre::Vector3    camera_pos = (m_cam_forced) ? m_cam_forced_position : App::GetCameraManager()->GetCameraNode()->getPosition();
    const Ogre::Quaternion camera_rot = (m_cam_forced) ? m_cam_forced_orientation : App::GetCameraManager()->GetCameraNode()->getOrientation();
    const Ogre::Radian     camera_fov = (m_cam_forced) ? m_cam_forced_fovy : App::GetCameraManager()->GetCamera()->getFOVy();

    if (m_waterplane_node)
    {
        const Vector3 water_cam_pos(camera_pos.x, m_water_height, camera_pos.z);
        Vector3 sightPos(water_cam_pos);
        // Direction points down -Z by default (adapted from Ogre::Camera)
        Ogre::Vector3 cameraDir = camera_rot * -Ogre::Vector3::UNIT_Z;

        Ray lineOfSight(camera_pos, cameraDir);
        Plane waterPlane(Vector3::UNIT_Y, Vector3::UNIT_Y * m_water_height);

        std::pair<bool, Real> intersection = lineOfSight.intersects(waterPlane);

        if (intersection.first && intersection.second > 0.0f)
            sightPos = lineOfSight.getPoint(intersection.second);

        Real offset = std::min(water_cam_pos.distance(sightPos), std::min(m_map_size.x, m_map_size.z) * 0.5f);

        Vector3 waterPos = water_cam_pos + (sightPos - water_cam_pos).normalisedCopy() * offset;
        Vector3 bottomPos = Vector3(waterPos.x, m_bottom_height, waterPos.z);

        if (waterPos.distance(m_waterplane_node->getPosition()) > 200.0f || m_waterplane_force_update_pos)
        {
            m_waterplane_node->setPosition(Vector3(waterPos.x, m_water_height, waterPos.z));
            m_bottomplane_node->setPosition(bottomPos);
        }

    }

    
    m_waterplane_force_update_pos = false;
}

void Water::WaterPrepareShutdown()
{

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

void Water::SetWaterBottomHeight(float value)
{
    m_bottom_height = value;
}

float Water::CalcWavesHeight(Vector3 pos)
{
    // no waves?
    if (!RoR::App::gfx_water_waves->GetBool() || RoR::App::mp_state->GetEnum<MpState>() == RoR::MpState::CONNECTED)
    {
        // constant height, sea is flat as pancake
        return m_water_height;
    }

    const float time_sec = (float)(App::GetAppContext()->GetOgreRoot()->getTimer()->getMilliseconds() * 0.001);

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
        result += amp * sin(Math::TWO_PI * ((time_sec * m_wavetrain_defs[i].wavespeed + m_wavetrain_defs[i].dir_sin * pos.x + m_wavetrain_defs[i].dir_cos * pos.z) / m_wavetrain_defs[i].wavelength));
    }
    // return the summed up waves
    return result;
}

bool Water::IsUnderWater(Vector3 pos)
{
    float waterheight = m_water_height;

    if (RoR::App::gfx_water_waves->GetBool() && RoR::App::mp_state->GetEnum<MpState>() == RoR::MpState::DISABLED)
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
    if (!RoR::App::gfx_water_waves->GetBool() || RoR::App::mp_state->GetEnum<MpState>() == RoR::MpState::CONNECTED)
        return Vector3::ZERO;

    float waveheight = GetWaveHeight(pos);

    if (pos.y > m_water_height + m_max_ampl)
        return Vector3::ZERO;

    Vector3 result(Vector3::ZERO);

    const float time_sec = (float)(App::GetAppContext()->GetOgreRoot()->getTimer()->getMilliseconds() * 0.001);

    for (size_t i = 0; i < m_wavetrain_defs.size(); i++)
    {
        float amp = std::min(m_wavetrain_defs[i].amplitude * waveheight, m_wavetrain_defs[i].maxheight);
        float speed = Math::TWO_PI * amp / (m_wavetrain_defs[i].wavelength / m_wavetrain_defs[i].wavespeed);
        float coeff = Math::TWO_PI * (time_sec * m_wavetrain_defs[i].wavespeed + m_wavetrain_defs[i].dir_sin * pos.x + m_wavetrain_defs[i].dir_cos * pos.z) / m_wavetrain_defs[i].wavelength;
        result.y += speed * cos(coeff);
        result += Vector3(m_wavetrain_defs[i].dir_sin, 0, m_wavetrain_defs[i].dir_cos) * speed * sin(coeff);
    }

    return result;
}

void Water::UpdateReflectionPlane(float h)
{

}

void Water::FrameStepWater(float dt)
{
    if (dt)
    {
        this->UpdateWater();
    }
}

void Water::SetForcedCameraTransform(Ogre::Radian fovy, Ogre::Vector3 pos, Ogre::Quaternion rot)
{
    m_cam_forced = true;
    m_cam_forced_fovy = fovy;
    m_cam_forced_position = pos;
    m_cam_forced_orientation = rot;
}

void Water::ClearForcedCameraTransform()
{
    m_cam_forced = false;
    m_cam_forced_fovy = 0;
    m_cam_forced_position = Ogre::Vector3::ZERO;
    m_cam_forced_orientation = Ogre::Quaternion::IDENTITY;
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
