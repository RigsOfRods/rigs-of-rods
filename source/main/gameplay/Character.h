/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2017-2018 Petr Ohlidal

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

#include "ForwardDeclarations.h"
#include "SurveyMapEntity.h"
#include "CartesianToTriangleTransform.h"

#include <enet/enet.h>
#include <OgreMeshManager.h>
#include <OgreTimer.h>
#include <string>

namespace RoR {

/// @addtogroup Gameplay
/// @{

/// @addtogroup Character
/// @{

struct CharacterCabContactInfo
{
    ActorInstanceID_t contacting_actor = ACTORINSTANCEID_INVALID;
    CollisionCabID_t  contacting_cab = COLLISIONCABID_INVALID;
    Triangle          cab_cached_worldpos;
    TriangleCoord     chara_localpos;
    Ogre::Radian      vehicle_rotation = Ogre::Radian(0.f);
    float             depth = 0.f;
    CollisionCabID_t  dbg_intersect_cab = COLLISIONCABID_INVALID;
    float             dbg_intersect_depth = 0.f;
};

class Character
{
public:

    Character(int source = -1, unsigned int streamid = 0, std::string playerName = "", int color_number = 0, bool is_remote = true);
    ~Character();

    int              cr_net_stream_id;
    int              cr_net_source_id;


    int            GetColorNum() const                  { return m_color_number; }
    bool           GetIsRemote() const                  { return m_is_remote; }
    std::string const&     GetNetUsername()             { return m_net_username; }
    std::string const &    GetAnimName() const          { return m_anim_name; }
    float          GetAnimTime() const                  { return m_anim_time; }
    Ogre::Radian   getRotation() const                  { return m_character_rotation; }
    ActorPtr       GetOccupiedActor();
    void           SetOccupiedActor(const ActorPtr& actor, int seat_num);
    void           setColour(int color)                 { this->m_color_number = color; }
    Ogre::Vector3  getPosition();
    void           setPosition(Ogre::Vector3 position);
    void           setRotation(Ogre::Radian rotation);
    void           move(Ogre::Vector3 offset);
    void           update(float dt);
    void           updateCharacterRotation();
    void           receiveStreamData(ENetPacket* packet);
    GfxCharacter*  SetupGfx();

private:


    void           SendStreamData();
    void           SendStreamSetup();
    void           SetAnimState(std::string mode, float time = 0);

    Ogre::Radian     m_character_rotation;
    float            m_character_h_speed;
    float            m_character_v_speed;
    Ogre::Vector3    m_character_position;
    Ogre::Vector3    m_prev_position;
    int              m_color_number;
    bool             m_can_jump;
    bool             m_is_remote;
    std::string      m_anim_name;
    float            m_anim_time;
    float            m_net_last_anim_time;
    std::string      m_instance_name;
    std::string      m_net_username;
    Ogre::Timer      m_net_timer;
    unsigned long    m_net_last_update_time;
    GfxCharacter*    m_gfx_character;

    // Occupying an actor (seating):
    ActorPtr         m_occupied_actor; //!< The vehicle or machine which the character occupies
    int              m_occupied_seat = 0;
    float            m_driving_anim_length;

    // Collision with actor (standing):
    bool             m_inertia = false;
    Ogre::Vector3    m_inertia_translation;
    Ogre::Radian     m_inertia_rotation;
    void             DrawDebugUI();
    Ogre::Vector3    CalcCabAveragePos(ActorPtr actor, int cab_index);
    CharacterCabContactInfo FindContactingCab(const Ogre::Vector3& position);
    CharacterCabContactInfo m_last_contact_info;
    CharacterCabContactInfo m_contact_info;
    CharacterCabContactInfo m_debug_lastlast_contact_info;
};

/// @} // addtogroup Character
/// @} // addtogroup Gameplay

struct GfxCharacter
{
    struct SimBuffer //!< Buffered simulation state for async gfx scene update
    {
        Ogre::Vector3      simbuf_character_pos;
        Ogre::Radian       simbuf_character_rot; //!< When on foot
        std::string        simbuf_net_username;
        bool               simbuf_is_remote;
        int                simbuf_color_number;
        ActorPtr           simbuf_actor_coupling;
        std::string        simbuf_anim_name;
        float              simbuf_anim_time; // Intentionally left empty = forces initial update.
    };
    
    ~GfxCharacter();
    
    void            BufferSimulationData();
    void            UpdateCharacterInScene();

    Ogre::SceneNode*          xc_scenenode;
    SimBuffer                 xc_simbuf;
    SimBuffer                 xc_simbuf_prev;
    Character*                xc_character;
    std::string               xc_instance_name; // TODO: Store MaterialPtr-s directly ~only_a_ptr, 05/2018
    SurveyMapEntity           xc_surveymap_entity;
};

} // namespace RoR

