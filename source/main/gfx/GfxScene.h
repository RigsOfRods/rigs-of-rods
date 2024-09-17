/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2013-2020 Petr Ohlidal

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
/// @author Petr Ohlidal
/// @date   05/2018

#pragma once

#include "CameraManager.h"
#include "ForwardDeclarations.h"
#include "EnvironmentMap.h" // RoR::GfxEnvmap
#include "SimBuffers.h"
#include "Skidmark.h"

#include <map>
#include <string>
#include <memory>

namespace RoR {

/// @addtogroup Gfx
/// @{

/// Provides a 3D graphical representation of the simulation
/// Idea: simulation runs at it's own constant rate, scene updates and rendering run asynchronously.
class GfxScene
{
public:

    void           Init();

    // Particles:
    void           CreateDustPools();
    DustPool*      GetDustPool(const char* name);
    void           AdjustParticleSystemTimeFactor(Ogre::ParticleSystem* psys);

    void           SetParticlesVisible(bool visible);
    void           DrawNetLabel(Ogre::Vector3 pos, float cam_dist, std::string const& nick, int colornum);
    void           UpdateScene(float dt);
    void           ClearScene();
    void           RegisterGfxActor(RoR::GfxActor* gfx_actor);
    void           RemoveGfxActor(RoR::GfxActor* gfx_actor);
    void           RegisterGfxCharacter(RoR::GfxCharacter* gfx_character);
    void           RemoveGfxCharacter(RoR::GfxCharacter* gfx_character);
    void           BufferSimulationData(); //!< Run this when simulation is halted
    GameContextSB&     GetSimDataBuffer() { return m_simbuf; }
    GfxEnvmap&     GetEnvMap() { return m_envmap; }
    RoR::SkidmarkConfig* GetSkidmarkConf () { return &m_skidmark_conf; }
    Ogre::SceneManager* GetSceneManager() { return m_scene_manager; }
    std::vector<GfxActor*>& GetGfxActors() { return m_all_gfx_actors; }
    std::vector<GfxCharacter*>& GetGfxCharacters() { return m_all_gfx_characters; }

private:

    std::map<std::string, DustPool *> m_dustpools;
    Ogre::SceneManager*               m_scene_manager = nullptr;
    std::vector<GfxActor*>            m_all_gfx_actors;
    std::vector<GfxActor*>            m_live_gfx_actors;
    std::vector<GfxCharacter*>        m_all_gfx_characters;
    RoR::GfxEnvmap                    m_envmap;
    GameContextSB                     m_simbuf;
    SkidmarkConfig                    m_skidmark_conf;
};

/// @} // addtogroup Gfx

} // namespace RoR
