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
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Rigs of Rods. If not, see <http://www.gnu.org/licenses/>.
*/

#include "ContentManager.h"


#include <Overlay/OgreOverlayManager.h>
#include <Overlay/OgreOverlay.h>
#include <Plugins/ParticleFX/OgreBoxEmitterFactory.h>


#include "Application.h"
#include "ColoredTextAreaOverlayElementFactory.h"
#include "ErrorUtils.h"
#include "SoundScriptManager.h"
#include "SkinFileFormat.h"
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

#include <OgreFileSystem.h>
#include <regex>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <sstream>

using namespace Ogre;
using namespace RoR;

// ================================================================================
// Static variables
// ================================================================================

#define DECLARE_RESOURCE_PACK(_FIELD_, _NAME_, _RESOURCE_GROUP_) \
    const ContentManager::ResourcePack ContentManager::ResourcePack::_FIELD_(_NAME_, _RESOURCE_GROUP_);

DECLARE_RESOURCE_PACK( OGRE_CORE,             "OgreCore",             "OgreCoreRG");
DECLARE_RESOURCE_PACK( WALLPAPERS,            "wallpapers",           "Wallpapers");
DECLARE_RESOURCE_PACK( AIRFOILS,              "airfoils",             "AirfoilsRG");
DECLARE_RESOURCE_PACK( CAELUM,                "caelum",               "CaelumRG");
DECLARE_RESOURCE_PACK( CUBEMAPS,              "cubemaps",             "CubemapsRG");
DECLARE_RESOURCE_PACK( DASHBOARDS,            "dashboards",           "DashboardsRG");
DECLARE_RESOURCE_PACK( FAMICONS,              "famicons",             "FamiconsRG");
DECLARE_RESOURCE_PACK( FLAGS,                 "flags",                "FlagsRG");
DECLARE_RESOURCE_PACK( FONTS,                 "fonts",                "FontsRG");
DECLARE_RESOURCE_PACK( HYDRAX,                "hydrax",               "HydraxRG");
DECLARE_RESOURCE_PACK( ICONS,                 "icons",                "IconsRG");
DECLARE_RESOURCE_PACK( MATERIALS,             "materials",            "MaterialsRG");
DECLARE_RESOURCE_PACK( MESHES,                "meshes",               "MeshesRG");
DECLARE_RESOURCE_PACK( MYGUI,                 "mygui",                "MyGuiRG");
DECLARE_RESOURCE_PACK( OVERLAYS,              "overlays",             "OverlaysRG");
DECLARE_RESOURCE_PACK( PAGED,                 "paged",                "PagedRG");
DECLARE_RESOURCE_PACK( PARTICLES,             "particles",            "ParticlesRG");
DECLARE_RESOURCE_PACK( PSSM,                  "pssm",                 "PssmRG");
DECLARE_RESOURCE_PACK( RTSHADER,              "rtshader",             "RtShaderRG");
DECLARE_RESOURCE_PACK( SCRIPTS,               "scripts",              "ScriptsRG");
DECLARE_RESOURCE_PACK( SOUNDS,                "sounds",               "SoundsRG");
DECLARE_RESOURCE_PACK( TEXTURES,              "textures",             "TexturesRG");
DECLARE_RESOURCE_PACK( SKYX,                  "SkyX",                 "SkyXRG");
DECLARE_RESOURCE_PACK( SHADOWVOLUME,          "ShadowVolume",         "ShadowVolumeRG");
DECLARE_RESOURCE_PACK( RTSHADERLIB,           "RTShaderLib",          "RTShaderLibRG");

// ================================================================================
// Functions
// ================================================================================

void ContentManager::AddResourcePack(ResourcePack const& resource_pack, std::string const& override_rgn)
{
    Ogre::ResourceGroupManager& rgm = Ogre::ResourceGroupManager::getSingleton();

    Ogre::String rg_name;
    if (!override_rgn.empty()) // Custom RG defined?
    {
        rg_name = override_rgn;
    }
    else // Use default RG
    {
        if (rgm.resourceGroupExists(resource_pack.resource_group_name)) // Already loaded?
        {
            return; // Nothing to do, nothing to report
        }
        rg_name = resource_pack.resource_group_name;
    }

    std::stringstream log_msg;
    log_msg << "[RoR|ContentManager] Loading resource pack \"" << resource_pack.name << "\" to group \"" << rg_name << "\"";
    std::string dir_path = PathCombine(App::sys_resources_dir->GetStr(), resource_pack.name);
    std::string zip_path = dir_path + ".zip";
    if (FileExists(zip_path))
    {
        log_msg << " (ZIP archive)";
        LOG(log_msg.str());
        rgm.addResourceLocation(zip_path, "Zip", rg_name);
    }
    else
    {
        if (FolderExists(dir_path))
        {
            log_msg << " (directory)";
            LOG(log_msg.str());
            rgm.addResourceLocation(dir_path, "FileSystem", rg_name);
        }
        else
        {
            log_msg << " failed, data not found.";
            throw std::runtime_error(log_msg.str());
        }
    }

    if (override_rgn.empty()) // Only init the default RG
    {
        rgm.initialiseResourceGroup(rg_name);
    }
}

void ContentManager::InitContentManager()
{
    ResourceGroupManager::getSingleton().addResourceLocation(
        App::sys_config_dir->GetStr(), "FileSystem", RGN_CONFIG, /*recursive=*/false, /*readOnly=*/false);
    ResourceGroupManager::getSingleton().addResourceLocation(
        App::sys_savegames_dir->GetStr(), "FileSystem", RGN_SAVEGAMES, /*recursive=*/false, /*readOnly=*/false);

    Ogre::ScriptCompilerManager::getSingleton().setListener(this);

    // Initialize "managed materials" first
    //   These are base materials referenced by user content
    //   They must be initialized before any content is loaded,
    //   otherwise material links are unresolved and loading ends with an exception
    this->InitManagedMaterials(RGN_MANAGED_MATS);

    // set listener if none has already been set
    if (!Ogre::ResourceGroupManager::getSingleton().getLoadingListener())
        Ogre::ResourceGroupManager::getSingleton().setLoadingListener(this);

    // by default, display everything in the depth map
    Ogre::MovableObject::setDefaultVisibilityFlags(DEPTHMAP_ENABLED);


    this->AddResourcePack(ResourcePack::MYGUI);
    this->AddResourcePack(ResourcePack::DASHBOARDS);


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
    App::CreateSoundScriptManager();
    App::GetSoundScriptManager()->setLoadingBaseSounds(true);
#endif // USE_OPENAL

    AddResourcePack(ResourcePack::SOUNDS);

    // streams path, to be processed later by the cache system
    LOG("RoR|ContentManager: Loading filesystems");

    // add scripts folder
    ResourceGroupManager::getSingleton().addResourceLocation(std::string(App::sys_user_dir->GetStr()) + PATH_SLASH + "scripts", "FileSystem", "Scripts");

    LOG("RoR|ContentManager: Registering colored text overlay factory");
    ColoredTextAreaOverlayElementFactory* pCT = new ColoredTextAreaOverlayElementFactory();
    OverlayManager::getSingleton().addOverlayElementFactory(pCT);

    // set default mipmap level (NB some APIs ignore this)
    if (TextureManager::getSingletonPtr())
        TextureManager::getSingleton().setDefaultNumMipmaps(5);

    TextureFilterOptions tfo = TFO_NONE;
    switch (App::gfx_texture_filter->GetEnum<GfxTexFilter>())
    {
    case GfxTexFilter::ANISOTROPIC: tfo = TFO_ANISOTROPIC;        break;
    case GfxTexFilter::TRILINEAR:   tfo = TFO_TRILINEAR;          break;
    case GfxTexFilter::BILINEAR:    tfo = TFO_BILINEAR;           break;
    case GfxTexFilter::NONE:        tfo = TFO_NONE;               break;
    }
    MaterialManager::getSingleton().setDefaultAnisotropy(Math::Clamp(App::gfx_anisotropy->GetInt(), 1, 16));
    MaterialManager::getSingleton().setDefaultTextureFiltering(tfo);

    // load all resources now, so the zip files are also initiated
    LOG("RoR|ContentManager: Calling initialiseAllResourceGroups()");
    try
    {
        ResourceGroupManager::getSingleton().initialiseAllResourceGroups();
    }
    catch (Ogre::Exception& e)
    {
        LOG("RoR|ContentManager: catched error while initializing Resource groups: " + e.getFullDescription());
    }
#ifdef USE_OPENAL
    App::GetSoundScriptManager()->setLoadingBaseSounds(false);
#endif // USE_OPENAL
}

void ContentManager::InitModCache(CacheValidity validity)
{
    ResourceGroupManager::getSingleton().addResourceLocation(
        App::sys_cache_dir->GetStr(), "FileSystem", RGN_CACHE, /*recursive=*/false, /*readOnly=*/false);
    std::string user = App::sys_user_dir->GetStr();
    std::string base = App::sys_process_dir->GetStr();
    std::string objects = PathCombine("resources", "beamobjects.zip");

    if (!App::app_extra_mod_path->GetStr().empty())
    {
        std::string extra_mod_path = App::app_extra_mod_path->GetStr();
        ResourceGroupManager::getSingleton().addResourceLocation(extra_mod_path           , "FileSystem", RGN_CONTENT);
    }
    ResourceGroupManager::getSingleton().addResourceLocation(PathCombine(user, "mods")    , "FileSystem", RGN_CONTENT);
    ResourceGroupManager::getSingleton().addResourceLocation(PathCombine(user, "packs")   , "FileSystem", RGN_CONTENT);
    ResourceGroupManager::getSingleton().addResourceLocation(PathCombine(user, "terrains"), "FileSystem", RGN_CONTENT);
    ResourceGroupManager::getSingleton().addResourceLocation(PathCombine(user, "vehicles"), "FileSystem", RGN_CONTENT);
    ResourceGroupManager::getSingleton().addResourceLocation(PathCombine(base, "content") , "FileSystem", RGN_CONTENT);
    ResourceGroupManager::getSingleton().addResourceLocation(PathCombine(base, objects)   , "Zip"       , RGN_CONTENT);

    ResourceGroupManager::getSingleton().createResourceGroup(RGN_TEMP, false);
    if (!App::app_extra_mod_path->GetStr().empty())
    {
        std::string extra_mod_path = App::app_extra_mod_path->GetStr();
        ResourceGroupManager::getSingleton().addResourceLocation(extra_mod_path           , "FileSystem", RGN_TEMP, true);
    }
    ResourceGroupManager::getSingleton().addResourceLocation(PathCombine(user, "mods")    , "FileSystem", RGN_TEMP, true);
    ResourceGroupManager::getSingleton().addResourceLocation(PathCombine(user, "packs")   , "FileSystem", RGN_TEMP, true);
    ResourceGroupManager::getSingleton().addResourceLocation(PathCombine(user, "terrains"), "FileSystem", RGN_TEMP, true);
    ResourceGroupManager::getSingleton().addResourceLocation(PathCombine(user, "vehicles"), "FileSystem", RGN_TEMP, true);
    ResourceGroupManager::getSingleton().addResourceLocation(PathCombine(base, "content") , "FileSystem", RGN_TEMP, true);
    FileInfoListPtr files = ResourceGroupManager::getSingleton().findResourceFileInfo(RGN_TEMP, "*", true);
    for (const auto& file : *files)
    {
        if (!file.archive)
            continue;
        String fullpath = PathCombine(file.archive->getName(), file.filename);
        ResourceGroupManager::getSingleton().addResourceLocation(fullpath, "FileSystem", RGN_CONTENT);
    }
    ResourceGroupManager::getSingleton().destroyResourceGroup(RGN_TEMP);

    if (validity == CacheValidity::UNKNOWN)
    {
        validity = m_mod_cache.EvaluateCacheValidity();
    }
    m_mod_cache.LoadModCache(validity);
    App::SetCacheSystem(&m_mod_cache); // Temporary solution until Modcache+ContentManager are fully merged and `App::GetCacheSystem()` is removed ~ only_a_ptr, 10/2018

    ResourceGroupManager::getSingleton().destroyResourceGroup(RGN_CONTENT);
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
    // RoR loads each resource bundle (see CacheSystem.h for info)
    // into dedicated resource group outside the global pool [see CacheSystem::LoadResource()]
    // This means resource collision is entirely content creator's fault.
    RoR::LogFormat("[RoR|ContentManager] Skipping resource with duplicate name: '%s' (origin: '%s')",
        resource->getName().c_str(), resource->getOrigin().c_str());
    return false; // Instruct OGRE to drop the new resource and keep the original.
}

bool ContentManager::handleEvent(ScriptCompiler *compiler, ScriptCompilerEvent *evt, void *retval)
{
    if (evt->mType == CreateMaterialScriptCompilerEvent::eventType)
    {
        // Workaround for OGRE script compiler not properly checking that material name is not empty.
        // See https://github.com/RigsOfRods/rigs-of-rods/issues/2349
        auto* matEvent = static_cast<CreateMaterialScriptCompilerEvent*>(evt);
        if (matEvent->mName.empty())
        {
            RoR::LogFormat("[RoR] Got malformed material (empty name) from file: '%s' - forcing OGRE to fail loading.",
                matEvent->mFile.c_str());
            // Report "handled" but create nothing -> OGRE will interrupt the loading
            //   with message "failed to find or create material" [in MaterialTranslator::translate()]
            return true;
        }
    }
    else if (evt->mType == CreateParticleSystemScriptCompilerEvent::eventType)
    {
        // Workaround for OGRE ignoring resource groups when registering particle templates
        // See https://github.com/RigsOfRods/rigs-of-rods/pull/2398
        auto* particleEvent = static_cast<CreateParticleSystemScriptCompilerEvent*>(evt);
        if (Ogre::ParticleSystemManager::getSingleton().getTemplate(particleEvent->mName) != nullptr)
        {
            // Duplicate name -> OGRE would throw exception and fail initializing whole resource group
            RoR::LogFormat("[RoR] Duplicate particle system name '%s' in file: '%s' - forcing OGRE to fail loading.",
                particleEvent->mName.c_str(), particleEvent->mFile.c_str());
            return true; // Instruct OGRE to skip the particle system
        }
    }

    return false; // Report "not handled"
}

void ContentManager::InitManagedMaterials(std::string const & rg_name)
{
    Ogre::String managed_materials_dir = PathCombine(App::sys_resources_dir->GetStr(), "managed_materials");

    //Dirty, needs to be improved
    if (App::gfx_shadow_type->GetEnum<GfxShadowType>() == GfxShadowType::PSSM)
    {
        if (rg_name == RGN_MANAGED_MATS) // Only load shared resources on startup
        {
            ResourceGroupManager::getSingleton().addResourceLocation(PathCombine(managed_materials_dir, "shadows/pssm/on/shared"), "FileSystem", rg_name);
        }
        ResourceGroupManager::getSingleton().addResourceLocation(PathCombine(managed_materials_dir, "shadows/pssm/on"), "FileSystem", rg_name);
    }
    else
    {
        ResourceGroupManager::getSingleton().addResourceLocation(PathCombine(managed_materials_dir,"shadows/pssm/off"), "FileSystem", rg_name);
    }

    ResourceGroupManager::getSingleton().addResourceLocation(PathCombine(managed_materials_dir, "texture"), "FileSystem", rg_name);

    // Last
    ResourceGroupManager::getSingleton().addResourceLocation(managed_materials_dir, "FileSystem", rg_name);

    if (rg_name == RGN_MANAGED_MATS) // Only initialize the global resource group
        ResourceGroupManager::getSingleton().initialiseResourceGroup(rg_name);
}

void ContentManager::LoadGameplayResources()
{
    if (!m_base_resource_loaded)
    {
        this->AddResourcePack(ContentManager::ResourcePack::AIRFOILS);
        this->AddResourcePack(ContentManager::ResourcePack::TEXTURES);
        this->AddResourcePack(ContentManager::ResourcePack::FAMICONS);
        this->AddResourcePack(ContentManager::ResourcePack::MATERIALS);
        this->AddResourcePack(ContentManager::ResourcePack::MESHES);
        this->AddResourcePack(ContentManager::ResourcePack::OVERLAYS);
        this->AddResourcePack(ContentManager::ResourcePack::PARTICLES);
        this->AddResourcePack(ContentManager::ResourcePack::SCRIPTS);
        this->AddResourcePack(ContentManager::ResourcePack::SHADOWVOLUME);

        m_base_resource_loaded = true;
    }

    if (App::gfx_water_mode->GetEnum<GfxWaterMode>() == GfxWaterMode::HYDRAX)
        this->AddResourcePack(ContentManager::ResourcePack::HYDRAX);

    if (App::gfx_sky_mode->GetEnum<GfxSkyMode>() == GfxSkyMode::CAELUM)
        this->AddResourcePack(ContentManager::ResourcePack::CAELUM);

    if (App::gfx_sky_mode->GetEnum<GfxSkyMode>() == GfxSkyMode::SKYX)
        this->AddResourcePack(ContentManager::ResourcePack::SKYX);

    if (App::gfx_vegetation_mode->GetEnum<GfxVegetation>() != RoR::GfxVegetation::NONE)
        this->AddResourcePack(ContentManager::ResourcePack::PAGED);
}

std::string ContentManager::ListAllUserContent()
{
    std::stringstream buf;

    auto dir_list = Ogre::ResourceGroupManager::getSingleton().listResourceFileInfo(RGN_CONTENT, true);
    for (auto dir: *dir_list)
    {
        buf << dir.filename << std::endl;
    }

    // Any filename + listed extensions, ignore case
    std::regex file_whitelist("^.\\.(airplane|boat|car|fixed|load|machine|skin|terrn2|train|truck)$", std::regex::icase);

    auto file_list = Ogre::ResourceGroupManager::getSingleton().listResourceFileInfo(RGN_CONTENT, false);
    for (auto file: *file_list)
    {
        if ((file.archive != nullptr) || std::regex_match(file.filename, file_whitelist))
        {
            buf << file.filename << std::endl;
        }
    }

    return buf.str();
}

bool ContentManager::LoadAndParseJson(std::string const& filename, std::string const& rg_name, rapidjson::Document& j_doc)
{
    try
    {
        Ogre::DataStreamPtr stream = Ogre::ResourceGroupManager::getSingleton().openResource(filename, rg_name);
        Ogre::String json_str = stream->getAsString();
        rapidjson::MemoryStream j_stream(json_str.data(), json_str.length());
        j_doc.ParseStream<rapidjson::kParseNanAndInfFlag>(j_stream);
    }
    catch (Ogre::FileNotFoundException)
    {
        return false; // Error already logged by OGRE
    }
    catch (std::exception& e)
    {
        RoR::LogFormat("[RoR] Failed to open or read json file '%s' (resource group '%s'), message: '%s'",
                       filename.c_str(), rg_name.c_str(), e.what());
        return false;
    }

    if (j_doc.HasParseError())
    {
        RoR::LogFormat("[RoR] Error parsing JSON file '%s' (resource group '%s')",
                       filename.c_str(), rg_name.c_str());
        return false;
    }

    return true;
}

bool ContentManager::SerializeAndWriteJson(std::string const& filename, std::string const& rg_name, rapidjson::Document& j_doc)
{
    // Serialize JSON to string
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer, rapidjson::UTF8<>, rapidjson::UTF8<>,
                      rapidjson::CrtAllocator, rapidjson::kWriteNanAndInfFlag>
                      writer(buffer);
    j_doc.Accept(writer);

    // Write JSON to file
    try
    {
        Ogre::DataStreamPtr stream
            = Ogre::ResourceGroupManager::getSingleton().createResource(
                filename, rg_name, /*overwrite=*/true);
        size_t written = stream->write(buffer.GetString(), buffer.GetSize());
        if (written < buffer.GetSize())
        {
            RoR::LogFormat("[RoR] Error writing JSON file '%s' (resource group '%s'), ",
                           "only written %u out of %u bytes!",
                           filename.c_str(), rg_name.c_str(), written, buffer.GetSize());
            return false;
        }
        return true;
    }
    catch (std::exception& e)
    {
        RoR::LogFormat("[RoR] Error writing JSON file '%s' (resource group '%s'), message: '%s'",
                       filename.c_str(), rg_name.c_str(), e.what());
        return false;
    }
}

bool ContentManager::DeleteDiskFile(std::string const& filename, std::string const& rg_name)
{
    try
    {
        Ogre::ResourceGroupManager::getSingleton().deleteResource(filename, rg_name);
        return true;
    }
    catch (std::exception& e)
    {
        RoR::LogFormat("[RoR|ModCache] Error deleting file '%s' (resource group '%s'), message: '%s'",
                        filename.c_str(), rg_name.c_str(), e.what());
        return false;
    }
}

