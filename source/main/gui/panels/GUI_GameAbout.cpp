/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2013-2019 Petr Ohlidal

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

void RoR::GUI::GameAbout::Draw()
{
    RoR::GUIManager::GuiTheme const& theme = RoR::App::GetGuiManager()->GetTheme();

    ImGui::SetNextWindowSize(ImVec2(475.f, ImGui::GetIO().DisplaySize.y - 40.f), ImGuiCond_Appearing);
    ImGui::SetNextWindowPosCenter(ImGuiCond_Appearing);
    ImGuiWindowFlags win_flags = ImGuiWindowFlags_NoCollapse;
    bool keep_open = true;
    ImGui::Begin(_LC("About", "About Rigs of Rods"), &keep_open, win_flags);

    ImGui::TextDisabled("%s: ", _LC("About", "Version"));
    ImGui::SameLine();
    ImGui::Text("%s", ROR_VERSION_STRING);

    ImGui::TextDisabled("%s: ", _LC("About", "Network protocol"));
    ImGui::SameLine();
    ImGui::Text("%s", RORNET_VERSION);

    ImGui::TextDisabled("%s: ", _LC("About", "Build time"));
    ImGui::SameLine();
    ImGui::Text("%s, %s", ROR_BUILD_DATE, ROR_BUILD_TIME);

    ImGui::Separator();

    ImGui::TextColored(theme.value_blue_text_color, "%s:", _LC("About", "Authors"));
    ImGui::Text("%s%s", "Pierre-Michel Ricordel (pricorde):",  " Physics Genius, Original Author, Core Developer, retired");
    ImGui::Text("%s%s", "Thomas Fischer (tdev):",              " Core Developer, inactive");

    ImGui::NewLine();
    ImGui::TextColored(theme.value_blue_text_color, "%s:", _LC("About", "Current Developers"));
    ImGui::Text("%s%s", "Petr Ohlidal (only_a_ptr):",  " Core Developer, active");
    ImGui::Text("%s%s", "Edgar (AnotherFoxGuy):",      " Various fixes and features, developer web services, active");

    ImGui::NewLine();
    ImGui::TextColored(theme.value_blue_text_color, "%s:", _LC("About", "Server Contributors"));
    ImGui::Text("%s%s", "Austin:",       " Server funding");
    ImGui::Text("%s%s", "DarthCain:",    " Forum software funding");
    ImGui::Text("%s%s", "Zentro:",       " Systems administrator, web designer");
    ImGui::Text("%s%s", "Charger:",      " Branding designer");
    ImGui::Text("%s%s", "CuriousMike:",  " Repository & multiplayer server management");

    ImGui::NewLine();
    ImGui::TextColored(theme.value_blue_text_color, "%s:", _LC("About", "Code Contributors"));
    ImGui::Text("%s%s", "Estama:",       " Physics Core Optimizations, Collision/Friction code, Support Beams");
    ImGui::Text("%s%s", "Lifter:",       " Triggers, Animators, Animated Props, Shocks2");
    ImGui::Text("%s%s", "Aperion:",      " Slidenodes, Axles, Improved Engine code, Rigidifiers, Networking code");
    ImGui::Text("%s%s", "FlyPiper:",     " Inertia Code, minor patches");
    ImGui::Text("%s%s", "knied:",        " MacOSX Patches");
    ImGui::Text("%s%s", "altren:",       " Coded some MyGUI windows");
    ImGui::Text("%s%s", "petern:",       " Repair on spot, Linux patches");
    ImGui::Text("%s%s", "imrenagy:",     " Moving chair hardware support, several fixes");
    ImGui::Text("%s%s", "priotr:",       " Several Linux fixes");
    ImGui::Text("%s%s", "neorej16:",     " AngelScript improvements");
    ImGui::Text("%s%s", "cptf:",         " Several Linux gcc fixes");
    ImGui::Text("%s%s", "88Toyota:",     " Clutch force patches");
    ImGui::Text("%s%s", "synthead:",     " Minor Linux fixes");
    ImGui::Text("%s%s", "theshark:",                    " Various fixes");
    ImGui::Text("%s%s", "Clockwork (a.k.a VeyronEB):",  " GUI Designer and tweaker");
    ImGui::Text("%s%s", "Klink:",                       " Terrains conversion, few fixes and tweaks, dashboard designer");
    ImGui::Text("%s%s", "hagdervriese:",                " Linux fixes");
    ImGui::Text("%s%s", "skybon:",                      " Web services, fixes, utilities");
    ImGui::Text("%s%s", "Niklas Kersten (Hiradur):",    " Various fixes and tweaks, retired");
    ImGui::Text("%s%s", "Moncef Ben Slimane (max98):",  " Few fixes, Few improvements, GUI Overhaul");
    ImGui::Text("%s%s", "mikadou:",                     " Modernized thread pool, cmake, various fixes");
    ImGui::Text("%s%s", "ulteq:",                       " Various features, multithreading, lots of fixes");
    ImGui::Text("%s%s", "tritonas00:",                  " Various improvements and Linux fixes");

    ImGui::NewLine();
    ImGui::TextColored(theme.value_blue_text_color, "%s:", _LC("About", "Core Content Contributors"));
    ImGui::Text("%s%s", "donoteat:",       " Improved spawner models, terrain work");
    ImGui::Text("%s%s", "kevinmce:",       " Old character");
    ImGui::Text("%s%s", "vido89",          " Character animations");

    ImGui::NewLine();
    ImGui::TextColored(theme.value_blue_text_color, "%s:", _LC("About", "Mod Contributors"));
    ImGui::Text("%s%s", "The Rigs of Rods community:",  " Provides us with lots of mods to play with");

    ImGui::NewLine();
    ImGui::TextColored(theme.value_blue_text_color, "%s:", _LC("About", "Testers"));
    ImGui::Text("%s%s", "Invited core team:",  " The invited members helped us a lot along the way at various corners");

    ImGui::NewLine();
    ImGui::TextColored(theme.value_blue_text_color, "%s:", _LC("About", "Used Libs"));
    ImGui::Text("%s%s", "Ogre3D:",         " 3D rendering engine");
#ifdef USE_CAELUM
    ImGui::Text("%s%s", "Caelum:",         " Atmospheric effects");
#endif
    ImGui::Text("%s%s", "Hydrax:",         " Water rendering");
#ifdef USE_ANGELSCRIPT
    ImGui::Text("%s%s", "AngelScript:",    " Scripting Backend");
#endif
#ifdef USE_OPENAL
    ImGui::Text("%s%s", "OpenAL Soft:",    " Sound engine");
#endif
    ImGui::Text("%s%s", "MyGUI:",          " Legacy GUI System");
    ImGui::Text("%s%s", "Dear ImGui:",     " GUI System");
#ifdef USE_MOFILEREADER
    ImGui::Text("%s%s", "mofilereader:",   " Used for Internationalization");
#endif
    ImGui::Text("%s%s", "OIS:",            " Used as Input System");

    ImGui::Text("%s%s", "pagedgeometry:",  " Used for foliage (grass, trees, etc)");

#ifdef USE_CURL
    ImGui::Text("%s%s", "curl:",           " Used for www-server communication");
#endif
#ifdef USE_SOCKETW
    ImGui::Text("%s%s", "SocketW:",        " Used as cross-platform socket abstraction");
#endif
    ImGui::Text("%s%s", "pThreads:",       " POSIX threads library");
    ImGui::Text("%s%s", "RapidJSON:",      " JSON parser/generator, used for online services");

    App::GetGuiManager()->RequestGuiCaptureKeyboard(ImGui::IsWindowHovered());
    ImGui::End();
    if (!keep_open)
    {
        this->SetVisible(false);
    }
}

void RoR::GUI::GameAbout::SetVisible(bool v)
{
    m_is_visible = v;
    if(!v && (App::app_state->GetActiveEnum<AppState>() == RoR::AppState::MAIN_MENU))
    {
        App::GetGuiManager()->SetVisible_GameMainMenu(true);
    }
}

