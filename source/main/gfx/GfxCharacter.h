/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2017-2022 Petr Ohlidal

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

#include "CharacterFileFormat.h"
#include "ForwardDeclarations.h"
#include "SimBuffers.h"

#include <Ogre.h>
#include <string>

namespace RoR {

/// @addtogroup Gfx
/// @{

/// See `GUI::CharacterPoseUtil`
struct CharacterActionDbg
{
    bool active = false;

    // State diagnostic.
    BitMask_t missing_situations = 0;  //!< `RoRnet::SituationFlags`; The flags from 'for_situations' mask which are not satisfied.
    BitMask_t missing_controls = 0;    //!< `RoRnet::ControlFlags`; The flags from 'for_controls' mask which are not satisfied.
    BitMask_t blocking_situations = 0; //!< `RoRnet::SituationFlags`; The flags from 'except_situations' mask which block this anim.
    BitMask_t blocking_controls = 0;   //!< `RoRnet::ControlFlags`; The flags from 'except_controls' mask which block this anim.

    // The raw source data of anim position.
    float source_dt = 0.f;
    float source_hspeed = 0.f;
    float source_steering = 0.f;

    // The transformed inputs to anim position.
    float input_dt = 0.f;
    float input_hspeed = 0.f;
    float input_steering = 0.f;
};

/// A visual counterpart to `RoR::Character`.
/// 3D objects are loaded and updated here, but positioning and animations are determined in simulation!
struct GfxCharacter
{
    GfxCharacter(Character* character);
    ~GfxCharacter();
    
    void            BufferSimulationData();
    void            UpdateCharacterInScene(float dt);
    void            DisableAnim(Ogre::AnimationState* anim_state);
    void            EnableAnim(Ogre::AnimationState* anim_state, float time);
    void            UpdateAnimations(float dt);
    void            EvaluateActionDef(CharacterActionDef const& def, float dt);
    void            SetupBoneBlendMask(BoneBlendMaskDef const& mask_def);

    // Spawn helper which resolves skins (SkinZips)
    // Derived from `ActorSpawner::FindOrCreateCustomizedMaterial()`
    Ogre::MaterialPtr FindOrCreateCustomizedMaterial(const std::string& mat_lookup_name);

    // Spawn helper
    void ApplyMultiplayerColoring(Ogre::MaterialPtr mat, std::string sharedMatName);

    Ogre::SceneNode*          xc_scenenode;
    CharacterSB               xc_simbuf;
    CharacterSB               xc_simbuf_prev;
    Character*                xc_character;
    std::string               xc_instance_name;
    std::string               xc_custom_resource_group;
    
    // `GUI::CharacterPoseUtil` context
    std::vector<CharacterActionDbg> xc_action_dbg_states;
    bool                            xc_manual_pose_active = false;

};

/// @} // addtogroup Gfx

} // namespace RoR

