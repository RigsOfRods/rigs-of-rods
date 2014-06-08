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
// created by thomas{AT}thomasfischer{DOT}biz, 5th of July 2010
#ifndef __PreviewRenderer_H_
#define __PreviewRenderer_H_

#include "RoRPrerequisites.h"

class PreviewRenderer : public ZeroedMemoryAllocator
{
public:

	PreviewRenderer();
	~PreviewRenderer();

	void render();

protected:

	Ogre::String fn;

	void render(Ogre::String ext);
	void render3dpreview(Beam *truck, Ogre::Camera *renderCamera, float minCameraRadius, Ogre::SceneNode *camNode);
	void render2dviews(Beam *truck, Ogre::Camera *renderCamera, float minCameraRadius);
};

#endif // __PreviewRenderer_H_
