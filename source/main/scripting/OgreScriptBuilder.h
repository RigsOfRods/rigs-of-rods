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
/// @author Thomas Fischer
/// @date   15th of May 2011

#pragma once

#include "Application.h"

#include "scriptbuilder/scriptbuilder.h"

#include <angelscript.h>

/// @addtogroup Scripting
/// @{

// our own class that wraps the CScriptBuilder and just overwrites the file loading parts
// to use the ogre resource system
class OgreScriptBuilder : public AngelScript::CScriptBuilder
{
public:
    Ogre::String GetHash() { return hash; };
protected:
    Ogre::String hash;
    int LoadScriptSection(const char* filename);
};

/// @}   //addtogroup Scripting
