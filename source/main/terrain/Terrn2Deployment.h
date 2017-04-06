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

#pragma once

/// @file
/// @author Petr Ohlidal, 04/2017

#include "ConfigFile.h"
#include "OTCFileformat.h"
#include "Terrn2FIleformat.h"
#include "TObjFileFormat.h"

#include <json/json.h>
#include <string>
#include <OgreColourValue.h>
#include <OgreQuaternion.h>
#include <OgreVector3.h>

namespace RoR {

/// Reads all terrain definition files (terrn2, otc, tobj, landuse, odef...)
/// and composes a single JSON file
/// GUIDELINE: Empty string values are forbidden in output - use 'null'
class Terrn2Deployer
{
public:
    bool          DeployTerrn2(std::string const& terrn2_filename);
    Json::Value&  GetJson() { return m_json; }

private:
    void          ProcessTerrn2Json();
    void          ProcessOtcJson();
    void          ProcessLanduseJson();
    void          ProcessTobjJson();
    void          ProcessTobjTreesJson();
    void          AddCollMeshJson(const char* name, float pos_x, float pos_z, Ogre::Quaternion rot, Ogre::Vector3 scale);
    void          HandleException(const char* action);
    bool          CheckAndLoadOTC();
    void          LoadLanduseConfig();
    void          LoadTobjFiles();
    Json::Value   ColorToJson(Ogre::ColourValue color);
    Json::Value   Vector3ToJson(Ogre::Vector3 pos);
    Json::Value   StringOrNull(std::string const & str);

    Terrn2Def                      m_terrn2;
    std::shared_ptr<RoR::OTCFile>  m_otc;
    std::list<std::shared_ptr<RoR::TObjFile>>  m_tobj_list;
    RoR::ConfigFile                m_landuse;
    Json::Value                    m_json;
};

} // namespace RoR
