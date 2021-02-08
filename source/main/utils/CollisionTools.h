/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2013-2020 Petr Ohlidal

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
/******************************************************************************************
    MOC - Minimal Ogre Collision v 1.0
    The MIT License

    Copyright (c) 2008, 2009 MouseVolcano (Thomas Gradl, Karolina Sefyrin), Esa Kylli

    Thanks to Erik Biermann for the help with the Videos, SEO and Webwork

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in
    all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
    THE SOFTWARE.
******************************************************************************************/

#pragma once

#include "Application.h"
#include <Ogre.h>

// uncomment if you want to use ETM as terrainmanager
//#define ETM_TERRAIN

#ifdef ETM_TERRAIN
#include "ETTerrainInfo.h"
#endif

namespace MOC {

typedef struct mesh_info_
{
    size_t vertex_count;
    size_t index_count;
    Ogre::Vector3* vertices;
    Ogre::uint32* indices;
    bool store;
} mesh_info_t;

class CollisionTools : public ZeroedMemoryAllocator
{
public:
    Ogre::RaySceneQuery* mRaySceneQuery;
    Ogre::RaySceneQuery* mTSMRaySceneQuery;

    Ogre::SceneManager* mSceneMgr;

#ifdef ETM_TERRAIN
	const ET::TerrainInfo* mTerrainInfo;
	CollisionTools(Ogre::SceneManager *sceneMgr, const ET::TerrainInfo* terrainInfo);
#endif

    CollisionTools(Ogre::SceneManager* sceneMgr);
    ~CollisionTools();

    bool raycastFromCamera(Ogre::Window* rw, Ogre::Camera* camera, const Ogre::Vector2& mousecoords, Ogre::Vector3& result, Ogre::MovableObject* & target, float& closest_distance, const Ogre::uint32 queryMask = 0xFFFFFFFF);
    // convenience wrapper with Ogre::v1::Entity to it:
    bool raycastFromCamera(Ogre::Window* rw, Ogre::Camera* camera, const Ogre::Vector2& mousecoords, Ogre::Vector3& result, Ogre::v1::Entity* & target, float& closest_distance, const Ogre::uint32 queryMask = 0xFFFFFFFF);

    bool collidesWithEntity(const Ogre::Vector3& fromPoint, const Ogre::Vector3& toPoint, const float collisionRadius = 2.5f, const float rayHeightLevel = 0.0f, const Ogre::uint32 queryMask = 0xFFFFFFFF);

    void calculateY(Ogre::SceneNode* n, const bool doTerrainCheck = true, const bool doGridCheck = true, const float gridWidth = 1.0f, const Ogre::uint32 queryMask = 0xFFFFFFFF);

    float getTSMHeightAt(const float x, const float z);

    bool raycastFromPoint(const Ogre::Vector3& point, const Ogre::Vector3& normal, Ogre::Vector3& result, Ogre::MovableObject* & target, float& closest_distance, const Ogre::uint32 queryMask = 0xFFFFFFFF);
    // convenience wrapper with Ogre::v1::Entity to it:
    bool raycastFromPoint(const Ogre::Vector3& point, const Ogre::Vector3& normal, Ogre::Vector3& result, Ogre::v1::Entity* & target, float& closest_distance, const Ogre::uint32 queryMask = 0xFFFFFFFF);

    bool raycast(const Ogre::Ray& ray, Ogre::Vector3& result, Ogre::MovableObject* & target, float& closest_distance, const Ogre::uint32 queryMask = 0xFFFFFFFF);
    // convenience wrapper with Ogre::v1::Entity to it:
    bool raycast(const Ogre::Ray& ray, Ogre::Vector3& result, Ogre::v1::Entity* & target, float& closest_distance, const Ogre::uint32 queryMask = 0xFFFFFFFF);

    void setHeightAdjust(const float heightadjust);
    float getHeightAdjust(void);

private:

    float _heightAdjust;

    void GetMeshInformation(const Ogre::MeshPtr mesh,
        size_t& vertex_count,
        Ogre::Vector3* & vertices,
        size_t& index_count,
        Ogre::uint32* & indices,
        const Ogre::Vector3& position,
        const Ogre::Quaternion& orient,
        const Ogre::Vector3& scale);

    void getStaticGeometry(Ogre::StaticGeometry* mesh,
        Ogre::StaticGeometry::Region* rg,
        size_t& overtex_count,
        Ogre::Vector3* & overtices,
        size_t& oindex_count,
        Ogre::uint32* & oindices,
        const Ogre::Vector3& position,
        const Ogre::Quaternion& orient,
        const Ogre::Vector3& scale);

    std::map<Ogre::String, mesh_info_t> meshInfoStorage;
};

};
