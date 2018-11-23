/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2013-2019 Petr Ohlidal

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

/// @file   CacheSystem.h
/// @author Thomas Fischer, 21th of May 2008
/// @author Petr Ohlidal, 2018

#include "CacheSystem.h"

#include <OgreException.h>
#include "Application.h"
#include "BeamData.h"
#include "ContentManager.h"
#include "ErrorUtils.h"
#include "GUI_LoadingWindow.h"
#include "GUIManager.h"
#include "GfxActor.h"
#include "Language.h"
#include "PlatformUtils.h"
#include "RigDef_Parser.h"
#include "RoRFrameListener.h"
#include "Settings.h"
#include "SkinManager.h"
#include "TerrainManager.h"
#include "Terrn2Fileformat.h"
#include "Utils.h"

#include <OgreFileSystem.h>
#include <rapidjson/document.h>
#include <rapidjson/istreamwrapper.h>
#include <rapidjson/ostreamwrapper.h>
#include <rapidjson/writer.h>
#include <fstream>

using namespace Ogre;
using namespace RoR;

const CacheCategory CacheSystem::CATEGORIES[] = {
    {9991,   "All"},
    {9992,   "Fresh"},
    {129,    "AddonTerrains"},
    {114,    "Aircraft"},
    {150,    "Bikes"},
    {110,    "Boats"},
    {107,    "Buses"},
    {859,    "Container"},
    {155,    "Crawlers"},
    {149,    "FantasyCars"},
    {159,    "FantasyTrucks"},
    {156,    "Forklifts"},
    {113,    "Helicopters"},
    {9993,   "Hidden"},
    {147,    "LightRacingCars"},
    {153,    "MobileCranes"},
    {5001,   "NightTerrains"},
    {5000,   "OfficialTerrains"},
    {148,    "OffroadCars"},
    {162,    "OffroadTrucks"},
    {108,    "OtherLandVehicles"},
    {118,    "OtherLoads"},
    {154,    "Othercranes"},
    {161,    "RacingTrucks"},
    {146,    "StreetCars"},
    {875,    "Submarine"},
    {152,    "Towercranes"},
    {151,    "Tractors"},
    {117,    "Trailers"},
    {160,    "TransportTrucks"},
    {9990,   "Unsorted"},
};

CacheEntry::CacheEntry() :
    addtimestamp(0),
    beamcount(0),
    categoryid(0),
    commandscount(0),
    custom_particles(false),
    customtach(false),
    deleted(false),
    driveable(0), // driveable = 0 = NOT_DRIVEABLE
    enginetype('t'), // enginetype = t = truck is default
    exhaustscount(0),
    fileformatversion(0),
    filetime(0),
    fixescount(0),
    flarescount(0),
    flexbodiescount(0),
    forwardcommands(false),
    hasSubmeshs(false),
    hydroscount(0),
    importcommands(false),
    loadmass(0),
    maxrpm(0),
    minrpm(0),
    nodecount(0),
    number(0),
    numgears(0),
    propscount(0),
    propwheelcount(0),
    rescuer(false),
    rotatorscount(0),
    shockcount(0),
    soundsourcescount(0),
    torque(0),
    truckmass(0),
    turbojetcount(0),
    turbopropscount(0),
    usagecounter(0),
    version(0),
    wheelcount(0),
    wingscount(0)
{
}

CacheSystem::CacheSystem()
{
    // register the extensions
    m_known_extensions.push_back("machine");
    m_known_extensions.push_back("fixed");
    m_known_extensions.push_back("terrn2");
    m_known_extensions.push_back("truck");
    m_known_extensions.push_back("car");
    m_known_extensions.push_back("boat");
    m_known_extensions.push_back("airplane");
    m_known_extensions.push_back("trailer");
    m_known_extensions.push_back("load");
    m_known_extensions.push_back("train");
    m_known_extensions.push_back("skin");

    // Prepare lookup table
    for (size_t i = 0; i < NUM_CATEGORIES; ++i)
    {
        m_category_lookup.insert(std::make_pair(
            CacheSystem::CATEGORIES[i].ccg_id, &CacheSystem::CATEGORIES[i]));
    }
}

void CacheSystem::LoadModCache(CacheValidityState validity)
{
    m_resource_paths.clear();
    m_update_time = getTimeStamp();

    if (validity != CACHE_VALID)
    {
        if (validity == CACHE_NEEDS_REBUILD)
        {
            RoR::Log("[RoR|ModCache] Performing rebuild ...");
            this->ClearCache();
        }
        else
        {
            RoR::Log("[RoR|ModCache] Performing update ...");
            this->PruneCache();
        }
        App::diag_log_console_echo->SetActiveVal(false);
        this->ParseZipArchives(RGN_CONTENT);
        this->ParseKnownFiles(RGN_CONTENT);
        App::diag_log_console_echo->SetActiveVal(App::diag_log_console_echo->GetActiveVal<bool>());
        this->DetectDuplicates();
        this->WriteCacheFileJson();
    }

    this->LoadCacheFileJson();

    App::app_force_cache_purge->SetActiveVal(false);
    App::app_force_cache_udpate->SetActiveVal(false);

    RoR::Log("[RoR|ModCache] Cache loaded");
}

CacheEntry* CacheSystem::FindEntryByFilename(std::string filename)
{
    StringUtil::toLowerCase(filename);
    for (CacheEntry& entry : m_entries)
    {
        String fname = entry.fname;
        String fname_without_uid = entry.fname_without_uid;
        StringUtil::toLowerCase(fname);
        StringUtil::toLowerCase(fname_without_uid);
        if (fname == filename || fname_without_uid == filename)
            return &entry;
    }
    return nullptr;
}

void CacheSystem::UnloadActorFromMemory(std::string filename)
{
    CacheEntry* cache_entry = this->FindEntryByFilename(filename);
    if (cache_entry != nullptr)
    {
        cache_entry->actor_def.reset();
        String group = cache_entry->resource_group;
        if (!group.empty() && ResourceGroupManager::getSingleton().resourceGroupExists(group))
        {
            bool unused = true;
            for (auto gfx_actor : App::GetSimController()->GetGfxScene().GetGfxActors())
            {
                if (gfx_actor->GetResourceGroup() == group)
                {
                    unused = false;
                    break;
                }
            }
            if (unused)
            {
                ResourceGroupManager::getSingleton().destroyResourceGroup(group);
                m_loaded_resource_bundles[cache_entry->resource_bundle_path] = "";
            }
        }
    }
}

CacheSystem::CacheValidityState CacheSystem::EvaluateCacheValidity()
{
    this->GenerateHashFromFilenames();

    // First, open cache file and get hash for quick update check
    rapidjson::Document j_doc;
    if (!App::GetContentManager()->LoadAndParseJson(CACHE_FILE, RGN_CACHE, j_doc))
    {
        RoR::Log("[RoR|ModCache] Invalid or missing cache file");
        return CACHE_NEEDS_REBUILD;
    }

    if (j_doc["format_version"].GetInt() != CACHE_FILE_FORMAT)
    {
        RoR::Log("[RoR|ModCache] Invalid cache file format");
        return CACHE_NEEDS_REBUILD;
    }

    if (App::app_force_cache_purge->GetActiveVal<bool>())
    {
        RoR::Log("[RoR|ModCache] Cache rebuild requested");
        return CACHE_NEEDS_REBUILD;
    }

    if (j_doc["global_hash"].GetString() != m_filenames_hash)
    {
        RoR::Log("[RoR|ModCache] Cache file out of date");
        return CACHE_NEEDS_UPDATE;
    }

    if (App::app_force_cache_udpate->GetActiveVal<bool>())
    {
        RoR::Log("[RoR|ModCache] Cache update requested");
        return CACHE_NEEDS_UPDATE;
    }

    RoR::Log("[RoR|ModCache] Cache valid");
    return CACHE_VALID;
}

void CacheSystem::ImportEntryFromJson(rapidjson::Value& j_entry, CacheEntry & out_entry)
{
    // Common details
    out_entry.usagecounter =           j_entry["usagecounter"].GetInt();
    out_entry.addtimestamp =           j_entry["addtimestamp"].GetInt();
    out_entry.resource_bundle_type =   j_entry["resource_bundle_type"].GetString();
    out_entry.resource_bundle_path =   j_entry["resource_bundle_path"].GetString();
    out_entry.fpath =                  j_entry["fpath"].GetString();
    out_entry.fname =                  j_entry["fname"].GetString();
    out_entry.fname_without_uid =      j_entry["fname_without_uid"].GetString();
    out_entry.fext =                   j_entry["fext"].GetString();
    out_entry.filetime =               j_entry["filetime"].GetInt();
    out_entry.dname =                  j_entry["dname"].GetString();
    out_entry.uniqueid =               j_entry["uniqueid"].GetString();
    out_entry.version =                j_entry["version"].GetInt();
    out_entry.filecachename =          j_entry["filecachename"].GetString();

    out_entry.guid = j_entry["guid"].GetString();
    Ogre::StringUtil::trim(out_entry.guid);

    // Category
    int category_id = j_entry["categoryid"].GetInt();
    auto category_itor = m_category_lookup.find(category_id);
    if (category_itor == m_category_lookup.end() || category_id >= CID_Max)
    {
        category_itor = m_category_lookup.find(CID_Unsorted);
    }
    out_entry.categoryname = category_itor->second->ccg_name;
    out_entry.categoryid = category_itor->second->ccg_id;

     // Common - Authors
    for (rapidjson::Value& j_author: j_entry["authors"].GetArray())
    {
        AuthorInfo author;

        author.type  =  j_author["type"].GetString();
        author.name  =  j_author["name"].GetString();
        author.email =  j_author["email"].GetString();
        author.id    =  j_author["id"].GetInt();

        out_entry.authors.push_back(author);
    }

    // Vehicle details
    out_entry.description =       j_entry["description"].GetString();
    out_entry.tags =              j_entry["tags"].GetString();
    out_entry.fileformatversion = j_entry["fileformatversion"].GetInt();
    out_entry.hasSubmeshs =       j_entry["hasSubmeshs"].GetBool();
    out_entry.nodecount =         j_entry["nodecount"].GetInt();
    out_entry.beamcount =         j_entry["beamcount"].GetInt();
    out_entry.shockcount =        j_entry["shockcount"].GetInt();
    out_entry.fixescount =        j_entry["fixescount"].GetInt();
    out_entry.hydroscount =       j_entry["hydroscount"].GetInt();
    out_entry.wheelcount =        j_entry["wheelcount"].GetInt();
    out_entry.propwheelcount =    j_entry["propwheelcount"].GetInt();
    out_entry.commandscount =     j_entry["commandscount"].GetInt();
    out_entry.flarescount =       j_entry["flarescount"].GetInt();
    out_entry.propscount =        j_entry["propscount"].GetInt();
    out_entry.wingscount =        j_entry["wingscount"].GetInt();
    out_entry.turbopropscount =   j_entry["turbopropscount"].GetInt();
    out_entry.turbojetcount =     j_entry["turbojetcount"].GetInt();
    out_entry.rotatorscount =     j_entry["rotatorscount"].GetInt();
    out_entry.exhaustscount =     j_entry["exhaustscount"].GetInt();
    out_entry.flexbodiescount =   j_entry["flexbodiescount"].GetInt();
    out_entry.soundsourcescount = j_entry["soundsourcescount"].GetInt();
    out_entry.truckmass =         j_entry["truckmass"].GetFloat();
    out_entry.loadmass =          j_entry["loadmass"].GetFloat();
    out_entry.minrpm =            j_entry["minrpm"].GetFloat();
    out_entry.maxrpm =            j_entry["maxrpm"].GetFloat();
    out_entry.torque =            j_entry["torque"].GetFloat();
    out_entry.customtach =        j_entry["customtach"].GetBool();
    out_entry.custom_particles =  j_entry["custom_particles"].GetBool();
    out_entry.forwardcommands =   j_entry["forwardcommands"].GetBool();
    out_entry.importcommands =    j_entry["importcommands"].GetBool();
    out_entry.rescuer =           j_entry["rescuer"].GetBool();
    out_entry.driveable =         j_entry["driveable"].GetInt();
    out_entry.numgears =          j_entry["numgears"].GetInt();
    out_entry.enginetype =        static_cast<char>(j_entry["enginetype"].GetInt());

    // Vehicle 'section-configs' (aka Modules in RigDef namespace)
    for (rapidjson::Value& j_module_name: j_entry["sectionconfigs"].GetArray())
    {
        out_entry.sectionconfigs.push_back(j_module_name.GetString());
    }
}

void CacheSystem::LoadCacheFileJson()
{
    // Clear existing entries
    m_entries.clear();

    rapidjson::Document j_doc;
    if (!App::GetContentManager()->LoadAndParseJson(CACHE_FILE, RGN_CACHE, j_doc) ||
        !j_doc.IsObject() || !j_doc.HasMember("entries") || !j_doc["entries"].IsArray())
    {
        RoR::Log("[RoR|ModCache] Error, cache file still invalid after check/update, content selector will be empty.");
        return;
    }

    for (rapidjson::Value& j_entry: j_doc["entries"].GetArray())
    {
        CacheEntry entry;
        this->ImportEntryFromJson(j_entry, entry);
        entry.number = static_cast<int>(m_entries.size() + 1); // Let's number mods from 1
        m_entries.push_back(entry);
    }
}

void CacheSystem::PruneCache()
{
    this->LoadCacheFileJson();

    std::vector<String> paths;
    for (auto& entry : m_entries)
    {
        std::string fn = entry.resource_bundle_path;
        if (entry.resource_bundle_type == "FileSystem")
        {
            fn = PathCombine(fn, entry.fname);
        }

        if (!RoR::FileExists(fn.c_str()) || (entry.filetime != RoR::GetFileLastModifiedTime(fn)))
        {
            if (!entry.deleted)
            {
                if (std::find(paths.begin(), paths.end(), fn) == paths.end())
                {
                    RoR::LogFormat("[RoR|ModCache] Removing '%s'", fn.c_str());
                    paths.push_back(fn);
                }
                this->RemoveFileCache(entry);
            }
            entry.deleted = true;
        }
        else
        {
            m_resource_paths.insert(fn);
        }
    }
}

void CacheSystem::DetectDuplicates()
{
    RoR::Log("[RoR|ModCache] Searching for duplicates ...");
    std::map<String, String> possible_duplicates;
    for (int i=0; i<m_entries.size(); i++) 
    {
        if (m_entries[i].deleted)
            continue;

        String dnameA = m_entries[i].dname;
        StringUtil::toLowerCase(dnameA);
        StringUtil::trim(dnameA);
        String dirA = m_entries[i].resource_bundle_path;
        StringUtil::toLowerCase(dirA);
        String basenameA, basepathA;
        StringUtil::splitFilename(dirA, basenameA, basepathA);
        String filenameWUIDA = m_entries[i].fname_without_uid;
        StringUtil::toLowerCase(filenameWUIDA);

        for (int j=i+1; j<m_entries.size(); j++) 
        {
            if (m_entries[j].deleted)
                continue;

            String filenameWUIDB = m_entries[j].fname_without_uid;
            StringUtil::toLowerCase(filenameWUIDB);
            if (filenameWUIDA != filenameWUIDB)
                continue;

            String dnameB = m_entries[j].dname;
            StringUtil::toLowerCase(dnameB);
            StringUtil::trim(dnameB);
            if (dnameA != dnameB)
                continue;

            String dirB = m_entries[j].resource_bundle_path;
            StringUtil::toLowerCase(dirB);
            String basenameB, basepathB;
            StringUtil::splitFilename(dirB, basenameB, basepathB);
            basenameA = Ogre::StringUtil::replaceAll(basenameA, " ", "_");
            basenameA = Ogre::StringUtil::replaceAll(basenameA, "-", "_");
            basenameB = Ogre::StringUtil::replaceAll(basenameB, " ", "_");
            basenameB = Ogre::StringUtil::replaceAll(basenameB, "-", "_");
            if (StripSHA1fromString(basenameA) != StripSHA1fromString(basenameB))
                continue;

            if (m_entries[i].resource_bundle_path == m_entries[j].resource_bundle_path)
            {
                LOG("- duplicate: " + m_entries[i].fpath + m_entries[i].fname
                             + " <--> " + m_entries[j].fpath + m_entries[j].fname);
                LOG("  - " + m_entries[j].resource_bundle_path);
                int idx = m_entries[i].fpath.size() < m_entries[j].fpath.size() ? i : j;
                m_entries[idx].deleted = true;
            }
            else
            {
                possible_duplicates[m_entries[i].resource_bundle_path] = m_entries[j].resource_bundle_path;
            }
        }
    }
    for (auto duplicate : possible_duplicates)
    {
        LOG("- possible duplicate: ");
        LOG("  - " + duplicate.first);
        LOG("  - " + duplicate.second);
    }
}

CacheEntry* CacheSystem::GetEntry(int modid)
{
    for (std::vector<CacheEntry>::iterator it = m_entries.begin(); it != m_entries.end(); it++)
    {
        if (modid == it->number)
            return &(*it);
    }
    return 0;
}

String CacheSystem::GetPrettyName(String fname)
{
    for (std::vector<CacheEntry>::iterator it = m_entries.begin(); it != m_entries.end(); it++)
    {
        if (fname == it->fname)
            return it->dname;
    }
    return "";
}

void CacheSystem::ExportEntryToJson(rapidjson::Value& j_entries, rapidjson::Document& j_doc, CacheEntry const & entry)
{
    rapidjson::Value j_entry(rapidjson::kObjectType);

    // Common details
    j_entry.AddMember("usagecounter",         entry.usagecounter,                                          j_doc.GetAllocator());
    j_entry.AddMember("addtimestamp",         static_cast<int64_t>(entry.addtimestamp),                    j_doc.GetAllocator());
    j_entry.AddMember("resource_bundle_type", rapidjson::StringRef(entry.resource_bundle_type.c_str()),    j_doc.GetAllocator());
    j_entry.AddMember("resource_bundle_path", rapidjson::StringRef(entry.resource_bundle_path.c_str()),    j_doc.GetAllocator());
    j_entry.AddMember("fpath",                rapidjson::StringRef(entry.fpath.c_str()),                   j_doc.GetAllocator());
    j_entry.AddMember("fname",                rapidjson::StringRef(entry.fname.c_str()),                   j_doc.GetAllocator());
    j_entry.AddMember("fname_without_uid",    rapidjson::StringRef(entry.fname_without_uid.c_str()),       j_doc.GetAllocator());
    j_entry.AddMember("fext",                 rapidjson::StringRef(entry.fext.c_str()),                    j_doc.GetAllocator());
    j_entry.AddMember("filetime",             static_cast<int64_t>(entry.filetime),                        j_doc.GetAllocator()); 
    j_entry.AddMember("dname",                rapidjson::StringRef(entry.dname.c_str()),                   j_doc.GetAllocator());
    j_entry.AddMember("categoryid",           entry.categoryid,                                            j_doc.GetAllocator());
    j_entry.AddMember("uniqueid",             rapidjson::StringRef(entry.uniqueid.c_str()),                j_doc.GetAllocator());
    j_entry.AddMember("guid",                 rapidjson::StringRef(entry.guid.c_str()),                    j_doc.GetAllocator());
    j_entry.AddMember("version",              entry.version,                                               j_doc.GetAllocator());
    j_entry.AddMember("filecachename",        rapidjson::StringRef(entry.filecachename.c_str()),           j_doc.GetAllocator());

    // Common - Authors
    rapidjson::Value j_authors(rapidjson::kArrayType);
    for (AuthorInfo const& author: entry.authors)
    {
        rapidjson::Value j_author(rapidjson::kObjectType);

        j_author.AddMember("type",   rapidjson::StringRef(author.type.c_str()),   j_doc.GetAllocator());
        j_author.AddMember("name",   rapidjson::StringRef(author.name.c_str()),   j_doc.GetAllocator());
        j_author.AddMember("email",  rapidjson::StringRef(author.email.c_str()),  j_doc.GetAllocator());
        j_author.AddMember("id",     author.id,                                   j_doc.GetAllocator());

        j_authors.PushBack(j_author, j_doc.GetAllocator());
    }
    j_entry.AddMember("authors", j_authors, j_doc.GetAllocator());

    // Vehicle details
    j_entry.AddMember("description",         rapidjson::StringRef(entry.description.c_str()),       j_doc.GetAllocator());
    j_entry.AddMember("tags",                rapidjson::StringRef(entry.tags.c_str()),              j_doc.GetAllocator());
    j_entry.AddMember("fileformatversion",   entry.fileformatversion, j_doc.GetAllocator());
    j_entry.AddMember("hasSubmeshs",         entry.hasSubmeshs,       j_doc.GetAllocator());
    j_entry.AddMember("nodecount",           entry.nodecount,         j_doc.GetAllocator());
    j_entry.AddMember("beamcount",           entry.beamcount,         j_doc.GetAllocator());
    j_entry.AddMember("shockcount",          entry.shockcount,        j_doc.GetAllocator());
    j_entry.AddMember("fixescount",          entry.fixescount,        j_doc.GetAllocator());
    j_entry.AddMember("hydroscount",         entry.hydroscount,       j_doc.GetAllocator());
    j_entry.AddMember("wheelcount",          entry.wheelcount,        j_doc.GetAllocator());
    j_entry.AddMember("propwheelcount",      entry.propwheelcount,    j_doc.GetAllocator());
    j_entry.AddMember("commandscount",       entry.commandscount,     j_doc.GetAllocator());
    j_entry.AddMember("flarescount",         entry.flarescount,       j_doc.GetAllocator());
    j_entry.AddMember("propscount",          entry.propscount,        j_doc.GetAllocator());
    j_entry.AddMember("wingscount",          entry.wingscount,        j_doc.GetAllocator());
    j_entry.AddMember("turbopropscount",     entry.turbopropscount,   j_doc.GetAllocator());
    j_entry.AddMember("turbojetcount",       entry.turbojetcount,     j_doc.GetAllocator());
    j_entry.AddMember("rotatorscount",       entry.rotatorscount,     j_doc.GetAllocator());
    j_entry.AddMember("exhaustscount",       entry.exhaustscount,     j_doc.GetAllocator());
    j_entry.AddMember("flexbodiescount",     entry.flexbodiescount,   j_doc.GetAllocator());
    j_entry.AddMember("soundsourcescount",   entry.soundsourcescount, j_doc.GetAllocator());
    j_entry.AddMember("truckmass",           entry.truckmass,         j_doc.GetAllocator());
    j_entry.AddMember("loadmass",            entry.loadmass,          j_doc.GetAllocator());
    j_entry.AddMember("minrpm",              entry.minrpm,            j_doc.GetAllocator());
    j_entry.AddMember("maxrpm",              entry.maxrpm,            j_doc.GetAllocator());
    j_entry.AddMember("torque",              entry.torque,            j_doc.GetAllocator());
    j_entry.AddMember("customtach",          entry.customtach,        j_doc.GetAllocator());
    j_entry.AddMember("custom_particles",    entry.custom_particles,  j_doc.GetAllocator());
    j_entry.AddMember("forwardcommands",     entry.forwardcommands,   j_doc.GetAllocator());
    j_entry.AddMember("importcommands",      entry.importcommands,    j_doc.GetAllocator());
    j_entry.AddMember("rescuer",             entry.rescuer,           j_doc.GetAllocator());
    j_entry.AddMember("driveable",           entry.driveable,         j_doc.GetAllocator());
    j_entry.AddMember("numgears",            entry.numgears,          j_doc.GetAllocator());
    j_entry.AddMember("enginetype",          entry.enginetype,        j_doc.GetAllocator());

    // Vehicle 'section-configs' (aka Modules in RigDef namespace)
    rapidjson::Value j_sectionconfigs(rapidjson::kArrayType);
    for (std::string const & module_name: entry.sectionconfigs)
    {
        j_sectionconfigs.PushBack(rapidjson::StringRef(module_name.c_str()), j_doc.GetAllocator());
    }
    j_entry.AddMember("sectionconfigs", j_sectionconfigs, j_doc.GetAllocator());

    // Add entry to list
    j_entries.PushBack(j_entry, j_doc.GetAllocator());
}

void CacheSystem::WriteCacheFileJson()
{
    // Basic file structure
    rapidjson::Document j_doc;
    j_doc.SetObject();
    j_doc.AddMember("format_version", CACHE_FILE_FORMAT, j_doc.GetAllocator());
    j_doc.AddMember("global_hash", rapidjson::StringRef(m_filenames_hash.c_str()), j_doc.GetAllocator());

    // Entries
    rapidjson::Value j_entries(rapidjson::kArrayType);
    for (CacheEntry const& entry : m_entries)
    {
        if (!entry.deleted)
        {
            this->ExportEntryToJson(j_entries, j_doc, entry);
        }
    }
    j_doc.AddMember("entries", j_entries, j_doc.GetAllocator());

    // Write to file
    if (App::GetContentManager()->SerializeAndWriteJson(CACHE_FILE, RGN_CACHE, j_doc)) // Logs errors
    {
        RoR::LogFormat("[RoR|ModCache] File '%s' written OK", CACHE_FILE);
    }
}

void CacheSystem::ClearCache()
{
    App::GetContentManager()->DeleteDiskFile(CACHE_FILE, RGN_CACHE);
    for (auto& entry : m_entries)
    {
        String group = entry.resource_group;
        if (!group.empty())
        {
            if (ResourceGroupManager::getSingleton().resourceGroupExists(group))
                ResourceGroupManager::getSingleton().destroyResourceGroup(group);
        }
        this->RemoveFileCache(entry);
    }
    m_entries.clear();
}

Ogre::String CacheSystem::StripUIDfromString(Ogre::String uidstr)
{
    size_t pos = uidstr.find("-");
    if (pos != String::npos && pos >= 3 && uidstr.substr(pos - 3, 3) == "UID")
        return uidstr.substr(pos + 1, uidstr.length() - pos);
    return uidstr;
}

Ogre::String CacheSystem::StripSHA1fromString(Ogre::String sha1str)
{
    size_t pos = sha1str.find_first_of("-_");
    if (pos != String::npos && pos >= 20)
        return sha1str.substr(pos + 1, sha1str.length() - pos);
    return sha1str;
}

void CacheSystem::AddFile(String group, Ogre::FileInfo f, String ext)
{
    String type = f.archive ? f.archive->getType() : "FileSystem";
    String path = f.archive ? f.archive->getName() : "";

    if (std::find_if(m_entries.begin(), m_entries.end(), [&](CacheEntry& e)
                { return !e.deleted && e.fname == f.filename && e.resource_bundle_path == path; }) != m_entries.end())
        return;

    RoR::LogFormat("[RoR|CacheSystem] Preparing to add file '%f'", f.filename.c_str());

    try
    {
        DataStreamPtr ds = ResourceGroupManager::getSingleton().openResource(f.filename, group);
        // ds closes automatically, so do _not_ close it explicitly below

        std::vector<CacheEntry> new_entries;
        if (ext == "terrn2")
        {
            new_entries.resize(1);
            FillTerrainDetailInfo(new_entries.back(), ds, f.filename);
        }
        else if (ext == "skin")
        {
            auto new_skins = RoR::SkinParser::ParseSkins(ds);
            for (auto skin_def: new_skins)
            {
                CacheEntry entry;
                if (!skin_def->author_name.empty())
                {
                    AuthorInfo a;
                    a.id = skin_def->author_id;
                    a.name = skin_def->author_name;
                    entry.authors.push_back(a);
                }

                entry.dname       = skin_def->name;
                entry.guid        = skin_def->guid;
                entry.description = skin_def->description;
                entry.categoryid  = -1;
                entry.skin_def    = skin_def; // Needed to generate preview image

                new_entries.push_back(entry);
            }
        }
        else
        {
            new_entries.resize(1);
            FillTruckDetailInfo(new_entries.back(), ds, f.filename, group);
        }

        for (auto& entry: new_entries)
        {
            Ogre::StringUtil::toLowerCase(entry.guid); // Important for comparsion
            entry.fpath = f.path;
            entry.fname = f.filename;
            entry.fname_without_uid = StripUIDfromString(f.filename);
            entry.fext = ext;
            if (type == "Zip")
            {
                entry.filetime = RoR::GetFileLastModifiedTime(path);
            }
            else
            {
                entry.filetime = RoR::GetFileLastModifiedTime(PathCombine(path, f.filename));
            }
            entry.resource_bundle_type = type;
            entry.resource_bundle_path = path;
            entry.number = static_cast<int>(m_entries.size() + 1); // Let's number mods from 1
            entry.addtimestamp = m_update_time;
            this->GenerateFileCache(entry, group);
            m_entries.push_back(entry);
        }
    }
    catch (Ogre::Exception& e)
    {
        RoR::LogFormat("[RoR|CacheSystem] Error processing file '%s', message :%s",
            f.filename.c_str(), e.getFullDescription().c_str());
    }
}

void CacheSystem::FillTruckDetailInfo(CacheEntry& entry, Ogre::DataStreamPtr stream, String file_name, String group)
{
    /* LOAD AND PARSE THE VEHICLE */
    RigDef::Parser parser;
    parser.Prepare();
    parser.ProcessOgreStream(stream.getPointer(), group);
    parser.GetSequentialImporter()->Disable();
    parser.Finalize();

    /* RETRIEVE DATA */

    std::shared_ptr<RigDef::File> def = parser.GetFile();

    /* Name */
    if (!def->name.empty())
    {
        entry.dname = def->name; // Use retrieved name
    }
    else
    {
        entry.dname = "@" + file_name; // Fallback
    }

    /* Description */
    std::vector<Ogre::String>::iterator desc_itor = def->description.begin();
    for (; desc_itor != def->description.end(); desc_itor++)
    {
        entry.description += *desc_itor + "\n";
    }

    /* Authors */
    std::vector<RigDef::Author>::iterator author_itor = def->authors.begin();
    for (; author_itor != def->authors.end(); author_itor++)
    {
        AuthorInfo author;
        author.email = author_itor->email;
        author.id    = author_itor->forum_account_id;
        author.name  = author_itor->name;
        author.type  = author_itor->type;

        entry.authors.push_back(author);
    }

    /* Modules (previously called "sections") */
    std::map<Ogre::String, std::shared_ptr<RigDef::File::Module>>::iterator module_itor = def->user_modules.begin();
    for (; module_itor != def->user_modules.end(); module_itor++)
    {
        entry.sectionconfigs.push_back(module_itor->second->name);
    }

    /* Engine */
    /* TODO: Handle engines in modules */
    if (def->root_module->engine != nullptr)
    {
        std::shared_ptr<RigDef::Engine> engine = def->root_module->engine;
        entry.numgears = static_cast<int>(engine->gear_ratios.size());
        entry.minrpm = engine->shift_down_rpm;
        entry.maxrpm = engine->shift_up_rpm;
        entry.torque = engine->torque;
        entry.enginetype = 't'; /* Truck (default) */
        if (def->root_module->engoption != nullptr
            && def->root_module->engoption->type == RigDef::Engoption::ENGINE_TYPE_c_CAR)
        {
            entry.enginetype = 'c';
        }
    }

    /* File info */
    if (def->file_info != nullptr)
    {
        entry.uniqueid = def->file_info->unique_id;
        entry.categoryid = static_cast<int>(def->file_info->category_id);
        entry.version = static_cast<int>(def->file_info->file_version);
    }
    else
    {
        entry.uniqueid = "-1";
        entry.categoryid = -1;
        entry.version = -1;
    }

    /* Vehicle type */
    /* NOTE: RigDef::File allows modularization of vehicle type. Cache only supports single type.
        This is a temporary solution which has undefined results for mixed-type vehicles.
    */
    int vehicle_type = NOT_DRIVEABLE;
    module_itor = def->user_modules.begin();
    for (; module_itor != def->user_modules.end(); module_itor++)
    {
        if (module_itor->second->engine != nullptr)
        {
            vehicle_type = TRUCK;
        }
        else if (module_itor->second->screwprops.size() > 0)
        {
            vehicle_type = BOAT;
        }
        /* Note: Sections 'turboprops' and 'turboprops2' are unified in TruckParser2013 */
        else if (module_itor->second->turbojets.size() > 0 || module_itor->second->pistonprops.size() > 0 || module_itor->second->turboprops_2.size() > 0)
        {
            vehicle_type = AIRPLANE;
        }
    }
    /* Root module */
    if (def->root_module->engine != nullptr)
    {
        vehicle_type = TRUCK;
    }
    else if (def->root_module->screwprops.size() > 0)
    {
        vehicle_type = BOAT;
    }
    /* Note: Sections 'turboprops' and 'turboprops2' are unified in TruckParser2013 */
    else if (def->root_module->turbojets.size() > 0 || def->root_module->pistonprops.size() > 0 || def->root_module->turboprops_2.size() > 0)
    {
        vehicle_type = AIRPLANE;
    }

    if (def->root_module->globals)
    {
        entry.truckmass = def->root_module->globals->dry_mass;
        entry.loadmass = def->root_module->globals->cargo_mass;
    }
    
    entry.forwardcommands = def->forward_commands;
    entry.importcommands = def->import_commands;
    entry.rescuer = def->rescuer;
    entry.guid = def->guid;
    entry.fileformatversion = def->file_format_version;
    entry.hasSubmeshs = static_cast<int>(def->root_module->submeshes.size() > 0);
    entry.nodecount = static_cast<int>(def->root_module->nodes.size());
    entry.beamcount = static_cast<int>(def->root_module->beams.size());
    entry.shockcount = static_cast<int>(def->root_module->shocks.size() + def->root_module->shocks_2.size());
    entry.fixescount = static_cast<int>(def->root_module->fixes.size());
    entry.hydroscount = static_cast<int>(def->root_module->hydros.size());
    entry.driveable = vehicle_type;
    entry.commandscount = static_cast<int>(def->root_module->commands_2.size());
    entry.flarescount = static_cast<int>(def->root_module->flares_2.size());
    entry.propscount = static_cast<int>(def->root_module->props.size());
    entry.wingscount = static_cast<int>(def->root_module->wings.size());
    entry.turbopropscount = static_cast<int>(def->root_module->turboprops_2.size());
    entry.rotatorscount = static_cast<int>(def->root_module->rotators.size() + def->root_module->rotators_2.size());
    entry.exhaustscount = static_cast<int>(def->root_module->exhausts.size());
    entry.custom_particles = def->root_module->particles.size() > 0;
    entry.turbojetcount = static_cast<int>(def->root_module->turbojets.size());
    entry.flexbodiescount = static_cast<int>(def->root_module->flexbodies.size());
    entry.soundsourcescount = static_cast<int>(def->root_module->soundsources.size() + def->root_module->soundsources.size());

    entry.wheelcount = 0;
    entry.propwheelcount = 0;
    for (const auto& w : def->root_module->wheels)
    {
        entry.wheelcount++;
        if (w.propulsion != RigDef::Wheels::PROPULSION_NONE)
            entry.propwheelcount++;
    }
    for (const auto& w : def->root_module->wheels_2)
    {
        entry.wheelcount++;
        if (w.propulsion != RigDef::Wheels::PROPULSION_NONE)
            entry.propwheelcount++;
    }
    for (const auto& w : def->root_module->mesh_wheels)
    {
        entry.wheelcount++;
        if (w.propulsion != RigDef::Wheels::PROPULSION_NONE)
            entry.propwheelcount++;
    }
    for (const auto& w : def->root_module->flex_body_wheels)
    {
        entry.wheelcount++;
        if (w.propulsion != RigDef::Wheels::PROPULSION_NONE)
            entry.propwheelcount++;
    }

    if (!def->root_module->axles.empty())
    {
        entry.propwheelcount = static_cast<int>(def->root_module->axles.size() * 2);
    }

    /* NOTE: std::shared_ptr cleans everything up. */
}

Ogre::String detectMiniType(String filename, String group)
{
    if (ResourceGroupManager::getSingleton().resourceExists(group, filename + "dds"))
        return "dds";

    if (ResourceGroupManager::getSingleton().resourceExists(group, filename + "png"))
        return "png";

    if (ResourceGroupManager::getSingleton().resourceExists(group, filename + "jpg"))
        return "jpg";

    return "";
}

void CacheSystem::RemoveFileCache(CacheEntry& entry)
{
    if (!entry.filecachename.empty())
    {
        App::GetContentManager()->DeleteDiskFile(entry.filecachename, RGN_CACHE);
    }
}

void CacheSystem::GenerateFileCache(CacheEntry& entry, String group)
{
    if (entry.fname.empty())
        return;

    String bundle_basename, bundle_path;
    StringUtil::splitFilename(entry.resource_bundle_path, bundle_basename, bundle_path);

    String src_path;
    String dst_path;
    if (entry.fext == "skin")
    {
        if (entry.skin_def->thumbnail.empty())
            return;
        src_path = entry.skin_def->thumbnail;
        String mini_fbase, minitype;
        StringUtil::splitBaseFilename(entry.skin_def->thumbnail, mini_fbase, minitype);
        dst_path = bundle_basename + "_" + mini_fbase + ".mini." + minitype;
    }
    else
    {
        String fbase, fext;
        StringUtil::splitBaseFilename(entry.fname, fbase, fext);
        String minifn = fbase + "-mini.";
        String minitype = detectMiniType(minifn, group);
        if (minitype.empty())
            return;
        src_path = minifn + minitype;
        dst_path = bundle_basename + "_" + entry.fname + ".mini." + minitype;
    }

    try
    {
        DataStreamPtr src_ds = ResourceGroupManager::getSingleton().openResource(src_path, group);
        DataStreamPtr dst_ds = ResourceGroupManager::getSingleton().createResource(dst_path, RGN_CACHE, true);
        std::vector<char> buf(src_ds->size());
        size_t read = src_ds->read(buf.data(), src_ds->size());
        if (read > 0)
        {
            dst_ds->write(buf.data(), read); 
            entry.filecachename = dst_path;
        }
    }
    catch (Ogre::Exception& e)
    {
        LOG("error while generating file cache: " + e.getFullDescription());
    }

    LOG("done generating file cache!");
}

void CacheSystem::ParseZipArchives(String group)
{
    auto files = ResourceGroupManager::getSingleton().findResourceFileInfo(group, "*.zip");
    auto skinzips = ResourceGroupManager::getSingleton().findResourceFileInfo(group, "*.skinzip");
    for (const auto& skinzip : *skinzips)
        files->push_back(skinzip);

    int i = 0, count = static_cast<int>(files->size());
    for (const auto& file : *files)
    {
        int progress = ((float)i++ / (float)count) * 100;
        UTFString tmp = _L("Loading zips in group ") + ANSI_TO_UTF(group) + L"\n" +
            ANSI_TO_UTF(file.filename) + L"\n" + ANSI_TO_UTF(TOSTRING(i)) + L"/" + ANSI_TO_UTF(TOSTRING(count));
        RoR::App::GetGuiManager()->GetLoadingWindow()->setProgress(progress, tmp);

        String path = PathCombine(file.archive->getName(), file.filename);
        this->ParseSingleZip(path);
    }

    RoR::App::GetGuiManager()->SetVisible_LoadingWindow(false);
}

void CacheSystem::ParseSingleZip(String path)
{
    if (std::find(m_resource_paths.begin(), m_resource_paths.end(), path) == m_resource_paths.end())
    {
        RoR::LogFormat("[RoR|ModCache] Adding archive '%s'", path.c_str());
        ResourceGroupManager::getSingleton().createResourceGroup(RGN_TEMP, false);
        try
        {
            ResourceGroupManager::getSingleton().addResourceLocation(path, "Zip", RGN_TEMP);
            if (ParseKnownFiles(RGN_TEMP))
            {
                LOG("No usable content in: '" + path + "'");
            }
        }
        catch (Ogre::Exception& e)
        {
            LOG("Error while opening archive: '" + path + "': " + e.getFullDescription());
        }
        ResourceGroupManager::getSingleton().destroyResourceGroup(RGN_TEMP);
        m_resource_paths.insert(path);
    }
}

bool CacheSystem::ParseKnownFiles(Ogre::String group)
{
    bool empty = true;
    for (auto ext : m_known_extensions)
    {
        auto files = ResourceGroupManager::getSingleton().findResourceFileInfo(group, "*." + ext);
        for (const auto& file : *files)
        {
            this->AddFile(group, file, ext);
            empty = false;
        }
    }
    return empty;
}

void CacheSystem::GenerateHashFromFilenames()
{
    std::string filenames = App::GetContentManager()->ListAllUserContent();
    m_filenames_hash = HashData(filenames.c_str(), static_cast<int>(filenames.size()));
}

void CacheSystem::FillTerrainDetailInfo(CacheEntry& entry, Ogre::DataStreamPtr ds, Ogre::String fname)
{
    Terrn2Def def;
    Terrn2Parser parser;
    parser.LoadTerrn2(def, ds);

    for (Terrn2Author& author : def.authors)
    {
        AuthorInfo a;
        a.id = -1;
        a.name = author.name;
        a.type = author.type;
        entry.authors.push_back(a);
    }

    entry.dname      = def.name;
    entry.categoryid = def.category_id;
    entry.uniqueid   = def.guid;
    entry.version    = def.version;
}

bool CacheSystem::CheckResourceLoaded(Ogre::String & filename)
{
    Ogre::String group = "";
    return CheckResourceLoaded(filename, group);
}

bool CacheSystem::CheckResourceLoaded(Ogre::String & filename, Ogre::String& group)
{
    try
    {
        // check if we already loaded it via ogre ...
        if (ResourceGroupManager::getSingleton().resourceExistsInAnyGroup(filename))
        {
            group = ResourceGroupManager::getSingleton().findGroupContainingResource(filename);
            return true;
        }

        for (auto& entry : m_entries)
        {
            // case insensitive comparison
            String fname = entry.fname;
            String fname_without_uid = entry.fname_without_uid;
            StringUtil::toLowerCase(fname);
            StringUtil::toLowerCase(filename);
            StringUtil::toLowerCase(fname_without_uid);
            if (fname == filename || fname_without_uid == filename)
            {
                // we found the file, load it
                LoadResource(entry);
                filename = entry.fname;
                group = entry.resource_group;
                return !group.empty() && ResourceGroupManager::getSingleton().resourceExists(group, filename);
            }
        }
    }
    catch (Ogre::Exception& oex) {} // Already logged by OGRE

    return false;
}

void CacheSystem::LoadResource(CacheEntry& t)
{
    if (!m_loaded_resource_bundles[t.resource_bundle_path].empty())
    {
        t.resource_group = m_loaded_resource_bundles[t.resource_bundle_path];
        return;
    }

    static int rg_counter = 0;
    String group = std::to_string(rg_counter++) + "-" + t.fname;

    try
    {
        if (t.fext == "terrn2")
        {
            // PagedGeometry is hardcoded to use `Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME`
            ResourceGroupManager::getSingleton().createResourceGroup(group, /*inGlobalPool=*/true);
            ResourceGroupManager::getSingleton().addResourceLocation(t.resource_bundle_path, t.resource_bundle_type, group);
        }
        else if (t.fext == "skin")
        {
            // This is a SkinZip bundle - use `inGlobalPool=false` to prevent resource name conflicts.
            ResourceGroupManager::getSingleton().createResourceGroup(group, /*inGlobalPool=*/false);
            ResourceGroupManager::getSingleton().addResourceLocation(t.resource_bundle_path, t.resource_bundle_type, group);
            App::GetContentManager()->InitManagedMaterials(group);
        }
        else
        {
            // A vehicle bundle - use `inGlobalPool=false` to prevent resource name conflicts.
            // See bottom 'note' at https://ogrecave.github.io/ogre/api/latest/_resource-_management.html#Resource-Groups
            ResourceGroupManager::getSingleton().createResourceGroup(group, /*inGlobalPool=*/false);
            ResourceGroupManager::getSingleton().addResourceLocation(t.resource_bundle_path, t.resource_bundle_type, group);

            App::GetContentManager()->InitManagedMaterials(group);
            App::GetContentManager()->AddResourcePack(ContentManager::ResourcePack::TEXTURES, group);
            App::GetContentManager()->AddResourcePack(ContentManager::ResourcePack::MATERIALS, group);
            App::GetContentManager()->AddResourcePack(ContentManager::ResourcePack::MESHES, group);
        }

        ResourceGroupManager::getSingleton().initialiseResourceGroup(group);

        t.resource_group = group;
        m_loaded_resource_bundles[t.resource_bundle_path] = group;
    }
    catch (Ogre::Exception& e)
    {
        RoR::LogFormat("[RoR] Error while loading '%s', message: %s",
            t.resource_bundle_path.c_str(), e.getFullDescription().c_str());
        if (ResourceGroupManager::getSingleton().resourceGroupExists(group))
        {
            ResourceGroupManager::getSingleton().destroyResourceGroup(group);
        }
    }
}

CacheEntry* CacheSystem::FetchSkinByName(std::string const & skin_name)
{
    for (CacheEntry & entry: m_entries)
    {
        if (entry.dname == skin_name && entry.fext == "skin")
        {
            return &entry;
        }
    }
    return nullptr;
}

std::shared_ptr<SkinDef> CacheSystem::FetchSkinDef(CacheEntry* cache_entry)
{
    if (cache_entry->skin_def != nullptr) // If already parsed, re-use
    {
        return cache_entry->skin_def;
    }

    try
    {
        App::GetCacheSystem()->LoadResource(*cache_entry); // Load if not already
        Ogre::DataStreamPtr ds = Ogre::ResourceGroupManager::getSingleton()
            .openResource(cache_entry->fname, cache_entry->resource_group);

        auto new_skins = RoR::SkinParser::ParseSkins(ds); // Load the '.skin' file
        for (auto def: new_skins)
        {
            for (CacheEntry& e: m_entries)
            {
                if (e.resource_bundle_path == cache_entry->resource_bundle_path
                    && e.resource_bundle_type == cache_entry->resource_bundle_type
                    && e.fname == cache_entry->fname
                    && e.dname == def->name)
                {
                    e.skin_def = def;
                    e.resource_group = cache_entry->resource_group;
                }
            }
        }

        if (cache_entry->skin_def == nullptr)
        {
            RoR::LogFormat("Definition of skin '%s' was not found in file '%s'",
               cache_entry->dname.c_str(), cache_entry->fname.c_str());
        }
        return cache_entry->skin_def;
    }
    catch (Ogre::Exception& oex)
    {
        RoR::LogFormat("[RoR] Error loading skin file '%s', message: %s",
            cache_entry->fname.c_str(), oex.getFullDescription().c_str());
        return nullptr;
    }
}

size_t CacheSystem::Query(CacheQuery& query)
{
    Ogre::StringUtil::toLowerCase(query.cqy_search_string);
    for (CacheEntry& entry: m_entries)
    {
        // Filter by GUID
        if (!query.cqy_filter_guid.empty() && entry.guid != query.cqy_filter_guid)
        {
            continue;
        }

        // Filter by entry type
        bool add = false;
        if (entry.fext == "terrn2")
            add = (query.cqy_filter_type == LT_Terrain);
        if (entry.fext == "skin")
            add = (query.cqy_filter_type == LT_Skin);
        else if (entry.fext == "truck")
            add = (query.cqy_filter_type == LT_AllBeam || query.cqy_filter_type == LT_Vehicle || query.cqy_filter_type == LT_Truck);
        else if (entry.fext == "car")
            add = (query.cqy_filter_type == LT_AllBeam || query.cqy_filter_type == LT_Vehicle || query.cqy_filter_type == LT_Truck || query.cqy_filter_type == LT_Car);
        else if (entry.fext == "boat")
            add = (query.cqy_filter_type == LT_AllBeam || query.cqy_filter_type == LT_Boat);
        else if (entry.fext == "airplane")
            add = (query.cqy_filter_type == LT_AllBeam || query.cqy_filter_type == LT_Airplane);
        else if (entry.fext == "trailer")
            add = (query.cqy_filter_type == LT_AllBeam || query.cqy_filter_type == LT_Trailer || query.cqy_filter_type == LT_Extension);
        else if (entry.fext == "train")
            add = (query.cqy_filter_type == LT_AllBeam || query.cqy_filter_type == LT_Train);
        else if (entry.fext == "load")
            add = (query.cqy_filter_type == LT_AllBeam || query.cqy_filter_type == LT_Load || query.cqy_filter_type == LT_Extension);

        if (!add)
        {
            continue;
        }

        query.cqy_res_category_usage[entry.categoryid]++;
        query.cqy_res_category_usage[CacheCategoryId::CID_All]++;

        // Filter by category
        if (query.cqy_filter_category_id < CacheCategoryId::CID_Max &&
            query.cqy_filter_category_id != entry.categoryid)
        {
            continue;
        }

        // Search
        size_t score = 0;
        bool match = false;
        Str<100> wheels_str;
        switch (query.cqy_search_method)
        {
        case CacheSearchMethod::FULLTEXT:
            if (match = this->Match(score, entry.dname,       query.cqy_search_string, 0))   { break; }
            if (match = this->Match(score, entry.fname,       query.cqy_search_string, 100)) { break; }
            if (match = this->Match(score, entry.description, query.cqy_search_string, 200)) { break; }
            for (AuthorInfo const& author: entry.authors)
            {
                if (match = this->Match(score, author.name,  query.cqy_search_string, 300)) { break; }
                if (match = this->Match(score, author.email, query.cqy_search_string, 400)) { break; }
            }
            break;

        case CacheSearchMethod::GUID:
            match = this->Match(score, entry.guid, query.cqy_search_string, 0);
            break;

        case CacheSearchMethod::AUTHORS:
            for (AuthorInfo const& author: entry.authors)
            {
                if (match = this->Match(score, author.name,  query.cqy_search_string, 0)) { break; }
                if (match = this->Match(score, author.email, query.cqy_search_string, 0)) { break; }
            }
            break;

        case CacheSearchMethod::WHEELS:
            wheels_str << entry.wheelcount << "x" << entry.propwheelcount;
            match = this->Match(score, wheels_str.ToCStr(), query.cqy_search_string, 0);
            break;

        case CacheSearchMethod::FILENAME:
            match = this->Match(score, entry.fname, query.cqy_search_string, 100);
            break;

        default: // CacheSearchMethod::NONE
            match = true;
            break;
        };

        if (match)
        {
            query.cqy_results.emplace_back(&entry, score);
            query.cqy_res_last_update = std::max(query.cqy_res_last_update, entry.addtimestamp);
        }
    }

    std::sort(query.cqy_results.begin(), query.cqy_results.end());
    return query.cqy_results.size();
}

bool CacheSystem::Match(size_t& out_score, std::string data, std::string const& query, size_t score)
{
    Ogre::StringUtil::toLowerCase(data);
    size_t pos = data.find(query);
    if (pos != std::string::npos)
    {
        out_score = score + pos;
        return true;
    }
    else
    {
        return false;
    }
}

bool CacheQueryResult::operator<(CacheQueryResult const& other)
{
    if (cqr_score == other.cqr_score)
    {
        Ogre::String first = this->cqr_entry->dname;
        Ogre::String second = other.cqr_entry->dname;
        Ogre::StringUtil::toLowerCase(first);
        Ogre::StringUtil::toLowerCase(second);
        return first < second;
    }

    return cqr_score < other.cqr_score;
}


void CacheSystem::PruneInvalidProjects()
{
    auto itor = m_projects.begin();
    while (itor != m_projects.end())
    {
        ProjectEntry* proj = itor->get();
        if (!proj->prj_valid)
        {
            Ogre::ResourceGroupManager::getSingleton().destroyResourceGroup(proj->prj_rg_name);
            itor = m_projects.erase(itor);
        }
        else
        {
            ++itor;
        }
    }
}
