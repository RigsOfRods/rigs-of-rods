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
#include "RefCountingObject.h"
#include "RefCountingObjectPtr.h"
#include "RigDef_File.h"
#include "SimData.h"

#include <Ogre.h>
#include <rapidjson/document.h>
#include <string>
#include <set>

#define CACHE_FILE "mods.cache"
#define CACHE_FILE_FORMAT 14
#define CACHE_FILE_FRESHNESS 86400 // 60*60*24 = one day

namespace RoR {

struct AuthorInfo
{
    int id = -1;
    Ogre::String type;
    Ogre::String name;
    Ogre::String email;
};

class CacheEntry: public RefCountingObject<CacheEntry>
{

public:

    /** default constructor resets the data. */
    CacheEntry();
    ~CacheEntry();

    CacheEntryID_t number;              //!< Sequential number, assigned internally, used by Selector-GUI

    Ogre::String fpath;                 //!< filepath relative to the .zip file
    Ogre::String fname;                 //!< filename
    Ogre::String fname_without_uid;     //!< filename
    Ogre::String fext;                  //!< file's extension
    Ogre::String dname;                 //!< name parsed from the file

    int categoryid;                     //!< category id
    Ogre::String categoryname;          //!< category name

    std::time_t addtimestamp;           //!< timestamp when this file was added to the cache
    Ogre::String uniqueid;              //!< file's unique id
    Ogre::String guid;                  //!< global unique id; Type "addonpart" leaves this empty and uses `addonpart_guids`; Always lowercase.
    int version;                        //!< file's version
    
    std::string resource_bundle_type;   //!< Archive type recognized by OGRE resource system: 'FileSystem' or 'Zip'
    std::string resource_bundle_path;   //!< Path of ZIP or directory which contains the media. Shared between CacheEntries, loaded only once.
    
    std::time_t filetime;               //!< filetime
    bool deleted;                       //!< is this mod deleted?
    int usagecounter;                   //!< how much it was used already
    std::vector<AuthorInfo> authors;    //!< authors
    Ogre::String filecachename;         //!< preview image filename

    Ogre::String resource_group;        //!< Resource group of the loaded bundle. Empty if not loaded yet.

    RigDef::DocumentPtr actor_def; //!< Cached actor definition (aka truckfile) after first spawn.
    std::shared_ptr<RoR::SkinDef> skin_def;  //!< Cached skin info, added on first use or during cache rebuild
    RoR::TuneupDefPtr tuneup_def;  //!< Cached tuning info, added on first use or during cache rebuild
    RoR::TuneupDefPtr addonpart_data_only; //!< Cached addonpart data (dummy tuneup), only used for evaluating conflicts, see `AddonPartUtility::RecordAddonpartConflicts()`
    // TBD: Make Terrn2Def a RefcountingObjectPtr<> and cache it here too.

    // following all ADDONPART detail information:
    std::set<std::string> addonpart_guids; //!< GUIDs of all vehicles this addonpart is used with.
    std::set<std::string> addonpart_filenames; //!< File names of all vehicles this addonpart is used with. If empty, any filename goes.

    // following all TUNEUP detail information:
    std::string tuneup_associated_filename; //!< Value of 'filename' field in the tuneup file; always lowercase.

    // following all TRUCK detail information:
    Ogre::String description;
    Ogre::String tags;
    std::string default_skin;
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

typedef RefCountingObjectPtr<CacheEntry> CacheEntryPtr;

enum CacheCategoryId
{
    CID_None          = 0,

    CID_Projects      = 8000, //!< For truck files under 'projects/' directory, to allow listing from editors.
    CID_Tuneups       = 8001, //!< For unsorted tuneup files.

    CID_Max           = 9000, //!< SPECIAL VALUE - Maximum allowed to be present in any mod files.
    CID_Unsorted      = 9990,
    CID_All           = 9991,
    CID_Fresh         = 9992,
    CID_Hidden        = 9993,
    CID_SearchResults = 9994,
};

struct CacheQueryResult
{
    CacheQueryResult(CacheEntryPtr entry, size_t score);

    bool operator<(CacheQueryResult const& other) const;

    CacheEntryPtr cqr_entry;
    size_t        cqr_score;
};

enum class CacheSearchMethod // Always case-insensitive
{
    NONE,     //!< Ignore the search string and find all
    FULLTEXT, //!< Partial match in: name, filename, description, author name/mail
    GUID,     //!< Partial match in: guid 
    AUTHORS,  //!< Partial match in: author name/email
    WHEELS,   //!< Wheel configuration, i.e. 4x4
    FILENAME  //!< Partial match in file name
};

struct CacheQuery
{
    RoR::LoaderType                cqy_filter_type = RoR::LoaderType::LT_None;
    int                            cqy_filter_category_id = CacheCategoryId::CID_All;
    std::string                    cqy_filter_guid; //!< Exact match (case-insensitive); leave empty to disable
    std::string                    cqy_filter_target_filename; //!< Exact match (case-insensitive); leave empty to disable (currently only used with addonparts)
    CacheSearchMethod              cqy_search_method = CacheSearchMethod::NONE;
    std::string                    cqy_search_string;
    
    std::vector<CacheQueryResult>  cqy_results;
    std::map<int, size_t>          cqy_res_category_usage; //!< Total usage (ignores search params + category filter)
    std::time_t                    cqy_res_last_update = std::time_t();

    void resetResults()
    {
        cqy_results.clear();
        cqy_res_category_usage.clear();
        cqy_res_last_update = std::time_t();
    }
};

enum class CacheValidity
{
    UNKNOWN,
    VALID,
    NEEDS_UPDATE,
    NEEDS_REBUILD,
};

enum class CreateProjectRequestType
{
    NONE,
    DEFAULT,                 //!< Copy files from source mod. Source mod Determines mod file extension.
    SAVE_TUNEUP,             //!< Dumps .tuneup file with `CID_Tuneup` from source actor, will not overwrite existing unless explicitly instructed.
};

/// Creates subdirectory in 'My Games\Rigs of Rods\projects', pre-populates it with files and adds modcache entry.
struct CreateProjectRequest
{
    CreateProjectRequest();
    ~CreateProjectRequest();

    std::string cpr_name;            //!< Directory and also the mod file (without extension).
    std::string cpr_description;     //!< Optional, implemented for tuneups.
    CacheEntryPtr cpr_source_entry;  //!< The original mod to copy files from.
    ActorPtr cpr_source_actor;       //!< Only for type `SAVE_TUNEUP`
    CreateProjectRequestType cpr_type = CreateProjectRequestType::NONE;     
    bool cpr_overwrite = false;
};

enum class ModifyProjectRequestType
{
    NONE,
    TUNEUP_USE_ADDONPART_SET,          //!< 'subject' is addonpart filename.
    TUNEUP_USE_ADDONPART_RESET,        //!< 'subject' is addonpart filename.
    TUNEUP_FORCEREMOVE_PROP_SET,       //!< 'subject_id' is prop ID.
    TUNEUP_FORCEREMOVE_PROP_RESET,     //!< 'subject_id' is prop ID.
    TUNEUP_FORCEREMOVE_FLEXBODY_SET,   //!< 'subject_id' is flexbody ID.
    TUNEUP_FORCEREMOVE_FLEXBODY_RESET, //!< 'subject_id' is flexbody ID.
    TUNEUP_FORCED_WHEEL_SIDE_SET,      //!< 'subject_id' is wheel ID, 'value_int' is RoR::WheelSide
    TUNEUP_FORCED_WHEEL_SIDE_RESET,    //!< 'subject_id' is wheel ID.
    TUNEUP_FORCEREMOVE_FLARE_SET,      //!< 'subject_id' is flare ID.
    TUNEUP_FORCEREMOVE_FLARE_RESET,    //!< 'subject_id' is flare ID.
    TUNEUP_FORCEREMOVE_EXHAUST_SET,    //!< 'subject_id' is exhaust ID.
    TUNEUP_FORCEREMOVE_EXHAUST_RESET,  //!< 'subject_id' is exhaust ID.
    TUNEUP_FORCEREMOVE_MANAGEDMAT_SET, //!< 'subject' is managed material name.
    TUNEUP_FORCEREMOVE_MANAGEDMAT_RESET,//!< 'subject' is managed material name.
    TUNEUP_PROTECTED_PROP_SET,         //!< 'subject_id' is prop ID.
    TUNEUP_PROTECTED_PROP_RESET,       //!< 'subject_id' is prop ID.
    TUNEUP_PROTECTED_FLEXBODY_SET,     //!< 'subject_id' is flexbody ID.
    TUNEUP_PROTECTED_FLEXBODY_RESET,   //!< 'subject_id' is flexbody ID.  
    TUNEUP_PROTECTED_WHEEL_SET,        //!< 'subject_id' is wheel ID.
    TUNEUP_PROTECTED_WHEEL_RESET,      //!< 'subject_id' is wheel ID.
    TUNEUP_PROTECTED_FLARE_SET,        //!< 'subject_id' is flare ID.
    TUNEUP_PROTECTED_FLARE_RESET,      //!< 'subject_id' is flare ID.
    TUNEUP_PROTECTED_EXHAUST_SET,      //!< 'subject_id' is exhaust ID.
    TUNEUP_PROTECTED_EXHAUST_RESET,    //!< 'subject_id' is exhaust ID.
    TUNEUP_PROTECTED_MANAGEDMAT_SET,   //!< 'subject' is managed material name.
    TUNEUP_PROTECTED_MANAGEDMAT_RESET, //!< 'subject' is managed material name.
    PROJECT_LOAD_TUNEUP,               //!< 'subject' is tuneup filename. This overwrites the auto-generated tuneup with the save.
    PROJECT_RESET_TUNEUP,              //!< 'subject' is empty. This resets the auto-generated tuneup to orig. values.
};

struct ModifyProjectRequest
{
    ActorPtr mpr_target_actor;
    ModifyProjectRequestType mpr_type = ModifyProjectRequestType::NONE;

    // Subject (either name or ID applies depending on type)
    std::string mpr_subject; // addonpart
    int mpr_subject_id = -1; // wheel, prop, flexbody, node
    int mpr_value_int;       // forced wheel side
};

/// A content database
/// MOTIVATION:
///    RoR users usually have A LOT of content installed. Traversing it all on every game startup would be a pain.
/// HOW IT WORKS:
///    For each recognized resource type (vehicle, terrain, skin...) an instance of 'CacheEntry' is created.
///       These entries are persisted in file CACHE_FILE (see above)
///    Associated media live in a "resource bundle" (ZIP archive or subdirectory) in content directory (ROR_HOME/mods) and subdirectories.
///       If multiple CacheEntries share a bundle, the bundle is loaded only once. Each bundle has dedicated OGRE resource group.
/// UPDATING THE CACHE:
///    Historically it was a synchronous process which could only happen at main menu, in bulk.
///    In October 2023 it became an ad-hoc process but all synchronous logic was kept, to be slowly phased out later.
///    See https://github.com/RigsOfRods/rigs-of-rods/pull/3096
class CacheSystem
{
    friend class ContentManager;
public:
    typedef std::map<int, Ogre::String> CategoryIdNameMap;

    CacheSystem();

    /// @name Startup
    /// @{
    void                  LoadModCache(CacheValidity validity);
    bool                  IsModCacheLoaded() { return m_loaded; }
    /// @}

    /// @name Lookups
    /// @{
    CacheEntryPtr         FindEntryByFilename(RoR::LoaderType type, bool partial, const std::string& filename); //!< Returns NULL if none found
    CacheEntryPtr         GetEntryByNumber(int modid);
    CacheEntryPtr         FetchSkinByName(std::string const & skin_name);
    size_t                Query(CacheQuery& query);
    /// @}

    /// @name Loading
    /// @{
    void                  LoadResource(CacheEntryPtr& t); //!< Loads the associated resource bundle if not already done.
    bool                  CheckResourceLoaded(Ogre::String &in_out_filename); //!< Finds + loads the associated resource bundle if not already done.
    bool                  CheckResourceLoaded(Ogre::String &in_out_filename, Ogre::String &out_group); //!< Finds given resource, outputs group name. Also loads the associated resource bundle if not already done.
    void                  ReLoadResource(CacheEntryPtr& t); //!< Forces reloading the associated bundle.
    void                  UnLoadResource(CacheEntryPtr& t); //!< Unloads associated bundle, destroying all spawned actors.
    void                  LoadSupplementaryDocuments(CacheEntryPtr& t); //!< Loads the associated .truck*, .skin and .tuneup files.
    void                  LoadAssetPack(CacheEntryPtr& t_dest, Ogre::String const & assetpack_filename); //!< Adds asset pack to the requesting cache entry's resource group.
    /// @}

    /// @name Projects
    /// @{
    CacheEntryPtr         CreateProject(CreateProjectRequest* request); //!< Creates subdirectory in 'My Games\Rigs of Rods\projects', pre-populates it with files and adds modcache entry.
    void                  ModifyProject(ModifyProjectRequest* request);
    void                  DeleteProject(CacheEntryPtr& entry);
    /// @}

    const std::vector<CacheEntryPtr>   &GetEntries()        const { return m_entries; }
    const CategoryIdNameMap         &GetCategories()     const { return m_categories; }

    Ogre::String GetPrettyName(Ogre::String fname);
    std::string ActorTypeToName(ActorType driveable);

    const std::vector<std::string>& GetContentDirs() const { return m_content_dirs; }

private:

    CacheValidity EvaluateCacheValidity(); // Called by `ContentManager` on startup only.

    void WriteCacheFileJson();
    void ExportEntryToJson(rapidjson::Value& j_entries, rapidjson::Document& j_doc, CacheEntryPtr const & entry);
    CacheValidity LoadCacheFileJson();
    void ImportEntryFromJson(rapidjson::Value& j_entry, CacheEntryPtr & out_entry);

    static Ogre::String StripUIDfromString(Ogre::String uidstr); 
    static Ogre::String StripSHA1fromString(Ogre::String sha1str);
    static std::string ComposeResourceGroupName(const CacheEntryPtr& entry);

    void ParseZipArchives(Ogre::String group);
    bool ParseKnownFiles(Ogre::String group); // returns true if no known files are found
    void ParseSingleZip(Ogre::String path);

    void ClearCache(); // removes                   all files from the cache
    void PruneCache(); // removes modified (or deleted) files from the cache
    void ClearResourceGroups();

    void AddFile(Ogre::String group, Ogre::FileInfo f, Ogre::String ext);

    void DetectDuplicates();

    /// @name Document loading helpers
    /// @{
    void LoadAssociatedSkinDef(CacheEntryPtr& cache_entry); //!< Loads+parses the .skin file and updates all related CacheEntries
    void LoadAssociatedTuneupDef(CacheEntryPtr& cache_entry); //!< Loads+parses the .tuneup file and updates all related CacheEntries
    /// @}

    /// @name Cache update helpers
    /// @{
    void FillTerrainDetailInfo(CacheEntryPtr &entry, Ogre::DataStreamPtr ds, Ogre::String fname);
    void FillTruckDetailInfo(CacheEntryPtr &entry, Ogre::DataStreamPtr ds, Ogre::String fname, Ogre::String group);
    void FillSkinDetailInfo(CacheEntryPtr &entry, std::shared_ptr<SkinDef>& skin_def);
    void FillAddonPartDetailInfo(CacheEntryPtr &entry, Ogre::DataStreamPtr ds);
    void FillTuneupDetailInfo(CacheEntryPtr &entry, TuneupDefPtr& tuneup_def);
    void FillAssetPackDetailInfo(CacheEntryPtr &entry, Ogre::DataStreamPtr ds);
    /// @}

    void GenerateHashFromFilenames();         //!< For quick detection of added/removed content

    void GenerateFileCache(CacheEntryPtr &entry, Ogre::String group);
    void RemoveFileCache(CacheEntryPtr &entry);

    bool Match(size_t& out_score, std::string data, std::string const& query, size_t );

    bool IsPathContentDirRoot(const std::string& path) const;

    bool                                 m_loaded = false;
    std::time_t                          m_update_time;      //!< Ensures that all inserted files share the same timestamp
    std::string                          m_filenames_hash_loaded;   //!< hash from cachefile, for quick update detection
    std::string                          m_filenames_hash_generated;   //!< stores hash over the content, for quick update detection
    std::vector<CacheEntryPtr>           m_entries;
    std::vector<Ogre::String>            m_known_extensions; //!< the extensions we track in the cache system
    std::vector<std::string>             m_content_dirs;     //!< the various mod directories we track in the cache system
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

            {CID_Projects, _LC("ModCategory", "Projects")},
            {CID_Tuneups, _LC("ModCategory", "Tuneups")},

            // do not use category numbers above 9000!
            {CID_Unsorted, _LC("ModCategory", "Unsorted")},
            {CID_All, _LC("ModCategory", "All")},
            {CID_Fresh, _LC("ModCategory", "Fresh")},
            {CID_Hidden, _LC("ModCategory", "Hidden")},
        };
};

} // namespace RoR
