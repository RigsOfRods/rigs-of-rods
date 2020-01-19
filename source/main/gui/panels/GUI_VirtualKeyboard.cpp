/*
    This source file is part of Rigs of Rods
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

#pragma once

#include "GUI_VirtualKeyboard.h"

/* ------- CKEYS --------
Deps: 
    Bundled: NFD (Native File Dialog)
    Bundled: imgui-SFML (https://github.com/eliasdaler/imgui-sfml/blob/master/imgui-SFML.h)
    Bundled: TinyXML2
    Bundled: Jsmn (https://github.com/zserge/jsmn)

  ~~ AppMain.cpp: `AppMain::Run()`
  		//  Draw
		//------------------
		app->Gui(); ~~ Main menu, settings, help

		window->resetGLStates();

		app->Graph(); ~~ draw keyboard

		ImGui::Render();

		window->display();


*/

void RoR::GUI::VirtualKeyboard::Draw()
{
   
}


