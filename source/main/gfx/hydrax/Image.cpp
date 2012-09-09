/*
--------------------------------------------------------------------------------
This source file is part of Hydrax.
Visit ---

Copyright (C) 2008 Xavier Verguín González <xavierverguin@hotmail.com>
                                           <xavyiy@gmail.com>

This program is free software; you can redistribute it and/or modify it under
the terms of the GNU Lesser General Public License as published by the Free Software
Foundation; either version 2 of the License, or (at your option) any later
version.

This program is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License along with
this program; if not, write to the Free Software Foundation, Inc., 59 Temple
Place - Suite 330, Boston, MA 02111-1307, USA, or go to
http://www.gnu.org/copyleft/lesser.txt.
--------------------------------------------------------------------------------
*/

#include <Image.h>

namespace Hydrax
{
	Image::Image(const Size &Size)
		: mSize(Hydrax::Size(Size.Width+1,Size.Height+1))
		, mChannels(static_cast<int>(TYPE_RGBA))
		, mData(0)
	{
		_Initialize(0);
	}

	Image::Image(const Size &Size, const Type &Type)
		: mSize(Hydrax::Size(Size.Width+1,Size.Height+1))
		, mChannels(static_cast<int>(Type))
		, mData(0)
	{
		_Initialize(0);
	}

	Image::Image(const Size &Size, const Type &Type, const float &v)
		: mSize(Hydrax::Size(Size.Width+1,Size.Height+1))
		, mChannels(static_cast<int>(Type))
		, mData(0)
	{
		_Initialize(v);
	}

	Image::~Image()
	{
		delete mData;
	}

	const float& Image::getValue(const int &x, const int &y, const int &c) const
	{
#if HYDRAX_IMAGE_CHECK_PIXELS == 1
		if (c < 0 || c > mChannels ||
			x < 0 || x > mSize.Width ||
			y < 0 || y > mSize.Height)
		{
			HydraxLOG("Error in Image::getValue, x = " + Ogre::StringConverter::toString(x) 
				                             + " y = " + Ogre::StringConverter::toString(y) 
											 + " Channel = " + Ogre::StringConverter::toString(c));

			return 0;
		}
#endif

		return mData[(y*mSize.Width+x)*mChannels+c];
	}

	float Image::getValueLI(const float &x, const float &y, const int &c) const
	{
#if HYDRAX_IMAGE_CHECK_PIXELS == 1
		if (c < 0 || c > mChannels ||
			x < 0 || x > mSize.Width ||
			y < 0 || y > mSize.Height)
		{
			HydraxLOG("Error in Image::getValue, x = " + Ogre::StringConverter::toString(x) 
				                             + " y = " + Ogre::StringConverter::toString(y) 
											 + " Channel = " + Ogre::StringConverter::toString(c));

			return 0;
		}
#endif

		int xINT = static_cast<int>(x),
		    yINT = static_cast<int>(y);

		float xDIFF = x-xINT,
			  yDIFF = y-yINT,
			  _xDIFF = 1-xDIFF,
			  _yDIFF = 1-yDIFF;

		//   A      B
		//
		//
		//   C      D
		float A = mData[(yINT*mSize.Width+xINT)*mChannels+c],
			  B = mData[(yINT*mSize.Width+xINT+1)*mChannels+c],
			  C = mData[((yINT+1)*mSize.Width+xINT)*mChannels+c],
			  D = mData[((yINT+1)*mSize.Width+xINT+1)*mChannels+c];

		return A*_xDIFF*_yDIFF +
			   B* xDIFF*_yDIFF +
			   C*_xDIFF* yDIFF +
			   D* xDIFF* yDIFF;
	}

    Image::Pixel Image::getPixel(const int &x, const int &y) const
	{
#if HYDRAX_IMAGE_CHECK_PIXELS == 1
		if (x < 0 || x > mSize.Width ||
			y < 0 || y > mSize.Height)
		{
			HydraxLOG("Error in Image::getPixel, x = " + Ogre::StringConverter::toString(x) 
				                             + " y = " + Ogre::StringConverter::toString(y));

			return Pixel(0);
		}
#endif

		float v[4];

		for(int k = 0; k < 4; k++)
		{
			if (mChannels >= k)
			{
			    v[k] = getValue(x, y, k);
			}
			else
			{
				v[k] = 0;
			}
		}

		return Pixel(v[0], v[1], v[2], v[3]);
	}

	Image::Pixel Image::getPixelLI(const float &x, const float &y) const
	{
#if HYDRAX_IMAGE_CHECK_PIXELS == 1
		if (x < 0 || x > mSize.Width ||
			y < 0 || y > mSize.Height)
		{
			HydraxLOG("Error in Image::getPixel, x = " + Ogre::StringConverter::toString(x) 
				                             + " y = " + Ogre::StringConverter::toString(y));

			return Pixel(0);
		}
#endif

		float v[4];

		for(int k = 0; k < 4; k++)
		{
			if (mChannels >= k)
			{
			    v[k] = getValueLI(x, y, k);
			}
			else
			{
				v[k] = 0;
			}
		}

		return Pixel(v[0], v[1], v[2], v[3]);
	}

	void Image::setValue(const int &x, const int &y, const int &c, const float &v)
	{
#if HYDRAX_IMAGE_CHECK_PIXELS == 1
		if (c < 0 || c > mChannels   ||
			x < 0 || x > mSize.Width ||
			y < 0 || y > mSize.Height)
		{
			HydraxLOG("Error in Image::setValue, x = " + Ogre::StringConverter::toString(x) 
				                             + " y = " + Ogre::StringConverter::toString(y) 
											 + " Channel = " + Ogre::StringConverter::toString(c));

			return;
		}
#endif

		mData[(y*mSize.Width+x)*mChannels+c] = v;
	}

	void Image::setPixel(const int &x, const int &y, const Pixel &p)
	{
#if HYDRAX_IMAGE_CHECK_PIXELS == 1
		if (x < 0 || x > mSize.Width ||
			y < 0 || y > mSize.Height)
		{
			HydraxLOG("Error in Image::setPixel, x = " + Ogre::StringConverter::toString(x) 
				                             + " y = " + Ogre::StringConverter::toString(y));

			return;
		}
#endif

		switch(mChannels)
		{
		    case 1: 
			{
				setValue(x, y, 0, p.red);
			}
			break;

			case 2:
			{
				setValue(x, y, 0, p.red);
				setValue(x, y, 1, p.green);
			}
			break;

			case 3:
			{
				setValue(x, y, 0, p.red);
				setValue(x, y, 1, p.green);
				setValue(x, y, 2, p.blue);
			}
			break;

			case 4:
			{
				setValue(x, y, 0, p.red);
				setValue(x, y, 1, p.green);
				setValue(x, y, 2, p.blue);
				setValue(x, y, 3, p.alpha);
			}
			break;
		}
	}

	void Image::_Initialize(const float &v)
	{
		mData = new float[(mSize.Width)  * 
			              (mSize.Height) * 
						  mChannels];

		int x,y,c;

		for (x = 0; x < mSize.Width; x++)
		{
			for (y = 0; y < mSize.Height; y++)
			{
				for(c = 0; c < mChannels; c++)
				{
					mData[(y*mSize.Width+x)*mChannels+c] = v;
				}
			}
		}
	}
}
