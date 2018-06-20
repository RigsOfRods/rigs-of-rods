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

#pragma once

#include <vector>
#include <Overlay/OgreTextAreaOverlayElement.h>

#include "RoRPrerequisites.h"

class ColoredTextAreaOverlayElement : public Ogre::TextAreaOverlayElement
{
public:
    ColoredTextAreaOverlayElement(const Ogre::String& name);
    ~ColoredTextAreaOverlayElement(void);

    void setValueBottom(float Value);
    void setValueTop(float Value);
    void setCaption(const Ogre::DisplayString& text);
    static Ogre::DisplayString StripColors(const Ogre::String& text);
    static Ogre::ColourValue GetColor(unsigned char ID, float Value = 1.0f);

    void updateColours(void);

protected:
    std::vector<unsigned char> m_Colors;
    float m_ValueTop;
    float m_ValueBottom;
};
