/*
	This source file is part of Rigs of Rods
	Copyright 2005-2012 Pierre-Michel Ricordel
	Copyright 2007-2012 Thomas Fischer
	Copyright 2013-2014 Petr Ohlidal

	For more information, see http://www.rigsofrods.com/

	Rigs of Rods is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License version 3, as
	published by the Free Software Foundation.

	Rigs of Rods is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Rigs of Rods. If not, see <http://www.gnu.org/licenses/>.
*/

/** 
	@file ContentManager.h
*/

#pragma once

#include "RoRPrerequisites.h"
#include "Singleton.h"

#include <OgreResourceGroupManager.h>

namespace RoR
{

class ContentManager : public Ogre::ResourceLoadingListener, public ZeroedMemoryAllocator
{
	friend class RoR::Application; // Manages lifecycle of this class

public:
	
	void initBootstrap(void);
	bool init(void);

	void loadMainResource(Ogre::String name, Ogre::String group=Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);

protected:

	ContentManager();
	~ContentManager();

	void exploreFolders(Ogre::String rg);
	void exploreZipFolders(Ogre::String rg);

	// implementation for resource loading listener
	Ogre::DataStreamPtr resourceLoading(const Ogre::String &name, const Ogre::String &group, Ogre::Resource *resource);
	void resourceStreamOpened(const Ogre::String &name, const Ogre::String &group, Ogre::Resource *resource, Ogre::DataStreamPtr& dataStream);
	bool resourceCollision(Ogre::Resource *resource, Ogre::ResourceManager *resourceManager);

};

} // namespace RoR
