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

#ifndef _Hydrax_Enums_H_
#define _Hydrax_Enums_H_

#include "Prerequisites.h"

/// @addtogroup Gfx
/// @{

/// @addtogroup Hydrax
/// @{

namespace Hydrax
{
    /** Texture quality enumeration(2^n)
     */
    enum TextureQuality
    {
        TEX_QUA_2     = 2,
        TEX_QUA_4     = 4,
        TEX_QUA_8     = 8,
        TEX_QUA_16    = 16,
        TEX_QUA_32    = 32,
        TEX_QUA_64    = 64,
        TEX_QUA_128   = 128,
        TEX_QUA_256   = 256,
        TEX_QUA_512   = 512,
        TEX_QUA_1024  = 1024
    };

    /** Hydrax flags to select components wich we want to use.
        0 for none, 1 for all.
     */
    enum HydraxComponent
    {
        HYDRAX_COMPONENT_SUN        = 1 << 0,
        HYDRAX_COMPONENT_FOAM       = 1 << 1,
        HYDRAX_COMPONENT_DEPTH      = 1 << 2,
        /// Smooth transitions and caustics components need depth component
        HYDRAX_COMPONENT_SMOOTH     = 1 << 3,
        HYDRAX_COMPONENT_CAUSTICS   = 1 << 4,
		HYDRAX_COMPONENT_UNDERWATER = 1 << 5,
		/// Underwater reflections and god rays need underwater component
		HYDRAX_COMPONENT_UNDERWATER_REFLECTIONS = 1 << 6,
		HYDRAX_COMPONENT_UNDERWATER_GODRAYS = 1 << 7,

        HYDRAX_COMPONENTS_NONE    = 0x0000,
        HYDRAX_COMPONENTS_ALL     = 0x001F,
    };
}

/// @} // addtogroup Hydrax
/// @} // addtogroup Gfx

#endif
