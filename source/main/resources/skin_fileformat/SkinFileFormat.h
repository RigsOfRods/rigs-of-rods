/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2013-2020 Petr Ohlidal

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

#include "Application.h"

#include <OgreResourceManager.h>

#include <map>
#include <vector>
#include <string>
#include <memory>

namespace RoR {

struct SkinDocument
{
    SkinDocument(): author_id(-1) {}

    std::map<std::string, std::string>  replace_textures;
    std::map<std::string, std::string>  replace_materials;
    std::string   name;
    std::string   guid;
    std::string   thumbnail;
    std::string   description;
    std::string   author_name;
    int           author_id;
};

class SkinParser
{
public:

    static std::vector<SkinDocumentPtr> ParseSkins(Ogre::DataStreamPtr& stream);

private:

    static void ParseSkinAttribute(const std::string& line, SkinDocument* skin_def);
};

}; // namespace RoR
