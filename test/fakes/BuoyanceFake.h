#pragma once

#include "../../source/main/physics/water/Buoyance.h"

#ifdef ROR_FAKES_IMPL
    Buoyance::Buoyance(class DustPool *,class DustPool *){}
    Buoyance::~Buoyance() {}
    void Buoyance::computeNodeForce(struct node_t *,struct node_t *,struct node_t *,bool,int) {}
#endif // ROR_FAKES_IMPL
