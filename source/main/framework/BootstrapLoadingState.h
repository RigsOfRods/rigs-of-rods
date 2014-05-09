/*
This source file is part of Rigs of Rods
Copyright 2005-2012 Pierre-Michel Ricordel
Copyright 2007-2012 Thomas Fischer

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
#ifndef DUMMY_STATE_HPP
#define DUMMY_STATE_HPP

#include "RoRPrerequisites.h"
#include "AppState.h"
#include "ExampleLoadingBar.h"

class BootstrapLoadingState : public AppState
{
public:

	BootstrapLoadingState();
	DECLARE_APPSTATE_CLASS(BootstrapLoadingState)

	void enter();
	void createScene();
	void exit();
	bool pause();
	void resume();

	void update(double timeSinceLastFrame);
	void resized(Ogre::RenderWindow* rw);

protected:

	ExampleLoadingBar *mLoadingBar;
};

#endif //DUMMY_STATE_HPP
