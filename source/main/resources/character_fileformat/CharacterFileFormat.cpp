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

#include "Actor.h"
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
    m_ctx.action = CharacterActionDef();
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

static ForceAnimBlend ParseForceAnimBlend(std::string const& line)
{
    if (line == "none") return ForceAnimBlend::NONE;
    if (line == "average") return ForceAnimBlend::AVERAGE;
    if (line == "cumulative") return ForceAnimBlend::CUMULATIVE;
    return ForceAnimBlend::NONE;
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
    if (!m_ctx.in_action && !m_ctx.in_bone_blend_mask)
    {
        // Root level

        if (StartsWith(m_cur_line, "character_name"))
        {
            m_def->character_name = GetParam(1);
        }
        if (StartsWith(m_cur_line, "character_description"))
        {
            m_def->character_description = GetParam(1);
        }
        else if (StartsWith(m_cur_line, "mesh_name"))
        {
            m_def->mesh_name = GetParam(1);
        }
        else if (StartsWith(m_cur_line, "character_guid"))
        {
            m_def->character_guid = GetParam(1);
        }
        else if (StartsWith(m_cur_line, "mesh_scale"))
        {
            if (m_cur_args.size() > 1) m_def->mesh_scale.x = Ogre::StringConverter::parseReal(GetParam(1));
            if (m_cur_args.size() > 2) m_def->mesh_scale.y = Ogre::StringConverter::parseReal(GetParam(2));
            if (m_cur_args.size() > 3) m_def->mesh_scale.z = Ogre::StringConverter::parseReal(GetParam(3));
        }
        else if (StartsWith(m_cur_line, "force_animblend"))
        {
            m_def->force_animblend = ParseForceAnimBlend(GetParam(1));
        }
        else if (StartsWith(m_cur_line, "begin_action"))
        {
            m_ctx.in_action = true;
        }
        else if (StartsWith(m_cur_line, "begin_bone_blend_mask"))
        {
            m_ctx.in_bone_blend_mask = true;
        }
        else if (StartsWith(m_cur_line, "author"))
        {
            CharacterAuthorInfo author;
            author.type = GetParam(1);
            author.id = Ogre::StringConverter::parseInt(GetParam(2));
            author.name = GetParam(3);
            author.email = GetParam(4);
            m_def->authors.push_back(author);
        }
    }
    else if (m_ctx.in_action)
    {
        // In '[begin/end]_action' block.

        if (StartsWith(m_cur_line, "end_action"))
        {
            m_ctx.action.action_id = static_cast<CharacterActionID_t>(m_def->actions.size());
            m_def->actions.push_back(m_ctx.action);
            m_ctx.action = CharacterActionDef();
            m_ctx.in_action = false;
        }
        else if (StartsWith(m_cur_line, "anim_name"))
        {
            m_ctx.action.anim_name = GetParam(1);
        }
        else if (StartsWith(m_cur_line, "action_description"))
        {
            m_ctx.action.action_description = GetParam(1);
        }
        else if (StartsWith(m_cur_line, "for_situation"))
        {
            BITMASK_SET_1(m_ctx.action.for_situations, Character::SituationFlagFromString(GetParam(1)));
        }
        else if (StartsWith(m_cur_line, "for_control"))
        {
            BITMASK_SET_1(m_ctx.action.for_controls, Character::ControlFlagFromString(GetParam(1)));
        }
        else if (StartsWith(m_cur_line, "except_situation"))
        {
            BITMASK_SET_1(m_ctx.action.except_situations, Character::SituationFlagFromString(GetParam(1)));
        }
        else if (StartsWith(m_cur_line, "except_control"))
        {
            BITMASK_SET_1(m_ctx.action.except_controls, Character::ControlFlagFromString(GetParam(1)));
        }
        else if (StartsWith(m_cur_line, "playback_time_ratio"))
        {
            m_ctx.action.playback_time_ratio = Ogre::StringConverter::parseReal(GetParam(1));
        }
        else if (StartsWith(m_cur_line, "playback_h_speed_ratio"))
        {
            m_ctx.action.playback_h_speed_ratio = Ogre::StringConverter::parseReal(GetParam(1));
        }
        else if (StartsWith(m_cur_line, "playback_steering_ratio"))
        {
            m_ctx.action.playback_steering_ratio = Ogre::StringConverter::parseReal(GetParam(1));
        }
        else if (StartsWith(m_cur_line, "weight"))
        {
            m_ctx.action.weight = Ogre::StringConverter::parseReal(GetParam(1));
        }
        else if (StartsWith(m_cur_line, "playback_trim"))
        {
            m_ctx.action.playback_trim = Ogre::StringConverter::parseReal(GetParam(1));
        }
        else if (StartsWith(m_cur_line, "anim_continuous"))
        {
            m_ctx.action.anim_continuous = Ogre::StringConverter::parseBool(GetParam(1));
        }
        else if (StartsWith(m_cur_line, "anim_autorestart"))
        {
            m_ctx.action.anim_autorestart = Ogre::StringConverter::parseBool(GetParam(1));
        }
        else if (StartsWith(m_cur_line, "anim_neutral_mid"))
        {
            m_ctx.action.anim_neutral_mid = Ogre::StringConverter::parseBool(GetParam(1));
        }
        else if (StartsWith(m_cur_line, "source_percentual"))
        {
            m_ctx.action.source_percentual = Ogre::StringConverter::parseBool(GetParam(1));
        }
    }
    else if (m_ctx.in_bone_blend_mask)
    {
        // In '[begin/end]_bone_blend_mask' block.

        if (StartsWith(m_cur_line, "end_bone_blend_mask"))
        {
            m_def->bone_blend_masks.push_back(m_ctx.bone_blend_mask);
            m_ctx.bone_blend_mask = BoneBlendMaskDef();
            m_ctx.in_bone_blend_mask = false;
        }
        else if (StartsWith(m_cur_line, "anim_name"))
        {
            m_ctx.bone_blend_mask.anim_name = GetParam(1);
        }
        else if (StartsWith(m_cur_line, "bone_weight"))
        {
            BoneBlendMaskWeightDef def;
            def.bone_name = GetParam(1);
            def.bone_weight = Ogre::StringConverter::parseReal(GetParam(2));
            m_ctx.bone_blend_mask.bone_weights.push_back(def);
        }
    }
}


