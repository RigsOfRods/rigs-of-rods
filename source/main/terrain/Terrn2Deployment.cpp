/*
    This source file is part of Rigs of Rods
    Copyright 2016-2017 Petr Ohlidal & contributors

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

#include "Terrn2Deployment.h"

#include "RoRPrerequisites.h"

#define LOGSTREAM Ogre::LogManager::getSingleton().stream() << "[RoR|DeployTerrn2] "

bool RoR::Terrn2Deployer::DeployTerrn2(std::string const & terrn2_filename)
{
    auto& ogre_resources = Ogre::ResourceGroupManager::getSingleton();

    // PROCESS .TERRRN2
    try
    {
        // create the Stream
        const std::string group_name = ogre_resources.findGroupContainingResource(terrn2_filename);
        Ogre::DataStreamPtr stream = ogre_resources.openResource(terrn2_filename, group_name);

        // Parse the file
        Terrn2Parser parser;
        const bool loaded_ok = parser.LoadTerrn2(m_terrn2, stream);

        // Report messages
        if (parser.GetMessages().size() > 0)
        {
            LOGSTREAM << "Terrn2 parser messages:";
            for (std::string const& msg : parser.GetMessages())
                LOGSTREAM << '\t' << msg;
        }

        // Error?
        if (!loaded_ok)
            return false;
    }
    catch (...)
    {
        this->HandleException("processing .terrn2 file");
        return false;
    }

    // TODO: Process others (OTC, ODEF, TOBJ...)

    // COMPOSE JSON
    m_json = Json::objectValue;
    this->ProcessTerrn2Json();

    return true;
}

Json::Value RoR::Terrn2Deployer::ColorToJson(Ogre::ColourValue color)
{
    Json::Value j_color;
    j_color["r"] = color.r;
    j_color["g"] = color.g;
    j_color["b"] = color.b;
    j_color["a"] = color.a;
    return j_color;
}

Json::Value RoR::Terrn2Deployer::Vector3ToJson(Ogre::Vector3 pos)
{
    Json::Value j_pos;
    j_pos["x"] = pos.x;
    j_pos["y"] = pos.y;
    j_pos["z"] = pos.z;
    return j_pos;
}

Json::Value RoR::Terrn2Deployer::StringOrNull(std::string const & str)
{
    if (str.empty())
        return Json::nullValue;
    else
        return str;
}

void RoR::Terrn2Deployer::ProcessTerrn2Json()
{
    m_json["terrn2"] = Json::objectValue;
    Json::Value& j_terrn2 = m_json["terrn2"];

    j_terrn2["ambient_color"]        = this->ColorToJson(m_terrn2.ambient_color);
    j_terrn2["start_position"]       = this->Vector3ToJson(m_terrn2.start_position);
    j_terrn2["category_id"]          = m_terrn2.category_id;//Int
    j_terrn2["guid"]                 = this->StringOrNull(m_terrn2.guid);
    j_terrn2["version"]              = m_terrn2.version;//Int
    j_terrn2["gravity"]              = m_terrn2.gravity;//Float
    j_terrn2["caelum_config"]        = this->StringOrNull(m_terrn2.caelum_config);
    j_terrn2["cubemap_config"]       = this->StringOrNull(m_terrn2.cubemap_config);
    j_terrn2["caelum_fog_start"]     = m_terrn2.caelum_fog_start;//Int
    j_terrn2["caelum_fog_end"]       = m_terrn2.caelum_fog_end;//Int
    j_terrn2["has_water"]            = m_terrn2.has_water;//Bool
    j_terrn2["hydrax_conf_file"]     = this->StringOrNull(m_terrn2.hydrax_conf_file);
    j_terrn2["traction_map_file"]    = this->StringOrNull(m_terrn2.traction_map_file);
    j_terrn2["water_height"]         = m_terrn2.water_height;//Float
    j_terrn2["water_bottom_height"]  = m_terrn2.water_bottom_height;//Float
    j_terrn2["has_water"]            = m_terrn2.has_water;//Bool
    j_terrn2["otc_file"]             = this->StringOrNull(m_terrn2.ogre_ter_conf_filename);

    j_terrn2["tobj_files"] = Json::arrayValue;
    for (std::string const & tobj_filename : m_terrn2.tobj_files)
    {
        j_terrn2["tobj_files"].append(Json::Value(tobj_filename));
    }

    j_terrn2["authors"] = Json::arrayValue;
    for (auto const & t2_author : m_terrn2.authors)
    {
        Json::Value j_author = Json::objectValue;
        j_author["name"] = t2_author.name;
        j_author["type"] = t2_author.type;
        j_terrn2["authors"].append(j_author);
    }

    j_terrn2["as_scripts"] = Json::arrayValue;
    for (std::string const & as_filename : m_terrn2.as_files)
    {
        j_terrn2["as_scripts"].append(Json::Value(as_filename));
    }

    j_terrn2["teleport"] = Json::objectValue;
    j_terrn2["teleport"]["telepoints"] = Json::arrayValue;
    for (auto const & t2_tele : m_terrn2.telepoints)
    {
        Json::Value j_author = Json::objectValue;
        j_author["name"] = t2_tele.name;
        j_author["position"] = this->Vector3ToJson(t2_tele.position);
        j_terrn2["teleport"]["telepoints"].append(j_author);
    }
}

void RoR::Terrn2Deployer::HandleException(const char* action)
{
    try { throw; } // Rethrow
    catch (Ogre::Exception& e)
    {
        LOGSTREAM << "Error " << action << ", message: " << e.getFullDescription();
    }
    catch (std::exception& e)
    {
        LOGSTREAM << "Error " << action << ", message: " << e.what();
    }
}