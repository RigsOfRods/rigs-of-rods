#pragma once

#include "../../source/main/gui/DashBoardManager.h"

#ifdef ROR_FAKES_IMPL
    DashBoardManager::DashBoardManager(void) {}

    void DashBoardManager::windowResized(void) {}
    int  DashBoardManager::loadDashBoard(std::string,bool) {return 0;}
    void DashBoardManager::setVisible(bool) {}
    DashBoardManager::~DashBoardManager(void) {}
    void DashBoardManager::setVisible3d(bool) {}
    void DashBoardManager::update(float &) {}
    void DashBoardManager::updateFeatures(void) {}
#endif // ROR_FAKES_IMPL
