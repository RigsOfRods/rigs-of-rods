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

/// @file
/// @author Petr Ohlidal, 11/2016

#include "Terrn2FileFormat.h"

#include "AppContext.h"
#include "ConfigFile.h"
#include "Console.h"
#include "Utils.h"
#include "SimConstants.h"

#include <OgreException.h>

using namespace RoR;
using namespace Ogre;

const std::string   VALUE_NOT_FOUND("@@NotFound!!");

bool Terrn2Parser::LoadTerrn2(Terrn2Def& def, Ogre::DataStreamPtr &ds)
{
    RoR::ConfigFile file;
    file.load(ds, "\t:=", true);

    // read in the settings
    def.name = file.GetStringEx("Name", "General");
    if (def.name.empty())
    {
        Str<500> msg; msg << "Error in file '" << ds->getName() << "': Terrain name is empty";
        App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_TERRN, Console::CONSOLE_SYSTEM_ERROR, msg.ToCStr());
        return false;
    }

    def.ogre_ter_conf_filename = file.GetStringEx("GeometryConfig", "General");
    // otc = ogre terrain config
    if (!def.ogre_ter_conf_filename.empty() && def.ogre_ter_conf_filename.find(".otc") == String::npos)
    {
        Str<500> msg; msg << "Error in file '" << ds->getName() << "': Invalid geometry config file; only '.otc' is supported";
        App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_TERRN, Console::CONSOLE_SYSTEM_ERROR, msg.ToCStr());
        return false;
    }

    def.ambient_color        = file.GetColourValue("AmbientColor",     "General", ColourValue::White);
    def.category_id          = file.getInt        ("CategoryID",       "General", 129);
    def.guid                 = file.GetStringEx   ("GUID",             "General");
    def.version              = file.getInt        ("Version",          "General", 1);
    def.gravity              = file.getFloat      ("Gravity",          "General", -9.81);
    def.caelum_config        = file.GetStringEx   ("CaelumConfigFile", "General");
    def.cubemap_config       = file.GetStringEx   ("SandStormCubeMap", "General");
    def.caelum_fog_start     = file.getInt        ("CaelumFogStart",   "General", -1);
    def.caelum_fog_end       = file.getInt        ("CaelumFogEnd",     "General", -1);
    def.has_water            = file.getBool       ("Water",            "General", false);
    def.hydrax_conf_file     = file.GetStringEx   ("HydraxConfigFile", "General");
    def.skyx_config          = file.GetStringEx   ("SkyXConfigFile",   "General");
    def.traction_map_file    = file.GetStringEx   ("TractionMap",      "General");
    def.water_height         = file.getFloat      ("WaterLine",        "General");
    def.water_bottom_height  = file.getFloat      ("WaterBottomLine",  "General");
    def.custom_material_name = file.GetStringEx   ("CustomMaterial",   "General");

    def.start_position       = StringConverter::parseVector3(file.GetStringEx("StartPosition", "General"), Vector3(512.0f, 0.0f, 512.0f));

    if (file.HasSection("Authors"))
    {
        for (auto& author: file.getSettings("Authors"))
        {
            String type = SanitizeUtf8String(author.first);  // e.g. terrain
            String name = SanitizeUtf8String(author.second); // e.g. john doe

            if (!name.empty())
            {
                Terrn2Author author;
                author.type = type;
                author.name = name;
                def.authors.push_back(author);
            }
        }
    }

    if (file.HasSection("Objects"))
    {
        for (auto& tobj: file.getSettings("Objects"))
        {
            Ogre::String tobj_filename = SanitizeUtf8String(tobj.first);
            def.tobj_files.push_back(TrimStr(tobj_filename));
        }
    }

    if (file.HasSection("Scripts"))
    {
        for (auto& script: file.getSettings("Scripts"))
        {
            Ogre::String as_filename = SanitizeUtf8String(script.first);
            def.as_files.push_back(TrimStr(as_filename));
        }
    }

    this->ProcessTeleport(def, &file);

    return true;
}

void Terrn2Parser::ProcessTeleport(Terrn2Def& def, RoR::ConfigFile* file)
{
    def.teleport_map_image = file->GetStringEx("NavigationMapImage", "Teleport");

    unsigned int telepoint_number = 1;
    for (;;)
    {
        char key_position [50];
        char key_name     [50];

        snprintf(key_position,  50, "Telepoint%u/Position" , telepoint_number);
        snprintf(key_name,      50, "Telepoint%u/Name" ,     telepoint_number);

        std::string pos_str = file->GetStringEx(key_position, "Teleport", VALUE_NOT_FOUND);
        if (pos_str == VALUE_NOT_FOUND)
        {
            break; // No more telepoints
        }
        Terrn2Telepoint t_point;
        if (sscanf(pos_str.c_str(), "%f, %f, %f", &t_point.position.x, &t_point.position.y, &t_point.position.z) != 3)
        {
            char msg_buf[500];
            snprintf(msg_buf, 500,
                "ERROR: Field '[Teleport]/%s' ('%s') is not valid XYZ position. Skipping telepoint %u.",
                key_position, pos_str.c_str(), telepoint_number);
            App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_TERRN,
                                          Console::CONSOLE_SYSTEM_WARNING, msg_buf);
        }
        else
        {
            t_point.name  = file->GetStringEx(key_name, "Teleport"); // Optional field
            def.telepoints.push_back(t_point); // Persist the entry
        }

        ++telepoint_number;
    }
}

Terrn2Def::Terrn2Def():
    ambient_color(Ogre::ColourValue::Black),
    category_id(-1),
    start_position(Ogre::Vector3::ZERO),
    version(1),
    gravity(DEFAULT_GRAVITY),
    water_height       (0),
    water_bottom_height(0),
    caelum_fog_start   (0),
    caelum_fog_end     (0),
    has_water(false)
{}
