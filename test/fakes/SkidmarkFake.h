#pragma once

#include "../../source/main/gfx/Skidmark.h"

#ifdef ROR_FAKES_IMPL
    RoR::Skidmark::Skidmark(class RoR::SkidmarkConfig *,struct wheel_t *,class Ogre::SceneNode *,int,int) {}
    RoR::Skidmark::~Skidmark(void) {}
    void RoR::Skidmark::update(Ogre::Vector3,int,float,std::string) {}
#endif // ROR_FAKES_IMPL
