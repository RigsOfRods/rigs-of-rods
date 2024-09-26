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

struct UserLoginToken
{};

struct UserProfile
{
    std::string username;
    std::string email;
    std::string avatar_url;
};

const char* const ROUTE_LOGIN = "/login";

class LoginBox {
public:
    LoginBox();
    ~LoginBox();

    void                        SetVisible(bool visible);
    bool                        IsVisible() const { return m_is_visible; }
    void                        ShowError(std::string const& msg);
    void                        ConfirmTfa();
    void                        TriggerTfa();
    void                        NeedsTfa(std::vector<std::string> tfa_providers);
    void                        TfaTriggered();
    void                        Login();
    void                        Draw();

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
};

}
}