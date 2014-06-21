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
	@file   RigEditor_Main.cpp
	@date   06/2014
	@author Petr Ohlidal
*/

#include "RigEditor_Main.h"

#include "Application.h"
#include "GlobalEnvironment.h"
#include "InputEngine.h"
#include "MainThread.h"
#include "OgreSubsystem.h"
#include "RigEditor_InputHandler.h"
#include "Settings.h"

#include <OgreRoot.h>
#include <OgreRenderWindow.h>
#include <OgreMaterialManager.h>
#include <OgreMaterial.h>

using namespace RoR;
using namespace RoR::RigEditor;

Main::Main():
	m_scene_manager(nullptr),
	m_camera(nullptr),
	m_viewport(nullptr),
	m_rig_entity(nullptr),
	m_exit_loop_requested(false),
	m_input_handler(nullptr)
{
	/* Load config */
	m_config_file.load(SSETTING("Config Root", "") + "rig_editor.cfg");

	/* Parse config */
	m_config.viewport_background_color = m_config_file.GetColourValue("viewport_background_color_rgb");
	m_config.beam_generic_color        = m_config_file.GetColourValue("beam_generic_color_rgb");

	/* Setup 3D engine */
	OgreSubsystem* ror_ogre_subsystem = RoR::Application::GetOgreSubsystem();
	assert(ror_ogre_subsystem != nullptr);
	m_scene_manager = ror_ogre_subsystem->GetOgreRoot()->createSceneManager(Ogre::ST_GENERIC);
	m_camera = m_scene_manager->createCamera("rig_editor_camera");	

	/* Setup input */
	m_input_handler = new InputHandler();
}

void Main::EnterMainLoop()
{
	/* Setup 3D engine */
	OgreSubsystem* ror_ogre_subsystem = RoR::Application::GetOgreSubsystem();
	assert(ror_ogre_subsystem != nullptr);
	m_viewport = ror_ogre_subsystem->GetRenderWindow()->addViewport(m_camera);
	m_viewport->setBackgroundColour(m_config.viewport_background_color);
	m_camera->setAspectRatio(m_viewport->getActualHeight() / m_viewport->getActualWidth());

	/* Setup input */
	RoR::Application::GetInputEngine()->SetKeyboardListener(m_input_handler);
	RoR::Application::GetInputEngine()->SetMouseListener(m_input_handler);

	while (! m_exit_loop_requested)
	{
		UpdateMainLoop();

		Ogre::RenderWindow* rw = RoR::Application::GetOgreSubsystem()->GetRenderWindow();
		if (rw->isClosed())
		{
			RoR::Application::GetMainThreadLogic()->RequestShutdown();
			break;
		}

		/* Render */
		RoR::Application::GetOgreSubsystem()->GetOgreRoot()->renderOneFrame();

		if (!rw->isActive() && rw->isVisible())
		{
			rw->update(); // update even when in background !
		}
	}

	m_exit_loop_requested = false;
}

void Main::UpdateMainLoop()
{
	/* Process window events */
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32 || OGRE_PLATFORM == OGRE_PLATFORM_LINUX
	RoRWindowEventUtilities::messagePump();
#endif

	/* Update input devices */
	RoR::Application::GetInputEngine()->Capture();

	if (m_input_handler->WasEventFired(InputHandler::Event::QUIT_RIG_EDITOR))
	{
		RoR::Application::GetMainThreadLogic()->SetNextApplicationState(Application::STATE_SIMULATION);
		m_exit_loop_requested = true;
		return;
	}
}
