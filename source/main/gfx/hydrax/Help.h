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

#ifndef _Hydrax_Help_H_
#define _Hydrax_Help_H_

#include "Prerequisites.h"

/// @addtogroup Gfx
/// @{

/// @addtogroup Hydrax
/// @{

namespace Hydrax
{
    /** Struct wich contains an especific width and height value
    */
    struct Size
    {
        /// Width value
        int Width;
        /// Height value
        int Height;

        /** Default constructor
        */
        Size()
        {
            Width  = 0;
            Height = 0;
        }

        /** Constructor
        	@param size The width and height values
        */
        Size(const int &size)
        {
            Width  = size;
            Height = size;
        }

        /** Constructor
        	@param width Width value
        	@param height Height value
        */
        Size(const int &width, const int &height)
        {
            Width  = width;
            Height = height;
        }

        /** Destructor
        */
        ~Size()
        {
        }

        /** Sets the same width and height value
        	@param size The width and height values
        */
        void setSize(const int &size)
        {
            Width  = size;
            Height = size;
        }

        /** Sets the especified values
        	@param width Width value
        	@param height Height value
        */
        void setSize(const int &width, const int &height)
        {
            Width  = width;
            Height = height;
        }
    };

	/** Math class with some help funtions
	 */
	class Math
	{
	public:
		/** Constructor
		 */
		Math(){};
		/** Destructor
		 */
		~Math(){};

		/** Find the intersection point of two lines
		    @param a First line origin
			@param b First line final
			@param c First line origin
			@param d First line final
			@return Ogre::Vector2::ZERO if there isn't intersection, intersection point
		 */
		static Ogre::Vector2 intersectionOfTwoLines(const Ogre::Vector2 &a, const Ogre::Vector2 &b,
			                                        const Ogre::Vector2 &c, const Ogre::Vector2 &d);
	};
}

/// @} // addtogroup Hydrax
/// @} // addtogroup Gfx

#endif
