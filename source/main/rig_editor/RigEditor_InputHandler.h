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
	@file   RigEditor_InputHandler.h
	@date   06/2014
	@author Petr Ohlidal
*/

#pragma once

#include <OISKeyboard.h>
#include <OISMouse.h>
#include <bitset>

namespace RoR
{

namespace RigEditor
{

/** Editor-specific input handler (because class InputEngine is too tightly-coupled with simulation logic)
*/
class InputHandler:
	public OIS::MouseListener,
	public OIS::KeyListener
{
public:

	static const int KEY_MAPPING_ARRAY_SIZE = 256; // Maximum number of keys for OIS.

	struct Event
	{
		static const Event INVALID;
		static const Event CAMERA_VIEW_FRONT;
		static const Event CAMERA_VIEW_SIDE;
		static const Event CAMERA_VIEW_TOP;
		static const Event CAMERA_VIEW_TOGGLE_PERSPECTIVE;
		static const Event QUIT_RIG_EDITOR;

		Event(unsigned int index, const char * name):
			index(index),
			name(name)
		{}

		const char * name;
		unsigned int index;
	};

	InputHandler();

	~InputHandler()
	{}

	bool WasEventFired(Event const & event);

	void ResetEvents();

	/* OIS::KeyListener */
	bool keyPressed( const OIS::KeyEvent &arg );
	bool keyReleased( const OIS::KeyEvent &arg );

	/* OIS MouseListener */
	bool mouseMoved( const OIS::MouseEvent &arg );
	bool mousePressed( const OIS::MouseEvent &arg, OIS::MouseButtonID id );
	bool mouseReleased( const OIS::MouseEvent &arg, OIS::MouseButtonID id );

private:

	void SetupDefaultKeyMappings();

	const Event* m_key_mappings[KEY_MAPPING_ARRAY_SIZE];

	std::bitset<64> m_events_fired;
};

} // namespace RigEditor

} // namespace RoR
