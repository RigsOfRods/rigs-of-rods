#pragma once

#include "../../source/main/resources/rig_def_fileformat/RigDef_File.h"

#ifdef ROR_FAKES_IMPL
    const char* RigDef::ROOT_MODULE_NAME = "_Root_";

    // File
    RigDef::File::File(void) {}
    char const *  RigDef::File::KeywordToString(enum RigDef::File::Keyword) {return "TEST_STUB";}

    RigDef::NodeDefaults::NodeDefaults():
        load_weight(-1.f),
        friction(1),
        volume(1),
        surface(1),
        options(0)
    {}

    // File::Module
    RigDef::File::Module::Module(std::string const& name) {}
#endif // ROR_FAKES_IMPL
