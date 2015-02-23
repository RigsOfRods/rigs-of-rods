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
#ifndef GLOWMATERIALLISTENER_H__
#define GLOWMATERIALLISTENER_H__

#include "RoRPrerequisites.h"
#include <Ogre.h>
#include <OgreMaterialManager.h>

class GlowMaterialListener : public Ogre::MaterialManager::Listener, public ZeroedMemoryAllocator
{
protected:
	Ogre::MaterialPtr mBlackMat;
public:
	GlowMaterialListener()
	{
		mBlackMat = Ogre::MaterialManager::getSingleton().create("mGlowBlack", "Internal");
		mBlackMat->getTechnique(0)->getPass(0)->setDiffuse(0,0,0,0);
		mBlackMat->getTechnique(0)->getPass(0)->setSpecular(0,0,0,0);
		mBlackMat->getTechnique(0)->getPass(0)->setAmbient(0,0,0);
		mBlackMat->getTechnique(0)->getPass(0)->setSelfIllumination(0,0,0);
	}

	Ogre::Technique *handleSchemeNotFound(unsigned short, const Ogre::String& schemeName, Ogre::Material*mat, unsigned short, const Ogre::Renderable*)
	{
		if (schemeName == "glow")
		{
			//LOG(">> adding glow to material: "+mat->getName());
			return mBlackMat->getTechnique(0);
		}
		return NULL;
	}
};

#endif //GLOWMATERIALLISTENER_H__