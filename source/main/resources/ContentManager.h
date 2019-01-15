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
#include "RoRPrerequisites.h"

#include <OgreResourceGroupManager.h>

namespace RoR {

class ContentManager : public Ogre::ResourceLoadingListener
{
public:

    ContentManager(): m_skin_manager(nullptr) {}

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
        static const ResourcePack RTSHADER;
        static const ResourcePack SCRIPTS;
        static const ResourcePack SOUNDS;
        static const ResourcePack TEXTURES;

        const char* name;
        const char* resource_group_name;
    };

    void               AddResourcePack(ResourcePack const& resource_pack); //!< Loads resources if not already loaded (currently resources never unload until shutdown)
    void               InitContentManager();
    void               InitModCache();
    void               LoadGameplayResources();  //!< Checks GVar settings and loads required resources.
    void               SynchUpdateModCache(); //!< Only to be called from main menu loop; performs rendering calls (status window)
    std::string        ListAllUserContent(); //!< Used by ModCache for quick detection of added/removed content
    RoR::SkinManager*  GetSkinManager()  { return m_skin_manager; }

private:

    void exploreFolders(Ogre::String rg);
    void exploreZipFolders(Ogre::String rg);
    void InitManagedMaterials();

    // implementation for resource loading listener
    Ogre::DataStreamPtr resourceLoading(const Ogre::String& name, const Ogre::String& group, Ogre::Resource* resource);
    void resourceStreamOpened(const Ogre::String& name, const Ogre::String& group, Ogre::Resource* resource, Ogre::DataStreamPtr& dataStream);
    bool resourceCollision(Ogre::Resource* resource, Ogre::ResourceManager* resourceManager);

    RoR::SkinManager* m_skin_manager;
    CacheSystem       m_mod_cache; //!< Database of addon content
    bool              m_base_resource_loaded;
};

} // namespace RoR
