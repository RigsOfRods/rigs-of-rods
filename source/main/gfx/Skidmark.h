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

#include "RoRPrerequisites.h"

#include "Singleton.h"

class SkidmarkManager : public RoRSingleton<SkidmarkManager>, public ZeroedMemoryAllocator
{
public:

    SkidmarkManager();
    ~SkidmarkManager();

    int getTexture(Ogre::String model, Ogre::String ground, float slip, Ogre::String& texture);

private:

    struct SkidmarkConf
    {
        Ogre::String ground;
        Ogre::String texture;
        float slipFrom;
        float slipTo;
    };

    int loadDefaultModels();
    int processLine(Ogre::StringVector args, Ogre::String model);

    std::map<Ogre::String, std::vector<SkidmarkConf>> m_models;
};

class Skidmark : public ZeroedMemoryAllocator
{
public:

    /// Constructor - see setOperationType() for description of argument.
    Skidmark(wheel_t* m_wheel, Ogre::SceneNode* snode, int m_length = 500, int m_bucket_count = 20);
    virtual ~Skidmark();

    void updatePoint();

    void update();

private:

    static int instanceCounter;

    struct SkidmarkObject
    {
        Ogre::ManualObject* obj;
        std::vector<Ogre::Vector3> points;
        std::vector<Ogre::Real> faceSizes;
        std::vector<Ogre::String> groundTexture;
        Ogre::Vector3 lastPointAv;
        int pos;
        Ogre::ColourValue colour;
        int facecounter;
    };

    void limitObjects();
    void addObject(Ogre::Vector3 start, Ogre::String texture);
    void setPointInt(unsigned short index, const Ogre::Vector3& value, Ogre::Real fsize, Ogre::String texture);
    void addPoint(const Ogre::Vector3& value, Ogre::Real fsize, Ogre::String texture);
    
    bool                 m_is_dirty;
    std::queue<SkidmarkObject> m_objects;
    float                m_max_distance;
    float                m_max_distance_squared;
    float                m_min_distance;
    float                m_min_distance_squared;
    static Ogre::Vector2 m_tex_coords[4];
    int                  m_bucket_count;
    int                  m_length;
    wheel_t*             m_wheel;
    Ogre::SceneNode*     m_scene_node;  
};
