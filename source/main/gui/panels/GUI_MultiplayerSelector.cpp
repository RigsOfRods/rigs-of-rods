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

#include "GUI_MultiplayerSelector.h"

#include "Application.h"
#include "GameContext.h"
#include "GUIManager.h"
#include "GUIUtils.h"
#include "RoRnet.h"
#include "RoRVersion.h"

#include <imgui.h>
#include <rapidjson/document.h>
#include <vector>

#ifdef USE_CURL
#   include <curl/curl.h>
#   include <curl/easy.h>
#endif //USE_CURL

#if defined(_MSC_VER) && defined(GetObject) // This MS Windows macro from <wingdi.h> (Windows Kit 8.1) clashes with RapidJSON
#   undef GetObject
#endif

using namespace RoR;
using namespace GUI;

#if defined(USE_CURL)

// From example: https://gist.github.com/whoshuu/2dc858b8730079602044
size_t CurlWriteFunc(void *ptr, size_t size, size_t nmemb, std::string* data)
{
    data->append((char*) ptr, size * nmemb);
    return size * nmemb;
}

void FetchServerlist(std::string portal_url)
{
    std::string serverlist_url = portal_url + "/server-list?json=true";
    std::string response_payload;
    std::string response_header;
    long        response_code = 0;

    CURL *curl = curl_easy_init();
    curl_easy_setopt(curl, CURLOPT_URL,           serverlist_url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, CurlWriteFunc);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA,     &response_payload);
    curl_easy_setopt(curl, CURLOPT_HEADERDATA,    &response_header);

    curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);

    curl_easy_cleanup(curl);
    curl = nullptr;

    if (response_code != 200)
    {
        Ogre::LogManager::getSingleton().stream() 
            << "[RoR|Multiplayer] Failed to retrieve serverlist; HTTP status code: " << response_code;
        App::GetGameContext()->PushMessage(
            Message(MSG_NET_REFRESH_SERVERLIST_FAILURE, "Error connecting to server :("));
        return;
    }

    rapidjson::Document j_data_doc;
    j_data_doc.Parse(response_payload.c_str());
    if (j_data_doc.HasParseError() || !j_data_doc.IsArray())
    {
        Ogre::LogManager::getSingleton().stream() 
            << "[RoR|Multiplayer] Error parsing serverlist JSON"; // TODO: Report the actual error
        App::GetGameContext()->PushMessage(
            Message(MSG_NET_REFRESH_SERVERLIST_FAILURE, "Server returned invalid data :("));
        return;
    }

    // Pre-process data for display
    size_t num_rows = j_data_doc.GetArray().Size();
    GUI::MpServerInfoVec* servers_ptr = new GUI::MpServerInfoVec();
    GUI::MpServerInfoVec& servers = *servers_ptr;
    servers.resize(num_rows);
    for (size_t i = 0; i < num_rows; ++i)
    {
        rapidjson::Value& j_row = j_data_doc[static_cast<rapidjson::SizeType>(i)];

        servers[i].display_name  = j_row["name"].GetString();
        servers[i].display_terrn = j_row["terrain-name"].GetString();
        servers[i].net_host      = j_row["ip"].GetString();
        servers[i].net_port      = j_row["port"].GetInt();

        servers[i].has_password  = j_row["has-password"].GetBool();
        servers[i].display_passwd = servers[i].has_password ? "Yes" : "No";

        char display_host[400];
        snprintf(display_host, 400, "%s:%d", j_row["ip"].GetString(), j_row["port"].GetInt());
        servers[i].display_host  = display_host;

        char display_users[200];
        snprintf(display_users, 200, "%d / %d", j_row["current-users"].GetInt(), j_row["max-clients"].GetInt());
        servers[i].display_users = display_users;

        servers[i].net_version = j_row["version"].GetString();
        servers[i].display_version = Ogre::StringUtil::replaceAll(j_row["version"].GetString(), "RoRnet_", "");
    }

    App::GetGameContext()->PushMessage(
        Message(MSG_NET_REFRESH_SERVERLIST_SUCCESS, (void*)servers_ptr));
}
#endif // defined(USE_CURL)

inline void DrawTableHeader(const char* title) // Internal helper
{
    float table_padding_y = 4.f;
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + table_padding_y);
    ImGui::Text("%s", title);
    ImGui::NextColumn();
}

MultiplayerSelector::MultiplayerSelector():
    m_selected_item(-1), m_mode(Mode::ONLINE), m_draw_table(false), m_is_visible(false)
{
    snprintf(m_window_title, 100, "Multiplayer (Rigs of Rods %s | %s)", ROR_VERSION_STRING, RORNET_VERSION);
}

MultiplayerSelector::~MultiplayerSelector()
{}

void MultiplayerSelector::MultiplayerSelector::Draw()
{
    const float TABS_BOTTOM_PADDING = 4.f; // They're actually buttons in role of tabs.
    const float CONTENT_TOP_PADDING = 4.f; // Extra space under top horizontal separator bar.
    const float BUTTONS_EXTRA_SPACE = 6.f;
    const float TABLE_PADDING_LEFT = 4.f;

    int window_flags = ImGuiWindowFlags_NoCollapse;
    ImGui::SetNextWindowSize(ImVec2(750.f, 400.f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowPosCenter();
    bool keep_open = true;
    ImGui::Begin(m_window_title, &keep_open, window_flags);

    // Window mode buttons
    MultiplayerSelector::Mode next_mode = m_mode;

    if (ImGui::Button("Online (click to refresh)"))
    {
        if (m_mode == Mode::ONLINE)
            this->StartAsyncRefresh();
        else
            next_mode = Mode::ONLINE;
    }
    ImGui::SameLine();
    if (ImGui::Button("Direct IP"))
    {
        next_mode = Mode::DIRECT;
    }
    ImGui::SameLine();
    if (ImGui::Button("Settings"))
    {
        next_mode = Mode::SETUP;
    }

    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + TABS_BOTTOM_PADDING);
    ImGui::Separator();
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + CONTENT_TOP_PADDING);

    m_mode = next_mode;

    if (m_mode == Mode::SETUP)
    {
        ImGui::PushID("setup");

        DrawGCheckbox(App::mp_join_on_startup,    "Auto connect");
        DrawGCheckbox(App::mp_chat_auto_hide,     "Auto hide chat");
        DrawGCheckbox(App::mp_hide_net_labels,    "Hide net labels");
        DrawGCheckbox(App::mp_hide_own_net_label, "Hide own net label");
        DrawGCheckbox(App::mp_pseudo_collisions,  "Multiplayer collisions");

        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + BUTTONS_EXTRA_SPACE);
        ImGui::Separator();

        ImGui::PushItemWidth(250.f);
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + CONTENT_TOP_PADDING);
        DrawGTextEdit(App::mp_player_name,        "Player nickname", m_player_name_buf);
        DrawGTextEdit(App::mp_server_password,    "Default server password", m_password_buf);

        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + BUTTONS_EXTRA_SPACE);
        ImGui::Separator();

        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + CONTENT_TOP_PADDING);
        DrawGTextEdit(App::mp_player_token,       "User token", m_user_token_buf);
        ImGui::PopItemWidth();

        ImGui::PopID();
    }
    else if (m_mode == Mode::DIRECT)
    {
        ImGui::PushID("direct");

        ImGui::PushItemWidth(250.f);
        DrawGTextEdit(App::mp_server_host, "Server host", m_server_host_buf);
        DrawGIntBox(App::mp_server_port, "Server port");
        ImGui::InputText("Server password", m_password_buf.GetBuffer(), m_password_buf.GetCapacity());
        ImGui::PopItemWidth();

        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + BUTTONS_EXTRA_SPACE);
        if (ImGui::Button("Join"))
        {
            App::mp_server_password->SetStr(m_password_buf.GetBuffer());
            App::GetGameContext()->PushMessage(Message(MSG_NET_CONNECT_REQUESTED));
        }

        ImGui::PopID();
    }
    else if (m_mode == Mode::ONLINE)
    {
        const char* draw_label_text = nullptr;
        ImVec4      draw_label_color;

        // DETERMINE WHAT TO DRAW
        if (!m_serverlist_msg.empty())
        {
            draw_label_text = m_serverlist_msg.c_str();
            draw_label_color = m_serverlist_msg_color;
        }

        // DRAW SERVERLIST TABLE
        if (m_draw_table)
        {
            // Setup serverlist table ... the scroll area
            const float table_height = ImGui::GetWindowHeight()
                - ((2.f * ImGui::GetStyle().WindowPadding.y) + (3.f * ImGui::GetItemsLineHeightWithSpacing())
                    + TABS_BOTTOM_PADDING + CONTENT_TOP_PADDING - ImGui::GetStyle().ItemSpacing.y);
            ImGui::BeginChild("scrolling", ImVec2(0.f, table_height), false);
            // ... and the table itself
            const float table_width = ImGui::GetWindowContentRegionWidth();
            ImGui::Columns(6, "mp-selector-columns");         // Col #0: Passwd
            ImGui::SetColumnOffset(1, 0.09f * table_width);   // Col #1: Server name
            ImGui::SetColumnOffset(2, 0.36f * table_width);   // Col #2: Terrain name
            ImGui::SetColumnOffset(3, 0.67f * table_width);   // Col #3: Users/Max
            ImGui::SetColumnOffset(4, 0.74f * table_width);   // Col #4: Version
            ImGui::SetColumnOffset(5, 0.82f * table_width);   // Col #5: Host/Port
            // Draw table header
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + TABLE_PADDING_LEFT);
            DrawTableHeader("Passwd?");
            DrawTableHeader("Name");
            DrawTableHeader("Terrain");
            DrawTableHeader("Users");
            DrawTableHeader("Version");
            DrawTableHeader("Host/Port");
            ImGui::Separator();
            // Draw table body
            for (int i = 0; i < (int)m_serverlist_data.size(); i++)
            {
                ImGui::PushID(i);

                // First column - selection control
                ImGui::SetCursorPosX(ImGui::GetCursorPosX() + TABLE_PADDING_LEFT);
                MpServerInfo& server = m_serverlist_data[i];
                if (ImGui::Selectable(server.display_passwd.c_str(), m_selected_item == i, ImGuiSelectableFlags_SpanAllColumns))
                {
                    m_selected_item = i;
                }
                ImGui::NextColumn();

                bool compatible = (server.net_version == RORNET_VERSION);
                ImVec4 version_color = compatible ? ImVec4(0.0f, 0.9f, 0.0f, 1.0f) : ImVec4(0.9f, 0.0f, 0.0f, 1.0f);

                // Other collumns
                ImGui::Text("%s", server.display_name.c_str());    ImGui::NextColumn();
                ImGui::Text("%s", server.display_terrn.c_str());   ImGui::NextColumn();
                ImGui::Text("%s", server.display_users.c_str());   ImGui::NextColumn();
                ImGui::PushStyleColor(ImGuiCol_Text, version_color);
                ImGui::Text("%s", server.display_version.c_str()); ImGui::NextColumn();
                ImGui::PopStyleColor();
                ImGui::Text("%s", server.display_host.c_str());    ImGui::NextColumn();

                ImGui::PopID();
            }
            ImGui::Columns(1);
            ImGui::EndChild(); // End of scroll area

            // Simple join button (and password input box)
            if (m_selected_item != -1 && m_serverlist_data[m_selected_item].net_version == RORNET_VERSION)
            {
                MpServerInfo& server = m_serverlist_data[m_selected_item];
                if (ImGui::Button("Join", ImVec2(200.f, 0.f)))
                {
                    App::mp_server_password->SetStr(m_password_buf.GetBuffer());
                    App::mp_server_host->SetStr(server.net_host.c_str());
                    App::mp_server_port->SetVal(server.net_port);
                    App::GetGameContext()->PushMessage(Message(MSG_NET_CONNECT_REQUESTED));
                }
                if (server.has_password)
                {
                    // TODO: Find out why this is always visible ~ ulteq 01/2019
                    ImGui::SameLine();
                    ImGui::PushItemWidth(250.f);
                    ImGui::InputText("Server password", m_password_buf.GetBuffer(), m_password_buf.GetCapacity());
                    ImGui::PopItemWidth();
                }
            }
        }

        // DRAW CENTERED LABEL
        if (draw_label_text != nullptr)
        {
            const ImVec2 label_size = ImGui::CalcTextSize(draw_label_text);
            ImGui::SetCursorPosX((ImGui::GetWindowSize().x / 2.f) - (label_size.x / 2.f));
            ImGui::SetCursorPosY((ImGui::GetWindowSize().y / 2.f) - (label_size.y / 2.f));
            ImGui::TextColored(draw_label_color, "%s", draw_label_text);
        }
    }

    App::GetGuiManager()->RequestGuiCaptureKeyboard(ImGui::IsWindowHovered());
    ImGui::End();
    if (!keep_open)
    {
        this->SetVisible(false);
    }
}

void MultiplayerSelector::StartAsyncRefresh()
{
#if defined(USE_CURL)
    m_draw_table = false;
    m_serverlist_data.clear();
    m_selected_item = -1;
    m_serverlist_msg = "... refreshing ...";
    m_serverlist_msg_color = App::GetGuiManager()->GetTheme().in_progress_text_color;
    std::packaged_task<void(std::string)> task(FetchServerlist);
    std::thread(std::move(task), App::mp_api_url->GetStr()).detach(); // launch on a thread
#endif // defined(USE_CURL)
}

void MultiplayerSelector::SetVisible(bool visible)
{
    m_is_visible = visible;
    if (visible && m_serverlist_data.size() == 0) // Do an initial refresh
    {
        this->StartAsyncRefresh();
        m_password_buf = App::mp_server_password->GetStr();
    }
    else if (!visible && App::app_state->GetEnum<AppState>() == AppState::MAIN_MENU)
    {
        App::GetGuiManager()->SetVisible_GameMainMenu(true);
    }
}

void MultiplayerSelector::DisplayRefreshFailed(std::string const& msg)
{
    m_serverlist_msg = msg;
    m_serverlist_msg_color = App::GetGuiManager()->GetTheme().error_text_color;
    m_draw_table = false;
}

void MultiplayerSelector::UpdateServerlist(MpServerInfoVec* data)
{
    m_serverlist_data = *data;
    m_draw_table = true;
    if (m_serverlist_data.empty())
    {
        m_serverlist_msg = "There are no available servers :/";
        m_serverlist_msg_color = App::GetGuiManager()->GetTheme().no_entries_text_color;
    }
    else
    {
        m_serverlist_msg = "";
    }
}
