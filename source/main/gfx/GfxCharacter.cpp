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
#include "SkinFileFormat.h"
#include "Terrain.h"
#include "Utils.h"
#include "Water.h"

using namespace Ogre;
using namespace RoR;

GfxCharacter::GfxCharacter(Character* character)
    : xc_character(character)
    , xc_instance_name(character->m_instance_name)
{
    ROR_ASSERT(character);
    ROR_ASSERT(character->m_cache_entry);
    ROR_ASSERT(character->m_cache_entry->character_def);

    // Use the modcache bundle (ZIP) resource group.
    // This is equivalent to what `Actor`s do.
    xc_custom_resource_group = character->m_cache_entry->resource_group;

    Entity* entity = App::GetGfxScene()->GetSceneManager()->createEntity(
        /*entityName:*/ xc_instance_name + "_mesh",
        /*meshName:*/ xc_character->m_cache_entry->character_def->mesh_name,
        /*groupName:*/ character->m_cache_entry->resource_group);

    // fix disappearing mesh
    AxisAlignedBox aabb;
    aabb.setInfinite();
    entity->getMesh()->_setBounds(aabb);

    // add entity to the scene node
    xc_scenenode = App::GetGfxScene()->GetSceneManager()->getRootSceneNode()->createChildSceneNode();
    xc_scenenode->attachObject(entity);
    xc_scenenode->setScale(xc_character->m_cache_entry->character_def->mesh_scale);
    xc_scenenode->setVisible(false);

    // setup material - resolve skins (SkinZips) and apply multiplayer color
    // NOTE: The mesh (for example Sinbad the Ogre) may use multiple materials
    //       which means having multiple submeshes in Mesh and subentities in Entity
    //       (each submesh/subentity can have only 1 material)
    for (Ogre::SubEntity* subent: entity->getSubEntities())
    {
        const std::string sharedMatName = subent->getSubMesh()->getMaterialName();
        subent->setMaterial(this->FindOrCreateCustomizedMaterial(sharedMatName));
    }

    // setup animation blend
    switch (character->getCharacterDocument()->force_animblend)
    {
    case ForceAnimBlend::CUMULATIVE: entity->getSkeleton()->setBlendMode(ANIMBLEND_CUMULATIVE); break;
    case ForceAnimBlend::AVERAGE: entity->getSkeleton()->setBlendMode(ANIMBLEND_AVERAGE); break;
    default:; // Keep the preset we loaded from .skeleton file
    }

    // setup bone blend masks
    for (BoneBlendMaskDef& mask_def : character->getCharacterDocument()->bone_blend_masks)
    {
        this->SetupBoneBlendMask(mask_def);
    }

    // setup diagnostic UI
    App::GetGuiManager()->CharacterPoseUtil.action_dbg_states.resize(xc_character->m_cache_entry->character_def->actions.size());
    for (CharacterActionDef const& def : xc_character->m_cache_entry->character_def->actions)
    {
        App::GetGuiManager()->CharacterPoseUtil.action_dbg_states[def.action_id] = CharacterActionDbg();
    }
}

void GfxCharacter::SetupBoneBlendMask(BoneBlendMaskDef const& mask_def)
{
    Entity* entity = static_cast<Entity*>(xc_scenenode->getAttachedObject(0));

    AnimationState* mask_created = nullptr;
    try
    {
        AnimationState* anim = entity->getAnimationState(mask_def.anim_name);
        if (mask_def.bone_weights.size() > 0)
        {
            anim->createBlendMask(entity->getSkeleton()->getNumBones());
            mask_created = anim;
            for (BoneBlendMaskWeightDef const& def : mask_def.bone_weights)
            {
                Bone* bone = entity->getSkeleton()->getBone(def.bone_name);
                anim->setBlendMaskEntry(bone->getHandle(), def.bone_weight);
            }
        }
    }
    catch (Exception& eeh)
    {
        App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_ERROR,
            fmt::format("error setting up bone blend mask for animation '{}', message:{}", mask_def.anim_name, eeh.getFullDescription()));
        if (mask_created)
            mask_created->destroyBlendMask();
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
    xc_simbuf.simbuf_control_flags          = xc_character->m_control_flags;
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

void GfxCharacter::EvaluateActionDef(CharacterActionDef const& def, float dt)
{
    CharacterActionDbg dbg;

    // Test if applicable.
    if ((!BITMASK_IS_1(xc_simbuf.simbuf_situation_flags, def.for_situations)) || // not all situation flags are satisified
        (xc_simbuf.simbuf_situation_flags & def.except_situations) || // any of the forbidden situation matches
        (!BITMASK_IS_1(xc_simbuf.simbuf_control_flags, def.for_controls)) || // not all action flags are satisfied
        (xc_simbuf.simbuf_control_flags & def.except_controls)) // any of the forbidden situation matches
    {
        dbg.blocking_situations = xc_simbuf.simbuf_situation_flags & def.except_situations;
        dbg.blocking_controls = xc_simbuf.simbuf_control_flags & def.except_controls;
        dbg.missing_situations = def.for_situations & ~xc_simbuf.simbuf_situation_flags;
        dbg.missing_controls = def.for_controls & ~xc_simbuf.simbuf_control_flags;
        App::GetGuiManager()->CharacterPoseUtil.action_dbg_states[def.action_id] = dbg;
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
        if (!BITMASK_IS_1(xc_simbuf_prev.simbuf_control_flags, def.for_controls))
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
    App::GetGuiManager()->CharacterPoseUtil.action_dbg_states[def.action_id] = dbg;
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

    for (CharacterActionDef const& def : xc_character->m_cache_entry->character_def->actions)
    {
        this->EvaluateActionDef(def, dt);
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


Ogre::MaterialPtr GfxCharacter::FindOrCreateCustomizedMaterial(const std::string& mat_lookup_name)
{
    // Spawn helper which resolves skins (SkinZips) and returns unique material
    // Derived from `ActorSpawner::FindOrCreateCustomizedMaterial()`
    // ------------------------------------------------------------------------

    // Query .skin material replacements
    if (xc_character->m_used_skin_entry != nullptr)
    {
        std::shared_ptr<RoR::SkinDef>& skin_def = xc_character->m_used_skin_entry->skin_def;

        auto skin_res = skin_def->replace_materials.find(mat_lookup_name);
        if (skin_res != skin_def->replace_materials.end())
        {
            // Material substitution found - check if the material exists.
            Ogre::MaterialPtr skin_mat = Ogre::MaterialManager::getSingleton().getByName(
                skin_res->second, xc_character->m_used_skin_entry->resource_group);
            if (!skin_mat.isNull())
            {
                // Material exists - clone and return right away - texture substitutions aren't done in this case.
                std::string name_buf = fmt::format("{}@{}", skin_mat->getName(), xc_character->m_instance_name);
                return skin_mat->clone(name_buf, /*changeGroup=*/true, xc_custom_resource_group);
            }
            else
            {
                // Material doesn't exist - log warning and continue.
                std::stringstream buf;
                buf << "Character: Material '" << skin_res->second << "' from skin '" << xc_character->m_used_skin_entry->dname
                    << "' not found (filename: '" << xc_character->m_used_skin_entry->fname 
                    << "', resource group: '"<< xc_character->m_used_skin_entry->resource_group
                    <<"')! Ignoring it...";
                App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_WARNING, buf.str());
            }
        }
    }

    // Skinzip material replacement not found - clone the input material
    MaterialPtr sharedMat = MaterialManager::getSingleton().getByName(
        /*name:*/ mat_lookup_name,
        /*groupName:*/ xc_character->m_cache_entry->resource_group);
    std::string name_buf = fmt::format("{}@{}", mat_lookup_name, xc_character->m_instance_name);
    MaterialPtr ownMat = sharedMat->clone(name_buf, /*changeGroup=*/true, xc_custom_resource_group);

    // Finally, query .skin texture replacements
    if (xc_character->m_used_skin_entry != nullptr)
    {
        for (auto& technique: ownMat->getTechniques())
        {
            for (auto& pass: technique->getPasses())
            {
                for (auto& tex_unit: pass->getTextureUnitStates())
                {
                    const size_t num_frames = tex_unit->getNumFrames();
                    for (size_t i = 0; i < num_frames; ++i)
                    {
                        const auto end = xc_character->m_used_skin_entry->skin_def->replace_textures.end();
                        const auto query = xc_character->m_used_skin_entry->skin_def->replace_textures.find(tex_unit->getFrameTextureName((unsigned int)i));
                        if (query != end)
                        {
                            // Skin has replacement for this texture
                            if (xc_character->m_used_skin_entry->resource_group != xc_custom_resource_group) // The skin comes from a SkinZip bundle (different resource group)
                            {
                                Ogre::TexturePtr tex = Ogre::TextureManager::getSingleton().getByName(
                                    query->second, xc_character->m_used_skin_entry->resource_group);
                                if (tex.isNull())
                                {
                                    // `Ogre::TextureManager` doesn't automatically register all images in resource groups,
                                    // it waits for `Ogre::Resource`s to be created explicitly.
                                    // Normally this is done by `Ogre::MaterialManager` when loading a material.
                                    // In this case we must do it manually
                                    tex = Ogre::TextureManager::getSingleton().create(
                                        query->second, xc_character->m_used_skin_entry->resource_group);
                                }
                                tex_unit->_setTexturePtr(tex, i);
                            }
                            else // The skin lives in the character bundle (same resource group)
                            {
                                tex_unit->setFrameTextureName(query->second, (unsigned int)i);
                            }
                        }
                    } // texture unit frames
                } // texture unit states
            } // passes
        } // techniques
    }

    // Apply player color
    if (App::mp_state->getEnum<MpState>() == MpState::CONNECTED)
    {
        this->ApplyMultiplayerColoring(ownMat, mat_lookup_name);
    }    

    return ownMat;
}

void GfxCharacter::ApplyMultiplayerColoring(Ogre::MaterialPtr mat, std::string sharedMatName)
{
    if (!mat.isNull() && mat->getNumTechniques() > 0) // sanity check
    {
        Ogre::Pass* colorChangePass = mat->getTechnique(0)->getPass("ColorChange");
        if (colorChangePass)
        {
            Ogre::TextureUnitState* playerColorTexUnit = colorChangePass->getTextureUnitState("PlayerColor");
            if (playerColorTexUnit)
            {
                playerColorTexUnit->setColourOperationEx(LBX_BLEND_CURRENT_ALPHA, LBS_MANUAL, LBS_CURRENT,
                    App::GetNetwork()->GetPlayerColor(xc_character->m_color_number));
            }
            else
            {
                std::string msg = fmt::format("Character '{}' material '{}', pass 'ColorChange' doesn't have texture unit named 'PlayerColor', cannot apply player color", xc_character->m_instance_name, sharedMatName);
                LOG(msg);                
            }
        }
        else
        {
            std::string msg = fmt::format("Character '{}' material '{}' doesn't have pass named 'ColorChange', cannot apply player color", xc_character->m_instance_name, sharedMatName);
            LOG(msg);
        }
    }
}
