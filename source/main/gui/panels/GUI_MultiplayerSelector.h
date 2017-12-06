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

#pragma once

#include "Application.h"

#include <future>
#include <memory>
#include <thread>
#include <vector>

namespace RoR{
namespace GUI {

struct MpServerlistData; // Forward declaration, private implementation.

class MultiplayerSelector
{
public:

    MultiplayerSelector();
    ~MultiplayerSelector();

    void         SetVisible(bool v);
    inline bool  IsVisible()                           { return m_is_visible; }
    void         RefreshServerlist();                  /// Launch refresh from main thread
    bool         IsRefreshThreadRunning() const;       /// Check status from main thread
    void         CheckAndProcessRefreshResult();       /// To be invoked periodically from main thread if refresh is in progress.
    void         Draw();

private:
    enum class Mode { ONLINE, DIRECT, SETUP };

    std::future<MpServerlistData*> m_serverlist_future;
    std::unique_ptr<MpServerlistData> m_serverlist_data;
    int                            m_selected_item;
    Mode                           m_mode;
    bool                           m_is_refreshing;
    char                           m_window_title[100];
    bool                           m_is_visible;
    Str<200>                       m_user_token_buf;
    Str<100>                       m_player_name_buf;
    Str<100>                       m_password_buf;
    Str<200>                       m_server_host_buf;
};

} // namespace GUI
} // namespace RoR
