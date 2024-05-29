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

/// @file   
/// @brief  Generic UI dialog (not modal). Invocable from scripting. Any number of buttons. Configurable to fire script events or post MQ messages.
/// @author Moncef Ben Slimane, 2014
/// @author Petr Ohlidal, 2021 - extended config.

#pragma once

#include "ForwardDeclarations.h"
#include "Application.h"

#include <string>
#include <vector>

namespace RoR {
namespace GUI {

    // ----------------------------
    // Config

struct MessageBoxButton
{
    std::string mbb_caption;
    MsgType     mbb_mq_message = MsgType::MSG_INVALID; //!< Message to queue on click.
    std::string mbb_mq_description;                    //!< Message argument to queue on click.
    void*       mbb_mq_payload = nullptr;              //!< Message argument to queue on click.
    int         mbb_script_number = -1;                //!< Valid values are >= 1. -1 means not defined.
};

struct MessageBoxConfig
{
    std::string mbc_title;
    std::string mbc_text;
    CVar*       mbc_always_ask_conf = nullptr;         //!< If set, displays classic checkbox "[x] Always ask".
    bool*       mbc_close_handle = nullptr;            //!< External close handle - not required for `mbc_allow_close`.
    bool        mbc_allow_close = false;               //!< Show close handle even if `dbc_close_handle` isn't set.
    float       mbc_content_width = 300.f;             //!< Parameter to `ImGui::SetContentWidth()` - hard limit on content size.

    std::vector<MessageBoxButton> mbc_buttons;
};

    // ----------------------------
    // Execution

class MessageBoxDialog
{
public:
    void          Show(MessageBoxConfig const& cfg);
    void          Show(const char* title, const char* text, bool allow_close, const char* button1_text, const char* button2_text);
    void          Draw();
    bool          IsVisible() const { return m_is_visible; }

private:
    void          DrawButton(MessageBoxButton const& button);

    MessageBoxConfig  m_cfg;
    bool*             m_close_handle        = nullptr; //!< If nullptr, close button is hidden. Otherwise visible.
    bool              m_is_visible          = false;
};

} // namespace GUI
} // namespace RoR
