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

/// @file   
/// @brief  Parser and data structures for TOBJ (Terrain Objects) file format.
/// @author Petr Ohlidal, 11/2016

#include "Collisions.h"
#include "ProceduralManager.h"

#include <Ogre.h>

#include <memory>

namespace RoR  {

namespace TObj {

    const int STR_LEN = 300;
    const int LINE_BUF_LEN = 4000;

    enum class SpecialObject
    {
        NONE,
        TRUCK,
        LOAD,
        MACHINE,
        BOAT,
        TRUCK2,
        GRID,
        // Road types
        ROAD,
        ROAD_BORDER_LEFT,
        ROAD_BORDER_RIGHT,
        ROAD_BORDER_BOTH,
        ROAD_BRIDGE_NO_PILLARS,
        ROAD_BRIDGE,
    };

} // namespace TObj

// -----------------------------------------------------------------------------
struct TObjTree
{
    TObjTree():
        yaw_from(0.f),            yaw_to(0.f),
        scale_from(0.f),          scale_to(0.f),
        min_distance(90),         max_distance(700),
        high_density(1.f),
        grid_spacing(0.f)
    {
        tree_mesh     [0] = '\0';
        color_map     [0] = '\0';
        density_map   [0] = '\0';
        collision_mesh[0] = '\0';
    }

    float    yaw_from,        yaw_to;
    float    scale_from,      scale_to;
    float    min_distance,    max_distance;
    float    high_density;
    float    grid_spacing;
    
    char     tree_mesh[TObj::STR_LEN];
    char     color_map[TObj::STR_LEN];
    char     density_map[TObj::STR_LEN];
    char     collision_mesh[TObj::STR_LEN];
};

// -----------------------------------------------------------------------------
/// Unified 'grass' and 'grass2'
struct TObjGrass
{
    TObjGrass():
        range(80),
        technique(1), // GRASSTECH_CROSSQUADS
        grow_techniq(0),
        sway_speed(0.5f),
        sway_length(0.05f),
        sway_distrib(10.f),

        min_x(0.2f),   min_y(0.2f),   min_h(-9999.f),
        max_x(1.0f),   max_y(0.6f),   max_h(+9999.f)
    {
        material_name        [0] = '\0';
        color_map_filename   [0] = '\0';
        density_map_filename [0] = '\0';
    }

    int      range;
    int      technique;
    int      grow_techniq;
    float    sway_speed;
    float    sway_length;
    float    sway_distrib;
    float    density;

    float    min_x,   min_y,   min_h;
    float    max_x,   max_y,   max_h;

    char     material_name[TObj::STR_LEN];
    char     color_map_filename[TObj::STR_LEN];
    char     density_map_filename[TObj::STR_LEN];
};

// -----------------------------------------------------------------------------
struct TObjVehicle
{
    Ogre::Vector3       position;
    Ogre::Quaternion    rotation;
    char                name[TObj::STR_LEN];
    TObj::SpecialObject type;
};

// -----------------------------------------------------------------------------
struct TObjEntry
{
    TObjEntry() {};
    TObjEntry(
        Ogre::Vector3 pos, Ogre::Vector3 rot, const char* instance_name,
        TObj::SpecialObject special, const char* type, const char* name);

    bool IsActor() const;
    bool IsRoad() const;

    Ogre::Vector3        position                     = Ogre::Vector3::ZERO;
    Ogre::Vector3        rotation                     = Ogre::Vector3::ZERO;
    TObj::SpecialObject  special                      = TObj::SpecialObject::NONE;
    char                 type[TObj::STR_LEN]          = {};
    char                 instance_name[TObj::STR_LEN] = {};
    char                 odef_name[TObj::STR_LEN]     = {};
    float                rendering_distance           = 0.f; // 0 means 'always rendered', see https://ogrecave.github.io/ogre/api/1.11/class_ogre_1_1_movable_object.html#afe1f2a1009e3f14f36e1bcc9b1b9557e
};

// -----------------------------------------------------------------------------
struct TObjFile
{
    TObjFile():
        grid_position(),
        grid_enabled(false)
    {}

    Ogre::Vector3                 grid_position;
    bool                          grid_enabled;
    std::vector<TObjTree>         trees;
    std::vector<TObjGrass>        grass;
    std::vector<TObjVehicle>      vehicles;
    std::vector<TObjEntry>        objects;
    std::vector<ProceduralObjectPtr> proc_objects;
};

// -----------------------------------------------------------------------------
class TObjParser
{
public:
    void                       Prepare();
    bool                       ProcessLine(const char* line);
    void                       ProcessOgreStream(Ogre::DataStream* stream);
    std::shared_ptr<TObjFile>  Finalize(); //!< Passes ownership

private:
    // Processing:
    bool                       ProcessCurrentLine();
    void                       ProcessProceduralLine();
    void                       ProcessGridLine();
    void                       ProcessTreesLine();
    void                       ProcessGrassLine();
    void                       ProcessActorObject(const TObjEntry& object);
    void                       ProcessRoadObject(const TObjEntry& object);

    // Helpers:
    void                       ImportProceduralPoint(Ogre::Vector3 const& pos, Ogre::Vector3 const& rot, TObj::SpecialObject special);
    Ogre::Quaternion           CalcRotation(Ogre::Vector3 const& rot) const;
    bool                       ParseObjectLine(TObjEntry& object);
    void                       FlushProceduralObject();

    std::shared_ptr<TObjFile>  m_def;
    std::string                m_filename;
    int                        m_line_number;
    const char*                m_cur_line;
    const char*                m_cur_line_trimmed;
    float                      m_default_rendering_distance;

    // Procedural roads
    bool                       m_in_procedural_road;
    ProceduralObjectPtr        m_cur_procedural_obj;
    int                        m_cur_procedural_obj_start_line;

    // Auto-importing legacy road blocks (ODEF) as procedural roads
    Ogre::Vector3              m_road2_last_pos;
    Ogre::Vector3              m_road2_last_rot;
    int                        m_road2_num_blocks;
};

} // namespace RoR
