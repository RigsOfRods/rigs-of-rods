/*
    This source file is part of Rigs of Rods
    Copyright 2022 Petr Ohlidal

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

#include "CharacterFileFormat.h"

#include "Character.h"
#include "Console.h"
#include "Utils.h"

using namespace RoR;
using namespace Ogre;

const int CHA_LINE_BUF_LEN = 4000;

CharacterDocumentPtr CharacterParser::ProcessOgreStream(Ogre::DataStreamPtr stream)
{
    char raw_line_buf[CHA_LINE_BUF_LEN];
    m_filename = stream->getName();
    m_ctx.anim = CharacterAnimDef();
    m_def = std::make_shared<CharacterDocument>();
    while (!stream->eof())
    {
        stream->readLine(raw_line_buf, CHA_LINE_BUF_LEN);
        this->ProcessLine(raw_line_buf);
    }
    return m_def;
}

void CharacterParser::ProcessLine(const char* line)
{
    if ((line != nullptr) && (line[0] != 0))
    {
        m_cur_line = line;
        Ogre::StringUtil::trim(m_cur_line);
        this->TokenizeCurrentLine();
        this->ProcessCurrentLine();
    }
    m_line_number++;
}

inline bool StartsWith(std::string const& line, const char* test)
{
    return line.compare(0, strlen(test), test) == 0;
}

std::string CharacterParser::GetParam(int pos)
{
    if (pos < m_cur_args.size())
        return m_cur_args[pos];
    else
        return "";
}

void CharacterParser::TokenizeCurrentLine()
{
    // Recognizes quoted strings!
    // --------------------------
    m_cur_args.clear();
    m_cur_args.push_back("");

    bool in_str = false;
    for (char c : m_cur_line)
    {
        if (in_str)
        {
            if (c == '"')
            {
                in_str = false;
            }
            else
            {
                m_cur_args[m_cur_args.size() - 1] += c;
            }
        }
        else
        {
            if (c == ' ' || c == '\t') // delimiters
            {
                if (m_cur_args[m_cur_args.size() - 1] != "")
                {
                    m_cur_args.push_back("");
                }
            }
            else if (c == '"')
            {
                in_str = true;
            }
            else
            {
                m_cur_args[m_cur_args.size() - 1] += c;
            }
        }
    }
}

// retval true = continue processing (false = stop)
void CharacterParser::ProcessCurrentLine()
{
    if (!m_ctx.in_anim)
    {
        if (StartsWith(m_cur_line, "character_name"))
        {
            m_def->character_name = GetParam(1);
        }
        else if (StartsWith(m_cur_line, "mesh_name"))
        {
            m_def->mesh_name = GetParam(1);
        }
        else if (StartsWith(m_cur_line, "begin_animation"))
        {
            m_ctx.in_anim = true;
        }
    }
    else
    {
        if (StartsWith(m_cur_line, "end_animation"))
        {
            m_ctx.anim.game_id = (int)m_def->anims.size();
            m_def->anims.push_back(m_ctx.anim);
            m_ctx.anim = CharacterAnimDef();
            m_ctx.in_anim = false;
        }
        else if (StartsWith(m_cur_line, "anim_name"))
        {
            m_ctx.anim.anim_name = GetParam(1);
        }
        else if (StartsWith(m_cur_line, "game_description"))
        {
            m_ctx.anim.game_description = GetParam(1);
        }
        else if (StartsWith(m_cur_line, "for_situation"))
        {
            BITMASK_SET_1(m_ctx.anim.for_situations, Character::SituationFlagFromString(GetParam(1)));
        }
        else if (StartsWith(m_cur_line, "for_action"))
        {
            BITMASK_SET_1(m_ctx.anim.for_actions, Character::ActionFlagFromString(GetParam(1)));
        }
        else if (StartsWith(m_cur_line, "except_situation"))
        {
            BITMASK_SET_1(m_ctx.anim.except_situations, Character::SituationFlagFromString(GetParam(1)));
        }
        else if (StartsWith(m_cur_line, "except_action"))
        {
            BITMASK_SET_1(m_ctx.anim.except_actions, Character::ActionFlagFromString(GetParam(1)));
        }
        else if (StartsWith(m_cur_line, "playback_time_ratio"))
        {
            m_ctx.anim.playback_time_ratio = Ogre::StringConverter::parseReal(GetParam(1));
        }
        else if (StartsWith(m_cur_line, "playback_h_speed_ratio"))
        {
            m_ctx.anim.playback_h_speed_ratio = Ogre::StringConverter::parseReal(GetParam(1));
        }
        else if (StartsWith(m_cur_line, "playback_steering_ratio"))
        {
            m_ctx.anim.playback_steering_ratio = Ogre::StringConverter::parseReal(GetParam(1));
        }
        else if (StartsWith(m_cur_line, "weight"))
        {
            m_ctx.anim.weight = Ogre::StringConverter::parseReal(GetParam(1));
        }
        else if (StartsWith(m_cur_line, "playback_trim"))
        {
            m_ctx.anim.playback_trim = Ogre::StringConverter::parseReal(GetParam(1));
        }
        else if (StartsWith(m_cur_line, "anim_continuous"))
        {
            m_ctx.anim.anim_continuous = Ogre::StringConverter::parseBool(GetParam(1));
        }
        else if (StartsWith(m_cur_line, "anim_autorestart"))
        {
            m_ctx.anim.anim_autorestart = Ogre::StringConverter::parseBool(GetParam(1));
        }
        else if (StartsWith(m_cur_line, "anim_neutral_mid"))
        {
            m_ctx.anim.anim_neutral_mid = Ogre::StringConverter::parseBool(GetParam(1));
        }
        else if (StartsWith(m_cur_line, "source_percentual"))
        {
            m_ctx.anim.source_percentual = Ogre::StringConverter::parseBool(GetParam(1));
        }
    }
}


