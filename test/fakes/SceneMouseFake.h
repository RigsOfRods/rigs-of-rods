#pragma once

#include "../../source/main/gameplay/SceneMouse.h"

#ifdef ROR_FAKES_IMPL
    RoR::SceneMouse::SceneMouse(void) {}
    void RoR::SceneMouse::InitializeVisuals(void) {}
    void RoR::SceneMouse::UpdateSimulation(void) {}
    void RoR::SceneMouse::DiscardVisuals(void) {}
#endif // ROR_FAKES_IMPL
