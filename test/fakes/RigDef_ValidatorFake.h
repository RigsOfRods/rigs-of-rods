#pragma once

#include "../../source/main/resources/rig_def_fileformat/RigDef_Validator.h"

#ifdef ROR_FAKES_IMPL
    void RigDef::Validator::Setup(class std::shared_ptr<struct RigDef::File>) {}
    bool RigDef::Validator::Validate(void) {return false;}
    std::string RigDef::Validator::ProcessMessagesToString(void) {return "";}
#endif // ROR_FAKES_IMPL
