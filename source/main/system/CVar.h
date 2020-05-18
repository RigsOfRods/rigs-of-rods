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
#include "Str.h"

#include <Ogre.h>

namespace RoR {

static const char* CVAR_LOG_FMT = "[RoR|CVar]  %20s:  \"%s\"( old: \"%s\")";

enum CVarFlags
{
    CVAR_ARCHIVE  = BITMASK(1),    //!< Will be written to RoR.cfg
    CVAR_TYPE_BOOL    = BITMASK(4),
    CVAR_TYPE_INT     = BITMASK(5),
    CVAR_TYPE_FLOAT   = BITMASK(6),
    CVAR_NO_LOG       = BITMASK(9)     //!< Will not be written to RoR.log
};

/// Inspired by Quake:
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
        m_flags(flags)
    {}

    void SetFlags(int f)
    {
        BITMASK_SET_1(m_flags, f);
    }

    void ClearFlags(int f)
    {
        BITMASK_SET_0(m_flags, f);
    }

    bool HasFlags(int f)
    {
        return BITMASK_IS_1(m_flags, f);
    }

    template <typename T>
    void SetActiveVal(T val)
    {
        if (m_value_active.GetValue<T>() != val)
        {
            this->LogUpdate(m_value_active.GetString(), Val::ConvertStr(val, m_flags));
            m_value_active.SetValue(val, m_flags);
        }
    }

    void SetActiveStr(std::string const& str)
    {
        if (m_value_active.GetString() != str)
        {
            this->LogUpdate(m_value_active.GetString(), str);
            m_value_active.SetString(str);
        }
    }

    std::string const& GetActiveStr()
    {
        return m_value_active.GetString();
    }

    template <typename T>
    T GetActiveVal()
    {
        return m_value_active.GetValue<T>();
    }

    template <typename T>
    T GetActiveEnum()
    {
        return (T)m_value_active.GetValue<int>();
    }

    std::string const& GetName() const
    {
        return m_name;
    }

    std::string const& GetLongName() const
    {
        return m_long_name;
    }

private:
    class Val
    {
    public:
        static std::string ConvertStr(float val, int flags)
        {
            Str<50> buf;
            if (flags & CVAR_TYPE_BOOL)
            {
                buf = ((bool)val) ? "Yes" : "No";
            }
            else if (flags & CVAR_TYPE_INT)
            {
                buf << (int)val;
            }
            else if (flags & CVAR_TYPE_FLOAT)
            {
                buf << (float)val;
            }
            else
            {
                buf = std::to_string(val);
            }
            return buf.ToCStr();
        }

        template<typename T>
        void SetValue(T val, int flags)
        {
            m_value_num = (float)val;
            m_value_str = Val::ConvertStr((float)val, flags);
        }

        void SetString(std::string const& str)
        {
            m_value_str = str;
            m_value_num = 0;
        }

        template<typename T>
        T GetValue()
        {
            return (T) m_value_num;
        }

        std::string const& GetString()
        {
            return m_value_str;
        }

        bool operator==(const Val& other) const
        {
            return m_value_num == other.m_value_num && m_value_str == other.m_value_str;
        }

        bool operator!=(const Val& other) const
        {
            return !(*this == other);
        }

    private:
        float          m_value_num;
        std::string    m_value_str;
    };

    void LogUpdate(std::string const& old_val, std::string const& new_val);

    std::string         m_name;
    std::string         m_long_name;

    Val                 m_value_active;

    int                 m_flags;
};

} // namespace RoR
