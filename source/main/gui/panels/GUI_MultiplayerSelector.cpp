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
/// @date   11/2014

#include "GUI_MultiplayerSelector.h"

#include "RoRPrerequisites.h"
#include "Utils.h"
#include "RoRVersion.h"
#include "RoRnet.h"
#include "Language.h"
#include "GUIManager.h"
#include "Application.h"
#include "MainMenu.h"

#include <MyGUI.h>
#include <rapidjson/document.h>
#include <string>
#include <thread>
#include <future>

#ifdef USE_CURL
#   include <stdio.h>
#   include <curl/curl.h>
#   include <curl/easy.h>
#endif //USE_CURL

#if defined(_MSC_VER) && defined(GetObject) // This MS Windows macro from <wingdi.h> (Windows Kit 8.1) clashes with RapidJSON
#   undef GetObject
#endif

namespace RoR {
namespace GUI {

const MyGUI::Colour status_updating_color(1.f, 0.832031f, 0.f);
const MyGUI::Colour status_failure_color(1.f, 0.175439f, 0.175439f);
const MyGUI::Colour status_emptylist_color(0.7f, 0.7f, 0.7f);

#define CLASS        MultiplayerSelector
#define MAIN_WIDGET  ((MyGUI::Window*)mMainWidget)

struct MpServerlistData
{
    struct ServerInfo
    {
        std::string display_name;
        std::string display_users;
        std::string display_host;
        std::string display_terrn;
        std::string net_host;
        int         net_port;
    };

    MpServerlistData(): success(false) {}

    std::vector<ServerInfo> servers;
    std::string             message;
    bool                    success;
};

// From example: https://gist.github.com/whoshuu/2dc858b8730079602044
size_t CurlWriteFunc(void *ptr, size_t size, size_t nmemb, std::string* data)
{
    data->append((char*) ptr, size * nmemb);
    return size * nmemb;
}

#if defined(USE_CURL)
MpServerlistData* FetchServerlist(std::string portal_url)
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

    MpServerlistData* res = new MpServerlistData();
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

CLASS::CLASS() :
    m_is_refreshing(false)
{
    MyGUI::WindowPtr win = dynamic_cast<MyGUI::WindowPtr>(mMainWidget);
    win->eventWindowButtonPressed += MyGUI::newDelegate(this, &CLASS::NotifyWindowButtonPressed); //The "X" button thing

    m_join_button->eventMouseButtonClick += MyGUI::newDelegate(this, &CLASS::CallbackJoinOnlineBtnPress);
    m_entertab_button_connect->eventMouseButtonClick += MyGUI::newDelegate(this, &CLASS::CallbackJoinDirectBtnPress);
    m_servers_list->eventListSelectAccept += MyGUI::newDelegate(this, &CLASS::CallbackJoinOnlineListItem);

    m_ror_net_ver->setCaptionWithReplacing(RORNET_VERSION);

    m_entertab_ip_editbox->setCaption(App::GetMpServerHost());
    m_entertab_port_editbox->setCaption(TOSTRING(App::GetMpServerPort()));

#if defined(USE_CURL)
    m_refresh_button->eventMouseButtonClick += MyGUI::newDelegate(this, &CLASS::CallbackRefreshOnlineBtnPress);
#else
    m_refresh_button->setEnabled(false);
    m_status_label->setVisible(true);
    m_status_label->setCaption("---- Serverlist not supported in this version ----");
#endif

    CenterToScreen();

    MAIN_WIDGET->setVisible(false);
}

CLASS::~CLASS()
{} // Must be defined here to have `std::unique_ptr<UndefinedClass>` in the header

void CLASS::SetVisible(bool visible)
{
    MAIN_WIDGET->setVisible(visible);
    if (visible && (m_serverlist_data == nullptr))
    {
        this->RefreshServerlist();
    }
}

void CLASS::RefreshServerlist()
{
#if defined(USE_CURL)
    m_serverlist_data.reset();
    m_servers_list->removeAllItems();
    m_status_label->setVisible(true);
    m_status_label->setCaption("* * * * UPDATING * * * *");
    m_status_label->setTextColour(status_updating_color);
    m_refresh_button->setEnabled(false);
    m_is_refreshing = true;
    std::packaged_task<MpServerlistData*(std::string)> task(FetchServerlist);
    m_serverlist_future = task.get_future();
    std::thread(std::move(task), App::GetMpPortalUrl()).detach(); // launch on a thread
#endif // defined(USE_CURL)
}

void CLASS::CenterToScreen()
{
    MyGUI::IntSize windowSize = MAIN_WIDGET->getSize();
    MyGUI::IntSize parentSize = MAIN_WIDGET->getParentSize();

    MAIN_WIDGET->setPosition((parentSize.width - windowSize.width) / 2, (parentSize.height - windowSize.height) / 2);
}

bool CLASS::IsVisible()
{
    return MAIN_WIDGET->getVisible();
}


void CLASS::CallbackRefreshOnlineBtnPress(MyGUI::WidgetPtr _sender)
{
    this->RefreshServerlist();
}

void CLASS::CallbackJoinOnlineListItem(MyGUI::MultiListBox* _sender, size_t index)
{
    this->ServerlistJoin(index);
}


void CLASS::CheckAndProcessRefreshResult()
{
    std::future_status status = m_serverlist_future.wait_for(std::chrono::seconds(0));
    if (status != std::future_status::ready)
    {
        return;
    }

    m_serverlist_data = std::unique_ptr<MpServerlistData>(m_serverlist_future.get());

    if (!m_serverlist_data->success)
    {
        if (m_serverlist_data->message.empty())
            m_status_label->setCaption("Could not connect to server :(");
        else
            m_status_label->setCaption(m_serverlist_data->message);

        m_refresh_button->setEnabled(true);
        m_is_refreshing = false;
        return;
    }

    size_t num_rows = m_serverlist_data->servers.size();
    if (num_rows == 0)
    {
        m_status_label->setTextColour(status_emptylist_color);
        m_status_label->setCaption("There are no available servers :/");
        m_refresh_button->setEnabled(true);
        m_is_refreshing = false;
        return;
    }

    for (size_t i = 0; i < num_rows; ++i)
    {
        MpServerlistData::ServerInfo& server = m_serverlist_data->servers[i];
        m_servers_list->addItem(server.display_name, i); // Only fills 1st column. Userdata = `size_t` index to `m_serverlist_data::servers` array
        m_servers_list->setSubItemNameAt(1, i, server.display_terrn); // Fill other columns
        m_servers_list->setSubItemNameAt(2, i, server.display_users);
        m_servers_list->setSubItemNameAt(3, i, server.display_host);
    }

    m_status_label->setVisible(false);
    m_refresh_button->setEnabled(true);
    m_is_refreshing = false;
}

void CLASS::ServerlistJoin(size_t sel_index)
{
    if (sel_index != MyGUI::ITEM_NONE)
    {
        size_t i = *m_servers_list->getItemDataAt<size_t>(sel_index);

        App::SetMpServerHost(m_serverlist_data->servers[i].net_host);
        App::SetMpServerPort(m_serverlist_data->servers[i].net_port);
        App::GetMainMenu()->JoinMultiplayerServer();
    }
}

void CLASS::CallbackJoinOnlineBtnPress(MyGUI::WidgetPtr _sender)
{
    this->ServerlistJoin(m_servers_list->getIndexSelected());
}

void CLASS::CallbackJoinDirectBtnPress(MyGUI::WidgetPtr _sender)
{
    MAIN_WIDGET->setVisibleSmooth(false);
    App::SetMpServerHost(m_entertab_ip_editbox->getCaption().asUTF8());
    App::SetMpServerPort(Ogre::StringConverter::parseInt(m_entertab_port_editbox->getCaption().asUTF8()));
    App::GetMainMenu()->JoinMultiplayerServer();
}

void CLASS::NotifyWindowButtonPressed(MyGUI::WidgetPtr _sender, const std::string& _name)
{
    if (_name == "close")
    {
        MAIN_WIDGET->setVisibleSmooth(false);
        if (App::GetActiveAppState() == App::APP_STATE_MAIN_MENU)
        {
            App::GetGuiManager()->SetVisible_GameMainMenu(true);
        }
    }
}

} // namespace RoR
} // namespace GUI
