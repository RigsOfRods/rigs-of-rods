/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2013-2023 Petr Ohlidal

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
#include "CacheSystem.h"
#include "RefCountingObject.h"

#include <OgreResourceManager.h>

#include <vector>
#include <string>
#include <set>

namespace RoR {

struct TuneupDef: public RefCountingObject<TuneupDef>
{
    TuneupDefPtr clone();
    bool         isPropProtected(const std::string& meshname) { return protected_props.find(meshname) != protected_props.end(); }
    bool         isFlexbodyProtected(const std::string& meshname) { return protected_flexbodies.find(meshname) != protected_flexbodies.end(); }

    /// @name General info
    /// @{
    std::string      name;
    std::string      guid;
    std::string      thumbnail;
    std::string      description;
    std::string      author_name;
    int              author_id = -1;
    CacheCategoryId  category_id = CID_None;
    /// @}

    /// @name Modding attributes
    /// @{
    std::set<std::string>  use_addonparts;       //!< Addonpart filenames
    std::set<std::string>  remove_props;         //!< Mesh names of props to be removed.
    std::set<std::string>  remove_flexbodies;    //!< Mesh names of flexbodies to be removed.
    std::set<std::string>  protected_props;      //!< Mesh names of props which cannot be removed via 'addonpart_unwanted_*' directive.
    std::set<std::string>  protected_flexbodies; //!< Mesh names of flexbodies which cannot be removed via 'addonpart_unwanted_*' directive.
    /// @}
};

class TuneupParser
{
public:

    static std::vector<TuneupDefPtr> ParseTuneups(Ogre::DataStreamPtr& stream);
    static void ExportTuneup(Ogre::DataStreamPtr& stream, TuneupDefPtr& tuneup);

private:

    static void ParseTuneupAttribute(const std::string& line, TuneupDefPtr& tuneup_def);
};

}; // namespace RoR
