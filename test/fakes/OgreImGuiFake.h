#pragma once

#include "../../source/main/gui/imgui/OgreImGui.h"

#ifdef ROR_FAKES_IMPL
    bool OgreImGui::frameRenderingQueued(struct Ogre::FrameEvent const&) { return false; }
#endif // ROR_FAKES_IMPL
