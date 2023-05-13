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
/// @brief  Generic UI dialog (not modal). Invocable from scripting.
/// @author Moncef Ben Slimane, 2014
/// @author Petr Ohlidal, 2021 - extended config.

#include "GUI_MessageBox.h"

#include "Application.h"
#include "Language.h"
#include "GameContext.h"
#include "GUIManager.h"
#include "ScriptEngine.h"

#include <imgui.h>

using namespace RoR;
using namespace GUI;

    // Setup functions

void MessageBoxDialog::Show(MessageBoxConfig const& cfg)
{
    m_is_visible = true;
    m_cfg = cfg;

    if (m_cfg.mbc_close_handle)
    {
        m_close_handle = m_cfg.mbc_close_handle;
    }
    else if (m_cfg.mbc_allow_close)
    {
        m_close_handle = &m_is_visible;
    }
    else
    {
        m_close_handle = nullptr;
    }
}

void MessageBoxDialog::Show(const char* title, const char* text, bool allow_close, const char* button1_text, const char* button2_text)
{
    MessageBoxConfig conf;
    conf.mbc_title = title;
    conf.mbc_text = text;
    conf.mbc_allow_close = allow_close;

    if (button1_text)
    {
        MessageBoxButton button;
        button.mbb_caption = button1_text;
        button.mbb_script_number = 1; // Standard
        conf.mbc_buttons.push_back(button);
    }

    if (button2_text)
    {
        MessageBoxButton button;
        button.mbb_caption = button2_text;
        button.mbb_script_number = 2; // Standard
        conf.mbc_buttons.push_back(button);
    }

    this->Show(conf);
}

    // Draw funcions

void MessageBoxDialog::Draw()
{
    const bool was_visible = m_is_visible;

    // Draw window
    ImGui::SetNextWindowContentWidth(300.f); // Initial size only
    ImGui::SetNextWindowPosCenter(ImGuiCond_Appearing); // Initial pos. only
    ImGui::Begin(m_cfg.mbc_title.c_str(), m_close_handle);
    ImGui::TextWrapped("%s", m_cfg.mbc_text.c_str());

    // Draw buttons
    for (MessageBoxButton const& button: m_cfg.mbc_buttons)
    {
        this->DrawButton(button);
    }

    // Draw "always ask"
    if (m_cfg.mbc_always_ask_conf)
    {
        ImGui::Separator();
        bool ask = m_cfg.mbc_always_ask_conf->getBool();
        if (ImGui::Checkbox(_LC("MessageBox", "Always ask"), &ask))
        {
            m_cfg.mbc_always_ask_conf->setVal(ask);
        }
    }

    // Finalize
    App::GetGuiManager()->RequestGuiCaptureKeyboard(ImGui::IsWindowHovered());
    ImGui::End();
    if (m_is_visible != was_visible)
    {
        TRIGGER_EVENT_ASYNC(SE_GENERIC_MESSAGEBOX_CLICK, 0); // Scripting
    }
}

void MessageBoxDialog::DrawButton(MessageBoxButton const& button)
{
    if (ImGui::Button(button.mbb_caption.c_str()))
    {
        ROR_ASSERT(button.mbb_script_number != 0); // Reserved value (close button)

        if (button.mbb_script_number > 0)
        {
            TRIGGER_EVENT_ASYNC(SE_GENERIC_MESSAGEBOX_CLICK, button.mbb_script_number); // Scripting
        }

        if (button.mbb_mq_message != MsgType::MSG_INVALID)
        {
            Message m(button.mbb_mq_message);
            m.description = button.mbb_mq_description;
            m.payload = button.mbb_mq_payload;
            App::GetGameContext()->PushMessage(m);
        }

        m_is_visible = false;
    }
}

