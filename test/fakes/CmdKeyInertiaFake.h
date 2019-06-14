#pragma once

#include "../../source/main/physics/CmdKeyInertia.h"

#ifdef ROR_FAKES_IMPL
    CmdKeyInertia::CmdKeyInertia(void) {}
    int CmdKeyInertia::setCmdKeyDelay(int,float,float,std::string,std::string) {return 0;}
    void CmdKeyInertia::resetCmdKeyDelay(void) {}
    float CmdKeyInertia::calcCmdKeyDelay(float,int,float) {return 0.f;}
#endif // ROR_FAKES_IMPL
