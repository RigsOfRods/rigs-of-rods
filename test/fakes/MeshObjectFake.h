#pragma once

#include "../../source/main/utils/MeshObject.h"

#ifdef ROR_FAKES_IMPL
    MeshObject::MeshObject(std::string, std::string, std::string, Ogre::SceneNode *){}

    void MeshObject::setMaterialName(std::string) {}
    void MeshObject::setCastShadows(bool) {}
#endif // ROR_FAKES_IMPL
