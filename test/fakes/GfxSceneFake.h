#pragma once

#include "../../source/main/gfx/GfxScene.h"

#ifdef ROR_FAKES_IMPL
    void          RoR::GfxScene::RegisterGfxActor(class RoR::GfxActor*) {}
    DustPool *    RoR::GfxScene::GetDustPool(char const *) {return nullptr;}
    RoR::GfxScene::GfxScene(void) {}
    RoR::GfxScene::~GfxScene(void) {}
    void RoR::GfxScene::UpdateScene(float) {}
    void RoR::GfxScene::RemoveGfxActor(class RoR::GfxActor *) {}
    void RoR::GfxScene::BufferSimulationData(void) {}
    RoR::GfxScene::SimBuffer::SimBuffer(void) {}
#endif // ROR_FAKES_IMPL
