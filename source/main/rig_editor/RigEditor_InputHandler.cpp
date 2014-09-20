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
	@file   RigEditor_InputHandler.cpp
	@date   06/2014
	@author Petr Ohlidal
*/

#include "RigEditor_InputHandler.h"

#include "Application.h"
#include "GUIManager.h"
#include "InputEngine.h"

using namespace RoR;
using namespace RoR::RigEditor;

// ================================================================================
// Static variables
// ================================================================================

#define DECLARE_EVENT(_FIELD_, _INDEX_, _NAME_) const InputHandler::Event InputHandler::Event::_FIELD_(_INDEX_, _NAME_);

DECLARE_EVENT (                           INVALID,   0,                             "INVALID" )
DECLARE_EVENT (                 CAMERA_VIEW_FRONT,   1,                   "CAMERA_VIEW_FRONT" )
DECLARE_EVENT (                  CAMERA_VIEW_SIDE,   2,                    "CAMERA_VIEW_SIDE" )
DECLARE_EVENT (                   CAMERA_VIEW_TOP,   3,                     "CAMERA_VIEW_TOP" )
DECLARE_EVENT (    CAMERA_VIEW_TOGGLE_PERSPECTIVE,   4,      "CAMERA_VIEW_TOGGLE_PERSPECTIVE" )
DECLARE_EVENT (                   QUIT_RIG_EDITOR,   5,                     "QUIT_RIG_EDITOR" )

#define DECLARE_MODE(_FIELD_, _INDEX_, _NAME_) const InputHandler::Mode InputHandler::Mode::_FIELD_(_INDEX_, _NAME_);

DECLARE_MODE (                            INVALID,   0,                             "INVALID" )
DECLARE_MODE (                    CREATE_NEW_NODE,   1,                     "CREATE_NEW_NODE" )
DECLARE_MODE (                         GRAB_NODES,   2,                          "GRAB_NODES" )

// ================================================================================
// Functions
// ================================================================================

Vector2int InputHandler::MouseMotionEvent::GetPreviousAbsolutePosition() const
{
	return Vector2int( abs_x - rel_x, abs_y - rel_y );
}

InputHandler::MouseMotionEvent::MouseMotionEvent():
	rel_x(0),
	rel_y(0),
	rel_wheel(0),
	abs_x(0),
	abs_y(0),
	abs_wheel(0)
{
}

InputHandler::InputHandler()
{
	std::memset(m_event_key_mappings, 0, KEY_MAPPING_ARRAY_SIZE * sizeof(Event *)); 
	std::memset(m_mode_key_mappings, 0, KEY_MAPPING_ARRAY_SIZE * sizeof(Mode *)); 

	SetupDefaultKeyMappings();
}

void InputHandler::SetupDefaultKeyMappings()
{
	m_event_key_mappings[OIS::KC_NUMPAD1] = & Event::CAMERA_VIEW_FRONT;
	m_event_key_mappings[OIS::KC_NUMPAD3] = & Event::CAMERA_VIEW_SIDE;
	m_event_key_mappings[OIS::KC_NUMPAD7] = & Event::CAMERA_VIEW_TOP;
	m_event_key_mappings[OIS::KC_NUMPAD5] = & Event::CAMERA_VIEW_TOGGLE_PERSPECTIVE;
	m_event_key_mappings[OIS::KC_ESCAPE]  = & Event::QUIT_RIG_EDITOR;

	m_mode_key_mappings[OIS::KC_N]        = & Mode::CREATE_NEW_NODE;
	m_mode_key_mappings[OIS::KC_G]        = & Mode::GRAB_NODES;
}

bool InputHandler::WasEventFired(Event const & event)
{
	return m_events_fired.test(event.index);
}

void InputHandler::ResetEvents()
{
	m_events_fired.reset();

	m_mouse_motion_event.ResetRelativeMove();
	m_mouse_motion_event.ResetRelativeScroll();

	m_mouse_button_event.ResetEvents();

	m_modes_entered.reset();
	m_modes_exited.reset();
}

InputHandler::MouseMotionEvent const & InputHandler::GetMouseMotionEvent()
{
	return m_mouse_motion_event;
}

InputHandler::MouseButtonEvent const & InputHandler::GetMouseButtonEvent()
{
	return m_mouse_button_event;
}

bool InputHandler::IsModeActive(Mode const & mode)
{
	return m_active_modes[mode.index];
}

bool InputHandler::WasModeEntered(Mode const & mode)
{
	return m_modes_entered[mode.index];
}
	
bool InputHandler::WasModeExited(Mode const & mode)
{
	return m_modes_exited[mode.index];
}

// ================================================================================
// OIS Keyboard listener
// ================================================================================

bool InputHandler::keyPressed( const OIS::KeyEvent &arg )
{
	if (RoR::Application::GetGuiManager()->keyPressed(arg))
	{
		return true;
	}

	// HANDLE EVENTS \\

	const Event* event_ptr = m_event_key_mappings[arg.key];
	if (event_ptr != nullptr) 
	{
		m_events_fired[event_ptr->index] = true;
	}
	
	// HANDLE MODES \\

	const Mode* mode_ptr = m_mode_key_mappings[arg.key];
	if (mode_ptr != nullptr) 
	{
		m_active_modes[mode_ptr->index] = true;
		m_modes_entered[mode_ptr->index] = true;
	}

	return true;
}

bool InputHandler::keyReleased( const OIS::KeyEvent &arg )
{
	if (RoR::Application::GetGuiManager()->keyReleased(arg))
	{
		return true;
	}

	// HANDLE MODES \\

	const Mode* mode_ptr = m_mode_key_mappings[arg.key];
	if (mode_ptr != nullptr) 
	{
		m_active_modes[mode_ptr->index] = false;
		m_modes_exited[mode_ptr->index] = true;
	}

	return true;
}

// ================================================================================
// OIS Mouse listener
// ================================================================================

bool InputHandler::mouseMoved( const OIS::MouseEvent &mouse_event )
{
	MyGUI::InputManager::getInstance().injectMouseMove(
		mouse_event.state.X.abs, 
		mouse_event.state.Y.abs,
		mouse_event.state.Z.abs
		);

	m_mouse_motion_event.AddRelativeMove(mouse_event.state.X.rel, mouse_event.state.Y.rel, mouse_event.state.Z.rel);
	m_mouse_motion_event.abs_x = mouse_event.state.X.abs;
	m_mouse_motion_event.abs_y = mouse_event.state.Y.abs;
	m_mouse_motion_event.abs_wheel = mouse_event.state.Z.abs;

	return true;
}

bool InputHandler::mousePressed( const OIS::MouseEvent &mouse_event, OIS::MouseButtonID button_id )
{
	MyGUI::InputManager::getInstance().injectMousePress(
		mouse_event.state.X.abs, 
		mouse_event.state.Y.abs,
		MyGUI::MouseButton::Enum(button_id)
		);

	switch (button_id)
	{
	case OIS::MB_Right:
		m_mouse_button_event.RightButtonDown();
		break;
	case OIS::MB_Left:
		m_mouse_button_event.LeftButtonDown();
		break;
	case OIS::MB_Middle:
		m_mouse_button_event.MiddleButtonDown();
		break;
	default:
		assert(false && "Invalid OIS::MouseButtonID");
	}

	return true;
}

bool InputHandler::mouseReleased( const OIS::MouseEvent &mouse_event, OIS::MouseButtonID button_id )
{
	MyGUI::InputManager::getInstance().injectMouseRelease(
		mouse_event.state.X.abs, 
		mouse_event.state.Y.abs,
		MyGUI::MouseButton::Enum(button_id)
		);

	switch (button_id)
	{
	case OIS::MB_Right:
		m_mouse_button_event.RightButtonUp();
		break;
	case OIS::MB_Left:
		m_mouse_button_event.LeftButtonUp();
		break;
	case OIS::MB_Middle:
		m_mouse_button_event.MiddleButtonUp();
		break;
	default:
		assert(false && "Invalid OIS::MouseButtonID");
	}

	return true;
}
