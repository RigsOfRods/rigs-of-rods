/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2013-2017 Petr Ohlidal & contributors

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

/** 
    @file   RigDef_Validator.h
    @author Petr Ohlidal
    @date   12/2013
    @brief  .truck format validator

    TERMINOLOGY:
        Module = An optional section of .truck file.
        Root module = The default, required module.
        Configuration = a set of modules the player chose.
*/

#pragma once

#include "RigDef_File.h"

#include <memory>
#include <OgreString.h>

namespace RigDef
{

/**
* Performs a formal validation of the file (missing required parts, conflicts of modules, etc...)
*/
class Validator
{
public:
    
    struct Message
    {
        enum Type
        {
            TYPE_INFO,
            TYPE_WARNING,
            TYPE_ERROR,
            TYPE_FATAL_ERROR,

            TYPE_INVALID = 0xFFFFFFFF
        };

        Message(Type type, Ogre::String const & message):
            type(type),
            text(message)
        {}

        Type type;
        Ogre::String text;
    };

    /**
    * Prepares the validation.
    */
    void Setup(std::shared_ptr<RigDef::File> file);

    /**
    * Adds a vehicle module to the validated configuration.
    * @param module_name A module from the validated rig-def file.
    */
    bool AddModule(Ogre::String const & module_name);

    bool Validate();

    std::list<Message> & GetMessages()
    {
        return m_messages;
    }

    void SetCheckBeams(bool check_beams)
    {
        m_check_beams = check_beams;
    }

    int GetMessagesNumErrors()   const { return m_messages_num_errors;   }
    int GetMessagesNumWarnings() const { return m_messages_num_warnings; }
    int GetMessagesNumOther()    const { return m_messages_num_other;    }

    std::string ProcessMessagesToString();

private:

    /**
    * Finds section in configuration and performs checks.
    * @param unique Is this section required to be unique?
    * @param required Is this section required?
    * @return True if all conditions were met.
    */
    bool CheckSection(RigDef::File::Keyword keyword, bool unique, bool required);

    /**
    * Checks if a module contains a section.
    */
    bool HasModuleKeyword(std::shared_ptr<RigDef::File::Module> module, RigDef::File::Keyword keyword);

    /**
    * Inline-ection 'submesh_groundmodel', unique across all modules.
    */
    bool CheckSectionSubmeshGroundmodel();

    /**
    * Checks there's at least 1 forward gear.
    */
    bool CheckGearbox();

    void AddMessage(Validator::Message::Type type, Ogre::String const & text);

/* -------------------------------------------------------------------------- */
/* Individual section checkers.                                               */
/* -------------------------------------------------------------------------- */	

    bool CheckShock2(RigDef::Shock2 & shock2);

    bool CheckAnimator(RigDef::Animator & def);

    bool CheckCommand(RigDef::Command2 & def);

    bool CheckTrigger(RigDef::Trigger & def);

    /**
    * Section 'videocamera'.
    */
    bool CheckVideoCamera(RigDef::VideoCamera & def);

    bool CheckFlare2(RigDef::Flare2 & def);

/* -------------------------------------------------------------------------- */
/* Properties                                                                 */
/* -------------------------------------------------------------------------- */

    std::list<Message> m_messages;
    int m_messages_num_errors;
    int m_messages_num_warnings;
    int m_messages_num_other;
    std::shared_ptr<RigDef::File> m_file; //!< The parsed input file.
    std::list<std::shared_ptr<RigDef::File::Module>> m_selected_modules;
    bool m_check_beams;

};

} // namespace RigDef
