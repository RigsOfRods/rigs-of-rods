/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer

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

#include "Mirrors.h"

#include "Application.h"
#include "Beam.h"

#include <Ogre.h>
#include <OgreMaterial.h>
#include <OgreSubsystem.h>

using namespace Ogre;
using namespace RoR;

LegacyRearViewMirrors::LegacyRearViewMirrors():
    m_is_initialized(false),
    m_is_enabled(false),
    m_mirror_camera(nullptr),
    m_rtt_target(nullptr)
{}

void LegacyRearViewMirrors::Init(Ogre::SceneManager* scene_mgr, Ogre::Camera* main_cam)
{
    assert(!m_is_initialized); // double init

    m_texture = TextureManager::getSingleton().createManual("mirrortexture"
        , ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME
        , TEX_TYPE_2D
        , 128
        , 256
        , 0
        , PF_R8G8B8
        , TU_RENDERTARGET);

    m_rtt_target = m_texture->getBuffer()->getRenderTarget();
    m_rtt_target->setActive(false);

    m_mirror_camera = scene_mgr->createCamera("MirrorCam");
    m_mirror_camera->setNearClipDistance(0.2f);
    m_mirror_camera->setFarClipDistance(main_cam->getFarClipDistance());
    m_mirror_camera->setFOVy(Degree(50));
    m_mirror_camera->setAspectRatio((main_cam->getViewport()->getActualWidth() / main_cam->getViewport()->getActualHeight()) / 2.0f);

    Viewport* v = m_rtt_target->addViewport(m_mirror_camera);
    v->setClearEveryFrame(true);
    v->setBackgroundColour(main_cam->getViewport()->getBackgroundColour());
    v->setOverlaysEnabled(false);

    m_material = MaterialManager::getSingleton().getByName("mirror");
    m_material->getTechnique(0)->getPass(0)->getTextureUnitState(0)->setTextureName("mirror.dds");
    m_material->getTechnique(0)->getPass(0)->setLightingEnabled(false);

    m_is_initialized = true;
}

void LegacyRearViewMirrors::Shutdown(Ogre::SceneManager* scene_mgr)
{
    // DO NOT: Ogre::MaterialManager::getSingleton().remove(m_material->getHandle()); // Must be kept global, vehicles reference it directly
    m_material.setNull();
    m_rtt_target = nullptr;
    TextureManager::getSingleton().remove(m_texture->getHandle());
    m_texture.setNull();
    scene_mgr->destroyCamera(m_mirror_camera);
    m_mirror_camera = nullptr;
    m_is_initialized = false;
}

void LegacyRearViewMirrors::SetActive(bool state)
{
    if (m_is_enabled == state)
        return;

    m_is_enabled = state;
    m_rtt_target->setActive(state);
    if (state)
        m_material->getTechnique(0)->getPass(0)->getTextureUnitState(0)->setTextureName("mirrortexture");
    else
        m_material->getTechnique(0)->getPass(0)->getTextureUnitState(0)->setTextureName("mirror.dds");
}

void LegacyRearViewMirrors::Update(Beam* truck, Ogre::Vector3 main_cam_pos, 
    Ogre::Vector3 main_cam_dir, Ogre::Radian main_cam_fov_y, float main_cam_aspect_ratio)
{
    if (!truck || !m_is_enabled)
    {
        SetActive(false);
        return;
    }

    // We can only render one mirror, so we choose the nearest one in the fov
    float minDist = 10000.0f;
    int mirrorType = 0;
    SceneNode* mirrorNode = nullptr;
    for (int i = 0; i < truck->free_prop; i++)
    {
        if (truck->props[i].mirror != 0)
        {
            Vector3 dist = truck->props[i].scene_node->getPosition() - main_cam_pos;
            if (dist.directionEquals(main_cam_dir, main_cam_fov_y * main_cam_aspect_ratio / 1.2f))
            {
                float fdist = dist.length();
                if (fdist < minDist)
                {
                    minDist = fdist;
                    mirrorNode = truck->props[i].scene_node;
                    mirrorType = truck->props[i].mirror;
                };
            }
        }
    }
    if (mirrorType == 0 || mirrorNode == nullptr)
    {
        SetActive(false);
        return;
    }

    Vector3 normal = Vector3::ZERO;
    Vector3 center = Vector3::ZERO;
    Radian roll = Degree(360) - Radian(asin(truck->getDirection().dotProduct(Vector3::UNIT_Y)));

    if (mirrorType == 1)
    {
        normal = mirrorNode->getOrientation() * Vector3(cos(truck->leftMirrorAngle), sin(truck->leftMirrorAngle), 0.0f);
        center = mirrorNode->getPosition() + mirrorNode->getOrientation() * Vector3(0.07f, -0.22f, 0);
    }
    else
    {
        normal = mirrorNode->getOrientation() * Vector3(cos(truck->rightMirrorAngle), sin(truck->rightMirrorAngle), 0.0f);
        center = mirrorNode->getPosition() + mirrorNode->getOrientation() * Vector3(0.07f, +0.22f, 0);
    }

    Plane plane = Plane(normal, center);
    Vector3 project = plane.projectVector(main_cam_pos - center);

    m_mirror_camera->setPosition(center);
    m_mirror_camera->lookAt(main_cam_pos - 2.0f * project);
    m_mirror_camera->roll(roll);
}
