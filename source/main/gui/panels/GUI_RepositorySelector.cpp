/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2013-2021 Petr Ohlidal

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

/// @file   GUI_RepositorySelector.cpp
/// @author Rafael Galvan, 04/2021
/// @author Petr Ohlidal, 2022
/// @author tritonas00, 2022

#include "GUI_RepositorySelector.h"

#include "Application.h"
#include "BBDocument.h"
#include "GameContext.h"
#include "AppContext.h"
#include "Console.h"
#include "ContentManager.h"
#include "GUIManager.h"
#include "GUIUtils.h"
#include "Language.h"
#include "PlatformUtils.h"
#include "RoRVersion.h"

#include <imgui.h>
#include <imgui_internal.h>
#include <rapidjson/document.h>
#include <vector>
#include <fmt/core.h>
#include <stdio.h>
#include <OgreFileSystemLayer.h>

#ifdef USE_CURL
#   include <curl/curl.h>
#   include <curl/easy.h>
#endif //USE_CURL

#if defined(_MSC_VER) && defined(GetObject) // This MS Windows macro from <wingdi.h> (Windows Kit 8.1) clashes with RapidJSON
#   undef GetObject
#endif

using namespace RoR;
using namespace GUI;
using namespace bbcpp; // See 'BBDocument.h'

#if defined(USE_CURL)

static size_t CurlWriteFunc(void *ptr, size_t size, size_t nmemb, std::string* data)
{
    data->append((char*)ptr, size * nmemb);
    return size * nmemb;
}

struct RepoProgressContext
{
    std::string filename;
    double old_perc = 0;
};

static size_t CurlProgressFunc(void* ptr, double filesize_B, double downloaded_B)
{
    // Ensure that the file to be downloaded is not empty because that would cause a division by zero error later on
    if (filesize_B <= 0.0)
    {
        return 0;
    }

    RepoProgressContext* context = (RepoProgressContext*)ptr;

    double perc = (downloaded_B / filesize_B) * 100;

    if (perc > context->old_perc)
    {
        RoR::Message m(MSG_GUI_DOWNLOAD_PROGRESS);
        m.payload = reinterpret_cast<void*>(new int(perc));
        m.description = fmt::format("{} {}\n{}: {:.2f}{}\n{}: {:.2f}{}", "Downloading", context->filename, "File size", filesize_B/(1024 * 1024), "MB", "Downloaded", downloaded_B/(1024 * 1024), "MB");
        App::GetGameContext()->PushMessage(m);
    }

    context->old_perc = perc;

    // If you don't return 0, the transfer will be aborted - see the documentation
    return 0;
}

static size_t CurlOgreDataStreamWriteFunc(char* data_ptr, size_t _unused, size_t data_length, void* userdata)
{
    Ogre::DataStream* ogre_datastream = static_cast<Ogre::DataStream*>(userdata);
    if (data_length > 0 && ogre_datastream->isWriteable())
    {
        return ogre_datastream->write((const void*)data_ptr, data_length);
    }
    else
    {
        return 0;
    }
}

std::vector<GUI::ResourceCategories> GetResourceCategories(std::string portal_url)
{
    std::string repolist_url = portal_url + "/resource-categories";
    std::string response_payload;
    std::string response_header;
    long response_code = 0;
    std::string user_agent = fmt::format("{}/{}", "Rigs of Rods Client", ROR_VERSION_STRING);

    CURL *curl = curl_easy_init();
    curl_easy_setopt(curl, CURLOPT_URL, repolist_url.c_str());
    curl_easy_setopt(curl, CURLOPT_IPRESOLVE, CURL_IPRESOLVE_V4);
#ifdef _WIN32
    curl_easy_setopt(curl, CURLOPT_SSL_OPTIONS, CURLSSLOPT_NATIVE_CA);
#endif // _WIN32
    curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "gzip");
    curl_easy_setopt(curl, CURLOPT_USERAGENT, user_agent.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, CurlWriteFunc);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_payload);
    curl_easy_setopt(curl, CURLOPT_HEADERDATA, &response_header);

    CURLcode curl_result = curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);

    curl_easy_cleanup(curl);
    curl = nullptr;

    std::vector<GUI::ResourceCategories> cat;
    if (curl_result != CURLE_OK || response_code != 200)
    {
        Ogre::LogManager::getSingleton().stream()
            << "[RoR|Repository] Failed to retrieve category list;"
            << " Error: '" << curl_easy_strerror(curl_result) << "'; HTTP status code: " << response_code;
        return cat;
    }

    rapidjson::Document j_data_doc;
    j_data_doc.Parse(response_payload.c_str());
    
    rapidjson::Value& j_resp_body = j_data_doc["categories"];
    size_t num_rows = j_resp_body.GetArray().Size();
    cat.resize(num_rows);
    for (size_t i = 0; i < num_rows; i++)
    {
        rapidjson::Value& j_row = j_resp_body[static_cast<rapidjson::SizeType>(i)];

        cat[i].title = j_row["title"].GetString();
        cat[i].resource_category_id = j_row["resource_category_id"].GetInt();
        cat[i].resource_count = j_row["resource_count"].GetInt();
        cat[i].description = j_row["description"].GetString();
        cat[i].display_order = j_row["display_order"].GetInt();
    }

    return cat;
}

void GetResources(std::string portal_url)
{
    std::string repolist_url = portal_url + "/resources";
    std::string response_payload;
    std::string response_header;
    long response_code = 0;
    std::string user_agent = fmt::format("{}/{}", "Rigs of Rods Client", ROR_VERSION_STRING);

    CURL *curl = curl_easy_init();
    curl_easy_setopt(curl, CURLOPT_URL, repolist_url.c_str());
    curl_easy_setopt(curl, CURLOPT_IPRESOLVE, CURL_IPRESOLVE_V4);
#ifdef _WIN32
    curl_easy_setopt(curl, CURLOPT_SSL_OPTIONS, CURLSSLOPT_NATIVE_CA);
#endif // _WIN32
    curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "gzip");
    curl_easy_setopt(curl, CURLOPT_USERAGENT, user_agent.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, CurlWriteFunc);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_payload);
    curl_easy_setopt(curl, CURLOPT_HEADERDATA, &response_header);

    CURLcode curl_result = curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);

    curl_easy_cleanup(curl);
    curl = nullptr;

    if (curl_result != CURLE_OK || response_code != 200)
    {
        Ogre::LogManager::getSingleton().stream()
            << "[RoR|Repository] Failed to retrieve repolist;"
            << " Error: '"<< curl_easy_strerror(curl_result) << "'; HTTP status code: " << response_code;

        CurlFailInfo* failinfo = new CurlFailInfo();
        failinfo->title = _LC("RepositorySelector", "Could not connect to server. Please check your connection.");
        failinfo->curl_result = curl_result;
        failinfo->http_response = response_code;

        App::GetGameContext()->PushMessage(
                Message(MSG_NET_REFRESH_REPOLIST_FAILURE, failinfo));
        return;
    }

    rapidjson::Document j_data_doc;
    j_data_doc.Parse(response_payload.c_str());
    if (j_data_doc.HasParseError() || !j_data_doc.IsObject())
    {
        Ogre::LogManager::getSingleton().stream()
                << "[RoR|Repository] Error parsing repolist JSON, code: " << j_data_doc.GetParseError();
        App::GetGameContext()->PushMessage(
                Message(MSG_NET_REFRESH_REPOLIST_FAILURE, _LC("RepositorySelector", "Received malformed data. Please try again.")));
        return;
    }

    GUI::ResourcesCollection* cdata_ptr = new GUI::ResourcesCollection();

    std::vector<GUI::ResourceItem> resc;
    rapidjson::Value& j_resp_body = j_data_doc["resources"];
    size_t num_rows = j_resp_body.GetArray().Size();
    resc.resize(num_rows);

    for (size_t i = 0; i < num_rows; i++)
    {
        rapidjson::Value& j_row = j_resp_body[static_cast<rapidjson::SizeType>(i)];

        resc[i].title = j_row["title"].GetString();
        resc[i].tag_line = j_row["tag_line"].GetString();
        resc[i].resource_id = j_row["resource_id"].GetInt();
        resc[i].download_count = j_row["download_count"].GetInt();
        resc[i].last_update = j_row["last_update"].GetInt();
        resc[i].resource_category_id = j_row["resource_category_id"].GetInt();
        resc[i].icon_url = j_row["icon_url"].GetString();
        resc[i].rating_avg = j_row["rating_avg"].GetFloat();
        resc[i].rating_count = j_row["rating_count"].GetInt();
        resc[i].version = j_row["version"].GetString();
        resc[i].authors = j_row["custom_fields"]["authors"].GetString();
        resc[i].view_url = j_row["view_url"].GetString();
        resc[i].resource_date = j_row["resource_date"].GetInt();
        resc[i].view_count = j_row["view_count"].GetInt();
        resc[i].preview_tex = Ogre::TexturePtr(); // null
        // NOTE: description is stripped here for bandwidth reasons - fetched separately from individual resources.
    }

    cdata_ptr->items = resc;
    cdata_ptr->categories = GetResourceCategories(portal_url);

    App::GetGameContext()->PushMessage(
            Message(MSG_NET_REFRESH_REPOLIST_SUCCESS, (void*)cdata_ptr));
}

void GetResourceFiles(std::string portal_url, int resource_id)
{
    std::string response_payload;
    std::string resource_url = portal_url + "/resources/" + std::to_string(resource_id);
    std::string user_agent = fmt::format("{}/{}", "Rigs of Rods Client", ROR_VERSION_STRING);
    long response_code = 0;

    CURL *curl = curl_easy_init();
    curl_easy_setopt(curl, CURLOPT_URL, resource_url.c_str());
    curl_easy_setopt(curl, CURLOPT_IPRESOLVE, CURL_IPRESOLVE_V4);
#ifdef _WIN32
    curl_easy_setopt(curl, CURLOPT_SSL_OPTIONS, CURLSSLOPT_NATIVE_CA);
#endif // _WIN32
    curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "gzip");
    curl_easy_setopt(curl, CURLOPT_USERAGENT, user_agent.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, CurlWriteFunc);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_payload);

    CURLcode curl_result = curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);

    curl_easy_cleanup(curl);
    curl = nullptr;

    if (curl_result != CURLE_OK || response_code != 200)
    {
        Ogre::LogManager::getSingleton().stream()
            << "[RoR|Repository] Failed to retrieve resource;"
            << " Error: '" << curl_easy_strerror(curl_result) << "'; HTTP status code: " << response_code;

        // FIXME: we need a FAILURE message for MSG_NET_OPEN_RESOURCE_SUCCESS
    }

    GUI::ResourcesCollection* cdata_ptr = new GUI::ResourcesCollection();

    rapidjson::Document j_data_doc;
    j_data_doc.Parse(response_payload.c_str());

    std::vector<GUI::ResourceFiles> resc;
    rapidjson::Value& j_resp_body = j_data_doc["resource"]["current_files"];
    size_t num_rows = j_resp_body.GetArray().Size();
    resc.resize(num_rows);

    for (size_t i = 0; i < num_rows; i++)
    {
        rapidjson::Value& j_row = j_resp_body[static_cast<rapidjson::SizeType>(i)];

        resc[i].id = j_row["id"].GetInt();
        resc[i].filename = j_row["filename"].GetString();
        resc[i].size = j_row["size"].GetInt();
    }

    cdata_ptr->files = resc;

    // Also pass on the description (via a dummy item)
    ResourceItem item;
    item.description = bbcpp::BBDocument::create();
    item.description->load(j_data_doc["resource"]["description"].GetString());
    item.resource_id = j_data_doc["resource"]["resource_id"].GetInt();
    cdata_ptr->items.push_back(item);

    App::GetGameContext()->PushMessage(
            Message(MSG_NET_OPEN_RESOURCE_SUCCESS, (void*)cdata_ptr));
}

void DownloadResourceFile(int resource_id, std::string filename, int id)
{
    RoR::Message m(MSG_GUI_DOWNLOAD_PROGRESS);
    int perc = 0;
    m.payload = reinterpret_cast<void*>(new int(perc));
    m.description = "Initialising...";
    App::GetGameContext()->PushMessage(m);

    std::string url = "https://forum.rigsofrods.org/resources/" + std::to_string(resource_id) + "/download?file=" + std::to_string(id);
    std::string path = PathCombine(App::sys_user_dir->getStr(), "mods");
    std::string file = PathCombine(path, filename);

    RepoProgressContext progress_context;
    progress_context.filename = filename;
    long response_code = 0;

    CURL *curl = curl_easy_init();
    try // We write using Ogre::DataStream which throws exceptions
    {
        // smart pointer - closes stream automatically
        Ogre::DataStreamPtr datastream = Ogre::ResourceGroupManager::getSingleton().createResource(file, RGN_THUMBNAILS);

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_IPRESOLVE, CURL_IPRESOLVE_V4);
#ifdef _WIN32
        curl_easy_setopt(curl, CURLOPT_SSL_OPTIONS, CURLSSLOPT_NATIVE_CA);
#endif // _WIN32
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, CurlOgreDataStreamWriteFunc);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, datastream.get());
        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, NULL); // Disable Internal CURL progressmeter
        curl_easy_setopt(curl, CURLOPT_PROGRESSDATA, &progress_context);
        curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION, CurlProgressFunc); // Use our progress window
        
        CURLcode curl_result = curl_easy_perform(curl);
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);

        if (curl_result != CURLE_OK || response_code != 200)
        {
            Ogre::LogManager::getSingleton().stream()
                << "[RoR|Repository] Failed to download resource;"
                << " Error: '" << curl_easy_strerror(curl_result) << "'; HTTP status code: " << response_code;

            // FIXME: we need a FAILURE message for MSG_GUI_DOWNLOAD_FINISHED
        }
    }
    catch (Ogre::Exception& oex)
    {
        App::GetConsole()->putMessage(
            Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_ERROR,
            fmt::format("Repository UI: cannot download file '{}' - {}",
                url, oex.getFullDescription()));
    }
    curl_easy_cleanup(curl);
    curl = nullptr;

    App::GetGameContext()->PushMessage(
            Message(MSG_GUI_DOWNLOAD_FINISHED));
}
#endif // defined(USE_CURL)

RepositorySelector::RepositorySelector()
{
    Ogre::WorkQueue* wq = Ogre::Root::getSingleton().getWorkQueue();
    m_ogre_workqueue_channel = wq->getChannel("RoR/RepoThumbnails");
    wq->addRequestHandler(m_ogre_workqueue_channel, this);
    wq->addResponseHandler(m_ogre_workqueue_channel, this);

    m_fallback_thumbnail = FetchIcon("ror.png");
}

RepositorySelector::~RepositorySelector()
{}

void RepositorySelector::Draw()
{
    GUIManager::GuiTheme const& theme = App::GetGuiManager()->GetTheme();

    ImGui::SetNextWindowSize(ImVec2((ImGui::GetIO().DisplaySize.x / 1.4), (ImGui::GetIO().DisplaySize.y / 1.2)), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowPosCenter(ImGuiCond_Appearing);
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;
    bool keep_open = true;
    Ogre::TexturePtr tex1 = FetchIcon("arrow_rotate_anticlockwise.png");
    Ogre::TexturePtr tex2 = FetchIcon("accept.png");
    Ogre::TexturePtr tex3 = FetchIcon("star.png");
    Ogre::TexturePtr tex4 = FetchIcon("arrow_left.png");

    ImGui::Begin(_LC("RepositorySelector", "Rigs of Rods Repository"), &keep_open, window_flags);

    if (m_resourceview_item_arraypos != RESOURCEITEMARRAYPOS_INVALID
        && ImGui::ImageButton(reinterpret_cast<ImTextureID>(tex4->getHandle()), ImVec2(16, 16)))
    {
        if (m_gallery_mode_attachment_id != -1)
        {
            m_gallery_mode_attachment_id = -1;
        }
        else
        {
            m_resourceview_item_arraypos = RESOURCEITEMARRAYPOS_INVALID;
        }
    }
    else if (m_resourceview_item_arraypos == RESOURCEITEMARRAYPOS_INVALID
        && ImGui::ImageButton(reinterpret_cast<ImTextureID>(tex1->getHandle()), ImVec2(16, 16)))
    {
        this->Refresh();
    }
    ImGui::SameLine();

    if (m_draw)
    {
        // Deactivate in resource view
        if (m_resourceview_item_arraypos != RESOURCEITEMARRAYPOS_INVALID)
        {
            ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
        }

        // Category dropdown
        ImGui::SetNextItemWidth(ImGui::GetWindowSize().x
            - 16   // refresh button width
            - 170  // search box width
            - 2*80 // sort + view menu width
            - 6*ImGui::GetStyle().ItemSpacing.x
            - 2*ImGui::GetStyle().WindowPadding.x);

        // Calculate items of every shown category
        int count = 0;
        for (int i = 0; i < m_data.categories.size(); i++)
        {
            // Skip non mod categories
            if (m_data.categories[i].resource_category_id >= 8 && m_data.categories[i].resource_category_id <= 13)
            {
                continue;
            }
            count += m_data.categories[i].resource_count;
        }

        // Fill "All" category
        if (m_current_category_id == 1)
        {
            m_current_category = "(" + std::to_string(count) + ") All";
            m_all_category_label = m_current_category;
        }

        if (ImGui::BeginCombo("##repo-selector-cat", m_current_category.c_str()))
        {
            if (ImGui::Selectable(m_all_category_label.c_str(), m_current_category_id == 1))
            {
                m_current_category = m_all_category_label;
                m_current_category_id = 1;
            }

            for (int i = 0; i < m_data.categories.size(); i++)
            {
                // Skip non mod categories
                if (m_data.categories[i].resource_category_id >= 8 && m_data.categories[i].resource_category_id <= 13)
                {
                    continue;
                }

                m_current_category_label = "(" + std::to_string(m_data.categories[i].resource_count) + ") " + m_data.categories[i].title;
                bool is_selected = (m_current_category_id == m_data.categories[i].resource_category_id);

                if (ImGui::Selectable(m_current_category_label.c_str(), is_selected))
                {
                    m_current_category = m_current_category_label;
                    m_current_category_id = m_data.categories[i].resource_category_id;
                }
                if (is_selected)
                {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }

        // Search box
        ImGui::SameLine();
        float searchbox_x = ImGui::GetCursorPosX();
        ImGui::SetNextItemWidth(170);
        float search_pos = ImGui::GetCursorPosX();
        ImGui::InputText("##Search", m_search_input.GetBuffer(), m_search_input.GetCapacity());

        // Sort dropdown
        ImGui::SameLine();
        ImGui::SetNextItemWidth(80);

        if (ImGui::BeginCombo("##repo-selector-sort", _LC("RepositorySelector", "Sort")))
        {
            if (ImGui::Selectable(_LC("RepositorySelector", "Last Update"), m_current_sort == "Last Update"))
            {
                m_current_sort = "Last Update";
                std::sort(m_data.items.begin(), m_data.items.end(), [](ResourceItem a, ResourceItem b) { return a.last_update > b.last_update; });
            }
            if (ImGui::Selectable(_LC("RepositorySelector", "Date Added"), m_current_sort == "Date Added"))
            {
                m_current_sort = "Date Added";
                std::sort(m_data.items.begin(), m_data.items.end(), [](ResourceItem a, ResourceItem b) { return a.resource_date > b.resource_date; });
            }
            if (ImGui::Selectable(_LC("RepositorySelector", "Title"), m_current_sort == "Title"))
            {
                m_current_sort = "Title";
                std::sort(m_data.items.begin(), m_data.items.end(), [](ResourceItem a, ResourceItem b) { return a.title < b.title; });
            }
            if (ImGui::Selectable(_LC("RepositorySelector", "Downloads"), m_current_sort == "Downloads"))
            {
                m_current_sort = "Downloads";
                std::sort(m_data.items.begin(), m_data.items.end(), [](ResourceItem a, ResourceItem b) { return a.download_count > b.download_count; });
            }
            if (ImGui::Selectable(_LC("RepositorySelector", "Rating"), m_current_sort == "Rating"))
            {
                m_current_sort = "Rating";
                std::sort(m_data.items.begin(), m_data.items.end(), [](ResourceItem a, ResourceItem b) { return a.rating_avg > b.rating_avg; });
            }
            if (ImGui::Selectable(_LC("RepositorySelector", "Rating Count"), m_current_sort == "Rating Count"))
            {
                m_current_sort = "Rating Count";
                std::sort(m_data.items.begin(), m_data.items.end(), [](ResourceItem a, ResourceItem b) { return a.rating_count > b.rating_count; });
            }
            ImGui::EndCombo();
        }

        // View mode dropdown
        ImGui::SameLine();
        ImGui::SetNextItemWidth(80);

        if (ImGui::BeginCombo("##repo-selector-view", _LC("RepositorySelector", "View")))
        {
            if (ImGui::Selectable(_LC("RepositorySelector", "List"), m_view_mode == "List"))
            {
                m_view_mode = "List";
            }
            if (ImGui::Selectable(_LC("RepositorySelector", "Compact"), m_view_mode == "Compact"))
            {
                m_view_mode = "Compact";
            }
            if (ImGui::Selectable(_LC("RepositorySelector", "Basic"), m_view_mode == "Basic"))
            {
                m_view_mode = "Basic";
            }
            ImGui::EndCombo();
        }

        // Search box default text
        if (m_search_input.IsEmpty())
        {
            ImGui::SameLine();
            ImGui::SetCursorPosX(search_pos + ImGui::GetStyle().ItemSpacing.x);
            ImGui::TextDisabled("%s", _LC("RepositorySelector", "Search Title, Author"));
        }

        if (m_resourceview_item_arraypos != RESOURCEITEMARRAYPOS_INVALID)
        {
            ImGui::PopItemFlag();
            ImGui::PopStyleVar();
        }

        const float table_height = ImGui::GetWindowHeight()
                - ((2.f * ImGui::GetStyle().WindowPadding.y) + (3.f * ImGui::GetItemsLineHeightWithSpacing())
                - ImGui::GetStyle().ItemSpacing.y);

        if (m_resourceview_item_arraypos != RESOURCEITEMARRAYPOS_INVALID)
        {
            this->DrawResourceView(searchbox_x);
        }
        else
        {

            float col0_width = 0.40f * ImGui::GetWindowContentRegionWidth();
            float col1_width = 0.15f * ImGui::GetWindowContentRegionWidth();
            float col2_width = 0.20f * ImGui::GetWindowContentRegionWidth();
            float col3_width = 0.10f * ImGui::GetWindowContentRegionWidth();

            if (m_view_mode == "Basic")
            {
                ImGui::Columns(5, "repo-selector-columns-basic-headers", false);
                ImGui::SetColumnWidth(0, col0_width + ImGui::GetStyle().ItemSpacing.x);
                ImGui::SetColumnWidth(1, col1_width);
                ImGui::SetColumnWidth(2, col2_width);
                ImGui::SetColumnWidth(3, col3_width);

                ImGui::TextDisabled("%s", _LC("RepositorySelector", "Title"));
                ImGui::NextColumn();
                ImGui::TextDisabled("%s", _LC("RepositorySelector", "Version"));
                ImGui::NextColumn();
                ImGui::TextDisabled("%s", _LC("RepositorySelector", "Last Update"));
                ImGui::NextColumn();
                ImGui::TextDisabled("%s", _LC("RepositorySelector", "Downloads"));
                ImGui::NextColumn();
                ImGui::TextDisabled("%s", _LC("RepositorySelector", "Rating"));
                ImGui::Columns(1);
            }

            // Scroll area
            ImGui::BeginChild("scrolling", ImVec2(0.f, table_height), false);

            if (m_view_mode == "List")
            {
                ImGui::Columns(2, "repo-selector-columns");
                ImGui::SetColumnWidth(0, 100.f);
                ImGui::Separator();
            }
            else if (m_view_mode == "Basic")
            {
                ImGui::Columns(5, "repo-selector-columns-basic");
                ImGui::SetColumnWidth(0, col0_width);
                ImGui::SetColumnWidth(1, col1_width);
                ImGui::SetColumnWidth(2, col2_width);
                ImGui::SetColumnWidth(3, col3_width);
                ImGui::Separator();
            }

            // Draw table body
            int num_drawn_items = 0;
            for (int i = 0; i < m_data.items.size(); i++)
            {
                // Skip items from non mod categories
                if (m_data.items[i].resource_category_id >= 8 && m_data.items[i].resource_category_id <= 13)
                {
                    continue;
                }

                if (m_data.items[i].resource_category_id == m_current_category_id || m_current_category_id == 1)
                {
                    // Simple search filter: convert both title/author and input to lowercase, if input not found in the title/author continue
                    std::string title = m_data.items[i].title;
                    for (auto& c : title)
                    {
                        c = tolower(c);
                    }
                    std::string author = m_data.items[i].authors;
                    for (auto& c : author)
                    {
                        c = tolower(c);
                    }
                    std::string search = m_search_input.GetBuffer();
                    for (auto& c : search)
                    {
                        c = tolower(c);
                    }
                    if (title.find(search) == std::string::npos && author.find(search) == std::string::npos)
                    {
                        continue;
                    }

                    ImGui::PushID(i);

                    if (m_view_mode == "List")
                    {
                        // Thumbnail
                        ImGui::SetCursorPosX(ImGui::GetCursorPosX() - ImGui::GetStyle().ItemSpacing.x);
                        const ImVec2 thumb_size = ImVec2(ImGui::GetColumnWidth() - ImGui::GetStyle().ItemSpacing.x, 96);
                        const float spinner_size = ImGui::GetColumnWidth() / 4;
                        const float spinner_cursor_x(((ImGui::GetColumnWidth() - ImGui::GetStyle().ItemSpacing.x) / 2.f) - spinner_size);
                        const float spinner_cursor_y(ImGui::GetCursorPosY() + 5 * ImGui::GetStyle().ItemSpacing.y);
                        this->DrawThumbnail(i, thumb_size, spinner_size, ImVec2(spinner_cursor_x, spinner_cursor_y));

                        float width = (ImGui::GetColumnWidth() + 90);
                        ImGui::NextColumn();

                        // Columns already colored, just add a light background
                        ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.17f, 0.17f, 0.17f, 0.90f));
                        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImGui::GetStyle().Colors[ImGuiCol_Header]);
                        ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.22f, 0.22f, 0.22f, 0.90f));

                        // Wrap a Selectable around the whole column
                        float orig_cursor_y = ImGui::GetCursorPosY();
                        std::string item_id = "##" + std::to_string(i);

                        if (ImGui::Selectable(item_id.c_str(), /*selected:*/false, 0, ImVec2(0, 100)))
                        {
                            m_resourceview_item_arraypos = i;
                            this->OpenResource(m_data.items[i].resource_id);
                        }

                        ImGui::SetCursorPosY(orig_cursor_y);
                        ImGui::PopStyleColor(3);

                        // Title, version
                        ImGui::Text("%s", m_data.items[i].title.c_str());
                        ImGui::SameLine();
                        ImGui::TextDisabled("%s", m_data.items[i].version.c_str());

                        // Rating
                        for (int i = 1; i <= 5; i++)
                        {
                            ImGui::SameLine();
                            ImGui::SetCursorPosX(ImGui::GetColumnWidth() + 16 * i);
                            ImGui::Image(reinterpret_cast<ImTextureID>(tex3->getHandle()), ImVec2(16, 16), ImVec2(0.f, 0.f), ImVec2(1.f, 1.f), ImVec4(1.f, 1.f, 1.f, 0.2f));
                        }

                        int rating = round(m_data.items[i].rating_avg);
                        for (int i = 1; i <= rating; i++)
                        {
                            ImGui::SameLine();
                            ImGui::SetCursorPosX(ImGui::GetColumnWidth() + 16 * i);
                            ImGui::Image(reinterpret_cast<ImTextureID>(tex3->getHandle()), ImVec2(16, 16));
                        }

                        // Authors, rating count, last update, download count, description
                        ImGui::TextDisabled("%s:", _LC("RepositorySelector", "Authors"));
                        ImGui::SameLine();
                        ImGui::SetCursorPosX(width);
                        ImGui::TextColored(theme.value_blue_text_color, "%s", m_data.items[i].authors.c_str());

                        ImGui::SameLine();
                        std::string rc = std::to_string(m_data.items[i].rating_count) + " ratings";
                        ImGui::SetCursorPosX(ImGui::GetColumnWidth() - (ImGui::CalcTextSize(rc.c_str()).x / 2) + 16 * 3.5);
                        ImGui::TextDisabled("%s", rc.c_str());

                        ImGui::TextDisabled("%s:", _LC("RepositorySelector", "Last Update"));
                        ImGui::SameLine();
                        ImGui::SetCursorPosX(width);
                        time_t rawtime = (const time_t)m_data.items[i].last_update;
                        ImGui::TextColored(theme.value_blue_text_color, "%s", asctime(gmtime(&rawtime)));

                        ImGui::TextDisabled("%s:", _LC("RepositorySelector", "Downloads"));
                        ImGui::SameLine();
                        ImGui::SetCursorPosX(width);
                        ImGui::TextColored(theme.value_blue_text_color, "%d", m_data.items[i].download_count);

                        ImGui::TextDisabled("%s:", _LC("RepositorySelector", "Description"));
                        ImGui::SameLine();
                        ImGui::SetCursorPosX(width);
                        ImGui::TextColored(theme.value_blue_text_color, "%s", m_data.items[i].tag_line.c_str());

                        ImGui::NextColumn();

                        ImGui::Separator();
                    }
                    else if (m_view_mode == "Compact")
                    {
                        float orig_cursor_x = ImGui::GetCursorPos().x;

                        // Calc box size: Draw 3 boxes per line, 2 for small resolutions
                        float box_width = (ImGui::GetIO().DisplaySize.x / 1.4) / 3;
                        if (ImGui::GetIO().DisplaySize.x <= 1280)
                        {
                            box_width = (ImGui::GetIO().DisplaySize.x / 1.4) / 2;
                        }

                        // Skip to new line if at least 50% of the box can't fit on current line.
                        if (orig_cursor_x > ImGui::GetWindowContentRegionMax().x - (box_width * 0.5))
                        {
                            // Unless this is the 1st line... not much to do with such narrow window.
                            if (num_drawn_items != 0)
                            {
                                ImGui::NewLine();
                            }
                        }

                        ImGui::BeginGroup();

                        ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.70f, 0.70f, 0.70f, 0.90f));
                        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImGui::GetStyle().Colors[ImGuiCol_Header]);
                        ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.90f, 0.90f, 0.90f, 0.90f));

                        // Wrap a Selectable around images + text
                        float orig_cursor_y = ImGui::GetCursorPosY();
                        std::string item_id = "##" + std::to_string(i);

                        if (ImGui::Selectable(item_id.c_str(), /*selected:*/false, 0, ImVec2(box_width - ImGui::GetStyle().ItemSpacing.x, 100)))
                        {
                            m_resourceview_item_arraypos = i;
                            this->OpenResource(m_data.items[i].resource_id);
                        }

                        // Add a light background
                        ImVec2 p_min = ImGui::GetItemRectMin();
                        ImVec2 p_max = ImGui::GetItemRectMax();
                        ImGui::GetWindowDrawList()->AddRectFilled(p_min, p_max, ImColor(ImVec4(0.15f, 0.15f, 0.15f, 0.90f)));

                        ImGui::SetCursorPosY(orig_cursor_y);
                        ImGui::PopStyleColor(3);

                        // Thumbnail
                        const ImVec2 thumbnail_size(76, 86);
                        const float spinner_size = 25;
                        const float spinner_cursor_x(ImGui::GetCursorPosX() + 2 * ImGui::GetStyle().ItemSpacing.x);
                        const float spinner_cursor_y(ImGui::GetCursorPosY() + 20);
                        this->DrawThumbnail(i, thumbnail_size, spinner_size, ImVec2(spinner_cursor_x, spinner_cursor_y));
                        if (!m_data.items[i].preview_tex)
                        {
                            ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 76 - (35 + spinner_size)); //adjustment after spinner
                        }

                        // Rating
                        float pos_y;
                        for (int i = 1; i <= 5; i++)
                        {
                            pos_y = ImGui::GetCursorPosY();
                            ImGui::Image(reinterpret_cast<ImTextureID>(tex3->getHandle()), ImVec2(11, 11), ImVec2(0.f, 0.f), ImVec2(1.f, 1.f), ImVec4(1.f, 1.f, 1.f, 0.2f));
                            if (i < 5) { ImGui::SameLine(); }
                        }

                        int rating = round(m_data.items[i].rating_avg);
                        if (rating >= 1)
                        {
                            for (int i = 1; i <= rating; i++)
                            {
                                ImGui::SetCursorPosY(pos_y);
                                ImGui::Image(reinterpret_cast<ImTextureID>(tex3->getHandle()), ImVec2(11, 11));
                                if (i < rating) { ImGui::SameLine(); }
                            }
                        }

                        // Move text top right of the image
                        ImGui::SetCursorPosX(ImGui::GetCursorPos().x + 86);
                        ImGui::SetCursorPosY(ImGui::GetCursorPos().y - 100);

                        // Trim the title, can be long
                        std::string tl = m_data.items[i].title;
                        if (ImGui::CalcTextSize(tl.c_str()).x > box_width / 12)
                        {
                            tl.resize(box_width / 12);
                            tl += "...";
                        }

                        // Title, version, last update, download count
                        ImGui::Text("%s", tl.c_str());

                        ImGui::SetCursorPosX(ImGui::GetCursorPos().x + 86);
                        ImGui::TextColored(theme.value_blue_text_color, "%s %s", _LC("RepositorySelector", "Version"), m_data.items[i].version.c_str());

                        ImGui::SetCursorPosX(ImGui::GetCursorPos().x + 86);
                        time_t rawtime = (const time_t)m_data.items[i].last_update;
                        ImGui::TextColored(theme.value_blue_text_color, "%s", asctime(gmtime(&rawtime)));

                        ImGui::SetCursorPosX(ImGui::GetCursorPos().x + 86);
                        ImGui::TextColored(theme.value_blue_text_color, "%s %d %s", _LC("RepositorySelector", "Downloaded"), m_data.items[i].download_count, _LC("RepositorySelector", "times"));

                        // Add space for next item
                        ImGui::SetCursorPosX(ImGui::GetCursorPos().x + box_width);
                        ImGui::SetCursorPosY(ImGui::GetCursorPos().y + (1.5f * ImGui::GetStyle().WindowPadding.y));

                        ImGui::EndGroup();
                        ImGui::SameLine();
                    }
                    else if (m_view_mode == "Basic")
                    {
                        // Columns already colored, just add a light background
                        ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.18f, 0.18f, 0.18f, 0.90f));
                        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImGui::GetStyle().Colors[ImGuiCol_Header]);
                        ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.22f, 0.22f, 0.22f, 0.90f));

                        // Wrap a Selectable around the whole column
                        std::string item_id = "##" + std::to_string(i);

                        if (ImGui::Selectable(item_id.c_str(), /*selected:*/false, ImGuiSelectableFlags_SpanAllColumns))
                        {
                            m_resourceview_item_arraypos = i;
                            this->OpenResource(m_data.items[i].resource_id);
                        }

                        ImGui::PopStyleColor(3);

                        // Draw columns
                        ImGui::SameLine();
                        ImGui::SetCursorPosX(ImGui::GetCursorPosX() - 2 * ImGui::GetStyle().ItemSpacing.x);
                        ImGui::Text("%s", m_data.items[i].title.c_str());

                        ImGui::NextColumn();

                        ImGui::TextColored(theme.value_blue_text_color, "%s", m_data.items[i].version.c_str());

                        ImGui::NextColumn();

                        time_t rawtime = (const time_t)m_data.items[i].last_update;
                        ImGui::TextColored(theme.value_blue_text_color, "%s", asctime(gmtime(&rawtime)));

                        ImGui::NextColumn();

                        ImGui::TextColored(theme.value_blue_text_color, "%d", m_data.items[i].download_count);

                        ImGui::NextColumn();

                        float pos_x = ImGui::GetCursorPosX();

                        // Rating
                        for (int i = 1; i <= 5; i++)
                        {
                            ImGui::Image(reinterpret_cast<ImTextureID>(tex3->getHandle()), ImVec2(16, 16), ImVec2(0.f, 0.f), ImVec2(1.f, 1.f), ImVec4(1.f, 1.f, 1.f, 0.2f));
                            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 16 * i);
                            ImGui::SameLine();
                        }
                        ImGui::SetCursorPosX(pos_x);

                        int rating = round(m_data.items[i].rating_avg);
                        for (int i = 1; i <= rating; i++)
                        {
                            ImGui::Image(reinterpret_cast<ImTextureID>(tex3->getHandle()), ImVec2(16, 16));
                            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 16 * i);
                            ImGui::SameLine();
                        }

                        ImGui::NextColumn();

                        ImGui::Separator();
                    }
                    ImGui::PopID();
                    num_drawn_items++;
                }
            }
            ImGui::EndChild();
        }
    }

    if (m_show_spinner)
    {
        float spinner_size = 27.f;
        ImGui::SetCursorPosX((ImGui::GetWindowSize().x / 2.f) - spinner_size);
        ImGui::SetCursorPosY((ImGui::GetWindowSize().y / 2.f) - spinner_size);
        LoadingIndicatorCircle("spinner", spinner_size, theme.value_blue_text_color, theme.value_blue_text_color, 10, 10);
    }

    if (m_repolist_msg != "")
    {
        const ImVec2 label_size = ImGui::CalcTextSize(m_repolist_msg.c_str());
        float y = (ImGui::GetWindowSize().y / 2.f) - (ImGui::GetTextLineHeight() / 2.f);
        ImGui::SetCursorPosX((ImGui::GetWindowSize().x / 2.f) - (label_size.x / 2.f));
        ImGui::SetCursorPosY(y);
        ImGui::TextColored(m_repolist_msg_color, "%s", m_repolist_msg.c_str());
        y += ImGui::GetTextLineHeightWithSpacing();

        if (m_repolist_curlmsg != "")
        {
            const ImVec2 detail_size = ImGui::CalcTextSize(m_repolist_curlmsg.c_str());
            ImGui::SetCursorPosX((ImGui::GetWindowSize().x / 2.f) - (detail_size.x / 2.f));
            ImGui::SetCursorPosY(y);
            ImGui::TextDisabled("%s", m_repolist_curlmsg.c_str());
            y += ImGui::GetTextLineHeight();
        }

        if (m_repolist_httpmsg != "")
        {
            const ImVec2 detail_size = ImGui::CalcTextSize(m_repolist_httpmsg.c_str());
            ImGui::SetCursorPosX((ImGui::GetWindowSize().x / 2.f) - (detail_size.x / 2.f));
            ImGui::SetCursorPosY(y);
            ImGui::TextDisabled("%s", m_repolist_httpmsg.c_str());
        }
    }

    ImGui::End();
    if (!keep_open)
    {
        this->SetVisible(false);
    }
}

void RepositorySelector::DrawResourceView(float searchbox_x)
{
    GUIManager::GuiTheme const& theme = App::GetGuiManager()->GetTheme();
    Ogre::TexturePtr tex2 = FetchIcon("accept.png");
    Ogre::TexturePtr tex3 = FetchIcon("star.png");
    Ogre::TexturePtr tex4 = FetchIcon("arrow_left.png");
    ResourceItem& selected_item = m_data.items[m_resourceview_item_arraypos];

    if (m_gallery_mode_attachment_id != -1)
    {
        // Gallery mode - just draw the pic and be done with it.
        auto itor = m_repo_attachments.find(m_gallery_mode_attachment_id);
        if (itor != m_repo_attachments.end())
        {
            Ogre::TexturePtr& tex = itor->second;
            ImVec2 img_size(tex->getWidth(), tex->getHeight());
            float scale_ratio = 1.f;
            // Shrink to fit
            if (img_size.x > ImGui::GetContentRegionAvail().x)
            {
                scale_ratio = ImGui::GetContentRegionAvail().x / img_size.x;
                if ((img_size.y * scale_ratio) > ImGui::GetContentRegionAvail().y)
                {
                    scale_ratio = ImGui::GetContentRegionAvail().y / img_size.y;
                }
            }
            ImGui::Image(reinterpret_cast<ImTextureID>(tex->getHandle()), img_size * scale_ratio);
            // Left-licking the image will close the gallery mode again
            if (ImGui::IsItemHovered(0))
            {
                ImGui::SetMouseCursor(7);// Hand cursor
                if (ImGui::IsMouseClicked(0)) // Left button
                {
                    m_gallery_mode_attachment_id = -1;
                }
            }
        }
        else
        {
            m_gallery_mode_attachment_id = -1; // Image not found - close gallery mode.
        }
    }

    const float INFOBAR_HEIGHT = 100.f;
    const float INFOBAR_SPACING_LEFTSIDE = 2.f;

    // --- top info bar, left side ---

    // Black background
    ImVec2 leftmost_cursor = ImGui::GetCursorPos();
    float left_pane_width = searchbox_x - (leftmost_cursor.x + ImGui::GetStyle().ItemSpacing.x);
    ImVec2 backdrop_size = ImVec2(left_pane_width, INFOBAR_HEIGHT + ImGui::GetStyle().WindowPadding.y * 2);
    ImGui::GetWindowDrawList()->AddRectFilled(ImGui::GetCursorScreenPos(), ImGui::GetCursorScreenPos() + backdrop_size, ImColor(0.f, 0.f, 0.f, 0.5f), /*rounding:*/5.f);
    ImGui::SetCursorPos(ImGui::GetCursorPos() + ImGui::GetStyle().WindowPadding);

    // The thumbnail again (like on web repo)
    ImVec2 thumbnail_cursor = ImGui::GetCursorPos();
    const float spinner_size = INFOBAR_HEIGHT / 4;
    const ImVec2 spinner_cursor = thumbnail_cursor + ImVec2(INFOBAR_HEIGHT/5, INFOBAR_HEIGHT/5);
    this->DrawThumbnail(m_resourceview_item_arraypos, ImVec2(INFOBAR_HEIGHT, INFOBAR_HEIGHT), spinner_size, spinner_cursor);

    // Title + version (like on web repo)
    ImGui::SameLine();
    ImVec2 newline_cursor = thumbnail_cursor + ImVec2(INFOBAR_HEIGHT + ImGui::GetStyle().WindowPadding.x*2, 0.f);
    ImGui::SetCursorPos(newline_cursor);
    ImGui::TextColored(RESOURCE_TITLE_COLOR, "%s", m_data.items[m_resourceview_item_arraypos].title.c_str());
    ImGui::SameLine();

    // Far right - "view in browser" hyperlink.
    std::string browser_text = _LC("RepositorySelector", "View in web browser");
    ImGui::SetCursorPosX(searchbox_x - (ImGui::CalcTextSize(browser_text.c_str()).x + ImGui::GetStyle().ItemSpacing.x + ImGui::GetStyle().WindowPadding.x));
    ImHyperlink(selected_item.view_url, browser_text);

    // One line below - the tagline (with [..] tooltip button if oversize)
    newline_cursor += ImVec2(0.f, ImGui::GetTextLineHeight() + INFOBAR_SPACING_LEFTSIDE);
    ImGui::SetCursorPos(newline_cursor);
    const ImVec2 tagline_btnsize = ImGui::CalcTextSize("[...]") + ImGui::GetStyle().ItemSpacing * 2;
    const ImVec2 tagline_size = ImGui::CalcTextSize(selected_item.tag_line.c_str());
    const ImVec2 tagline_clipmin = ImGui::GetCursorScreenPos();
    const ImVec2 tagline_clipmax((ImGui::GetWindowPos().x + searchbox_x) - (tagline_btnsize.x + ImGui::GetStyle().FramePadding.x), tagline_clipmin.y + ImGui::GetTextLineHeight());
    ImGui::PushClipRect(tagline_clipmin, tagline_clipmax, /* intersect_with_current_cliprect:*/ false);
    ImGui::Text("%s", selected_item.tag_line.c_str());
    ImGui::PopClipRect();
    if (tagline_size.x > tagline_clipmax.x - tagline_clipmin.x)
    {
        // tagline is oversize - draw [...] tooltip (same line, far right)
        ImGui::SetCursorPos(ImVec2(searchbox_x - (tagline_btnsize.x + ImGui::GetStyle().FramePadding.x), newline_cursor.y));
        ImGui::TextDisabled("[...]");
        if (ImGui::IsItemHovered())
        {
            ImGui::BeginTooltip();
            ImGui::Text("%s", selected_item.tag_line.c_str()); // no clipping this time
            ImGui::EndTooltip();
        }
    }

    // One line below (bigger gap) - the version and num downloads
    newline_cursor += ImVec2(0.f, ImGui::GetTextLineHeight() + INFOBAR_SPACING_LEFTSIDE * 4);
    ImGui::SetCursorPos(newline_cursor);
    ImGui::TextDisabled("%s", _LC("RepositorySelector", "Version:"));
    ImGui::SameLine();
    ImGui::TextColored(theme.value_blue_text_color, "%s", selected_item.version.c_str());
    ImGui::SameLine();
    ImGui::TextDisabled("(%s", _LC("RepositorySelector", "Downloads:"));
    ImGui::SameLine();
    ImGui::TextColored(theme.value_blue_text_color, "%d", selected_item.download_count);
    ImGui::SameLine();
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() - ImGui::GetStyle().ItemSpacing.x); // Don't put gap between the blue value and gray parentheses
    ImGui::TextDisabled(")");

    // One line below - the rating and rating count
    newline_cursor += ImVec2(0.f, ImGui::GetTextLineHeight() + INFOBAR_SPACING_LEFTSIDE);
    ImGui::SetCursorPos(newline_cursor);
    ImGui::TextDisabled("%s", _LC("RepositorySelector", "Rating:"));
    ImGui::SameLine();
    ImGui::TextColored(theme.value_blue_text_color, "%.1f", selected_item.rating_avg);
    ImGui::SameLine();
    // (Rating stars)
    ImVec2 stars_cursor = ImGui::GetCursorPos();
    int rating = round(selected_item.rating_avg);
    for (int i = 1; i <= 5; i++)
    {
        ImGui::SetCursorPosX(stars_cursor.x + 16 * (i-1));
        ImVec4 tint_color = (i <= rating) ? ImVec4(1, 1, 1, 1) : ImVec4(1.f, 1.f, 1.f, 0.2f);
        ImGui::Image(reinterpret_cast<ImTextureID>(tex3->getHandle()), ImVec2(16, 16), ImVec2(0.f, 0.f), ImVec2(1.f, 1.f), tint_color);
        ImGui::SameLine();
    }
    ImGui::SetCursorPosX(stars_cursor.x + 16 * 5 + ImGui::GetStyle().ItemSpacing.x);
    ImGui::TextDisabled("(%s", _LC("RepositorySelector", "Rating Count:"));
    ImGui::SameLine();
    ImGui::TextColored(theme.value_blue_text_color, "%d", selected_item.rating_count);
    ImGui::SameLine();
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() - ImGui::GetStyle().ItemSpacing.x); // Don't put gap between the blue value and gray parentheses
    ImGui::TextDisabled(")");

    // One line below - authors
    newline_cursor += ImVec2(0.f, ImGui::GetTextLineHeight() + INFOBAR_SPACING_LEFTSIDE);
    ImGui::SetCursorPos(newline_cursor);
    ImGui::TextDisabled("%s", _LC("RepositorySelector", "Authors:"));
    ImGui::SameLine();
    ImGui::TextColored(theme.value_blue_text_color, "%s", selected_item.authors.c_str());

    // --- Right column ---

    ImGui::SetCursorPos(ImVec2(searchbox_x, leftmost_cursor.y));
    this->DrawResourceViewRightColumn();

    // --- content area ---
    ImGui::SetCursorPos(leftmost_cursor + ImVec2(0.f, backdrop_size.y + ImGui::GetStyle().ItemSpacing.y));
    const float table_height = ImGui::GetWindowHeight()
        - ((2.f * ImGui::GetStyle().WindowPadding.y) + (3.f * ImGui::GetItemsLineHeightWithSpacing() + backdrop_size.y + ImGui::GetStyle().ItemSpacing.y));

    // Scroll area
    // Make child windows use padding - only works when border is visible, so set it to transparent
    // see https://github.com/ocornut/imgui/issues/462
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.f, 0.f, 0.f, 0.f));
    ImGui::BeginChild("resource-view-scrolling", ImVec2(left_pane_width, table_height), /*border:*/true);
    ImGui::PopStyleColor();

    if (!m_repofiles_msg.empty())
    {
        // Downloading failed
        ImGui::TextDisabled("%s", m_repofiles_msg.c_str());
    }
    else if (m_data.files.empty())
    {
        // Downloading in progress - show spinner (centered)
        float spinner_radius = 25.f;
        ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2(ImGui::GetContentRegionAvailWidth() / 2 - spinner_radius, 200.f));
        LoadingIndicatorCircle("spinner", spinner_radius, theme.value_blue_text_color, theme.value_blue_text_color, 10, 10);
    }
    else
    {
        // Files + description downloaded OK
        this->DrawResourceDescriptionBBCode(selected_item);
    }

    ImGui::EndChild();
}

void RepositorySelector::DrawResourceViewRightColumn()
{
    if (m_data.files.size() == 0)
    {
        return; // Nothing to draw yet
    }

    ResourceItem& selected_item = m_data.items[m_resourceview_item_arraypos];

    const float table_height = ImGui::GetWindowHeight()
        - ((2.f * ImGui::GetStyle().WindowPadding.y) + (3.f * ImGui::GetItemsLineHeightWithSpacing())
            - ImGui::GetStyle().ItemSpacing.y);

    ImVec2 rightcol_size = ImVec2(ImGui::GetContentRegionAvailWidth(), table_height);
    // Make child windows use padding - only works when border is visible, so set it to transparent
    // see https://github.com/ocornut/imgui/issues/462
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.f, 0.f, 0.f, 0.f));
    ImGui::BeginChild("resource-view-files", rightcol_size, /*border:*/true);
    ImGui::PopStyleColor();

    GUIManager::GuiTheme const& theme = App::GetGuiManager()->GetTheme();
    Ogre::TexturePtr tex2 = FetchIcon("accept.png");
    ImGui::Text("%s", _LC("RepositorySelector", "Files:"));

    // Check for duplicate files, remove the outdated one (lower id)
    std::sort(m_data.files.begin(), m_data.files.end(), [](ResourceFiles a, ResourceFiles b) { return a.id > b.id; });
    auto last = std::unique(m_data.files.begin(), m_data.files.end(), [](ResourceFiles a, ResourceFiles b) { return a.filename == b.filename; });
    m_data.files.erase(last, m_data.files.end());

    for (int i = 0; i < m_data.files.size(); i++)
    {
        ImGui::PushID(i);

        ImGui::AlignTextToFramePadding();

        // File
        std::string path = PathCombine(App::sys_user_dir->getStr(), "mods");
        std::string file = PathCombine(path, m_data.files[i].filename);

        // Get created time
        int file_time = 0;
        if (FileExists(file))
        {
            file_time = GetFileLastModifiedTime(file);
        }

        // Filename and size on separate line
        ImGui::TextColored(theme.value_blue_text_color, "%s", m_data.files[i].filename.c_str());

        if (FileExists(file) && ImGui::IsItemHovered())
        {
            ImGui::BeginTooltip();

            time_t c = (const time_t)file_time;
            ImGui::TextDisabled("%s %s", "Installed on", asctime(gmtime(&c)));

            ImGui::EndTooltip();
        }

        // File size
        ImGui::SameLine();

        int size = m_data.files[i].size / 1024;
        ImGui::TextDisabled("(%d %s)", size, "KB");

        // File exists show indicator
        if (FileExists(file))
        {
            ImGui::SameLine();
            ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 3.5f);
            ImGui::Image(reinterpret_cast<ImTextureID>(tex2->getHandle()), ImVec2(16, 16));
        }

        // Buttons (new line)

        std::string btn_label;
        ImVec4 btn_color = ImGui::GetStyle().Colors[ImGuiCol_Button];
        ImVec4 text_color = ImGui::GetStyle().Colors[ImGuiCol_Text];
        if (FileExists(file) && selected_item.last_update > file_time)
        {
            btn_label = fmt::format(_LC("RepositorySelector", "Update"));
        }
        else if (FileExists(file))
        {
            btn_label = fmt::format(_LC("RepositorySelector", "Reinstall"));
        }
        else
        {
            btn_label = fmt::format(_LC("RepositorySelector", "Install"));
            btn_color = RESOURCE_INSTALL_BTN_COLOR;
            text_color = ImVec4(0.1, 0.1, 0.1, 1.0);
        }

        ImGui::PushStyleColor(ImGuiCol_Button, btn_color);
        ImGui::PushStyleColor(ImGuiCol_Text, text_color);
        if (ImGui::Button(btn_label.c_str(), ImVec2(100, 0)))
        {
            this->Download(selected_item.resource_id, m_data.files[i].filename, m_data.files[i].id);
        }
        ImGui::PopStyleColor(2); // Button, Text

        if (FileExists(file))
        {
            ImGui::SameLine();
            if (ImGui::Button(_LC("RepositorySelector", "Remove"), ImVec2(100, 0)))
            {
                Ogre::ArchiveManager::getSingleton().unload(file);
                Ogre::FileSystemLayer::removeFile(file);
                m_update_cache = true;
            }
        }
        else
        {
            ImGui::SameLine();
            ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
            ImGui::Button(_LC("RepositorySelector", "Remove"), ImVec2(100, 0));
            ImGui::PopItemFlag();
            ImGui::PopStyleVar();
        }


        ImGui::PopID(); // i
    }

    ImGui::Separator();
    ImGui::NewLine();

    // Right side: The detail text
    ImGui::Text("%s", _LC("RepositorySelector", "Details:"));

    // Right side, next line
    ImGui::TextDisabled("%s", _LC("RepositorySelector", "Resource ID:"));
    ImGui::SameLine();
    ImGui::TextColored(theme.value_blue_text_color, "%d", selected_item.resource_id);

    // Right side, next line
    ImGui::TextDisabled("%s", _LC("RepositorySelector", "View Count:"));
    ImGui::SameLine();
    ImGui::TextColored(theme.value_blue_text_color, "%d", selected_item.view_count);

    // Right side, next line
    ImGui::TextDisabled(_LC("RepositorySelector", "Date Added:"));
    ImGui::SameLine();
    time_t a = (const time_t)selected_item.resource_date;
    ImGui::TextColored(theme.value_blue_text_color, "%s", asctime(gmtime(&a)));

    // Right side, next line
    ImGui::TextDisabled(_LC("RepositorySelector", "Last Update:"));
    ImGui::SameLine();
    time_t b = (const time_t)selected_item.last_update;
    ImGui::TextColored(theme.value_blue_text_color, "%s", asctime(gmtime(&b)));

    ImGui::EndChild();
}

void RepositorySelector::Refresh()
{
#if defined(USE_CURL)
    m_show_spinner = true;
    m_draw = false;
    m_data.items.clear();
    m_repolist_msg = "";
    std::packaged_task<void(std::string)> task(GetResources);
    std::thread(std::move(task), App::remote_query_url->getStr()).detach();
#endif // defined(USE_CURL)
}

void RepositorySelector::UpdateResources(ResourcesCollection* data)
{
    m_show_spinner = false;
    m_data.categories = data->categories;
    m_data.items = data->items;

    // Sort
    std::sort(m_data.items.begin(), m_data.items.end(), [](ResourceItem a, ResourceItem b) { return a.last_update > b.last_update; });

    if (m_data.items.empty())
    {
        m_repolist_msg = _LC("RepositorySelector", "Sorry, the repository isn't available. Try again later.");
        m_repolist_msg_color = App::GetGuiManager()->GetTheme().no_entries_text_color;
    }
    else
    {
        m_repolist_msg = "";
        m_draw = true;
    }
}

void RepositorySelector::UpdateResourceFilesAndDescription(ResourcesCollection* data)
{
    // Assign data to currently viewed resource item.
    m_data.files = data->files;

    if (m_data.files.empty())
    {
        m_repofiles_msg = _LC("RepositorySelector", "No files available :(");
    }
    else
    {
        m_repofiles_msg = "";
    }

    // Fill in item's description (stripped from resource list for bandwidth reasons).
    for (auto& item : m_data.items)
    {
        if (item.resource_id == data->items[0].resource_id)
        {
            item.description = data->items[0].description;
            break;
        }
    }

    // Finally, initiate download of attachments (images included in description).
    // To do that we must locate [ATTACH] BBCode tags in the description.
    this->DownloadBBCodeAttachmentsRecursive(*data->items[0].description);
}

bool DecodeBBAttachment(bbcpp::BBNodePtr node, int& attachment_id, std::string& attachment_ext)
{
    const auto element = node->downCast<BBElementPtr>();
    if (!element 
        || element->getElementType() == bbcpp::BBElement::CLOSING 
        || element->getChildren().size() == 0)
    {
        return false;
    }
    const auto textnode = element->getChildren().front()->downCast<BBTextPtr>();
    if (!textnode)
    {
        return false;
    }

    try // `std::stoi()` throws exception on bad input
    {
        attachment_id = std::stoi(textnode->getText());

        // Our XenForo always stores original filename in 'alt' attribute.
        std::string basename; // dummy
        Ogre::StringUtil::splitBaseFilename(element->findParameter("alt"), basename, attachment_ext);
        return true;
    }
    catch (...)
    {
        return false;
    }
}

void RepositorySelector::DownloadBBCodeAttachmentsRecursive(const bbcpp::BBNode& parent)
{
    for (const auto node : parent.getChildren())
    {
        if (node->getNodeType() == BBNode::NodeType::ELEMENT
            && node->getNodeName() == "ATTACH")
        {
            int attachment_id = 0;
            std::string attachment_ext = "";
            if (DecodeBBAttachment(node, /*[out]*/ attachment_id, /*[out]*/ attachment_ext))
            {
                this->DownloadAttachment(attachment_id, attachment_ext);
            }
            // Don't log any error - the node may not be applicable (end tag)
            // and parsing will also fail upon render, so the image won't be missed.
        }
        this->DownloadBBCodeAttachmentsRecursive(*node);
    }
}

void RepositorySelector::DownloadAttachment(int attachment_id, std::string const& attachment_ext)
{
    // Check if file is already downloaded
    const std::string filename = fmt::format("{}.{}", attachment_id, attachment_ext);
    const std::string filepath = PathCombine(App::sys_repo_attachments_dir->getStr(), filename);
    if (FileExists(filepath))
    {
        // Load image to memory
        Ogre::TexturePtr tex;
        try // Check if loads correctly (not null, not invalid etc...)
        {
            tex = Ogre::TextureManager::getSingleton().load(filename, RGN_REPO_ATTACHMENTS);
        }
        catch (...) // Doesn't load, fallback
        {
            tex = m_fallback_thumbnail;
        }
        m_repo_attachments[attachment_id] = tex;
    }
    else
    {
        // Request the async download
        RepoWorkQueueTicket ticket;
        ticket.attachment_id = attachment_id;
        ticket.attachment_ext = attachment_ext;
        Ogre::Root::getSingleton().getWorkQueue()->addRequest(m_ogre_workqueue_channel, 1234, Ogre::Any(ticket));
    }
}

void RepositorySelector::OpenResource(int resource_id)
{
#if defined(USE_CURL)
    m_data.files.clear();
    m_repofiles_msg = "";
    std::packaged_task<void(std::string, int)> task(GetResourceFiles);
    std::thread(std::move(task), App::remote_query_url->getStr(), resource_id).detach();
#endif // defined(USE_CURL)
}

void RepositorySelector::Download(int resource_id, std::string filename, int id)
{
#if defined(USE_CURL)
    m_update_cache = false;
    this->SetVisible(false);
    std::packaged_task<void(int, std::string, int)> task(DownloadResourceFile);
    std::thread(std::move(task), resource_id, filename, id).detach();
#endif // defined(USE_CURL)
}

void RepositorySelector::DownloadFinished()
{
    m_update_cache = true;
}

void RepositorySelector::ShowError(CurlFailInfo* failinfo)
{
    m_repolist_msg = failinfo->title;
    m_repolist_msg_color = App::GetGuiManager()->GetTheme().error_text_color;
    m_draw = false;
    m_show_spinner = false;
    if (failinfo->curl_result != CURLE_OK)
        m_repolist_curlmsg = curl_easy_strerror(failinfo->curl_result);
    if (failinfo->http_response != 0)
        m_repolist_httpmsg = fmt::format(_L("HTTP response code: {}"), failinfo->http_response);
}

void RepositorySelector::SetVisible(bool visible)
{
    m_is_visible = visible;
    if (visible && m_data.items.size() == 0)
    {
        this->Refresh();
    }
    else if (!visible && (App::app_state->getEnum<AppState>() == AppState::MAIN_MENU))
    {
        App::GetGuiManager()->GameMainMenu.SetVisible(true);
        if (m_update_cache)
        {
            m_update_cache = false;
            App::GetGameContext()->PushMessage(Message(MSG_APP_MODCACHE_UPDATE_REQUESTED));
        }
    }
}

// Internal helper used by `DrawResourceDescriptionBBCode()`
// Adopted from 'bbcpp' library's utility code. See 'BBDocument.h'.
class RoR::GUI::BBCodeDrawingContext
{
    // Because we simulate text effect with just color, we need rules.
    bool m_italic_text = false;
    bool m_bold_text = false; // Wins over italic
    bool m_underline_text = false; // Wins over bold

    void HandleBBText(const BBTextPtr& textnode)
    {
        ImVec4 color = ImGui::GetStyle().Colors[ImGuiCol_Text];
        if (m_italic_text)
        {
            color = ImVec4(0.205f, 0.789f, 0.820f, 1.f);
        }
        if (m_bold_text) // wins over italic
        {
            color = ImVec4(0.860f, 0.740f, 0.0602f, 1.f);
        }
        if (m_underline_text) // wins over bold
        {
            color = ImVec4(0.00f, 0.930f, 0.480f, 1.f);
        }
        std::string text = textnode->getText();
        if (text != "")
        {
            m_feeder.AddMultiline(ImColor(color), m_wrap_width, text.c_str(), text.c_str() + text.length());
        }
    }

    bool HandleBBElement(const BBElementPtr& element)
    {
        bool recurse_children = true;
        if (element->getNodeName() == "B")
        {
            m_bold_text = element->getElementType() != BBElement::CLOSING;
        }
        else if (element->getNodeName() == "I")
        {
            m_italic_text = element->getElementType() != BBElement::CLOSING;
        }
        else if (element->getNodeName() == "U")
        {
            m_underline_text = element->getElementType() != BBElement::CLOSING;
        }
        else if (element->getNodeName() == "ATTACH")
        {
            recurse_children = false; // the only child is the image ID text

            int attachment_id = 0;
            std::string attachment_ext = "";
            if (DecodeBBAttachment(element, /*[out]*/ attachment_id, /*[out]*/ attachment_ext))
            {
                App::GetGuiManager()->RepositorySelector.DrawAttachment(this, attachment_id);
            }
            else
            {
                ImGui::TextColored(ImVec4(1, 0, 0, 1), "<bad image>");
            }
        }
        else if (element->getNodeName() == "*")
        {
            // We can't just call `ImGui::BulletText()` in the middle of using the feeder.
            // Adapted from `ImGui::BuletTextV()` ...
            ImU32 text_col = ImColor(ImGui::GetStyle().Colors[ImGuiCol_Text]);
            ImVec2 center = m_feeder.cursor + ImVec2(ImGui::GetStyle().FramePadding.x + ImGui::GetFontSize() * 0.5f, ImGui::GetTextLineHeight() * 0.5f);
            ImGui::RenderBullet(ImGui::GetWindowDrawList(), center, text_col);
            m_feeder.cursor += ImVec2(ImGui::GetStyle().FramePadding.x * 2 + ImGui::GetFontSize(), 0.f);
        }
        return recurse_children;

        /* for reference:
                if (element->getElementType() == BBElement::PARAMETER)
                {
                    std::stringstream ss;
                    ss << element->getParameters();
                    ImGui::TextDisabled(ss.str().c_str());
                }
        */
    }

public:
    BBCodeDrawingContext(ImTextFeeder& feeder, float wrap_w) : m_feeder(feeder), m_wrap_width(wrap_w) {}
    ImTextFeeder& m_feeder;
    float m_wrap_width;
    void DrawBBCodeChildrenRecursive(const BBNode& parent)
    {
        for (const auto node : parent.getChildren())
        {
            switch (node->getNodeType())
            {
            default:
                this->DrawBBCodeChildrenRecursive(*node);
                break;

            case BBNode::NodeType::ELEMENT:
                if (this->HandleBBElement(node->downCast<BBElementPtr>()))
                {
                    this->DrawBBCodeChildrenRecursive(*node);
                }
                break;

            case BBNode::NodeType::TEXT:
                this->HandleBBText(node->downCast<BBTextPtr>());
                break;
            }
        }
    }

};

void RepositorySelector::DrawResourceDescriptionBBCode(const ResourceItem& item)
{
    // Decomposes BBCode into DearIMGUI function calls.
    // ------------------------------------------------

    if (!item.description)
        return; // Not loaded yet.

    ImVec2 text_pos = ImGui::GetCursorScreenPos();
    ImTextFeeder feeder(ImGui::GetWindowDrawList(), text_pos);
    BBCodeDrawingContext bb_ctx(feeder, ImGui::GetWindowContentRegionWidth());
    bb_ctx.DrawBBCodeChildrenRecursive(*item.description);
    feeder.NextLine(); // Account correctly for last line height - there may be images on it.

    // From `ImGui::TextEx()` ...
    ImRect bb(text_pos, text_pos + feeder.size);
    ImGui::ItemSize(feeder.size);
    ImGui::ItemAdd(bb, 0);
}

// -------------------------------------------------------
// Async thumbnail/attachment download via Ogre::WorkQueue
// see https://wiki.ogre3d.org/How+to+use+the+WorkQueue

void RepositorySelector::DrawThumbnail(ResourceItemArrayPos_t resource_arraypos, ImVec2 image_size, float spinner_size, ImVec2 spinner_cursor)
{
    // Runs on main thread when drawing GUI
    // Displays a thumbnail image if available, or shows a spinner and initiates async download.
    // -----------------------------------------------------------------------------------------

    GUIManager::GuiTheme const& theme = App::GetGuiManager()->GetTheme();

    if (!m_data.items[resource_arraypos].preview_tex)
    {
        if (m_data.items[resource_arraypos].icon_url == "")
        {
            // No thumbnail defined - use a placeholder logo.
            m_data.items[resource_arraypos].preview_tex = m_fallback_thumbnail;
        }
        else
        {
            // Thumbnail defined - see if we want to initiate download.
            if (ImGui::IsRectVisible(image_size)
                && !m_data.items[resource_arraypos].thumbnail_dl_queued)
            {
                // Image is in visible screen area and not yet downloading.
                RepoWorkQueueTicket ticket;
                ticket.thumb_resourceitem_idx = resource_arraypos;
                Ogre::Root::getSingleton().getWorkQueue()->addRequest(m_ogre_workqueue_channel, 1234, Ogre::Any(ticket));
                m_data.items[resource_arraypos].thumbnail_dl_queued = true;
            }
        }
    }

    if (m_data.items[resource_arraypos].preview_tex)
    {
        // Thumbnail downloaded or replaced by placeholder - draw it.
        ImGui::Image(
            reinterpret_cast<ImTextureID>(m_data.items[resource_arraypos].preview_tex->getHandle()),
            image_size);
    }
    else
    {
        // Thumbnail is downloading - draw spinner.
        ImGui::SetCursorPos(spinner_cursor);
        LoadingIndicatorCircle("spinner", spinner_size, theme.value_blue_text_color, theme.value_blue_text_color, 10, 10);
    }
}

void RepositorySelector::DrawAttachment(BBCodeDrawingContext* context, int attachment_id)
{
    // Runs on main thread when drawing GUI
    // Displays a thumbnail image if already downloaded, or shows a spinner if not yet.
    // Note that downloading attachments is initiated in `UpdateResourceFilesAndDescription()`
    // -----------------------------------------------------------------------------------------

    GUIManager::GuiTheme const& theme = App::GetGuiManager()->GetTheme();

    auto itor = m_repo_attachments.find(attachment_id);
    if (itor != m_repo_attachments.end())
    {
        // Attachment image is already downloaded - draw it.
        Ogre::TexturePtr& tex = itor->second;
        // Scale down and maintain ratio.
        float img_scale = ATTACH_MAX_WIDTH / tex->getWidth();
        if (tex->getHeight() * img_scale > ATTACH_MAX_HEIGHT)
        {
            img_scale = ATTACH_MAX_HEIGHT / tex->getHeight();
        }
        ImVec2 img_size(tex->getWidth() * img_scale, tex->getHeight() * img_scale);
        // Update feeder to account the image
        ImVec2 img_min;
        context->m_feeder.AddRectWrapped(img_size, ImGui::GetStyle().ItemSpacing, context->m_wrap_width, /* [out] */ img_min);
        const ImVec2 img_max = img_min + img_size;
        // Draw image directly via ImGui Drawlist
        context->m_feeder.drawlist->AddImage(
            reinterpret_cast<ImTextureID>(tex->getHandle()), img_min, img_max);
        // Handle mouse hover and click
        if (ImGui::GetMousePos().x > img_min.x && ImGui::GetMousePos().y > img_min.y
            && ImGui::GetMousePos().x < img_max.x && ImGui::GetMousePos().y < img_max.y)
        {
            ImGui::SetMouseCursor(7);//Hand cursor
            if (ImGui::IsMouseClicked(0))
            {
                m_gallery_mode_attachment_id = attachment_id;
            }
        }
    }
    else
    {
        // Attachment image is not downloaded yet - draw spinner
        ImVec2 spinnerbox_size = (ATTACH_SPINNER_PADDING + ImVec2(ATTACH_SPINNER_RADIUS, ATTACH_SPINNER_RADIUS)) * 2.f;
        // Update feeder to account the spinner
        ImVec2 spinnerbox_min;
        context->m_feeder.AddRectWrapped(spinnerbox_size, ImGui::GetStyle().ItemSpacing, context->m_wrap_width, /* [out] */ spinnerbox_min);
        // Draw the spinner body
        ImVec2 backup_screenpos = ImGui::GetCursorScreenPos();
        ImGui::SetCursorScreenPos(spinnerbox_min + ATTACH_SPINNER_PADDING);
        LoadingIndicatorCircle("spinner", ATTACH_SPINNER_RADIUS, theme.value_blue_text_color, theme.value_blue_text_color, 10, 10);
        ImGui::SetCursorScreenPos(backup_screenpos);
    }
}

Ogre::WorkQueue::Response* RepositorySelector::handleRequest(const Ogre::WorkQueue::Request *req, const Ogre::WorkQueue *srcQ)
{
    // This runs on background worker thread in Ogre::WorkQueue's thread pool.
    // Purpose: to fetch one thumbnail image using CURL.
    // -----------------------------------------------------------------------

    auto ticket = Ogre::any_cast<RepoWorkQueueTicket>(req->getData());
    std::string filename, filepath, rg_name, url;
    if (ticket.thumb_resourceitem_idx != -1)
    {
        filename = std::to_string(m_data.items[ticket.thumb_resourceitem_idx].resource_id) + ".png";
        filepath = PathCombine(App::sys_thumbnails_dir->getStr(), filename);
        rg_name = RGN_THUMBNAILS;
        url = m_data.items[ticket.thumb_resourceitem_idx].icon_url;

    }
    else if (ticket.attachment_id != -1)
    {
        filename = fmt::format("{}.{}", ticket.attachment_id, ticket.attachment_ext);
        filepath = PathCombine(App::sys_repo_attachments_dir->getStr(), filename);
        rg_name = RGN_REPO_ATTACHMENTS;
        url = "https://forum.rigsofrods.org/attachments/" + std::to_string(ticket.attachment_id);
    }
    else
    {
        // Invalid request, return empty response.
        LOG("[RoR|RepoUI] Invalid (empty) download request - ignoring it");
        return OGRE_NEW Ogre::WorkQueue::Response(req, /*success:*/false, Ogre::Any(ticket));
    }
    long response_code = 0;

    if (FileExists(filepath))
    {
        return OGRE_NEW Ogre::WorkQueue::Response(req, /*success:*/false, Ogre::Any(ticket));
    }
    else
    {
        try // We write using Ogre::DataStream which throws exceptions
        {
            // smart pointer - closes stream automatically
            Ogre::DataStreamPtr datastream = Ogre::ResourceGroupManager::getSingleton().createResource(filename, rg_name);

            curl_easy_setopt(curl_th, CURLOPT_URL, url.c_str());
            curl_easy_setopt(curl_th, CURLOPT_IPRESOLVE, CURL_IPRESOLVE_V4);
            curl_easy_setopt(curl_th, CURLOPT_FOLLOWLOCATION, 1L); // Necessary for attachment images
#ifdef _WIN32
            curl_easy_setopt(curl_th, CURLOPT_SSL_OPTIONS, CURLSSLOPT_NATIVE_CA);
#endif // _WIN32
            curl_easy_setopt(curl_th, CURLOPT_WRITEFUNCTION, CurlOgreDataStreamWriteFunc);
            curl_easy_setopt(curl_th, CURLOPT_WRITEDATA, datastream.get());
            CURLcode curl_result = curl_easy_perform(curl_th);

            // If CURL follows a redirect then it returns 0 for HTTP response code.
            if (curl_result != CURLE_OK || (response_code != 0 && response_code != 200))
            {
                Ogre::LogManager::getSingleton().stream()
                    << "[RoR|Repository] Failed to download image;"
                    << " URL: '" << url << "',"
                    << " Error: '" << curl_easy_strerror(curl_result) << "',"
                    << " HTTP status code: " << response_code;

                return OGRE_NEW Ogre::WorkQueue::Response(req, /*success:*/false, Ogre::Any(ticket));
            }
            else
            {
                return OGRE_NEW Ogre::WorkQueue::Response(req, /*success:*/true, Ogre::Any(ticket));
            }
        }
        catch (Ogre::Exception& oex)
        {
            App::GetConsole()->putMessage(
                Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_ERROR,
                fmt::format("Repository UI: cannot download image '{}' - {}",
                    url, oex.getDescription()));

            return OGRE_NEW Ogre::WorkQueue::Response(req, /*success:*/false, Ogre::Any(ticket));
        }
    }
}

void RepositorySelector::handleResponse(const Ogre::WorkQueue::Response *req, const Ogre::WorkQueue *srcQ)
{
    // This runs on main thread.
    // It's safe to load the texture and modify GUI data.
    // --------------------------------------------------

    auto ticket = Ogre::any_cast<RepoWorkQueueTicket>(req->getData());
    std::string filename, filepath, rg_name;
    if (ticket.thumb_resourceitem_idx != -1)
    {
        filename = std::to_string(m_data.items[ticket.thumb_resourceitem_idx].resource_id) + ".png";
        filepath = PathCombine(App::sys_thumbnails_dir->getStr(), filename);
        rg_name = RGN_THUMBNAILS;
    }
    else if (ticket.attachment_id != -1)
    {
        filename = fmt::format("{}.{}", ticket.attachment_id, ticket.attachment_ext);
        filepath = PathCombine(App::sys_repo_attachments_dir->getStr(), filename);
        rg_name = RGN_REPO_ATTACHMENTS;
    }
    else
    {
        // Invalid request, nothing to do.
        LOG("[RoR|RepoUI] Invalid (empty) download response - ignoring it");
        return;
    }

    if (FileExists(filepath)) // We have an image
    {
        Ogre::TexturePtr tex;
        try // Check if loads correctly (not null, not invalid etc...)
        {
            tex = Ogre::TextureManager::getSingleton().load(filename, rg_name);
        }
        catch (...) // Doesn't load, fallback
        {
            tex = m_fallback_thumbnail;
        }

        if (ticket.thumb_resourceitem_idx != -1)
        {
            // Thumbnail for resource item
            m_data.items[ticket.thumb_resourceitem_idx].preview_tex = tex;
        }
        else if (ticket.attachment_id != -1)
        {
            // Attachment image to display in description
            m_repo_attachments[ticket.attachment_id] = tex;
        }
    }
}
