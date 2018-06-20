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

#include "WriteTextToTexture.h"

#include "Application.h"

#include <Overlay/OgreFont.h>
#include <OgreHardwarePixelBuffer.h>
#include <OgreMaterial.h>
#include <OgreTechnique.h>
#include <OgreTexture.h>
#include <OgreTextureManager.h>
#include <Overlay/OgreFontManager.h>

// source: ogre wiki: http://www.ogre3d.org/tikiwiki/tiki-index.php?page=HowTo%3A+Write+text+on+texture
using namespace Ogre;

void SaveImage(TexturePtr TextureToSave, String filename)
{
    HardwarePixelBufferSharedPtr readbuffer;
    readbuffer = TextureToSave->getBuffer(0, 0);
    readbuffer->lock(HardwareBuffer::HBL_NORMAL);
    const PixelBox& readrefpb = readbuffer->getCurrentLock();
    uchar* readrefdata = static_cast<uchar*>(readrefpb.data);

    Image img;
    img = img.loadDynamicImage(readrefdata, TextureToSave->getWidth(),
        TextureToSave->getHeight(), TextureToSave->getFormat());
    img.save(filename);

    readbuffer->unlock();
}

void WriteToTexture(const String& str, TexturePtr destTexture, Ogre::Box destRectangle, Ogre::Font* Reffont, const ColourValue& color, int fontSize, int fontDPI, char justify, bool wordwrap)
{
    using namespace Ogre;

    if (destTexture->getHeight() < destRectangle.bottom)
        destRectangle.bottom = destTexture->getHeight();
    if (destTexture->getWidth() < destRectangle.right)
        destRectangle.right = destTexture->getWidth();

    // Find our scaled up (or down) font.
    std::string fontname = "WTTFont_" + TOSTRING(fontSize + fontDPI) + "_" + Reffont->getSource();
    FontPtr font = FontManager::getSingleton().getByName(fontname);

    // Font not found, let's created it :)
    if (font.isNull())
    {
        font = FontManager::getSingleton().create(fontname, "General");
        font->setType(FT_TRUETYPE);
        font->setSource(Reffont->getSource());

        font->setTrueTypeSize(fontSize);
        font->setTrueTypeResolution(fontDPI);
        font->load();
        LOG("[WriteToTexture] Created font: " + fontname);
    }

    TexturePtr fontTexture = (TexturePtr)TextureManager::getSingleton().getByName(font->getMaterial()->getTechnique(0)->getPass(0)->getTextureUnitState(0)->getTextureName());

    HardwarePixelBufferSharedPtr fontBuffer = fontTexture->getBuffer();
    HardwarePixelBufferSharedPtr destBuffer = destTexture->getBuffer();

    PixelBox destPb = destBuffer->lock(destRectangle, HardwareBuffer::HBL_NORMAL);

    // The font texture buffer was created write only...so we cannot read it back :o). One solution is to copy the buffer  instead of locking it. (Maybe there is a way to create a font texture which is not write_only ?)

    // create a buffer
    size_t nBuffSize = fontBuffer->getSizeInBytes();
    uint8* buffer = (uint8*)calloc(nBuffSize, sizeof(uint8));

    // create pixel box using the copy of the buffer
    PixelBox fontPb(fontBuffer->getWidth(), fontBuffer->getHeight(), fontBuffer->getDepth(), fontBuffer->getFormat(), buffer);
    fontBuffer->blitToMemory(fontPb);

    uint8* fontData = static_cast<uint8*>(fontPb.data);
    uint8* destData = static_cast<uint8*>(destPb.data);

    const size_t fontPixelSize = PixelUtil::getNumElemBytes(fontPb.format);
    const size_t destPixelSize = PixelUtil::getNumElemBytes(destPb.format);

    const size_t fontRowPitchBytes = fontPb.rowPitch * fontPixelSize;
    const size_t destRowPitchBytes = destPb.rowPitch * destPixelSize;

    Box* GlyphTexCoords;
    GlyphTexCoords = new Box[str.size()];

    Ogre::Font::UVRect glypheTexRect;
    size_t charheight = 0;
    size_t charwidth = 0;

    for (unsigned int i = 0; i < str.size(); i++)
    {
        if ((str[i] != '\t') && (str[i] != '\n') && (str[i] != ' '))
        {
            glypheTexRect = font->getGlyphTexCoords(str[i]);
            GlyphTexCoords[i].left = glypheTexRect.left * fontTexture->getSrcWidth();
            GlyphTexCoords[i].top = glypheTexRect.top * fontTexture->getSrcHeight();
            GlyphTexCoords[i].right = glypheTexRect.right * fontTexture->getSrcWidth();
            GlyphTexCoords[i].bottom = glypheTexRect.bottom * fontTexture->getSrcHeight();

            if (GlyphTexCoords[i].getHeight() > charheight)
                charheight = GlyphTexCoords[i].getHeight();
            if (GlyphTexCoords[i].getWidth() > charwidth)
                charwidth = GlyphTexCoords[i].getWidth();
        }
    }

    size_t cursorX = 0;
    size_t cursorY = 0;
    size_t lineend = destRectangle.getWidth();
    bool carriagreturn = true;
    for (unsigned int strindex = 0; strindex < str.size(); strindex++)
    {
        switch (str[strindex])
        {
        case ' ': cursorX += charwidth;
            break;
        case '\t': cursorX += charwidth * 3;
            break;
        case '\n': cursorY += charheight;
            carriagreturn = true;
            break;
        default:
            {
                //wrapping
                if ((cursorX + GlyphTexCoords[strindex].getWidth() > lineend) && !carriagreturn)
                {
                    cursorY += charheight;
                    carriagreturn = true;
                }

                //justify
                if (carriagreturn)
                {
                    size_t l = strindex;
                    size_t textwidth = 0;
                    size_t wordwidth = 0;

                    while (l < str.size() && str[l] != '\n')
                    {
                        wordwidth = 0;

                        switch (str[l])
                        {
                        case ' ': wordwidth = charwidth;
                            ++l;
                            break;
                        case '\t': wordwidth = charwidth * 3;
                            ++l;
                            break;
                        case '\n': l = str.size();
                        }

                        if (wordwrap)
                            while (l < str.size() && str[l] != ' ' && str[l] != '\t' && str[l] != '\n')
                            {
                                wordwidth += GlyphTexCoords[l].getWidth();
                                ++l;
                            }
                        else
                        {
                            wordwidth += GlyphTexCoords[l].getWidth();
                            l++;
                        }

                        if ((textwidth + wordwidth) <= destRectangle.getWidth())
                            textwidth += (wordwidth);
                        else
                            break;
                    }

                    if ((textwidth == 0) && (wordwidth > destRectangle.getWidth()))
                        textwidth = destRectangle.getWidth();

                    switch (justify)
                    {
                    case 'c': cursorX = (destRectangle.getWidth() - textwidth) / 2;
                        lineend = destRectangle.getWidth() - cursorX;
                        break;

                    case 'r': cursorX = (destRectangle.getWidth() - textwidth);
                        lineend = destRectangle.getWidth();
                        break;

                    default: cursorX = 0;
                        lineend = textwidth;
                        break;
                    }

                    carriagreturn = false;
                }

                //abort - net enough space to draw
                if ((cursorY + charheight) > destRectangle.getHeight())
                    goto stop;

                //draw pixel by pixel
                for (size_t i = 0; i < GlyphTexCoords[strindex].getHeight(); i++)
                    for (size_t j = 0; j < GlyphTexCoords[strindex].getWidth(); j++)
                    {
                        float alpha = color.a * (fontData[(i + GlyphTexCoords[strindex].top) * fontRowPitchBytes + (j + GlyphTexCoords[strindex].left) * fontPixelSize + 1] / 255.0);
                        float invalpha = 1.0 - alpha;
                        size_t offset = (i + cursorY) * destRowPitchBytes + (j + cursorX) * destPixelSize;
                        ColourValue pix;
                        PixelUtil::unpackColour(&pix, destPb.format, &destData[offset]);
                        pix = (pix * invalpha) + (color * alpha);
                        PixelUtil::packColour(pix, destPb.format, &destData[offset]);
                    }

                cursorX += GlyphTexCoords[strindex].getWidth();
            }//default
        }//switch
    }//for

stop:
    delete[] GlyphTexCoords;

    destBuffer->unlock();

    // Free the memory allocated for the buffer
    free(buffer);
    buffer = 0;
}
