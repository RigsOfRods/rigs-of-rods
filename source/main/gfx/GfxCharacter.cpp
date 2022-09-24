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

#include "GfxCharacter.h"

#include "Application.h"
#include "Actor.h"
#include "ActorManager.h"
#include "CameraManager.h"
#include "Collisions.h"
#include "Console.h"
#include "GameContext.h"
#include "GfxScene.h"
#include "GUIManager.h"
#include "GUI_CharacterPoseUtil.h"
#include "InputEngine.h"
#include "MovableText.h"
#include "Network.h"
#include "Terrain.h"
#include "Utils.h"
#include "Water.h"

using namespace Ogre;
using namespace RoR;

GfxCharacter::GfxCharacter(Character* character)
    : xc_character(character)
    , xc_instance_name(character->m_instance_name)
{
    Entity* entity = App::GetGfxScene()->GetSceneManager()->createEntity(xc_instance_name + "_mesh", xc_character->m_character_def->mesh_name);

    // fix disappearing mesh
    AxisAlignedBox aabb;
    aabb.setInfinite();
    entity->getMesh()->_setBounds(aabb);

    // add entity to the scene node
    xc_scenenode = App::GetGfxScene()->GetSceneManager()->getRootSceneNode()->createChildSceneNode();
    xc_scenenode->attachObject(entity);
    xc_scenenode->setScale(0.02f, 0.02f, 0.02f);
    xc_scenenode->setVisible(false);

    // setup colour
    MaterialPtr mat1 = MaterialManager::getSingleton().getByName("tracks/character");
    MaterialPtr mat2 = mat1->clone("tracks/" + xc_instance_name);
    entity->setMaterialName("tracks/" + xc_instance_name);

    // setup diagnostic UI
    for (CharacterAnimDef const& def : xc_character->m_character_def->anims)
    {
        App::GetGuiManager()->CharacterPoseUtil.anim_dbg_states[def.game_id] = CharacterAnimDbg();
    }
}

RoR::GfxCharacter::~GfxCharacter()
{
    Entity* ent = static_cast<Ogre::Entity*>(xc_scenenode->getAttachedObject(0));
    xc_scenenode->detachAllObjects();
    App::GetGfxScene()->GetSceneManager()->destroySceneNode(xc_scenenode);
    App::GetGfxScene()->GetSceneManager()->destroyEntity(ent);
    MaterialManager::getSingleton().unload("tracks/" + xc_instance_name);
}

void RoR::GfxCharacter::BufferSimulationData()
{
    xc_simbuf_prev = xc_simbuf;

    xc_simbuf.simbuf_character_pos          = xc_character->getPosition();
    xc_simbuf.simbuf_character_rot          = xc_character->getRotation();
    xc_simbuf.simbuf_color_number           = xc_character->GetColorNum();
    xc_simbuf.simbuf_net_username           = xc_character->GetNetUsername();
    xc_simbuf.simbuf_is_remote              = xc_character->isRemote();
    xc_simbuf.simbuf_actor_coupling         = xc_character->GetActorCoupling();
    xc_simbuf.simbuf_action_flags           = xc_character->m_action_flags;
    xc_simbuf.simbuf_situation_flags        = xc_character->m_situation_flags;
    xc_simbuf.simbuf_character_h_speed      = xc_character->m_character_h_speed;
}

void RoR::GfxCharacter::UpdateCharacterInScene(float dt)
{
    // Actor coupling
    if (xc_simbuf.simbuf_actor_coupling != xc_simbuf_prev.simbuf_actor_coupling)
    {
        if (xc_simbuf.simbuf_actor_coupling != nullptr)
        {
            // Entering/switching vehicle
            xc_scenenode->getAttachedObject(0)->setCastShadows(false);
            xc_scenenode->setVisible(xc_simbuf.simbuf_actor_coupling->GetGfxActor()->HasDriverSeatProp());
        }
        else if (xc_simbuf_prev.simbuf_actor_coupling != nullptr)
        {
            // Leaving vehicle
            xc_scenenode->getAttachedObject(0)->setCastShadows(true);
            xc_scenenode->resetOrientation();
        }
    }

    // Position + Orientation
    Ogre::Entity* entity = static_cast<Ogre::Entity*>(xc_scenenode->getAttachedObject(0));
    if (xc_simbuf.simbuf_actor_coupling != nullptr)
    {
        // We're in vehicle
        GfxActor* gfx_actor = xc_simbuf.simbuf_actor_coupling->GetGfxActor();

        // Update character visibility first
        switch (gfx_actor->GetSimDataBuffer().simbuf_actor_state)
        {
        case ActorState::NETWORKED_HIDDEN:
            entity->setVisible(false);
            break;
        case ActorState::NETWORKED_OK:
            entity->setVisible(gfx_actor->HasDriverSeatProp());
            break;
        default:
            break; // no change.
        }

        // If visible, update position
        if (entity->isVisible())
        {
            Ogre::Vector3 pos;
            Ogre::Quaternion rot;
            xc_simbuf.simbuf_actor_coupling->GetGfxActor()->CalculateDriverPos(pos, rot);
            xc_scenenode->setOrientation(rot);
            // hack to position the character right perfect on the default seat (because the mesh has decentered origin)
            xc_scenenode->setPosition(pos + (rot * Vector3(0.f, -0.6f, 0.f)));
        }
    }
    else
    {
        xc_scenenode->resetOrientation();
        xc_scenenode->yaw(-xc_simbuf.simbuf_character_rot);
        xc_scenenode->setPosition(xc_simbuf.simbuf_character_pos);
        xc_scenenode->setVisible(true);
    }

    if (!App::GetGuiManager()->CharacterPoseUtil.IsManualPoseActive())
    {
        try
        {
            this->UpdateAnimations(dt);
        }
        catch (Ogre::Exception& eeh) {
            App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_ERROR,
                fmt::format("error updating animations, message:{}", eeh.getFullDescription()));
        }
    }

    // Multiplayer label
#ifdef USE_SOCKETW
    if (App::mp_state->getEnum<MpState>() == MpState::CONNECTED && !xc_simbuf.simbuf_actor_coupling)
    {
        // From 'updateCharacterNetworkColor()'
        const String materialName = "tracks/" + xc_instance_name;

        MaterialPtr mat = MaterialManager::getSingleton().getByName(materialName);
        if (!mat.isNull() && mat->getNumTechniques() > 0 && mat->getTechnique(0)->getNumPasses() > 1 &&
                mat->getTechnique(0)->getPass(1)->getNumTextureUnitStates() > 1)
        {
            const auto& state = mat->getTechnique(0)->getPass(1)->getTextureUnitState(1);
            Ogre::ColourValue color = App::GetNetwork()->GetPlayerColor(xc_simbuf.simbuf_color_number);
            state->setColourOperationEx(LBX_BLEND_CURRENT_ALPHA, LBS_MANUAL, LBS_CURRENT, color);
        }

        if ((!xc_simbuf.simbuf_is_remote && !App::mp_hide_own_net_label->getBool()) ||
            (xc_simbuf.simbuf_is_remote && !App::mp_hide_net_labels->getBool()))
        {
            float camDist = (xc_scenenode->getPosition() - App::GetCameraManager()->GetCameraNode()->getPosition()).length();
            Ogre::Vector3 scene_pos = xc_scenenode->getPosition();
            scene_pos.y += (1.9f + camDist / 100.0f);

            App::GetGfxScene()->DrawNetLabel(scene_pos, camDist, xc_simbuf.simbuf_net_username, xc_simbuf.simbuf_color_number);
        }
    }
#endif // USE_SOCKETW
}

void GfxCharacter::EvaluateAnimDef(CharacterAnimDef const& def, float dt)
{
    CharacterAnimDbg dbg;

    // Test if applicable.
    if ((!BITMASK_IS_1(xc_simbuf.simbuf_situation_flags, def.for_situations)) || // not all situation flags are satisified
        (xc_simbuf.simbuf_situation_flags & def.except_situations) || // any of the forbidden situation matches
        (!BITMASK_IS_1(xc_simbuf.simbuf_action_flags, def.for_actions)) || // not all action flags are satisfied
        (xc_simbuf.simbuf_action_flags & def.except_actions)) // any of the forbidden situation matches
    {
        dbg.blocking_situations = xc_simbuf.simbuf_situation_flags & def.except_situations;
        dbg.blocking_actions = xc_simbuf.simbuf_action_flags & def.except_actions;
        dbg.missing_situations = def.for_situations & ~xc_simbuf.simbuf_situation_flags;
        dbg.missing_actions = def.for_actions & ~xc_simbuf.simbuf_action_flags;
        App::GetGuiManager()->CharacterPoseUtil.anim_dbg_states[def.game_id] = dbg;
        return;
    }

    Ogre::Entity* entity = static_cast<Ogre::Entity*>(xc_scenenode->getAttachedObject(0));
    AnimationState* as = entity->getAnimationState(def.anim_name);

    // Query data sources.
    float timepos = 1.f;
    if (def.playback_time_ratio != 0.f)
    {
        timepos *= (def.playback_time_ratio * dt);
        dbg.source_dt = dt;
        dbg.input_dt = (def.playback_time_ratio * dt);
    }
    if (def.playback_h_speed_ratio != 0.f)
    {
        timepos *= (def.playback_h_speed_ratio * xc_simbuf.simbuf_character_h_speed);
        dbg.source_hspeed = xc_simbuf.simbuf_character_h_speed;
        dbg.input_hspeed = (def.playback_h_speed_ratio * xc_simbuf.simbuf_character_h_speed);
    }
    if (def.playback_steering_ratio != 0.f && xc_simbuf.simbuf_actor_coupling)
    {
        timepos *= (def.playback_steering_ratio * xc_simbuf.simbuf_actor_coupling->ar_hydro_dir_wheel_display);
        dbg.source_steering = xc_simbuf.simbuf_actor_coupling->ar_hydro_dir_wheel_display;
        dbg.input_steering = (def.playback_steering_ratio * xc_simbuf.simbuf_actor_coupling->ar_hydro_dir_wheel_display);
    }

    // Transform the anim pos.
    if (def.source_percentual)
    {
        if (def.anim_neutral_mid)
        {
            timepos = (timepos + 1.0f) * 0.5f;
        }
        timepos *= as->getLength();
    }
    if (def.playback_trim > 0.f)
    {
        // prevent animation flickering on the borders:
        if (timepos < def.playback_trim)
        {
            timepos = def.playback_trim;
        }
        if (timepos > as->getLength() - def.playback_trim)
        {
            timepos = as->getLength() - def.playback_trim;
        }
    }
    if (def.anim_autorestart)
    {
        // If the animation was just activated, start from 0.
        if (!BITMASK_IS_1(xc_simbuf_prev.simbuf_action_flags, def.for_actions))
        {
            as->setTimePosition(0.f);
        }
    }
    if (def.anim_continuous)
    {
        timepos += as->getTimePosition();
    }

    // Update the OGRE object
    as->setTimePosition(timepos);
    as->setWeight(def.weight);
    as->setEnabled(true);

    dbg.active = true;
    App::GetGuiManager()->CharacterPoseUtil.anim_dbg_states[def.game_id] = dbg;
}

void GfxCharacter::UpdateAnimations(float dt)
{
    // Reset all anims
    Ogre::Entity* entity = static_cast<Ogre::Entity*>(xc_scenenode->getAttachedObject(0));
    AnimationStateSet* stateset = entity->getAllAnimationStates();
    for (auto& state_pair : stateset->getAnimationStates())
    {
        AnimationState* as = state_pair.second;
        as->setEnabled(false);
        as->setWeight(0);
    }

    for (CharacterAnimDef const& def : xc_character->m_character_def->anims)
    {
        this->EvaluateAnimDef(def, dt);
    }
}

void GfxCharacter::DisableAnim(Ogre::AnimationState* as)
{
    as->setEnabled(false);
    as->setWeight(0);
}

void GfxCharacter::EnableAnim(Ogre::AnimationState* as, float time)
{
    as->setEnabled(true);
    as->setWeight(1);
    as->setTimePosition(time); // addTime() ?
}
