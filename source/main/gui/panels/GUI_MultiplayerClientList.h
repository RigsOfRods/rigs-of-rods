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
#include "Network.h"

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
    bool DrawPlotSmall(const char* label, const char* overlay_text, NetGraphData& graphdata, float width);
    void DrawPlotBig(const char* label, NetGraphData& graphdata);
    void CacheIcons();

    std::vector<RoRnet::UserInfo> m_users; // only updated on demand to reduce mutex locking and vector allocating overhead.

    // Icon cache
    bool             m_icons_cached = false;
    Ogre::TexturePtr m_icon_arrow_down;
    Ogre::TexturePtr m_icon_arrow_down_grey;
    Ogre::TexturePtr m_icon_arrow_down_red;
    Ogre::TexturePtr m_icon_arrow_up;
    Ogre::TexturePtr m_icon_arrow_up_grey;
    Ogre::TexturePtr m_icon_arrow_up_red;
    Ogre::TexturePtr m_icon_flag_red;
    Ogre::TexturePtr m_icon_flag_blue;
    Ogre::TexturePtr m_icon_flag_green;
    Ogre::TexturePtr m_icon_warn_triangle;
};

} // namespace GUI
} // namespace RoR
