#pragma once

#include "../../source/main/utils/ForceFeedback.h"

#ifdef ROR_FAKES_IMPL

    void RoR::ForceFeedback::SetEnabled(bool) {}
    void RoR::ForceFeedback::SetForces(float,float,float,float,float) {}
#endif // ROR_FAKES_IMPL
