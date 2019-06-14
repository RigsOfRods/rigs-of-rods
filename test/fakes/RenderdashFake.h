#pragma once

#include "../../source/main/gfx/Renderdash.h"

#ifdef ROR_FAKES_IMPL
    RoR::Renderdash::Renderdash(std::string const &,std::string const &,std::string const &) {}
    RoR::Renderdash::~Renderdash(void) {}

    void RoR::Renderdash::preRenderTargetUpdate(struct Ogre::RenderTargetEvent const &) {}
    void RoR::Renderdash::postRenderTargetUpdate(struct Ogre::RenderTargetEvent const &) {}
#endif // ROR_FAKES_IMPL
