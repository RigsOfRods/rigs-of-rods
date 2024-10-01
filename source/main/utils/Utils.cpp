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

#include "Utils.h"

#include "CVar.h"
#include "RoRnet.h"
#include "RoRVersion.h"
#include "SHA1.h"
#include "Application.h"

#include <Ogre.h>
#include <string>

#ifndef _WIN32
#   include <iconv.h>
#endif

#ifdef _WIN32
#   include <windows.h> // Sleep()
#endif

using namespace Ogre;

String RoR::sha1sum(const char *key, int len)
{
    RoR::CSHA1 sha1;
    sha1.UpdateHash((uint8_t *)key, len);
    sha1.Final();
    return sha1.ReportHash();
}

String RoR::HashData(const char *key, int len)
{
    std::stringstream result;
    result << std::hex << FastHash(key, len);
    return result.str();
}

UTFString RoR::tryConvertUTF(const char* buffer)
{
    std::string str_in(buffer);
    return UTFString(SanitizeUtf8String(str_in));
}

UTFString RoR::formatBytes(double bytes)
{
    wchar_t tmp[128] = L"";
    const wchar_t* si_prefix[] = {L"B", L"KB", L"MB", L"GB", L"TB", L"EB", L"ZB", L"YB"};
    int base = 1024;
    int c = bytes > 0 ? std::min((int)(log(bytes) / log((float)base)), (int)sizeof(si_prefix) - 1) : 0;
    swprintf(tmp, 128, L"%1.2f %ls", bytes / pow((float)base, c), si_prefix[c]);
    return UTFString(tmp);
}

std::time_t RoR::getTimeStamp()
{
    return time(NULL); //this will overflow in 2038
}

String RoR::getVersionString(bool multiline)
{
    char tmp[1024] = "";
    if (multiline)
    {
        sprintf(tmp, "Rigs of Rods\n"
            " version: %s\n"
            " protocol version: %s\n"
            " build time: %s, %s\n"
            , ROR_VERSION_STRING, RORNET_VERSION, ROR_BUILD_DATE, ROR_BUILD_TIME);
    }
    else
    {
        sprintf(tmp, "Rigs of Rods version %s, protocol version: %s, build time: %s, %s", ROR_VERSION_STRING, RORNET_VERSION, ROR_BUILD_DATE, ROR_BUILD_TIME);
    }

    return String(tmp);
}

Real RoR::Round(Ogre::Real value, unsigned short ndigits /* = 0 */)
{
    Real f = 1.0f;

    while (ndigits--)
        f = f * 10.0f;

    value *= f;

    if (value >= 0.0f)
        value = std::floor(value + 0.5f);
    else
        value = std::ceil(value - 0.5f);

    value /= f;

    return value;
}

std::string RoR::SanitizeUtf8String(std::string const& str_in)
{
    // Cloned from UTFCPP tutorial: http://utfcpp.sourceforge.net/#fixinvalid
    std::string str_out;
    utf8::replace_invalid(str_in.begin(), str_in.end(), std::back_inserter(str_out));
    return str_out;
}

std::string RoR::SanitizeUtf8CString(const char* start, const char* end /* = nullptr */)
{
    if (end == nullptr)
    {
        end = (start + strlen(start));
    }

    // Cloned from UTFCPP tutorial: http://utfcpp.sourceforge.net/#fixinvalid
    std::string str_out;
    utf8::replace_invalid(start, end, std::back_inserter(str_out));
    return str_out;
}

std::string RoR::Sha1Hash(std::string const & input)
{
    RoR::CSHA1 sha1;
    sha1.UpdateHash((uint8_t *)input.c_str(), (int)input.length());
    sha1.Final();
    return sha1.ReportHash();
}

std::string RoR::JoinStrVec(Ogre::StringVector tokens, const std::string& delim)
{
    Str<500> res;
    for (String& tok : tokens)
    {
        if (res.GetLength() > 0)
            res << delim;
        res << tok;
    }
    return res.ToCStr();
}

bool RoR::IsDistanceWithin(Ogre::Vector3 const& a, Ogre::Vector3 const& b, float max)
{
    return a.squaredDistance(b) <= max * max;
}

void formatVertexDeclInfo(RoR::Str<4000>& text, Ogre::VertexDeclaration* vertexDeclaration, int j)
{
    const VertexElement* ve = vertexDeclaration->getElement(j);
    text << "\n" << "\telement #" << (j) << "/" << (vertexDeclaration->getElementCount());
    text << " binding:" << (ve->getSource());
    text << ", offset:" << (ve->getOffset());
    text << ", type:" << (ve->getType());
    text << ", semantic:" << (ve->getSemantic());
    text << ", size:" << (ve->getSize());
}

std::string RoR::PrintMeshInfo(std::string const& title, MeshPtr mesh)
{
    Str<4000> text;
    text << title;

    if (mesh->sharedVertexData)
    {
        text << "\n" <<("Mesh has Shared Vertices:");
        VertexData* vt=mesh->sharedVertexData;
        for (int j=0; j<(int)vt->vertexDeclaration->getElementCount(); j++)
        {
            formatVertexDeclInfo(text, vt->vertexDeclaration, j);
        }
    }
    text << "\n" <<("Mesh has "+TOSTRING(mesh->getNumSubMeshes())+" submesh(es)");
    for (int i=0; i<mesh->getNumSubMeshes(); i++)
    {
        SubMesh* submesh = mesh->getSubMesh(i);
        text << "\n" <<("SubMesh "+TOSTRING(i)+": uses shared?:"+TOSTRING(submesh->useSharedVertices));
        if (!submesh->useSharedVertices)
        {
            VertexData* vt=submesh->vertexData;
            for (int j=0; j<(int)vt->vertexDeclaration->getElementCount(); j++)
            {
                formatVertexDeclInfo(text, vt->vertexDeclaration, j);
            }
        }
    }

    return text.ToCStr();
}

void RoR::CvarAddFileToList(CVar* cvar, const std::string& filename)
{
    StringVector files = StringUtil::split(cvar->getStr(), ",");
    if (std::find(files.begin(), files.end(), filename) == files.end()) // Is file not in list yet?
    {
        files.push_back(filename);
    }
    cvar->setStr(JoinStrVec(files, ","));
}

void RoR::CvarRemoveFileFromList(CVar* cvar, const std::string& filename)
{
    StringVector files = StringUtil::split(cvar->getStr(), ",");
    auto found = (std::find(files.begin(), files.end(), filename));
    if (found != files.end()) // Was file in list?
    {
        files.erase(found);
    }

    cvar->setStr(JoinStrVec(files, ","));
}

void RoR::SplitBundleQualifiedFilename(const std::string& bundleQualifiedFilename, std::string& out_bundleName, std::string& out_filename)
{
    size_t pos = bundleQualifiedFilename.find(':');
    if (pos != std::string::npos)
    {
        out_bundleName = bundleQualifiedFilename.substr(0, pos);
        out_filename = bundleQualifiedFilename.substr(pos + 1);
    }
    else
    {
        out_bundleName = "";
        out_filename = bundleQualifiedFilename;
    }
}
