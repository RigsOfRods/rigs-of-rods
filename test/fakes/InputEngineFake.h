#pragma once

#include "../../source/main/utils/InputEngine.h"

#ifdef ROR_FAKES_IMPL
    InputEngine::InputEngine() {}
    InputEngine::~InputEngine() {}

    bool InputEngine::buttonPressed(const OIS::JoyStickEvent& arg, int button)         { return false; }
    bool InputEngine::buttonReleased(const OIS::JoyStickEvent& arg, int button)        { return false; }
    bool InputEngine::axisMoved(const OIS::JoyStickEvent& arg, int axis)               { return false; }
    bool InputEngine::sliderMoved(const OIS::JoyStickEvent&, int)                      { return false; }
    bool InputEngine::povMoved(const OIS::JoyStickEvent&, int)                         { return false; }
    bool InputEngine::keyPressed(const OIS::KeyEvent& arg)                             { return false; }
    bool InputEngine::keyReleased(const OIS::KeyEvent& arg)                            { return false; }
    bool InputEngine::mouseMoved(const OIS::MouseEvent& arg)                           { return false; }
    bool InputEngine::mousePressed(const OIS::MouseEvent& arg, OIS::MouseButtonID id)  { return false; }
    bool InputEngine::mouseReleased(const OIS::MouseEvent& arg, OIS::MouseButtonID id) { return false; }
    int  InputEngine::resolveEventName(std::string)                                    { return 0; }
    void InputEngine::Capture(void) {}
    float InputEngine::getEventValue(int,bool,int) {return 0.f;}
    bool InputEngine::getEventBoolValue(int) {return false;}
    bool InputEngine::getEventBoolValueBounce(int,float) {return false;}
    float InputEngine::getEventBounceTime(int) {return 0.f;}
    bool InputEngine::isKeyDown(enum OIS::KeyCode) {return false;}
    bool InputEngine::isKeyDownValueBounce(enum OIS::KeyCode,float) {return false;}
    void InputEngine::resetKeys(void) {}
    bool InputEngine::isEventDefined(int) {return false;}
    OIS::MouseState InputEngine::getMouseState(void) {return OIS::MouseState();}
    void InputEngine::windowResized(class Ogre::RenderWindow *) {}
    void InputEngine::updateKeyBounces(float) {}
#endif // ROR_FAKES_IMPL
