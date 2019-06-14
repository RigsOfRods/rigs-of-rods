#pragma once

#include "../../source/main/terrain/map/SurveyMapManager.h"

#ifdef ROR_FAKES_IMPL

    class SurveyMapEntity * SurveyMapManager::createMapEntity(std::string) {return nullptr;}
    void SurveyMapManager::windowResized(void) {}
    std::string SurveyMapManager::getTypeByDriveable(int) {return "";}
#endif // ROR_FAKES_IMPL
