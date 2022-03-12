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

#ifndef REAL_H_INCLUDED
#define REAL_H_INCLUDED

// ----------------------------------------------------------------------------
// Include standar libraries
// ----------------------------------------------------------------------------
#include <vector>

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

// ----------------------------------------------------------------------------
// Include the components
// ----------------------------------------------------------------------------
#include "Wave.h"
#include "PressurePoint.h"

/// @addtogroup Gfx
/// @{

/// @addtogroup Hydrax
/// @{

namespace Hydrax{ namespace Noise
{

/** @class Real Real.h Noise/Real/Real.h
 * @brief This class is a sea elevation module that combines several effects: \n
 * Waves, defined by direction, amplitude, period and optionally, phase.
 * Perlin noise, used to randomize the sea surface.
 * Pressure points, used to create puntual waves.
 */
class Real : public Noise
{
public:
    /** Default constructor
     */
    Real();

    /** Destructor
     */
    ~Real();

    /** Create
     */
    void create();

    /** Remove
     */
    void remove();

    /** Adds a wave to the system.
     * @param dir Direction of the wave.
     * @param A Amplitude of the wave (m).
     * @param T Period of the wave (s).
     * @param p Phase of the wave (rad).
     * @return Id of the wave.
     */
    int addWave(Ogre::Vector2 dir, float A, float T, float p=0.f);

    /** Adds a pressure point to the system
     * @param Orig Origin of perturbation.
     * @param p Pressure pulse (N·m). \n
     * The maximum half-amplitude of the wave will result from divide this value
     * with g*rho. So, a pressure of 10000 N·m produces 2 m waves.
     * @param T Maximum time of perturbation (s). \n
     * The effect of the perturbation will decay. When T time will be elapsed,
     * any effects will remain.
     * @param L wave length. \n
     * To objects falling, similar values to amplitude can be OK. For ships Kelvin
     * wakes probably length of the ship is a better choice.
     * @return Id of the pressure point.
     * @warning Pressure point will be autodestroyed when it have not any effect,
     * so it's not available any modify after the creation.
     */
    int addPressurePoint(Ogre::Vector2 Orig, float p, float T, float L);

    /** Returns a wave to the system.
     * @param id Identifier of the wave.
     * @return Selected wave.
     */
    Wave getWave(int id) const;

    /** Removes a wave from the system.
     * @param id Identifier of the wave.
     * @return true if all gone right, or false if errors happen.
     */
    bool eraseWave(int id);

    /** Modify a wave from the system.
     * @param id Identifier of the wave.
     * @param A Amplitude of the wave (m).
     * @param T Period of the wave (s).
     * @param p Phase of the wave (rad).
     * @return true if all gone right, or false if errors happen.
     */
    bool modifyWave(int id,Ogre::Vector2 dir, float A, float T, float p=0.f);

    /** Create GPUNormalMap resources
        @param g GPUNormalMapManager pointer
        @return true if it needs to be created, false if not
     */
    bool createGPUNormalMapResources(GPUNormalMapManager *g);

    /** Call it each frame
        @param timeSinceLastFrame Time since last frame(delta)
     */
    void update(const Ogre::Real &timeSinceLastFrame);

    /** Save config
        @param Data String reference
     */
    void saveCfg(Ogre::String &Data);

    /** Load config
        @param CgfFile Ogre::ConfigFile reference
        @return True if is the correct noise config
     */
    bool loadCfg(Ogre::ConfigFile &CfgFile);

    /** Get the especified x/y noise value
        @param x X Coord
        @param y Y Coord
        @return Noise value
        @remarks range [~-0.2, ~0.2]
     */
    float getValue(const float &x, const float &y);

    /** Get current Real noise options
        @return Current Real noise options
     */
    inline Perlin* getPerlinNoise() const{return mPerlinNoise;}

protected:


private:
    /// Elapsed time
    double mTime;

    /// Perlin noise
    Perlin *mPerlinNoise;

    /// GPUNormalMapManager pointer
    GPUNormalMapManager *mGPUNormalMapManager;

    /// Vector of waves
    std::vector<Wave> mWaves;

    /// Vector of pressure points
    std::vector<PressurePoint> mPressurePoints;
};

}}  // Namespace

/// @} // addtogroup Hydrax
/// @} // addtogroup Gfx

#endif // REAL_H_INCLUDED
