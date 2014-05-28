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

#include "ContentManager.h"

#include "Application.h"
#include "Settings.h"
#include "ColoredTextAreaOverlayElementFactory.h"
#include "SoundScriptManager.h"
#include "SkinManager.h"
#include "Language.h"
#include "PlatformUtils.h"

#include "CacheSystem.h"

#include "OgreShaderParticleRenderer.h"
#include "OgreBoxEmitterFactory.h"

#ifdef USE_ANGELSCRIPT
#include "FireExtinguisherAffectorFactory.h"
#include "ExtinguishableFireAffectorFactory.h"
#endif // USE_ANGELSCRIPT

#include "Utils.h"

using namespace Ogre;
using namespace std;
using namespace RoR;

// ================================================================================
// Static variables
// ================================================================================

#define DECLARE_RESOURCE_PACK(_NUMBER_, _FIELD_, _NAME_, _RESOURCE_GROUP_) \
	const ContentManager::ResourcePack ContentManager::ResourcePack::_FIELD_(BITMASK_64(_NUMBER_), _NAME_, _RESOURCE_GROUP_);

DECLARE_RESOURCE_PACK(  1, OGRE_CORE,             "OgreCore",             "Bootstrap");
DECLARE_RESOURCE_PACK(  2, GUI_MENU_WALLPAPERS,   "gui_menu_wallpapers",  "Wallpapers");
DECLARE_RESOURCE_PACK(  3, GUI_STARTUP_SCREEN,    "gui_startup_screen",   "Bootstrap");
DECLARE_RESOURCE_PACK(  4, AIRFOILS,              "airfoils",             "General");
DECLARE_RESOURCE_PACK(  5, BEAM_OBJECTS,          "beamobjects",          "General");
DECLARE_RESOURCE_PACK(  6, BLUR,                  "blur",                 "General");
DECLARE_RESOURCE_PACK(  7, CAELUM,                "caelum",               "General");
DECLARE_RESOURCE_PACK(  8, CUBEMAPS,              "cubemaps",             "General");
DECLARE_RESOURCE_PACK(  9, DASHBOARDS,            "dashboards",           "General");
DECLARE_RESOURCE_PACK( 10, DEPTH_OF_FIELD,        "dof",                  "General");
DECLARE_RESOURCE_PACK( 11, FAMICONS,              "famicons",             "General");
DECLARE_RESOURCE_PACK( 12, FLAGS,                 "flags",                "General");
DECLARE_RESOURCE_PACK( 13, GLOW,                  "glow",                 "General");
DECLARE_RESOURCE_PACK( 14, HDR,                   "hdr",                  "General");
DECLARE_RESOURCE_PACK( 15, HEATHAZE,              "heathaze",             "General");
DECLARE_RESOURCE_PACK( 16, HYDRAX,                "hydrax",               "General");
DECLARE_RESOURCE_PACK( 17, ICONS,                 "icons",                "General");
DECLARE_RESOURCE_PACK( 18, MATERIALS,             "materials",            "General");
DECLARE_RESOURCE_PACK( 19, MESHES,                "meshes",               "General");
DECLARE_RESOURCE_PACK( 20, MYGUI,                 "mygui",                "General");
DECLARE_RESOURCE_PACK( 21, OVERLAYS,              "overlays",             "General");
DECLARE_RESOURCE_PACK( 22, PAGED,                 "paged",                "General");
DECLARE_RESOURCE_PACK( 23, PARTICLES,             "particles",            "General");
DECLARE_RESOURCE_PACK( 24, PSSM,                  "pssm",                 "General");
DECLARE_RESOURCE_PACK( 25, RTSHADER,              "rtshader",             "General");
DECLARE_RESOURCE_PACK( 26, SCRIPTS,               "scripts",              "General");
DECLARE_RESOURCE_PACK( 27, SOUNDS,                "sounds",               "General");
DECLARE_RESOURCE_PACK( 28, SUNBURN,               "sunburn",              "General");
DECLARE_RESOURCE_PACK( 29, TEXTURES,              "textures",             "General");

// ================================================================================
// Functions
// ================================================================================

ContentManager::ContentManager():
	m_loaded_resource_packs(0)
{
}

ContentManager::~ContentManager()
{
}

void ContentManager::AddResourcePack(ResourcePack const & resource_pack)
{
	std::stringstream log_msg;
	log_msg << "[RoR|ContentManager] Loading resource pack '" << resource_pack.name << "' from group '" << resource_pack.resource_group_name << "'";
	Ogre::String resources_dir = SSETTING("Resources Path", "resources" + PlatformUtils::DIRECTORY_SEPARATOR);
	Ogre::String zip_path = resources_dir + resource_pack.name + Ogre::String(".zip");
	if (PlatformUtils::FileExists(zip_path))
	{
		log_msg << " (ZIP archive)";
		LOG(log_msg.str());
		ResourceGroupManager::getSingleton().addResourceLocation(zip_path, "Zip", resource_pack.resource_group_name);
		BITMASK_64_SET_1(m_loaded_resource_packs, resource_pack.mask);
	}
	else
	{
		Ogre::String dir_path = resources_dir + resource_pack.name;
		if (PlatformUtils::FolderExists(dir_path))
		{
			log_msg << " (directory)";
			LOG(log_msg.str());
			ResourceGroupManager::getSingleton().addResourceLocation(dir_path, "FileSystem", resource_pack.resource_group_name);
			BITMASK_64_SET_1(m_loaded_resource_packs, resource_pack.mask);
		}
		else
		{
			log_msg << " failed, data not found.";
			throw std::runtime_error(log_msg.str());
		}
	}
}

bool ContentManager::init(void)
{
    // set listener if none has already been set
    if (!Ogre::ResourceGroupManager::getSingleton().getLoadingListener())
        Ogre::ResourceGroupManager::getSingleton().setLoadingListener(this);

	// try to get correct paths
	// note: we don't have LogManager available yet!
	// FIRST: Get the "program path" and the user space path

	// note: this is now done in the settings class, so set it up
	// note: you need to set the build mode correctly before you build the paths!

	// by default, display everything in the depth map
	Ogre::MovableObject::setDefaultVisibilityFlags(DEPTHMAP_ENABLED);

	ColoredTextAreaOverlayElementFactory *cef = new ColoredTextAreaOverlayElementFactory();
	OverlayManager::getSingleton().addOverlayElementFactory(cef);



	// load bootstrap and main resources
	String dirsep="/";
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
	dirsep="\\";
#endif
	// bootstrap
	// we load the bootstrap before already

	// main game resources
	LOG("Loading main resources");
	AddResourcePack(ResourcePack::AIRFOILS);
	AddResourcePack(ResourcePack::BEAM_OBJECTS);
	AddResourcePack(ResourcePack::MATERIALS);
	AddResourcePack(ResourcePack::MESHES);
	AddResourcePack(ResourcePack::OVERLAYS);
	AddResourcePack(ResourcePack::PARTICLES);
#ifdef USE_MYGUI
	AddResourcePack(ResourcePack::MYGUI);
	AddResourcePack(ResourcePack::DASHBOARDS);
#endif // USE_MYGUI
	AddResourcePack(ResourcePack::SCRIPTS);
	AddResourcePack(ResourcePack::TEXTURES);
	AddResourcePack(ResourcePack::FLAGS);
	AddResourcePack(ResourcePack::ICONS);
	AddResourcePack(ResourcePack::FAMICONS);


#ifdef WIN32
	// TODO: FIX UNDER LINUX!
	// register particle classes
	LOG("Registering Particle Box Emitter");
	ParticleSystemRendererFactory *mParticleSystemRendererFact = OGRE_NEW ShaderParticleRendererFactory();
	ParticleEmitterFactory *mParticleEmitterFact = OGRE_NEW BoxEmitterFactory();
	ParticleSystemManager::getSingleton().addRendererFactory(mParticleSystemRendererFact);
	ParticleSystemManager::getSingleton().addEmitterFactory(mParticleEmitterFact);
#endif // WIN32

#ifdef USE_ANGELSCRIPT
	// FireExtinguisherAffector
	ParticleAffectorFactory* pAffFact = OGRE_NEW FireExtinguisherAffectorFactory();
	ParticleSystemManager::getSingleton().addAffectorFactory(pAffFact);

	// ExtinguishableFireAffector
	pAffFact = OGRE_NEW ExtinguishableFireAffectorFactory();
	ParticleSystemManager::getSingleton().addAffectorFactory(pAffFact);
#endif // USE_ANGELSCRIPT

	// optional ones

	// sound is a bit special as we mark the base sounds so we don't clear them accidentally later on
#ifdef USE_OPENAL
	LOG("Creating Sound Manager");
	SoundScriptManager::getSingleton().setLoadingBaseSounds(true);
#endif // USE_OPENAL

	if (SSETTING("AudioDevice", "") != "No Output")
		AddResourcePack(ResourcePack::SOUNDS);

	if (SSETTING("Sky effects", "Caelum (best looking, slower)") == "Caelum (best looking, slower)")
		AddResourcePack(ResourcePack::CAELUM);

	AddResourcePack(ResourcePack::HYDRAX);

	if (SSETTING("Vegetation", "None (fastest)") != "None (fastest)")
		AddResourcePack(ResourcePack::PAGED);

	if (BSETTING("HDR", false))
		AddResourcePack(ResourcePack::HDR);

	if (BSETTING("DOF", false))
		AddResourcePack(ResourcePack::DEPTH_OF_FIELD);

	if (BSETTING("Glow", false))
		AddResourcePack(ResourcePack::GLOW);

	if (BSETTING("Motion blur", false))
		AddResourcePack(ResourcePack::BLUR);

	if (BSETTING("HeatHaze", false))
		AddResourcePack(ResourcePack::HEATHAZE);

	if (BSETTING("Sunburn", false))
		AddResourcePack(ResourcePack::SUNBURN);

	if (SSETTING("Shadow technique", "") == "Parallel-split Shadow Maps")
		AddResourcePack(ResourcePack::PSSM);

	// streams path, to be processed later by the cache system
	LOG("Loading filesystems");

	ResourceGroupManager::getSingleton().addResourceLocation(SSETTING("User Path", "")+"cache", "FileSystem", "cache");
	// config, flat
	ResourceGroupManager::getSingleton().addResourceLocation(SSETTING("User Path", "")+"config", "FileSystem", ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
	ResourceGroupManager::getSingleton().addResourceLocation(SSETTING("User Path", "")+"alwaysload", "FileSystem", ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
	// packs, to be processed later by the cache system

	// add scripts folder
	ResourceGroupManager::getSingleton().addResourceLocation(SSETTING("User Path", "")+"scripts", "FileSystem", "Scripts");

	// init skin manager, important to happen before trucks resource loading!
	LOG("registering Skin Manager");
	new SkinManager();

	LOG("registering colored text overlay factory");
	ColoredTextAreaOverlayElementFactory *pCT = new ColoredTextAreaOverlayElementFactory();
	OverlayManager::getSingleton().addOverlayElementFactory(pCT);

	// set default mipmap level (NB some APIs ignore this)
	if (TextureManager::getSingletonPtr())
		TextureManager::getSingleton().setDefaultNumMipmaps(5);
	String tft=SSETTING("Texture Filtering", "Trilinear");
	TextureFilterOptions tfo=TFO_NONE;
	if (tft=="Bilinear") tfo=TFO_BILINEAR;
	if (tft=="Trilinear") tfo=TFO_TRILINEAR;
	if (tft=="Anisotropic (best looking)") tfo=TFO_ANISOTROPIC;
	MaterialManager::getSingleton().setDefaultAnisotropy(8);
	MaterialManager::getSingleton().setDefaultTextureFiltering(tfo);

	// load all resources now, so the zip files are also initiated
	LOG("initialiseAllResourceGroups()");
	try
	{
		if (BSETTING("Background Loading", false))
			ResourceBackgroundQueue::getSingleton().initialiseAllResourceGroups();
		else
			ResourceGroupManager::getSingleton().initialiseAllResourceGroups();
	} catch(Ogre::Exception& e)
	{
		LOG("catched error while initializing Resource groups: " + e.getFullDescription());
	}
#ifdef USE_OPENAL
	SoundScriptManager::getSingleton().setLoadingBaseSounds(false);
#endif // USE_OPENAL

	// and the content
	ResourceGroupManager::getSingleton().addResourceLocation(SSETTING("Program Path", "")+"packs", "FileSystem", "Packs", true);
	ResourceGroupManager::getSingleton().addResourceLocation(SSETTING("User Path", "")+"packs", "FileSystem", "Packs", true);
	ResourceGroupManager::getSingleton().addResourceLocation(SSETTING("User Path", "")+"mods",  "FileSystem", "Packs", true);

	ResourceGroupManager::getSingleton().addResourceLocation(SSETTING("User Path", "")+"vehicles", "FileSystem", "VehicleFolders");
	ResourceGroupManager::getSingleton().addResourceLocation(SSETTING("User Path", "")+"terrains", "FileSystem", "TerrainFolders");

	exploreFolders("VehicleFolders");
	exploreFolders("TerrainFolders");
	exploreZipFolders("Packs"); // this is required for skins to work

	LOG("initialiseAllResourceGroups() - Content");
	try
	{
		if (BSETTING("Background Loading", false))
			ResourceBackgroundQueue::getSingleton().initialiseResourceGroup("Packs");
		else
			ResourceGroupManager::getSingleton().initialiseResourceGroup("Packs");
	} catch(Ogre::Exception& e)
	{
		LOG("catched error while initializing Content Resource groups: " + e.getFullDescription());
	}

	LanguageEngine::getSingleton().postSetup();

	return true;
}

Ogre::DataStreamPtr ContentManager::resourceLoading(const Ogre::String &name, const Ogre::String &group, Ogre::Resource *resource)
{
	return Ogre::DataStreamPtr();
}

void ContentManager::resourceStreamOpened(const Ogre::String &name, const Ogre::String &group, Ogre::Resource *resource, Ogre::DataStreamPtr& dataStream)
{
}

bool ContentManager::resourceCollision(Ogre::Resource *resource, Ogre::ResourceManager *resourceManager)
{
	/*
	// TODO: do something useful here
	if (resourceManager->getResourceType() == "Material")
	{
		if (instanceCountMap.find(resource->getName()) == instanceCountMap.end())
		{
			instanceCountMap[resource->getName()] = 1;
		}
		int count = instanceCountMap[resource->getName()]++;
		MaterialPtr mat = (MaterialPtr)resourceManager->create(resource->getName() + TOSTRING(count), resource->getGroup());
		resource = (Ogre::Resource *)mat.getPointer();
		return true;
	}
	*/
	return false;
}

	

void ContentManager::exploreZipFolders(Ogre::String rg)
{
	String dirsep="/";
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
	dirsep="\\";
#endif
	ResourceGroupManager& rgm = ResourceGroupManager::getSingleton();

	FileInfoListPtr files= rgm.findResourceFileInfo(rg, "*.skinzip"); //search for skins
	FileInfoList::iterator iterFiles = files->begin();
	for (; iterFiles!= files->end(); ++iterFiles)
	{
		if (!iterFiles->archive) continue;
		String fullpath = iterFiles->archive->getName() + dirsep;
		rgm.addResourceLocation(fullpath + iterFiles->filename, "Zip", rg);
	}
	// DO NOT initialize ...
}

void ContentManager::exploreFolders(Ogre::String rg)
{
	String dirsep="/";
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
	dirsep="\\";
#endif
	ResourceGroupManager& rgm = ResourceGroupManager::getSingleton();

	FileInfoListPtr files= rgm.findResourceFileInfo(rg, "*", true); // searching for dirs
	FileInfoList::iterator iterFiles = files->begin();
	for (; iterFiles!= files->end(); ++iterFiles)
	{
		if (!iterFiles->archive) continue;
		if (iterFiles->filename==String(".svn")) continue;
		// trying to get the full path
		String fullpath = iterFiles->archive->getName() + dirsep;
		rgm.addResourceLocation(fullpath+iterFiles->filename, "FileSystem", rg);
	}
	LOG("initialiseResourceGroups: "+rg);
	try
	{
		if (BSETTING("Background Loading", false))
			ResourceBackgroundQueue::getSingleton().initialiseResourceGroup(rg);
		else
			ResourceGroupManager::getSingleton().initialiseResourceGroup(rg);
	} catch(Ogre::Exception& e)
	{
		LOG("catched error while initializing Resource group '" + rg + "' : " + e.getFullDescription());
	}
}
