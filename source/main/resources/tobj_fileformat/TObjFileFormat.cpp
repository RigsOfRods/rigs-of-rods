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

#include "TObjFileFormat.h"

#include "ProceduralRoad.h"

#define LOGSTREAM Ogre::LogManager::getSingleton().stream() << "[RoR|TObj fileformat] "

using namespace RoR;
using namespace Ogre;

// --------------------------------
// TObjEntry

TObjEntry::TObjEntry(Ogre::Vector3 pos, Ogre::Vector3 rot, const char* odef, TObj::SpecialObject spc, const char* ty, const char* nam):
    position(pos),
    rotation(rot),
    special(spc)
{
    strcpy(type, ty);
    strcpy(instance_name, nam);
    strcpy(odef_name, odef);
}

bool TObjEntry::IsRoad() const
{
    return (special >= TObj::SpecialObject::ROAD) && (special <= TObj::SpecialObject::ROAD_BRIDGE);
}

bool TObjEntry::IsActor() const
{
    return (special == TObj::SpecialObject::TRUCK)   || (special == TObj::SpecialObject::LOAD) ||
           (special == TObj::SpecialObject::MACHINE) || (special == TObj::SpecialObject::BOAT) ||
           (special == TObj::SpecialObject::TRUCK2);
}

// --------------------------------
// Parser

void TObjParser::Prepare()
{
    m_cur_line           = nullptr;
    m_line_number        = 0;
    m_in_procedural_road = false;
    m_in_procedural_road = false;
    m_road2_use_old_mode = false;
    
    m_def = std::shared_ptr<TObjFile>(new TObjFile());
}

bool TObjParser::ProcessLine(const char* line)
{
    bool result = true;
    if ((line != nullptr) && (line[0] != 0) && (line[0] != '/') && (line[0] != ';'))
    {
        m_cur_line = line; // No trimming by design.
        result = this->ProcessCurrentLine();
    }
    m_line_number++;
    return result;
}

// retval true = continue processing (false = stop)
bool TObjParser::ProcessCurrentLine()
{
    // ** Process keywords

    if (!strcmp(m_cur_line, "end"))
    {
        return false;
    }
    if (strncmp(m_cur_line, "collision-tris", 14) == 0)
    {
        return true; // Obsolete - ignore it.
    }
    if (strncmp(m_cur_line, "grid", 4) == 0)
    {
        this->ProcessGridLine();
        return true;
    }
    if (strncmp(m_cur_line, "trees", 5) == 0)
    {
        this->ProcessTreesLine();
        return true;
    }
    if (strncmp(m_cur_line, "grass", 5) == 0)
    {
        this->ProcessGrassLine();
        return true;
    }
    if (strncmp("begin_procedural_roads", m_cur_line, 22) == 0)
    {
        m_cur_procedural_obj = ProceduralObject(); // Hard reset, discarding last "non-procedural" road strip. For backwards compatibility. ~ Petr Ohlidal, 08/2020
        m_in_procedural_road = true;
        m_road2_use_old_mode = true;
        return true;
    }
    else if (strncmp("end_procedural_roads", m_cur_line, 20) == 0)
    {
        if (m_road2_use_old_mode)
        {
            m_def->proc_objects.push_back(m_cur_procedural_obj);
            m_cur_procedural_obj = ProceduralObject();
        }
        m_in_procedural_road = false;
        return true;
    }

    // ** Process entries (ODEF or special objects)

    if (m_in_procedural_road)
    {
        if (m_road2_use_old_mode)
        {
            this->ProcessProceduralLine();
        }
    }
    else
    {
        TObjEntry object;
        if (this->ParseObjectLine(object))
        {
            if (object.IsActor())
            {
                this->ProcessActorObject(object);
            }
            else if (object.IsRoad())
            {
                this->ProcessRoadObject(object);
            }
            else
            {
                m_def->objects.push_back(object);
            }
        }
    }
    return true;
}

std::shared_ptr<TObjFile> TObjParser::Finalize()
{
    // finish the last road
    if (m_road2_use_old_mode != 0)
    {
        Vector3 pp_pos = m_road2_last_pos + m_road2_last_rot * Vector3(10.0f, 0.0f, 0.9f);
        this->ImportProceduralPoint(pp_pos, m_road2_last_rot, TObj::SpecialObject::ROAD);

        m_def->proc_objects.push_back(m_cur_procedural_obj);
    }

    std::shared_ptr<TObjFile> tmp_def = m_def;
    m_def.reset();
    return tmp_def; // Pass ownership
}

void TObjParser::ProcessOgreStream(Ogre::DataStream* stream)
{
    char raw_line_buf[TObj::LINE_BUF_LEN];
    bool keep_reading = true;
    while (keep_reading && !stream->eof())
    {
        stream->readLine(raw_line_buf, TObj::LINE_BUF_LEN);
        keep_reading = this->ProcessLine(raw_line_buf);
    }
}

// --------------------------------
// Processing

void TObjParser::ProcessProceduralLine()
{
    ProceduralPoint point;
    Str<300> obj_name;
    Ogre::Vector3 rot = Ogre::Vector3::ZERO;
    sscanf(m_cur_line, "%f, %f, %f, %f, %f, %f, %f, %f, %f, %s",
        &point.position.x, &point.position.y,  &point.position.z,
        &rot.x, &rot.y, &rot.z,
        &point.width, &point.bwidth, &point.bheight, obj_name.GetBuffer());

    point.rotation = this->CalcRotation(rot);

         if (obj_name == "flat"             ) { point.type = ProceduralRoad::ROAD_FLAT;  }
    else if (obj_name == "left"             ) { point.type = ProceduralRoad::ROAD_LEFT;  }
    else if (obj_name == "right"            ) { point.type = ProceduralRoad::ROAD_RIGHT; }
    else if (obj_name == "both"             ) { point.type = ProceduralRoad::ROAD_BOTH;  }
    else if (obj_name == "bridge"           ) { point.type = ProceduralRoad::ROAD_BRIDGE;    point.pillartype = 1; }
    else if (obj_name == "monorail"         ) { point.type = ProceduralRoad::ROAD_MONORAIL;  point.pillartype = 2; }
    else if (obj_name == "monorail2"        ) { point.type = ProceduralRoad::ROAD_MONORAIL;  point.pillartype = 0; }
    else if (obj_name == "bridge_no_pillars") { point.type = ProceduralRoad::ROAD_BRIDGE;    point.pillartype = 0; }
    else                                      { point.type = ProceduralRoad::ROAD_AUTOMATIC; point.pillartype = 0; }

    m_cur_procedural_obj.points.push_back(point);
}

void TObjParser::ProcessGridLine()
{
    Ogre::Vector3 & pos = m_def->grid_position;
    sscanf(m_cur_line, "grid %f, %f, %f", &pos.x, &pos.y, &pos.z); // No error check by design
    m_def->grid_enabled = true;
}

void TObjParser::ProcessTreesLine()
{
    TObjTree tree;
    sscanf(m_cur_line, "trees %f, %f, %f, %f, %f, %f, %f, %s %s %s %f %s",
        &tree.yaw_from,      &tree.yaw_to,
        &tree.scale_from,    &tree.scale_to,
        &tree.high_density,
        &tree.min_distance,  &tree.max_distance,
         tree.tree_mesh,      tree.color_map,         tree.density_map,
        &tree.grid_spacing,   tree.collision_mesh);

    m_def->trees.push_back(tree);
}

void TObjParser::ProcessGrassLine()
{
    TObjGrass grass;
    if (strncmp(m_cur_line, "grass2", 6) == 0)
    {
        sscanf(m_cur_line, "grass2 %d, %f, %f, %f, %f, %f, %f, %f, %f, %d, %f, %f, %d, %s %s %s",
            &grass.range,
            &grass.sway_speed,   &grass.sway_length, &grass.sway_distrib, &grass.density,
            &grass.min_x,        &grass.min_y,       &grass.max_x,        &grass.max_y,
            &grass.grow_techniq, &grass.min_h,       &grass.max_h,        &grass.technique,
                grass.material_name,
                grass.color_map_filename,
                grass.density_map_filename);
    }
    else
    {
        // Same as 'grass2', except without 'technique' parameter
        sscanf(m_cur_line, "grass %d, %f, %f, %f, %f, %f, %f, %f, %f, %d, %f, %f, %s %s %s",
            &grass.range,
            &grass.sway_speed,   &grass.sway_length, &grass.sway_distrib, &grass.density,
            &grass.min_x,        &grass.min_y,       &grass.max_x,        &grass.max_y,
            &grass.grow_techniq, &grass.min_h,       &grass.max_h,
                grass.material_name,
                grass.color_map_filename,
                grass.density_map_filename);
    }

    // 0: GRASSTECH_QUAD;       // Grass constructed of randomly placed and rotated quads
    // 1: GRASSTECH_CROSSQUADS; // Grass constructed of two quads forming a "X" cross shape
    // 2: GRASSTECH_SPRITE;     // Grass constructed of camera-facing billboard quads
    if ((grass.technique < 0) || (grass.technique > 2))
    {
        LOGSTREAM << "Invalid parameter 'technique': '" << grass.technique << "', falling back to default '1: GRASSTECH_CROSSQUADS'";
        grass.technique = 1;
    }

    m_def->grass.push_back(grass);
}

void TObjParser::ProcessActorObject(const TObjEntry& object)
{
    TObjVehicle v;
    v.position = object.position;
    v.rotation = this->CalcRotation(object.rotation);
    v.type = object.special;
    strcpy(v.name, object.type);

    m_def->vehicles.push_back(v);
}

void TObjParser::ProcessRoadObject(const TObjEntry& object)
{
    // ** Import road objects as procedural road

    if (object.position.distance(m_road2_last_pos) > 20.0f)
    {
        // break the road
        if (m_road2_use_old_mode)
        {
            Vector3 pp_pos = m_road2_last_pos + this->CalcRotation(m_road2_last_rot) * Vector3(10.0f, 0.0f, 0.9f);
            this->ImportProceduralPoint(pp_pos, m_road2_last_rot, object.special);

            // finish it and start new object
            m_def->proc_objects.push_back(m_cur_procedural_obj);
            m_cur_procedural_obj = ProceduralObject();
        }
        m_road2_use_old_mode = true;

        // beginning of new
        this->ImportProceduralPoint(object.position, object.rotation, object.special);
    }
    else
    {
        this->ImportProceduralPoint(object.position, object.rotation, object.special);
    }
    m_road2_last_pos=object.position;
    m_road2_last_rot=object.rotation;
}

// --------------------------------
// Helpers

void TObjParser::ImportProceduralPoint(Ogre::Vector3 const& pos, Ogre::Vector3 const& rot, TObj::SpecialObject special)
{
    ProceduralPoint pp;
    pp.bheight    = 0.2;
    pp.bwidth     = 1.4;
    pp.pillartype = (int)(special != TObj::SpecialObject::ROAD_BRIDGE_NO_PILLARS);
    pp.position   = pos;
    pp.rotation   = this->CalcRotation(rot);
    pp.type       = (special == TObj::SpecialObject::ROAD) ? ProceduralRoad::ROAD_FLAT : ProceduralRoad::ROAD_AUTOMATIC;
    pp.width      = 8;

    m_cur_procedural_obj.points.push_back(pp);
}

Ogre::Quaternion TObjParser::CalcRotation(Ogre::Vector3 const& rot) const
{
    return Quaternion(Degree(rot.x), Vector3::UNIT_X) *
           Quaternion(Degree(rot.y), Vector3::UNIT_Y) *
           Quaternion(Degree(rot.z), Vector3::UNIT_Z);
}

bool TObjParser::ParseObjectLine(TObjEntry& object)
{
    Str<300> odef("generic");
    char type[100] = {};
    char name[100] = {};
    Ogre::Vector3 pos(Ogre::Vector3::ZERO);
    Ogre::Vector3 rot(Ogre::Vector3::ZERO);
    int r = sscanf(m_cur_line, "%f, %f, %f, %f, %f, %f, %s %s %s",
        &pos.x, &pos.y, &pos.z, &rot.x, &rot.y, &rot.z, odef.GetBuffer(), type, name);
    if (r < 6)
    {
        return false;
    }

    TObj::SpecialObject special = TObj::SpecialObject::NONE;
         if (odef == "truck"             ) { special = TObj::SpecialObject::TRUCK                 ; }
    else if (odef == "load"              ) { special = TObj::SpecialObject::LOAD                  ; }
    else if (odef == "machine"           ) { special = TObj::SpecialObject::MACHINE               ; }
    else if (odef == "boat"              ) { special = TObj::SpecialObject::BOAT                  ; }
    else if (odef == "truck2"            ) { special = TObj::SpecialObject::TRUCK2                ; }
    else if (odef == "grid"              ) { special = TObj::SpecialObject::GRID                  ; }
    else if (odef == "road"              ) { special = TObj::SpecialObject::ROAD                  ; }
    else if (odef == "roadborderleft"    ) { special = TObj::SpecialObject::ROAD_BORDER_LEFT      ; }
    else if (odef == "roadborderright"   ) { special = TObj::SpecialObject::ROAD_BORDER_RIGHT     ; }
    else if (odef == "roadborderboth"    ) { special = TObj::SpecialObject::ROAD_BORDER_BOTH      ; }
    else if (odef == "roadbridgenopillar") { special = TObj::SpecialObject::ROAD_BRIDGE_NO_PILLARS; }
    else if (odef == "roadbridge"        ) { special = TObj::SpecialObject::ROAD_BRIDGE           ; }

    object = TObjEntry(pos, rot, odef.ToCStr(), special, type, name);
    return true;
}
