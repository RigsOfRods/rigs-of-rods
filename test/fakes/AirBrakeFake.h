#pragma once

#include "../../source/main/physics/air/AirBrake.h"

#ifdef ROR_FAKES_IMPL
    Airbrake::Airbrake(class Actor *,char const *,int,struct node_t *,struct node_t *,struct node_t *,struct node_t *,class Ogre::Vector<3,float>,float,float,float,std::string const &,float,float,float,float,float) {}
    void Airbrake::updatePosition(float) {}
    void Airbrake::applyForce(void) {}
#endif // ROR_FAKES_IMPL
