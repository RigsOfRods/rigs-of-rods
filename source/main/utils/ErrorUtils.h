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
	@file   ErrorUtils.h
	@author Thomas Fischer
	@date   3rd October 2009
*/

#pragma once

#include "RoRPrerequisites.h"

struct ErrorUtils
{

	/**
	 * shows a simple error message box
	 * @param title very short summar of the error
	 * @param msg error text
	 * @return 0 on success, everything else on error
	 */
	static int ShowError(Ogre::UTFString title, Ogre::UTFString message);

	/**
	 * shows a simple info message box
	 * @param title very short summar of the error
	 * @param msg error text
	 * @return 0 on success, everything else on error
	 */
	static int ShowInfo(Ogre::UTFString title, Ogre::UTFString message);

	/**
	 * shows a generic message box
	 * @param title very short summar of the error
	 * @param msg error text
	 * @param type 0 for error, 1 for info
	 * @return 0 on success, everything else on error
	 */
	static int ShowMsgBox(Ogre::UTFString title, Ogre::UTFString err, int type);

	/**
	 * shows a simple error message box, and allows the user to open a website to help further.
	 * May not be implemented so that the browers opens for all platforms.
	 * @param title very short summar of the error
	 * @param msg error text
	 * @param url the URL to open if the user wishs
	 * @return 0 on success, everything else on error
	 */
	static int ShowWebError(Ogre::UTFString title, Ogre::UTFString message, Ogre::UTFString url);


	/**
	 * shows a simple error message box, and allows the user to open a website to help further and tries to exit Ogre fullscreen mode
	 * May not be implemented so that the browers opens for all platforms.
	 * @param title very short summar of the error
	 * @param msg error text
	 * @param url the URL to open if the user wishs
	 * @return 0 on success, everything else on error
	 */
	static int ShowOgreWebError(Ogre::UTFString title, Ogre::UTFString err, Ogre::UTFString url);

	static void ShowStoredOgreWebErrors();

};
