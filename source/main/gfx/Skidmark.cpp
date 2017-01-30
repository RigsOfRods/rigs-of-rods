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

#include <Ogre.h>

#include "BeamData.h"
#include "IHeightFinder.h"
#include "Settings.h"
#include "Utils.h"

using namespace Ogre;

int Skidmark::instanceCounter = 0;

SkidmarkManager::SkidmarkManager()
{
    LOG("SkidmarkManager created");
    loadDefaultModels();
}

SkidmarkManager::~SkidmarkManager()
{
    LOG("SkidmarkManager destroyed");
}

int SkidmarkManager::loadDefaultModels()
{
    LOG("SkidmarkManager loading default m_models");
    // check if we have a config file
    String group = "";
    try
    {
        group = ResourceGroupManager::getSingleton().findGroupContainingResource("skidmarks.cfg");
    }
    catch (...)
    {
    }
    // emit a warning if we did not found the file
    if (group.empty())
    {
        LOG("skidmarks| skidmarks.cfg not found");
        return 1;
    }

    // open the file for reading
    DataStreamPtr ds = ResourceGroupManager::getSingleton().openResource("skidmarks.cfg", group);
    String line = "";
    String currentModel = "";

    while (!ds->eof())
    {
        line = RoR::Utils::SanitizeUtf8String(ds->getLine());
        StringUtil::trim(line);

        if (line.empty() || line[0] == ';')
            continue;

        StringVector args = StringUtil::split(line, ",");

        if (args.size() == 1)
        {
            currentModel = line;
            continue;
        }

        // process the line if we got a model
        if (!currentModel.empty())
            processLine(args, currentModel);
    }
    return 0;
}

int SkidmarkManager::processLine(StringVector args, String modelName)
{
    // we only accept 4 arguments
    if (args.size() != 4)
        return 1;

    // parse the data
    SkidmarkConf cfg;
    cfg.ground = args[0];
    StringUtil::trim(cfg.ground);
    cfg.texture = args[1];
    StringUtil::trim(cfg.texture);

    cfg.slipFrom = StringConverter::parseReal(args[2]);
    cfg.slipTo = StringConverter::parseReal(args[3]);

    if (!m_models.size() || m_models.find(modelName) == m_models.end())
        m_models[modelName] = std::vector<SkidmarkConf>();

    m_models[modelName].push_back(cfg);
    return 0;
}

int SkidmarkManager::getTexture(String model, String ground, float slip, String& texture)
{
    if (m_models.find(model) == m_models.end())
        return 1;
    for (std::vector<SkidmarkConf>::iterator it = m_models[model].begin(); it != m_models[model].end(); it++)
    {
        if (it->ground == ground && it->slipFrom <= slip && it->slipTo > slip)
        {
            texture = it->texture.c_str();
            return 0;
        }
    }
    return 2;
}

/////////////// Skidmark below

// this is a hardcoded array which we use to map ground types to a certain texture with UV/ coords
Vector2 Skidmark::m_tex_coords[4] = {Vector2(0, 0), Vector2(0, 1), Vector2(1, 0), Vector2(1, 1)};

Skidmark::Skidmark(wheel_t* m_wheel, Ogre::SceneNode* snode, int m_length /* = 500 */, int m_bucket_count /* = 20 */) :
    m_scene_node(snode)
    , m_is_dirty(true)
    , m_length(m_length)
    , m_bucket_count(m_bucket_count)
    , m_wheel(m_wheel)
    , m_min_distance(0.1f)
    , m_max_distance(std::max(0.5f, m_wheel->width * 1.1f))
    , m_min_distance_squared(m_min_distance * m_min_distance)
    , m_max_distance_squared(m_max_distance * m_max_distance)
{
    if (m_length % 2)
    {
        m_length--;
    }

    //Configurable limits of skidmarks
    int c_BucketCount = ISETTING("SkidmarksBuckets", 0);

    if (c_BucketCount > 0)
    {
        m_bucket_count = c_BucketCount;
    }
}

Skidmark::~Skidmark()
{
}

void Skidmark::addObject(Vector3 start, String texture)
{
    //LOG("new skidmark section");
    SkidmarkObject skid;
    skid.pos = 0;
    skid.lastPointAv = start;
    skid.facecounter = 0;

    skid.colour = ColourValue(Math::RangeRandom(0, 100) / 100.0f, Math::RangeRandom(0, 100) / 100.0f, Math::RangeRandom(0, 100) / 100.0f, 0.8f);

    // new material
    char bname[256] = "";
    sprintf(bname, "mat-skidmark-%d", instanceCounter);
    MaterialPtr mat = (MaterialPtr)(MaterialManager::getSingleton().create(bname, ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME));
    Pass* p = mat->getTechnique(0)->getPass(0);

    TextureUnitState* tus = p->createTextureUnitState(texture);
    p->setSceneBlending(SBT_TRANSPARENT_ALPHA);
    p->setLightingEnabled(false);
    p->setDepthWriteEnabled(false);
    p->setDepthBias(3, 3);
    p->setCullingMode(CULL_NONE);

    skid.points.resize(m_length);
    skid.faceSizes.resize(m_length);
    skid.groundTexture.resize(m_length);
    skid.obj = gEnv->sceneManager->createManualObject("skidmark" + TOSTRING(instanceCounter++));
    skid.obj->setDynamic(true);
    skid.obj->setRenderingDistance(800); // 800m view distance
    skid.obj->begin(bname, RenderOperation::OT_TRIANGLE_STRIP);
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

    limitObjects();
}

void Skidmark::limitObjects()
{
    if ((int)m_objects.size() > m_bucket_count)
    {
        //LOG("deleting first skidmarks section to keep the limits");
        m_objects.front().points.clear();
        m_objects.front().faceSizes.clear();
        gEnv->sceneManager->destroyManualObject(m_objects.front().obj);
        m_objects.pop();
    }
}

void Skidmark::setPointInt(unsigned short index, const Vector3& value, Real fsize, String texture)
{
    m_objects.back().points[index] = value;
    m_objects.back().faceSizes[index] = fsize;
    m_objects.back().groundTexture[index] = texture;

    m_is_dirty = true;
}

void Skidmark::updatePoint()
{
    Vector3 thisPoint = m_wheel->lastContactType ? m_wheel->lastContactOuter : m_wheel->lastContactInner;
    Vector3 axis = m_wheel->lastContactType ? (m_wheel->refnode1->RelPosition - m_wheel->refnode0->RelPosition) : (m_wheel->refnode0->RelPosition - m_wheel->refnode1->RelPosition);
    Vector3 thisPointAV = thisPoint + axis * 0.5f;
    Real distance = 0;
    Real maxDist = m_max_distance;
    String texture = "none";
    SkidmarkManager::getSingleton().getTexture("default", m_wheel->lastGroundModel->name, m_wheel->lastSlip, texture);

    // dont add points with no texture
    if (texture == "none")
        return;

    if (m_wheel->speed > 1)
        maxDist *= m_wheel->speed;

    if (!m_objects.size())
    {
        // add first bucket
        addObject(thisPoint, texture);
    }
    else
    {
        // check existing buckets
        SkidmarkObject skid = m_objects.back();

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
                addObject(thisPoint, texture);
            }
            else
            {
                // add new bucket with connection to last bucket
                Vector3 lp1 = m_objects.back().points[m_objects.back().pos - 1];
                Vector3 lp2 = m_objects.back().points[m_objects.back().pos - 2];
                addObject(lp1, texture);
                addPoint(lp2, distance, texture);
                addPoint(lp1, distance, texture);
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
                    addObject(thisPoint, texture);
                }
                else
                {
                    // add new bucket with connection to last bucket
                    Vector3 lp1 = m_objects.back().points[m_objects.back().pos - 1];
                    Vector3 lp2 = m_objects.back().points[m_objects.back().pos - 2];
                    addObject(lp1, texture);
                    addPoint(lp2, distance, texture);
                    addPoint(lp1, distance, texture);
                }
            }
            else if (distance > m_max_distance)
            {
                // just new bucket, no connection to last bucket
                addObject(thisPoint, texture);
            }
        }
    }

    SkidmarkObject skid = m_objects.back();

    float overaxis = 0.2f;
    // tactics: we always choose the latest oint and then create two points

    // choose node m_wheel by the latest added point
    if (!m_wheel->lastContactType)
    {
        // choose inner
        addPoint(m_wheel->lastContactInner - (axis * overaxis), distance, texture);
        addPoint(m_wheel->lastContactInner + axis + (axis * overaxis), distance, texture);
    }
    else
    {
        // choose outer
        addPoint(m_wheel->lastContactOuter + axis + (axis * overaxis), distance, texture);
        addPoint(m_wheel->lastContactOuter - (axis * overaxis), distance, texture);
    }

    // save as last point (in the middle of the m_wheel)
    m_objects.back().lastPointAv = thisPointAV;
}

void Skidmark::addPoint(const Vector3& value, Real fsize, String texture)
{
    if (m_objects.back().pos >= m_length)
    {
        //LOG("E: boundary protection hit");
        return;
    }
    setPointInt(m_objects.back().pos, value, fsize, texture);
    m_objects.back().pos++;
}

void Skidmark::update()
{
    if (!m_objects.size())
        return;
    SkidmarkObject skid = m_objects.back();
    Vector3 vaabMin = skid.points[0];
    Vector3 vaabMax = skid.points[0];
    skid.obj->beginUpdate(0);
    bool behindEnd = false;
    Vector3 lastValid = Vector3::ZERO;
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

            Vector2 tco = m_tex_coords[to_counter];
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

    skid.obj->setBoundingBox(AxisAlignedBox(vaabMin, vaabMax));

    m_is_dirty = false;
}
