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

#include <OgreColourValue.h>
#include <OgreDataStream.h>
#include <OgreVector3.h>

#include <memory>

#ifdef DELETE // Windows hack: The `EventType::DELETE` name collides with `DELETE` from <winnt.h> which *MAY* be included at this point.
#    undef DELETE
#endif

namespace RoR {
namespace ODef {

    const int STR_LEN = 300;
    const int LINE_BUF_LEN = 4000;

    enum class Localizer
    {
        NONE,
        HORIZONTAL,
        VERTICAL,
        NDB,
        VOR,
    };

    enum class EventType
    {
        NONE,
        AVATAR,
        TRUCK,
        AIRPLANE,
        BOAT,
        DELETE,
    };

} // namespace ODef

struct ODefCollisionBox
{
    ODefCollisionBox(Ogre::Vector3 min, Ogre::Vector3 max, Ogre::Vector3 rot,
        Ogre::Vector3 campos, Ogre::Vector3 dir, Ogre::Vector3 scal,
        std::string ev_name, int ev_filter, bool is_rot, bool is_virt, bool forcecam):

        aabb_min(min), aabb_max(max), box_rot(rot), cam_pos(campos), direction(dir), scale(scal),
        event_name(ev_name), event_filter(ev_filter),
        is_rotating(is_rot), is_virtual(is_virt), force_cam_pos(forcecam)
    {}

    Ogre::Vector3 aabb_min;
    Ogre::Vector3 aabb_max;
    Ogre::Vector3 box_rot;
    Ogre::Vector3 cam_pos;
    Ogre::Vector3 direction;
    Ogre::Vector3 scale;
    std::string   event_name;
    int           event_filter;

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

    std::string   mesh_name;
    Ogre::Vector3 scale;
    std::string   groundmodel_name;
};

struct ODefParticleSys
{
    ODefParticleSys() { memset(this, 0, sizeof(ODefParticleSys)); }

    char           instance_name[ODef::STR_LEN];
    char           template_name[ODef::STR_LEN];
    Ogre::Vector3  pos;
    float          scale;
};

struct ODefAnimation
{
    ODefAnimation() { memset(this, 0, sizeof(ODefAnimation)); }

    float speed_min, speed_max;
    char  name[ODef::STR_LEN];
};

struct ODefTexPrint
{
    ODefTexPrint() { memset(this, 0, sizeof(ODefTexPrint)); }

    char    font_name[ODef::STR_LEN];
    int     font_size;
    int     font_dpi;
    char    text[ODef::STR_LEN];
    char    option;

    float   x,  y,  w,  h;
    float   a,  r,  g,  b;
};

struct ODefSpotlight
{
    ODefSpotlight() { memset(this, 0, sizeof(ODefSpotlight)); }

    Ogre::Vector3      pos;
    Ogre::Vector3      dir;
    float              range;
    float              angle_inner; ///< Degrees
    float              angle_outer; ///< Degrees
    Ogre::ColourValue  color;
    char               name[ODef::STR_LEN];
};

struct ODefPointLight
{
    ODefPointLight() { memset(this, 0, sizeof(ODefPointLight)); }

    Ogre::Vector3      pos;
    Ogre::Vector3      dir;
    float              range;
    Ogre::ColourValue  color;
    char               name[ODef::STR_LEN];
};

struct ODefFile
{
    ODefFile():
        mode_standard(false)
    {}

    struct ODefFileHeader
    {
        std::string   mesh_name;
        std::string   entity_name;
        Ogre::Vector3 scale;
    }                             header;

    bool                          mode_standard;
    std::vector<ODef::Localizer>  localizers;
    std::list<std::string>        sounds;
    std::list<std::string>        groundmodel_files;
    std::list<ODefCollisionBox>   collision_boxes;
    std::list<ODefCollisionMesh>  collision_meshes;
    std::list<ODefParticleSys>    particle_systems;
    std::list<ODefAnimation>      animations;
    std::list<ODefTexPrint>       texture_prints;    ///< Section 'drawTextOnMeshTexture'
    std::list<ODefSpotlight>      spotlights;
    std::list<ODefPointLight>     point_lights;
    std::string                   mat_name;          ///< Section 'setMeshMaterial'
    std::string                   mat_name_generate; ///< Section 'generateMaterialShaders'
};

// -----------------------------------------------------------------------------
class ODefParser
{
public:
    void                       Prepare();
    bool                       ProcessLine(const char* line);
    void                       ProcessOgreStream(Ogre::DataStream* stream);
    std::shared_ptr<ODefFile>  Finalize(); ///< Passes ownership

private:
    bool                       ProcessCurrentLine();
    void                       ResetCBoxContext();

    struct ODefParserContext
    {
        bool                header_done;
        std::string         header_mesh_name;
        Ogre::Vector3       header_scale;
        // Collision boxes or meshes
        Ogre::Vector3       cbox_direction;
        bool                cbox_is_rotating;
        bool                cbox_is_virtual;
        bool                cbox_force_cam;
        Ogre::Vector3       cbox_rotation;
        Ogre::Vector3       cbox_cam_pos; ///< Section 'forcecamera'
        ODef::EventType     cbox_event_filter;
        std::string         cbox_event_name;
        std::string         cbox_mesh_name;
        std::string         cbox_groundmodel_name;
        Ogre::Vector3       cbox_aabb_min;
        Ogre::Vector3       cbox_aabb_max;
    }                          m_ctx; ///< Parser context

    std::shared_ptr<ODefFile>  m_def;
    int                        m_line_number;
    const char*                m_cur_line;
};

} // namespace RoR
