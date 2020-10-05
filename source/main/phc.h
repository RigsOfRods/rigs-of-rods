/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2013-2020 Petr Ohlidal & contributors

    For more information, see http://www.rigsofrods.org/

    Rigs of Rods is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 3, as
    published by the Free Software Foundation.

    Rigs of Rods is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Rigs of Rods. If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include <Ogre.h>
#include <OgrePrerequisites.h>
#include <Bites/OgreWindowEventUtilities.h>
#include <OIS.h>
#include <MyGUI.h>
#include <imgui.h>
#include <rapidjson/rapidjson.h>

#ifdef USE_SOCKETW
#include <SocketW.h>
#endif //USE_SOCKETW

#ifdef USE_MOFILEREADER
#include <moFileReader.h>
#endif //USE_MOFILEREADER

#ifdef USE_ANGELSCRIPT
#include <angelscript.h>
#endif //USE_ANGELSCRIPT

#ifdef USE_CURL
#include <curl/curl.h>
#endif //USE_CURL

#ifdef USE_DISCORD_RPC
#include <discord_rpc.h>
#endif //USE_DISCORD_RPC

#ifdef USE_CAELUM
#include <CaelumPrerequisites.h>
#include <Caelum.h>
#endif //USE_CAELUM

#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
#include <windows.h>
#endif

