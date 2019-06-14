#pragma once

#include "../../source/main/gameplay/BeamEngine.h"

#ifdef ROR_FAKES_IMPL
    EngineSim::EngineSim(float,float,float,class std::vector<float,class std::allocator<float> >,float,class Actor *) {}

    void   EngineSim::SetAcceleration(float)      {}
    void   EngineSim::SetClutch(float)            {}
    float  EngineSim::GetAcceleration(void)       { return 0.f; }
    float  EngineSim::GetClutch(void)             { return 0.f; }
    float  EngineSim::GetClutchForce(void)        { return 0.f; }
    float  EngineSim::GetCrankFactor(void)        { return 0.f; }
    float  EngineSim::GetSmoke(void)              { return 0.f; }
    float  EngineSim::GetTorque(void)             { return 0.f; }
    float  EngineSim::GetTurboPsi(void)           { return 0.f; }
    void   EngineSim::SetWheelSpin(float)         {}
    void   EngineSim::SetTCaseRatio(float)        {}
    void   EngineSim::StartEngine(void)           {}
    int    EngineSim::GetGear(void)               { return 0; }
    int    EngineSim::getAutoShift(void)          { return 0; }
    void   EngineSim::shift(int)                  {}
    void   EngineSim::OffStart(void)              {}
    void   EngineSim::UpdateEngineSim(float,int)  {}
    void   EngineSim::UpdateEngineAudio(void)     {}
    void   EngineSim::SetEngineOptions(float,char,float,float,float,float,float,float,float,float,float) {}
    void   EngineSim::SetTurboOptions(int,float,int,float,float,float,float,float,float,float,float,float,float,float) {}
    void   EngineSim::SetAutoMode(enum RoR::SimGearboxMode) {}
    EngineSim::~EngineSim(void) {}
    void EngineSim::SetEnginePriming(bool) {}
    void EngineSim::SetHydroPumpWork(float) {}
    void EngineSim::StopEngine(void) {}
#endif // ROR_FAKES_IMPL
