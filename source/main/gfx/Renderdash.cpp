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

#include "Renderdash.h"

#include <Overlay/OgreOverlay.h>
#include <Overlay/OgreOverlayManager.h>

RoR::Renderdash::Renderdash(std::string const &rg_name, std::string const &tex_name, std::string const &cam_name)
    : m_dash_cam(nullptr), m_rtt_tex(nullptr), m_blend_overlay(nullptr), m_dash_overlay(nullptr), m_fps_overlay(nullptr),
      m_needles_overlay(nullptr), m_fps_displayed(false)
{
    m_texture = Ogre::TextureManager::getSingleton().createManual(tex_name, rg_name, Ogre::TEX_TYPE_2D, 1024, 512, 0,
                                                                  Ogre::PF_R8G8B8, Ogre::TU_RENDERTARGET);

    m_rtt_tex = m_texture->getBuffer()->getRenderTarget();

    static int cam_counter = 0;
    m_dash_cam             = gEnv->sceneManager->createCamera(cam_name);
    m_dash_cam->setNearClipDistance(1.0);
    m_dash_cam->setFarClipDistance(10.0);
    m_dash_cam->setPosition(Ogre::Vector3(0.0, -10000.0, 0.0));

    m_dash_cam->setAspectRatio(2.0);

    Ogre::Viewport *v = m_rtt_tex->addViewport(m_dash_cam);
    v->setClearEveryFrame(true);
    v->setBackgroundColour(Ogre::ColourValue::Black);

    // NOTE: The 'renderdash' material is defined as a dummy in file 'ror.material' which is loaded into every actor's resource
    // group.
    Ogre::MaterialPtr mat = Ogre::MaterialManager::getSingleton().getByName("renderdash", rg_name);
    mat->getTechnique(0)->getPass(0)->getTextureUnitState(0)->setTexture(m_texture);

    m_dash_overlay    = Ogre::OverlayManager::getSingleton().getByName("tracks/3D_DashboardOverlay");
    m_needles_overlay = Ogre::OverlayManager::getSingleton().getByName("tracks/3D_NeedlesOverlay");
    m_blend_overlay   = Ogre::OverlayManager::getSingleton().getByName("tracks/3D_BlendOverlay");

    m_rtt_tex->addListener(this);
    m_rtt_tex->setActive(false);
}

RoR::Renderdash::~Renderdash()
{
    if (m_rtt_tex != nullptr) m_rtt_tex->removeListener(this);
    gEnv->sceneManager->destroyCamera(m_dash_cam);
    Ogre::TextureManager::getSingleton().remove(m_texture);
}

void RoR::Renderdash::setEnable(bool en)
{
    m_rtt_tex->setActive(en);
}

void RoR::Renderdash::preRenderTargetUpdate(const Ogre::RenderTargetEvent &evt)
{
    // hide everything
    gEnv->sceneManager->setFindVisibleObjects(false);

    // hide fps stats
    if (m_fps_overlay)
    {
        m_fps_displayed = m_fps_overlay->isVisible();
        if (m_fps_displayed) { m_fps_overlay->hide(); }
    }
    else
    {
        // this must be here and not in the constructor, as upon construction time the overlaymanager is not working, somehow
        m_fps_overlay = Ogre::OverlayManager::getSingleton().getByName("Core/DebugOverlay");
    }

    // show overlay
    m_dash_overlay->show();
    m_needles_overlay->show();
    m_blend_overlay->show();
}

void RoR::Renderdash::postRenderTargetUpdate(const Ogre::RenderTargetEvent &evt)
{
    // show everything
    gEnv->sceneManager->setFindVisibleObjects(true);

    // show everything again, if it was displayed before hiding it...
    if (m_fps_overlay && m_fps_displayed) { m_fps_overlay->show(); }

    // hide overlay
    m_dash_overlay->hide();
    m_needles_overlay->hide();
    m_blend_overlay->hide();
}
