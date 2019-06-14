#pragma once

#include "../../source/main/physics/air/TurboProp.h"

#ifdef ROR_FAKES_IMPL
    Turboprop::Turboprop(char const *,struct node_t *,int,int,int,int,int,int,int,float,std::string const &,int,int,bool,bool,float){}
    Turboprop::~Turboprop(void) {}
    void Turboprop::updateVisuals(class RoR::GfxActor *) {}
    void Turboprop::updateForces(float,int) {}
    void Turboprop::setThrottle(float) {}
    float Turboprop::getThrottle(void) {return 0.f;}
    void Turboprop::reset(void) {}
    void Turboprop::toggleReverse(void) {}
    void Turboprop::setReverse(bool) {}
    void Turboprop::flipStart(void) {}
    void Turboprop::setRPM(float) {}
#endif // ROR_FAKES_IMPL
