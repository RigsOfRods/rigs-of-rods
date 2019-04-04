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

#pragma once

#include "RoRPrerequisites.h"

#include <Ogre.h>

namespace RoR {

/// 'renderdash' is a name of a classic Render-To-Texture animated material with gauges and other dashboard info.
class Renderdash: public Ogre::RenderTargetListener
{
public:
    Renderdash(std::string const& rg_name, std::string const& tex_name);
    ~Renderdash();

    void setEnable(bool en);

    // Ogre::RenderTargetListener
    void preRenderTargetUpdate(const Ogre::RenderTargetEvent& evt) override;
    void postRenderTargetUpdate(const Ogre::RenderTargetEvent& evt) override;

private:
    Ogre::Camera*          m_dash_cam;
    Ogre::RenderTexture*   m_rtt_tex;
    Ogre::TexturePtr       m_texture;
    Ogre::Overlay*         m_blend_overlay;
    Ogre::Overlay*         m_dash_overlay;
    Ogre::Overlay*         m_fps_overlay;
    Ogre::Overlay*         m_needles_overlay;
    bool                   m_fps_displayed;
};

} // namespace RoR
