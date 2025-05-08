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

#include "TuneupFileFormat.h"

#include "Actor.h"
#include "Application.h"
#include "CacheSystem.h"
#include "Console.h"
#include "Utils.h"

#include <OgreEntity.h>
#include <OgreMaterialManager.h>
#include <OgrePass.h>
#include <OgreSubEntity.h>
#include <OgreTechnique.h>

using namespace RoR;

TuneupDefPtr TuneupDef::clone()
{
    TuneupDefPtr ret = new TuneupDef();
    
    // General info
    ret->name               =     this->name              ; //std::string           
    ret->guid               =     this->guid              ; //std::string           
    ret->thumbnail          =     this->thumbnail         ; //std::string           
    ret->description        =     this->description       ; //std::string           
    ret->author_name        =     this->author_name       ; //std::string           
    ret->author_id          =     this->author_id         ; //int                   
    ret->category_id        =     this->category_id       ; //CacheCategoryId   
    ret->filename           =     this->filename          ; //std::string

    // addonparts
    ret->use_addonparts       =   this->use_addonparts    ;

    // props
    ret->protected_props = this->protected_props;
    ret->unwanted_props = this->unwanted_props;
    ret->force_remove_props = this->force_remove_props;
    ret->prop_tweaks = this->prop_tweaks;

    // flexbodies
    ret->protected_flexbodies = this->protected_flexbodies;
    ret->unwanted_flexbodies = this->unwanted_flexbodies;
    ret->force_remove_flexbodies = this->force_remove_flexbodies;
    ret->flexbody_tweaks = this->flexbody_tweaks;

    // wheels
    ret->protected_wheels = this->protected_wheels;
    ret->force_wheel_sides = this->force_wheel_sides;
    ret->wheel_tweaks = this->wheel_tweaks;

    // nodes
    ret->protected_nodes = this->protected_nodes;
    ret->node_tweaks = this->node_tweaks;

    // video cameras
    ret->force_video_cam_roles = this->force_video_cam_roles;

    // flares
    ret->protected_flares = this->protected_flares;

    // exhausts
    ret->protected_exhausts = this->protected_exhausts;

    // managed materials
    ret->protected_managedmats = this->protected_managedmats;

    return ret;
}

bool TuneupDef::isWheelSideForced(WheelID_t wheelid, WheelSide& out_val) const
{
    auto itor = force_wheel_sides.find(wheelid);
    if (itor != force_wheel_sides.end())
    {
        out_val = itor->second;
        return true;
    }
    else
    {
        return false;
    }
}

bool TuneupDef::isVideoCameraRoleForced(VideoCameraID_t camera_id, VideoCamRole& out_val) const
{
    auto itor = force_video_cam_roles.find(camera_id);
    if (itor != force_video_cam_roles.end())
    {
        out_val = itor->second;
        return true;
    }
    else
    {
        return false;
    }
}

    // Tweaking helpers

float RoR::TuneupUtil::getTweakedWheelTireRadius(TuneupDefPtr& tuneup_def, WheelID_t wheel_id, float orig_val)
{
    TuneupWheelTweak* tweak = nullptr;
    if (TuneupUtil::isWheelTweaked(tuneup_def, wheel_id, /*out:*/ tweak))
    {
        ROR_ASSERT(tweak);
        return (tweak->twt_tire_radius > 0) ? tweak->twt_tire_radius : orig_val;
    }
    else
    {
        return orig_val;
    }
}

float RoR::TuneupUtil::getTweakedWheelRimRadius(TuneupDefPtr& tuneup_def, WheelID_t wheel_id, float orig_val)
{
    TuneupWheelTweak* tweak = nullptr;
    if (TuneupUtil::isWheelTweaked(tuneup_def, wheel_id, tweak))
    {
        ROR_ASSERT(tweak);
        return (tweak->twt_rim_radius > 0) ? tweak->twt_rim_radius : orig_val;
    }
    else
    {
        return orig_val;
    }
}

std::string RoR::TuneupUtil::getTweakedWheelMedia(TuneupDefPtr& tuneup_def, WheelID_t wheel_id, int media_idx, const std::string& orig_val)
{
    TuneupWheelTweak* tweak = nullptr;
    if (TuneupUtil::isWheelTweaked(tuneup_def, wheel_id, tweak))
    {
        ROR_ASSERT(tweak);
        ROR_ASSERT(tweak->twt_media.size() > media_idx);
        return (tweak->twt_media[media_idx] != "") ? tweak->twt_media[media_idx] : orig_val;
    }
    else
    {
        return orig_val;
    }
}

std::string RoR::TuneupUtil::getTweakedWheelMediaRG(TuneupDefPtr& tuneup_def, WheelID_t wheel_id, int media_idx, const std::string& orig_val)
{
    TuneupWheelTweak* tweak = nullptr;
    if (TuneupUtil::isWheelTweaked(tuneup_def, wheel_id, tweak))
    {
        ROR_ASSERT(tweak);
        ROR_ASSERT(tweak->twt_media.size() > media_idx);
        if (tweak->twt_media[media_idx] != "")
        {
            // Find the tweak addonpart
            CacheEntryPtr addonpart_entry = App::GetCacheSystem()->FindEntryByFilename(LT_AddonPart, /*'partial':*/false, tweak->twt_origin);
            if (addonpart_entry)
            {
                return addonpart_entry->resource_group;
            }
            else
            {
                LOG(fmt::format("[RoR|Tuneup] WARN Addonpart '{}' not found in modcache!", tweak->twt_origin));
                return orig_val;
            }
        }
    }
    
    return orig_val;
    
}

WheelSide RoR::TuneupUtil::getTweakedWheelSide(TuneupDefPtr& tuneup_def, WheelID_t wheel_id, WheelSide orig_val)
{
    

    if (tuneup_def)
    {
        WheelSide forced_side = WheelSide::INVALID;
        if (tuneup_def->isWheelSideForced(wheel_id, forced_side))
        {
            return forced_side;
        }

        TuneupWheelTweak* tweak = nullptr;
        if (TuneupUtil::isWheelTweaked(tuneup_def, wheel_id, tweak))
        {
            ROR_ASSERT(tweak);
            if (tweak->twt_side != WheelSide::INVALID)
            {
                return tweak->twt_side;
            }
        }
    }

    return orig_val;
}

VideoCamRole RoR::TuneupUtil::getTweakedVideoCameraRole(TuneupDefPtr& tuneup_def, VideoCameraID_t camera_id, VideoCamRole orig_val)
{
    if (tuneup_def)
    {
        VideoCamRole forced_role = VCAM_ROLE_INVALID;
        if (tuneup_def->isVideoCameraRoleForced(camera_id, forced_role))
        {
            return forced_role;
        }

        // STUB: actual tweaking isn't implemented yet
    }

    return orig_val;
}

bool RoR::TuneupUtil::isWheelTweaked(TuneupDefPtr& tuneup_def, WheelID_t wheel_id, TuneupWheelTweak*& out_tweak)
{
    

    if (tuneup_def)
    {
        auto itor = tuneup_def->wheel_tweaks.find(wheel_id);
        if (itor != tuneup_def->wheel_tweaks.end())
        {
            out_tweak = &itor->second;
            return true;
        }
    }

    return false;
}

Ogre::Vector3 RoR::TuneupUtil::getTweakedNodePosition(TuneupDefPtr& tuneup_def, NodeNum_t nodenum, Ogre::Vector3 orig_val)
{
    TuneupNodeTweak* tweak = nullptr;
    if (TuneupUtil::isNodeTweaked(tuneup_def, nodenum, tweak))
    {
        ROR_ASSERT(tweak);
        return tweak->tnt_pos;
    }
    else
    {
        return orig_val;
    }
}

Ogre::Vector3 RoR::TuneupUtil::getTweakedCineCameraPosition(TuneupDefPtr& tuneup_def, CineCameraID_t cinecamid, Ogre::Vector3 orig_val)
{
    TuneupCineCameraTweak* tweak = nullptr;
    if (TuneupUtil::isCineCameraTweaked(tuneup_def, cinecamid, tweak))
    {
        ROR_ASSERT(tweak);
        return tweak->tct_pos;
    }
    else
    {
        return orig_val;
    }
}

bool RoR::TuneupUtil::isNodeTweaked(TuneupDefPtr& tuneup_def, NodeNum_t nodenum, TuneupNodeTweak*& out_tweak)
{
    if (tuneup_def)
    {
        auto itor = tuneup_def->node_tweaks.find(nodenum);
        if (itor != tuneup_def->node_tweaks.end())
        {
            out_tweak = &itor->second;
            return true;
        }
    }

    return false;
}

bool RoR::TuneupUtil::isCineCameraTweaked(TuneupDefPtr& tuneup_def, CineCameraID_t cinecamid, TuneupCineCameraTweak*& out_tweak)
{
    if (tuneup_def)
    {
        auto itor = tuneup_def->cinecam_tweaks.find(cinecamid);
        if (itor != tuneup_def->cinecam_tweaks.end())
        {
            out_tweak = &itor->second;
            return true;
        }
    }

    return false;
}


// > prop
bool RoR::TuneupUtil::isPropAnyhowRemoved(TuneupDefPtr& tuneup_def, PropID_t prop_id)
{    
    return tuneup_def 
        && (tuneup_def->isPropUnwanted(prop_id) || tuneup_def->isPropForceRemoved(prop_id));   
}

Ogre::Vector3 RoR::TuneupUtil::getTweakedPropOffset(TuneupDefPtr& tuneup_def, PropID_t prop_id, Ogre::Vector3 orig_val)
{
    TuneupPropTweak* tweak = nullptr;
    if (TuneupUtil::isPropTweaked(tuneup_def, prop_id, tweak))
    {
        ROR_ASSERT(tweak);
        return tweak->tpt_offset;
    }
    else
    {
        return orig_val;
    }
}

Ogre::Vector3 RoR::TuneupUtil::getTweakedPropRotation(TuneupDefPtr& tuneup_def, PropID_t prop_id, Ogre::Vector3 orig_val)
{
    TuneupPropTweak* tweak = nullptr;
    if (TuneupUtil::isPropTweaked(tuneup_def, prop_id, tweak))
    {
        ROR_ASSERT(tweak);
        return tweak->tpt_rotation;
    }
    else
    {
        return orig_val;
    }
}

std::string RoR::TuneupUtil::getTweakedPropMedia(TuneupDefPtr& tuneup_def, PropID_t prop_id, int media_idx, const std::string& orig_val)
{
    TuneupPropTweak* tweak = nullptr;
    if (TuneupUtil::isPropTweaked(tuneup_def, prop_id, tweak))
    {
        ROR_ASSERT(tweak);
        ROR_ASSERT(tweak->tpt_media.size() > media_idx);
        return (tweak->tpt_media[media_idx] != "") ? tweak->tpt_media[media_idx] : orig_val;
    }
    else
    {
        return orig_val;
    }
}

std::string RoR::TuneupUtil::getTweakedPropMediaRG(TuneupDefPtr& tuneup_def, PropID_t prop_id, int media_idx, const std::string& orig_val)
{
    TuneupPropTweak* tweak = nullptr;
    if (TuneupUtil::isPropTweaked(tuneup_def, prop_id, tweak))
    {
        ROR_ASSERT(tweak);
        ROR_ASSERT(tweak->tpt_media.size() > media_idx);
        if (tweak->tpt_media[media_idx] != "")
        {
            // Find the tweak addonpart
            CacheEntryPtr addonpart_entry = App::GetCacheSystem()->FindEntryByFilename(LT_AddonPart, /*partial:*/false, tweak->tpt_origin);
            if (addonpart_entry)
            {
                return addonpart_entry->resource_group;
            }
            else
            {
                LOG(fmt::format("[RoR|Tuneup] WARN Addonpart '{}' not found in modcache!", tweak->tpt_origin));
            }
        }
    }

    return orig_val;
}

bool RoR::TuneupUtil::isPropTweaked(TuneupDefPtr& tuneup_def, PropID_t prop_id, TuneupPropTweak*& out_tweak)
{
    

    if (tuneup_def)
    {
        auto itor = tuneup_def->prop_tweaks.find(prop_id);
        if (itor != tuneup_def->prop_tweaks.end())
        {
            out_tweak = &itor->second;
            return true;
        }
    }

    return false;
}

// > flexbody
bool RoR::TuneupUtil::isFlexbodyAnyhowRemoved(TuneupDefPtr& tuneup_def, FlexbodyID_t flexbody_id)
{
    return tuneup_def && 
        (tuneup_def->isFlexbodyUnwanted(flexbody_id) || tuneup_def->isFlexbodyForceRemoved(flexbody_id));
}

Ogre::Vector3 RoR::TuneupUtil::getTweakedFlexbodyOffset(TuneupDefPtr& tuneup_def, FlexbodyID_t flexbody_id, Ogre::Vector3 orig_val)
{
    TuneupFlexbodyTweak* tweak = nullptr;
    if (TuneupUtil::isFlexbodyTweaked(tuneup_def, flexbody_id, tweak))
    {
        ROR_ASSERT(tweak);
        return tweak->tft_offset;
    }
    else
    {
        return orig_val;
    }
}

Ogre::Vector3 RoR::TuneupUtil::getTweakedFlexbodyRotation(TuneupDefPtr& tuneup_def, FlexbodyID_t flexbody_id, Ogre::Vector3 orig_val)
{
    TuneupFlexbodyTweak* tweak = nullptr;
    if (TuneupUtil::isFlexbodyTweaked(tuneup_def, flexbody_id, tweak))
    {
        ROR_ASSERT(tweak);
        return tweak->tft_rotation;
    }
    else
    {
        return orig_val;
    }
}

std::string RoR::TuneupUtil::getTweakedFlexbodyMedia(TuneupDefPtr& tuneup_def, FlexbodyID_t flexbody_id, int media_idx, const std::string& orig_val)
{
    TuneupFlexbodyTweak* tweak = nullptr;
    if (TuneupUtil::isFlexbodyTweaked(tuneup_def, flexbody_id, tweak))
    {
        ROR_ASSERT(tweak);
        return (tweak->tft_media != "") ? tweak->tft_media : orig_val;
    }
    else
    {
        return orig_val;
    }
}

std::string RoR::TuneupUtil::getTweakedFlexbodyMediaRG(TuneupDefPtr& tuneup_def, FlexbodyID_t flexbody_id, int media_idx, const std::string& orig_val)
{
    TuneupFlexbodyTweak* tweak = nullptr;
    if (TuneupUtil::isFlexbodyTweaked(tuneup_def, flexbody_id, tweak))
    {
        ROR_ASSERT(tweak);
        if (tweak->tft_media != "")
        {
            // Find the tweak addonpart
            CacheEntryPtr addonpart_entry = App::GetCacheSystem()->FindEntryByFilename(LT_AddonPart, /*partial:*/false, tweak->tft_origin);
            if (addonpart_entry)
            {
                return addonpart_entry->resource_group;
            }
            else
            {
                LOG(fmt::format("[RoR|Tuneup] WARN Addonpart '{}' not found in modcache!", tweak->tft_origin));
                return orig_val;
            }
        }
    }

    return orig_val;
}

bool RoR::TuneupUtil::isFlexbodyTweaked(TuneupDefPtr& tuneup_def, FlexbodyID_t flexbody_id, TuneupFlexbodyTweak*& out_tweak)
{
    

    if (tuneup_def)
    {
        auto itor = tuneup_def->flexbody_tweaks.find(flexbody_id);
        if (itor != tuneup_def->flexbody_tweaks.end())
        {
            out_tweak = &itor->second;
            return true;
        }
    }

    return false;
}

// > flare
bool RoR::TuneupUtil::isFlareAnyhowRemoved(TuneupDefPtr& tuneup_def, FlareID_t flare_id)
{    
    return tuneup_def 
        && (tuneup_def->isFlareUnwanted(flare_id) || tuneup_def->isFlareForceRemoved(flare_id));   
}

// > exhaust
bool RoR::TuneupUtil::isExhaustAnyhowRemoved(TuneupDefPtr& tuneup_def, ExhaustID_t exhaust_id)
{    
    return tuneup_def 
        && (tuneup_def->isExhaustUnwanted(exhaust_id) || tuneup_def->isExhaustForceRemoved(exhaust_id));   
}

// > managedmaterials
bool RoR::TuneupUtil::isManagedMatAnyhowRemoved(TuneupDefPtr& tuneup_def, const std::string& material_name)
{
    return tuneup_def
        && (tuneup_def->isManagedMatUnwanted(material_name) || tuneup_def->isManagedMatForceRemoved(material_name));
}

bool RoR::TuneupUtil::isManagedMatTweaked(TuneupDefPtr& tuneup_def, const std::string& material_name, TuneupManagedMatTweak*& out_tweak)
{
    if (tuneup_def)
    {
        auto itor = tuneup_def->managedmat_tweaks.find(material_name);
        if (itor != tuneup_def->managedmat_tweaks.end())
        {
            out_tweak = &itor->second;
            return true;
        }
    }

    return false;
}

std::string RoR::TuneupUtil::getTweakedManagedMatType(TuneupDefPtr& tuneup_def, const std::string& material_name, const std::string& orig_val)
{
    TuneupManagedMatTweak* tweak = nullptr;
    if (TuneupUtil::isManagedMatTweaked(tuneup_def, material_name, tweak))
    {
        ROR_ASSERT(tweak);
        return (tweak->tmt_type != "") ? tweak->tmt_type : orig_val;
    }
    else
    {
        return orig_val;
    }
}

std::string RoR::TuneupUtil::getTweakedManagedMatMedia(TuneupDefPtr& tuneup_def, const std::string& material_name, int media_idx, const std::string& orig_val)
{
    TuneupManagedMatTweak* tweak = nullptr;
    if (TuneupUtil::isManagedMatTweaked(tuneup_def, material_name, tweak))
    {
        ROR_ASSERT(tweak);
        ROR_ASSERT(tweak->tmt_media.size() > media_idx);
        return (tweak->tmt_media[media_idx] != "") ? tweak->tmt_media[media_idx] : orig_val;
    }
    else
    {
        return orig_val;
    }
}

std::string RoR::TuneupUtil::getTweakedManagedMatMediaRG(TuneupDefPtr& tuneup_def, const std::string& material_name, int media_idx, const std::string& orig_val)
{
    TuneupManagedMatTweak* tweak = nullptr;
    if (TuneupUtil::isManagedMatTweaked(tuneup_def, material_name, tweak))
    {
        ROR_ASSERT(tweak);
        ROR_ASSERT(tweak->tmt_media.size() > media_idx);
        if (tweak->tmt_media[media_idx] != "")
        {
            // Find the tweak addonpart
            CacheEntryPtr addonpart_entry = App::GetCacheSystem()->FindEntryByFilename(LT_AddonPart, /*partial:*/false, tweak->tmt_origin);
            if (addonpart_entry)
            {
                return addonpart_entry->resource_group;
            }
            else
            {
                LOG(fmt::format("[RoR|Tuneup] WARN managedmaterial '{}': Addonpart '{}' not found in modcache!", material_name, tweak->tmt_origin));
                return orig_val;
            }
        }
    }

    return orig_val;
}

bool RoR::TuneupUtil::isAddonPartUsed(TuneupDefPtr& tuneup_entry, const std::string& filename)
{
    return tuneup_entry
        && tuneup_entry->use_addonparts.find(filename) != tuneup_entry->use_addonparts.end();
}

std::vector<TuneupDefPtr> RoR::TuneupUtil::ParseTuneups(Ogre::DataStreamPtr& stream)
{
    std::vector<TuneupDefPtr> result;
    TuneupDefPtr curr_tuneup;
    try
    {
        while(!stream->eof())
        {
            std::string line = SanitizeUtf8String(stream->getLine());

            // Ignore blanks & comments
            if (!line.length() || line.substr(0, 2) == "//")
            {
                continue;
            }

            if (!curr_tuneup)
            {
                // No current tuneup -- So first valid data should be tuneup name
                Ogre::StringUtil::trim(line);
                curr_tuneup = new TuneupDef();
                curr_tuneup->name = line;
                stream->skipLine("{");
            }
            else
            {
                // Already in tuneup
                if (line == "}")
                {
                    result.push_back(curr_tuneup); // Finished
                    curr_tuneup = nullptr;
                }
                else
                {
                    RoR::TuneupUtil::ParseTuneupAttribute(line, curr_tuneup);
                }
            }
        }

        if (curr_tuneup)
        {
            App::GetConsole()->putMessage(
                Console::CONSOLE_MSGTYPE_ACTOR, Console::CONSOLE_SYSTEM_WARNING,
                fmt::format("Tuneup '{}' in file '{}' not properly closed with '}}'",
                    curr_tuneup->name, stream->getName()));
            result.push_back(curr_tuneup); // Submit anyway
            curr_tuneup = nullptr;
        }
    }
    catch (Ogre::Exception& e)
    {
        App::GetConsole()->putMessage(
            Console::CONSOLE_MSGTYPE_ACTOR, Console::CONSOLE_SYSTEM_WARNING,
            fmt::format("Error parsing tuneup file '{}', message: {}",
                stream->getName(), e.getFullDescription()));
    }
    return result;
}

void RoR::TuneupUtil::ParseTuneupAttribute(const std::string& line, TuneupDefPtr& tuneup_def) // static
{
    Ogre::StringVector params = Ogre::StringUtil::split(line, "\t=,;\n");
    for (unsigned int i=0; i < params.size(); i++)
    {
        Ogre::StringUtil::trim(params[i]);
    }
    Ogre::String& attrib = params[0];
    Ogre::StringUtil::toLowerCase(attrib);

    // General info
    if (attrib == "preview"         && params.size() >= 2) { tuneup_def->thumbnail = params[1]; return; }
    if (attrib == "description"     && params.size() >= 2) { tuneup_def->description = params[1]; return; }
    if (attrib == "author_name"     && params.size() >= 2) { tuneup_def->author_name = params[1]; return; }
    if (attrib == "author_id"       && params.size() == 2) { tuneup_def->author_id = PARSEINT(params[1]); return; }
    if (attrib == "category_id"     && params.size() == 2) { tuneup_def->category_id = (CacheCategoryId)PARSEINT(params[1]); return; }
    if (attrib == "guid"            && params.size() >= 2) { tuneup_def->guid = params[1]; Ogre::StringUtil::trim(tuneup_def->guid); Ogre::StringUtil::toLowerCase(tuneup_def->guid); return; }
    if (attrib == "name"            && params.size() >= 2) { tuneup_def->name = params[1]; Ogre::StringUtil::trim(tuneup_def->name); return; }
    if (attrib == "filename"        && params.size() >= 2) { tuneup_def->filename = params[1]; Ogre::StringUtil::trim(tuneup_def->filename); return; }

    // Addonparts and extracted data
    if (attrib == "use_addonpart"   && params.size() == 2) { tuneup_def->use_addonparts.insert(params[1]); return; }
    if (attrib == "unwanted_prop"     && params.size() == 2) { tuneup_def->unwanted_props.insert(PARSEINT(params[1])); return; }
    if (attrib == "unwanted_flexbody" && params.size() == 2) { tuneup_def->unwanted_flexbodies.insert(PARSEINT(params[1])); return; }
    if (attrib == "protected_prop"     && params.size() == 2) { tuneup_def->protected_props.insert(PARSEINT(params[1])); return; }
    if (attrib == "protected_flexbody" && params.size() == 2) { tuneup_def->protected_flexbodies.insert(PARSEINT(params[1])); return; }

    // UI overrides
    if (attrib == "forced_wheel_side" && params.size() == 3) { tuneup_def->force_wheel_sides[PARSEINT(params[1])] = (WheelSide)PARSEINT(params[2]); return; }
    if (attrib == "force_remove_prop" && params.size() == 2) { tuneup_def->force_remove_props.insert(PARSEINT(params[1])); return; }
    if (attrib == "force_remove_flexbody" && params.size() == 2) { tuneup_def->force_remove_flexbodies.insert(PARSEINT(params[1])); return; }
}

void RoR::TuneupUtil::ExportTuneup(Ogre::DataStreamPtr& stream, TuneupDefPtr& tuneup)
{
    Str<TUNEUP_BUF_SIZE> buf;
    buf << tuneup->name << "\n";
    buf << "{\n";

    // General info:
    buf << "\tpreview = "     << tuneup->thumbnail    << "\n";
    buf << "\tdescription = " << tuneup->description  << "\n";
    buf << "\tauthor_name = " << tuneup->author_name  << "\n";
    buf << "\tauthor_id = "   << tuneup->author_id    << "\n";
    buf << "\tcategory_id = " << (int)tuneup->category_id  << "\n";
    buf << "\tguid = "        << tuneup->guid         << "\n";
    buf << "\tfilename = "    << tuneup->filename     << "\n";
    buf << "\n";

    // Addonparts and extracted data:
    for (const std::string& addonpart: tuneup->use_addonparts)
    {
        buf << "\tuse_addonpart = " << addonpart << "\n";
    }
    for (PropID_t unwanted_prop: tuneup->unwanted_props)
    {
        buf << "\tunwanted_prop = " << (int)unwanted_prop << "\n";
    }
    for (FlexbodyID_t unwanted_flexbody: tuneup->unwanted_flexbodies)
    {
        buf << "\tunwanted_flexbody = " << (int)unwanted_flexbody << "\n";
    }
    for (PropID_t protected_prop: tuneup->protected_props)
    {
        buf << "\tprotected_prop = " << (int)protected_prop << "\n";
    }
    for (FlexbodyID_t protected_flexbody: tuneup->protected_flexbodies)
    {
        buf << "\tprotected_flexbody = " << (int)protected_flexbody << "\n";
    }

    // UI overrides:
    for (PropID_t prop: tuneup->force_remove_props)
    {
        buf << "\tforce_remove_prop = " << (int)prop << "\n";
    }
    for (FlexbodyID_t flexbody: tuneup->force_remove_flexbodies)
    {
        buf << "\tforce_remove_flexbody = " << (int)flexbody << "\n";
    }
    for (auto& pair: tuneup->force_wheel_sides)
    {
        buf << "\tforced_wheel_side = " << pair.first << ", " << (int)pair.second << "\n";
    }
    buf << "}\n\n";

    size_t written = stream->write(buf.GetBuffer(), buf.GetLength());
    if (written < buf.GetLength())
    {
        App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_WARNING,
            fmt::format("Error writing file '{}': only written {}/{} bytes", stream->getName(), written, buf.GetLength()));
    }
}

