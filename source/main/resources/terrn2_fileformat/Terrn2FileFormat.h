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
/// @author Petr Ohlidal, 11/2016

#include "ForwardDeclarations.h"

#include <string>
#include <list>

#include <OgreColourValue.h>
#include <OgreDataStream.h>
#include <OgreVector3.h>

namespace RoR {

struct Terrn2Author
{
    std::string type;
    std::string name;
};

struct Terrn2Telepoint //!< Teleport drop location
{
    Ogre::Vector3 position;
    std::string   name;
};

struct Terrn2Document
{
    Terrn2Document();

    std::string              name;
    std::string              ogre_ter_conf_filename;
    Ogre::ColourValue        ambient_color;
    int                      category_id;
    Ogre::Vector3            start_position;
    std::string              guid;
    int                      version;
    float                    gravity;
    float                    water_height;
    float                    water_bottom_height;
    std::list<Terrn2Author>  authors;
    std::list<std::string>   tobj_files;
    std::list<std::string>   as_files;
    std::list<std::string>   assetpack_files;
    std::list<std::string>   ai_presets_files;
    std::list<Terrn2Telepoint> telepoints;
    std::string              caelum_config;
    int                      caelum_fog_start;
    int                      caelum_fog_end;
    std::string              cubemap_config;
    bool                     has_water;
    std::string              hydrax_conf_file;
    std::string              skyx_config;
    std::string              traction_map_file;
    std::string              custom_material_name;
    std::string              teleport_map_image;
};

class Terrn2Parser
{
public:
    Terrn2DocumentPtr LoadTerrn2(Ogre::DataStreamPtr &ds);

private:
    void ProcessTeleport(Terrn2DocumentPtr def, RoR::ConfigFile* file);
};

} // namespace RoR
