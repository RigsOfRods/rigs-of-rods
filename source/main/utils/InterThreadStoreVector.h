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

#include "RoRPrerequisites.h"

#include <mutex>

/// this class is a helper to exchange data in a class between different threads, it can be pushed and pulled in various threads
template <class T>
class InterThreadStoreVector
{
public:

    void push(T v)
    {
        std::lock_guard<std::mutex> lock(m_vector_mutex);
        store.push_back(v);
    }

    void pull(std::vector<T>& res)
    {
        std::lock_guard<std::mutex> lock(m_vector_mutex);
        res = store;
        store.clear();
    }

protected:

    std::mutex m_vector_mutex;
    std::vector<T> store;
};
