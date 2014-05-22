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
	along with Rigs of Rods.  If not, see <http://www.gnu.org/licenses/>.
*/

/** 
	@file   MainThread.h
	@author Petr Ohlidal
	@date   05/2014
	@brief  Main thread logic. Manages startup and shutdown etc...
*/

#pragma once

#include "RoRPrerequisites.h"

#include <pthread.h>

namespace RoR
{

class MainThread
{

public:

	MainThread();
	
	void Go();

	void Exit();

	void Shutdown();

	void ShutdownSynced();

protected:

	void EnterMainLoop();

	bool               m_no_rendering;
	bool               m_shutdown;
	pthread_mutex_t    m_lock;
	RoRFrameListener*  m_ror_frame_listener;
};

} // namespace RoR