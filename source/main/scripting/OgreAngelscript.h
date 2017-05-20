/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2013-2017 Petr Ohlidal & contributors

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
/// @author Thomas Fischer
/// @date   30th of April 2010

// created on 31th of July 2009 by Thomas Fischer

#pragma once

#include <angelscript.h>
#include <Ogre.h>

#include "RoRPrerequisites.h"

// This function will register the following objects with the scriptengine:
//    - Ogre::Vector3
//    - Ogre::Radian
//    - Ogre::Degree
//    - Ogre::Quaternion
void registerOgreObjects(AngelScript::asIScriptEngine* engine);

// The following functions shouldn't be called directly!
// Use the registerOgreObjects function above instead.
void registerOgreVector3(AngelScript::asIScriptEngine* engine);
void registerOgreRadian(AngelScript::asIScriptEngine* engine);
void registerOgreDegree(AngelScript::asIScriptEngine* engine);
void registerOgreQuaternion(AngelScript::asIScriptEngine* engine);

