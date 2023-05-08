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

//created by thomas fischer 23 February 2009

#pragma once

#include "Application.h"
#include "SimData.h"

namespace RoR {

/// @addtogroup Terrain
/// @{

class Landusemap
{
public:

    Landusemap(Ogre::String cfgfilename);
    ~Landusemap();

    ground_model_t* getGroundModelAt(int x, int z);
    int loadConfig(const Ogre::String& filename);

protected:

    ground_model_t** data;
    ground_model_t* default_ground_model;

    Ogre::Vector3 mapsize;
};

/// @} // addtogroup Terrain

} // namespace RoR
