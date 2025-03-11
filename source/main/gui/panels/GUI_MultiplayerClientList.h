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
    void DrawIcon(Ogre::TexturePtr tex, ImVec2 reference_box);
    void CacheIcons();

    std::vector<RoRnet::UserInfo> m_users; // only updated on demand to reduce mutex locking and vector allocating overhead; see `MSG_GUI_MP_CLIENTS_REFRESH`.
    std::vector <BitMask_t> m_users_peeropts; // updated along with `m_users`, see `MSG_GUI_MP_CLIENTS_REFRESH`.

    // Peer options menu - opened by [<] button, closes when mouse cursor leaves.
    void DrawPeerOptionsMenu();
    void DrawPeerOptCheckbox(const BitMask_t flag, const std::string& label);
    void DrawServerCommandBtn(const std::string& cmdfmt, const std::string& label);
    const int PEEROPTS_MENU_CONTENT_WIDTH = 150;
    const int PEEROPTS_MENU_MARGIN = 10;
    const int PEEROPTS_HOVER_MARGIN = 100;
    int m_peeropts_menu_active_user_vectorpos = -1;
    ImVec2 m_peeropts_menu_corner_tl = ImVec2(0, 0);
    ImVec2 m_peeropts_menu_corner_br = ImVec2(0, 0);

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
