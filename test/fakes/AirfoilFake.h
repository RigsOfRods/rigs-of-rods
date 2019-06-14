#pragma once

#include "../../source/main/physics/air/Airfoil.h"

#ifdef ROR_FAKES_IMPL
    Airfoil::Airfoil(std::string const &) {}
    Airfoil::~Airfoil(void) {}
    void Airfoil::getparams(float,float,float,float *,float *,float *) {}
#endif // ROR_FAKES_IMPL
