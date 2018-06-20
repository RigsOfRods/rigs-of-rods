/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2013-2017 Petr Ohlidal & contributors

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

#pragma once

#include "RoRPrerequisites.h"

#include <OgreResourceManager.h>

#include <map>
#include <vector>
#include <string>

namespace RoR {

struct SkinDef
{
    std::map<std::string, std::string>  replace_textures;
    std::map<std::string, std::string>  replace_materials;
    std::string   name;
    std::string   guid;
    std::string   thumbnail;
    std::string   description;
    std::string   author_name;
    int           author_id;
};

/// Manages Skin resources, parsing .skin files and generally organizing them.
class SkinManager : public Ogre::ResourceManager
{
public:
    SkinManager();
    ~SkinManager();

    void GetUsableSkins(std::string guid, std::vector<SkinDef *>& skins);
    static void ApplySkinTextureReplacements(SkinDef* skin_def, Ogre::Entity* e);

    // == Ogre::ResourceManager interface functions ==

    Ogre::Resource* createImpl(const Ogre::String& name, Ogre::ResourceHandle handle,
        const Ogre::String& group, bool isManual, Ogre::ManualResourceLoader* loader,
        const Ogre::NameValuePairList* params) override
    {
        return nullptr; // Not used
    }
    void parseScript(Ogre::DataStreamPtr& stream, const Ogre::String& groupName) override;
    void reloadUnreferencedResources(bool reloadableOnly = true);
    void unloadUnreferencedResources(bool reloadableOnly = true);
    void unload     (const Ogre::String& name);
    void unload     (Ogre::ResourceHandle handle);
    void unloadAll  (bool reloadableOnly = true);
    
    void remove     (Ogre::ResourcePtr& r);
    void remove     (const Ogre::String& name);
    void remove     (Ogre::ResourceHandle handle);
    void removeAll  () override;
    void reloadAll  (bool reloadableOnly = true);

    static void ReplaceMaterialTextures(SkinDef* skin_def, std::string materialName);

private:

    void ParseSkinAttribute(const std::string& line, SkinDef* skin_def);

    std::map<std::string, SkinDef*> m_skins;
};

}; // namespace RoR
