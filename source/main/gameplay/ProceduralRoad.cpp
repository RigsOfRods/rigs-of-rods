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

#include "ProceduralRoad.h"

#include "Application.h"
#include "Collisions.h"
#include "GfxScene.h"
#include "Terrain.h"

using namespace Ogre;
using namespace RoR;

ProceduralRoad::ProceduralRoad(int id) :
    first(true)
    , mid(id)
    , snode(0)
    , tricount(0)
    , vertexcount(0)
{
    msh.setNull();
}

ProceduralRoad::~ProceduralRoad()
{
    if (snode)
    {
        snode->removeAndDestroyAllChildren();
    }
    if (!msh.isNull())
    {
        MeshManager::getSingleton().remove(msh->getName());
        msh.setNull();
    }
    for (int number : registeredCollTris)
    {
        App::GetSimTerrain()->GetCollisions()->removeCollisionTri(number);
    }
}

void ProceduralRoad::finish()
{
    Vector3 pts[8];
    computePoints(pts, lastpos, lastrot, lasttype, lastwidth, lastbwidth, lastbheight);
    addQuad(pts[7], pts[6], pts[5], pts[4], TEXFIT_NONE, lastpos, lastpos, lastwidth);
    addQuad(pts[7], pts[4], pts[3], pts[0], TEXFIT_NONE, lastpos, lastpos, lastwidth);
    addQuad(pts[3], pts[2], pts[1], pts[0], TEXFIT_NONE, lastpos, lastpos, lastwidth);

    createMesh();
    String entity_name = String("RoadSystem_Instance-").append(StringConverter::toString(mid));
    String mesh_name = String("RoadSystem-").append(StringConverter::toString(mid));
    Entity* ec = App::GetGfxScene()->GetSceneManager()->createEntity(entity_name, mesh_name);
    snode = App::GetGfxScene()->GetSceneManager()->getRootSceneNode()->createChildSceneNode();
    snode->attachObject(ec);
}

void ProceduralRoad::addBlock(Vector3 pos, Quaternion rot, int type, float width, float bwidth, float bheight, int pillartype)
{
    if (type == ROAD_AUTOMATIC)
    {
        width = 10.0;
        bwidth = 1.4;
        bheight = 0.2;
        //define type
        Vector3 leftv = pos + rot * Vector3(0, 0, bwidth + width / 2.0);
        Vector3 rightv = pos + rot * Vector3(0, 0, -bwidth - width / 2.0);
        float dleft = leftv.y - RoR::App::GetSimTerrain()->GetHeightAt(leftv.x, leftv.z);
        float dright = rightv.y - RoR::App::GetSimTerrain()->GetHeightAt(rightv.x, rightv.z);
        if (dleft < bheight + 0.1 && dright < bheight + 0.1)
            type = ROAD_FLAT;
        if (dleft < bheight + 0.1 && dright >= bheight + 0.1 && dright < 4.0)
            type = ROAD_LEFT;
        if (dleft >= bheight + 0.1 && dleft < 4.0 && dright < bheight + 0.1)
            type = ROAD_RIGHT;
        if (dleft >= bheight + 0.1 && dleft < 4.0 && dright >= bheight + 0.1 && dright < 4.0)
            type = ROAD_BOTH;
        if (type == ROAD_AUTOMATIC)
            type = ROAD_BRIDGE;
        if (type != ROAD_FLAT)
        {
            width = 10.0;
            bwidth = 0.4;
            bheight = 0.5;
        };
    }

    Vector3 pts[8];
    if (!first)
    {
        Vector3 lpts[8];
        if (type == ROAD_MONORAIL)
            pos.y += 2;

        computePoints(pts, pos, rot, type, width, bwidth, bheight);
        computePoints(lpts, lastpos, lastrot, lasttype, lastwidth, lastbwidth, lastbheight);

        //tarmac
        if (type == ROAD_MONORAIL)
            addQuad(pts[4], lpts[4], lpts[3], pts[3], TEXFIT_CONCRETETOP, pos, lastpos, width);
        else
            addQuad(pts[4], lpts[4], lpts[3], pts[3], TEXFIT_ROAD, pos, lastpos, width);

        if (type == ROAD_FLAT && lasttype == ROAD_FLAT)
        {
            //sides (close)
            addQuad(pts[5], lpts[5], lpts[4], pts[4], TEXFIT_ROADS3, pos, lastpos, width);
            addQuad(pts[3], lpts[3], lpts[2], pts[2], TEXFIT_ROADS2, pos, lastpos, width);
            //sides (far)
            addQuad(pts[6], lpts[6], lpts[5], pts[5], TEXFIT_ROADS4, pos, lastpos, width);
            addQuad(pts[2], lpts[2], lpts[1], pts[1], TEXFIT_ROADS1, pos, lastpos, width);
        }
        else
        {
            //sides (close)
            addQuad(pts[5], lpts[5], lpts[4], pts[4], TEXFIT_CONCRETEWALLI, pos, lastpos, width, (type == ROAD_FLAT || type == ROAD_LEFT));
            addQuad(pts[3], lpts[3], lpts[2], pts[2], TEXFIT_CONCRETEWALLI, pos, lastpos, width, !(type == ROAD_FLAT || type == ROAD_RIGHT));
            //sides (far)
            addQuad(pts[6], lpts[6], lpts[5], pts[5], TEXFIT_CONCRETETOP, pos, lastpos, width, (type == ROAD_FLAT || type == ROAD_LEFT));
            addQuad(pts[2], lpts[2], lpts[1], pts[1], TEXFIT_CONCRETETOP, pos, lastpos, width, !(type == ROAD_FLAT || type == ROAD_RIGHT));
        }
        if (type == ROAD_BRIDGE || lasttype == ROAD_BRIDGE || type == ROAD_MONORAIL || lasttype == ROAD_MONORAIL)
        {
            //walls
            addQuad(pts[1], lpts[1], lpts[0], pts[0], TEXFIT_CONCRETEWALL, pos, lastpos, width);
            addQuad(lpts[6], pts[6], pts[7], lpts[7], TEXFIT_CONCRETEWALL, pos, lastpos, width);
            //underside - we flip the underside so it folds gracefully with the top
            addQuad(pts[0], lpts[0], lpts[7], pts[7], TEXFIT_CONCRETEUNDER, pos, lastpos, width, true);
        }
        else
        {
            //walls
            addQuad(pts[1], lpts[1], lpts[0], pts[0], TEXFIT_BRICKWALL, pos, lastpos, width);
            addQuad(lpts[6], pts[6], pts[7], lpts[7], TEXFIT_BRICKWALL, pos, lastpos, width);
        }
        if ((type == ROAD_BRIDGE || type == ROAD_MONORAIL) && pillartype > 0)
        {
            /* this is the basic bridge pillar mod.
             * it will create on pillar for each segment!
             * @todo: create only a few pillars instead of so much!
             */
            // construct the pillars
            Vector3 leftv = pos + rot * Vector3(0, 0, bwidth + width / 2.0);
            Vector3 rightv = pos + rot * Vector3(0, 0, -bwidth - width / 2.0);
            Vector3 middle = lpts[0] - ((lpts[0] + (pts[1] - lpts[0]) / 2) -
                (lpts[7] + (pts[6] - lpts[7]) / 2)) * 0.5;
            float heightleft = RoR::App::GetSimTerrain()->GetHeightAt(leftv.x, leftv.z);
            float heightright = RoR::App::GetSimTerrain()->GetHeightAt(rightv.x, rightv.z);
            float heightmiddle = RoR::App::GetSimTerrain()->GetHeightAt(middle.x, middle.z);

            bool builtpillars = true;

            float sidefactor = 0.5; // 0.5 = middle
            // only re-position short pillars! (< 10 meters)
            // so big bridge pillars do not get repositioned
            if (pos.y - heightmiddle < 10)
            {
                if (heightleft >= heightright)
                    sidefactor = 0.8;
                else
                    sidefactor = 0.2;
            }

            static int pillarcounter = 0;
            pillarcounter++;

            if (pillartype == 2)
            {
                // always in the middle
                sidefactor = 0.5;
                // only build every fifth pillar
                if (pillarcounter % 5)
                    builtpillars = false;
            }

            middle = lpts[0] - ((lpts[0] + (pts[1] - lpts[0]) / 2) -
                (lpts[7] + (pts[6] - lpts[7]) / 2)) * sidefactor;
            float len = middle.y - RoR::App::GetSimTerrain()->GetHeightAt(middle.x, middle.z) + 5;
            float width2 = len / 30;

            if (pillartype == 2 && len > 20)
            // no over-long pillars
                builtpillars = false;

            // do not draw too small pillars, the bridge may hold without them ;)
            if (width2 > 5)
                width2 = 5;

            if (pillartype == 2)
                width2 = 0.2;

            if (width2 >= 0.2 && builtpillars)
            {
                //sides
                addQuad(middle + Vector3(-width2, -len, -width2),
                    middle + Vector3(-width2, 0, -width2),
                    middle + Vector3(width2, 0, -width2),
                    middle + Vector3(width2, -len, -width2),
                    TEXFIT_CONCRETETOP, pos, lastpos, width2);

                addQuad(middle + Vector3(width2, -len, width2),
                    middle + Vector3(width2, 0, width2),
                    middle + Vector3(-width2, 0, width2),
                    middle + Vector3(-width2, -len, width2),
                    TEXFIT_CONCRETETOP, pos, lastpos, width2);

                addQuad(middle + Vector3(-width2, -len, width2),
                    middle + Vector3(-width2, 0, width2),
                    middle + Vector3(-width2, 0, -width2),
                    middle + Vector3(-width2, -len, -width2),
                    TEXFIT_CONCRETETOP, pos, lastpos, width2);

                addQuad(middle + Vector3(width2, -len, -width2),
                    middle + Vector3(width2, 0, -width2),
                    middle + Vector3(width2, 0, width2),
                    middle + Vector3(width2, -len, width2),
                    TEXFIT_CONCRETETOP, pos, lastpos, width2);
            }
        }
    }
    else
    {
        first = false;
        computePoints(pts, pos, rot, type, width, bwidth, bheight);
        addQuad(pts[0], pts[1], pts[2], pts[3], TEXFIT_NONE, pos, pos, width);
        addQuad(pts[0], pts[3], pts[4], pts[7], TEXFIT_NONE, pos, pos, width);
        addQuad(pts[4], pts[5], pts[6], pts[7], TEXFIT_NONE, pos, pos, width);
    }
    lastpos = pos;
    lastrot = rot;
    lastwidth = width;
    lastbwidth = bwidth;
    lastbheight = bheight;
    lasttype = type;

    if (App::diag_terrn_log_roads->getBool())
    {
        Str<2000> msg; msg << "[RoR] Road Block |";
        msg << " pos=(" << pos.x << " " << pos.y << " " << pos.z << ")";
        msg << " rot=(" << rot.x << " " << rot.y << " " << rot.z << ")";
        msg << " width=" << width;
        msg << " bwidth=" << bwidth;
        msg << " bheight=" << bheight;
        msg << " type=" << type;
        for (int i = 0; i < 8; ++i)
        {
            msg << "\n\t Point#" << i << ": " << pts[i].x << " " << pts[i].y << " " << pts[i].z;
        }
        Log(msg.ToCStr());
    }
}

void ProceduralRoad::computePoints(Vector3* pts, Vector3 pos, Quaternion rot, int type, float width, float bwidth, float bheight)
{
    if (type == ROAD_FLAT)
    {
        pts[1] = pos + rot * Vector3(0, -bheight, bwidth + width / 2.0);
        pts[0] = baseOf(pts[1]);
        pts[2] = pos + rot * Vector3(0, -bheight / 4.0, bwidth / 3.0 + width / 2.0);
        pts[3] = pos + rot * Vector3(0, 0, width / 2.0);
        pts[4] = pos + rot * Vector3(0, 0, -width / 2.0);
        pts[5] = pos + rot * Vector3(0, -bheight / 4.0, -bwidth / 3.0 - width / 2.0);
        pts[6] = pos + rot * Vector3(0, -bheight, -bwidth - width / 2.0);
        pts[7] = baseOf(pts[6]);
    }
    if (type == ROAD_BOTH)
    {
        pts[1] = pos + rot * Vector3(0, bheight, bwidth + width / 2.0);
        pts[0] = baseOf(pts[1]);
        pts[2] = pos + rot * Vector3(0, bheight, width / 2.0);
        pts[3] = pos + rot * Vector3(0, 0, width / 2.0);
        pts[4] = pos + rot * Vector3(0, 0, -width / 2.0);
        pts[5] = pos + rot * Vector3(0, bheight, -width / 2.0);
        pts[6] = pos + rot * Vector3(0, bheight, -bwidth - width / 2.0);
        pts[7] = baseOf(pts[6]);
    }
    if (type == ROAD_LEFT)
    {
        pts[1] = pos + rot * Vector3(0, -bheight, bwidth + width / 2.0);
        pts[0] = baseOf(pts[1]);
        pts[2] = pos + rot * Vector3(0, -bheight / 4.0, bwidth / 3.0 + width / 2.0);
        pts[3] = pos + rot * Vector3(0, 0, width / 2.0);
        pts[4] = pos + rot * Vector3(0, 0, -width / 2.0);
        pts[5] = pos + rot * Vector3(0, bheight, -width / 2.0);
        pts[6] = pos + rot * Vector3(0, bheight, -bwidth - width / 2.0);
        pts[7] = baseOf(pts[6]);
    }
    if (type == ROAD_RIGHT)
    {
        pts[1] = pos + rot * Vector3(0, bheight, bwidth + width / 2.0);
        pts[0] = baseOf(pts[1]);
        pts[2] = pos + rot * Vector3(0, bheight, width / 2.0);
        pts[3] = pos + rot * Vector3(0, 0, width / 2.0);
        pts[4] = pos + rot * Vector3(0, 0, -width / 2.0);
        pts[5] = pos + rot * Vector3(0, -bheight / 4.0, -bwidth / 3.0 - width / 2.0);
        pts[6] = pos + rot * Vector3(0, -bheight, -bwidth - width / 2.0);
        pts[7] = baseOf(pts[6]);
    }
    if (type == ROAD_BRIDGE)
    {
        pts[0] = pos + rot * Vector3(0, -0.4, bwidth + width / 2.0);
        pts[1] = pos + rot * Vector3(0, bheight, bwidth + width / 2.0);
        pts[2] = pos + rot * Vector3(0, bheight, width / 2.0);
        pts[3] = pos + rot * Vector3(0, 0, width / 2.0);
        pts[4] = pos + rot * Vector3(0, 0, -width / 2.0);
        pts[5] = pos + rot * Vector3(0, bheight, -width / 2.0);
        pts[6] = pos + rot * Vector3(0, bheight, -bwidth - width / 2.0);
        pts[7] = pos + rot * Vector3(0, -0.4, -bwidth - width / 2.0);
    }
    if (type == ROAD_MONORAIL)
    {
        pts[0] = pos + rot * Vector3(0, -1.4, bwidth + width / 2.0);
        pts[1] = pos + rot * Vector3(0, bheight, bwidth + width / 2.0);
        pts[2] = pos + rot * Vector3(0, bheight, width / 2.0);
        pts[3] = pos + rot * Vector3(0, 0, width / 2.0);
        pts[4] = pos + rot * Vector3(0, 0, -width / 2.0);
        pts[5] = pos + rot * Vector3(0, bheight, -width / 2.0);
        pts[6] = pos + rot * Vector3(0, bheight, -bwidth - width / 2.0);
        pts[7] = pos + rot * Vector3(0, -1.4, -bwidth - width / 2.0);
    }
}

inline Vector3 ProceduralRoad::baseOf(Vector3 p)
{
    float y = RoR::App::GetSimTerrain()->GetHeightAt(p.x, p.z) - 0.01;

    if (y > p.y)
    {
        y = p.y - 0.01;
    }

    return Vector3(p.x, y, p.z);
}

void ProceduralRoad::addQuad(Vector3 p1, Vector3 p2, Vector3 p3, Vector3 p4, int texfit, Vector3 pos, Vector3 lastpos, float width, bool flip)
{
    if (vertexcount + 3 >= MAX_VERTEX || tricount * 3 + 3 + 2 >= MAX_TRIS * 3)
        return;
    Vector2 texf[4];
    textureFit(p1, p2, p3, p4, texfit, texf, pos, lastpos, width);
    //vertexes
    vertex[vertexcount] = p1;
    tex[vertexcount] = texf[0];
    vertex[vertexcount + 1] = p2;
    tex[vertexcount + 1] = texf[1];
    vertex[vertexcount + 2] = p3;
    tex[vertexcount + 2] = texf[2];
    vertex[vertexcount + 3] = p4;
    tex[vertexcount + 3] = texf[3];
    //tris
    if (flip)
    {
        tris[tricount * 3] = vertexcount;
        tris[tricount * 3 + 1] = vertexcount + 1;
        tris[tricount * 3 + 2] = vertexcount + 3;
        tris[tricount * 3 + 3] = vertexcount + 1;
        tris[tricount * 3 + 3 + 1] = vertexcount + 2;
        tris[tricount * 3 + 3 + 2] = vertexcount + 3;
    }
    else
    {
        tris[tricount * 3] = vertexcount;
        tris[tricount * 3 + 1] = vertexcount + 1;
        tris[tricount * 3 + 2] = vertexcount + 2;
        tris[tricount * 3 + 3] = vertexcount;
        tris[tricount * 3 + 3 + 1] = vertexcount + 2;
        tris[tricount * 3 + 3 + 2] = vertexcount + 3;
    }
    if (collision)
    {
        ground_model_t* gm = App::GetSimTerrain()->GetCollisions()->getGroundModelByString("concrete");
        if (texfit == TEXFIT_ROAD || texfit == TEXFIT_ROADS1 || texfit == TEXFIT_ROADS2 || texfit == TEXFIT_ROADS3 || texfit == TEXFIT_ROADS4)
            gm = App::GetSimTerrain()->GetCollisions()->getGroundModelByString("asphalt");
        addCollisionQuad(p1, p2, p3, p4, gm, flip);
    }
    tricount += 2;
    vertexcount += 4;
}

void ProceduralRoad::textureFit(Vector3 p1, Vector3 p2, Vector3 p3, Vector3 p4, int texfit, Vector2* texc, Vector3 pos, Vector3 lastpos, float width)
{
    int i;

    if (texfit == TEXFIT_BRICKWALL || texfit == TEXFIT_CONCRETEWALL || texfit == TEXFIT_CONCRETEWALLI)
    {
        Vector3 ps[4];
        ps[0] = p1;
        ps[1] = p2;
        ps[2] = p3;
        ps[3] = p4;
        Vector3 pref1 = pos;
        Vector3 pref2 = lastpos;
        //make matrix
        Vector3 bx = pref2 - pref1;
        bx.normalise();
        Vector3 by = Vector3::UNIT_Y;
        Vector3 bz = bx.crossProduct(by);
        //coordinates change matrix
        Matrix3 reverse;
        reverse.SetColumn(0, bx);
        reverse.SetColumn(1, by);
        reverse.SetColumn(2, bz);
        Matrix3 forward;
        forward = reverse.Inverse();
        //transpose
        for (i = 0; i < 4; i++)
        {
            Vector3 trv = forward * (ps[i] - pref1);
            if (texfit == TEXFIT_BRICKWALL)
            {
                float ty = 0.746 - trv.y * 0.25 / 4.5;
                // fix overlapping
                if (ty > 1)
                    ty = 1;
                texc[i] = Vector2(trv.x / 10.0, ty);
            }
            if (texfit == TEXFIT_CONCRETEWALL)
            {
                // fix overlapping
                float ty = 0.496 - (trv.y - 0.7) * 0.25 / 4.5;
                if (ty > 1)
                    ty = 1;
                texc[i] = Vector2(trv.x / 10.0, ty);
            }
            if (texfit == TEXFIT_CONCRETEWALLI)
            {
                float ty = 0.496 + trv.y * 0.25 / 4.5;
                // fix overlapping
                if (ty > 1)
                    ty = 1;
                texc[i] = Vector2(trv.x / 10.0, ty);
            }
        }
        return;
    }
    if (texfit == TEXFIT_ROAD || texfit == TEXFIT_ROADS1 || texfit == TEXFIT_ROADS2 || texfit == TEXFIT_ROADS3 || texfit == TEXFIT_ROADS4 || texfit == TEXFIT_CONCRETETOP || texfit == TEXFIT_CONCRETEUNDER)
    {
        Vector3 ps[4];
        ps[0] = p1;
        ps[1] = p2;
        ps[2] = p3;
        ps[3] = p4;
        Vector3 pref1 = pos;
        Vector3 pref2 = lastpos;
        //project
        for (i = 0; i < 4; i++)
            ps[i].y = 0;
        pref1.y = 0;
        pref2.y = 0;
        //make matrix
        Vector3 bx = pref2 - pref1;
        bx.normalise();
        Vector3 by = Vector3::UNIT_Y;
        Vector3 bz = bx.crossProduct(by);
        //coordinates change matrix
        Matrix3 reverse;
        reverse.SetColumn(0, bx);
        reverse.SetColumn(1, by);
        reverse.SetColumn(2, bz);
        Matrix3 forward;
        forward = reverse.Inverse();
        //transpose
        float trvrefz = 0.0;
        for (i = 0; i < 4; i++)
        {
            Vector3 trv = forward * (ps[i] - pref1);
            if (texfit == TEXFIT_CONCRETETOP)
            {
                if (i == 0)
                    trvrefz = trv.z;
                texc[i] = Vector2(trv.x / 10.0, 0.621 + (trv.z - trvrefz) * 0.25 / 4.5);
            }
            else
            {
                float v1 = 0.072;
                float v2 = 0.423;
                if (texfit == TEXFIT_ROADS1)
                {
                    v1 = 0.001;
                    v2 = 0.036;
                };
                if (texfit == TEXFIT_ROADS2)
                {
                    v1 = 0.036;
                    v2 = 0.072;
                };
                if (texfit == TEXFIT_ROADS3)
                {
                    v1 = 0.423;
                    v2 = 0.458;
                };
                if (texfit == TEXFIT_ROADS4)
                {
                    v1 = 0.458;
                    v2 = 0.493;
                };
                if (texfit == TEXFIT_CONCRETEUNDER)
                {
                    v1 = 0.496;
                    v2 = 0.745;
                };
                if (i < 2)
                    texc[i] = Vector2(trv.x / 10.0, v1);
                else
                    texc[i] = Vector2(trv.x / 10.0, v2);
            }
        }
        return;
    }
    //default
    for (i = 0; i < 4; i++)
        texc[i] = Vector2(0, 0);
}

void ProceduralRoad::addCollisionQuad(Vector3 p1, Vector3 p2, Vector3 p3, Vector3 p4, ground_model_t* gm, bool flip)
{
    int triID = 0;
    if (flip)
    {
        triID = App::GetSimTerrain()->GetCollisions()->addCollisionTri(p1, p2, p4, gm);
        if (triID >= 0)
            registeredCollTris.push_back(triID);

        triID = App::GetSimTerrain()->GetCollisions()->addCollisionTri(p4, p2, p3, gm);
        if (triID >= 0)
            registeredCollTris.push_back(triID);
    }
    else
    {
        triID = App::GetSimTerrain()->GetCollisions()->addCollisionTri(p1, p2, p3, gm);
        if (triID >= 0)
            registeredCollTris.push_back(triID);

        triID = App::GetSimTerrain()->GetCollisions()->addCollisionTri(p1, p3, p4, gm);
        if (triID >= 0)
            registeredCollTris.push_back(triID);
    }
}

void ProceduralRoad::createMesh()
{
    AxisAlignedBox aab;
    union
    {
        float* vertices;
        CoVertice_t* covertices;
    };
    /// Create the mesh via the MeshManager
    Ogre::String mesh_name = Ogre::String("RoadSystem-").append(Ogre::StringConverter::toString(mid));
    msh = MeshManager::getSingleton().createManual(mesh_name, ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);

    mainsub = msh->createSubMesh();
    mainsub->setMaterialName("road2");

    /// Define the vertices
    size_t vbufCount = (2 * 3 + 2) * vertexcount;
    vertices = (float*)malloc(vbufCount * sizeof(float));
    int i;
    //fill values
    for (i = 0; i < vertexcount; i++)
    {
        covertices[i].texcoord = tex[i];
        covertices[i].vertex = vertex[i];
        //normals are computed later
        covertices[i].normal = Vector3::ZERO;
        aab.merge(vertex[i]);
    }

    /// Define triangles
    size_t ibufCount = 3 * tricount;

    //compute normals
    for (i = 0; i < tricount && i * 3 + 2 < MAX_TRIS * 3; i++)
    {
        Vector3 v1, v2;
        v1 = covertices[tris[i * 3 + 1]].vertex - covertices[tris[i * 3]].vertex;
        v2 = covertices[tris[i * 3 + 2]].vertex - covertices[tris[i * 3]].vertex;
        v1 = v1.crossProduct(v2);
        v1.normalise();
        covertices[tris[i * 3]].normal += v1;
        covertices[tris[i * 3 + 1]].normal += v1;
        covertices[tris[i * 3 + 2]].normal += v1;
    }
    //normalize
    for (i = 0; i < vertexcount; i++)
    {
        covertices[i].normal.normalise();
    }

    /// Create vertex data structure for vertices shared between sub meshes
    msh->sharedVertexData = new VertexData();
    msh->sharedVertexData->vertexCount = vertexcount;

    /// Create declaration (memory format) of vertex data
    VertexDeclaration* decl = msh->sharedVertexData->vertexDeclaration;
    size_t offset = 0;
    decl->addElement(0, offset, VET_FLOAT3, VES_POSITION);
    offset += VertexElement::getTypeSize(VET_FLOAT3);
    decl->addElement(0, offset, VET_FLOAT3, VES_NORMAL);
    offset += VertexElement::getTypeSize(VET_FLOAT3);
    decl->addElement(0, offset, VET_FLOAT2, VES_TEXTURE_COORDINATES, 0);
    offset += VertexElement::getTypeSize(VET_FLOAT2);

    /// Allocate vertex buffer of the requested number of vertices (vertexCount)
    /// and bytes per vertex (offset)
    HardwareVertexBufferSharedPtr vbuf =
        HardwareBufferManager::getSingleton().createVertexBuffer(
            offset, msh->sharedVertexData->vertexCount, HardwareBuffer::HBU_STATIC_WRITE_ONLY);

    /// Upload the vertex data to the card
    vbuf->writeData(0, vbuf->getSizeInBytes(), vertices, true);

    /// Set vertex buffer binding so buffer 0 is bound to our vertex buffer
    VertexBufferBinding* bind = msh->sharedVertexData->vertexBufferBinding;
    bind->setBinding(0, vbuf);

    //for the face
    /// Allocate index buffer of the requested number of vertices (ibufCount)
    HardwareIndexBufferSharedPtr ibuf = HardwareBufferManager::getSingleton().
        createIndexBuffer(
            HardwareIndexBuffer::IT_16BIT,
            ibufCount,
            HardwareBuffer::HBU_STATIC_WRITE_ONLY);

    /// Upload the index data to the card
    ibuf->writeData(0, ibuf->getSizeInBytes(), tris, true);

    /// Set parameters of the submesh
    mainsub->useSharedVertices = true;
    mainsub->indexData->indexBuffer = ibuf;
    mainsub->indexData->indexCount = ibufCount;
    mainsub->indexData->indexStart = 0;

    msh->_setBounds(aab, true);

    /// Notify Mesh object that it has been loaded
    msh->load();

    free(vertices);
};
