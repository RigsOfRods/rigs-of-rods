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

#include "Application.h"

#include <OgreMaterial.h>
#include <OgreString.h>
#include <OgreVector2.h>
#include <OgreVector3.h>

namespace RoR {

class SkidmarkConfig //!< Skidmark config file parser and data container
{
public:

    void LoadDefaultSkidmarkDefs();

    int getTexture(Ogre::String model, Ogre::String ground, float slip, Ogre::String& texture);

private:

    struct SkidmarkDef
    {
        Ogre::String ground; //!< Ground model name, see `struct ground_model_t`
        Ogre::String texture;
        float slipFrom; //!< Minimum slipping velocity
        float slipTo;   //!< Maximum slipping velocity
    };

    int ProcessSkidmarkConfLine(Ogre::StringVector args, Ogre::String model);

    std::map<Ogre::String, std::vector<SkidmarkDef>> m_models;
};

/// @addtogroup Gfx
/// @{

class Skidmark
{
public:

    /// Constructor - see setOperationType() for description of argument.
    Skidmark(ActorPtr owner, SkidmarkConfig* config,  wheel_t* m_wheel, Ogre::SceneNode* snode, int m_length = 500, int m_bucket_count = 20);
    virtual ~Skidmark();

    void reset();
    void update(Ogre::Vector3 contact_point, int index, float slip, Ogre::String ground_model_name);

private:

    struct SkidmarkSegment //!< Also reffered to as 'bucket'
    {
        Ogre::ManualObject* obj;
        Ogre::MaterialPtr material;
        std::vector<Ogre::Vector3> points;
        std::vector<Ogre::Real> faceSizes;
        std::vector<Ogre::String> groundTexture;
        Ogre::Vector3 lastPointAv;
        int pos;
        int facecounter;
    };

    void PopSegment();
    void LimitObjects();
    void AddObject(Ogre::Vector3 start, Ogre::String texture);
    void SetPointInt(unsigned short index, const Ogre::Vector3& value, Ogre::Real fsize, Ogre::String texture);
    void AddPoint(const Ogre::Vector3& value, Ogre::Real fsize, Ogre::String texture);
    void UpdatePoint(Ogre::Vector3 contact_point, int index, float slip, Ogre::String ground_model_name);

    ActorPtr             m_actor;
    static int           m_instance_counter;
    bool                 m_is_dirty;
    std::queue<SkidmarkSegment> m_objects;
    float                m_max_distance;
    float                m_min_distance;
    static Ogre::Vector2 m_tex_coords[4];
    int                  m_bucket_count;
    int                  m_length;
    wheel_t*             m_wheel;
    Ogre::SceneNode*     m_scene_node;  
    SkidmarkConfig*      m_config;
};

/// @} // addtogroup Gfx

} // namespace RoR
