
#include "GUI_MultiplayerSelector.h"

#include "Application.h"
#include "GUIManager.h"
#include "RoRnet.h"
#include "RoRVersion.h"

#include <imgui.h>
#include <vector>

inline void DrawTableHeader(const char* title) // Internal helper
{
    float table_padding_y = 4.f;
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + table_padding_y);
    ImGui::Text(title);
    ImGui::NextColumn();
}

RoR::GUI::MultiplayerSelector::MpServerData::MpServerData(
    const char* name, const char* terrn, int users, int cap, const char* ip, int port, bool pw, int ping):
    num_users(users), max_users(cap), net_port(port), has_password(pw), net_ping(ping)
{
    server_name.Assign(name);
    terrain_name.Assign(terrn);
    ip_addr.Assign(ip);

    display_users << num_users << "/" << max_users;
    display_addr << ip_addr << ":" << net_port;
    display_ping << net_ping;
    display_passwd << (pw ? "Yes" : "No");
}

RoR::GUI::MultiplayerSelector::MultiplayerSelector():
    m_selected_item(-1), m_mode(Mode::ONLINE), m_is_refreshing(false), m_is_visible(false)
{
    snprintf(m_window_title, 100, "Multiplayer (Rigs of Rods %s | %s)", ROR_VERSION_STRING, RORNET_VERSION);
    // test dummies
    m_servers.emplace_back("test server A", "A.terrn", 5, 15, "1.1.1.1", 1111, true , 1);
    m_servers.emplace_back("test server B", "B.terrn", 4, 14, "2.2.2.2", 2222, false, 2);
    m_servers.emplace_back("test server C", "C.terrn", 3, 13, "3.3.3.3", 3333, false, 3);
    m_servers.emplace_back("test server D", "D.terrn", 2, 13, "1.3.3.3", 1333, false, 3);
    m_servers.emplace_back("test server E", "E.terrn", 1, 11, "0.1.3.3", 0333, false, 1);
    m_servers.emplace_back("test server F", "F.terrn", 0, 10, "1.0.1.3", 1313, false, 0);
    m_servers.emplace_back("test server G", "G.terrn", 1, 11, "2.1.0.1", 2303, false, 1);
    m_servers.emplace_back("test server H", "H.terrn", 2, 12, "3.2.1.0", 3313, false, 2);
    m_servers.emplace_back("test server I", "I.terrn", 3, 13, "4.3.2.1", 4323, false, 3);
    m_servers.emplace_back("test server J", "J.terrn", 4, 14, "5.4.3.2", 5333, false, 4);
    m_servers.emplace_back("test server K", "K.terrn", 5, 15, "6.5.4.3", 6343, false, 5);
    m_servers.emplace_back("test server L", "L.terrn", 6, 16, "7.6.5.4", 7353, false, 6);
    m_servers.emplace_back("test server M", "M.terrn", 7, 17, "8.7.6.5", 8363, false, 7);
    m_servers.emplace_back("test server N", "N.terrn", 8, 18, "9.8.7.6", 9373, false, 8);
    m_servers.emplace_back("test server O", "O.terrn", 9, 19, "0.9.8.7", 5383, false, 9);
    m_servers.emplace_back("test server P", "P.terrn", 0, 10, "1.0.9.8", 1393, false, 0);
}

void RoR::GUI::MultiplayerSelector::MultiplayerSelector::Draw()
{
    const float TABS_BOTTOM_PADDING = 4.f; // They're actually buttons in role of tabs.
    const float BUTTONS_EXTRA_SPACE = 6.f;
    const float TABLE_PADDING_LEFT = 4.f;

    int window_flags = ImGuiWindowFlags_NoCollapse;
    ImGui::SetNextWindowSize(ImVec2(600.f, 400.f), ImGuiSetCond_FirstUseEver);
    if (!ImGui::Begin(m_window_title, &m_is_visible, window_flags))
    {
        return;
    }

    if (!m_is_visible) // If the window was closed...
    {
        App::GetGuiManager()->SetVisible_GameMainMenu(true);
    }

    // Window mode buttons
    MultiplayerSelector::Mode next_mode = m_mode;

    if (ImGui::Button("Online (refresh)"))
    {
        next_mode = Mode::ONLINE;
        m_is_refreshing = !m_is_refreshing; // DEBUG
        // TODO: refresh
    }
    ImGui::SameLine();
    if (ImGui::Button("Direct IP"))
    {
        next_mode = Mode::DIRECT;
    }
    ImGui::SameLine();
    if (ImGui::Button("Setup"))
    {
        next_mode = Mode::SETUP;
    }

    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + TABS_BOTTOM_PADDING);
    ImGui::Separator();

    if (next_mode != m_mode) // Handle switching window modes
    {
        if (m_mode == Mode::SETUP) // If leaving SETUP mode, reset 'pending' values of GVars
        {
            App::mp_player_name    .SetPending(App::mp_player_name.GetActive()); // TODO: implement 'ResetPending()' ?
            App::mp_server_password.SetPending(App::mp_server_password.GetActive());
        }
        if (m_mode == Mode::DIRECT) // If leaving DIRECT mode, reset 'pending' values of GVars
        {
            App::mp_server_password.SetPending(App::mp_server_password.GetActive()); // TODO: implement 'ResetPending()' ?
            App::mp_server_host    .SetPending(App::mp_server_host.GetActive());
            App::mp_server_port    .SetPending(App::mp_server_port.GetActive());
        }
    }
    m_mode = next_mode;

    if (m_mode == Mode::SETUP)
    {
        ImGui::PushID("setup");

        ImGui::PushItemWidth(250.f);
        ImGui::InputText("Player nickname",         App::mp_player_name.GetPending().GetBuffer(),        App::mp_player_name.GetPending().GetCapacity());
        ImGui::InputText("Default server password", App::mp_server_password.GetPending().GetBuffer(),    App::mp_server_password.GetPending().GetCapacity());
        ImGui::PopItemWidth();

        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + BUTTONS_EXTRA_SPACE);
        if (ImGui::Button("Save"))
        {
            App::mp_player_name.ApplyPending();
            App::mp_server_password.ApplyPending();
        }

        ImGui::PopID();
    }
    else if (m_mode == Mode::DIRECT)
    {
        ImGui::PushID("direct");

        ImGui::PushItemWidth(250.f);
        ImGui::InputText("Server host", App::mp_server_host.GetPending().GetBuffer(), App::mp_server_host.GetPending().GetCapacity());
        int port = App::mp_server_port.GetPending();
        if (ImGui::InputInt("Server port", &port))
        {
            App::mp_server_port.SetPending(port);
        }
        ImGui::InputText("Server password (default)", App::mp_server_password.GetPending().GetBuffer(), App::mp_server_password.GetPending().GetCapacity());
        ImGui::PopItemWidth();

        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + BUTTONS_EXTRA_SPACE);
        if (ImGui::Button("Save & join"))
        {
            App::mp_server_host.ApplyPending();
            App::mp_server_port.ApplyPending();
            App::mp_server_password.ApplyPending();

            // TODO: perform the join.
        }
        ImGui::SameLine();
        if (ImGui::Button("Save only"))
        {
            App::mp_server_host.ApplyPending();
            App::mp_server_port.ApplyPending();
            App::mp_server_password.ApplyPending();
        }

        ImGui::PopID();
    }
    if (m_mode == Mode::ONLINE && m_is_refreshing)
    {
        const char* refresh_lbl = "... refreshing ...";
        const ImVec2 refresh_size = ImGui::CalcTextSize(refresh_lbl);
        ImGui::SetCursorPosX((ImGui::GetWindowSize().x / 2.f) - (refresh_size.x / 2.f));
        ImGui::SetCursorPosY((ImGui::GetWindowSize().y / 2.f) - (refresh_size.y / 2.f));
        ImGui::Text(refresh_lbl);
    }
    else if (m_mode == Mode::ONLINE && !m_is_refreshing)
    {
        // Setup serverlist table ... the scroll area
        const float table_height = ImGui::GetWindowHeight()
            - ((2.f * ImGui::GetStyle().WindowPadding.y) + (3.f * ImGui::GetItemsLineHeightWithSpacing())
                + TABS_BOTTOM_PADDING - ImGui::GetStyle().ItemSpacing.y);
        ImGui::BeginChild("scrolling", ImVec2(0.f, table_height), false);
        // ... and the table itself
        const float table_width = ImGui::GetWindowContentRegionWidth();
        ImGui::Columns(6, "mp-selector-columns");         // Col #0: Passwd
        ImGui::SetColumnOffset(1, 0.08f * table_width);   // Col #1: Server name
        ImGui::SetColumnOffset(2, 0.35f * table_width);   // Col #2: Terrain name
        ImGui::SetColumnOffset(3, 0.70f * table_width);   // Col #3: Users/Max
        ImGui::SetColumnOffset(4, 0.77f * table_width);   // Col #4: Ping
        ImGui::SetColumnOffset(5, 0.82f * table_width);   // Col #5: Host/Port
        // Draw table header
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + TABLE_PADDING_LEFT);
        DrawTableHeader("Passwd?");
        DrawTableHeader("Name");
        DrawTableHeader("Terrain");
        DrawTableHeader("Users");
        DrawTableHeader("Ping");
        DrawTableHeader("Host/Port");
        ImGui::Separator();
        // Draw table body
        int num_servers = static_cast<int>(m_servers.size());
        for (int i = 0; i < num_servers; i++)
        {
            // First column - selection control
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + TABLE_PADDING_LEFT);
            MpServerData& server = m_servers[i];
            if (ImGui::Selectable(server.display_passwd, m_selected_item == i, ImGuiSelectableFlags_SpanAllColumns))
            {
                m_selected_item = i;
            }
            ImGui::NextColumn();

            // Other collumns
            ImGui::Text(server.server_name);           ImGui::NextColumn();
            ImGui::Text(server.terrain_name);          ImGui::NextColumn();
            ImGui::Text(server.display_users);         ImGui::NextColumn();
            ImGui::Text(server.display_ping);          ImGui::NextColumn();
            ImGui::Text(server.display_addr);          ImGui::NextColumn();
        }
        ImGui::Columns(1);
        ImGui::EndChild(); // End of scroll area

        // Simple join button
        if (ImGui::Button("Join", ImVec2(200.f, 0.f)))
        {
            // TODO
        }
    }

    ImGui::End();
}

void RoR::GUI::MultiplayerSelector::RefreshServerlist()
{} // todo

bool RoR::GUI::MultiplayerSelector::IsRefreshThreadRunning() const
{
    return false;
} // todo

void RoR::GUI::MultiplayerSelector::CheckAndProcessRefreshResult()
{} // todo
