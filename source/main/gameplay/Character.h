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


#include "ForwardDeclarations.h"
#include "SimBuffers.h"

#include <OgreUTFString.h>
#include <OgreMeshManager.h>
#include <OgreTimer.h>
#include <string>

namespace RoR {

/// @addtogroup Gameplay
/// @{

/// @addtogroup Character
/// @{

/// Character uses simplified physics and occupies single point in space.
/// Note on animations: 
///     This object decides what animations are played and how fast, but doesn't apply it to visual scene.
///     Visual 3D model and animations are loaded and updated by `RoR::GfxCharacter` using data from sim buffers (see file 'SimBuffers.h')
class Character
{
    friend struct GfxCharacter; // visual counterpart.

public:

    Character(int source = -1, unsigned int streamid = 0, Ogre::UTFString playerName = "", int color_number = 0, bool is_remote = true);
    ~Character();
       
    Ogre::UTFString const& GetNetUsername()             { return m_net_username; }
    Ogre::Radian   getRotation() const                  { return m_character_rotation; }
    ActorPtr       GetActorCoupling();
    
    Ogre::Vector3  getPosition();
    void           setPosition(Ogre::Vector3 position);
    void           setRotation(Ogre::Radian rotation);
    void           move(Ogre::Vector3 offset);
    void           update(float dt);
    void           updateCharacterRotation();
    void           SetActorCoupling(bool enabled, ActorPtr actor);

    // network
    void           receiveStreamData(unsigned int& type, int& source, unsigned int& streamid, char* buffer);
    int            getSourceID() const                  { return m_source_id; }
    bool           isRemote() const                     { return m_is_remote; }
    int            GetColorNum() const                  { return m_color_number; }
    void           setColour(int color)                 { this->m_color_number = color; }

    // anims
    std::string const & GetUpperAnimName() const        { return m_anim_upper_name; }
    float          GetUpperAnimTime() const             { return m_anim_upper_time; }
    std::string const & GetLowerAnimName() const        { return m_anim_lower_name; }
    float          GetLowerAnimTime() const             { return m_anim_lower_time; }

private:

    void           ReportError(const char* detail);
    void           SendStreamData();
    void           SendStreamSetup();
    void           SetUpperAnimState(std::string mode, float time = 0);
    void           SetLowerAnimState(std::string mode, float time = 0);

    ActorPtr         m_actor_coupling; //!< The vehicle or machine which the character occupies
    Ogre::Radian     m_character_rotation;
    float            m_character_h_speed;
    float            m_character_v_speed;
    Ogre::Vector3    m_character_position;
    Ogre::Vector3    m_prev_position;
    bool             m_can_jump;
    std::string      m_instance_name;

    // visuals
    float            m_driving_anim_length;

    // network
    bool             m_is_remote;
    float            m_net_last_anim_time;
    int              m_color_number;
    Ogre::UTFString  m_net_username;
    Ogre::Timer      m_net_timer;
    unsigned long    m_net_last_update_time;
    int              m_stream_id;
    int              m_source_id;

    // anims
    std::string      m_anim_upper_name;
    float            m_anim_upper_time;
    std::string      m_anim_lower_name;
    float            m_anim_lower_time;
};

/// @} // addtogroup Character
/// @} // addtogroup Gameplay

} // namespace RoR

