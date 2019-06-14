#pragma once

#include "../../source/main/resources/rig_def_fileformat/RigDef_Parser.h"

#ifdef ROR_FAKES_IMPL
    RigDef::Parser::Parser(void) {}

    void          RigDef::Parser::Prepare(void)  {}
    void          RigDef::Parser::Finalize(void) {}
    void          RigDef::Parser::ProcessOgreStream(class Ogre::DataStream*, std::string) {}
    std::string   RigDef::Parser::ProcessMessagesToString(void) { return ""; }
#endif // ROR_FAKES_IMPL
