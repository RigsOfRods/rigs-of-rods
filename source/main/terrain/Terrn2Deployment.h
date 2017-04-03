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

#include "OTCFileformat.h"
#include "Terrn2FIleformat.h"

#include <json/json.h>
#include <string>

namespace RoR {

/// Reads all terrain definition files (terrn2, otc, tobj, odef...)
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
    void          HandleException(const char* action);
    bool          CheckAndLoadOTC();
    Json::Value   ColorToJson(Ogre::ColourValue color);
    Json::Value   Vector3ToJson(Ogre::Vector3 pos);
    Json::Value   StringOrNull(std::string const & str);

    Terrn2Def                      m_terrn2;
    std::shared_ptr<RoR::OTCFile>  m_otc;
    Json::Value                    m_json;
};

} // namespace RoR
