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

/** 
    @file
    @author Petr Ohlidal
    @date   12/2013
    @brief  .truck format validator

    TERMINOLOGY:
        Module = An optional section of .truck file.
        Implicit module = The default, required module.
        Configuration = a set of modules the player chose.
*/

#pragma once

#include "TruckFileFormat.h"

#include <memory>
#include <OgreString.h>

namespace RoR {
namespace Truck {

/**
* Performs a formal validation of the file (missing required parts, conflicts of modules, etc...)
*/
class Validator
{
public:
    
    struct Message // TODO: remove, use console API directly
    {
        enum Type
        {
            TYPE_INFO,
            TYPE_WARNING,
            TYPE_ERROR,
            TYPE_FATAL_ERROR,

            TYPE_INVALID = 0xFFFFFFFF
        };
    };

    /**
    * Prepares the validation.
    */
    void Setup(Truck::DocumentPtr truck);


    void AddModule(Truck::ModulePtr modul);

    bool Validate();

    void SetCheckBeams(bool check_beams)
    {
        m_check_beams = check_beams;
    }

private:



    void AddMessage(Validator::Message::Type type, Ogre::String const & text);

/* -------------------------------------------------------------------------- */
/* Individual section checkers.                                               */
/* -------------------------------------------------------------------------- */	

    bool CheckShock2(Truck::Shock2 & shock2);

    bool CheckShock3(Truck::Shock3 & shock3);

    bool CheckAnimator(Truck::Animator & def);

    bool CheckCommand(Truck::Command2 & def);

    bool CheckTrigger(Truck::Trigger & def);

    /**
    * Section 'videocamera'.
    */
    bool CheckVideoCamera(Truck::VideoCamera & def);

    bool CheckFlare2(Truck::Flare2 & def);

/* -------------------------------------------------------------------------- */
/* Properties                                                                 */
/* -------------------------------------------------------------------------- */

    Truck::DocumentPtr m_truck;
    std::list<Truck::ModulePtr> m_selected_modules;
    bool m_check_beams;

};

} // namespace Truck
} // namespace RoR
