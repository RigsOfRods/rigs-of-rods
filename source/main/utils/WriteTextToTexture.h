/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer

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

/// @file WriteTextToTexture.h
/// @author Copied from the ogre wiki: http://www.ogre3d.org/tikiwiki/tiki-index.php?page=HowTo%3A+Write+text+on+texture

#pragma once

#include <OgreImage.h>
#include <OgrePrerequisites.h>
#include <Overlay/OgreFont.h>

#include "RoRPrerequisites.h"

/**
 *  @brief Saves a texture to the disk.
 *  
 *  @param TextureToSave The texture to save.
 *  @param filename The file-name.
 */
void SaveImage(Ogre::TexturePtr TextureToSave, Ogre::String filename);

/**
 *  @brief Writes a string onto a texture.
 *  
 *  @param str The string to write onto the texture.
 *  @param destTexture The texture to write the string on.
 *  @param destRectangle The Area to write in.
 *  @param font A pointer to the font to use.
 *  @param color The color of the text.
 *  @param fontSize The size of the font in points.
 *  @param fontDPI  The resolution of the font in dpi.
 *  @param justify 'l' = left aligned, 'c' = centered, 'r' = right aligned.
 *  @param wordwrap if true the line will only be wrapped after a word.
 */
void WriteToTexture(const Ogre::String& str, Ogre::TexturePtr destTexture, Ogre::Box destRectangle, Ogre::Font* font, const Ogre::ColourValue& color, int fontSize = 15, int fontDPI = 400, char justify = 'l', bool wordwrap = true);
