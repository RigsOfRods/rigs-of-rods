/*
This source file is part of Rigs of Rods
Copyright 2005-2012 Pierre-Michel Ricordel
Copyright 2007-2012 Thomas Fischer

For more information, see http://www.rigsofrods.com/

Rigs of Rods is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License version 3, as
published by the Free Software Foundation.

Rigs of Rods is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Rigs of Rods.  If not, see <http://www.gnu.org/licenses/>.
*/
// created by Thomas Fischer thomas{AT}thomasfischer{DOT}biz, 13th of August 2009
#ifdef USE_MYGUI

#include "GUIMenu.h"

#include "BeamFactory.h"
#include "Character.h"
#include "ChatSystem.h"
#include "Console.h"
#include "GUIFriction.h"
#include "GUIManager.h"
#include "Language.h"
#include "Network.h"
#include "RoRFrameListener.h"
#include "Savegame.h"
#include "SelectorWindow.h"
#include "Settings.h"
#include "TextureToolWindow.h"
#include "Utils.h"

#include "CameraManager.h"

using namespace Ogre;

GUI_MainMenu::GUI_MainMenu() :
	  menuWidth(800)
	, menuHeight(20)
	, vehicleListNeedsUpdate(false)
{
	setSingleton(this);
	pthread_mutex_init(&updateLock, NULL);

	//MyGUI::WidgetPtr back = createWidget<MyGUI::Widget>("Panel", 0, 0, 912, 652,MyGUI::Align::Default, "Back");
	mainmenu = MyGUI::Gui::getInstance().createWidget<MyGUI::MenuBar>("MenuBar", 0, 0, menuWidth, menuHeight,  MyGUI::Align::HStretch | MyGUI::Align::Top, "Back");
	mainmenu->setCoord(0, 0, menuWidth, menuHeight);
	
	// === Simulation menu
	MyGUI::MenuItemPtr mi = mainmenu->createWidget<MyGUI::MenuItem>("MenuBarButton", 0, 0, 60, menuHeight,  MyGUI::Align::Default);
	MyGUI::PopupMenuPtr p = mi->createWidget<MyGUI::PopupMenu>(MyGUI::WidgetStyle::Popup, "PopupMenu",MyGUI::IntCoord(0,0,88,68),MyGUI::Align::Default, "Popup");
	mi->setItemType(MyGUI::MenuItemType::Popup);
	mi->setCaption(_L("Simulation"));
	p->setPopupAccept(true);
	//mi->setPopupAccept(true);

	MyGUI::IntSize s = mi->getTextSize();
	menuHeight = s.height + 6;
	mainmenu->setCoord(0, 0, menuWidth, menuHeight);

	
	p->addItem(_L("get new Vehicle"), MyGUI::MenuItemType::Normal);
	p->addItem(_L("reload current Vehicle"), MyGUI::MenuItemType::Normal);
	p->addItem(_L("remove current Vehicle"), MyGUI::MenuItemType::Normal);
	p->addItem(_L("activate all Vehicles"), MyGUI::MenuItemType::Normal);
	p->addItem(_L("activated Vehicles never sleep"), MyGUI::MenuItemType::Normal);
	p->addItem(_L("send all Vehicles to sleep"), MyGUI::MenuItemType::Normal);
	p->addItem("-", MyGUI::MenuItemType::Separator);
	p->addItem(_L("Save Scenery"), MyGUI::MenuItemType::Normal);
	p->addItem(_L("Load Scenery"), MyGUI::MenuItemType::Normal);
	//p->addItem("-", MyGUI::MenuItemType::Separator);
	//p->addItem(_L("Terrain Editor Mode"), MyGUI::MenuItemType::Normal);
	p->addItem("-", MyGUI::MenuItemType::Separator);
	p->addItem(_L("Exit"), MyGUI::MenuItemType::Normal);
	pop.push_back(p);

	// === vehicles
	mi = mainmenu->createWidget<MyGUI::MenuItem>("MenuBarButton", 0, 0, 60, menuHeight,  MyGUI::Align::Default);
	vehiclesMenu = mi->createWidget<MyGUI::PopupMenu>(MyGUI::WidgetStyle::Popup, "PopupMenu", MyGUI::IntCoord(0,0,88,68),MyGUI::Align::Default, "Popup");
	p = vehiclesMenu;
	mi->setItemType(MyGUI::MenuItemType::Popup);
	mi->setCaption("Vehicles");
	pop.push_back(p);

	// this is not working :(
	//vehiclesMenu->setPopupAccept(true);
	//vehiclesMenu->eventMenuCtrlAccept += MyGUI::newDelegate(this, &GUI_MainMenu::onVehicleMenu);


	/*
	// === view
	mi = mainmenu->createWidget<MyGUI::MenuItem>("MenuBarButton", 0, 0, 60, menuHeight,  MyGUI::Align::Default);
	p = mi->createWidget<MyGUI::PopupMenu>(MyGUI::WidgetStyle::Popup, "PopupMenu",MyGUI::IntCoord(0,0,88,68),MyGUI::Align::Default, "Popup");
	mi->setItemType(MyGUI::MenuItemType::Popup);
	mi->setCaption(_L("View"));
	MyGUI::MenuItemPtr mi2 = p->createWidget<MyGUI::MenuItem>("MenuBarButton", 0, 0, 60, menuHeight,  MyGUI::Align::Default);
	mi2->setItemType(MyGUI::MenuItemType::Popup);
	mi2->setCaption(_L("Camera Mode"));

	p = mi2->createWidget<MyGUI::PopupMenu>(MyGUI::WidgetStyle::Popup, "PopupMenu",MyGUI::IntCoord(0,0,88,68),MyGUI::Align::Default, "Popup");
	p->addItem(_L("First Person"), MyGUI::MenuItemType::Normal, "camera_first_person");
	p->addItem(_L("External"), MyGUI::MenuItemType::Normal, "camera_external");
	p->addItem(_L("Free Mode"), MyGUI::MenuItemType::Normal, "camera_free");
	p->addItem(_L("Free fixed mode"), MyGUI::MenuItemType::Normal, "camera_freefixed");
	p->addItem("-", MyGUI::MenuItemType::Separator);

	mi2 = p->createWidget<MyGUI::MenuItem>("MenuBarButton", 0, 0, 60, menuHeight,  MyGUI::Align::Default);
	mi2->setItemType(MyGUI::MenuItemType::Popup);
	mi2->setCaption(_L("Truck Camera"));

	p = mi2->createWidget<MyGUI::PopupMenu>(MyGUI::WidgetStyle::Popup, "PopupMenu",MyGUI::IntCoord(0,0,88,68),MyGUI::Align::Default, "Popup");
	p->addItem(_L("1. Camera"), MyGUI::MenuItemType::Normal, "camera_truck_1");
	p->addItem(_L("2. Camera"), MyGUI::MenuItemType::Normal, "camera_truck_2");
	p->addItem(_L("3. Camera"), MyGUI::MenuItemType::Normal, "camera_truck_3");
	*/

	// === windows
	mi = mainmenu->createWidget<MyGUI::MenuItem>("MenuBarButton", 0, 0, 60, menuHeight,  MyGUI::Align::Default);
	p = mi->createWidget<MyGUI::PopupMenu>(MyGUI::WidgetStyle::Popup, "PopupMenu",MyGUI::IntCoord(0,0,88,68),MyGUI::Align::Default, "Popup");
	mi->setItemType(MyGUI::MenuItemType::Popup);
	mi->setCaption("Windows");
	//p->addItem(_L("Camera Control"), MyGUI::MenuItemType::Normal, "cameratool");
	p->addItem(_L("Friction Settings"), MyGUI::MenuItemType::Normal, "frictiongui");
	p->addItem(_L("Show Console"), MyGUI::MenuItemType::Normal, "showConsole");
	p->addItem(_L("Texture Tool"), MyGUI::MenuItemType::Normal, "texturetool");
	pop.push_back(p);

	// === debug
	mi = mainmenu->createWidget<MyGUI::MenuItem>("MenuBarButton", 0, 0, 60, menuHeight,  MyGUI::Align::Default);
	p = mi->createWidget<MyGUI::PopupMenu>(MyGUI::WidgetStyle::Popup, "PopupMenu",MyGUI::IntCoord(0,0,88,68),MyGUI::Align::Default, "Popup");
	mi->setItemType(MyGUI::MenuItemType::Popup);
	mi->setCaption("Debug");
	p->addItem(_L("no visual debug"), MyGUI::MenuItemType::Normal, "debug-none");
	p->addItem(_L("show Node numbers"), MyGUI::MenuItemType::Normal, "debug-node-numbers");
	p->addItem(_L("show Beam numbers"), MyGUI::MenuItemType::Normal, "debug-beam-numbers");
	p->addItem(_L("show Node&Beam numbers"), MyGUI::MenuItemType::Normal, "debug-nodenbeam-numbers");
	p->addItem(_L("show Node mass"), MyGUI::MenuItemType::Normal, "debug-node-mass");
	p->addItem(_L("show Node locked"), MyGUI::MenuItemType::Normal, "debug-node-locked");
	p->addItem(_L("show Beam compression"), MyGUI::MenuItemType::Normal, "debug-beam-compression");
	p->addItem(_L("show Beam broken"), MyGUI::MenuItemType::Normal, "debug-beam-broken");
	p->addItem(_L("show Beam stress"), MyGUI::MenuItemType::Normal, "debug-beam-stress");
	p->addItem(_L("show Beam strength"), MyGUI::MenuItemType::Normal, "debug-beam-strength");
	p->addItem(_L("show Beam hydros"), MyGUI::MenuItemType::Normal, "debug-beam-hydros");
	p->addItem(_L("show Beam commands"), MyGUI::MenuItemType::Normal, "debug-beam-commands");
	pop.push_back(p);

	

	// event callbacks
	mainmenu->eventMenuCtrlAccept += MyGUI::newDelegate(this, &GUI_MainMenu::onMenuBtn);

	// initial mouse position somewhere so the menu is hidden
	updatePositionUponMousePosition(500, 500);
}

GUI_MainMenu::~GUI_MainMenu()
{
}

UTFString GUI_MainMenu::getUserString(user_info_t &user, int num_vehicles)
{
	UTFString tmp = ChatSystem::getColouredName(user);

	tmp = tmp + U(" #000000(");

	// some more info
	if (user.authstatus & AUTH_ADMIN)
		tmp = tmp + _L("#c97100admin#000000, ");
	if (user.authstatus & AUTH_RANKED)
		tmp = tmp + _L("#00c900ranked#000000, ");
	if (user.authstatus & AUTH_MOD)
		tmp = tmp + _L("#c90000moderator#000000, ");
	if (user.authstatus & AUTH_BANNED)
		tmp = tmp + _L("banned, ");
	if (user.authstatus & AUTH_BOT)
		tmp = tmp + _L("#0000c9bot#000000, ");

	tmp = tmp + _L("#0000ddversion:#000000 ");
	tmp = tmp + ANSI_TO_UTF(user.clientversion);
	tmp = tmp + U(", ");

	tmp = tmp + _L("#0000ddlanguage:#000000 ");
	tmp = tmp + ANSI_TO_UTF(user.language);
	tmp = tmp + U(", ");

	if (num_vehicles == 0)
		tmp = tmp + _L("no vehicles");
	else
		tmp = tmp + TOUTFSTRING(num_vehicles) + _L(" vehicles");

	tmp = tmp + U( "#000000)");

	return tmp;
}

void GUI_MainMenu::addUserToMenu(user_info_t &user)
{
	int numTrucks = BeamFactory::getSingleton().getTruckCount();
	Beam **trucks = BeamFactory::getSingleton().getTrucks();

	// now search the vehicles of that user together
	std::vector<int> matches;
	for (int j = 0; j < numTrucks; j++)
	{
		if (!trucks[j]) continue;

		if (trucks[j]->getSourceID() == user.uniqueid)
		{
			// match, found truck :)
			matches.push_back(j);
		}
	}

	// now add this user to the list
	{
		MyGUI::UString userStr = "- " + convertToMyGUIString(getUserString(user, (int)matches.size()));
		// finally add the user line
		vehiclesMenu->addItem(userStr, MyGUI::MenuItemType::Normal, "USER_"+TOSTRING(user.uniqueid));

		// and add the vehicles below the user name
		if (!matches.empty())
		{
			for (unsigned int j = 0; j < matches.size(); j++)
			{
				char tmp[512] = "";
				sprintf(tmp, "  + %s (%s)", trucks[matches[j]]->realtruckname.c_str(),  trucks[matches[j]]->realtruckfilename.c_str());
				MyGUI::UString vehName = convertToMyGUIString(ANSI_TO_UTF(tmp));
				vehiclesMenu->addItem(vehName, MyGUI::MenuItemType::Normal, "TRUCK_"+TOSTRING(matches[j]));
			}
		}
	}
}

void GUI_MainMenu::vehiclesListUpdate()
{
	vehiclesMenu->removeAllItems();
	
	if (!gEnv->network)
	{
		// single player mode: add vehicles simply, no users
		int numTrucks = BeamFactory::getSingleton().getTruckCount();
		Beam **trucks = BeamFactory::getSingleton().getTrucks();

		// simple iterate through :)
		for (int i = 0; i < numTrucks; i++)
		{
			if (!trucks[i]) continue;

			if (trucks[i]->hideInChooser) continue;

			char tmp[255] = {};
			sprintf(tmp, "[%d] %s", i, trucks[i]->realtruckname.c_str());

			vehiclesMenu->addItem(String(tmp), MyGUI::MenuItemType::Normal, "TRUCK_"+TOSTRING(i));
		}
	} else
	{
		// sort the list according to the network users

		// add self first
		user_info_t *local_user = gEnv->network->getLocalUserData();
		addUserToMenu(*local_user);

		// get network clients
		client_t c[MAX_PEERS];
		gEnv->network->getClientInfos(c);
		// iterate over them
		for (int i = 0; i < MAX_PEERS; i++)
		{
			if (!c[i].used) continue;
			addUserToMenu(c[i].user);
		}
	}
}

void GUI_MainMenu::onVehicleMenu(MyGUI::MenuControl* _sender, MyGUI::MenuItem* _item)
{
	// not working :(
	//vehiclesListUpdate();
}

void GUI_MainMenu::onMenuBtn(MyGUI::MenuCtrlPtr _sender, MyGUI::MenuItemPtr _item)
{
	UTFString miname = UTFString(_item->getCaption().asWStr());
	String id     = _item->getItemId();

	if (id.substr(0,6) == "TRUCK_")
	{
		int truck = PARSEINT(id.substr(6));
		if (truck >= 0 && truck < BeamFactory::getSingleton().getTruckCount())
		{
			BeamFactory::getSingleton().setCurrentTruck(-1);
			BeamFactory::getSingleton().setCurrentTruck(truck);
		}
	}

	if (id.substr(0,5) == "USER_")
	{
		int user_uid = PARSEINT(id.substr(5));

		// cannot whisper with self...
		if (user_uid == gEnv->network->getUID()) return;

		Console::getSingleton().startPrivateChat(user_uid);
	}
	
	if (!gEnv->frameListener) return;

	if (miname == _L("get new Vehicle") && gEnv->player)
	{
		if (gEnv->frameListener->loading_state == NONE_LOADED) return;
		// get out first
		if (BeamFactory::getSingleton().getCurrentTruckNumber() != -1) BeamFactory::getSingleton().setCurrentTruck(-1);
		gEnv->frameListener->reload_pos = gEnv->player->getPosition() + Vector3(0.0f, 1.0f, 0.0f); // 1 meter above the character
		gEnv->frameListener->freeTruckPosition = true;
		gEnv->frameListener->loading_state = RELOADING;
		SelectorWindow::getSingleton().show(SelectorWindow::LT_AllBeam);

	} else if (miname == _L("reload current Vehicle") && gEnv->player)
	{
		if (BeamFactory::getSingleton().getCurrentTruckNumber() != -1)
		{
			gEnv->frameListener->reloadCurrentTruck();
			GUIManager::getSingleton().unfocus();
		}
	} else if (miname == _L("Save Scenery") || miname == _L("Load Scenery"))
	{
		if (gEnv->frameListener->loading_state != ALL_LOADED)
		{
			LOG("you need to open a map before trying to save or load its scenery.");
			return;
		}
		//String fname = SSETTING("Cache Path", "") + gEnv->frameListener->loadedTerrain + ".rorscene";

		//Savegame s;
		if (miname == _L("Save Scenery"))
		{
			//s.save(fname);
		} else
		{
			//s.load(fname);
		}

	} else if (miname == _L("Terrain Editor Mode"))
	{
		gEnv->frameListener->loading_state = RoRFrameListener::TERRAIN_EDITOR;
		gEnv->cameraManager->switchBehavior(CameraManager::CAMERA_BEHAVIOR_ISOMETRIC, true);
		

	} else if (miname == _L("remove current Vehicle"))
	{
		BeamFactory::getSingleton().removeCurrentTruck();

	} else if (miname == _L("activate all Vehicles"))
	{
		BeamFactory::getSingleton().activateAllTrucks();

	} else if (miname == _L("activated Vehicles never sleep")) 
	{
		BeamFactory::getSingleton().setTrucksForcedActive(true);
		_item->setCaption(_L("activated Vehicles can sleep"));

	} else if (miname == _L("activated Vehicles can sleep")) 
	{
		BeamFactory::getSingleton().setTrucksForcedActive(false);
		_item->setCaption(_L("activated Vehicles never sleep"));

	} else if (miname == _L("send all Vehicles to sleep"))
	{
		// get out first
		if (BeamFactory::getSingleton().getCurrentTruckNumber() != -1)
			BeamFactory::getSingleton().setCurrentTruck(-1);
		BeamFactory::getSingleton().sendAllTrucksSleeping();

	} else if (miname == _L("Friction Settings"))
	{
		GUI_Friction::getSingleton().setVisible(true);

	} else if (miname == _L("Exit"))
	{
		gEnv->frameListener->shutdown_final();
	} else if (miname == _L("Show Console"))
	{
		Console *c = Console::getSingletonPtrNoCreation();
		if (c) c->setVisible(!c->getVisible());
	}
	// the debug menu
	else if (miname == _L("no visual debug"))
	{
		Beam *b = BeamFactory::getSingleton().getCurrentTruck();
		if (b) b->setDebugOverlayState(0);
	} else if (miname == _L("show Node numbers"))
	{
		Beam *b = BeamFactory::getSingleton().getCurrentTruck();
		if (b) b->setDebugOverlayState(1);
	} else if (miname == _L("show Beam numbers"))
	{
		Beam *b = BeamFactory::getSingleton().getCurrentTruck();
		if (b) b->setDebugOverlayState(2);
	} else if (miname == _L("show Node&Beam numbers"))
	{
		Beam *b = BeamFactory::getSingleton().getCurrentTruck();
		if (b) b->setDebugOverlayState(3);
	} else if (miname == _L("show Node mass"))
	{
		Beam *b = BeamFactory::getSingleton().getCurrentTruck();
		if (b) b->setDebugOverlayState(4);
	} else if (miname == _L("show Node locked"))
	{
		Beam *b = BeamFactory::getSingleton().getCurrentTruck();
		if (b) b->setDebugOverlayState(5);
	} else if (miname == _L("show Beam compression"))
	{
		Beam *b = BeamFactory::getSingleton().getCurrentTruck();
		if (b) b->setDebugOverlayState(6);
	} else if (miname == _L("show Beam broken"))
	{
		Beam *b = BeamFactory::getSingleton().getCurrentTruck();
		if (b) b->setDebugOverlayState(7);
	} else if (miname == _L("show Beam stress"))
	{
		Beam *b = BeamFactory::getSingleton().getCurrentTruck();
		if (b) b->setDebugOverlayState(8);
	} else if (miname == _L("show Beam strength"))
	{
		Beam *b = BeamFactory::getSingleton().getCurrentTruck();
		if (b) b->setDebugOverlayState(9);
	} else if (miname == _L("show Beam hydros"))
	{
		Beam *b = BeamFactory::getSingleton().getCurrentTruck();
		if (b) b->setDebugOverlayState(10);
	} else if (miname == _L("show Beam commands"))
	{
		Beam *b = BeamFactory::getSingleton().getCurrentTruck();
		if (b) b->setDebugOverlayState(11);
	} else if (miname == _L("Texture Tool"))
	{
		TextureToolWindow::getSingleton().show();
	}

	//LOG(" menu button pressed: " + _item->getCaption());
}

void GUI_MainMenu::setVisible(bool value)
{
	mainmenu->setVisible(value);
	if (!value) GUIManager::getSingleton().unfocus();
	//MyGUI::PointerManager::getInstance().setVisible(value);
}

bool GUI_MainMenu::getVisible()
{
	return mainmenu->getVisible();
}

void GUI_MainMenu::updatePositionUponMousePosition(int x, int y)
{
	int h = mainmenu->getHeight();
	bool focused = false;
	for (unsigned int i=0;i<pop.size(); i++)
		focused |= pop[i]->getVisible();

	if (focused)
	{
		mainmenu->setPosition(0, 0);
	} else
	{
		if (y > 2*h)
			mainmenu->setPosition(0, -h);

		else
			mainmenu->setPosition(0, std::min(0, -y+10));
	}

	// this is hacky, but needed as the click callback is not working
	if (vehicleListNeedsUpdate)
	{
		vehiclesListUpdate();
		MUTEX_LOCK(&updateLock);
		vehicleListNeedsUpdate = false;
		MUTEX_UNLOCK(&updateLock);
	}
}

void GUI_MainMenu::triggerUpdateVehicleList()
{
	MUTEX_LOCK(&updateLock);
	vehicleListNeedsUpdate = true;
	MUTEX_UNLOCK(&updateLock);
}

#endif // USE_MYGUI
