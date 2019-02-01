/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2013-2018 Petr Ohlidal

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
#include "BeamEngine.h"
#include "ContentManager.h"
#include "DustManager.h"
#include "ErrorUtils.h"
#include "GUI_LoadingWindow.h"
#include "GUIManager.h"
#include "GfxActor.h"
#include "ImprovedConfigFile.h"
#include "Language.h"
#include "PlatformUtils.h"
#include "RigDef_Parser.h"
#include "RoRFrameListener.h"
#include "Settings.h"
#include "SoundScriptManager.h"
#include "TerrainManager.h"
#include "Terrn2Fileformat.h"
#include "Utils.h"

#include <OgreFileSystem.h>
#ifdef Bool // Conflicts with RapidJSON, see https://github.com/Tencent/rapidjson/issues/628
#   undef Bool
#endif
#include <rapidjson/document.h>
#include <rapidjson/istreamwrapper.h>
#include <rapidjson/ostreamwrapper.h>
#include <rapidjson/writer.h>
#include <fstream>

using namespace Ogre;
using namespace RoR;

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
}

void CacheSystem::LoadModCache(CacheValidityState validity)
{
    m_update_time = getTimeStamp();

    if (validity == CACHE_NEEDS_UPDATE_FULL)
    {
        RoR::Log("[RoR|ModCache] Performing full rebuild");

        this->ClearCache();

        this->loadAllZipsInResourceGroup(RGN_CONTENT);
        this->loadAllDirectoriesInResourceGroup(RGN_CONTENT);
        
        this->checkForNewKnownFiles(); // TODO: does some duplicate work, but needed to pick up flat files

        this->detectDuplicates();

        this->WriteCacheFileJson();
        this->LoadCacheFileJson(); // Reloading from the disk removes all the 'deleted' cache entries
    }
    else if (validity == CACHE_NEEDS_UPDATE_INCREMENTAL)
    {
        RoR::Log("[RoR|ModCache] Performing incremental update");

        this->incrementalCacheUpdate();

        this->WriteCacheFileJson();
        this->LoadCacheFileJson(); // Reloading from the disk removes all the 'deleted' cache entries
    }
    else
    {
        RoR::Log("[RoR|ModCache] Cache valid");
        this->LoadCacheFileJson();
    }

    // show error on zero content
    if (m_entries.empty())
    {
        ErrorUtils::ShowError(_L("No content found"), _L("Failed to generate list of installed content"));
        exit(1);
    }

    App::app_force_cache_purge.SetActive(false);
    App::app_force_cache_udpate.SetActive(false);

    RoR::Log("[RoR|ModCache] Cache loaded");
}

CacheEntry* CacheSystem::FindEntryByFilename(std::string filename)
{
    CacheEntry* fallback = nullptr;
    for (CacheEntry& entry : m_entries)
    {
        if (entry.fname == filename)
            return &entry;
        else if (entry.fname_without_uid == filename)
            fallback = &entry;
    }
    return fallback;
}

void CacheSystem::UnloadActorFromMemory(std::string filename)
{
    CacheEntry* cache_entry = this->FindEntryByFilename(filename);
    if (cache_entry != nullptr)
    {
        cache_entry->actor_def.reset();
        String group = cache_entry->resource_group;
        if (!group.empty())
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
                m_loaded_resource_bundles[cache_entry->resource_bundle_path] = false;
            }
        }
    }
}

std::map<int, String>* CacheSystem::getCategories()
{
    return &m_categories;
}

std::vector<CacheEntry>* CacheSystem::getEntries()
{
    return &m_entries;
}

String CacheSystem::getCacheConfigFilename()
{
    return PathCombine(App::sys_cache_dir.GetActive(), CACHE_FILE);
}

CacheSystem::CacheValidityState CacheSystem::EvaluateCacheValidity()
{
    this->GenerateHashFromFilenames();

    // First, open cache file and get SHA1 hash for quick update check
    std::ifstream ifs(this->getCacheConfigFilename()); // TODO: Load using OGRE resource system ~ only_a_ptr, 10/2018
    rapidjson::IStreamWrapper isw(ifs);
    rapidjson::Document j_doc;
    j_doc.ParseStream<rapidjson::kParseNanAndInfFlag>(isw);
    if (!j_doc.IsObject() ||
        !j_doc.HasMember("global_hash") || !j_doc["global_hash"].IsString() || 
        !j_doc.HasMember("format_version") ||!j_doc["format_version"].IsNumber() ||
        !j_doc.HasMember("entries") || !j_doc["entries"].IsArray())
    {
        RoR::Log("[RoR|ModCache] Invalid or missing cache file ...");
        return CACHE_NEEDS_UPDATE_FULL; 
    }

    if (j_doc["format_version"].GetInt() != CACHE_FILE_FORMAT)
    {
        RoR::Log("[RoR|ModCache] Invalid cache file format ...");
        return CACHE_NEEDS_UPDATE_FULL;
    }

    if (App::app_force_cache_purge.GetActive())
    {
        RoR::Log("[RoR|ModCache] Cache rebuild requested ...");
        return CACHE_NEEDS_UPDATE_FULL;
    }

    if (j_doc["global_hash"].GetString() != m_filenames_hash)
    {
        RoR::Log("[RoR|ModCache] Cache file out of date ...");
        return CACHE_NEEDS_UPDATE_INCREMENTAL;
    }

    if (App::app_force_cache_udpate.GetActive())
    {
        RoR::Log("[RoR|ModCache] Cache update requested ...");
        return CACHE_NEEDS_UPDATE_INCREMENTAL;
    }

    RoR::Log("[RoR|ModCache] valid, using it.");
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

    int category_id = j_entry["categoryid"].GetInt();
    if (m_categories.find(category_id) != m_categories.end())
    {
        out_entry.categoryname = m_categories[category_id];
        out_entry.categoryid = category_id;
    }
    else
    {
        out_entry.categoryid = -1;
        out_entry.categoryname = "Unsorted";
    }
    
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

    std::ifstream ifs(this->getCacheConfigFilename()); // TODO: Load using OGRE resource system ~ only_a_ptr, 10/2018
    rapidjson::IStreamWrapper isw(ifs);
    rapidjson::Document j_doc;
    j_doc.ParseStream<rapidjson::kParseNanAndInfFlag>(isw);

    if (!j_doc.IsObject() || !j_doc.HasMember("entries") || !j_doc["entries"].IsArray())
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

String CacheSystem::getRealPath(String path)
{
    // this shall convert the path names to fit the operating system's flavor
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
    // this is required for windows since we are case insensitive ...
    path = StringUtil::replaceAll(path, "/", "\\");
    StringUtil::toLowerCase(path);
#endif
    return path;
}

String CacheSystem::getVirtualPath(String path)
{
    path = StringUtil::replaceAll(path, "\\", "/");
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
    // this is required for windows since we are case insensitive ...
    StringUtil::toLowerCase(path);
#endif
    return path;
}

void CacheSystem::incrementalCacheUpdate()
{
    this->LoadCacheFileJson();

    LOG("* incremental check starting ...");
    LOG("* incremental check (1/5): deleted and changed files ...");
    auto* loading_win = RoR::App::GetGuiManager()->GetLoadingWindow();
    loading_win->setProgress(20, _L("incremental check: deleted and changed files"));

    int counter = 0;
    std::set<String> changed_entries;
    for (auto it = m_entries.begin(); it != m_entries.end(); it++ , counter++)
    {
        int progress = ((float)counter / (float)(m_entries.size())) * 100;
        UTFString tmp = _L("incremental check: deleted and changed files\n") + ANSI_TO_UTF(it->resource_bundle_type) + _L(": ") + ANSI_TO_UTF(it->fname);
        loading_win->setProgress(progress, tmp, false);

        std::string fn; // file path
        if (it->resource_bundle_type == "Zip")
        {
             fn = this->getRealPath(it->resource_bundle_path);
        }
        else if (it->resource_bundle_type == "FileSystem")
        {
            fn = getRealPath(it->resource_bundle_path + "/" + it->fname);
        }

        if (! RoR::FileExists(fn.c_str()))
        {
            LOG("- "+fn+" is not existing");
            tmp = _L("incremental check: deleted and changed files\n") + ANSI_TO_UTF(it->fname) + _L(" not existing");
            loading_win->setProgress(20, tmp);
            if (!it->deleted)
            {
                removeFileCache(*it);
            }
            it->deleted = true;
            // do not try: entries.erase(it)
            continue;
        }
        // check whether it changed
        if (it->filetime != RoR::GetFileLastModifiedTime(fn))
        {
            LOG("- "+fn+" changed");
            it->deleted = true; // see below
            changed_entries.insert(this->getRealPath(it->resource_bundle_path));
        }
    }

    LOG("* incremental check (2/5): processing changed zips ...");
    loading_win->setProgress(40, _L("incremental check: processing changed zips\n"));
    for (auto path : changed_entries)
    {
        loadSingleZip(path);
    }
    LOG("* incremental check (3/5): new content ...");
    loading_win->setProgress(60, _L("incremental check: new content\n"));
    checkForNewContent();

    LOG("* incremental check (4/5): new files ...");
    loading_win->setProgress(80, _L("incremental check: new files\n"));
    checkForNewKnownFiles();

    LOG("* incremental check (5/5): duplicates ...");
    loading_win->setProgress(90, _L("incremental check: duplicates\n"));
    detectDuplicates();

    RoR::App::GetGuiManager()->SetVisible_LoadingWindow(false);
    LOG("* incremental check done.");
}

void CacheSystem::detectDuplicates()
{
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
            if (basepathA != basepathB)
                continue;
            basenameA = Ogre::StringUtil::replaceAll(basenameA, " ", "_");
            basenameA = Ogre::StringUtil::replaceAll(basenameA, "-", "_");
            basenameB = Ogre::StringUtil::replaceAll(basenameB, " ", "_");
            basenameB = Ogre::StringUtil::replaceAll(basenameB, "-", "_");
            if (stripSHA1fromString(basenameA) != stripSHA1fromString(basenameB))
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

CacheEntry* CacheSystem::getEntry(int modid)
{
    for (std::vector<CacheEntry>::iterator it = m_entries.begin(); it != m_entries.end(); it++)
    {
        if (modid == it->number)
            return &(*it);
    }
    return 0;
}

String CacheSystem::getPrettyName(String fname)
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
    j_entry.AddMember("addtimestamp",         entry.addtimestamp,                                          j_doc.GetAllocator());
    j_entry.AddMember("resource_bundle_type", rapidjson::StringRef(entry.resource_bundle_type.c_str()),    j_doc.GetAllocator());
    j_entry.AddMember("resource_bundle_path", rapidjson::StringRef(entry.resource_bundle_path.c_str()),    j_doc.GetAllocator());
    j_entry.AddMember("fpath",                rapidjson::StringRef(entry.fpath.c_str()),                   j_doc.GetAllocator());
    j_entry.AddMember("fname",                rapidjson::StringRef(entry.fname.c_str()),                   j_doc.GetAllocator());
    j_entry.AddMember("fname_without_uid",    rapidjson::StringRef(entry.fname_without_uid.c_str()),       j_doc.GetAllocator());
    j_entry.AddMember("fext",                 rapidjson::StringRef(entry.fext.c_str()),                    j_doc.GetAllocator());
    j_entry.AddMember("filetime",             entry.filetime,                                              j_doc.GetAllocator()); 
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
    String path = getCacheConfigFilename();
    LOG("writing cache to file ("+path+")...");

    std::ofstream ofs(path);
    rapidjson::OStreamWrapper j_ofs(ofs);
    rapidjson::Writer<rapidjson::OStreamWrapper, rapidjson::UTF8<>, rapidjson::UTF8<>, rapidjson::CrtAllocator, 
        rapidjson::kWriteNanAndInfFlag> j_writer(j_ofs);
    const bool written_ok = j_doc.Accept(j_writer);
    if (written_ok)
    {
        RoR::LogFormat("[RoR|ModCache] File '%s' written OK", path.c_str());
    }
    else
    {
        RoR::LogFormat("[RoR|ModCache] Error writing '%s'", path.c_str());
    }
}

void CacheSystem::ClearCache()
{
    std::remove(this->getCacheConfigFilename().c_str());
    for (auto& entry : m_entries)
    {
        String group = entry.resource_group;
        if (!group.empty())
        {
            if (ResourceGroupManager::getSingleton().resourceGroupExists(group))
                ResourceGroupManager::getSingleton().destroyResourceGroup(group);
        }
        this->removeFileCache(entry);
    }
    m_entries.clear();
}

Ogre::String CacheSystem::stripUIDfromString(Ogre::String uidstr)
{
    size_t pos = uidstr.find("-");
    if (pos != String::npos && pos >= 3 && uidstr.substr(pos - 3, 3) == "UID")
        return uidstr.substr(pos + 1, uidstr.length() - pos);
    return uidstr;
}

Ogre::String CacheSystem::stripSHA1fromString(Ogre::String sha1str)
{
    size_t pos = sha1str.find_first_of("-_");
    if (pos != String::npos && pos >= 20)
        return sha1str.substr(pos + 1, sha1str.length() - pos);
    return sha1str;
}

void CacheSystem::addFile(String group, Ogre::FileInfo f, String ext)
{
    LOG("Preparing to add " + f.filename);

    String archiveType = f.archive ? f.archive->getType() : "FileSystem";
    String archiveDirectory = f.archive ? f.archive->getName() : "";

    try
    {
        DataStreamPtr ds = ResourceGroupManager::getSingleton().openResource(f.filename, group);
        // ds closes automatically, so do _not_ close it explicitly below

        CacheEntry entry;
        if (ext == "terrn2")
        {
            fillTerrainDetailInfo(entry, ds, f.filename);
        }
        else
        {
            fillTruckDetailInfo(entry, ds, f.filename, group);
        }
        entry.fpath = f.path;
        entry.fname = f.filename;
        entry.fname_without_uid = stripUIDfromString(f.filename);
        entry.fext = ext;
        if (archiveType == "Zip")
        {
            entry.filetime = RoR::GetFileLastModifiedTime(archiveDirectory);
        }
        else
        {
            entry.filetime = RoR::GetFileLastModifiedTime(PathCombine(archiveDirectory, f.filename));
        }
        entry.resource_bundle_type = archiveType;
        entry.resource_bundle_path = archiveDirectory;
        entry.number = static_cast<int>(m_entries.size() + 1); // Let's number mods from 1
        entry.addtimestamp = m_update_time;
        generateFileCache(entry, group);
        m_entries.push_back(entry);
    }
    catch (Ogre::Exception& e)
    {
        LOG("Error while opening resource: '" + f.filename + "': " + e.getFullDescription());
    }
}

void CacheSystem::fillTruckDetailInfo(CacheEntry& entry, Ogre::DataStreamPtr stream, String file_name, String group)
{
    /* LOAD AND PARSE THE VEHICLE */
    RigDef::Parser parser;
    parser.Prepare();
    parser.ProcessOgreStream(stream.getPointer(), group);
    parser.Finalize();

    /* Report messages */
    if (parser.GetMessages().size() > 0)
    {
        std::stringstream report;
        report << "Cache: Parsing vehicle '" << file_name << "' yielded following messages:" << std::endl << std::endl;

        std::list<RigDef::Parser::Message>::const_iterator iter = parser.GetMessages().begin();
        for (; iter != parser.GetMessages().end(); iter++)
        {
            switch (iter->type)
            {
            case (RigDef::Parser::Message::TYPE_FATAL_ERROR):
                report << "FATAL_ERROR";
                break;

            case (RigDef::Parser::Message::TYPE_ERROR):
                report << "ERROR";
                break;

            case (RigDef::Parser::Message::TYPE_WARNING):
                report << "WARNING";
                break;

            default:
                report << "INFO";
                break;
            }
            report << " (Section " << RigDef::File::SectionToString(iter->section) << ")" << std::endl;
            report << "\tLine (# " << iter->line_number << "): " << iter->line << std::endl;
            report << "\tMessage: " << iter->message << std::endl;
        }

        Ogre::LogManager::getSingleton().logMessage(report.str());
    }

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
        author.id = (author_itor->_has_forum_account) ? static_cast<int>(author_itor->forum_account_id) : -1;
        author.name = author_itor->name;
        author.type = author_itor->type;

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
    /* NOTE: TruckParser2013 allows modularization of vehicle type. Cache only supports single type.
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

Ogre::String detectMiniType(String filename)
{
    if (ResourceGroupManager::getSingleton().resourceExists(RGN_TEMP, filename + "dds"))
        return "dds";

    if (ResourceGroupManager::getSingleton().resourceExists(RGN_TEMP, filename + "png"))
        return "png";

    if (ResourceGroupManager::getSingleton().resourceExists(RGN_TEMP, filename + "jpg"))
        return "jpg";

    return "";
}

void CacheSystem::removeFileCache(CacheEntry& entry)
{
    if (!entry.filecachename.empty())
    {
        String path = PathCombine(App::sys_cache_dir.GetActive(), entry.filecachename);
        std::remove(path.c_str());
    }
}

void CacheSystem::generateFileCache(CacheEntry& entry, String group)
{
    if (entry.fname.empty())
        return;

    String fbase = "", fext = "";
    StringUtil::splitBaseFilename(entry.fname, fbase, fext);
    String minifn = fbase + "-mini.";
    String minitype = detectMiniType(minifn);

    if (minitype.empty())
        return;

    String src_path = minifn + minitype;

    String outBasename = "";
    String outPath = "";
    StringUtil::splitFilename(entry.resource_bundle_path, outBasename, outPath);

    // no path info in the cache file name ...
    String dst_path = outBasename + "_" + entry.fname + ".mini." + minitype;

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

void CacheSystem::parseKnownFiles(Ogre::String group, Ogre::String dirname)
{
    dirname = getVirtualPath(dirname);

    for (auto ext : m_known_extensions)
    {
        auto files = ResourceGroupManager::getSingleton().findResourceFileInfo(group, "*." + ext);
        for (const auto& file : *files)
        {
            if (dirname.empty() || (file.archive && getVirtualPath(file.archive->getName()) == dirname))
                addFile(group, file, ext);
        }
    }
}

void CacheSystem::checkForNewKnownFiles()
{
    for (auto ext : m_known_extensions)
    {
        auto files = ResourceGroupManager::getSingleton().findResourceFileInfo(RGN_CONTENT, "*." + ext);
        for (const auto& file : *files)
        {
            std::string fn = file.filename;
            if (std::find_if(m_entries.begin(), m_entries.end(),
                [fn](CacheEntry& e) { return e.fname == fn; }) == m_entries.end())
            {
                LOG("- " + fn + " is new");
                addFile(RGN_CONTENT, file, ext);
            }
        }
    }
}

void CacheSystem::GenerateHashFromFilenames()
{
    std::string filenames = App::GetContentManager()->ListAllUserContent();
    m_filenames_hash = HashData(filenames.c_str(), static_cast<int>(filenames.size()));
}

void CacheSystem::fillTerrainDetailInfo(CacheEntry& entry, Ogre::DataStreamPtr ds, Ogre::String fname)
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

bool CacheSystem::checkResourceLoaded(Ogre::String & filename)
{
    Ogre::String group = "";
    return checkResourceLoaded(filename, group);
}

bool CacheSystem::checkResourceLoaded(Ogre::String & filename, Ogre::String& group)
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
            loadResource(entry);
            filename = entry.fname;
            group = entry.resource_group;
            return ResourceGroupManager::getSingleton().resourceExistsInAnyGroup(filename);
        }
    }

    return false;
}

void CacheSystem::loadResource(CacheEntry& t)
{
    if (m_loaded_resource_bundles[t.resource_bundle_path])
    {
        return;
    }

    static int rg_counter = 0;
    String group = "Mod-" + std::to_string(rg_counter++);
    ResourceGroupManager::getSingleton().addResourceLocation(t.resource_bundle_path, t.resource_bundle_type, group);
    try
    {
        ResourceGroupManager::getSingleton().initialiseResourceGroup(group);
    }
    catch (Ogre::Exception& e)
    {
        LOG("Error while loading '" + t.resource_bundle_path + "': " + e.getFullDescription());
    }
    t.resource_group = group;

    m_loaded_resource_bundles[t.resource_bundle_path] = true;
}

void CacheSystem::loadSingleZip(String path)
{
    RoR::LogFormat("[RoR|ModCache] Adding archive '%s'", path.c_str());

    ResourceGroupManager::getSingleton().createResourceGroup(RGN_TEMP, false);
    ResourceGroupManager::getSingleton().addResourceLocation(path, "Zip", RGN_TEMP);

    parseKnownFiles(RGN_TEMP, "");

    ResourceGroupManager::getSingleton().destroyResourceGroup(RGN_TEMP);
}

void CacheSystem::loadAllZipsInResourceGroup(String group)
{
    std::map<String, bool> loadedZips;
    ResourceGroupManager& rgm = ResourceGroupManager::getSingleton();
    FileInfoListPtr files = rgm.findResourceFileInfo(group, "*.zip");
    FileInfoList::iterator iterFiles = files->begin();
    size_t i = 0, filecount = files->size();
    for (; iterFiles != files->end(); ++iterFiles , i++)
    {
        if (loadedZips[iterFiles->filename])
        {
            LOG(" zip already loaded: " + iterFiles->filename);
            // already loaded for some strange reason
            continue;
        }
        // update loader
        int progress = ((float)i / (float)filecount) * 100;
        UTFString tmp = _L("Loading zips in group ") + ANSI_TO_UTF(group) + L"\n" + ANSI_TO_UTF(iterFiles->filename) + L"\n" + ANSI_TO_UTF(TOSTRING(i)) + L"/" + ANSI_TO_UTF(TOSTRING(filecount));
        auto* loading_win = RoR::App::GetGuiManager()->GetLoadingWindow();
        loading_win->setProgress(progress, tmp, false);

        loadSingleZip(PathCombine(iterFiles->archive->getName(), iterFiles->filename));
        loadedZips[iterFiles->filename] = true;
    }
    // hide loader again
    RoR::App::GetGuiManager()->SetVisible_LoadingWindow(false);
}

void CacheSystem::loadAllDirectoriesInResourceGroup(String group)
{
    FileInfoListPtr list = ResourceGroupManager::getSingleton().listResourceFileInfo(group, true);
    size_t i = 0, filecount = list->size();
    for (FileInfoList::iterator listitem = list->begin(); listitem != list->end(); ++listitem , i++)
    {
        if (!listitem->archive)
            continue;
        String dirname = listitem->archive->getName() + PATH_SLASH + listitem->filename;
        // update loader
        int progress = ((float)i / (float)filecount) * 100;
        RoR::App::GetGuiManager()->GetLoadingWindow()->setProgress(progress, _L("Loading directory\n") + Utils::SanitizeUtf8String(listitem->filename), false);
        parseKnownFiles(group, dirname);
    }
    // hide loader again
    RoR::App::GetGuiManager()->SetVisible_LoadingWindow(false);
}

void CacheSystem::checkForNewZipsInResourceGroup(std::set<std::string>const & resource_bundles, String group)
{
    FileInfoListPtr files = ResourceGroupManager::getSingleton().findResourceFileInfo(group, "*.zip");
    FileInfoList::iterator iterFiles = files->begin();
    size_t i = 0, filecount = files->size();
    for (; iterFiles != files->end(); ++iterFiles , i++)
    {
        String zippath = iterFiles->archive->getName() + "\\" + iterFiles->filename;
        String zippath2 = "";
        if (iterFiles->archive->getName()[iterFiles->archive->getName().size() - 1] != '/')
            zippath2 = iterFiles->archive->getName() + "/" + iterFiles->filename;
        else
            zippath2 = iterFiles->archive->getName() + iterFiles->filename;
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
        //everything fine
#elif OGRE_PLATFORM == OGRE_PLATFORM_LINUX
        zippath=zippath2;
#elif OGRE_PLATFORM == OGRE_PLATFORM_APPLE
        zippath=zippath2;
#endif
        int progress = ((float)i / (float)filecount) * 100;
        std::string filename_utf8 = Utils::SanitizeUtf8String(iterFiles->filename);
        RoR::App::GetGuiManager()->GetLoadingWindow()->setProgress(progress, _L("checking for new zips in ") + group + "\n" + filename_utf8 + "\n" + TOSTRING(i) + "/" + TOSTRING(filecount), false);
        const bool bundle_exists = resource_bundles.find(this->getVirtualPath(zippath2)) != resource_bundles.end();
        if (!bundle_exists)
        {
            RoR::App::GetGuiManager()->GetLoadingWindow()->setProgress(progress, _L("checking for new zips in ") + group + "\n" + _L("loading new zip: ") + filename_utf8 + "\n" + TOSTRING(i) + "/" + TOSTRING(filecount));
            LOG("- "+zippath+" is new");
            loadSingleZip(PathCombine(iterFiles->archive->getName(), iterFiles->filename));
        }
    }
    RoR::App::GetGuiManager()->SetVisible_LoadingWindow(false);
}

void CacheSystem::checkForNewDirectoriesInResourceGroup(std::set<std::string>const & resource_bundles, String group)
{
    FileInfoListPtr list = ResourceGroupManager::getSingleton().listResourceFileInfo(group, true);
    size_t i = 0, filecount = list->size();
    for (FileInfoList::iterator listitem = list->begin(); listitem != list->end(); ++listitem , i++)
    {
        if (!listitem->archive)
            continue;
        String dirname = listitem->archive->getName() + PATH_SLASH + listitem->filename;
        int progress = ((float)i / (float)filecount) * 100;
        std::string filename_utf8 = Utils::SanitizeUtf8String(listitem->filename);
        RoR::App::GetGuiManager()->GetLoadingWindow()->setProgress(progress, _L("checking for new directories in ") + group + "\n" + filename_utf8 + "\n" + TOSTRING(i) + "/" + TOSTRING(filecount), false);
        const bool bundle_exists = resource_bundles.find(this->getVirtualPath(dirname)) != resource_bundles.end();
        if (!bundle_exists)
        {
            RoR::App::GetGuiManager()->GetLoadingWindow()->setProgress(progress, _L("checking for new directories in ") + group + "\n" + _L("loading new directory: ") + filename_utf8 + "\n" + TOSTRING(i) + "/" + TOSTRING(filecount));
            LOG("- "+dirname+" is new");
            parseKnownFiles(group, dirname);
        }
    }
    RoR::App::GetGuiManager()->SetVisible_LoadingWindow(false);
}

void CacheSystem::checkForNewContent() // Only used when performing "incremental update"
{
    std::set<std::string> resource_bundles; // List all existing bundles
    for (auto it = m_entries.begin(); it != m_entries.end(); it++)
    {
        resource_bundles.insert(this->getVirtualPath(it->resource_bundle_path));
    }

    checkForNewZipsInResourceGroup(resource_bundles, RGN_CONTENT);
    checkForNewDirectoriesInResourceGroup(resource_bundles, RGN_CONTENT);
}

