/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2013+     Petr Ohlidal & contributors

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
/// @author Moncef Ben Slimane
/// @date   12/2014

#pragma once

#include "ForwardDeclarations.h"
#include "GUI_MultiplayerSelectorLayout.h"

namespace RoR
{

namespace GUI
{

struct ServerlistData; // Forward decl

class MultiplayerSelector : public MultiplayerSelectorLayout
{

public:
    MultiplayerSelector();

    void SetVisible(bool v);
    bool IsVisible();
    /// Launch refresh from main thread
    void RefreshServerlist();
    /// For main thread
    bool IsRefreshThreadRunning() const { return m_is_refreshing; }
    /// To be invoked periodically from main thread if refresh is in progress.
    void CheckAndProcessRefreshResult();

private:
    void CallbackJoinOnlineBtnPress(MyGUI::WidgetPtr _sender);
    void CallbackJoinDirectBtnPress(MyGUI::WidgetPtr _sender);
    void CallbackRefreshOnlineBtnPress(MyGUI::WidgetPtr _sender);
    void CallbackJoinOnlineListItem(MyGUI::MultiListBox* _sender, size_t index);
    void NotifyWindowButtonPressed(MyGUI::WidgetPtr _sender, const std::string& _name);
    void CenterToScreen();
    void ServerlistJoin(size_t sel_index);

    ServerlistData*            m_serverlist_data;
    bool                       m_is_refreshing;
};

} // namespace GUI

} // namespace RoR
