/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2013-2017 Petr Ohlidal & contributors

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

#include "ConfigFile.h"

#include <list>
#include <string>
#include <unordered_map>
#include <OgreColourValue.h>
#include <OgreDataStream.h>

namespace RoR
{

struct GroundModelDef
{
    enum class FxSurfaceType
    {
        FX_NONE,
        FX_HARD,     ///< hard surface: rubber burning and sparks
        FX_DUSTY,    ///< dusty surface (with dust colour)
        FX_CLUMPY,   ///< throws clumps (e.g. snow, grass) with colour
        FX_PARTICLE
    };

    GroundModelDef(): // Defaults form old parser
        va(0.f),
        ms(0.f),
        mc(0.f),
        t2(0.f),
        vs(0.f),
        alpha(2.f),
        strength(1.f),
        fluid_density(0.f),
        flow_consistency_index(0.f),
        flow_behavior_index(0.f),
        solid_ground_level(0.f),
        drag_anisotropy(0.f),
        fx_type(FxSurfaceType::FX_NONE),
        fx_particle_amount(0.f),
        fx_particle_min_velo(5.f),
        fx_particle_max_velo(99999.f),
        fx_particle_fade(-1.f),
        fx_particle_timedelta(1.f),
        fx_particle_velo_factor(0.7f),
        fx_particle_ttl(2.f)
    {}

    void CopyFrom(GroundModelDef const& other)
    {
        this->va                      = other.va;
        this->ms                      = other.ms;
        this->mc                      = other.mc;
        this->t2                      = other.t2;
        this->vs                      = other.vs;
        this->alpha                   = other.alpha;
        this->strength                = other.strength;
        this->fluid_density           = other.fluid_density;
        this->flow_consistency_index  = other.flow_consistency_index;
        this->flow_behavior_index     = other.flow_behavior_index;
        this->solid_ground_level      = other.solid_ground_level;
        this->drag_anisotropy         = other.drag_anisotropy;
        this->fx_type                 = other.fx_type;
        this->fx_particle_amount      = other.fx_particle_amount;
        this->fx_particle_min_velo    = other.fx_particle_min_velo;
        this->fx_particle_max_velo    = other.fx_particle_max_velo;
        this->fx_particle_fade        = other.fx_particle_fade;
        this->fx_particle_timedelta   = other.fx_particle_timedelta;
        this->fx_particle_velo_factor = other.fx_particle_velo_factor;
        this->fx_particle_ttl         = other.fx_particle_ttl;
    }

    float va;                       ///< adhesion velocity
    float ms;                       ///< static friction coefficient
    float mc;                       ///< sliding friction coefficient
    float t2;                       ///< hydrodynamic friction (s/m)
    float vs;                       ///< stribeck velocity (m/s)
    float alpha;                    ///< steady-steady
    float strength;                 ///< ground strength

    float fluid_density;            ///< Density of liquid
    float flow_consistency_index;   ///< general drag coefficient

    //! if flow_behavior_index<1 then liquid is Pseudoplastic (ketchup, whipped cream, paint)
    //! if =1 then liquid is Newtoni'an fluid
    //! if >1 then liquid is Dilatant fluid (less common)
    float flow_behavior_index;

    float solid_ground_level;       ///< how deep the solid ground is
    float drag_anisotropy;          ///< Upwards/Downwards drag anisotropy

    FxSurfaceType fx_type;
    Ogre::ColourValue fx_colour;
    std::string name;               ///< Name of this ground-model
    std::string base_name;          ///< Parent ground-model to derive from
    std::string particle_name;

    int fx_particle_amount;         ///< amount of particles

    float fx_particle_min_velo;     ///< minimum velocity to display sparks
    float fx_particle_max_velo;     ///< maximum velocity to display sparks
    float fx_particle_fade;         ///< fade coefficient
    float fx_particle_timedelta;    ///< delta for particle animation
    float fx_particle_velo_factor;  ///< velocity factor
    float fx_particle_ttl;
};

class GroundModelManager
{
public:
    static const int LATEST_GROUND_MODEL_VERSION = 3;

    void             LoadGroundModels(std::string const& filename);
    GroundModelDef*  GetGroundModel(const char* name);
    inline std::unordered_map<std::string, GroundModelDef>& GetAllGroundModels() { return m_models; }

private:
    void             ParseGroundModels();
    void             ResolveDependencies();

    std::unordered_map<std::string, GroundModelDef> m_models;
    std::list<std::string> m_messages;
    RoR::ConfigFile m_conf;
    int m_version;
};

} // namespace RoR