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

/// @file
/// @author Thomas Fischer
/// @date   12th of October 2009

#pragma once

#include "RoRPrerequisites.h"

#include "Singleton.h"

class DustManager : public ZeroedMemoryAllocator
{
public:

    DustManager():
        mEnabled(false),
        m_is_initialised(false)
    {
    }

    void      DustManCheckAndInit    (Ogre::SceneManager* sm);
    void      DustManDiscard         (Ogre::SceneManager* sm);

    DustPool* getGroundModelDustPool(ground_model_t* g);

    void update();

    void setVisible(bool visible);

    DustPool* getDustPool(Ogre::String name);

protected:

    bool mEnabled;
    bool m_is_initialised;
    std::map<Ogre::String, DustPool *> dustpools;
};
