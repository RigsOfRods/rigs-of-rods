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

#include "Actor.h"
#include "Application.h"
#include "GfxScene.h"
#include "GUIManager.h"
#include "GUI_DirectionArrow.h"
#include "OverlayWrapper.h"

#include <Overlay/OgreOverlayManager.h>
#include <Overlay/OgreOverlay.h>

using namespace RoR;

RoR::Renderdash::Renderdash(std::string const& rg_name, std::string const& tex_name, std::string const& cam_name)
    : m_dash_cam(nullptr)
    , m_rtt_tex(nullptr)
    , m_blend_overlay(nullptr)
    , m_dash_overlay(nullptr)
    , m_needles_overlay(nullptr)
{
    m_texture = Ogre::TextureManager::getSingleton().createManual(
        tex_name, rg_name, Ogre::TEX_TYPE_2D, 1024, 512, 0, Ogre::PF_R8G8B8, Ogre::TU_RENDERTARGET);

    m_rtt_tex = m_texture->getBuffer()->getRenderTarget();

    static int cam_counter = 0;
    m_dash_cam = App::GetGfxScene()->GetSceneManager()->createCamera(cam_name);
    m_dash_cam->setNearClipDistance(1.0);
    m_dash_cam->setFarClipDistance(10.0);

    Ogre::SceneNode* m_dash_cam_snode = App::GetGfxScene()->GetSceneManager()->getRootSceneNode()->createChildSceneNode();
    m_dash_cam_snode->attachObject(m_dash_cam);
    m_dash_cam_snode->setPosition(Ogre::Vector3(0.0, -10000.0, 0.0));

    m_dash_cam->setAspectRatio(2.0);

    Ogre::Viewport* v = m_rtt_tex->addViewport(m_dash_cam);
    v->setClearEveryFrame(true);
    v->setBackgroundColour(Ogre::ColourValue::Black);

    // NOTE: The 'renderdash' material is defined as a dummy in file 'ror.material' which is loaded into every actor's resource group.
    Ogre::MaterialPtr mat = Ogre::MaterialManager::getSingleton().getByName("renderdash", rg_name);
    mat->getTechnique(0)->getPass(0)->getTextureUnitState(0)->setTexture(m_texture);

    m_dash_overlay = Ogre::OverlayManager::getSingleton().getByName("tracks/3D_DashboardOverlay");
    m_needles_overlay = Ogre::OverlayManager::getSingleton().getByName("tracks/3D_NeedlesOverlay");
    m_blend_overlay = Ogre::OverlayManager::getSingleton().getByName("tracks/3D_BlendOverlay");

    m_rtt_tex->addListener(this);
    m_rtt_tex->setActive(false);

    // IMPORTANT: Must be manually updated as a workaround
    //  Under Windows with DirectX9 rendering, once auto-updated RTT is enabled, all DearIMGUI colors change: blue turns light orange, yellow turns light blue, red turns dark purple etc... looks like a hue shift but I haven't researched further. The moment the RTT is deactivated colors go back to normal.
    m_rtt_tex->setAutoUpdated(false);
}

RoR::Renderdash::~Renderdash()
{
    if (m_rtt_tex != nullptr)
        m_rtt_tex->removeListener(this);
    App::GetGfxScene()->GetSceneManager()->destroyCamera(m_dash_cam);
    Ogre::TextureManager::getSingleton().remove(m_texture);
}

void RoR::Renderdash::setEnable(bool en)
{
    m_rtt_tex->setActive(en);
}

void RoR::Renderdash::preRenderTargetUpdate(const Ogre::RenderTargetEvent& evt)
{
    // hide everything
    App::GetGfxScene()->GetSceneManager()->setFindVisibleObjects(false);

    // Disable DearIMGUI overlay
    App::GetGuiManager()->GetImGui()->setVisible(false);

    // Disable other overlays
    App::GetOverlayWrapper()->HideRacingOverlay();
    App::GetGuiManager()->DirectionArrow.SetVisible(false);

    //show overlay
    m_dash_overlay->show();
    m_needles_overlay->show();
    m_blend_overlay->show();
}

void RoR::Renderdash::postRenderTargetUpdate(const Ogre::RenderTargetEvent& evt)
{
    // show everything
    App::GetGfxScene()->GetSceneManager()->setFindVisibleObjects(true);

    // Enable DearIMGUI overlay
    App::GetGuiManager()->GetImGui()->setVisible(true);

    // Overlays 'racing' and 'direction arrow' are re-enabled automatically if needed

    // hide overlay
    m_dash_overlay->hide();
    m_needles_overlay->hide();
    m_blend_overlay->hide();
}
