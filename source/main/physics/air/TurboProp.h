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

using namespace RoR;

class Turboprop: public AeroEngine, public ZeroedMemoryAllocator
{
public:

    bool is_piston;
    float pitch;
    float indicated_torque;
    float max_torque;

    Turboprop(
        const char* propname,
        node_t* nd,
        int nr,
        int nb,
        int np1,
        int np2,
        int np3,
        int np4,
        int tqn,
        float power,
        Ogre::String const& propfoilname,
        int mnumber,
        int trucknum,
        bool disable_smoke,
        bool ispiston,
        float fpitch
    );
    ~Turboprop();

    void updateVisuals(RoR::GfxActor* gfx_actor) override;
    void updateForces(float dt, int doUpdate);

    void setThrottle(float val);

    float getThrottle();

    void reset();

    void toggleReverse();
    void setReverse(bool val);
    bool getReverse() { return reverse; };

    void flipStart();

    float getRPM() { return rpm; };
    float getRPMpc() { return rpm / 10.0f; };
    void setRPM(float _rpm);
    float getpropwash() { return propwash; };
    Ogre::Vector3 getAxis() { return axis; };
    bool isFailed() { return failed; };
    AeroEngineType getType() { return AEROENGINE_TURBOPROP_PISTONPROP; };
    bool getIgnition() { return ignition; };
    void setIgnition(bool val) { ignition = val; };
    int getNoderef() { return noderef; };
    bool getWarmup() { return warmup; };
    float getRadius() { return radius; };

private:

    node_t* nodes;
    int nodeback;
    int nodep[4];
    int torquenode;
    float torquedist;
    Airfoil* airfoil;
    float fullpower; //!< in kW
    float proparea;
    float airdensity;
    float timer;
    float lastflip;
    float warmupstart;
    float warmuptime;
    int number;
    int numblades;
    float bladewidth;
    float pitchspeed;
    float maxrevpitch;
    float regspeed;
    Ogre::ParticleSystem* smokePS;
    Ogre::SceneNode* smokeNode;
    float twistmap[5];
    double rotenergy;
    float fixed_pitch;

    bool reverse;
    bool warmup;
    bool ignition;
    float radius;
    bool failed;
    bool failedold;
    float rpm;
    float throtle;
    int noderef;
    char debug[256];
    float propwash;
    Ogre::Vector3 axis;
    int trucknum;
    int mod_id;
    int src_id;
    int thr_id;
};
