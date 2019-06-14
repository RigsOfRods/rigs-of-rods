#pragma once

#include "../../source/main/physics/Differentials.h"

#ifdef ROR_FAKES_IMPL
    Ogre::UTFString Differential::GetDifferentialTypeName(void) {return Ogre::UTFString();}
    void Differential::ToggleDifferentialMode(void) {}
    void Differential::CalcAxleTorque(struct DifferentialData &) {}
#endif // ROR_FAKES_IMPL
