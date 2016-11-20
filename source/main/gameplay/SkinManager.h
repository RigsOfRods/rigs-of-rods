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

#pragma once

#include <OgreResourceManager.h>

#include "RoRPrerequisites.h"

#include "Skin.h"

namespace RoR {

/** Manages Skin resources, parsing .skin files and generally organizing them. */
class SkinManager : public Ogre::ResourceManager
{
public:
    SkinManager();
    ~SkinManager();

    void GetUsableSkins(Ogre::String guid, std::vector<Skin *>& skins);

    // == Ogre::ResourceManager interface functions ==

    void parseScript(Ogre::DataStreamPtr& stream, const Ogre::String& groupName);
    void unload(const Ogre::String& name);
    void unload(Ogre::ResourceHandle handle);
    void unloadAll(bool reloadableOnly = true);
    void unloadUnreferencedResources(bool reloadableOnly = true);
    void remove(Ogre::ResourcePtr& r);
    void remove(const Ogre::String& name);
    void remove(Ogre::ResourceHandle handle);
    void removeAll(void);
    void reloadAll(bool reloadableOnly = true);
    void reloadUnreferencedResources(bool reloadableOnly = true);

protected:

    void ParseSkinAttribute(const Ogre::String& line, Skin* pSkin);

    // Internal methods
    Ogre::Resource* createImpl(const Ogre::String& name, Ogre::ResourceHandle handle,
        const Ogre::String& group, bool isManual, Ogre::ManualResourceLoader* loader,
        const Ogre::NameValuePairList* params);
};

}; // namespace RoR
