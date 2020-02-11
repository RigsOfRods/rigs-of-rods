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

#include "Str.h"

#include <Ogre.h>

namespace RoR {

enum CVarFlags
{
    CVAR_ALLOW_STORE  = 1,    //<! Will be written to RoR.cfg
    CVAR_AUTO_STORE   = 2,    //!< 'Active' value automatically propagates to 'Stored'
    CVAR_AUTO_APPLY   = 4,    //!< 'Pending' value automatically propagates to 'Active'
    CVAR_TYPE_BOOL    = 8,
    CVAR_TYPE_INT     = 16,
    CVAR_TYPE_FLOAT   = 32,
};

/// Inspired by Quake:
///   * Struct:     https://github.com/yquake2/yquake2/blob/master/src/common/header/shared.h#L332
///   * Functions:  https://github.com/yquake2/yquake2/blob/master/src/common/cvar.c
///   * Global var: https://github.com/yquake2/yquake2/blob/master/src/common/header/common.h#L432
///
/// Concept:
///   * [A] Active value = the value currently in effect. Each CVar has an active value.
///   * [P] Pending value = the value to be set as active on earliest occasion (occasion may be anything from next frame to game restart)
///                         When no change is requested, value of Pending equals value of Active.
///   * [S] Stored value = The user-defined value to be persisted in config file.
///                        - it's unaffected by Active and only changes when user wants.
///
/// API usage:
///   SetPending():   new pending value, active stands.
///   SetActive():    direct update of active & pending values.
///   ApplyPending(): updates active from pending.
///   ResetPending(): updates pending from active.
///   GetStored():    gets stored value.
///   SetStored():    direct update of stored value.
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
        m_flags |= f;
    }

    void ClearFlags(int f)
    {
        m_flags &= ~f;
    }

    bool HasFlags(int f)
    {
        return m_flags & f;
    }

    template <typename T>
    void SetActiveVal(T val)
    {
        m_value_active.SetValue(val, m_flags);
        m_value_pending.SetValue(val, m_flags);
        if (m_flags & (CVAR_ALLOW_STORE | CVAR_AUTO_STORE))
        {
            m_value_stored.SetValue(val, m_flags);
        }
    }

    void SetActiveStr(std::string const& str)
    {
        m_value_active.SetString(str);
        m_value_pending.SetString(str);
        if (m_flags & (CVAR_ALLOW_STORE | CVAR_AUTO_STORE))
        {
            m_value_stored.SetString(str);
        }
    }

    template <typename T>
    void SetPendingVal(T val)
    {
        m_value_pending.SetValue(val, m_flags);
        if (m_flags & CVAR_AUTO_APPLY)
        {
            m_value_active.SetValue(val, m_flags);
            if (m_flags & (CVAR_ALLOW_STORE | CVAR_AUTO_STORE))
            {
                m_value_stored.SetValue(val, m_flags);
            }
        }
    }

    void SetPendingStr(std::string const& str)
    {
        m_value_pending.SetString(str);
        if (m_flags & CVAR_AUTO_APPLY)
        {
            m_value_active.SetString(str);
            if (m_flags & (CVAR_ALLOW_STORE | CVAR_AUTO_STORE))
            {
                m_value_stored.SetString(str);
            }
        }
    }

    template <typename T>
    void SetStoredVal(T val)
    {
        if (m_flags & CVAR_ALLOW_STORE)
        {
            m_value_stored.SetValue(val, m_flags);
            if (m_flags & CVAR_AUTO_APPLY)
            {
                m_value_active.SetValue(val, m_flags);
                m_value_pending.SetValue(val, m_flags);
            }
        }
    }

    void SetStoredStr(std::string const& str)
    {
        if (m_flags & CVAR_ALLOW_STORE)
        {
            m_value_stored.SetString(str);
            if (m_flags & CVAR_AUTO_APPLY)
            {
                m_value_active.SetString(str);
                m_value_pending.SetString(str);
            }
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

    std::string const& GetPendingStr()
    {
        return m_value_pending.GetString();
    }

    template <typename T>
    T GetPendingVal()
    {
        return m_value_pending.GetValue<T>();
    }

    template <typename T>
    T GetPendingEnum()
    {
        return (T)m_value_pending.GetValue<int>();
    }

    std::string const& GetStoredStr()
    {
        return m_value_stored.GetString();
    }

    template <typename T>
    T GetStoredVal()
    {
        return m_value_stored.GetValue<T>();
    }

    template <typename T>
    T GetStoredEnum()
    {
        return (T)m_value_stored.GetValue<int>();
    }

    void ApplyPending()
    {
        m_value_active = m_value_pending;
    }

    void ResetPending()
    {
        m_value_pending = m_value_active;
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
        template<typename T>
        void SetValue(T val, int flags)
        {
            m_value_num = (float)val;

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
            m_value_str = buf.ToCStr();
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

    private:
        float          m_value_num;
        std::string    m_value_str;
    };

    std::string         m_name;
    std::string         m_long_name;

    Val                 m_value_active;
    Val                 m_value_pending;
    Val                 m_value_stored;

    int                 m_flags;
};

} // namespace RoR
