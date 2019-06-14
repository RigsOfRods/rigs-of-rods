#pragma once

#include "../../source/main/resources/rig_def_fileformat/RigDef_SequentialImporter.h"

#ifdef ROR_FAKES_IMPL
    std::string RigDef::SequentialImporter::ProcessMessagesToString(void) {return "";}
    std::string RigDef::SequentialImporter::GetNodeStatistics(void) {return "";}
    std::string RigDef::SequentialImporter::IterateAndPrintAllNodes(void) {return "";}
#endif // ROR_FAKES_IMPL
