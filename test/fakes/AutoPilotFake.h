#pragma once

#include "../../source/main/gameplay/AutoPilot.h"

#ifdef ROR_FAKES_IMPL
    Autopilot::Autopilot(int){}

    void Autopilot::setInertialReferences(struct node_t *,struct node_t *,struct node_t *,struct node_t *){}
    void Autopilot::disconnect(void) {}
    float Autopilot::getAilerons(void) {return 0.f;}
    float Autopilot::getElevator(void) {return 0.f;}
    float Autopilot::getRudder(void) {return 0.f;}
    void Autopilot::gpws_update(float) {}
    void Autopilot::UpdateIls(class std::vector<struct TerrainObjectManager::localizer_t,class std::allocator<struct TerrainObjectManager::localizer_t> >) {}
    void Autopilot::reset(void) {}
#endif // ROR_FAKES_IMPL
