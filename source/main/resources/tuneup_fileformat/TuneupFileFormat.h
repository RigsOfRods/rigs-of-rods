/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2013-2024 Petr Ohlidal

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

/** @file
*   @brief  The vehicle tuning system; applies addonparts and user overrides to vehicles.
*   @see    `RoR::ActorSpawner::ConfigureAddonParts()`
*/

#include "Application.h"
#include "CacheSystem.h"
#include "RefCountingObject.h"

#include <OgreResourceManager.h>

#include <array>
#include <vector>
#include <string>
#include <set>

namespace RoR {

const size_t TUNEUP_BUF_SIZE = 2000;

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
    WheelSide  twt_side = WheelSide::INVALID; //!< Arg#4, optional, default LEFT (Only applicable to mesh/flexbody wheels)
    float              twt_tire_radius = -1.f;  //!< Arg#5, optional
    float              twt_rim_radius = -1.f;   //!< Arg#6, optional, only applies to some wheel types
    std::string        twt_origin;              //!< Addonpart filename
    
};

struct TuneupPropTweak //!< Data of 'addonpart_tweak_prop <prop ID> <offsetX> <offsetY> <offsetZ> <rotX> <rotY> <rotZ> <media1> <media2>'
{
    PropID_t        tpt_prop_id = PROPID_INVALID;
    std::array<std::string, 2> tpt_media;                     //!< Media1 = prop mesh; Media2: Steering wheel mesh or beacon flare material.
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

/// Dual purpose:
///  1. representing a .tuneup file, see `CacheEntry::tuneup_def` (the obvious use)
///  2. holding addonpart data for conflict resolution, see `CacheEntry::addonpart_data_only` (an additional hack)
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

    /// @name Addonparts and extracted data
    /// @{
    std::set<std::string>                         use_addonparts;        //!< Addonpart filenames

    std::map<NodeNum_t, TuneupNodeTweak>          node_tweaks;           //!< Node position overrides via 'addonpart_tweak_node'
    std::map<WheelID_t, TuneupWheelTweak>         wheel_tweaks;          //!< Mesh name and radius overrides via 'addonpart_tweak_wheel'
    std::map<PropID_t, TuneupPropTweak>           prop_tweaks;           //!< Mesh name(s), offset and rotation overrides via 'addonpart_tweak_prop'
    std::map<FlexbodyID_t, TuneupFlexbodyTweak>   flexbody_tweaks;       //!< Mesh name, offset and rotation overrides via 'addonpart_tweak_flexbody'
    std::set<PropID_t>                            unwanted_props;          //!< 'addonpart_unwanted_prop' directives.
    std::set<FlexbodyID_t>                        unwanted_flexbodies;     //!< 'addonpart_unwanted_flexbody' directives.
    std::set<FlareID_t>                           unwanted_flares;         //!< 'addonpart_unwanted_flare' directives.
    std::set<ExhaustID_t>                         unwanted_exhausts;       //!< 'addonpart_unwanted_exhaust' directives.
    /// @}

    /// @name UI-controlled forced changes (override addonparts)
    /// @{
    std::set<PropID_t>                            force_remove_props;      //!< UI overrides
    std::set<FlexbodyID_t>                        force_remove_flexbodies; //!< UI overrides
    std::map<WheelID_t, WheelSide>                force_wheel_sides;       //!< UI overrides
    std::set<FlareID_t>                           force_remove_flares;     //!< User unticked an UI checkbox in Tuning menu, section Flares.
    std::set<ExhaustID_t>                         force_remove_exhausts;   //!< User unticked an UI checkbox in Tuning menu, section Exhausts.
    /// @}

    /// @name UI-controlled protection from addonpart tweaks
    /// @{
    std::set<NodeNum_t>                           protected_nodes;       //!< Nodes that cannot be altered via 'addonpart_tweak_node'
    std::set<WheelID_t>                           protected_wheels;      //!< Wheels that cannot be altered via 'addonpart_tweak_wheel'
    std::set<PropID_t>                            protected_props;       //!< Props which cannot be altered via 'addonpart_tweak_prop' or 'addonpart_unwanted_prop' directive.
    std::set<FlexbodyID_t>                        protected_flexbodies;  //!< Flexbodies which cannot be altered via 'addonpart_tweak_flexbody' or 'addonpart_unwanted_flexbody' directive.
    std::set<FlareID_t>                           protected_flares;      //!< Flares which cannot be altered via 'addonpart_unwanted_flare' directive.
    std::set<ExhaustID_t>                         protected_exhausts;    //!< Exhausts which cannot be altered via 'addonpart_unwanted_exhaust' directive.
    /// @}

    TuneupDefPtr clone();
    void reset();

    /// @name Protection helpers
    /// @{
    bool         isPropProtected(PropID_t propid) const { return protected_props.find(propid) != protected_props.end(); }
    bool         isFlexbodyProtected(FlexbodyID_t flexbodyid) const { return protected_flexbodies.find(flexbodyid) != protected_flexbodies.end(); }
    bool         isWheelProtected(WheelID_t wheelid) const { return protected_wheels.find(wheelid) != protected_wheels.end(); }
    bool         isNodeProtected(NodeNum_t nodenum) const { return protected_nodes.find(nodenum) != protected_nodes.end(); }
    bool         isFlareProtected(FlareID_t flareid) const { return protected_flares.find(flareid) != protected_flares.end(); }
    bool         isExhaustProtected(ExhaustID_t exhaustid) const { return protected_exhausts.find(exhaustid) != protected_exhausts.end(); }
    /// @}

    /// @name Unwanted-state helpers
    /// @{
    bool         isPropUnwanted(PropID_t propid) { return unwanted_props.find(propid) != unwanted_props.end(); }
    bool         isFlexbodyUnwanted(FlexbodyID_t flexbodyid) { return unwanted_flexbodies.find(flexbodyid) != unwanted_flexbodies.end(); }
    bool         isFlareUnwanted(FlareID_t flareid) { return unwanted_flares.find(flareid) != unwanted_flares.end(); }
    bool         isExhaustUnwanted(ExhaustID_t exhaustid) { return unwanted_exhausts.find(exhaustid) != unwanted_exhausts.end(); }
    /// @}

    /// @name Forced-state helpers
    /// @{
    bool         isPropForceRemoved(PropID_t propid) { return force_remove_props.find(propid) != force_remove_props.end(); }
    bool         isFlexbodyForceRemoved(FlexbodyID_t flexbodyid) { return force_remove_flexbodies.find(flexbodyid) != force_remove_flexbodies.end(); }
    bool         isWheelSideForced(WheelID_t wheelid, WheelSide& out_val) const;
    bool         isFlareForceRemoved(FlareID_t flareid) { return force_remove_flares.find(flareid) != force_remove_flares.end(); }
    bool         isExhaustForceRemoved(ExhaustID_t exhaustid) { return force_remove_exhausts.find(exhaustid) != force_remove_exhausts.end(); }
    /// @}
};

class TuneupUtil
{
public:

    static std::vector<TuneupDefPtr> ParseTuneups(Ogre::DataStreamPtr& stream);
    static void ExportTuneup(Ogre::DataStreamPtr& stream, TuneupDefPtr& tuneup);

    /// @name AddonPart helpers
    /// @{
    static bool               isAddonPartUsed(TuneupDefPtr& tuneup_entry, const std::string& filename);
    /// @}

    /// @name Wheel helpers
    /// @{
    static float              getTweakedWheelTireRadius(TuneupDefPtr& tuneup_entry, WheelID_t wheel_id, float orig_val);
    static float              getTweakedWheelRimRadius(TuneupDefPtr& tuneup_entry, WheelID_t wheel_id, float orig_val);
    static std::string        getTweakedWheelMedia(TuneupDefPtr& tuneup_entry, WheelID_t wheel_id, int media_idx, const std::string& orig_val);
    static std::string        getTweakedWheelMediaRG(TuneupDefPtr& tuneup_def, WheelID_t wheel_id, int media_idx, const std::string& orig_val);
    static WheelSide          getTweakedWheelSide(TuneupDefPtr& tuneup_entry, WheelID_t wheel_id, WheelSide orig_val);
    static bool               isWheelTweaked(TuneupDefPtr& tuneup_entry, WheelID_t wheel_id, TuneupWheelTweak*& out_tweak);
    /// @}

    /// @name Node helpers
    /// @{
    static Ogre::Vector3      getTweakedNodePosition(TuneupDefPtr& tuneup_entry, NodeNum_t nodenum, Ogre::Vector3 orig_val);
    static bool               isNodeTweaked(TuneupDefPtr& tuneup_entry, NodeNum_t nodenum, TuneupNodeTweak*& out_tweak);
    /// @}

    /// @name Prop helpers
    /// @{
    static bool               isPropAnyhowRemoved(TuneupDefPtr& tuneup_def, PropID_t prop_id);
    static Ogre::Vector3      getTweakedPropOffset(TuneupDefPtr& tuneup_entry, PropID_t prop_id, Ogre::Vector3 orig_val);
    static Ogre::Vector3      getTweakedPropRotation(TuneupDefPtr& tuneup_entry, PropID_t prop_id, Ogre::Vector3 orig_val);
    static std::string        getTweakedPropMedia(TuneupDefPtr& tuneup_entry, PropID_t prop_id, int media_idx, const std::string& orig_val);
    static std::string        getTweakedPropMediaRG(TuneupDefPtr& tuneup_def, PropID_t prop_id, int media_idx, const std::string& orig_val);
    static bool               isPropTweaked(TuneupDefPtr& tuneup_entry, PropID_t flexbody_id, TuneupPropTweak*& out_tweak);
    /// @}

    /// @name Flexbody helpers
    /// @{
    static bool               isFlexbodyAnyhowRemoved(TuneupDefPtr& tuneup_def, FlexbodyID_t flexbody_id);
    static Ogre::Vector3      getTweakedFlexbodyOffset(TuneupDefPtr& tuneup_entry, FlexbodyID_t flexbody_id, Ogre::Vector3 orig_val);
    static Ogre::Vector3      getTweakedFlexbodyRotation(TuneupDefPtr& tuneup_entry, FlexbodyID_t flexbody_id, Ogre::Vector3 orig_val);
    static std::string        getTweakedFlexbodyMedia(TuneupDefPtr& tuneup_entry, FlexbodyID_t flexbody_id, int media_idx, const std::string& orig_val);
    static std::string        getTweakedFlexbodyMediaRG(TuneupDefPtr& tuneup_def, FlexbodyID_t flexbody_id, int media_idx, const std::string& orig_val);
    static bool               isFlexbodyTweaked(TuneupDefPtr& tuneup_entry, FlexbodyID_t flexbody_id, TuneupFlexbodyTweak*& out_tweak);
    /// @}

    /// @name Flare helpers
    /// @{
    static bool               isFlareAnyhowRemoved(TuneupDefPtr& tuneup_def, FlareID_t flare_id);
    /// @}

    /// @name Exhaust helpers
    /// @{
    static bool               isExhaustAnyhowRemoved(TuneupDefPtr& tuneup_def, ExhaustID_t exhaust_id);
    /// @}

private:

    static void ParseTuneupAttribute(const std::string& line, TuneupDefPtr& tuneup_def);
};

}; // namespace RoR
