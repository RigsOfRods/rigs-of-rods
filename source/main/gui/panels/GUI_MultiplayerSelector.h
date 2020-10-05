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

#pragma once

#include "Application.h"
#include "OgreImGui.h" // ImVec4

#include <future>
#include <memory>
#include <thread>
#include <vector>

namespace RoR{
namespace GUI {

struct MpServerInfo
{
    bool          has_password;
    std::string   display_passwd;
    std::string   display_name;
    std::string   display_terrn;
    int           num_users;
    int           max_users;
    std::string   display_users;
    std::string   net_host;
    std::string   net_version;
    std::string   display_version;
    int           net_port;
    std::string   display_host;
};

typedef std::vector<MpServerInfo> MpServerInfoVec;

class MultiplayerSelector
{
public:
    MultiplayerSelector();
    ~MultiplayerSelector();

    void                SetVisible(bool v);
    inline bool         IsVisible() const { return m_is_visible; }
    void                StartAsyncRefresh(); //!< Launch refresh from main thread
    void                Draw();
    void                DisplayRefreshFailed(std::string const& msg);
    void                UpdateServerlist(MpServerInfoVec* data);

private:
    enum class Mode { ONLINE, DIRECT, SETUP };

    MpServerInfoVec     m_serverlist_data;
    std::string         m_serverlist_msg;
    ImVec4              m_serverlist_msg_color;
    int                 m_selected_item;
    Mode                m_mode;
    char                m_window_title[100];
    bool                m_is_visible;
    bool                m_draw_table;
    Str<1000>           m_user_token_buf;
    Str<1000>           m_player_name_buf;
    Str<1000>           m_password_buf;
    Str<1000>           m_server_host_buf;
};

} // namespace GUI
} // namespace RoR
