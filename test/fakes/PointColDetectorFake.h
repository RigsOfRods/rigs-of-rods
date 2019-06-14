#pragma once

#include "../../source/main/physics/collision/PointColDetector.h"

#ifdef ROR_FAKES_IMPL
    void  PointColDetector::UpdateInterPoint(bool) {}
    void PointColDetector::UpdateIntraPoint(bool) {}
    void PointColDetector::query(Ogre::Vector3 const &,Ogre::Vector3 const &,Ogre::Vector3 const &,float) {}
#endif // ROR_FAKES_IMPL
