/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2013-2023 Petr Ohlidal

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

#include "TuneupFileFormat.h"

#include "Application.h"
#include "Console.h"
#include "Utils.h"

#include <OgreEntity.h>
#include <OgreMaterialManager.h>
#include <OgrePass.h>
#include <OgreSubEntity.h>
#include <OgreTechnique.h>

using namespace RoR;

TuneupDefPtr TuneupDef::clone()
{
    TuneupDefPtr ret = new TuneupDef();
    
    ret->use_addonparts     =     this->use_addonparts    ; //std::set<std::string> 
    ret->remove_props       =     this->remove_props      ; //std::set<std::string> 
    ret->remove_flexbodies  =     this->remove_flexbodies ; //std::set<std::string> 
    ret->name               =     this->name              ; //std::string           
    ret->guid               =     this->guid              ; //std::string           
    ret->thumbnail          =     this->thumbnail         ; //std::string           
    ret->description        =     this->description       ; //std::string           
    ret->author_name        =     this->author_name       ; //std::string           
    ret->author_id          =     this->author_id         ; //int                   
    ret->category_id        =     this->category_id       ; //CacheCategoryId   

    return ret;
}

std::vector<TuneupDefPtr> RoR::TuneupParser::ParseTuneups(Ogre::DataStreamPtr& stream)
{
    std::vector<TuneupDefPtr> result;
    TuneupDefPtr curr_tuneup;
    try
    {
        while(!stream->eof())
        {
            std::string line = SanitizeUtf8String(stream->getLine());

            // Ignore blanks & comments
            if (!line.length() || line.substr(0, 2) == "//")
            {
                continue;
            }

            if (!curr_tuneup)
            {
                // No current tuneup -- So first valid data should be tuneup name
                Ogre::StringUtil::trim(line);
                curr_tuneup = new TuneupDef();
                curr_tuneup->name = line;
                stream->skipLine("{");
            }
            else
            {
                // Already in tuneup
                if (line == "}")
                {
                    result.push_back(curr_tuneup); // Finished
                    curr_tuneup = nullptr;
                }
                else
                {
                    RoR::TuneupParser::ParseTuneupAttribute(line, curr_tuneup);
                }
            }
        }

        if (curr_tuneup)
        {
            App::GetConsole()->putMessage(
                Console::CONSOLE_MSGTYPE_ACTOR, Console::CONSOLE_SYSTEM_WARNING,
                fmt::format("Tuneup '{}' in file '{}' not properly closed with '}}'",
                    curr_tuneup->name, stream->getName()));
            result.push_back(curr_tuneup); // Submit anyway
            curr_tuneup = nullptr;
        }
    }
    catch (Ogre::Exception& e)
    {
        App::GetConsole()->putMessage(
            Console::CONSOLE_MSGTYPE_ACTOR, Console::CONSOLE_SYSTEM_WARNING,
            fmt::format("Error parsing tuneup file '{}', message: {}",
                stream->getName(), e.getFullDescription()));
    }
    return result;
}

void RoR::TuneupParser::ParseTuneupAttribute(const std::string& line, TuneupDefPtr& tuneup_def) // static
{
    Ogre::StringVector params = Ogre::StringUtil::split(line, "\t=,;\n");
    for (unsigned int i=0; i < params.size(); i++)
    {
        Ogre::StringUtil::trim(params[i]);
    }
    Ogre::String& attrib = params[0];
    Ogre::StringUtil::toLowerCase(attrib);

    if (attrib == "use_addonpart"   && params.size() == 2) { tuneup_def->use_addonparts.insert(params[1]); return; }
    if (attrib == "remove_prop"     && params.size() == 2) { tuneup_def->remove_props.insert(params[1]); return; }
    if (attrib == "remove_flexbody" && params.size() == 2) { tuneup_def->remove_flexbodies.insert(params[1]); return; }
    if (attrib == "preview"         && params.size() >= 2) { tuneup_def->thumbnail = params[1]; return; }
    if (attrib == "description"     && params.size() >= 2) { tuneup_def->description = params[1]; return; }
    if (attrib == "author_name"     && params.size() >= 2) { tuneup_def->author_name = params[1]; return; }
    if (attrib == "author_id"       && params.size() == 2) { tuneup_def->author_id = PARSEINT(params[1]); return; }
    if (attrib == "category_id"     && params.size() == 2) { tuneup_def->category_id = (CacheCategoryId)PARSEINT(params[1]); return; }
    if (attrib == "guid"            && params.size() >= 2) { tuneup_def->guid = params[1]; Ogre::StringUtil::trim(tuneup_def->guid); Ogre::StringUtil::toLowerCase(tuneup_def->guid); return; }
    if (attrib == "name"            && params.size() >= 2) { tuneup_def->name = params[1]; Ogre::StringUtil::trim(tuneup_def->name); return; }
}

void RoR::TuneupParser::ExportTuneup(Ogre::DataStreamPtr& stream, TuneupDefPtr& tuneup)
{
    Str<2000> buf;
    buf << tuneup->name << "\n";
    buf << "{\n";
    buf << "\tpreview = "     << tuneup->thumbnail    << "\n";
    buf << "\tdescription = " << tuneup->description  << "\n";
    buf << "\tauthor_name = " << tuneup->author_name  << "\n";
    buf << "\tauthor_id = "   << tuneup->author_id    << "\n";
    buf << "\tcategory_id = " << (int)tuneup->category_id  << "\n";
    buf << "\tguid = "        << tuneup->guid         << "\n";
    buf << "\n";
    for (const std::string& addonpart: tuneup->use_addonparts)
    {
        buf << "\tuse_addonpart = " << addonpart << "\n";
    }
    for (const std::string& remove_prop: tuneup->remove_props)
    {
        buf << "\tremove_prop = " << remove_prop << "\n";
    }
    for (const std::string& remove_flexbody: tuneup->remove_flexbodies)
    {
        buf << "\tremove_flexbody = " << remove_flexbody << "\n";
    }
    buf << "}\n\n";

    size_t written = stream->write(buf.GetBuffer(), buf.GetLength());
    if (written < buf.GetLength())
    {
        App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_WARNING,
            fmt::format("Error writing file '{}': only written {}/{} bytes", stream->getName(), written, buf.GetLength()));
    }
}
