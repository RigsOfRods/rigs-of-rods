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

// based on the openttd class of the same name
// this helps finding problems when uninitialized member pointers are used

#pragma once

/**
 * Base class that provides memory initialization on dynamically created objects.
 * All allocated memory will be zeroed.
 */
class ZeroedMemoryAllocator
{
  public:
    ZeroedMemoryAllocator()
    {
    }

    virtual ~ZeroedMemoryAllocator()
    {
    }

    /**
     * Memory allocator for a single class instance.
     * @param size the amount of bytes to allocate.
     * @return the given amounts of bytes zeroed.
     */
    inline void *operator new(size_t size)
    {
        return calloc(size, sizeof(unsigned char));
    }

    /**
     * Memory allocator for an array of class instances.
     * @param size the amount of bytes to allocate.
     * @return the given amounts of bytes zeroed.
     */
    inline void *operator new[](size_t size)
    {
        return calloc(size, sizeof(unsigned char));
    }

    /**
     * Memory release for a single class instance.
     * @param ptr  the memory to free.
     */
    inline void operator delete(void *ptr)
    {
        free(ptr);
    }

    /**
     * Memory release for an array of class instances.
     * @param ptr  the memory to free.
     */
    inline void operator delete[](void *ptr)
    {
        free(ptr);
    }
};
