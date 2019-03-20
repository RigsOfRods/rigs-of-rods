/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2013-2019 Petr Ohlidal & contributors

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

#include "SkinManager.h"

#include "Application.h"
#include "OgreSubsystem.h"
#include "Utils.h"

#include <OgreEntity.h>
#include <OgreMaterialManager.h>
#include <OgrePass.h>
#include <OgreSubEntity.h>
#include <OgreTechnique.h>

std::vector<std::shared_ptr<RoR::SkinDef>> RoR::SkinParser::ParseSkins(Ogre::DataStreamPtr& stream)
{
    std::vector<std::shared_ptr<RoR::SkinDef>> result;
    std::unique_ptr<RoR::SkinDef> curr_skin;
    try
    {
        while(!stream->eof())
        {
            std::string line = RoR::Utils::SanitizeUtf8String(stream->getLine());

            // Ignore blanks & comments
            if (!line.length() || line.substr(0, 2) == "//")
            {
                continue;
            }

            if (!curr_skin)
            {
                // No current skin -- So first valid data should be skin name
                Ogre::StringUtil::trim(line);
                curr_skin = std::unique_ptr<SkinDef>(new SkinDef);
                curr_skin->name = line;
                stream->skipLine("{");
            }
            else
            {
                // Already in skin
                if (line == "}")
                {
                    result.push_back(std::shared_ptr<SkinDef>(curr_skin.release())); // Finished
                }
                else
                {
                    RoR::SkinParser::ParseSkinAttribute(line, curr_skin.get());
                }
            }
        }
    }
    catch (Ogre::Exception& e)
    {
        RoR::LogFormat("[RoR] Error parsing skin file '%s', message: %s",
            stream->getName().c_str(), e.getFullDescription().c_str());
    }
    return result;
}

void RoR::SkinParser::ParseSkinAttribute(const std::string& line, SkinDef* skin_def) // static
{
    Ogre::StringVector params = Ogre::StringUtil::split(line, "\t=,;\n");
    for (unsigned int i=0; i < params.size(); i++)
    {
        Ogre::StringUtil::trim(params[i]);
    }
    Ogre::String& attrib = params[0];
    Ogre::StringUtil::toLowerCase(attrib);

    if (attrib == "replacetexture"  && params.size() == 3) { skin_def->replace_textures.insert(std::make_pair(params[1], params[2])); return; }
    if (attrib == "replacematerial" && params.size() == 3) { skin_def->replace_materials.insert(std::make_pair(params[1], params[2])); return; }
    if (attrib == "preview"         && params.size() >= 2) { skin_def->thumbnail = params[1]; return; }
    if (attrib == "description"     && params.size() >= 2) { skin_def->description = params[1]; return; }
    if (attrib == "authorname"      && params.size() >= 2) { skin_def->author_name = params[1]; return; }
    if (attrib == "authorid"        && params.size() == 2) { skin_def->author_id = PARSEINT(params[1]); return; }
    if (attrib == "guid"            && params.size() >= 2) { skin_def->guid = params[1]; Ogre::StringUtil::trim(skin_def->guid); Ogre::StringUtil::toLowerCase(skin_def->guid); return; }
    if (attrib == "name"            && params.size() >= 2) { skin_def->name = params[1]; Ogre::StringUtil::trim(skin_def->name); return; }
}
