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

struct TuneupNodeTweak //!< Data of 'addonpart_tweak_node <nodenum> <posX> <posY> <posZ>'
{
    NodeNum_t       tnt_nodenum = NODENUM_INVALID; //!< Arg#1, required
    Ogre::Vector3   tnt_pos = Ogre::Vector3::ZERO; //!< Args#234, required
    std::string     tnt_origin; //!< Addonpart filename
};

struct TuneupWheelTweak //!< Data of 'addonpart_tweak_wheel <wheel ID> <rim mesh> <tire radius> <rim radius>'
{
    int             twt_wheel_id = -1;       //!< Arg#1, required
    std::string     twt_rim_mesh;            //!< Arg#2, required
    float           twt_tire_radius = -1.f;  //!< Arg#3, optional
    float           twt_rim_radius = -1.f;   //!< Arg#4, optional, only applies to some wheel types
    std::string     twt_origin;              //!< Addonpart filename
    
};

struct TuneupDef: public RefCountingObject<TuneupDef>
{
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

    /// @name Modding attributes and overrides
    /// @{
    std::set<std::string>  use_addonparts;       //!< Addonpart filenames
    std::set<std::string>  remove_props;         //!< Mesh names of props to be removed.
    std::set<std::string>  remove_flexbodies;    //!< Mesh names of flexbodies to be removed.
    std::set<std::string>  protected_props;      //!< Mesh names of props which cannot be removed via 'addonpart_unwanted_*' directive.
    std::set<std::string>  protected_flexbodies; //!< Mesh names of flexbodies which cannot be removed via 'addonpart_unwanted_*' directive.
    std::map<NodeNum_t, TuneupNodeTweak> node_tweaks; //!< Node position overrides via 'addonpart_tweak_node'
    std::map<int, TuneupWheelTweak> wheel_tweaks; //!< Mesh name and radius overrides via 'addonpart_tweak_wheel'
    std::set<NodeNum_t>    protected_nodes;      //!< Node numbers that cannot be altered via 'addonpart_tweak_node'
    std::set<int>          protected_wheels;     //!< Wheel sequential numbers that cannot be altered via 'addonpart_tweak_wheel'
    /// @}

    TuneupDefPtr clone();

    /// @name Protection helpers
    /// @{
    bool         isPropProtected(const std::string& meshname) { return protected_props.find(meshname) != protected_props.end(); }
    bool         isFlexbodyProtected(const std::string& meshname) { return protected_flexbodies.find(meshname) != protected_flexbodies.end(); }
    bool         isWheelProtected(int wheelid) const { return protected_wheels.find(wheelid) != protected_wheels.end(); }
    bool         isNodeProtected(NodeNum_t nodenum) const { return protected_nodes.find(nodenum) != protected_nodes.end(); }
    /// @}

    /// @name Tweaking helpers
    /// @{
    static float        getTweakedWheelTireRadius(CacheEntryPtr& tuneup_entry, int wheel_id, float orig_val);
    static float        getTweakedWheelRimRadius(CacheEntryPtr& tuneup_entry, int wheel_id, float orig_val);
    static std::string  getTweakedWheelRimMesh(CacheEntryPtr& tuneup_entry, int wheel_id, const std::string& orig_val);
    static Ogre::Vector3 getTweakedNodePosition(CacheEntryPtr& tuneup_entry, NodeNum_t nodenum, Ogre::Vector3 orig_val);
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
