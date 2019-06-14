#pragma once

#include "../../source/main/gfx/MovableText.h"

#ifdef ROR_FAKES_IMPL
    Ogre::MovableText::MovableText(class Ogre::UTFString const&, class Ogre::UTFString const&, class Ogre::UTFString const&, float, class Ogre::ColourValue const&) {}
    Ogre::MovableText::~MovableText(void) {}

    void Ogre::MovableText::setTextAlignment(const Ogre::MovableText::HorizontalAlignment &,const Ogre::MovableText::VerticalAlignment &) {}
    void Ogre::MovableText::getWorldTransforms(class Ogre::Matrix4 *)const {}
    void Ogre::MovableText::_notifyCurrentCamera(class Ogre::Camera *) {}
    void Ogre::MovableText::_updateRenderQueue(class Ogre::RenderQueue *) {}
    void Ogre::MovableText::getRenderOperation(class Ogre::RenderOperation &) {}
#endif // ROR_FAKES_IMPL
