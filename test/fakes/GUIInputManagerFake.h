#pragma once

#include "../../source/main/gui/GUIInputManager.h"

#include <OgreString.h>
#include <OgreUTFString.h>

#include <OISEvents.h>

#ifdef ROR_FAKES_IMPL
    GUIInputManager::GUIInputManager(void) {}
    GUIInputManager::~GUIInputManager(void) {}

    bool GUIInputManager::keyReleased(class OIS::KeyEvent const&) { return false; }
    bool GUIInputManager::mouseMoved(class OIS::MouseEvent const &) { return false; }
    bool GUIInputManager::mousePressed(class OIS::MouseEvent const &,enum OIS::MouseButtonID) { return false; }
    bool GUIInputManager::mouseReleased(class OIS::MouseEvent const &,enum OIS::MouseButtonID) { return false; }
    bool GUIInputManager::keyPressed(class OIS::KeyEvent const &) { return false; }
#endif // ROR_FAKES_IMPL
