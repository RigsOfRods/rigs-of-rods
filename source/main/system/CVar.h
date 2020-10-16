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

#include "BitFlags.h"

#include <Ogre.h>
#include <string>

namespace RoR {

enum CVarFlags
{
    CVAR_TYPE_BOOL    = BITMASK(1),
    CVAR_TYPE_INT     = BITMASK(2),
    CVAR_TYPE_FLOAT   = BITMASK(3),
    CVAR_ARCHIVE      = BITMASK(4),    //!< Will be written to RoR.cfg
    CVAR_NO_LOG       = BITMASK(5)     //!< Will not be written to RoR.log
};

/// Inspired by Quake II:
///   * Struct:     https://github.com/yquake2/yquake2/blob/master/src/common/header/shared.h#L332
///   * Functions:  https://github.com/yquake2/yquake2/blob/master/src/common/cvar.c
///   * Global var: https://github.com/yquake2/yquake2/blob/master/src/common/header/common.h#L432
///
/// @author Petr Ohlidal, 2020
class CVar
{
public:

    CVar(std::string const& name, std::string const& long_name, int flags):
        m_name(name),
        m_long_name(long_name),
        m_flags(flags),
        m_value_num(0.f)
    {
        // Initialize string representation
        if (this->HasFlag(CVAR_TYPE_BOOL | CVAR_TYPE_INT | CVAR_TYPE_FLOAT))
        {
            m_value_str = this->ConvertStr(m_value_num);
        }
    }

    // Data setters

    template <typename T>
    void SetVal(T val)
    {
        if ((T)m_value_num != val)
        {
            std::string str = this->ConvertStr((float)val);
            this->LogUpdate(str);
            m_value_num = (float)val;
            m_value_str = str;
        }
    }

    void SetStr(std::string const& str)
    {
        if (m_value_str != str)
        {
            this->LogUpdate(str);
            m_value_num = 0;
            m_value_str = str;
        }
    }

    // Data getters

    std::string const&      GetStr() const   { return m_value_str; }
    float                   GetFloat() const { return m_value_num; }
    int                     GetInt() const   { return (int)m_value_num; }
    bool                    GetBool() const  { return (bool)m_value_num; }
    template<typename T> T  GetEnum() const  { return (T)this->GetInt(); }

    // Info getters

    std::string const&      GetName() const       { return m_name; }
    std::string const&      GetLongName() const   { return m_long_name; }
    bool                    HasFlag(int f) const  { return m_flags & f; }

private:
    void                    LogUpdate(std::string const& new_val);
    std::string             ConvertStr(float val);

    // Variables

    std::string        m_name;
    std::string        m_long_name;
                       
    std::string        m_value_str;
    float              m_value_num;
    int                m_flags;
};

} // namespace RoR
