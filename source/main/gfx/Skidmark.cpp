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

#include "Skidmark.h"

#include "Application.h"
#include "BeamData.h"
#include "Utils.h"

#include <Ogre.h>

int RoR::Skidmark::m_instance_counter = 0;

RoR::SkidmarkConfig::SkidmarkConfig()
{
    this->LoadDefaultSkidmarkDefs();
}

void RoR::SkidmarkConfig::LoadDefaultSkidmarkDefs()
{
    LOG("[RoR] Loading skidmarks.cfg...");
    Ogre::String group = "";
    try
    {
        group = Ogre::ResourceGroupManager::getSingleton().findGroupContainingResource("skidmarks.cfg");
        if (group.empty())
        {
            LOG("[RoR] Failed to load skidmarks.cfg (file not found)");
            return;
        }

        Ogre::DataStreamPtr ds = Ogre::ResourceGroupManager::getSingleton().openResource("skidmarks.cfg", group);
        Ogre::String line = "";
        Ogre::String currentModel = "";

        while (!ds->eof())
        {
            line = RoR::Utils::SanitizeUtf8String(ds->getLine());
            Ogre::StringUtil::trim(line);

            if (line.empty() || line[0] == ';')
                continue;

            Ogre::StringVector args = Ogre::StringUtil::split(line, ",");

            if (args.size() == 1)
            {
                currentModel = line;
                continue;
            }

            // process the line if we got a model
            if (!currentModel.empty())
                this->ProcessSkidmarkConfLine(args, currentModel);
        }
    }
    catch (...)
    {
        LOG("[RoR] Error loading skidmarks.cfg (unknown error)");
        m_models.clear(); // Delete anything we might have loaded
        return;
    }
    LOG("[RoR] skidmarks.cfg loaded OK");
}

int RoR::SkidmarkConfig::ProcessSkidmarkConfLine(Ogre::StringVector args, Ogre::String modelName)
{
    // we only accept 4 arguments
    if (args.size() != 4)
        return 1;

    // parse the data
    SkidmarkDef cfg;
    cfg.ground = args[0];
    Ogre::StringUtil::trim(cfg.ground);
    cfg.texture = args[1];
    Ogre::StringUtil::trim(cfg.texture);

    cfg.slipFrom = Ogre::StringConverter::parseReal(args[2]);
    cfg.slipTo = Ogre::StringConverter::parseReal(args[3]);

    if (!m_models.size() || m_models.find(modelName) == m_models.end())
        m_models[modelName] = std::vector<SkidmarkDef>();

    m_models[modelName].push_back(cfg);
    return 0;
}

int RoR::SkidmarkConfig::getTexture(Ogre::String model, Ogre::String ground, float slip, Ogre::String& texture)
{
    if (m_models.find(model) == m_models.end())
        return 1;
    for (std::vector<SkidmarkDef>::iterator it = m_models[model].begin(); it != m_models[model].end(); it++)
    {
        if (it->ground == ground && it->slipFrom <= slip && it->slipTo > slip)
        {
            texture = it->texture.c_str();
            return 0;
        }
    }
    return 2;
}

// this is a hardcoded array which we use to map ground types to a certain texture with UV/ coords
Ogre::Vector2 RoR::Skidmark::m_tex_coords[4] = {Ogre::Vector2(0, 0), Ogre::Vector2(0, 1), Ogre::Vector2(1, 0), Ogre::Vector2(1, 1)};

RoR::Skidmark::Skidmark(RoR::SkidmarkConfig* config, wheel_t* m_wheel, 
        Ogre::SceneNode* snode, int m_length /* = 500 */, int m_bucket_count /* = 20 */)
    : m_scene_node(snode)
    , m_is_dirty(true)
    , m_length(m_length)
    , m_bucket_count(m_bucket_count)
    , m_wheel(m_wheel)
    , m_min_distance(0.1f)
    , m_max_distance(std::max(0.5f, m_wheel->wh_width * 1.1f))
    , m_min_distance_squared(m_min_distance * m_min_distance)
    , m_max_distance_squared(m_max_distance * m_max_distance)
    , m_config(config)
{
    if (m_length % 2)
    {
        m_length--;
    }
}

RoR::Skidmark::~Skidmark()
{
    while (m_objects.size() != 0) // Remove all skid segments
        this->PopSegment();
}

void RoR::Skidmark::AddObject(Ogre::Vector3 start, Ogre::String texture)
{
    SkidmarkSegment skid;
    skid.pos = 0;
    skid.lastPointAv = start;
    skid.facecounter = 0;

    // new material
    char bname[256] = "";
    sprintf(bname, "mat-skidmark-%d", m_instance_counter);
    skid.material = Ogre::MaterialManager::getSingleton().create(bname, Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
    Ogre::Pass* p = skid.material->getTechnique(0)->getPass(0);

    p->createTextureUnitState(texture);
    p->setSceneBlending(Ogre::SBT_TRANSPARENT_ALPHA);
    p->setLightingEnabled(false);
    p->setDepthWriteEnabled(false);
    p->setDepthBias(3, 3);
    p->setCullingMode(Ogre::CULL_NONE);

    skid.points.resize(m_length);
    skid.faceSizes.resize(m_length);
    skid.groundTexture.resize(m_length);
    skid.obj = gEnv->sceneManager->createManualObject("skidmark" + TOSTRING(m_instance_counter++));
    skid.obj->setDynamic(true);
    skid.obj->setRenderingDistance(800); // 800m view distance
    skid.obj->begin(bname, Ogre::RenderOperation::OT_TRIANGLE_STRIP);
    for (int i = 0; i < m_length; i++)
    {
        skid.points[i] = start;
        skid.faceSizes[i] = 0;
        skid.groundTexture[i] = "0.png";
        skid.obj->position(start);
        skid.obj->textureCoord(0, 0);
    }
    skid.obj->end();
    m_scene_node->attachObject(skid.obj);

    m_objects.push(skid);

    this->LimitObjects();
}

void RoR::Skidmark::PopSegment()
{
    SkidmarkSegment& skid = m_objects.front();
    skid.points.clear();
    skid.faceSizes.clear();
    Ogre::MaterialManager::getSingleton().remove(skid.material->getName());
    skid.material.setNull();
    gEnv->sceneManager->destroyManualObject(skid.obj);
    m_objects.pop();
}

void RoR::Skidmark::LimitObjects()
{
    if ((int)m_objects.size() > m_bucket_count)
    {
        this->PopSegment();
    }
}

void RoR::Skidmark::SetPointInt(unsigned short index, const Ogre::Vector3& value, Ogre::Real fsize, Ogre::String texture)
{
    m_objects.back().points[index] = value;
    m_objects.back().faceSizes[index] = fsize;
    m_objects.back().groundTexture[index] = texture;

    m_is_dirty = true;
}

void RoR::Skidmark::UpdatePoint(Ogre::Vector3 contact_point, float slip, Ogre::String ground_model_name)
{
    Ogre::Vector3 thisPoint = contact_point;
    Ogre::Vector3 axis = m_wheel->wh_axis_node_1->RelPosition - m_wheel->wh_axis_node_0->RelPosition;
    Ogre::Vector3 thisPointAV = thisPoint + axis * 0.5f;
    Ogre::Real distance = 0;
    Ogre::Real maxDist = m_max_distance;
    Ogre::String texture = "none";
    m_config->getTexture("default", ground_model_name, slip, texture);

    // dont add points with no texture
    if (texture == "none")
        return;

    if (m_wheel->wh_speed > 1)
        maxDist *= m_wheel->wh_speed;

    if (!m_objects.size())
    {
        // add first bucket
        this->AddObject(thisPoint, texture);
    }
    else
    {
        // check existing buckets
        SkidmarkSegment skid = m_objects.back();

        distance = skid.lastPointAv.distance(thisPointAV);
        // too near to update?
        if (distance < m_min_distance)
        {
            //LOG("E: too near for update");
            return;
        }

        // change ground texture if required
        if (skid.pos > 0 && skid.groundTexture[0] != texture)
        {
            // new object with new texture
            if (distance > maxDist)
            {
                // to far away for connection
                this->AddObject(thisPoint, texture);
            }
            else
            {
                // add new bucket with connection to last bucket
                Ogre::Vector3 lp1 = m_objects.back().points[m_objects.back().pos - 1];
                Ogre::Vector3 lp2 = m_objects.back().points[m_objects.back().pos - 2];
                this->AddObject(lp1, texture);
                this->AddPoint(lp2, distance, texture);
                this->AddPoint(lp1, distance, texture);
            }
        }
        else
        {
            // no texture change required :D

            // far enough for new bucket?
            if (skid.pos >= (int)skid.points.size())
            {
                if (distance > maxDist)
                {
                    // to far away for connection
                    this->AddObject(thisPoint, texture);
                }
                else
                {
                    // add new bucket with connection to last bucket
                    Ogre::Vector3 lp1 = m_objects.back().points[m_objects.back().pos - 1];
                    Ogre::Vector3 lp2 = m_objects.back().points[m_objects.back().pos - 2];
                    this->AddObject(lp1, texture);
                    this->AddPoint(lp2, distance, texture);
                    this->AddPoint(lp1, distance, texture);
                }
            }
            else if (distance > m_max_distance)
            {
                // just new bucket, no connection to last bucket
                this->AddObject(thisPoint, texture);
            }
        }
    }

    const float overaxis = 0.2f;

    this->AddPoint(contact_point - (axis * overaxis), distance, texture);
    this->AddPoint(contact_point + axis + (axis * overaxis), distance, texture);

    // save as last point (in the middle of the m_wheel)
    m_objects.back().lastPointAv = thisPointAV;
}

void RoR::Skidmark::AddPoint(const Ogre::Vector3& value, Ogre::Real fsize, Ogre::String texture)
{
    if (m_objects.back().pos >= m_length)
    {
        return;
    }
    this->SetPointInt(m_objects.back().pos, value, fsize, texture);
    m_objects.back().pos++;
}

void RoR::Skidmark::update(Ogre::Vector3 contact_point, float slip, Ogre::String ground_model_name)
{
    this->UpdatePoint(contact_point, slip, ground_model_name);
    if (!m_is_dirty)
        return;
    if (!m_objects.size())
        return;
    SkidmarkSegment skid = m_objects.back();
    Ogre::Vector3 vaabMin = skid.points[0];
    Ogre::Vector3 vaabMax = skid.points[0];
    skid.obj->beginUpdate(0);
    bool behindEnd = false;
    Ogre::Vector3 lastValid = Ogre::Vector3::ZERO;
    int to_counter = 0;
    float tcox_counter = 0;

    for (int i = 0; i < m_length; i++ , to_counter++)
    {
        if (i >= skid.pos)
            behindEnd = true;

        if (to_counter > 3)
            to_counter = 0;

        if (!behindEnd)
            tcox_counter += skid.faceSizes[i] / m_min_distance;

        while (tcox_counter > 1)
            tcox_counter--;

        if (behindEnd)
        {
            skid.obj->position(lastValid);
            skid.obj->textureCoord(0, 0);
        }
        else
        {
            skid.obj->position(skid.points[i]);

            Ogre::Vector2 tco = m_tex_coords[to_counter];
            tco.x *= skid.faceSizes[i] / m_min_distance; // scale texture according face size
            skid.obj->textureCoord(tco);

            lastValid = skid.points[i];
        }

        if (skid.points[i].x < vaabMin.x)
            vaabMin.x = skid.points[i].x;
        if (skid.points[i].y < vaabMin.y)
            vaabMin.y = skid.points[i].y;
        if (skid.points[i].z < vaabMin.z)
            vaabMin.z = skid.points[i].z;
        if (skid.points[i].x > vaabMax.x)
            vaabMax.x = skid.points[i].x;
        if (skid.points[i].y > vaabMax.y)
            vaabMax.y = skid.points[i].y;
        if (skid.points[i].z > vaabMax.z)
            vaabMax.z = skid.points[i].z;
    }
    skid.obj->end();

    skid.obj->setBoundingBox(Ogre::AxisAlignedBox(vaabMin, vaabMax));

    m_is_dirty = false;
}
