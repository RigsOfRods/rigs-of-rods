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

#include <array>
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

struct TuneupWheelTweak //!< Data of 'addonpart_tweak_wheel <wheel ID> <media1> <media2> <side flag> <tire radius> <rim radius>'
{
    WheelID_t          twt_wheel_id = WHEELID_INVALID;       //!< Arg#1, required
    /// `twt_media[0]` Arg#2, required ('wheels[2]': face material, 'meshwheels[2]/flexbodywheels': rim mesh)
    /// `twt_media[1]` Arg#3, optional ('wheels[2]': band material, 'meshwheels[2]/flexbodywheels': tire material)
    std::array<std::string, 2> twt_media;      
    RigDef::WheelSide  twt_side = RigDef::WheelSide::INVALID; //!< Arg#4, optional, default LEFT (Only applicable to mesh/flexbody wheels)
    float              twt_tire_radius = -1.f;  //!< Arg#5, optional
    float              twt_rim_radius = -1.f;   //!< Arg#6, optional, only applies to some wheel types
    std::string        twt_origin;              //!< Addonpart filename
    
};

struct TuneupPropTweak //!< Data of 'addonpart_tweak_prop <prop ID> <offsetX> <offsetY> <offsetZ> <rotX> <rotY> <rotZ> <media1> <media2>'
{
    PropID_t        tpt_prop_id = PROPID_INVALID;
    std::string     tpt_media[2];                     //!< Media1 = prop mesh; Media2: Steering wheel mesh or beacon flare material.
    Ogre::Vector3   tpt_offset = Ogre::Vector3::ZERO;
    Ogre::Vector3   tpt_rotation = Ogre::Vector3::ZERO;
    std::string     tpt_origin;                       //!< Addonpart filename
};

struct TuneupFlexbodyTweak //!< Data of 'addonpart_tweak_flexbody <flexbody ID> <offsetX> <offsetY> <offsetZ> <rotX> <rotY> <rotZ> <meshName>'
{
    FlexbodyID_t    tft_flexbody_id = FLEXBODYID_INVALID;
    std::string     tft_media;
    Ogre::Vector3   tft_offset = Ogre::Vector3::ZERO;
    Ogre::Vector3   tft_rotation = Ogre::Vector3::ZERO;
    std::string     tft_origin;                       //!< Addonpart filename
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

    std::map<NodeNum_t, TuneupNodeTweak> node_tweaks; //!< Node position overrides via 'addonpart_tweak_node'
    std::map<WheelID_t, TuneupWheelTweak> wheel_tweaks; //!< Mesh name and radius overrides via 'addonpart_tweak_wheel'
    std::map<PropID_t, TuneupPropTweak> prop_tweaks;
    std::map<FlexbodyID_t, TuneupFlexbodyTweak> flexbody_tweaks;

    std::set<PropID_t>     remove_props;
    std::set<FlexbodyID_t> remove_flexbodies;

    std::set<NodeNum_t>    protected_nodes;      //!< Nodes that cannot be altered via 'addonpart_tweak_node'
    std::set<WheelID_t>    protected_wheels;     //!< Wheels that cannot be altered via 'addonpart_tweak_wheel'
    std::set<PropID_t>     protected_props;      //!< Props which cannot be altered via 'addonpart_tweak_prop' or 'addonpart_remove_prop' directive.
    std::set<FlexbodyID_t> protected_flexbodies; //!< Flexbodies which cannot be removed via 'addonpart_tweak_flexbody' or 'addonpart_remove_flexbody' directive.
    /// @}

    TuneupDefPtr clone();

    /// @name Protection helpers
    /// @{
    bool         isPropProtected(PropID_t propid) { return protected_props.find(propid) != protected_props.end(); }
    bool         isFlexbodyProtected(FlexbodyID_t flexbodyid) { return protected_flexbodies.find(flexbodyid) != protected_flexbodies.end(); }
    bool         isWheelProtected(int wheelid) const { return protected_wheels.find(wheelid) != protected_wheels.end(); }
    bool         isNodeProtected(NodeNum_t nodenum) const { return protected_nodes.find(nodenum) != protected_nodes.end(); }
    /// @}
};

class TuneupUtil
{
public:

    static std::vector<TuneupDefPtr> ParseTuneups(Ogre::DataStreamPtr& stream);
    static void ExportTuneup(Ogre::DataStreamPtr& stream, TuneupDefPtr& tuneup);

    /// @name Tweaking helpers
    /// @{
    // > wheel
    static float              getTweakedWheelTireRadius(CacheEntryPtr& tuneup_entry, int wheel_id, float orig_val);
    static float              getTweakedWheelRimRadius(CacheEntryPtr& tuneup_entry, int wheel_id, float orig_val);
    static std::string        getTweakedWheelMedia(CacheEntryPtr& tuneup_entry, int wheel_id, int media_idx, const std::string& orig_val);
    static std::string        getTweakedWheelMediaRG(ActorPtr& actor, int wheel_id, int media_idx);
    static RigDef::WheelSide  getTweakedWheelSide(CacheEntryPtr& tuneup_entry, int wheel_id, RigDef::WheelSide orig_val);
    // > node
    static Ogre::Vector3      getTweakedNodePosition(CacheEntryPtr& tuneup_entry, NodeNum_t nodenum, Ogre::Vector3 orig_val);
    // > prop
    static bool               isPropRemoved(ActorPtr& actor, PropID_t prop_id);
    static Ogre::Vector3      getTweakedPropOffset(CacheEntryPtr& tuneup_entry, PropID_t prop_id, Ogre::Vector3 orig_val);
    static Ogre::Vector3      getTweakedPropRotation(CacheEntryPtr& tuneup_entry, PropID_t prop_id, Ogre::Vector3 orig_val);
    static std::string        getTweakedPropMedia(CacheEntryPtr& tuneup_entry, PropID_t prop_id, int media_idx, const std::string& orig_val);
    static std::string        getTweakedPropMediaRG(ActorPtr& actor, PropID_t prop_id, int media_idx);
    // > flexbody
    static bool               isFlexbodyRemoved(ActorPtr& actor, FlexbodyID_t flexbody_id);
    static Ogre::Vector3      getTweakedFlexbodyOffset(CacheEntryPtr& tuneup_entry, FlexbodyID_t flexbody_id, Ogre::Vector3 orig_val);
    static Ogre::Vector3      getTweakedFlexbodyRotation(CacheEntryPtr& tuneup_entry, FlexbodyID_t flexbody_id, Ogre::Vector3 orig_val);
    static std::string        getTweakedFlexbodyMedia(CacheEntryPtr& tuneup_entry, FlexbodyID_t flexbody_id, int media_idx, const std::string& orig_val);
    static std::string        getTweakedFlexbodyMediaRG(ActorPtr& actor, FlexbodyID_t flexbody_id, int media_idx);
    /// @}

private:

    static void ParseTuneupAttribute(const std::string& line, TuneupDefPtr& tuneup_def);
};

}; // namespace RoR
