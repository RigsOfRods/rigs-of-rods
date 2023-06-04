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
#include "ContentManager.h"
#include "GameContext.h"
#include "GUIManager.h"
#include "GUIUtils.h"
#include "RoRnet.h"
#include "RoRVersion.h"
#include "Language.h"

#include <imgui.h>
#include <rapidjson/document.h>
#include <fmt/core.h>
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

    CURLcode curl_result = curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);

    curl_easy_cleanup(curl);
    curl = nullptr;

    if (curl_result != CURLE_OK || response_code != 200)
    {
        Ogre::LogManager::getSingleton().stream() 
            << "[RoR|Multiplayer] Failed to retrieve serverlist;"
            << " Error: '" << curl_easy_strerror(curl_result) << "'; HTTP status code: " << response_code;

        CurlFailInfo* failinfo = new CurlFailInfo();
        failinfo->title = _LC("MultiplayerSelector", "Error connecting to server :(");
        failinfo->curl_result = curl_result;
        failinfo->http_response = response_code;

        App::GetGameContext()->PushMessage(
            Message(MSG_NET_REFRESH_SERVERLIST_FAILURE, failinfo));
        return;
    }

    rapidjson::Document j_data_doc;
    j_data_doc.Parse(response_payload.c_str());
    if (j_data_doc.HasParseError() || !j_data_doc.IsArray())
    {
        Ogre::LogManager::getSingleton().stream() 
            << "[RoR|Multiplayer] Error parsing serverlist JSON"; // TODO: Report the actual error
        App::GetGameContext()->PushMessage(
            Message(MSG_NET_REFRESH_SERVERLIST_FAILURE, _LC("MultiplayerSelector", "Server returned invalid data :(")));
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
        servers[i].display_passwd = servers[i].has_password ? _LC("MultiplayerSelector","Yes") : _LC("MultiplayerSelector","No");

        servers[i].display_host  = fmt::format("{}:{}", j_row["ip"].GetString(), j_row["port"].GetInt());
        servers[i].display_users = fmt::format("{} / {}", j_row["current-users"].GetInt(), j_row["max-clients"].GetInt());

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

MultiplayerSelector::MultiplayerSelector()
{
    snprintf(m_window_title, 100, "Multiplayer (Rigs of Rods %s | %s)", ROR_VERSION_STRING, RORNET_VERSION);
}

void MultiplayerSelector::Draw()
{
    int window_flags = ImGuiWindowFlags_NoCollapse;
    ImGui::SetNextWindowSize(ImVec2(750.f, 400.f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowPosCenter();
    bool keep_open = true;
    ImGui::Begin(m_window_title, &keep_open, window_flags);

    ImGui::BeginTabBar("GameSettingsTabs");

    if (ImGui::BeginTabItem(_LC("MultiplayerSelector", "Online (click to refresh)")))
    {
        if (ImGui::IsItemClicked())
        {
            this->StartAsyncRefresh();
        }
        this->DrawServerlistTab();
        ImGui::EndTabItem();
    }
    if (ImGui::BeginTabItem(_LC("MultiplayerSelector", "Direct IP")))
    {
        this->DrawDirectTab();
        ImGui::EndTabItem();
    }
    if (ImGui::BeginTabItem(_LC("MultiplayerSelector", "Settings")))
    {
        this->DrawSetupTab();
        ImGui::EndTabItem();
    }

    ImGui::EndTabBar();

    ImGui::End();
    if (!keep_open)
    {
        this->SetVisible(false);
    }
}

void MultiplayerSelector::DrawSetupTab()
{
    ImGui::PushID("setup");

    DrawGCheckbox(App::mp_join_on_startup,    _LC("MultiplayerSelector", "Auto connect"));
    DrawGCheckbox(App::mp_chat_auto_hide,     _LC("MultiplayerSelector", "Auto hide chat"));
    DrawGCheckbox(App::mp_hide_net_labels,    _LC("MultiplayerSelector", "Hide net labels"));
    DrawGCheckbox(App::mp_hide_own_net_label, _LC("MultiplayerSelector", "Hide own net label"));
    DrawGCheckbox(App::mp_pseudo_collisions,  _LC("MultiplayerSelector", "Multiplayer collisions"));
    DrawGCheckbox(App::mp_cyclethru_net_actors, _LC("MultiplayerSelector", "Include remote actors when cycling via hotkeys"));
    this->DrawCharacterOverrideCfg();

    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + BUTTONS_EXTRA_SPACE);
    ImGui::Separator();

    ImGui::PushItemWidth(250.f);

    DrawGTextEdit(App::mp_player_name,        _LC("MultiplayerSelector", "Player nickname"), m_player_name_buf);
    DrawGTextEdit(App::mp_server_password,    _LC("MultiplayerSelector", "Default server password"), m_password_buf);

    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + BUTTONS_EXTRA_SPACE);
    ImGui::Separator();

    DrawGTextEdit(App::mp_player_token,       _LC("MultiplayerSelector", "User token"), m_user_token_buf);
    ImGui::PopItemWidth();

    ImGui::PopID();
}

void MultiplayerSelector::DrawDirectTab()
{
    ImGui::PushID("direct");

    ImGui::PushItemWidth(250.f);
    DrawGTextEdit(App::mp_server_host,  _LC("MultiplayerSelector", "Server host"), m_server_host_buf);
    DrawGIntBox(App::mp_server_port,    _LC("MultiplayerSelector", "Server port"));
    ImGui::InputText(                   _LC("MultiplayerSelector", "Server password"), m_password_buf.GetBuffer(), m_password_buf.GetCapacity());
    ImGui::PopItemWidth();

    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + BUTTONS_EXTRA_SPACE);
    if (ImGui::Button(_LC("MultiplayerSelector", "Join")))
    {
        App::mp_server_password->setStr(m_password_buf.GetBuffer());
        App::GetGameContext()->PushMessage(Message(MSG_NET_CONNECT_REQUESTED));
    }

    ImGui::PopID();
}


void MultiplayerSelector::DrawServerlistTab()
{
    GUIManager::GuiTheme const& theme = App::GetGuiManager()->GetTheme();

    // LOAD RESOURCES
    if (!m_lock_icon)
    {
        try
        {
            App::GetContentManager()->AddResourcePack(ContentManager::ResourcePack::FAMICONS);
            m_lock_icon = Ogre::TextureManager::getSingleton().load(
                "lock.png", ContentManager::ResourcePack::FAMICONS.resource_group_name);
        }
        catch (...) {} // Logged by OGRE
    }

    if (m_show_spinner)
    {
        float spinner_size = 25.f;
        ImGui::SetCursorPosX((ImGui::GetWindowSize().x / 2.f) - spinner_size);
        ImGui::SetCursorPosY((ImGui::GetWindowSize().y / 2.f) - spinner_size);
        LoadingIndicatorCircle("spinner", spinner_size, theme.value_blue_text_color, theme.value_blue_text_color, 10, 10);
    }

    // DRAW SERVERLIST TABLE
    if (m_draw_table)
    {
        // Setup serverlist table ... the scroll area
        const float table_height = ImGui::GetWindowHeight()
            - ((2.f * ImGui::GetStyle().WindowPadding.y) + (3.f * ImGui::GetItemsLineHeightWithSpacing())
                + ImGui::GetStyle().ItemSpacing.y);
        ImGui::BeginChild("scrolling", ImVec2(0.f, table_height), false);
        // ... and the table itself
        const float table_width = ImGui::GetWindowContentRegionWidth();
        ImGui::Columns(5, "mp-selector-columns");         // Col #0: Server name (and lock icon)
        ImGui::SetColumnOffset(1, 0.36f * table_width);   // Col #1: Terrain name
        ImGui::SetColumnOffset(2, 0.67f * table_width);   // Col #2: Users/Max
        ImGui::SetColumnOffset(3, 0.74f * table_width);   // Col #3: Version
        ImGui::SetColumnOffset(4, 0.82f * table_width);   // Col #4: Host/Port
        // Draw table header
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + TABLE_PADDING_LEFT);
        DrawTableHeader(_LC("MultiplayerSelector", "Name"));
        DrawTableHeader(_LC("MultiplayerSelector", "Terrain"));
        DrawTableHeader(_LC("MultiplayerSelector", "Users"));
        DrawTableHeader(_LC("MultiplayerSelector", "Version"));
        DrawTableHeader(_LC("MultiplayerSelector", "Host/Port"));
        ImGui::Separator();
        // Draw table body
        for (int i = 0; i < (int)m_serverlist_data.size(); i++)
        {
            ImGui::PushID(i);

            // First column (name)
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + TABLE_PADDING_LEFT);
            MpServerInfo& server = m_serverlist_data[i];
            if (ImGui::Selectable(server.display_name.c_str(), m_selected_item == i, ImGuiSelectableFlags_SpanAllColumns))
            {
                // Update selection
                m_selected_item = i;
            }
            if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0))
            {
                // Handle left doubleclick
                App::mp_server_password->setStr(m_password_buf.GetBuffer());
                App::mp_server_host->setStr(server.net_host.c_str());
                App::mp_server_port->setVal(server.net_port);
                App::GetGameContext()->PushMessage(Message(MSG_NET_CONNECT_REQUESTED));
            }
            if (server.has_password && m_lock_icon)
            {
                // Draw lock icon for password-protected servers.
                ImGui::SameLine();
                ImGui::Image(reinterpret_cast<ImTextureID>(m_lock_icon->getHandle()), ImVec2(16, 16));
            }
            ImGui::NextColumn();

            bool compatible = (server.net_version == RORNET_VERSION);
            ImVec4 version_color = compatible ? ImVec4(0.0f, 0.9f, 0.0f, 1.0f) : ImVec4(0.9f, 0.0f, 0.0f, 1.0f);

            // Other collumns
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
            if (ImGui::Button(_LC("MultiplayerSelector", "Join"), ImVec2(200.f, 0.f)))
            {
                App::mp_server_password->setStr(m_password_buf.GetBuffer());
                App::mp_server_host->setStr(server.net_host.c_str());
                App::mp_server_port->setVal(server.net_port);
                App::GetGameContext()->PushMessage(Message(MSG_NET_CONNECT_REQUESTED));
            }
            if (server.has_password)
            {
                // TODO: Find out why this is always visible ~ ulteq 01/2019
                ImGui::SameLine();
                ImGui::PushItemWidth(250.f);
                ImGui::InputText(_LC("MultiplayerSelector", "Server password"), m_password_buf.GetBuffer(), m_password_buf.GetCapacity());
                ImGui::PopItemWidth();
            }
        }
    }

    // DRAW CENTERED LABEL
    if (m_serverlist_msg != "")
    {
        const ImVec2 label_size = ImGui::CalcTextSize(m_serverlist_msg.c_str());
        float y = (ImGui::GetWindowSize().y / 2.f) - (ImGui::GetTextLineHeight() / 2.f);
        ImGui::SetCursorPosX((ImGui::GetWindowSize().x / 2.f) - (label_size.x / 2.f));
        ImGui::SetCursorPosY(y);
        ImGui::TextColored(m_serverlist_msg_color, "%s", m_serverlist_msg.c_str());
        y += ImGui::GetTextLineHeightWithSpacing();

        if (m_serverlist_curlmsg != "")
        {
            const ImVec2 detail_size = ImGui::CalcTextSize(m_serverlist_curlmsg.c_str());
            ImGui::SetCursorPosX((ImGui::GetWindowSize().x / 2.f) - (detail_size.x / 2.f));
            ImGui::SetCursorPosY(y);
            ImGui::TextDisabled("%s", m_serverlist_curlmsg.c_str());
            y += ImGui::GetTextLineHeight();
        }

        if (m_serverlist_httpmsg != "")
        {
            const ImVec2 detail_size = ImGui::CalcTextSize(m_serverlist_httpmsg.c_str());
            ImGui::SetCursorPosX((ImGui::GetWindowSize().x / 2.f) - (detail_size.x / 2.f));
            ImGui::SetCursorPosY(y);
            ImGui::TextDisabled("%s", m_serverlist_httpmsg.c_str());
        }
    }
}

void MultiplayerSelector::StartAsyncRefresh()
{
#if defined(USE_CURL)
    m_show_spinner = true;
    m_draw_table = false;
    m_serverlist_data.clear();
    m_selected_item = -1;
    m_serverlist_msg = "";
    std::packaged_task<void(std::string)> task(FetchServerlist);
    std::thread(std::move(task), App::mp_api_url->getStr()).detach(); // launch on a thread
#endif // defined(USE_CURL)
}

void MultiplayerSelector::SetVisible(bool visible)
{
    m_is_visible = visible;
    if (visible && m_serverlist_data.size() == 0) // Do an initial refresh
    {
        this->StartAsyncRefresh();
        m_password_buf = App::mp_server_password->getStr();
    }
    else if (!visible && App::app_state->getEnum<AppState>() == AppState::MAIN_MENU)
    {
        App::GetGuiManager()->GameMainMenu.SetVisible(true);
    }
}

void MultiplayerSelector::DisplayRefreshFailed(CurlFailInfo* failinfo)
{
    m_show_spinner = false;
    m_serverlist_msg = failinfo->title;
    m_serverlist_msg_color = App::GetGuiManager()->GetTheme().error_text_color;
    m_draw_table = false;
    if (failinfo->curl_result != CURLE_OK)
        m_serverlist_curlmsg = curl_easy_strerror(failinfo->curl_result);
    if (failinfo->http_response != 0)
        m_serverlist_httpmsg = fmt::format(_L("HTTP code: {}"), failinfo->http_response);
}

void MultiplayerSelector::UpdateServerlist(MpServerInfoVec* data)
{
    m_show_spinner = false;
    m_serverlist_data = *data;
    m_draw_table = true;
    if (m_serverlist_data.empty())
    {
        m_serverlist_msg = _LC("MultiplayerSelector", "There are no available servers :/");
        m_serverlist_msg_color = App::GetGuiManager()->GetTheme().no_entries_text_color;
    }
    else
    {
        m_serverlist_msg = "";
    }
}

void MultiplayerSelector::DrawCharacterOverrideCfg()
{
    // Character
    ImGui::Separator();
    ImGui::TextDisabled("%s:", _LC("MultiplayerSelector", "Override character"));
    ImGui::SameLine();
    ImGui::Text("%s", App::mp_override_character->getStr().c_str());
    ImGui::SameLine();
    if (ImGui::SmallButton(_LC("MultiplayerSelector", "Select")))
    {
        LoaderType* payload = new LoaderType(LoaderType::LT_CharacterMP);
        App::GetGameContext()->PushMessage(Message(MSG_GUI_OPEN_SELECTOR_REQUESTED, (void*)payload));
    }
    ImGui::SameLine();
    if (ImGui::SmallButton(_LC("MultiplayerSelector", "Clear")))
    {
        App::mp_override_character->setStr("");
    }
}

