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

// ================================================================================
// Functions
// ================================================================================

InputHandler::InputHandler()
{
	std::memset(m_key_mappings, 0, KEY_MAPPING_ARRAY_SIZE * sizeof(Event *)); 

	SetupDefaultKeyMappings();
}

void InputHandler::SetupDefaultKeyMappings()
{
	m_key_mappings[OIS::KC_NUMPAD1] = & Event::CAMERA_VIEW_FRONT;
	m_key_mappings[OIS::KC_NUMPAD3] = & Event::CAMERA_VIEW_SIDE;
	m_key_mappings[OIS::KC_NUMPAD7] = & Event::CAMERA_VIEW_TOP;
	m_key_mappings[OIS::KC_NUMPAD5] = & Event::CAMERA_VIEW_TOGGLE_PERSPECTIVE;
	m_key_mappings[OIS::KC_ESCAPE]  = & Event::QUIT_RIG_EDITOR;
}

bool InputHandler::WasEventFired(Event const & event)
{
	return m_events_fired.test(event.index);
}

// ================================================================================
// OIS Keyboard listener
// ================================================================================

bool InputHandler::keyPressed( const OIS::KeyEvent &arg )
{
	const Event* event_ptr = m_key_mappings[arg.key];
	assert(event_ptr != nullptr);
	unsigned int event_index = event_ptr->index;
	m_events_fired[event_index] = true;
	return true;
}

bool InputHandler::keyReleased( const OIS::KeyEvent &arg )
{
	return true;
}

// ================================================================================
// OIS Mouse listener
// ================================================================================

bool InputHandler::mouseMoved( const OIS::MouseEvent &arg )
{
	return true;
} // Stub!

bool InputHandler::mousePressed( const OIS::MouseEvent &arg, OIS::MouseButtonID id )
{
	return true;
} // Stub!

bool InputHandler::mouseReleased( const OIS::MouseEvent &arg, OIS::MouseButtonID id )
{
	return true;
} // Stub!
