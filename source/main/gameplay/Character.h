/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer

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

#include <OgreVector3.h>
#include <OgreMath.h> // Ogre::Radian
#include <OgreUTFString.h>

#include <string>
#include <string>

class Character
{
public:

    Character(int source = -1, unsigned int streamid = 0, int color_number = 0, bool is_remote = true);
    ~Character();
       
    int            getSourceID() const                  { return m_source_id; }
    bool           isRemote() const                     { return m_is_remote; }
    Ogre::Radian   getRotation() const                  { return m_character_rotation; }
    bool           IsCoupledWithActor() const           { return m_have_coupling_seat; }
    void           setColour(int color)                 { this->m_color_number = color; }
    Ogre::Vector3  getPosition();
    bool           getVisible(); 
    void           setPosition(Ogre::Vector3 position);
    void           setRotation(Ogre::Radian rotation);
    void           setVisible(bool visible);
    void           move(Ogre::Vector3 offset);
    void           unwindMovement(float distance);
    void           update(float dt);
    void           updateCharacterNetworkColour();
    void           updateCharacterRotation();
    void           updateMapIcon();
    void           updateLabels();
    void           receiveStreamData(unsigned int& type, int& source, unsigned int& streamid, char* buffer);
    void           SetActorCoupling(bool enabled, Actor* actor = nullptr);    

private:

    void           AddPersonToSurveyMap();
    void           ResizePersonNetLabel();
    void           ReportError(const char* detail);
    void           SendStreamData();
    void           SendStreamSetup();
    void           SetAnimState(std::string mode, float time = 0);

    Actor*           m_actor_coupling; //!< The vehicle or machine which the character occupies
    SurveyMapEntity* m_survey_map_entity;
    Ogre::Radian     m_character_rotation;
    float            m_character_h_speed;
    float            m_character_v_speed;
    int              m_color_number;
    int              m_stream_id;
    int              m_source_id;
    bool             m_can_jump;
    bool             m_is_remote;
    bool             m_hide_own_net_label;
    bool             m_have_coupling_seat;    
    std::string      m_last_anim;
    std::string      m_instance_name;
    Ogre::UTFString  m_net_username;
    Ogre::SceneNode*          m_character_scenenode;
    Ogre::MovableText*        m_movable_text;
    Ogre::AnimationStateSet*  m_anim_state;
    std::deque<Ogre::Vector3> m_prev_positions; 


};

