/*
--------------------------------------------------------------------------------
This source file is part of Hydrax.
Visit http://www.ogre3d.org/tikiwiki/Hydrax

Copyright (C) 2011 Jose Luis Cercós Pita <jlcercos@gmail.com>

This program is free software; you can redistribute it and/or modify it under
the terms of the GNU Lesser General Public License as published by the Free Software
Foundation; either version 2 of the License, or (at your option) any later
version.

This program is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License along with
this program; if not, write to the Free Software Foundation, Inc., 59 Temple
Place - Suite 330, Boston, MA 02111-1307, USA, or go to
http://www.gnu.org/copyleft/lesser.txt.
--------------------------------------------------------------------------------
*/

// ----------------------------------------------------------------------------
// Include the main header
// ----------------------------------------------------------------------------
#include <PressurePoint.h>

// ----------------------------------------------------------------------------
// Include Hydrax
// ----------------------------------------------------------------------------
#include <Hydrax.h>

#define _def_PackedNoise true

#ifndef _LN001_
    #define _LN001_ -4.605170186
#endif

// 0 = linear decay function
// 1 = exponential decay function
// 2 = cuadratic decay function
#define _DECAYFUNCTION_ 2

using namespace Hydrax::Noise;

PressurePoint::PressurePoint(Ogre::Vector2 Orig, float p, float T, float L) : mTime(0), mPos(Orig), mP(p), mT(T), mL(L)
{
    mA = mP / _HydraxDensity_ / _HydraxGravity_;
    mK = 2.f * M_PI / mL;
    mC = 5.f * sqrt(mL);
    mW = mC * mK;
}

PressurePoint::~PressurePoint()
{
}

bool PressurePoint::update(const Ogre::Real &timeSinceLastFrame)
{
    mTime += timeSinceLastFrame;
    if (mTime >= mT)
    {
        mK1 = 0.f;
        return false;
    }

    mK1 = exp(_LN001_ / mT * mTime) - 0.01f;

    return true;
}

float PressurePoint::getValue(const float &x, const float &y)
{
    //! 1st.- Calculate distance.
    float R  = mC * mTime;
    float dx = x - mPos.x;
    if (dx > R) return 0.f;
    float dy = y - mPos.y;
    if (dy > R) return 0.f;
#if _DECAYFUNCTION_ == 0
    float r = sqrt(dx * dx + dy * dy);
#elif _DECAYFUNCTION_ == 1
    float r = sqrt(dx * dx + dy * dy);
#elif _DECAYFUNCTION_ == 2
    float r  = dx * dx + dy * dy;
    float R2 = R * R;
#endif
    //! 2nd.- Calculate time decay factors
    if (mTime >= mT) return 0.f;
    //! 2nd.- Calculate distance decay factors
    if (r >= R) return 0.f;
#if _DECAYFUNCTION_ == 0
    mK2 = 1.f - r / R;
#elif _DECAYFUNCTION_ == 1
    mK2     = exp(_LN001_ * r / R) - 0.01f;
#elif _DECAYFUNCTION_ == 2
    float f  = r / R2;
    mK2      = 1.f - f;
    r        = f * R;
#endif
    //! 3rd.- Calculate height
    return mK1 * mK2 * mA * sin(mW * mTime - mK * r);
}
