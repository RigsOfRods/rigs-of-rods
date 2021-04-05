/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2013-2019 Petr Ohlidal

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
/// @author Thomas Fischer (thomas{AT}thomasfischer{DOT}biz)
/// @date   18th of July 2010

/// @author Remake to DearIMGUI: Petr Ohlidal, 11/2019

#pragma once

#include "RoRnet.h"

#include <Ogre.h>
#include <imgui.h>

namespace RoR {
namespace GUI {

class MpClientList
{
public:
    void Draw();
    void UpdateClients();

private:
    bool DrawIcon(Ogre::TexturePtr tex, ImVec2 reference_box); // Returns true if hovered

    std::vector<RoRnet::UserInfo> m_users; // only updated on demand to reduce mutex locking and vector allocating overhead.
};

} // namespace GUI
} // namespace RoR
