/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer

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


#include "GUI_MultiplayerClientList.h"

#include "Application.h"
#include "BeamFactory.h"
#include "GUIManager.h"
#include "Language.h"
#include "Network.h"
#include "RoRFrameListener.h"

using namespace RoR;
using namespace GUI;
using namespace Ogre;

MpClientList::MpClientList() :
    clients(0)
    , lineheight(16)
    , msgwin(0)
{
    // allocate some buffers
    clients = (client_t *)calloc(RORNET_MAX_PEERS, sizeof(client_t));

    // tooltip window
    tooltipPanel = MyGUI::Gui::getInstance().createWidget<MyGUI::Widget>("PanelSkin", 0, 0, 200, 20, MyGUI::Align::Default, "ToolTip");
    tooltipText = tooltipPanel->createWidget<MyGUI::TextBox>("TextBox", 4, 2, 200, 16, MyGUI::Align::Default);
    tooltipText->setFontName("VeraMono");
    //tooltipPanel->setAlpha(0.9f);
    tooltipText->setFontHeight(16);
    tooltipPanel->setVisible(false);

    // message window
    msgwin = MyGUI::Gui::getInstance().createWidget<MyGUI::Window>("WindowCSX", 0, 0, 400, 300, MyGUI::Align::Center, "Overlapped");
    msgwin->setCaption(_L("Player Information"));
    msgtext = msgwin->createWidget<MyGUI::Edit>("EditStretch", 0, 0, 400, 300, MyGUI::Align::Default, "helptext");
    msgtext->setCaption("");
    msgtext->setEditWordWrap(true);
    msgtext->setEditStatic(true);
    msgwin->setVisible(false);

    // network quality warning
    netmsgwin = MyGUI::Gui::getInstance().createWidget<MyGUI::Window>("FlowContainer", 5, 30, 300, 40, MyGUI::Align::Default, "Main");
    netmsgwin->setAlpha(0.8f);
    MyGUI::ImageBox* nimg = netmsgwin->createWidget<MyGUI::ImageBox>("ImageBox", 0, 0, 16, 16, MyGUI::Align::Default, "Main");
    nimg->setImageTexture("error.png");
    netmsgtext = netmsgwin->createWidget<MyGUI::TextBox>("TextBox", 18, 2, 300, 40, MyGUI::Align::Default, "helptext");
    netmsgtext->setCaption(_L("Slow  Network  Download"));
    netmsgtext->setFontName("DefaultBig");
    netmsgtext->setTextColour(MyGUI::Colour::Red);
    netmsgtext->setFontHeight(lineheight);
    netmsgwin->setVisible(false);

    // now the main GUI
    MyGUI::IntSize gui_area = MyGUI::RenderManager::getInstance().getViewSize();
    int x = gui_area.width - 300, y = 30;

    MyGUI::ImageBox* ib = MyGUI::Gui::getInstance().createWidget<MyGUI::ImageBox>("ImageBox", x, y, sidebarWidth, gui_area.height, MyGUI::Align::Default, "Main");
    ib->setImageTexture("mpbg.png");

    mpPanel = ib; //->createWidget<MyGUI::Widget>("FlowContainer", x, y, sidebarWidth, gui_area.height,  MyGUI::Align::Default, "Main");
    mpPanel->setVisible(false);

    y = 5;
    UTFString tmp;
    for (int i = 0; i < RORNET_MAX_PEERS + 1; i++) // plus 1 for local entry
    {
        x = 100; // space for icons
        player_row_t* row = &player_rows[i];
        row->playername = mpPanel->createWidget<MyGUI::TextBox>("TextBox", x, y + 1, sidebarWidth, lineheight, MyGUI::Align::Default, "Main");
        row->playername->setCaption("Player " + TOSTRING(i));
        row->playername->setFontName("DefaultBig");
        tmp = _L("user name");
        row->playername->setUserString("tooltip", tmp.asUTF8());
        row->playername->eventToolTip += MyGUI::newDelegate(this, &MpClientList::openToolTip);
        row->playername->setNeedToolTip(true);
        row->playername->setVisible(false);
        row->playername->setFontHeight(lineheight);
        row->playername->setAlpha(1);

        x -= 18;
        row->flagimg = mpPanel->createWidget<MyGUI::ImageBox>("ImageBox", x, y + 3, 16, 11, MyGUI::Align::Default, "Main");
        tmp = _L("user country");
        row->flagimg->setUserString("tooltip", tmp.asUTF8());
        row->flagimg->eventToolTip += MyGUI::newDelegate(this, &MpClientList::openToolTip);
        row->flagimg->setNeedToolTip(true);
        row->flagimg->setVisible(false);

        x -= 18;
        row->statimg = mpPanel->createWidget<MyGUI::ImageBox>("ImageBox", x, y, 16, 16, MyGUI::Align::Default, "Main");
        tmp = _L("user authentication level");
        row->statimg->setUserString("tooltip", tmp.asUTF8());
        row->statimg->eventToolTip += MyGUI::newDelegate(this, &MpClientList::openToolTip);
        row->statimg->setNeedToolTip(true);
        row->statimg->setVisible(false);

        x -= 18;
        row->user_actor_ok_img = mpPanel->createWidget<MyGUI::ImageBox>("ImageBox", x, y, 16, 16, MyGUI::Align::Default, "Main");
        tmp = _L("truck loading state");
        row->user_actor_ok_img->setUserString("tooltip", tmp.asUTF8());
        row->user_actor_ok_img->eventToolTip += MyGUI::newDelegate(this, &MpClientList::openToolTip);
        row->user_actor_ok_img->setNeedToolTip(true);
        row->user_actor_ok_img->setVisible(false);
        row->user_actor_ok_img->eventMouseButtonClick += MyGUI::newDelegate(this, &MpClientList::clickInfoIcon);

        x -= 18;
        row->user_remote_actor_ok_img = mpPanel->createWidget<MyGUI::ImageBox>("ImageBox", x, y, 16, 16, MyGUI::Align::Default, "Main");
        tmp = _L("remote truck loading state");
        row->user_remote_actor_ok_img->setUserString("tooltip", tmp.asUTF8());
        row->user_remote_actor_ok_img->eventToolTip += MyGUI::newDelegate(this, &MpClientList::openToolTip);
        row->user_remote_actor_ok_img->setNeedToolTip(true);
        row->user_remote_actor_ok_img->setVisible(false);
        row->user_remote_actor_ok_img->eventMouseButtonClick += MyGUI::newDelegate(this, &MpClientList::clickInfoIcon);

        x -= 18;
        row->usergoimg = mpPanel->createWidget<MyGUI::ImageBox>("ImageBox", x, y, 16, 16, MyGUI::Align::Default, "Main");
        row->usergoimg->setUserString("num", TOSTRING(i));
        tmp = _L("go to user");
        row->usergoimg->setUserString("tooltip", tmp.asUTF8());
        row->usergoimg->setImageTexture("user_go.png");
        row->usergoimg->eventToolTip += MyGUI::newDelegate(this, &MpClientList::openToolTip);
        row->usergoimg->setNeedToolTip(true);
        row->usergoimg->setVisible(false);
        row->usergoimg->eventMouseButtonClick += MyGUI::newDelegate(this, &MpClientList::clickUserGoIcon);

        /*
        img = MyGUI::Gui::getInstance().createWidget<MyGUI::ImageBox>("ImageBox", x-36, y, 16, 16,  MyGUI::Align::Default, "Overlapped");
        img->setImageTexture("information.png");
        img->eventMouseButtonClick += MyGUI::newDelegate(this, &MpClientList::clickInfoIcon);
        img->eventToolTip += MyGUI::newDelegate(this, &MpClientList::openToolTip);
        img->setNeedToolTip(true);
        img->setUserString("info", TOSTRING(i));
        img->setUserString("tooltip", _L("information about the user"));
        */

        y += lineheight;
    }
}

MpClientList::~MpClientList()
{
    if (clients)
    {
        free(clients);
        clients = 0;
    }
}

void MpClientList::updateSlot(player_row_t* row, RoRnet::UserInfo c, bool self)
{
    if (!row)
        return;

    int x = 100;
    int y = row->playername->getPosition().top;
    // name
    row->playername->setCaption(c.username);
#if USE_SOCKETW
    ColourValue col = Networking::GetPlayerColor(c.colournum);
    row->playername->setTextColour(MyGUI::Colour(col.r, col.g, col.b, col.a));
#endif
    row->playername->setVisible(true);
    x -= 18;

    // flag
    StringVector parts = StringUtil::split(String(c.language), "_");
    if (parts.size() == 2)
    {
        String lang = parts[1];
        StringUtil::toLowerCase(lang);
        row->flagimg->setImageTexture(lang + ".png");
        row->flagimg->setUserString("tooltip", _L("user language: ") + parts[0] + _L(" user country: ") + parts[1]);
        row->flagimg->setVisible(true);
        row->flagimg->setPosition(x, y);
        x -= 18;
    }
    else
    {
        row->flagimg->setVisible(false);
    }

    UTFString tmp;
    // auth
    if (c.authstatus == RoRnet::AUTH_NONE)
    {
        row->statimg->setVisible(false);
    }
    else if (c.authstatus & RoRnet::AUTH_ADMIN)
    {
        row->statimg->setVisible(true);
        row->statimg->setImageTexture("flag_red.png");
        tmp = _L("Server Administrator");
        row->statimg->setUserString("tooltip", tmp.asUTF8());
        row->statimg->setPosition(x, y);
        x -= 18;
    }
    else if (c.authstatus & RoRnet::AUTH_MOD)
    {
        row->statimg->setVisible(true);
        row->statimg->setImageTexture("flag_blue.png");
        tmp = _L("Server Moderator");
        row->statimg->setUserString("tooltip", tmp.asUTF8());
        row->statimg->setPosition(x, y);
        x -= 18;
    }
    else if (c.authstatus & RoRnet::AUTH_RANKED)
    {
        row->statimg->setVisible(true);
        row->statimg->setImageTexture("flag_green.png");
        tmp = _L("ranked user");
        row->statimg->setUserString("tooltip", tmp.asUTF8());
        row->statimg->setPosition(x, y);
        x -= 18;
    }

    // truck ok image
    if (!self && App::app_state.GetActive() != AppState::MAIN_MENU)
    {
        row->user_actor_ok_img->setVisible(true);
        row->user_remote_actor_ok_img->setVisible(true);
        row->user_actor_ok_img->setUserString("uid", TOSTRING(c.uniqueid));
        row->user_remote_actor_ok_img->setUserString("uid", TOSTRING(c.uniqueid));
        row->user_actor_ok_img->setPosition(x, y);
        x -= 10;
        row->user_remote_actor_ok_img->setPosition(x, y);
        x -= 10;

        int ok = App::GetSimController()->GetBeamFactory()->CheckNetworkStreamsOk(c.uniqueid);
        if (ok == 0)
        {
            row->user_actor_ok_img->setImageTexture("arrow_down_red.png");
            tmp = _L("Truck loading errors");
            row->user_actor_ok_img->setUserString("tooltip", tmp.asUTF8());
        }
        else if (ok == 1)
        {
            row->user_actor_ok_img->setImageTexture("arrow_down.png");
            tmp = _L("Truck loaded correctly, no errors");
            row->user_actor_ok_img->setUserString("tooltip", tmp.asUTF8());
        }
        else if (ok == 2)
        {
            row->user_actor_ok_img->setImageTexture("arrow_down_grey.png");
            tmp = _L("no truck loaded");
            row->user_actor_ok_img->setUserString("tooltip", tmp.asUTF8());
        }

        int rok = App::GetSimController()->GetBeamFactory()->CheckNetRemoteStreamsOk(c.uniqueid);
        if (rok == 0)
        {
            row->user_remote_actor_ok_img->setImageTexture("arrow_up_red.png");
            tmp = _L("Remote Truck loading errors");
            row->user_remote_actor_ok_img->setUserString("tooltip", tmp.asUTF8());
        }
        else if (rok == 1)
        {
            row->user_remote_actor_ok_img->setImageTexture("arrow_up.png");
            tmp = _L("Remote Truck loaded correctly, no errors");
            row->user_remote_actor_ok_img->setUserString("tooltip", tmp.asUTF8());
        }
        else if (rok == 2)
        {
            row->user_remote_actor_ok_img->setImageTexture("arrow_up_grey.png");
            tmp = _L("No Trucks loaded");
            row->user_remote_actor_ok_img->setUserString("tooltip", tmp.asUTF8());
        }
    }
    else
    {
        row->user_actor_ok_img->setVisible(false);
        row->user_remote_actor_ok_img->setVisible(false);
    }

    // user go img
    row->usergoimg->setVisible(false);
}

void MpClientList::update()
{
#ifdef USE_SOCKETW
    int slotid = 0;

    MyGUI::IntSize gui_area = MyGUI::RenderManager::getInstance().getViewSize();
    int x = gui_area.width - sidebarWidth, y = 65;
    mpPanel->setPosition(x, y);

    // add local player to first slot always
    RoRnet::UserInfo lu = RoR::Networking::GetLocalUserData();
    updateSlot(&player_rows[slotid], lu, true);
    slotid++;

    // add remote players
    std::vector<RoRnet::UserInfo> users = RoR::Networking::GetUserInfos();
    for (RoRnet::UserInfo user : users)
    {
        player_row_t* row = &player_rows[slotid];
        slotid++;
        try
        {
            updateSlot(row, user, false);
        }
        catch (...)
        {
        }
    }
    for (int i = slotid; i < RORNET_MAX_PEERS; i++)
    {
        player_row_t* row = &player_rows[i];
        // not used, hide everything
        row->flagimg->setVisible(false);
        row->playername->setVisible(false);
        row->statimg->setVisible(false);
        row->usergoimg->setVisible(false);
        row->user_actor_ok_img->setVisible(false);
        row->user_remote_actor_ok_img->setVisible(false);
    }

    netmsgwin->setVisible(RoR::Networking::GetNetQuality() != 0);

    int height = lineheight * (slotid + 1);
    mpPanel->setSize(sidebarWidth, height);
#endif
}

void MpClientList::clickUserGoIcon(MyGUI::WidgetPtr sender)
{
    //int uid = StringConverter::parseInt(sender->getUserString("uid"));
}

void MpClientList::clickInfoIcon(MyGUI::WidgetPtr sender)
{
    //msgtext->setCaption("FOOBAR: "+sender->getUserString("info"));
    //msgwin->setVisible(true);
}

void MpClientList::openToolTip(MyGUI::WidgetPtr sender, const MyGUI::ToolTipInfo& t)
{
    if (t.type == MyGUI::ToolTipInfo::Show)
    {
        String txt = sender->getUserString("tooltip");
        if (!txt.empty())
        {
            tooltipText->setCaption(txt);
            MyGUI::IntSize s = tooltipText->getTextSize();
            int newWidth = s.width + 10;
            tooltipPanel->setPosition(t.point - MyGUI::IntPoint(newWidth + 10, 10));
            tooltipPanel->setSize(newWidth, 20);
            tooltipText->setSize(newWidth, 16);
            tooltipPanel->setVisible(true);
        }
    }
    else if (t.type == MyGUI::ToolTipInfo::Hide)
    {
        tooltipPanel->setVisible(false);
    }
}

void MpClientList::SetVisible(bool value)
{
    mpPanel->setVisible(value);
}

bool MpClientList::IsVisible()
{
    return mpPanel->getVisible();
}
