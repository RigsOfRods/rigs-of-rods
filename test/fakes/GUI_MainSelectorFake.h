#pragma once

#include "../../source/main/gui/panels/GUI_MainSelector.h"

#ifdef ROR_FAKES_IMPL

    void RoR::GUI::MainSelector::Show(enum LoaderType,struct RoR::ActorSpawnRequest) {}
    void RoR::GUI::MainSelector::Show(enum LoaderType) {}
    void RoR::GUI::MainSelector::Reset(void) {}
    void RoR::GUI::MainSelector::Cancel(void) {}
#endif // ROR_FAKES_IMPL
