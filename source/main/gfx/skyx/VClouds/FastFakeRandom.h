/*
--------------------------------------------------------------------------------
This source file is part of SkyX.
Visit http://www.paradise-studios.net/products/skyx/

Copyright (C) 2009-2012 Xavier Vergu�n Gonz�lez <xavyiy@gmail.com>

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

#ifndef _SkyX_VClouds_FastFakeRandom_H_
#define _SkyX_VClouds_FastFakeRandom_H_

#include "Prerequisites.h"

namespace SkyX { namespace VClouds{

	class FastFakeRandom 
	{
	public:
		/** Constructor 
		    @param n Capacity
			@param min Min value
			@param max Max value
		 */
		FastFakeRandom(const int& n, const Ogre::Real& min, const Ogre::Real& max);

		/** Destructor
	     */
		~FastFakeRandom();

		/** Get random number
		 */
		float& get();

	private:
		/// Data pointer
		float *mData;
		/// Capacity
		int mCapacity;
		/// Index
		int mIndex;

	};

}}

#endif