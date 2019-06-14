#pragma once

#include "../../source/main/gui/panels/GUI_GameConsole.h"

#include <OgreUTFString.h>
#include <OgreString.h>

#ifdef ROR_FAKES_IMPL
    void RoR::Console::putMessage(int, int, Ogre::UTFString, Ogre::String, unsigned long, bool) {}
#endif // ROR_FAKES_IMPL
