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
/// @author Thomas Fischer (thomas{AT}thomasfischer{DOT}biz)
/// @date   12th of October 2009

#include "DustManager.h"

#include "Application.h"
#include "DustPool.h"
#include "Settings.h"

using namespace Ogre;

void DustManager::DustManCheckAndInit(Ogre::SceneManager* sm)
{
    if (m_is_initialised)
    {
        return;
    }
    mEnabled = RoR::App::gfx_particles_mode.GetActive() == 1;

    if (mEnabled)
    {
        dustpools["dust"]   = new DustPool(sm, "tracks/Dust",   20);
        dustpools["clump"]  = new DustPool(sm, "tracks/Clump",  20);
        dustpools["sparks"] = new DustPool(sm, "tracks/Sparks", 10);
        dustpools["drip"]   = new DustPool(sm, "tracks/Drip",   50);
        dustpools["splash"] = new DustPool(sm, "tracks/Splash", 20);
        dustpools["ripple"] = new DustPool(sm, "tracks/Ripple", 20);
    }
    m_is_initialised = true;
}

void DustManager::DustManDiscard(Ogre::SceneManager* sm)
{
    // delete all created dustpools and remove them
    std::map<Ogre::String, DustPool *>::iterator it;
    for (it = dustpools.begin(); it != dustpools.end(); it++)
    {
        // delete the DustPool instance
		it->second->Discard(sm);
        delete(it->second);
        it->second = 0;
    }
    // then clear the vector
    dustpools.clear();
}

DustPool* DustManager::getGroundModelDustPool(ground_model_t* g)
{
    return 0;
}

void DustManager::update()
{
    if (!mEnabled)
        return;
    std::map<Ogre::String, DustPool *>::iterator it;
    for (it = dustpools.begin(); it != dustpools.end(); it++)
    {
        it->second->update();
    }
}

void DustManager::setVisible(bool visible)
{
    if (!mEnabled)
        return;
    std::map<Ogre::String, DustPool *>::iterator it;
    for (it = dustpools.begin(); it != dustpools.end(); it++)
    {
        it->second->setVisible(visible);
    }
}

DustPool* DustManager::getDustPool(Ogre::String name)
{
    if (dustpools.find(name) == dustpools.end())
        return 0;
    return dustpools[name];
}
