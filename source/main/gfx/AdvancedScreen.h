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

// create by thomas fischer, 6th of februray 2011

#pragma once

#include "Application.h"

#include <thread>

#define SET_BIT(var, pos)   ((var) |= (1<<(pos)))
#define CLEAR_BIT(var, pos) ((var) &= ~(1<<(pos)))
#define SET_LSB(var)        ((var) |= 1)
#define CLEAR_LSB(var)      ((var) &= ~1)
#define CHECK_BIT(var,pos)  ((var) & (1<<(pos)))

void save(Ogre::uchar* data, Ogre::uchar* databuf, int mWidth, int mHeight, Ogre::PixelFormat pf, Ogre::String filename)
{
    Ogre::Image img;
    img.loadDynamicImage(data, mWidth, mHeight, 1, pf, false, 1, 0);
    img.save(filename);

    OGRE_FREE(data, Ogre::MEMCATEGORY_RENDERSYS);
    OGRE_FREE(databuf, Ogre::MEMCATEGORY_RENDERSYS);
}

// this only works with lossless image compression (png)

class AdvancedScreen
{
public:

    AdvancedScreen(Ogre::RenderWindow* win, Ogre::String filename) :
        win(win)
        , filename(filename)
    {
    }

    ~AdvancedScreen()
    {
    }

    void addData(Ogre::String name, Ogre::String value)
    {
        if (value.empty())
            return;
        map[name] = value;
    }

    void write()
    {
        // please do not misinterpret this feature
        // its used in the forums like an EXTIF data to display things beside the screenshots
        int mWidth = win->getWidth();
        int mHeight = win->getHeight();

        // grab the image data
        Ogre::PixelFormat pf = Ogre::PF_B8G8R8; //win->suggestPixelFormat();
        long isize = mWidth * mHeight * (long)Ogre::PixelUtil::getNumElemBytes(pf);
        Ogre::uchar* data = OGRE_ALLOC_T(Ogre::uchar, isize, Ogre::MEMCATEGORY_RENDERSYS);
        Ogre::PixelBox pb(mWidth, mHeight, 1, pf, data);
        win->copyContentsToMemory(pb);

        // now do the fancy stuff ;)

        // 0. allocate enough buffer
        Ogre::uchar* databuf = OGRE_ALLOC_T(Ogre::uchar, 32768, Ogre::MEMCATEGORY_RENDERSYS);
        char* ptr = (char *)databuf;
        // header
        long dsize = 0;
        int w = sprintf(ptr, "RORED\n");
        ptr += w;
        dsize += w;

        // 1. convert the std::map to something properly
        for (std::map<Ogre::String, Ogre::String>::iterator it = map.begin(); it != map.end(); it++)
        {
            int w2 = sprintf(ptr, "%s:%s\n", it->first.c_str(), it->second.c_str());
            ptr += w2;
            dsize += w2;
        }

        // now buffer ready, put it into the image
        Ogre::uchar* ptri = data;
        // set data pointer to the start again
        ptr = (char *)databuf;
        int bc = 7;
        for (long b = 0; b < isize && b < dsize * 8 + 40; b++ , ptri++) // 40 = 5 zero bytes
        {
            if (CHECK_BIT(*ptr, bc))
            SET_LSB(*ptri);
            else
            CLEAR_LSB(*ptri);
            bc--;
            if (bc < 0)
            {
                bc = 7;
                ptr++;
            }
        }

        std::thread(save, data, databuf, mWidth, mHeight, pf, filename).detach();
    }

protected:

    Ogre::RenderWindow* win;
    Ogre::String filename;
    std::map<Ogre::String, Ogre::String> map;
};
