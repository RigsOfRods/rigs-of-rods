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
	@file   Application.h
	@author Petr Ohlidal
	@date   05/2014
	@brief  Central object manager and communications hub.
*/

#pragma once

#include "RoRPrerequisites.h"
#include "Settings.h"

namespace RoR
{

/** Central object manager and communications hub.
*/
class Application
{
	friend class MainThread; // Manages lifecycle of this class.

public:

	enum State
	{
		STATE_NONE,
		STATE_BOOTSTRAP,
		STATE_MAIN_MENU,
		STATE_SIMULATION,
		STATE_RIG_EDITOR
	};

	static OgreSubsystem* GetOgreSubsystem()
	{
		assert(ms_ogre_subsystem != nullptr && "OgreSubsystem not created");
		return ms_ogre_subsystem;
	};

	static Settings& GetSettings()
	{
		return Settings::getSingleton(); // Temporary solution
	};

	static ContentManager* GetContentManager()
	{
		return ms_content_manager;
	}

	static OverlayWrapper* GetOverlayWrapper()
	{
		return ms_overlay_wrapper;
	}

	static SceneMouse* GetSceneMouse()
	{
		return ms_scene_mouse;
	}

	static GUIManager* GetGuiManager()
	{
		return ms_gui_manager;
	}

	static GuiManagerInterface* GetGuiManagerInterface();

	static Console* GetConsole()
	{
		return ms_console;
	}

	static InputEngine* GetInputEngine()
	{
		return ms_input_engine;
	}

	static CacheSystem* GetCacheSystem()
	{
		return ms_cache_system;
	}

	static MainThread* GetMainThreadLogic()
	{
		return ms_main_thread_logic;
	}

private:

	static void StartOgreSubsystem();

	static void ShutdownOgreSubsystem();

	static void CreateContentManager();

	static void DestroyContentManager();

	static void CreateOverlayWrapper();

	static void DestroyOverlayWrapper();

	static void CreateSceneMouse();

	static void DeleteSceneMouse();

	/** Creates instance if it doesn't already exist.
	*/
	static void CreateGuiManagerIfNotExists();

	/** Destroys instance if it exists.
	*/
	static void DeleteGuiManagerIfExists();

	/** Creates instance if it doesn't already exist.
	*/
	static void CreateConsoleIfNotExists();

	/** Destroys instance if it exists.
	*/
	static void DeleteConsoleIfExists();

	static void CreateInputEngine();

	static void CreateCacheSystem();

	static void SetMainThreadLogic(MainThread* main_thread_logic)
	{
		ms_main_thread_logic = main_thread_logic;
	}

	/* Properties */

	static OgreSubsystem*   ms_ogre_subsystem;
	static ContentManager*  ms_content_manager;
	static OverlayWrapper*  ms_overlay_wrapper;
	static SceneMouse*      ms_scene_mouse;
	static GUIManager*      ms_gui_manager;
	static Console*         ms_console;
	static InputEngine*     ms_input_engine;
	static CacheSystem*     ms_cache_system;
	static MainThread*      ms_main_thread_logic;
};

} // namespace RoR
