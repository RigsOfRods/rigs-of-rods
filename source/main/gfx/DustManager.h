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

/// @file
/// @author Petr Ohlidal
/// @date   05/2018

#pragma once

#include "ForwardDeclarations.h"
#include "EnvironmentMap.h" // RoR::GfxEnvmap

#include <map>
#include <string>
#include <memory>

namespace RoR {

// Developer note:
//   This was originally `DustManager` (particle manager), which is scene-wide logic updated every frame.
//     -> It was handy to re-purpose it to manage all visuals.
//   I'm temporarily re-using the file because it eases debugging for me (I can switch revisions without re-generating VisualStudio project)
//   ~ only_a_ptr, 05/2018

/// Provides a 3D graphical representation of the simulation
/// Idea: simulation runs at it's own constant rate, scene updates and rendering run asynchronously.
class GfxScene
{
public:

    struct SimBuffer /// Buffered simulation state
    {
        SimBuffer();

        Actor*         simbuf_player_actor;
        Ogre::Vector3  simbuf_character_pos;
        Ogre::Vector3  simbuf_dir_arrow_target;
        bool           simbuf_tyrepressurize_active;
        bool           simbuf_sim_paused;
        float          simbuf_sim_speed;
        float          simbuf_race_time;
        float          simbuf_race_bestlap_time;
        bool           simbuf_race_in_progress;
        bool           simbuf_race_in_progress_prev;
    };

    GfxScene();
    ~GfxScene();

    void           InitScene(Ogre::SceneManager* sm);
    DustPool*      GetDustPool(const char* name);
    void           SetParticlesVisible(bool visible);
    void           UpdateScene(float dt_sec);
    void           RegisterGfxActor(RoR::GfxActor* gfx_actor);
    void           RemoveGfxActor(RoR::GfxActor* gfx_actor);
    void           RegisterGfxCharacter(RoR::GfxCharacter* gfx_character);
    void           RemoveGfxCharacter(RoR::GfxCharacter* gfx_character);
    void           BufferSimulationData(); //!< Run this when simulation is halted
    SimBuffer&     GetSimDataBuffer() { return m_simbuf; }
    void           InitSurveyMap(); //!< Must be called after terrain was loaded
    SurveyMapManager* GetSurveyMap() { return m_survey_map.get(); }
    std::vector<GfxActor*> GetGfxActors() { return m_all_gfx_actors; }

private:

    std::map<std::string, DustPool *> m_dustpools;
    Ogre::SceneManager*               m_ogre_scene;
    std::vector<GfxActor*>            m_all_gfx_actors;
    std::vector<GfxActor*>            m_live_gfx_actors;
    std::vector<GfxCharacter*>        m_all_gfx_characters;
    RoR::GfxEnvmap                    m_envmap;
    SimBuffer                         m_simbuf;
    std::unique_ptr<SurveyMapManager> m_survey_map; //!< Minimap; placed here rather than GUIManager because it's lifetime is tied to terrain.

};

} // namespace RoR
