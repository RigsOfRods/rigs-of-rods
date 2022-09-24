/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2013-2022 Petr Ohlidal

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

#pragma once

#include "BitFlags.h"

#include <memory>
#include <string>
#include <vector>

namespace RoR {

/// @addtogroup Gameplay
/// @{

/// @addtogroup Character
/// @{

struct CharacterAnimDef
{
    std::string anim_name; //!< Name of the skeletal animation from OGRE's *.skeleton file.
    std::string description; //!< Internal game name.

    // Conditions
    BitMask_t for_situations = 0;    //!< Character::SITUATION_, all must be satisfied.
    BitMask_t except_situations = 0; //!< Character::SITUATION_, none must be satisfied.
    BitMask_t for_actions = 0;       //!< Character::ACTION_, all must be satisfied.
    BitMask_t except_actions = 0;    //!< Character::ACTION_, none must be satisfied.

    // Anim position calculation
    float playback_time_ratio = 0.f; //!< How much elapsed time affects animation position.
    float playback_h_speed_ratio = 0.f; //!< How much horizontal movement speed affects animation position.
    float playback_steering_ratio = 0.f; //!< How much vehicle steering angle affects animation position.
    bool  anim_continuous = true; //!< Should animation keep advancing and looping, or should it be set to exact position?
    bool  anim_autorestart = false; //!< Should animation always restart from 0 when activated?
    bool  anim_neutral_mid = false; //!< Does the anim have the 'neutral' position in it's middle (such as steering left/right) instead of at start? Only effective together with "percentual".
    bool  source_percentual = false; //!< Is the position source value a percentage of animation length?
    float playback_trim = 0.0f; //!< How much to trim the animation position. Useful for i.e. steering animation to avoid flickering.

    // Anim blending weight
    float weight = 1.0f;
};

struct CharacterDef
{
    std::string name;
    std::string mesh_name;
    std::vector<CharacterAnimDef> anims;
};

typedef std::shared_ptr<CharacterDef> CharacterDefPtr;

/// @} // addtogroup Character
/// @} // addtogroup Gameplay

} // namespace RoR