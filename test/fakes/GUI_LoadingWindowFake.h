#pragma once

#include "../../source/main/gui/panels/GUI_LoadingWindow.h"

#ifdef ROR_FAKES_IMPL

    void RoR::GUI::LoadingWindow::setProgress(int,class Ogre::UTFString const &,bool) {}
#endif // ROR_FAKES_IMPL
