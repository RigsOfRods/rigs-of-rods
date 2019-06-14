#pragma once

#include "../../source/main/physics/flex/FlexObj.h"

#ifdef ROR_FAKES_IMPL
    FlexObj::FlexObj(class RoR::GfxActor *,struct node_t *,class std::vector<struct CabTexcoord,class std::allocator<struct CabTexcoord> > &,int,int *,class std::vector<struct CabSubmesh,class std::allocator<struct CabSubmesh> > &,char *,char const *,char *,char *) {}
    FlexObj::~FlexObj(void) {}
#endif // ROR_FAKES_IMPL
