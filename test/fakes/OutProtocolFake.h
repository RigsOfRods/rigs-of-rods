#pragma once

#include "../../source/main/gameplay/OutProtocol.h"

#ifdef ROR_FAKES_IMPL
    OutProtocol::OutProtocol(void) {}
    bool OutProtocol::Update(float,class Actor *) {return false;}
    OutProtocol::~OutProtocol(void) {}
#endif // ROR_FAKES_IMPL
