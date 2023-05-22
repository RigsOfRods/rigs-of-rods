/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2015-2020 Petr Ohlidal

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

#include "RecoveryMode.h"

#include "GameContext.h"
#include "InputEngine.h"

using namespace RoR;

void RecoveryMode::UpdateInputEvents(float dt)
{
    if (App::sim_state->getEnum<SimState>() != SimState::RUNNING &&
        App::GetGameContext()->GetPlayerActor() &&
        App::GetGameContext()->GetPlayerActor()->ar_state != ActorState::NETWORKED_OK &&
        App::GetGameContext()->GetPlayerActor()->ar_state != ActorState::LOCAL_REPLAY)
    {
        return;
    }

    if (!App::GetGameContext()->GetPlayerActor())
    {
        m_advanced_vehicle_repair = false;
        m_advanced_vehicle_repair_timer = 0.0f;
        return;
    }

    if (!App::GetInputEngine()->getEventBoolValue(EV_COMMON_REPAIR_TRUCK))
    {
        m_advanced_vehicle_repair_timer = 0.0f;
    }

    if (App::GetInputEngine()->getEventBoolValue(EV_COMMON_REPAIR_TRUCK) || m_advanced_vehicle_repair)
    {
        if (App::GetInputEngine()->getEventBoolValue(EV_COMMON_REPAIR_TRUCK))
        {
            m_advanced_vehicle_repair = m_advanced_vehicle_repair_timer > 1.0f;
        }

        Ogre::Vector3 translation = Ogre::Vector3::ZERO;
        float rotation = 0.0f;

        if (App::GetInputEngine()->getEventBoolValue(EV_TRUCK_ACCELERATE))
        {
            translation += 2.0f * Ogre::Vector3::UNIT_Y * dt;
        }
        else if (App::GetInputEngine()->getEventBoolValue(EV_TRUCK_BRAKE))
        {
            translation -= 2.0f * Ogre::Vector3::UNIT_Y * dt;
        }
        if (App::GetInputEngine()->getEventBoolValue(EV_TRUCK_STEER_LEFT))
        {
            rotation += 0.5f * dt;
        }
        else if (App::GetInputEngine()->getEventBoolValue(EV_TRUCK_STEER_RIGHT))
        {
            rotation -= 0.5f * dt;
        }
        if (App::GetInputEngine()->getEventBoolValue(EV_CHARACTER_FORWARD))
        {
            float curRot = App::GetGameContext()->GetPlayerActor()->getRotation();
            translation.x += 2.0f * cos(curRot - Ogre::Math::HALF_PI) * dt;
            translation.z += 2.0f * sin(curRot - Ogre::Math::HALF_PI) * dt;
        }
        else if (App::GetInputEngine()->getEventBoolValue(EV_CHARACTER_BACKWARDS))
        {
            float curRot = App::GetGameContext()->GetPlayerActor()->getRotation();
            translation.x -= 2.0f * cos(curRot - Ogre::Math::HALF_PI) * dt;
            translation.z -= 2.0f * sin(curRot - Ogre::Math::HALF_PI) * dt;
        }
        if (App::GetInputEngine()->getEventBoolValue(EV_CHARACTER_SIDESTEP_RIGHT))
        {
            float curRot = App::GetGameContext()->GetPlayerActor()->getRotation();
            translation.x += 2.0f * cos(curRot) * dt;
            translation.z += 2.0f * sin(curRot) * dt;
        }
        else if (App::GetInputEngine()->getEventBoolValue(EV_CHARACTER_SIDESTEP_LEFT))
        {
            float curRot = App::GetGameContext()->GetPlayerActor()->getRotation();
            translation.x -= 2.0f * cos(curRot) * dt;
            translation.z -= 2.0f * sin(curRot) * dt;
        }

        if (translation != Ogre::Vector3::ZERO || rotation != 0.0f)
        {
            float scale = App::GetInputEngine()->isKeyDown(OIS::KC_LMENU) ? 0.1f : 1.0f;
            scale *= App::GetInputEngine()->isKeyDown(OIS::KC_LSHIFT) ? 3.0f : 1.0f;
            scale *= App::GetInputEngine()->isKeyDown(OIS::KC_LCONTROL) ? 10.0f : 1.0f;

            Ogre::Vector3 rotation_center = App::GetGameContext()->GetPlayerActor()->getRotationCenter();

            rotation *= Ogre::Math::Clamp(scale, 0.1f, 10.0f);
            translation *= scale;

            App::GetGameContext()->GetPlayerActor()->requestRotation(rotation, rotation_center);
            App::GetGameContext()->GetPlayerActor()->requestTranslation(translation);

            if (App::sim_soft_reset_mode->getBool())
            {
                for (ActorPtr actor : App::GetGameContext()->GetPlayerActor()->getAllLinkedActors())
                {
                    actor->requestRotation(rotation, rotation_center);
                    actor->requestTranslation(translation);
                }
            }

            m_advanced_vehicle_repair_timer = 0.0f;
        }
        else if (App::GetInputEngine()->isKeyDownValueBounce(OIS::KC_SPACE))
        {
            App::GetGameContext()->GetPlayerActor()->requestAngleSnap(45);
            if (App::sim_soft_reset_mode->getBool())
            {
                for (ActorPtr actor : App::GetGameContext()->GetPlayerActor()->getAllLinkedActors())
                {
                    actor->requestAngleSnap(45);
                }
            }
        }
        else
        {
            m_advanced_vehicle_repair_timer += dt;
        }

        auto reset_type = ActorModifyRequest::Type::RESET_ON_SPOT;
        if (App::sim_soft_reset_mode->getBool())
        {
            reset_type = ActorModifyRequest::Type::SOFT_RESET;
            for (ActorPtr actor : App::GetGameContext()->GetPlayerActor()->getAllLinkedActors())
            {
                ActorModifyRequest* rq = new ActorModifyRequest;
                rq->amr_actor = actor->ar_instance_id;
                rq->amr_type = reset_type;
                App::GetGameContext()->PushMessage(Message(MSG_SIM_MODIFY_ACTOR_REQUESTED, (void*)rq));
            }
        }

        ActorModifyRequest* rq = new ActorModifyRequest;
        rq->amr_actor = App::GetGameContext()->GetPlayerActor()->ar_instance_id;
        rq->amr_type = reset_type;
        App::GetGameContext()->PushMessage(Message(MSG_SIM_MODIFY_ACTOR_REQUESTED, (void*)rq));
    }
}
