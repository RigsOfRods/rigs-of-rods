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

#include "Application.h"
#include "AeroEngine.h"
#include "RigDef_File.h"
#include "SimData.h"

namespace RoR {

class TurbojetVisual
{
public:
    ~TurbojetVisual();
    void SetupVisuals(RigDef::Turbojet & def, int num, std::string const& propname, Ogre::Entity* nozzle, Ogre::Entity* afterburner_flame);
    void SetNodes(NodeNum_t front, NodeNum_t back, NodeNum_t ref);
    void UpdateVisuals(RoR::GfxActor* gfx_actor);
    void SetVisible(bool visible);
    bool IsVisible() const { return m_visible; }

private:
    Ogre::SceneNode*      m_smoke_scenenode;
    Ogre::ParticleSystem* m_smoke_particle;
    Ogre::Entity*         m_flame_entity;
    Ogre::SceneNode*      m_flame_scenenode;
    Ogre::Entity*         m_nozzle_entity;
    Ogre::SceneNode*      m_nozzle_scenenode;

    bool     m_visible = false; // Needed for flames which are hidden by default.
    int      m_number;
    float    m_radius;
    NodeNum_t m_node_back                      = NODENUM_INVALID;
    NodeNum_t m_node_front                     = NODENUM_INVALID;
    NodeNum_t m_node_ref                       = NODENUM_INVALID;
};

/// @addtogroup Gameplay
/// @{

/// @addtogroup Aerial
/// @{

class Turbojet: public AeroEngine
{

public:

    Turbojet(ActorPtr actor, NodeNum_t tnodefront, NodeNum_t tnodeback, NodeNum_t tnoderef, RigDef::Turbojet & def);
    ~Turbojet();

    void flipStart();
    void reset();
    void setRPM(float _rpm);
    void setThrottle(float val);
    void toggleReverse();
    void setReverse(bool val);
    bool getReverse() { return m_reverse; };
    void updateForces(float dt, int doUpdate);

    Ogre::Vector3 getAxis() { return m_axis; };

    bool getIgnition() { return m_ignition; };
    void setIgnition(bool val) { m_ignition = val; };
    bool getWarmup() { return m_warmup; };
    bool isFailed() { return m_is_failed; };
    float getAfterburner() { return (float)m_afterburner_active; };
    float getAfterburnThrust() const { return m_afterburn_thrust; }
    float getExhaustVelocity() const { return m_exhaust_velocity; }
    float getRPM() { return m_rpm_percent; }; // FIXME - bad func name
    float getRPMpc() { return m_rpm_percent; };
    float getRadius() { return m_radius; };
    float getThrottle();
    float getpropwash() { return m_propwash; };
    int getNoderef() { return m_node_back; };
    AeroEngineType getType() { return AeroEngineType::AE_TURBOJET; };

    // AeroEngine visuals

    void updateVisuals(RoR::GfxActor* gfx_actor) override;
    void setVisible(bool visible) override;

    bool tjet_afterburnable;
    TurbojetVisual tjet_visual;

private:
    Ogre::Vector3 m_axis;
    bool m_afterburner_active;
    bool m_is_failed;
    bool m_ignition;
    bool m_reversable;
    bool m_reverse;
    bool m_warmup;
    float m_afterburn_thrust; //!< in kN
    float m_area;
    float m_exhaust_velocity; //!< in m/s
    float m_last_flip;
    float m_max_dry_thrust; //!< in kN
    float m_propwash;
    float m_radius;
    float m_reflen;
    float m_rpm_percent; //!< in percent!
    float m_throtle;
    float m_timer;
    float m_warmup_start;
    float m_warmup_time;
    int m_sound_ab;
    int m_sound_mod;
    int m_sound_src;
    int m_sound_thr;

    // Attachment
    ActorPtr m_actor;
    NodeNum_t m_node_back;
    NodeNum_t m_node_front;
    NodeNum_t m_node_ref;
};

/// @} // addtogroup Aerial
/// @} // addtogroup Gameplay

} // namespace RoR
