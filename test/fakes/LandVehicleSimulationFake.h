#pragma once

#include "../../source/main/gameplay/LandVehicleSimulation.h"

#ifdef ROR_FAKES_IMPL
    void RoR::LandVehicleSimulation::UpdateVehicle(class Actor *,float) {} /*static*/
    void RoR::LandVehicleSimulation::UpdateInputEvents(class Actor *,float) {}
#endif // ROR_FAKES_IMPL
