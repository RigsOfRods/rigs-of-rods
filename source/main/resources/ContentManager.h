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

namespace RoR {

/// Manages game files (under 'GAMEDIR/resources/')
class ContentManager:
    public Ogre::ResourceLoadingListener, // Ogre::ResourceGroupManager::getSingleton().setLoadingListener()
    public Ogre::ScriptCompilerListener   // Ogre::ScriptCompilerManager::getSingleton().setListener()
{
public:

                       /// Loads game files if not already loaded
                       /// @param override_rg If not set, the OGRE builtin "Default" group is used -> resources won't unload until shutdown
    void               AddResourcePack(std::string const& resource_pack, std::string const& override_rgn = "");
    void               InitBaseManagedMaterials(std::string const & rg_name);
    void               InitActorManagedMaterials(std::string const & rg_name);
    void               InitContentManager();
    void               InitModCache(CacheValidity validity);
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
};

} // namespace RoR
