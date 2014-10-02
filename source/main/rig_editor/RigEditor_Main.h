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
#include "RigEditor_ForwardDeclarations.h"
#include "RoRPrerequisites.h"

namespace RoR
{

namespace RigEditor
{

class Main: public IMain
{
public:

	struct OpenSaveFileDialogMode
	{
		static const MyGUI::UString MODE_OPEN_TRUCK;
		static const MyGUI::UString MODE_SAVE_TRUCK_AS;
	};

	Main(Config* config);
	~Main();

	void EnterMainLoop();

	void UpdateMainLoop();

	Ogre::SceneManager* GetOgreSceneManager()
	{
		return m_scene_manager;
	}

	Config* GetConfig()
	{
		return m_config;
	}

	Rig* GetRig()
	{
		return m_rig;
	}

	/* IMain implementations */

	virtual void CommandShowDialogOpenRigFile();

	virtual void CommandShowDialogSaveRigFileAs();

	virtual void CommandSaveRigFile();

	virtual void CommandCloseCurrentRig();

	virtual void CommandCurrentRigDeleteSelectedNodes();

	virtual void CommandCurrentRigDeleteSelectedBeams();

	virtual void CommandQuitRigEditor();

	virtual void CommandShowFilePropertiesWindow();

	/* GUI callbacks */

	void NotifyFileSelectorEnded(RoR::GUI::Dialog* dialog, bool result);

private:

	bool LoadRigDefFile(MyGUI::UString const & directory, MyGUI::UString const & filename);

	Config*              m_config;
	Ogre::SceneManager*  m_scene_manager;
	Ogre::Viewport*      m_viewport;
	Ogre::Camera*        m_camera;
	Ogre::Entity*        m_rig_entity;
	InputHandler*        m_input_handler;
	CameraHandler*       m_camera_handler;
	Rig*                 m_rig;

	bool                 m_exit_loop_requested;

	// GUI
	MyGUI::TextBox*                             m_debug_box;
	std::unique_ptr<GUI::RigEditorMenubar>      m_gui_menubar;
	std::unique_ptr<GUI::OpenSaveFileDialog>    m_gui_open_save_file_dialog;
	std::unique_ptr<GUI::RigEditorDeleteMenu>   m_gui_delete_menu;
	std::unique_ptr<GUI::RigEditorFilePropertiesWindow>   m_gui_file_properties_window;

};

} // namespace RigEditor

} // namespace RoR
