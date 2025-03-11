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

#include "ODefFileFormat.h"

#include "Actor.h"
#include "Utils.h"

using namespace RoR;
using namespace Ogre;

const int ODEF_LINE_BUF_LEN = 4000;

void ODefParser::ProcessOgreStream(Ogre::DataStream* stream)
{
    char raw_line_buf[ODEF_LINE_BUF_LEN];
    bool keep_reading = true;
    while (keep_reading && !stream->eof())
    {
        stream->readLine(raw_line_buf, ODEF_LINE_BUF_LEN);
        keep_reading = this->ProcessLine(raw_line_buf);
    }
}

bool ODefParser::ProcessLine(const char* line)
{
    bool result = true;
    if ((line != nullptr) && (line[0] != 0))
    {
        m_cur_line = line; // No trimming by design.
        result = this->ProcessCurrentLine();
    }
    m_line_number++;
    return result;
}

void ODefParser::Prepare()
{
    m_ctx.header_done = false;
    m_ctx.header_scale = Ogre::Vector3::ZERO;
    this->ResetCBoxContext();
    m_def = std::make_shared<ODefDocument>();
}

std::shared_ptr<ODefDocument> ODefParser::Finalize()
{
    std::shared_ptr<ODefDocument> def = m_def; // Pass ownership
    m_def.reset();
    return def;
}

inline bool StartsWith(std::string const & line, const char* test)
{
    return line.compare(0, strlen(test), test) == 0;
}

// retval true = continue processing (false = stop)
bool ODefParser::ProcessCurrentLine()
{
    if (!m_ctx.header_done)
    {
        if (strcmp(m_cur_line, "LOD") == 0) // Clone of old parser logic.
        {
            return true; // 'LOD line' = obsolete
        }

        if (m_ctx.header_mesh_name.empty())
        {
            m_ctx.header_mesh_name = m_cur_line; // No trimming by design
            return true;
        }

        sscanf(m_cur_line, "%f, %f, %f", &m_ctx.header_scale.x, &m_ctx.header_scale.y, &m_ctx.header_scale.z);

        m_def->header.mesh_name = m_ctx.header_mesh_name;
        m_def->header.scale = m_ctx.header_scale;
        m_ctx.header_done = true;
        return true;
    }

    std::string line_str = m_cur_line;
    Ogre::StringUtil::trim(line_str);
    line_str = SanitizeUtf8String(line_str);
    if ((line_str[0] == 0) || (line_str[0] == '/') || (line_str[0] == ';'))
    {
        return true;
    }

    if (line_str == "end")
    {
        return false;
    }

    if (line_str == "movable")
    {
        // Unused keyword
    }
    else if (line_str == "standard")
    {
        m_def->mode_standard = true;
    }
    else if (StartsWith(line_str, "localizer-"))
    {
        if (line_str.compare(10, 3, "vor") == 0) { m_def->localizers.push_back(LOCALIZER_VOR       ); return true; }
        if (line_str.compare(10, 3, "ndb") == 0) { m_def->localizers.push_back(LOCALIZER_NDB       ); return true; }
        if (line_str.compare(10, 1, "v"  ) == 0) { m_def->localizers.push_back(LOCALIZER_VERTICAL  ); return true; }
        if (line_str.compare(10, 1, "h"  ) == 0) { m_def->localizers.push_back(LOCALIZER_HORIZONTAL); return true; }

        LOG("[RoR|ODef] Invalid line: " + line_str);
    }
    else if (StartsWith(line_str, "sound"))
    {
        char tmp[201] = "";
        sscanf(line_str.c_str(), "sound %200s", tmp);
        m_def->sounds.push_back(tmp);
    }
    else if (StartsWith(line_str, "particleSystem"))
    {
        ODefParticleSys psys;
        char instance_name[201] = "";
        char template_name[201] = "";
        int res = sscanf(line_str.c_str(), "particleSystem %f, %f, %f, %f, %200s %200s", 
            &psys.scale, &psys.pos.x, &psys.pos.y, &psys.pos.z, instance_name, template_name);

        if (res == 6)
        {
            psys.instance_name = instance_name;
            psys.template_name = template_name;
            m_def->particle_systems.push_back(psys);
        }
    }
    else if (StartsWith(line_str, "setMeshMaterial") && line_str.length() > 16)
    {
        m_def->mat_name = line_str.substr(16); // Format: "setMeshMaterial %s"
    }
    else if (StartsWith(line_str, "generateMaterialShaders") && line_str.length() > 24)
    {
        m_def->mat_name_generate = line_str.substr(24); // Format: "generateMaterialShaders %s"
    }
    else if (StartsWith(line_str, "playanimation"))
    {
        ODefAnimation anim;
        char anim_name[201] = "";
        sscanf(line_str.c_str(), "playanimation %f, %f, %200s", &anim.speed_min, &anim.speed_max, anim_name);
        anim.name = anim_name;
        if (anim.name != "")
        {
            m_def->animations.push_back(anim);
        }
    }
    else if (StartsWith(line_str, "drawTextOnMeshTexture"))
    {
        ODefTexPrint tp;
        char font_name[201] = "";
        char text[501] = "";
        int res = sscanf(line_str.c_str(),
            "drawTextOnMeshTexture %f, %f, %f, %f, %f, %f, %f, %f, %c, %i, %i, %200s %500s", 
            &tp.x, &tp.y, &tp.w, &tp.h, &tp.r, &tp.g, &tp.b, &tp.a,
            &tp.option, &tp.font_size, &tp.font_dpi, font_name, text);

        if (res == 13)
        {
            tp.font_name = font_name;
            tp.text = text;
            m_def->texture_prints.push_back(tp);
        }
        else
            LOG("[RoR|ODef] Warning: invalid 'drawTextOnMeshTexture' line.");
    }
    else if (StartsWith(line_str, "spotlight"))
    {
        ODefSpotlight sl;
        int res = sscanf(line_str.c_str(), "spotlight %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f",
            &sl.pos.x, &sl.pos.y, &sl.pos.z, &sl.dir.x, &sl.dir.y, &sl.dir.z,
            &sl.color.r, &sl.color.g, &sl.color.b, &sl.range, &sl.angle_inner, &sl.angle_outer);
        if (res == 12)
        {
            m_def->spotlights.push_back(sl);
        }
        else
        {
            LOG("[RoR|ODef] Warning: invalid 'spotlight' line.");
        }
    }
    else if (StartsWith(line_str, "pointlight"))
    {
        ODefPointLight pl;
        int res = sscanf(line_str.c_str(), "pointlight %f, %f, %f, %f, %f, %f, %f, %f, %f, %f",
            &pl.pos.x, &pl.pos.y, &pl.pos.z, &pl.dir.x, &pl.dir.y, &pl.dir.z,
            &pl.color.r, &pl.color.g, &pl.color.b, &pl.range);
        if (res == 10)
        {
            m_def->point_lights.push_back(pl);
        }
        else
        {
            LOG("[RoR|ODef] Warning: invalid 'pointlight' line.");
        }
    }

    // Collision boxes or meshes
    else if ((line_str == "beginbox") || (line_str == "beginmesh"))
    {
        this->ResetCBoxContext();
    }
    else if (StartsWith(line_str, "boxcoords"))
    {
        sscanf(line_str.c_str(), "boxcoords %f, %f, %f, %f, %f, %f",
            &m_ctx.cbox_aabb_min.x, &m_ctx.cbox_aabb_max.x,
            &m_ctx.cbox_aabb_min.y, &m_ctx.cbox_aabb_max.y,
            &m_ctx.cbox_aabb_min.z, &m_ctx.cbox_aabb_max.z);
    }
    else if (StartsWith(line_str, "mesh"))
    {
        char tmp[200] = "";
        sscanf(line_str.c_str(), "mesh %200s", tmp);
        m_ctx.cbox_mesh_name = tmp;
    }
    else if (StartsWith(line_str, "rotate"))
    {
        sscanf(line_str.c_str(), "rotate %f, %f, %f",
            &m_ctx.cbox_rotation.x, &m_ctx.cbox_rotation.y, &m_ctx.cbox_rotation.z);
        m_ctx.cbox_is_rotating = true;
    }
    else if (StartsWith(line_str, "forcecamera"))
    {
        sscanf(line_str.c_str(), "forcecamera %f, %f, %f",
            &m_ctx.cbox_cam_pos.x, &m_ctx.cbox_cam_pos.y, &m_ctx.cbox_cam_pos.z);
        m_ctx.cbox_force_cam = true;
    }
    else if (StartsWith(line_str, "direction"))
    {
        sscanf(line_str.c_str(), "direction %f, %f, %f",
            &m_ctx.cbox_direction.x, &m_ctx.cbox_direction.y, &m_ctx.cbox_direction.z);
    }
    else if (StartsWith(line_str, "frictionconfig") && line_str.length() > 15)
    {
        m_def->groundmodel_files.push_back(line_str.substr(15));
    }
    else if ((StartsWith(line_str, "stdfriction") || StartsWith(line_str, "usefriction")) && line_str.length() > 12)
    {
        m_ctx.cbox_groundmodel_name = line_str.substr(12);
    }
    else if (line_str == "virtual")
    {
        m_ctx.cbox_is_virtual = true;
    }
    else if (StartsWith(line_str, "event"))
    {
        char ev_name[301] = "";
        char ev_type[301] = "";
        sscanf(line_str.c_str(), "event %300s %300s", ev_name, ev_type);
        m_ctx.cbox_event_name = ev_name;

             if (!strncmp(ev_type, "avatar",        6)) { m_ctx.cbox_event_filter = EVENT_AVATAR;       }
        else if (!strncmp(ev_type, "truck_wheels", 12)) { m_ctx.cbox_event_filter = EVENT_TRUCK_WHEELS; }
        else if (!strncmp(ev_type, "truck",         5)) { m_ctx.cbox_event_filter = EVENT_TRUCK;        }
        else if (!strncmp(ev_type, "airplane",      8)) { m_ctx.cbox_event_filter = EVENT_AIRPLANE;     }
        else if (!strncmp(ev_type, "boat",          4)) { m_ctx.cbox_event_filter = EVENT_BOAT;         }
        else                                            { m_ctx.cbox_event_filter = EVENT_ALL;          }

        // hack to avoid fps drops near spawnzones
        if (!strncmp(ev_name, "spawnzone", 9)) { m_ctx.cbox_event_filter = EVENT_AVATAR; }
    }
    else if (StartsWith(line_str, "reverb_preset"))
    {
        char tmp[200] = "";
        sscanf(line_str.c_str(), "reverb_preset %199s", tmp);
        m_ctx.cbox_reverb_preset_name = tmp;
    }
    else if (line_str == "endbox")
    {
        m_def->collision_boxes.emplace_back(
            m_ctx.cbox_aabb_min, m_ctx.cbox_aabb_max,
            m_ctx.cbox_rotation, m_ctx.cbox_cam_pos,
            m_ctx.cbox_direction, m_ctx.header_scale,
            m_ctx.cbox_reverb_preset_name,
            m_ctx.cbox_event_name, m_ctx.cbox_event_filter,
            m_ctx.cbox_is_rotating, m_ctx.cbox_is_virtual, m_ctx.cbox_force_cam);
    }
    else if (line_str == "endmesh")
    {
        m_def->collision_meshes.emplace_back(
            m_ctx.cbox_mesh_name, m_ctx.header_scale, m_ctx.cbox_groundmodel_name);
    }
    else if (line_str == "nocast")
    {
        m_def->header.cast_shadows = false;
    }

    return true;
}

void ODefParser::ResetCBoxContext()
{
    m_ctx.cbox_direction = Ogre::Vector3::ZERO;
    m_ctx.cbox_is_rotating = false;
    m_ctx.cbox_is_virtual = false;
    m_ctx.cbox_force_cam = false;
    m_ctx.cbox_event_filter = EVENT_NONE;
    m_ctx.cbox_event_name.clear();
    m_ctx.cbox_mesh_name.clear();
    m_ctx.cbox_reverb_preset_name.clear();
    m_ctx.cbox_groundmodel_name = "concrete";
}

