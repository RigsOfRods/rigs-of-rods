/*
	This source file is part of Rigs of Rods
	Copyright 2005-2012 Pierre-Michel Ricordel
	Copyright 2007-2012 Thomas Fischer
	Copyright 2013-2014 Petr Ohlidal

	For more information, see http://www.rigsofrods.com/

	Rigs of Rods is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License version 3, as
	published by the Free Software Foundation.

	Rigs of Rods is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Rigs of Rods. If not, see <http://www.gnu.org/licenses/>.
*/

/** 
	@file   Application.cpp
	@author Petr Ohlidal
	@date   05/2014
*/

#include "Application.h"

#include "CacheSystem.h"
#include "Console.h"
#include "ContentManager.h"
#include "GUIManager.h"
#include "InputEngine.h"
#include "OgreSubsystem.h"
#include "OverlayWrapper.h"
#include "SceneMouse.h"

#include <OgreException.h>

namespace RoR
{

// ================================================================================
// Static variables
// ================================================================================

OgreSubsystem*   Application::ms_ogre_subsystem    = nullptr;
ContentManager*  Application::ms_content_manager   = nullptr;
OverlayWrapper*  Application::ms_overlay_wrapper   = nullptr;
SceneMouse*      Application::ms_scene_mouse       = nullptr;
GUIManager*      Application::ms_gui_manager       = nullptr;
Console*         Application::ms_console           = nullptr;
InputEngine*     Application::ms_input_engine      = nullptr;
CacheSystem*     Application::ms_cache_system      = nullptr;
MainThread*      Application::ms_main_thread_logic = nullptr;

// ================================================================================
// Functions
// ================================================================================

void Application::StartOgreSubsystem()
{
	ms_ogre_subsystem = new OgreSubsystem();
	if (ms_ogre_subsystem == nullptr)
	{
		throw std::runtime_error("[RoR] Failed to create OgreSubsystem");
	}

	if (! ms_ogre_subsystem->StartOgre("RoR", "", ""))
	{
		throw std::runtime_error("[RoR] Failed to start up OGRE 3D engine");
	}
}

void Application::ShutdownOgreSubsystem()
{
	assert(ms_ogre_subsystem != nullptr && "Application::ShutdownOgreSubsystem(): Ogre subsystem was not started");
	delete ms_ogre_subsystem;
	ms_ogre_subsystem = nullptr;
}

void Application::CreateContentManager()
{
	ms_content_manager = new ContentManager();
}

void Application::DestroyContentManager()
{
	assert(ms_content_manager != nullptr && "Application::DestroyContentManager(): ContentManager never created");
	delete ms_content_manager;
	ms_content_manager = nullptr;
}

void Application::CreateOverlayWrapper()
{
	ms_overlay_wrapper = new OverlayWrapper();
	if (ms_overlay_wrapper == nullptr)
	{
		throw std::runtime_error("[RoR] Failed to create OverlayWrapper");
	}
}

void Application::DestroyOverlayWrapper()
{
	assert(ms_overlay_wrapper != nullptr && "Application::DestroyOverlayWrapper(): OverlayWrapper never created");
	delete ms_overlay_wrapper;
	ms_overlay_wrapper = nullptr;
}

void Application::CreateSceneMouse()
{
	assert (ms_scene_mouse == nullptr);
	ms_scene_mouse = new SceneMouse();
}

void Application::DeleteSceneMouse()
{
	assert (ms_scene_mouse != nullptr);
	delete ms_scene_mouse;
	ms_scene_mouse = nullptr;
}

void Application::CreateGuiManagerIfNotExists()
{
	if (ms_gui_manager == nullptr)
	{
		ms_gui_manager = new GUIManager();
	}
}

void Application::DeleteGuiManagerIfExists()
{
	if (ms_gui_manager != nullptr)
	{
		delete ms_gui_manager;
		ms_gui_manager = nullptr;
	}
}

void Application::CreateConsoleIfNotExists()
{
	if (ms_console == nullptr)
	{
		ms_console = new Console();
	}
}

void Application::DeleteConsoleIfExists()
{
	if (ms_console != nullptr)
	{
		delete ms_console;
		ms_console = nullptr;
	}
}

void Application::CreateInputEngine()
{
	assert(ms_input_engine == nullptr);
	ms_input_engine = new InputEngine();
}

void Application::CreateCacheSystem()
{
	assert(ms_cache_system == nullptr);
	ms_cache_system = new CacheSystem();
}

GuiManagerInterface* Application::GetGuiManagerInterface()
{
	return static_cast<GuiManagerInterface*>(ms_gui_manager);
}

} // namespace RoR
