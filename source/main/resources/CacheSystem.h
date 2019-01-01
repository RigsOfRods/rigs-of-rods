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
/// @brief  A database of user-installed content alias 'mods' (vehicles, terrains...)

#pragma once

#include "RoRPrerequisites.h"
#include "RigDef_File.h"

#include <Ogre.h>

#define CACHE_FILE "mods.cache"
#define CACHE_FILE_FORMAT 8

// 60*60*24 = one day
#define CACHE_FILE_FRESHNESS 86400

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

    Ogre::String minitype;              //!< type of preview picture, either png or dds
    Ogre::String fpath;                 //!< filepath relative to the .zip file
    Ogre::String fname;                 //!< filename
    Ogre::String fname_without_uid;     //!< filename
    Ogre::String dname;                 //!< name parsed from the file
    
    int categoryid;                     //!< category id
    Ogre::String categoryname;          //!< category name
    
    int addtimestamp;                   //!< timestamp when this file was added to the cache
    
    Ogre::String uniqueid;              //!< file's unique id
    Ogre::String guid;                  //!< global unique id
    int version;                        //!< file's version
    Ogre::String fext;                  //!< file's extension
    std::string resource_bundle_type;   //!< Archive type recognized by OGRE resource system: 'FileSystem' or 'Zip'
    std::string resource_bundle_path;   //!< Path of ZIP or directory which contains the media.
    int number;                         //!< Sequential number, assigned internally, used by Selector-GUI
    std::time_t filetime;               //!< filetime
    bool deleted;                       //!< is this mod deleted?
    int usagecounter;                   //!< how much it was used already
    std::vector<AuthorInfo> authors;    //!< authors
    Ogre::String filecachename;         //!< preview image filename

    std::shared_ptr<RigDef::File> actor_def; //!< Cached actor definition (aka truckfile) after first spawn

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
    bool rollon;
    bool rescuer;

    int driveable;
    int numgears;
    char enginetype;
    std::vector<Ogre::String> sectionconfigs;
};

/// A content database
/// MOTIVATION:
///    RoR users usually have A LOT of content installed. Traversing it all on every game startup would be a pain.
/// HOW IT WORKS:
///    For each recognized resource type (vehicle, terrain, skin...) an instance of 'CacheEntry' is created.
///       These entries are persisted in file CACHE_FILE (see above)
///    Associated media live in a "resource bundle" (ZIP archive or subdirectory) in content directories (ROR_HOME/vehicles, ROR_HOME/terrains etc...)
///       Bundles are shared among cache entries and loaded only once, when related CacheEntry is loaded.
class CacheSystem : public ZeroedMemoryAllocator
{
public:

    CacheSystem();

    enum CacheValidityState
    {
        CACHE_VALID                    = 0,
        CACHE_NEEDS_UPDATE_INCREMENTAL = -1,
        CACHE_NEEDS_UPDATE_FULL        = -2,

        CACHE_STATE_UNKNOWN            = 0xFFFFFFFF
    };

    void                  LoadModCache(CacheValidityState validity);
    CacheEntry*           FindEntryByFilename(std::string const & filename); //<! Returns NULL if none found
    void                  UnloadActorDefFromMemory(std::string const & filename);
    CacheValidityState    EvaluateCacheValidity();

    bool checkResourceLoaded(CacheEntry& t); //!< Loads the associated resource bundle if not already done. Updates the bundle (resource group, loaded state)
    bool checkResourceLoaded(Ogre::String &in_out_filename); //!< Finds + loads the associated resource bundle if not already done.
    bool checkResourceLoaded(Ogre::String &in_out_filename, Ogre::String &out_group); //!< Finds given resource, outputs group name. Also loads the associated resource bundle if not already done.

    static bool resourceExistsInAllGroups(Ogre::String filename);

    std::map<int, Ogre::String> *getCategories();
    std::vector<CacheEntry> *getEntries();

    CacheEntry *getEntry(int modid);
    Ogre::String getPrettyName(Ogre::String fname);

    Ogre::String getCacheConfigFilename(); // returns absolute path of the cache file

    int getTimeStamp();

    enum CategoryID {CID_Max=9000, CID_Unsorted=9990, CID_All=9991, CID_Fresh=9992, CID_Hidden=9993, CID_SearchResults=9994};

private:

    void WriteCacheFileJson();
    void ExportEntryToJson(rapidjson::Value& j_entries, rapidjson::Document& j_doc, CacheEntry const & entry);
    void LoadCacheFileJson();
    void ImportEntryFromJson(rapidjson::Value& j_entry, CacheEntry & out_entry);

    static Ogre::String stripUIDfromString(Ogre::String uidstr); // helper
    int addUniqueString(std::set<Ogre::String> &list, Ogre::String str);

    static Ogre::String stripSHA1fromString(Ogre::String sha1str); // helper

    /** parses all files loaded with a certain extension */
    void parseFilesOneRG(Ogre::String ext, Ogre::String rg);
    void parseKnownFilesOneRG(Ogre::String rg);
    void parseKnownFilesOneRGDirectory(Ogre::String rg, Ogre::String dir);

    void checkForNewKnownFiles();

    void addFile(Ogre::FileInfo f, Ogre::String ext);	// adds a file to entries
    void addFile(Ogre::String filename, Ogre::String filepath, Ogre::String archiveType, Ogre::String archiveDirectory, Ogre::String ext);

    // reads all advanced information out of the entry's file
    void fillTerrainDetailInfo(CacheEntry &entry, Ogre::DataStreamPtr ds, Ogre::String fname);
    void fillTruckDetailInfo(CacheEntry &entry, Ogre::DataStreamPtr ds, Ogre::String fname);

    void GenerateHashFromFilenames();         //!< For quick detection of added/removed content
    void incrementalCacheUpdate();             // tries to update parts of the Cache only
    void detectDuplicates();                   // tries to detect duplicates

    void generateFileCache(CacheEntry &entry);	// generates a new cache
    void deleteFileCache(const char *full_path); //!< Delete single file from cache
    void deleteFileCache(std::string const& full_path) { this->deleteFileCache(full_path.c_str()); }

    // adds a zip to the cache
    void loadSingleZip(Ogre::FileInfo f);
    void loadSingleZipInternal(Ogre::String zippath, int cfactor);

    Ogre::String detectFilesMiniType(Ogre::String filename);
    void removeFileFromFileCache(std::vector<CacheEntry>::iterator it);
    void loadSingleDirectory(Ogre::String dirname, Ogre::String group);

    Ogre::String getRealPath(Ogre::String path);
    Ogre::String getVirtualPath(Ogre::String path);

    void checkForNewFiles(Ogre::String ext);

    void checkForNewContent();

    void checkForNewZipsInResourceGroup(std::set<std::string> const& resource_bundles, Ogre::String group);
    void checkForNewDirectoriesInResourceGroup(std::set<std::string>const & resource_bundles, Ogre::String group);

    bool isFileInEntries(Ogre::String filename);

    void loadAllDirectoriesInResourceGroup(Ogre::String group);
    void loadAllZipsInResourceGroup(Ogre::String group);

    std::string                          m_filenames_hash;   //!< stores hash over the content, for quick update detection
    std::map<Ogre::String, bool>         m_loaded_resource_bundles;
    std::vector<CacheEntry>              m_entries;
    std::vector<Ogre::String>            m_known_extensions; //!< the extensions we track in the cache system
    std::map<int, Ogre::String>          m_categories = {
            // these are the category numbers from the repository. do not modify them!

            // vehicles
            {108, "Other Land Vehicles"},

            {146, "Street Cars"},
            {147, "Light Racing Cars"},
            {148, "Offroad Cars"},
            {149, "Fantasy Cars"},
            {150, "Bikes"},
            {155, "Crawlers"},

            {152, "Towercranes"},
            {153, "Mobile Cranes"},
            {154, "Other cranes"},

            {107, "Buses"},
            {151, "Tractors"},
            {156, "Forklifts"},
            {159, "Fantasy Trucks"},
            {160, "Transport Trucks"},
            {161, "Racing Trucks"},
            {162, "Offroad Trucks"},

            {110, "Boats"},

            {113, "Helicopters"},
            {114, "Aircraft"},

            {117, "Trailers"},
            {118, "Other Loads"},

            // terrains
            {129, "Addon Terrains"},

            {859, "Container"},

            {875, "Submarine"},

            // note: these categories are NOT in the repository:
            {5000, "Official Terrains"},
            {5001, "Night Terrains"},

            // do not use category numbers above 9000!
            {9990, "Unsorted"},
            {9991, "All"},
            {9992, "Fresh"},
            {9993, "Hidden"}
        };
};
