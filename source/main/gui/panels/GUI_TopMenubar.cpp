/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2016-2017 Petr Ohlidal & contributors

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
/// @date   13th of August 2009
/// @author Thomas Fischer thomas{AT}thomasfischer{DOT}biz


#include "GUI_TopMenubar.h"

#include "Application.h"
#include "BeamFactory.h"
#include "Character.h"
#include "ChatSystem.h"
#include "GUIManager.h"
#include "GUI_GameConsole.h"
#include "GUI_MainSelector.h"
#include "Language.h"
#include "MainMenu.h"
#include "Network.h"
#include "RoRFrameListener.h"
#include "Settings.h"
#include "Utils.h"

#include "CameraManager.h"

namespace RoR {
namespace GUI {

using namespace Ogre;

TopMenubar::TopMenubar() :
      m_item_activate_all(nullptr)
    , m_item_never_sleep(nullptr)
    , m_item_sleep_all(nullptr)
    , m_menu_width(350)
    , m_menu_height(20)
    , m_vehicle_list_needs_update(false)
{

    /* -------------------------------------------------------------------------------- */
    /* MENU BAR */

    m_menubar_widget = MyGUI::Gui::getInstance().createWidget<MyGUI::MenuBar>("MenuBar", 0, 0, m_menu_width, m_menu_height, MyGUI::Align::Top, "Main");
    m_menubar_widget->setCoord(0, 0, m_menu_width, m_menu_height);
    m_menubar_widget->setVisible(false);

    /* -------------------------------------------------------------------------------- */
    /* SIMULATION POPUP MENU */

    MyGUI::MenuItemPtr mi = m_menubar_widget->createWidget<MyGUI::MenuItem>("MenuBarButton", 0, 0, 60, m_menu_height,  MyGUI::Align::Default);
    MyGUI::PopupMenuPtr p = mi->createWidget<MyGUI::PopupMenu>(MyGUI::WidgetStyle::Popup, "PopupMenu",MyGUI::IntCoord(0,0,88,68),MyGUI::Align::Default, "Popup");
    mi->setItemType(MyGUI::MenuItemType::Popup);
    mi->setCaption(_L("Simulation"));
    p->setPopupAccept(true);

    p->addItem(_L("Get new vehicle"),                 MyGUI::MenuItemType::Normal);
    p->addItem(_L("Show vehicle description"),        MyGUI::MenuItemType::Normal);
    p->addItem(_L("Reload current vehicle"),          MyGUI::MenuItemType::Normal);
    p->addItem(_L("Remove current vehicle"),          MyGUI::MenuItemType::Normal);

    m_item_activate_all = p->addItem(_L("Activate all vehicles"),          MyGUI::MenuItemType::Normal);
    m_item_never_sleep  = p->addItem(_L("Activated vehicles never sleep"), MyGUI::MenuItemType::Normal);
    m_item_sleep_all    = p->addItem(_L("Send all vehicles to sleep"),     MyGUI::MenuItemType::Normal);

    p->addItem("-",                                   MyGUI::MenuItemType::Separator);

    /*p->addItem(_L("Save Scenery"),                    MyGUI::MenuItemType::Normal);
    p->addItem(_L("Load Scenery"),                    MyGUI::MenuItemType::Normal);
    p->addItem("-",                                   MyGUI::MenuItemType::Separator);*/ //Disabled for the moment as far as i know -max98

    p->addItem(_L("Back to menu"),                    MyGUI::MenuItemType::Normal);
    p->addItem(_L("Exit"),                            MyGUI::MenuItemType::Normal);
    m_popup_menus.push_back(p);

    /* -------------------------------------------------------------------------------- */
    /* VEHICLES POPUP MENU */

    mi = m_menubar_widget->createWidget<MyGUI::MenuItem>("MenuBarButton", 0, 0, 60, m_menu_height,  MyGUI::Align::Default);
    m_vehicles_menu_widget = mi->createWidget<MyGUI::PopupMenu>(MyGUI::WidgetStyle::Popup, "PopupMenu", MyGUI::IntCoord(0,0,88,68),MyGUI::Align::Default, "Popup");
    p = m_vehicles_menu_widget;
    mi->setItemType(MyGUI::MenuItemType::Popup);
    mi->setCaption("Vehicles");
    m_popup_menus.push_back(p);

    /* -------------------------------------------------------------------------------- */
    /* WINDOWS POPUP MENU */

    mi = m_menubar_widget->createWidget<MyGUI::MenuItem>("MenuBarButton", 0, 0, 60, m_menu_height,  MyGUI::Align::Default);
    p = mi->createWidget<MyGUI::PopupMenu>(MyGUI::WidgetStyle::Popup, "PopupMenu",MyGUI::IntCoord(0,0,88,68),MyGUI::Align::Default, "Popup");
    mi->setItemType(MyGUI::MenuItemType::Popup);
    mi->setCaption("Windows");

    p->addItem(_L("Friction Settings"),  MyGUI::MenuItemType::Normal, "frictiongui");
    p->addItem(_L("Show Console"),       MyGUI::MenuItemType::Normal, "showConsole");
    p->addItem(_L("Texture Tool"),       MyGUI::MenuItemType::Normal, "texturetool");
    p->addItem(_L("Debug Options"),		 MyGUI::MenuItemType::Normal, "debugoptions");
    m_item_spawner_log = p->addItem(_L("Spawner log"), MyGUI::MenuItemType::Normal, "spawnerlog");
    m_popup_menus.push_back(p);

    /* -------------------------------------------------------------------------------- */
    /* DEBUG POPUP MENU */

    mi = m_menubar_widget->createWidget<MyGUI::MenuItem>("MenuBarButton", 0, 0, 60, m_menu_height,  MyGUI::Align::Default);
    p = mi->createWidget<MyGUI::PopupMenu>(MyGUI::WidgetStyle::Popup, "PopupMenu",MyGUI::IntCoord(0,0,88,68),MyGUI::Align::Default, "Popup");
    mi->setItemType(MyGUI::MenuItemType::Popup);
    mi->setCaption("Debug");
    p->addItem(_L("no visual debug"),         MyGUI::MenuItemType::Normal, "debug-none");
    p->addItem(_L("show Node numbers"),       MyGUI::MenuItemType::Normal, "debug-node-numbers");
    p->addItem(_L("show Beam numbers"),       MyGUI::MenuItemType::Normal, "debug-beam-numbers");
    p->addItem(_L("show Node&Beam numbers"),  MyGUI::MenuItemType::Normal, "debug-nodenbeam-numbers");
    p->addItem(_L("show Node mass"),          MyGUI::MenuItemType::Normal, "debug-node-mass");
    p->addItem(_L("show Node locked"),        MyGUI::MenuItemType::Normal, "debug-node-locked");
    p->addItem(_L("show Beam compression"),   MyGUI::MenuItemType::Normal, "debug-beam-compression");
    p->addItem(_L("show Beam broken"),        MyGUI::MenuItemType::Normal, "debug-beam-broken");
    p->addItem(_L("show Beam stress"),        MyGUI::MenuItemType::Normal, "debug-beam-stress");
    p->addItem(_L("show Beam strength"),      MyGUI::MenuItemType::Normal, "debug-beam-strength");
    p->addItem(_L("show Beam hydros"),        MyGUI::MenuItemType::Normal, "debug-beam-hydros");
    p->addItem(_L("show Beam commands"),      MyGUI::MenuItemType::Normal, "debug-beam-commands");
    m_popup_menus.push_back(p);

    /* -------------------------------------------------------------------------------- */
    /* MENU BAR POSITION */

    MyGUI::IntSize s = mi->getTextSize();
    m_menu_height = s.height + 6;
    m_menubar_widget->setCoord(0, 0, m_menu_width, m_menu_height);

    /* -------------------------------------------------------------------------------- */

    // event callbacks
    m_menubar_widget->eventMenuCtrlAccept += MyGUI::newDelegate(this, &TopMenubar::onMenuBtn);

    // initial mouse position somewhere so the menu is hidden
    updatePositionUponMousePosition(500, 500);
}

TopMenubar::~TopMenubar()
{
    m_menubar_widget->setVisible(false);
    m_menubar_widget->_shutdown();
    m_menubar_widget = nullptr;
}

UTFString TopMenubar::getUserString(RoRnet::UserInfo &user, int num_vehicles)
{
    UTFString tmp = RoR::ChatSystem::GetColouredName(user.username, user.colournum);

    tmp = tmp + U(": ");

    // some more info
    if (user.authstatus & RoRnet::AUTH_BOT)
        tmp = tmp + _L("#0000c9 Bot, ");
    else if (user.authstatus & RoRnet::AUTH_BANNED)
        tmp = tmp + _L("banned, ");
    else if (user.authstatus & RoRnet::AUTH_RANKED)
        tmp = tmp + _L("#00c900 Ranked, ");
    else if (user.authstatus & RoRnet::AUTH_MOD)
        tmp = tmp + _L("#c90000 Moderator, ");
    else if (user.authstatus & RoRnet::AUTH_ADMIN)
        tmp = tmp + _L("#c97100 Admin, ");

    tmp = tmp + _L("#ff8d00 version: #3eff20 ");
    tmp = tmp + ANSI_TO_UTF(user.clientversion);
    tmp = tmp + U(", ");

    tmp = tmp + _L("#ff8d00 language: #46b1f9 ");
    tmp = tmp + ANSI_TO_UTF(user.language);
    tmp = tmp + U(", ");

    if (num_vehicles == 0)
        tmp = tmp + _L("no vehicles");
    else
        tmp = tmp + TOUTFSTRING(num_vehicles) + _L(" vehicles");

    return tmp;
}

void TopMenubar::addUserToMenu(RoRnet::UserInfo &user)
{
#ifdef USE_SOCKETW
    int num_actor_slots = App::GetSimController()->GetBeamFactory()->GetNumUsedActorSlots();
    Actor **actor_slots = App::GetSimController()->GetBeamFactory()->GetInternalActorSlots();

    // now search the vehicles of that user together
    std::vector<int> matches;
    for (int j = 0; j < num_actor_slots; j++)
    {
        if (!actor_slots[j]) continue;

        if (actor_slots[j]->ar_net_source_id == user.uniqueid)
        {
            // match, found truck :)
            matches.push_back(j);
        }
    }

    // now add this user to the list
    {
        MyGUI::UString userStr = "- " + convertToMyGUIString(getUserString(user, (int)matches.size()));
        // finally add the user line
        m_vehicles_menu_widget->addItem(userStr, MyGUI::MenuItemType::Normal, "USER_"+TOSTRING(user.uniqueid));

        // and add the vehicles below the user name
        if (!matches.empty())
        {
            for (unsigned int j = 0; j < matches.size(); j++)
            {
                char tmp[512] = "";
                sprintf(tmp, "  + %s (%s)", actor_slots[matches[j]]->ar_design_name.c_str(),  actor_slots[matches[j]]->ar_filename.c_str());
                MyGUI::UString vehName = convertToMyGUIString(ANSI_TO_UTF(tmp));
                m_vehicles_menu_widget->addItem(vehName, MyGUI::MenuItemType::Normal, "TRUCK_"+TOSTRING(matches[j]));
            }
        }
    }
#endif
}

void TopMenubar::vehiclesListUpdate()
{
    m_vehicles_menu_widget->removeAllItems();

    if (!(App::mp_state.GetActive() == RoR::MpState::CONNECTED))
    {
        // single player mode: add vehicles simply, no users
        int num_actor_slots = App::GetSimController()->GetBeamFactory()->GetNumUsedActorSlots();
        Actor **actor_slots = App::GetSimController()->GetBeamFactory()->GetInternalActorSlots();

        // simple iterate through :)
        for (int i = 0; i < num_actor_slots; i++)
        {
            if (!actor_slots[i]) continue;

            if (actor_slots[i]->ar_hide_in_actor_list) continue;

            char tmp[255] = {};
            sprintf(tmp, "[%d] %s", i, actor_slots[i]->ar_design_name.c_str());

            m_vehicles_menu_widget->addItem(String(tmp), MyGUI::MenuItemType::Normal, "TRUCK_"+TOSTRING(i));
        }
    }
    else
    {
#ifdef USE_SOCKETW
        // sort the list according to the network users

        RoRnet::UserInfo local_user = RoR::Networking::GetLocalUserData();
        addUserToMenu(local_user);

        auto users = RoR::Networking::GetUserInfos();
        for (auto user : users)
        {
            addUserToMenu(user);
        }
#endif // USE_SOCKETW
    }
}

void TopMenubar::onMenuBtn(MyGUI::MenuCtrlPtr _sender, MyGUI::MenuItemPtr _item)
{
    UTFString miname = UTFString(_item->getCaption().asWStr());
    String id        = _item->getItemId();
    auto* gui_man    = RoR::App::GetGuiManager();

    if (id.substr(0,6) == "TRUCK_")
    {
        int actor_id = PARSEINT(id.substr(6));
        App::GetSimController()->SetPlayerActorById(actor_id); // Silently fails if the actor_id is dead.
    }

    if (id.substr(0,5) == "USER_")
    {
        int user_uid = PARSEINT(id.substr(5));

        // cannot whisper with self...
#ifdef USE_SOCKETW
        if (user_uid == RoR::Networking::GetUID()) return;
#endif // USE_SOCKETW

        //RoR::App::GetConsole()->startPrivateChat(user_uid);
        //TODO: Separate Chat and console
    }

    const auto app_state = App::app_state.GetActive();
    if (app_state == RoR::AppState::BOOTSTRAP)
    {
        return;
    }

    if (_item == m_item_spawner_log)
    {
        App::GetGuiManager()->SetVisible_SpawnerReport(true);
    }
    else if (miname == _L("Get new vehicle") && gEnv->player)
    {
        if (app_state != RoR::AppState::SIMULATION)
        {
            return;
        }
        App::sim_state.SetActive(RoR::SimState::SELECTING); // TODO: use 'pending' mechanism
        gui_man->GetMainSelector()->Show(LT_AllBeam);

    } else if (miname == _L("Reload current vehicle") && gEnv->player)
    {
        if (App::GetSimController()->GetPlayerActor() != nullptr)
        {
            App::GetSimController()->ReloadPlayerActor(); // TODO: Use SIM_STATE + 'pending' mechanisms
            gui_man->UnfocusGui();
        }
    }
    else if (miname == _L("Back to menu"))
    {
        App::app_state.SetPending(RoR::AppState::MAIN_MENU);
    }
    else if (miname == _L("Remove current vehicle"))
    {
        App::GetSimController()->RemovePlayerActor();

    } else if (miname == _L("Activate all vehicles"))
    {
        App::GetSimController()->GetBeamFactory()->WakeUpAllActors();

    } else if (miname == _L("Activated vehicles never sleep"))
    {
        App::GetSimController()->GetBeamFactory()->SetTrucksForcedAwake(true);
        _item->setCaption(_L("Activated vehicles can sleep"));

    } else if (miname == _L("Activated vehicles can sleep"))
    {
        App::GetSimController()->GetBeamFactory()->SetTrucksForcedAwake(false);
        _item->setCaption(_L("Activated vehicles never sleep"));

    } else if (miname == _L("Send all vehicles to sleep"))
    {
        // get out first
        if (App::GetSimController()->GetPlayerActor() != nullptr)
        {
            App::GetSimController()->SetPlayerActor(nullptr);
        }
        App::GetSimController()->GetBeamFactory()->SendAllActorsSleeping();

    } else if (miname == _L("Friction Settings"))
    {
        App::GetGuiManager()->SetVisible_FrictionSettings(true);
    } else if (miname == _L("Exit"))
    {
        App::app_state.SetPending(RoR::AppState::SHUTDOWN);
    } else if (miname == _L("Show Console"))
    {
        gui_man->SetVisible_Console(! gui_man->IsVisible_Console());
    }
    // the debug menu
    else if (miname == _L("no visual debug"))
    {
        Actor *b = App::GetSimController()->GetPlayerActor();
        if (b) b->setDebugOverlayState(0);
    } else if (miname == _L("show Node numbers"))
    {
        Actor *b = App::GetSimController()->GetPlayerActor();
        if (b) b->setDebugOverlayState(1);
    } else if (miname == _L("show Beam numbers"))
    {
        Actor *b = App::GetSimController()->GetPlayerActor();
        if (b) b->setDebugOverlayState(2);
    } else if (miname == _L("show Node&Beam numbers"))
    {
        Actor *b = App::GetSimController()->GetPlayerActor();
        if (b) b->setDebugOverlayState(3);
    } else if (miname == _L("show Node mass"))
    {
        Actor *b = App::GetSimController()->GetPlayerActor();
        if (b) b->setDebugOverlayState(4);
    } else if (miname == _L("show Node locked"))
    {
        Actor *b = App::GetSimController()->GetPlayerActor();
        if (b) b->setDebugOverlayState(5);
    } else if (miname == _L("show Beam compression"))
    {
        Actor *b = App::GetSimController()->GetPlayerActor();
        if (b) b->setDebugOverlayState(6);
    } else if (miname == _L("show Beam broken"))
    {
        Actor *b = App::GetSimController()->GetPlayerActor();
        if (b) b->setDebugOverlayState(7);
    } else if (miname == _L("show Beam stress"))
    {
        Actor *b = App::GetSimController()->GetPlayerActor();
        if (b) b->setDebugOverlayState(8);
    } else if (miname == _L("show Beam strength"))
    {
        Actor *b = App::GetSimController()->GetPlayerActor();
        if (b) b->setDebugOverlayState(9);
    } else if (miname == _L("show Beam hydros"))
    {
        Actor *b = App::GetSimController()->GetPlayerActor();
        if (b) b->setDebugOverlayState(10);
    } else if (miname == _L("show Beam commands"))
    {
        Actor *b = App::GetSimController()->GetPlayerActor();
        if (b) b->setDebugOverlayState(11);
    }
    else if (miname == _L("Texture Tool"))
    {
        gui_man->SetVisible_TextureToolWindow(true);
    }
    else if (miname == _L("Debug Options"))
    {
        gui_man->SetVisible_DebugOptions(true);
    }
    else if (miname == _L("Show vehicle description"))
    {
        if (App::GetSimController()->GetPlayerActor() != 0)
        {
            gui_man->SetVisible_VehicleDescription(true);
        }
    }
}

void TopMenubar::SetVisible(bool value)
{
    m_menubar_widget->setVisible(value);
    if (!value) RoR::App::GetGuiManager()->UnfocusGui();
}

bool TopMenubar::IsVisible()
{
    return m_menubar_widget->getVisible();
}

void TopMenubar::updatePositionUponMousePosition(int x, int y)
{
    int h = m_menubar_widget->getHeight();
    bool focused = false;
    for (unsigned int i=0;i<m_popup_menus.size(); i++)
        focused |= m_popup_menus[i]->getVisible();

    if (focused)
    {
        m_menubar_widget->setPosition(0, 0);
    } else
    {
        if (y > 2*h)
            m_menubar_widget->setPosition(0, -h);

        else
            m_menubar_widget->setPosition(0, std::min(0, -y+10));
    }

    // this is hacky, but needed as the click callback is not working
    if (m_vehicle_list_needs_update)
    {
        vehiclesListUpdate();
        m_vehicle_list_needs_update = false;
    }
}

void TopMenubar::triggerUpdateVehicleList()
{
    m_vehicle_list_needs_update = true;
}

void TopMenubar::ReflectMultiplayerState()
{
    const bool online = App::mp_state.GetActive() == RoR::MpState::CONNECTED;
    m_item_activate_all->setEnabled(!online);
    m_item_never_sleep ->setEnabled(!online);
    m_item_sleep_all   ->setEnabled(!online);
    triggerUpdateVehicleList();
}

} // namespace GUI
} // namespace RoR
