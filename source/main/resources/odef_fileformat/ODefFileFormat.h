/*
    This source file is part of Rigs of Rods
    Copyright 2016-2017 Petr Ohlidal

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

#pragma once

#include "SimData.h"

#include <OgreColourValue.h>
#include <OgreDataStream.h>
#include <OgreVector3.h>

#include <memory>
#include <string>

#ifdef DELETE // Windows hack: The `EventType::DELETE` name collides with `DELETE` from <winnt.h> which *MAY* be included at this point.
#    undef DELETE
#endif

namespace RoR {

struct ODefCollisionBox
{
    ODefCollisionBox(Ogre::Vector3 min, Ogre::Vector3 max, Ogre::Vector3 rot,
        Ogre::Vector3 campos, Ogre::Vector3 dir, Ogre::Vector3 scal,
        std::string reverb_preset_name, std::string ev_name, CollisionEventFilter ev_filter,
        bool is_rot, bool is_virt, bool forcecam):

        aabb_min(min), aabb_max(max), box_rot(rot), cam_pos(campos), direction(dir), scale(scal),
        reverb_preset_name(reverb_preset_name), event_name(ev_name), event_filter(ev_filter),
        is_rotating(is_rot), is_virtual(is_virt), force_cam_pos(forcecam)
    {}

    Ogre::Vector3        aabb_min;
    Ogre::Vector3        aabb_max;
    Ogre::Vector3        box_rot;
    Ogre::Vector3        cam_pos;
    Ogre::Vector3        direction;
    Ogre::Vector3        scale;
    std::string          reverb_preset_name;
    std::string          event_name;
    CollisionEventFilter event_filter;

    // Flags
    bool is_rotating:1;
    bool is_virtual:1;
    bool force_cam_pos:1;
};

struct ODefCollisionMesh
{
    ODefCollisionMesh(std::string meshname, Ogre::Vector3 scal, std::string gmodelname):
        mesh_name(meshname), scale(scal), groundmodel_name(gmodelname)
    {}

    std::string      mesh_name;
    Ogre::Vector3    scale;
    std::string      groundmodel_name;
};

struct ODefParticleSys
{
    std::string      instance_name;
    std::string      template_name;
    Ogre::Vector3    pos = Ogre::Vector3::ZERO;
    float            scale = 0.f;
};

struct ODefAnimation
{
    float            speed_min=0.f, speed_max=0.f;
    std::string      name;
};

struct ODefTexPrint
{
    std::string      font_name;
    int              font_size = 0;
    int              font_dpi = 0;
    std::string      text;
    char             option = '\0';

    float   x=0,  y=0,  w=0,  h=0;
    float   a=0,  r=0,  g=0,  b=0;
};

struct ODefSpotlight
{
    Ogre::Vector3      pos = Ogre::Vector3::ZERO;
    Ogre::Vector3      dir = Ogre::Vector3::ZERO;
    float              range = 0.f;
    float              angle_inner = 0.f;                //!< Degrees
    float              angle_outer = 0.f;                //!< Degrees
    Ogre::ColourValue  color = Ogre::ColourValue::Black;
};

struct ODefPointLight
{
    Ogre::Vector3      pos = Ogre::Vector3::ZERO;
    Ogre::Vector3      dir = Ogre::Vector3::ZERO;
    float              range = 0.f;
    Ogre::ColourValue  color = Ogre::ColourValue::Black;
};

struct ODefDocument
{
    struct ODefDocumentHeader
    {
        std::string   mesh_name;
        Ogre::Vector3 scale;
        bool          cast_shadows = true;
    }                             header;

    bool                          mode_standard = false;
    std::vector<LocalizerType>    localizers;
    std::list<std::string>        sounds;
    std::list<std::string>        groundmodel_files;
    std::list<ODefCollisionBox>   collision_boxes;
    std::list<ODefCollisionMesh>  collision_meshes;
    std::list<ODefParticleSys>    particle_systems;
    std::list<ODefAnimation>      animations;
    std::list<ODefTexPrint>       texture_prints;    //!< Section 'drawTextOnMeshTexture'
    std::list<ODefSpotlight>      spotlights;
    std::list<ODefPointLight>     point_lights;
    std::string                   mat_name;          //!< Section 'setMeshMaterial'
    std::string                   mat_name_generate; //!< Section 'generateMaterialShaders'
};

// -----------------------------------------------------------------------------
class ODefParser
{
public:
    void                       Prepare();
    bool                       ProcessLine(const char* line);
    void                       ProcessOgreStream(Ogre::DataStream* stream);
    std::shared_ptr<ODefDocument>  Finalize(); //!< Passes ownership

private:
    bool                       ProcessCurrentLine();
    void                       ResetCBoxContext();

    struct ODefParserContext
    {
        bool                 header_done;
        std::string          header_mesh_name;
        Ogre::Vector3        header_scale;
        // Collision boxes or meshes
        Ogre::Vector3        cbox_direction;
        bool                 cbox_is_rotating;
        bool                 cbox_is_virtual;
        bool                 cbox_force_cam;
        Ogre::Vector3        cbox_rotation;
        Ogre::Vector3        cbox_cam_pos; //!< Section 'forcecamera'
        CollisionEventFilter cbox_event_filter;
        std::string          cbox_event_name;
        std::string          cbox_mesh_name;
        std::string          cbox_reverb_preset_name;
        std::string          cbox_groundmodel_name;
        Ogre::Vector3        cbox_aabb_min;
        Ogre::Vector3        cbox_aabb_max;
    }                          m_ctx; //!< Parser context

    std::shared_ptr<ODefDocument>  m_def;
    int                        m_line_number;
    const char*                m_cur_line;
};

} // namespace RoR
