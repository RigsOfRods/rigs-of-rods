/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2013-2020 Petr Ohlidal

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
/// @brief  Mouse interaction with 3D scene: node grab (LMB), player actor/camera node select (MMB)
/// @author Thomas Fischer <thomas@thomasfischer.biz>
/// @date   11th of March 2011

// Remade with DearIMGUI by Petr Ohlidal, 2020

#pragma once

#include "Application.h"
#include <OIS.h>

#include <OIS.h>
#include <Ogre.h>

namespace RoR {
namespace GUI {

class SceneMouse
{
public:
    void Draw();

protected:
    Ogre::Ray CastMouseRay();

    int minnode = -1;
    float mindist = 0;
    Actor* grab_truck = nullptr;
    Ogre::Vector3 lastgrabpos = Ogre::Vector3::ZERO;
};

} // namespace GUI
} // namespace RoR
