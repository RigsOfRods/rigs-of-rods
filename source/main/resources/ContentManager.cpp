/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2013-2015 Petr Ohlidal

    For more information, see http://www.rigsofrods.org/

    Rigs of Rods is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 3, as
    published by the Free Software Foundation.

    Rigs of Rods is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Rigs of Rods. If not, see <http://www.gnu.org/licenses/>.
*/

#include "ContentManager.h"

#ifdef ROR_USE_OGRE_1_9
#	include <Overlay/OgreOverlayManager.h>
#	include <Overlay/OgreOverlay.h>
#	include <Plugins/ParticleFX/OgreBoxEmitterFactory.h>
#else
#	include <OgreOverlayManager.h>
#	include <OgreOverlayElement.h>
#endif

#include "Application.h"
#include "Settings.h"
#include "ColoredTextAreaOverlayElementFactory.h"
#include "ErrorUtils.h"
#include "SoundScriptManager.h"
#include "SkinManager.h"
#include "Language.h"
#include "PlatformUtils.h"

#include "CacheSystem.h"

#include "OgreShaderParticleRenderer.h"

// Removed by Skybon as part of OGRE 1.9 port 
// Disabling temporarily for 1.8.1 as well. ~ only_a_ptr, 2015-11
// TODO: Study the system, then re-enable or remove entirely.
//#include "OgreBoxEmitterFactory.h"

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
DECLARE_RESOURCE_PACK(  2, WALLPAPERS,            "wallpapers",           "Wallpapers");
// UNUSED               3
DECLARE_RESOURCE_PACK(  4, AIRFOILS,              "airfoils",             "LoadBeforeMap");
DECLARE_RESOURCE_PACK(  5, BEAM_OBJECTS,          "beamobjects",          "LoadBeforeMap");
DECLARE_RESOURCE_PACK(  6, BLUR,                  "blur",                 "LoadBeforeMap");
DECLARE_RESOURCE_PACK(  7, CAELUM,                "caelum",               "LoadBeforeMap");
DECLARE_RESOURCE_PACK(  8, CUBEMAPS,              "cubemaps",             "General");
DECLARE_RESOURCE_PACK(  9, DASHBOARDS,            "dashboards",           "General");
DECLARE_RESOURCE_PACK( 10, DEPTH_OF_FIELD,        "dof",                  "LoadBeforeMap");
DECLARE_RESOURCE_PACK( 11, FAMICONS,              "famicons",             "LoadBeforeMap");
DECLARE_RESOURCE_PACK( 12, FLAGS,                 "flags",                "LoadBeforeMap");
DECLARE_RESOURCE_PACK( 13, GLOW,                  "glow",                 "LoadBeforeMap");
DECLARE_RESOURCE_PACK( 14, HDR,                   "hdr",                  "LoadBeforeMap");
DECLARE_RESOURCE_PACK( 15, HEATHAZE,              "heathaze",             "LoadBeforeMap");
DECLARE_RESOURCE_PACK( 16, HYDRAX,                "hydrax",               "LoadBeforeMap");
DECLARE_RESOURCE_PACK( 17, ICONS,                 "icons",                "LoadBeforeMap");
DECLARE_RESOURCE_PACK( 18, MATERIALS,             "materials",            "LoadBeforeMap");
DECLARE_RESOURCE_PACK( 19, MESHES,                "meshes",               "LoadBeforeMap");
DECLARE_RESOURCE_PACK( 20, MYGUI,                 "mygui",                "General");
DECLARE_RESOURCE_PACK( 21, OVERLAYS,              "overlays",             "LoadBeforeMap");
DECLARE_RESOURCE_PACK( 22, PAGED,                 "paged",                "LoadBeforeMap");
DECLARE_RESOURCE_PACK( 23, PARTICLES,             "particles",            "LoadBeforeMap");
DECLARE_RESOURCE_PACK( 24, PSSM,                  "pssm",                 "LoadBeforeMap");
DECLARE_RESOURCE_PACK( 25, RTSHADER,              "rtshader",             "General");
DECLARE_RESOURCE_PACK( 26, SCRIPTS,               "scripts",              "LoadBeforeMap");
DECLARE_RESOURCE_PACK( 27, SOUNDS,                "sounds",               "General");
DECLARE_RESOURCE_PACK( 28, SUNBURN,               "sunburn",              "LoadBeforeMap");
DECLARE_RESOURCE_PACK( 29, TEXTURES,              "textures",             "LoadBeforeMap");

// ================================================================================
// Functions
// ================================================================================

ContentManager::ContentManager():
    m_skin_manager(nullptr),
    m_loaded_resource_packs(0)
{
}

ContentManager::~ContentManager()
{
}

bool ContentManager::isLoaded(Ogre::uint64 res_pack_id)
{
    if (BITMASK_64_IS_1(m_loaded_resource_packs, res_pack_id)) // Already loaded?
    {
        return true;
    }
    return false;
}

void ContentManager::AddResourcePack(ResourcePack const& resource_pack)
{
    std::stringstream log_msg;
    if (BITMASK_64_IS_1(m_loaded_resource_packs, resource_pack.mask)) // Already loaded?
    {
        log_msg << "[RoR|ContentManager] Resource pack \"" << resource_pack.name << "\" already loaded.";
        LOG(log_msg.str());
        return;
    }
    log_msg << "[RoR|ContentManager] Loading resource pack \"" << resource_pack.name << "\" from group \"" << resource_pack.resource_group_name << "\"";
    Ogre::String resources_dir = Ogre::String(App::sys_resources_dir.GetActive()) + PATH_SLASH;
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


    AddResourcePack(ResourcePack::MYGUI);
    AddResourcePack(ResourcePack::DASHBOARDS);


#ifdef _WIN32
    // TODO: FIX UNDER LINUX!
    // register particle classes
    LOG("RoR|ContentManager: Registering Particle Box Emitter");
    ParticleSystemRendererFactory* mParticleSystemRendererFact = OGRE_NEW ShaderParticleRendererFactory();
    ParticleSystemManager::getSingleton().addRendererFactory(mParticleSystemRendererFact);

    // Removed by Skybon as part of OGRE 1.9 port 
    // Disabling temporarily for 1.8.1 as well.  ~ only_a_ptr, 2015-11
    //ParticleEmitterFactory *mParticleEmitterFact = OGRE_NEW BoxEmitterFactory();
    //ParticleSystemManager::getSingleton().addEmitterFactory(mParticleEmitterFact);

#endif // _WIN32

#ifdef USE_ANGELSCRIPT
    // FireExtinguisherAffector
    ParticleAffectorFactory* pAffFact = OGRE_NEW FireExtinguisherAffectorFactory();
    ParticleSystemManager::getSingleton().addAffectorFactory(pAffFact);

    // ExtinguishableFireAffector
    pAffFact = OGRE_NEW ExtinguishableFireAffectorFactory();
    ParticleSystemManager::getSingleton().addAffectorFactory(pAffFact);
#endif // USE_ANGELSCRIPT

    // sound is a bit special as we mark the base sounds so we don't clear them accidentally later on
#ifdef USE_OPENAL
    LOG("RoR|ContentManager: Creating Sound Manager");
    SoundScriptManager::getSingleton().setLoadingBaseSounds(true);
#endif // USE_OPENAL

    AddResourcePack(ResourcePack::SOUNDS);

    // streams path, to be processed later by the cache system
    LOG("RoR|ContentManager: Loading filesystems");

#if OGRE_VERSION_MAJOR >= 1 && OGRE_VERSION_MINOR >= 9
    ResourceGroupManager::getSingleton().addResourceLocation(App::sys_cache_dir.GetActive(), "FileSystem", "cache", false, false);
#else
    ResourceGroupManager::getSingleton().addResourceLocation(std::string(App::sys_cache_dir.GetActive()), "FileSystem", "cache");
#endif

    // config, flat
    ResourceGroupManager::getSingleton().addResourceLocation(std::string(RoR::App::sys_config_dir.GetActive()), "FileSystem", ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
    // packs, to be processed later by the cache system

    // add scripts folder
    ResourceGroupManager::getSingleton().addResourceLocation(std::string(App::sys_user_dir.GetActive()) + PATH_SLASH + "scripts", "FileSystem", "Scripts");

    // init skin manager, important to happen before trucks resource loading!
    LOG("RoR|ContentManager: Registering Skin Manager");
    m_skin_manager = new RoR::SkinManager(); // SkinManager registers itself

    LOG("RoR|ContentManager: Registering colored text overlay factory");
    ColoredTextAreaOverlayElementFactory* pCT = new ColoredTextAreaOverlayElementFactory();
    OverlayManager::getSingleton().addOverlayElementFactory(pCT);

    // set default mipmap level (NB some APIs ignore this)
    if (TextureManager::getSingletonPtr())
        TextureManager::getSingleton().setDefaultNumMipmaps(5);

    TextureFilterOptions tfo = TFO_NONE;
    switch (App::gfx_texture_filter.GetActive())
    {
    case GfxTexFilter::ANISOTROPIC: tfo = TFO_ANISOTROPIC;        break;
    case GfxTexFilter::TRILINEAR:   tfo = TFO_TRILINEAR;          break;
    case GfxTexFilter::BILINEAR:    tfo = TFO_BILINEAR;           break;
    case GfxTexFilter::NONE:        tfo = TFO_NONE;               break;
    }
    MaterialManager::getSingleton().setDefaultAnisotropy(8);
    MaterialManager::getSingleton().setDefaultTextureFiltering(tfo);

    // load all resources now, so the zip files are also initiated
    LOG("RoR|ContentManager: Calling initialiseAllResourceGroups()");
    try
    {
        if (BSETTING("Background Loading", false))
            ResourceBackgroundQueue::getSingleton().initialiseAllResourceGroups();
        else
            ResourceGroupManager::getSingleton().initialiseAllResourceGroups();
    }
    catch (Ogre::Exception& e)
    {
        LOG("RoR|ContentManager: catched error while initializing Resource groups: " + e.getFullDescription());
    }
#ifdef USE_OPENAL
    SoundScriptManager::getSingleton().setLoadingBaseSounds(false);
#endif // USE_OPENAL

    // and the content
    std::string content_base = std::string(App::sys_user_dir.GetActive()) + PATH_SLASH;
    ResourceGroupManager::getSingleton().addResourceLocation(content_base + "packs", "FileSystem", "Packs", true);
    ResourceGroupManager::getSingleton().addResourceLocation(content_base + "mods", "FileSystem", "Packs", true);

    ResourceGroupManager::getSingleton().addResourceLocation(content_base + "vehicles", "FileSystem", "VehicleFolders");
    ResourceGroupManager::getSingleton().addResourceLocation(content_base + "terrains", "FileSystem", "TerrainFolders");

    exploreFolders("VehicleFolders");
    exploreFolders("TerrainFolders");
    exploreZipFolders("Packs"); // this is required for skins to work

    LOG("RoR|ContentManager: Calling initialiseAllResourceGroups() - Content");
    try
    {
        if (BSETTING("Background Loading", false))
            ResourceBackgroundQueue::getSingleton().initialiseResourceGroup("Packs");
        else
            ResourceGroupManager::getSingleton().initialiseResourceGroup("Packs");
    }
    catch (Ogre::Exception& e)
    {
        LOG("RoR|ContentManager: catched error while initializing Content Resource groups: " + e.getFullDescription());
    }

    LanguageEngine::getSingleton().postSetup();

    return true;
}

Ogre::DataStreamPtr ContentManager::resourceLoading(const Ogre::String& name, const Ogre::String& group, Ogre::Resource* resource)
{
    return Ogre::DataStreamPtr();
}

void ContentManager::resourceStreamOpened(const Ogre::String& name, const Ogre::String& group, Ogre::Resource* resource, Ogre::DataStreamPtr& dataStream)
{
}

bool ContentManager::resourceCollision(Ogre::Resource* resource, Ogre::ResourceManager* resourceManager)
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
    ResourceGroupManager& rgm = ResourceGroupManager::getSingleton();

    FileInfoListPtr files = rgm.findResourceFileInfo(rg, "*.skinzip"); //search for skins
    FileInfoList::iterator iterFiles = files->begin();
    for (; iterFiles != files->end(); ++iterFiles)
    {
        if (!iterFiles->archive)
            continue;
        String fullpath = iterFiles->archive->getName() + PATH_SLASH;
        rgm.addResourceLocation(fullpath + iterFiles->filename, "Zip", rg);
    }
    // DO NOT initialize ...
}

void ContentManager::exploreFolders(Ogre::String rg)
{
    ResourceGroupManager& rgm = ResourceGroupManager::getSingleton();

    FileInfoListPtr files = rgm.findResourceFileInfo(rg, "*", true); // searching for dirs
    FileInfoList::iterator iterFiles = files->begin();
    for (; iterFiles != files->end(); ++iterFiles)
    {
        if (!iterFiles->archive)
            continue;
        if (iterFiles->filename == String(".svn"))
            continue;
        // trying to get the full path
        String fullpath = iterFiles->archive->getName() + PATH_SLASH;
        rgm.addResourceLocation(fullpath + iterFiles->filename, "FileSystem", rg);
    }
    LOG("initialiseResourceGroups: "+rg);
    try
    {
        if (BSETTING("Background Loading", false))
            ResourceBackgroundQueue::getSingleton().initialiseResourceGroup(rg);
        else
            ResourceGroupManager::getSingleton().initialiseResourceGroup(rg);
    }
    catch (Ogre::Exception& e)
    {
        LOG("catched error while initializing Resource group '" + rg + "' : " + e.getFullDescription());
    }
}

void ContentManager::InitManagedMaterials()
{
    Ogre::String managed_materials_dir = Ogre::String(App::sys_resources_dir.GetActive()) + PATH_SLASH + "managed_materials" + PATH_SLASH;

    //Dirty, needs to be improved
    if (App::gfx_shadow_type.GetActive() == GfxShadowType::PSSM)
        ResourceGroupManager::getSingleton().addResourceLocation(managed_materials_dir + "shadows/pssm/on/", "FileSystem", "ShadowsMats");
    else
        ResourceGroupManager::getSingleton().addResourceLocation(managed_materials_dir + "shadows/pssm/off/", "FileSystem", "ShadowsMats");

    ResourceGroupManager::getSingleton().initialiseResourceGroup("ShadowsMats");

    ResourceGroupManager::getSingleton().addResourceLocation(managed_materials_dir + "texture/", "FileSystem", "TextureManager");
    ResourceGroupManager::getSingleton().initialiseResourceGroup("TextureManager");

    //Last
    ResourceGroupManager::getSingleton().addResourceLocation(managed_materials_dir, "FileSystem", "ManagedMats");
    ResourceGroupManager::getSingleton().initialiseResourceGroup("ManagedMats");
}

void ContentManager::CheckAndLoadBaseResources()
{
    if (!m_base_resource_loaded)
    {
        RoR::App::GetContentManager()->AddResourcePack(ContentManager::ResourcePack::AIRFOILS);
        RoR::App::GetContentManager()->AddResourcePack(ContentManager::ResourcePack::BEAM_OBJECTS);

        RoR::App::GetContentManager()->AddResourcePack(ContentManager::ResourcePack::MATERIALS);
        RoR::App::GetContentManager()->AddResourcePack(ContentManager::ResourcePack::MESHES);
        RoR::App::GetContentManager()->AddResourcePack(ContentManager::ResourcePack::OVERLAYS);
        RoR::App::GetContentManager()->AddResourcePack(ContentManager::ResourcePack::PARTICLES);
        RoR::App::GetContentManager()->AddResourcePack(ContentManager::ResourcePack::SCRIPTS);
        RoR::App::GetContentManager()->AddResourcePack(ContentManager::ResourcePack::TEXTURES);
        RoR::App::GetContentManager()->AddResourcePack(ContentManager::ResourcePack::FLAGS);
        RoR::App::GetContentManager()->AddResourcePack(ContentManager::ResourcePack::ICONS);
        RoR::App::GetContentManager()->AddResourcePack(ContentManager::ResourcePack::FAMICONS);

        m_base_resource_loaded = true;
    }
}

void ContentManager::RegenCache()
{
    
    RoR::App::GetCacheSystem()->Startup(true); // true = force regeneration

    // Get stats
    int num_new = RoR::App::GetCacheSystem()->newFiles;
    int num_changed = RoR::App::GetCacheSystem()->changedFiles;
    int num_deleted = RoR::App::GetCacheSystem()->deletedFiles;

    // Report
    Ogre::UTFString str = _L("Cache regeneration done.\n");
    if (num_new > 0)
    {
        str = str + TOUTFSTRING(num_new) + _L(" new files\n");
    }
    if (num_changed > 0)
    {
        str = str + TOUTFSTRING(num_changed) + _L(" changed files\n");
    }
    if (num_deleted > 0)
    {
        str = str + TOUTFSTRING(num_deleted) + _L(" deleted files\n");
    }
    if (num_new + num_changed + num_deleted == 0)
    {
        str = str + _L("no changes");
    }
    str = str + _L("\n(These stats can be imprecise)");

    ErrorUtils::ShowError(_L("Cache regeneration done"), str);
}

