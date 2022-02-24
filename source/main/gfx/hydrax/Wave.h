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

#ifndef WAVE_H_INCLUDED
#define WAVE_H_INCLUDED

// ----------------------------------------------------------------------------
// Include standar libraries
// ----------------------------------------------------------------------------
#include <math.h>

// ----------------------------------------------------------------------------
// Include the hydrax prerequisites
// ----------------------------------------------------------------------------
#include "Prerequisites.h"

// ----------------------------------------------------------------------------
// Include the noise module
// ----------------------------------------------------------------------------
#include "Noise.h"

// ----------------------------------------------------------------------------
// Include the perlin noise
// ----------------------------------------------------------------------------
#include "Perlin.h"

/// @addtogroup Gfx
/// @{

/// @addtogroup Hydrax
/// @{

namespace Hydrax{ namespace Noise
{

/** @class Wave Wave.h Noise/Wave/Wave.h
 * @brief A wave defined by the direction, amplitude, period, and optionally phase.
 */
class Wave
{
public:
    /** Default constructor
     * @param dir Direction of the wave.
     * @param A Amplitude of the wave (m).
     * @param T Period of the wave (s).
     * @param p Phase of the wave (rad).
     */
    Wave(Ogre::Vector2 dir, float A, float T, float p=0.f);

    /** Destructor
     */
    ~Wave();

    /** Call it each frame
        @param timeSinceLastFrame Time since last frame(delta)
     */
    void update(const Ogre::Real &timeSinceLastFrame);

    /** Get the especified x/y noise value
        @param x X Coord
        @param y Y Coord
        @return Noise value
        @remarks range [~-0.2, ~0.2]
     */
    float getValue(const float &x, const float &y);

    /** Returns direction of the wave.
     * @return Direction.
     */
    inline Ogre::Vector2 getDirection() const{return mDir;}
    /** Returns amplitude of the wave.
     * @return Amplitude.
     */
    inline float getAmplitude() const{return mA;}
    /** Returns period of the wave.
     * @return Period.
     */
    inline float getPeriod() const{return mT;}
    /** Returns phase of the wave.
     * @return Phase.
     */
    inline float getPhase() const{return mP;}
    /** Returns phase speed of the wave.
     * @return Phase speed.
     */
    inline float getSpeed() const{return mC;}
    /** Returns longitude of the wave.
     * @return Wave longitude.
     */
    inline float getLongitude() const{return mL;}

protected:


private:
    /// Elapsed time
    double mTime;

    /// Direction (must be normalised)
    Ogre::Vector2 mDir;
    /// Amplitude
    float mA;
    /// Period
    float mT;
    /// Phase
    float mP;
    /// Speed (calculated)
    float mC;
    /// Longitude (calculated)
    float mL;
    /// Angular frec.
    float mF;
    /// Dispersion factor
    float mK;
};

/// @} // addtogroup Hydrax
/// @} // addtogroup Gfx

}}  // Namespace

#endif // WAVE_H_INCLUDED
