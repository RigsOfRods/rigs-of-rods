#pragma once

#include "../../source/main/utils/MeshObject.h"

#ifdef ROR_FAKES_IMPL
    MeshObject::MeshObject(class std::string,class std::string,class std::string,class Ogre::SceneNode *){}

    void MeshObject::setMaterialName(class std::string) {}
    void MeshObject::setCastShadows(bool) {}
#endif // ROR_FAKES_IMPL
