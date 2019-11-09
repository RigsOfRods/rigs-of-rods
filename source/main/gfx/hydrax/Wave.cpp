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
#include <Wave.h>

// ----------------------------------------------------------------------------
// Include Hydrax
// ----------------------------------------------------------------------------
#include <Hydrax.h>

#define _def_PackedNoise true

using namespace Hydrax::Noise;

Wave::Wave(Ogre::Vector2 dir, float A, float T, float p) : mTime(0), mDir(dir), mA(A), mT(T), mP(p)
{
    mDir.normalise();
    mL = 1.5625f * mT * mT;
    mC = 1.25f * sqrt(mL);
    mF = 2.f * M_PI / mT;
    mK = 2.f * M_PI / mL;
}

Wave::~Wave()
{
}

void Wave::update(const Ogre::Real &timeSinceLastFrame)
{
    mTime += timeSinceLastFrame;
}

float Wave::getValue(const float &x, const float &y)
{
    float X = mDir.x * x + mDir.y * y;
    return mA * sin(mF * mTime - mK * X + mP);
}
