/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2013-2017 Petr Ohlidal & contributors

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

#include "Collisions.h"

#include "Application.h"
#include "ApproxMath.h"
#include "BeamFactory.h"
#include "ErrorUtils.h"
#include "Landusemap.h"
#include "Language.h"
#include "MovableText.h"
#include "PlatformUtils.h"
#include "RoRFrameListener.h"
#include "Scripting.h"
#include "Settings.h"
#include "TerrainManager.h"

// some gcc fixes
#if OGRE_PLATFORM == OGRE_PLATFORM_LINUX
#pragma GCC diagnostic ignored "-Wfloat-equal"
#endif //OGRE_PLATFORM_LINUX

//hash function SBOX
//from http://home.comcast.net/~bretm/hash/10.html
unsigned int sbox[] =
{
    0xF53E1837, 0x5F14C86B, 0x9EE3964C, 0xFA796D53,
    0x32223FC3, 0x4D82BC98, 0xA0C7FA62, 0x63E2C982,
    0x24994A5B, 0x1ECE7BEE, 0x292B38EF, 0xD5CD4E56,
    0x514F4303, 0x7BE12B83, 0x7192F195, 0x82DC7300,
    0x084380B4, 0x480B55D3, 0x5F430471, 0x13F75991,
    0x3F9CF22C, 0x2FE0907A, 0xFD8E1E69, 0x7B1D5DE8,
    0xD575A85C, 0xAD01C50A, 0x7EE00737, 0x3CE981E8,
    0x0E447EFA, 0x23089DD6, 0xB59F149F, 0x13600EC7,
    0xE802C8E6, 0x670921E4, 0x7207EFF0, 0xE74761B0,
    0x69035234, 0xBFA40F19, 0xF63651A0, 0x29E64C26,
    0x1F98CCA7, 0xD957007E, 0xE71DDC75, 0x3E729595,
    0x7580B7CC, 0xD7FAF60B, 0x92484323, 0xA44113EB,
    0xE4CBDE08, 0x346827C9, 0x3CF32AFA, 0x0B29BCF1,
    0x6E29F7DF, 0xB01E71CB, 0x3BFBC0D1, 0x62EDC5B8,
    0xB7DE789A, 0xA4748EC9, 0xE17A4C4F, 0x67E5BD03,
    0xF3B33D1A, 0x97D8D3E9, 0x09121BC0, 0x347B2D2C,
    0x79A1913C, 0x504172DE, 0x7F1F8483, 0x13AC3CF6,
    0x7A2094DB, 0xC778FA12, 0xADF7469F, 0x21786B7B,
    0x71A445D0, 0xA8896C1B, 0x656F62FB, 0x83A059B3,
    0x972DFE6E, 0x4122000C, 0x97D9DA19, 0x17D5947B,
    0xB1AFFD0C, 0x6EF83B97, 0xAF7F780B, 0x4613138A,
    0x7C3E73A6, 0xCF15E03D, 0x41576322, 0x672DF292,
    0xB658588D, 0x33EBEFA9, 0x938CBF06, 0x06B67381,
    0x07F192C6, 0x2BDA5855, 0x348EE0E8, 0x19DBB6E3,
    0x3222184B, 0xB69D5DBA, 0x7E760B88, 0xAF4D8154,
    0x007A51AD, 0x35112500, 0xC9CD2D7D, 0x4F4FB761,
    0x694772E3, 0x694C8351, 0x4A7E3AF5, 0x67D65CE1,
    0x9287DE92, 0x2518DB3C, 0x8CB4EC06, 0xD154D38F,
    0xE19A26BB, 0x295EE439, 0xC50A1104, 0x2153C6A7,
    0x82366656, 0x0713BC2F, 0x6462215A, 0x21D9BFCE,
    0xBA8EACE6, 0xAE2DF4C1, 0x2A8D5E80, 0x3F7E52D1,
    0x29359399, 0xFEA1D19C, 0x18879313, 0x455AFA81,
    0xFADFE838, 0x62609838, 0xD1028839, 0x0736E92F,
    0x3BCA22A3, 0x1485B08A, 0x2DA7900B, 0x852C156D,
    0xE8F24803, 0x00078472, 0x13F0D332, 0x2ACFD0CF,
    0x5F747F5C, 0x87BB1E2F, 0xA7EFCB63, 0x23F432F0,
    0xE6CE7C5C, 0x1F954EF6, 0xB609C91B, 0x3B4571BF,
    0xEED17DC0, 0xE556CDA0, 0xA7846A8D, 0xFF105F94,
    0x52B7CCDE, 0x0E33E801, 0x664455EA, 0xF2C70414,
    0x73E7B486, 0x8F830661, 0x8B59E826, 0xBB8AEDCA,
    0xF3D70AB9, 0xD739F2B9, 0x4A04C34A, 0x88D0F089,
    0xE02191A2, 0xD89D9C78, 0x192C2749, 0xFC43A78F,
    0x0AAC88CB, 0x9438D42D, 0x9E280F7A, 0x36063802,
    0x38E8D018, 0x1C42A9CB, 0x92AAFF6C, 0xA24820C5,
    0x007F077F, 0xCE5BC543, 0x69668D58, 0x10D6FF74,
    0xBE00F621, 0x21300BBE, 0x2E9E8F46, 0x5ACEA629,
    0xFA1F86C7, 0x52F206B8, 0x3EDF1A75, 0x6DA8D843,
    0xCF719928, 0x73E3891F, 0xB4B95DD6, 0xB2A42D27,
    0xEDA20BBF, 0x1A58DBDF, 0xA449AD03, 0x6DDEF22B,
    0x900531E6, 0x3D3BFF35, 0x5B24ABA2, 0x472B3E4C,
    0x387F2D75, 0x4D8DBA36, 0x71CB5641, 0xE3473F3F,
    0xF6CD4B7F, 0xBF7D1428, 0x344B64D0, 0xC5CDFCB6,
    0xFE2E0182, 0x2C37A673, 0xDE4EB7A3, 0x63FDC933,
    0x01DC4063, 0x611F3571, 0xD167BFAF, 0x4496596F,
    0x3DEE0689, 0xD8704910, 0x7052A114, 0x068C9EC5,
    0x75D0E766, 0x4D54CC20, 0xB44ECDE2, 0x4ABC653E,
    0x2C550A21, 0x1A52C0DB, 0xCFED03D0, 0x119BAFE2,
    0x876A6133, 0xBC232088, 0x435BA1B2, 0xAE99BBFA,
    0xBB4F08E4, 0xA62B5F49, 0x1DA4B695, 0x336B84DE,
    0xDC813D31, 0x00C134FB, 0x397A98E6, 0x151F0E64,
    0xD9EB3E69, 0xD3C7DF60, 0xD2F2C336, 0x2DDD067B,
    0xBD122835, 0xB0B3BD3A, 0xB0D54E46, 0x8641F1E4,
    0xA0B38F96, 0x51D39199, 0x37A6AD75, 0xDF84EE41,
    0x3C034CBA, 0xACDA62FC, 0x11923B8B, 0x45EF170A,
};

using namespace Ogre;
using namespace RoR;

Collisions::Collisions():
      debugMode(false)
    , forcecam(false)
    , free_eventsource(0)
    , hashmask(0)
    , landuse(0)
    , m_terrain_size(App::GetSimTerrain()->getMaxTerrainSize())
{
    debugMode = RoR::App::diag_collisions.GetActive(); // TODO: make interactive - do not copy the value, use GVar directly
    for (int i=0; i < HASH_POWER; i++)
    {
        hashmask = hashmask << 1;
        hashmask++;
    }

    loadDefaultModels();
    defaultgm = getGroundModelByString("concrete");
    defaultgroundgm = getGroundModelByString("gravel");

    if (debugMode)
    {
        debugmo = gEnv->sceneManager->createManualObject();
        debugmo->begin("tracks/debug/collision/triangle", RenderOperation::OT_TRIANGLE_LIST);
    }
}

Collisions::~Collisions()
{
}

int Collisions::loadDefaultModels()
{
    return loadGroundModelsConfigFile(std::string(RoR::App::sys_config_dir.GetActive()) + RoR::PATH_SLASH + "ground_models.cfg");
}

int Collisions::loadGroundModelsConfigFile(Ogre::String filename)
{
    String group = "";
    try
    {
        group = ResourceGroupManager::getSingleton().findGroupContainingResource(filename);
    }
    catch (...)
    {
        // we wont catch anything, since the path could be absolute as well, then the group is not found
    }

    Ogre::ConfigFile cfg;
    try
    {
        // try to load directly otherwise via resource group
        if (group == "")
            cfg.loadDirect(filename);
        else
            cfg.loadFromResourceSystem(filename, group, "\x09:=", true);
    }
    catch (Ogre::Exception& e)
    {
        ErrorUtils::ShowError("Error while loading ground model", e.getFullDescription());
        return 1;
    }

    // parse the whole config
    parseGroundConfig(&cfg);

    // after it was parsed, resolve the dependencies
    std::map<Ogre::String, ground_model_t>::iterator it;
    for (it=ground_models.begin(); it!=ground_models.end(); it++)
    {
        if (!it->second.basename) continue; // no base, normal material
        String bname = String(it->second.basename);
        if (ground_models.find(bname) == ground_models.end()) continue; // base not found!
        // copy the values from the base if not set otherwise
        ground_model_t *thisgm = &(it->second);
        ground_model_t *basegm = &ground_models[bname];
        memcpy(thisgm, basegm, sizeof(ground_model_t));
        // re-set the name
        strncpy(thisgm->name, it->first.c_str(), 255);
        // after that we need to reload the config to overwrite settings of the base
        parseGroundConfig(&cfg, it->first);
    }
    // check the version
    if (this->collision_version != LATEST_GROUND_MODEL_VERSION)
    {
        ErrorUtils::ShowError(_L("Configuration error"), _L("Your ground configuration is too old, please copy skeleton/config/ground_models.cfg to My Documents/Rigs of Rods/config"));
        exit(124);
    }
    return 0;
}


void Collisions::parseGroundConfig(Ogre::ConfigFile *cfg, String groundModel)
{
    Ogre::ConfigFile::SectionIterator seci = cfg->getSectionIterator();

    Ogre::String secName, kname, kvalue;
    while (seci.hasMoreElements())
    {
        secName = seci.peekNextKey();
        Ogre::ConfigFile::SettingsMultiMap *settings = seci.getNext();

        if (!groundModel.empty() && secName != groundModel) continue;

        Ogre::ConfigFile::SettingsMultiMap::iterator i;
        for (i = settings->begin(); i != settings->end(); ++i)
        {
            kname = i->first;
            kvalue = i->second;
            // we got all the data available now, processing now
            if (secName == "general" || secName == "config")
            {
                // set some class properties accoring to the information in this section
                if (kname == "version") this->collision_version = StringConverter::parseInt(kvalue);
            } else
            {
                // we assume that all other sections are separate ground types!
                if (ground_models.find(secName) == ground_models.end())
                {
                    // ground models not known yet, init it!
                    ground_models[secName] = ground_model_t();
                    // clear it
                    memset(&ground_models[secName], 0, sizeof(ground_model_t));
                    // set some default values
                    ground_models[secName].alpha = 2.0f;
                    ground_models[secName].strength = 1.0f;
                    // some fx defaults
                    ground_models[secName].fx_particle_amount = 20;
                    ground_models[secName].fx_particle_min_velo = 5;
                    ground_models[secName].fx_particle_max_velo = 99999;
                    ground_models[secName].fx_particle_velo_factor = 0.7f;
                    ground_models[secName].fx_particle_fade = -1;
                    ground_models[secName].fx_particle_timedelta = 1;
                    ground_models[secName].fx_particle_ttl = 2;
                    strncpy(ground_models[secName].name, secName.c_str(), 255);

                }

                if (kname == "adhesion velocity") ground_models[secName].va = StringConverter::parseReal(kvalue);
                else if (kname == "static friction coefficient") ground_models[secName].ms = StringConverter::parseReal(kvalue);
                else if (kname == "sliding friction coefficient") ground_models[secName].mc = StringConverter::parseReal(kvalue);
                else if (kname == "hydrodynamic friction") ground_models[secName].t2 = StringConverter::parseReal(kvalue);
                else if (kname == "stribeck velocity") ground_models[secName].vs = StringConverter::parseReal(kvalue);
                else if (kname == "alpha") ground_models[secName].alpha = StringConverter::parseReal(kvalue);
                else if (kname == "strength") ground_models[secName].strength = StringConverter::parseReal(kvalue);
                else if (kname == "base") strncpy(ground_models[secName].basename, kvalue.c_str(), 255);
                else if (kname == "fx_type")
                {
                    if (kvalue == "PARTICLE")
                        ground_models[secName].fx_type = FX_PARTICLE;
                    else if (kvalue == "HARD")
                        ground_models[secName].fx_type = FX_HARD;
                    else if (kvalue == "DUSTY")
                        ground_models[secName].fx_type = FX_DUSTY;
                    else if (kvalue == "CLUMPY")
                        ground_models[secName].fx_type = FX_CLUMPY;
                }
                else if (kname == "fx_particle_name") strncpy(ground_models[secName].particle_name, kvalue.c_str(), 255);
                else if (kname == "fx_colour") ground_models[secName].fx_colour = StringConverter::parseColourValue(kvalue);
                else if (kname == "fx_particle_amount") ground_models[secName].fx_particle_amount = StringConverter::parseInt(kvalue);
                else if (kname == "fx_particle_min_velo") ground_models[secName].fx_particle_min_velo = StringConverter::parseReal(kvalue);
                else if (kname == "fx_particle_max_velo") ground_models[secName].fx_particle_max_velo = StringConverter::parseReal(kvalue);
                else if (kname == "fx_particle_fade") ground_models[secName].fx_particle_fade = StringConverter::parseReal(kvalue);
                else if (kname == "fx_particle_timedelta") ground_models[secName].fx_particle_timedelta = StringConverter::parseReal(kvalue);
                else if (kname == "fx_particle_velo_factor") ground_models[secName].fx_particle_velo_factor = StringConverter::parseReal(kvalue);
                else if (kname == "fx_particle_ttl") ground_models[secName].fx_particle_ttl = StringConverter::parseReal(kvalue);


                else if (kname == "fluid density") ground_models[secName].fluid_density = StringConverter::parseReal(kvalue);
                else if (kname == "flow consistency index") ground_models[secName].flow_consistency_index = StringConverter::parseReal(kvalue);
                else if (kname == "flow behavior index") ground_models[secName].flow_behavior_index = StringConverter::parseReal(kvalue);
                else if (kname == "solid ground level") ground_models[secName].solid_ground_level = StringConverter::parseReal(kvalue);
                else if (kname == "drag anisotropy") ground_models[secName].drag_anisotropy = StringConverter::parseReal(kvalue);

            }
        }

        if (!groundModel.empty()) break; // we dont need to go through the other sections
    }
}

Ogre::Vector3 Collisions::calcCollidedSide(const Ogre::Vector3& pos, const Ogre::Vector3& lo, const Ogre::Vector3& hi)
{	
    Ogre::Real min = pos.x - lo.x;
    Ogre::Vector3 newPos = Ogre::Vector3(lo.x, pos.y, pos.z);
    
    Ogre::Real t = pos.y - lo.y;
    if (t < min) {
        min=t;
        newPos = Ogre::Vector3(pos.x, lo.y, pos.z);
    }
    
    t = pos.z - lo.z;
    if (t < min) {
        min=t;
        newPos = Ogre::Vector3(pos.x, pos.y, lo.z);
    }
    
    t = hi.x - pos.x;
    if (t < min) {
        min=t;
        newPos = Ogre::Vector3(hi.x, pos.y, pos.z);
    }
    
    t = hi.y - pos.y;
    if (t < min) {
        min=t;
        newPos = Ogre::Vector3(pos.x, hi.y, pos.z);
    }
    
    t = hi.z - pos.z;
    if (t < min) {
        min=t;
        newPos = Ogre::Vector3(pos.x, pos.y, hi.z);
    }
    
    return newPos;
}

void Collisions::setupLandUse(const char *configfile)
{
#ifdef USE_PAGED
    if (landuse) return;
    landuse = new Landusemap(configfile);
#else
    LOG("RoR was not compiled with PagedGeometry support. You cannot use Landuse maps with it.");
#endif //USE_PAGED
}

ground_model_t *Collisions::getGroundModelByString(const String name)
{
    if (!ground_models.size() || ground_models.find(name) == ground_models.end())
        return 0;

    return &ground_models[name];
}

unsigned int Collisions::hashfunc(unsigned int cellid)
{
    unsigned int hash = 0;
    for (int i=0; i < 4; i++)
    {
        hash ^= sbox[((unsigned char*)&cellid)[i]];
        hash *= 3;
    }
    return hash&hashmask;
}

void Collisions::hash_add(int cell_x, int cell_z, int value, float h)
{
    unsigned int cell_id = (cell_x << 16) + cell_z;
    unsigned int pos    = hashfunc(cell_id);

    hashtable[pos].emplace_back(cell_id, value);
    hashtable_height[pos] = std::max(hashtable_height[pos], h);
}

int Collisions::hash_find(int cell_x, int cell_z)
{
    unsigned int cellid = (cell_x << 16) + cell_z;
    unsigned int pos    = hashfunc(cellid);

    return static_cast<int>(pos);
}

int Collisions::addCollisionBox(SceneNode *tenode, bool rotating, bool virt, Vector3 pos, Ogre::Vector3 rot, Ogre::Vector3 l, Ogre::Vector3 h, Ogre::Vector3 sr, const Ogre::String &eventname, const Ogre::String &instancename, bool forcecam, Ogre::Vector3 campos, Ogre::Vector3 sc /* = Vector3::UNIT_SCALE */, Ogre::Vector3 dr /* = Vector3::ZERO */, int event_filter /* = EVENT_ALL */, int scripthandler /* = -1 */)
{
    Quaternion rotation  = Quaternion(Degree(rot.x), Vector3::UNIT_X) * Quaternion(Degree(rot.y), Vector3::UNIT_Y) * Quaternion(Degree(rot.z), Vector3::UNIT_Z);
    Quaternion direction = Quaternion(Degree(dr.x), Vector3::UNIT_X) * Quaternion(Degree(dr.y), Vector3::UNIT_Y) * Quaternion(Degree(dr.z), Vector3::UNIT_Z);
    int coll_box_index = this->GetNumCollisionBoxes();
    collision_box_t coll_box;

    coll_box.enabled = true;
    
    // set refined box anyway
    coll_box.relo = l*sc;
    coll_box.rehi = h*sc;

    // calculate selfcenter anyway
    coll_box.selfcenter  = coll_box.relo;
    coll_box.selfcenter += coll_box.rehi;
    coll_box.selfcenter *= 0.5;
    
    // and center too (we need it)
    coll_box.center = pos;
    coll_box.virt = virt;
    coll_box.event_filter = event_filter;

    // camera stuff
    coll_box.camforced = forcecam;
    if (forcecam)
    {
        coll_box.campos = coll_box.center + rotation * campos;
    }

    // first, self-rotate
    if (rotating)
    {
        // we have a self-rotated block
        coll_box.selfrotated = true;
        coll_box.selfrot     = Quaternion(Degree(sr.x), Vector3::UNIT_X) * Quaternion(Degree(sr.y), Vector3::UNIT_Y) * Quaternion(Degree(sr.z), Vector3::UNIT_Z);
        coll_box.selfunrot   = coll_box.selfrot.Inverse();
    } else
    {
        coll_box.selfrotated = false;
    }

    coll_box.eventsourcenum = -1;

    if (!eventname.empty())
    {
        //LOG("COLL: adding "+TOSTRING(free_eventsource)+" "+String(instancename)+" "+String(eventname));
        // this is event-generating
        strcpy(eventsources[free_eventsource].boxname, eventname.c_str());
        strcpy(eventsources[free_eventsource].instancename, instancename.c_str());
        eventsources[free_eventsource].scripthandler = scripthandler;
        eventsources[free_eventsource].cbox = coll_box_index;
        eventsources[free_eventsource].snode = tenode;
        eventsources[free_eventsource].direction = direction;
        eventsources[free_eventsource].enabled = true;
        coll_box.eventsourcenum = free_eventsource;
        free_eventsource++;
    }

    // next, global rotate
    if (fabs(rot.x) < 0.0001f && fabs(rot.y) < 0.0001f && fabs(rot.z) < 0.0001f)
    {
        // unrefined box
        coll_box.refined = false;
    } else
    {
        // refined box
        coll_box.refined = true;
        // build rotation
        coll_box.rot   = rotation;
        coll_box.unrot = rotation.Inverse();
    }

    SceneNode *debugsn = 0;
    
    if (debugMode)
    {
        debugsn = gEnv->sceneManager->getRootSceneNode()->createChildSceneNode();
    }

    // set raw box
    // 8 points of a cube
    Vector3 cube_points[8];
    if (coll_box.selfrotated || coll_box.refined)
    {
        cube_points[0] = Ogre::Vector3(l.x, l.y, l.z) * sc;
        cube_points[1] = Ogre::Vector3(h.x, l.y, l.z) * sc;
        cube_points[2] = Ogre::Vector3(l.x, h.y, l.z) * sc;
        cube_points[3] = Ogre::Vector3(h.x, h.y, l.z) * sc;
        cube_points[4] = Ogre::Vector3(l.x, l.y, h.z) * sc;
        cube_points[5] = Ogre::Vector3(h.x, l.y, h.z) * sc;
        cube_points[6] = Ogre::Vector3(l.x, h.y, h.z) * sc;
        cube_points[7] = Ogre::Vector3(h.x, h.y, h.z) * sc;
        
        // rotate box
        if (coll_box.selfrotated)
            for (int i=0; i < 8; i++)
            {
                cube_points[i]=cube_points[i]-coll_box.selfcenter;
                cube_points[i]=coll_box.selfrot*cube_points[i];
                cube_points[i]=cube_points[i]+coll_box.selfcenter;
            }
            if (coll_box.refined)
            {
                for (int i=0; i < 8; i++)
                {
                    cube_points[i] = coll_box.rot * cube_points[i];
                }
            }
            // find min/max
            coll_box.lo = cube_points[0];
            coll_box.hi = cube_points[0];
            for (int i=1; i < 8; i++)
            {
                coll_box.lo.makeFloor(cube_points[i]);
                coll_box.hi.makeCeil(cube_points[i]);
            }
            // set absolute coords
            coll_box.lo += pos;
            coll_box.hi += pos;
    } else
    {
        // unrefined box
        coll_box.lo = pos + coll_box.relo;
        coll_box.hi = pos + coll_box.rehi;
        Vector3 d = (coll_box.rehi - coll_box.relo);
        cube_points[0] = coll_box.relo;
        cube_points[1] = coll_box.relo;	cube_points[1].x += d.x;
        cube_points[2] = coll_box.relo;                          cube_points[2].y += d.y;
        cube_points[3] = coll_box.relo; cube_points[3].x += d.x; cube_points[3].y += d.y;
        cube_points[4] = coll_box.relo;                                                   cube_points[4].z += d.z;
        cube_points[5] = coll_box.relo; cube_points[5].x += d.x;                          cube_points[5].z += d.z;
        cube_points[6] = coll_box.relo; cube_points[6].y += d.y;                          cube_points[6].z += d.z;
        cube_points[7] = coll_box.relo; cube_points[7].x += d.x; cube_points[7].y += d.y; cube_points[7].z += d.z;
    }

    if (debugsn)
    {
        debugsn->setPosition(pos);
        // box content
        ManualObject *mo = gEnv->sceneManager->createManualObject();
        String matName = "tracks/debug/collision/box";
        if (virt && scripthandler == -1)
            matName = "tracks/debug/eventbox/unused";
        else if (virt)
            matName = "tracks/debug/eventbox/used";
        AxisAlignedBox *aa = new AxisAlignedBox();
        for (int i=0; i < 8; i++)
        {
            aa->merge(cube_points[i]);
        }
        mo->begin(matName, Ogre::RenderOperation::OT_TRIANGLE_LIST);
        mo->position(cube_points[0]);
        mo->position(cube_points[1]);
        mo->position(cube_points[2]);
        mo->position(cube_points[3]);
        mo->position(cube_points[4]);
        mo->position(cube_points[5]);
        mo->position(cube_points[6]);
        mo->position(cube_points[7]);

        // front
        mo->triangle(0,1,2);
        mo->triangle(1,3,2);
        // right side
        mo->triangle(3,1,5);
        mo->triangle(5,7,3);
        // left side
        mo->triangle(6,4,0);
        mo->triangle(0,2,6);
        // back side
        mo->triangle(7,5,4);
        mo->triangle(4,6,7);
        // bottom
        mo->triangle(5,4,1);
        mo->triangle(4,0,1);
        // top
        mo->triangle(2,3,6);
        mo->triangle(3,7,6);

        mo->end();
        mo->setBoundingBox(*aa);
        mo->setRenderingDistance(200);
        debugsn->attachObject(mo);

        // the border
        mo = gEnv->sceneManager->createManualObject();
        mo->begin(matName, Ogre::RenderOperation::OT_LINE_LIST);
        mo->position(cube_points[0]);
        mo->position(cube_points[1]);
        mo->position(cube_points[2]);
        mo->position(cube_points[3]);
        mo->position(cube_points[4]);
        mo->position(cube_points[5]);
        mo->position(cube_points[6]);
        mo->position(cube_points[7]);
        //front
        mo->index(0);mo->index(1); mo->index(1);mo->index(3); mo->index(3);mo->index(2); mo->index(2);mo->index(0);
        // right side
        mo->index(1);mo->index(5); mo->index(5);mo->index(7); mo->index(7);mo->index(3); mo->index(3);mo->index(1);
        // left side
        mo->index(0);mo->index(2); mo->index(2);mo->index(6); mo->index(6);mo->index(4); mo->index(4);mo->index(0);
        // back side
        mo->index(5);mo->index(4); mo->index(4);mo->index(6); mo->index(6);mo->index(7); mo->index(7);mo->index(5);
        // bottom and top not needed
        mo->end();
        mo->setBoundingBox(*aa);
        debugsn->attachObject(mo);
        mo->setRenderingDistance(200);
        delete(aa);

        // label
        // setup a label
        if (virt)
        {
            String labelName = "collision_box_label_"+TOSTRING(coll_box_index);
            String labelCaption = "EVENTBOX\nevent:"+String(eventname) + "\ninstance:" + String(instancename);
            if (scripthandler != -1)
                labelCaption += "\nhandler:" + TOSTRING(scripthandler);
            MovableText *mt = new MovableText(labelName, labelCaption);
            mt->setFontName("highcontrast_black");
            mt->setTextAlignment(MovableText::H_CENTER, MovableText::V_ABOVE);
            mt->setAdditionalHeight(1);
            mt->showOnTop(false);
            mt->setCharacterHeight(0.3);
            mt->setColor(ColourValue::Black);
            mt->setRenderingDistance(200);

            SceneNode *n2 = gEnv->sceneManager->getRootSceneNode()->createChildSceneNode();
            n2->attachObject(mt);
            n2->setPosition(coll_box.lo + (coll_box.hi - coll_box.lo) * 0.5f);
        }
    }

    // register this collision box in the index
    coll_box.ilo = Ogre::Vector3(coll_box.lo / Ogre::Real(CELL_SIZE));
    coll_box.ihi = Ogre::Vector3(coll_box.hi / Ogre::Real(CELL_SIZE));
    
    // clamp between 0 and MAXIMUM_CELL;
    coll_box.ilo.makeCeil(Ogre::Vector3(0.0f));
    coll_box.ilo.makeFloor(Ogre::Vector3(MAXIMUM_CELL));
    coll_box.ihi.makeCeil(Ogre::Vector3(0.0f));
    coll_box.ihi.makeFloor(Ogre::Vector3(MAXIMUM_CELL));

    for (int i=coll_box.ilo.x; i <= coll_box.ihi.x; i++)
    {
        for (int j=coll_box.ilo.z; j <= coll_box.ihi.z; j++)
        {
            //LOG("Adding a reference to cell "+TOSTRING(i)+" "+TOSTRING(j)+" at index "+TOSTRING(collision_index_free[i*NUM_COLLISON_CELLS+j]));
            hash_add(i,j,coll_box_index,coll_box.hi.y);
        }
    }

    m_collision_boxes.push_back(coll_box);
    return coll_box_index;
}

int Collisions::addCollisionTri(Vector3 p1, Vector3 p2, Vector3 p3, ground_model_t* gm)
{
    int new_tri_index = this->GetNumCollisionTris();
    collision_tri_t new_tri;
    new_tri.a=p1;
    new_tri.b=p2;
    new_tri.c=p3;
    new_tri.gm=gm;
    new_tri.enabled=true;
    // compute transformations
    // base construction
    Vector3 bx=p2-p1;
    Vector3 by=p3-p1;
    Vector3 bz=bx.crossProduct(by);
    bz.normalise();
    // coordinates change matrix
    new_tri.reverse.SetColumn(0, bx);
    new_tri.reverse.SetColumn(1, by);
    new_tri.reverse.SetColumn(2, bz);
    new_tri.forward=new_tri.reverse.Inverse();

    // compute tri AAB
    new_tri.aab.merge(p1);
    new_tri.aab.merge(p2);
    new_tri.aab.merge(p3);
    new_tri.aab.setMinimum(new_tri.aab.getMinimum() - 0.1f);
    new_tri.aab.setMaximum(new_tri.aab.getMaximum() + 0.1f);
    
    // register this collision tri in the index
    Ogre::Vector3 ilo(new_tri.aab.getMinimum() / Ogre::Real(CELL_SIZE));
    Ogre::Vector3 ihi(new_tri.aab.getMaximum() / Ogre::Real(CELL_SIZE));
    
    // clamp between 0 and MAXIMUM_CELL;
    ilo.makeCeil(Ogre::Vector3(0.0f));
    ilo.makeFloor(Ogre::Vector3(MAXIMUM_CELL));
    ihi.makeCeil(Ogre::Vector3(0.0f));
    ihi.makeFloor(Ogre::Vector3(MAXIMUM_CELL));
    
    for (int i = ilo.x; i <= ihi.x; i++)
    {
        for (int j=ilo.z; j<=ihi.z; j++)
        {
            hash_add(i,j,new_tri_index+hash_coll_element_t::ELEMENT_TRI_BASE_INDEX,new_tri.aab.getMaximum().y);
        }
    }
    
    if (debugMode)
    {
        debugmo->position(p1);
        debugmo->position(p2);
        debugmo->position(p3);
    }

    m_collision_tris.push_back(new_tri);
    return new_tri_index;
}

bool Collisions::envokeScriptCallback(collision_box_t *cbox, node_t *node)
{
    bool handled = false;

#ifdef USE_ANGELSCRIPT
    // check if this box is active anymore
    if (!eventsources[cbox->eventsourcenum].enabled)
        return false;
    
    std::lock_guard<std::mutex> lock(m_scriptcallback_mutex);
    // this prevents that the same callback gets called at 2k FPS all the time, serious hit on FPS ...
    if (std::find(std::begin(m_last_called_cboxes), std::end(m_last_called_cboxes), cbox) == m_last_called_cboxes.end())
    {
        if (!ScriptEngine::getSingleton().envokeCallback(eventsources[cbox->eventsourcenum].scripthandler, &eventsources[cbox->eventsourcenum], node))
            handled = true;
        m_last_called_cboxes.push_back(cbox);
    }
#endif //USE_ANGELSCRIPT

    return handled;
}

void Collisions::clearEventCache()
{
    m_last_called_cboxes.clear();
}

bool Collisions::collisionCorrect(Vector3 *refpos, bool envokeScriptCallbacks)
{
    // find the correct cell
    if (!(refpos->x > 0 && refpos->x < m_terrain_size.x && refpos->z > 0 && refpos->z < m_terrain_size.z))
        return false;

    int refx = (int)(refpos->x / (float)CELL_SIZE);
    int refz = (int)(refpos->z / (float)CELL_SIZE);
    int hash = hash_find(refx, refz);

    if (refpos->y > hashtable_height[hash])
        return false;

    collision_tri_t *minctri = 0;
    float minctridist = 100.0f;
    Vector3 minctripoint;

    bool contacted = false;
    bool isScriptCallbackEnvoked = false;

    size_t num_elements = hashtable[hash].size();
    for (size_t k = 0; k < num_elements; k++)
    {
        if (hashtable[hash][k].IsCollisionBox())
        {
            collision_box_t* cbox = &m_collision_boxes[hashtable[hash][k].element_index];
            if ( !( (*refpos) > cbox->lo && (*refpos) < cbox->hi ) ) continue;

            if (cbox->refined || cbox->selfrotated)
            {
                // we may have a collision, do a change of repere
                Vector3 Pos = *refpos - cbox->center;
                if (cbox->refined)
                {
                    Pos = cbox->unrot * Pos;
                }
                if (cbox->selfrotated)
                {
                    Pos = Pos - cbox->selfcenter;
                    Pos = cbox->selfunrot * Pos;
                    Pos = Pos + cbox->selfcenter;
                }
                // now test with the inner box
                if (Pos > cbox->relo && Pos < cbox->rehi)
                {
                    if (cbox->eventsourcenum!=-1 && permitEvent(cbox->event_filter) && envokeScriptCallbacks)
                    {
                        envokeScriptCallback(cbox);
                        isScriptCallbackEnvoked = true;
                    }
                    if (cbox->camforced && !forcecam)
                    {
                        forcecam = true;
                        forcecampos = cbox->campos;
                    }
                    if (!cbox->virt)
                    {
                        // collision, process as usual
                        // we have a collision
                        contacted = true;
                        // determine which side collided
                        Pos = calcCollidedSide(Pos, cbox->relo, cbox->rehi);
                        
                        // resume repere
                        if (cbox->selfrotated)
                        {
                            Pos = Pos - cbox->selfcenter;
                            Pos = cbox->selfrot * Pos;
                            Pos = Pos + cbox->selfcenter;
                        }
                        if (cbox->refined)
                        {
                            Pos = cbox->rot * Pos;
                        }
                        *refpos = Pos + cbox->center;
                    }
                }

            } else
            {
                if (cbox->eventsourcenum!=-1 && permitEvent(cbox->event_filter) && envokeScriptCallbacks)
                {
                    envokeScriptCallback(cbox);
                    isScriptCallbackEnvoked = true;
                }
                if (cbox->camforced && !forcecam)
                {
                    forcecam = true;
                    forcecampos = cbox->campos;
                }
                if (!cbox->virt)
                {
                    // we have a collision
                    contacted = true;
                    // determine which side collided
                    (*refpos) = calcCollidedSide((*refpos), cbox->lo, cbox->hi);
                }
            }
        }
        else // The element is a triangle
        {
            const int ctri_index = hashtable[hash][k].element_index - hash_coll_element_t::ELEMENT_TRI_BASE_INDEX;
            collision_tri_t *ctri=&m_collision_tris[ctri_index];
            if (!ctri->enabled)
                continue;
            if (!ctri->aab.contains(*refpos))
                continue;
            // check if this tri is minimal
            // transform
            Vector3 point = ctri->forward * (*refpos-ctri->a);
            // test if within tri collision volume (potential cause of bug!)
            if (point.x >= 0 && point.y >= 0 && (point.x + point.y) <= 1.0 && point.z < 0 && point.z > -0.1)
            {
                if (-point.z < minctridist)
                {
                    minctri = ctri;
                    minctridist = -point.z;
                    minctripoint = point;
                }
            }
        }
    }

    if (envokeScriptCallbacks && !isScriptCallbackEnvoked)
        clearEventCache();

    // process minctri collision
    if (minctri)
    {
        // we have a contact
        contacted = true;
        // correct point
        minctripoint.z = 0;
        // reverse transform
        *refpos = (minctri->reverse * minctripoint) + minctri->a;
    }
    return contacted;
}

bool Collisions::permitEvent(int filter)
{
    Actor *b = RoR::App::GetSimController()->GetPlayerActor();

    switch (filter)
    {
    case EVENT_ALL:
        return true;
    case EVENT_AVATAR:
        return !b;
    case EVENT_TRUCK:
        return b && b->ar_driveable == TRUCK;
    case EVENT_AIRPLANE:
        return b && b->ar_driveable == AIRPLANE;
    case EVENT_BOAT:
        return b && b->ar_driveable == BOAT;
    case EVENT_DELETE:
        return !b;
    default:
        return false;
    }
}

bool Collisions::nodeCollision(node_t *node, float dt)
{
    // find the correct cell
    int refx = (int)(node->AbsPosition.x / CELL_SIZE);
    int refz = (int)(node->AbsPosition.z / CELL_SIZE);
    int hash = hash_find(refx, refz);
    unsigned int cell_id = (refx << 16) + refz;

    if (node->AbsPosition.y > hashtable_height[hash])
        return false;

    collision_tri_t *minctri = 0;
    float minctridist = 100.0;
    Vector3 minctripoint;

    bool contacted = false;

    size_t num_elements = hashtable[hash].size();
    for (size_t k=0; k < num_elements; k++)
    {
        if (hashtable[hash][k].cell_id != cell_id)
        {
            continue;
        }
        else if (hashtable[hash][k].IsCollisionBox())
        {
            collision_box_t *cbox = &m_collision_boxes[hashtable[hash][k].element_index];
            if (node->AbsPosition > cbox->lo && node->AbsPosition < cbox->hi)
            {
                if (cbox->refined || cbox->selfrotated)
                {
                    // we may have a collision, do a change of repere
                    Vector3 Pos = node->AbsPosition-cbox->center;
                    if (cbox->refined)
                    {
                        Pos = cbox->unrot * Pos;
                    }
                    if (cbox->selfrotated)
                    {
                        Pos = Pos - cbox->selfcenter;
                        Pos = cbox->selfunrot * Pos;
                        Pos = Pos + cbox->selfcenter;
                    }
                    // now test with the inner box
                    if (Pos > cbox->relo && Pos < cbox->rehi)
                    {
                        if (cbox->eventsourcenum!=-1 && permitEvent(cbox->event_filter))
                        {
                            envokeScriptCallback(cbox, node);
                        }
                        if (cbox->camforced && !forcecam)
                        {
                            forcecam = true;
                            forcecampos = cbox->campos;
                        }
                        if (!cbox->virt)
                        {
                            // collision, process as usual
                            // we have a collision
                            contacted = true;
                            // determine which side collided
                            float t = cbox->rehi.z - Pos.z;
                            float min = Pos.z - cbox->relo.z;
                            Vector3 normal = Vector3(0, 0, -1);
                            if (t < min) { min = t; normal = Vector3(0,0,1);}; //north
                            t = Pos.x - cbox->relo.x;
                            if (t < min) { min = t; normal = Vector3(-1,0,0);}; //west
                            t = cbox->rehi.x - Pos.x;
                            if (t < min) { min = t; normal = Vector3(1,0,0);}; //east
                            t = Pos.y - cbox->relo.y;
                            if (t < min) { min = t; normal = Vector3(0,-1,0);}; //down
                            t = cbox->rehi.y - Pos.y;
                            if (t < min) { min = t; normal = Vector3(0,1,0);}; //up

                            // resume repere for the normal
                            if (cbox->selfrotated) normal = cbox->selfrot * normal;
                            if (cbox->refined) normal = cbox->rot * normal;

                            // collision boxes are always out of concrete as it seems
                            primitiveCollision(node, node->Forces, node->Velocity, normal, dt, defaultgm);
                            node->nd_last_collision_gm = defaultgm;
                        }
                    }
                } else
                {
                    if (cbox->eventsourcenum!=-1 && permitEvent(cbox->event_filter))
                    {
                        envokeScriptCallback(cbox, node);
                    }
                    if (cbox->camforced && !forcecam)
                    {
                        forcecam = true;
                        forcecampos = cbox->campos;
                    }
                    if (!cbox->virt)
                    {
                        // we have a collision
                        contacted=true;
                        // determine which side collided
                        float t = cbox->hi.z - node->AbsPosition.z;
                        float min = node->AbsPosition.z - cbox->lo.z;
                        Vector3 normal = Vector3(0, 0, -1);
                        if (t < min) {min = t; normal = Vector3(0,0,1);}; //north
                        t = node->AbsPosition.x - cbox->lo.x;
                        if (t < min) {min = t; normal = Vector3(-1,0,0);}; //west
                        t = cbox->hi.x - node->AbsPosition.x;
                        if (t < min) {min = t; normal = Vector3(1,0,0);}; //east
                        t = node->AbsPosition.y - cbox->lo.y;
                        if (t < min) {min = t; normal = Vector3(0,-1,0);}; //down
                        t = cbox->hi.y - node->AbsPosition.y;
                        if (t < min) {min = t; normal = Vector3(0,1,0);}; //up

                        // resume repere for the normal
                        if (cbox->selfrotated) normal = cbox->selfrot * normal;
                        if (cbox->refined) normal = cbox->rot * normal;

                        // collision boxes are always out of concrete as it seems
                        primitiveCollision(node, node->Forces, node->Velocity, normal, dt, defaultgm);
                        node->nd_last_collision_gm = defaultgm;
                    }
                }
            }
        }
        else
        {
            // tri collision
            const int ctri_index = hashtable[hash][k].element_index - hash_coll_element_t::ELEMENT_TRI_BASE_INDEX;
            collision_tri_t *ctri=&m_collision_tris[ctri_index];
            if (node->AbsPosition.y > ctri->aab.getMaximum().y || node->AbsPosition.y < ctri->aab.getMinimum().y ||
                node->AbsPosition.x > ctri->aab.getMaximum().x || node->AbsPosition.x < ctri->aab.getMinimum().x ||
                node->AbsPosition.z > ctri->aab.getMaximum().z || node->AbsPosition.z < ctri->aab.getMinimum().z)
                continue;
            // check if this tri is minimal
            // transform
            Vector3 point = ctri->forward * (node->AbsPosition - ctri->a);
            // test if within tri collision volume (potential cause of bug!)
            if (point.x >= 0 && point.y >= 0 && (point.x + point.y) <= 1.0 && point.z < 0 && point.z > -0.1)
            {
                if (-point.z < minctridist)
                {
                    minctri = ctri;
                    minctridist = -point.z;
                    minctripoint = point;
                }
            }
        }
    }

    // process minctri collision
    if (minctri)
    {
        // we have a contact
        contacted=true;
        // we need the normal
        // resume repere for the normal
        Vector3 normal = minctri->reverse * Vector3::UNIT_Z;
        primitiveCollision(node, node->Forces, node->Velocity, normal, dt, minctri->gm);
        node->nd_last_collision_gm = minctri->gm;
    }

    return contacted;
}


Vector3 Collisions::getPosition(const Ogre::String &inst, const Ogre::String &box)
{
    for (int i=0; i<free_eventsource; i++)
    {
        if (!strcmp(inst.c_str(), eventsources[i].instancename) && !strcmp(box.c_str(), eventsources[i].boxname))
        {
            return m_collision_boxes[eventsources[i].cbox].center+m_collision_boxes[eventsources[i].cbox].rot*m_collision_boxes[eventsources[i].cbox].selfcenter;
        }
    }
    return Vector3::ZERO;
}

Quaternion Collisions::getDirection(const Ogre::String &inst, const Ogre::String &box)
{
    for (int i=0; i<free_eventsource; i++)
    {
        if (!strcmp(inst.c_str(), eventsources[i].instancename) && !strcmp(box.c_str(), eventsources[i].boxname))
        {
            return m_collision_boxes[eventsources[i].cbox].rot*eventsources[i].direction;
        }
    }
    return Quaternion::ZERO;
}

collision_box_t *Collisions::getBox(const Ogre::String &inst, const Ogre::String &box)
{
    for (int i=0; i<free_eventsource; i++)
    {
        if (!strcmp(inst.c_str(), eventsources[i].instancename) && !strcmp(box.c_str(), eventsources[i].boxname))
        {
            return &m_collision_boxes[eventsources[i].cbox];
        }
    }
    return NULL;
}

eventsource_t *Collisions::isTruckInEventBox(Actor *truck)
{
    if (!truck) return 0;
    // check all boxes
    for (int i=0; i<free_eventsource; i++)
    {
        collision_box_t *cb = &m_collision_boxes[eventsources[i].cbox];

        if (!cb->enabled)
            continue;

        // check all nodes

        bool allInside = true;
        for (int n=0; n < truck->ar_num_nodes; n++)
        {
            if (!isInside(truck->ar_nodes[n].AbsPosition, cb))
            {
                // node not in box, no need to check the rest
                allInside=false;
                break;
            }
        }
        if (allInside && cb->eventsourcenum != -1)
        {
            return &eventsources[cb->eventsourcenum];
        }

    }
    return 0;
}

bool Collisions::isInside(Vector3 pos, const Ogre::String &inst, const Ogre::String &box, float border)
{
    collision_box_t *cbox = getBox(inst, box);

    return isInside(pos, cbox, border);
}

bool Collisions::isInside(Vector3 pos, collision_box_t *cbox, float border)
{
    if (!cbox) return false;
    
    if (pos + border > cbox->lo
     && pos - border < cbox->hi)
    {
        if (cbox->refined || cbox->selfrotated)
        {
            // we may have a collision, do a change of repere
            Vector3 rpos = pos - cbox->center;
            if (cbox->refined)
            {
                rpos = cbox->unrot * rpos;
            }
            if (cbox->selfrotated)
            {
                rpos = rpos - cbox->selfcenter;
                rpos = cbox->selfunrot * rpos;
                rpos = rpos + cbox->selfcenter;
            }
            
            // now test with the inner box
            if (rpos > cbox->relo
             && rpos < cbox->rehi)
            {
                return true;
            }
        } else
        {
            return true;
        }
    }
    return false;
}

bool Collisions::groundCollision(node_t *node, float dt)
{
    Real v = App::GetSimTerrain()->GetHeightAt(node->AbsPosition.x, node->AbsPosition.z);
    if (v > node->AbsPosition.y)
    {
        ground_model_t* ogm = landuse ? landuse->getGroundModelAt(node->AbsPosition.x, node->AbsPosition.z) : nullptr;
        // when landuse fails or we don't have it, use the default value
        if (!ogm) ogm = defaultgroundgm;
        Ogre::Vector3 normal = App::GetSimTerrain()->GetNormalAt(node->AbsPosition.x, v, node->AbsPosition.z);
        primitiveCollision(node, node->Forces, node->Velocity, normal, dt, ogm, v - node->AbsPosition.y);
        node->nd_last_collision_gm = ogm;
        return true;
    }
    return false;
}

void primitiveCollision(node_t *node, Vector3 &force, const Vector3 &velocity, const Vector3 &normal, float dt, ground_model_t* gm, float penetration, float reaction)
{
    float Vnormal = velocity.dotProduct(normal);

    // if we are inside the fluid (solid ground is below us)
    if (gm->solid_ground_level != 0.0f && penetration >= 0)
    {
        float Vsquared = velocity.squaredLength();
        // First of all calculate power law fluid viscosity
        float m = gm->flow_consistency_index * approx_pow(Vsquared, (gm->flow_behavior_index - 1.0f)*0.5f);

        // Then calculate drag based on above. We'are using a simplified Stokes' drag.
        // Per node fluid drag surface coefficient set by node property applies here
        Vector3 Fdrag = velocity * (-m * node->surface_coef);

        // If we have anisotropic drag
        if (gm->drag_anisotropy < 1.0f && Vnormal > 0)
        {
            float da_factor;
            if (Vsquared > gm->va * gm->va)
                da_factor = 1.0;
            else
                da_factor = Vsquared / (gm->va * gm->va);
            Fdrag += (Vnormal * m * (1.0f - gm->drag_anisotropy) * da_factor) * normal;
        }
        force += Fdrag;

        // Now calculate upwards force based on a simplified boyancy equation;
        // If the fluid is pseudoplastic then boyancy is constrained to only "stopping" a node from going downwards
        // Buoyancy per node volume coefficient set by node property applies here
        float Fboyancy = gm->fluid_density * penetration * (-DEFAULT_GRAVITY) * node->volume_coef;
        if (gm->flow_behavior_index < 1.0f && Vnormal >= 0.0f)
        {
            float Fnormal = force.dotProduct(normal);
            if (Fnormal < 0 && Fboyancy>-Fnormal)
            {
                Fboyancy = -Fnormal;
            }
        }
        force += Fboyancy*normal;
    }

    // if we are inside or touching the solid ground
    if (penetration >= gm->solid_ground_level)
    {
        Vector3 slip = velocity - Vnormal*normal;
        float slipv = slip.normalise();

        float Freaction;

        float Fnormal = force.dotProduct(normal);
        float Fdnormal = Fnormal;

        // steady force
        if (reaction < 0)
        {
            Freaction = -Fnormal;
            // impact force
            if (Vnormal < 0)
            {
                Freaction += -Vnormal * node->mass / dt; // Newton's second law
            }
            if (Freaction < 0) Freaction = 0.0f;
            // We only update this if we handle ground collisions
            node->nd_last_collision_slip = slipv;
        } else
        {
            Freaction = reaction;
            Fnormal = 0.0f;
        }
        // If the velocity that we slip is lower than adhesion velocity and
        // we have a downforce and the slip forces are lower than static friction
        // forces then it's time to go into static friction physics mode.
        // This code is a direct translation of textbook static friction physics
        float Greaction = Freaction * gm->strength * node->friction_coef; //General moderated reaction, node property sets friction_coef as a pernodefriction setting
        float msGreaction = gm->ms * Greaction;
        if (slipv < gm->va && Greaction > 0.0f && (force - Fdnormal * normal).squaredLength() <= msGreaction * msGreaction)
        {
            // Static friction model (with a little smoothing to help the integrator deal with it)
            float ff = -msGreaction * (1.0f - approx_exp(-slipv / gm->va));
            force = (Fnormal + Freaction) * normal + ff * slip;
        } else
        {
            // Stribek model. It also comes directly from textbooks.
            float g = gm->mc + (gm->ms - gm->mc) * approx_exp(-approx_pow(slipv / gm->vs, gm->alpha));
            float ff = -(g + std::min(gm->t2 * slipv, 5.0f)) * Greaction;
            force += Freaction * normal + ff * slip;
        }
    }
}

int Collisions::createCollisionDebugVisualization()
{
    LOG("COLL: Creating collision debug visualization ...");

    static int loaded = 0;
    // prevent double calling
    if (loaded != 0) return -1;

    // create materials
    int i = 0;
    char bname[256];
    for (i=0;i<=100;i++)
    {
        // register a material for skeleton view
        sprintf(bname, "mat-coll-dbg-%d", i);
        MaterialPtr mat=(MaterialPtr)(MaterialManager::getSingleton().create(bname, ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME));
        float f = fabs(((float)i)/100);
        Pass *p = mat->getTechnique(0)->getPass(0); //
        p->createTextureUnitState()->setColourOperationEx(LBX_MODULATE, LBS_MANUAL, LBS_CURRENT, ColourValue(f*2.0, 2.0*(1.0-f), 0.2, 0.7));
        p->setSceneBlending(Ogre::SBT_TRANSPARENT_ALPHA);
        p->setLightingEnabled(false);
        p->setDepthWriteEnabled(false);
        p->setDepthBias(3, 3);
        p->setCullingMode(Ogre::CULL_NONE);

        Pass *p2 = mat->getTechnique(0)->createPass();
        p2->setSceneBlending(Ogre::SBT_TRANSPARENT_ALPHA);
        p2->setLightingEnabled(false);
        p2->setDepthWriteEnabled(false);
        p2->setDepthBias(3, 3);
        p2->setCullingMode(Ogre::CULL_NONE);
        p2->setSceneBlending(Ogre::SBT_TRANSPARENT_ALPHA);
        TextureUnitState *tus2 = p2->createTextureUnitState();
        tus2->setTextureName("tile.png");


        mat->setLightingEnabled(false);
        mat->setReceiveShadows(false);
    }

    for (int x=0; x<(int)(m_terrain_size.x); x+=(int)CELL_SIZE)
    {
        for (int z=0; z<(int)(m_terrain_size.z); z+=(int)CELL_SIZE)
        {
            int cellx = (int)(x/(float)CELL_SIZE);
            int cellz = (int)(z/(float)CELL_SIZE);
            const int hash = hash_find(cellx, cellz);

            bool used = std::find_if(hashtable[hash].begin(), hashtable[hash].end(), [&](hash_coll_element_t const &c) {
                    return c.cell_id == (cellx << 16) + cellz;
            }) != hashtable[hash].end();

            if (used)
            {
                float groundheight = -9999;
                float x2 = x+CELL_SIZE;
                float z2 = z+CELL_SIZE;

                // find a good ground height for all corners of the cell ...
                groundheight = std::max(groundheight, App::GetSimTerrain()->GetHeightAt(x, z));
                groundheight = std::max(groundheight, App::GetSimTerrain()->GetHeightAt(x2, z));
                groundheight = std::max(groundheight, App::GetSimTerrain()->GetHeightAt(x, z2));
                groundheight = std::max(groundheight, App::GetSimTerrain()->GetHeightAt(x2, z2));
                groundheight += 0.1; // 10 cm hover

                float percentd = static_cast<float>(hashtable[hash].size()) / static_cast<float>(CELL_BLOCKSIZE);

                if (percentd > 1) percentd = 1;
                String matName = "mat-coll-dbg-"+TOSTRING((int)(percentd*100));
                String cell_name="("+TOSTRING(cellx)+","+ TOSTRING(cellz)+")";

                ManualObject *mo =  gEnv->sceneManager->createManualObject("collisionDebugVisualization"+cell_name);
                SceneNode *mo_node = gEnv->sceneManager->getRootSceneNode()->createChildSceneNode("collisionDebugVisualization_node"+cell_name);

                mo->begin(matName, Ogre::RenderOperation::OT_TRIANGLE_LIST);

                // 1st tri
                mo->position(-CELL_SIZE/(float)2.0, 0, -CELL_SIZE/(float)2.0);
                mo->textureCoord(0,0);

                mo->position(CELL_SIZE/(float)2.0, 0, CELL_SIZE/(float)2.0);
                mo->textureCoord(1,1);

                mo->position(CELL_SIZE/(float)2.0, 0, -CELL_SIZE/(float)2.0);
                mo->textureCoord(1,0);

                // 2nd tri
                mo->position(-CELL_SIZE/(float)2.0, 0, CELL_SIZE/(float)2.0);
                mo->textureCoord(0,1);

                mo->position(CELL_SIZE/(float)2.0, 0, CELL_SIZE/(float)2.0);
                mo->textureCoord(1,1);

                mo->position(-CELL_SIZE/(float)2.0, 0, -CELL_SIZE/(float)2.0);
                mo->textureCoord(0,0);

                mo->end();
                mo->setBoundingBox(AxisAlignedBox(0, 0, 0, CELL_SIZE, 1, CELL_SIZE));
                mo_node->attachObject(mo);

#if 0
                // setup the label
                String labelName = "label_"+cell_name;
                String labelCaption = cell_name+" "+TOSTRING(percent*100,2) + "% usage ("+TOSTRING(cc)+"/"+TOSTRING(CELL_BLOCKSIZE)+") DEEP: " + TOSTRING(deep);
                MovableText *mt = new MovableText(labelName, labelCaption);
                mt->setFontName("highcontrast_black");
                mt->setTextAlignment(MovableText::H_CENTER, MovableText::V_ABOVE);
                mt->setAdditionalHeight(1);
                mt->showOnTop(false);
                mt->setCharacterHeight(0.3);
                mt->setColor(ColourValue::Black);
                mo_node->attachObject(mt);
#endif

                mo_node->setVisible(true);
                mo_node->setPosition(Vector3(x+CELL_SIZE/(float)2.0, groundheight, z+CELL_SIZE/(float)2.0));
            }
        }
    }

    loaded = 1;
    return 0;
}

int Collisions::addCollisionMesh(Ogre::String meshname, Ogre::Vector3 pos, Ogre::Quaternion q, Ogre::Vector3 scale, ground_model_t *gm, std::vector<int> *collTris)
{
    // normal, non virtual collision box
    Entity *ent = gEnv->sceneManager->createEntity(meshname);
    ent->setMaterialName("tracks/debug/collision/mesh");

    if (!gm)
    {
        gm = getGroundModelByString("concrete");
    }

    size_t vertex_count,index_count;
    Vector3* vertices;
    unsigned* indices;

    getMeshInformation(ent->getMesh().getPointer(),vertex_count,vertices,index_count,indices, pos, q, scale);

    //LOG(LML_NORMAL,"Vertices in mesh: %u",vertex_count);
    //LOG(LML_NORMAL,"Triangles in mesh: %u",index_count / 3);
    for (int i=0; i<(int)index_count/3; i++)
    {
        int triID = addCollisionTri(vertices[indices[i*3]], vertices[indices[i*3+1]], vertices[indices[i*3+2]], gm);
        if (collTris)
            collTris->push_back(triID);
    }

    delete[] vertices;
    delete[] indices;
    if (!debugMode)
    {
        gEnv->sceneManager->destroyEntity(ent);
    } else
    {
        SceneNode *n=gEnv->sceneManager->getRootSceneNode()->createChildSceneNode();
        n->attachObject(ent);
        n->setPosition(pos);
        n->setScale(scale);
        n->setOrientation(q);
    
        String labelName = "collision_mesh_label_"+TOSTRING(this->GetNumCollisionTris());
        String labelCaption = "COLLMESH\nmeshname:"+meshname + "\ngroundmodel:" + String(gm->name);
        MovableText *mt = new MovableText(labelName, labelCaption);
        mt->setFontName("highcontrast_black");
        mt->setTextAlignment(MovableText::H_CENTER, MovableText::V_ABOVE);
        mt->setAdditionalHeight(1);
        mt->showOnTop(false);
        mt->setCharacterHeight(0.3);
        mt->setColor(ColourValue::Black);
        mt->setRenderingDistance(200);
        
        n->attachObject(mt);
    }
    return 0;
}

void Collisions::getMeshInformation(Mesh* mesh,size_t &vertex_count,Vector3* &vertices,
                                              size_t &index_count, unsigned* &indices,
                                              const Vector3 &position,
                                              const Quaternion &orient,const Vector3 &scale)
{
    vertex_count = index_count = 0;

    bool added_shared = false;
    size_t current_offset = vertex_count;
    size_t shared_offset = vertex_count;
    size_t next_offset = vertex_count;
    size_t index_offset = index_count;
    //size_t prev_vert = vertex_count;
    //size_t prev_ind = index_count;

    // Calculate how many vertices and indices we're going to need
    for (int i = 0;i < mesh->getNumSubMeshes();i++)
    {
        SubMesh* submesh = mesh->getSubMesh(i);

        // We only need to add the shared vertices once
        if (submesh->useSharedVertices)
        {
            if (!added_shared)
            {
                VertexData* vertex_data = mesh->sharedVertexData;
                vertex_count += vertex_data->vertexCount;
                added_shared = true;
            }
        } else
        {
            VertexData* vertex_data = submesh->vertexData;
            vertex_count += vertex_data->vertexCount;
        }

        // Add the indices
        Ogre::IndexData* index_data = submesh->indexData;
        index_count += index_data->indexCount;
    }

    // Allocate space for the vertices and indices
    vertices = new Vector3[vertex_count];
    indices = new unsigned[index_count];

    added_shared = false;

    // Run through the sub-meshes again, adding the data into the arrays
    for (int i = 0;i < mesh->getNumSubMeshes();i++)
    {
        SubMesh* submesh = mesh->getSubMesh(i);

        Ogre::VertexData* vertex_data = submesh->useSharedVertices ? mesh->sharedVertexData : submesh->vertexData;
        if ((!submesh->useSharedVertices)||(submesh->useSharedVertices && !added_shared))
        {
            if (submesh->useSharedVertices)
            {
                added_shared = true;
                shared_offset = current_offset;
            }

            const Ogre::VertexElement* posElem = vertex_data->vertexDeclaration->findElementBySemantic(Ogre::VES_POSITION);
            Ogre::HardwareVertexBufferSharedPtr vbuf = vertex_data->vertexBufferBinding->getBuffer(posElem->getSource());
            unsigned char* vertex = static_cast<unsigned char*>(vbuf->lock(Ogre::HardwareBuffer::HBL_READ_ONLY));
            Ogre::Real* pReal;

            for (size_t j = 0; j < vertex_data->vertexCount; ++j, vertex += vbuf->getVertexSize())
            {
                posElem->baseVertexPointerToElement(vertex, &pReal);

                Vector3 pt;

                pt.x = (*pReal++);
                pt.y = (*pReal++);
                pt.z = (*pReal++);

                pt = (orient * (pt * scale)) + position;

                vertices[current_offset + j].x = pt.x;
                vertices[current_offset + j].y = pt.y;
                vertices[current_offset + j].z = pt.z;
            }
            vbuf->unlock();
            next_offset += vertex_data->vertexCount;
        }

        Ogre::IndexData* index_data = submesh->indexData;

        size_t numTris = index_data->indexCount / 3;
        unsigned short* pShort = 0;
        unsigned int* pInt = 0;
        Ogre::HardwareIndexBufferSharedPtr ibuf = index_data->indexBuffer;
        
        bool use32bitindexes = (ibuf->getType() == Ogre::HardwareIndexBuffer::IT_32BIT);

        if (use32bitindexes)
            pInt = static_cast<unsigned int*>(ibuf->lock(Ogre::HardwareBuffer::HBL_READ_ONLY));
        else
            pShort = static_cast<unsigned short*>(ibuf->lock(Ogre::HardwareBuffer::HBL_READ_ONLY));

        for (size_t k = 0; k < numTris; ++k)
        {
            size_t offset = (submesh->useSharedVertices)?shared_offset:current_offset;

            unsigned int vindex = use32bitindexes? *pInt++ : *pShort++;
            indices[index_offset + 0] = vindex + (unsigned int)offset;
            vindex = use32bitindexes? *pInt++ : *pShort++;
            indices[index_offset + 1] = vindex + (unsigned int)offset;
            vindex = use32bitindexes? *pInt++ : *pShort++;
            indices[index_offset + 2] = vindex + (unsigned int)offset;

            index_offset += 3;
        }
        ibuf->unlock();
        current_offset = next_offset;
    }
}

void Collisions::finishLoadingTerrain()
{
    if (debugMode)
    {
        SceneNode *debugsn = gEnv->sceneManager->getRootSceneNode()->createChildSceneNode();
        debugmo->end();
        debugsn->setPosition(Vector3::ZERO);
        debugsn->attachObject(debugmo);

        createCollisionDebugVisualization();
    }
}
