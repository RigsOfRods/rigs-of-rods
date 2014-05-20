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
	@file   AppStateManager.h
*/

#pragma once

#include "RoRPrerequisites.h"
#include "AppState.h"

#include <pthread.h>

class AppStateManager : public AppStateListener
{
	friend class RoR::Application; // Manages lifecycle of this class.

public:

	struct state_info
	{
		Ogre::String name;
		AppState* state;
	};	

	void manageAppState(Ogre::String stateName, AppState* state);

	AppState* findByName(Ogre::String stateName);

	void update(double dt);

	void start(AppState* state);
	void changeAppState(AppState* state);
	bool pushAppState(AppState* state);
	void popAppState();
	void pauseAppState();
	void shutdown();
	void tryShutdown(); // used upon errors and such where the state (and thus locking) is not clear
	void pauseRendering();
	void resized(Ogre::RenderWindow* rw);
    void popAllAndPushAppState(AppState* state);

protected:

	AppStateManager();
	~AppStateManager();

	void init(AppState *state);
	pthread_mutex_t lock;

	std::vector<AppState*>		m_ActiveStateStack;
	std::vector<state_info>		m_States;
	bool						m_bShutdown; // exits the program
	bool                        m_bNoRendering; // no more rendering in the main loop
};
