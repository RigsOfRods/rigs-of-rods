/*
    This source file is part of Rigs of Rods
    Copyright 2009 Lefteris Stamatogiannakis

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

/// @file
/// @author estama
/// @date   April 6, 2009, 2:57 AM

#pragma once

#include "RoRPrerequisites.h"

static int mirand = 1;

// Returns a random number in the range [0, 1]
inline float frand()
{
    unsigned int a;

    mirand *= 16807;

    a = (mirand & 0x007fffff) | 0x40000000;

    return (*((float *)&a) - 2.0f) * 0.5f;
}

// Returns a random number in the range [0, 2]
inline float frand_02()
{
    unsigned int a;

    mirand *= 16807;

    a = (mirand & 0x007fffff) | 0x40000000;

    return (*((float *)&a) - 2.0f);
}

// Returns a random number in the range [-1, 1]
inline float frand_11()
{
    unsigned int a;

    mirand *= 16807;

    a = (mirand & 0x007fffff) | 0x40000000;

    return (*((float *)&a) - 3.0f);
}

// Calculates approximate e^x.
// Use it in code not requiring precision
inline float approx_exp(const float x)
{
    if (x < -15)
        return 0.f;
    else if (x > 88)
        return 1e38f;
    else
    {
        int i = 12102203 * x + 1064652319;
        return *(float *)&i;
    }
}

// Calculates approximate 2^x
// Use it in code not requiring precision
inline float approx_pow2(const float x)
{
    int i = 8388608 * x + 1065353216;

    return *(float *)&i;
}

// Calculates approximate x^y
// Use it in code not requiring precision
inline float approx_pow(const float x, const float y)
{
    float v = x;
    int   i = y * ((*(int *)&v) - 1065353216) + 1065353216;

    return *(float *)&i;
}

// Calculates approximate square_root(x)
// Use it in code not requiring precision
inline float approx_sqrt(const float y)
{
    float f = y;
    int   i = (((*(int *)&f) - 1065353216) >> 1) + 1065353216;

    return *(float *)&i;
}

// Calculates approximate 1/square_root(x)
// it is faster than fast_invSqrt BUT
// use it in code not requiring precision
inline float approx_invSqrt(const float y)
{
    float f = y;
    int   i = 0x5f3759df - ((*(int *)&f) >> 1);

    return *(float *)&i;
}

// This function is a classic 1/square_root(x)code
// used by quake's game engine.
// It is very fast and has enough precision
// to drive a physics engine.
inline float fast_invSqrt(const float v)
{
    float y = v;
    int   i = 0x5f3759df - ((*(int *)&y) >> 1);
    y       = *(float *)&i;

    y *= (1.5f - (0.5f * v * y * y));
    return y;
}

// It calculates a fast and accurate square_root(x)
inline float fast_sqrt(const float x)
{
    return x * fast_invSqrt(x);
}

inline float sign(const float x)
{
    return (x > 0.0f) ? 1.0f : (x < 0.0f) ? -1.0f : 0.0f;
}

// Ogre3 specific helpers
inline Ogre::Vector3 approx_normalise(Ogre::Vector3 v)
{
    return v * approx_invSqrt(v.squaredLength());
}

inline Ogre::Vector3 fast_normalise(Ogre::Vector3 v)
{
    return v * fast_invSqrt(v.squaredLength());
}

inline float approx_length(Ogre::Vector3 v)
{
    return approx_sqrt(v.squaredLength());
}

inline float fast_length(Ogre::Vector3 v)
{
    return fast_sqrt(v.squaredLength());
}
