/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2013-2020 Petr Ohlidal

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


/// @file
/// @date   4th of January 2009
/// @author Thomas Fischer
/// @description This is a global configuration hub.
///     Values from both config file and command line are propagated here
///     and converted to GVars - see 'ParseGlobalVarSetting()' for details.
///     Entries without corresponding GVar can be read
///     by ad-hoc by macros like SSETTING(), BSETTING() etc...


#pragma once

#include "RoRPrerequisites.h"

#include "Singleton.h"



class Settings : public RoRSingleton<Settings>, public ZeroedMemoryAllocator
{
    friend class RoRSingleton<Settings>;

public:

    void LoadRoRCfg(); // Reads GVars

    void SaveSettings(); // Writes GVars

    void ParseGlobalVarSetting(RoR::CVar* cvar, std::string const & val);

    static bool SetupAllPaths();

protected:

    static Settings* myInstance;
};
