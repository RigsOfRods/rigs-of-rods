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
#include "CacheSystem.h"
#include "GlobalEnvironment.h"
#include "GUI_RigEditorMenubar.h"
#include "GUIManager.h"
#include "InputEngine.h"
#include "MainThread.h"
#include "OgreSubsystem.h"
#include "RigDef_Parser.h"
#include "RigDef_Validator.h"
#include "RigEditor_CameraHandler.h"
#include "RigEditor_Config.h"
#include "RigEditor_InputHandler.h"
#include "RigEditor_Node.h"
#include "RigEditor_RigFactory.h"
#include "RigEditor_Rig.h"
#include "Settings.h"

#include <OgreEntity.h>
#include <OgreMaterialManager.h>
#include <OgreMaterial.h>
#include <OgreMovableObject.h>
#include <OgreRoot.h>
#include <OgreRenderWindow.h>
#include <sstream>

using namespace RoR;
using namespace RoR::RigEditor;

const MyGUI::UString Main::OpenSaveFileDialogMode::MODE_OPEN_TRUCK     ("RigEditor_OpenTruckFile");
const MyGUI::UString Main::OpenSaveFileDialogMode::MODE_SAVE_TRUCK_AS  ("RigEditor_SaveTruckFileAs");

Main::Main(Config* config):
	m_config(config),
	m_scene_manager(nullptr),
	m_camera(nullptr),
	m_viewport(nullptr),
	m_rig_entity(nullptr),
	m_exit_loop_requested(false),
	m_input_handler(nullptr),
	m_debug_box(nullptr),
	m_gui_menubar(nullptr),
	m_rig(nullptr),
	m_gui_open_save_file_dialog(nullptr)
{
	/* Setup 3D engine */
	OgreSubsystem* ror_ogre_subsystem = RoR::Application::GetOgreSubsystem();
	assert(ror_ogre_subsystem != nullptr);
	m_scene_manager = ror_ogre_subsystem->GetOgreRoot()->createSceneManager(Ogre::ST_GENERIC, "rig_editor_scene_manager");
	m_scene_manager->setAmbientLight(m_config->scene_ambient_light_color);

	/* Camera */
	m_camera = m_scene_manager->createCamera("rig_editor_camera");
	m_camera->setNearClipDistance(config->camera_near_clip_distance);
	m_camera->setFarClipDistance(config->camera_far_clip_distance);
	m_camera->setFOVy(Ogre::Degree(config->camera_FOVy_degrees));
	m_camera->setAutoAspectRatio(true);

	/* Setup input */
	m_input_handler = new InputHandler();

	/* Camera handling */
	m_camera_handler = new CameraHandler(m_camera);
	m_camera_handler->SetOrbitTarget(m_scene_manager->getRootSceneNode());	
	m_camera_handler->SetOrthoZoomRatio(config->ortho_camera_zoom_ratio);
	m_camera->setPosition(Ogre::Vector3(10,5,10));

	/* Debug output box */
	m_debug_box = MyGUI::Gui::getInstance().createWidget<MyGUI::TextBox>
		("TextBox", 10, 30, 400, 100, MyGUI::Align::Top, "Main", "rig_editor_quick_debug_text_box");
	m_debug_box->setCaption("Hello\nRigEditor!");
	m_debug_box->setTextColour(MyGUI::Colour(0.8, 0.8, 0.8));
	m_debug_box->setFontName("DefaultBig");
}

Main::~Main()
{
	if (m_gui_menubar != nullptr)
	{
		delete m_gui_menubar;
		m_gui_menubar = nullptr;
	}
	if (m_gui_open_save_file_dialog != nullptr)
	{
		delete m_gui_open_save_file_dialog;
		m_gui_open_save_file_dialog = nullptr;
	}

	if (m_rig != nullptr)
	{
		m_rig->DetachFromScene();
		delete m_rig;
		m_rig = nullptr;
	}
}

void Main::EnterMainLoop()
{
	/* Setup 3D engine */
	OgreSubsystem* ror_ogre_subsystem = RoR::Application::GetOgreSubsystem();
	assert(ror_ogre_subsystem != nullptr);
	m_viewport = ror_ogre_subsystem->GetRenderWindow()->addViewport(nullptr);
	int viewport_width = m_viewport->getActualWidth();
	m_viewport->setBackgroundColour(m_config->viewport_background_color);
	m_camera->setAspectRatio(m_viewport->getActualHeight() / viewport_width);
	m_viewport->setCamera(m_camera);

	/* Setup GUI */
	RoR::Application::GetGuiManager()->SetSceneManager(m_scene_manager);
	if (m_gui_menubar == nullptr)
	{
		m_gui_menubar = new GUI::RigEditorMenubar(this);
	}
	else
	{
		m_gui_menubar->Show();
	}
	m_gui_menubar->SetWidth(viewport_width);
	if (m_gui_open_save_file_dialog == nullptr)
	{
		m_gui_open_save_file_dialog = new GUI::OpenSaveFileDialog();
	}

	/* Setup input */
	RoR::Application::GetInputEngine()->SetKeyboardListener(m_input_handler);
	RoR::Application::GetInputEngine()->SetMouseListener(m_input_handler);

	/* Show debug box */
	m_debug_box->setVisible(true);

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

	/* Hide GUI */
	m_gui_menubar->Hide();
	if (m_gui_open_save_file_dialog->isModal())
	{
		m_gui_open_save_file_dialog->endModal(); // Hides the dialog
	}

	/* Hide debug box */
	m_debug_box->setVisible(false);

	m_exit_loop_requested = false;
}

void Main::UpdateMainLoop()
{
	/* Process window events */
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32 || OGRE_PLATFORM == OGRE_PLATFORM_LINUX
	RoRWindowEventUtilities::messagePump();
#endif

	/* Update input events */
	m_input_handler->ResetEvents();
	RoR::Application::GetInputEngine()->Capture(); // Also injects input to GUI

	/* Handle key presses */
	if (m_input_handler->WasEventFired(InputHandler::Event::QUIT_RIG_EDITOR))
	{
		m_exit_loop_requested = true;
		return;
	}
	bool camera_ortho_toggled = false;
	if (m_input_handler->WasEventFired(InputHandler::Event::CAMERA_VIEW_TOGGLE_PERSPECTIVE))
	{
		m_camera_handler->ToggleOrtho();
		camera_ortho_toggled = true;
	}

	/* Handle camera control */
	bool camera_view_changed = false;
	if (m_input_handler->GetMouseMotionEvent().HasMoved() || m_input_handler->GetMouseMotionEvent().HasScrolled())
	{
		camera_view_changed = m_camera_handler->InjectMouseMove(
			m_input_handler->GetMouseButtonEvent().IsRightButtonDown(), /* (bool do_orbit) */
			m_input_handler->GetMouseMotionEvent().rel_x,
			m_input_handler->GetMouseMotionEvent().rel_y,
			m_input_handler->GetMouseMotionEvent().rel_wheel
		);
	}

	/* Handle mouse selection of nodes */
	if (m_rig != nullptr)
	{
		if (camera_view_changed || camera_ortho_toggled)
		{
			m_rig->RefreshAllNodesScreenPositions(m_camera_handler);
		}

		bool node_selection_changed = false;
		bool node_hover_changed = false;
		if (m_input_handler->GetMouseMotionEvent().HasMoved() || camera_view_changed || camera_ortho_toggled)
		{
			node_hover_changed = m_rig->RefreshMouseHoveredNode(m_input_handler->GetMouseMotionEvent().GetAbsolutePosition());
		}

		if ( (! node_hover_changed) && m_input_handler->GetMouseButtonEvent().WasLeftButtonPressed())
		{
			node_selection_changed = m_rig->ToggleMouseHoveredNodeSelected();
		}

		if (node_selection_changed || node_hover_changed)
		{
			m_rig->RefreshNodesDynamicMeshes(m_scene_manager->getRootSceneNode());
		}
	}

	/* Update devel console */
	std::stringstream msg;
	msg << "Camera pos: [X "<<m_camera->getPosition().x <<", Y "<<m_camera->getPosition().y << ", Z "<<m_camera->getPosition().z <<"] "<<std::endl;
	msg << "Mouse node: ";
	if (m_rig != nullptr && m_rig->GetMouseHoveredNode() != nullptr)
	{
		RigDef::Node const & node_def = m_rig->GetMouseHoveredNode()->GetDefinition();
		msg << node_def.id.ToString() << std::endl;
	}
	else
	{
		msg << "[null]"<< std::endl;
	}
	m_debug_box->setCaption(msg.str());
}

void Main::CommandShowDialogOpenRigFile()
{
	m_gui_open_save_file_dialog->setDialogInfo(MyGUI::UString("Open rig file"), MyGUI::UString("Open"), false);
	m_gui_open_save_file_dialog->eventEndDialog = MyGUI::newDelegate(this, &Main::NotifyFileSelectorEnded);
	m_gui_open_save_file_dialog->setMode(OpenSaveFileDialogMode::MODE_OPEN_TRUCK);
	m_gui_open_save_file_dialog->doModal(); // Shows the dialog
}

void Main::CommandShowDialogSaveRigFileAs()
{
	// TODO
}

void Main::CommandSaveRigFile()
{
	// TODO
}

void Main::CommandCloseCurrentRig()
{
	if (m_rig != nullptr)
	{
		m_rig->DetachFromScene();
		delete m_rig;
		m_rig = nullptr;
	}
}

void Main::NotifyFileSelectorEnded(GUI::Dialog* dialog, bool result)
{
	if (result)
	{
		const MyGUI::UString & mode = m_gui_open_save_file_dialog->getMode();

		if (mode == OpenSaveFileDialogMode::MODE_OPEN_TRUCK)
		{
			LoadRigDefFile(m_gui_open_save_file_dialog->getCurrentFolder(), m_gui_open_save_file_dialog->getFileName());
		}
		else if (mode == OpenSaveFileDialogMode::MODE_SAVE_TRUCK_AS)
		{
			// TODO
		}
	}
	dialog->endModal(); // Hides the dialog
}

void RigEditor_LogParserMessages(RigDef::Parser & parser)
{
	if (parser.GetMessages().size() == 0)
	{
		LOG("RigEditor: Parsing done OK");
		return;
	}

	std::stringstream report;
	report << "RigEditor: Parsing done, report:" << std::endl <<std::endl;

	for (auto iter = parser.GetMessages().begin(); iter != parser.GetMessages().end(); iter++)
	{
		switch (iter->type)
		{
			case (RigDef::Parser::Message::TYPE_FATAL_ERROR): 
				report << "FATAL_ERROR"; 
				break;

			case (RigDef::Parser::Message::TYPE_ERROR): 
				report << "ERROR"; 
				break;

			case (RigDef::Parser::Message::TYPE_WARNING): 
				report << "WARNING"; 
				break;

			default:
				report << "INFO"; 
				break;
		}
		report << " (Section " << RigDef::File::SectionToString(iter->section) << ")" << std::endl;
		report << "\tLine (# " << iter->line_number << "): " << iter->line << std::endl;
		report << "\tMessage: " << iter->message << std::endl;
	}

	Ogre::LogManager::getSingleton().logMessage(report.str());
}

void RigEditor_LogValidatorMessages(RigDef::Validator & validator)
{
	if (validator.GetMessages().empty())
	{
		Ogre::LogManager::getSingleton().logMessage("RigEditor: Validating done OK");
		return;
	}

	std::ostringstream report;
	report << "RigEditor: Validating done, report:" <<std::endl << std::endl;

	for(auto itor = validator.GetMessages().begin(); itor != validator.GetMessages().end(); itor++)
	{
		switch (itor->type)
		{
			case (RigDef::Validator::Message::TYPE_FATAL_ERROR):
				report << "FATAL ERROR";
				break;
			case (RigDef::Validator::Message::TYPE_ERROR):
				report << "ERROR";
				break;
			case (RigDef::Validator::Message::TYPE_WARNING):
				report << "WARNING";
				break;
			default:
				report << "INFO";
		}

		report << ": " << itor->text << std::endl;
	}

	Ogre::LogManager::getSingleton().logMessage(report.str());
}

bool Main::LoadRigDefFile(MyGUI::UString const & directory, MyGUI::UString const & filename)
{
	Ogre::DataStreamPtr stream = Ogre::DataStreamPtr();
	//Ogre::String fixed_file_name = file_path;
	//Ogre::String found_resource_group;
	Ogre::String resource_group_name("RigEditor_CurrentProject");

	try
	{
		Ogre::ResourceGroupManager::getSingleton().addResourceLocation(directory, "FileSystem", resource_group_name);

		//RoR::Application::GetCacheSystem()->checkResourceLoaded(fixed_file_name, found_resource_group); /* Fixes the filename and finds resource group */
		stream = Ogre::ResourceGroupManager::getSingleton().openResource(filename, resource_group_name);
	} 
	catch (Ogre::Exception& e)
	{
		// TODO: Report error to user

		LOG("RigEditor: Failed to retrieve rig file" + filename + ", Ogre::Exception was thrown with message: " + e.what());
		return false;
	}

	/* PARSE */

	LOG("RigEditor: Parsing rig file: " + filename);

	RigDef::Parser parser;
	parser.Prepare();
	while(! stream->eof())
	{
		parser.ParseLine(stream->getLine());
	}
	parser.Finalize();

	RigEditor_LogParserMessages(parser);

	/* VALIDATE */

	LOG("RigEditor: Validating vehicle: " + parser.GetFile()->name);

	RigDef::Validator validator;
	validator.Setup(parser.GetFile());
	bool valid = validator.Validate();

	RigEditor_LogValidatorMessages(validator);

	if (! valid)
	{
		// TODO: Report error to user

		LOG("RigEditor: Validating failed!");
		return false;
	}

	/* BUILD RIG MESH */

	RigFactory rig_factory;
	std::vector< boost::shared_ptr<RigDef::File::Module> > selected_modules;
	selected_modules.push_back(parser.GetFile()->root_module); // TODO: Handle multiple modules
	m_rig = rig_factory.BuildRig(parser.GetFile().get(), selected_modules, this);

	/* SHOW MESH */

	m_rig->AttachToScene(m_scene_manager->getRootSceneNode());
	/* Handle mouse selection of nodes */
	m_rig->RefreshAllNodesScreenPositions(m_camera_handler);
	if (m_rig->RefreshMouseHoveredNode(m_input_handler->GetMouseMotionEvent().GetAbsolutePosition()))
	{
		m_rig->RefreshNodesDynamicMeshes(m_scene_manager->getRootSceneNode());
	}

	LOG("RigEditor: Rig loaded OK");

	return true;
}
