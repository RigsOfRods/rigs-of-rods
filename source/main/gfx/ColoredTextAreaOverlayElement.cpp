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

#include "ColoredTextAreaOverlayElement.h"

#include <OgreRoot.h>

#define POS_TEX_BINDING 0
#define COLOUR_BINDING 1

using namespace Ogre;
using namespace std;

ColoredTextAreaOverlayElement::ColoredTextAreaOverlayElement(const String& name)
    : TextAreaOverlayElement(name)
    , m_ValueTop(1.0f)
    , m_ValueBottom(0.8f)
{
}

ColoredTextAreaOverlayElement::~ColoredTextAreaOverlayElement(void)
{
}

void ColoredTextAreaOverlayElement::setValueBottom(float Value)
{
    m_ValueTop = Value;
    mColoursChanged = true;
}

void ColoredTextAreaOverlayElement::setValueTop(float Value)
{
    m_ValueBottom = Value;
    mColoursChanged = true;
}

ColourValue ColoredTextAreaOverlayElement::GetColor(unsigned char ID, float Value)
{
    switch (ID)
    {
    case 0:
        return ColourValue(0, 0, 0); // Black
    case 1:
        return ColourValue(Value, 0, 0); // Red
    case 2:
        return ColourValue(0, Value, 0); // Green
    case 3:
        return ColourValue(Value, Value, 0); // Yellow
    case 4:
        return ColourValue(0, 0, Value); // Blue
    case 5:
        return ColourValue(0, Value, Value); // Cyan
    case 6:
        return ColourValue(Value, 0, Value); // Magenta
    case 7:
        return ColourValue(Value, Value, Value); // White
    case 8:
        return ColourValue(Value * 0.9, Value * 0.9, Value * 0.9); // Gray
    case 9:
        return ColourValue(0.5, 0.5, Value * 0.9); // dark blue
    }
    return ColourValue::Black;
}

Ogre::v1::DisplayString ColoredTextAreaOverlayElement::StripColors(const Ogre::String& otext)
{
    try
    {
        Ogre::v1::DisplayString text(otext.c_str());
        Ogre::v1::DisplayString StrippedText;
        int i;
        for (i = 0; i < (int)text.size() - 1; ++i)
        {
            if (text[i] == '^' &&
                text[i + 1] >= '0' && text[i + 1] <= '9') // This is a color code, ignore it
            {
                ++i;
            }
            else
            {
                StrippedText.append(1, text[i]);
            }
        }
        // One last character to add because loop went to size()-1
        if (i < (int)text.size())
            StrippedText.append(1, text[i]);
        return StrippedText;
    }
    catch (...)
    {
    }
    return String("UTF8 error (String cannot be displayed with current font set)");
}

void ColoredTextAreaOverlayElement::setCaption(const Ogre::v1::DisplayString& text)
{
    m_Colors.clear();
    m_Colors.resize(text.size(), 9);
    bool noColor = true;
    int i, iNumColorCodes = 0, iNumSpaces = 0;
    for (i = 0; i < (int)text.size() - 1; ++i)
    {
        if (text[i] == ' ' || text[i] == '\n')
        {
            // Spaces and newlines are skipped when rendering and as such can't have a color
            ++iNumSpaces;
        }
        else if (text[i] == '^' &&
            text[i + 1] >= '0' && text[i + 1] <= '9') // This is a color code
        {
            // Fill the color array starting from this point to the end with the new color code
            // adjustments need to made because color codes will be removed and spaces are not counted
            fill(m_Colors.begin() + i - (2 * iNumColorCodes) - iNumSpaces, m_Colors.end(), text[i + 1] - '0');
            ++i;
            ++iNumColorCodes;
            mColoursChanged = true;
            noColor = false;
        }
    }
    if (noColor)
        mColoursChanged = true;
    // Set the caption using the base class, but strip the color codes from it first
    TextAreaOverlayElement::setCaption(StripColors(text));
}

void ColoredTextAreaOverlayElement::updateColours(void)
{
    if (!mRenderOp.vertexData)
        return;
    // Convert to system-specific
    RGBA topColour, bottomColour;
    // Set default to white
    Root::getSingleton().convertColourValue(ColourValue::White, &topColour);
    Root::getSingleton().convertColourValue(ColourValue::White, &bottomColour);

    Ogre::v1::HardwareVertexBufferSharedPtr vbuf =
        mRenderOp.vertexData->vertexBufferBinding->getBuffer(COLOUR_BINDING);

    //RGBA* pDest = static_cast<RGBA*>(
    //	vbuf->lock(HardwareBuffer::HBL_NORMAL) );
    RGBA* pDest = (RGBA*)malloc(vbuf->getSizeInBytes());
    RGBA* oDest = pDest;

    for (size_t i = 0; i < mAllocSize; ++i)
    {
        if (i < m_Colors.size())
        {
            Root::getSingleton().convertColourValue(GetColor(m_Colors[i], m_ValueTop), &topColour);
            Root::getSingleton().convertColourValue(GetColor(m_Colors[i], m_ValueBottom), &bottomColour);
        }

        // First tri (top, bottom, top)
        *pDest++ = topColour;
        *pDest++ = bottomColour;
        *pDest++ = topColour;
        // Second tri (top, bottom, bottom)
        *pDest++ = topColour;
        *pDest++ = bottomColour;
        *pDest++ = bottomColour;
    }
    vbuf->writeData(0, vbuf->getSizeInBytes(), oDest, true);
    free(oDest);
    //vbuf->unlock();
}
