/*
    This source file is part of Rigs of Rods
    Copyright 2009 Lefteris Stamatogiannakis

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

class PointColDetector : public ZeroedMemoryAllocator
{
public:

    struct pointid_t
    {
        Actor* actor;
        short node_id;
    };

    std::vector<pointid_t*> hit_list;

    PointColDetector(): m_object_list_size(-1) {};

    void UpdateIntraPoint(Actor* actor);
    void UpdateInterPoint(Actor* actor, bool ignorestate = false);
    void query(const Ogre::Vector3& vec1, const Ogre::Vector3& vec2, const Ogre::Vector3& vec3, const float enlargeBB);

private:

    struct refelem_t
    {
        pointid_t* pidref;
        const float* point;
    };

    struct kdnode_t
    {
        float min;
        int end;
        float max;
        refelem_t* ref;
        float middle;
        int begin;
    };

    std::vector<Actor*>    m_actors;
    std::vector<refelem_t> m_ref_list;
    std::vector<pointid_t> m_pointid_list;
    std::vector<kdnode_t>  m_kdtree;
    Ogre::Vector3          m_bbmin;
    Ogre::Vector3          m_bbmax;
    int                    m_object_list_size;

    void queryrec(int kdindex, int axis);
    void build_kdtree_incr(int axis, int index);
    void partintwo(const int start, const int median, const int end, const int axis, float& minex, float& maxex);
    void update_structures_for_contacters();
};
