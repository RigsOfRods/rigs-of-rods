#pragma once

#include "../../source/main/physics/air/TurboJet.h"

#ifdef ROR_FAKES_IMPL
    Turbojet::Turbojet(int,int,struct node_t *,int,int,int,float,bool,float,float) {}

    void  Turbojet::SetupVisuals(class std::string const &,class Ogre::Entity *,float,float,class Ogre::Entity *,bool) {}
    Turbojet::~Turbojet(void) {}
    void Turbojet::flipStart(void) {}
    void Turbojet::reset(void) {}
    void Turbojet::setRPM(float) {}
    void Turbojet::setThrottle(float) {}
    void Turbojet::toggleReverse(void) {}
    void Turbojet::setReverse(bool) {}
    void Turbojet::updateForces(float,int) {}
    void Turbojet::updateVisuals(class RoR::GfxActor *) {}
    float Turbojet::getThrottle(void) {return 0.f;}
#endif // ROR_FAKES_IMPL
