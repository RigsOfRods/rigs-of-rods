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

Terrn2DocumentPtr Terrn2Parser::LoadTerrn2(Ogre::DataStreamPtr &ds)
{
    RoR::ConfigFile file;
    file.load(ds, "\t:=", true);
    file.setLoggingInfo(ds->getName(), Console::CONSOLE_MSGTYPE_TERRN);

    Terrn2DocumentPtr def = std::make_shared<Terrn2Document>();

    // read in the settings
    def->name = file.getString("Name", "General");
    if (def->name.empty())
    {
        Str<500> msg; msg << "Error in file '" << ds->getName() << "': Terrain name is empty";
        App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_TERRN, Console::CONSOLE_SYSTEM_ERROR, msg.ToCStr());
        return nullptr;
    }

    def->ogre_ter_conf_filename = file.getString("GeometryConfig", "General");
    // otc = ogre terrain config
    if (!def->ogre_ter_conf_filename.empty() && def->ogre_ter_conf_filename.find(".otc") == String::npos)
    {
        Str<500> msg; msg << "Error in file '" << ds->getName() << "': Invalid geometry config file; only '.otc' is supported";
        App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_TERRN, Console::CONSOLE_SYSTEM_ERROR, msg.ToCStr());
        return nullptr;
    }

    def->ambient_color        = file.getColourValue("AmbientColor",     "General", ColourValue::White);
    def->category_id          = file.getInt        ("CategoryID",       "General", 129);
    def->guid                 = file.getString     ("GUID",             "General");
    def->version              = file.getInt        ("Version",          "General", 1);
    def->gravity              = file.getFloat      ("Gravity",          "General", -9.81);
    def->caelum_config        = file.getString     ("CaelumConfigFile", "General");
    def->cubemap_config       = file.getString     ("SandStormCubeMap", "General");
    def->caelum_fog_start     = file.getInt        ("CaelumFogStart",   "General", -1);
    def->caelum_fog_end       = file.getInt        ("CaelumFogEnd",     "General", -1);
    def->has_water            = file.getBool       ("Water",            "General", false);
    def->hydrax_conf_file     = file.getString     ("HydraxConfigFile", "General");
    def->skyx_config          = file.getString     ("SkyXConfigFile",   "General");
    def->traction_map_file    = file.getString     ("TractionMap",      "General");
    def->water_height         = file.getFloat      ("WaterLine",        "General");
    def->water_bottom_height  = file.getFloat      ("WaterBottomLine",  "General");
    def->custom_material_name = file.getString     ("CustomMaterial",   "General");
    def->start_position       = file.getVector3    ("StartPosition",    "General", Vector3(512.0f, 0.0f, 512.0f));

    def->start_rotation_specified = file.HasSetting("General", "StartRotation");
    def->start_rotation = Ogre::Degree(file.getFloat("StartRotation", "General"));

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
                def->authors.push_back(author);
            }
        }
    }

    if (file.HasSection("Objects"))
    {
        for (auto& tobj: file.getSettings("Objects"))
        {
            Ogre::String tobj_filename = SanitizeUtf8String(tobj.first);
            def->tobj_files.push_back(TrimStr(tobj_filename));
        }
    }

    if (file.HasSection("Scripts"))
    {
        for (auto& script: file.getSettings("Scripts"))
        {
            Ogre::String as_filename = SanitizeUtf8String(script.first);
            def->as_files.push_back(TrimStr(as_filename));
        }
    }

    if (file.HasSection("AssetPacks"))
    {
        for (auto& assetpack: file.getSettings("AssetPacks"))
        {
            Ogre::String assetpack_filename = SanitizeUtf8String(assetpack.first);
            def->assetpack_files.push_back(TrimStr(assetpack_filename));
        }
    }

    if (file.HasSection("AI Presets"))
    {
        for (auto& presets: file.getSettings("AI Presets"))
        {
            Ogre::String presets_filename = SanitizeUtf8String(presets.first);
            def->ai_presets_files.push_back(TrimStr(presets_filename));
        }
    }

    this->ProcessTeleport(def, &file);

    return def;
}

void Terrn2Parser::ProcessTeleport(Terrn2DocumentPtr def, RoR::ConfigFile* file)
{
    def->teleport_map_image = file->getString("NavigationMapImage", "Teleport");

    unsigned int telepoint_number = 1;
    for (;;)
    {
        char key_position [50];
        char key_name     [50];

        snprintf(key_position,  50, "Telepoint%u/Position" , telepoint_number);
        snprintf(key_name,      50, "Telepoint%u/Name" ,     telepoint_number);

        std::string pos_str = file->getString(key_position, "Teleport", VALUE_NOT_FOUND);
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
            t_point.name  = file->getString(key_name, "Teleport"); // Optional field
            def->telepoints.push_back(t_point); // Persist the entry
        }

        ++telepoint_number;
    }
}
