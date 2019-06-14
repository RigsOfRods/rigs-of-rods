#pragma once

#include "../../source/main/gui/OverlayWrapper.h"

#ifdef ROR_FAKES_IMPL
    OverlayWrapper::OverlayWrapper(void) {}
    OverlayWrapper::~OverlayWrapper(void) {}
    void OverlayWrapper::ToggleDashboardOverlays(class Actor *) {}
    void OverlayWrapper::showDashboardOverlays(bool,class Actor *) {}
    void OverlayWrapper::showDebugOverlay(int) {}
    void OverlayWrapper::showPressureOverlay(bool) {}
    void OverlayWrapper::windowResized(void) {}
    void OverlayWrapper::SetupDirectionArrow(void) {}
    void OverlayWrapper::HideDirectionOverlay(void) {}
    void OverlayWrapper::ShowDirectionOverlay(std::string const &) {}
    void OverlayWrapper::update(float) {}
    void OverlayWrapper::updateStats(bool) {}
#endif // ROR_FAKES_IMPL
