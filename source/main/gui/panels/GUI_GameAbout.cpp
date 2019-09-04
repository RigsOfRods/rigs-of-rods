/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2013-2014 Petr Ohlidal

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

/// @file   GUI_GameAbout.cpp
/// @author Moncef Ben Slimane
/// @date   11/2014

#include "GUI_GameAbout.h"

#include "RoRPrerequisites.h"

#include "Application.h"
#include "GUIManager.h"
#include "Language.h"
#include "RoRVersion.h"
#include "Utils.h"
#include "RoRnet.h"

#include <MyGUI.h>

using namespace RoR;
using namespace GUI;

#define CLASS        GameAbout
#define MAIN_WIDGET  ((MyGUI::Window*)mMainWidget)

CLASS::CLASS()
{
    MyGUI::WindowPtr win = dynamic_cast<MyGUI::WindowPtr>(mMainWidget);
    win->eventWindowButtonPressed += MyGUI::newDelegate(this, &CLASS::notifyWindowButtonPressed); //The "X" button thing

    m_backbtn->eventMouseButtonClick += MyGUI::newDelegate(this, &CLASS::eventMouseButtonClickBackButton);
    m_ror_version->setCaption(Ogre::String(ROR_VERSION_STRING));
    m_net_version->setCaption(Ogre::String(RORNET_VERSION));
    m_build_time->setCaption(Ogre::String(ROR_BUILD_DATE) + ", " + Ogre::String(ROR_BUILD_TIME));

    initMisc();
    CenterToScreen();

    MAIN_WIDGET->setVisible(false);
}

CLASS::~CLASS()
{
}

void CLASS::Show()
{
    MAIN_WIDGET->setVisibleSmooth(true);
}

void CLASS::Hide()
{
    MAIN_WIDGET->setVisibleSmooth(false);
    if (App::app_state.GetActive() == AppState::MAIN_MENU)
    {
        App::GetGuiManager()->SetVisible_GameMainMenu(true);
    }
}

void CLASS::CenterToScreen()
{
    MyGUI::IntSize windowSize = MAIN_WIDGET->getSize();
    MyGUI::IntSize parentSize = MAIN_WIDGET->getParentSize();

    MAIN_WIDGET->setPosition((parentSize.width - windowSize.width) / 2, (parentSize.height - windowSize.height) / 2);
}

bool CLASS::IsVisible()
{
    return MAIN_WIDGET->isVisible();
}

void CLASS::initMisc()
{
    Ogre::UTFString AuthorsText = "";
    Ogre::UTFString orange = U("#FF7D02"); // colour key shortcut
    Ogre::UTFString white = U("#FFFFFF"); // colour key shortcut
    Ogre::UTFString color1 = U("#66FF33"); // colour key shortcut
    Ogre::UTFString newline = U("\n");

    //Authors:
    AuthorsText = orange + "Authors:" + newline;
    AuthorsText = AuthorsText + color1 + "Pierre-Michel Ricordel (pricorde):" + white + " Physics Genius, Original Author, Core Developer, retired" + newline;
    AuthorsText = AuthorsText + color1 + "Thomas Fischer (tdev):" + white + " Core Developer, inactive" + newline;

    //Current Project devs:
    AuthorsText = AuthorsText + newline;
    AuthorsText = AuthorsText + orange + "Current Developers:" + newline;
    AuthorsText = AuthorsText + color1 + "Petr Ohlidal (only_a_ptr):" + white + " Core Developer, active" + newline;
    AuthorsText = AuthorsText + color1 + "Edgar (AnotherFoxGuy):" + white + " Various fixes and features, developer web services, active" + newline;

    //Server Contributors
    AuthorsText = AuthorsText + newline;
    AuthorsText = AuthorsText + orange + "Server Contributors:" + newline;
    AuthorsText = AuthorsText + color1 + "Austin:" + white + " Server funding" + newline;
    AuthorsText = AuthorsText + color1 + "DarthCain:" + white + " Forum software funding" + newline;
    AuthorsText = AuthorsText + color1 + "Zentro:" + white + " Systems administrator, web designer" + newline;
    AuthorsText = AuthorsText + color1 + "Charger:" + white + " Branding designer" + newline;
    AuthorsText = AuthorsText + color1 + "CuriousMike:" + white + " Repository & multiplayer server management" + newline;

    //Code Contributors:
    AuthorsText = AuthorsText + newline;
    AuthorsText = AuthorsText + orange + "Code Contributors:" + newline;
    AuthorsText = AuthorsText + color1 + "Estama:" + white + " Physics Core Optimizations, Collision/Friction code, Support Beams" + newline;
    AuthorsText = AuthorsText + color1 + "Lifter:" + white + " Triggers, Animators, Animated Props, Shocks2" + newline;
    AuthorsText = AuthorsText + color1 + "Aperion:" + white + " Slidenodes, Axles, Improved Engine code, Rigidifiers, Networking code" + newline;
    AuthorsText = AuthorsText + color1 + "FlyPiper:" + white + " Inertia Code, minor patches" + newline;
    AuthorsText = AuthorsText + color1 + "knied:" + white + " MacOSX Patches" + newline;
    AuthorsText = AuthorsText + color1 + "altren:" + white + " Coded some MyGUI windows" + newline;
    AuthorsText = AuthorsText + color1 + "petern:" + white + " Repair on spot, Linux patches" + newline;
    AuthorsText = AuthorsText + color1 + "imrenagy:" + white + " Moving chair hardware support, several fixes" + newline;
    AuthorsText = AuthorsText + color1 + "priotr:" + white + " Several Linux fixes" + newline;
    AuthorsText = AuthorsText + color1 + "neorej16:" + white + " AngelScript improvements" + newline;
    AuthorsText = AuthorsText + color1 + "cptf:" + white + " Several Linux gcc fixes" + newline;
    AuthorsText = AuthorsText + color1 + "88Toyota:" + white + " Clutch force patches" + newline;
    AuthorsText = AuthorsText + color1 + "synthead:" + white + " Minor Linux fixes" + newline;
    AuthorsText = AuthorsText + color1 + "theshark:" + white + " Various fixes" + newline;
    AuthorsText = AuthorsText + color1 + "Clockwork (a.k.a VeyronEB):" + white + " GUI Designer and tweaker" + newline;
    AuthorsText = AuthorsText + color1 + "Klink:" + white + " Terrains conversion, few fixes and tweaks, dashboard designer" + newline;
    AuthorsText = AuthorsText + color1 + "hagdervriese:" + white + " Linux fixes" + newline;
    AuthorsText = AuthorsText + color1 + "skybon:" + white + " Web services, fixes, utilities" + newline;
    AuthorsText = AuthorsText + color1 + "Niklas Kersten (Hiradur):" + white + " Various fixes and tweaks, retired" + newline;
    AuthorsText = AuthorsText + color1 + "Moncef Ben Slimane (max98):" + white + " Few fixes, Few improvements, GUI Overhaul" + newline;
    AuthorsText = AuthorsText + color1 + "mikadou:" + white + " Modernized thread pool, cmake, various fixes" + newline;
    AuthorsText = AuthorsText + color1 + "ulteq:" + white + " Various features, multithreading, lots of fixes" + newline;
    AuthorsText = AuthorsText + color1 + "tritonas00" + white + " Various improvements and Linux fixes" + newline;

    //Core Content Contributors
    AuthorsText = AuthorsText + newline;
    AuthorsText = AuthorsText + orange + "Core Content Contributors:" + newline;
    AuthorsText = AuthorsText + color1 + "donoteat:" + white + " Improved spawner models, terrain work" + newline;
    AuthorsText = AuthorsText + color1 + "kevinmce:" + white + " Old character" + newline;
    AuthorsText = AuthorsText + color1 + "vido89" + white + " Character animations" + newline;

    //Mod Contributors
    AuthorsText = AuthorsText + newline;
    AuthorsText = AuthorsText + orange + "Mod Contributors:" + newline;
    AuthorsText = AuthorsText + color1 + "The Rigs of Rods community:" + white + " Provides us with lots of mods to play with" + newline;

    //Testers
    AuthorsText = AuthorsText + newline;
    AuthorsText = AuthorsText + orange + "Testers:" + newline;
    AuthorsText = AuthorsText + color1 + "Invited core team:" + white + " The invited members helped us a lot along the way at various corners" + newline;

    //Used Libs
    AuthorsText = AuthorsText + newline;
    AuthorsText = AuthorsText + orange + "Used Libs:" + newline;
    AuthorsText = AuthorsText + color1 + "Ogre3D:" + white + " 3D rendering engine" + newline;
#ifdef USE_CAELUM
    AuthorsText = AuthorsText + color1 + "Caelum:" + white + " Atmospheric effects" + newline;
#endif
    AuthorsText = AuthorsText + color1 + "Hydrax:" + white + " Water rendering" + newline;
#ifdef USE_ANGELSCRIPT
    AuthorsText = AuthorsText + color1 + "AngelScript:" + white + " Scripting Backend" + newline;
#endif
#ifdef USE_OPENAL
    AuthorsText = AuthorsText + color1 + "OpenAL Soft:" + white + " Sound engine" + newline;
#endif
    AuthorsText = AuthorsText + color1 + "MyGUI:" + white + " Legacy GUI System" + newline;
    AuthorsText = AuthorsText + color1 + "Dear ImGui:" + white + " GUI System" + newline;
#ifdef USE_MOFILEREADER
    AuthorsText = AuthorsText + color1 + "mofilereader:" + white + " Used for Internationalization" + newline;
#endif
    AuthorsText = AuthorsText + color1 + "OIS:" + white + " Used as Input System" + newline;

    AuthorsText = AuthorsText + color1 + "pagedgeometry:" + white + " Used for foliage (grass, trees, etc)" + newline;

#ifdef USE_CURL
    AuthorsText = AuthorsText + color1 + "curl:" + white + " Used for www-server communication" + newline;
#endif
#ifdef USE_SOCKETW
    AuthorsText = AuthorsText + color1 + "SocketW:" + white + " Used as cross-platform socket abstraction" + newline;
#endif
	AuthorsText = AuthorsText + color1 + "pThreads:" + white + " POSIX threads library" + newline;
	AuthorsText = AuthorsText + color1 + "RapidJSON:" + white + " JSON parser/generator, used for online services" + newline;

    m_authors->setMaxTextLength(4096);
    m_authors->setCaption(Ogre::String(AuthorsText));
    m_authors->setVScrollPosition(0);
}

void CLASS::eventMouseButtonClickBackButton(MyGUI::WidgetPtr _sender)
{
    Hide();
}

void CLASS::notifyWindowButtonPressed(MyGUI::WidgetPtr _sender, const std::string& _name)
{
    if (_name == "close")
        Hide();
}

void CLASS::SetVisible(bool v)
{
    if (v)
        Show();
    else
        Hide();
}
