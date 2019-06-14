#pragma once

#include "../../source/main/physics/flex/FlexFactory.h"

#ifdef ROR_FAKES_IMPL
    RoR::FlexFactory::FlexFactory(class ActorSpawner *){}
    RoR::FlexBodyFileIO::FlexBodyFileIO(void) {}

    FlexBody *       RoR::FlexFactory::CreateFlexBody(struct RigDef::Flexbody *,int,int,int,class Ogre::Quaternion const &,class std::vector<unsigned int,class std::allocator<unsigned int> > &,class std::basic_string<char,struct std::char_traits<char>,class std::allocator<char> >){return nullptr;}
    FlexMeshWheel *  RoR::FlexFactory::CreateFlexMeshWheel(unsigned int,int,int,int,int,float,bool,class std::basic_string<char,struct std::char_traits<char>,class std::allocator<char> > const &,class std::basic_string<char,struct std::char_traits<char>,class std::allocator<char> > const &) {return nullptr;}
    void             RoR::FlexFactory::CheckAndLoadFlexbodyCache(void){}
    void             RoR::FlexFactory::SaveFlexbodiesToCache(void){}
#endif // ROR_FAKES_IMPL
