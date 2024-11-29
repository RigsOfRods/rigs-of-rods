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

/// @file   GUI_LoginBox.h
/// @author Rafael Galvan, 2024

#pragma once

#include "Application.h"
#include "OgreImGui.h"

#include <future>
#include <memory>
#include <thread>
#include <vector>

namespace RoR {
namespace GUI {

struct UserProfile
{
    int                 user_id;
    std::string         username;
    std::string         email;
    std::string         avatar_url;
    Ogre::TexturePtr    avatar;
};

struct UserAuthToken
{
    std::string         login_token;
    std::string         refresh_token;
};

class LoginBox
{
public:
    LoginBox();
    ~LoginBox();

    void                                SetVisible(bool visible);
    bool                                IsVisible() const { return m_is_visible; }
    void                                ShowError(std::string const& msg);

    void                                ConfirmTfa();
    void                                TriggerTfa();
    void                                NeedsTfa(std::vector<std::string> tfa_providers);
    void                                TfaTriggered();

    void                                Login();
    void                                Logout();
    void                                Draw();
    void                                FetchUserProfile();
    void                                FetchUserProfileAvatar();
    void                                UpdateUserProfile(UserProfile* data);
    void                                UpdateUserAuth(UserAuthToken* data);
    void                                UpdateUserProfileAvatar(std::string file);
    void                                ValidateOrRefreshToken();

    UserProfile                         GetUserProfile() { return m_user_profile; }
    int                                 GetUserAuthStatus() const { return m_logged_in;  }

private:
    bool                        m_is_visible = false;
    Str<1000>                   m_login;
    Str<1000>                   m_passwd;
    Str<1000>                   m_tfa_code;
    bool                        m_remember = false;
    std::string                 m_errors;
    bool                        m_needs_tfa = false;
    bool                        m_loading = false;
    std::vector<std::string>    m_tfa_providers;
    std::string                 m_tfa_provider;
    bool                        m_tfa_trigger = false;
    std::string                 m_base_url;
    bool                        m_logged_in = false; //< Local copy
    UserAuthToken               m_auth_tokens; //< Local copy
    UserProfile                 m_user_profile; //< Local copy
};

}
}