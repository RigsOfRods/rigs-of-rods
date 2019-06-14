#pragma once

#include "../../source/main/gfx/camera/CameraManager.h"

#ifdef ROR_FAKES_IMPL
    RoR::CameraManager::CameraManager(void) {}
    RoR::CameraManager::~CameraManager(void) {}

    bool RoR::CameraManager::Update(float,class Actor *,float)                                    { return false; }
    bool RoR::CameraManager::gameControlsLocked(void)const                                        { return false; }
    void RoR::CameraManager::NotifyContextChange(void)                                            { }
    void RoR::CameraManager::NotifyVehicleChanged(class Actor *,class Actor *)                    { }
    bool RoR::CameraManager::mouseMoved(class OIS::MouseEvent const &)                            { return false; }
    bool RoR::CameraManager::mousePressed(class OIS::MouseEvent const &,enum OIS::MouseButtonID)  { return false; }
#endif // ROR_FAKES_IMPL
