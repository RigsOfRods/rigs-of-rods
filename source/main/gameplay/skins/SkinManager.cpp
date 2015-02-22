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
#include "SkinManager.h"

using namespace Ogre;

SkinManager::SkinManager() : ResourceManager()
{
	mLoadOrder = 200.0f;

	mScriptPatterns.push_back("*.skin");

	ResourceGroupManager::getSingleton()._registerScriptLoader(this);

	mResourceType = "RoRVehicleSkins";

	ResourceGroupManager::getSingleton()._registerResourceManager(mResourceType, this);
}

SkinManager::~SkinManager()
{
	ResourceGroupManager::getSingleton()._unregisterResourceManager(mResourceType);
	ResourceGroupManager::getSingleton()._unregisterScriptLoader(this);
}

Resource* SkinManager::createImpl(const String& name, ResourceHandle handle,
	const String& group, bool isManual, ManualResourceLoader* loader,
	const NameValuePairList* params)
{
	try
	{
		bool existing = (mResources.find(name) != mResources.end());
		if (existing)
			return mResources[name].getPointer();
		else
			return new Skin(this, name, handle, group, isManual, loader);
	} catch(Ogre::ItemIdentityException e)
	{
		return mResources[name].getPointer();
	}
}

void SkinManager::parseScript(DataStreamPtr& stream, const String& groupName)
{
	try
	{
		String line = "";
		Skin *pSkin = 0;

		while(!stream->eof())
		{
			line = stream->getLine();

			// Ignore blanks & comments
			if (!line.length() || line.substr(0, 2) == "//")
			{
				continue;
			}

			if (!pSkin)
			{
				// No current skin
				// So first valid data should be skin name
				pSkin = (Skin *)create(line, groupName).getPointer();
				if (pSkin)
				{
					pSkin->_notifyOrigin(stream->getName());
					stream->skipLine("{");
				}
			} else
			{
				// Already in skin
				if (line == "}")
				{
					// Finished
					//addImpl((Ogre::ResourcePtr)pSkin);
					pSkin = 0;
					// NB skin isn't loaded until required
				} else
				{
					parseAttribute(line, pSkin);
				}
			}
		}
	} catch(Ogre::ItemIdentityException e)
	{
		// this catches duplicates -> to be ignored
		// this happens since we load the full skin data off the cache, so we don't need
		// to re-add it to the SkinManager
		return;
	}
}

void SkinManager::parseAttribute(const String& line, Skin *pSkin)
{
	Ogre::StringVector params = StringUtil::split(line, "\t=,;\n");
	for (unsigned int i=0; i < params.size(); i++)
	{
		StringUtil::trim(params[i]);
	}
	String& attrib = params[0];
	StringUtil::toLowerCase(attrib);

	if      (attrib == "replacetexture"     && params.size() == 3) pSkin->addTextureReplace(params[1], params[2]);
	if      (attrib == "replacematerial"    && params.size() == 3) pSkin->addMaterialReplace(params[1], params[2]);
	else if (attrib == "preview"            && params.size() >= 2) pSkin->thumbnail = params[1];
	else if (attrib == "description"        && params.size() >= 2) pSkin->description = params[1];
	else if (attrib == "authorname"         && params.size() >= 2) pSkin->authorName = params[1];
	else if (attrib == "authorid"           && params.size() == 2) pSkin->authorID = PARSEINT(params[1]);
	else if (attrib == "guid"               && params.size() >= 2) pSkin->guid = params[1];
	else if (attrib == "name"               && params.size() >= 2) pSkin->name = params[1];

	StringUtil::trim(pSkin->name);
}

void SkinManager::logBadAttrib(const String& line, Skin *pSkin)
{
	LOG("Bad attribute line: " + line + " in skin " + pSkin->getName());
}

bool SkinManager::hasSkinForGUID(Ogre::String guid)
{
	Ogre::ResourceManager::ResourceMapIterator it = getResourceIterator();
	while (it.hasMoreElements())
	{
		Skin *skin = (Skin *)it.getNext().getPointer();

		if (skin->guid == guid)
		{
			return true;
		}
	}
	return false;
}


void SkinManager::getMaterialAlternatives(Ogre::String materialName, std::vector<Skin *> &skinVector)
{
	Ogre::ResourceManager::ResourceMapIterator it = getResourceIterator();
	while (it.hasMoreElements())
	{
		Skin *skin = (Skin *)it.getNext().getPointer();

		if (skin->hasReplacementForMaterial(materialName))
		{
			skinVector.push_back(skin);
		}
	}
}

void SkinManager::getUsableSkins(String guid, std::vector<Skin *> &skins)
{
	Ogre::ResourceManager::ResourceMapIterator it = getResourceIterator();
	while (it.hasMoreElements())
	{
		Skin *skin = (Skin *)it.getNext().getPointer();

		// fix some possible problems
		String g1 = guid;
		String g2 = skin->guid;

		StringUtil::trim(g1);
		StringUtil::trim(g2);

		StringUtil::toLowerCase(g1);
		StringUtil::toLowerCase(g2);

		// then compare
		if (g1 == g2)
		{
			skins.push_back(skin);
		}
	}
}

int SkinManager::getSkinCount()
{
	return mResourcesByHandle.size();
}

void SkinManager::clear()
{
	mResourcesByHandle.clear();
}

//we wont unload skins once loaded!
void SkinManager::unload(const String& name) {}
void SkinManager::unload(ResourceHandle handle) {}
void SkinManager::unloadAll(bool reloadableOnly) {}
void SkinManager::unloadUnreferencedResources(bool reloadableOnly) {}
void SkinManager::remove(ResourcePtr& r) {}
void SkinManager::remove(const String& name) {}
void SkinManager::remove(ResourceHandle handle) {}
void SkinManager::removeAll(void) {}
void SkinManager::reloadAll(bool reloadableOnly) {}
void SkinManager::reloadUnreferencedResources(bool reloadableOnly) {}
