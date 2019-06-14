#pragma once

#include "../../source/main/gameplay/RoRFrameListener.h"

static RoR::InterActorBeamsMap s_interbeam_map;

#ifdef ROR_FAKES_IMPL
    SimController::SimController(class RoR::ForceFeedback *,class RoR::SkidmarkConfig *) {}

    RoR::InterActorBeamsMap & SimController::GetInterActorBeams() { return s_interbeam_map; }
    void SimController::windowFocusChange(class Ogre::RenderWindow *) {}
    void SimController::windowResized(class Ogre::RenderWindow *) {}
#endif // ROR_FAKES_IMPL
