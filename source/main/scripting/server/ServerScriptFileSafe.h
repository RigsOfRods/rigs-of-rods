
// =============================================================================
// This file is adopted from rorserver at commit 4a7109ae2d9a081ccfdad8cc696dc54efe49acb3
// Changes from the original are marked with "//RIGSOFRODS"
// =============================================================================

/*
This file is part of "Rigs of Rods Server" (Relay mode)

Copyright 2007   Pierre-Michel Ricordel
Copyright 2014+  Rigs of Rods Community

"Rigs of Rods Server" is free software: you can redistribute it
and/or modify it under the terms of the GNU General Public License
as published by the Free Software Foundation, either version 3
of the License, or (at your option) any later version.

"Rigs of Rods Server" is distributed in the hope that it will
be useful, but WITHOUT ANY WARRANTY; without even the implied
warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Foobar. If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#ifdef USE_ANGELSCRIPT

namespace RoR { // RIGSOFRODS

// copied from the angelscript library and edited for use in Rigs of Rods Multiplayer Server ~ 01 Jan 2012
// Copied from rorserver and modified to use `Ogre::DataStream` instead of `FILE*` ~ ohlidalp, 2026-07-17
class ScriptFileSafe {
public:
    ScriptFileSafe();

    void AddRef() const;

    void Release() const;

    // TODO: Implement the "r+", "w+" and "a+" modes
    // mode = "r" -> open the file for reading
    //        "w" -> open the file for writing (overwrites existing file)
    //        "a" -> open the file for appending
    int Open(const std::string &filename, const std::string &mode);

    int Close();

    int GetSize() const;

    bool IsEOF() const;

    // Reading
    int ReadString(unsigned int length, std::string &str);

    int ReadLine(std::string &str);

    AngelScript::asINT64 ReadInt(AngelScript::asUINT bytes);

    AngelScript::asQWORD ReadUInt(AngelScript::asUINT bytes);

    float ReadFloat();

    double ReadDouble();

    // Writing
    int WriteString(const std::string &str);

    int WriteInt(AngelScript::asINT64 v, AngelScript::asUINT bytes);

    int WriteUInt(AngelScript::asQWORD v, AngelScript::asUINT bytes);

    int WriteFloat(float v);

    int WriteDouble(double v);

    // Cursor
    int GetPos() const;

    int SetPos(int pos);

    int MovePos(int delta);

protected:
    ~ScriptFileSafe();

    mutable int refCount;
    Ogre::DataStreamPtr m_stream;
};

void RegisterScriptFile_Native(AngelScript::asIScriptEngine *engine);

} // namespace RoR

#endif // USE_ANGELSCRIPT