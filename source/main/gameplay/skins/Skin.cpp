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

#include "Skin.h"

#include "OgreLogManager.h"
#include "Ogre.h"
#include "CacheSystem.h"

using namespace Ogre;

int Skin::counter = 0;


Skin::Skin(ResourceManager* creator, const String& name, ResourceHandle handle, const String& group, bool isManual, ManualResourceLoader* loader) :
	Ogre::Resource(creator, name, handle, group, isManual, loader)
{
	this->name = name;
	this->thumbnail = "";
	this->description = "";
	this->authorName = "";
	this->authorID = -1;
}

Skin::~Skin()
{
}

void Skin::loadImpl()
{
}

void Skin::unloadImpl()
{
}

size_t Skin::calculateSize() const
{
	return 0;
}

int Skin::addMaterialReplace(Ogre::String from, Ogre::String to)
{
	replaceMaterials[from] = to;
	return 0;
}

int Skin::addTextureReplace(Ogre::String from, Ogre::String to)
{
	replaceTextures[from] = to;
	return 0;
}

int Skin::hasReplacementForMaterial(Ogre::String material)
{
	return (int)replaceMaterials.count(stripMaterialNameUniqueNess(material));
}

int Skin::hasReplacementForTexture(Ogre::String texture)
{
	return (int)replaceTextures.count(texture);
}

Ogre::String Skin::getReplacementForTexture(Ogre::String texture)
{
	String res = replaceTextures[texture];
	if (res.empty()) res = texture;
	return res;
}

Ogre::String Skin::stripMaterialNameUniqueNess(Ogre::String matName)
{
	// MORE MAGIC
	size_t pos = matName.find("_#UNIQUESKINMATERIAL#_");
	if (pos == matName.npos) return matName;
	return matName.substr(0, pos);
}

Ogre::String Skin::getReplacementForMaterial(Ogre::String material)
{
	String res = replaceMaterials[stripMaterialNameUniqueNess(material)];
	if (res.empty()) res = material;
	return res;
}

void Skin::replaceMaterialTextures(Ogre::String materialName)
{
	MaterialPtr mat = MaterialManager::getSingleton().getByName(materialName);
	if (!mat.isNull())
	{
		for (int t = 0; t < mat->getNumTechniques(); t++)
		{
			Technique *tech = mat->getTechnique(0);
			if (!tech) continue;
			for (int p=0; p < tech->getNumPasses(); p++)
			{
				Pass *pass = tech->getPass(p);
				if (!pass) continue;
				for (int tu = 0; tu < pass->getNumTextureUnitStates(); tu++)
				{
					TextureUnitState *tus = pass->getTextureUnitState(tu);
					if (!tus) continue;

					//if (tus->getTextureType() != TEX_TYPE_2D) continue; // only replace 2d images
					// walk the frames, usually there is only one
					for (unsigned int fr=0; fr<tus->getNumFrames(); fr++)
					{
						String textureName = tus->getFrameTextureName(fr);
						std::map<Ogre::String, Ogre::String>::iterator it = replaceTextures.find(textureName);
						if (it != replaceTextures.end())
						{
							textureName = it->second; //getReplacementForTexture(textureName);
							tus->setFrameTextureName(textureName, fr);
						}
					}
				}
			}
		}
	}
}

void Skin::replaceMeshMaterials(Ogre::Entity *e)
{
	if (!e) return;

	// make it unique FIRST, otherwise we change the base material ...
	uniquifyMeshMaterials(e);

	// then walk the entity and look for replacements
	for (int n=0; n<(int)e->getNumSubEntities();n++)
	{
		SubEntity *subent = e->getSubEntity(n);
		String materialName = subent->getMaterialName();
		std::map<Ogre::String, Ogre::String>::iterator it = replaceMaterials.find(stripMaterialNameUniqueNess(materialName));
		if (it != replaceMaterials.end())
		{
			materialName = it->second;
			subent->setMaterialName(materialName);
		} else
		{
			// look for texture replacements
			replaceMaterialTextures(subent->getMaterialName());
		}
	}
}

void Skin::uniquifyMeshMaterials(Ogre::Entity *e)
{
	if (!e) return;

	for (int n=0; n<(int)e->getNumSubEntities();n++)
	{
		SubEntity *subent = e->getSubEntity(n);
		String oldMaterialName = subent->getMaterialName();
		// MAGGIICCCC
		String newMaterialName = oldMaterialName + "_#UNIQUESKINMATERIAL#_" + TOSTRING(counter++);

		MaterialPtr mat = MaterialManager::getSingleton().getByName(oldMaterialName);
		if (!mat.isNull())
		{
			mat->clone(newMaterialName);
			subent->setMaterialName(newMaterialName);
		}
	}
}

bool Skin::operator==(const Skin& other) const
{
	return (this->name == other.name && this->thumbnail == other.thumbnail && this->authorID == other.authorID && this->authorName == other.authorName && this->description == other.description);
}
