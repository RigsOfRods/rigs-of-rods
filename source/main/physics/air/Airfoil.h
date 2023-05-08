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

#pragma once

#include "Application.h"

namespace RoR {

/// @addtogroup Physics
/// @{

/// Represents an airfoil http://en.wikipedia.org/wiki/Airfoil
class Airfoil
{
public:

    /// Parses the airfoil from file.
    /// @param fname File name (X-Plane's .AFL file format)
    Airfoil(Ogre::String const& fname);
    ~Airfoil();

    void getparams(float a, float cratio, float cdef, float* ocl, float* ocd, float* ocm);

private:

    float cl[3601];
    float cd[3601];
    float cm[3601];
};

/// @} // addtogroup Physics

} // namespace RoR
