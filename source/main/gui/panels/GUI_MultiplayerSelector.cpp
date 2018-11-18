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

#include "GUI_MultiplayerSelector.h"

#include "Application.h"
#include "GUIManager.h"
#include "GUIUtils.h"
#include "MainMenu.h"
#include "RoRnet.h"
#include "RoRVersion.h"
#include "SHA1.h"

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

struct RoR::GUI::MpServerlistData
{
    struct ServerInfo
    {
        bool        has_password;
        Str<50>     display_passwd;
        Str<100>    display_name;
        Str<100>    display_terrn;
        int         num_users;
        int         max_users;
        Str<20>     display_users;
        Str<100>    net_host;
        int         net_port;
        Str<50>     display_host;
    };

    MpServerlistData(): success(false) {}

    std::vector<ServerInfo> servers;
    std::string             message;
    bool                    success;
};

#if defined(USE_CURL)

// From example: https://gist.github.com/whoshuu/2dc858b8730079602044
size_t CurlWriteFunc(void *ptr, size_t size, size_t nmemb, std::string* data)
{
    data->append((char*) ptr, size * nmemb);
    return size * nmemb;
}

RoR::GUI::MpServerlistData* FetchServerlist(std::string portal_url)
{
    std::string serverlist_url = portal_url + "/server-list?json=true";
    std::string response_payload;
    std::string response_header;
    long        response_code = 0;

    CURL *curl = curl_easy_init();
    curl_easy_setopt(curl, CURLOPT_URL,           serverlist_url.c_str());
    curl_easy_setopt(curl, CURLOPT_NOPROGRESS,    1L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, CurlWriteFunc);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA,     &response_payload);
    curl_easy_setopt(curl, CURLOPT_HEADERDATA,    &response_header);

    curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);

    curl_easy_cleanup(curl);
    curl = nullptr;

    RoR::GUI::MpServerlistData* res = new RoR::GUI::MpServerlistData();
    if (response_code != 200)
    {
        Ogre::LogManager::getSingleton().stream() 
            << "[RoR|Multiplayer] Failed to retrieve serverlist; HTTP status code: " << response_code;
        res->message = "Error connecting to server :(";
        return res;
    }

    rapidjson::Document j_data_doc;
    j_data_doc.Parse(response_payload.c_str());
    if (j_data_doc.HasParseError() || !j_data_doc.IsArray())
    {
        Ogre::LogManager::getSingleton().stream() 
            << "[RoR|Multiplayer] Error parsing serverlist JSON"; // TODO: Report the actual error
        res->message = "Server returned invalid data :(";
        return res;
    }

    // Pre-process data for display
    size_t num_rows = j_data_doc.GetArray().Size();
    res->servers.resize(num_rows);
    for (size_t i = 0; i < num_rows; ++i)
    {
        rapidjson::Value& j_row = j_data_doc[i];

        res->servers[i].display_name  = j_row["name"].GetString();
        res->servers[i].display_terrn = j_row["terrain-name"].GetString();
        res->servers[i].net_host      = j_row["ip"].GetString();
        res->servers[i].net_port      = j_row["port"].GetInt();

        const bool has_pw = j_row["has-password"].GetBool();
        res->servers[i].has_password  = has_pw;
        res->servers[i].display_passwd = (has_pw) ? "Yes" : "No";

        char display_host[400];
        snprintf(display_host, 400, "%s:%d", j_row["ip"].GetString(), j_row["port"].GetInt());
        res->servers[i].display_host  = display_host;

        char display_users[200];
        snprintf(display_users, 200, "%d / %d", j_row["current-users"].GetInt(), j_row["max-clients"].GetInt());
        res->servers[i].display_users = display_users;
    }

    res->success = true;
    return res;
}
#endif // defined(USE_CURL)

inline void DrawTableHeader(const char* title) // Internal helper
{
    float table_padding_y = 4.f;
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + table_padding_y);
    ImGui::Text("%s", title);
    ImGui::NextColumn();
}

RoR::GUI::MultiplayerSelector::MultiplayerSelector():
    m_selected_item(-1), m_mode(Mode::ONLINE), m_is_refreshing(false), m_is_visible(false)
{
    snprintf(m_window_title, 100, "Multiplayer (Rigs of Rods %s | %s)", ROR_VERSION_STRING, RORNET_VERSION);
}

RoR::GUI::MultiplayerSelector::~MultiplayerSelector()
{}

void RoR::GUI::MultiplayerSelector::MultiplayerSelector::Draw()
{
    const float TABS_BOTTOM_PADDING = 4.f; // They're actually buttons in role of tabs.
    const float CONTENT_TOP_PADDING = 4.f; // Extra space under top horizontal separator bar.
    const float BUTTONS_EXTRA_SPACE = 6.f;
    const float TABLE_PADDING_LEFT = 4.f;

    int window_flags = ImGuiWindowFlags_NoCollapse;
    ImGui::SetNextWindowSize(ImVec2(750.f, 400.f), ImGuiSetCond_FirstUseEver);
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

    if (ImGui::Button("Online (click to refresh)"))
    {
        if (m_mode == Mode::ONLINE)
            this->RefreshServerlist();
        else
            next_mode = Mode::ONLINE;
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
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + CONTENT_TOP_PADDING);

    m_mode = next_mode;

    if (m_mode == Mode::SETUP)
    {
        ImGui::PushID("setup");

        ImGui::PushItemWidth(250.f);
        DrawGTextEdit(App::mp_player_name, "Player nickname", m_player_name_buf);
        DrawGTextEdit(App::mp_server_password, "Default server password", m_password_buf);
        ImGui::PopItemWidth();

        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + BUTTONS_EXTRA_SPACE);
        ImGui::Separator();

        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + CONTENT_TOP_PADDING);
        ImGui::PushItemWidth(250.f);
        ImGui::InputText("Player token", m_user_token_buf.GetBuffer(), m_user_token_buf.GetCapacity());
        ImGui::PopItemWidth();

        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + BUTTONS_EXTRA_SPACE);
        if (ImGui::Button("Update saved hash") && (m_user_token_buf.GetLength() > 0))
        {
            RoR::CSHA1 sha1;
            uint8_t* input_text = (uint8_t *)m_user_token_buf.GetBuffer();
            uint32_t input_size = (uint32_t)m_user_token_buf.GetLength();
            sha1.UpdateHash(input_text, input_size);
            sha1.Final();
            Str<250> hash;
            sha1.ReportHash(hash.GetBuffer(), RoR::CSHA1::REPORT_HEX_SHORT);
            App::mp_player_token_hash.SetActive(hash.ToCStr());
            m_user_token_buf.Clear();
        }
        ImGui::SameLine();
        ImGui::TextDisabled(" Hash: [%s]", App::mp_player_token_hash.GetActive());

        ImGui::PopID();
    }
    else if (m_mode == Mode::DIRECT)
    {
        ImGui::PushID("direct");

        ImGui::PushItemWidth(250.f);
        DrawGTextEdit(App::mp_server_host, "Server host", m_server_host_buf);
        DrawGIntBox(App::mp_server_port, "Server port");
        DrawGTextEdit(App::mp_server_password, "Server password (default)", m_password_buf);
        ImGui::PopItemWidth();

        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + BUTTONS_EXTRA_SPACE);
        if (ImGui::Button("Join"))
        {
            App::mp_state.SetPending (MpState::CONNECTED);
        }

        ImGui::PopID();
    }
    else if (m_mode == Mode::ONLINE)
    {
        const char* draw_label_text = nullptr;
        ImVec4      draw_label_color;
        bool        draw_table = false;

        // DETERMINE WHAT TO DRAW
        if (m_is_refreshing)
        {
            draw_label_text = "... refreshing ...";
            draw_label_color = App::GetGuiManager()->GetTheme().in_progress_text_color;
        }
        else if (m_serverlist_data != nullptr)
        {
            if (m_serverlist_data->success == true)
            {
                draw_table = true;
                if (m_serverlist_data->servers.size() == 0)
                {
                    draw_label_text = "There are no available servers :/"; // Draw empty table _and_ the label.
                    draw_label_color = App::GetGuiManager()->GetTheme().no_entries_text_color;
                }
            }
            else
            {
                draw_label_text = m_serverlist_data->message.c_str();
                draw_label_color = App::GetGuiManager()->GetTheme().error_text_color;
            }
        }

        // DRAW SERVERLIST TABLE
        if (draw_table)
        {
            // Setup serverlist table ... the scroll area
            const float table_height = ImGui::GetWindowHeight()
                - ((2.f * ImGui::GetStyle().WindowPadding.y) + (3.f * ImGui::GetItemsLineHeightWithSpacing())
                    + TABS_BOTTOM_PADDING + CONTENT_TOP_PADDING - ImGui::GetStyle().ItemSpacing.y);
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
            int num_servers = static_cast<int>(m_serverlist_data->servers.size());
            for (int i = 0; i < num_servers; i++)
            {
                ImGui::PushID(i);

                // First column - selection control
                ImGui::SetCursorPosX(ImGui::GetCursorPosX() + TABLE_PADDING_LEFT);
                MpServerlistData::ServerInfo& server = m_serverlist_data->servers[i];
                if (ImGui::Selectable(server.display_passwd, m_selected_item == i, ImGuiSelectableFlags_SpanAllColumns))
                {
                    m_selected_item = i;
                }
                ImGui::NextColumn();

                // Other collumns
                ImGui::Text("%s", server.display_name.ToCStr());   ImGui::NextColumn();
                ImGui::Text("%s", server.display_terrn.ToCStr());  ImGui::NextColumn();
                ImGui::Text("%s", server.display_users.ToCStr());  ImGui::NextColumn();
                ImGui::Text("~");                                  ImGui::NextColumn(); // TODO: ping
                ImGui::Text("%s", server.display_host.ToCStr());   ImGui::NextColumn();

                ImGui::PopID();
            }
            ImGui::Columns(1);
            ImGui::EndChild(); // End of scroll area

            // Simple join button
            if (m_selected_item != -1)
            {
                if (ImGui::Button("Join", ImVec2(200.f, 0.f)))
                {
                    App::mp_server_host.SetActive(m_serverlist_data->servers[m_selected_item].net_host);
                    App::mp_server_port.SetActive(m_serverlist_data->servers[m_selected_item].net_port);
                    App::mp_state.SetPending(MpState::CONNECTED);
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

    ImGui::End();
}

void RoR::GUI::MultiplayerSelector::RefreshServerlist()
{
#if defined(USE_CURL)
    m_serverlist_data.reset();
    m_selected_item = -1;
    m_is_refreshing = true;
    std::packaged_task<MpServerlistData*(std::string)> task(FetchServerlist);
    m_serverlist_future = task.get_future();
    std::thread(std::move(task), App::mp_portal_url.GetActive()).detach(); // launch on a thread
#endif // defined(USE_CURL)
}

bool RoR::GUI::MultiplayerSelector::IsRefreshThreadRunning() const
{
    return m_is_refreshing;
}

void RoR::GUI::MultiplayerSelector::CheckAndProcessRefreshResult()
{
    std::future_status status = m_serverlist_future.wait_for(std::chrono::seconds(0));
    if (status != std::future_status::ready)
    {
        return;
    }

    m_serverlist_data = std::unique_ptr<MpServerlistData>(m_serverlist_future.get());
    m_is_refreshing = false;
    return;
}

void RoR::GUI::MultiplayerSelector::SetVisible(bool visible)
{
    m_is_visible = visible;
    if (visible && (m_serverlist_data == nullptr)) // Do an initial refresh
    {
        this->RefreshServerlist();
    }
}
