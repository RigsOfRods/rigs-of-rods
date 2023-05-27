/*
    This source file is part of Rigs of Rods
    Copyright 2021 tritonas00
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

#pragma once

#include "Application.h"

#include <OgreStringVector.h>

namespace RoR {
namespace GUI {

class ScriptMonitor
{
public:
    void Draw();
private:
    void DrawCommentedSeparator(const char* text);
    Ogre::StringVector m_recent_displaylist;
};

} // namespace GUI
} // namespace RoR