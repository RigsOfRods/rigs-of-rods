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
	@file   ConfigFile.cpp
	@date   06/2014
	@author Petr Ohlidal
*/

#include "ConfigFile.h"

#include <OgreString.h>
#include <OgreStringConverter.h>

using namespace RoR;

float ConfigFile::GetFloat(Ogre::String const & key)
{
	return Ogre::StringConverter::parseReal(getSetting(key));
}

Ogre::ColourValue ConfigFile::GetColourValue(Ogre::String const & key)
{
	std::stringstream value(getSetting(key));
	float r = 0.f;
	float g = 0.f;
	float b = 0.f;
	float a = 1.f;
	value >> r >> g >> b >> a;
	return Ogre::ColourValue(r, g, b, a);
}

int ConfigFile::GetInt(Ogre::String const & key)
{
	return Ogre::StringConverter::parseInt(getSetting(key));
}

bool ConfigFile::GetBoolOrDefault(Ogre::String const & key, bool defaultValue)
{
	return Ogre::StringConverter::parseBool(getSetting(key), defaultValue);
}

int ConfigFile::GetIntOrDefault(Ogre::String const & key, int defaultValue)
{
	return Ogre::StringConverter::parseInt(getSetting(key), defaultValue);
}

Ogre::String ConfigFile::GetStringOrDefault(Ogre::String const & key, Ogre::String const & defaultValue)
{
	auto setting = getSetting(key);
	if (setting.empty())
	{
		return defaultValue;
	}
	return setting;
}
