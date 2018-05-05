/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2013-2018 Petr Ohlidal & contributors

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

#include "DustManager.h"

#include "Application.h"
#include "DustPool.h"
#include "Settings.h"

using namespace Ogre;

void RoR::GfxScene::InitScene(Ogre::SceneManager* sm)
{
    m_dustpools["dust"]   = new DustPool(sm, "tracks/Dust",   20);
    m_dustpools["clump"]  = new DustPool(sm, "tracks/Clump",  20);
    m_dustpools["sparks"] = new DustPool(sm, "tracks/Sparks", 10);
    m_dustpools["drip"]   = new DustPool(sm, "tracks/Drip",   50);
    m_dustpools["splash"] = new DustPool(sm, "tracks/Splash", 20);
    m_dustpools["ripple"] = new DustPool(sm, "tracks/Ripple", 20);

    m_ogre_scene = sm;
}

void RoR::GfxScene::DiscardScene()
{
    for (auto itor : m_dustpools)
    {
        itor.second->Discard(m_ogre_scene);
        delete itor.second;
    }
    m_dustpools.clear();
}

void RoR::GfxScene::UpdateScene()
{
    // Particles
    if (RoR::App::gfx_particles_mode.GetActive() == 1)
    {
        for (auto itor : m_dustpools)
        {
            itor.second->update();
        }
    }
}

void RoR::GfxScene::SetParticlesVisible(bool visible)
{
    for (auto itor : m_dustpools)
    {
        itor.second->setVisible(visible);
    }
}

DustPool* RoR::GfxScene::GetDustPool(const char* name)
{
    auto found = m_dustpools.find(name);
    if (found != m_dustpools.end())
    {
        return found->second;
    }
    else
    {
        return nullptr;
    }
}
