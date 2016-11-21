/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer

    For more information, see http://www.rigsofrods.org/

    Rigs of Rods is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 3, as
    published by the Free Software Foundation.

    Rigs of Rods is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Rigs of Rods. If not, see <http://www.gnu.org/licenses/>.
*/

// "Depth of Field" demo for Ogre
// Copyright (C) 2006  Christian Lindequist Larsen
//
// This code is in the public domain. You may do whatever you want with it.
//
// Author: Ryan Booker (eyevee99)

#include "Lens.h"

#include <Ogre.h>

using namespace Ogre;

Lens::Lens(const Real& _focalLength, const Real& _fStop, const Real& _frameSize, const Real& _circleOfConfusion)
{
    init(_focalLength, _fStop, _frameSize, _circleOfConfusion);
}

Lens::Lens(const Radian& _fieldOfView, const Real& _fStop, const Real& _frameSize, const Real& _circleOfConfusion)
{
    init(_fieldOfView, _fStop, _frameSize, _circleOfConfusion);
}

void Lens::init(const Real& _focalLength, const Real& _fStop, const Real& _frameSize, const Real& _circleOfConfusion)
{
    mFocalLength = _focalLength;
    mFStop = _fStop;
    mFrameSize = _frameSize;
    mCircleOfConfusion = _circleOfConfusion;
    recalculateFieldOfView();
}

void Lens::init(const Radian& _fieldOfView, const Real& _fStop, const Real& _frameSize, const Real& _circleOfConfusion)
{
    mFieldOfView = _fieldOfView;
    mFStop = _fStop;
    mFrameSize = _frameSize;
    mCircleOfConfusion = _circleOfConfusion;
    recalculateFocalLength();
}

void Lens::setFrameSize(const Real& _frameSize)
{
    mFrameSize = _frameSize;
    recalculateFieldOfView();
}

void Lens::setCircleOfConfusion(const Real& _circleOfConfusion)
{
    mCircleOfConfusion = _circleOfConfusion;
    recalculateHyperfocalLength();
}

void Lens::setFocalDistance(const Real& _focalDistance)
{
    mFocalDistance = std::max(_focalDistance, 0.0f);
}

void Lens::setFocalLength(const Real& _focalLength)
{
    mFocalLength = std::max(_focalLength, 0.3f);
    recalculateFieldOfView();
}

void Lens::setFieldOfView(const Radian& _fieldOfView)
{
    mFieldOfView = std::min(_fieldOfView, Radian(2.8f));
    recalculateFocalLength();
}

void Lens::setFStop(const Real& _fStop)
{
    mFStop = std::max(_fStop, 0.0f);
}

void Lens::recalculateFocalLength(void)
{
    mFocalLength = (mFrameSize / Math::Tan(mFieldOfView / 2.0)) / 2.0;
}

void Lens::recalculateFieldOfView(void)
{
    mFieldOfView = 2.0 * Math::ATan(mFrameSize / (2.0 * mFocalLength));
}

void Lens::recalculateHyperfocalLength(void)
{
    mHyperfocalLength = (mFocalLength * mFocalLength) / (mFStop * mCircleOfConfusion) + mFocalLength;
}

void Lens::recalculateDepthOfField(Real& _nearDepth, Real& _focalDepth, Real& _farDepth)
{
    // Set focalDepth to the current focalDistance
    _focalDepth = mFocalDistance;

    // Recalculate the hyperfocal length
    recalculateHyperfocalLength();

    // Calculate the numerator of the optics equations
    Real numerator = (mFocalDistance * (mHyperfocalLength - mFocalLength));

    Real nearClear = numerator / (mHyperfocalLength + mFocalDistance - (2.0 * mFocalLength));

    // Adjust the nearDepth relative to the aperture. This is an approximation.
    _nearDepth = std::min(nearClear - nearClear * mFStop, 0.0f);

    if (mFocalDistance < mHyperfocalLength)
    {
        // Calculate the far clear plane
        Real farClear = numerator / (mHyperfocalLength - mFocalDistance);

        // Adjust the farDepth relative to the aperture. This is an approximation.
        _farDepth = farClear + farClear * mFStop;
    }
    else
    {
        // Far depth of field should be infinite
        _farDepth = Math::POS_INFINITY;
    }
}
