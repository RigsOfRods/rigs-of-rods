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
#ifndef _SkinManager_H__
#define _SkinManager_H__

#include "RoRPrerequisites.h"

#include "OgreResourceManager.h"
#include "Singleton.h"
#include "Skin.h"

/** Manages Skin resources, parsing .skin files and generally organizing them. */
class SkinManager : public Ogre::ResourceManager, public RoRSingleton< SkinManager >
{
public:
	SkinManager();
	~SkinManager();

	void parseScript(Ogre::DataStreamPtr& stream, const Ogre::String& groupName);

	void getMaterialAlternatives(Ogre::String materialName, std::vector<Skin *> &skin);
	void getUsableSkins(Ogre::String guid, std::vector<Skin *> &skins);
	bool hasSkinForGUID(Ogre::String guid);

	int getSkinCount();
	int serialize(Ogre::String &dst);
	void clear();

	void parseAttribute(const Ogre::String& line, Skin *pSkin);
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

	/// Internal methods
	Ogre::Resource* createImpl(const Ogre::String& name, Ogre::ResourceHandle handle,
		const Ogre::String& group, bool isManual, Ogre::ManualResourceLoader* loader,
		const Ogre::NameValuePairList* params);

	void logBadAttrib(const Ogre::String& line, Skin *pSkin);


};

#endif
