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

static const char* CVAR_LOG_FMT = "[RoR|CVar]  %20s:  %s(), new: \"%s\", old: \"%s\"%s";

enum CVarFlags
{
    CVAR_ALLOW_STORE  = BITMASK(1),    //!< Will be written to RoR.cfg
    CVAR_AUTO_STORE   = BITMASK(2),    //!< 'Active' value automatically propagates to 'Stored'
    CVAR_AUTO_APPLY   = BITMASK(3),    //!< 'Stored/Pending' values automatically propagate to 'Active/Pending'
    CVAR_TYPE_BOOL    = BITMASK(4),
    CVAR_TYPE_INT     = BITMASK(5),
    CVAR_TYPE_FLOAT   = BITMASK(6),
    CVAR_FORCE_APPLY  = BITMASK(7),    //!< Function call argument only
    CVAR_FORCE_STORE  = BITMASK(8)     //!< Function call argument only
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
        this->LogVal("SetActive", m_value_active.GetValue<float>(), (float)val);

        m_value_active.SetValue(val, m_flags);
        m_value_pending.SetValue(val, m_flags);
        if (this->HasFlags(CVAR_ALLOW_STORE | CVAR_AUTO_STORE))
        {
            m_value_stored.SetValue(val, m_flags);
        }
    }

    void SetActiveStr(std::string const& str)
    {
        this->LogStr("SetActive", m_value_active.GetString(), str);

        m_value_active.SetString(str);
        m_value_pending.SetString(str);
        if (this->HasFlags(CVAR_ALLOW_STORE | CVAR_AUTO_STORE))
        {
            m_value_stored.SetString(str);
        }
    }

    template <typename T>
    void SetPendingVal(T val)
    {
        this->LogVal("SetPending", m_value_pending.GetValue<float>(), (float)val);

        m_value_pending.SetValue(val, m_flags);
        if (this->HasFlags(CVAR_AUTO_APPLY))
        {
            m_value_active.SetValue(val, m_flags);
            if (this->HasFlags(CVAR_ALLOW_STORE | CVAR_AUTO_STORE))
            {
                m_value_stored.SetValue(val, m_flags);
            }
        }
    }

    void SetPendingStr(std::string const& str)
    {
        this->LogStr("SetPending", m_value_pending.GetString(), str);

        m_value_pending.SetString(str);
        if (this->HasFlags(CVAR_AUTO_APPLY))
        {
            m_value_active.SetString(str);
            if (this->HasFlags(CVAR_ALLOW_STORE | CVAR_AUTO_STORE))
            {
                m_value_stored.SetString(str);
            }
        }
    }

    template <typename T>
    void SetStoredVal(T val)
    {
        if (this->HasFlags(CVAR_ALLOW_STORE))
        {
            this->LogVal("SetStored", m_value_stored.GetValue<float>(), (float)val);

            m_value_stored.SetValue(val, m_flags);
            if (this->HasFlags(CVAR_AUTO_APPLY))
            {
                m_value_active.SetValue(val, m_flags);
                m_value_pending.SetValue(val, m_flags);
            }
        }
    }

    void SetStoredStr(std::string const& str)
    {
        if (this->HasFlags(CVAR_ALLOW_STORE))
        {
            this->LogStr("SetStored", m_value_stored.GetString(), str);

            m_value_stored.SetString(str);
            if (this->HasFlags(CVAR_AUTO_APPLY))
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

    bool CheckPending()
    {
        return m_value_active != m_value_pending;
    }

    bool ApplyPending()
    {
        const bool pending = this->CheckPending();
        if (pending)
        {
            if (this->HasFlags(CVAR_TYPE_BOOL | CVAR_TYPE_INT | CVAR_TYPE_FLOAT))
                this->LogVal("ApplyPending", m_value_active.GetValue<float>(), m_value_pending.GetValue<float>());
            else
                this->LogStr("ApplyPending", m_value_active.GetString(), m_value_pending.GetString());
            m_value_active = m_value_pending;
        }
        return pending;
    }

    bool ResetPending()
    {
        const bool pending = this->CheckPending();
        if (pending)
        {
            if (this->HasFlags(CVAR_TYPE_BOOL | CVAR_TYPE_INT | CVAR_TYPE_FLOAT))
                this->LogVal("ResetPending", m_value_pending.GetValue<float>(), m_value_active.GetValue<float>());
            else
                this->LogStr("ResetPending", m_value_pending.GetString(), m_value_active.GetString());

            m_value_pending = m_value_active;
        }
        return pending;
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

    void LogVal(const char* op, float old_val, float new_val);
    void LogStr(const char* op, std::string const& old_val, std::string const& new_val);

    std::string         m_name;
    std::string         m_long_name;

    Val                 m_value_active;
    Val                 m_value_pending;
    Val                 m_value_stored;

    int                 m_flags;
};

} // namespace RoR
