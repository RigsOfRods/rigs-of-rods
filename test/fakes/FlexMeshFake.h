#pragma once

#include "../../source/main/physics/flex/FlexMesh.h"

#ifdef ROR_FAKES_IMPL
    FlexMesh::FlexMesh(std::string const &,class RoR::GfxActor *,int,int,int,int,std::string const &,std::string const &,bool,float){}
    FlexMesh::~FlexMesh(void) {}
    void FlexMesh::flexitCompute(void) {}
    Ogre::Vector3 FlexMesh::flexitFinal(void) {return Ogre::Vector3::ZERO;}
#endif // ROR_FAKES_IMPL
