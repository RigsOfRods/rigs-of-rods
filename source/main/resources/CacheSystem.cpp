/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2013-2014 Petr Ohlidal

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
/// @author Thomas Fischer
/// @date   21th of May 2008

#include "CacheSystem.h"

#include <OgreFileSystem.h>
#include <OgreException.h>

#include "Application.h"
#include "BeamData.h"
#include "BeamEngine.h"
#include "ErrorUtils.h"
#include "GUIManager.h"
#include "ImprovedConfigFile.h"
#include "Language.h"
#include "PlatformUtils.h"
#include "RigDef_Parser.h"
#include "Settings.h"
#include "SHA1.h"
#include "SoundScriptManager.h"
#include "TerrainManager.h"
#include "Terrn2Fileformat.h"
#include "Utils.h"

#include "GUI_LoadingWindow.h"

using namespace Ogre;
using namespace RoR;

// default constructor resets the data.
CacheEntry::CacheEntry() :
    //authors
    addtimestamp(0),
    beamcount(0),
    categoryid(0),
    categoryname(""),
    changedornew(false),
    commandscount(0),
    custom_particles(false),
    customtach(false),
    deleted(false),
    description(""),
    dirname(""),
    dname(""),
    driveable(0),
    enginetype('t'),
    exhaustscount(0),
    fext(""),
    filecachename(""),
    fileformatversion(0),
    filetime(0),
    fixescount(0),
    flarescount(0),
    flexbodiescount(0),
    fname(""),
    fname_without_uid(""),
    forwardcommands(false),
    hasSubmeshs(false),
    hash(""),
    hydroscount(0),
    importcommands(false),
    loadmass(0),
    managedmaterialscount(0),
    materialflarebindingscount(0),
    materials(),
    maxrpm(0),
    minitype(""),
    minrpm(0),
    nodecount(0),
    number(0),
    numgears(0),
    propscount(0),
    propwheelcount(0),
    rescuer(false),
    resourceLoaded(false),
    rollon(false),
    rotatorscount(0),
    shockcount(0),
    soundsourcescount(0),
    tags(""),
    torque(0),
    truckmass(0),
    turbojetcount(0),
    turbopropscount(0),
    type(""),
    uniqueid(""),
    usagecounter(0),
    version(0),
    wheelcount(0),
    wingscount(0)
{
    // driveable = 0 = NOT_DRIVEABLE
    // enginetype = t = truck is default
}

CacheSystem::CacheSystem() :
    changedFiles(0)
    , deletedFiles(0)
    , newFiles(0)
    , rgcounter(0)
{
    // register the extensions
    known_extensions.push_back("machine");
    known_extensions.push_back("fixed");
    known_extensions.push_back("terrn2");
    known_extensions.push_back("truck");
    known_extensions.push_back("car");
    known_extensions.push_back("boat");
    known_extensions.push_back("airplane");
    known_extensions.push_back("trailer");
    known_extensions.push_back("load");
    known_extensions.push_back("train");

    // TODO: Use GVars directly, don't copy values
    // this location MUST include a path separator at the end!
    location       = Ogre::String(App::sys_cache_dir.GetActive())  + PATH_SLASH;
    configlocation = Ogre::String(App::sys_config_dir.GetActive()) + PATH_SLASH;
}

CacheSystem::~CacheSystem()
{
}

void CacheSystem::Startup(bool force_check)
{
    if (BSETTING("NOCACHE", false))
    {
        LOG("Cache disabled via command line switch");
        return;
    }

    // read valid categories from file
    readCategoryTitles();

    // calculate sha1 over all the content
    currentSHA1 = filenamesSHA1();

    CacheValidityState validity = CACHE_STATE_UNKNOWN;
    if (force_check)
    {
        validity = CACHE_NEEDS_UPDATE_INCREMENTAL;
    }
    else
    {
        validity = IsCacheValid();
    }

    if (validity != CACHE_VALID)
    {
        LOG("cache invalid, updating ...");
        // generate the cache
        generateCache(validity == CACHE_NEEDS_UPDATE_FULL);

        LOG("Cache updated, enumerating all resource groups...");
        StringVector sv = ResourceGroupManager::getSingleton().getResourceGroups();
        for (auto itor = sv.begin(); itor != sv.end(); ++itor)
        {
            LOG("\t\t" + *itor);
        }
    }

    LOG("loading cache...");
    // load the cache finally!
    loadCache();

    // show error on zero content
    if (entries.empty())
    {
        ErrorUtils::ShowError(_L("No content installed"), _L("You have no content installed"));
        exit(1337);
    }

    LOG("Cache loaded, enumerating all resource groups...");
    StringVector sv = ResourceGroupManager::getSingleton().getResourceGroups();
    for (auto itor = sv.begin(); itor != sv.end(); ++itor)
    {
        LOG("\t\t" + *itor);
    }

    LOG("cache loaded!");
}

std::map<int, Category_Entry>* CacheSystem::getCategories()
{
    return &categories;
}

std::vector<CacheEntry>* CacheSystem::getEntries()
{
    return &entries;
}

String CacheSystem::getCacheConfigFilename(bool full)
{
    if (full)
        return location + String(CACHE_FILE);
    return String(CACHE_FILE);
}

// we implement this on our own, since we cannot reply on the ogre version
bool CacheSystem::resourceExistsInAllGroups(Ogre::String filename)
{
    try
    {
        String group = ResourceGroupManager::getSingleton().findGroupContainingResource(filename);
        return !group.empty();
    }
    catch (...)
    {
        return false;
    }
}

CacheSystem::CacheValidityState CacheSystem::IsCacheValid()
{
    String cfgfilename = getCacheConfigFilename(false);
    ImprovedConfigFile cfg;
    if (!resourceExistsInAllGroups(cfgfilename))
    {
        LOG("unable to load config file: "+cfgfilename);
        return CACHE_NEEDS_UPDATE_FULL;
    }

    String group = ResourceGroupManager::getSingleton().findGroupContainingResource(cfgfilename);
    DataStreamPtr stream = ResourceGroupManager::getSingleton().openResource(cfgfilename, group);
    cfg.load(stream, "\t:=", false);
    String shaone = cfg.GetString("shaone");
    String cacheformat = cfg.GetString("cacheformat");

    if (shaone == "" || shaone != currentSHA1)
    {
        LOG("* mod cache is invalid (not up to date), regenerating new one ...");
        return CACHE_NEEDS_UPDATE_INCREMENTAL;
    }
    if (cacheformat != String(CACHE_FILE_FORMAT))
    {
        entries.clear();
        LOG("* mod cache has invalid format, trying to regenerate");
        return CACHE_NEEDS_UPDATE_INCREMENTAL;
    }
    LOG("* mod cache is valid, using it.");
    return CACHE_VALID;
}

void CacheSystem::logBadTruckAttrib(const String& line, CacheEntry& t)
{
    LOG("Bad Mod attribute line: " + line + " in mod " + t.dname);
}

void CacheSystem::parseModAttribute(const String& line, CacheEntry& t)
{
    Ogre::StringVector params = StringUtil::split(line, "\x09\x0a=,");
    String& attrib = params[0];
    StringUtil::toLowerCase(attrib);
    if (attrib == "number")
    {
        // Check params
        if (params.size() != 2)
        {
            logBadTruckAttrib(line, t);
            return;
        }
        // Set
        t.number = StringConverter::parseInt(params[1]);
    }
    else if (attrib == "deleted")
    {
        // Check params
        if (params.size() != 2)
        {
            logBadTruckAttrib(line, t);
            return;
        }
        // Set
        t.deleted = StringConverter::parseBool(params[1]);
    }
    else if (attrib == "usagecounter")
    {
        // Check params
        if (params.size() != 2)
        {
            logBadTruckAttrib(line, t);
            return;
        }
        // Set
        t.usagecounter = StringConverter::parseInt(params[1]);
    }
    else if (attrib == "addtimestamp")
    {
        // Check params
        if (params.size() != 2)
        {
            logBadTruckAttrib(line, t);
            return;
        }
        // Set
        t.addtimestamp = StringConverter::parseInt(params[1]);
    }
    else if (attrib == "minitype")
    {
        // Check params
        if (params.size() != 2)
        {
            logBadTruckAttrib(line, t);
            return;
        }
        // Set
        t.minitype = params[1];
    }
    else if (attrib == "type")
    {
        // Check params
        if (params.size() != 2)
        {
            logBadTruckAttrib(line, t);
            return;
        }
        // Set
        t.type = params[1];
    }
    else if (attrib == "dirname")
    {
        // Check params
        if (params.size() != 2)
        {
            logBadTruckAttrib(line, t);
            return;
        }
        // Set
        t.dirname = params[1];
    }
    else if (attrib == "filecachename")
    {
        // Check params
        if (params.size() != 2)
        {
            logBadTruckAttrib(line, t);
            return;
        }
        // Set
        t.filecachename = params[1];
    }
    else if (attrib == "fname")
    {
        // Check params
        if (params.size() != 2)
        {
            logBadTruckAttrib(line, t);
            return;
        }
        // Set
        t.fname = params[1];
    }
    else if (attrib == "fname_without_uid")
    {
        // Check params
        if (params.size() != 2)
        {
            logBadTruckAttrib(line, t);
            return;
        }
        // Set
        t.fname_without_uid = params[1];
    }
    else if (attrib == "fext")
    {
        // Check params
        if (params.size() != 2)
        {
            logBadTruckAttrib(line, t);
            return;
        }
        // Set
        t.fext = params[1];
    }
    else if (attrib == "filetime")
    {
        // Check params
        if (params.size() != 2)
        {
            logBadTruckAttrib(line, t);
            return;
        }
        // Set
        t.filetime = Ogre::StringConverter::parseLong(params[1]);
    }
    else if (attrib == "dname")
    {
        // Check params
        if (params.size() < 2)
        {
            logBadTruckAttrib(line, t);
            return;
        }
        // Set
        t.dname = params[1];
    }
    else if (attrib == "hash")
    {
        // Check params
        if (params.size() != 2)
        {
            logBadTruckAttrib(line, t);
            return;
        }
        // Set
        t.hash = params[1];
    }
    else if (attrib == "categoryid")
    {
        // Check params
        if (params.size() != 2)
        {
            logBadTruckAttrib(line, t);
            return;
        }
        // Set
        t.categoryid = StringConverter::parseInt(params[1]);
        category_usage[t.categoryid] = category_usage[t.categoryid] + 1;
        if (categories.find(t.categoryid) != categories.end())
        {
            t.categoryname = categories[t.categoryid].title;
        }
        else
        {
            t.categoryid = -1;
            t.categoryname = "Unsorted";
        }
    }
    else if (attrib == "uniqueid")
    {
        // Check params
        if (params.size() != 2)
        {
            logBadTruckAttrib(line, t);
            return;
        }
        // Set
        t.uniqueid = params[1];
    }
    else if (attrib == "guid")
    {
        // Check params
        if (params.size() != 2)
        {
            logBadTruckAttrib(line, t);
            return;
        }
        // Set
        t.guid = params[1];
        StringUtil::trim(t.guid);
    }
    else if (attrib == "version")
    {
        // Check params
        if (params.size() != 2)
        {
            logBadTruckAttrib(line, t);
            return;
        }
        // Set
        t.version = StringConverter::parseInt(params[1]);
    }
    else if (attrib == "author")
    {
        // Check params
        if (params.size() < 5)
        {
            logBadTruckAttrib(line, t);
            return;
        }
        // Set
        AuthorInfo ai;
        ai.id = StringConverter::parseInt(params[2]);
        ai.type = params[1];
        ai.name = params[3];
        ai.email = params[4];
        t.authors.push_back(ai);
    }
    else if (attrib == "sectionconfig")
    {
        // Check params
        if (params.size() < 2)
        {
            logBadTruckAttrib(line, t);
            return;
        }
        t.sectionconfigs.push_back(params[1]);
    }

    // TRUCK detail parameters below
    else if (attrib == "description")
        if (params.size() < 2)
        {
            logBadTruckAttrib(line, t);
            return;
        }
        else
            t.description = deNormalizeText(params[1]);
    else if (attrib == "tags")
        if (params.size() < 2)
        {
            logBadTruckAttrib(line, t);
            return;
        }
        else
            t.tags = params[1];
    else if (attrib == "fileformatversion")
        if (params.size() != 2)
        {
            logBadTruckAttrib(line, t);
            return;
        }
        else
            t.fileformatversion = StringConverter::parseInt(params[1]);
    else if (attrib == "hasSubmeshs")
        if (params.size() != 2)
        {
            logBadTruckAttrib(line, t);
            return;
        }
        else
            t.hasSubmeshs = StringConverter::parseBool(params[1]);
    else if (attrib == "nodecount")
        if (params.size() != 2)
        {
            logBadTruckAttrib(line, t);
            return;
        }
        else
            t.nodecount = StringConverter::parseInt(params[1]);
    else if (attrib == "beamcount")
        if (params.size() != 2)
        {
            logBadTruckAttrib(line, t);
            return;
        }
        else
            t.beamcount = StringConverter::parseInt(params[1]);
    else if (attrib == "shockcount")
        if (params.size() != 2)
        {
            logBadTruckAttrib(line, t);
            return;
        }
        else
            t.shockcount = StringConverter::parseInt(params[1]);
    else if (attrib == "fixescount")
        if (params.size() != 2)
        {
            logBadTruckAttrib(line, t);
            return;
        }
        else
            t.fixescount = StringConverter::parseInt(params[1]);
    else if (attrib == "hydroscount")
        if (params.size() != 2)
        {
            logBadTruckAttrib(line, t);
            return;
        }
        else
            t.hydroscount = StringConverter::parseInt(params[1]);
    else if (attrib == "wheelcount")
        if (params.size() != 2)
        {
            logBadTruckAttrib(line, t);
            return;
        }
        else
            t.wheelcount = StringConverter::parseInt(params[1]);
    else if (attrib == "propwheelcount")
        if (params.size() != 2)
        {
            logBadTruckAttrib(line, t);
            return;
        }
        else
            t.propwheelcount = StringConverter::parseInt(params[1]);
    else if (attrib == "commandscount")
        if (params.size() != 2)
        {
            logBadTruckAttrib(line, t);
            return;
        }
        else
            t.commandscount = StringConverter::parseInt(params[1]);
    else if (attrib == "flarescount")
        if (params.size() != 2)
        {
            logBadTruckAttrib(line, t);
            return;
        }
        else
            t.flarescount = StringConverter::parseInt(params[1]);
    else if (attrib == "propscount")
        if (params.size() != 2)
        {
            logBadTruckAttrib(line, t);
            return;
        }
        else
            t.propscount = StringConverter::parseInt(params[1]);
    else if (attrib == "wingscount")
        if (params.size() != 2)
        {
            logBadTruckAttrib(line, t);
            return;
        }
        else
            t.wingscount = StringConverter::parseInt(params[1]);
    else if (attrib == "turbopropscount")
        if (params.size() != 2)
        {
            logBadTruckAttrib(line, t);
            return;
        }
        else
            t.turbopropscount = StringConverter::parseInt(params[1]);
    else if (attrib == "turbojetcount")
        if (params.size() != 2)
        {
            logBadTruckAttrib(line, t);
            return;
        }
        else
            t.turbojetcount = StringConverter::parseInt(params[1]);
    else if (attrib == "rotatorscount")
        if (params.size() != 2)
        {
            logBadTruckAttrib(line, t);
            return;
        }
        else
            t.rotatorscount = StringConverter::parseInt(params[1]);
    else if (attrib == "exhaustscount")
        if (params.size() != 2)
        {
            logBadTruckAttrib(line, t);
            return;
        }
        else
            t.exhaustscount = StringConverter::parseInt(params[1]);
    else if (attrib == "flexbodiescount")
        if (params.size() != 2)
        {
            logBadTruckAttrib(line, t);
            return;
        }
        else
            t.flexbodiescount = StringConverter::parseInt(params[1]);
    else if (attrib == "materialflarebindingscount")
        if (params.size() != 2)
        {
            logBadTruckAttrib(line, t);
            return;
        }
        else
            t.materialflarebindingscount = StringConverter::parseInt(params[1]);
    else if (attrib == "soundsourcescount")
        if (params.size() != 2)
        {
            logBadTruckAttrib(line, t);
            return;
        }
        else
            t.soundsourcescount = StringConverter::parseInt(params[1]);
    else if (attrib == "managedmaterialscount")
        if (params.size() != 2)
        {
            logBadTruckAttrib(line, t);
            return;
        }
        else
            t.managedmaterialscount = StringConverter::parseInt(params[1]);
    else if (attrib == "truckmass")
        if (params.size() != 2)
        {
            logBadTruckAttrib(line, t);
            return;
        }
        else
            t.truckmass = StringConverter::parseReal(params[1]);
    else if (attrib == "loadmass")
        if (params.size() != 2)
        {
            logBadTruckAttrib(line, t);
            return;
        }
        else
            t.loadmass = StringConverter::parseReal(params[1]);
    else if (attrib == "minrpm")
        if (params.size() != 2)
        {
            logBadTruckAttrib(line, t);
            return;
        }
        else
            t.minrpm = StringConverter::parseReal(params[1]);
    else if (attrib == "maxrpm")
        if (params.size() != 2)
        {
            logBadTruckAttrib(line, t);
            return;
        }
        else
            t.maxrpm = StringConverter::parseReal(params[1]);
    else if (attrib == "torque")
        if (params.size() != 2)
        {
            logBadTruckAttrib(line, t);
            return;
        }
        else
            t.torque = StringConverter::parseReal(params[1]);
    else if (attrib == "customtach")
        if (params.size() != 2)
        {
            logBadTruckAttrib(line, t);
            return;
        }
        else
            t.customtach = StringConverter::parseBool(params[1]);
    else if (attrib == "custom_particles")
        if (params.size() != 2)
        {
            logBadTruckAttrib(line, t);
            return;
        }
        else
            t.custom_particles = StringConverter::parseBool(params[1]);
    else if (attrib == "forwardcommands")
        if (params.size() != 2)
        {
            logBadTruckAttrib(line, t);
            return;
        }
        else
            t.forwardcommands = StringConverter::parseBool(params[1]);
    else if (attrib == "rollon")
        if (params.size() != 2)
        {
            logBadTruckAttrib(line, t);
            return;
        }
        else
            t.rollon = StringConverter::parseBool(params[1]);
    else if (attrib == "rescuer")
        if (params.size() != 2)
        {
            logBadTruckAttrib(line, t);
            return;
        }
        else
            t.rescuer = StringConverter::parseBool(params[1]);
    else if (attrib == "driveable")
        if (params.size() != 2)
        {
            logBadTruckAttrib(line, t);
            return;
        }
        else
            t.driveable = StringConverter::parseInt(params[1]);
    else if (attrib == "numgears")
        if (params.size() != 2)
        {
            logBadTruckAttrib(line, t);
            return;
        }
        else
            t.numgears = StringConverter::parseInt(params[1]);
    else if (attrib == "enginetype")
        if (params.size() != 2)
        {
            logBadTruckAttrib(line, t);
            return;
        }
        else
            t.enginetype = StringConverter::parseInt(params[1]);
    else if (attrib == "materials")
    {
        if (params.size() < 2)
        {
            logBadTruckAttrib(line, t);
            return;
        }
        else
        {
            String mat = params[1];
            Ogre::StringVector ar = StringUtil::split(mat, " ");
            for (Ogre::StringVector::iterator it = ar.begin(); it != ar.end(); it++)
            {
                t.materials.insert(*it);
            }
        }
    }
}

bool CacheSystem::loadCache()
{
    // Clear existing entries
    entries.clear();

    String cfgfilename = getCacheConfigFilename(false);

    if (!resourceExistsInAllGroups(cfgfilename))
    {
        LOG("unable to load config file: "+cfgfilename);
        return false;
    }

    String group = ResourceGroupManager::getSingleton().findGroupContainingResource(String(cfgfilename));
    DataStreamPtr stream = ResourceGroupManager::getSingleton().openResource(cfgfilename, group);

    LOG("CacheSystem::loadCache");

    CacheEntry t;
    String line = "";
    int mode = 0;

    while (!stream->eof())
    {
        line = stream->getLine();

        // Ignore blanks & comments
        if (line.empty() || line.substr(0, 2) == "//")
        {
            continue;
        }

        // Skip these
        if (StringUtil::startsWith(line, "shaone=") || StringUtil::startsWith(line, "modcount=") || StringUtil::startsWith(line, "cacheformat="))
        {
            continue;
        }

        if (mode == 0)
        {
            // No current entry
            if (line == "mod")
            {
                mode = 1;
                t = CacheEntry();
                t.resourceLoaded = false;
                t.deleted = false;
                t.changedornew = false; // default upon loading
                // Skip to and over next {
                stream->skipLine("{");
            }
        }
        else if (mode == 1)
        {
            // Already in mod
            if (line == "}")
            {
                // Finished
                if (!t.deleted)
                {
                    entries.push_back(t);
                }
                mode = 0;
            }
            else
            {
                parseModAttribute(line, t);
            }
        }
    }
    return true;
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

int CacheSystem::incrementalCacheUpdate()
{
    entries.clear();

    if (!loadCache())
    //error loading cache!
        return -1;

    LOG("* incremental check starting ...");
    LOG("* incremental check (1/5): deleted and changed files ...");
    auto* loading_win = RoR::App::GetGuiManager()->GetLoadingWindow();
    loading_win->setProgress(20, _L("incremental check: deleted and changed files"));
    std::vector<CacheEntry> changed_entries;
    UTFString tmp = "";
    String fn = "";
    int counter = 0;
    for (std::vector<CacheEntry>::iterator it = entries.begin(); it != entries.end(); it++ , counter++)
    {
        int progress = ((float)counter / (float)(entries.size())) * 100;
        tmp = _L("incremental check: deleted and changed files\n") + ANSI_TO_UTF(it->type) + _L(": ") + ANSI_TO_UTF(it->fname);
        loading_win->setProgress(progress, tmp);
        // check whether the file exists
        if (it->type == "Zip")
            fn = getRealPath(it->dirname);
        else if (it->type == "FileSystem")
            fn = getRealPath(it->dirname + "/" + it->fname);

        if ((it->type == "FileSystem" || it->type == "Zip") && ! RoR::FileExists(fn.c_str()))
        {
            LOG("- "+fn+" is not existing");
            tmp = _L("incremental check: deleted and changed files\n") + ANSI_TO_UTF(it->fname) + _L(" not existing");
            loading_win->setProgress(20, tmp);
            removeFileFromFileCache(it);
            it->deleted = true;
            // do not try: entries.erase(it)
            deletedFiles++;
            continue;
        }
        // check whether it changed
        if (it->type == "Zip")
        {
            // check file time, if that fails, fall back to sha1 (needed for platforms where filetime is not yet implemented!
            bool check = false;
            std::time_t ft = fileTime(fn);
            if (!ft)
            {
                // slow sha1 check
                char hash[256] = {};

                RoR::CSHA1 sha1;
                sha1.HashFile(const_cast<char*>(fn.c_str()));
                sha1.Final();
                sha1.ReportHash(hash, RoR::CSHA1::REPORT_HEX_SHORT);
                check = (it->hash != String(hash));
            }
            else
            {
                // faster file time check
                check = (it->filetime != ft);
            }

            if (check)
            {
                changedFiles++;
                LOG("- "+fn+" changed");
                it->changedornew = true;
                it->deleted = true; // see below
                changed_entries.push_back(*it);
            }
        }
    }

    // we try to reload one zip only one time, not multiple times if it contains more resources at once
    std::vector<Ogre::String> reloaded_zips;
    LOG("* incremental check (2/5): processing changed zips ...");
    loading_win->setProgress(40, _L("incremental check: processing changed zips\n"));
    for (std::vector<CacheEntry>::iterator it = changed_entries.begin(); it != changed_entries.end(); it++)
    {
        bool found = false;
        for (std::vector<Ogre::String>::iterator it2 = reloaded_zips.begin(); it2 != reloaded_zips.end(); it2++)
        {
            if (*it2 == it->dirname)
            {
                found = true;
                break;
            }
        }
        if (!found)
        {
            loading_win->setProgress(40, _L("incremental check: processing changed zips\n") + Utils::SanitizeUtf8String(it->fname));
            loadSingleZip(*it);
            reloaded_zips.push_back(it->dirname);
        }
    }
    LOG("* incremental check (3/5): new content ...");
    loading_win->setProgress(60, _L("incremental check: new content\n"));
    checkForNewContent();

    LOG("* incremental check (4/5): new files ...");
    loading_win->setProgress(80, _L("incremental check: new files\n"));
    checkForNewKnownFiles();

    LOG("* incremental check (5/5): duplicates ...");
    loading_win->setProgress(90, _L("incremental check: duplicates\n"));
    for (std::vector<CacheEntry>::iterator it = entries.begin(); it != entries.end(); it++)
    {
        if (it->deleted)
            continue;
        for (std::vector<CacheEntry>::iterator it2 = entries.begin(); it2 != entries.end(); it2++)
        {
            if (it2->deleted)
                continue;
            // clean paths, important since we compare them ...
            String basename, basepath;

            String dira = it->dirname;
            StringUtil::toLowerCase(dira);
            StringUtil::splitFilename(dira, basename, basepath);
            basepath = getVirtualPath(basepath);
            dira = basepath + basename;

            String dirb = it2->dirname;
            StringUtil::toLowerCase(dirb);
            StringUtil::splitFilename(dira, basename, basepath);
            basepath = getVirtualPath(basepath);
            dirb = basepath + basename;

            String dnameA = it->dname;
            StringUtil::toLowerCase(dnameA);
            StringUtil::trim(dnameA);
            String dnameB = it2->dname;
            StringUtil::toLowerCase(dnameB);
            StringUtil::trim(dnameB);

            String filenameA = it->fname;
            StringUtil::toLowerCase(filenameA);
            String filenameB = it2->fname;
            StringUtil::toLowerCase(filenameB);

            String filenameWUIDA = it->fname_without_uid;
            StringUtil::toLowerCase(filenameWUIDA);
            String filenameWUIDB = it2->fname_without_uid;
            StringUtil::toLowerCase(filenameWUIDB);

            // hard duplicate
            if (dira == dirb && dnameA == dnameB && filenameA == filenameB)
            {
                if (it->number == it2->number)
                    continue; // do not delete self
                LOG("- "+ it2->dirname+"/" + it->fname + " hard duplicate");
                it2->deleted = true;
                continue;
            }
            // soft duplicates
            else if (dira == dirb && dnameA == dnameB && filenameWUIDA == filenameWUIDB)
            {
                if (it->number == it2->number)
                    continue; // do not delete self
                LOG("- "+ it2->dirname+"/" + it->fname + " soft duplicate, resolving ...");
                // create sha1 and see whats the correct entry :)
                RoR::CSHA1 sha1;
                sha1.HashFile(const_cast<char*>(it2->dirname.c_str()));
                sha1.Final();
                char hashres[256] = "";
                sha1.ReportHash(hashres, RoR::CSHA1::REPORT_HEX_SHORT);
                String hashstr = String(hashres);
                if (hashstr == it->hash)
                {
                    LOG("  - entry 2 removed");
                    it2->deleted = true;
                }
                else if (hashstr == it2->hash)
                {
                    LOG("  - entry 1 removed");
                    it->deleted = true;
                }
                else
                {
                    LOG("  - entry 1 and 2 removed");
                    it->deleted = true;
                    it2->deleted = true;
                }
            }
        }
    }
    loading_win->setAutotrack(_L("loading...\n"));

    this->writeGeneratedCache();

    RoR::App::GetGuiManager()->SetVisible_LoadingWindow(false);
    LOG("* incremental check done.");
    return 0;
}

CacheEntry* CacheSystem::getEntry(int modid)
{
    for (std::vector<CacheEntry>::iterator it = entries.begin(); it != entries.end(); it++)
    {
        if (modid == it->number)
            return &(*it);
    }
    return 0;
}

void CacheSystem::generateCache(bool forcefull)
{
    this->modcounter = 0;

    // see if we can avoid a full regeneration
    if (forcefull || incrementalCacheUpdate())
    {
        loadAllZips();

        writeGeneratedCache();
    }
}

Ogre::String CacheSystem::formatInnerEntry(int counter, CacheEntry t)
{
    String result = "";
    result += "\tnumber=" + TOSTRING(counter) + "\n"; // always count linear!
    result += "\tdeleted=" + TOSTRING(t.deleted) + "\n";
    if (!t.deleted)
    {
        // this ensures that we wont break the format with empty ("") values
        if (t.minitype.empty())
            t.minitype = "unknown";
        if (t.type.empty())
            t.type = "unknown";
        if (t.dirname.empty())
            t.dirname = "unknown";
        if (t.fname.empty())
            t.fname = "unknown";
        if (t.fext.empty())
            t.fext = "unknown";
        if (t.dname.empty())
            t.dname = "unknown";
        if (t.hash.empty())
            t.hash = "none";
        if (t.uniqueid.empty())
            t.uniqueid = "no-uid";
        if (t.guid.empty())
            t.guid = "no-guid";
        if (t.fname_without_uid.empty())
            t.fname_without_uid = "unknown";
        if (t.filecachename.empty())
            t.filecachename = "none";

        result += "\tusagecounter=" + TOSTRING(t.usagecounter) + "\n";
        result += "\taddtimestamp=" + TOSTRING(t.addtimestamp) + "\n";
        result += "\tminitype=" + t.minitype + "\n";
        result += "\ttype=" + t.type + "\n";
        result += "\tdirname=" + t.dirname + "\n";
        result += "\tfname=" + t.fname + "\n";
        result += "\tfname_without_uid=" + t.fname_without_uid + "\n";
        result += "\tfext=" + t.fext + "\n";
        result += "\tfiletime=" + TOSTRING((long)t.filetime) + "\n";
        result += "\tdname=" + t.dname + "\n";
        result += "\thash=" + t.hash + "\n";
        result += "\tcategoryid=" + TOSTRING(t.categoryid) + "\n";
        result += "\tuniqueid=" + t.uniqueid + "\n";
        result += "\tguid=" + t.guid + "\n";
        result += "\tversion=" + TOSTRING(t.version) + "\n";
        result += "\tfilecachename=" + t.filecachename + "\n";
        //result += "\tnumauthors="+TOSTRING(t.authors.size())+"\n";

        if (t.authors.size() > 0)
        {
            for (int i = 0; i < (int)t.authors.size(); i++)
            {
                if (t.authors[i].type.empty())
                    t.authors[i].type = "unknown";
                if (t.authors[i].name.empty())
                    t.authors[i].name = "unknown";
                if (t.authors[i].email.empty())
                    t.authors[i].email = "unknown";
                result += "\tauthor=" + (t.authors[i].type) +
                    "," + TOSTRING(t.authors[i].id) +
                    "," + (t.authors[i].name) + "," + (t.authors[i].email) + "\n";
            }
        }

        // now add the truck details if existing
        if (t.description != "")
            result += "\tdescription=" + normalizeText(t.description) + "\n";
        if (t.tags != "")
            result += "\ttags=" + t.tags + "\n";
        if (t.fileformatversion != 0)
            result += "\tfileformatversion=" + TOSTRING(t.fileformatversion) + "\n";
        if (t.hasSubmeshs)
            result += "\thasSubmeshs=1\n";
        if (t.nodecount != 0)
            result += "\tnodecount=" + TOSTRING(t.nodecount) + "\n";
        if (t.beamcount != 0)
            result += "\tbeamcount=" + TOSTRING(t.beamcount) + "\n";
        if (t.shockcount != 0)
            result += "\tshockcount=" + TOSTRING(t.shockcount) + "\n";
        if (t.fixescount != 0)
            result += "\tfixescount=" + TOSTRING(t.fixescount) + "\n";
        if (t.hydroscount != 0)
            result += "\thydroscount=" + TOSTRING(t.hydroscount) + "\n";
        if (t.wheelcount != 0)
            result += "\twheelcount=" + TOSTRING(t.wheelcount) + "\n";
        if (t.propwheelcount != 0)
            result += "\tpropwheelcount=" + TOSTRING(t.propwheelcount) + "\n";
        if (t.commandscount != 0)
            result += "\tcommandscount=" + TOSTRING(t.commandscount) + "\n";
        if (t.flarescount != 0)
            result += "\tflarescount=" + TOSTRING(t.flarescount) + "\n";
        if (t.propscount != 0)
            result += "\tpropscount=" + TOSTRING(t.propscount) + "\n";
        if (t.wingscount != 0)
            result += "\twingscount=" + TOSTRING(t.wingscount) + "\n";
        if (t.turbopropscount != 0)
            result += "\tturbopropscount=" + TOSTRING(t.turbopropscount) + "\n";
        if (t.turbojetcount != 0)
            result += "\tturbojetcount=" + TOSTRING(t.turbojetcount) + "\n";
        if (t.rotatorscount != 0)
            result += "\trotatorscount=" + TOSTRING(t.rotatorscount) + "\n";
        if (t.exhaustscount != 0)
            result += "\texhaustscount=" + TOSTRING(t.exhaustscount) + "\n";
        if (t.flexbodiescount != 0)
            result += "\tflexbodiescount=" + TOSTRING(t.flexbodiescount) + "\n";
        if (t.materialflarebindingscount != 0)
            result += "\tmaterialflarebindingscount=" + TOSTRING(t.materialflarebindingscount) + "\n";
        if (t.soundsourcescount != 0)
            result += "\tsoundsourcescount=" + TOSTRING(t.soundsourcescount) + "\n";
        if (t.managedmaterialscount != 0)
            result += "\tmanagedmaterialscount=" + TOSTRING(t.managedmaterialscount) + "\n";
        if (t.truckmass > 1)
            result += "\ttruckmass=" + TOSTRING(t.truckmass) + "\n";
        if (t.loadmass > 1)
            result += "\tloadmass=" + TOSTRING(t.loadmass) + "\n";
        if (t.minrpm > 1)
            result += "\tminrpm=" + TOSTRING(t.minrpm) + "\n";
        if (t.maxrpm > 1)
            result += "\tmaxrpm=" + TOSTRING(t.maxrpm) + "\n";
        if (t.torque > 1)
            result += "\ttorque=" + TOSTRING(t.torque) + "\n";
        if (t.customtach)
            result += "\tcustomtach=1\n";
        if (t.custom_particles)
            result += "\tcustom_particles=1\n";
        if (t.forwardcommands)
            result += "\tforwardcommands=1\n";
        if (t.importcommands)
            result += "\timportcommands=1\n";
        if (t.rollon)
            result += "\trollon=1\n";
        if (t.rescuer)
            result += "\trescuer=1\n";
        if (t.driveable != 0)
            result += "\tdriveable=" + TOSTRING(t.driveable) + "\n";
        if (t.numgears != 0)
            result += "\tnumgears=" + TOSTRING(t.numgears) + "\n";
        if (t.enginetype != 0)
            result += "\tenginetype=" + TOSTRING(t.enginetype) + "\n";
        if (t.materials.size())
        {
            String matStr = "";
            for (std::set<Ogre::String>::iterator it = t.materials.begin(); it != t.materials.end(); it++)
            {
                matStr += *it + " ";
            }
            result += "\tmaterials=" + matStr + "\n";
        }

        if (t.sectionconfigs.size() > 0)
        {
            for (int i = 0; i < (int)t.sectionconfigs.size(); i++)
                result += "\tsectionconfig=" + t.sectionconfigs[i] + "\n";
        }
    }

    return result;
}

Ogre::String CacheSystem::normalizeText(Ogre::String text)
{
    String result = "";
    Ogre::StringVector str = Ogre::StringUtil::split(text, "\n");
    for (Ogre::StringVector::iterator it = str.begin(); it != str.end(); it++)
        result += *it + "$";
    return result;
}

Ogre::String CacheSystem::deNormalizeText(Ogre::String text)
{
    String result = "";
    Ogre::StringVector str = Ogre::StringUtil::split(text, "$");
    for (Ogre::StringVector::iterator it = str.begin(); it != str.end(); it++)
        result += *it + "\n";
    return result;
}

Ogre::String CacheSystem::formatEntry(int counter, CacheEntry t)
{
    String result = "mod\n";
    result += "{\n";
    result += formatInnerEntry(counter, t);
    result += "}\n\n";

    return result;
}

void CacheSystem::writeGeneratedCache()
{
    String path = getCacheConfigFilename(true);
    LOG("writing cache to file ("+path+")...");

    FILE* f = fopen(path.c_str(), "w");
    if (!f)
    {
        ErrorUtils::ShowError(_L("Fatal Error: Unable to write cache to disk"), _L("Unable to write file.\nPlease ensure the parent directories exists and that you have write access to this location:\n") + path);
        exit(1337);
    }
    fprintf(f, "shaone=%s\n", const_cast<char*>(currentSHA1.c_str()));
    fprintf(f, "modcount=%d\n", (int)entries.size());
    fprintf(f, "cacheformat=%s\n", CACHE_FILE_FORMAT);

    // mods
    std::vector<CacheEntry>::iterator it;
    int counter = 0;
    for (it = entries.begin(); it != entries.end(); it++)
    {
        if (it->deleted)
            continue;
        fprintf(f, "%s", formatEntry(counter, *it).c_str());
        counter++;
    }

    // close
    fclose(f);
    LOG("...done!");
}

void CacheSystem::updateSingleTruckEntryCache(int number, CacheEntry t)
{
    // todo: to be implemented
}

char* CacheSystem::replacesSpaces(char* str)
{
    char* ptr = str;
    while (*ptr != 0)
    {
        if (*ptr == ' ')
            *ptr = '_';
        ptr++;
    };
    return str;
}

char* CacheSystem::restoreSpaces(char* str)
{
    char* ptr = str;
    while (*ptr != 0)
    {
        if (*ptr == '_')
            *ptr = ' ';
        ptr++;
    };
    return str;
}

bool CacheSystem::stringHasUID(Ogre::String uidstr)
{
    size_t pos = uidstr.find("-");
    if (pos != String::npos && pos >= 3 && uidstr.substr(pos - 3, 3) == "UID")
        return true;
    return false;
}

Ogre::String CacheSystem::stripUIDfromString(Ogre::String uidstr)
{
    size_t pos = uidstr.find("-");
    if (pos != String::npos && pos >= 3 && uidstr.substr(pos - 3, 3) == "UID")
        return uidstr.substr(pos + 1, uidstr.length() - pos);
    return uidstr;
}

Ogre::String CacheSystem::getUIDfromString(Ogre::String uidstr)
{
    size_t pos = uidstr.find("-");
    if (pos != String::npos && pos >= 3 && uidstr.substr(pos - 3, 3) == "UID")
        return uidstr.substr(0, pos);
    return "";
}

void CacheSystem::addFile(Ogre::FileInfo f, String ext)
{
    String archiveType = "FileSystem";
    String archiveDirectory = "";
    if (f.archive)
    {
        archiveType = f.archive->getType();
        archiveDirectory = f.archive->getName();
    }

    addFile(f.filename, archiveType, archiveDirectory, ext);
}

void CacheSystem::addFile(String filename, String archiveType, String archiveDirectory, String ext)
{
    LOG("Preparing to add " + filename);

    //read first line
    CacheEntry entry;
    if (!resourceExistsInAllGroups(filename))
        return;

    String group = ResourceGroupManager::getSingleton().findGroupContainingResource(filename);
    if (ResourceGroupManager::getSingleton().resourceExists(group, filename))
    {
        try
        {
            //LOG("Read to add "+String(entry.dname)+" filename "+String(filename));
            DataStreamPtr ds = ResourceGroupManager::getSingleton().openResource(filename, group);

            if (ext == "terrn2")
            {
                fillTerrainDetailInfo(entry, ds, filename);
            }
            else
            {
                entry.dname = ds->getLine();
                fillTruckDetailInfo(entry, ds, filename);
            }

            // ds closes automatically, so do _not_ close it explicitly below
            entry.fname = filename;
            entry.fname_without_uid = stripUIDfromString(filename);
            entry.fext = ext;
            entry.filetime = fileTime(archiveDirectory);
            entry.type = archiveType;
            entry.dirname = archiveDirectory;
            entry.number = modcounter++;
            entry.addtimestamp = getTimeStamp();
            entry.usagecounter = 0;
            entry.deleted = false;
            String basen;
            String fnextension;
            StringUtil::splitBaseFilename(entry.fname, basen, fnextension);
            entry.minitype = detectFilesMiniType(basen + "-mini");
            entry.hash = "none";
            entry.changedornew = true;
            generateFileCache(entry);
            if (archiveType == "Zip")
                entry.hash = zipHashes[getVirtualPath(archiveDirectory)];
            if (entry.hash == "")
            // fallback if no hash was found
                entry.hash = "none";
            // read in author and category
            entries.push_back(entry);
        }
        catch (ItemIdentityException& e)
        {
            LOG(" *** error opening archive '"+filename+"': some files are duplicates of existing files. The file will be ignored.");
            LOG("error while opening resource: " + e.getFullDescription());
        }
        catch (Ogre::Exception& e)
        {
            LOG("error while opening resource: " + e.getFullDescription());
            LOG("error opening archive '"+String(filename)+"'. Is it corrupt?");
            LOG("trying to continue ...");
        }
    }
}

void CacheSystem::fillTruckDetailInfo(CacheEntry& entry, Ogre::DataStreamPtr stream, Ogre::String file_name)
{
    /* LOAD AND PARSE THE VEHICLE */
    RigDef::Parser parser;
    parser.Prepare();
    parser.ProcessOgreStream(stream.getPointer());
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
        entry.numgears = engine->gear_ratios.size();
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

    entry.forwardcommands = def->forward_commands;
    entry.importcommands = def->import_commands;
    entry.rollon = def->rollon;
    entry.rescuer = def->rescuer;
    entry.guid = def->guid;
    entry.fileformatversion = def->file_format_version;
    entry.hasSubmeshs = static_cast<int>(def->root_module->submeshes.size() > 0);
    entry.nodecount = static_cast<int>(def->root_module->nodes.size());
    entry.beamcount = static_cast<int>(def->root_module->beams.size());
    entry.shockcount = static_cast<int>(def->root_module->shocks.size() + def->root_module->shocks_2.size());
    entry.fixescount = static_cast<int>(def->root_module->fixes.size());
    entry.hydroscount = static_cast<int>(def->root_module->hydros.size());
    entry.propwheelcount = -1; /* TODO: Count propelled wheels */
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
    entry.wheelcount = static_cast<int>(
        def->root_module->wheels.size()
        + def->root_module->wheels_2.size()
        + def->root_module->mesh_wheels.size() // Also meshwheels2
    );

    /* NOTE: std::shared_ptr cleans everything up. */
}

int CacheSystem::addUniqueString(std::set<Ogre::String>& list, Ogre::String str)
{
    // ignore some render texture targets
    if (str == "mirror")
        return 0;
    if (str == "renderdash")
        return 0;

    str = stripUIDfromString(str);

    if (list.find(str) == list.end())
    {
        list.insert(str);
        return 1;
    }
    return 0;
}

Ogre::String CacheSystem::addMeshMaterials(CacheEntry& entry, Ogre::Entity* e)
{
    String materials = "";
    MeshPtr m = e->getMesh();
    if (!m.isNull())
    {
        for (int n = 0; n < (int)m->getNumSubMeshes(); n++)
        {
            SubMesh* sm = m->getSubMesh(n);
            addUniqueString(entry.materials, sm->getMaterialName());
        }
    }

    for (int n = 0; n < (int)e->getNumSubEntities(); n++)
    {
        SubEntity* subent = e->getSubEntity(n);
        addUniqueString(entry.materials, subent->getMaterialName());
    }
    return materials;
}

int CacheSystem::getTimeStamp()
{
    return (int)time(NULL); //this will overflow in 2038
}

void CacheSystem::deleteFileCache(char* filename)
{
    if (remove(filename))
    {
        LOG("error deleting file '"+String(filename)+"'");
    }
}

Ogre::String CacheSystem::detectFilesMiniType(String filename)
{
    if (resourceExistsInAllGroups(filename + ".dds"))
        return "dds";

    if (resourceExistsInAllGroups(filename + ".png"))
        return "png";

    if (resourceExistsInAllGroups(filename + ".jpg"))
        return "jpg";

    return "none";
}

void CacheSystem::removeFileFromFileCache(std::vector<CacheEntry>::iterator iter)
{
    if (iter->minitype != "none")
    {
        String fn = location + iter->filecachename;
        deleteFileCache(const_cast<char*>(fn.c_str()));
    }
}

void CacheSystem::generateFileCache(CacheEntry& entry, Ogre::String directory)
{
    try
    {
        if (directory.empty() && !entry.changedornew)
            return;

        if (entry.fname == "")
            return;

        LOG(" -"+entry.fname+" ...");
        if (entry.minitype == "none")
        {
            entry.filecachename = "none";
            return;
        }

        String outBasename = "";
        String outPath = "";
        StringUtil::splitFilename(entry.dirname, outBasename, outPath);

        if (directory.empty())
            directory = location;

        String dst = directory + outBasename + "_" + entry.fname + ".mini." + entry.minitype;

        // no path info in the cache file name ...
        entry.filecachename = outBasename + "_" + entry.fname + ".mini." + entry.minitype;

        String fbase = "", fext = "";
        StringUtil::splitBaseFilename(entry.fname, fbase, fext);
        String minifn = fbase + "-mini." + entry.minitype;

        String group = "";
        bool exists = ResourceGroupManager::getSingleton().resourceExistsInAnyGroup(minifn);
        if (!exists)
        {
            String base, ext;
            StringUtil::splitBaseFilename(entry.fname, base, ext);
            entry.minitype = detectFilesMiniType(base + "-mini");
            exists = ResourceGroupManager::getSingleton().resourceExistsInAnyGroup(minifn);
            if (!exists)
            {
                // no minipic found
                entry.filecachename = "none";
                return;
            }
            else
            {
                group = ResourceGroupManager::getSingleton().findGroupContainingResource(minifn);
            }
        }
        else
        {
            group = ResourceGroupManager::getSingleton().findGroupContainingResource(minifn);
        }

        FileInfoListPtr files = ResourceGroupManager::getSingleton().findResourceFileInfo(group, minifn);
        if (files->empty())
        {
            deleteFileCache(const_cast<char*>(dst.c_str()));
        }

        size_t fsize = 0;
        char* buffer = 0;
        {
            DataStreamPtr ds = ResourceGroupManager::getSingleton().openResource(minifn, group);
            fsize = ds->size();
            buffer = (char*)malloc(fsize);
            memset(buffer, 0, fsize);
            size_t read = ds->read(buffer, fsize);
            if (read != fsize)
            {
                free(buffer);
                return;
            }
        }

        bool written = false;
        if (buffer)
        {
            FILE* f = fopen(dst.c_str(), "wb");
            if (f)
            {
                fwrite(buffer, 1, fsize, f);
                fclose(f);
                written = true;
            }
            free(buffer);
        }
        if (!written)
        {
            deleteFileCache(const_cast<char*>(dst.c_str()));
        }
    }
    catch (Ogre::Exception& e)
    {
        LOG("error while generating File cache: " + e.getFullDescription());
        LOG("trying to continue ...");
    }
    LOG("done generating file cache!");
}

void CacheSystem::parseKnownFilesAllRG()
{
    for (std::vector<Ogre::String>::iterator sit = known_extensions.begin(); sit != known_extensions.end(); sit++)
        parseFilesAllRG(*sit);
}

void CacheSystem::parseFilesAllRG(Ogre::String ext)
{
    StringVector sv = ResourceGroupManager::getSingleton().getResourceGroups();
    StringVector::iterator it;
    for (it = sv.begin(); it != sv.end(); it++)
        parseFilesOneRG(ext, *it);

    LOG("* parsing files of all Resource Groups (" + ext + ") finished!");
}

void CacheSystem::parseKnownFilesOneRG(Ogre::String rg)
{
    for (std::vector<Ogre::String>::iterator sit = known_extensions.begin(); sit != known_extensions.end(); sit++)
        parseFilesOneRG(*sit, rg);
}

void CacheSystem::parseKnownFilesOneRGDirectory(Ogre::String rg, Ogre::String dir)
{
    String dirb = getVirtualPath(dir);

    for (std::vector<Ogre::String>::iterator it = known_extensions.begin(); it != known_extensions.end(); ++it)
    {
        FileInfoListPtr files = ResourceGroupManager::getSingleton().findResourceFileInfo(rg, "*." + *it);
        for (FileInfoList::iterator iterFiles = files->begin(); iterFiles != files->end(); ++iterFiles)
        {
            if (!iterFiles->archive)
                continue;

            String dira = getVirtualPath(iterFiles->archive->getName());

            if (dira == dirb)
                addFile(*iterFiles, *it);
        }
    }
}

void CacheSystem::parseFilesOneRG(Ogre::String ext, Ogre::String rg)
{
    FileInfoListPtr files = ResourceGroupManager::getSingleton().findResourceFileInfo(rg, "*." + ext);
    for (FileInfoList::iterator iterFiles = files->begin(); iterFiles != files->end(); ++iterFiles)
    {
        addFile(*iterFiles, ext);
    }
}

bool CacheSystem::isFileInEntries(Ogre::String filename)
{
    for (std::vector<CacheEntry>::iterator it = entries.begin(); it != entries.end(); ++it)
    {
        if (it->fname == filename)
            return true;
    }
    return false;
}

void CacheSystem::generateZipList()
{
    zipCacheList.clear();
    for (std::vector<CacheEntry>::iterator it = entries.begin(); it != entries.end(); it++)
    {
        zipCacheList.insert(getVirtualPath(it->dirname));
    }
}

bool CacheSystem::isZipUsedInEntries(Ogre::String filename)
{
    if (zipCacheList.empty())
        generateZipList();

    return (zipCacheList.find(getVirtualPath(filename)) != zipCacheList.end());
}

bool CacheSystem::isDirectoryUsedInEntries(Ogre::String directory)
{
    String dira = directory;
    dira = getVirtualPath(dira);

    for (std::vector<CacheEntry>::iterator it = entries.begin(); it != entries.end(); it++)
    {
        if (it->type != "FileSystem")
            continue;
        String dirb = getVirtualPath(it->dirname);
        if (dira == dirb)
            return true;
        if (dira.substr(0, dirb.size()) == dirb) // check if it is a subdirectory
            return true;
    }
    return false;
}

void CacheSystem::checkForNewKnownFiles()
{
    for (std::vector<Ogre::String>::iterator it = known_extensions.begin(); it != known_extensions.end(); ++it)
        checkForNewFiles(*it);
}

void CacheSystem::checkForNewFiles(Ogre::String ext)
{
    char fname[256];
    sprintf(fname, "*.%s", ext.c_str());

    StringVector sv = ResourceGroupManager::getSingleton().getResourceGroups();
    for (StringVector::iterator it = sv.begin(); it != sv.end(); ++it)
    {
        FileInfoListPtr files = ResourceGroupManager::getSingleton().findResourceFileInfo(*it, fname);
        for (FileInfoList::iterator iterFiles = files->begin(); iterFiles != files->end(); ++iterFiles)
        {
            String fn = iterFiles->filename.c_str();
            if (!isFileInEntries(fn))
            {
                if (iterFiles->archive->getType() == "Zip")
                LOG("- " + fn + " is new (in zip)");
                else
                LOG("- " + fn + " is new");
                newFiles++;
                addFile(*iterFiles, ext);
            }
        }
    }
}

String CacheSystem::filenamesSHA1()
{
    String filenames = "";

    // get all Files
    /*
    StringVector sv = ResourceGroupManager::getSingleton().getResourceGroups();
    StringVector::iterator it;
    for (it = sv.begin(); it!=sv.end(); it++)
    {
        StringVectorPtr files = ResourceGroupManager::getSingleton().listResourceNames(*it);
        for (StringVector::iterator i=files->begin(); i!=files->end(); i++)
        {
            // only use the important files :)
            for (std::vector<Ogre::String>::iterator sit=known_extensions.begin();sit!=known_extensions.end();sit++)
                if (i->find("."+*sit) != String::npos && i->find(".dds") == String::npos && i->find(".png") == String::npos)
                    filenames += "General/" + *i + "\n";
        }
    }
    */

    // and we use all folders and files for the hash
    String restype[3] = {"Packs", "TerrainFolders", "VehicleFolders"};
    for (int i = 0; i < 3; i++)
    {
        for (int b = 0; b < 2; b++)
        {
            FileInfoListPtr list = ResourceGroupManager::getSingleton().listResourceFileInfo(restype[i], (b == 1));
            for (FileInfoList::iterator iterFiles = list->begin(); iterFiles != list->end(); iterFiles++)
            {
                String name = restype[i] + "/";
                if (iterFiles->archive)
                    name += iterFiles->archive->getName() + "/";

                if (b == 0)
                {
                    // special file handling, only add important files!
                    bool vipfile = false;
                    for (std::vector<Ogre::String>::iterator sit = known_extensions.begin(); sit != known_extensions.end(); sit++)
                    {
                        if ((iterFiles->filename.find("." + *sit) != String::npos && iterFiles->filename.find(".dds") == String::npos && iterFiles->filename.find(".png") == String::npos && iterFiles->filename.find(".jpg") == String::npos)
                            || (iterFiles->filename.find(".zip") != String::npos))
                        {
                            vipfile = true;
                            break;
                        }
                    }
                    if (!vipfile)
                        continue;
                }
                name += iterFiles->filename;
                filenames += name + "\n";
            }
        }
    }

    char result[256] = {};

    RoR::CSHA1 sha1;
    char* data = const_cast<char*>(filenames.c_str());
    sha1.UpdateHash((uint8_t *)data, (uint32_t)strlen(data));
    sha1.Final();
    sha1.ReportHash(result, RoR::CSHA1::REPORT_HEX_SHORT);
    return result;
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

int CacheSystem::getCategoryUsage(int category)
{
    return category_usage[category];
}

void CacheSystem::readCategoryTitles()
{
    String filename = configlocation + String("categories.cfg");
    LOG("Loading category titles from " + filename);
    FILE* fd = fopen(filename.c_str(), "r");
    if (!fd)
    {
        LOG("error opening file: " + filename);
        return;
    }
    char line[1024] = {};
    while (!feof(fd))
    {
        int res = fscanf(fd, " %[^\n\r]", line);
        if (line[0] == ';')
            continue;
        char title[256] = {};
        const char delimiters[] = ",";
        char str_work[1024] = {};

        strncpy(str_work, line, 1023);
        str_work[1023] = '\0';
        char* token = strtok(str_work, delimiters);
        if (token == NULL)
            continue;
        int number = atoi(token);
        token = strtok(NULL, delimiters);
        if (token == NULL)
            continue;
        //strip spaces at the beginning
        while (*token == ' ')
            token++;
        strncpy(title, token, 255);

        //LOG(String(title));
        Category_Entry ce;
        ce.title = Ogre::String(title);
        ce.number = number;
        if (!ce.title.empty())
            categories[number] = ce;
    }
    fclose(fd);
}

bool CacheSystem::checkResourceLoaded(Ogre::String& filename)
{
    Ogre::String group = "";
    return checkResourceLoaded(filename, group);
}

CacheEntry CacheSystem::getResourceInfo(Ogre::String& filename)
{
    CacheEntry def;
    std::vector<CacheEntry>::iterator it;
    for (it = entries.begin(); it != entries.end(); it++)
        if (it->fname == filename || it->fname_without_uid == filename)
            return *it;
    return def;
}

bool CacheSystem::checkResourceLoaded(Ogre::String& filename, Ogre::String& group)
{
    // check if we already loaded it via ogre ...
    if (ResourceGroupManager::getSingleton().resourceExistsInAnyGroup(filename))
    {
        group = ResourceGroupManager::getSingleton().findGroupContainingResource(filename);
        return true;
    }

    std::vector<CacheEntry>::iterator it;

    for (it = entries.begin(); it != entries.end(); it++)
    {
        // case insensitive comparison
        String fname = it->fname;
        String filename_lower = filename;
        String fname_without_uid_lower = it->fname_without_uid;
        StringUtil::toLowerCase(fname);
        StringUtil::toLowerCase(filename_lower);
        StringUtil::toLowerCase(fname_without_uid_lower);
        if (fname == filename_lower || fname_without_uid_lower == filename_lower)
        {
            // we found the file, load it
            filename = it->fname;
            bool res = checkResourceLoaded(*it);
            bool exists = ResourceGroupManager::getSingleton().resourceExistsInAnyGroup(filename);
            if (!exists)
                return false;
            group = ResourceGroupManager::getSingleton().findGroupContainingResource(filename);
            return res;
        }
    }
    return false;
}

bool CacheSystem::checkResourceLoaded(CacheEntry t)
{
    static int rgcountera = 0;
    static std::map<String, bool> loaded;
    if (t.resourceLoaded || loaded[t.dirname])
    // only load once
        return true;
    if (t.type == "Zip")
    {
        //ScopeLog log("cache_"+t.fname);
        try
        {
            rgcountera++;
            String name = "General-Reloaded-" + TOSTRING(rgcountera);
            ResourceGroupManager::getSingleton().addResourceLocation(t.dirname, t.type, name);
            loaded[t.dirname] = true;
            ResourceGroupManager::getSingleton().initialiseResourceGroup(name);
            return true;
        }
        catch (ItemIdentityException& e)
        {
            LOG(" *** error opening '"+t.dirname+"': some files are duplicates of existing files. The archive/directory will be ignored.");
            LOG("error while opening resource: " + e.getFullDescription());
        }
        catch (Ogre::Exception& e)
        {
            LOG("error opening '"+t.dirname+"'.");
            if (t.type == "Zip")
                LOG("Is the zip archive corrupt? Error: " + e.getFullDescription());
            LOG("Error description : " + e.getFullDescription());
            LOG("trying to continue ...");
        }
    }
    return false;
}

void CacheSystem::loadSingleZip(CacheEntry e, bool unload, bool ownGroup)
{
    loadSingleZip(e.dirname, -1, unload, ownGroup);
}

void CacheSystem::loadSingleZip(Ogre::FileInfo f, bool unload, bool ownGroup)
{
    String zippath = f.archive->getName() + "/" + f.filename;
    int cfactor = -1;
    if (f.uncompressedSize > 0)
        cfactor = (f.compressedSize / f.uncompressedSize) * 100.0f;
    loadSingleZip(zippath, cfactor, unload, ownGroup);
}

void CacheSystem::loadSingleDirectory(String dirname, String group, bool alreadyLoaded)
{
    char hash[256];
    memset(hash, 0, 255);

    LOG("Adding directory " + dirname);

    rgcounter++;
    String rgname = "General-" + TOSTRING(rgcounter);

    try
    {
        if (alreadyLoaded)
        {
            parseKnownFilesOneRGDirectory(group, dirname);
        }
        else
        {
            LOG("Loading " + dirname);
            ResourceGroupManager::getSingleton().addResourceLocation(dirname, "FileSystem", rgname);
            ResourceGroupManager::getSingleton().initialiseResourceGroup(rgname);
            // parse everything
            parseKnownFilesOneRG(rgname);
            // unload it again
            LOG("UnLoading " + dirname);

#ifdef USE_OPENAL
            SoundScriptManager::getSingleton().clearNonBaseTemplates();
#endif //OPENAL
            //ParticleSystemManager::getSingleton().removeTemplatesByResourceGroup(rgname);
            ResourceGroupManager::getSingleton().clearResourceGroup(rgname);
            ResourceGroupManager::getSingleton().unloadResourceGroup(rgname);
            ResourceGroupManager::getSingleton().removeResourceLocation(dirname, rgname);
            ResourceGroupManager::getSingleton().destroyResourceGroup(rgname);
        }
    }
    catch (ItemIdentityException& e)
    {
        LOG(" *** error opening directory '" + dirname + "': some files are duplicates of existing files. The directory will be ignored.");
        LOG("error while opening resource: " + e.getFullDescription());
    }
    catch (Ogre::Exception& e)
    {
        LOG("error while loading directory: " + e.getFullDescription());
        LOG("error opening directory '"+dirname+"'");
        LOG("trying to continue ...");
    }
}

void CacheSystem::loadSingleZip(String zippath, int cfactor, bool unload, bool ownGroup)
{
#if 0
    // good for debugging:
    String outfn;
    String outpath;
    StringUtil::splitFilename(zippath, outfn, outpath);
    ScopeLog log("cache_loadzip_"+outfn);
#endif

    String realzipPath = getRealPath(zippath);
    char hash[256] = {};

    RoR::CSHA1 sha1;
    sha1.HashFile(const_cast<char*>(realzipPath.c_str()));
    sha1.Final();
    sha1.ReportHash(hash, RoR::CSHA1::REPORT_HEX_SHORT);
    zipHashes[getVirtualPath(zippath)] = hash;

    String compr = "";
    if (cfactor > 99)
        compr = "(No Compression)";
    else if (cfactor > 0)
        compr = "(Compression: " + TOSTRING(cfactor) + ")";
    LOG("Adding archive " + realzipPath + " (hash: "+String(hash)+") " + compr);

    rgcounter++;
    String rgname = "General-" + TOSTRING(rgcounter);

    // use general group?
    if (!ownGroup)
    {
        rgname = ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME;
    }

    try
    {
        ResourceGroupManager& rgm = ResourceGroupManager::getSingleton();

        // load it into a new resource group
        LOG("Loading " + realzipPath);
        rgm.addResourceLocation(realzipPath, "Zip", rgname);
        rgm.initialiseResourceGroup(rgname);

        // parse everything
        parseKnownFilesOneRG(rgname);

        // unload it again
        if (unload)
        {
            LOG("Unloading " + realzipPath);
#ifdef USE_OPENAL
            SoundScriptManager::getSingleton().clearNonBaseTemplates();
#endif //OPENAL
            //ParticleSystemManager::getSingleton().removeTemplatesByResourceGroup(rgname);
            rgm.removeResourceLocation(realzipPath, rgname);
            rgm.clearResourceGroup(rgname);
            rgm.unloadResourceGroup(rgname);
            rgm.destroyResourceGroup(rgname);
        }
    }
    catch (ItemIdentityException& e)
    {
        LOG(" *** error opening archive '"+realzipPath+"': some files are duplicates of existing files. The archive will be ignored.");
        LOG("error while opening resource: " + e.getFullDescription());
    }
    catch (Ogre::Exception& e)
    {
        LOG("error while loading single Zip: " + e.getFullDescription());
        LOG("error opening archive '"+realzipPath+"'. Is it corrupt? Ignoring that archive ...");
        LOG("trying to continue ...");
    }
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
        loading_win->setProgress(progress, tmp);

        loadSingleZip((Ogre::FileInfo)*iterFiles);
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
        RoR::App::GetGuiManager()->GetLoadingWindow()->setProgress(progress, _L("Loading directory\n") + Utils::SanitizeUtf8String(listitem->filename));
        loadSingleDirectory(dirname, group, true);
    }
    // hide loader again
    RoR::App::GetGuiManager()->SetVisible_LoadingWindow(false);
}

void CacheSystem::loadAllZips()
{
    static bool lodedalready = false;
    if (lodedalready)
        return;
    lodedalready = true;
    //setup zip packages
    //search zip in packs group
    loadAllZipsInResourceGroup("Packs");
    loadAllZipsInResourceGroup("VehicleFolders");
    loadAllZipsInResourceGroup("TerrainFolders");

    loadAllDirectoriesInResourceGroup("VehicleFolders");
    loadAllDirectoriesInResourceGroup("TerrainFolders");
}

void CacheSystem::checkForNewZipsInResourceGroup(String group)
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
        RoR::App::GetGuiManager()->GetLoadingWindow()->setProgress(progress, _L("checking for new zips in ") + group + "\n" + filename_utf8 + "\n" + TOSTRING(i) + "/" + TOSTRING(filecount));
        if (!isZipUsedInEntries(zippath2))
        {
            RoR::App::GetGuiManager()->GetLoadingWindow()->setProgress(progress, _L("checking for new zips in ") + group + "\n" + _L("loading new zip: ") + filename_utf8 + "\n" + TOSTRING(i) + "/" + TOSTRING(filecount));
            LOG("- "+zippath+" is new");
            newFiles++;
            loadSingleZip((Ogre::FileInfo)*iterFiles);
        }
    }
    RoR::App::GetGuiManager()->SetVisible_LoadingWindow(false);
}

void CacheSystem::checkForNewDirectoriesInResourceGroup(String group)
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
        RoR::App::GetGuiManager()->GetLoadingWindow()->setProgress(progress, _L("checking for new directories in ") + group + "\n" + filename_utf8 + "\n" + TOSTRING(i) + "/" + TOSTRING(filecount));
        if (!isDirectoryUsedInEntries(dirname))
        {
            RoR::App::GetGuiManager()->GetLoadingWindow()->setProgress(progress, _L("checking for new directories in ") + group + "\n" + _L("loading new directory: ") + filename_utf8 + "\n" + TOSTRING(i) + "/" + TOSTRING(filecount));
            LOG("- "+dirname+" is new");
            loadSingleDirectory(dirname, group, true);
        }
    }
    RoR::App::GetGuiManager()->SetVisible_LoadingWindow(false);
}

void CacheSystem::checkForNewContent()
{
    checkForNewZipsInResourceGroup("Packs");
    checkForNewZipsInResourceGroup("VehicleFolders");
    checkForNewZipsInResourceGroup("TerrainFolders");

    checkForNewDirectoriesInResourceGroup("VehicleFolders");
    checkForNewDirectoriesInResourceGroup("TerrainFolders");
}

std::time_t CacheSystem::fileTime(Ogre::String filename)
{
    FileSystemArchiveFactory FSAF;

    Archive* fsa = FSAF.createInstance(filename, true);

    std::time_t ft = fsa->getModifiedTime(filename);

    FSAF.destroyInstance(fsa);

    return ft;
}
