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
#ifndef _Skin_H__
#define _Skin_H__

#include "RoRPrerequisites.h"
#include "OgrePrerequisites.h"
#include "OgreResource.h"
#include "OgreTexture.h"
#include "OgreMaterial.h"
#include "OgreCommon.h"

class Skin : public Ogre::Resource
{
public:
	/** Constructor.
	@see Resource
	*/
	Skin(Ogre::ResourceManager* creator, const Ogre::String& name, Ogre::ResourceHandle handle, const Ogre::String& group, bool isManual = false, Ogre::ManualResourceLoader* loader = 0);
	virtual ~Skin();

	// we are lazy and wont use separate get/set
	Ogre::String name;
	Ogre::String thumbnail;
	Ogre::String description;
	Ogre::String authorName;
	Ogre::String guid;
	int authorID;

	// texture
	int addTextureReplace(Ogre::String from, Ogre::String to);
	int hasReplacementForTexture(Ogre::String texture);
	Ogre::String getReplacementForTexture(Ogre::String texture);

	// material
	int addMaterialReplace(Ogre::String from, Ogre::String to);
	int hasReplacementForMaterial(Ogre::String material);
	Ogre::String getReplacementForMaterial(Ogre::String material);
	Ogre::String stripMaterialNameUniqueNess(Ogre::String matName);

	// common
	void uniquifyMeshMaterials(Ogre::Entity *e);

	void replaceMeshMaterials(Ogre::Entity *e);
	void replaceMaterialTextures(Ogre::String materialName);

	bool operator==(const Skin& other) const;
protected:
	std::map<Ogre::String, Ogre::String> replaceTextures;
	std::map<Ogre::String, Ogre::String> replaceMaterials;

	static int counter;

	void loadImpl(void);
	void unloadImpl(void);
	size_t calculateSize(void) const;


};


#endif
