/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2016-2017 Petr Ohlidal & contributors.

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

#include "ForwardDeclarations.h"

#include <OgreMaterial.h>
#include <OgreColourValue.h>
#include <string>
#include <vector>

namespace RoR
{

/// Visuals of `Beam`
/// TODO: Move all OGRE interactions of `class Beam` to here.
class GfxActor
{
    friend class ::RigSpawner; // The factory

public:

    struct FlareMaterial
    {
        int               flare_index;
        Ogre::MaterialPtr mat_instance;
        Ogre::ColourValue emissive_color;
    };

    enum class VideoCamState
    {
        VCSTATE_UNKNOWN,
        VCSTATE_DISABLED,
        VCSTATE_ENABLED_OFFLINE,
        VCSTATE_ENABLED_ONLINE,
    };

    GfxActor(Beam* actor, std::string ogre_resource_group):
        m_actor(actor),
        m_custom_resource_group(ogre_resource_group),
        m_vidcam_state(VideoCamState::VCSTATE_ENABLED_ONLINE)
    {}

    ~GfxActor();

    void                 AddMaterialFlare    (int flare_index, Ogre::MaterialPtr mat);
    void                 SetMaterialFlareOn  (int flare_index, bool state_on);
    void                 RegisterCabMaterial (Ogre::MaterialPtr mat, Ogre::MaterialPtr mat_trans);
    void                 SetCabLightsActive  (bool state_on);
    Ogre::MaterialPtr&   GetCabTransMaterial () { return m_cab_mat_visual_trans; }
    void                 AddVideoCamera      (VideoCamera* vidcam);
    void                 SetVideoCamState    (VideoCamState state);
    inline VideoCamState GetVideoCamState    () const { return m_vidcam_state; }
    void                 UpdateVideoCameras  (float dt_sec);

private:

    Beam*                       m_actor;
    std::string                 m_custom_resource_group; ///< Stores OGRE resources individual to this actor
    std::vector<FlareMaterial>  m_flare_materials;
    VideoCamState               m_vidcam_state;
    std::vector<VideoCamera*>   m_videocameras;

    // Cab materials and their features
    Ogre::MaterialPtr           m_cab_mat_visual; ///< Updated in-place from templates
    Ogre::MaterialPtr           m_cab_mat_visual_trans;
    Ogre::MaterialPtr           m_cab_mat_template_plain;
    Ogre::MaterialPtr           m_cab_mat_template_emissive;
};

} // Namespace RoR
