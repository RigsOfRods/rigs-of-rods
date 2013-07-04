/*
This source file is part of Rigs of Rods
Copyright 2005-2012 Pierre-Michel Ricordel
Copyright 2007-2012 Thomas Fischer

For more information, see http://www.rigsofrods.com/

Rigs of Rods is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License version 3, as
published by the Free Software Foundation.

Rigs of Rods is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Rigs of Rods.  If not, see <http://www.gnu.org/licenses/>.
*/
// created on 21th of May 2008 by Thomas

#ifndef __CacheSystem_H_
#define __CacheSystem_H_

#include "RoRPrerequisites.h"

#include "BeamData.h" // for authorinfo_t
#include "Singleton.h"

#include <Ogre.h>

#define CACHE_FILE "mods.cache"
#define CACHE_FILE_FORMAT "6"

// 60*60*24 = one day
#define CACHE_FILE_FRESHNESS 86400

#define CACHE CacheSystem::getSingleton()

typedef struct
{
	Ogre::String title;					// Category title
	int number;							// Category number
} Category_Entry;

class CacheEntry
{
public:
	Ogre::String minitype;				// type of preview picture, either png or dds
	Ogre::String fname;					// filename
	Ogre::String fname_without_uid;		// filename
	Ogre::String dname;					// name parsed from the file
	
	int categoryid;						// category id
	Ogre::String categoryname;			// category name
	
	int addtimestamp;					// timestamp when this file was added to the cache
	
	Ogre::String uniqueid;				// file's unique id
	Ogre::String guid;                  // global unique id
	int version;						// file's version
	Ogre::String fext;					// file's extension
	Ogre::String type;					// Resource Type, FileSystem or Zip
	Ogre::String dirname;				// mostly, archive name
	Ogre::String hash;					// file's hash
	bool resourceLoaded;				// loaded?
	int number;							// mod number
	std::time_t filetime;				// filetime
	bool changedornew;					// is it added or changed during this runtime?
	bool deleted;						// is this mod deleted?
	int usagecounter;					// how much it was used already
	std::vector<authorinfo_t> authors;	// authors
	Ogre::String filecachename;			// preview image filename

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
	int materialflarebindingscount;
	int soundsourcescount;
	int managedmaterialscount;

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
	std::set<Ogre::String> materials;

	// default constructor resets the data.
	CacheEntry() :
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
};

class CacheSystem : public RoRSingleton<CacheSystem>, public ZeroedMemoryAllocator
{
	friend class RoRSingleton<CacheSystem>;
public:	
	void startup(bool forcecheck=false);
	void loadAllZips();
	
	static Ogre::String stripUIDfromString(Ogre::String uidstr);
	static Ogre::String getUIDfromString(Ogre::String uidstr);
	static bool stringHasUID(Ogre::String uidstr);
	
	void loadAllZipsInResourceGroup(Ogre::String group);

	bool checkResourceLoaded(CacheEntry t);
	bool checkResourceLoaded(Ogre::String &filename);
	bool checkResourceLoaded(Ogre::String &filename, Ogre::String &group);
	CacheEntry getResourceInfo(Ogre::String &filename);
	Ogre::String addMeshMaterials(CacheEntry &entry, Ogre::Entity *e);
	std::map<int, Category_Entry> *getCategories();
	std::vector<CacheEntry> *getEntries();

	int getCategoryUsage(int category);
	CacheEntry *getEntry(int modid);

	int getTimeStamp();

	// this location MUST include a path separator at the end!
	void setLocation(Ogre::String cachepath, Ogre::String configpath);
	
	// this is for stats only, maybe protect it by getter later
	int changedFiles, newFiles, deletedFiles;

	void loadSingleZip(Ogre::String zippath, int cfactor, bool unload=true, bool ownGroup=true);
	void loadSingleDirectory(Ogre::String dirname, Ogre::String group, bool alreadyLoaded=true);

	static bool resourceExistsInAllGroups(Ogre::String filename);

	// see: https://code.google.com/p/rigsofrods-streams/source/browse/trunk/0.39/win32-skeleton/config/categories.cfg
	enum CategoryID {CID_Max=9000, CID_Unsorted=9990, CID_All=9991, CID_Fresh=9992, CID_Hidden=9993, CID_SearchResults=9994};
protected:
	CacheSystem();
	~CacheSystem();
	CacheSystem(const CacheSystem&);
	CacheSystem& operator= (const CacheSystem&);
	static CacheSystem* myInstance;
	Ogre::String location;
	Ogre::String configlocation;


	Ogre::String currentSHA1;	// stores sha1 over the content
	int rgcounter;				// resource group counter, used to track the resource groups created
	int modcounter;				// counter the number of mods



	// the extensions we track in the cache system
	std::vector<Ogre::String> known_extensions;

	int addUniqueString(std::set<Ogre::String> &list, Ogre::String str);


	// parses all files loaded with a certain extension
	void parseFilesAllRG(Ogre::String ext);
	void parseFilesOneRG(Ogre::String ext, Ogre::String rg);
	void parseKnownFilesOneRG(Ogre::String rg);
	void parseKnownFilesAllRG();
	void parseKnownFilesOneRGDirectory(Ogre::String rg, Ogre::String dir);

	void checkForNewKnownFiles();


	void addFile(Ogre::FileInfo f, Ogre::String ext);	// adds a file to entries
	void addFile(Ogre::String filename, Ogre::String archiveType, Ogre::String archiveDirectory, Ogre::String ext);

	// reads all advanced information out of the entry's file
	void fillTerrainDetailInfo(CacheEntry &entry, Ogre::DataStreamPtr ds, Ogre::String fname);
	void fillTruckDetailInfo(CacheEntry &entry, Ogre::DataStreamPtr ds, Ogre::String fname);

	int isCacheValid();                       // validate cache
	void unloadUselessResourceGroups();       // unload unused resources after cache generation
	Ogre::String filenamesSHA1();             // generates the hash over the whole content
	bool loadCache();			              // loads cache config file, new format
	Ogre::String getCacheConfigFilename(bool full); // returns filename of the cache file
	int incrementalCacheUpdate();             // tries to update parts of the Cache only

	void generateFileCache(CacheEntry &entry, Ogre::String directory=Ogre::String());	// generates a new cache
	void deleteFileCache(char *filename); // removed files from cache
	void writeGeneratedCache();
	void writeStreamCache();
	
	// adds a zip to the cache
	void loadSingleZip(Ogre::FileInfo f, bool unload=true, bool ownGroup=true);
	void loadSingleZip(CacheEntry e, bool unload=true, bool ownGroup=true);

	Ogre::String detectFilesMiniType(Ogre::String filename);
	void removeFileFromFileCache(std::vector<CacheEntry>::iterator it);
	void generateCache(bool forcefull=false);
	Ogre::String formatEntry(int counter, CacheEntry t);
	Ogre::String formatInnerEntry(int counter, CacheEntry t);
	void updateSingleTruckEntryCache(int number, CacheEntry t);
	void parseModAttribute(const Ogre::String& line, CacheEntry& t);
	void logBadTruckAttrib(const Ogre::String& line, CacheEntry& t);

	// this holds all files
	std::vector<CacheEntry> entries;

	std::map<Ogre::String, Ogre::String> zipHashes;

	// categories
	std::map<int, Category_Entry> categories;
	std::map<int, int> category_usage;
	std::set<Ogre::String> zipCacheList;
	void readCategoryTitles();


	// helpers
	char *replacesSpaces(char *str);
	char *restoreSpaces(char *str);
	
	std::time_t fileTime(Ogre::String filename);

	Ogre::String getRealPath(Ogre::String path);
	Ogre::String getVirtualPath(Ogre::String path);

	Ogre::String normalizeText(Ogre::String text);
	Ogre::String deNormalizeText(Ogre::String text);
	
	void checkForNewFiles(Ogre::String ext);

	void checkForNewContent();

	void checkForNewZipsInResourceGroup(Ogre::String group);
	void checkForNewDirectoriesInResourceGroup(Ogre::String group);

	void generateZipList();
	bool isZipUsedInEntries(Ogre::String filename);
	bool isFileInEntries(Ogre::String filename);

	bool isDirectoryUsedInEntries(Ogre::String directory);

	void loadAllDirectoriesInResourceGroup(Ogre::String group);

};

#endif // __CacheSystem_H_
