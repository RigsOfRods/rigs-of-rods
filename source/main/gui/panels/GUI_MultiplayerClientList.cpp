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
/// @author Thomas Fischer (thomas{AT}thomasfischer{DOT}biz)
/// @date   7th of September 2009

/// @author Remake to DearIMGUI: Petr Ohlidal, 11/2019


#include "GUI_MultiplayerClientList.h"

#include "Application.h"
#include "ActorManager.h"
#include "GameContext.h"
#include "GUIManager.h"
#include "GUIUtils.h"
#include "Language.h"
#include "Network.h"

#include <vector>

using namespace RoR;
using namespace GUI;
using namespace Ogre;

void MpClientList::UpdateClients()
{
#if USE_SOCKETW
    // vectorpos may change, so we must look up ID again.
    int peeropts_menu_active_user_uid = -1;
    if (m_peeropts_menu_active_user_vectorpos != -1)
    {
        peeropts_menu_active_user_uid = (int)m_users[m_peeropts_menu_active_user_vectorpos].uniqueid;
    }
    m_peeropts_menu_active_user_vectorpos = -1;

    // Update the user data
    m_users = App::GetNetwork()->GetUserInfos();
    m_users.insert(m_users.begin(), App::GetNetwork()->GetLocalUserData());

    m_users_peeropts = App::GetNetwork()->GetAllUsersPeerOpts();
    m_users_peeropts.insert(m_users_peeropts.begin(), BitMask_t(0));

    // Restore the vectorpos
    for (int i = 0; i < (int)m_users.size(); i++)
    {
        if ((int)m_users[i].uniqueid == peeropts_menu_active_user_uid)
        {
            m_peeropts_menu_active_user_vectorpos = i;
        }
    }
#endif // USE_SOCKETW
}

void MpClientList::Draw()
{
#if USE_SOCKETW
    if (m_users.empty())
        return; // UpdateClients() wasn't called yet.

    if (!m_icons_cached)
    {
        this->CacheIcons();
    }

    GUIManager::GuiTheme const& theme = App::GetGuiManager()->GetTheme();

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar;
    const float content_width = 225.f;
    ImGui::SetNextWindowContentWidth(content_width);
    ImGui::SetNextWindowPos(ImVec2(
        ImGui::GetIO().DisplaySize.x - (content_width + (2*ImGui::GetStyle().WindowPadding.x) + theme.screen_edge_padding.x),
        theme.screen_edge_padding.y));

    int y = 20 + (ImGui::GetTextLineHeightWithSpacing() * m_users.size());

    if (App::GetNetwork()->GetNetQuality() != 0)
    {
        y += 20;
    }

    ImGui::SetNextWindowSize(ImVec2((content_width + (2*ImGui::GetStyle().WindowPadding.x)), y));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, theme.semitransparent_window_bg);
    ImGui::Begin("Peers", nullptr, flags);

    const RoRnet::UserInfo& local_user = m_users[0]; // See `UpdateClients()`
    int vectorpos = 0;
    for (RoRnet::UserInfo const& user: m_users)
    {
        ImGui::PushID(user.uniqueid);
        const ImVec2 hover_tl = ImGui::GetCursorScreenPos();

        // PeerOptions popup menu
        if (ImGui::Button(" < "))
        {
            if (m_peeropts_menu_active_user_vectorpos == vectorpos)
            {
                m_peeropts_menu_active_user_vectorpos = -1; // hide menu
            }
            else
            {
                m_peeropts_menu_active_user_vectorpos = vectorpos; // show menu
                m_peeropts_menu_corner_tl = hover_tl - ImVec2(PEEROPTS_MENU_WIDTH + 15, 0);
            }
        }
        ImGui::SameLine();

        // Icon sizes: flag(16x11), auth(16x16), up(16x16), down(16x16)
        Ogre::TexturePtr flag_tex;
        Ogre::TexturePtr auth_tex;
        Ogre::TexturePtr down_tex;
        Ogre::TexturePtr up_tex;

        // Stream state indicators
        if (user.uniqueid != local_user.uniqueid &&
            App::app_state->getEnum<AppState>() != AppState::MAIN_MENU)
        {
            switch (App::GetGameContext()->GetActorManager()->CheckNetworkStreamsOk(user.uniqueid))
            {
            case 0: down_tex = m_icon_arrow_down_red;  break;
            case 1: down_tex = m_icon_arrow_down;      break;
            case 2: down_tex = m_icon_arrow_down_grey; break;
            default:;
            }
            

            switch (App::GetGameContext()->GetActorManager()->CheckNetRemoteStreamsOk(user.uniqueid))
            {
            case 0: up_tex = m_icon_arrow_up_red;  break;
            case 1: up_tex = m_icon_arrow_up;      break;
            case 2: up_tex = m_icon_arrow_up_grey; break;
            default:;
            }
        }
        // Always invoke to keep usernames aligned
        this->DrawIcon(down_tex, ImVec2(8.f, ImGui::GetTextLineHeight()));
        this->DrawIcon(up_tex, ImVec2(8.f, ImGui::GetTextLineHeight()));

        // Auth icon
             if (user.authstatus & RoRnet::AUTH_ADMIN ) { auth_tex = m_icon_flag_red;   }
        else if (user.authstatus & RoRnet::AUTH_MOD   ) { auth_tex = m_icon_flag_blue;  }
        else if (user.authstatus & RoRnet::AUTH_RANKED) { auth_tex = m_icon_flag_green; }

        this->DrawIcon(auth_tex, ImVec2(14.f, ImGui::GetTextLineHeight()));

        // Country flag
        StringVector parts = StringUtil::split(user.language, "_");
        if (parts.size() == 2)
        {
            StringUtil::toLowerCase(parts[1]);
            flag_tex = FetchIcon((parts[1] + ".png").c_str());
            this->DrawIcon(flag_tex, ImVec2(16.f, ImGui::GetTextLineHeight()));
        }

        // Player name
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetStyle().ItemSpacing.x); // Some extra padding
        ColourValue col = App::GetNetwork()->GetPlayerColor(user.colournum);
        ImGui::TextColored(ImVec4(col.r, col.g, col.b, col.a), "%s", user.username);
        const ImVec2 hover_br = hover_tl + ImVec2(content_width, ImGui::GetTextLineHeight());
        const float HOVER_TL_SHIFTX = 20.f; // leave the [<] button (PeerOptions submenu) out of the hover check.
        const bool hovered
             = hover_br.x > ImGui::GetIO().MousePos.x
            && hover_br.y > ImGui::GetIO().MousePos.y
            && ImGui::GetIO().MousePos.x > (hover_tl.x + HOVER_TL_SHIFTX)
            && ImGui::GetIO().MousePos.y > hover_tl.y;

        // Tooltip
        if (hovered)
        {
            ImGui::BeginTooltip();

            // TextDisabled() are captions, Text() are values

            ImGui::TextDisabled("%s: ",_LC("MultiplayerClientList", "user name"));
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(col.r, col.g, col.b, col.a), "%s", user.username);
            ImGui::Separator();

            ImGui::TextDisabled("%s: ",_LC("MultiplayerClientList", "user uid"));
            ImGui::SameLine();
            ImGui::Text("%d", user.uniqueid);

            ImGui::TextDisabled("%s", _LC("MultiplayerClientList", "user language: "));
            ImGui::SameLine();
            ImGui::Text("%s", parts[0].c_str());

            ImGui::TextDisabled("%s", _LC("MultiplayerClientList", "user country: "));
            ImGui::SameLine();
            ImGui::Text("%s", parts[1].c_str());
            if (flag_tex)
            {
                ImGui::SameLine();
                ImGui::Image(reinterpret_cast<ImTextureID>(flag_tex->getHandle()),
                    ImVec2(flag_tex->getWidth(), flag_tex->getHeight()));
            }

            ImGui::Separator();
            ImGui::TextDisabled("%s", _LC("MultiplayerClientList", "user authentication level"));
            if (auth_tex)
            {
                ImGui::Image(reinterpret_cast<ImTextureID>(auth_tex->getHandle()),
                    ImVec2(auth_tex->getWidth(), auth_tex->getHeight()));
                ImGui::SameLine();
            }

            ImGui::Text("%s", App::GetNetwork()->UserAuthToStringLong(user).c_str());

            // Stream state
            if (user.uniqueid != local_user.uniqueid &&
                App::app_state->getEnum<AppState>() != AppState::MAIN_MENU)
            {
                ImGui::Separator();
                ImGui::TextDisabled("%s", _LC("MultiplayerClientList", "truck loading state"));
                if (down_tex)
                {
                    ImGui::Image(reinterpret_cast<ImTextureID>(down_tex->getHandle()),
                        ImVec2(down_tex->getWidth(), down_tex->getHeight()));
                    ImGui::SameLine();
                }
                switch (App::GetGameContext()->GetActorManager()->CheckNetworkStreamsOk(user.uniqueid))
                {
                case 0: ImGui::Text("%s", _LC("MultiplayerClientList", "Truck loading errors")); break;
                case 1: ImGui::Text("%s", _LC("MultiplayerClientList", "Truck loaded correctly, no errors")); break;
                case 2: ImGui::Text("%s", _LC("MultiplayerClientList", "no truck loaded")); break;
                default:; // never happens
                }

                ImGui::TextDisabled("%s", _LC("MultiplayerClientList", "remote truck loading state"));
                if (up_tex)
                {
                    ImGui::Image(reinterpret_cast<ImTextureID>(up_tex->getHandle()),
                        ImVec2(up_tex->getWidth(), up_tex->getHeight()));
                    ImGui::SameLine();
                }
                switch (App::GetGameContext()->GetActorManager()->CheckNetRemoteStreamsOk(user.uniqueid))
                {
                case 0: ImGui::Text("%s", _LC("MultiplayerClientList", "Remote Truck loading errors")); break;
                case 1: ImGui::Text("%s", _LC("MultiplayerClientList", "Remote Truck loaded correctly, no errors")); break;
                case 2: ImGui::Text("%s", _LC("MultiplayerClientList", "No Trucks loaded")); break;
                default:; // never happens
                }
            }

            ImGui::EndTooltip();
        }
        ImGui::PopID(); // user.uniqueid
        vectorpos++;
    }

    if (App::GetNetwork()->GetNetQuality() != 0)
    {
        ImGui::Separator();
        ImGui::Image(reinterpret_cast<ImTextureID>(m_icon_warn_triangle->getHandle()),
            ImVec2(m_icon_warn_triangle->getWidth(), m_icon_warn_triangle->getHeight()));
        ImGui::SameLine();
        ImGui::TextColored(App::GetGuiManager()->GetTheme().error_text_color, "%s", _LC("MultiplayerClientList", "Slow  Network  Download"));
    }

    ImGui::End();
    ImGui::PopStyleColor(1); // WindowBg

    this->DrawPeerOptionsMenu();
#endif // USE_SOCKETW
}

void MpClientList::DrawPeerOptCheckbox(const BitMask_t flag, const std::string& label)
{
    int uid = (int)m_users[m_peeropts_menu_active_user_vectorpos].uniqueid;
    bool flagval = m_users_peeropts[m_peeropts_menu_active_user_vectorpos] & flag;

    if (ImGui::Checkbox(label.c_str(), &flagval))
    {
        MsgType peeropt_msg = flagval ? MSG_NET_ADD_PEEROPTIONS_REQUESTED : MSG_NET_REMOVE_PEEROPTIONS_REQUESTED;
        App::GetGameContext()->PushMessage(Message(peeropt_msg, new PeerOptionsRequest{ uid, flag }));
        App::GetGameContext()->ChainMessage(Message(MSG_GUI_MP_CLIENTS_REFRESH));
    }
}

void MpClientList::DrawPeerOptionsMenu()
{
    if (m_peeropts_menu_active_user_vectorpos == -1)
        return; // Menu not visible

    // Sanity check
    const bool vectorpos_sane = (m_peeropts_menu_active_user_vectorpos >= 0
        && m_peeropts_menu_active_user_vectorpos < (int)m_users_peeropts.size());
    ROR_ASSERT(vectorpos_sane);
    if (!vectorpos_sane)
        return; // Minimize damage

    // Draw UI
    ImGui::SetNextWindowPos(m_peeropts_menu_corner_tl);
    const int flags = ImGuiWindowFlags_NoDecoration;
    if (ImGui::Begin("PeerOptions", nullptr, flags))
    {
        ImGui::TextDisabled("%s", _LC("MultiplayerClientList", "Peer options"));
        ImGui::Separator();
        this->DrawPeerOptCheckbox(RoRnet::PEEROPT_MUTE_CHAT, _LC("MultiplayerClientList", "Mute chat"));
        this->DrawPeerOptCheckbox(RoRnet::PEEROPT_MUTE_ACTORS, _LC("MultiplayerClientList", "Mute actors"));
        this->DrawPeerOptCheckbox(RoRnet::PEEROPT_HIDE_ACTORS, _LC("MultiplayerClientList", "Hide actors"));
        m_peeropts_menu_corner_br = ImGui::GetCursorScreenPos();

        ImGui::End();
    }

    // Check hover and hide
    const ImVec2 hoverbox_tl = m_peeropts_menu_corner_tl - ImVec2(PEEROPTS_HOVER_PAD, PEEROPTS_HOVER_PAD);
    const ImVec2 hoverbox_br = m_peeropts_menu_corner_br + ImVec2(PEEROPTS_HOVER_PAD, PEEROPTS_HOVER_PAD);
    const ImVec2 mousepos = ImGui::GetIO().MousePos;
    if (mousepos.x < hoverbox_tl.x || mousepos.x > hoverbox_br.x
        || mousepos.y < hoverbox_tl.y || mousepos.y > hoverbox_br.y)
    {
        m_peeropts_menu_active_user_vectorpos = -1;
    }
}

void MpClientList::DrawIcon(Ogre::TexturePtr tex, ImVec2 reference_box)
{
    ImVec2 orig_pos = ImGui::GetCursorPos();
    if (tex)
    {
   // TODO: moving the cursor somehow deforms the image
   //     ImGui::SetCursorPosX(orig_pos.x + (reference_box.x - tex->getWidth()) / 2.f);
   //     ImGui::SetCursorPosY(orig_pos.y + (reference_box.y - tex->getHeight()) / 2.f);
        ImGui::Image(reinterpret_cast<ImTextureID>(tex->getHandle()), ImVec2(16, 16));
    }
    ImGui::SetCursorPosX(orig_pos.x + reference_box.x + ImGui::GetStyle().ItemSpacing.x);
    ImGui::SetCursorPosY(orig_pos.y);
}

void MpClientList::CacheIcons()
{
    m_icon_arrow_down      = FetchIcon("arrow_down.png");
    m_icon_arrow_down_grey = FetchIcon("arrow_down_grey.png");
    m_icon_arrow_down_red  = FetchIcon("arrow_down_red.png");
    m_icon_arrow_up        = FetchIcon("arrow_up.png");
    m_icon_arrow_up_grey   = FetchIcon("arrow_up_grey.png");
    m_icon_arrow_up_red    = FetchIcon("arrow_up_red.png");
    m_icon_flag_red        = FetchIcon("flag_red.png");
    m_icon_flag_blue       = FetchIcon("flag_blue.png");
    m_icon_flag_green      = FetchIcon("flag_green.png");
    m_icon_warn_triangle   = FetchIcon("error.png");

    m_icons_cached = true;
}
