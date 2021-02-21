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
/// @brief  A database of user-installed content alias 'mods' (vehicles, terrains...)

#pragma once

#include "Application.h"
#include "Language.h"
#include "TruckFileFormat.h"
#include "SimData.h"

#include <Ogre.h>
#include <rapidjson/document.h>
#include <string>

#define CACHE_FILE "mods.cache"
#define CACHE_FILE_FORMAT 11

namespace RoR {

struct AuthorInfo
{
    int id;
    Ogre::String type;
    Ogre::String name;
    Ogre::String email;
};

class CacheEntry
{

public:

    /** default constructor resets the data. */
    CacheEntry();

    Ogre::String fpath;                 //!< filepath relative to the .zip file
    Ogre::String fname;                 //!< filename
    Ogre::String fname_without_uid;     //!< filename
    Ogre::String dname;                 //!< name parsed from the file

    int categoryid;                     //!< category id
    Ogre::String categoryname;          //!< category name

    std::time_t addtimestamp;           //!< timestamp when this file was added to the cache

    Ogre::String uniqueid;              //!< file's unique id
    Ogre::String guid;                  //!< global unique id
    int version;                        //!< file's version
    Ogre::String fext;                  //!< file's extension
    std::string resource_bundle_type;   //!< Archive type recognized by OGRE resource system: 'FileSystem' or 'Zip'
    std::string resource_bundle_path;   //!< Path of ZIP or directory which contains the media. Shared between CacheEntries, loaded only once.
    int number;                         //!< Sequential number, assigned internally, used by Selector-GUI
    std::time_t filetime;               //!< filetime
    bool deleted;                       //!< is this mod deleted?
    int usagecounter;                   //!< how much it was used already
    std::vector<AuthorInfo> authors;    //!< authors
    Ogre::String filecachename;         //!< preview image filename

    Ogre::String resource_group;        //!< Resource group of the loaded bundle. Empty if not loaded yet.

    Truck::DocumentPtr actor_def; //!< Cached actor definition (aka truckfile) after first spawn
    std::shared_ptr<RoR::SkinDef> skin_def;  //!< Cached skin info, added on first use or during cache rebuild

    // following all TRUCK detail information:
    Ogre::String description;
    Ogre::String tags;
    int fileformatversion;
    bool hasSubmeshs;
    int nodecount;
    int beamcount;
    int shockcount;
    int fixescount;
    int hydroscount;
    int wheelcount;
    int propwheelcount;
    int commandscount;
    int flarescount;
    int propscount;
    int wingscount;
    int turbopropscount;
    int turbojetcount;
    int rotatorscount;
    int exhaustscount;
    int flexbodiescount;
    int soundsourcescount;

    float truckmass;
    float loadmass;
    float minrpm;
    float maxrpm;
    float torque;
    bool customtach;
    bool custom_particles;
    bool forwardcommands;
    bool importcommands;
    bool rescuer;

    ActorType driveable;
    int numgears;
    char enginetype;
    std::vector<Ogre::String> sectionconfigs;
};

enum CacheCategoryId
{
    CID_Max           = 9000,
    CID_Unsorted      = 9990,
    CID_All           = 9991,
    CID_Fresh         = 9992,
    CID_Hidden        = 9993,
    CID_SearchResults = 9994
};

struct CacheQueryResult
{
    CacheQueryResult(CacheEntry* entry, size_t score):
        cqr_entry(entry), cqr_score(score)
    {}

    bool operator<(CacheQueryResult const& other);

    CacheEntry* cqr_entry;
    size_t      cqr_score;
};

enum class CacheSearchMethod // Always case-insensitive
{
    NONE,     // No searching
    FULLTEXT, // Fields: name, filename, description, author name/mail (in this order, with descending rank) and returns rank+string pos as score
    GUID,     // Fields: guid
    AUTHORS,  // Fields: name, email), 'wheels' (), 'file' (filename)
    WHEELS,   // Fields: num wheels (string), num propelled wheels (string)
    FILENAME  // Fields: truckfile name
};

struct CacheQuery
{
    LoaderType                     cqy_filter_type = LoaderType::LT_None;
    int                            cqy_filter_category_id = CacheCategoryId::CID_All;
    std::string                    cqy_filter_guid; //!< Exact match; leave empty to disable
    CacheSearchMethod              cqy_search_method = CacheSearchMethod::NONE;
    std::string                    cqy_search_string;
    
    std::vector<CacheQueryResult>  cqy_results;
    std::map<int, size_t>          cqy_res_category_usage; //!< Total usage (ignores search params + category filter)
    std::time_t                    cqy_res_last_update = std::time_t();
};

/// A content database
/// MOTIVATION:
///    RoR users usually have A LOT of content installed. Traversing it all on every game startup would be a pain.
/// HOW IT WORKS:
///    For each recognized resource type (vehicle, terrain, skin...) an instance of 'CacheEntry' is created.
///       These entries are persisted in file CACHE_FILE (see above)
///    Associated media live in a "resource bundle" (ZIP archive or subdirectory) in content directory (ROR_HOME/mods) and subdirectories.
///       If multiple CacheEntries share a bundle, the bundle is loaded only once. Each bundle has dedicated OGRE resource group.
class CacheSystem : public ZeroedMemoryAllocator
{
public:
    typedef std::map<int, Ogre::String> CategoryIdNameMap;
    
    CacheSystem();

    enum CacheValidityState
    {
        CACHE_VALID         = 0,
        CACHE_NEEDS_UPDATE  = -1,
        CACHE_NEEDS_REBUILD = -2,

        CACHE_STATE_UNKNOWN = 0xFFFFFFFF
    };

    void                  LoadModCache(CacheValidityState validity);
    CacheEntry*           FindEntryByFilename(RoR::LoaderType type, bool partial, std::string filename); //!< Returns NULL if none found
    CacheEntry*           FetchSkinByName(std::string const & skin_name);
    CacheValidityState    EvaluateCacheValidity();
    size_t                Query(CacheQuery& query);

    void LoadResource(CacheEntry& t); //!< Loads the associated resource bundle if not already done.
    bool CheckResourceLoaded(Ogre::String &in_out_filename); //!< Finds + loads the associated resource bundle if not already done.
    bool CheckResourceLoaded(Ogre::String &in_out_filename, Ogre::String &out_group); //!< Finds given resource, outputs group name. Also loads the associated resource bundle if not already done.
    void ReLoadResource(CacheEntry& t); //!< Forces reloading the associated bundle.
    void UnLoadResource(CacheEntry& t); //!< Unloads associated bundle, destroying all spawned actors.

    const std::vector<CacheEntry>   &GetEntries()        const { return m_entries; }
    const CategoryIdNameMap         &GetCategories()     const { return m_categories; }

    std::shared_ptr<RoR::SkinDef> FetchSkinDef(CacheEntry* cache_entry); //!< Loads+parses the .skin file once

    CacheEntry *GetEntry(int modid);
    Ogre::String GetPrettyName(Ogre::String fname);
    std::string ActorTypeToName(ActorType driveable);

private:

    void WriteCacheFileJson();
    void ExportEntryToJson(rapidjson::Value& j_entries, rapidjson::Document& j_doc, CacheEntry const & entry);
    void LoadCacheFileJson();
    void ImportEntryFromJson(rapidjson::Value& j_entry, CacheEntry & out_entry);

    static Ogre::String StripUIDfromString(Ogre::String uidstr); 
    static Ogre::String StripSHA1fromString(Ogre::String sha1str);

    void ParseZipArchives(Ogre::String group);
    bool ParseKnownFiles(Ogre::String group); // returns true if no known files are found
    void ParseSingleZip(Ogre::String path);

    void ClearCache(); // removes                   all files from the cache
    void PruneCache(); // removes modified (or deleted) files from the cache

    void AddFile(Ogre::String group, Ogre::FileInfo f, Ogre::String ext);

    void DetectDuplicates();

    void FillTerrainDetailInfo(CacheEntry &entry, Ogre::DataStreamPtr ds, Ogre::String fname);
    void FillTruckDetailInfo(CacheEntry &entry, Ogre::DataStreamPtr ds, Ogre::String fname, Ogre::String group);

    void GenerateHashFromFilenames();         //!< For quick detection of added/removed content

    void GenerateFileCache(CacheEntry &entry, Ogre::String group);
    void RemoveFileCache(CacheEntry &entry);

    bool Match(size_t& out_score, std::string data, std::string const& query, size_t );

    std::time_t                          m_update_time;      //!< Ensures that all inserted files share the same timestamp
    std::string                          m_filenames_hash;   //!< stores hash over the content, for quick update detection
    std::vector<CacheEntry>              m_entries;
    std::vector<Ogre::String>            m_known_extensions; //!< the extensions we track in the cache system
    std::set<Ogre::String>               m_resource_paths;   //!< A temporary list of existing resource paths
    std::map<int, Ogre::String>          m_categories = {
            // these are the category numbers from the repository. do not modify them!

            // vehicles
            {108, _LC("ModCategory", "Other Land Vehicles")},

            {146, _LC("ModCategory", "Street Cars")},
            {147, _LC("ModCategory", "Light Racing Cars")},
            {148, _LC("ModCategory", "Offroad Cars")},
            {149, _LC("ModCategory", "Fantasy Cars")},
            {150, _LC("ModCategory", "Bikes")},
            {155, _LC("ModCategory", "Crawlers")},

            {152, _LC("ModCategory", "Towercranes")},
            {153, _LC("ModCategory", "Mobile Cranes")},
            {154, _LC("ModCategory", "Other cranes")},

            {107, _LC("ModCategory", "Buses")},
            {151, _LC("ModCategory", "Tractors")},
            {156, _LC("ModCategory", "Forklifts")},
            {159, _LC("ModCategory", "Fantasy Trucks")},
            {160, _LC("ModCategory", "Transport Trucks")},
            {161, _LC("ModCategory", "Racing Trucks")},
            {162, _LC("ModCategory", "Offroad Trucks")},

            {110, _LC("ModCategory", "Boats")},

            {113, _LC("ModCategory", "Helicopters")},
            {114, _LC("ModCategory", "Aircraft")},

            {117, _LC("ModCategory", "Trailers")},
            {118, _LC("ModCategory", "Other Loads")},

            // terrains
            {129, _LC("ModCategory", "Addon Terrains")},

            {859, _LC("ModCategory", "Container")},

            {875, _LC("ModCategory", "Submarine")},

            // note: these categories are NOT in the repository:
            {5000, _LC("ModCategory", "Official Terrains")},
            {5001, _LC("ModCategory", "Night Terrains")},

            // do not use category numbers above 9000!
            {9990, _LC("ModCategory", "Unsorted")},
            {9991, _LC("ModCategory", "All")},
            {9992, _LC("ModCategory", "Fresh")},
            {9993, _LC("ModCategory", "Hidden")}
        };
};

} // namespace RoR
