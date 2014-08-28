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
	@file   RigEditor_Main.h
	@date   06/2014
	@author Petr Ohlidal
*/

#pragma once

#include "ConfigFile.h"
#include "GUI_OpenSaveFileDialog.h"
#include "RigDef_Prerequisites.h"
#include "RigEditor_IMain.h"
#include "RoRPrerequisites.h"

namespace RoR
{

namespace RigEditor
{

class Main: public IMain
{
public:

	struct Config
	{
		Ogre::ColourValue viewport_background_color;
		Ogre::ColourValue scene_ambient_light_color;

		/* Beam coloring */
		Ogre::ColourValue beam_generic_color;
		Ogre::ColourValue beam_invisible_color;
		Ogre::ColourValue beam_support_color;
		Ogre::ColourValue beam_rope_color;

		/* Node display */
		Ogre::ColourValue node_generic_color;
		float             node_generic_point_size;
	};

	struct OpenSaveFileDialogMode
	{
		static const MyGUI::UString MODE_OPEN_TRUCK;
		static const MyGUI::UString MODE_SAVE_TRUCK_AS;
	};

	Main();
	~Main();

	void EnterMainLoop();

	void UpdateMainLoop();

	Ogre::SceneManager* GetOgreSceneManager()
	{
		return m_scene_manager;
	}

	Config & GetConfig()
	{
		return m_config;
	}

	/* IMain implementations */

	virtual void CommandShowDialogOpenRigFile();

	virtual void CommandShowDialogSaveRigFileAs();

	virtual void CommandSaveRigFile();

	/* GUI callbacks */

	void NotifyFileSelectorEnded(GUI::Dialog* dialog, bool result);

private:

	bool LoadRigDefFile(MyGUI::UString const & directory, MyGUI::UString const & filename);

	RoR::ConfigFile      m_config_file;
	Config               m_config;
	Ogre::SceneManager*  m_scene_manager;
	Ogre::Viewport*      m_viewport;
	Ogre::Camera*        m_camera;
	Ogre::Entity*        m_rig_entity;
	InputHandler*        m_input_handler;
	CameraHandler*       m_camera_handler;
	Rig*                 m_rig;

	bool                 m_exit_loop_requested;

	// GUI
	MyGUI::TextBox*          m_debug_box;
	GUI::RigEditorMenubar*   m_gui_menubar;
	GUI::OpenSaveFileDialog* m_gui_open_save_file_dialog;

};

} // namespace RigEditor

} // namespace RoR
