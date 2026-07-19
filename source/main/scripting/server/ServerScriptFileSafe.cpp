
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

#include "ServerScriptFileSafe.h"

// RIGSOFRODS: from rorserver's 'ScriptFileSafe.cpp'

#ifdef USE_ANGELSCRIPT

using namespace RoR; // RIGSOFRODS
using namespace AngelScript;

ScriptFileSafe *ScriptFile_Factory() {
    return new ScriptFileSafe();
}

void RoR::RegisterScriptFile_Native(asIScriptEngine *engine) {
    int r;

    r = engine->RegisterObjectType("file", 0, asOBJ_REF);
    assert(r >= 0);
    r = engine->RegisterObjectBehaviour("file", asBEHAVE_FACTORY, "file @f()", asFUNCTION(ScriptFile_Factory),
                                        asCALL_CDECL);
    assert(r >= 0);
    r = engine->RegisterObjectBehaviour("file", asBEHAVE_ADDREF, "void f()", asMETHOD(ScriptFileSafe, AddRef),
                                        asCALL_THISCALL);
    assert(r >= 0);
    r = engine->RegisterObjectBehaviour("file", asBEHAVE_RELEASE, "void f()", asMETHOD(ScriptFileSafe, Release),
                                        asCALL_THISCALL);
    assert(r >= 0);

    r = engine->RegisterObjectMethod("file", "int open(const string &in, const string &in)",
                                     asMETHOD(ScriptFileSafe, Open), asCALL_THISCALL);
    assert(r >= 0);
    r = engine->RegisterObjectMethod("file", "int close()", asMETHOD(ScriptFileSafe, Close), asCALL_THISCALL);
    assert(r >= 0);
    r = engine->RegisterObjectMethod("file", "int getSize() const", asMETHOD(ScriptFileSafe, GetSize), asCALL_THISCALL);
    assert(r >= 0);
    r = engine->RegisterObjectMethod("file", "bool isEndOfFile() const", asMETHOD(ScriptFileSafe, IsEOF),
                                     asCALL_THISCALL);
    assert(r >= 0);
    r = engine->RegisterObjectMethod("file", "int readString(uint, string &out)", asMETHOD(ScriptFileSafe, ReadString),
                                     asCALL_THISCALL);
    assert(r >= 0);
    r = engine->RegisterObjectMethod("file", "int readLine(string &out)", asMETHOD(ScriptFileSafe, ReadLine),
                                     asCALL_THISCALL);
    assert(r >= 0);
    r = engine->RegisterObjectMethod("file", "int64 readInt(uint)", asMETHOD(ScriptFileSafe, ReadInt), asCALL_THISCALL);
    assert(r >= 0);
    r = engine->RegisterObjectMethod("file", "uint64 readUInt(uint)", asMETHOD(ScriptFileSafe, ReadUInt),
                                     asCALL_THISCALL);
    assert(r >= 0);
    r = engine->RegisterObjectMethod("file", "float readFloat()", asMETHOD(ScriptFileSafe, ReadFloat), asCALL_THISCALL);
    assert(r >= 0);
    r = engine->RegisterObjectMethod("file", "double readDouble()", asMETHOD(ScriptFileSafe, ReadDouble),
                                     asCALL_THISCALL);
    assert(r >= 0);

    r = engine->RegisterObjectMethod("file", "int writeString(const string &in)", asMETHOD(ScriptFileSafe, WriteString),
                                     asCALL_THISCALL);
    assert(r >= 0);
    r = engine->RegisterObjectMethod("file", "int writeInt(int64, uint)", asMETHOD(ScriptFileSafe, WriteInt),
                                     asCALL_THISCALL);
    assert(r >= 0);
    r = engine->RegisterObjectMethod("file", "int writeUInt(uint64, uint)", asMETHOD(ScriptFileSafe, WriteUInt),
                                     asCALL_THISCALL);
    assert(r >= 0);
    r = engine->RegisterObjectMethod("file", "int writeFloat(float)", asMETHOD(ScriptFileSafe, WriteFloat),
                                     asCALL_THISCALL);
    assert(r >= 0);
    r = engine->RegisterObjectMethod("file", "int writeDouble(double)", asMETHOD(ScriptFileSafe, WriteDouble),
                                     asCALL_THISCALL);
    assert(r >= 0);

    r = engine->RegisterObjectMethod("file", "int getPos() const", asMETHOD(ScriptFileSafe, GetPos), asCALL_THISCALL);
    assert(r >= 0);
    r = engine->RegisterObjectMethod("file", "int setPos(int)", asMETHOD(ScriptFileSafe, SetPos), asCALL_THISCALL);
    assert(r >= 0);
    r = engine->RegisterObjectMethod("file", "int movePos(int)", asMETHOD(ScriptFileSafe, MovePos), asCALL_THISCALL);
    assert(r >= 0);

    assert(r >= 0);
}


ScriptFileSafe::ScriptFileSafe() {
    refCount = 1;
}

ScriptFileSafe::~ScriptFileSafe() {
    Close();
}

void ScriptFileSafe::AddRef() const {
    ++refCount;
}

void ScriptFileSafe::Release() const {
    if (--refCount == 0)
        delete this;
}

int ScriptFileSafe::Open(const std::string &filename, const std::string &mode) {
    // Close the previously opened file handle
    if (m_stream)
        m_stream->close();

    // Validate the mode
    std::string m;
#if AS_WRITE_OPS == 1
    if (mode != "r" && mode != "w" && mode != "a")
#else
        if( mode != "r" )
#endif
        return -2;
    else
        m = mode;

    // By default windows translates "\r\n" to "\n", but we want to read the file as-is.
    m += "b";

    std::string myFilename = filename;

    // Remove the possible .asdata extension
    if (myFilename.length() > 7 && myFilename.substr(myFilename.length() - 7, 7) == ".asdata")
        myFilename = myFilename.substr(0, myFilename.length() - 7);

    // Replace all forbidden characters in the filename	
    std::string allowedChars = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_-";
    for (std::string::iterator it = myFilename.begin(); it < myFilename.end(); ++it) {
        if (allowedChars.find(*it) == std::string::npos)
            *it = '_';
    }

    // Open the file
    try
    {
        m_stream = Ogre::ResourceGroupManager::getSingleton().openResource(myFilename, RGN_SERVER_SCRIPTS);
    }
    catch (...)
    {
        HandleGenericException("file.open()");
        return -3;
    }

    return 0;
}

int ScriptFileSafe::Close() {
    if (!m_stream)
        return -1;

    m_stream->close();
    return 0;
}

int ScriptFileSafe::GetSize() const {
    if (!m_stream)
        return -1;

    return m_stream->size();
}

int ScriptFileSafe::GetPos() const {
    if (!m_stream)
        return -1;

    return m_stream->tell();
}

int ScriptFileSafe::SetPos(int pos) {
    if (!m_stream)
        return -1;

    m_stream->seek(pos);

    return 0;
}

int ScriptFileSafe::MovePos(int delta) {
    if (!m_stream)
        return -1;

    m_stream->seek(m_stream->tell() + delta);

    return 0;
}

int ScriptFileSafe::ReadString(unsigned int length, std::string &str) {
    if (!m_stream)
        return 0;

    // Read the string
    str.resize(length);
    size_t size = (int)m_stream->read(&str[0], length);
    str.resize(size);

    return static_cast<int>(size);
}

int ScriptFileSafe::ReadLine(std::string &str) {
    if (!m_stream)
        return 0;

    char buf[2000] = {};
    size_t count = m_stream->readLine(buf, 2000);
    str.assign(buf, count);

    return static_cast<int>(count);
}

asINT64 ScriptFileSafe::ReadInt(asUINT bytes) {
    if (!m_stream)
        return 0;

    if (bytes > 8) bytes = 8;
    if (bytes == 0) return 0;

    unsigned char buf[8];
    size_t r = m_stream->read(buf, bytes);
    if (r == 0) return 0;

    asINT64 val = 0;

        unsigned int n = 0;
        for (; n < bytes; n++)
            val |= asQWORD(buf[n]) << (n * 8);
        if (buf[0] & 0x80)
            for (; n < 8; n++)
                val |= asQWORD(0xFF) << (n * 8);


    return val;
}

asQWORD ScriptFileSafe::ReadUInt(asUINT bytes) {
    if (!m_stream)
        return 0;

    if (bytes > 8) bytes = 8;
    if (bytes == 0) return 0;

    unsigned char buf[8];
    size_t r = m_stream->read(buf, bytes);
    if (r == 0) return 0;

    asQWORD val = 0;

        unsigned int n = 0;
        for (; n < bytes; n++)
            val |= asQWORD(buf[n]) << (n * 8);


    return val;
}

float ScriptFileSafe::ReadFloat() {
    if (!m_stream)
        return 0;

    unsigned char buf[4];
    size_t r = m_stream->read(buf, 4);
    if (r == 0) return 0;

    asUINT val = 0;

        unsigned int n = 0;
        for (; n < 4; n++)
            val |= asUINT(buf[n]) << (n * 8);


    return *reinterpret_cast<float *>(&val);
}

double ScriptFileSafe::ReadDouble() {
    if (!m_stream)
        return 0;

    unsigned char buf[8];
    size_t r = m_stream->read(buf, 8);
    if (r == 0) return 0;

    asQWORD val = 0;

        unsigned int n = 0;
        for (; n < 8; n++)
            val |= asQWORD(buf[n]) << (n * 8);


    return *reinterpret_cast<double *>(&val);
}

bool ScriptFileSafe::IsEOF() const {
    if (!m_stream)
        return true;

    return m_stream->eof();
}

int ScriptFileSafe::WriteString(const std::string &str) {
    if (!m_stream)
        return -1;

    // Write the entire string
    size_t r = m_stream->write(&str[0], str.length());
    return int(r);
}

int ScriptFileSafe::WriteInt(asINT64 val, asUINT bytes) {
    if (!m_stream)
        return 0;

    unsigned char buf[8];

        for (unsigned int n = 0; n < bytes; n++)
            buf[n] = (val >> (n * 8)) & 0xFF;


    size_t r = m_stream->write(&buf, bytes);
    return int(r);
}

int ScriptFileSafe::WriteUInt(asQWORD val, asUINT bytes) {
    if (!m_stream)
        return 0;

    unsigned char buf[8];

        for (unsigned int n = 0; n < bytes; n++)
            buf[n] = (val >> (n * 8)) & 0xFF;


    size_t r = m_stream->write(&buf, bytes);
    return int(r);
}

int ScriptFileSafe::WriteFloat(float f) {
    if (!m_stream)
        return 0;

    unsigned char buf[4];
    asUINT val = *reinterpret_cast<asUINT *>(&f);

        for (unsigned int n = 0; n < 4; n++)
            buf[n] = (val >> (n * 8)) & 0xFF;
    

    size_t r = m_stream->write(&buf, 4);
    return int(r);
}

int ScriptFileSafe::WriteDouble(double d) {
    if (!m_stream)
        return 0;

    unsigned char buf[8];
    asQWORD val = *reinterpret_cast<asQWORD *>(&d);
    
        for (unsigned int n = 0; n < 8; n++)
            buf[n] = (val >> (n * 8)) & 0xFF;
    

    size_t r = m_stream->write(&buf, 8);
    return int(r);
}

#endif //USE_ANGELSCRIPT