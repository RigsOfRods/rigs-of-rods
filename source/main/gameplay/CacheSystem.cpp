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

#include "CacheSystem.h"

#include "BeamEngine.h"
#include "ErrorUtils.h"
#include "ImprovedConfigFile.h"
#include "Language.h"
#include "SerializedRig.h"
#include "Settings.h"
#include "SHA1.h"
#include "SoundScriptManager.h"
#include "Utils.h"
#include "TerrainManager.h"
	
#ifdef USE_MYGUI
#include "LoadingWindow.h"
#endif // USE_MYGUI

#include <OgreFileSystem.h>

using namespace Ogre;

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

	if (BSETTING("streamCacheGenerationOnly", false))
	{
		writeStreamCache();
		exit(0);
	}
}

CacheSystem::~CacheSystem()
{
}

void CacheSystem::setLocation(String cachepath, String configpath)
{
	location=cachepath;
	configlocation=configpath;
}

void CacheSystem::startup(bool forcecheck)
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

	int valid = isCacheValid();
	if (forcecheck)
		valid = -1;
	// valid == -1 : try incemental update
	// valid == -2 : must update fully
	if (valid<0)
	{
		LOG("cache invalid, updating ...");
		// generate the cache
		generateCache((valid < -1));

		//LOG("unloading unused resource groups ...");
		// important: unload everything again!
		//unloadUselessResourceGroups();
	}

	LOG("loading cache...");
	// load the cache finally!
	loadCache();

	// show error on zero content
	if (entries.empty())
	{
		showOgreWebError(_L("No content installed"), _L("You have no content installed"), _L("http://www.rigsofrods.com/wiki/pages/Install_Content"));
		showStoredOgreWebErrors();
		exit(1337);
	}

	LOG("cache loaded!");
}

std::map<int, Category_Entry> *CacheSystem::getCategories()
{
	return &categories;
}

std::vector<CacheEntry> *CacheSystem::getEntries()
{
	return &entries;
}

void CacheSystem::unloadUselessResourceGroups()
{
	StringVector sv = ResourceGroupManager::getSingleton().getResourceGroups();
	StringVector::iterator it;
	for (it = sv.begin(); it!=sv.end(); it++)
	{
		if (it->substr(0, 8) == "General-")
		{
			try
			{
#ifdef USE_OPENAL
				SoundScriptManager::getSingleton().clearNonBaseTemplates();
#endif //OPENAL
				// we cannot fix this problem below Ogre version 1.7
				ParticleSystemManager::getSingleton().removeTemplatesByResourceGroup(*it);
				ResourceGroupManager::getSingleton().clearResourceGroup(*it);
				ResourceGroupManager::getSingleton().unloadResourceGroup(*it);
				ResourceGroupManager::getSingleton().destroyResourceGroup(*it);
			} catch(Ogre::Exception& e)
			{
				LOG("error while unloading resource groups: " + e.getFullDescription());
				LOG("trying to continue ...");
			}
		}
	}
}

String CacheSystem::getCacheConfigFilename(bool full)
{
	if (full) return location+String(CACHE_FILE);
	return String(CACHE_FILE);
}

// we implement this on our own, since we cannot reply on the ogre version
bool CacheSystem::resourceExistsInAllGroups(Ogre::String filename)
{
	String group;
	bool exists=true;
	try
	{
		group = ResourceGroupManager::getSingleton().findGroupContainingResource(filename);
	}catch(...)
	{
		exists=false;
	}
	if (group.empty())
		exists = false;
	return exists;
}

int CacheSystem::isCacheValid()
{
	String cfgfilename = getCacheConfigFilename(false);
	ImprovedConfigFile cfg;
	ConfigFile ff;
	if (!resourceExistsInAllGroups(cfgfilename))
	{
		LOG("unable to load config file: "+cfgfilename);
		return -2;
	}

	String group = ResourceGroupManager::getSingleton().findGroupContainingResource(cfgfilename);
	DataStreamPtr stream=ResourceGroupManager::getSingleton().openResource(cfgfilename, group);
	cfg.load(stream, "\t:=", false);
	String shaone = cfg.getSetting("shaone");
	String cacheformat = cfg.getSetting("cacheformat");

	if (shaone == "" || shaone != currentSHA1)
	{
		LOG("* mod cache is invalid (not up to date), regenerating new one ...");
		return -1;
	}
	if (cacheformat != String(CACHE_FILE_FORMAT))
	{
		entries.clear();
		LOG("* mod cache has invalid format, trying to regenerate");
		return -1;
	}
	LOG("* mod cache is valid, using it.");
	return 0;
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
		t.categoryname=categories[t.categoryid].title;
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
		authorinfo_t ai;
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
		if (params.size() < 2) { logBadTruckAttrib(line, t); return; } else  t.description = deNormalizeText(params[1]);
	else if (attrib == "tags")
		if (params.size() < 2) { logBadTruckAttrib(line, t); return; } else  t.tags = params[1];
	else if (attrib == "fileformatversion")
		if (params.size() != 2) { logBadTruckAttrib(line, t); return; } else  t.fileformatversion = StringConverter::parseInt(params[1]);
	else if (attrib == "hasSubmeshs")
		if (params.size() != 2) { logBadTruckAttrib(line, t); return; } else  t.hasSubmeshs = StringConverter::parseBool(params[1]);
	else if (attrib == "nodecount")
		if (params.size() != 2) { logBadTruckAttrib(line, t); return; } else  t.nodecount = StringConverter::parseInt(params[1]);
	else if (attrib == "beamcount")
		if (params.size() != 2) { logBadTruckAttrib(line, t); return; } else  t.beamcount = StringConverter::parseInt(params[1]);
	else if (attrib == "shockcount")
		if (params.size() != 2) { logBadTruckAttrib(line, t); return; } else  t.shockcount = StringConverter::parseInt(params[1]);
	else if (attrib == "fixescount")
		if (params.size() != 2) { logBadTruckAttrib(line, t); return; } else  t.fixescount = StringConverter::parseInt(params[1]);
	else if (attrib == "hydroscount")
		if (params.size() != 2) { logBadTruckAttrib(line, t); return; } else  t.hydroscount = StringConverter::parseInt(params[1]);
	else if (attrib == "wheelcount")
		if (params.size() != 2) { logBadTruckAttrib(line, t); return; } else  t.wheelcount = StringConverter::parseInt(params[1]);
	else if (attrib == "propwheelcount")
		if (params.size() != 2) { logBadTruckAttrib(line, t); return; } else  t.propwheelcount = StringConverter::parseInt(params[1]);
	else if (attrib == "commandscount")
		if (params.size() != 2) { logBadTruckAttrib(line, t); return; } else  t.commandscount = StringConverter::parseInt(params[1]);
	else if (attrib == "flarescount")
		if (params.size() != 2) { logBadTruckAttrib(line, t); return; } else  t.flarescount = StringConverter::parseInt(params[1]);
	else if (attrib == "propscount")
		if (params.size() != 2) { logBadTruckAttrib(line, t); return; } else  t.propscount = StringConverter::parseInt(params[1]);
	else if (attrib == "wingscount")
		if (params.size() != 2) { logBadTruckAttrib(line, t); return; } else  t.wingscount = StringConverter::parseInt(params[1]);
	else if (attrib == "turbopropscount")
		if (params.size() != 2) { logBadTruckAttrib(line, t); return; } else  t.turbopropscount = StringConverter::parseInt(params[1]);
	else if (attrib == "turbojetcount")
		if (params.size() != 2) { logBadTruckAttrib(line, t); return; } else  t.turbojetcount = StringConverter::parseInt(params[1]);
	else if (attrib == "rotatorscount")
		if (params.size() != 2) { logBadTruckAttrib(line, t); return; } else  t.rotatorscount = StringConverter::parseInt(params[1]);
	else if (attrib == "exhaustscount")
		if (params.size() != 2) { logBadTruckAttrib(line, t); return; } else  t.exhaustscount = StringConverter::parseInt(params[1]);
	else if (attrib == "flexbodiescount")
		if (params.size() != 2) { logBadTruckAttrib(line, t); return; } else  t.flexbodiescount = StringConverter::parseInt(params[1]);
	else if (attrib == "materialflarebindingscount")
		if (params.size() != 2) { logBadTruckAttrib(line, t); return; } else  t.materialflarebindingscount = StringConverter::parseInt(params[1]);
	else if (attrib == "soundsourcescount")
		if (params.size() != 2) { logBadTruckAttrib(line, t); return; } else  t.soundsourcescount = StringConverter::parseInt(params[1]);
	else if (attrib == "managedmaterialscount")
		if (params.size() != 2) { logBadTruckAttrib(line, t); return; } else  t.managedmaterialscount = StringConverter::parseInt(params[1]);
	else if (attrib == "truckmass")
		if (params.size() != 2) { logBadTruckAttrib(line, t); return; } else  t.truckmass = StringConverter::parseReal(params[1]);
	else if (attrib == "loadmass")
		if (params.size() != 2) { logBadTruckAttrib(line, t); return; } else  t.loadmass = StringConverter::parseReal(params[1]);
	else if (attrib == "minrpm")
		if (params.size() != 2) { logBadTruckAttrib(line, t); return; } else  t.minrpm = StringConverter::parseReal(params[1]);
	else if (attrib == "maxrpm")
		if (params.size() != 2) { logBadTruckAttrib(line, t); return; } else  t.maxrpm = StringConverter::parseReal(params[1]);
	else if (attrib == "torque")
		if (params.size() != 2) { logBadTruckAttrib(line, t); return; } else  t.torque = StringConverter::parseReal(params[1]);
	else if (attrib == "customtach")
		if (params.size() != 2) { logBadTruckAttrib(line, t); return; } else  t.customtach = StringConverter::parseBool(params[1]);
	else if (attrib == "custom_particles")
		if (params.size() != 2) { logBadTruckAttrib(line, t); return; } else  t.custom_particles = StringConverter::parseBool(params[1]);
	else if (attrib == "forwardcommands")
		if (params.size() != 2) { logBadTruckAttrib(line, t); return; } else  t.forwardcommands = StringConverter::parseBool(params[1]);
	else if (attrib == "rollon")
		if (params.size() != 2) { logBadTruckAttrib(line, t); return; } else  t.rollon = StringConverter::parseBool(params[1]);
	else if (attrib == "rescuer")
		if (params.size() != 2) { logBadTruckAttrib(line, t); return; } else  t.rescuer = StringConverter::parseBool(params[1]);
	else if (attrib == "driveable")
		if (params.size() != 2) { logBadTruckAttrib(line, t); return; } else  t.driveable = StringConverter::parseInt(params[1]);
	else if (attrib == "numgears")
		if (params.size() != 2) { logBadTruckAttrib(line, t); return; } else  t.numgears = StringConverter::parseInt(params[1]);
	else if (attrib == "enginetype")
		if (params.size() != 2) { logBadTruckAttrib(line, t); return; } else  t.enginetype = StringConverter::parseInt(params[1]);
	else if (attrib == "materials")
		if (params.size() < 2)
		{
			logBadTruckAttrib(line, t);
			return;
		}
		else
		{
			String mat = params[1];
			Ogre::StringVector ar = StringUtil::split(mat," ");
			for (Ogre::StringVector::iterator it = ar.begin(); it!=ar.end(); it++)
				t.materials.insert(*it);
		}
}

bool CacheSystem::loadCache()
{
	// Clear existing entries
	entries.clear();

	String cfgfilename = getCacheConfigFilename(false);

	if ( !resourceExistsInAllGroups(cfgfilename) )
	{
		LOG("unable to load config file: "+cfgfilename);
		return false;
	}

	String group = ResourceGroupManager::getSingleton().findGroupContainingResource(String(cfgfilename));
	DataStreamPtr stream=ResourceGroupManager::getSingleton().openResource(cfgfilename, group);

	LOG("CacheSystem::loadCache");

	CacheEntry t;
	String line = "";
	int mode = 0;

	while( !stream->eof() )
	{
		line = stream->getLine();

		// Ignore blanks & comments
		if ( line.empty() || line.substr( 0, 2 ) == "//" )
		{
			continue;
		}

		// Skip these
		if ( StringUtil::startsWith(line, "shaone=") || StringUtil::startsWith(line, "modcount=") || StringUtil::startsWith(line, "cacheformat=") )
		{
			continue;
		}

		if ( mode == 0 )
		{
			// No current entry
			if ( line == "mod" )
			{
				mode = 1;
				t = CacheEntry();
				t.resourceLoaded = false;
				t.deleted        = false;
				t.changedornew   = false; // default upon loading
				// Skip to and over next {
				stream->skipLine("{");
			}
		} else if ( mode == 1 )
		{
			// Already in mod
			if ( line == "}" )
			{
				// Finished
				if ( !t.deleted )
				{
					entries.push_back(t);
				}
				mode = 0;
			} else
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
#ifdef USE_MYGUI
	LoadingWindow::getSingleton().setProgress(20, _L("incremental check: deleted and changed files"));
#endif //USE_MYGUI
	std::vector<CacheEntry> changed_entries;
	UTFString tmp = "";
	String fn = "";
	int counter = 0;
	for (std::vector<CacheEntry>::iterator it = entries.begin(); it != entries.end(); it++, counter++)
	{
#ifdef USE_MYGUI
		int progress = ((float)counter/(float)(entries.size()))*100;
		tmp = _L("incremental check: deleted and changed files\n") + ANSI_TO_UTF(it->type) + _L(": ") + ANSI_TO_UTF(it->fname);
		LoadingWindow::getSingleton().setProgress(progress, tmp);
#endif //USE_MYGUI
		// check whether the file exists
		if (it->type == "Zip")
			fn = getRealPath(it->dirname);
		else if (it->type == "FileSystem")
			fn = getRealPath(it->dirname + "/" + it->fname);

		if ((it->type == "FileSystem" || it->type == "Zip") && !fileExists(fn.c_str()))
		{
			LOG("- "+fn+" is not existing");
#ifdef USE_MYGUI
			tmp = _L("incremental check: deleted and changed files\n") + ANSI_TO_UTF(it->fname) + _L(" not existing");
			LoadingWindow::getSingleton().setProgress(20, tmp);
#endif //USE_MYGUI
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
			} else
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
#ifdef USE_MYGUI
	LoadingWindow::getSingleton().setProgress(40, _L("incremental check: processing changed zips\n"));
#endif //USE_MYGUI
	for (std::vector<CacheEntry>::iterator it = changed_entries.begin(); it != changed_entries.end(); it++)
	{
		bool found=false;
		for (std::vector<Ogre::String>::iterator it2 = reloaded_zips.begin(); it2!=reloaded_zips.end(); it2++)
		{
			if (*it2 == it->dirname)
			{
				found=true;
				break;
			}
		}
		if (!found)
		{
#ifdef USE_MYGUI
			LoadingWindow::getSingleton().setProgress(40, _L("incremental check: processing changed zips\n")+it->fname);
#endif //USE_MYGUI
			loadSingleZip(*it);
			reloaded_zips.push_back(it->dirname);
		}
	}
	LOG("* incremental check (3/5): new content ...");
#ifdef USE_MYGUI
	LoadingWindow::getSingleton().setProgress(60, _L("incremental check: new content\n"));
#endif //USE_MYGUI
	checkForNewContent();

	LOG("* incremental check (4/5): new files ...");
#ifdef USE_MYGUI
	LoadingWindow::getSingleton().setProgress(80, _L("incremental check: new files\n"));
#endif //USE_MYGUI
	checkForNewKnownFiles();

	LOG("* incremental check (5/5): duplicates ...");
#ifdef USE_MYGUI
	LoadingWindow::getSingleton().setProgress(90, _L("incremental check: duplicates\n"));
#endif //USE_MYGUI
	for (std::vector<CacheEntry>::iterator it = entries.begin(); it != entries.end(); it++)
	{
		if (it->deleted) continue;
		for (std::vector<CacheEntry>::iterator it2 = entries.begin(); it2 != entries.end(); it2++)
		{
			if (it2->deleted) continue;
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
				if (it->number == it2->number) continue; // do not delete self
				LOG("- "+ it2->dirname+"/" + it->fname + " hard duplicate");
				it2->deleted=true;
				continue;
			}
			// soft duplicates
			else if (dira == dirb && dnameA == dnameB && filenameWUIDA == filenameWUIDB)
			{
				if (it->number == it2->number) continue; // do not delete self
				LOG("- "+ it2->dirname+"/" + it->fname + " soft duplicate, resolving ...");
				// create sha1 and see whats the correct entry :)
				RoR::CSHA1 sha1;
				sha1.HashFile(const_cast<char*>(it2->dirname.c_str()));
				sha1.Final();
				char hashres[256]="";
				sha1.ReportHash(hashres, RoR::CSHA1::REPORT_HEX_SHORT);
				String hashstr = String(hashres);
				if (hashstr == it->hash)
				{
					LOG("  - entry 2 removed");
					it2->deleted=true;
				} else if (hashstr == it2->hash)
				{
					LOG("  - entry 1 removed");
					it->deleted=true;
				} else
				{
					LOG("  - entry 1 and 2 removed");
					it->deleted=true;
					it2->deleted=true;
				}
			}
		}
	}
#ifdef USE_MYGUI
	LoadingWindow::getSingleton().setAutotrack(_L("loading...\n"));
#endif //USE_MYGUI

	//LOG("* incremental check (5/5): regenerating file cache ...");
	//generateFileCache(true);

	writeGeneratedCache();

#ifdef USE_MYGUI
	LoadingWindow::getSingleton().hide();
#endif //USE_MYGUI
	LOG("* incremental check done.");
	return 0;
}

CacheEntry *CacheSystem::getEntry(int modid)
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
	result += "\tnumber="+TOSTRING(counter)+"\n"; // always count linear!
	result += "\tdeleted="+TOSTRING(t.deleted)+"\n";
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

		result += "\tusagecounter="+TOSTRING(t.usagecounter)+"\n";
		result += "\taddtimestamp="+TOSTRING(t.addtimestamp)+"\n";
		result += "\tminitype="+t.minitype+"\n";
		result += "\ttype="+t.type+"\n";
		result += "\tdirname="+t.dirname+"\n";
		result += "\tfname="+t.fname+"\n";
		result += "\tfname_without_uid="+t.fname_without_uid+"\n";
		result += "\tfext="+t.fext+"\n";
		result += "\tfiletime="+TOSTRING((long)t.filetime)+"\n";
		result += "\tdname="+t.dname+"\n";
		result += "\thash="+t.hash+"\n";
		result += "\tcategoryid="+TOSTRING(t.categoryid)+"\n";
		result += "\tuniqueid="+t.uniqueid+"\n";
		result += "\tguid="+t.guid+"\n";
		result += "\tversion="+TOSTRING(t.version)+"\n";
		result += "\tfilecachename="+t.filecachename+"\n";
		//result += "\tnumauthors="+TOSTRING(t.authors.size())+"\n";

		if (t.authors.size() > 0)
		{
			for (int i=0;i<(int)t.authors.size();i++)
			{
				if (t.authors[i].type.empty())  t.authors[i].type = "unknown";
				if (t.authors[i].name.empty())  t.authors[i].name = "unknown";
				if (t.authors[i].email.empty()) t.authors[i].email = "unknown";
				result += "\tauthor=" + (t.authors[i].type) + \
					"," + TOSTRING(t.authors[i].id) + \
					"," + (t.authors[i].name) + "," + (t.authors[i].email) + "\n";
			}
		}

		// now add the truck details if existing
		if (t.description!="") result += "\tdescription="+normalizeText(t.description)+"\n";
		if (t.tags!="") result += "\ttags="+t.tags+"\n";
		if (t.fileformatversion!=0) result += "\tfileformatversion="+TOSTRING(t.fileformatversion)+"\n";
		if (t.hasSubmeshs) result += "\thasSubmeshs=1\n";
		if (t.nodecount!=0) result += "\tnodecount="+TOSTRING(t.nodecount)+"\n";
		if (t.beamcount!=0) result += "\tbeamcount="+TOSTRING(t.beamcount)+"\n";
		if (t.shockcount!=0) result += "\tshockcount="+TOSTRING(t.shockcount)+"\n";
		if (t.fixescount!=0) result += "\tfixescount="+TOSTRING(t.fixescount)+"\n";
		if (t.hydroscount!=0) result += "\thydroscount="+TOSTRING(t.hydroscount)+"\n";
		if (t.wheelcount!=0) result += "\twheelcount="+TOSTRING(t.wheelcount)+"\n";
		if (t.propwheelcount!=0) result += "\tpropwheelcount="+TOSTRING(t.propwheelcount)+"\n";
		if (t.commandscount!=0) result += "\tcommandscount="+TOSTRING(t.commandscount)+"\n";
		if (t.flarescount!=0) result += "\tflarescount="+TOSTRING(t.flarescount)+"\n";
		if (t.propscount!=0) result += "\tpropscount="+TOSTRING(t.propscount)+"\n";
		if (t.wingscount!=0) result += "\twingscount="+TOSTRING(t.wingscount)+"\n";
		if (t.turbopropscount!=0) result += "\tturbopropscount="+TOSTRING(t.turbopropscount)+"\n";
		if (t.turbojetcount!=0) result += "\tturbojetcount="+TOSTRING(t.turbojetcount)+"\n";
		if (t.rotatorscount!=0) result += "\trotatorscount="+TOSTRING(t.rotatorscount)+"\n";
		if (t.exhaustscount!=0) result += "\texhaustscount="+TOSTRING(t.exhaustscount)+"\n";
		if (t.flexbodiescount!=0) result += "\tflexbodiescount="+TOSTRING(t.flexbodiescount)+"\n";
		if (t.materialflarebindingscount!=0) result += "\tmaterialflarebindingscount="+TOSTRING(t.materialflarebindingscount)+"\n";
		if (t.soundsourcescount!=0) result += "\tsoundsourcescount="+TOSTRING(t.soundsourcescount)+"\n";
		if (t.managedmaterialscount!=0) result += "\tmanagedmaterialscount="+TOSTRING(t.managedmaterialscount)+"\n";
		if (t.truckmass>1) result += "\ttruckmass="+TOSTRING(t.truckmass)+"\n";
		if (t.loadmass>1) result += "\tloadmass="+TOSTRING(t.loadmass)+"\n";
		if (t.minrpm>1) result += "\tminrpm="+TOSTRING(t.minrpm)+"\n";
		if (t.maxrpm>1) result += "\tmaxrpm="+TOSTRING(t.maxrpm)+"\n";
		if (t.torque>1) result += "\ttorque="+TOSTRING(t.torque)+"\n";
		if (t.customtach) result += "\tcustomtach=1\n";
		if (t.custom_particles) result += "\tcustom_particles=1\n";
		if (t.forwardcommands) result += "\tforwardcommands=1\n";
		if (t.importcommands) result += "\timportcommands=1\n";
		if (t.rollon) result += "\trollon=1\n";
		if (t.rescuer) result += "\trescuer=1\n";
		if (t.driveable!=0) result += "\tdriveable="+TOSTRING(t.driveable)+"\n";
		if (t.numgears!=0) result += "\tnumgears="+TOSTRING(t.numgears)+"\n";
		if (t.enginetype!=0) result += "\tenginetype="+TOSTRING(t.enginetype)+"\n";
		if (t.materials.size())
		{
			String matStr = "";
			for (std::set < Ogre::String >::iterator it = t.materials.begin(); it != t.materials.end(); it++)
			{
				matStr += *it + " ";
			}
			result += "\tmaterials=" + matStr + "\n";
		}

		if (t.sectionconfigs.size() > 0)
		{
			for (int i=0;i<(int)t.sectionconfigs.size();i++)
				result += "\tsectionconfig="+t.sectionconfigs[i]+"\n";
		}
	}

	return result;
}

Ogre::String CacheSystem::normalizeText(Ogre::String text)
{
	String result = "";
	Ogre::StringVector str = Ogre::StringUtil::split(text, "\n");
	for (Ogre::StringVector::iterator it = str.begin(); it!=str.end(); it++)
		result += *it + "$";
	return result;
}

Ogre::String CacheSystem::deNormalizeText(Ogre::String text)
{
	String result = "";
	Ogre::StringVector str = Ogre::StringUtil::split(text, "$");
	for (Ogre::StringVector::iterator it = str.begin(); it!=str.end(); it++)
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

	FILE *f = fopen(path.c_str(), "w");
	if (!f)
	{
		showError(_L("Fatal Error: Unable to write cache to disk"), _L("Unable to write file.\nPlease ensure the parent directories exists and that you have write access to this location:\n") + path);
		exit(1337);
	}
	fprintf(f, "shaone=%s\n", const_cast<char*>(currentSHA1.c_str()));
	fprintf(f, "modcount=%d\n", (int)entries.size());
	fprintf(f, "cacheformat=%s\n", CACHE_FILE_FORMAT);

	// mods
	std::vector<CacheEntry>::iterator it;
	int counter=0;
	for (it = entries.begin(); it != entries.end(); it++)
	{
		if (it->deleted) continue;
		fprintf(f, "%s", formatEntry(counter, *it).c_str());
		counter++;
	}

	// close
	fclose(f);
	LOG("...done!");
}

void CacheSystem::writeStreamCache()
{
#if 0
	String dirsep="/";
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
	dirsep="\\";
#endif
	ResourceGroupManager& rgm = ResourceGroupManager::getSingleton();

	FileInfoListPtr dirs = rgm.findResourceFileInfo("Streams", "*", true);
	for (FileInfoList::iterator itDir = dirs->begin(); itDir!= dirs->end(); ++itDir)
	{
		if (itDir->filename == String(".svn")) continue;
		String dirName = SSETTING("Streams Path", "") + (*itDir).filename;
		String cacheFilename = dirName + dirsep + "stream.cache";
		FILE *f = fopen(cacheFilename.c_str(), "w");

		// iterate through mods
		std::vector<CacheEntry>::iterator it;
		int counter=0;
		for (it = entries.begin(); it != entries.end(); it++)
		{
			if (it->deleted) continue;
			if (it->dirname.substr(0, dirName.size()) == dirName)
			{
				if (f) fprintf(f, "%s", formatEntry(counter, *it).c_str());

				ResourceGroupManager::getSingleton().addResourceLocation(it->dirname, "Zip");
				generateFileCache(*it, dirName + dirsep);
				ResourceGroupManager::getSingleton().removeResourceLocation(it->dirname);
				counter++;
			}
		}
		if (f) fclose(f);
	}
#endif
}

void CacheSystem::updateSingleTruckEntryCache(int number, CacheEntry t)
{
	// todo: to be implemented
}

char *CacheSystem::replacesSpaces(char *str)
{
	char *ptr = str;
	while (*ptr!=0) {if (*ptr==' ') *ptr='_';ptr++;};
	return str;
}

char *CacheSystem::restoreSpaces(char *str)
{
	char *ptr = str;
	while (*ptr!=0) {if (*ptr=='_') *ptr=' ';ptr++;};
	return str;
}

bool CacheSystem::stringHasUID(Ogre::String uidstr)
{
	size_t pos = uidstr.find("-");
	if (pos != String::npos && pos >= 3 && uidstr.substr(pos-3, 3) == "UID")
		return true;
	return false;
}

Ogre::String CacheSystem::stripUIDfromString(Ogre::String uidstr)
{
	size_t pos = uidstr.find("-");
	if (pos != String::npos && pos >= 3 && uidstr.substr(pos-3, 3) == "UID")
		return uidstr.substr(pos+1, uidstr.length()-pos);
	return uidstr;
}

Ogre::String CacheSystem::getUIDfromString(Ogre::String uidstr)
{
	size_t pos = uidstr.find("-");
	if (pos != String::npos && pos >= 3 && uidstr.substr(pos-3, 3) == "UID")
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
			} else
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
			entry.usagecounter=0;
			entry.deleted = false;
			String basen;
			String fnextension;
			StringUtil::splitBaseFilename(entry.fname, basen, fnextension);
			entry.minitype = detectFilesMiniType(basen+"-mini");
			entry.hash="none";
			entry.changedornew=true;
			generateFileCache(entry);
			if (archiveType == "Zip")
				entry.hash = zipHashes[getVirtualPath(archiveDirectory)];
			if (entry.hash == "")
				// fallback if no hash was found
				entry.hash="none";
			// read in author and category
			entries.push_back(entry);
		} catch(Ogre::Exception& e)
		{
			if (e.getNumber() == Ogre::Exception::ERR_DUPLICATE_ITEM)
			{
				LOG(" *** error opening archive '"+filename+"': some files are duplicates of existing files. The file will be ignored.");
				LOG("error while opening resource: " + e.getFullDescription());
			}else
			{
				LOG("error while opening resource: " + e.getFullDescription());
				LOG("error opening archive '"+String(filename)+"'. Is it corrupt?");
				LOG("trying to continue ...");
			}
		}
	}
}

void CacheSystem::fillTruckDetailInfo(CacheEntry &entry, Ogre::DataStreamPtr ds, Ogre::String fname)
{
	SerializedRig* r = new SerializedRig();

	r->loadTruckVirtual(fname, true);

	// copy over some values

	for (unsigned int i=0; i<r->description.size(); i++)    entry.description += r->description[i] + "\n";
	for (unsigned int i=0; i<r->authors.size(); i++)        entry.authors.push_back(r->authors[i]);
	for (unsigned int i=0; i<r->sectionconfigs.size(); i++) entry.sectionconfigs.push_back(r->sectionconfigs[i]);

	if (r->engine)
	{
		entry.numgears   = r->engine->getNumGears();
		entry.minrpm     = r->engine->getMinRPM();
		entry.maxrpm     = r->engine->getMaxRPM();
		entry.torque     = r->engine->getEngineTorque();
		entry.enginetype = r->engine->getType();
	}
	entry.uniqueid   = r->uniquetruckid;
	entry.categoryid = r->categoryid;
	entry.version    = r->truckversion;
	entry.forwardcommands = r->forwardcommands;
	entry.importcommands  = r->importcommands;
	entry.rollon = r->wheel_contact_requested;
	entry.rescuer = r->rescuer;
	entry.guid = String(r->guid);
	entry.fileformatversion = r->fileformatversion;
	entry.hasSubmeshs = (r->free_sub > 0);
	entry.nodecount   = r->free_node;
	entry.beamcount   = r->free_beam;
	entry.shockcount  = r->free_shock;
	entry.fixescount  = r->free_fixes;
	entry.hydroscount = r->free_hydro;
	entry.wheelcount  = r->free_wheel;
	entry.propwheelcount = r->propwheelcount;
	entry.driveable   = r->driveable;
	entry.commandscount = r->free_commands;
	entry.flarescount = r->free_flare;
	entry.propscount = r->free_prop;
	entry.wingscount = r->free_wing;
	entry.turbopropscount = r->free_aeroengine;
	entry.rotatorscount = r->free_rotator;
	entry.exhaustscount = (int)r->exhausts.size();
	entry.custom_particles = (r->free_cparticle > 0);
	entry.turbojetcount = r->free_aeroengine;
	entry.flexbodiescount = r->free_flexbody;
	entry.soundsourcescount = r->free_soundsource;
	//entry.managedmaterialscount++;
	//entry.customtach=true;
	//entry.materialflarebindingscount++;
	delete r;
}

int CacheSystem::addUniqueString(std::set<Ogre::String> &list, Ogre::String str)
{
	// ignore some render texture targets
	if (str == "mirror") return 0;
	if (str == "renderdash") return 0;

	str = stripUIDfromString(str);

	if (list.find(str) == list.end())
	{
		list.insert(str);
		return 1;
	}
	return 0;
}

Ogre::String CacheSystem::addMeshMaterials(CacheEntry &entry, Ogre::Entity *e)
{
	String materials = "";
	MeshPtr m = e->getMesh();
	if (!m.isNull())
	{
		for (int n=0; n<(int)m->getNumSubMeshes();n++)
		{
			SubMesh *sm = m->getSubMesh(n);
			addUniqueString(entry.materials, sm->getMaterialName());
		}
	}

	for (int n=0; n<(int)e->getNumSubEntities();n++)
	{
		SubEntity *subent = e->getSubEntity(n);
		addUniqueString(entry.materials, subent->getMaterialName());
	}
	return materials;
}

int CacheSystem::getTimeStamp()
{
	return (int)time(NULL); //this will overflow in 2038
}

void CacheSystem::deleteFileCache(char *filename)
{
	if (remove(filename))
	{
		LOG("error deleting file '"+String(filename)+"'");
	}
}

Ogre::String CacheSystem::detectFilesMiniType(String filename)
{
	if (resourceExistsInAllGroups(filename+".dds"))
		return "dds";

	if (resourceExistsInAllGroups(filename+".png"))
		return "png";

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

void CacheSystem::generateFileCache(CacheEntry &entry, Ogre::String directory)
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
			} else
			{
				group = ResourceGroupManager::getSingleton().findGroupContainingResource(minifn);
			}
		} else
		{
			group = ResourceGroupManager::getSingleton().findGroupContainingResource(minifn);
		}

		FileInfoListPtr files = ResourceGroupManager::getSingleton().findResourceFileInfo(group, minifn);
		if (files->empty())
		{
			deleteFileCache(const_cast<char*>(dst.c_str()));
			return;
		}

		size_t fsize = 0;
		char *buffer = 0;
		{
			DataStreamPtr ds=ResourceGroupManager::getSingleton().openResource(minifn, group);
			fsize = ds->size();
			buffer = (char*)malloc(fsize);
			memset(buffer, 0, fsize);
			size_t read = ds->read(buffer, fsize);
			if (read != fsize)
				return;
		}

		bool written=false;
		if (buffer)
		{
			FILE *f = fopen(dst.c_str(),"wb");
			if (f)
			{
				fwrite(buffer, 1, fsize,  f);
				fclose(f);
				written=true;
			}
			free(buffer);
		}
		if (!written)
		{
			deleteFileCache(const_cast<char*>(dst.c_str()));
		}
	} catch(Ogre::Exception& e)
	{
		LOG("error while generating File cache: " + e.getFullDescription());
		LOG("trying to continue ...");
	}
	LOG("done generating file cache!");
}

void CacheSystem::parseKnownFilesAllRG()
{
	for (std::vector<Ogre::String>::iterator sit=known_extensions.begin();sit!=known_extensions.end();sit++)
		parseFilesAllRG(*sit);
}

void CacheSystem::parseFilesAllRG(Ogre::String ext)
{
	StringVector sv = ResourceGroupManager::getSingleton().getResourceGroups();
	StringVector::iterator it;
	for (it = sv.begin(); it!=sv.end(); it++)
		parseFilesOneRG(ext, *it);

	LOG("* parsing files of all Resource Groups (" + ext + ") finished!");
}

void CacheSystem::parseKnownFilesOneRG(Ogre::String rg)
{
	for (std::vector<Ogre::String>::iterator sit=known_extensions.begin();sit!=known_extensions.end();sit++)
		parseFilesOneRG(*sit, rg);
}

void CacheSystem::parseKnownFilesOneRGDirectory(Ogre::String rg, Ogre::String dir)
{
	String dirb = getVirtualPath(dir);
	
	for (std::vector<Ogre::String>::iterator it=known_extensions.begin(); it != known_extensions.end(); ++it)
	{
		FileInfoListPtr files = ResourceGroupManager::getSingleton().findResourceFileInfo(rg, "*." + *it);
		for (FileInfoList::iterator iterFiles = files->begin(); iterFiles!= files->end(); ++iterFiles)
		{
			if (!iterFiles->archive) continue;

			String dira = getVirtualPath(iterFiles->archive->getName());

			if (dira == dirb)
				addFile(*iterFiles, *it);
		}
	}
}

void CacheSystem::parseFilesOneRG(Ogre::String ext, Ogre::String rg)
{
	FileInfoListPtr files = ResourceGroupManager::getSingleton().findResourceFileInfo(rg, "*." + ext);
	for (FileInfoList::iterator iterFiles = files->begin(); iterFiles!= files->end(); ++iterFiles)
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

	for (std::vector<CacheEntry>::iterator it = entries.begin(); it!=entries.end(); it++)
	{
		if (it->type != "FileSystem") continue;
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
	for (std::vector<Ogre::String>::iterator it=known_extensions.begin(); it != known_extensions.end(); ++it)
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
	for (int i=0; i<3; i++)
	{
		for (int b=0 ;b<2; b++)
		{
			FileInfoListPtr list = ResourceGroupManager::getSingleton().listResourceFileInfo(restype[i], (b==1));
			for (FileInfoList::iterator iterFiles = list->begin(); iterFiles!=list->end(); iterFiles++)
			{
				String name = restype[i] + "/";
				if (iterFiles->archive) name += iterFiles->archive->getName() + "/";

				if (b==0)
				{
					// special file handling, only add important files!
					bool vipfile = false;
					for (std::vector<Ogre::String>::iterator sit=known_extensions.begin();sit!=known_extensions.end();sit++)
					{
						if ((iterFiles->filename.find("."+*sit) != String::npos && iterFiles->filename.find(".dds") == String::npos && iterFiles->filename.find(".png") == String::npos)
						 || (iterFiles->filename.find(".zip") != String::npos))
						{
							vipfile = true;
							break;
						}
					}
					if (!vipfile) continue;
				}
				name += iterFiles->filename;
				filenames += name + "\n";
			}
		}
	}

	char result[256] = {};

	RoR::CSHA1 sha1;
	char *data = const_cast<char*>(filenames.c_str());
	sha1.UpdateHash((uint8_t *)data, (uint32_t)strlen(data));
	sha1.Final();
	sha1.ReportHash(result, RoR::CSHA1::REPORT_HEX_SHORT);
	return result;
}

void CacheSystem::fillTerrainDetailInfo(CacheEntry &entry, Ogre::DataStreamPtr ds, Ogre::String fname)
{
	TerrainManager tm;
	tm.loadTerrainConfigBasics(ds);

	//parsing the current file
	entry.authors	 = tm.getAuthors();
	entry.dname      = tm.getTerrainName();
	entry.categoryid = tm.getCategoryID();
	entry.uniqueid   = tm.getGUID();
	entry.version    = tm.getVersion();
}

int CacheSystem::getCategoryUsage(int category)
{
	return category_usage[category];
}

void CacheSystem::readCategoryTitles()
{
	LOG("Loading category titles from "+configlocation+"categories.cfg");
	String filename = configlocation + String("categories.cfg");
	FILE *fd = fopen(filename.c_str(), "r");
	if (!fd)
	{
		LOG("error opening file: "+configlocation+"categories.cfg");
		return;
	}
	char line[1024] = {};
	while (!feof(fd))
	{
		int res = fscanf(fd, " %[^\n\r]", line);
		if (line[0] == ';')	continue;
		char title[256] = {};
		const char delimiters[] = ",";
		char str_work[1024] = {};

		strncpy(str_work, line, 1023);
		str_work[1023] = '\0';
		char *token = strtok(str_work, delimiters);
		if (token == NULL) continue;
		int number = atoi(token);
		token = strtok(NULL, delimiters);
		if (token == NULL) continue;
		//strip spaces at the beginning
		while(*token == ' ') token++;
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

bool CacheSystem::checkResourceLoaded(Ogre::String &filename)
{
	Ogre::String group = "";
	return checkResourceLoaded(filename, group);
}

CacheEntry CacheSystem::getResourceInfo(Ogre::String &filename)
{
	CacheEntry def;
	std::vector<CacheEntry>::iterator it;
	for (it = entries.begin(); it != entries.end(); it++)
		if (it->fname == filename || it->fname_without_uid == filename)
			return *it;
	return def;
}

bool CacheSystem::checkResourceLoaded(Ogre::String &filename, Ogre::String &group)
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
			String name = "General-Reloaded-"+TOSTRING(rgcountera);
			ResourceGroupManager::getSingleton().addResourceLocation(t.dirname, t.type, name);
			loaded[t.dirname]=true;
			ResourceGroupManager::getSingleton().initialiseResourceGroup(name);
			return true;
		} catch(Ogre::Exception& e)
		{
			if (e.getNumber() == Ogre::Exception::ERR_DUPLICATE_ITEM)
			{
				LOG(" *** error opening '"+t.dirname+"': some files are duplicates of existing files. The archive/directory will be ignored.");
				LOG("error while opening resource: " + e.getFullDescription());
			} else
			{
				LOG("error opening '"+t.dirname+"'.");
				if (t.type == "Zip")
					LOG("Is the zip archive corrupt? Error: " + e.getFullDescription());
				LOG("Error description : " + e.getFullDescription());
				LOG("trying to continue ...");
			}
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
	String rgname = "General-"+TOSTRING(rgcounter);

	try
	{
		if (alreadyLoaded)
		{
			parseKnownFilesOneRGDirectory(group, dirname);
		} else
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

	} catch(Ogre::Exception& e)
	{
		if (e.getNumber() == Ogre::Exception::ERR_DUPLICATE_ITEM)
		{
			LOG(" *** error opening directory '"+dirname+"': some files are duplicates of existing files. The directory will be ignored.");
			LOG("error while opening resource: " + e.getFullDescription());

		} else
		{
			LOG("error while loading directory: " + e.getFullDescription());
			LOG("error opening directory '"+dirname+"'");
			LOG("trying to continue ...");
		}
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
	String rgname = "General-"+TOSTRING(rgcounter);

	// use general group?
	if (!ownGroup)
	{
		rgname = ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME;
	}

	try
	{
		ResourceGroupManager &rgm = ResourceGroupManager::getSingleton();

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
	} catch(Ogre::Exception& e)
	{
		if (e.getNumber() == Ogre::Exception::ERR_DUPLICATE_ITEM)
		{
			LOG(" *** error opening archive '"+realzipPath+"': some files are duplicates of existing files. The archive will be ignored.");
			LOG("error while opening resource: " + e.getFullDescription());

		} else
		{
			LOG("error while loading single Zip: " + e.getFullDescription());
			LOG("error opening archive '"+realzipPath+"'. Is it corrupt? Ignoring that archive ...");
			LOG("trying to continue ...");
		}
	}
}

void CacheSystem::loadAllZipsInResourceGroup(String group)
{
	std::map<String, bool> loadedZips;
	ResourceGroupManager& rgm = ResourceGroupManager::getSingleton();
	FileInfoListPtr files = rgm.findResourceFileInfo(group, "*.zip");
	FileInfoList::iterator iterFiles = files->begin();
	size_t i=0, filecount=files->size();
	for (; iterFiles!= files->end(); ++iterFiles, i++)
	{
		if (loadedZips[iterFiles->filename])
		{
			LOG(" zip already loaded: " + iterFiles->filename);
			// already loaded for some strange reason
			continue;
		}
		// update loader
		int progress = ((float)i/(float)filecount)*100;
#ifdef USE_MYGUI
		UTFString tmp = _L("Loading zips in group ") + ANSI_TO_UTF(group) + L"\n" + ANSI_TO_UTF(iterFiles->filename) + L"\n" + ANSI_TO_UTF(TOSTRING(i)) + L"/" + ANSI_TO_UTF(TOSTRING(filecount));
		LoadingWindow::getSingleton().setProgress(progress, tmp);
#endif //USE_MYGUI

		loadSingleZip((Ogre::FileInfo)*iterFiles);
		loadedZips[iterFiles->filename] = true;
	}
	// hide loader again
#ifdef USE_MYGUI
	LoadingWindow::getSingleton().hide();
#endif //USE_MYGUI
}

void CacheSystem::loadAllDirectoriesInResourceGroup(String group)
{
	FileInfoListPtr list = ResourceGroupManager::getSingleton().listResourceFileInfo(group, true);
	size_t i=0, filecount=list->size();
	for (FileInfoList::iterator listitem = list->begin(); listitem!= list->end(); ++listitem,i++)
	{
		if (!listitem->archive) continue;
		String dirname = listitem->archive->getName() + SSETTING("dirsep", "\\") + listitem->filename;
		// update loader
		int progress = ((float)i/(float)filecount)*100;
#ifdef USE_MYGUI
		LoadingWindow::getSingleton().setProgress(progress, _L("Loading directory\n") + listitem->filename);
#endif //USE_MYGUI
		loadSingleDirectory(dirname, group, true);
	}
	// hide loader again
#ifdef USE_MYGUI
	LoadingWindow::getSingleton().hide();
#endif //USE_MYGUI
}

void CacheSystem::loadAllZips()
{
	static bool lodedalready=false;
	if (lodedalready)
		return;
	lodedalready=true;
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
	size_t i=0, filecount=files->size();
	for (; iterFiles!= files->end(); ++iterFiles, i++)
	{
		String zippath = iterFiles->archive->getName() + "\\" + iterFiles->filename;
		String zippath2="";
		if (iterFiles->archive->getName()[iterFiles->archive->getName().size()-1] != '/')
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
		int progress = ((float)i/(float)filecount)*100;
#ifdef USE_MYGUI
		LoadingWindow::getSingleton().setProgress(progress, _L("checking for new zips in ") + group + "\n" + iterFiles->filename + "\n" + TOSTRING(i) + "/" + TOSTRING(filecount));
#endif //USE_MYGUI
		if (!isZipUsedInEntries(zippath2))
		{
#ifdef USE_MYGUI
			LoadingWindow::getSingleton().setProgress(progress, _L("checking for new zips in ") + group + "\n" + _L("loading new zip: ") + iterFiles->filename + "\n" + TOSTRING(i) + "/" + TOSTRING(filecount));
#endif //USE_MYGUI
			LOG("- "+zippath+" is new");
			newFiles++;
			loadSingleZip((Ogre::FileInfo)*iterFiles);
		}
	}
#ifdef USE_MYGUI
	LoadingWindow::getSingleton().hide();
#endif //USE_MYGUI
}

void CacheSystem::checkForNewDirectoriesInResourceGroup(String group)
{
	FileInfoListPtr list = ResourceGroupManager::getSingleton().listResourceFileInfo(group, true);
	size_t i=0, filecount=list->size();
	for (FileInfoList::iterator listitem = list->begin(); listitem!= list->end(); ++listitem, i++)
	{
		if (!listitem->archive) continue;
		String dirname = listitem->archive->getName() + SSETTING("dirsep", "\\") + listitem->filename;
		int progress = ((float)i/(float)filecount)*100;
#ifdef USE_MYGUI
		LoadingWindow::getSingleton().setProgress(progress, _L("checking for new directories in ") + group + "\n" + listitem->filename + "\n" + TOSTRING(i) + "/" + TOSTRING(filecount));
#endif //USE_MYGUI
		if (!isDirectoryUsedInEntries(dirname))
		{
#ifdef USE_MYGUI
			LoadingWindow::getSingleton().setProgress(progress, _L("checking for new directories in ") + group + "\n" + _L("loading new directory: ") + listitem->filename + "\n" + TOSTRING(i) + "/" + TOSTRING(filecount));
#endif //USE_MYGUI
			LOG("- "+dirname+" is new");
			loadSingleDirectory(dirname, group, true);
		}
	}
#ifdef USE_MYGUI
	LoadingWindow::getSingleton().hide();
#endif //USE_MYGUI
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
	Archive *fsa = FSAF.createInstance(filename);

	std::time_t ft = fsa->getModifiedTime(filename);

	FSAF.destroyInstance(fsa);

	return ft;
}
