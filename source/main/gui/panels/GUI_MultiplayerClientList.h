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

/// @file
/// @author Thomas Fischer (thomas{AT}thomasfischer{DOT}biz)
/// @date   18th of July 2010


#pragma once

#include "RoRPrerequisites.h"

#include "RoRnet.h"

#include <MyGUI.h>

namespace RoR {
namespace GUI {

struct client_t
{
    RoRnet::UserInfo   user;                 //!< user struct
    bool               used;                 //!< if this slot is used already
};

class MpClientList
{
public:

    MpClientList();
    ~MpClientList();

    void update();

    bool IsVisible();
    void SetVisible(bool value);

protected:

    struct player_row_t
    {
        MyGUI::StaticImagePtr flagimg;
        MyGUI::StaticImagePtr statimg;
        MyGUI::StaticImagePtr usergoimg;
        MyGUI::StaticImagePtr user_actor_ok_img;
        MyGUI::StaticImagePtr user_remote_actor_ok_img;
        MyGUI::StaticTextPtr playername;
    };

    MyGUI::EditPtr msgtext;
    MyGUI::StaticTextPtr tooltipText;
    MyGUI::WidgetPtr tooltipPanel, mpPanel;
    MyGUI::WindowPtr msgwin;

    player_row_t player_rows[RORNET_MAX_PEERS + 1];

    void clickInfoIcon(MyGUI::WidgetPtr sender);
    void clickUserGoIcon(MyGUI::WidgetPtr sender);
    void openToolTip(MyGUI::WidgetPtr sender, const MyGUI::ToolTipInfo& t);

    MyGUI::WindowPtr netmsgwin;
    MyGUI::StaticTextPtr netmsgtext;

    void updateSlot(player_row_t* row, RoRnet::UserInfo c, bool self);

    client_t* clients;
    int lineheight;

    static const int sidebarWidth = 250;
};


} // namespace GUI
} // namespace RoR
