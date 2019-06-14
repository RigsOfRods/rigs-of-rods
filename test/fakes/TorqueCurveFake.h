#pragma once

#include "../../source/main/gameplay/TorqueCurve.h"

#include <OgreString.h>

#ifdef ROR_FAKES_IMPL
    const Ogre::String TorqueCurve::customModel = "";

    int   TorqueCurve::setTorqueModel(Ogre::String){return 0;}
    bool  TorqueCurve::CreateNewCurve(Ogre::String const&){return false;}
    void  TorqueCurve::AddCurveSample(float,float,Ogre::String const&){}
    int   TorqueCurve::spaceCurveEvenly(class Ogre::SimpleSpline *){return 0;}
#endif // ROR_FAKES_IMPL
