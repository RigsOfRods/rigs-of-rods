/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2013-2021 Petr Ohlidal

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
/// @brief  Addons are AngelScript-powered feature packages for Rigs of Rods.
/// @author Petr Ohlidal, 05/2021

#include "ForwardDeclarations.h"
#include "CacheSystem.h"

#include <string>
#include <list>

namespace RoR {

struct AddonDef
{
    std::string              name;
    std::string              guid;
    int                      category_id = -1;
    int                      version = -1;
    std::string              readme_file;
    std::list<AuthorInfo>    authors;

    std::vector<std::string>   as_files;
    std::string              as_load_func_name;
    std::string              as_unload_func_name;
};

class AddonParser
{
public:
    bool LoadAddonFile(AddonDef& def, Ogre::DataStreamPtr &ds);

};

} // namespace RoR
