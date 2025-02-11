/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2013-2024 Petr Ohlidal

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

/// @file   GUI_LoginBox.cpp
/// @author Rafael Galvan, 2024

#include "GUI_LoginBox.h"

#include "Application.h"
#include "GameContext.h"
#include "GUIManager.h"
#include "GUIUtils.h"
#include "AppContext.h"
#include "PlatformUtils.h"
#include "Language.h"
#include "RoRVersion.h"

#include <imgui.h>
#include <string>
#include <imgui_internal.h>
#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <vector>
#include <fmt/core.h>
#include <stdio.h>

#ifdef USE_URL
#   include <curl/curl.h>
#   include <curl/easy.h>
#endif

#if defined(_MSC_VER) && defined(GetObject) // This MS Windows macro from <wingdi.h> (Windows Kit 8.1) clashes with RapidJSON
#   undef GetObject
#endif

using namespace RoR;
using namespace GUI;

#if defined(USE_CURL)

static size_t CurlWriteFunc(void* ptr, size_t size, size_t nmemb, std::string* data)
{
    data->append((char*)ptr, size * nmemb);
    return size * nmemb;
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

void GetUserProfileAvatarTask(int user_id, std::string avatar_url)
{
    // The avatar URL may not be of *.rigsofrods.org, as it may also be a gravatar.
    std::string user_agent = fmt::format("{}/{}", "Rigs of Rods Client", ROR_VERSION_STRING);
    std::string filename = std::to_string(user_id) + ".png";
    std::string file = PathCombine(App::sys_avatar_dir->getStr(), filename);
    long response_code = 0;

    CURL* curl = curl_easy_init();
    Ogre::DataStreamPtr datastream = Ogre::ResourceGroupManager::getSingleton().createResource(file, RGN_AVATAR);

    curl_easy_setopt(curl, CURLOPT_URL, avatar_url.c_str());
    curl_easy_setopt(curl, CURLOPT_IPRESOLVE, CURL_IPRESOLVE_V4);
#ifdef _WIN32
    curl_easy_setopt(curl, CURLOPT_SSL_OPTIONS, CURLSSLOPT_NATIVE_CA);
#endif // _WIN32
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, CurlOgreDataStreamWriteFunc);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, datastream.get());

    CURLcode curl_result = curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);

    if (curl_result != CURLE_OK || response_code != 200)
    {
        Ogre::LogManager::getSingleton().stream()
            << "[RoR|UserAuthManager] Failed to download user avatar, player will have a default avatar"
            << " Error: '" << curl_easy_strerror(curl_result) << "'; HTTP status code: " << response_code;
    }

    curl_easy_cleanup(curl);
    curl = nullptr;
    App::GetGameContext()->PushMessage(Message(MSG_NET_USERPROFILE_AVATAR_FINISHED, file));
}

void UserAuthInvalidateTokenTask()
{
    std::string auth_header = std::string("Authorization: Bearer ") + App::remote_login_token->getStr();
    std::string user_agent = fmt::format("{}/{}", "Rigs of Rods Client", ROR_VERSION_STRING);
    std::string url = App::remote_query_url->getStr() + "/auth/logout";
    std::string response_payload;
    std::string response_header;
    long response_code = 0;

    struct curl_slist* slist;
    slist = NULL;
    slist = curl_slist_append(slist, "Accept: application/json");
    slist = curl_slist_append(slist, "Content-Type: application/json");
    slist = curl_slist_append(slist, auth_header.c_str());

    CURL* curl = curl_easy_init();
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
#ifdef _WIN32
    curl_easy_setopt(curl, CURLOPT_SSL_OPTIONS, CURLSSLOPT_NATIVE_CA);
#endif // _WIN32
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, slist);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, CurlWriteFunc);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_payload);
    curl_easy_setopt(curl, CURLOPT_HEADERDATA, &response_header);

    CURLcode curl_result = curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);

    curl_easy_cleanup(curl);
    curl = nullptr;
    slist = NULL;

    rapidjson::Document j_data_doc;
    j_data_doc.Parse(response_payload.c_str());

    if (curl_result != CURLE_OK || response_code != 200)
    {
        Ogre::LogManager::getSingleton().stream()
            << "[RoR|UserAuthManager] Failed invalidate user tokens, the player's user profile will still be deleted;"
            << " Error: '" << curl_easy_strerror(curl_result) << "'; HTTP status code: " << response_code;
        return;
    }

    // job done
}

void GetUserProfileTask()
{
    std::string auth_header = std::string("Authorization: Bearer ") + App::remote_login_token->getStr();
    std::string user_agent = fmt::format("{}/{}", "Rigs of Rods Client", ROR_VERSION_STRING);
    std::string url = App::remote_query_url->getStr() + "/users/me";
    std::string response_payload;
    std::string response_header;
    long response_code = 0;

    struct curl_slist* slist;
    slist = NULL;
    slist = curl_slist_append(slist, "Accept: application/json");
    slist = curl_slist_append(slist, "Content-Type: application/json");
    slist = curl_slist_append(slist, auth_header.c_str());

    CURL* curl = curl_easy_init();
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
#ifdef _WIN32
    curl_easy_setopt(curl, CURLOPT_SSL_OPTIONS, CURLSSLOPT_NATIVE_CA);
#endif // _WIN32
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, slist);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, CurlWriteFunc);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_payload);
    curl_easy_setopt(curl, CURLOPT_HEADERDATA, &response_header);

    CURLcode curl_result = curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);

    curl_easy_cleanup(curl);
    curl = nullptr;
    slist = NULL;

    rapidjson::Document j_data_doc;
    j_data_doc.Parse(response_payload.c_str());

    if (curl_result != CURLE_OK || response_code != 200)
    {
        Ogre::LogManager::getSingleton().stream()
            << "[RoR|UserAuthManager] Failed to fetch user profile, the player will have missing user profile data;"
            << " Error: '" << curl_easy_strerror(curl_result) << "'; HTTP status code: " << response_code;
        return;
    }

    GUI::UserProfile* user_profile_ptr = new GUI::UserProfile();
    rapidjson::Value& j_response_body = j_data_doc["me"];
    user_profile_ptr->username = j_response_body["username"].GetString();
    user_profile_ptr->avatar_url = j_response_body["avatar_urls"]["o"].GetString(); 
    user_profile_ptr->email = j_response_body["email"].GetString();
    user_profile_ptr->user_id = j_response_body["user_id"].GetInt();
    user_profile_ptr->avatar = Ogre::TexturePtr();

    App::GetGameContext()->PushMessage(
        Message(MSG_NET_USERPROFILE_FINISHED,
            static_cast<void*>(user_profile_ptr)));
}

void ValidateOrRefreshTokenTask(std::string login_token, std::string refresh_token)
{
    rapidjson::Document j_request_body;
    j_request_body.SetObject();
    j_request_body.AddMember("login_token", rapidjson::StringRef(login_token.c_str()), j_request_body.GetAllocator());
    j_request_body.AddMember("refresh_token", rapidjson::StringRef(refresh_token.c_str()), j_request_body.GetAllocator());
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    j_request_body.Accept(writer);
    std::string request_body = buffer.GetString();

    std::string user_agent = fmt::format("{}/{}", "Rigs of Rods Client", ROR_VERSION_STRING);
    std::string url = App::remote_query_url->getStr() + "/auth/refresh";
    std::string response_payload;
    std::string response_header;
    long response_code = 0;

    struct curl_slist* slist;
    slist = NULL;
    slist = curl_slist_append(slist, "Content-Type: application/json");

    CURL* curl = curl_easy_init();
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, request_body.c_str());
#ifdef _WIN32
    curl_easy_setopt(curl, CURLOPT_SSL_OPTIONS, CURLSSLOPT_NATIVE_CA);
#endif // _WIN32
    curl_easy_setopt(curl, CURLOPT_POST, 1);
    curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "gzip");
    curl_easy_setopt(curl, CURLOPT_USERAGENT, user_agent.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, slist);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, CurlWriteFunc);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_payload);
    curl_easy_setopt(curl, CURLOPT_HEADERDATA, &response_header);

    CURLcode curl_result = curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);

    curl_easy_cleanup(curl);
    curl = nullptr;

    rapidjson::Document j_response_body;
    j_response_body.Parse(response_payload.c_str());

    if (j_response_body.HasParseError())
    {
        Ogre::LogManager::getSingleton().stream()
            << "[RoR|UserAuthManager] Failed to parse JSON response body; the player will not be logged in in;"
            << " Error: '" << j_response_body.GetParseError();
        App::GetGameContext()->PushMessage(
            Message(MSG_NET_USERAUTH_FAILURE, _LC("Login", "There was an unexpected server error. Please retry.")));
        return;
    }

    if (response_code != 200)
    {
        Ogre::LogManager::getSingleton().stream()
            << "[RoR|UserAuthManager] Failed to refresh or validate login token, the player will not be logged in;"
            << " Error: '" << curl_easy_strerror(curl_result) << "'; HTTP status code: " << response_code;
        return;
    }

    GUI::UserAuthToken* auth_tokens_ptr = new GUI::UserAuthToken();
    auth_tokens_ptr->login_token = j_response_body["login_token"].GetString();
    auth_tokens_ptr->refresh_token = j_response_body["refresh_token"].GetString();

    App::GetGameContext()->PushMessage(Message(MSG_NET_USERAUTH_RV_SUCCESS, static_cast<void*>(auth_tokens_ptr)));
}

void UserAuthWithTfaTask(std::string login, std::string passwd, std::string provider, std::string code)
{
    rapidjson::Document j_request_body;
    j_request_body.SetObject();
    j_request_body.AddMember("login", rapidjson::StringRef(login.c_str()), j_request_body.GetAllocator());
    j_request_body.AddMember("password", rapidjson::StringRef(passwd.c_str()), j_request_body.GetAllocator());
    j_request_body.AddMember("tfa_provider", rapidjson::StringRef(provider.c_str()), j_request_body.GetAllocator());
    j_request_body.AddMember("code", rapidjson::StringRef(code.c_str()), j_request_body.GetAllocator());
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    j_request_body.Accept(writer);
    std::string request_body = buffer.GetString();

    std::string user_agent = fmt::format("{}/{}", "Rigs of Rods Client", ROR_VERSION_STRING);
    std::string url = App::remote_query_url->getStr() + "/auth/login";
    std::string response_payload;
    std::string response_header;
    long response_code = 0;

    struct curl_slist* slist;
    slist = NULL;
    slist = curl_slist_append(slist, "Content-Type: application/json");


    CURL* curl = curl_easy_init();
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str()); // todo api url + endpoint
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, request_body.c_str()); // post request body
#ifdef _WIN32
    curl_easy_setopt(curl, CURLOPT_SSL_OPTIONS, CURLSSLOPT_NATIVE_CA);
#endif // _WIN32
    curl_easy_setopt(curl, CURLOPT_POST, 1);
    curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "gzip");
    curl_easy_setopt(curl, CURLOPT_USERAGENT, user_agent.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, slist);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, CurlWriteFunc);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_payload);
    curl_easy_setopt(curl, CURLOPT_HEADERDATA, &response_header);

    curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);

    curl_easy_cleanup(curl);
    curl = nullptr;

    rapidjson::Document j_response_body;
    j_response_body.Parse(response_payload.c_str());

    if (j_response_body.HasParseError())
    {
        Ogre::LogManager::getSingleton().stream()
            << "[RoR|UserAuthManager] Failed to parse JSON response body; the player will not be logged in in;"
            << " Error: '" << j_response_body.GetParseError();
        App::GetGameContext()->PushMessage(
            Message(MSG_NET_USERAUTH_FAILURE, _LC("Login", "There was an unexpected server error. Please retry.")));
        return;
    }

    if (response_code == 400) // a failure, bad tfa code
    {
        App::GetGameContext()->PushMessage(
            Message(MSG_NET_USERAUTH_TFA_FAILURE, _LC("Login", "The two-step verification value could not be confirmed. Please retry."))
        );
        return;
    }
    else if (response_code != 200) // a net failure, restart from beginning
    {
        App::GetGameContext()->PushMessage(
            Message(MSG_NET_USERAUTH_FAILURE, _LC("Login", "Connection error. Please check your connection and try again."))
        );
        return;
    }

    GUI::UserAuthToken* auth_tokens_ptr = new GUI::UserAuthToken();
    auth_tokens_ptr->login_token = j_response_body["login_token"].GetString();
    auth_tokens_ptr->refresh_token = j_response_body["refresh_token"].GetString();

    App::GetGameContext()->PushMessage(Message(MSG_NET_USERAUTH_SUCCESS, static_cast<void*>(auth_tokens_ptr)));
}

void PostAuthTriggerTfa(std::string login, std::string passwd, std::string provider)
{
    rapidjson::Document j_request_body;
    j_request_body.SetObject();
    j_request_body.AddMember("login", rapidjson::StringRef(login.c_str()), j_request_body.GetAllocator());
    j_request_body.AddMember("password", rapidjson::StringRef(passwd.c_str()), j_request_body.GetAllocator());
    j_request_body.AddMember("tfa_provider", rapidjson::StringRef(provider.c_str()), j_request_body.GetAllocator());
    j_request_body.AddMember("tfa_trigger", true, j_request_body.GetAllocator());  // Assuming tfa_trigger is a bool
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    j_request_body.Accept(writer);
    std::string request_body = buffer.GetString();

    std::string user_agent = fmt::format("{}/{}", "Rigs of Rods Client", ROR_VERSION_STRING);
    std::string url = App::remote_query_url->getStr() + "/auth/login";
    std::string response_payload;
    std::string response_header;
    long response_code = 0;

    struct curl_slist* slist;
    slist = NULL;
    slist = curl_slist_append(slist, "Content-Type: application/json");

    CURL* curl = curl_easy_init();
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, request_body.c_str());
    curl_easy_setopt(curl, CURLOPT_IPRESOLVE, CURL_IPRESOLVE_V4);
#ifdef _WIN32
    curl_easy_setopt(curl, CURLOPT_SSL_OPTIONS, CURLSSLOPT_NATIVE_CA);
#endif // _WIN32
    curl_easy_setopt(curl, CURLOPT_POST, 1);
    curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "gzip");
    curl_easy_setopt(curl, CURLOPT_USERAGENT, user_agent.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, slist);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, CurlWriteFunc);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_payload);
    curl_easy_setopt(curl, CURLOPT_HEADERDATA, &response_header);

    curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);

    curl_easy_cleanup(curl);
    curl = nullptr;

    if (response_code != 202)
    {
        Ogre::LogManager::getSingleton().stream()
            << "[RoR|HTTP|UserAuth] Failed to trigger two-factor; HTTP status code: " << response_code;
        App::GetGameContext()->PushMessage(
            Message(MSG_NET_USERAUTH_FAILURE, _LC("Login", "Connection error. Please check your connection and try again."))
        );
        return;
    }

    App::GetGameContext()->PushMessage(Message(MSG_NET_USERAUTH_TFA_TRIGGERED));
}

void UserAuthTask(std::string login, std::string passwd)
{
    rapidjson::Document j_request_body;
    j_request_body.SetObject();
    j_request_body.AddMember("login", rapidjson::StringRef(login.c_str()), j_request_body.GetAllocator());
    j_request_body.AddMember("password", rapidjson::StringRef(passwd.c_str()), j_request_body.GetAllocator());
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    j_request_body.Accept(writer);
    std::string request_body = buffer.GetString();

    std::string user_agent = fmt::format("{}/{}", "Rigs of Rods Client", ROR_VERSION_STRING);
    std::string url = App::remote_query_url->getStr() + "/auth/login";
    std::string response_payload;
    std::string response_header;
    long response_code = 0;

    struct curl_slist* slist;
    slist = NULL;
    slist = curl_slist_append(slist, "Content-Type: application/json");

    CURL* curl = curl_easy_init();
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_IPRESOLVE, CURL_IPRESOLVE_V4);
#ifdef _WIN32
    curl_easy_setopt(curl, CURLOPT_SSL_OPTIONS, CURLSSLOPT_NATIVE_CA);
#endif // _WIN32
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, request_body.c_str());
    curl_easy_setopt(curl, CURLOPT_POST, 1);
    curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "gzip");
    curl_easy_setopt(curl, CURLOPT_USERAGENT, user_agent.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, slist);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, CurlWriteFunc);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_payload);
    curl_easy_setopt(curl, CURLOPT_HEADERDATA, &response_header);

    curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);

    curl_easy_cleanup(curl);
    curl = nullptr;

    rapidjson::Document j_response_body;
    j_response_body.Parse(response_payload.c_str());

    if (j_response_body.HasParseError())
    {
        Ogre::LogManager::getSingleton().stream()
            << "[RoR|UserAuthManager] Failed to parse JSON response body; the player will not be logged in in;"
            << " Error: '" << j_response_body.GetParseError();
        App::GetGameContext()->PushMessage(
            Message(MSG_NET_USERAUTH_FAILURE, _LC("Login", "There was an unexpected server error. Please retry.")));
        return;
    }

    if (response_code == 400)
    {
        if (j_response_body["tfa_providers"].IsString())
        {
            std::string providers_str = j_response_body["tfa_providers"].GetString();
            std::vector<std::string>* tfa_providers_ptr = new std::vector<std::string>();

            std::istringstream ss(providers_str);
            std::string provider;
            while (std::getline(ss, provider, ','))
            {
                tfa_providers_ptr->push_back(provider);
            }

            App::GetGameContext()->PushMessage(Message(MSG_NET_USERAUTH_TFA_REQUESTED, static_cast<void*>(tfa_providers_ptr)));
            return;
        }

        App::GetGameContext()->PushMessage(
            Message(MSG_NET_USERAUTH_FAILURE, _LC("Login", "You did not sign in correctly or your account is temporarily disabled. Please retry."))
        );
        return;
    }
    else if (response_code == 401)
    {
        App::GetGameContext()->PushMessage(
            Message(MSG_NET_USERAUTH_FAILURE, _LC("Login", "Could not log you in. Your account has been suspended."))
        );
        return;
    }
    else if (response_code >= 300)
    {
        Ogre::LogManager::getSingleton().stream()
            << "[RoR|UserAuthManager] Failed to log in player, the player will not be logged in; HTTP status code: " << response_code;
        App::GetGameContext()->PushMessage(
            Message(MSG_NET_USERAUTH_FAILURE, _LC("Login", "Connection error. Please check your connection and try again."))
        );
        return;
    }

    GUI::UserAuthToken* auth_tokens_ptr = new GUI::UserAuthToken();
    auth_tokens_ptr->login_token = j_response_body["login_token"].GetString();
    auth_tokens_ptr->refresh_token = j_response_body["refresh_token"].GetString();

    App::GetGameContext()->PushMessage(Message(MSG_NET_USERAUTH_SUCCESS, static_cast<void*>(auth_tokens_ptr)));
}

#endif

LoginBox::LoginBox()
{}

LoginBox::~LoginBox()
{}

void LoginBox::Draw()
{
    if (App::remote_user_auth_state->getEnum<UserAuthState>() == UserAuthState::AUTHENTICATED ||
        App::remote_user_auth_state->getEnum<UserAuthState>() == UserAuthState::INVALID)
    {
        this->SetVisible(false);
    }

    GUIManager::GuiTheme const& theme = App::GetGuiManager()->GetTheme();

    ImGui::SetNextWindowSize(ImVec2(400.f, 250.f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowPosCenter(ImGuiCond_Appearing);
    ImGuiWindowFlags win_flags = 
        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar | 
        ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoMove | 
        ImGuiWindowFlags_NoResize;
    bool keep_open = true;
    ImGui::Begin(_LC("Login", "Login"), &keep_open, win_flags);

    if (!m_loading)
    {
        if (!m_errors.empty())
        {
            ImGui::TextColored(App::GetGuiManager()->GetTheme().error_text_color, "%s", m_errors.c_str());
        }

        if (m_needs_tfa)
        {
            ImGui::BeginTabBar("TfaOptTab");
            if (std::find(m_tfa_providers.begin(), m_tfa_providers.end(), "totp") != m_tfa_providers.end())
            {
                if (ImGui::BeginTabItem(_LC("Login", "Verification code via app")))
                {
                    m_tfa_provider = "totp";
                    ImGui::TextWrapped(_LC("Login", "Please enter the verification code generated by the app on your phone."));
                    ImGui::EndTabItem();
                }
            }
            if (std::find(m_tfa_providers.begin(), m_tfa_providers.end(), "email") != m_tfa_providers.end())
            {
                if (ImGui::BeginTabItem(_LC("Login", "Email confirmation")))
                {
                    m_tfa_provider = "email";
                    if (!m_tfa_trigger)
                        this->TriggerTfa(); // Without this, there will be an infinite loop.
                    ImGui::TextWrapped(_LC("Login", "An email has been sent with a single-use code. Please enter that code to continue."));
                    ImGui::EndTabItem();
                }
            }
            ImGui::EndTabBar();
            ImGui::InputText("##2fa", m_tfa_code.GetBuffer(), m_tfa_code.GetCapacity());
            if (ImGui::Button("Confirm"))
            {
                this->ConfirmTfa();
            }
            ImGui::Separator();
            ImGui::TextWrapped(_LC("Login", "A backup code can be used when you don't have access to an alternative verification method. To do so, you must login using a web browser."));
        }
        else
        {
            ImGui::Text(_LC("Login", "Your name or email address"));
            ImGui::InputText("##login", m_login.GetBuffer(), m_login.GetCapacity());
            ImGui::Text(_LC("Login", "Password"));
            ImGui::InputText("##password", m_passwd.GetBuffer(), m_passwd.GetCapacity(), ImGuiInputTextFlags_Password | ImGuiInputTextFlags_CharsNoBlank);
            if (ImGui::Button("Login"))
            {
                this->Login();
            }
        }
    }
    else
    {
        float spinner_size = 27.f;
        ImGui::SetCursorPosX((ImGui::GetWindowSize().x / 2.f) - spinner_size);
        ImGui::SetCursorPosY((ImGui::GetWindowSize().y / 2.f) - spinner_size);
        LoadingIndicatorCircle("spinner", spinner_size, theme.value_blue_text_color, theme.value_blue_text_color, 10, 10);
    }

    ImGui::End();
    if (!keep_open)
    {
        this->SetVisible(false);
    }
}

void LoginBox::Login()
{
#if defined(USE_CURL)
    m_loading = true;

    if (m_login.IsEmpty() && m_passwd.IsEmpty())
    {
        App::GetGameContext()->PushMessage(
            Message(MSG_NET_USERAUTH_FAILURE, _LC("Login", "There must not be any empty fields."))
        );
        return;
    }

    std::string login(m_login);
    std::string passwd(m_passwd);

    std::packaged_task<void(std::string, std::string)> task(UserAuthTask);
    std::thread(std::move(task), login, passwd).detach();
#endif
}

void LoginBox::Logout()
{
#if defined(USE_CURL)
    std::thread([] {
        UserAuthInvalidateTokenTask();
        }).detach();
#endif
}

void LoginBox::UpdateUserAuth(UserAuthToken* data)
{
    if (data) {
        m_logged_in = true;
        m_auth_tokens = *data;
        App::remote_login_token->setStr(data->login_token);
        App::remote_refresh_token->setStr(data->refresh_token);
    }
}

void LoginBox::FetchUserProfile()
{
#if defined(USE_CURL)
    std::thread(GetUserProfileTask).detach();
#endif // defined(USE_CURL)
}

void LoginBox::FetchUserProfileAvatar()
{
#if defined(USE_CURL)
    std::packaged_task<void(int, std::string)> task(GetUserProfileAvatarTask);
    std::thread(std::move(task), m_user_profile.user_id, m_user_profile.avatar_url).detach();
#endif // defined(USE_CURL)
}

void LoginBox::UpdateUserProfile(UserProfile* data)
{
    m_user_profile = *data;
}

void LoginBox::ShowError(std::string const& msg)
{
    m_loading = false;
    m_errors = msg;
}

void LoginBox::UpdateUserProfileAvatar(std::string file)
{
    m_user_profile.avatar = FetchIcon(file.c_str());
    m_user_profile.avatar->load();
}

void LoginBox::ConfirmTfa()
{
#if defined(USE_CURL)
    m_loading = true;

    if (m_tfa_code.IsEmpty())
    {
        App::GetGameContext()->PushMessage(
            Message(MSG_NET_USERAUTH_FAILURE, _LC("Login", "There must not be any empty fields."))
        );
        return;
    }

    std::string login(m_login);
    std::string passwd(m_passwd);
    std::string tfa_code(m_tfa_code);

    std::packaged_task<void(std::string, std::string, std::string, std::string)> task(UserAuthWithTfaTask);
    std::thread(std::move(task), login, passwd, m_tfa_provider, tfa_code).detach();
#endif
}

void LoginBox::TriggerTfa()
{
#if defined(USE_CURL)
    m_loading = true;
    m_tfa_trigger = true;

    std::string login(m_login);
    std::string passwd(m_passwd);

    std::packaged_task<void(std::string, std::string, std::string)> task(PostAuthTriggerTfa);
    std::thread(std::move(task), login, passwd, m_tfa_provider).detach();
#endif
}

void LoginBox::NeedsTfa(std::vector<std::string> tfa_providers)
{
    m_loading = false;
    m_needs_tfa = true;
    m_tfa_providers = tfa_providers;
}

void LoginBox::TfaTriggered()
{
    //m_tfa_trigger = false;
    m_loading = false;
}

void LoginBox::ValidateOrRefreshToken()
{
#if defined(USE_CURL)
    //m_loading = true;

    std::packaged_task<void(std::string, std::string)> task(ValidateOrRefreshTokenTask);
    std::thread(
        std::move(task), 
        App::remote_login_token->getStr(),
        App::remote_refresh_token->getStr())
    .detach();
#endif
}

void LoginBox::SetVisible(bool visible)
{
    m_is_visible = visible;
    if (!visible && (App::app_state->getEnum<AppState>() == AppState::MAIN_MENU))
    {
        App::GetGuiManager()->GameMainMenu.SetVisible(true);
    }
}
