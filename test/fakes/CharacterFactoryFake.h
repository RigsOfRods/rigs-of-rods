#pragma once

#include "../../source/main/gameplay/CharacterFactory.h"

#ifdef ROR_FAKES_IMPL
    class Character * RoR::CharacterFactory::createLocal(class Ogre::UTFString,int) {return nullptr;}
    void RoR::CharacterFactory::DeleteAllRemoteCharacters(void) {}
    void RoR::CharacterFactory::update(float) {}
#endif // ROR_FAKES_IMPL
