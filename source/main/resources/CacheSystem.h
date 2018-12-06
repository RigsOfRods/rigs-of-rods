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

/// @file
/// @author Thomas Fischer
/// @date   21th of May 2008
/// @brief  Caches information about installed mods (vehicles, terrains...)

#pragma once

#include "RoRPrerequisites.h"
#include "RigDef_File.h"

#include <Ogre.h>

#define CACHE_FILE "mods.cache"
#define CACHE_FILE_FORMAT "7"

// 60*60*24 = one day
#define CACHE_FILE_FRESHNESS 86400

struct Category_Entry
{
    Ogre::String title; //!< Category title
    int number;         //!< Category number
};

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
    Ogre::String hash;                  //!< file's hash
    std::string resource_bundle_type;   //!< Archive type recognized by OGRE resource system: 'FileSystem' or 'Zip'
    std::string resource_bundle_path;   //!< Path of ZIP or directory which contains the media.
    int number;                         //!< mod number
    std::time_t filetime;               //!< filetime
    bool changedornew;                  //!< is it added or changed during this runtime?
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
    ~CacheSystem();

    enum CacheValidityState
    {
        CACHE_VALID                    = 0,
        CACHE_NEEDS_UPDATE_INCREMENTAL = -1,
        CACHE_NEEDS_UPDATE_FULL        = -2,

        CACHE_STATE_UNKNOWN            = 0xFFFFFFFF
    };

    void Startup();
    CacheEntry* FindEntryByFilename(std::string const & filename); //<! Returns NULL if none found
    void UnloadActorDefFromMemory(std::string const & filename);

    bool checkResourceLoaded(CacheEntry& t); //!< Loads the associated resource bundle if not already done. Updates the bundle (resource group, loaded state)
    bool checkResourceLoaded(Ogre::String &in_out_filename); //!< Finds + loads the associated resource bundle if not already done.
    bool checkResourceLoaded(Ogre::String &in_out_filename, Ogre::String &out_group); //!< Finds given resource, outputs group name. Also loads the associated resource bundle if not already done.

    static bool resourceExistsInAllGroups(Ogre::String filename);

    std::map<int, Category_Entry> *getCategories();
    std::vector<CacheEntry> *getEntries();

    CacheEntry *getEntry(int modid);

    int getTimeStamp();

    enum CategoryID {CID_Max=9000, CID_Unsorted=9990, CID_All=9991, CID_Fresh=9992, CID_Hidden=9993, CID_SearchResults=9994};

private:

    static Ogre::String stripUIDfromString(Ogre::String uidstr); // helper
    int addUniqueString(std::set<Ogre::String> &list, Ogre::String str);

    /** parses all files loaded with a certain extension */
    void parseFilesOneRG(Ogre::String ext, Ogre::String rg);
    void parseKnownFilesOneRG(Ogre::String rg);
    void parseKnownFilesOneRGDirectory(Ogre::String rg, Ogre::String dir);

    void checkForNewKnownFiles();

    void addFile(Ogre::FileInfo f, Ogre::String ext);	// adds a file to entries
    void addFile(Ogre::String filename, Ogre::String archiveType, Ogre::String archiveDirectory, Ogre::String ext);

    // reads all advanced information out of the entry's file
    void fillTerrainDetailInfo(CacheEntry &entry, Ogre::DataStreamPtr ds, Ogre::String fname);
    void fillTruckDetailInfo(CacheEntry &entry, Ogre::DataStreamPtr ds, Ogre::String fname);

    /// Checks if update is needed
    CacheValidityState IsCacheValid();
    void GenerateHashFromFilenames();         //!< For quick detection of added/removed content
    void LoadCacheFile();                  //!< Loads all entries from cache file
    Ogre::String getCacheConfigFilename(bool full); // returns filename of the cache file
    void incrementalCacheUpdate();             // tries to update parts of the Cache only

    void generateFileCache(CacheEntry &entry, Ogre::String directory=Ogre::String());	// generates a new cache
    void deleteFileCache(char *filename); // removed files from cache
    void writeGeneratedCache();

    // adds a zip to the cache
    void loadSingleZip(Ogre::FileInfo f);
    void loadSingleZip(CacheEntry const& e);
    void loadSingleZipInternal(Ogre::String zippath, int cfactor);

    Ogre::String detectFilesMiniType(Ogre::String filename);
    void removeFileFromFileCache(std::vector<CacheEntry>::iterator it);
    Ogre::String formatEntry(int counter, CacheEntry t);
    Ogre::String formatInnerEntry(int counter, CacheEntry t);
    void parseModAttribute(const Ogre::String& line, CacheEntry& t);
    void logBadTruckAttrib(const Ogre::String& line, CacheEntry& t);
    void loadSingleDirectory(Ogre::String dirname, Ogre::String group);

    void readCategoryTitles();

    // helpers
    char *replacesSpaces(char *str);
    char *restoreSpaces(char *str);

    Ogre::String getRealPath(Ogre::String path);
    Ogre::String getVirtualPath(Ogre::String path);

    Ogre::String normalizeText(Ogre::String text);
    Ogre::String deNormalizeText(Ogre::String text);
    
    void checkForNewFiles(Ogre::String ext);

    void checkForNewContent();

    void checkForNewZipsInResourceGroup(std::set<std::string> const& resource_bundles, Ogre::String group);
    void checkForNewDirectoriesInResourceGroup(std::set<std::string>const & resource_bundles, Ogre::String group);

    bool isFileInEntries(Ogre::String filename);

    void loadAllDirectoriesInResourceGroup(Ogre::String group);
    void loadAllZipsInResourceGroup(Ogre::String group);



    std::string                  m_filenames_hash;   //!< stores SHA1 hash over the content, for quick update detection
    size_t                       m_mod_counter;      //!< counts the mods (all types)
    std::map<Ogre::String, bool> m_loaded_resource_bundles;


    int rgcounter;              //!< resource group counter, used to track the resource groups created
    
    std::vector<Ogre::String> known_extensions; //!< the extensions we track in the cache system

    std::vector<CacheEntry> entries; //!< this holds all files

    std::map<Ogre::String, Ogre::String> zipHashes;

    // categories
    std::map<int, Category_Entry> categories;
    

};
