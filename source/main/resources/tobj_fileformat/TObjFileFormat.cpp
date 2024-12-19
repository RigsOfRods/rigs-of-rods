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

#include "Actor.h"
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
    m_cur_procedural_obj = new ProceduralObject();
    m_cur_procedural_obj_start_line = -1;
    m_road2_num_blocks   = 0;
    m_default_rendering_distance = 0.f;
    m_rot_yxz            = false;
    
    m_def = TObjDocumentPtr(new TObjDocument());
}

bool TObjParser::ProcessLine(const char* line)
{
    bool result = true;
    if ((line != nullptr) && (line[0] != 0))
    {
        bool is_comment = (line[0] == '/') || (line[0] == ';');
        if (is_comment)
        {
            int text_start = 1;
            while (line[text_start] == '/')
            {
                text_start++;
            }
            m_preceding_line_comments += std::string(line + text_start) + "\n";
        }
        else
        {
            m_cur_line = line; // No trimming by design.
            m_cur_line_trimmed = line;
            while (m_cur_line_trimmed[0] == ' ' || m_cur_line_trimmed[0] == '\t')
            {
                m_cur_line_trimmed++;
            }
            result = this->ProcessCurrentLine();
            m_preceding_line_comments = "";
        }
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
    if (strncmp(m_cur_line, "set_default_rendering_distance", 30) == 0)
    {
        const int result = sscanf(m_cur_line, "set_default_rendering_distance %f", &m_default_rendering_distance);
        if (result != 1)
        {
            LOG(fmt::format("too few parameters on line: '{}' ({}, line {})", m_cur_line, m_filename, m_line_number));
        }
        return true;
    }
    if (strncmp(m_cur_line, "rot_yxz", 7) == 0)
    {
        m_rot_yxz = true;
        return true;
    }
    if (strncmp("begin_procedural_roads", m_cur_line, 22) == 0)
    {
        m_cur_procedural_obj = new ProceduralObject(); // Hard reset, discarding last "non-procedural" road strip. For backwards compatibility. ~ Petr Ohlidal, 08/2020
        m_in_procedural_road = true;
        m_cur_procedural_obj_start_line = m_line_number;
        return true;
    }
    if (strncmp("end_procedural_roads", m_cur_line, 20) == 0)
    {
        if (m_in_procedural_road)
        {
            this->FlushProceduralObject();
        }
        m_in_procedural_road = false;
        return true;
    }
    if (strncmp("smoothing_num_splits", m_cur_line_trimmed, 20) == 0)
    {
        if (m_in_procedural_road)
        {
            int result = sscanf(m_cur_line_trimmed, "smoothing_num_splits %d", &m_cur_procedural_obj->smoothing_num_splits);
            if (result != 1)
            {
                LOG(fmt::format("[RoR|TObj] not enough parameters at line '{}' ({}, line {})", m_cur_line, m_filename, m_line_number));
            }
        }
        return true;
    }
    if (strncmp("collision_enabled", m_cur_line_trimmed, 17) == 0)
    {
        if (m_in_procedural_road)
        {
            const char* value = m_cur_line_trimmed + 17; // C pointer arithmetic
            m_cur_procedural_obj->collision_enabled = Ogre::StringConverter::parseBool(value, false);
        }
        return true;
    }

    // ** Process entries (ODEF or special objects)

    if (m_in_procedural_road)
    {
        this->ProcessProceduralLine();
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

TObjDocumentPtr TObjParser::Finalize()
{
    // finish the last road
    if (m_road2_num_blocks > 0)
    {
        Vector3 pp_pos = m_road2_last_pos + m_road2_last_rot * Vector3(10.0f, 0.0f, 0.9f);
        this->ImportProceduralPoint(pp_pos, m_road2_last_rot, TObj::SpecialObject::ROAD);

        this->FlushProceduralObject();
    }

    m_def->document_name = m_filename;
    m_filename = "";

    TObjDocumentPtr tmp_def = m_def;
    m_def.reset();
    return tmp_def; // Pass ownership
}

void TObjParser::ProcessOgreStream(Ogre::DataStream* stream)
{
    m_filename = stream->getName();
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

         if (obj_name == "flat"             ) { point.type = RoadType::ROAD_FLAT;  }
    else if (obj_name == "left"             ) { point.type = RoadType::ROAD_LEFT;  }
    else if (obj_name == "right"            ) { point.type = RoadType::ROAD_RIGHT; }
    else if (obj_name == "both"             ) { point.type = RoadType::ROAD_BOTH;  }
    else if (obj_name == "bridge"           ) { point.type = RoadType::ROAD_BRIDGE;    point.pillartype = 1; }
    else if (obj_name == "monorail"         ) { point.type = RoadType::ROAD_MONORAIL;  point.pillartype = 2; }
    else if (obj_name == "monorail2"        ) { point.type = RoadType::ROAD_MONORAIL;  point.pillartype = 0; }
    else if (obj_name == "bridge_no_pillars") { point.type = RoadType::ROAD_BRIDGE;    point.pillartype = 0; }
    else                                      { point.type = RoadType::ROAD_AUTOMATIC; point.pillartype = 1; }

    // Attach comments
    point.comments = m_preceding_line_comments;

    m_cur_procedural_obj->points.push_back(new ProceduralPoint(point));
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
        if (m_road2_num_blocks > 0)
        {
            Vector3 pp_pos = m_road2_last_pos + this->CalcRotation(m_road2_last_rot) * Vector3(10.0f, 0.0f, 0.9f);
            this->ImportProceduralPoint(pp_pos, m_road2_last_rot, object.special);
            this->FlushProceduralObject();
        }
        m_road2_num_blocks++;

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
    pp.type       = (special == TObj::SpecialObject::ROAD) ? RoadType::ROAD_FLAT : RoadType::ROAD_AUTOMATIC;
    pp.width      = 8;

    // Attach comments
    pp.comments = m_preceding_line_comments;

    m_cur_procedural_obj->points.push_back(new ProceduralPoint(pp));
    if (m_cur_procedural_obj_start_line == -1)
    {
        m_cur_procedural_obj_start_line = m_line_number;
    }
}

Ogre::Quaternion TObjParser::CalcRotation(Ogre::Vector3 const& rot) const
{
    if (m_rot_yxz)
    {
        return Quaternion(Degree(rot.y), Vector3::UNIT_Y) *  // y global
               Quaternion(Degree(rot.x), Vector3::UNIT_X) *  // x local
               Quaternion(Degree(rot.z), Vector3::UNIT_Z);   // z local
    }
    else
    {
        return Quaternion(Degree(rot.x), Vector3::UNIT_X) *
               Quaternion(Degree(rot.y), Vector3::UNIT_Y) *
               Quaternion(Degree(rot.z), Vector3::UNIT_Z);
    }
}

bool TObjParser::ParseObjectLine(TObjEntry& object)
{
    Str<TObj::STR_LEN> odef("generic");
    Str<TObj::STR_LEN> type("");
    Str<TObj::STR_LEN> instance_name("");
    Ogre::Vector3 pos(Ogre::Vector3::ZERO);
    Ogre::Vector3 rot(Ogre::Vector3::ZERO);
    int r = sscanf(m_cur_line, "%f, %f, %f, %f, %f, %f, %s %s %s",
        &pos.x, &pos.y, &pos.z, &rot.x, &rot.y, &rot.z, odef.GetBuffer(), type.GetBuffer(), instance_name.GetBuffer());
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

    // If no instance name given, generate one, so that scripts can use `game.destroyObject(instanceName)` even for pre-placed objects.
    // Don't use spaces because TOBJ parser doesn't support "" yet (game tries to strip the 'auto^' instancenames on save, but someone could export via custom script).
    if (instance_name == "")
    {
        instance_name << "auto^" << m_filename << "(line:" << m_line_number << ")";
        // Also set 'type' arg to non-empty (same reason).
        if (special == TObj::SpecialObject::NONE)
        {
            type << "-";
        }
    }

    object = TObjEntry(pos, rot, odef.ToCStr(), special, type, instance_name);
    object.rendering_distance = m_default_rendering_distance;

    // Attach comments
    object.comments = m_preceding_line_comments;

    return true;
}

void TObjParser::FlushProceduralObject()
{
    // finish it and start new object
    m_cur_procedural_obj->name = fmt::format("{} (lines {} - {})", m_filename, m_cur_procedural_obj_start_line, m_line_number);
    m_def->proc_objects.push_back(m_cur_procedural_obj);
    m_cur_procedural_obj = new ProceduralObject();
    m_cur_procedural_obj_start_line = -1;
    m_road2_num_blocks = 0;
}

const char* TObj::SpecialObjectToString(SpecialObject val)
{
    switch (val)
    {
    case TObj::SpecialObject::TRUCK                 : return "truck";
    case TObj::SpecialObject::LOAD                  : return "load";
    case TObj::SpecialObject::MACHINE               : return "machine";
    case TObj::SpecialObject::BOAT                  : return "boat";
    case TObj::SpecialObject::TRUCK2                : return "truck2";
    case TObj::SpecialObject::GRID                  : return "grid";
    case TObj::SpecialObject::ROAD                  : return "road";
    case TObj::SpecialObject::ROAD_BORDER_LEFT      : return "roadborderleft";
    case TObj::SpecialObject::ROAD_BORDER_RIGHT     : return "roadborderright";
    case TObj::SpecialObject::ROAD_BORDER_BOTH      : return "roadborderboth";
    case TObj::SpecialObject::ROAD_BRIDGE_NO_PILLARS: return "roadbridgenopillar";
    case TObj::SpecialObject::ROAD_BRIDGE           : return "roadbridge";
    default: "";
    }
}

void WriteTObjDelimiter(Ogre::DataStreamPtr& stream, const std::string& title, size_t count)
{
    if (count > 0)
    {
        std::string line = fmt::format("\n\n//    ~~~~~~~~~~    {} ({})    ~~~~~~~~~~\n\n", title, count);
        stream->write(line.c_str(), line.length());
    }
}

void TObj::WriteToStream(TObjDocumentPtr doc, Ogre::DataStreamPtr stream)
{
    // assert on Debug, play safe on Release
    ROR_ASSERT(doc);
    ROR_ASSERT(stream);
    if (!doc || !stream)
    {
        return;
    }

    // 'grid'
    WriteTObjDelimiter(stream, "grid", (int)doc->grid_enabled);
    if (doc->grid_enabled)
    {
        std::string line = fmt::format("grid {}, {}, {}\n");
        stream->write(line.c_str(), line.length());
    }

    // 'trees'
    WriteTObjDelimiter(stream, "trees", doc->trees.size());
    for (TObjTree& tree : doc->trees)
    {
        std::string line = fmt::format("trees {:9f}, {:9f}, {:9f}, {:9f}, {:9f}, {:9f}, {:9f}, {} {} {} {:9f} {}\n",
            tree.yaw_from, tree.yaw_to,
            tree.scale_from, tree.scale_to,
            tree.high_density,
            tree.min_distance, tree.max_distance,
            tree.tree_mesh, tree.color_map, tree.density_map,
            tree.grid_spacing, tree.collision_mesh);
        stream->write(line.c_str(), line.length());
    }

    // 'grass2' (incudes 'grass' elements)
    WriteTObjDelimiter(stream, "grass", doc->grass.size());
    for (TObjGrass& grass : doc->grass)
    {
        std::string line = fmt::format("grass2 {}, {:9f}, {:9f}, {:9f}, {:9f}, {:9f}, {:9f}, {:9f}, {:9f}, {}, {:9f}, {:9f}, {}, {} {} {}\n",
            grass.range,
            grass.sway_speed, grass.sway_length, grass.sway_distrib, grass.density,
            grass.min_x, grass.min_y, grass.max_x, grass.max_y,
            grass.grow_techniq, grass.min_h, grass.max_h, grass.technique,
            grass.material_name,
            grass.color_map_filename,
            grass.density_map_filename);
        stream->write(line.c_str(), line.length());
    }

    // vehicles ('truck', 'truck2', 'boat', 'load', 'machine')
    WriteTObjDelimiter(stream, "vehicles/loads/machines", doc->vehicles.size());
    for (TObjVehicle& vehicle : doc->vehicles)
    {
        // Handle preceding comments
        if (vehicle.comments != "")
        {
            for (Ogre::String& commenttext : Ogre::StringUtil::split(vehicle.comments, "\n"))
            {
                std::string commentline = fmt::format("// {}\n", commenttext);
                stream->write(commentline.c_str(), commentline.length());
            }
        }

        std::string line = fmt::format("{:9f}, {:9f}, {:9f}, {:9f}, {:9f}, {:9f}, {} {}\n",
            vehicle.position.x, vehicle.position.y, vehicle.position.z,
            vehicle.rotation.getRoll().valueDegrees(), vehicle.rotation.getYaw().valueDegrees(), vehicle.rotation.getPitch().valueDegrees(),
            SpecialObjectToString(vehicle.type), (const char*)vehicle.name);
        stream->write(line.c_str(), line.length());
    }

    // procedural objects
    WriteTObjDelimiter(stream, "roads", doc->proc_objects.size());
    for (ProceduralObjectPtr& procobj : doc->proc_objects)
    {
        std::string bline = "begin_procedural_roads\n";
        stream->write(bline.c_str(), bline.length());

        std::string sline = fmt::format("    smoothing_num_splits {}\n", procobj->smoothing_num_splits);
        stream->write(sline.c_str(), sline.length());

        std::string cline = fmt::format("    collision_enabled {}\n", procobj->collision_enabled);
        stream->write(cline.c_str(), cline.length());

        for (ProceduralPointPtr& point : procobj->points)
        {
            std::string type_str;
            switch (point->type)
            {
            case RoadType::ROAD_AUTOMATIC: type_str = "auto"; break;
            case RoadType::ROAD_FLAT: type_str = "flat"; break;
            case RoadType::ROAD_LEFT: type_str = "left"; break;
            case RoadType::ROAD_RIGHT: type_str = "right"; break;
            case RoadType::ROAD_BOTH: type_str = "both"; break;
            case RoadType::ROAD_BRIDGE: type_str = (point->pillartype == 1) ? "bridge" : "bridge_no_pillars"; break;
            case RoadType::ROAD_MONORAIL: type_str = (point->pillartype == 2) ? "monorail" : "monorail2"; break;
            }

            // Handle preceding comments
            if (point->comments != "")
            {
                for (Ogre::String& commenttext : Ogre::StringUtil::split(point->comments, "\n"))
                {
                    std::string commentline = fmt::format("// {}\n", commenttext);
                    stream->write(commentline.c_str(), commentline.length());
                }
            }

            std::string line = fmt::format(
                "\t{:13f}, {:13f}, {:13f}, 0, {:13f}, 0, {:13f}, {:13f}, {:13f}, {}\n",
                point->position.x, point->position.y, point->position.z,
                point->rotation.getYaw(false).valueDegrees(),
                point->width, point->bwidth, point->bheight, type_str);
            stream->write(line.c_str(), line.length());
        }

        std::string eline = "end_procedural_roads\n";
        stream->write(eline.c_str(), eline.length());
    }

    // static objects
    WriteTObjDelimiter(stream, "static objects", doc->objects.size());
    for (TObjEntry& entry : doc->objects)
    {
        // Handle preceding comments
        if (entry.comments != "")
        {
            for (Ogre::String& commenttext : Ogre::StringUtil::split(entry.comments, "\n"))
            {
                std::string commentline = fmt::format("// {}\n", commenttext);
                stream->write(commentline.c_str(), commentline.length());
            }
        }

        // Don't save autogenerated instance names
        std::string valid_instance_name;
        if (strncmp(entry.instance_name, "auto^", 5) != 0)
        {
            valid_instance_name = entry.instance_name;
        }

        std::string line = fmt::format("{:8.3f}, {:8.3f}, {:8.3f}, {:9f}, {:9f}, {:9f}, {} {} {}\n",
            entry.position.x, entry.position.y, entry.position.z,
            entry.rotation.x, entry.rotation.y, entry.rotation.z,
            entry.odef_name, entry.type, valid_instance_name);
        stream->write(line.c_str(), line.length());
    }
}

