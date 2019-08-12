/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2017-2019 Petr Ohlidal & contributors

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

#include "TurboJet.h"

#include <Ogre.h>

#include "Beam.h" // class Actor
#include "BeamData.h"
#include "GfxActor.h"
#include "SoundScriptManager.h"

using namespace Ogre;

Turbojet::Turbojet(Actor* actor, int tnodefront, int tnodeback, int tnoderef, float tmaxdrythrust, bool treversable, float tafterburnthrust, float diskdiam)
{
    m_actor = actor;
#ifdef USE_OPENAL
    switch (actor->ar_num_aeroengines)
    {
    case 0:  m_sound_mod = SS_MOD_AEROENGINE1;  m_sound_src = SS_TRIG_AEROENGINE1;  m_sound_thr = SS_MOD_THROTTLE1;  m_sound_ab = SS_TRIG_AFTERBURNER1;  break;
    case 1:  m_sound_mod = SS_MOD_AEROENGINE2;  m_sound_src = SS_TRIG_AEROENGINE2;  m_sound_thr = SS_MOD_THROTTLE2;  m_sound_ab = SS_TRIG_AFTERBURNER2;  break;
    case 2:  m_sound_mod = SS_MOD_AEROENGINE3;  m_sound_src = SS_TRIG_AEROENGINE3;  m_sound_thr = SS_MOD_THROTTLE3;  m_sound_ab = SS_TRIG_AFTERBURNER3;  break;
    case 3:  m_sound_mod = SS_MOD_AEROENGINE4;  m_sound_src = SS_TRIG_AEROENGINE4;  m_sound_thr = SS_MOD_THROTTLE4;  m_sound_ab = SS_TRIG_AFTERBURNER4;  break;
    case 4:  m_sound_mod = SS_MOD_AEROENGINE5;  m_sound_src = SS_TRIG_AEROENGINE5;  m_sound_thr = SS_MOD_THROTTLE5;  m_sound_ab = SS_TRIG_AFTERBURNER5;  break;
    case 5:  m_sound_mod = SS_MOD_AEROENGINE6;  m_sound_src = SS_TRIG_AEROENGINE6;  m_sound_thr = SS_MOD_THROTTLE6;  m_sound_ab = SS_TRIG_AFTERBURNER6;  break;
    case 6:  m_sound_mod = SS_MOD_AEROENGINE7;  m_sound_src = SS_TRIG_AEROENGINE7;  m_sound_thr = SS_MOD_THROTTLE7;  m_sound_ab = SS_TRIG_AFTERBURNER7;  break;
    case 7:  m_sound_mod = SS_MOD_AEROENGINE8;  m_sound_src = SS_TRIG_AEROENGINE8;  m_sound_thr = SS_MOD_THROTTLE8;  m_sound_ab = SS_TRIG_AFTERBURNER8;  break;
    default: m_sound_mod = SS_MOD_NONE;         m_sound_src = SS_TRIG_NONE;         m_sound_thr = SS_MOD_NONE;       m_sound_ab = SS_TRIG_NONE;
    }
#endif
    m_node_back = tnodeback;
    m_node_front = tnodefront;
    m_node_ref = tnoderef;
    afterburnable = (tafterburnthrust > 0.f);
    m_reversable = treversable;
    m_max_dry_thrust = tmaxdrythrust;
    m_afterburn_thrust = tafterburnthrust;
    m_afterburner_active = false;
    m_timer = 0;
    m_warmup_time = 15.0;
    m_last_flip = 0;
    m_area = 2 * 3.14159 * m_radius * 0.6 * m_radius * 0.6;
    m_exhaust_velocity = 0;
    m_axis = m_actor->ar_nodes[m_node_front].RelPosition - m_actor->ar_nodes[m_node_back].RelPosition;
    m_reflen = m_axis.length();
    m_axis = m_axis / m_reflen;
    reset();
}

void Turbojet::SetupVisuals(std::string const& propname, Ogre::Entity* nozzle, float nozdiam, float nozlength, Ogre::Entity* afterburner_flame, bool disable_smoke)
{
    m_radius = nozdiam / 2.0;

    m_nozzle_entity = nozzle;
    m_nozzle_scenenode = gEnv->sceneManager->getRootSceneNode()->createChildSceneNode();
    m_nozzle_scenenode->attachObject(m_nozzle_entity);
    m_nozzle_scenenode->setScale(nozlength, nozdiam, nozdiam);

    if (afterburner_flame != nullptr)
    {
        m_flame_entity = afterburner_flame;
        m_flame_scenenode = gEnv->sceneManager->getRootSceneNode()->createChildSceneNode();
        m_flame_scenenode->attachObject(m_flame_entity);
        m_flame_scenenode->setScale(1.0, nozdiam, nozdiam);
        m_flame_scenenode->setVisible(false);
    }
    //smoke visual
    if (disable_smoke)
    {
        m_smoke_scenenode = 0;
        m_smoke_particle = 0;
    }
    else
    {
        m_smoke_scenenode = gEnv->sceneManager->getRootSceneNode()->createChildSceneNode();
        m_smoke_particle = gEnv->sceneManager->createParticleSystem("SmokeParticle-"+propname, "tracks/TurbopropSmoke");
        if (m_smoke_particle)
        {
            m_smoke_particle->setVisibilityFlags(DEPTHMAP_DISABLED); // disable particles in depthmap
            m_smoke_scenenode->attachObject(m_smoke_particle);
            m_smoke_particle->setCastShadows(false);
        }
    }
}

Turbojet::~Turbojet()
{
    //A fast work around 
    //
    SOUND_MODULATE(m_actor, m_sound_thr, 0);
    SOUND_MODULATE(m_actor, m_sound_mod, 0);
    SOUND_STOP(m_actor, m_sound_ab);
    SOUND_STOP(m_actor, m_sound_src);

    if (m_flame_entity != nullptr)
    {
        m_flame_entity->setVisible(false);
    }

    if (m_nozzle_entity != nullptr)
    {
        m_nozzle_entity->setVisible(false);
    }

    if (m_smoke_particle != nullptr)
    {
        m_smoke_particle->removeAllEmitters();
    }
}

void Turbojet::updateVisuals(RoR::GfxActor* gfx_actor)
{
    RoR::GfxActor::NodeData* node_buf = gfx_actor->GetSimNodeBuffer();

    //nozzle
    m_nozzle_scenenode->setPosition(node_buf[m_node_back].AbsPosition);
    //build a local system
    Vector3 laxis = node_buf[m_node_front].AbsPosition - node_buf[m_node_back].AbsPosition;
    laxis.normalise();
    Vector3 paxis = Plane(laxis, 0).projectVector(node_buf[m_node_ref].AbsPosition - node_buf[m_node_back].AbsPosition);
    paxis.normalise();
    Vector3 taxis = laxis.crossProduct(paxis);
    Quaternion dir = Quaternion(laxis, paxis, taxis);
    m_nozzle_scenenode->setOrientation(dir);
    //m_afterburner_active
    if (m_afterburner_active)
    {
        m_flame_scenenode->setVisible(true);
        float flamelength = (m_afterburn_thrust / 15.0) * (m_rpm_percent / 100.0);
        flamelength = flamelength * (1.0 + (((Real)rand() / (Real)RAND_MAX) - 0.5) / 10.0);
        m_flame_scenenode->setScale(flamelength, m_radius * 2.0, m_radius * 2.0);
        m_flame_scenenode->setPosition(node_buf[m_node_back].AbsPosition + dir * Vector3(-0.2, 0.0, 0.0));
        m_flame_scenenode->setOrientation(dir);
    }
    else
        m_flame_scenenode->setVisible(false);
    //smoke
    if (m_smoke_scenenode)
    {
        m_smoke_scenenode->setPosition(node_buf[m_node_back].AbsPosition);
        ParticleEmitter* emit = m_smoke_particle->getEmitter(0);
        emit->setDirection(-m_axis);
        emit->setParticleVelocity(m_exhaust_velocity);
        if (!m_is_failed)
        {
            if (m_ignition)
            {
                emit->setEnabled(true);
                emit->setColour(ColourValue(0.0, 0.0, 0.0, 0.02 + m_throtle * 0.03));
                emit->setTimeToLive((0.02 + m_throtle * 0.03) / 0.1);
            }
            else
            {
                emit->setEnabled(false);
            }
        }
        else
        {
            emit->setDirection(Vector3(0, 1, 0));
            emit->setParticleVelocity(7.0);
            emit->setEnabled(true);
            emit->setColour(ColourValue(0.0, 0.0, 0.0, 0.1));
            emit->setTimeToLive(0.1 / 0.1);           
        }
    }
}

void Turbojet::updateForces(float dt, int doUpdate)
{
    if (doUpdate)
    {
        SOUND_MODULATE(m_actor, m_sound_mod, m_rpm_percent);
    }
    m_timer += dt;
    m_axis = m_actor->ar_nodes[m_node_front].RelPosition - m_actor->ar_nodes[m_node_back].RelPosition;
    float axlen = m_axis.length();
    m_axis = m_axis / axlen; //normalize
    if (fabs(m_reflen - axlen) > 0.1)
    {
        m_rpm_percent = 0;
        m_is_failed = true;
    }; //check for broken

    float warmupfactor = 1.0;
    if (m_warmup)
    {
        warmupfactor = (m_timer - m_warmup_start) / m_warmup_time;
        if (warmupfactor >= 1.0)
            m_warmup = false;
    }

    //turbine RPM
    //braking (compression)
    if (m_rpm_percent < 0)
        m_rpm_percent = 0;
    float torque = -m_rpm_percent / 100.0;
    //powering with limiter
    if (m_rpm_percent < 100.0 && !m_is_failed && m_ignition)
        torque += ((0.2 + m_throtle * 0.8) * warmupfactor);
    //integration
    m_rpm_percent += (double)dt * torque * 30.0;

    float enginethrust = 0;
    if (!m_is_failed && m_ignition)
    {
        enginethrust = m_max_dry_thrust * m_rpm_percent / 100.0;
        m_afterburner_active = (afterburnable && m_throtle > 0.95 && m_rpm_percent > 80);
        if (m_afterburner_active)
            enginethrust += (m_afterburn_thrust - m_max_dry_thrust);
    }
    else
        m_afterburner_active = false;

    if (m_afterburner_active)
        SOUND_START(m_actor, m_sound_ab);
    else
        SOUND_STOP(m_actor, m_sound_ab);

    m_actor->ar_nodes[m_node_back].Forces += (enginethrust * 1000.0) * m_axis;
    m_exhaust_velocity = enginethrust * 5.6 / m_area;
}

void Turbojet::setThrottle(float val)
{
    if (val > 1.0)
        val = 1.0;

    if (val < 0.0)
        val = 0.0;

    m_throtle = val;
    SOUND_MODULATE(m_actor, m_sound_thr, val);
}

float Turbojet::getThrottle()
{
    return m_throtle;
}

void Turbojet::setRPM(float _rpm)
{
    m_rpm_percent = _rpm;
}

void Turbojet::reset()
{
    m_rpm_percent = 0;
    m_throtle = 0;
    m_propwash = 0;
    m_is_failed = false;
    m_ignition = false;
    m_reverse = false;
}

void Turbojet::toggleReverse()
{
    if (!m_reversable)
        return;
    m_throtle = 0;
    m_reverse = !m_reverse;
}

void Turbojet::setReverse(bool val)
{
    m_reverse = val;
}

void Turbojet::flipStart()
{
    if (m_timer - m_last_flip < 0.3)
        return;
    m_ignition = !m_ignition;
    if (m_ignition && !m_is_failed)
    {
        m_warmup = true;
        m_warmup_start = m_timer;
        SOUND_START(m_actor, m_sound_src);
    }
    else
    {
        SOUND_STOP(m_actor, m_sound_src);
    }

    m_last_flip = m_timer;
}
