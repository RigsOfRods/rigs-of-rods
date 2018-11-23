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

#include <cstdio>
#include <cstring>
#include <string>

namespace RoR {

/// Wrapper for classic c-string (local buffer)
/// Refresher: `strlen()` excludes '\0' terminator; `strncat()` Appends '\0' terminator
/// @author Petr Ohlidal, 2017
template<size_t L> class Str
{
public:
    // Constructors
    inline             Str()                                 { this->Clear(); }
    inline             Str(Str<L> const & src)               { this->Assign(src); }
    inline             Str(const char* src)                  { this->Assign(src); }
    inline             Str(std::string const& src)           { this->Assign(src.c_str()); }
    inline             Str(std::string::const_iterator& itor,
                           std::string::const_iterator& endi){ this->Assign(itor, endi); }

    // Reading
    inline const char* ToCStr() const                        { return m_buffer; }
    inline bool        IsEmpty() const                       { return m_buffer[0] == '\0'; }
    inline char*       GetBuffer()                           { return m_buffer; }
    inline size_t      GetCapacity() const                   { return m_capacity; }
    inline int         Compare(const char* str) const        { return std::strncmp(m_buffer, str, L); }
    inline size_t      GetLength() const                     { return std::strlen(m_buffer); }

    // Writing
    inline Str&        Clear()                                   { std::memset(m_buffer, 0, L); return *this; }
    inline Str&        Assign(const char* src)                   { this->Clear(); this->Append(src); return *this; }
    inline Str&        Assign(std::string::const_iterator& itor,
                              std::string::const_iterator& endi) { this->Clear(); this->Append(itor, endi); return *this; }

    inline Str&        Append(const char* src)                   { std::strncat(m_buffer, src, (L - (this->GetLength() + 1))); return *this; }
    inline Str&        Append(float f)                           { char buf[50]; std::snprintf(buf, 50, "%f", f); this->Append(buf); return *this; }
    inline Str&        Append(int i)                             { char buf[50]; std::snprintf(buf, 50, "%d", i); this->Append(buf); return *this; }
    inline Str&        Append(size_t z)                          { char buf[50]; std::snprintf(buf, 50, "%lu", static_cast<unsigned long>(z)); this->Append(buf); return *this; }
    inline Str&        Append(char c)                            { char buf[2] = {}; buf[0] = c; this->Append(buf); return *this; }
    inline Str&        Append(std::string::const_iterator& itor,
                              std::string::const_iterator& endi) { for(;itor!=endi;++itor) { this->Append(*itor); } return *this; }

    // Operators
    inline             operator const char*() const          { return this->ToCStr(); }
    inline Str&        operator=  (const char* src)          { return this->Assign(src); }
    inline Str&        operator=  (std::string const& str)   { return this->Assign(str.c_str()); }
    inline Str&        operator<< (const char* src)          { return this->Append(src); }
    inline Str&        operator<< (std::string const& str)   { return this->Append(str.c_str()); }
    inline Str&        operator<< (float f)                  { return this->Append(f); }
    inline Str&        operator<< (int i)                    { return this->Append(i); }
    inline Str&        operator<< (size_t z)                 { return this->Append(z); }
    inline Str&        operator<< (char c)                   { return this->Append(c); }
    inline bool        operator== (const char* other) const  { return (this->Compare(other) == 0); }

private:
    char         m_buffer[L];
    const size_t m_capacity = L;
};

} // namespace RoR
