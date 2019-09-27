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

/// @file

#pragma once

#include "CacheSystem.h"
#include "Application.h"

#include <OgreResourceGroupManager.h>
#include <OgreScriptCompiler.h>
#include <rapidjson/document.h>

#define RGN_TEMP "Temp"
#define RGN_CACHE "Cache"
#define RGN_CONFIG "Config"
#define RGN_CONTENT "Content"
#define RGN_SAVEGAMES "Savegames"
#define RGN_MANAGED_MATS "ManagedMaterials"

namespace RoR {

class ContentManager:
    public Ogre::ResourceLoadingListener, // Ogre::ResourceGroupManager::getSingleton().setLoadingListener()
    public Ogre::ScriptCompilerListener   // Ogre::ScriptCompilerManager::getSingleton().setListener()
{
public:

    struct ResourcePack
    {
        ResourcePack(const char* name, const char* resource_group_name):
            name(name), resource_group_name(resource_group_name)
        {}

        static const ResourcePack OGRE_CORE;
        static const ResourcePack WALLPAPERS;
        static const ResourcePack AIRFOILS;
        static const ResourcePack BEAM_OBJECTS;
        static const ResourcePack CAELUM;
        static const ResourcePack CUBEMAPS;
        static const ResourcePack DASHBOARDS;
        static const ResourcePack FAMICONS;
        static const ResourcePack FLAGS;
        static const ResourcePack FONTS;
        static const ResourcePack HYDRAX;
        static const ResourcePack ICONS;
        static const ResourcePack MATERIALS;
        static const ResourcePack MESHES;
        static const ResourcePack MYGUI;
        static const ResourcePack OVERLAYS;
        static const ResourcePack PAGED;
        static const ResourcePack PARTICLES;
        static const ResourcePack PSSM;
        static const ResourcePack SKYX;
        static const ResourcePack SHADOWVOLUME;
        static const ResourcePack RTSHADER;
        static const ResourcePack SCRIPTS;
        static const ResourcePack SOUNDS;
        static const ResourcePack TEXTURES;
        static const ResourcePack RTSHADERLIB;

        const char* name;
        const char* resource_group_name;
    };

                       /// Loads resources if not already loaded
                       /// @param override_rg If not set, the ResourcePack's RG is used -> resources won't unload until shutdown
    void               AddResourcePack(ResourcePack const& resource_pack, std::string const& override_rgn = "");
    void               InitManagedMaterials(std::string const & rg_name);
    void               InitContentManager();
    void               InitModCache(CacheValidity validity);
    void               LoadGameplayResources();  //!< Checks GVar settings and loads required resources.
    std::string        ListAllUserContent(); //!< Used by ModCache for quick detection of added/removed content
    bool               DeleteDiskFile(std::string const& filename, std::string const& rg_name);

    // JSON:
    bool               LoadAndParseJson(std::string const& filename, std::string const& rg_name, rapidjson::Document& j_doc);
    bool               SerializeAndWriteJson(std::string const& filename, std::string const& rg_name, rapidjson::Document& j_doc);

private:

    // Ogre::ResourceLoadingListener
    Ogre::DataStreamPtr resourceLoading(const Ogre::String& name, const Ogre::String& group, Ogre::Resource* resource) override;
    void resourceStreamOpened(const Ogre::String& name, const Ogre::String& group, Ogre::Resource* resource, Ogre::DataStreamPtr& dataStream) override;
    bool resourceCollision(Ogre::Resource* resource, Ogre::ResourceManager* resourceManager) override;

    // Ogre::ScriptCompilerListener
    bool handleEvent(Ogre::ScriptCompiler *compiler, Ogre::ScriptCompilerEvent *evt, void *retval) override;

    CacheSystem       m_mod_cache; //!< Database of addon content
    bool              m_base_resource_loaded;
};

} // namespace RoR
