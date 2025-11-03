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

#ifndef _Hydrax_Image_H_
#define _Hydrax_Image_H_

#include "Prerequisites.h"

#include "Help.h"

/// @addtogroup Gfx
/// @{

/// @addtogroup Hydrax
/// @{

namespace Hydrax
{
	/** Class for store variable channels of an image
	 */
	class Image
	{
	public:
		/** Image type enum
		 */
		enum Type
		{
			TYPE_ONE_CHANNEL  = 1,
			TYPE_TWO_CHANNELS = 2,
			TYPE_RGB          = 3,
			/// Default
			TYPE_RGBA         = 4
		};

		/** Channel enum
		 */
		enum Channel
		{
			CHANNEL_R = 0, // Red
			CHANNEL_G = 1, // Green
			CHANNEL_B = 2, // Blue
			CHANNEL_A = 3  // Alpha
		};

		/** Pixel structure
		 */
		struct Pixel
		{
			/** Default constructor
			 */
			Pixel()
				: red(0)
				, green(0)
				, blue(0)
				, alpha(0)
			{
			}

			/** Constructor
			    @param v RGBA Value
			 */
			Pixel(const float &v)
				: red(v)
				, green(v)
				, blue(v)
				, alpha(v)
			{
			}

			/** Constructor
			    @param r Red value
				@param g Green value
				@param b Blue value
			    @remarks Alpha component = 0
			 */
			Pixel(const float &r,
				  const float &g,
				  const float &b)
				: red(r)
				, green(g)
				, blue(b)
				, alpha(0)
			{
			}

			/** Constructor
			    @param r Red value
				@param g Green value
				@param b Blue value
				@param a Alpha value
			 */
			Pixel(const float &r,
				  const float &g,
				  const float &b,
				  const float &a)
				: red(r)
				, green(g)
				, blue(b)
				, alpha(a)
			{
			}

			/// Pixel values (RGBA)
			float red,
				  green,
				  blue,
				  alpha;
		};

		/** Constructor
		    @param Size Image size
		 */
		Image(const Size &Size);

		/** Constructor
		    @param Size Image size
			@param Type Image type
		 */
		Image(const Size &Size, const Type &Type);

		/** Constructor
		    @param Size Image size
			@param Type Image type
			@param v Initial channel values
		 */
		Image(const Size &Size, const Type &Type, const float &v);

		/** Destructor
		 */
		~Image();

		/** Get a pixel value
		    @param x X value
			@param y Y value
			@param c Channel
			@return Pixel channel value
		 */
		const float& getValue(const int &x, const int &y, const int &c) const;

		/** Get a pixel value with linear interpolation,
		    like x = 4.56, y = 8.34
		    @param x X value
			@param y Y value
			@param c Channel
			@return Pixel channel value
		 */
		float getValueLI(const float &x, const float &y, const int &c) const;

		/** Get a pixel value
		    @param x X value
			@param y Y value
			@param c Channel
			@return Pixel channel value
		 */
		inline const float& getValue(const int &x, const int &y, const Channel &c) const
		{
			return getValue(x, y, static_cast<int>(c));
		}

		/** Get a pixel value with linear interpolation,
		    like x = 4.56, y = 8.34
		    @param x X value
			@param y Y value
			@param c Channel
			@return Pixel channel value
		 */
		inline float getValueLI(const float &x, const float &y, const Channel &c) const
		{
			return getValueLI(x, y, static_cast<int>(c));
		}

		/** Get a pixel
		    @param x X value
			@param y Y value
			@return Pixel
		 */
		Pixel getPixel(const int &x, const int &y) const;

		/** Get a pixel with linear interpolation,
		    like x = 4.56, y = 8.34
		    @param x X value
			@param y Y value
			@return Pixel
		 */
		Pixel getPixelLI(const float &x, const float &y) const;

		/** Set a pixel value
		    @param x X value
			@param y Y value
			@param c Channel
			@param v Value
		 */
		void setValue(const int &x, const int &y, const int &c, const float &v);

		/** Set a pixel value
		    @param x X value
			@param y Y value
			@param c Channel
			@param v Value
		 */
		inline void setValue(const int &x, const int &y, const Channel &c, const float &v)
		{
			setValue(x, y, static_cast<int>(c), v);
		}

		/** Set a pixel
		    @param x X value
			@param y Y value
			@param p Pixel
		 */
		void setPixel(const int &x, const int &y, const Pixel &p);

		/** Get image size
		    @return Image size
		 */
		inline Size getSize() const
		{
			return Size(mSize.Width-1,mSize.Height-1);
		}

		/** Get image type
		    @return Image type
		 */
		inline Type getType() const
		{
			return static_cast<Type>(mChannels);
		}

		/** Get number of channels
		    @return Number of channels
		 */
		inline const int& getNumberOfChannels() const
		{
			return mChannels;
		}

	private:
		/** Initialize array (Reserve dynamic memory)
		    @param v Initial values
		 */
		void _Initialize(const float &v);

		/// Image size
		Size mSize;
		/// Number of channels
		int mChannels;

		/// Our image data
		float *mData;
	};
}

/// @} // addtogroup Hydrax
/// @} // addtogroup Gfx

#endif