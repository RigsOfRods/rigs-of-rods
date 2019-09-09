#pragma once

#include "../../source/main/physics/air/TurboJet.h"

#ifdef ROR_FAKES_IMPL
    Turbojet::Turbojet(Actor* actor, int tnodefront, int tnodeback, int tnoderef, RigDef::Turbojet & def) {}
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
    TurbojetVisual::~TurbojetVisual(void) {}
    void TurbojetVisual::SetNodes(int,int,int) {}
    void TurbojetVisual::SetupVisuals(struct RigDef::Turbojet &,int,std::string const &,class Ogre::Entity *,class Ogre::Entity *,bool) {}
#endif // ROR_FAKES_IMPL
