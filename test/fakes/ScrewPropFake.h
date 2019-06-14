#pragma once

#include "../../source/main/physics/water/ScrewProp.h"

#ifdef ROR_FAKES_IMPL
    Screwprop::Screwprop(struct node_t *,int,int,int,float,int){}
    void Screwprop::setThrottle(float) {}
    void Screwprop::setRudder(float) {}
    float Screwprop::getThrottle(void) {return 0.f;}
    float Screwprop::getRudder(void) {return 0.f;}
    void Screwprop::toggleReverse(void) {}
    void Screwprop::reset(void) {}
    void Screwprop::updateForces(int) {}
#endif // ROR_FAKES_IMPL
