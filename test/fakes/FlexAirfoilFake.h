#pragma once

#include "../../source/main/physics/flex/FlexAirfoil.h"

#ifdef ROR_FAKES_IMPL
    FlexAirfoil::FlexAirfoil(std::string const &,class Actor *,int,int,int,int,int,int,int,int,class std::string const &,class Ogre::Vector<2,float>,class Ogre::Vector<2,float>,class Ogre::Vector<2,float>,class Ogre::Vector<2,float>,char,float,float,float,std::string const &,float,bool){}

    void  FlexAirfoil::enableInducedDrag(float,float,bool){}
    void  FlexAirfoil::addwash(int,float){}
    FlexAirfoil::~FlexAirfoil(void) {}
    void FlexAirfoil::updateVerticesPhysics(void) {}
    void FlexAirfoil::setControlDeflection(float) {}
    void FlexAirfoil::updateForces(void) {}
#endif // ROR_FAKES_IMPL
