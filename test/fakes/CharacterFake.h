#pragma once

#include "../../source/main/gameplay/Character.h"

#ifdef ROR_FAKES_IMPL
    Character::~Character(void) {}
    Ogre::Vector3 Character::getPosition(void) {return Ogre::Vector3::ZERO;}
    void Character::setPosition(Ogre::Vector3) {}
    void Character::setRotation(class Ogre::Radian) {}
    void Character::SetActorCoupling(bool,class Actor *) {}
#endif // ROR_FAKES_IMPL
