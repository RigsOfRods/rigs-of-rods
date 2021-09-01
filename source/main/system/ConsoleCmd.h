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

#pragma once

#include "Application.h"
#include "Language.h"

#include <Ogre.h>

namespace RoR {

class ConsoleCmd // abstract
{
public:
    ConsoleCmd(std::string const& name, std::string const& usage, std::string const& doc):
        m_name(name), m_usage(usage), m_doc(doc) {}

    virtual ~ConsoleCmd() {}

    virtual void Run(Ogre::StringVector const& args) = 0;

    std::string const& getName() const { return m_name; }
    std::string const& GetUsage() const { return m_usage; }
    std::string const& GetDoc() const { return m_doc; }

protected:
    bool CheckAppState(AppState state);

    std::string m_name;
    std::string m_usage;
    std::string m_doc;
};

} // namespace RoR
