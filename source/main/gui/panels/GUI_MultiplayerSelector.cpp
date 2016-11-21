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
#include "rornet.h"
#include "Language.h"
#include "GUIManager.h"
#include "Application.h"
#include "MainThread.h"

#ifdef USE_JSONCPP
#include "json/json.h"
#endif //USE_JSONCPP

#include <MyGUI.h>
#include <thread>
#include <future>

#ifdef USE_CURL
#   include <stdio.h>
#   include <curl/curl.h>
#   include <curl/easy.h>
#endif //USE_CURL

namespace RoR {
namespace GUI {

const MyGUI::Colour status_updating_color(1.f, 0.832031f, 0.f);
const MyGUI::Colour status_failure_color(1.f, 0.175439f, 0.175439f);

#define CLASS        MultiplayerSelector
#define MAIN_WIDGET  ((MyGUI::Window*)mMainWidget)

/// Private impl. to minimze header deps ("json.h", <thread>, <future>)
struct ServerlistData
{
#ifdef USE_JSONCPP
    std::future<Json::Value> future_json;
#endif //USE_JSONCPP
};

// From example: https://gist.github.com/whoshuu/2dc858b8730079602044
size_t CurlWriteFunc(void *ptr, size_t size, size_t nmemb, std::string* data) {
    data->append((char*) ptr, size * nmemb);
    return size * nmemb;
}

#if defined(USE_CURL) && defined(USE_JSONCPP)
/// Returns JSON in format { "success":bool, "message":str, "data":$response_string }
Json::Value FetchServerlist()
{
    std::string response_payload;
    std::string response_header;
    long        response_code = 0;

    CURL *curl = curl_easy_init();
    curl_easy_setopt(curl, CURLOPT_URL,           "http://multiplayer.rigsofrods.org/server-list?json=true");
    curl_easy_setopt(curl, CURLOPT_NOPROGRESS,    1L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, CurlWriteFunc);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA,     &response_payload);
    curl_easy_setopt(curl, CURLOPT_HEADERDATA,    &response_header);

    curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
    curl_easy_cleanup(curl);
    curl = nullptr;

    Json::Value result(Json::objectValue);

    if (response_code != 200)
    {
        Ogre::LogManager::getSingleton().stream() 
            << "[RoR|Multiplayer] Failed to retrieve serverlist; HTTP status code: " << response_code;
        result["success"] = false;
        result["message"] = "Error connecting to server :(";
        return result;
    }

    Json::CharReaderBuilder b;
    Json::CharReader* reader = b.newCharReader(); // Java, anyone?
    const char* payload = response_payload.c_str();
    const char* payload_end = payload + response_payload.size();
    std::string parse_errors;
    Json::Value parse_output;
    if (!reader->parse(payload, payload_end, &parse_output, &parse_errors))
    {
        Ogre::LogManager::getSingleton().stream() 
            << "[RoR|Multiplayer] Error parsing serverlist JSON, messages: " << parse_errors;
        result["success"] = false;
        result["message"] = "Server returned invalid data :(";
        return result;
    }

    result["success"] = true;
    result["data"] = parse_output;
    return result;
}
#endif // defined(USE_CURL) && defined(USE_JSONCPP)

CLASS::CLASS() :
    m_serverlist_data(nullptr),
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

#if defined(USE_CURL) && defined(USE_JSONCPP)
    m_refresh_button->eventMouseButtonClick += MyGUI::newDelegate(this, &CLASS::CallbackRefreshOnlineBtnPress);
#else
    m_refresh_button->setEnabled(false);
    m_status_label->setVisible(true);
    m_status_label->setCaption("---- Serverlist not supported in this version ----");
#endif

    CenterToScreen();

    MAIN_WIDGET->setVisible(false);
}

void CLASS::SetVisible(bool visible)
{
    MAIN_WIDGET->setVisible(visible);
    if (visible && m_serverlist_data == nullptr)
    {
        this->RefreshServerlist();
    }
}

void CLASS::RefreshServerlist()
{
#if defined(USE_CURL) && defined(USE_JSONCPP)
    m_servers_list->removeAllItems();
    m_status_label->setVisible(true);
    m_status_label->setCaption("* * * * UPDATING * * * *");
    m_status_label->setTextColour(status_updating_color);
    m_refresh_button->setEnabled(false);
    m_is_refreshing = true;
    if (m_serverlist_data == nullptr)
    {
        m_serverlist_data = new ServerlistData;
    }
    std::packaged_task<Json::Value()> task(FetchServerlist);
    m_serverlist_data->future_json = task.get_future();
    std::thread(std::move(task)).detach(); // launch on a thread
#endif // defined(USE_CURL) && defined(USE_JSONCPP)
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
#ifdef USE_JSONCPP
    std::future_status status = m_serverlist_data->future_json.wait_for(std::chrono::seconds(0));
    if (status != std::future_status::ready)
    {
        return;
    }

    auto json = m_serverlist_data->future_json.get();
    if (!json.isObject() || json["success"] != true || !json["data"].isArray())
    {
        std::string message = (json["message"].isString()) ? json["message"].asString() : "Could not connect to server :(";
        m_status_label->setTextColour(status_failure_color);
        m_status_label->setCaption(message);
        m_refresh_button->setEnabled(true);
        m_is_refreshing = false;
        return;
    }

    int num_rows = json["data"].size();
    for (int i = 0; i < num_rows; ++i)
    {
        auto row = json["data"][i];
        m_servers_list->addItem(row["name"].asString(), row); // Only fills 1st column. Userdata = JSON
        m_servers_list->setSubItemNameAt(1, i, row["terrain-name"].asString()); // Fill other columns
        m_servers_list->setSubItemNameAt(2, i, row["current-users"].asString() + " / " +row["max-clients"].asString());
        m_servers_list->setSubItemNameAt(3, i, row["ip"].asString());
    }

    m_status_label->setVisible(false);
    m_refresh_button->setEnabled(true);
    m_is_refreshing = false;
#endif // USE_JSONCPP
}

void CLASS::ServerlistJoin(size_t sel_index)
{
#ifdef USE_JSONCPP
    if (sel_index != MyGUI::ITEM_NONE)
    {
        Json::Value& json = *m_servers_list->getItemDataAt<Json::Value>(sel_index);
        App::SetMpServerHost(json["ip"].asString());
        App::SetMpServerPort(json["port"].asInt());
        App::GetMainThreadLogic()->JoinMultiplayerServer();
    }
#endif // USE_JSONCPP
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
    App::GetMainThreadLogic()->JoinMultiplayerServer();
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
