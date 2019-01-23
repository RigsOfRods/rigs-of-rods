/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2017      Petr Ohlidal & contributors

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

#include "BeamData.h"
#include "GfxActor.h"
#include "SoundScriptManager.h"

using namespace Ogre;

Turbojet::Turbojet(int tnumber, int trucknum, node_t* nd, int tnodefront, int tnodeback, int tnoderef, float tmaxdrythrust, bool treversable, float tafterburnthrust, float diskdiam)
{
    nodes = nd;
    number = tnumber;
    this->trucknum = trucknum;
#ifdef USE_OPENAL
    switch (number)
    {
    case 0:  mod_id = SS_MOD_AEROENGINE1;  src_id = SS_TRIG_AEROENGINE1;  thr_id = SS_MOD_THROTTLE1;  ab_id = SS_TRIG_AFTERBURNER1;  break;
    case 1:  mod_id = SS_MOD_AEROENGINE2;  src_id = SS_TRIG_AEROENGINE2;  thr_id = SS_MOD_THROTTLE2;  ab_id = SS_TRIG_AFTERBURNER2;  break;
    case 2:  mod_id = SS_MOD_AEROENGINE3;  src_id = SS_TRIG_AEROENGINE3;  thr_id = SS_MOD_THROTTLE3;  ab_id = SS_TRIG_AFTERBURNER3;  break;
    case 3:  mod_id = SS_MOD_AEROENGINE4;  src_id = SS_TRIG_AEROENGINE4;  thr_id = SS_MOD_THROTTLE4;  ab_id = SS_TRIG_AFTERBURNER4;  break;
    case 4:  mod_id = SS_MOD_AEROENGINE5;  src_id = SS_TRIG_AEROENGINE5;  thr_id = SS_MOD_THROTTLE5;  ab_id = SS_TRIG_AFTERBURNER5;  break;
    case 5:  mod_id = SS_MOD_AEROENGINE6;  src_id = SS_TRIG_AEROENGINE6;  thr_id = SS_MOD_THROTTLE6;  ab_id = SS_TRIG_AFTERBURNER6;  break;
    case 6:  mod_id = SS_MOD_AEROENGINE7;  src_id = SS_TRIG_AEROENGINE7;  thr_id = SS_MOD_THROTTLE7;  ab_id = SS_TRIG_AFTERBURNER7;  break;
    case 7:  mod_id = SS_MOD_AEROENGINE8;  src_id = SS_TRIG_AEROENGINE8;  thr_id = SS_MOD_THROTTLE8;  ab_id = SS_TRIG_AFTERBURNER8;  break;
    default: mod_id = SS_MOD_NONE;         src_id = SS_TRIG_NONE;         thr_id = SS_MOD_NONE;       ab_id = SS_TRIG_NONE;
    }
#endif
    nodeback = tnodeback;
    nodefront = tnodefront;
    noderef = tnoderef;
    afterburnable = (tafterburnthrust > 0.f);
    reversable = treversable;
    maxdrythrust = tmaxdrythrust;
    afterburnthrust = tafterburnthrust;
    afterburner = false;
    timer = 0;
    warmuptime = 15.0;
    lastflip = 0;
    area = 2 * 3.14159 * radius * 0.6 * radius * 0.6;
    exhaust_velocity = 0;
    axis = nodes[nodefront].RelPosition - nodes[nodeback].RelPosition;
    reflen = axis.length();
    axis = axis / reflen;
    reset();
}

void Turbojet::SetupVisuals(std::string const& propname, Ogre::Entity* nozzle, float nozdiam, float nozlength, Ogre::Entity* afterburner_flame, bool disable_smoke)
{
    radius = nozdiam / 2.0;

    nozzleMesh = nozzle;
    nzsnode = gEnv->sceneManager->getRootSceneNode()->createChildSceneNode();
    nzsnode->attachObject(nozzleMesh);
    nzsnode->setScale(nozlength, nozdiam, nozdiam);

    if (afterburner_flame != nullptr)
    {
        flameMesh = afterburner_flame;
        absnode = gEnv->sceneManager->getRootSceneNode()->createChildSceneNode();
        absnode->attachObject(flameMesh);
        absnode->setScale(1.0, nozdiam, nozdiam);
        absnode->setVisible(false);
    }
    //smoke visual
    if (disable_smoke)
    {
        smokeNode = 0;
        smokePS = 0;
    }
    else
    {
        smokeNode = gEnv->sceneManager->getRootSceneNode()->createChildSceneNode();
        smokePS = gEnv->sceneManager->createParticleSystem("SmokeParticle-"+propname, "tracks/TurbopropSmoke");
        if (smokePS)
        {
            smokePS->setVisibilityFlags(DEPTHMAP_DISABLED); // disable particles in depthmap
            smokeNode->attachObject(smokePS);
            smokePS->setCastShadows(false);
        }
    }
}

Turbojet::~Turbojet()
{
    //A fast work around 
    //
    SOUND_MODULATE(trucknum, thr_id, 0);
    SOUND_MODULATE(trucknum, mod_id, 0);
    SOUND_STOP(trucknum, ab_id);
    SOUND_STOP(trucknum, src_id);

    if (flameMesh != nullptr)
    {
        delete flameMesh;
    }

    if (nozzleMesh != nullptr)
    {
        delete nozzleMesh;
    }

    if (smokePS != nullptr)
    {
        delete smokePS;
    }
}

void Turbojet::updateVisuals(RoR::GfxActor* gfx_actor)
{
    RoR::GfxActor::NodeData* node_buf = gfx_actor->GetSimNodeBuffer();

    //nozzle
    nzsnode->setPosition(node_buf[nodeback].AbsPosition);
    //build a local system
    Vector3 laxis = node_buf[nodefront].AbsPosition - node_buf[nodeback].AbsPosition;
    laxis.normalise();
    Vector3 paxis = Plane(laxis, 0).projectVector(node_buf[noderef].AbsPosition - node_buf[nodeback].AbsPosition);
    paxis.normalise();
    Vector3 taxis = laxis.crossProduct(paxis);
    Quaternion dir = Quaternion(laxis, paxis, taxis);
    nzsnode->setOrientation(dir);
    //afterburner
    if (afterburner)
    {
        absnode->setVisible(true);
        float flamelength = (afterburnthrust / 15.0) * (rpm / 100.0);
        flamelength = flamelength * (1.0 + (((Real)rand() / (Real)RAND_MAX) - 0.5) / 10.0);
        absnode->setScale(flamelength, radius * 2.0, radius * 2.0);
        absnode->setPosition(node_buf[nodeback].AbsPosition + dir * Vector3(-0.2, 0.0, 0.0));
        absnode->setOrientation(dir);
    }
    else
        absnode->setVisible(false);
    //smoke
    if (smokeNode)
    {
        smokeNode->setPosition(node_buf[nodeback].AbsPosition);
        ParticleEmitter* emit = smokePS->getEmitter(0);
        emit->setDirection(-axis);
        emit->setParticleVelocity(exhaust_velocity);
        if (!failed)
        {
            if (ignition)
            {
                emit->setEnabled(true);
                emit->setColour(ColourValue(0.0, 0.0, 0.0, 0.02 + throtle * 0.03));
                emit->setTimeToLive((0.02 + throtle * 0.03) / 0.1);
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
        SOUND_MODULATE(trucknum, mod_id, rpm);
    }
    timer += dt;
    axis = nodes[nodefront].RelPosition - nodes[nodeback].RelPosition;
    float axlen = axis.length();
    axis = axis / axlen; //normalize
    if (fabs(reflen - axlen) > 0.1)
    {
        rpm = 0;
        failed = true;
    }; //check for broken

    float warmupfactor = 1.0;
    if (warmup)
    {
        warmupfactor = (timer - warmupstart) / warmuptime;
        if (warmupfactor >= 1.0)
            warmup = false;
    }

    //turbine RPM
    //braking (compression)
    if (rpm < 0)
        rpm = 0;
    float torque = -rpm / 100.0;
    //powering with limiter
    if (rpm < 100.0 && !failed && ignition)
        torque += ((0.2 + throtle * 0.8) * warmupfactor);
    //integration
    rpm += (double)dt * torque * 30.0;

    float enginethrust = 0;
    if (!failed && ignition)
    {
        enginethrust = maxdrythrust * rpm / 100.0;
        afterburner = (afterburnable && throtle > 0.95 && rpm > 80);
        if (afterburner)
            enginethrust += (afterburnthrust - maxdrythrust);
    }
    else
        afterburner = false;

    if (afterburner)
        SOUND_START(trucknum, ab_id);
    else
        SOUND_STOP(trucknum, ab_id);

    nodes[nodeback].Forces += (enginethrust * 1000.0) * axis;
    exhaust_velocity = enginethrust * 5.6 / area;
}

void Turbojet::setThrottle(float val)
{
    if (val > 1.0)
        val = 1.0;

    if (val < 0.0)
        val = 0.0;

    throtle = val;
    SOUND_MODULATE(trucknum, thr_id, val);
}

float Turbojet::getThrottle()
{
    return throtle;
}

void Turbojet::setRPM(float _rpm)
{
    rpm = _rpm;
}

void Turbojet::reset()
{
    rpm = 0;
    throtle = 0;
    propwash = 0;
    failed = false;
    ignition = false;
    reverse = false;
}

void Turbojet::toggleReverse()
{
    if (!reversable)
        return;
    throtle = 0;
    reverse = !reverse;
}

void Turbojet::setReverse(bool val)
{
    reverse = val;
}

void Turbojet::flipStart()
{
    if (timer - lastflip < 0.3)
        return;
    ignition = !ignition;
    if (ignition && !failed)
    {
        warmup = true;
        warmupstart = timer;
        SOUND_START(trucknum, src_id);
    }
    else
    {
        SOUND_STOP(trucknum, src_id);
    }

    lastflip = timer;
}
