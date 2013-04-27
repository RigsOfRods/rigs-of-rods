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

// source: ogre wiki: http://www.ogre3d.org/wiki/index.php/HowTo:_Write_text_on_texture

#include "RoRPrerequisites.h"

#include "OgrePrerequisites.h"
#include <OgreImage.h>

void SaveImage(Ogre::TexturePtr TextureToSave, Ogre::String filename);
void WriteToTexture(const Ogre::String &str, Ogre::TexturePtr destTexture, Ogre::Image::Box destRectangle, Ogre::Font* font, const Ogre::ColourValue &color, char justify = 'l',  bool wordwrap = true);
