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

#include "Beam.h"

#include <Ogre.h>
#ifdef ROR_USE_OGRE_1_9
#	include <Overlay/OgreOverlayManager.h>
#	include <Overlay/OgreOverlay.h>
#else
#	include <OgreOverlayManager.h>
#	include <OgreOverlayElement.h>
#endif

#include "AirBrake.h"
#include "Airfoil.h"
#include "Application.h"
#include "ApproxMath.h"
#include "AutoPilot.h"
#include "BeamData.h"
#include "BeamEngine.h"
#include "BeamFactory.h"
#include "BeamStats.h"
#include "Buoyance.h"
#include "CacheSystem.h"
#include "CameraManager.h"
#include "CartesianToTriangleTransform.h"
#include "CmdKeyInertia.h"
#include "Collisions.h"
#include "GfxActor.h"
#include "GUI_GameConsole.h"
#include "DashBoardManager.h"
#include "Differentials.h"
#include "DynamicCollisions.h"
#include "ErrorUtils.h"
#include "FlexAirfoil.h"
#include "FlexBody.h"
#include "FlexMesh.h"
#include "FlexMeshWheel.h"
#include "FlexObj.h"
#include "IHeightFinder.h"
#include "InputEngine.h"
#include "Language.h"
#include "MeshObject.h"

#include "MovableText.h"
#include "Network.h"
#include "PointColDetector.h"
#include "PositionStorage.h"
#include "Replay.h"
#include "RigLoadingProfiler.h"
#include "RigSpawner.h"
#include "RoRFrameListener.h"
#include "ScrewProp.h"
#include "Scripting.h"
#include "Settings.h"
#include "Skidmark.h"
#include "SlideNode.h"
#include "SoundScriptManager.h"
#include "TerrainManager.h"
#include "ThreadPool.h"
#include "Triangle.h"
#include "TurboJet.h"
#include "TurboProp.h"
#include "VehicleAI.h"
#include "Water.h"
#include "GUIManager.h"

#define LOAD_RIG_PROFILE_CHECKPOINT(ENTRY) rig_loading_profiler->Checkpoint(RoR::RigLoadingProfiler::ENTRY);

#include "RigDef_Parser.h"
#include "RigDef_Validator.h"

using namespace Ogre;
using namespace RoR;

Beam::~Beam()
{
    TRIGGER_EVENT(SE_GENERIC_DELETED_TRUCK, ar_instance_id);

    // TODO: IMPROVE below: delete/destroy prop entities, etc

    this->disjoinInterTruckBeams();

    // hide everything, prevents deleting stuff while drawing
    this->setBeamVisibility(false);
    this->setMeshVisibility(false);

    // delete all classes we might have constructed
    if (ar_dashboard != nullptr)
    {
        delete ar_dashboard;
        ar_dashboard = nullptr;
    }

    // stop all the Sounds
#ifdef USE_OPENAL
    for (int i = SS_TRIG_NONE + 1; i < SS_MAX_TRIG; i++)
    {
        SOUND_STOP(this, i);
    }
#endif // USE_OPENAL
    StopAllSounds();

    if (ar_engine != nullptr)
    {
        delete ar_engine;
        ar_engine = nullptr;
    }

    if (ar_autopilot != nullptr)
    {
        delete ar_autopilot;
        ar_autopilot = nullptr;
    }

    if (m_fusealge_airfoil)
        delete m_fusealge_airfoil;
    m_fusealge_airfoil = 0;

    if (m_cab_mesh != nullptr)
    {
        this->fadeMesh(m_cab_scene_node, 1.f); // Reset transparency of "skeleton view"

        m_cab_scene_node->detachAllObjects();
        m_cab_scene_node->getParentSceneNode()->removeAndDestroyChild(m_cab_scene_node->getName());
        m_cab_scene_node = nullptr;

        m_cab_entity->_getManager()->destroyEntity(m_cab_entity);
        m_cab_entity = nullptr;

        delete m_cab_mesh; // Unloads the ManualMesh resource; do this last
        m_cab_mesh = nullptr;
    }

    if (m_replay_handler)
        delete m_replay_handler;
    m_replay_handler = nullptr;

    if (ar_vehicle_ai)
        delete ar_vehicle_ai;
    ar_vehicle_ai = 0;

    // TODO: Make sure we catch everything here
    // remove all scene nodes
    if (m_deletion_scene_nodes.size() > 0)
    {
        for (unsigned int i = 0; i < m_deletion_scene_nodes.size(); i++)
        {
            if (!m_deletion_scene_nodes[i])
                continue;
            m_deletion_scene_nodes[i]->removeAndDestroyAllChildren();
            gEnv->sceneManager->destroySceneNode(m_deletion_scene_nodes[i]);
        }
        m_deletion_scene_nodes.clear();
    }
    // remove all entities
    if (m_deletion_entities.size() > 0)
    {
        for (unsigned int i = 0; i < m_deletion_entities.size(); i++)
        {
            if (!m_deletion_entities[i])
                continue;
            m_deletion_entities[i]->detachAllObjectsFromBone();
            gEnv->sceneManager->destroyEntity(m_deletion_entities[i]->getName());
        }
        m_deletion_entities.clear();
    }

    // delete wings
    for (int i = 0; i < ar_num_wings; i++)
    {
        // flexAirfoil, airfoil
        if (ar_wings[i].fa)
            delete ar_wings[i].fa;
        if (ar_wings[i].cnode)
        {
            ar_wings[i].cnode->removeAndDestroyAllChildren();
            gEnv->sceneManager->destroySceneNode(ar_wings[i].cnode);
        }
    }

    // delete aeroengines
    for (int i = 0; i < ar_num_aeroengines; i++)
    {
        if (ar_aeroengines[i])
            delete ar_aeroengines[i];
    }

    // delete screwprops
    for (int i = 0; i < ar_num_screwprops; i++)
    {
        if (ar_screwprops[i])
        {
            delete ar_screwprops[i];
            ar_screwprops[i] = nullptr;
        }
    }

    // delete airbrakes
    for (int i = 0; i < ar_num_airbrakes; i++)
    {
        if (ar_airbrakes[i])
        {
            delete ar_airbrakes[i];
            ar_airbrakes[i] = 0;
        }
    }

    // delete flexbodies
    for (int i = 0; i < ar_num_flexbodies; i++)
    {
        if (ar_flexbodies[i])
        {
            delete ar_flexbodies[i];
            ar_flexbodies[i] = nullptr;
        }
    }

    // delete meshwheels
    for (int i = 0; i < free_wheel; i++)
    {
        if (vwheels[i].fm)
            delete vwheels[i].fm;
        if (vwheels[i].cnode)
        {
            vwheels[i].cnode->removeAndDestroyAllChildren();
            gEnv->sceneManager->destroySceneNode(vwheels[i].cnode);
        }
    }

    // delete skidmarks
    for (int i = 0; i < free_wheel; ++i)
    {
        delete m_skid_trails[i];
        m_skid_trails[i] = nullptr;
    }

    // delete props
    for (int i = 0; i < ar_num_props; i++)
    {
        for (int k = 0; k < 4; ++k)
        {
            if (ar_props[i].beacon_flare_billboard_scene_node[k])
            {
                Ogre::SceneNode* scene_node = ar_props[i].beacon_flare_billboard_scene_node[k];
                scene_node->removeAndDestroyAllChildren();
                gEnv->sceneManager->destroySceneNode(scene_node);
            }
            if (ar_props[i].beacon_light[k])
            {
                gEnv->sceneManager->destroyLight(ar_props[i].beacon_light[k]);
            }
        }

        if (ar_props[i].scene_node)
        {
            ar_props[i].scene_node->removeAndDestroyAllChildren();
            gEnv->sceneManager->destroySceneNode(ar_props[i].scene_node);
        }
        if (ar_props[i].wheel)
        {
            ar_props[i].wheel->removeAndDestroyAllChildren();
            gEnv->sceneManager->destroySceneNode(ar_props[i].wheel);
        }
        if (ar_props[i].mo)
        {
            delete ar_props[i].mo;
        }
        if (ar_props[i].wheelmo)
        {
            delete ar_props[i].wheelmo;
        }
    }

    // delete flares
    for (size_t i = 0; i < this->ar_flares.size(); i++)
    {
        if (ar_flares[i].snode)
        {
            ar_flares[i].snode->removeAndDestroyAllChildren();
            gEnv->sceneManager->destroySceneNode(ar_flares[i].snode);
        }
        if (ar_flares[i].bbs)
            gEnv->sceneManager->destroyBillboardSet(ar_flares[i].bbs);
        if (ar_flares[i].light)
            gEnv->sceneManager->destroyLight(ar_flares[i].light);
    }
    this->ar_flares.clear();

    // delete exhausts
    for (std::vector<exhaust_t>::iterator it = exhausts.begin(); it != exhausts.end(); it++)
    {
        if (it->smokeNode)
        {
            it->smokeNode->removeAndDestroyAllChildren();
            gEnv->sceneManager->destroySceneNode(it->smokeNode);
        }
        if (it->smoker)
        {
            it->smoker->removeAllAffectors();
            it->smoker->removeAllEmitters();
            gEnv->sceneManager->destroyParticleSystem(it->smoker);
        }
    }

    // delete ar_custom_particles
    for (int i = 0; i < ar_num_custom_particles; i++)
    {
        if (ar_custom_particles[i].snode)
        {
            ar_custom_particles[i].snode->removeAndDestroyAllChildren();
            gEnv->sceneManager->destroySceneNode(ar_custom_particles[i].snode);
        }
        if (ar_custom_particles[i].psys)
        {
            ar_custom_particles[i].psys->removeAllAffectors();
            ar_custom_particles[i].psys->removeAllEmitters();
            gEnv->sceneManager->destroyParticleSystem(ar_custom_particles[i].psys);
        }
    }

    // delete beams
    for (int i = 0; i < ar_num_beams; i++)
    {
        if (ar_beams[i].mSceneNode)
        {
            ar_beams[i].mSceneNode->removeAndDestroyAllChildren();
            gEnv->sceneManager->destroySceneNode(ar_beams[i].mSceneNode);
        }
    }

    // delete Rails
    for (std::vector<RailGroup*>::iterator it = m_railgroups.begin(); it != m_railgroups.end(); it++)
    {
        // signal to the Rail that
        (*it)->cleanUp();
        delete (*it);
    }

    if (m_net_label_mt)
    {
        m_net_label_mt->setVisible(false);
        delete m_net_label_mt;
        m_net_label_mt = nullptr;
    }

    if (m_intra_point_col_detector)
    {
        delete m_intra_point_col_detector;
        m_intra_point_col_detector = nullptr;
    }

    if (m_inter_point_col_detector)
    {
        delete m_inter_point_col_detector;
        m_inter_point_col_detector = nullptr;
    }

    if (m_command_inertia)
        delete m_command_inertia;
    if (m_hydro_inertia)
        delete m_hydro_inertia;
    if (m_rotator_inertia)
        delete m_rotator_inertia;

    for (int i = 0; i < m_num_axles; ++i)
    {
        if (m_axles[i] != nullptr)
            delete (m_axles[i]);
    }

    delete ar_nodes;
    delete ar_beams;
    delete ar_shocks;
    delete ar_rotators;
    delete ar_wings;
}

// This method scales trucks. Stresses should *NOT* be scaled, they describe
// the material type and they do not depend on length or scale.
void Beam::scaleTruck(float value)
{
    BES_GFX_START(BES_GFX_ScaleTruck);

    if (value < 0)
        return;
    ar_scale *= value;
    // scale beams
    for (int i = 0; i < ar_num_beams; i++)
    {
        //ar_beams[i].k *= value;
        ar_beams[i].d *= value;
        ar_beams[i].L *= value;
        ar_beams[i].refL *= value;
        ar_beams[i].Lhydro *= value;
        ar_beams[i].hydroRatio *= value;

        ar_beams[i].diameter *= value;
    }
    // scale nodes
    Vector3 refpos = ar_nodes[0].AbsPosition;
    Vector3 relpos = ar_nodes[0].RelPosition;
    for (int i = 1; i < ar_num_nodes; i++)
    {
        ar_nodes[i].initial_pos = refpos + (ar_nodes[i].initial_pos - refpos) * value;
        ar_nodes[i].AbsPosition = refpos + (ar_nodes[i].AbsPosition - refpos) * value;
        ar_nodes[i].RelPosition = relpos + (ar_nodes[i].RelPosition - relpos) * value;
        ar_nodes[i].Velocity *= value;
        ar_nodes[i].Forces *= value;
        ar_nodes[i].mass *= value;
    }
    updateSlideNodePositions();

    // props and stuff
    // TOFIX: care about prop positions as well!
    for (int i = 0; i < ar_num_props; i++)
    {
        if (ar_props[i].scene_node)
            ar_props[i].scene_node->scale(value, value, value);

        if (ar_props[i].wheel)
            ar_props[i].wheel->scale(value, value, value);

        if (ar_props[i].wheel)
            ar_props[i].wheelpos = relpos + (ar_props[i].wheelpos - relpos) * value;

        if (ar_props[i].beacon_flare_billboard_scene_node[0])
            ar_props[i].beacon_flare_billboard_scene_node[0]->scale(value, value, value);

        if (ar_props[i].beacon_flare_billboard_scene_node[1])
            ar_props[i].beacon_flare_billboard_scene_node[1]->scale(value, value, value);

        if (ar_props[i].beacon_flare_billboard_scene_node[2])
            ar_props[i].beacon_flare_billboard_scene_node[2]->scale(value, value, value);

        if (ar_props[i].beacon_flare_billboard_scene_node[3])
            ar_props[i].beacon_flare_billboard_scene_node[3]->scale(value, value, value);
    }
    // tell the cabmesh that resizing is ok, and they dont need to break ;)
    if (m_cab_mesh)
        m_cab_mesh->ScaleFlexObj(value);

    // todo: scale flexbody
    for (int i = 0; i < ar_num_flexbodies; i++)
    {
        ar_flexbodies[i]->getSceneNode()->scale(value, value, value);
    }
    // todo: fix meshwheels
    //for (int i=0;i<free_wheel;i++)
    //{
    //if (vwheels[i].cnode) vwheels[i].cnode->scale(value, value, value);
    //if (vwheels[i].fm && vwheels[i].cnode) vwheels[i].cnode->scale(value, value, value);
    //}
    BES_GFX_STOP(BES_GFX_ScaleTruck);
}

void Beam::initSimpleSkeleton()
{
    m_skeletonview_manual_mesh = gEnv->sceneManager->createManualObject();

    m_skeletonview_manual_mesh->estimateIndexCount(ar_num_beams * 2);
    m_skeletonview_manual_mesh->setCastShadows(false);
    m_skeletonview_manual_mesh->setDynamic(true);
    m_skeletonview_manual_mesh->setRenderingDistance(300);
    m_skeletonview_manual_mesh->begin("vehicle-skeletonview-material", RenderOperation::OT_LINE_LIST);
    for (int i = 0; i < ar_num_beams; i++)
    {
        m_skeletonview_manual_mesh->position(ar_beams[i].p1->AbsPosition);
        m_skeletonview_manual_mesh->colour(1.0f, 1.0f, 1.0f);
        m_skeletonview_manual_mesh->position(ar_beams[i].p2->AbsPosition);
        m_skeletonview_manual_mesh->colour(0.0f, 0.0f, 0.0f);
    }
    m_skeletonview_manual_mesh->end();
    m_skeletonview_scenenode->attachObject(m_skeletonview_manual_mesh);
    m_skeletonview_mesh_initialized = true;
}

void Beam::updateSimpleSkeleton()
{
    BES_GFX_START(BES_GFX_UpdateSkeleton);

    ColourValue color;

    if (!m_skeletonview_mesh_initialized)
        initSimpleSkeleton();

    m_skeletonview_manual_mesh->beginUpdate(0);
    for (int i = 0; i < ar_num_beams; i++)
    {
        float stress_ratio = ar_beams[i].stress / ar_beams[i].minmaxposnegstress;
        float color_scale = std::abs(stress_ratio);
        color_scale = std::min(color_scale, 1.0f);

        if (stress_ratio <= 0)
            color = ColourValue(0.2f, 1.0f - color_scale, color_scale, 0.8f);
        else
            color = ColourValue(color_scale, 1.0f - color_scale, 0.2f, 0.8f);

        m_skeletonview_manual_mesh->position(ar_beams[i].p1->AbsPosition);
        m_skeletonview_manual_mesh->colour(color);

        // remove broken beams
        if (ar_beams[i].bm_broken || ar_beams[i].bm_disabled)
            m_skeletonview_manual_mesh->position(ar_beams[i].p1->AbsPosition); // Start+End on same point -> beam will not be visible
        else
            m_skeletonview_manual_mesh->position(ar_beams[i].p2->AbsPosition);

        m_skeletonview_manual_mesh->colour(color);
    }
    m_skeletonview_manual_mesh->end();

    BES_GFX_STOP(BES_GFX_UpdateSkeleton);
}

void Beam::moveOrigin(Vector3 offset)
{
    ar_origin += offset;
    for (int i = 0; i < ar_num_nodes; i++)
    {
        ar_nodes[i].RelPosition -= offset;
    }
}

float Beam::getRotation()
{
    Vector3 cur_dir = getDirection();

    return atan2(cur_dir.dotProduct(Vector3::UNIT_X), cur_dir.dotProduct(-Vector3::UNIT_Z));
}

Vector3 Beam::getDirection()
{
    Vector3 cur_dir = ar_nodes[0].AbsPosition;
    if (ar_camera_node_pos[0] != ar_camera_node_dir[0] && this->IsNodeIdValid(ar_camera_node_pos[0]) && this->IsNodeIdValid(ar_camera_node_dir[0]))
    {
        cur_dir = ar_nodes[ar_camera_node_pos[0]].RelPosition - ar_nodes[ar_camera_node_dir[0]].RelPosition;
    }
    else if (ar_num_nodes > 1)
    {
        float max_dist = 0.0f;
        int furthest_node = 1;
        for (int i = 0; i < ar_num_nodes; i++)
        {
            float dist = ar_nodes[i].RelPosition.squaredDistance(ar_nodes[0].RelPosition);
            if (dist > max_dist)
            {
                max_dist = dist;
                furthest_node = i;
            }
        }
        cur_dir = ar_nodes[0].RelPosition - ar_nodes[furthest_node].RelPosition;
    }

    cur_dir.normalise();

    return cur_dir;
}

Vector3 Beam::getPosition()
{
    return m_avg_node_position; //the position is already in absolute position
}

void Beam::CreateSimpleSkeletonMaterial()
{
    if (MaterialManager::getSingleton().resourceExists("vehicle-skeletonview-material"))
    {
        return;
    }

    MaterialPtr mat = (MaterialPtr)(MaterialManager::getSingleton().create("vehicle-skeletonview-material", ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME));

    mat->getTechnique(0)->getPass(0)->createTextureUnitState();
    mat->getTechnique(0)->getPass(0)->getTextureUnitState(0)->setTextureFiltering(TFO_ANISOTROPIC);
    mat->getTechnique(0)->getPass(0)->getTextureUnitState(0)->setTextureAnisotropy(3);
    mat->setLightingEnabled(false);
    mat->setReceiveShadows(false);
}

void Beam::pushNetwork(char* data, int size)
{
    BES_GFX_START(BES_GFX_pushNetwork);
    if (!oob3)
        return;

    // check if the size of the data matches to what we expected
    if ((unsigned int)size == (m_net_buffer_size + sizeof(RoRnet::TruckState)))
    {
        // we walk through the incoming data and separate it a bit
        char* ptr = data;

        // put the RoRnet::TruckState in front, describes truck basics, engine state, flares, etc
        memcpy((char*)oob3, ptr, sizeof(RoRnet::TruckState));
        ptr += sizeof(RoRnet::TruckState);

        // then copy the node data
        memcpy((char*)netb3, ptr, m_net_node_buf_size);
        ptr += m_net_node_buf_size;

        // then take care of the wheel speeds
        for (int i = 0; i < free_wheel; i++)
        {
            float wspeed = *(float*)(ptr);
            wheels[i].wh_net_rp3 = wspeed;

            ptr += sizeof(float);
        }
    }
    else
    {
        // TODO: show the user the problem in the GUI
        LogFormat("[RoR|Network] Wrong data size: %d bytes, expected %d bytes, actor ID: %d",
            size, m_net_buffer_size+sizeof(RoRnet::TruckState), ar_instance_id);
        ar_sim_state = SimState::INVALID;
        return;
    }

    // and the buffer switching to have linear smoothing
    RoRnet::TruckState* ot;
    ot = oob1;
    oob1 = oob2;
    oob2 = oob3;
    oob3 = ot;

    char* ft;
    ft = netb1;
    netb1 = netb2;
    netb2 = netb3;
    netb3 = ft;

    for (int i = 0; i < free_wheel; i++)
    {
        float rp;
        rp = wheels[i].wh_net_rp1;
        wheels[i].wh_net_rp1 = wheels[i].wh_net_rp2;
        wheels[i].wh_net_rp2 = wheels[i].wh_net_rp3;
        wheels[i].wh_net_rp3 = rp;
    }
    m_net_update_counter++;

    BES_GFX_STOP(BES_GFX_pushNetwork);
}

void Beam::calcNetwork()
{
    using namespace RoRnet;

    if (m_net_update_counter < 1)
        return;

    if (m_net_update_counter == 1)
    {
        memcpy((char*)oob1, oob2, sizeof(RoRnet::TruckState));
    }

    BES_GFX_START(BES_GFX_calcNetwork);

    // we must update Nodes positions from available network informations
    int tnow = ar_net_timer.getMilliseconds();
    // adjust offset to match remote time
    int rnow = tnow + m_net_time_offset;
    // if we receive older data from the future, we must correct the offset
    if (oob1->time > rnow)
    {
        m_net_time_offset = oob1->time - tnow;
        rnow = tnow + m_net_time_offset;
    }
    //if we receive last data from the past, we must correct the offset
    if (oob2->time < rnow)
    {
        m_net_time_offset = oob2->time - tnow;
        rnow = tnow + m_net_time_offset;
    }
    float tratio = (float)(rnow - oob1->time) / (float)(oob2->time - oob1->time);

    short* sp1 = (short*)(netb1 + sizeof(float) * 3);
    short* sp2 = (short*)(netb2 + sizeof(float) * 3);
    Vector3 p1ref = Vector3::ZERO;
    Vector3 p2ref = Vector3::ZERO;
    Vector3 apos = Vector3::ZERO;
    Vector3 p1 = Vector3::ZERO;
    Vector3 p2 = Vector3::ZERO;

    for (int i = 0; i < m_net_first_wheel_node; i++)
    {
        if (i == 0)
        {
            // first node is uncompressed
            p1.x = ((float*)netb1)[0];
            p1.y = ((float*)netb1)[1];
            p1.z = ((float*)netb1)[2];
            p1ref = p1;

            p2.x = ((float*)netb2)[0];
            p2.y = ((float*)netb2)[1];
            p2.z = ((float*)netb2)[2];
            p2ref = p2;
        }
        else
        {
            // all other nodes are compressed:
            // short int compared to previous node
            p1.x = (float)(sp1[(i - 1) * 3 + 0]) / 300.0f;
            p1.y = (float)(sp1[(i - 1) * 3 + 1]) / 300.0f;
            p1.z = (float)(sp1[(i - 1) * 3 + 2]) / 300.0f;
            p1 = p1 + p1ref;

            p2.x = (float)(sp2[(i - 1) * 3 + 0]) / 300.0f;
            p2.y = (float)(sp2[(i - 1) * 3 + 1]) / 300.0f;
            p2.z = (float)(sp2[(i - 1) * 3 + 2]) / 300.0f;
            p2 = p2 + p2ref;
        }

        // linear interpolation
        ar_nodes[i].AbsPosition = p1 + tratio * (p2 - p1);
        ar_nodes[i].RelPosition = ar_nodes[i].AbsPosition - ar_origin;

        apos += ar_nodes[i].AbsPosition;
    }
    m_avg_node_position = apos / m_net_first_wheel_node;

    for (int i = 0; i < free_wheel; i++)
    {
        float rp = wheels[i].wh_net_rp1 + tratio * (wheels[i].wh_net_rp2 - wheels[i].wh_net_rp1);
        //compute ideal positions
        Vector3 axis = wheels[i].wh_axis_node_1->RelPosition - wheels[i].wh_axis_node_0->RelPosition;
        axis.normalise();
        Plane pplan = Plane(axis, wheels[i].wh_axis_node_0->AbsPosition);
        Vector3 ortho = -pplan.projectVector(wheels[i].wh_near_attach_node->AbsPosition) - wheels[i].wh_axis_node_0->AbsPosition;
        Vector3 ray = ortho.crossProduct(axis);
        ray.normalise();
        ray *= wheels[i].wh_radius;
        float drp = Math::TWO_PI / (wheels[i].wh_num_nodes / 2);
        for (int j = 0; j < wheels[i].wh_num_nodes / 2; j++)
        {
            Vector3 uray = Quaternion(Radian(rp - drp * j), axis) * ray;

            wheels[i].wh_nodes[j * 2 + 0]->AbsPosition = wheels[i].wh_axis_node_0->AbsPosition + uray;
            wheels[i].wh_nodes[j * 2 + 0]->RelPosition = wheels[i].wh_nodes[j * 2]->AbsPosition - ar_origin;

            wheels[i].wh_nodes[j * 2 + 1]->AbsPosition = wheels[i].wh_axis_node_1->AbsPosition + uray;
            wheels[i].wh_nodes[j * 2 + 1]->RelPosition = wheels[i].wh_nodes[j * 2 + 1]->AbsPosition - ar_origin;
        }
    }

    float engspeed = oob1->engine_speed + tratio * (oob2->engine_speed - oob1->engine_speed);
    float engforce = oob1->engine_force + tratio * (oob2->engine_force - oob1->engine_force);
    float engclutch = oob1->engine_clutch + tratio * (oob2->engine_clutch - oob1->engine_clutch);
    float netwspeed = oob1->wheelspeed + tratio * (oob2->wheelspeed - oob1->wheelspeed);
    float netbrake = oob1->brake + tratio * (oob2->brake - oob1->brake);

    ar_hydro_dir_wheel_display = oob1->hydrodirstate;
    ar_wheel_speed = netwspeed;

    int gear = oob1->engine_gear;
    unsigned int flagmask = oob1->flagmask;

    if (ar_engine)
    {
        SOUND_MODULATE(ar_instance_id, SS_MOD_ENGINE, engspeed);
    }
    if (ar_num_aeroengines > 0)
    {
        SOUND_MODULATE(ar_instance_id, SS_MOD_AEROENGINE1, engspeed);
        SOUND_MODULATE(ar_instance_id, SS_MOD_AEROENGINE2, engspeed);
        SOUND_MODULATE(ar_instance_id, SS_MOD_AEROENGINE3, engspeed);
        SOUND_MODULATE(ar_instance_id, SS_MOD_AEROENGINE4, engspeed);
    }

    ar_brake = netbrake;

    if (ar_engine)
    {
        int automode = -1;
             if ((flagmask & NETMASK_ENGINE_MODE_AUTOMATIC)     != 0) { automode = static_cast<int>(SimGearboxMode::AUTO); }
        else if ((flagmask & NETMASK_ENGINE_MODE_SEMIAUTO)      != 0) { automode = static_cast<int>(SimGearboxMode::SEMI_AUTO); }
        else if ((flagmask & NETMASK_ENGINE_MODE_MANUAL)        != 0) { automode = static_cast<int>(SimGearboxMode::MANUAL); }
        else if ((flagmask & NETMASK_ENGINE_MODE_MANUAL_STICK)  != 0) { automode = static_cast<int>(SimGearboxMode::MANUAL_STICK); }
        else if ((flagmask & NETMASK_ENGINE_MODE_MANUAL_RANGES) != 0) { automode = static_cast<int>(SimGearboxMode::MANUAL_RANGES); }

        bool contact = ((flagmask & NETMASK_ENGINE_CONT) != 0);
        bool running = ((flagmask & NETMASK_ENGINE_RUN) != 0);

        ar_engine->netForceSettings(engspeed, engforce, engclutch, gear, running, contact, automode);
    }

    // set particle cannon
    if (((flagmask & NETMASK_PARTICLE) != 0) != m_custom_particles_enabled)
        toggleCustomParticles();

    // set lights
    if (((flagmask & NETMASK_LIGHTS) != 0) != ar_lights)
        lightsToggle();
    if (((flagmask & NETMASK_BEACONS) != 0) != m_beacon_light_is_active)
        beaconsToggle();

    m_antilockbrake = flagmask & NETMASK_ALB_ACTIVE;
    m_tractioncontrol = flagmask & NETMASK_TC_ACTIVE;
    ar_parking_brake = flagmask & NETMASK_PBRAKE;

    blinktype btype = BLINK_NONE;
    if ((flagmask & NETMASK_BLINK_LEFT) != 0)
        btype = BLINK_LEFT;
    else if ((flagmask & NETMASK_BLINK_RIGHT) != 0)
        btype = BLINK_RIGHT;
    else if ((flagmask & NETMASK_BLINK_WARN) != 0)
        btype = BLINK_WARN;
    setBlinkType(btype);

    setCustomLightVisible(0, ((flagmask & NETMASK_CLIGHT1) > 0));
    setCustomLightVisible(1, ((flagmask & NETMASK_CLIGHT2) > 0));
    setCustomLightVisible(2, ((flagmask & NETMASK_CLIGHT3) > 0));
    setCustomLightVisible(3, ((flagmask & NETMASK_CLIGHT4) > 0));

    m_net_brake_light = ((flagmask & NETMASK_BRAKES) != 0);
    m_net_reverse_light = ((flagmask & NETMASK_REVERSE) != 0);

    if ((flagmask & NETMASK_HORN))
        SOUND_START(ar_instance_id, SS_TRIG_HORN);
    else
        SOUND_STOP(ar_instance_id, SS_TRIG_HORN);

    if (m_net_reverse_light)
        SOUND_START(ar_instance_id, SS_TRIG_REVERSE_GEAR);
    else
        SOUND_STOP(ar_instance_id, SS_TRIG_REVERSE_GEAR);

    updateDashBoards(tratio);

    BES_GFX_STOP(BES_GFX_calcNetwork);
}

bool Beam::addPressure(float v)
{
    if (!ar_free_pressure_beam)
        return false;

    float newpressure = std::max(0.0f, std::min(m_ref_tyre_pressure + v, 100.0f));

    if (newpressure == m_ref_tyre_pressure)
        return false;

    m_ref_tyre_pressure = newpressure;
    for (int i = 0; i < ar_free_pressure_beam; i++)
    {
        ar_beams[ar_pressure_beams[i]].k = 10000 + m_ref_tyre_pressure * 10000;
    }
    return true;
}

float Beam::getPressure()
{
    if (ar_free_pressure_beam)
        return m_ref_tyre_pressure;
    return 0;
}

void Beam::calc_masses2(Real total, bool reCalc)
{
    BES_GFX_START(BES_GFX_calc_masses2);

    bool debugMass = App::diag_truck_mass.GetActive();

    //reset
    for (int i = 0; i < ar_num_nodes; i++)
    {
        if (!ar_nodes[i].iswheel)
        {
            if (!ar_nodes[i].loadedMass)
            {
                ar_nodes[i].mass = 0;
            }
            else if (!ar_nodes[i].overrideMass)
            {
                ar_nodes[i].mass = m_load_mass / (float)m_masscount;
            }
        }
    }
    //average linear density
    Real len = 0.0f;
    for (int i = 0; i < ar_num_beams; i++)
    {
        if (ar_beams[i].bm_type != BEAM_VIRTUAL)
        {
            Real half_newlen = ar_beams[i].L / 2.0;
            if (!(ar_beams[i].p1->iswheel))
                len += half_newlen;
            if (!(ar_beams[i].p2->iswheel))
                len += half_newlen;
        }
    }

    if (!reCalc)
    {
        for (int i = 0; i < ar_num_beams; i++)
        {
            if (ar_beams[i].bm_type != BEAM_VIRTUAL)
            {
                Real half_mass = ar_beams[i].L * total / len / 2.0f;
                if (!(ar_beams[i].p1->iswheel))
                    ar_beams[i].p1->mass += half_mass;
                if (!(ar_beams[i].p2->iswheel))
                    ar_beams[i].p2->mass += half_mass;
            }
        }
    }
    //fix rope masses
    for (std::vector<rope_t>::iterator it = ropes.begin(); it != ropes.end(); it++)
    {
        it->beam->p2->mass = 100.0f;
    }

    // Apply pre-defined cinecam node mass
    for (int i = 0; i < this->ar_num_cinecams; ++i)
    {
        // TODO: this expects all cinecams to be defined in root module (i.e. outside 'section/end_section')
        ar_nodes[ar_cinecam_node[i]].mass = m_definition->root_module->cinecam[i].node_mass;
    }

    //update mass
    for (int i = 0; i < ar_num_nodes; i++)
    {
        //LOG("Nodemass "+TOSTRING(i)+"-"+TOSTRING(ar_nodes[i].mass));
        //for stability
        if (!ar_nodes[i].iswheel && ar_nodes[i].mass < m_minimass)
        {
            if (App::diag_truck_mass.GetActive())
            {
                char buf[300];
                snprintf(buf, 300, "Node '%d' mass (%f Kg) is too light. Resetting to 'minimass' (%f Kg)", i, ar_nodes[i].mass, m_minimass);
                LOG(buf);
            }
            ar_nodes[i].mass = m_minimass;
        }
    }

    m_total_mass = 0;
    for (int i = 0; i < ar_num_nodes; i++)
    {
        if (App::diag_truck_mass.GetActive())
        {
            String msg = "Node " + TOSTRING(i) + " : " + TOSTRING((int)ar_nodes[i].mass) + " kg";
            if (ar_nodes[i].loadedMass)
            {
                if (ar_nodes[i].overrideMass)
                    msg += " (overriden by node mass)";
                else
                    msg += " (normal load node: " + TOSTRING(m_load_mass) + " kg / " + TOSTRING(m_masscount) + " nodes)";
            }
            LOG(msg);
        }
        m_total_mass += ar_nodes[i].mass;
    }
    LOG("TOTAL VEHICLE MASS: " + TOSTRING((int)m_total_mass) +" kg");

    BES_GFX_STOP(BES_GFX_calc_masses2);
}

// this recalculates the masses (useful when the gravity was changed...)
void Beam::recalc_masses()
{
    this->calc_masses2(m_total_mass, true);
}

float Beam::getTotalMass(bool withLocked)
{
    if (!withLocked)
        return m_total_mass; // already computed in calc_masses2

    float mass = m_total_mass;

    for (std::list<Beam*>::iterator it = m_linked_actors.begin(); it != m_linked_actors.end(); ++it)
    {
        mass += (*it)->m_total_mass;
    }

    return mass;
}

void Beam::determineLinkedBeams()
{
    m_linked_actors.clear();

    bool found = true;
    std::map<Beam*, bool> lookup_table;
    std::pair<std::map<Beam*, bool>::iterator, bool> ret;

    lookup_table.insert(std::pair<Beam*, bool>(this, false));
    
    auto interTruckLinks = App::GetSimController()->GetBeamFactory()->interTruckLinks;

    while (found)
    {
        found = false;

        for (std::map<Beam*, bool>::iterator it_beam = lookup_table.begin(); it_beam != lookup_table.end(); ++it_beam)
        {
            if (!it_beam->second)
            {
                auto truck = it_beam->first;
                for (auto it = interTruckLinks.begin(); it != interTruckLinks.end(); it++)
                {
                    auto truck_pair = it->second;
                    if (truck == truck_pair.first || truck == truck_pair.second)
                    {
                        auto other_truck = (truck != truck_pair.first) ? truck_pair.first : truck_pair.second;
                        ret = lookup_table.insert(std::pair<Beam*, bool>(other_truck, false));
                        if (ret.second)
                        {
                            m_linked_actors.push_back(other_truck);
                            found = true;
                        }
                    }
                }
                it_beam->second = true;
            }
        }
    }
}

int Beam::getWheelNodeCount()
{
    return m_wheel_node_count;
}

void Beam::calcNodeConnectivityGraph()
{
    BES_GFX_START(BES_GFX_calcNodeConnectivityGraph);
    int i;

    ar_node_to_node_connections.resize(ar_num_nodes, std::vector<int>());
    ar_node_to_beam_connections.resize(ar_num_nodes, std::vector<int>());

    for (i = 0; i < ar_num_beams; i++)
    {
        if (ar_beams[i].p1 != NULL && ar_beams[i].p2 != NULL && ar_beams[i].p1->pos >= 0 && ar_beams[i].p2->pos >= 0)
        {
            ar_node_to_node_connections[ar_beams[i].p1->pos].push_back(ar_beams[i].p2->pos);
            ar_node_to_beam_connections[ar_beams[i].p1->pos].push_back(i);
            ar_node_to_node_connections[ar_beams[i].p2->pos].push_back(ar_beams[i].p1->pos);
            ar_node_to_beam_connections[ar_beams[i].p2->pos].push_back(i);
        }
    }
    BES_GFX_STOP(BES_GFX_calcNodeConnectivityGraph);
}

Vector3 Beam::calculateCollisionOffset(Vector3 direction)
{
    if (direction == Vector3::ZERO)
        return Vector3::ZERO;

    AxisAlignedBox bb = ar_bounding_box;
    bb.merge(ar_bounding_box.getMinimum() + direction);
    bb.merge(ar_bounding_box.getMaximum() + direction);

    Real max_distance = direction.length();
    direction.normalise();

    Beam** trucks = App::GetSimController()->GetBeamFactory()->getTrucks();
    int trucksnum = App::GetSimController()->GetBeamFactory()->getTruckCount();

    if (m_intra_point_col_detector)
        m_intra_point_col_detector->update(this, true);

    if (m_inter_point_col_detector)
        m_inter_point_col_detector->update(this, trucks, trucksnum, true);

    // collision displacement
    Vector3 collision_offset = Vector3::ZERO;

    for (int t = 0; t < trucksnum; t++)
    {
        if (!trucks[t])
            continue;
        if (t == ar_instance_id)
            continue;
        if (!bb.intersects(trucks[t]->ar_bounding_box))
            continue;

        // Test own contacters against others cabs
        if (m_intra_point_col_detector)
        {
            for (int i = 0; i < trucks[t]->ar_num_collcabs; i++)
            {
                if (collision_offset.length() >= max_distance)
                    break;
                Vector3 offset = collision_offset;
                while (offset.length() < max_distance)
                {
                    int tmpv = trucks[t]->ar_collcabs[i] * 3;
                    node_t* no = &trucks[t]->ar_nodes[ar_cabs[tmpv]];
                    node_t* na = &trucks[t]->ar_nodes[ar_cabs[tmpv + 1]];
                    node_t* nb = &trucks[t]->ar_nodes[ar_cabs[tmpv + 2]];

                    m_intra_point_col_detector->query(no->AbsPosition + offset,
                        na->AbsPosition + offset,
                        nb->AbsPosition + offset,
                        ar_collision_range);

                    if (m_intra_point_col_detector->hit_count == 0)
                    {
                        collision_offset = offset;
                        break;
                    }
                    offset += direction * 0.01f;
                }
            }
        }

        float proximity = 0.05f;
        proximity = std::max(proximity, ar_bounding_box.getSize().length() / 50.0f);
        proximity = std::max(proximity, trucks[t]->ar_bounding_box.getSize().length() / 50.0f);

        // Test proximity of own nodes against others nodes
        for (int i = 0; i < ar_num_nodes; i++)
        {
            if (ar_nodes[i].contactless)
                continue;
            if (collision_offset.length() >= max_distance)
                break;
            Vector3 offset = collision_offset;
            while (offset.length() < max_distance)
            {
                Vector3 query_position = ar_nodes[i].AbsPosition + offset;

                bool node_proximity = false;

                for (int j = 0; j < trucks[t]->ar_num_nodes; j++)
                {
                    if (trucks[t]->ar_nodes[j].contactless)
                        continue;
                    if (query_position.squaredDistance(trucks[t]->ar_nodes[j].AbsPosition) < proximity)
                    {
                        node_proximity = true;
                        break;
                    }
                }

                if (!node_proximity)
                {
                    collision_offset = offset;
                    break;
                }
                offset += direction * 0.01f;
            }
        }
    }

    // Test own cabs against others contacters
    if (m_inter_point_col_detector)
    {
        for (int i = 0; i < ar_num_collcabs; i++)
        {
            if (collision_offset.length() >= max_distance)
                break;
            Vector3 offset = collision_offset;
            while (offset.length() < max_distance)
            {
                int tmpv = ar_collcabs[i] * 3;
                node_t* no = &ar_nodes[ar_cabs[tmpv]];
                node_t* na = &ar_nodes[ar_cabs[tmpv + 1]];
                node_t* nb = &ar_nodes[ar_cabs[tmpv + 2]];

                m_inter_point_col_detector->query(no->AbsPosition + offset,
                    na->AbsPosition + offset,
                    nb->AbsPosition + offset,
                    ar_collision_range);

                if (m_inter_point_col_detector->hit_count == 0)
                {
                    collision_offset = offset;
                    break;
                }
                offset += direction * 0.01f;
            }
        }
    }

    return collision_offset;
}

void Beam::resolveCollisions(Vector3 direction)
{
    Vector3 offset = calculateCollisionOffset(direction);

    if (offset == Vector3::ZERO)
        return;

    // Additional 20 cm safe-guard (horizontally)
    Vector3 dir = Vector3(offset.x, 0.0f, offset.z).normalisedCopy();
    offset += 0.2f * dir;

    resetPosition(ar_nodes[0].AbsPosition.x + offset.x, ar_nodes[0].AbsPosition.z + offset.z, true, ar_nodes[ar_lowest_contacting_node].AbsPosition.y + offset.y);
}

void Beam::resolveCollisions(float max_distance, bool consider_up)
{
    Vector3 offset = Vector3::ZERO;

    Vector3 f = max_distance * getDirection();
    Vector3 l = max_distance * Vector3(-sin(getRotation() + Math::HALF_PI), 0.0f, cos(getRotation() + Math::HALF_PI));
    Vector3 u = max_distance * Vector3::UNIT_Y;

    // Calculate an ideal collision avoidance direction (prefer left over right over [front / back / up])

    Vector3 front = calculateCollisionOffset(+f);
    Vector3 back = calculateCollisionOffset(-f);
    Vector3 left = calculateCollisionOffset(+l);
    Vector3 right = calculateCollisionOffset(-l);

    offset = front.length() < back.length() * 1.2f ? front : back;

    Vector3 side = left.length() < right.length() * 1.1f ? left : right;
    if (side.length() < offset.length() + m_min_camera_radius / 2.0f)
        offset = side;

    if (consider_up)
    {
        Vector3 up = calculateCollisionOffset(+u);
        if (up.length() < offset.length())
            offset = up;
    }

    if (offset == Vector3::ZERO)
        return;

    // Additional 20 cm safe-guard (horizontally)
    Vector3 dir = Vector3(offset.x, 0.0f, offset.z).normalisedCopy();
    offset += 0.2f * dir;

    resetPosition(ar_nodes[0].AbsPosition.x + offset.x, ar_nodes[0].AbsPosition.z + offset.z, true, ar_nodes[ar_lowest_contacting_node].AbsPosition.y + offset.y);
}

int Beam::savePosition(int indexPosition)
{
    if (!m_position_storage)
        return -1;
    Vector3* nbuff = m_position_storage->getStorage(indexPosition);
    if (!nbuff)
        return -3;
    for (int i = 0; i < ar_num_nodes; i++)
        nbuff[i] = ar_nodes[i].AbsPosition;
    m_position_storage->setUsage(indexPosition, true);
    return 0;
}

int Beam::loadPosition(int indexPosition)
{
    if (!m_position_storage)
        return -1;
    if (!m_position_storage->getUsage(indexPosition))
        return -2;

    Vector3* nbuff = m_position_storage->getStorage(indexPosition);
    if (!nbuff)
        return -3;
    Vector3 pos = Vector3(0, 0, 0);
    for (int i = 0; i < ar_num_nodes; i++)
    {
        ar_nodes[i].AbsPosition = nbuff[i];
        ar_nodes[i].RelPosition = nbuff[i] - ar_origin;

        // reset forces
        ar_nodes[i].Velocity = Vector3::ZERO;
        ar_nodes[i].Forces = Vector3::ZERO;

        pos = pos + nbuff[i];
    }
    m_avg_node_position = pos / (float)(ar_num_nodes);

    resetSlideNodes();

    return 0;
}

void Beam::calculateAveragePosition()
{
    // calculate average position
    if (ar_custom_camera_node >= 0)
    {
        m_avg_node_position = ar_nodes[ar_custom_camera_node].AbsPosition;
    }
    else if (ar_extern_camera_mode == 1 && ar_num_cinecams > 0)
    {
        // the new (strange) approach: reuse the cinecam node
        m_avg_node_position = ar_nodes[ar_cinecam_node[0]].AbsPosition;
    }
    else if (ar_extern_camera_mode == 2 && ar_extern_camera_node >= 0)
    {
        // the new (strange) approach #2: reuse a specified node
        m_avg_node_position = ar_nodes[ar_extern_camera_node].AbsPosition;
    }
    else
    {
        // the classic approach: average over all nodes and beams
        Vector3 aposition = Vector3::ZERO;
        for (int n = 0; n < ar_num_nodes; n++)
        {
            aposition += ar_nodes[n].AbsPosition;
        }
        m_avg_node_position = aposition / ar_num_nodes;
    }
}

void Beam::updateBoundingBox()
{
    ar_bounding_box = AxisAlignedBox(ar_nodes[0].AbsPosition, ar_nodes[0].AbsPosition);
    for (int i = 0; i < ar_num_nodes; i++)
    {
        ar_bounding_box.merge(ar_nodes[i].AbsPosition);
    }
    ar_bounding_box.setMinimum(ar_bounding_box.getMinimum() - Vector3(0.05f, 0.05f, 0.05f));
    ar_bounding_box.setMaximum(ar_bounding_box.getMaximum() + Vector3(0.05f, 0.05f, 0.05f));
}

void Beam::preUpdatePhysics(float dt)
{
    m_avg_node_position_prev = m_avg_node_position;

    if (ar_nodes[0].RelPosition.squaredLength() > 10000.0)
    {
        moveOrigin(ar_nodes[0].RelPosition);
    }
}

void Beam::postUpdatePhysics(float dt)
{
    calculateAveragePosition();

    // Calculate average truck velocity
    m_avg_node_velocity = (m_avg_node_position - m_avg_node_position_prev) / dt;
}

void Beam::resetAngle(float rot)
{
    // Set origin of rotation to camera node
    Vector3 origin = ar_nodes[0].AbsPosition;

    if (this->IsNodeIdValid(ar_camera_node_pos[0]))
    {
        origin = ar_nodes[ar_camera_node_pos[0]].AbsPosition;
    }

    // Set up matrix for yaw rotation
    Matrix3 matrix;
    matrix.FromEulerAnglesXYZ(Radian(0), Radian(-rot + m_spawn_rotation), Radian(0));

    for (int i = 0; i < ar_num_nodes; i++)
    {
        // Move node back to origin, apply rotation matrix, and move node back
        ar_nodes[i].AbsPosition -= origin;
        ar_nodes[i].AbsPosition = matrix * ar_nodes[i].AbsPosition;
        ar_nodes[i].AbsPosition += origin;
        ar_nodes[i].RelPosition = ar_nodes[i].AbsPosition - this->ar_origin;
    }

    resetSlideNodePositions();

    updateBoundingBox();
    calculateAveragePosition();
}

void Beam::resetPosition(float px, float pz, bool setInitPosition, float miny)
{
    // horizontal displacement
    Vector3 offset = Vector3(px, ar_nodes[0].AbsPosition.y, pz) - ar_nodes[0].AbsPosition;
    for (int i = 0; i < ar_num_nodes; i++)
    {
        ar_nodes[i].AbsPosition += offset;
        ar_nodes[i].RelPosition = ar_nodes[i].AbsPosition - ar_origin;
    }

    // vertical displacement
    float vertical_offset = -ar_nodes[ar_lowest_contacting_node].AbsPosition.y + miny;
    if (gEnv->terrainManager->getWater())
    {
        vertical_offset += std::max(0.0f, gEnv->terrainManager->getWater()->GetStaticWaterHeight() - (ar_nodes[ar_lowest_contacting_node].AbsPosition.y + vertical_offset));
    }
    for (int i = 1; i < ar_num_nodes; i++)
    {
        if (ar_nodes[i].contactless)
            continue;
        float terrainHeight = gEnv->terrainManager->getHeightFinder()->getHeightAt(ar_nodes[i].AbsPosition.x, ar_nodes[i].AbsPosition.z);
        vertical_offset += std::max(0.0f, terrainHeight - (ar_nodes[i].AbsPosition.y + vertical_offset));
    }
    for (int i = 0; i < ar_num_nodes; i++)
    {
        ar_nodes[i].AbsPosition.y += vertical_offset;
        ar_nodes[i].RelPosition = ar_nodes[i].AbsPosition - ar_origin;
    }

    // mesh displacement
    float mesh_offset = 0.0f;
    for (int i = 0; i < ar_num_nodes; i++)
    {
        if (mesh_offset >= 1.0f)
            break;
        if (ar_nodes[i].contactless)
            continue;
        float offset = mesh_offset;
        while (offset < 1.0f)
        {
            Vector3 query = ar_nodes[i].AbsPosition + Vector3(0.0f, offset, 0.0f);
            if (!gEnv->collisions->collisionCorrect(&query, false))
            {
                mesh_offset = offset;
                break;
            }
            offset += 0.001f;
        }
    }
    for (int i = 0; i < ar_num_nodes; i++)
    {
        ar_nodes[i].AbsPosition.y += mesh_offset;
        ar_nodes[i].RelPosition = ar_nodes[i].AbsPosition - ar_origin;
    }

    resetPosition(Vector3::ZERO, setInitPosition);
}

void Beam::resetPosition(Vector3 translation, bool setInitPosition)
{
    // total displacement
    if (translation != Vector3::ZERO)
    {
        Vector3 offset = translation - ar_nodes[0].AbsPosition;
        for (int i = 0; i < ar_num_nodes; i++)
        {
            ar_nodes[i].AbsPosition += offset;
            ar_nodes[i].RelPosition = ar_nodes[i].AbsPosition - ar_origin;
        }
    }

    if (setInitPosition)
    {
        for (int i = 0; i < ar_num_nodes; i++)
        {
            ar_nodes[i].initial_pos = ar_nodes[i].AbsPosition;
        }
    }

    updateBoundingBox();
    calculateAveragePosition();

    // calculate minimum camera radius
    if (m_min_camera_radius < 0.0f)
    {
        for (int i = 0; i < ar_num_nodes; i++)
        {
            Real dist = ar_nodes[i].AbsPosition.squaredDistance(m_avg_node_position);
            if (dist > m_min_camera_radius)
            {
                m_min_camera_radius = dist;
            }
        }
        m_min_camera_radius = std::sqrt(m_min_camera_radius) * 1.2f; // twenty percent buffer
    }

    resetSlideNodePositions();
}

void Beam::mouseMove(int node, Vector3 pos, float force)
{
    m_mouse_grab_node = node;
    m_mouse_grab_move_force = force;
    m_mouse_grab_pos = pos;
}

bool Beam::hasDriverSeat()
{
    return ar_driverseat_prop != 0;
}

void Beam::calculateDriverPos(Vector3& out_pos, Quaternion& out_rot)
{
    assert(this->ar_driverseat_prop != nullptr);

    BES_GFX_START(BES_GFX_calculateDriverPos);

    Vector3 x_pos = ar_nodes[ar_driverseat_prop->nodex].AbsPosition;
    Vector3 y_pos = ar_nodes[ar_driverseat_prop->nodey].AbsPosition;
    Vector3 center_pos = ar_nodes[ar_driverseat_prop->noderef].AbsPosition;

    Vector3 x_vec = x_pos - center_pos;
    Vector3 y_vec = y_pos - center_pos;

    Vector3 normal = (y_vec.crossProduct(x_vec)).normalisedCopy();

    // Output position
    Vector3 pos = center_pos;
    pos += (this->ar_driverseat_prop->offsetx * x_vec);
    pos += (this->ar_driverseat_prop->offsety * y_vec);
    pos += (this->ar_driverseat_prop->offsetz * normal);
    out_pos = pos;

    // Output orientation
    Vector3 x_vec_norm = x_vec.normalisedCopy();
    Vector3 y_vec_norm = x_vec_norm.crossProduct(normal);
    Quaternion rot(x_vec_norm, normal, y_vec_norm);
    rot = rot * ar_driverseat_prop->rot;
    rot = rot * Quaternion(Degree(180), Vector3::UNIT_Y); // rotate towards the driving direction
    out_rot = rot;

    BES_GFX_STOP(BES_GFX_calculateDriverPos);
}

void Beam::resetAutopilot()
{
    ar_autopilot->disconnect();
    OverlayManager::getSingleton().getOverlayElement("tracks/ap_hdg_but")->setMaterialName("tracks/hdg-off");
    OverlayManager::getSingleton().getOverlayElement("tracks/ap_wlv_but")->setMaterialName("tracks/wlv-off");
    OverlayManager::getSingleton().getOverlayElement("tracks/ap_nav_but")->setMaterialName("tracks/nav-off");
    OverlayManager::getSingleton().getOverlayElement("tracks/ap_alt_but")->setMaterialName("tracks/hold-off");
    OverlayManager::getSingleton().getOverlayElement("tracks/ap_vs_but")->setMaterialName("tracks/vs-off");
    OverlayManager::getSingleton().getOverlayElement("tracks/ap_ias_but")->setMaterialName("tracks/athr-off");
    OverlayManager::getSingleton().getOverlayElement("tracks/ap_gpws_but")->setMaterialName("tracks/gpws-on");
    OverlayManager::getSingleton().getOverlayElement("tracks/ap_brks_but")->setMaterialName("tracks/brks-off");
    OverlayManager::getSingleton().getOverlayElement("tracks/ap_hdg_val")->setCaption("000");
    OverlayManager::getSingleton().getOverlayElement("tracks/ap_alt_val")->setCaption("1000");
    OverlayManager::getSingleton().getOverlayElement("tracks/ap_vs_val")->setCaption("000");
    OverlayManager::getSingleton().getOverlayElement("tracks/ap_ias_val")->setCaption("150");
}

void Beam::disconnectAutopilot()
{
    ar_autopilot->disconnect();
    OverlayManager::getSingleton().getOverlayElement("tracks/ap_hdg_but")->setMaterialName("tracks/hdg-off");
    OverlayManager::getSingleton().getOverlayElement("tracks/ap_wlv_but")->setMaterialName("tracks/wlv-off");
    OverlayManager::getSingleton().getOverlayElement("tracks/ap_nav_but")->setMaterialName("tracks/nav-off");
    OverlayManager::getSingleton().getOverlayElement("tracks/ap_alt_but")->setMaterialName("tracks/hold-off");
    OverlayManager::getSingleton().getOverlayElement("tracks/ap_vs_but")->setMaterialName("tracks/vs-off");
    OverlayManager::getSingleton().getOverlayElement("tracks/ap_ias_but")->setMaterialName("tracks/athr-off");
}

void Beam::toggleAxleLock()
{
    for (int i = 0; i < m_num_axles; ++i)
    {
        if (!m_axles[i])
            continue;
        m_axles[i]->toggleDiff();
    }
}

int Beam::getAxleLockCount()
{
    return m_num_axles;
}

String Beam::getAxleLockName()
{
    if (!m_axles[0])
        return String();
    return m_axles[0]->getDiffTypeName();
}

void Beam::reset(bool keepPosition)
{
    if (keepPosition)
        m_reset_request = REQUEST_RESET_ON_SPOT;
    else
        m_reset_request = REQUEST_RESET_ON_INIT_POS;
}

void Beam::displace(Vector3 translation, float rotation)
{
    if (rotation != 0.0f)
    {
        Vector3 rotation_center = getRotationCenter();
        Quaternion rot = Quaternion(Radian(rotation), Vector3::UNIT_Y);

        for (int i = 0; i < ar_num_nodes; i++)
        {
            ar_nodes[i].AbsPosition -= rotation_center;
            ar_nodes[i].AbsPosition = rot * ar_nodes[i].AbsPosition;
            ar_nodes[i].AbsPosition += rotation_center;
            ar_nodes[i].RelPosition = ar_nodes[i].AbsPosition - ar_origin;
        }
    }

    if (translation != Vector3::ZERO)
    {
        for (int i = 0; i < ar_num_nodes; i++)
        {
            ar_nodes[i].AbsPosition += translation;
            ar_nodes[i].RelPosition = ar_nodes[i].AbsPosition - ar_origin;
        }
    }

    if (rotation != 0.0f || translation != Vector3::ZERO)
    {
        updateBoundingBox();
        calculateAveragePosition();
    }
}

Ogre::Vector3 Beam::getRotationCenter()
{
    Vector3 rotation_center = Vector3::ZERO;

    if (m_is_cinecam_rotation_center)
    {
        Vector3 cinecam = ar_nodes[0].AbsPosition;
        if (this->IsNodeIdValid(ar_camera_node_pos[0])) // TODO: Check cam. nodes once on spawn! They never change --> no reason to repeat the check. ~only_a_ptr, 06/2017
        {
            cinecam = ar_nodes[ar_camera_node_pos[0]].AbsPosition;
        }
        rotation_center = cinecam;
    }
    else
    {
        Vector3 sum = Vector3::ZERO;
        for (int i = 0; i < ar_num_nodes; i++)
        {
            sum += ar_nodes[i].AbsPosition;
        }
        rotation_center = sum / ar_num_nodes;
    }

    return rotation_center;
}

void Beam::SyncReset()
{
    ar_hydro_dir_state = 0.0;
    ar_hydro_aileron_state = 0.0;
    ar_hydro_rudder_state = 0.0;
    ar_hydro_elevator_state = 0.0;
    ar_hydro_dir_wheel_display = 0.0;
    if (m_hydro_inertia)
        m_hydro_inertia->resetCmdKeyDelay();
    ar_parking_brake = 0;
    cc_mode = false;
    ar_fusedrag = Vector3::ZERO;
    ar_origin = Vector3::ZERO;
    float yPos = ar_nodes[ar_lowest_contacting_node].AbsPosition.y;

    Vector3 cur_position = ar_nodes[0].AbsPosition;
    float cur_rot = getRotation();
    if (ar_engine)
    {
        ar_engine->start();
    }

    for (int i = 0; i < ar_num_nodes; i++)
    {
        ar_nodes[i].AbsPosition = ar_nodes[i].initial_pos;
        ar_nodes[i].RelPosition = ar_nodes[i].initial_pos - ar_origin;
        ar_nodes[i].Velocity = Vector3::ZERO;
        ar_nodes[i].Forces = Vector3::ZERO;
    }

    for (int i = 0; i < ar_num_beams; i++)
    {
        ar_beams[i].maxposstress    = ar_beams[i].default_beam_deform;
        ar_beams[i].maxnegstress    = -ar_beams[i].default_beam_deform;
        ar_beams[i].minmaxposnegstress = ar_beams[i].default_beam_deform;
        ar_beams[i].strength        = ar_beams[i].initial_beam_strength;
        ar_beams[i].plastic_coef    = ar_beams[i].default_beam_plastic_coef;
        ar_beams[i].L               = ar_beams[i].refL;
        ar_beams[i].stress          = 0.0;
        ar_beams[i].bm_broken       = false;
        ar_beams[i].bm_disabled     = false;
    }

    disjoinInterTruckBeams();

    for (std::vector<hook_t>::iterator it = ar_hooks.begin(); it != ar_hooks.end(); it++)
    {
        it->beam->bm_disabled = true;
        it->locked = UNLOCKED;
        it->lockNode = 0;
        it->lockTruck = 0;
        it->beam->p2 = &ar_nodes[0];
        it->beam->p2truck = false;
        it->beam->L = (ar_nodes[0].AbsPosition - it->hookNode->AbsPosition).length();
        removeInterTruckBeam(it->beam);
    }
    for (std::vector<rope_t>::iterator it = ropes.begin(); it != ropes.end(); it++)
    {
        it->locked = UNLOCKED;
        if (it->lockedto_ropable)
            it->lockedto_ropable->in_use = false;
        it->lockedto = &ar_nodes[0];
        it->lockedtruck = 0;
    }
    for (std::vector<tie_t>::iterator it = ties.begin(); it != ties.end(); it++)
    {
        it->tied = false;
        it->tying = false;
        if (it->lockedto)
            it->lockedto->in_use = false;
        it->beam->p2 = &ar_nodes[0];
        it->beam->p2truck = false;
        it->beam->bm_disabled = true;
        removeInterTruckBeam(it->beam);
    }

    for (int i = 0; i < ar_num_aeroengines; i++)
        ar_aeroengines[i]->reset();
    for (int i = 0; i < ar_num_screwprops; i++)
        ar_screwprops[i]->reset();
    for (int i = 0; i < ar_num_rotators; i++)
        ar_rotators[i].angle = 0.0;
    for (int i = 0; i < ar_num_wings; i++)
        ar_wings[i].fa->broken = false;
    for (int i = 0; i < free_wheel; i++)
    {
        wheels[i].wh_speed = 0.0;
        wheels[i].wh_is_detached = false;
    }
    if (m_buoyance)
        m_buoyance->setsink(0);
    m_ref_tyre_pressure = 50.0;
    addPressure(0.0);
    if (ar_autopilot)
        resetAutopilot();

    for (int i = 0; i < ar_num_flexbodies; i++)
    {
        ar_flexbodies[i]->reset();
    }

    // reset on spot with backspace
    if (m_reset_request != REQUEST_RESET_ON_INIT_POS)
    {
        resetAngle(cur_rot);
        resetPosition(cur_position.x, cur_position.z, false, yPos);
    }

    // reset commands (self centering && push once/twice forced to terminate moving commands)
    for (int i = 0; i < MAX_COMMANDS; i++)
    {
        commandkey[i].commandValue = 0.0;
        commandkey[i].triggerInputValue = 0.0f;
        commandkey[i].playerInputValue = 0.0f;
    }

    resetSlideNodes();

    if (m_reset_request != REQUEST_RESET_ON_SPOT)
    {
        m_reset_request = REQUEST_RESET_NONE;
    }
    else
    {
        m_reset_request = REQUEST_RESET_FINAL;
    }
}

bool Beam::replayStep()
{
    if (!ar_replay_mode || !m_replay_handler || !m_replay_handler->isValid())
        return false;

    // no replay update needed if position was not changed
    if (ar_replay_pos != m_replay_pos_prev)
    {
        unsigned long time = 0;

        node_simple_t* nbuff = (node_simple_t *)m_replay_handler->getReadBuffer(ar_replay_pos, 0, time);
        if (nbuff)
        {
            for (int i = 0; i < ar_num_nodes; i++)
            {
                ar_nodes[i].AbsPosition = nbuff[i].position;
                ar_nodes[i].RelPosition = nbuff[i].position - ar_origin;

                ar_nodes[i].Velocity = nbuff[i].velocity;
                ar_nodes[i].Forces = nbuff[i].forces;
            }

            updateSlideNodePositions();
            updateBoundingBox();
            calculateAveragePosition();
        }

        beam_simple_t* bbuff = (beam_simple_t *)m_replay_handler->getReadBuffer(ar_replay_pos, 1, time);
        if (bbuff)
        {
            for (int i = 0; i < ar_num_beams; i++)
            {
                ar_beams[i].bm_broken = bbuff[i].broken;
                ar_beams[i].bm_disabled = bbuff[i].disabled;
            }
        }
        m_replay_pos_prev = ar_replay_pos;
    }

    return true;
}

void Beam::ForceFeedbackStep(int steps)
{
    m_force_sensors.out_body_forces = m_force_sensors.accu_body_forces / steps;
    if (ar_num_hydros != 0) // Vehicle has hydros?
    {
        m_force_sensors.out_hydros_forces = (m_force_sensors.accu_hydros_forces / steps) / ar_num_hydros;    
    }
}

void Beam::updateAngelScriptEvents(float dt)
{
#ifdef USE_ANGELSCRIPT

    // TODO: restore events SE_TRUCK_LOCKED and SE_TRUCK_UNLOCKED
    if (m_water_contact && !m_water_contact_old)
    {
        m_water_contact_old = m_water_contact;
        ScriptEngine::getSingleton().triggerEvent(SE_TRUCK_TOUCHED_WATER, ar_instance_id);
    }
#endif // USE_ANGELSCRIPT
}

void Beam::handleResetRequests(float dt)
{
    if (m_reset_request)
        SyncReset();
}

void Beam::sendStreamSetup()
{
    RoRnet::TruckStreamRegister reg;
    memset(&reg, 0, sizeof(RoRnet::TruckStreamRegister));
    reg.status = 0;
    reg.type = 0;
    reg.bufferSize = m_net_buffer_size;
    strncpy(reg.name, ar_filename.c_str(), 128);
    if (!m_truck_config.empty())
    {
        // insert section config
        for (int i = 0; i < std::min<int>((int)m_truck_config.size(), 10); i++)
            strncpy(reg.truckconfig[i], m_truck_config[i].c_str(), 60);
    }

#ifdef USE_SOCKETW
    RoR::Networking::AddLocalStream((RoRnet::StreamRegister *)&reg, sizeof(RoRnet::TruckStreamRegister));
#endif // USE_SOCKETW

    ar_net_source_id = reg.origin_sourceid;
    ar_net_stream_id = reg.origin_streamid;
}

void Beam::sendStreamData()
{
    using namespace RoRnet;

    BES_GFX_START(BES_GFX_sendStreamData);
#ifdef USE_SOCKETW
    ar_net_last_update_time = ar_net_timer.getMilliseconds();

    //look if the packet is too big first
    int final_packet_size = sizeof(RoRnet::TruckState) + sizeof(float) * 3 + m_net_first_wheel_node * sizeof(float) * 3 + free_wheel * sizeof(float);
    if (final_packet_size > 8192)
    {
        ErrorUtils::ShowError(_L("Truck is too big to be send over the net."), _L("Network error!"));
        exit(126);
    }

    char send_buffer[8192] = {0};

    unsigned int packet_len = 0;

    // RoRnet::TruckState is at the beginning of the buffer
    {
        RoRnet::TruckState* send_oob = (RoRnet::TruckState *)send_buffer;
        packet_len += sizeof(RoRnet::TruckState);

        send_oob->flagmask = 0;

        send_oob->time = ar_net_timer.getMilliseconds();
        if (ar_engine)
        {
            send_oob->engine_speed = ar_engine->getRPM();
            send_oob->engine_force = ar_engine->getAcc();
            send_oob->engine_clutch = ar_engine->getClutch();
            send_oob->engine_gear = ar_engine->getGear();

            if (ar_engine->hasContact())
                send_oob->flagmask += NETMASK_ENGINE_CONT;
            if (ar_engine->isRunning())
                send_oob->flagmask += NETMASK_ENGINE_RUN;

            switch (ar_engine->getAutoMode())
            {
            case RoR::SimGearboxMode::AUTO: send_oob->flagmask += NETMASK_ENGINE_MODE_AUTOMATIC;
                break;
            case RoR::SimGearboxMode::SEMI_AUTO: send_oob->flagmask += NETMASK_ENGINE_MODE_SEMIAUTO;
                break;
            case RoR::SimGearboxMode::MANUAL: send_oob->flagmask += NETMASK_ENGINE_MODE_MANUAL;
                break;
            case RoR::SimGearboxMode::MANUAL_STICK: send_oob->flagmask += NETMASK_ENGINE_MODE_MANUAL_STICK;
                break;
            case RoR::SimGearboxMode::MANUAL_RANGES: send_oob->flagmask += NETMASK_ENGINE_MODE_MANUAL_RANGES;
                break;
            }
        }
        if (ar_num_aeroengines > 0)
        {
            float rpm = ar_aeroengines[0]->getRPM();
            send_oob->engine_speed = rpm;
        }

        send_oob->hydrodirstate = ar_hydro_dir_state;
        send_oob->brake = ar_brake;
        send_oob->wheelspeed = ar_wheel_speed;

        blinktype b = getBlinkType();
        if (b == BLINK_LEFT)
            send_oob->flagmask += NETMASK_BLINK_LEFT;
        else if (b == BLINK_RIGHT)
            send_oob->flagmask += NETMASK_BLINK_RIGHT;
        else if (b == BLINK_WARN)
            send_oob->flagmask += NETMASK_BLINK_WARN;

        if (ar_lights)
            send_oob->flagmask += NETMASK_LIGHTS;
        if (getCustomLightVisible(0))
            send_oob->flagmask += NETMASK_CLIGHT1;
        if (getCustomLightVisible(1))
            send_oob->flagmask += NETMASK_CLIGHT2;
        if (getCustomLightVisible(2))
            send_oob->flagmask += NETMASK_CLIGHT3;
        if (getCustomLightVisible(3))
            send_oob->flagmask += NETMASK_CLIGHT4;

        if (getBrakeLightVisible())
            send_oob->flagmask += NETMASK_BRAKES;
        if (getReverseLightVisible())
            send_oob->flagmask += NETMASK_REVERSE;
        if (getBeaconMode())
            send_oob->flagmask += NETMASK_BEACONS;
        if (getCustomParticleMode())
            send_oob->flagmask += NETMASK_PARTICLE;

        if (ar_parking_brake)
            send_oob->flagmask += NETMASK_PBRAKE;
        if (m_tractioncontrol)
            send_oob->flagmask += NETMASK_TC_ACTIVE;
        if (m_antilockbrake)
            send_oob->flagmask += NETMASK_ALB_ACTIVE;

        if (SOUND_GET_STATE(ar_instance_id, SS_TRIG_HORN))
            send_oob->flagmask += NETMASK_HORN;
    }

    // then process the contents
    {
        char* ptr = send_buffer + sizeof(RoRnet::TruckState);
        float* send_nodes = (float *)ptr;
        packet_len += m_net_buffer_size;

        // copy data into the buffer
        int i;

        // reference node first
        Vector3& refpos = ar_nodes[0].AbsPosition;
        send_nodes[0] = refpos.x;
        send_nodes[1] = refpos.y;
        send_nodes[2] = refpos.z;

        ptr += sizeof(float) * 3;// plus 3 floats from above

        // then copy the other nodes into a compressed short format
        short* sbuf = (short*)ptr;
        for (i = 1; i < m_net_first_wheel_node; i++)
        {
            Vector3 relpos = ar_nodes[i].AbsPosition - refpos;
            sbuf[(i - 1) * 3 + 0] = (short int)(relpos.x * 300.0f);
            sbuf[(i - 1) * 3 + 1] = (short int)(relpos.y * 300.0f);
            sbuf[(i - 1) * 3 + 2] = (short int)(relpos.z * 300.0f);

            ptr += sizeof(short int) * 3; // increase pointer
        }

        // then to the wheels
        float* wfbuf = (float*)ptr;
        for (i = 0; i < free_wheel; i++)
        {
            wfbuf[i] = wheels[i].wh_net_rp;
        }
    }

    RoR::Networking::AddPacket(ar_net_stream_id, MSG2_STREAM_DATA, packet_len, send_buffer);
#endif //SOCKETW
    BES_GFX_STOP(BES_GFX_sendStreamData);
}

void Beam::receiveStreamData(unsigned int type, int source, unsigned int streamid, char* buffer, unsigned int len)
{
    if (ar_sim_state != SimState::NETWORKED_OK)
        return;

    BES_GFX_START(BES_GFX_receiveStreamData);
    if (type == RoRnet::MSG2_STREAM_DATA && source == ar_net_source_id && streamid == ar_net_stream_id)
    {
        pushNetwork(buffer, len);
    }
    BES_GFX_STOP(BES_GFX_receiveStreamData);
}

void Beam::calcAnimators(const int flag_state, float& cstate, int& div, Real timer, const float lower_limit, const float upper_limit, const float option3)
{
    BES_GFX_START(BES_GFX_calcAnimators);
    Real dt = timer;

    //boat rudder
    if (flag_state & ANIM_FLAG_BRUDDER)
    {
        int spi;
        float ctmp = 0.0f;
        for (spi = 0; spi < ar_num_screwprops; spi++)
            if (ar_screwprops[spi])
                ctmp += ar_screwprops[spi]->getRudder();

        if (spi > 0)
            ctmp = ctmp / spi;
        cstate = ctmp;
        div++;
    }

    //boat throttle
    if (flag_state & ANIM_FLAG_BTHROTTLE)
    {
        int spi;
        float ctmp = 0.0f;
        for (spi = 0; spi < ar_num_screwprops; spi++)
            if (ar_screwprops[spi])
                ctmp += ar_screwprops[spi]->getThrottle();

        if (spi > 0)
            ctmp = ctmp / spi;
        cstate = ctmp;
        div++;
    }

    //differential lock status
    if (flag_state & ANIM_FLAG_DIFFLOCK)
    {
        if (m_num_axles)
        {
            if (getAxleLockName() == "Open")
                cstate = 0.0f;
            if (getAxleLockName() == "Split")
                cstate = 0.5f;
            if (getAxleLockName() == "Locked")
                cstate = 1.0f;
        }
        else // no axles/diffs avail, mode is split by default
            cstate = 0.5f;

        div++;
    }

    //heading
    if (flag_state & ANIM_FLAG_HEADING)
    {
        float heading = getHeadingDirectionAngle();
        // rad2deg limitedrange  -1 to +1
        cstate = (heading * 57.29578f) / 360.0f;
        div++;
    }

    //torque
    if (ar_engine && flag_state & ANIM_FLAG_TORQUE)
    {
        float torque = ar_engine->getCrankFactor();
        if (torque <= 0.0f)
            torque = 0.0f;
        if (torque >= ar_anim_previous_crank)
            cstate -= torque / 10.0f;
        else
            cstate = 0.0f;

        if (cstate <= -1.0f)
            cstate = -1.0f;
        ar_anim_previous_crank = torque;
        div++;
    }

    //shifterseq, to amimate sequentiell shifting
    if (ar_engine && (flag_state & ANIM_FLAG_SHIFTER) && option3 == 3.0f)
    {
        // opt1 &opt2 = 0   this is a shifter
        if (!lower_limit && !upper_limit)
        {
            int shifter = ar_engine->getGear();
            if (shifter > m_previous_gear)
            {
                cstate = 1.0f;
                ar_anim_shift_timer = 0.2f;
            }
            if (shifter < m_previous_gear)
            {
                cstate = -1.0f;
                ar_anim_shift_timer = -0.2f;
            }
            m_previous_gear = shifter;

            if (ar_anim_shift_timer > 0.0f)
            {
                cstate = 1.0f;
                ar_anim_shift_timer -= dt;
                if (ar_anim_shift_timer < 0.0f)
                    ar_anim_shift_timer = 0.0f;
            }
            if (ar_anim_shift_timer < 0.0f)
            {
                cstate = -1.0f;
                ar_anim_shift_timer += dt;
                if (ar_anim_shift_timer > 0.0f)
                    ar_anim_shift_timer = 0.0f;
            }
        }
        else
        {
            // check if lower_limit is a valid to get commandvalue, then get commandvalue
            if (lower_limit >= 1.0f && lower_limit <= 48.0)
                if (commandkey[int(lower_limit)].commandValue > 0)
                    cstate += 1.0f;
            // check if upper_limit is a valid to get commandvalue, then get commandvalue
            if (upper_limit >= 1.0f && upper_limit <= 48.0)
                if (commandkey[int(upper_limit)].commandValue > 0)
                    cstate -= 1.0f;
        }

        div++;
    }

    //shifterman1, left/right
    if (ar_engine && (flag_state & ANIM_FLAG_SHIFTER) && option3 == 1.0f)
    {
        int shifter = ar_engine->getGear();
        if (!shifter)
        {
            cstate = -0.5f;
        }
        else if (shifter < 0)
        {
            cstate = 1.0f;
        }
        else
        {
            cstate -= int((shifter - 1.0) / 2.0);
        }
        div++;
    }

    //shifterman2, up/down
    if (ar_engine && (flag_state & ANIM_FLAG_SHIFTER) && option3 == 2.0f)
    {
        int shifter = ar_engine->getGear();
        cstate = 0.5f;
        if (shifter < 0)
        {
            cstate = 1.0f;
        }
        if (shifter > 0)
        {
            cstate = shifter % 2;
        }
        div++;
    }

    //shifterlinear, to amimate cockpit gearselect gauge and autotransmission stick
    if (ar_engine && (flag_state & ANIM_FLAG_SHIFTER) && option3 == 4.0f)
    {
        int shifter = ar_engine->getGear();
        int numgears = ar_engine->getNumGears();
        cstate -= (shifter + 2.0) / (numgears + 2.0);
        div++;
    }

    //parking brake
    if (flag_state & ANIM_FLAG_PBRAKE)
    {
        float pbrake = ar_parking_brake;
        cstate -= pbrake;
        div++;
    }

    //speedo ( scales with speedomax )
    if (flag_state & ANIM_FLAG_SPEEDO)
    {
        float speedo = ar_wheel_speed / ar_speedo_max_kph;
        cstate -= speedo * 3.0f;
        div++;
    }

    //engine tacho ( scales with maxrpm, default is 3500 )
    if (ar_engine && flag_state & ANIM_FLAG_TACHO)
    {
        float tacho = ar_engine->getRPM() / ar_engine->getMaxRPM();
        cstate -= tacho;
        div++;
    }

    //turbo
    if (ar_engine && flag_state & ANIM_FLAG_TURBO)
    {
        float turbo = ar_engine->getTurboPSI() * 3.34;
        cstate -= turbo / 67.0f;
        div++;
    }

    //brake
    if (flag_state & ANIM_FLAG_BRAKE)
    {
        float brakes = ar_brake / ar_brake_force;
        cstate -= brakes;
        div++;
    }

    //accelerator
    if (ar_engine && flag_state & ANIM_FLAG_ACCEL)
    {
        float accel = ar_engine->getAcc();
        cstate -= accel + 0.06f;
        //( small correction, get acc is nver smaller then 0.06.
        div++;
    }

    //clutch
    if (ar_engine && flag_state & ANIM_FLAG_CLUTCH)
    {
        float clutch = ar_engine->getClutch();
        cstate -= fabs(1.0f - clutch);
        div++;
    }

    //aeroengines rpm + throttle + torque ( turboprop ) + pitch ( turboprop ) + status +  fire
    int ftp = ar_num_aeroengines;

    if (ftp > option3 - 1.0f)
    {
        int aenum = int(option3 - 1.0f);
        if (flag_state & ANIM_FLAG_RPM)
        {
            float angle;
            float pcent = ar_aeroengines[aenum]->getRPMpc();
            if (pcent < 60.0)
                angle = -5.0 + pcent * 1.9167;
            else if (pcent < 110.0)
                angle = 110.0 + (pcent - 60.0) * 4.075;
            else
                angle = 314.0;
            cstate -= angle / 314.0f;
            div++;
        }
        if (flag_state & ANIM_FLAG_THROTTLE)
        {
            float throttle = ar_aeroengines[aenum]->getThrottle();
            cstate -= throttle;
            div++;
        }

        if (flag_state & ANIM_FLAG_AETORQUE)
            if (ar_aeroengines[aenum]->getType() == AeroEngine::AEROENGINE_TYPE_TURBOPROP)
            {
                Turboprop* tp = (Turboprop*)ar_aeroengines[aenum];
                cstate = (100.0 * tp->indicated_torque / tp->max_torque) / 120.0f;
                div++;
            }

        if (flag_state & ANIM_FLAG_AEPITCH)
            if (ar_aeroengines[aenum]->getType() == AeroEngine::AEROENGINE_TYPE_TURBOPROP)
            {
                Turboprop* tp = (Turboprop*)ar_aeroengines[aenum];
                cstate = tp->pitch / 120.0f;
                div++;
            }

        if (flag_state & ANIM_FLAG_AESTATUS)
        {
            if (!ar_aeroengines[aenum]->getIgnition())
                cstate = 0.0f;
            else
                cstate = 0.5f;
            if (ar_aeroengines[aenum]->isFailed())
                cstate = 1.0f;
            div++;
        }
    }

    //airspeed indicator
    if (flag_state & ANIM_FLAG_AIRSPEED)
    {
        // TODO Unused Varaible
        //float angle=0.0;
        float ground_speed_kt = ar_nodes[0].Velocity.length() * 1.9438;
        float altitude = ar_nodes[0].AbsPosition.y;

        // TODO Unused Varaible
        //float sea_level_temperature=273.15+15.0; //in Kelvin
        float sea_level_pressure = 101325; //in Pa

        // TODO Unused Varaible
        //float airtemperature=sea_level_temperature-altitude*0.0065; //in Kelvin
        float airpressure = sea_level_pressure * pow(1.0 - 0.0065 * altitude / 288.15, 5.24947); //in Pa
        float airdensity = airpressure * 0.0000120896;//1.225 at sea level
        float kt = ground_speed_kt * sqrt(airdensity / 1.225);
        cstate -= kt / 100.0f;
        div++;
    }

    //vvi indicator
    if (flag_state & ANIM_FLAG_VVI)
    {
        float vvi = ar_nodes[0].Velocity.y * 196.85;
        // limit vvi scale to +/- 6m/s
        cstate -= vvi / 6000.0f;
        if (cstate >= 1.0f)
            cstate = 1.0f;
        if (cstate <= -1.0f)
            cstate = -1.0f;
        div++;
    }

    //altimeter
    if (flag_state & ANIM_FLAG_ALTIMETER)
    {
        //altimeter indicator 1k oscillating
        if (option3 == 3.0f)
        {
            float altimeter = (ar_nodes[0].AbsPosition.y * 1.1811) / 360.0f;
            int alti_int = int(altimeter);
            float alti_mod = (altimeter - alti_int);
            cstate -= alti_mod;
        }

        //altimeter indicator 10k oscillating
        if (option3 == 2.0f)
        {
            float alti = ar_nodes[0].AbsPosition.y * 1.1811 / 3600.0f;
            int alti_int = int(alti);
            float alti_mod = (alti - alti_int);
            cstate -= alti_mod;
            if (cstate <= -1.0f)
                cstate = -1.0f;
        }

        //altimeter indicator 100k limited
        if (option3 == 1.0f)
        {
            float alti = ar_nodes[0].AbsPosition.y * 1.1811 / 36000.0f;
            cstate -= alti;
            if (cstate <= -1.0f)
                cstate = -1.0f;
        }
        div++;
    }

    //AOA
    if (flag_state & ANIM_FLAG_AOA)
    {
        float aoa = 0;
        if (ar_num_wings > 4)
            aoa = (ar_wings[4].fa->aoa) / 25.0f;
        if ((ar_nodes[0].Velocity.length() * 1.9438) < 10.0f)
            aoa = 0;
        cstate -= aoa;
        if (cstate <= -1.0f)
            cstate = -1.0f;
        if (cstate >= 1.0f)
            cstate = 1.0f;
        div++;
    }

    Vector3 cam_pos = ar_nodes[0].RelPosition;
    Vector3 cam_roll = ar_nodes[0].RelPosition;
    Vector3 cam_dir = ar_nodes[0].RelPosition;

    if (this->IsNodeIdValid(ar_camera_node_pos[0])) // TODO: why check this on each update when it cannot change after spawn?
    {
        cam_pos = ar_nodes[ar_camera_node_pos[0]].RelPosition;
        cam_roll = ar_nodes[ar_camera_node_roll[0]].RelPosition;
        cam_dir = ar_nodes[ar_camera_node_dir[0]].RelPosition;
    }

    // roll
    if (flag_state & ANIM_FLAG_ROLL)
    {
        Vector3 rollv = (cam_pos - cam_roll).normalisedCopy();
        Vector3 dirv = (cam_pos - cam_dir).normalisedCopy();
        Vector3 upv = dirv.crossProduct(-rollv);
        float rollangle = asin(rollv.dotProduct(Vector3::UNIT_Y));
        // rad to deg
        rollangle = Math::RadiansToDegrees(rollangle);
        // flip to other side when upside down
        if (upv.y < 0)
            rollangle = 180.0f - rollangle;
        cstate = rollangle / 180.0f;
        // data output is -0.5 to 1.5, normalize to -1 to +1 without changing the zero position.
        // this is vital for the animator beams and does not effect the animated props
        if (cstate >= 1.0f)
            cstate = cstate - 2.0f;
        div++;
    }

    // pitch
    if (flag_state & ANIM_FLAG_PITCH)
    {
        Vector3 dirv = (cam_pos - cam_dir).normalisedCopy();
        float pitchangle = asin(dirv.dotProduct(Vector3::UNIT_Y));
        // radian to degrees with a max cstate of +/- 1.0
        cstate = (Math::RadiansToDegrees(pitchangle) / 90.0f);
        div++;
    }

    // airbrake
    if (flag_state & ANIM_FLAG_AIRBRAKE)
    {
        float airbrake = ar_airbrake_intensity;
        // cstate limited to -1.0f
        cstate -= airbrake / 5.0f;
        div++;
    }

    //flaps
    if (flag_state & ANIM_FLAG_FLAP)
    {
        float flaps = flapangles[ar_aerial_flap];
        // cstate limited to -1.0f
        cstate = flaps;
        div++;
    }

    BES_GFX_STOP(BES_GFX_calcAnimators);
}

void Beam::calcShocks2(int beam_i, Real difftoBeamL, Real& k, Real& d, Real dt, int update)
{
    if (!ar_beams[beam_i].shock)
        return;

    int i = beam_i;
    float beamsLep = ar_beams[i].L * 0.8f;
    float longboundprelimit = ar_beams[i].longbound * beamsLep;
    float shortboundprelimit = -ar_beams[i].shortbound * beamsLep;
    // this is a shock2
    float logafactor;
    //shock extending since last cycle
    if (ar_beams[i].shock->lastpos < difftoBeamL)
    {
        //get outbound values
        k = ar_beams[i].shock->springout;
        d = ar_beams[i].shock->dampout;
        // add progression
        if (ar_beams[i].longbound != 0.0f)
        {
            logafactor = difftoBeamL / (ar_beams[i].longbound * ar_beams[i].L);
            logafactor = logafactor * logafactor;
        }
        else
        {
            logafactor = 1.0f;
        }
        if (logafactor > 1.0f)
            logafactor = 1.0f;
        k = k + (ar_beams[i].shock->sprogout * k * logafactor);
        d = d + (ar_beams[i].shock->dprogout * d * logafactor);
    }
    else
    {
        //shock compresssing since last cycle
        //get inbound values
        k = ar_beams[i].shock->springin;
        d = ar_beams[i].shock->dampin;
        // add progression
        if (ar_beams[i].shortbound != 0.0f)
        {
            logafactor = difftoBeamL / (ar_beams[i].shortbound * ar_beams[i].L);
            logafactor = logafactor * logafactor;
        }
        else
        {
            logafactor = 1.0f;
        }
        if (logafactor > 1.0f)
            logafactor = 1.0f;
        k = k + (ar_beams[i].shock->sprogin * k * logafactor);
        d = d + (ar_beams[i].shock->dprogin * d * logafactor);
    }
    if (ar_beams[i].shock->flags & SHOCK_FLAG_SOFTBUMP)
    {
        // soft bump shocks
        if (difftoBeamL > longboundprelimit)
        {
            //reset to longbound progressive values (oscillating beam workaround)
            k = ar_beams[i].shock->springout;
            d = ar_beams[i].shock->dampout;
            // add progression
            if (ar_beams[i].longbound != 0.0f)
            {
                logafactor = difftoBeamL / (ar_beams[i].longbound * ar_beams[i].L);
                logafactor = logafactor * logafactor;
            }
            else
            {
                logafactor = 1.0f;
            }
            if (logafactor > 1.0f)
                logafactor = 1.0f;
            k = k + (ar_beams[i].shock->sprogout * k * logafactor);
            d = d + (ar_beams[i].shock->dprogout * d * logafactor);
            //add shortbump progression
            if (ar_beams[i].longbound != 0.0f)
            {
                logafactor = ((difftoBeamL - longboundprelimit) * 5.0f) / (ar_beams[i].longbound * ar_beams[i].L);
                logafactor = logafactor * logafactor;
            }
            else
            {
                logafactor = 1.0f;
            }
            if (logafactor > 1.0f)
                logafactor = 1.0f;
            k = k + (k + 100.0f) * ar_beams[i].shock->sprogout * logafactor;
            d = d + (d + 100.0f) * ar_beams[i].shock->dprogout * logafactor;
            if (ar_beams[i].shock->lastpos > difftoBeamL)
            // rebound mode..get new values
            {
                k = ar_beams[i].shock->springin;
                d = ar_beams[i].shock->dampin;
            }
        }
        else if (difftoBeamL < shortboundprelimit)
        {
            //reset to shortbound progressive values (oscillating beam workaround)
            k = ar_beams[i].shock->springin;
            d = ar_beams[i].shock->dampin;
            if (ar_beams[i].shortbound != 0.0f)
            {
                logafactor = difftoBeamL / (ar_beams[i].shortbound * ar_beams[i].L);
                logafactor = logafactor * logafactor;
            }
            else
            {
                logafactor = 1.0f;
            }
            if (logafactor > 1.0f)
                logafactor = 1.0f;
            k = k + (ar_beams[i].shock->sprogin * k * logafactor);
            d = d + (ar_beams[i].shock->dprogin * d * logafactor);
            //add shortbump progression
            if (ar_beams[i].shortbound != 0.0f)
            {
                logafactor = ((difftoBeamL - shortboundprelimit) * 5.0f) / (ar_beams[i].shortbound * ar_beams[i].L);
                logafactor = logafactor * logafactor;
            }
            else
            {
                logafactor = 1.0f;
            }
            if (logafactor > 1.0f)
                logafactor = 1.0f;
            k = k + (k + 100.0f) * ar_beams[i].shock->sprogout * logafactor;
            d = d + (d + 100.0f) * ar_beams[i].shock->dprogout * logafactor;
            if (ar_beams[i].shock->lastpos < difftoBeamL)
            // rebound mode..get new values
            {
                k = ar_beams[i].shock->springout;
                d = ar_beams[i].shock->dampout;
            }
        }
        if (difftoBeamL > ar_beams[i].longbound * ar_beams[i].L || difftoBeamL < -ar_beams[i].shortbound * ar_beams[i].L)
        {
            // block reached...hard bump in soft mode with 4x default damping
            if (k < ar_beams[i].shock->sbd_spring)
                k = ar_beams[i].shock->sbd_spring;
            if (d < ar_beams[i].shock->sbd_damp)
                d = ar_beams[i].shock->sbd_damp;
        }
    }

    if (ar_beams[i].shock->flags & SHOCK_FLAG_NORMAL)
    {
        if (difftoBeamL > ar_beams[i].longbound * ar_beams[i].L || difftoBeamL < -ar_beams[i].shortbound * ar_beams[i].L)
        {
            if (ar_beams[i].shock && !(ar_beams[i].shock->flags & SHOCK_FLAG_ISTRIGGER)) // this is NOT a trigger beam
            {
                // hard (normal) shock bump
                k = ar_beams[i].shock->sbd_spring;
                d = ar_beams[i].shock->sbd_damp;
            }
        }

        if (ar_beams[i].shock && (ar_beams[i].shock->flags & SHOCK_FLAG_ISTRIGGER) && ar_beams[i].shock->trigger_enabled) // this is a trigger and its enabled
        {
            if (difftoBeamL > ar_beams[i].longbound * ar_beams[i].L || difftoBeamL < -ar_beams[i].shortbound * ar_beams[i].L) // that has hit boundary
            {
                ar_beams[i].shock->trigger_switch_state -= dt;
                if (ar_beams[i].shock->trigger_switch_state <= 0.0f) // emergency release for dead-switched trigger
                    ar_beams[i].shock->trigger_switch_state = 0.0f;
                if (ar_beams[i].shock->flags & SHOCK_FLAG_TRG_BLOCKER) // this is an enabled blocker and past boundary
                {
                    for (int scount = i + 1; scount <= i + ar_beams[i].shock->trigger_cmdshort; scount++) // (cycle blockerbeamID +1) to (blockerbeamID + beams to lock)
                    {
                        if (ar_beams[scount].shock && (ar_beams[scount].shock->flags & SHOCK_FLAG_ISTRIGGER)) // don't mess anything up if the user set the number too big
                        {
                            if (m_trigger_debug_enabled && !ar_beams[scount].shock->trigger_enabled && ar_beams[i].shock->last_debug_state != 1)
                            {
                                LOG(" Trigger disabled. Blocker BeamID " + TOSTRING(i) + " enabled trigger " + TOSTRING(scount));
                                ar_beams[i].shock->last_debug_state = 1;
                            }
                            ar_beams[scount].shock->trigger_enabled = false; // disable the trigger
                        }
                    }
                }
                else if (ar_beams[i].shock->flags & SHOCK_FLAG_TRG_BLOCKER_A) // this is an enabled inverted blocker and inside boundary
                {
                    for (int scount = i + 1; scount <= i + ar_beams[i].shock->trigger_cmdlong; scount++) // (cycle blockerbeamID + 1) to (blockerbeamID + beams to release)
                    {
                        if (ar_beams[scount].shock && (ar_beams[scount].shock->flags & SHOCK_FLAG_ISTRIGGER)) // don't mess anything up if the user set the number too big
                        {
                            if (m_trigger_debug_enabled && ar_beams[scount].shock->trigger_enabled && ar_beams[i].shock->last_debug_state != 9)
                            {
                                LOG(" Trigger enabled. Inverted Blocker BeamID " + TOSTRING(i) + " disabled trigger " + TOSTRING(scount));
                                ar_beams[i].shock->last_debug_state = 9;
                            }
                            ar_beams[scount].shock->trigger_enabled = true; // enable the triggers
                        }
                    }
                }
                else if (ar_beams[i].shock->flags & SHOCK_FLAG_TRG_CMD_BLOCKER) // this an enabled cmd-key-blocker and past a boundary
                {
                    commandkey[ar_beams[i].shock->trigger_cmdshort].trigger_cmdkeyblock_state = false; // Release the cmdKey
                    if (m_trigger_debug_enabled && ar_beams[i].shock->last_debug_state != 2)
                    {
                        LOG(" F-key trigger block released. Blocker BeamID " + TOSTRING(i) + " Released F" + TOSTRING(ar_beams[i].shock->trigger_cmdshort));
                        ar_beams[i].shock->last_debug_state = 2;
                    }
                }
                else if (ar_beams[i].shock->flags & SHOCK_FLAG_TRG_CMD_SWITCH) // this is an enabled cmdkey switch and past a boundary
                {
                    if (!ar_beams[i].shock->trigger_switch_state)// this switch is triggered first time in this boundary
                    {
                        for (int scount = 0; scount < ar_num_shocks; scount++)
                        {
                            int short1 = ar_beams[ar_shocks[scount].beamid].shock->trigger_cmdshort; // cmdshort of checked trigger beam
                            int short2 = ar_beams[i].shock->trigger_cmdshort; // cmdshort of switch beam
                            int long1 = ar_beams[ar_shocks[scount].beamid].shock->trigger_cmdlong; // cmdlong of checked trigger beam
                            int long2 = ar_beams[i].shock->trigger_cmdlong; // cmdlong of switch beam
                            int tmpi = ar_beams[ar_shocks[scount].beamid].shock->beamid; // beamID global of checked trigger beam
                            if (((short1 == short2 && long1 == long2) || (short1 == long2 && long1 == short2)) && i != tmpi) // found both command triggers then swap if its not the switching trigger
                            {
                                int tmpcmdkey = ar_beams[ar_shocks[scount].beamid].shock->trigger_cmdlong;
                                ar_beams[ar_shocks[scount].beamid].shock->trigger_cmdlong = ar_beams[ar_shocks[scount].beamid].shock->trigger_cmdshort;
                                ar_beams[ar_shocks[scount].beamid].shock->trigger_cmdshort = tmpcmdkey;
                                ar_beams[i].shock->trigger_switch_state = ar_beams[i].shock->trigger_boundary_t; //prevent trigger switching again before leaving boundaries or timeout
                                if (m_trigger_debug_enabled && ar_beams[i].shock->last_debug_state != 3)
                                {
                                    LOG(" Trigger F-key commands switched. Switch BeamID " + TOSTRING(i)+ " switched commands of Trigger BeamID " + TOSTRING(ar_beams[ar_shocks[scount].beamid].shock->beamid) + " to cmdShort: F" + TOSTRING(ar_beams[ar_shocks[scount].beamid].shock->trigger_cmdshort) + ", cmdlong: F" + TOSTRING(ar_beams[ar_shocks[scount].beamid].shock->trigger_cmdlong));
                                    ar_beams[i].shock->last_debug_state = 3;
                                }
                            }
                        }
                    }
                }
                else
                { // just a trigger, check high/low boundary and set action
                    if (difftoBeamL > ar_beams[i].longbound * ar_beams[i].L) // trigger past longbound
                    {
                        if (ar_beams[i].shock->flags & SHOCK_FLAG_TRG_HOOK_UNLOCK)
                        {
                            if (update)
                            {
                                //autolock hooktoggle unlock
                                hookToggle(ar_beams[i].shock->trigger_cmdlong, HOOK_UNLOCK, -1);
                            }
                        }
                        else if (ar_beams[i].shock->flags & SHOCK_FLAG_TRG_HOOK_LOCK)
                        {
                            if (update)
                            {
                                //autolock hooktoggle lock
                                hookToggle(ar_beams[i].shock->trigger_cmdlong, HOOK_LOCK, -1);
                            }
                        }
                        else if (ar_beams[i].shock->flags & SHOCK_FLAG_TRG_ENGINE)
                        {
                            engineTriggerHelper(ar_beams[i].shock->trigger_cmdshort, ar_beams[i].shock->trigger_cmdlong, 1.0f);
                        }
                        else
                        {
                            //just a trigger
                            if (!commandkey[ar_beams[i].shock->trigger_cmdlong].trigger_cmdkeyblock_state) // related cmdkey is not blocked
                            {
                                if (ar_beams[i].shock->flags & SHOCK_FLAG_TRG_CONTINUOUS)
                                    commandkey[ar_beams[i].shock->trigger_cmdshort].triggerInputValue = 1; // continuous trigger only operates on trigger_cmdshort
                                else
                                    commandkey[ar_beams[i].shock->trigger_cmdlong].triggerInputValue = 1;
                                if (m_trigger_debug_enabled && ar_beams[i].shock->last_debug_state != 4)
                                {
                                    LOG(" Trigger Longbound activated. Trigger BeamID " + TOSTRING(i) + " Triggered F" + TOSTRING(ar_beams[i].shock->trigger_cmdlong));
                                    ar_beams[i].shock->last_debug_state = 4;
                                }
                            }
                        }
                    }
                    else // trigger past short bound
                    {
                        if (ar_beams[i].shock->flags & SHOCK_FLAG_TRG_HOOK_UNLOCK)
                        {
                            if (update)
                            {
                                //autolock hooktoggle unlock
                                hookToggle(ar_beams[i].shock->trigger_cmdshort, HOOK_UNLOCK, -1);
                            }
                        }
                        else if (ar_beams[i].shock->flags & SHOCK_FLAG_TRG_HOOK_LOCK)
                        {
                            if (update)
                            {
                                //autolock hooktoggle lock
                                hookToggle(ar_beams[i].shock->trigger_cmdshort, HOOK_LOCK, -1);
                            }
                        }
                        else if (ar_beams[i].shock->flags & SHOCK_FLAG_TRG_ENGINE)
                        {
                            bool triggerValue = !(ar_beams[i].shock->flags & SHOCK_FLAG_TRG_CONTINUOUS); // 0 if trigger is continuous, 1 otherwise

                            engineTriggerHelper(ar_beams[i].shock->trigger_cmdshort, ar_beams[i].shock->trigger_cmdlong, triggerValue);
                        }
                        else
                        {
                            //just a trigger
                            if (!commandkey[ar_beams[i].shock->trigger_cmdshort].trigger_cmdkeyblock_state) // related cmdkey is not blocked
                            {
                                if (ar_beams[i].shock->flags & SHOCK_FLAG_TRG_CONTINUOUS)
                                    commandkey[ar_beams[i].shock->trigger_cmdshort].triggerInputValue = 0; // continuous trigger only operates on trigger_cmdshort
                                else
                                    commandkey[ar_beams[i].shock->trigger_cmdshort].triggerInputValue = 1;

                                if (m_trigger_debug_enabled && ar_beams[i].shock->last_debug_state != 5)
                                {
                                    LOG(" Trigger Shortbound activated. Trigger BeamID " + TOSTRING(i) + " Triggered F" + TOSTRING(ar_beams[i].shock->trigger_cmdshort));
                                    ar_beams[i].shock->last_debug_state = 5;
                                }
                            }
                        }
                    }
                }
            }
            else // this is a trigger inside boundaries and its enabled
            {
                if (ar_beams[i].shock->flags & SHOCK_FLAG_TRG_CONTINUOUS) // this is an enabled continuous trigger
                {
                    if (ar_beams[i].longbound - ar_beams[i].shortbound > 0.0f)
                    {
                        float diffPercentage = difftoBeamL / ar_beams[i].L;
                        float triggerValue = (diffPercentage - ar_beams[i].shortbound) / (ar_beams[i].longbound - ar_beams[i].shortbound);

                        triggerValue = std::max(0.0f, triggerValue);
                        triggerValue = std::min(triggerValue, 1.0f);

                        if (ar_beams[i].shock->flags & SHOCK_FLAG_TRG_ENGINE) // this trigger controls an engine
                        {
                            engineTriggerHelper(ar_beams[i].shock->trigger_cmdshort, ar_beams[i].shock->trigger_cmdlong, triggerValue);
                        }
                        else
                        {
                            // normal trigger
                            commandkey[ar_beams[i].shock->trigger_cmdshort].triggerInputValue = triggerValue;
                            commandkey[ar_beams[i].shock->trigger_cmdlong].triggerInputValue = triggerValue;
                        }
                    }
                }
                else if (ar_beams[i].shock->flags & SHOCK_FLAG_TRG_BLOCKER) // this is an enabled blocker and inside boundary
                {
                    for (int scount = i + 1; scount <= i + ar_beams[i].shock->trigger_cmdlong; scount++) // (cycle blockerbeamID + 1) to (blockerbeamID + beams to release)
                    {
                        if (ar_beams[scount].shock && (ar_beams[scount].shock->flags & SHOCK_FLAG_ISTRIGGER)) // don't mess anything up if the user set the number too big
                        {
                            if (m_trigger_debug_enabled && ar_beams[scount].shock->trigger_enabled && ar_beams[i].shock->last_debug_state != 6)
                            {
                                LOG(" Trigger enabled. Blocker BeamID " + TOSTRING(i) + " disabled trigger " + TOSTRING(scount));
                                ar_beams[i].shock->last_debug_state = 6;
                            }
                            ar_beams[scount].shock->trigger_enabled = true; // enable the triggers
                        }
                    }
                }
                else if (ar_beams[i].shock->flags & SHOCK_FLAG_TRG_BLOCKER_A) // this is an enabled reverse blocker and past boundary
                {
                    for (int scount = i + 1; scount <= i + ar_beams[i].shock->trigger_cmdshort; scount++) // (cylce blockerbeamID +1) to (blockerbeamID + beams tob lock)
                    {
                        if (ar_beams[scount].shock && (ar_beams[scount].shock->flags & SHOCK_FLAG_ISTRIGGER)) // dont mess anything up if the user set the number too big
                        {
                            if (m_trigger_debug_enabled && !ar_beams[scount].shock->trigger_enabled && ar_beams[i].shock->last_debug_state != 10)
                            {
                                LOG(" Trigger disabled. Inverted Blocker BeamID " + TOSTRING(i) + " enabled trigger " + TOSTRING(scount));
                                ar_beams[i].shock->last_debug_state = 10;
                            }
                            ar_beams[scount].shock->trigger_enabled = false; // disable the trigger
                        }
                    }
                }
                else if ((ar_beams[i].shock->flags & SHOCK_FLAG_TRG_CMD_SWITCH) && ar_beams[i].shock->trigger_switch_state) // this is a switch that was activated and is back inside boundaries again
                {
                    ar_beams[i].shock->trigger_switch_state = 0.0f; //trigger_switch reset
                    if (m_trigger_debug_enabled && ar_beams[i].shock->last_debug_state != 7)
                    {
                        LOG(" Trigger switch reset. Switch BeamID " + TOSTRING(i));
                        ar_beams[i].shock->last_debug_state = 7;
                    }
                }
                else if ((ar_beams[i].shock->flags & SHOCK_FLAG_TRG_CMD_BLOCKER) && !commandkey[ar_beams[i].shock->trigger_cmdshort].trigger_cmdkeyblock_state) // this cmdkeyblocker is inside boundaries and cmdkeystate is diabled
                {
                    commandkey[ar_beams[i].shock->trigger_cmdshort].trigger_cmdkeyblock_state = true; // activate trigger blocking
                    if (m_trigger_debug_enabled && ar_beams[i].shock->last_debug_state != 8)
                    {
                        LOG(" F-key trigger blocked. Blocker BeamID " + TOSTRING(i) + " Blocked F" + TOSTRING(ar_beams[i].shock->trigger_cmdshort));
                        ar_beams[i].shock->last_debug_state = 8;
                    }
                }
            }
        }
    }
    // save beam position for next simulation cycle
    ar_beams[i].shock->lastpos = difftoBeamL;
}

// call this once per frame in order to update the skidmarks
void Beam::updateSkidmarks()
{
    if (!m_use_skidmarks)
        return;

    BES_START(BES_CORE_Skidmarks);

    for (int i = 0; i < free_wheel; i++)
    {
        // ignore wheels without data
        if (wheels[i].lastContactInner == Vector3::ZERO && wheels[i].lastContactOuter == Vector3::ZERO)
            continue;

        if (m_skid_trails[i])
        {
            m_skid_trails[i]->updatePoint();
            if (wheels[i].isSkiding)
            {
                m_skid_trails[i]->update();
            }
        }
    }

    BES_STOP(BES_CORE_Skidmarks);
}

Quaternion Beam::specialGetRotationTo(const Vector3& src, const Vector3& dest) const
{
    // Based on Stan Melax's article in Game Programming Gems
    Quaternion q;
    // Copy, since cannot modify local
    Vector3 v0 = src;
    Vector3 v1 = dest;
    v0.normalise();
    v1.normalise();

    // NB if the crossProduct approaches zero, we get unstable because ANY axis will do
    // when v0 == -v1
    Real d = v0.dotProduct(v1);
    // If dot == 1, vectors are the same
    if (d >= 1.0f)
    {
        return Quaternion::IDENTITY;
    }
    if (d < (1e-6f - 1.0f))
    {
        // Generate an axis
        Vector3 axis = Vector3::UNIT_X.crossProduct(src);
        if (axis.isZeroLength()) // pick another if colinear
            axis = Vector3::UNIT_Y.crossProduct(src);
        axis.normalise();
        q.FromAngleAxis(Radian(Math::PI), axis);
    }
    else
    {
        Real s = fast_sqrt((1 + d) * 2);
        if (s == 0)
            return Quaternion::IDENTITY;

        Vector3 c = v0.crossProduct(v1);
        Real invs = 1 / s;

        q.x = c.x * invs;
        q.y = c.y * invs;
        q.z = c.z * invs;
        q.w = s * 0.5;
    }
    return q;
}

void Beam::SetPropsCastShadows(bool do_cast_shadows)
{
    if (m_cab_scene_node && m_cab_scene_node->numAttachedObjects() && m_cab_scene_node->getAttachedObject(0))
    {
        ((Entity*)(m_cab_scene_node->getAttachedObject(0)))->setCastShadows(do_cast_shadows);
    }
    int i;
    for (i = 0; i < ar_num_props; i++)
    {
        if (ar_props[i].scene_node && ar_props[i].scene_node->numAttachedObjects())
        {
            ar_props[i].scene_node->getAttachedObject(0)->setCastShadows(do_cast_shadows);
        }
        if (ar_props[i].wheel && ar_props[i].wheel->numAttachedObjects())
        {
            ar_props[i].wheel->getAttachedObject(0)->setCastShadows(do_cast_shadows);
        }
    }
    for (i = 0; i < free_wheel; i++)
    {
        if (vwheels[i].cnode && vwheels[i].cnode->numAttachedObjects())
        {
            vwheels[i].cnode->getAttachedObject(0)->setCastShadows(do_cast_shadows);
        }
    }
    for (i = 0; i < ar_num_beams; i++)
    {
        if (ar_beams[i].mEntity)
        {
            ar_beams[i].mEntity->setCastShadows(do_cast_shadows);
        }
    }
}

void Beam::prepareInside(bool inside)
{
    if (inside)
    {
        gEnv->mainCamera->setNearClipDistance(0.1f);

        // enable transparent seat
        MaterialPtr seatmat = (MaterialPtr)(MaterialManager::getSingleton().getByName("driversseat"));
        seatmat->setDepthWriteEnabled(false);
        seatmat->setSceneBlending(SBT_TRANSPARENT_ALPHA);
    }
    else
    {
        if (ar_dashboard)
        {
            ar_dashboard->setVisible(false);
        }

        gEnv->mainCamera->setNearClipDistance(0.5f);

        // disable transparent seat
        MaterialPtr seatmat = (MaterialPtr)(MaterialManager::getSingleton().getByName("driversseat"));
        seatmat->setDepthWriteEnabled(true);
        seatmat->setSceneBlending(SBT_REPLACE);
    }

    if (m_cab_scene_node != nullptr)
    {
        m_gfx_actor->GetCabTransMaterial()->setReceiveShadows(!inside);
    }

    if (m_gfx_reduce_shadows)
    {
        SetPropsCastShadows(!inside);
    }
}

void Beam::lightsToggle()
{
    // no lights toggling in skeleton mode because of possible bug with emissive texture
    if (ar_skeletonview_is_active)
        return;

    Beam** trucks = App::GetSimController()->GetBeamFactory()->getTrucks();
    int trucksnum = App::GetSimController()->GetBeamFactory()->getTruckCount();

    // export light command
    Beam* current_truck = App::GetSimController()->GetBeamFactory()->getCurrentTruck();
    if (ar_sim_state == Beam::SimState::LOCAL_SIMULATED && this == current_truck && ar_forward_commands)
    {
        for (int i = 0; i < trucksnum; i++)
        {
            if (trucks[i] && trucks[i]->ar_sim_state == Beam::SimState::LOCAL_SIMULATED && this->ar_instance_id != i && trucks[i]->ar_import_commands)
                trucks[i]->lightsToggle();
        }
    }
    ar_lights = !ar_lights;
    if (!ar_lights)
    {
        for (size_t i = 0; i < ar_flares.size(); i++)
        {
            if (ar_flares[i].type == 'f')
            {
                ar_flares[i].snode->setVisible(false);
                if (ar_flares[i].bbs)
                    ar_flares[i].snode->detachAllObjects();
                if (ar_flares[i].light)
                    ar_flares[i].light->setVisible(false);
                ar_flares[i].isVisible = false;
            }
        }
    }
    else
    {
        for (size_t i = 0; i < ar_flares.size(); i++)
        {
            if (ar_flares[i].type == 'f')
            {
                if (ar_flares[i].light)
                    ar_flares[i].light->setVisible(true);
                ar_flares[i].isVisible = true;
                if (ar_flares[i].bbs)
                    ar_flares[i].snode->attachObject(ar_flares[i].bbs);
            }
        }
    }

    m_gfx_actor->SetCabLightsActive(ar_lights != 0);

    TRIGGER_EVENT(SE_TRUCK_LIGHT_TOGGLE, ar_instance_id);
}

void Beam::updateFlares(float dt, bool isCurrent)
{
    if (m_custom_light_toggle_countdown > -1)
        m_custom_light_toggle_countdown -= dt;

    if (m_flares_mode == GfxFlaresMode::NONE) { return; }

    bool enableAll = true;
    if ((m_flares_mode == GfxFlaresMode::CURR_VEHICLE_HEAD_ONLY) && !isCurrent) { enableAll = false; }

    BES_GFX_START(BES_GFX_updateFlares);

    //okay, this is just ugly, we have flares in props!
    //we have to update them here because they run

    Ogre::Vector3 camera_position = gEnv->mainCamera->getPosition();

    if (m_beacon_light_is_active)
    {
        for (int i = 0; i < ar_num_props; i++)
        {
            if (ar_props[i].beacontype == 'b')
            {
                // Get data
                Ogre::SceneNode* beacon_scene_node = ar_props[i].scene_node;
                Quaternion beacon_orientation = beacon_scene_node->getOrientation();
                Ogre::Light* beacon_light = ar_props[i].beacon_light[0];
                float beacon_rotation_rate = ar_props[i].beacon_light_rotation_rate[0];
                float beacon_rotation_angle = ar_props[i].beacon_light_rotation_angle[0]; // Updated at end of block

                // Transform
                beacon_light->setPosition(beacon_scene_node->getPosition() + beacon_orientation * Vector3(0, 0, 0.12));
                beacon_rotation_angle += dt * beacon_rotation_rate;//rotate baby!
                beacon_light->setDirection(beacon_orientation * Vector3(cos(beacon_rotation_angle), sin(beacon_rotation_angle), 0));
                //billboard
                Vector3 vdir = beacon_light->getPosition() - camera_position; // Any reason to query light position instead of scene node position? Where is light position updated, anyway? ~ only_a_ptr, 2015/11
                float vlen = vdir.length();
                if (vlen > 100.0)
                {
                    ar_props[i].beacon_flare_billboard_scene_node[0]->setVisible(false);
                    continue;
                }
                //normalize
                vdir = vdir / vlen;
                ar_props[i].beacon_flare_billboard_scene_node[0]->setPosition(beacon_light->getPosition() - vdir * 0.1);
                float amplitude = beacon_light->getDirection().dotProduct(vdir);
                if (amplitude > 0)
                {
                    ar_props[i].beacon_flare_billboard_scene_node[0]->setVisible(true);
                    ar_props[i].beacon_flares_billboard_system[0]->setDefaultDimensions(amplitude * amplitude * amplitude, amplitude * amplitude * amplitude);
                }
                else
                {
                    ar_props[i].beacon_flare_billboard_scene_node[0]->setVisible(false);
                }
                beacon_light->setVisible(enableAll);

                // Update
                ar_props[i].beacon_light_rotation_angle[0] = beacon_rotation_angle;
                // NOTE: Light position is not updated here!
            }
            else if (ar_props[i].beacontype == 'p')
            {
                for (int k = 0; k < 4; k++)
                {
                    //update light
                    Quaternion orientation = ar_props[i].scene_node->getOrientation();
                    switch (k)
                    {
                    case 0: ar_props[i].beacon_light[k]->setPosition(ar_props[i].scene_node->getPosition() + orientation * Vector3(-0.64, 0, 0.14));
                        break;
                    case 1: ar_props[i].beacon_light[k]->setPosition(ar_props[i].scene_node->getPosition() + orientation * Vector3(-0.32, 0, 0.14));
                        break;
                    case 2: ar_props[i].beacon_light[k]->setPosition(ar_props[i].scene_node->getPosition() + orientation * Vector3(+0.32, 0, 0.14));
                        break;
                    case 3: ar_props[i].beacon_light[k]->setPosition(ar_props[i].scene_node->getPosition() + orientation * Vector3(+0.64, 0, 0.14));
                        break;
                    }
                    ar_props[i].beacon_light_rotation_angle[k] += dt * ar_props[i].beacon_light_rotation_rate[k];//rotate baby!
                    ar_props[i].beacon_light[k]->setDirection(orientation * Vector3(cos(ar_props[i].beacon_light_rotation_angle[k]), sin(ar_props[i].beacon_light_rotation_angle[k]), 0));
                    //billboard
                    Vector3 vdir = ar_props[i].beacon_light[k]->getPosition() - gEnv->mainCamera->getPosition();
                    float vlen = vdir.length();
                    if (vlen > 100.0)
                    {
                        ar_props[i].beacon_flare_billboard_scene_node[k]->setVisible(false);
                        continue;
                    }
                    //normalize
                    vdir = vdir / vlen;
                    ar_props[i].beacon_flare_billboard_scene_node[k]->setPosition(ar_props[i].beacon_light[k]->getPosition() - vdir * 0.2);
                    float amplitude = ar_props[i].beacon_light[k]->getDirection().dotProduct(vdir);
                    if (amplitude > 0)
                    {
                        ar_props[i].beacon_flare_billboard_scene_node[k]->setVisible(true);
                        ar_props[i].beacon_flares_billboard_system[k]->setDefaultDimensions(amplitude * amplitude * amplitude, amplitude * amplitude * amplitude);
                    }
                    else
                    {
                        ar_props[i].beacon_flare_billboard_scene_node[k]->setVisible(false);
                    }
                    ar_props[i].beacon_light[k]->setVisible(enableAll);
                }
            }
            else if (ar_props[i].beacontype == 'r')
            {
                //update light
                Quaternion orientation = ar_props[i].scene_node->getOrientation();
                ar_props[i].beacon_light[0]->setPosition(ar_props[i].scene_node->getPosition() + orientation * Vector3(0, 0, 0.06));
                ar_props[i].beacon_light_rotation_angle[0] += dt * ar_props[i].beacon_light_rotation_rate[0];//rotate baby!
                //billboard
                Vector3 vdir = ar_props[i].beacon_light[0]->getPosition() - gEnv->mainCamera->getPosition();
                float vlen = vdir.length();
                if (vlen > 100.0)
                {
                    ar_props[i].beacon_flare_billboard_scene_node[0]->setVisible(false);
                    continue;
                }
                //normalize
                vdir = vdir / vlen;
                ar_props[i].beacon_flare_billboard_scene_node[0]->setPosition(ar_props[i].beacon_light[0]->getPosition() - vdir * 0.1);
                bool visible = false;
                if (ar_props[i].beacon_light_rotation_angle[0] > 1.0)
                {
                    ar_props[i].beacon_light_rotation_angle[0] = 0.0;
                    visible = true;
                }
                visible = visible && enableAll;
                ar_props[i].beacon_light[0]->setVisible(visible);
                ar_props[i].beacon_flare_billboard_scene_node[0]->setVisible(visible);
            }
            if (ar_props[i].beacontype == 'R' || ar_props[i].beacontype == 'L')
            {
                Vector3 mposition = ar_nodes[ar_props[i].noderef].AbsPosition + ar_props[i].offsetx * (ar_nodes[ar_props[i].nodex].AbsPosition - ar_nodes[ar_props[i].noderef].AbsPosition) + ar_props[i].offsety * (ar_nodes[ar_props[i].nodey].AbsPosition - ar_nodes[ar_props[i].noderef].AbsPosition);
                //billboard
                Vector3 vdir = mposition - gEnv->mainCamera->getPosition();
                float vlen = vdir.length();
                if (vlen > 100.0)
                {
                    ar_props[i].beacon_flare_billboard_scene_node[0]->setVisible(false);
                    continue;
                }
                //normalize
                vdir = vdir / vlen;
                ar_props[i].beacon_flare_billboard_scene_node[0]->setPosition(mposition - vdir * 0.1);
            }
            if (ar_props[i].beacontype == 'w')
            {
                Vector3 mposition = ar_nodes[ar_props[i].noderef].AbsPosition + ar_props[i].offsetx * (ar_nodes[ar_props[i].nodex].AbsPosition - ar_nodes[ar_props[i].noderef].AbsPosition) + ar_props[i].offsety * (ar_nodes[ar_props[i].nodey].AbsPosition - ar_nodes[ar_props[i].noderef].AbsPosition);
                ar_props[i].beacon_light[0]->setPosition(mposition);
                ar_props[i].beacon_light_rotation_angle[0] += dt * ar_props[i].beacon_light_rotation_rate[0];//rotate baby!
                //billboard
                Vector3 vdir = mposition - gEnv->mainCamera->getPosition();
                float vlen = vdir.length();
                if (vlen > 100.0)
                {
                    ar_props[i].beacon_flare_billboard_scene_node[0]->setVisible(false);
                    continue;
                }
                //normalize
                vdir = vdir / vlen;
                ar_props[i].beacon_flare_billboard_scene_node[0]->setPosition(mposition - vdir * 0.1);
                bool visible = false;
                if (ar_props[i].beacon_light_rotation_angle[0] > 1.0)
                {
                    ar_props[i].beacon_light_rotation_angle[0] = 0.0;
                    visible = true;
                }
                visible = visible && enableAll;
                ar_props[i].beacon_light[0]->setVisible(visible);
                ar_props[i].beacon_flare_billboard_scene_node[0]->setVisible(visible);
            }
        }
    }
    //the flares
    bool keysleep = false;
    for (size_t i = 0; i < this->ar_flares.size(); i++)
    {
        // let the light blink
        if (ar_flares[i].blinkdelay != 0)
        {
            ar_flares[i].blinkdelay_curr -= dt;
            if (ar_flares[i].blinkdelay_curr <= 0)
            {
                ar_flares[i].blinkdelay_curr = ar_flares[i].blinkdelay;
                ar_flares[i].blinkdelay_state = !ar_flares[i].blinkdelay_state;
            }
        }
        else
        {
            ar_flares[i].blinkdelay_state = true;
        }

        // manage light states
        bool isvisible = true; //this must be true to be able to switch on the frontlight
        if (ar_flares[i].type == 'f')
        {
            m_gfx_actor->SetMaterialFlareOn(i, (ar_lights == 1));
            if (!ar_lights)
                continue;
        }
        else if (ar_flares[i].type == 'b')
        {
            isvisible = getBrakeLightVisible();
        }
        else if (ar_flares[i].type == 'R')
        {
            if (ar_engine || m_reverse_light_active)
                isvisible = getReverseLightVisible();
            else
                isvisible = false;
        }
        else if (ar_flares[i].type == 'u' && ar_flares[i].controlnumber != -1)
        {
            if (ar_sim_state == Beam::SimState::LOCAL_SIMULATED && this == App::GetSimController()->GetBeamFactory()->getCurrentTruck()) // no network!!
            {
                // networked customs are set directly, so skip this
                if (RoR::App::GetInputEngine()->getEventBoolValue(EV_TRUCK_LIGHTTOGGLE01 + (ar_flares[i].controlnumber - 1)) && m_custom_light_toggle_countdown <= 0)
                {
                    ar_flares[i].controltoggle_status = ! ar_flares[i].controltoggle_status;
                    keysleep = true;
                }
            }
            isvisible = ar_flares[i].controltoggle_status;
        }
        else if (ar_flares[i].type == 'l')
        {
            isvisible = (m_blink_type == BLINK_LEFT || m_blink_type == BLINK_WARN);
        }
        else if (ar_flares[i].type == 'r')
        {
            isvisible = (m_blink_type == BLINK_RIGHT || m_blink_type == BLINK_WARN);
        }
        // apply blinking
        isvisible = isvisible && ar_flares[i].blinkdelay_state;

        if (ar_flares[i].type == 'l' && m_blink_type == BLINK_LEFT)
        {
            ar_left_blink_on = isvisible;

            if (ar_left_blink_on)
                SOUND_PLAY_ONCE(ar_instance_id, SS_TRIG_TURN_SIGNAL_TICK);

            ar_dashboard->setBool(DD_SIGNAL_TURNLEFT, isvisible);
        }
        else if (ar_flares[i].type == 'r' && m_blink_type == BLINK_RIGHT)
        {
            ar_right_blink_on = isvisible;

            if (ar_right_blink_on)
                SOUND_PLAY_ONCE(ar_instance_id, SS_TRIG_TURN_SIGNAL_TICK);

            ar_dashboard->setBool(DD_SIGNAL_TURNRIGHT, isvisible);
        }
        else if (ar_flares[i].type == 'l' && m_blink_type == BLINK_WARN)
        {
            ar_warn_blink_on = isvisible;

            if (ar_warn_blink_on)
                SOUND_PLAY_ONCE(ar_instance_id, SS_TRIG_TURN_SIGNAL_WARN_TICK);

            ar_dashboard->setBool(DD_SIGNAL_TURNRIGHT, isvisible);
            ar_dashboard->setBool(DD_SIGNAL_TURNLEFT, isvisible);
        }

        // update material Bindings
        m_gfx_actor->SetMaterialFlareOn(i, isvisible);

        ar_flares[i].snode->setVisible(isvisible);
        if (ar_flares[i].light)
            ar_flares[i].light->setVisible(isvisible && enableAll);
        ar_flares[i].isVisible = isvisible;

        Vector3 normal = (ar_nodes[ar_flares[i].nodey].AbsPosition - ar_nodes[ar_flares[i].noderef].AbsPosition).crossProduct(ar_nodes[ar_flares[i].nodex].AbsPosition - ar_nodes[ar_flares[i].noderef].AbsPosition);
        normal.normalise();
        Vector3 mposition = ar_nodes[ar_flares[i].noderef].AbsPosition + ar_flares[i].offsetx * (ar_nodes[ar_flares[i].nodex].AbsPosition - ar_nodes[ar_flares[i].noderef].AbsPosition) + ar_flares[i].offsety * (ar_nodes[ar_flares[i].nodey].AbsPosition - ar_nodes[ar_flares[i].noderef].AbsPosition);
        Vector3 vdir = mposition - gEnv->mainCamera->getPosition();
        float vlen = vdir.length();
        // not visible from 500m distance
        if (vlen > 500.0)
        {
            ar_flares[i].snode->setVisible(false);
            continue;
        }
        //normalize
        vdir = vdir / vlen;
        float amplitude = normal.dotProduct(vdir);
        ar_flares[i].snode->setPosition(mposition - 0.1 * amplitude * normal * ar_flares[i].offsetz);
        ar_flares[i].snode->setDirection(normal);
        float fsize = ar_flares[i].size;
        if (fsize < 0)
        {
            amplitude = 1;
            fsize *= -1;
        }
        if (ar_flares[i].light)
        {
            ar_flares[i].light->setPosition(mposition - 0.2 * amplitude * normal);
            // point the real light towards the ground a bit
            ar_flares[i].light->setDirection(-normal - Vector3(0, 0.2, 0));
        }
        if (ar_flares[i].isVisible)
        {
            if (amplitude > 0)
            {
                ar_flares[i].bbs->setDefaultDimensions(amplitude * fsize, amplitude * fsize);
                ar_flares[i].snode->setVisible(true);
            }
            else
            {
                ar_flares[i].snode->setVisible(false);
            }
        }
        //ar_flares[i].bbs->_updateBounds();
    }
    if (keysleep)
        m_custom_light_toggle_countdown = 0.2;
    BES_GFX_STOP(BES_GFX_updateFlares);
}

void Beam::setBlinkType(blinktype blink)
{
    m_blink_type = blink;

    ar_left_blink_on = false;
    ar_right_blink_on = false;
    ar_warn_blink_on = false;

    if (blink == BLINK_NONE)
    {
        SOUND_STOP(ar_instance_id, SS_TRIG_TURN_SIGNAL);
    }
    else
    {
        SOUND_START(ar_instance_id, SS_TRIG_TURN_SIGNAL);
    }
}

void Beam::autoBlinkReset()
{
    blinktype blink = getBlinkType();

    // TODO: make this set-able per truck
    float blink_lock_range = 0.1f;

    if (blink == BLINK_LEFT && ar_hydro_dir_state < -blink_lock_range)
    // passed the threshold: the turn signal gets locked
        m_blinker_autoreset = true;

    if (blink == BLINK_LEFT && m_blinker_autoreset && ar_hydro_dir_state > -blink_lock_range)
    {
        // steering wheel turned back: turn signal gets automatically unlocked
        setBlinkType(BLINK_NONE);
        m_blinker_autoreset = false;
    }

    // same for the right turn signal
    if (blink == BLINK_RIGHT && ar_hydro_dir_state > blink_lock_range)
        m_blinker_autoreset = true;

    if (blink == BLINK_RIGHT && m_blinker_autoreset && ar_hydro_dir_state < blink_lock_range)
    {
        setBlinkType(BLINK_NONE);
        m_blinker_autoreset = false;
    }

    bool stopblink = false;
    ar_dashboard->setBool(DD_SIGNAL_TURNLEFT, stopblink);
    ar_dashboard->setBool(DD_SIGNAL_TURNRIGHT, stopblink);
}

void Beam::updateProps()
{
    BES_GFX_START(BES_GFX_updateProps);

    for (int i = 0; i < ar_num_props; i++)
    {
        if (!ar_props[i].scene_node)
            continue;

        Vector3 diffX = ar_nodes[ar_props[i].nodex].AbsPosition - ar_nodes[ar_props[i].noderef].AbsPosition;
        Vector3 diffY = ar_nodes[ar_props[i].nodey].AbsPosition - ar_nodes[ar_props[i].noderef].AbsPosition;

        Vector3 normal = (diffY.crossProduct(diffX)).normalisedCopy();

        Vector3 mposition = ar_nodes[ar_props[i].noderef].AbsPosition + ar_props[i].offsetx * diffX + ar_props[i].offsety * diffY;
        ar_props[i].scene_node->setPosition(mposition + normal * ar_props[i].offsetz);

        Vector3 refx = diffX.normalisedCopy();
        Vector3 refy = refx.crossProduct(normal);
        Quaternion orientation = Quaternion(refx, normal, refy) * ar_props[i].rot;
        ar_props[i].scene_node->setOrientation(orientation);

        if (ar_props[i].wheel)
        {
            Quaternion brot = Quaternion(Degree(-59.0), Vector3::UNIT_X);
            brot = brot * Quaternion(Degree(ar_hydro_dir_wheel_display * ar_props[i].wheelrotdegree), Vector3::UNIT_Y);
            ar_props[i].wheel->setPosition(mposition + normal * ar_props[i].offsetz + orientation * ar_props[i].wheelpos);
            ar_props[i].wheel->setOrientation(orientation * brot);
        }
    }

    for (int i = 0; i < ar_num_airbrakes; i++)
    {
        ar_airbrakes[i]->updatePosition((float)ar_airbrake_intensity / 5.0);
    }

    BES_GFX_STOP(BES_GFX_updateProps);
}

void Beam::toggleCustomParticles()
{
    m_custom_particles_enabled = !m_custom_particles_enabled;
    for (int i = 0; i < ar_num_custom_particles; i++)
    {
        ar_custom_particles[i].active = !ar_custom_particles[i].active;
        for (int j = 0; j < ar_custom_particles[i].psys->getNumEmitters(); j++)
        {
            ar_custom_particles[i].psys->getEmitter(j)->setEnabled(ar_custom_particles[i].active);
        }
    }

    //ScriptEvent - Particle Toggle
    TRIGGER_EVENT(SE_TRUCK_CPARTICLES_TOGGLE, ar_instance_id);
}

void Beam::updateSoundSources()
{
    BES_GFX_START(BES_GFX_updateSoundSources);
#ifdef USE_OPENAL
    if (SoundScriptManager::getSingleton().isDisabled())
        return;
    for (int i = 0; i < ar_num_soundsources; i++)
    {
        ar_soundsources[i].ssi->setPosition(ar_nodes[ar_soundsources[i].nodenum].AbsPosition, ar_nodes[ar_soundsources[i].nodenum].Velocity);
    }
    //also this, so it is updated always, and for any vehicle
    SOUND_MODULATE(ar_instance_id, SS_MOD_AIRSPEED, ar_nodes[0].Velocity.length() * 1.9438);
    SOUND_MODULATE(ar_instance_id, SS_MOD_WHEELSPEED, ar_wheel_speed * 3.6);
#endif //OPENAL
    BES_GFX_STOP(BES_GFX_updateSoundSources);
}

void Beam::updateLabels(float dt)
{
    if (m_net_label_node && m_net_label_mt)
    {
        // this ensures that the nickname is always in a readable size
        m_net_label_node->setPosition(m_avg_node_position + Vector3(0.0f, (ar_bounding_box.getMaximum().y - ar_bounding_box.getMinimum().y), 0.0f));
        Vector3 vdir = m_avg_node_position - gEnv->mainCamera->getPosition();
        float vlen = vdir.length();
        float h = std::max(0.6, vlen / 30.0);

        m_net_label_mt->setCharacterHeight(h);
        if (vlen > 1000) // 1000 ... vlen
            m_net_label_mt->setCaption(m_net_username + "  (" + TOSTRING((float)(ceil(vlen / 100) / 10.0) ) + " km)");
        else if (vlen > 20) // 20 ... vlen ... 1000
            m_net_label_mt->setCaption(m_net_username + "  (" + TOSTRING((int)vlen) + " m)");
        else // 0 ... vlen ... 20
            m_net_label_mt->setCaption(m_net_username);

        //m_net_label_mt->setAdditionalHeight((boundingBox.getMaximum().y - boundingBox.getMinimum().y) + h + 0.1);
    }
}

void Beam::updateFlexbodiesPrepare()
{
    BES_GFX_START(BES_GFX_updateFlexBodies);

    if (m_cab_scene_node && m_cab_mesh)
        m_cab_scene_node->setPosition(m_cab_mesh->UpdateFlexObj());

    if (gEnv->threadPool)
    {
        m_flexmesh_prepare.reset();
        for (int i = 0; i < free_wheel; i++)
        {
            m_flexmesh_prepare.set(i, vwheels[i].cnode && vwheels[i].fm->flexitPrepare());
        }

        m_flexbody_prepare.reset();
        for (int i = 0; i < ar_num_flexbodies; i++)
        {
            m_flexbody_prepare.set(i, ar_flexbodies[i]->flexitPrepare());
        }

        // Push tasks into thread pool
        for (int i = 0; i < ar_num_flexbodies; i++)
        {
            if (m_flexbody_prepare[i])
            {
                auto func = std::function<void()>([this, i]()
                    {
                        ar_flexbodies[i]->flexitCompute();
                    });
                auto task_handle = gEnv->threadPool->RunTask(func);
                m_flexbody_tasks.push_back(task_handle);
            }
        }
        for (int i = 0; i < free_wheel; i++)
        {
            if (m_flexmesh_prepare[i])
            {
                auto func = std::function<void()>([this, i]()
                    {
                        vwheels[i].fm->flexitCompute();
                    });
                auto task_handle = gEnv->threadPool->RunTask(func);
                m_flexbody_tasks.push_back(task_handle);
            }
        }
    }
    else
    {
        for (int i = 0; i < free_wheel; i++)
        {
            if (vwheels[i].cnode && vwheels[i].fm->flexitPrepare())
            {
                vwheels[i].fm->flexitCompute();
                vwheels[i].cnode->setPosition(vwheels[i].fm->flexitFinal());
            }
        }
        for (int i = 0; i < ar_num_flexbodies; i++)
        {
            if (ar_flexbodies[i]->flexitPrepare())
            {
                ar_flexbodies[i]->flexitCompute();
                ar_flexbodies[i]->flexitFinal();
            }
        }
    }
}

void Beam::updateVisual(float dt)
{
    BES_GFX_START(BES_GFX_updateVisual);

    Vector3 ref(Vector3::UNIT_Y);
    autoBlinkReset();
    updateSoundSources();

    if (m_debug_visuals)
        updateDebugOverlay();

#ifdef USE_OPENAL
    //airplane radio chatter
    if (ar_driveable == AIRPLANE && ar_sim_state != SimState::LOCAL_SLEEPING)
    {
        // play random chatter at random time
        m_avionic_chatter_timer -= dt;
        if (m_avionic_chatter_timer < 0)
        {
            SOUND_PLAY_ONCE(ar_instance_id, SS_TRIG_AVICHAT01 + Math::RangeRandom(0, 12));
            m_avionic_chatter_timer = Math::RangeRandom(11, 30);
        }
    }
#endif //openAL

    //update custom particle systems
    for (int i = 0; i < ar_num_custom_particles; i++)
    {
        Vector3 pos = ar_nodes[ar_custom_particles[i].emitterNode].AbsPosition;
        Vector3 dir = pos - ar_nodes[ar_custom_particles[i].directionNode].AbsPosition;
        //dir.normalise();
        dir = fast_normalise(dir);
        ar_custom_particles[i].snode->setPosition(pos);
        for (int j = 0; j < ar_custom_particles[i].psys->getNumEmitters(); j++)
        {
            ar_custom_particles[i].psys->getEmitter(j)->setDirection(dir);
        }
    }
    // update exhausts
    if (!m_disable_smoke && ar_engine && exhausts.size() > 0)
    {
        std::vector<exhaust_t>::iterator it;
        for (it = exhausts.begin(); it != exhausts.end(); it++)
        {
            if (!it->smoker)
                continue;
            Vector3 dir = ar_nodes[it->emitterNode].AbsPosition - ar_nodes[it->directionNode].AbsPosition;
            //			dir.normalise();
            ParticleEmitter* emit = it->smoker->getEmitter(0);
            it->smokeNode->setPosition(ar_nodes[it->emitterNode].AbsPosition);
            emit->setDirection(dir);
            if (ar_engine->getSmoke() != -1.0)
            {
                emit->setEnabled(true);
                emit->setColour(ColourValue(0.0, 0.0, 0.0, 0.02 + ar_engine->getSmoke() * 0.06));
                emit->setTimeToLive((0.02 + ar_engine->getSmoke() * 0.06) / 0.04);
            }
            else
            {
                emit->setEnabled(false);
            }
            emit->setParticleVelocity(1.0 + ar_engine->getSmoke() * 2.0, 2.0 + ar_engine->getSmoke() * 3.0);
        }
    }

    updateProps();

    for (int i = 0; i < ar_num_aeroengines; i++)
        ar_aeroengines[i]->updateVisuals();

    //wings
    float autoaileron = 0;
    float autorudder = 0;
    float autoelevator = 0;
    if (ar_autopilot)
    {
        ar_autopilot->UpdateIls(gEnv->terrainManager->getObjectManager()->GetLocalizers());
        autoaileron = ar_autopilot->getAilerons();
        autorudder = ar_autopilot->getRudder();
        autoelevator = ar_autopilot->getElevator();
        ar_autopilot->gpws_update(ar_posnode_spawn_height);
    }
    autoaileron += ar_aileron;
    autorudder += ar_rudder;
    autoelevator += ar_elevator;
    if (autoaileron < -1.0)
        autoaileron = -1.0;
    if (autoaileron > 1.0)
        autoaileron = 1.0;
    if (autorudder < -1.0)
        autorudder = -1.0;
    if (autorudder > 1.0)
        autorudder = 1.0;
    if (autoelevator < -1.0)
        autoelevator = -1.0;
    if (autoelevator > 1.0)
        autoelevator = 1.0;
    for (int i = 0; i < ar_num_wings; i++)
    {
        if (ar_wings[i].fa->type == 'a')
            ar_wings[i].fa->setControlDeflection(autoaileron);
        if (ar_wings[i].fa->type == 'b')
            ar_wings[i].fa->setControlDeflection(-autoaileron);
        if (ar_wings[i].fa->type == 'r')
            ar_wings[i].fa->setControlDeflection(autorudder);
        if (ar_wings[i].fa->type == 'e' || ar_wings[i].fa->type == 'S' || ar_wings[i].fa->type == 'T')
            ar_wings[i].fa->setControlDeflection(autoelevator);
        if (ar_wings[i].fa->type == 'f')
            ar_wings[i].fa->setControlDeflection(flapangles[ar_aerial_flap]);
        if (ar_wings[i].fa->type == 'c' || ar_wings[i].fa->type == 'V')
            ar_wings[i].fa->setControlDeflection((autoaileron + autoelevator) / 2.0);
        if (ar_wings[i].fa->type == 'd' || ar_wings[i].fa->type == 'U')
            ar_wings[i].fa->setControlDeflection((-autoaileron + autoelevator) / 2.0);
        if (ar_wings[i].fa->type == 'g')
            ar_wings[i].fa->setControlDeflection((autoaileron + flapangles[ar_aerial_flap]) / 2.0);
        if (ar_wings[i].fa->type == 'h')
            ar_wings[i].fa->setControlDeflection((-autoaileron + flapangles[ar_aerial_flap]) / 2.0);
        if (ar_wings[i].fa->type == 'i')
            ar_wings[i].fa->setControlDeflection((-autoelevator + autorudder) / 2.0);
        if (ar_wings[i].fa->type == 'j')
            ar_wings[i].fa->setControlDeflection((autoelevator + autorudder) / 2.0);
        ar_wings[i].cnode->setPosition(ar_wings[i].fa->flexit());
    }
    //setup commands for hydros
    ar_hydro_aileron_command = autoaileron;
    ar_hydro_rudder_command = autorudder;
    ar_hydro_elevator_command = autoelevator;

    if (m_cab_fade_mode > 0 && dt > 0)
    {
        if (m_cab_fade_timer > 0)
            m_cab_fade_timer -= dt;

        if (m_cab_fade_timer < 0.1 && m_cab_fade_mode == 1)
        {
            m_cab_fade_mode = 0;
            cabFade(0.4);
        }
        else if (m_cab_fade_timer < 0.1 && m_cab_fade_mode == 2)
        {
            m_cab_fade_mode = 0;
            cabFade(1);
        }

        if (m_cab_fade_mode == 1)
            cabFade(0.4 + 0.6 * m_cab_fade_timer / m_cab_fade_time);
        else if (m_cab_fade_mode == 2)
            cabFade(1 - 0.6 * m_cab_fade_timer / m_cab_fade_time);
    }

    for (int i = 0; i < ar_num_beams; i++)
    {
        if (!ar_beams[i].mSceneNode)
            continue;

        if (ar_beams[i].bm_disabled || ar_beams[i].bm_broken)
        {
            ar_beams[i].mSceneNode->detachAllObjects();
        }
        else if (ar_beams[i].bm_type != BEAM_INVISIBLE && ar_beams[i].bm_type != BEAM_INVISIBLE_HYDRO && ar_beams[i].bm_type != BEAM_VIRTUAL)
        {
            if (ar_beams[i].mSceneNode->numAttachedObjects() == 0)
                ar_beams[i].mSceneNode->attachObject(ar_beams[i].mEntity);

            ar_beams[i].mSceneNode->setPosition(ar_beams[i].p1->AbsPosition.midPoint(ar_beams[i].p2->AbsPosition));
            ar_beams[i].mSceneNode->setOrientation(specialGetRotationTo(ref, ar_beams[i].p1->AbsPosition - ar_beams[i].p2->AbsPosition));
            ar_beams[i].mSceneNode->setScale(ar_beams[i].diameter, (ar_beams[i].p1->AbsPosition - ar_beams[i].p2->AbsPosition).length(), ar_beams[i].diameter);
        }
    }

    if (ar_request_skeletonview_change)
    {
        if (ar_skeletonview_is_active && ar_request_skeletonview_change < 0)
        {
            hideSkeleton(true);
        }
        else if (!ar_skeletonview_is_active && ar_request_skeletonview_change > 0)
        {
            showSkeleton(true, true);
        }

        ar_request_skeletonview_change = 0;
    }

    if (ar_skeletonview_is_active)
        updateSimpleSkeleton();

    BES_GFX_STOP(BES_GFX_updateVisual);
}

void Beam::joinFlexbodyTasks()
{
    if (gEnv->threadPool)
    {
        for (const auto& t : m_flexbody_tasks)
        {
            t->join();
        }
        m_flexbody_tasks.clear();
    }
}

void Beam::updateFlexbodiesFinal()
{
    if (gEnv->threadPool)
    {
        joinFlexbodyTasks();

        for (int i = 0; i < free_wheel; i++)
        {
            if (m_flexmesh_prepare[i])
                vwheels[i].cnode->setPosition(vwheels[i].fm->flexitFinal());
        }
        for (int i = 0; i < ar_num_flexbodies; i++)
        {
            if (m_flexbody_prepare[i])
            {
                ar_flexbodies[i]->flexitFinal();
            }
        }
    }

    BES_GFX_STOP(BES_GFX_updateFlexBodies);
}

//v=0: full detail
//v=1: no beams
void Beam::setDetailLevel(int v)
{
    if (v != m_gfx_detail_level)
    {
        if (m_gfx_detail_level == 0 && v == 1)
        {
            // detach
            gEnv->sceneManager->getRootSceneNode()->removeChild(m_beam_visuals_parent_scenenode);
        }
        if (m_gfx_detail_level == 1 && v == 0)
        {
            // attach
            gEnv->sceneManager->getRootSceneNode()->addChild(m_beam_visuals_parent_scenenode);
        }
        m_gfx_detail_level = v;
    }
}

void Beam::showSkeleton(bool meshes, bool linked)
{
    ar_skeletonview_is_active = true;

    if (meshes)
    {
        m_cab_fade_mode = 1;
        m_cab_fade_timer = m_cab_fade_time;
    }
    else
    {
        m_cab_fade_mode = -1;
        // directly hide meshes, no fading
        cabFade(0);
    }

    for (int i = 0; i < free_wheel; i++)
    {
        if (vwheels[i].cnode)
            vwheels[i].cnode->setVisible(false);

        if (vwheels[i].fm)
            vwheels[i].fm->setVisible(false);
    }

    for (int i = 0; i < ar_num_props; i++)
    {
        if (ar_props[i].scene_node)
            setMeshWireframe(ar_props[i].scene_node, true);

        if (ar_props[i].wheel)
            setMeshWireframe(ar_props[i].wheel, true);
    }

    if (m_skeletonview_scenenode)
    {
        m_skeletonview_scenenode->setVisible(true);
    }

    // hide mesh wheels
    for (int i = 0; i < free_wheel; i++)
    {
        if (vwheels[i].fm && vwheels[i].meshwheel)
        {
            Entity* e = ((FlexMeshWheel*)(vwheels[i].fm))->getRimEntity();
            if (e)
                e->setVisible(false);
        }
    }

    // wireframe drawning for flexbody
    for (int i = 0; i < ar_num_flexbodies; i++)
    {
        SceneNode* s = ar_flexbodies[i]->getSceneNode();
        if (s)
            setMeshWireframe(s, true);
    }

    if (linked)
    {
        // apply to all locked trucks
        determineLinkedBeams();
        for (std::list<Beam*>::iterator it = m_linked_actors.begin(); it != m_linked_actors.end(); ++it)
        {
            (*it)->showSkeleton(meshes, false);
        }
    }

    updateSimpleSkeleton();

    TRIGGER_EVENT(SE_TRUCK_SKELETON_TOGGLE, ar_instance_id);
}

void Beam::hideSkeleton(bool linked)
{
    ar_skeletonview_is_active = false;

    if (m_cab_fade_mode >= 0)
    {
        m_cab_fade_mode = 2;
        m_cab_fade_timer = m_cab_fade_time;
    }
    else
    {
        m_cab_fade_mode = -1;
        // directly show meshes, no fading
        cabFade(1);
    }

    for (int i = 0; i < free_wheel; i++)
    {
        if (vwheels[i].cnode)
            vwheels[i].cnode->setVisible(true);

        if (vwheels[i].fm)
            vwheels[i].fm->setVisible(true);
    }
    for (int i = 0; i < ar_num_props; i++)
    {
        if (ar_props[i].scene_node)
            setMeshWireframe(ar_props[i].scene_node, false);

        if (ar_props[i].wheel)
            setMeshWireframe(ar_props[i].wheel, false);
    }

    if (m_skeletonview_scenenode)
        m_skeletonview_scenenode->setVisible(false);

    // show mesh wheels
    for (int i = 0; i < free_wheel; i++)
    {
        if (vwheels[i].fm && vwheels[i].meshwheel)
        {
            Entity* e = ((FlexMeshWheel *)(vwheels[i].fm))->getRimEntity();
            if (e)
                e->setVisible(true);
        }
    }

    // normal drawning for flexbody
    for (int i = 0; i < ar_num_flexbodies; i++)
    {
        SceneNode* s = ar_flexbodies[i]->getSceneNode();
        if (!s)
            continue;
        setMeshWireframe(s, false);
    }

    if (linked)
    {
        // apply to all locked trucks
        determineLinkedBeams();
        for (std::list<Beam*>::iterator it = m_linked_actors.begin(); it != m_linked_actors.end(); ++it)
        {
            (*it)->hideSkeleton(false);
        }
    }
}

void Beam::fadeMesh(SceneNode* node, float amount)
{
    for (int a = 0; a < node->numAttachedObjects(); a++)
    {
        Entity* e = (Entity *)node->getAttachedObject(a);
        MaterialPtr m = e->getSubEntity(0)->getMaterial();
        if (m.getPointer() == 0)
            continue;
        for (int x = 0; x < m->getNumTechniques(); x++)
        {
            for (int y = 0; y < m->getTechnique(x)->getNumPasses(); y++)
            {
                // TODO: fix this
                //m->getTechnique(x)->getPass(y)->setAlphaRejectValue(0);
                if (m->getTechnique(x)->getPass(y)->getNumTextureUnitStates() > 0)
                    m->getTechnique(x)->getPass(y)->getTextureUnitState(0)->setAlphaOperation(LBX_MODULATE, LBS_TEXTURE, LBS_MANUAL, 1.0, amount);
            }
        }
    }
}

float Beam::getAlphaRejection(SceneNode* node)
{
    for (int a = 0; a < node->numAttachedObjects(); a++)
    {
        Entity* e = (Entity *)node->getAttachedObject(a);
        MaterialPtr m = e->getSubEntity(0)->getMaterial();
        if (m.getPointer() == 0)
            continue;
        for (int x = 0; x < m->getNumTechniques(); x++)
        {
            for (int y = 0; y < m->getTechnique(x)->getNumPasses(); y++)
            {
                return m->getTechnique(x)->getPass(y)->getAlphaRejectValue();
            }
        }
    }
    return 0;
}

void Beam::setAlphaRejection(SceneNode* node, float amount)
{
    for (int a = 0; a < node->numAttachedObjects(); a++)
    {
        Entity* e = (Entity *)node->getAttachedObject(a);
        MaterialPtr m = e->getSubEntity(0)->getMaterial();
        if (m.getPointer() == 0)
            continue;
        for (int x = 0; x < m->getNumTechniques(); x++)
        {
            for (int y = 0; y < m->getTechnique(x)->getNumPasses(); y++)
            {
                m->getTechnique(x)->getPass(y)->setAlphaRejectValue((unsigned char)amount);
                return;
            }
        }
    }
}

void Beam::setMeshWireframe(SceneNode* node, bool value)
{
    for (int a = 0; a < node->numAttachedObjects(); a++)
    {
        Entity* e = (Entity *)node->getAttachedObject(a);
        for (int se = 0; se < (int)e->getNumSubEntities(); se++)
        {
            MaterialPtr m = e->getSubEntity(se)->getMaterial();
            if (m.getPointer() == 0)
                continue;
            for (int x = 0; x < m->getNumTechniques(); x++)
                for (int y = 0; y < m->getTechnique(x)->getNumPasses(); y++)
                    if (value)
                        m->getTechnique(x)->getPass(y)->setPolygonMode(PM_WIREFRAME);
                    else
                        m->getTechnique(x)->getPass(y)->setPolygonMode(PM_SOLID);
        }
    }
}

void Beam::setBeamVisibility(bool visible)
{
    for (int i = 0; i < ar_num_beams; i++)
    {
        if (ar_beams[i].mSceneNode)
        {
            ar_beams[i].mSceneNode->setVisible(visible);
        }
    }

    ar_beams_visible = visible;
}

void Beam::setMeshVisibility(bool visible)
{
    for (int i = 0; i < ar_num_props; i++)
    {
        if (ar_props[i].mo)
            ar_props[i].mo->setVisible(visible);
        if (ar_props[i].wheel)
            ar_props[i].wheel->setVisible(visible);
        if (ar_props[i].beacon_flare_billboard_scene_node[0])
            ar_props[i].beacon_flare_billboard_scene_node[0]->setVisible(visible);
        if (ar_props[i].beacon_flare_billboard_scene_node[1])
            ar_props[i].beacon_flare_billboard_scene_node[1]->setVisible(visible);
        if (ar_props[i].beacon_flare_billboard_scene_node[2])
            ar_props[i].beacon_flare_billboard_scene_node[2]->setVisible(visible);
        if (ar_props[i].beacon_flare_billboard_scene_node[3])
            ar_props[i].beacon_flare_billboard_scene_node[3]->setVisible(visible);
    }
    for (int i = 0; i < ar_num_flexbodies; i++)
    {
        ar_flexbodies[i]->setVisible(visible);
    }
    for (int i = 0; i < free_wheel; i++)
    {
        if (vwheels[i].cnode)
        {
            vwheels[i].cnode->setVisible(visible);
        }
        if (vwheels[i].fm)
        {
            vwheels[i].fm->setVisible(visible);
        }
    }
    if (m_cab_scene_node)
    {
        m_cab_scene_node->setVisible(visible);
    }

    ar_meshes_visible = visible;
}

void Beam::cabFade(float amount)
{
    static float savedCabAlphaRejection = 0;

    // truck cab
    if (m_cab_scene_node)
    {
        if (amount == 0)
        {
            m_cab_scene_node->setVisible(false);
        }
        else
        {
            if (amount == 1)
                m_cab_scene_node->setVisible(true);
            if (savedCabAlphaRejection == 0)
                savedCabAlphaRejection = getAlphaRejection(m_cab_scene_node);
            if (amount == 1)
                setAlphaRejection(m_cab_scene_node, savedCabAlphaRejection);
            else if (amount < 1)
                setAlphaRejection(m_cab_scene_node, 0);
            fadeMesh(m_cab_scene_node, amount);
        }
    }

    // wings
    for (int i = 0; i < ar_num_wings; i++)
    {
        if (amount == 0)
        {
            ar_wings[i].cnode->setVisible(false);
        }
        else
        {
            if (amount == 1)
                ar_wings[i].cnode->setVisible(true);
            fadeMesh(ar_wings[i].cnode, amount);
        }
    }
}

void Beam::addInterTruckBeam(beam_t* beam, Beam* a, Beam* b)
{
    auto pos = std::find(ar_inter_beams.begin(), ar_inter_beams.end(), beam);
    if (pos == ar_inter_beams.end())
    {
        ar_inter_beams.push_back(beam);
    }

    std::pair<Beam*, Beam*> truck_pair(a, b);
    App::GetSimController()->GetBeamFactory()->interTruckLinks[beam] = truck_pair;

    a->determineLinkedBeams();
    for (auto truck : a->m_linked_actors)
        truck->determineLinkedBeams();

    b->determineLinkedBeams();
    for (auto truck : b->m_linked_actors)
        truck->determineLinkedBeams();
}

void Beam::removeInterTruckBeam(beam_t* beam)
{
    auto pos = std::find(ar_inter_beams.begin(), ar_inter_beams.end(), beam);
    if (pos != ar_inter_beams.end())
    {
        ar_inter_beams.erase(pos);
    }

    auto it = App::GetSimController()->GetBeamFactory()->interTruckLinks.find(beam);
    if (it != App::GetSimController()->GetBeamFactory()->interTruckLinks.end())
    {
        auto truck_pair = it->second;
        App::GetSimController()->GetBeamFactory()->interTruckLinks.erase(it);

        truck_pair.first->determineLinkedBeams();
        for (auto truck : truck_pair.first->m_linked_actors)
            truck->determineLinkedBeams();

        truck_pair.second->determineLinkedBeams();
        for (auto truck : truck_pair.second->m_linked_actors)
            truck->determineLinkedBeams();
    }
}

void Beam::disjoinInterTruckBeams()
{
    ar_inter_beams.clear();
    auto interTruckLinks = &App::GetSimController()->GetBeamFactory()->interTruckLinks;
    for (auto it = interTruckLinks->begin(); it != interTruckLinks->end();)
    {
        auto truck_pair = it->second;
        if (this == truck_pair.first || this == truck_pair.second)
        {
            it->first->p2truck = false;
            it->first->bm_disabled = true;
            interTruckLinks->erase(it++);

            truck_pair.first->determineLinkedBeams();
            for (auto truck : truck_pair.first->m_linked_actors)
                truck->determineLinkedBeams();

            truck_pair.second->determineLinkedBeams();
            for (auto truck : truck_pair.second->m_linked_actors)
                truck->determineLinkedBeams();
        }
        else
        {
            ++it;
        }
    }
}

void Beam::tieToggle(int group)
{
    Beam** trucks = App::GetSimController()->GetBeamFactory()->getTrucks();
    int trucksnum = App::GetSimController()->GetBeamFactory()->getTruckCount();

    // export tie commands
    Beam* current_truck = App::GetSimController()->GetBeamFactory()->getCurrentTruck();
    if (ar_sim_state == Beam::SimState::LOCAL_SIMULATED && this == current_truck && ar_forward_commands)
    {
        for (int i = 0; i < trucksnum; i++)
        {
            if (trucks[i] && trucks[i]->ar_sim_state == Beam::SimState::LOCAL_SIMULATED && this->ar_instance_id != i && trucks[i]->ar_import_commands)
                trucks[i]->tieToggle(group);
        }
    }

    // untie all ties if one is tied
    bool istied = false;

    for (std::vector<tie_t>::iterator it = ties.begin(); it != ties.end(); it++)
    {
        // only handle ties with correct group
        if (group != -1 && (it->group != -1 && it->group != group))
            continue;

        // if tied, untie it. And the other way round
        if (it->tied)
        {
            istied = !it->beam->bm_disabled;

            // tie is locked and should get unlocked and stop tying
            it->tied = false;
            it->tying = false;
            if (it->lockedto)
                it->lockedto->in_use = false;
            // disable the ties beam
            it->beam->p2 = &ar_nodes[0];
            it->beam->p2truck = false;
            it->beam->bm_disabled = true;
            if (it->locked_truck != this)
            {
                removeInterTruckBeam(it->beam);
                // update skeletonview on the untied truck
                it->locked_truck->ar_request_skeletonview_change = -1;
            }
            it->locked_truck = nullptr;
        }
    }

    // iterate over all ties
    if (!istied)
    {
        for (std::vector<tie_t>::iterator it = ties.begin(); it != ties.end(); it++)
        {
            // only handle ties with correct group
            if (group != -1 && (it->group != -1 && it->group != group))
                continue;

            if (!it->tied)
            {
                // tie is unlocked and should get locked, search new remote ropable to lock to
                float mindist = it->beam->refL;
                node_t* shorter = 0;
                Beam* shtruck = 0;
                ropable_t* locktedto = 0;
                // iterate over all trucks
                for (int t = 0; t < trucksnum; t++)
                {
                    if (!trucks[t])
                        continue;
                    if (trucks[t]->ar_sim_state == SimState::LOCAL_SLEEPING)
                        continue;
                    // and their ropables
                    for (std::vector<ropable_t>::iterator itr = trucks[t]->ropables.begin(); itr != trucks[t]->ropables.end(); itr++)
                    {
                        // if the ropable is not multilock and used, then discard this ropable
                        if (!itr->multilock && itr->in_use)
                            continue;

                        //skip if tienode is ropable too (no selflock)
                        if (itr->node->id == it->beam->p1->id)
                            continue;

                        // calculate the distance and record the nearest ropable
                        float dist = (it->beam->p1->AbsPosition - itr->node->AbsPosition).length();
                        if (dist < mindist)
                        {
                            mindist = dist;
                            shorter = itr->node;
                            shtruck = trucks[t];
                            locktedto = &(*itr);
                        }
                    }
                }
                // if we found a ropable, then tie towards it
                if (shorter)
                {
                    // enable the beam and visually display the beam
                    it->beam->bm_disabled = false;
                    // now trigger the tying action
                    it->locked_truck = shtruck;
                    it->beam->p2 = shorter;
                    it->beam->p2truck = shtruck != this;
                    it->beam->stress = 0;
                    it->beam->L = it->beam->refL;
                    it->tied = true;
                    it->tying = true;
                    it->lockedto = locktedto;
                    it->lockedto->in_use = true;
                    if (it->beam->p2truck)
                    {
                        addInterTruckBeam(it->beam, this, shtruck);
                        // update skeletonview on the tied truck
                        shtruck->ar_request_skeletonview_change = ar_skeletonview_is_active ? 1 : -1;
                    }
                }
            }
        }
    }

    //ScriptEvent - Tie toggle
    TRIGGER_EVENT(SE_TRUCK_TIE_TOGGLE, ar_instance_id);
}

void Beam::ropeToggle(int group)
{
    Beam** trucks = App::GetSimController()->GetBeamFactory()->getTrucks();
    int trucksnum = App::GetSimController()->GetBeamFactory()->getTruckCount();

    // iterate over all ropes
    for (std::vector<rope_t>::iterator it = ropes.begin(); it != ropes.end(); it++)
    {
        // only handle ropes with correct group
        if (group != -1 && (it->group != -1 && it->group != group))
            continue;

        if (it->locked == LOCKED || it->locked == PRELOCK)
        {
            // we unlock ropes
            it->locked = UNLOCKED;
            // remove node locking
            if (it->lockedto_ropable)
                it->lockedto_ropable->in_use = false;
            it->lockedto = &ar_nodes[0];
            it->lockedtruck = 0;
        }
        else
        {
            //we lock ropes
            // search new remote ropable to lock to
            float mindist = it->beam->L;
            node_t* shorter = 0;
            Beam* shtruck = 0;
            ropable_t* rop = 0;
            // iterate over all trucks
            for (int t = 0; t < trucksnum; t++)
            {
                if (!trucks[t])
                    continue;
                if (trucks[t]->ar_sim_state == SimState::LOCAL_SLEEPING)
                    continue;
                // and their ropables
                for (std::vector<ropable_t>::iterator itr = trucks[t]->ropables.begin(); itr != trucks[t]->ropables.end(); itr++)
                {
                    // if the ropable is not multilock and used, then discard this ropable
                    if (!itr->multilock && itr->in_use)
                        continue;

                    // calculate the distance and record the nearest ropable
                    float dist = (it->beam->p1->AbsPosition - itr->node->AbsPosition).length();
                    if (dist < mindist)
                    {
                        mindist = dist;
                        shorter = itr->node;
                        shtruck = trucks[t];
                        rop = &(*itr);
                    }
                }
            }
            // if we found a ropable, then lock it
            if (shorter)
            {
                //okay, we have found a rope to tie
                it->lockedto = shorter;
                it->lockedtruck = shtruck;
                it->locked = PRELOCK;
                it->lockedto_ropable = rop;
                it->lockedto_ropable->in_use = true;
            }
        }
    }
}

void Beam::hookToggle(int group, hook_states mode, int node_number)
{
    Beam** trucks = App::GetSimController()->GetBeamFactory()->getTrucks();
    int trucksnum = App::GetSimController()->GetBeamFactory()->getTruckCount();

    // iterate over all hooks
    for (std::vector<hook_t>::iterator it = ar_hooks.begin(); it != ar_hooks.end(); it++)
    {
        if (mode == MOUSE_HOOK_TOGGLE && it->hookNode->id != node_number)
        {
            //skip all other nodes except the one manually toggled by mouse
            continue;
        }
        if (mode == HOOK_TOGGLE && group == -1)
        {
            //manually triggerd (EV_COMMON_LOCK). Toggle all hooks groups with group#: -1, 0, 1 ++
            if (it->group <= -2)
                continue;
        }
        if (mode == HOOK_LOCK && group == -2)
        {
            //automatic lock attempt (cyclic with doupdate). Toggle all hooks groups with group#: -2, -3, -4 --, skip the ones which are not autolock (triggered only)
            if (it->group >= -1 || !it->autolock)
                continue;
        }
        if (mode == HOOK_UNLOCK && group == -2)
        {
            //manual unlock ALL autolock and triggerlock, do not unlock standard hooks (EV_COMMON_AUTOLOCK)
            if (it->group >= -1 || !it->autolock)
                continue;
        }
        if ((mode == HOOK_LOCK || mode == HOOK_UNLOCK) && group <= -3)
        {
            //trigger beam lock or unlock. Toggle one hook group with group#: group
            if (it->group != group)
                continue;
        }
        if ((mode == HOOK_LOCK || mode == HOOK_UNLOCK) && group >= -1)
        {
            continue;
        }
        if (mode == HOOK_LOCK && it->timer > 0.0f)
        {
            //check relock delay timer for autolock nodes and skip if not 0
            continue;
        }

        Beam* lastLockTruck = it->lockTruck; // memorize current value

        // do this only for toggle or lock attempts, skip prelocked or locked nodes for performance
        if (mode != HOOK_UNLOCK && it->locked == UNLOCKED)
        {
            // we lock hooks
            // search new remote ropable to lock to
            float mindist = it->lockrange;
            float distance = 100000000.0f;
            // iterate over all trucks
            for (int t = 0; t < trucksnum; t++)
            {
                if (!trucks[t])
                    continue;
                if (trucks[t]->ar_sim_state == SimState::LOCAL_SLEEPING || trucks[t]->ar_sim_state == SimState::INVALID)
                    continue;
                if (t == this->ar_instance_id && !it->selflock)
                    continue; // don't lock to self

                // do we lock against all nodes or just against ropables?
                bool found = false;
                if (it->lockNodes)
                {
                    int last_node = 0; // node number storage
                    // all nodes, so walk them
                    for (int i = 0; i < trucks[t]->ar_num_nodes; i++)
                    {
                        // skip all nodes with lockgroup 9999 (deny lock)
                        if (trucks[t]->ar_nodes[i].lockgroup == 9999)
                            continue;

                        // exclude this truck and its current hooknode from the locking search
                        if (this == trucks[t] && i == it->hookNode->id)
                            continue;

                        // a lockgroup for this hooknode is set -> skip all nodes that do not have the same lockgroup (-1 = default(all nodes))
                        if (it->lockgroup != -1 && it->lockgroup != trucks[t]->ar_nodes[i].lockgroup)
                            continue;

                        // measure distance
                        float n2n_distance = (it->hookNode->AbsPosition - trucks[t]->ar_nodes[i].AbsPosition).length();
                        if (n2n_distance < mindist)
                        {
                            if (distance >= n2n_distance)
                            {
                                // located a node that is closer
                                distance = n2n_distance;
                                last_node = i;
                                found = true;
                            }
                        }
                    }
                    if (found)
                    {
                        // we found a node, lock to it
                        it->lockNode = &(trucks[t]->ar_nodes[last_node]);
                        it->lockTruck = trucks[t];
                        it->locked = PRELOCK;
                    }
                }
                else
                {
                    // we lock against ropables

                    node_t* shorter = 0;
                    Beam* shtruck = 0;

                    // and their ropables
                    for (std::vector<ropable_t>::iterator itr = trucks[t]->ropables.begin(); itr != trucks[t]->ropables.end(); itr++)
                    {
                        // if the ropable is not multilock and used, then discard this ropable
                        if (!itr->multilock && itr->in_use)
                            continue;

                        // calculate the distance and record the nearest ropable
                        float dist = (it->hookNode->AbsPosition - itr->node->AbsPosition).length();
                        if (dist < mindist)
                        {
                            mindist = dist;
                            shorter = itr->node;
                            shtruck = trucks[t];
                        }
                    }

                    if (shorter)
                    {
                        // we found a ropable, lock to it
                        it->lockNode = shorter;
                        it->lockTruck = shtruck;
                        it->locked = PRELOCK;
                    }
                }
            }
        }
        // this is a locked or prelocked hook and its not a locking attempt or the locked truck was removed (p2truck == false)
        else if ((it->locked == LOCKED || it->locked == PRELOCK) && (mode != HOOK_LOCK || !it->beam->p2truck))
        {
            // we unlock ropes
            it->locked = PREUNLOCK;
            if (it->group <= -2)
            {
                it->timer = it->timer_preset; //timer reset for autolock nodes
            }
            it->lockNode = 0;
            it->lockTruck = 0;
            //disable hook-assistance beam
            it->beam->p2 = &ar_nodes[0];
            it->beam->p2truck = false;
            it->beam->L = (ar_nodes[0].AbsPosition - it->hookNode->AbsPosition).length();
            it->beam->bm_disabled = true;
        }

        // update skeletonview on the (un)hooked truck
        if (it->lockTruck != lastLockTruck)
        {
            if (it->lockTruck)
            {
                it->lockTruck->ar_request_skeletonview_change = ar_skeletonview_is_active ? 1 : -1;
            }
            else if (lastLockTruck != this)
            {
                lastLockTruck->ar_request_skeletonview_change = -1;
            }
        }
    }
}

void Beam::parkingbrakeToggle()
{
    ar_parking_brake = !ar_parking_brake;

    if (ar_parking_brake)
        SOUND_START(ar_instance_id, SS_TRIG_PARK);
    else
        SOUND_STOP(ar_instance_id, SS_TRIG_PARK);

    //ScriptEvent - Parking Brake toggle
    TRIGGER_EVENT(SE_TRUCK_PARKINGBREAK_TOGGLE, ar_instance_id);
}

void Beam::antilockbrakeToggle()
{
    alb_mode = !alb_mode;
}

void Beam::tractioncontrolToggle()
{
    tc_mode = !tc_mode;
}

void Beam::cruisecontrolToggle()
{
    cc_mode = !cc_mode;

    if (cc_mode)
    {
        cc_target_speed = ar_wheel_speed;
        cc_target_rpm = ar_engine->getRPM();
    }
    else
    {
        cc_target_speed = 0;
        cc_target_rpm = 0;
        cc_accs.clear();
    }
}

void Beam::beaconsToggle()
{
    if (m_flares_mode == GfxFlaresMode::NONE) { return; }

    const bool enableLight = (m_flares_mode != GfxFlaresMode::NO_LIGHTSOURCES);

    bool beacon_light_is_active = !m_beacon_light_is_active;
    for (int i = 0; i < ar_num_props; i++)
    {
        char beacon_type = ar_props[i].beacontype;
        if (beacon_type == 'b')
        {
            ar_props[i].beacon_light[0]->setVisible(beacon_light_is_active && enableLight);
            ar_props[i].beacon_flare_billboard_scene_node[0]->setVisible(beacon_light_is_active);
            if (ar_props[i].beacon_flares_billboard_system[0] && beacon_light_is_active && !ar_props[i].beacon_flare_billboard_scene_node[0]->numAttachedObjects())
            {
                ar_props[i].beacon_flares_billboard_system[0]->setVisible(true);
                ar_props[i].beacon_flare_billboard_scene_node[0]->attachObject(ar_props[i].beacon_flares_billboard_system[0]);
            }
            else if (ar_props[i].beacon_flares_billboard_system[0] && !beacon_light_is_active)
            {
                ar_props[i].beacon_flare_billboard_scene_node[0]->detachAllObjects();
                ar_props[i].beacon_flares_billboard_system[0]->setVisible(false);
            }
        }
        else if (beacon_type == 'R' || beacon_type == 'L')
        {
            ar_props[i].beacon_flare_billboard_scene_node[0]->setVisible(beacon_light_is_active);
            if (ar_props[i].beacon_flares_billboard_system[0] && beacon_light_is_active && !ar_props[i].beacon_flare_billboard_scene_node[0]->numAttachedObjects())
                ar_props[i].beacon_flare_billboard_scene_node[0]->attachObject(ar_props[i].beacon_flares_billboard_system[0]);
            else if (ar_props[i].beacon_flares_billboard_system[0] && !beacon_light_is_active)
                ar_props[i].beacon_flare_billboard_scene_node[0]->detachAllObjects();
        }
        else if (beacon_type == 'p')
        {
            for (int k = 0; k < 4; k++)
            {
                ar_props[i].beacon_light[k]->setVisible(beacon_light_is_active && enableLight);
                ar_props[i].beacon_flare_billboard_scene_node[k]->setVisible(beacon_light_is_active);
                if (ar_props[i].beacon_flares_billboard_system[k] && beacon_light_is_active && !ar_props[i].beacon_flare_billboard_scene_node[k]->numAttachedObjects())
                    ar_props[i].beacon_flare_billboard_scene_node[k]->attachObject(ar_props[i].beacon_flares_billboard_system[k]);
                else if (ar_props[i].beacon_flares_billboard_system[k] && !beacon_light_is_active)
                    ar_props[i].beacon_flare_billboard_scene_node[k]->detachAllObjects();
            }
        }
        else
        {
            for (int k = 0; k < 4; k++)
            {
                if (ar_props[i].beacon_light[k])
                {
                    ar_props[i].beacon_light[k]->setVisible(beacon_light_is_active && enableLight);
                }
                if (ar_props[i].beacon_flare_billboard_scene_node[k])
                {
                    ar_props[i].beacon_flare_billboard_scene_node[k]->setVisible(beacon_light_is_active);

                    if (ar_props[i].beacon_flares_billboard_system[k] && beacon_light_is_active && !ar_props[i].beacon_flare_billboard_scene_node[k]->numAttachedObjects())
                    {
                        ar_props[i].beacon_flare_billboard_scene_node[k]->attachObject(ar_props[i].beacon_flares_billboard_system[k]);
                    }
                    else if (ar_props[i].beacon_flares_billboard_system[k] && !beacon_light_is_active)
                    {
                        ar_props[i].beacon_flare_billboard_scene_node[k]->detachAllObjects();
                    }
                }
            }
        }
    }
    m_beacon_light_is_active = beacon_light_is_active;

    //ScriptEvent - Beacon toggle
    TRIGGER_EVENT(SE_TRUCK_BEACONS_TOGGLE, ar_instance_id);
}

void Beam::setReplayMode(bool rm)
{
    if (!m_replay_handler || !m_replay_handler->isValid())
        return;

    if (ar_replay_mode && !rm)
    {
        ar_replay_pos = 0;
        m_replay_pos_prev = -1;
    }

    ar_replay_mode = rm;
    m_replay_handler->setVisible(ar_replay_mode);
}

void Beam::setDebugOverlayState(int mode)
{
    // enable disable debug visuals
    m_debug_visuals = mode;

    if (nodes_debug.empty())
    {
        LOG("initializing m_debug_visuals");
        // add node labels
        for (int i = 0; i < ar_num_nodes; i++)
        {
            debugtext_t t;
            RoR::Str<100> element_name;
            RigSpawner::ComposeName(element_name, "DbgNode", i, ar_instance_id);
            t.id = i;
            t.txt = new MovableText(element_name.ToCStr(), "n" + TOSTRING(i));
            t.txt->setFontName("highcontrast_black");
            t.txt->setTextAlignment(MovableText::H_LEFT, MovableText::V_BELOW);
            //t.txt->setAdditionalHeight(0);
            t.txt->showOnTop(true);
            t.txt->setCharacterHeight(0.5f);
            t.txt->setColor(ColourValue::White);
            t.txt->setRenderingDistance(2);

            t.node = gEnv->sceneManager->getRootSceneNode()->createChildSceneNode();
            m_deletion_scene_nodes.emplace_back(t.node);
            t.node->attachObject(t.txt);
            t.node->setPosition(ar_nodes[i].AbsPosition);
            t.node->setScale(Vector3(0.05, 0.05, 0.05));

            // collision nodes debug, also mimics as node visual
            SceneNode* s = t.node->createChildSceneNode();
            m_deletion_scene_nodes.emplace_back(s);
            RigSpawner::ComposeName(element_name, "DbgEntity", i, ar_instance_id);
            Entity* b = gEnv->sceneManager->createEntity(element_name.ToCStr(), "sphere.mesh");
            m_deletion_entities.emplace_back(b);
            b->setMaterialName("tracks/transgreen");
            s->attachObject(b);
            float f = 0.005f;
            s->setScale(f, f, f);
            nodes_debug.push_back(t);
        }

        // add beam labels
        for (int i = 0; i < ar_num_beams; i++)
        {
            debugtext_t t;
            RoR::Str<100> element_name;
            RigSpawner::ComposeName(element_name, "DbgBeam", i, ar_instance_id);
            t.id = i;
            t.txt = new MovableText(element_name.ToCStr(), "b" + TOSTRING(i));
            t.txt->setFontName("highcontrast_black");
            t.txt->setTextAlignment(MovableText::H_LEFT, MovableText::V_BELOW);
            //t.txt->setAdditionalHeight(0);
            t.txt->showOnTop(true);
            t.txt->setCharacterHeight(1);
            t.txt->setColor(ColourValue::Black);
            t.txt->setRenderingDistance(2);

            t.node = gEnv->sceneManager->getRootSceneNode()->createChildSceneNode();
            m_deletion_scene_nodes.emplace_back(t.node);
            t.node->attachObject(t.txt);

            Vector3 pos = ar_beams[i].p1->AbsPosition - (ar_beams[i].p1->AbsPosition - ar_beams[i].p2->AbsPosition) / 2;
            t.node->setPosition(pos);
            t.node->setVisible(false);
            t.node->setScale(Vector3(0.1, 0.1, 0.1));
            beams_debug.push_back(t);
        }
    }

    // then hide them according to the state:
    bool nodesVisible = m_debug_visuals == 1 || (m_debug_visuals >= 3 && m_debug_visuals <= 5);
    bool beamsVisible = m_debug_visuals == 2 || m_debug_visuals == 3 || (m_debug_visuals >= 6 && m_debug_visuals <= 11);

    for (std::vector<debugtext_t>::iterator it = nodes_debug.begin(); it != nodes_debug.end(); it++)
        it->node->setVisible(nodesVisible);
    for (std::vector<debugtext_t>::iterator it = beams_debug.begin(); it != beams_debug.end(); it++)
        it->node->setVisible(beamsVisible);

    updateDebugOverlay();
}

void Beam::updateDebugOverlay()
{
    if (!m_debug_visuals)
        return;

    switch (m_debug_visuals)
    {
    case 0: // off
        return;
    case 1: // node-numbers
        // not written dynamically
        for (std::vector<debugtext_t>::iterator it = nodes_debug.begin(); it != nodes_debug.end(); it++)
            it->node->setPosition(ar_nodes[it->id].AbsPosition);
        break;
    case 2: // beam-numbers
        // not written dynamically
        for (std::vector<debugtext_t>::iterator it = beams_debug.begin(); it != beams_debug.end(); it++)
            it->node->setPosition(ar_beams[it->id].p1->AbsPosition - (ar_beams[it->id].p1->AbsPosition - ar_beams[it->id].p2->AbsPosition) / 2);
        break;
    case 3: // node-and-beam-numbers
        // not written dynamically
        for (std::vector<debugtext_t>::iterator it = nodes_debug.begin(); it != nodes_debug.end(); it++)
            it->node->setPosition(ar_nodes[it->id].AbsPosition);
        for (std::vector<debugtext_t>::iterator it = beams_debug.begin(); it != beams_debug.end(); it++)
            it->node->setPosition(ar_beams[it->id].p1->AbsPosition - (ar_beams[it->id].p1->AbsPosition - ar_beams[it->id].p2->AbsPosition) / 2);
        break;
    case 4: // node-mass
        for (std::vector<debugtext_t>::iterator it = nodes_debug.begin(); it != nodes_debug.end(); it++)
        {
            it->node->setPosition(ar_nodes[it->id].AbsPosition);
            it->txt->setCaption(TOSTRING(ar_nodes[it->id].mass));
        }
        break;
    case 5: // node-locked
        for (std::vector<debugtext_t>::iterator it = nodes_debug.begin(); it != nodes_debug.end(); it++)
        {
            it->txt->setCaption((ar_nodes[it->id].locked) ? "locked" : "unlocked");
            it->node->setPosition(ar_nodes[it->id].AbsPosition);
        }
        break;
    case 6: // beam-compression
        for (std::vector<debugtext_t>::iterator it = beams_debug.begin(); it != beams_debug.end(); it++)
        {
            it->node->setPosition(ar_beams[it->id].p1->AbsPosition - (ar_beams[it->id].p1->AbsPosition - ar_beams[it->id].p2->AbsPosition) / 2);
            float stress_ratio = ar_beams[it->id].stress / ar_beams[it->id].minmaxposnegstress;
            float color_scale = std::abs(stress_ratio);
            color_scale = std::min(color_scale, 1.0f);
            int scale = (int)(color_scale * 100);
            it->txt->setCaption(TOSTRING(scale));
        }
        break;
    case 7: // beam-broken
        for (std::vector<debugtext_t>::iterator it = beams_debug.begin(); it != beams_debug.end(); it++)
        {
            it->node->setPosition(ar_beams[it->id].p1->AbsPosition - (ar_beams[it->id].p1->AbsPosition - ar_beams[it->id].p2->AbsPosition) / 2);
            if (ar_beams[it->id].bm_broken)
            {
                it->node->setVisible(true);
                it->txt->setCaption("BROKEN");
            }
            else
            {
                it->node->setVisible(false);
            }
        }
        break;
    case 8: // beam-stress
        for (std::vector<debugtext_t>::iterator it = beams_debug.begin(); it != beams_debug.end(); it++)
        {
            it->node->setPosition(ar_beams[it->id].p1->AbsPosition - (ar_beams[it->id].p1->AbsPosition - ar_beams[it->id].p2->AbsPosition) / 2);
            it->txt->setCaption(TOSTRING((float) fabs(ar_beams[it->id].stress)));
        }
        break;
    case 9: // beam-strength
        for (std::vector<debugtext_t>::iterator it = beams_debug.begin(); it != beams_debug.end(); it++)
        {
            it->node->setPosition(ar_beams[it->id].p1->AbsPosition - (ar_beams[it->id].p1->AbsPosition - ar_beams[it->id].p2->AbsPosition) / 2);
            it->txt->setCaption(TOSTRING(ar_beams[it->id].strength));
        }
        break;
    case 10: // beam-hydros
        for (std::vector<debugtext_t>::iterator it = beams_debug.begin(); it != beams_debug.end(); it++)
        {
            if (ar_beams[it->id].bm_type == BEAM_HYDRO || ar_beams[it->id].bm_type == BEAM_INVISIBLE_HYDRO)
            {
                it->node->setPosition(ar_beams[it->id].p1->AbsPosition - (ar_beams[it->id].p1->AbsPosition - ar_beams[it->id].p2->AbsPosition) / 2);
                int v = (ar_beams[it->id].L / ar_beams[it->id].Lhydro) * 100;
                it->txt->setCaption(TOSTRING(v));
                it->node->setVisible(true);
            }
            else
            {
                it->node->setVisible(false);
            }
        }
        break;
    case 11: // beam-commands
        for (std::vector<debugtext_t>::iterator it = beams_debug.begin(); it != beams_debug.end(); it++)
        {
            it->node->setPosition(ar_beams[it->id].p1->AbsPosition - (ar_beams[it->id].p1->AbsPosition - ar_beams[it->id].p2->AbsPosition) / 2);
            int v = (ar_beams[it->id].L / ar_beams[it->id].commandLong) * 100;
            it->txt->setCaption(TOSTRING(v));
        }
        break;
    }
}

void Beam::updateNetworkInfo()
{
    if (!(RoR::App::mp_state.GetActive() == RoR::MpState::CONNECTED))
        return;

#ifdef USE_SOCKETW
    BES_GFX_START(BES_GFX_updateNetworkInfo);

    RoRnet::UserInfo info;

    if (ar_sim_state == SimState::NETWORKED_OK)
    {
        if (!RoR::Networking::GetUserInfo(ar_net_source_id, info))
        {
            return;
        }
    }
    else
    {
        info = RoR::Networking::GetLocalUserData();
    }

    m_net_username = UTFString(info.username);

    BES_GFX_STOP(BES_GFX_updateNetworkInfo);
#endif //SOCKETW
}

float Beam::getHeadingDirectionAngle()
{
    if (ar_camera_node_pos[0] >= 0 && ar_camera_node_dir[0] >= 0)
    {
        Vector3 idir = ar_nodes[ar_camera_node_pos[0]].RelPosition - ar_nodes[ar_camera_node_dir[0]].RelPosition;
        return atan2(idir.dotProduct(Vector3::UNIT_X), (idir).dotProduct(-Vector3::UNIT_Z));
    }

    return 0.0f;
}

bool Beam::getReverseLightVisible()
{
    if (ar_sim_state == SimState::NETWORKED_OK)
        return m_net_reverse_light;

    if (ar_engine)
        return (ar_engine->getGear() < 0);

    return m_reverse_light_active;
}

void Beam::StopAllSounds()
{
#ifdef USE_OPENAL
    for (int i = 0; i < ar_num_soundsources; i++)
    {
        if (ar_soundsources[i].ssi)
            ar_soundsources[i].ssi->setEnabled(false);
    }
#endif // USE_OPENAL
}

void Beam::UnmuteAllSounds()
{
#ifdef USE_OPENAL
    for (int i = 0; i < ar_num_soundsources; i++)
    {
        bool enabled = (ar_soundsources[i].type == -2 || ar_soundsources[i].type == ar_current_cinecam);
        ar_soundsources[i].ssi->setEnabled(enabled);
    }
#endif // USE_OPENAL
}

void Beam::changedCamera()
{
    // change sound setup
#ifdef USE_OPENAL
    for (int i = 0; i < ar_num_soundsources; i++)
    {
        bool enabled = (ar_soundsources[i].type == -2 || ar_soundsources[i].type == ar_current_cinecam);
        ar_soundsources[i].ssi->setEnabled(enabled);
    }
#endif // USE_OPENAL

    // look for props
    for (int i = 0; i < ar_num_props; i++)
    {
        bool enabled = (ar_props[i].cameramode == -2 || ar_props[i].cameramode == ar_current_cinecam);
        if (ar_props[i].mo)
            ar_props[i].mo->setMeshEnabled(enabled);
    }

    // look for flexbodies
    for (int i = 0; i < ar_num_flexbodies; i++)
    {
        bool enabled = (ar_flexbodies[i]->getCameraMode() == -2 || ar_flexbodies[i]->getCameraMode() == ar_current_cinecam);
        ar_flexbodies[i]->setEnabled(enabled);
    }
}

//Returns the number of active (non bounded) beams connected to a node
int Beam::nodeBeamConnections(int nodeid)
{
    int totallivebeams = 0;
    for (unsigned int ni = 0; ni < ar_node_to_beam_connections[nodeid].size(); ++ni)
    {
        if (!ar_beams[ar_node_to_beam_connections[nodeid][ni]].bm_disabled && !ar_beams[ar_node_to_beam_connections[nodeid][ni]].bounded)
            totallivebeams++;
    }
    return totallivebeams;
}

bool Beam::isTied()
{
    for (std::vector<tie_t>::iterator it = ties.begin(); it != ties.end(); it++)
        if (it->tied)
            return true;
    return false;
}

bool Beam::isLocked()
{
    for (std::vector<hook_t>::iterator it = ar_hooks.begin(); it != ar_hooks.end(); it++)
        if (it->locked == LOCKED)
            return true;
    return false;
}

void Beam::updateDashBoards(float dt)
{
    if (!ar_dashboard)
        return;
    // some temp vars
    Vector3 dir;

    // engine and gears
    if (ar_engine)
    {
        // gears first
        int gear = ar_engine->getGear();
        ar_dashboard->setInt(DD_ENGINE_GEAR, gear);

        int numGears = (int)ar_engine->getNumGears();
        ar_dashboard->setInt(DD_ENGINE_NUM_GEAR, numGears);

        String str = String();

        // now construct that classic gear string
        if (gear > 0)
            str = TOSTRING(gear) + "/" + TOSTRING(numGears);
        else if (gear == 0)
            str = String("N");
        else
            str = String("R");

        ar_dashboard->setChar(DD_ENGINE_GEAR_STRING, str.c_str());

        // R N D 2 1 String
        int cg = ar_engine->getAutoShift();
        if (cg != BeamEngine::MANUALMODE)
        {
            str = ((cg == BeamEngine::REAR) ? "#ffffff" : "#868686") + String("R\n");
            str += ((cg == BeamEngine::NEUTRAL) ? "#ff0012" : "#8a000a") + String("N\n");
            str += ((cg == BeamEngine::DRIVE) ? "#12ff00" : "#248c00") + String("D\n");
            str += ((cg == BeamEngine::TWO) ? "#ffffff" : "#868686") + String("2\n");
            str += ((cg == BeamEngine::ONE) ? "#ffffff" : "#868686") + String("1");
        }
        else
        {
            //str = "#b8b8b8M\na\nn\nu\na\nl";
            str = "#b8b8b8M\na\nn\nu";
        }
        ar_dashboard->setChar(DD_ENGINE_AUTOGEAR_STRING, str.c_str());

        // autogears
        int autoGear = ar_engine->getAutoShift();
        ar_dashboard->setInt(DD_ENGINE_AUTO_GEAR, autoGear);

        // clutch
        float clutch = ar_engine->getClutch();
        ar_dashboard->setFloat(DD_ENGINE_CLUTCH, clutch);

        // accelerator
        float acc = ar_engine->getAcc();
        ar_dashboard->setFloat(DD_ACCELERATOR, acc);

        // RPM
        float rpm = ar_engine->getRPM();
        ar_dashboard->setFloat(DD_ENGINE_RPM, rpm);

        // turbo
        float turbo = ar_engine->getTurboPSI() * 3.34f; // MAGIC :/
        ar_dashboard->setFloat(DD_ENGINE_TURBO, turbo);

        // ignition
        bool ign = (ar_engine->hasContact() && !ar_engine->isRunning());
        ar_dashboard->setBool(DD_ENGINE_IGNITION, ign);

        // battery
        bool batt = (ar_engine->hasContact() && !ar_engine->isRunning());
        ar_dashboard->setBool(DD_ENGINE_BATTERY, batt);

        // clutch warning
        bool cw = (fabs(ar_engine->getTorque()) >= ar_engine->getClutchForce() * 10.0f);
        ar_dashboard->setBool(DD_ENGINE_CLUTCH_WARNING, cw);
    }

    // brake
    float dash_brake = ar_brake / ar_brake_force;
    ar_dashboard->setFloat(DD_BRAKE, dash_brake);

    // speedo
    float velocity = ar_nodes[0].Velocity.length();

    if (ar_camera_node_pos[0] >= 0 && ar_camera_node_dir[0] >= 0)
    {
        Vector3 hdir = (ar_nodes[ar_camera_node_pos[0]].RelPosition - ar_nodes[ar_camera_node_dir[0]].RelPosition).normalisedCopy();
        velocity = hdir.dotProduct(ar_nodes[0].Velocity);
    }
    float speed_kph = velocity * 3.6f;
    ar_dashboard->setFloat(DD_ENGINE_SPEEDO_KPH, speed_kph);
    float speed_mph = velocity * 2.23693629f;
    ar_dashboard->setFloat(DD_ENGINE_SPEEDO_MPH, speed_mph);

    // roll
    if (this->IsNodeIdValid(ar_camera_node_pos[0])) // TODO: why check this on each update when it cannot change after spawn?
    {
        dir = ar_nodes[ar_camera_node_pos[0]].RelPosition - ar_nodes[ar_camera_node_roll[0]].RelPosition;
        dir.normalise();
        float angle = asin(dir.dotProduct(Vector3::UNIT_Y));
        if (angle < -1)
            angle = -1;
        if (angle > 1)
            angle = 1;

        float f = Radian(angle).valueDegrees();
        ar_dashboard->setFloat(DD_ROLL, f);
    }

    // active shocks / roll correction
    if (this->ar_has_active_shocks)
    {
        // TOFIX: certainly not working:
        float roll_corr = - m_stabilizer_shock_ratio * 10.0f;
        ar_dashboard->setFloat(DD_ROLL_CORR, roll_corr);

        bool corr_active = (m_stabilizer_shock_request > 0);
        ar_dashboard->setBool(DD_ROLL_CORR_ACTIVE, corr_active);
    }

    // pitch
    if (this->IsNodeIdValid(ar_camera_node_pos[0]))
    {
        dir = ar_nodes[ar_camera_node_pos[0]].RelPosition - ar_nodes[ar_camera_node_dir[0]].RelPosition;
        dir.normalise();
        float angle = asin(dir.dotProduct(Vector3::UNIT_Y));
        if (angle < -1)
            angle = -1;
        if (angle > 1)
            angle = 1;

        float f = Radian(angle).valueDegrees();
        ar_dashboard->setFloat(DD_PITCH, f);
    }

    // parking brake
    bool pbrake = (ar_parking_brake > 0);
    ar_dashboard->setBool(DD_PARKINGBRAKE, pbrake);

    // locked lamp
    bool locked = isLocked();
    ar_dashboard->setBool(DD_LOCKED, locked);

    // low pressure lamp
    bool low_pres = !ar_engine_hydraulics_ready;
    ar_dashboard->setBool(DD_LOW_PRESSURE, low_pres);

    // lights
    bool lightsOn = (ar_lights > 0);
    ar_dashboard->setBool(DD_LIGHTS, lightsOn);

    // Traction Control
    if (tc_present)
    {
        int dash_tc_mode = 1; // 0 = not present, 1 = off, 2 = on, 3 = active
        if (tc_mode)
        {
            if (m_tractioncontrol)
                dash_tc_mode = 3;
            else
                dash_tc_mode = 2;
        }
        ar_dashboard->setInt(DD_TRACTIONCONTROL_MODE, dash_tc_mode);
    }

    // Anti Lock Brake
    if (alb_present)
    {
        int dash_alb_mode = 1; // 0 = not present, 1 = off, 2 = on, 3 = active
        if (alb_mode)
        {
            if (m_antilockbrake)
                dash_alb_mode = 3;
            else
                dash_alb_mode = 2;
        }
        ar_dashboard->setInt(DD_ANTILOCKBRAKE_MODE, dash_alb_mode);
    }

    // load secured lamp
    int ties_mode = 0; // 0 = not locked, 1 = prelock, 2 = lock
    if (isTied())
    {
        if (fabs(commandkey[0].commandValue) > 0.000001f)
            ties_mode = 1;
        else
            ties_mode = 2;
    }
    ar_dashboard->setInt(DD_TIES_MODE, ties_mode);

    // Boat things now: screwprops and alike
    if (ar_num_screwprops)
    {
        // the throttle and rudder
        for (int i = 0; i < ar_num_screwprops && i < DD_MAX_SCREWPROP; i++)
        {
            float throttle = ar_screwprops[i]->getThrottle();
            ar_dashboard->setFloat(DD_SCREW_THROTTLE_0 + i, throttle);

            float steering = ar_screwprops[i]->getRudder();
            ar_dashboard->setFloat(DD_SCREW_STEER_0 + i, steering);
        }

        // water depth display, only if we have a screw prop at least
        if (this->IsNodeIdValid(ar_camera_node_pos[0])) // TODO: Check cam. nodes once on spawn! They never change --> no reason to repeat the check. ~only_a_ptr, 06/2017
        {
            // position
            Vector3 dir = ar_nodes[ar_camera_node_pos[0]].RelPosition - ar_nodes[ar_camera_node_dir[0]].RelPosition;
            dir.normalise();

            int low_node = getLowestNode();
            if (low_node != -1)
            {
                Vector3 pos = ar_nodes[low_node].AbsPosition;
                float depth = pos.y - gEnv->terrainManager->getHeightFinder()->getHeightAt(pos.x, pos.z);
                ar_dashboard->setFloat(DD_WATER_DEPTH, depth);
            }
        }

        // water speed
        if (this->IsNodeIdValid(ar_camera_node_pos[0]))
        {
            Vector3 hdir = ar_nodes[ar_camera_node_pos[0]].RelPosition - ar_nodes[ar_camera_node_dir[0]].RelPosition;
            hdir.normalise();
            float knots = hdir.dotProduct(ar_nodes[ar_camera_node_pos[0]].Velocity) * 1.9438f; // 1.943 = m/s in knots/s
            ar_dashboard->setFloat(DD_WATER_SPEED, knots);
        }
    }

    // now airplane things, aeroengines, etc.
    if (ar_num_aeroengines)
    {
        for (int i = 0; i < ar_num_aeroengines && i < DD_MAX_AEROENGINE; i++)
        {
            float throttle = ar_aeroengines[i]->getThrottle();
            ar_dashboard->setFloat(DD_AEROENGINE_THROTTLE_0 + i, throttle);

            bool failed = ar_aeroengines[i]->isFailed();
            ar_dashboard->setBool(DD_AEROENGINE_FAILED_0 + i, failed);

            float pcent = ar_aeroengines[i]->getRPMpc();
            ar_dashboard->setFloat(DD_AEROENGINE_RPM_0 + i, pcent);
        }
    }

    // wings stuff, you dont need an aeroengine
    if (ar_num_wings)
    {
        for (int i = 0; i < ar_num_wings && i < DD_MAX_WING; i++)
        {
            // Angle of Attack (AOA)
            float aoa = ar_wings[i].fa->aoa;
            ar_dashboard->setFloat(DD_WING_AOA_0 + i, aoa);
        }
    }

    // some things only activate when a wing or an aeroengine is present
    if (ar_num_wings || ar_num_aeroengines)
    {
        //airspeed
        {
            float ground_speed_kt = ar_nodes[0].Velocity.length() * 1.9438f; // 1.943 = m/s in knots/s

            //tropospheric model valid up to 11.000m (33.000ft)
            float altitude = ar_nodes[0].AbsPosition.y;
            //float sea_level_temperature = 273.15 + 15.0; //in Kelvin // MAGICs D:
            float sea_level_pressure = 101325; //in Pa
            //float airtemperature        = sea_level_temperature - altitude * 0.0065f; //in Kelvin
            float airpressure = sea_level_pressure * pow(1.0f - 0.0065f * altitude / 288.15f, 5.24947f); //in Pa
            float airdensity = airpressure * 0.0000120896f; //1.225 at sea level

            float knots = ground_speed_kt * sqrt(airdensity / 1.225f); //KIAS
            ar_dashboard->setFloat(DD_AIRSPEED, knots);
        }

        // altimeter (height above ground)
        {
            float alt = ar_nodes[0].AbsPosition.y * 1.1811f; // MAGIC
            ar_dashboard->setFloat(DD_ALTITUDE, alt);

            char altc[11];
            sprintf(altc, "%03u", (int)(ar_nodes[0].AbsPosition.y / 30.48f)); // MAGIC
            ar_dashboard->setChar(DD_ALTITUDE_STRING, altc);
        }
    }

    ar_dashboard->setFloat(DD_ODOMETER_TOTAL, m_odometer_total);
    ar_dashboard->setFloat(DD_ODOMETER_USER, m_odometer_user);

    // set the features of this vehicle once
    if (!m_hud_features_ok)
    {
        bool hasEngine = (ar_engine != nullptr);
        bool hasturbo = false;
        bool autogearVisible = false;

        if (hasEngine)
        {
            hasturbo = ar_engine->hasTurbo();
            autogearVisible = (ar_engine->getAutoShift() != BeamEngine::MANUALMODE);
        }

        ar_dashboard->setEnabled(DD_ENGINE_TURBO, hasturbo);
        ar_dashboard->setEnabled(DD_ENGINE_GEAR, hasEngine);
        ar_dashboard->setEnabled(DD_ENGINE_NUM_GEAR, hasEngine);
        ar_dashboard->setEnabled(DD_ENGINE_GEAR_STRING, hasEngine);
        ar_dashboard->setEnabled(DD_ENGINE_AUTO_GEAR, hasEngine);
        ar_dashboard->setEnabled(DD_ENGINE_CLUTCH, hasEngine);
        ar_dashboard->setEnabled(DD_ENGINE_RPM, hasEngine);
        ar_dashboard->setEnabled(DD_ENGINE_IGNITION, hasEngine);
        ar_dashboard->setEnabled(DD_ENGINE_BATTERY, hasEngine);
        ar_dashboard->setEnabled(DD_ENGINE_CLUTCH_WARNING, hasEngine);

        ar_dashboard->setEnabled(DD_TRACTIONCONTROL_MODE, tc_present);
        ar_dashboard->setEnabled(DD_ANTILOCKBRAKE_MODE, alb_present);
        ar_dashboard->setEnabled(DD_TIES_MODE, !ties.empty());
        ar_dashboard->setEnabled(DD_LOCKED, !ar_hooks.empty());

        ar_dashboard->setEnabled(DD_ENGINE_AUTOGEAR_STRING, autogearVisible);

        ar_dashboard->updateFeatures();
        m_hud_features_ok = true;
    }

    // TODO: compass value

#if 0
    // ADI - attitude director indicator
    //roll
	Vector3 rollv=curr_truck->ar_nodes[curr_truck->ar_camera_node_pos[0]].RelPosition-curr_truck->ar_nodes[curr_truck->ar_camera_node_roll[0]].RelPosition;
	rollv.normalise();
	float rollangle=asin(rollv.dotProduct(Vector3::UNIT_Y));

    //pitch
	Vector3 dirv=curr_truck->ar_nodes[curr_truck->ar_camera_node_pos[0]].RelPosition-curr_truck->ar_nodes[curr_truck->ar_camera_node_dir[0]].RelPosition;
	dirv.normalise();
	float pitchangle=asin(dirv.dotProduct(Vector3::UNIT_Y));
	Vector3 upv=dirv.crossProduct(-rollv);
	if (upv.y<0) rollangle=3.14159-rollangle;
	RoR::App::GetOverlayWrapper()->adibugstexture->setTextureRotate(Radian(-rollangle));
	RoR::App::GetOverlayWrapper()->aditapetexture->setTextureVScroll(-pitchangle*0.25);
	RoR::App::GetOverlayWrapper()->aditapetexture->setTextureRotate(Radian(-rollangle));

    // HSI - Horizontal Situation Indicator
	Vector3 idir=curr_truck->ar_nodes[curr_truck->ar_camera_node_pos[0]].RelPosition-curr_truck->ar_nodes[curr_truck->ar_camera_node_dir[0]].RelPosition;
    //			idir.normalise();
	float dirangle=atan2(idir.dotProduct(Vector3::UNIT_X), idir.dotProduct(-Vector3::UNIT_Z));
	RoR::App::GetOverlayWrapper()->hsirosetexture->setTextureRotate(Radian(dirangle));
	if (curr_truck->autopilot)
	{
		RoR::App::GetOverlayWrapper()->hsibugtexture->setTextureRotate(Radian(dirangle)-Degree(curr_truck->autopilot->heading));
		float vdev=0;
		float hdev=0;
		curr_truck->autopilot->getRadioFix(localizers, free_localizer, &vdev, &hdev);
		if (hdev>15) hdev=15;
		if (hdev<-15) hdev=-15;
		RoR::App::GetOverlayWrapper()->hsivtexture->setTextureUScroll(-hdev*0.02);
		if (vdev>15) vdev=15;
		if (vdev<-15) vdev=-15;
		RoR::App::GetOverlayWrapper()->hsihtexture->setTextureVScroll(-vdev*0.02);
	}

    // VVI - Vertical Velocity Indicator
	float vvi=curr_truck->ar_nodes[0].Velocity.y*196.85;
	if (vvi<1000.0 && vvi>-1000.0) angle=vvi*0.047;
	if (vvi>1000.0 && vvi<6000.0) angle=47.0+(vvi-1000.0)*0.01175;
	if (vvi>6000.0) angle=105.75;
	if (vvi<-1000.0 && vvi>-6000.0) angle=-47.0+(vvi+1000.0)*0.01175;
	if (vvi<-6000.0) angle=-105.75;
	RoR::App::GetOverlayWrapper()->vvitexture->setTextureRotate(Degree(-angle+90.0));


	if (curr_truck->aeroengines[0]->getType() == AeroEngine::AEROENGINE_TYPE_TURBOPROP)
	{
		Turboprop *tp=(Turboprop*)curr_truck->aeroengines[0];
    //pitch
		RoR::App::GetOverlayWrapper()->airpitch1texture->setTextureRotate(Degree(-tp->pitch*2.0));
    //torque
		pcent=100.0*tp->indicated_torque/tp->max_torque;
		if (pcent<60.0) angle=-5.0+pcent*1.9167;
		else if (pcent<110.0) angle=110.0+(pcent-60.0)*4.075;
		else angle=314.0;
		RoR::App::GetOverlayWrapper()->airtorque1texture->setTextureRotate(Degree(-angle));
	}

	if (ftp>1 && curr_truck->aeroengines[1]->getType()==AeroEngine::AEROENGINE_TYPE_TURBOPROP)
	{
		Turboprop *tp=(Turboprop*)curr_truck->aeroengines[1];
    //pitch
		RoR::App::GetOverlayWrapper()->airpitch2texture->setTextureRotate(Degree(-tp->pitch*2.0));
    //torque
		pcent=100.0*tp->indicated_torque/tp->max_torque;
		if (pcent<60.0) angle=-5.0+pcent*1.9167;
		else if (pcent<110.0) angle=110.0+(pcent-60.0)*4.075;
		else angle=314.0;
		RoR::App::GetOverlayWrapper()->airtorque2texture->setTextureRotate(Degree(-angle));
	}

	if (ftp>2 && curr_truck->aeroengines[2]->getType()==AeroEngine::AEROENGINE_TYPE_TURBOPROP)
	{
		Turboprop *tp=(Turboprop*)curr_truck->aeroengines[2];
    //pitch
		RoR::App::GetOverlayWrapper()->airpitch3texture->setTextureRotate(Degree(-tp->pitch*2.0));
    //torque
		pcent=100.0*tp->indicated_torque/tp->max_torque;
		if (pcent<60.0) angle=-5.0+pcent*1.9167;
		else if (pcent<110.0) angle=110.0+(pcent-60.0)*4.075;
		else angle=314.0;
		RoR::App::GetOverlayWrapper()->airtorque3texture->setTextureRotate(Degree(-angle));
	}

	if (ftp>3 && curr_truck->aeroengines[3]->getType()==AeroEngine::AEROENGINE_TYPE_TURBOPROP)
	{
		Turboprop *tp=(Turboprop*)curr_truck->aeroengines[3];
    //pitch
		RoR::App::GetOverlayWrapper()->airpitch4texture->setTextureRotate(Degree(-tp->pitch*2.0));
    //torque
		pcent=100.0*tp->indicated_torque/tp->max_torque;
		if (pcent<60.0) angle=-5.0+pcent*1.9167;
		else if (pcent<110.0) angle=110.0+(pcent-60.0)*4.075;
		else angle=314.0;
		RoR::App::GetOverlayWrapper()->airtorque4texture->setTextureRotate(Degree(-angle));
	}

    //starters
	if (curr_truck->aeroengines[0]->getIgnition()) RoR::App::GetOverlayWrapper()->engstarto1->setMaterialName("tracks/engstart-on"); else RoR::App::GetOverlayWrapper()->engstarto1->setMaterialName("tracks/engstart-off");
	if (ftp>1 && curr_truck->aeroengines[1]->getIgnition()) RoR::App::GetOverlayWrapper()->engstarto2->setMaterialName("tracks/engstart-on"); else RoR::App::GetOverlayWrapper()->engstarto2->setMaterialName("tracks/engstart-off");
	if (ftp>2 && curr_truck->aeroengines[2]->getIgnition()) RoR::App::GetOverlayWrapper()->engstarto3->setMaterialName("tracks/engstart-on"); else RoR::App::GetOverlayWrapper()->engstarto3->setMaterialName("tracks/engstart-off");
	if (ftp>3 && curr_truck->aeroengines[3]->getIgnition()) RoR::App::GetOverlayWrapper()->engstarto4->setMaterialName("tracks/engstart-on"); else RoR::App::GetOverlayWrapper()->engstarto4->setMaterialName("tracks/engstart-off");
}

#endif //0
    ar_dashboard->update(dt);
}

Vector3 Beam::getGForces()
{
    // TODO: Check cam. nodes once on spawn! They never change --> no reason to repeat the check. ~only_a_ptr, 06/2017
    if (this->IsNodeIdValid(ar_camera_node_pos[0]) && this->IsNodeIdValid(ar_camera_node_dir[0]) && this->IsNodeIdValid(ar_camera_node_roll[0]))
    {
        static Vector3 result = Vector3::ZERO;

        if (m_camera_gforces_count == 0) // multiple calls in one single frame, avoid division by 0
        {
            return result;
        }

        Vector3 acc = m_camera_gforces_accu / m_camera_gforces_count;
        m_camera_gforces_accu = Vector3::ZERO;
        m_camera_gforces_count = 0;

        float longacc = acc.dotProduct((ar_nodes[ar_camera_node_pos[0]].RelPosition - ar_nodes[ar_camera_node_dir[0]].RelPosition).normalisedCopy());
        float latacc = acc.dotProduct((ar_nodes[ar_camera_node_pos[0]].RelPosition - ar_nodes[ar_camera_node_roll[0]].RelPosition).normalisedCopy());

        Vector3 diffdir = ar_nodes[ar_camera_node_pos[0]].RelPosition - ar_nodes[ar_camera_node_dir[0]].RelPosition;
        Vector3 diffroll = ar_nodes[ar_camera_node_pos[0]].RelPosition - ar_nodes[ar_camera_node_roll[0]].RelPosition;

        Vector3 upv = diffdir.crossProduct(-diffroll);
        upv.normalise();

        float gravity = DEFAULT_GRAVITY;

        if (gEnv->terrainManager)
        {
            gravity = gEnv->terrainManager->getGravity();
        }

        float vertacc = std::abs(gravity) - acc.dotProduct(-upv);

        result = Vector3(vertacc / std::abs(gravity), longacc / std::abs(gravity), latacc / std::abs(gravity));

        return result;
    }

    return Vector3::ZERO;
}

void Beam::engineTriggerHelper(int engineNumber, int type, float triggerValue)
{
    // engineNumber tells us which engine
    BeamEngine* e = ar_engine; // placeholder: trucks do not have multiple engines yet

    switch (type)
    {
    case TRG_ENGINE_CLUTCH:
        if (e)
            e->setClutch(triggerValue);
        break;
    case TRG_ENGINE_BRAKE:
        ar_brake = triggerValue * ar_brake_force;
        break;
    case TRG_ENGINE_ACC:
        if (e)
            e->setAcc(triggerValue);
        break;
    case TRG_ENGINE_RPM:
        // TODO: Implement setTargetRPM in the BeamEngine.cpp
        break;
    case TRG_ENGINE_SHIFTUP:
        if (e)
            e->shift(1);
        break;
    case TRG_ENGINE_SHIFTDOWN:
        if (e)
            e->shift(-1);
        break;
    default:
        break;
    }
}

Beam::Beam(
    RoRFrameListener* sim_controller,
    int truck_number,
    Ogre::Vector3 pos,
    Ogre::Quaternion rot,
    const char* fname,
    RoR::RigLoadingProfiler* rig_loading_profiler,
    bool _networked, /* = false  */
    bool _networking, /* = false  */
    collision_box_t* spawnbox, /* = nullptr */
    bool ismachine, /* = false  */
    const std::vector<Ogre::String>* truckconfig, /* = nullptr */
    RoR::SkinDef* skin, /* = nullptr */
    bool freeposition, /* = false */
    bool preloaded_with_terrain, /* = false */
    int cache_entry_number /* = -1 */
) 
    : ar_nodes(nullptr), ar_num_nodes(0)
    , ar_beams(nullptr), ar_num_beams(0)
    , ar_shocks(nullptr), ar_num_shocks(0)
    , ar_has_active_shocks(false)
    , ar_rotators(nullptr), ar_num_rotators(0)
    , ar_wings(nullptr), ar_num_wings(0)
    , m_hud_features_ok(false)
    , ar_aileron(0)
    , m_avionic_chatter_timer(11.0f) // some pseudo random number,  doesn't matter
    , m_beacon_light_is_active(false)
    , ar_beams_visible(true)
    , m_blink_type(BLINK_NONE)
    , m_blinker_autoreset(false)
    , ar_brake(0.0)
    , m_cab_fade_mode(0)
    , m_cab_fade_time(0.3)
    , m_cab_fade_timer(0)
    , m_camera_gforces_accu(Ogre::Vector3::ZERO)
    , m_camera_gforces_count(0)
    , ar_engine_hydraulics_ready(true)
    , m_custom_particles_enabled(false)
    , ar_scale(1)
    , ar_current_cinecam(-1) // -1 = external
    , ar_dashboard(nullptr)
    , m_gfx_detail_level(0)
    , ar_disable_aerodyn_turbulent_drag(false)
    , ar_disable_actor2actor_collision(false)
    , ar_disable_self_collision(false)
    , ar_elevator(0)
    , ar_aerial_flap(0)
    , ar_fusedrag(Ogre::Vector3::ZERO)
    , m_high_res_wheelnode_collisions(false)
    , ar_hydro_aileron_command(0)
    , ar_hydro_aileron_state(0)
    , ar_hydro_dir_command(0)
    , ar_hydro_dir_state(0)
    , ar_hydro_dir_wheel_display(0.0)
    , ar_hydro_elevator_command(0)
    , ar_hydro_elevator_state(0)
    , ar_hydro_rudder_command(0)
    , ar_hydro_rudder_state(0)
    , m_increased_accuracy(false)
    , m_inter_point_col_detector(nullptr)
    , m_intra_point_col_detector(nullptr)
    , ar_net_last_update_time(0)
    , m_avg_node_position_prev(pos)
    , ar_left_mirror_angle(0.52)
    , ar_lights(1)
    , m_avg_node_velocity(Ogre::Vector3::ZERO)
    , ar_custom_camera_node(-1)
    , m_hide_own_net_label(BSETTING("HideOwnNetLabel", false))
    , m_is_cinecam_rotation_center(false)
    , m_preloaded_with_terrain(preloaded_with_terrain)
    , ar_request_skeletonview_change(0)
    , m_reset_request(REQUEST_RESET_NONE)
    , ar_skeletonview_is_active(false)
    , ar_net_source_id(0)
    , m_spawn_rotation(0.0)
    , ar_net_stream_id(0)
    , m_custom_light_toggle_countdown(0)
    , ar_meshes_visible(true)
    , m_min_camera_radius(-1.0f)
    , m_mouse_grab_move_force(0.0f)
    , m_mouse_grab_node(-1)
    , m_mouse_grab_pos(Ogre::Vector3::ZERO)
    , m_net_brake_light(false)
    , m_net_label_node(0)
    , m_net_label_mt(0)
    , m_net_reverse_light(false)
    , m_replay_pos_prev(-1)
    , ar_parking_brake(0)
    , m_position_storage(0)
    , m_avg_node_position(pos)
    , m_previous_gear(0)
    , m_ref_tyre_pressure(50.0)
    , m_replay_handler(nullptr)
    , ar_replay_precision(0)
    , m_replay_timer(0)
    , ar_replay_length(10000)
    , ar_replay_mode(false)
    , ar_replay_pos(0)
    , m_reverse_light_active(false)
    , ar_right_mirror_angle(-0.52)
    , ar_rudder(0)
    , m_skeletonview_mesh_initialized(false)
    , m_skeletonview_manual_mesh(0)
    , ar_update_physics(false)
    , ar_sleep_counter(0.0f)
    , m_stabilizer_shock_request(0)
    , m_stabilizer_shock_ratio(0.0)
    , m_stabilizer_shock_sleep(0.0)
    , m_total_mass(0)
    , m_water_contact(false)
    , m_water_contact_old(false)
    , m_num_axles(0)
    , m_axles{} // Init array to nullptr
    , m_has_command_beams(false)
    , m_num_command_beams(0)
    , m_minimass(50.f)
    , m_load_mass(0.f)
    , m_dry_mass(0.f)
    , ar_gui_use_engine_max_rpm(false)
    , ar_autopilot(nullptr)
    , ar_is_police(false)
    , m_disable_default_sounds(false)
    , ar_uses_networking(false)
    , ar_engine(nullptr)
    , ar_driveable(NOT_DRIVEABLE)
    , m_skid_trails{} // Init array to nullptr
    , ar_collision_range(DEFAULT_COLLISION_RANGE)
    , ar_instance_id(truck_number)
    , ar_rescuer_flag(false)
    , m_antilockbrake(0)
    , m_tractioncontrol(0)
    , ar_forward_commands(false)
    , ar_import_commands(false)
    , ar_flexbodies{} // Init array to nullptr
    , ar_airbrakes{} // Init array to nullptr
    , ar_cabs{} // Init array to 0
    , ar_num_cabs(0)
    , ar_hydro{} // Init array to 0
    , ar_num_hydros(0)
    , ar_screwprops{} // Init array to nullptr
    , ar_num_screwprops(0)
    , ar_num_camera_rails(0)
    , ar_aeroengines() // Zero-init array
    , ar_num_aeroengines() // Zero-init
    , ar_pressure_beams() // Zero-init array
    , ar_free_pressure_beam() // Zero-init
    , ar_contacters() // memset() the array to zero
    , ar_num_contacters() // zero-init
{
    m_high_res_wheelnode_collisions = App::sim_hires_wheel_col.GetActive();
    m_use_skidmarks = RoR::App::gfx_skidmarks_mode.GetActive() == 1;
    LOG(" ===== LOADING VEHICLE: " + Ogre::String(fname));

    m_spawn_free_positioned = freeposition;
    m_used_skin = skin;
    ar_uses_networking = _networking;
    if (ismachine)
    {
        ar_driveable = MACHINE;
    }
    ar_filename = Ogre::String(fname);

    // copy truck config
    if (truckconfig != nullptr && truckconfig->size() > 0u)
    {
        for (std::vector<Ogre::String>::const_iterator it = truckconfig->begin(); it != truckconfig->end(); ++it)
        {
            m_truck_config.push_back(*it);
        }
    }

    Ogre::SceneNode* beams_parent = gEnv->sceneManager->getRootSceneNode()->createChildSceneNode();

    LOAD_RIG_PROFILE_CHECKPOINT(ENTRY_BEAM_CTOR_PREPARE_LOADTRUCK);

    if (strnlen(fname, 200) > 0)
    {
        if (! LoadTruck(rig_loading_profiler, fname, beams_parent, pos, rot, spawnbox, cache_entry_number))
        {
            LOG(" ===== FAILED LOADING VEHICLE: " + Ogre::String(fname));
            ar_sim_state = SimState::INVALID;
            return;
        }
    }

    // setup sounds properly
    changedCamera();

    // setup replay mode

    if (App::sim_replay_enabled.GetActive() && !_networked && !ar_uses_networking)
    {
        ar_replay_length = App::sim_replay_length.GetActive();
        m_replay_handler = new Replay(this, ar_replay_length);

        int steps = App::sim_replay_stepping.GetActive();

        if (steps <= 0)
            ar_replay_precision = 0.0f;
        else
            ar_replay_precision = 1.0f / ((float)steps);
    }

    // add storage
    if (App::sim_position_storage.GetActive())
    {
        m_position_storage = new PositionStorage(ar_num_nodes, 10);
    }

    // calculate the number of wheel nodes
    m_wheel_node_count = 0;
    for (int i = 0; i < ar_num_nodes; i++)
    {
        if (ar_nodes[i].iswheel != NOWHEEL)
            m_wheel_node_count++;
    }

    // search m_net_first_wheel_node
    m_net_first_wheel_node = ar_num_nodes;
    for (int i = 0; i < ar_num_nodes; i++)
    {
        if (ar_nodes[i].iswheel == WHEEL_DEFAULT)
        {
            m_net_first_wheel_node = i;
            break;
        }
    }

    // network buffer layout (without RoRnet::TruckState):
    //
    //  - 3 floats (x,y,z) for the reference node 0
    //  - ar_num_nodes - 1 times 3 short ints (compressed position info)
    //  - free_wheel times a float for the wheel rotation
    //
    m_net_node_buf_size = sizeof(float) * 3 + (m_net_first_wheel_node - 1) * sizeof(short int) * 3;
    m_net_buffer_size = m_net_node_buf_size + free_wheel * sizeof(float);
    updateFlexbodiesPrepare();
    updateFlexbodiesFinal();
    updateVisual();
    // stop lights
    lightsToggle();

    updateFlares(0);
    updateProps();
    if (ar_engine)
    {
        ar_engine->offstart();
    }
    // pressurize tires
    addPressure(0.0);

    CreateSimpleSkeletonMaterial();

    ar_sim_state = SimState::LOCAL_SLEEPING;

    // start network stuff
    if (_networked)
    {
        ar_sim_state = SimState::NETWORKED_OK;
        // malloc memory
        oob1 = (RoRnet::TruckState*)malloc(sizeof(RoRnet::TruckState));
        oob2 = (RoRnet::TruckState*)malloc(sizeof(RoRnet::TruckState));
        oob3 = (RoRnet::TruckState*)malloc(sizeof(RoRnet::TruckState));
        netb1 = (char*)malloc(m_net_buffer_size);
        netb2 = (char*)malloc(m_net_buffer_size);
        netb3 = (char*)malloc(m_net_buffer_size);
        m_net_time_offset = 0;
        m_net_update_counter = 0;
        if (ar_engine)
        {
            ar_engine->start();
        }
    }

    if (ar_uses_networking)
    {
        if (ar_sim_state != SimState::NETWORKED_OK)
        {
            sendStreamSetup();
        }

        if (ar_sim_state == SimState::NETWORKED_OK || !m_hide_own_net_label)
        {
            RoR::Str<100> element_name;
            RigSpawner::ComposeName(element_name, "NetLabel", 0, ar_instance_id);
            m_net_label_mt = new MovableText(element_name.ToCStr(), m_net_username);
            m_net_label_mt->setFontName("CyberbitEnglish");
            m_net_label_mt->setTextAlignment(MovableText::H_CENTER, MovableText::V_ABOVE);
            m_net_label_mt->showOnTop(false);
            m_net_label_mt->setCharacterHeight(2);
            m_net_label_mt->setColor(ColourValue::Black);
            m_net_label_mt->setVisible(true);

            m_net_label_node = gEnv->sceneManager->getRootSceneNode()->createChildSceneNode();
            m_net_label_node->attachObject(m_net_label_mt);
            m_net_label_node->setVisible(true);
            m_deletion_scene_nodes.emplace_back(m_net_label_node);
        }
    }

    LOG(" ===== DONE LOADING VEHICLE");
}

bool Beam::LoadTruck(
    RoR::RigLoadingProfiler* rig_loading_profiler,
    Ogre::String const& file_name,
    Ogre::SceneNode* parent_scene_node,
    Ogre::Vector3 const& spawn_position,
    Ogre::Quaternion& spawn_rotation,
    collision_box_t* spawn_box,
    int cache_entry_number // = -1
)
{
    Ogre::DataStreamPtr ds = Ogre::DataStreamPtr();
    Ogre::String fixed_file_name = file_name;
    Ogre::String found_resource_group;
    Ogre::String errorStr;

    try
    {
        RoR::App::GetCacheSystem()->checkResourceLoaded(fixed_file_name, found_resource_group); /* Fixes the filename and finds resource group */

        // error on ds open lower
        // open the stream and start reading :)
        ds = Ogre::ResourceGroupManager::getSingleton().openResource(fixed_file_name, found_resource_group);
    }
    catch (Ogre::Exception& e)
    {
        errorStr = Ogre::String(e.what());
        return false;
    }

    LOAD_RIG_PROFILE_CHECKPOINT(ENTRY_BEAM_LOADTRUCK_OPENFILE);

    if (ds.isNull() || !ds->isReadable())
    {
        Console* console = RoR::App::GetConsole();
        if (console != nullptr)
        {
            console->putMessage(
                Console::CONSOLE_MSGTYPE_INFO,
                Console::CONSOLE_SYSTEM_ERROR,
                "unable to load vehicle (unable to open file): " + fixed_file_name + " : " + errorStr,
                "error.png",
                30000,
                true
            );
            RoR::App::GetGuiManager()->PushNotification("Error:", "unable to load vehicle (unable to open file): " + fixed_file_name + " : " + errorStr);
        }
        return false;
    }

    /* PARSING */

    LOG(" == Parsing vehicle file: " + file_name);

    RigDef::Parser parser;
    LOAD_RIG_PROFILE_CHECKPOINT(ENTRY_BEAM_LOADTRUCK_PARSER_CREATE);
    parser.Prepare();
    LOAD_RIG_PROFILE_CHECKPOINT(ENTRY_BEAM_LOADTRUCK_PARSER_PREPARE);
    parser.ProcessOgreStream(ds.getPointer());
    LOAD_RIG_PROFILE_CHECKPOINT(ENTRY_BEAM_LOADTRUCK_PARSER_RUN);
    parser.Finalize();
    LOAD_RIG_PROFILE_CHECKPOINT(ENTRY_BEAM_LOADTRUCK_PARSER_FINALIZE);

    int report_num_errors = parser.GetMessagesNumErrors();
    int report_num_warnings = parser.GetMessagesNumWarnings();
    int report_num_other = parser.GetMessagesNumOther();
    std::string report_text = parser.ProcessMessagesToString();
    report_text += "\n\n";
    LOG(report_text);

    auto* importer = parser.GetSequentialImporter();
    if (importer->IsEnabled() && App::diag_rig_log_messages.GetActive())
    {
        report_num_errors += importer->GetMessagesNumErrors();
        report_num_warnings += importer->GetMessagesNumWarnings();
        report_num_other += importer->GetMessagesNumOther();

        std::string importer_report = importer->ProcessMessagesToString();
        LOG(importer_report);

        report_text += importer_report + "\n\n";
    }

    /* VALIDATING */
    LOAD_RIG_PROFILE_CHECKPOINT(ENTRY_BEAM_LOADTRUCK_POST_PARSE);
    LOG(" == Validating vehicle: " + parser.GetFile()->name);

    RigDef::Validator validator;
    validator.Setup(parser.GetFile());
    LOAD_RIG_PROFILE_CHECKPOINT(ENTRY_BEAM_LOADTRUCK_VALIDATOR_INIT);

    // Workaround: Some terrains pre-load truckfiles with special purpose:
    //     "soundloads" = play sound effect at certain spot
    //     "fixes"      = structures of N/B fixed to the ground
    // These files can have no beams. Possible extensions: .load or .fixed
    Ogre::String file_extension = file_name.substr(file_name.find_last_of('.'));
    Ogre::StringUtil::toLowerCase(file_extension);
    bool extension_matches = (file_extension == ".load") | (file_extension == ".fixed");
    if (m_preloaded_with_terrain && extension_matches)
    {
        validator.SetCheckBeams(false);
    }
    bool valid = validator.Validate();
    LOAD_RIG_PROFILE_CHECKPOINT(ENTRY_BEAM_LOADTRUCK_VALIDATOR_RUN);

    report_num_errors += validator.GetMessagesNumErrors();
    report_num_warnings += validator.GetMessagesNumWarnings();
    report_num_other += validator.GetMessagesNumOther();
    std::string validator_report = validator.ProcessMessagesToString();
    LOG(validator_report);
    report_text += validator_report;
    report_text += "\n\n";
    // Continue anyway...
    LOAD_RIG_PROFILE_CHECKPOINT(ENTRY_BEAM_LOADTRUCK_POST_VALIDATION);

    /* PROCESSING */

    LOG(" == Spawning vehicle: " + parser.GetFile()->name);

    RigSpawner spawner;
    spawner.Setup(this, parser.GetFile(), parent_scene_node, spawn_position, cache_entry_number);
    LOAD_RIG_PROFILE_CHECKPOINT(ENTRY_BEAM_LOADTRUCK_SPAWNER_SETUP);
    /* Setup modules */
    spawner.AddModule(parser.GetFile()->root_module);
    if (parser.GetFile()->user_modules.size() > 0) /* The vehicle-selector may return selected modules even for vehicle with no modules defined! Hence this check. */
    {
        std::vector<Ogre::String>::iterator itor = m_truck_config.begin();
        for (; itor != m_truck_config.end(); itor++)
        {
            spawner.AddModule(*itor);
        }
    }
    LOAD_RIG_PROFILE_CHECKPOINT(ENTRY_BEAM_LOADTRUCK_SPAWNER_ADDMODULES);
    spawner.SpawnRig();
    LOAD_RIG_PROFILE_CHECKPOINT(ENTRY_BEAM_LOADTRUCK_SPAWNER_RUN);
    report_num_errors += spawner.GetMessagesNumErrors();
    report_num_warnings += spawner.GetMessagesNumWarnings();
    report_num_other += spawner.GetMessagesNumOther();
    // Spawner log already printed to RoR.log
    report_text += spawner.ProcessMessagesToString() + "\n\n";

    // Extra information to RoR.log
    if (importer->IsEnabled())
    {
        if (App::diag_rig_log_node_stats.GetActive())
        {
            LOG(importer->GetNodeStatistics());
        }
        if (App::diag_rig_log_node_import.GetActive())
        {
            LOG(importer->IterateAndPrintAllNodes());
        }
    }

    RoR::App::GetGuiManager()->AddRigLoadingReport(parser.GetFile()->name, report_text, report_num_errors, report_num_warnings, report_num_other);
    if (report_num_errors != 0)
    {
        if (BSETTING("AutoRigSpawnerReport", false))
        {
            RoR::App::GetGuiManager()->SetVisible_SpawnerReport(true);
        }
    }
    LOAD_RIG_PROFILE_CHECKPOINT(ENTRY_BEAM_LOADTRUCK_SPAWNER_LOG);
    /* POST-PROCESSING (Old-spawn code from Beam::loadTruck2) */

    // Apply spawn position & spawn rotation
    for (int i = 0; i < ar_num_nodes; i++)
    {
        ar_nodes[i].AbsPosition = spawn_position + spawn_rotation * (ar_nodes[i].AbsPosition - spawn_position);
        ar_nodes[i].RelPosition = ar_nodes[i].AbsPosition - ar_origin;
    };

    /* Place correctly */
    if (! parser.GetFile()->HasFixes())
    {
        Ogre::Vector3 vehicle_position = spawn_position;

        // check if over-sized
        RigSpawner::RecalculateBoundingBoxes(this);
        vehicle_position.x -= (ar_bounding_box.getMaximum().x + ar_bounding_box.getMinimum().x) / 2.0 - vehicle_position.x;
        vehicle_position.z -= (ar_bounding_box.getMaximum().z + ar_bounding_box.getMinimum().z) / 2.0 - vehicle_position.z;

        float miny = 0.0f;

        if (!m_preloaded_with_terrain)
        {
            miny = vehicle_position.y;
        }

        if (spawn_box != nullptr)
        {
            miny = spawn_box->relo.y + spawn_box->center.y;
        }

        if (m_spawn_free_positioned)
            resetPosition(vehicle_position, true);
        else
            resetPosition(vehicle_position.x, vehicle_position.z, true, miny);

        if (spawn_box != nullptr)
        {
            bool inside = true;

            for (int i = 0; i < ar_num_nodes; i++)
                inside = (inside && gEnv->collisions->isInside(ar_nodes[i].AbsPosition, spawn_box, 0.2f));

            if (!inside)
            {
                Vector3 gpos = Vector3(vehicle_position.x, 0.0f, vehicle_position.z);

                gpos -= spawn_rotation * Vector3((spawn_box->hi.x - spawn_box->lo.x + ar_bounding_box.getMaximum().x - ar_bounding_box.getMinimum().x) * 0.6f, 0.0f, 0.0f);

                resetPosition(gpos.x, gpos.z, true, miny);
            }
        }
    }
    else
    {
        resetPosition(spawn_position, true);
    }
    LOAD_RIG_PROFILE_CHECKPOINT(ENTRY_BEAM_LOADTRUCK_FIXES);

    //compute final mass
    calc_masses2(m_dry_mass);
    LOAD_RIG_PROFILE_CHECKPOINT(ENTRY_BEAM_LOADTRUCK_CALC_MASSES);
    //setup default sounds
    if (!m_disable_default_sounds)
    {
        RigSpawner::SetupDefaultSoundSources(this);
    }
    LOAD_RIG_PROFILE_CHECKPOINT(ENTRY_BEAM_LOADTRUCK_SET_DEFAULT_SND_SOURCES);

    //compute node connectivity graph
    calcNodeConnectivityGraph();
    LOAD_RIG_PROFILE_CHECKPOINT(ENTRY_BEAM_LOADTRUCK_CALC_NODE_CONNECT_GRAPH);

    RigSpawner::RecalculateBoundingBoxes(this);
    LOAD_RIG_PROFILE_CHECKPOINT(ENTRY_BEAM_LOADTRUCK_RECALC_BOUNDING_BOXES);

    // fix up submesh collision model
    std::string subMeshGroundModelName = spawner.GetSubmeshGroundmodelName();
    if (!subMeshGroundModelName.empty())
    {
        ar_submesh_ground_model = gEnv->collisions->getGroundModelByString(subMeshGroundModelName);
        if (!ar_submesh_ground_model)
        {
            ar_submesh_ground_model = gEnv->collisions->defaultgm;
        }
    }

    // print some truck memory stats
    int mem = 0, memr = 0, tmpmem = 0;
    LOG("BEAM: memory stats following");

    tmpmem = ar_num_props * sizeof(prop_t);
    mem += tmpmem;
    memr += MAX_PROPS * sizeof(beam_t);
    LOG("BEAM: prop memory: " + TOSTRING(tmpmem) + " B (" + TOSTRING(ar_num_props) + " x " + TOSTRING(sizeof(prop_t)) + " B) / " + TOSTRING(MAX_PROPS * sizeof(prop_t)));

    tmpmem = free_wheel * sizeof(wheel_t);
    mem += tmpmem;
    memr += MAX_WHEELS * sizeof(beam_t);
    LOG("BEAM: wheel memory: " + TOSTRING(tmpmem) + " B (" + TOSTRING(free_wheel) + " x " + TOSTRING(sizeof(wheel_t)) + " B) / " + TOSTRING(MAX_WHEELS * sizeof(wheel_t)));

    LOG("BEAM: truck memory used: " + TOSTRING(mem) + " B (" + TOSTRING(mem/1024) + " kB)");
    LOG("BEAM: truck memory allocated: " + TOSTRING(memr) + " B (" + TOSTRING(memr/1024) + " kB)");

    LOAD_RIG_PROFILE_CHECKPOINT(ENTRY_BEAM_LOADTRUCK_GROUNDMODEL_AND_STATS);
    // now load any dashboards
    if (ar_dashboard)
    {
        if (m_dashboard_layouts.empty())
        {
            // load default for a truck
            if (ar_driveable == TRUCK)
            {
                if (App::gfx_speedo_digital.GetActive())
                {
                    if (App::gfx_speedo_imperial.GetActive())
                    {
                        if (ar_engine->getMaxRPM() > 3500)
                        {
                            //7000 rpm tachometer thanks to klink
                            ar_dashboard->loadDashBoard("default_dashboard7000_mph.layout", false);
                            // TODO: load texture dashboard by default as well
                            ar_dashboard->loadDashBoard("default_dashboard7000_mph.layout", true);
                        }
                        else
                        {
                            ar_dashboard->loadDashBoard("default_dashboard3500_mph.layout", false);
                            // TODO: load texture dashboard by default as well
                            ar_dashboard->loadDashBoard("default_dashboard3500_mph.layout", true);
                        }
                    }
                    else
                    {
                        if (ar_engine->getMaxRPM() > 3500)
                        {
                            //7000 rpm tachometer thanks to klink
                            ar_dashboard->loadDashBoard("default_dashboard7000.layout", false);
                            // TODO: load texture dashboard by default as well
                            ar_dashboard->loadDashBoard("default_dashboard7000.layout", true);
                        }
                        else
                        {
                            ar_dashboard->loadDashBoard("default_dashboard3500.layout", false);
                            // TODO: load texture dashboard by default as well
                            ar_dashboard->loadDashBoard("default_dashboard3500.layout", true);
                        }
                    }
                }
                else
                {
                    if (App::gfx_speedo_imperial.GetActive())
                    {
                        if (ar_engine->getMaxRPM() > 3500)
                        {
                            //7000 rpm tachometer thanks to klink
                            ar_dashboard->loadDashBoard("default_dashboard7000_analog_mph.layout", false);
                            // TODO: load texture dashboard by default as well
                            ar_dashboard->loadDashBoard("default_dashboard7000_analog_mph.layout", true);
                        }
                        else
                        {
                            ar_dashboard->loadDashBoard("default_dashboard3500_analog_mph.layout", false);
                            // TODO: load texture dashboard by default as well
                            ar_dashboard->loadDashBoard("default_dashboard3500_analog_mph.layout", true);
                        }
                    }
                    else
                    {
                        if (ar_engine->getMaxRPM() > 3500)
                        {
                            //7000 rpm tachometer thanks to klink
                            ar_dashboard->loadDashBoard("default_dashboard7000_analog.layout", false);
                            // TODO: load texture dashboard by default as well
                            ar_dashboard->loadDashBoard("default_dashboard7000_analog.layout", true);
                        }
                        else
                        {
                            ar_dashboard->loadDashBoard("default_dashboard3500_analog.layout", false);
                            // TODO: load texture dashboard by default as well
                            ar_dashboard->loadDashBoard("default_dashboard3500_analog.layout", true);
                        }
                    }
                }
            }
            else if (ar_driveable == BOAT)
            {
                ar_dashboard->loadDashBoard("default_dashboard_boat.layout", false);
                // TODO: load texture dashboard by default as well
                ar_dashboard->loadDashBoard("default_dashboard_boat.layout", true);
            }
        }
        else
        {
            // load all dashes
            for (unsigned int i = 0; i < m_dashboard_layouts.size(); i++)
            {
                ar_dashboard->loadDashBoard(m_dashboard_layouts[i].first, m_dashboard_layouts[i].second);
            }
        }
        ar_dashboard->setVisible(false);
    }
    LOAD_RIG_PROFILE_CHECKPOINT(ENTRY_BEAM_LOADTRUCK_LOAD_DASHBOARDS);

    // Set beam defaults
    for (int i = 0; i < ar_num_beams; i++)
    {
        ar_beams[i].initial_beam_strength       = ar_beams[i].strength;
        ar_beams[i].default_beam_deform         = ar_beams[i].minmaxposnegstress;
        ar_beams[i].default_beam_plastic_coef   = ar_beams[i].plastic_coef;
    }

    // TODO: Check cam. nodes once on spawn! They never change --> no reason to repeat the check. ~only_a_ptr, 06/2017
    if (ar_camera_node_pos[0] != ar_camera_node_dir[0] && this->IsNodeIdValid(ar_camera_node_pos[0]) && this->IsNodeIdValid(ar_camera_node_dir[0]))
    {
        Vector3 cur_dir = ar_nodes[ar_camera_node_pos[0]].RelPosition - ar_nodes[ar_camera_node_dir[0]].RelPosition;
        m_spawn_rotation = atan2(cur_dir.dotProduct(Vector3::UNIT_X), cur_dir.dotProduct(-Vector3::UNIT_Z));
    }
    else if (ar_num_nodes > 1)
    {
        float max_dist = 0.0f;
        int furthest_node = 1;
        for (int i = 0; i < ar_num_nodes; i++)
        {
            float dist = ar_nodes[i].RelPosition.squaredDistance(ar_nodes[0].RelPosition);
            if (dist > max_dist)
            {
                max_dist = dist;
                furthest_node = i;
            }
        }
        Vector3 cur_dir = ar_nodes[0].RelPosition - ar_nodes[furthest_node].RelPosition;
        m_spawn_rotation = atan2(cur_dir.dotProduct(Vector3::UNIT_X), cur_dir.dotProduct(-Vector3::UNIT_Z));
    }

    Vector3 cinecam = ar_nodes[0].AbsPosition;
    if (this->IsNodeIdValid(ar_camera_node_pos[0]))
    {
        cinecam = ar_nodes[ar_camera_node_pos[0]].AbsPosition;
    }

    // Calculate the approximate median
    std::vector<Real> mx(ar_num_nodes, 0.0f);
    std::vector<Real> my(ar_num_nodes, 0.0f);
    std::vector<Real> mz(ar_num_nodes, 0.0f);
    for (int i = 0; i < ar_num_nodes; i++)
    {
        mx[i] = ar_nodes[i].AbsPosition.x;
        my[i] = ar_nodes[i].AbsPosition.y;
        mz[i] = ar_nodes[i].AbsPosition.z;
    }
    std::nth_element(mx.begin(), mx.begin() + ar_num_nodes / 2, mx.end());
    std::nth_element(my.begin(), my.begin() + ar_num_nodes / 2, my.end());
    std::nth_element(mz.begin(), mz.begin() + ar_num_nodes / 2, mz.end());
    Vector3 median = Vector3(mx[ar_num_nodes / 2], my[ar_num_nodes / 2], mz[ar_num_nodes / 2]);

    // Calculate the average
    Vector3 sum = Vector3::ZERO;
    for (int i = 0; i < ar_num_nodes; i++)
    {
        sum += ar_nodes[i].AbsPosition;
    }
    Vector3 average = sum / ar_num_nodes;

    // Decide whether or not the cinecam node is an appropriate rotation center
    m_is_cinecam_rotation_center = cinecam.squaredDistance(median) < average.squaredDistance(median);

    TRIGGER_EVENT(SE_GENERIC_NEW_TRUCK, ar_instance_id);

    return true;
}

ground_model_t* Beam::getLastFuzzyGroundModel()
{
    return m_last_fuzzy_ground_model;
}

float Beam::getSteeringAngle()
{
    return ar_hydro_dir_command;
}

std::string Beam::getTruckName()
{
    return ar_design_name;
}

std::string Beam::getTruckFileName()
{
    return ar_filename;
}

int Beam::getTruckType()
{
    return ar_driveable;
}

std::vector<authorinfo_t> Beam::getAuthors()
{
    return authors;
}

std::vector<std::string> Beam::getDescription()
{
    return description;
}

int Beam::getBeamCount()
{
    return ar_num_beams;
}

int Beam::getNodeCount()
{
    return ar_num_nodes;
}

void Beam::setMass(float m)
{
    m_dry_mass = m;
}

bool Beam::getBrakeLightVisible()
{
    if (ar_sim_state == SimState::NETWORKED_OK)
        return m_net_brake_light;

    //		return (brake > 0.15 && !parkingbrake);
    return (ar_brake > 0.15);
}

bool Beam::getCustomLightVisible(int number)
{
    if (number < 0 || number > 4)
    {
        LOG("AngelScript: Invalid Light ID (" + TOSTRING(number) + "), allowed range is (0 - 4)");
        return false;
    }

    unsigned int flareID = m_net_custom_lights[number];

    return flareID < ar_flares.size() && ar_flares[flareID].controltoggle_status;
}

void Beam::setCustomLightVisible(int number, bool visible)
{
    if (number < 0 || number > 4)
    {
        LOG("AngelScript: Invalid Light ID (" + TOSTRING(number) + "), allowed range is (0 - 4)");
        return;
    }

    unsigned int flareID = m_net_custom_lights[number];

    if (flareID < ar_flares.size() && ar_flares[flareID].snode)
    {
        ar_flares[flareID].controltoggle_status = visible;
    }
}

bool Beam::getBeaconMode() // Angelscript export
{
    return m_beacon_light_is_active;
}

blinktype Beam::getBlinkType()
{
    return m_blink_type;
}

bool Beam::getCustomParticleMode()
{
    return m_custom_particles_enabled;
}

int Beam::getLowestNode()
{
    return ar_lowest_node;
}

Ogre::Real Beam::getMinimalCameraRadius()
{
    return m_min_camera_radius;
}

Replay* Beam::getReplay()
{
    return m_replay_handler;
}

bool Beam::getSlideNodesLockInstant()
{
    return m_slidenodes_connect_on_spawn;
}

bool Beam::inRange(float num, float min, float max)
{
    return (num <= max && num >= min);
}

Vector3 Beam::getNodePosition(int nodeNumber)
{
    if (nodeNumber >= 0 && nodeNumber < ar_num_nodes)
    {
        return ar_nodes[nodeNumber].AbsPosition;
    }
    else
    {
        return Ogre::Vector3();
    }
}

void Beam::UpdatePropAnimations(const float dt)
{
    BES_START(BES_CORE_AnimatedProps);

    for (int propi = 0; propi < ar_num_props; propi++)
    {
        int animnum = 0;
        float rx = 0.0f;
        float ry = 0.0f;
        float rz = 0.0f;

        while (ar_props[propi].animFlags[animnum])
        {
            float cstate = 0.0f;
            int div = 0.0f;
            int flagstate = ar_props[propi].animFlags[animnum];
            const float lower_limit = ar_props[propi].constraints[animnum].lower_limit;
            const float upper_limit = ar_props[propi].constraints[animnum].upper_limit;
            float animOpt3 = ar_props[propi].animOpt3[animnum];

            calcAnimators(flagstate, cstate, div, dt, lower_limit, upper_limit, animOpt3);

            // key triggered animations
            if ((ar_props[propi].animFlags[animnum] & ANIM_FLAG_EVENT) && ar_props[propi].animKey[animnum] != -1)
            {
                if (RoR::App::GetInputEngine()->getEventValue(ar_props[propi].animKey[animnum]))
                {
                    // keystatelock is disabled then set cstate
                    if (ar_props[propi].animKeyState[animnum] == -1.0f)
                    {
                        cstate += RoR::App::GetInputEngine()->getEventValue(ar_props[propi].animKey[animnum]);
                    }
                    else if (!ar_props[propi].animKeyState[animnum])
                    {
                        // a key was pressed and a toggle was done already, so bypass
                        //toggle now
                        if (!ar_props[propi].lastanimKS[animnum])
                        {
                            ar_props[propi].lastanimKS[animnum] = 1.0f;
                            // use animkey as bool to determine keypress / release state of inputengine
                            ar_props[propi].animKeyState[animnum] = 1.0f;
                        }
                        else
                        {
                            ar_props[propi].lastanimKS[animnum] = 0.0f;
                            // use animkey as bool to determine keypress / release state of inputengine
                            ar_props[propi].animKeyState[animnum] = 1.0f;
                        }
                    }
                    else
                    {
                        // bypas mode, get the last set position and set it
                        cstate += ar_props[propi].lastanimKS[animnum];
                    }
                }
                else
                {
                    // keyevent exists and keylock is enabled but the key isnt pressed right now = get lastanimkeystatus for cstate and reset keypressed bool animkey
                    if (ar_props[propi].animKeyState[animnum] != -1.0f)
                    {
                        cstate += ar_props[propi].lastanimKS[animnum];
                        ar_props[propi].animKeyState[animnum] = 0.0f;
                    }
                }
            }

            //propanimation placed here to avoid interference with existing hydros(cstate) and permanent prop animation
            //truck steering
            if (ar_props[propi].animFlags[animnum] & ANIM_FLAG_STEERING)
                cstate += ar_hydro_dir_state;
            //aileron
            if (ar_props[propi].animFlags[animnum] & ANIM_FLAG_AILERONS)
                cstate += ar_hydro_aileron_state;
            //elevator
            if (ar_props[propi].animFlags[animnum] & ANIM_FLAG_ELEVATORS)
                cstate += ar_hydro_elevator_state;
            //rudder
            if (ar_props[propi].animFlags[animnum] & ANIM_FLAG_ARUDDER)
                cstate += ar_hydro_rudder_state;
            //permanent
            if (ar_props[propi].animFlags[animnum] & ANIM_FLAG_PERMANENT)
                cstate += 1.0f;

            cstate *= ar_props[propi].animratio[animnum];

            // autoanimate noflip_bouncer
            if (ar_props[propi].animOpt5[animnum])
                cstate *= (ar_props[propi].animOpt5[animnum]);

            //rotate prop
            if ((ar_props[propi].animMode[animnum] & ANIM_MODE_ROTA_X) || (ar_props[propi].animMode[animnum] & ANIM_MODE_ROTA_Y) || (ar_props[propi].animMode[animnum] & ANIM_MODE_ROTA_Z))
            {
                float limiter = 0.0f;
                // This code was formerly executed within a fixed timestep of 0.5ms and finetuned accordingly.
                // This is now taken into account by factoring in the respective fraction of the variable timestep.
                float const dt_frac = dt * 2000.f;
                if (ar_props[propi].animMode[animnum] & ANIM_MODE_AUTOANIMATE)
                {
                    if (ar_props[propi].animMode[animnum] & ANIM_MODE_ROTA_X)
                    {
                        ar_props[propi].rotaX += cstate * dt_frac;
                        limiter = ar_props[propi].rotaX;
                    }
                    if (ar_props[propi].animMode[animnum] & ANIM_MODE_ROTA_Y)
                    {
                        ar_props[propi].rotaY += cstate * dt_frac;
                        limiter = ar_props[propi].rotaY;
                    }
                    if (ar_props[propi].animMode[animnum] & ANIM_MODE_ROTA_Z)
                    {
                        ar_props[propi].rotaZ += cstate * dt_frac;
                        limiter = ar_props[propi].rotaZ;
                    }
                }
                else
                {
                    if (ar_props[propi].animMode[animnum] & ANIM_MODE_ROTA_X)
                        rx += cstate;
                    if (ar_props[propi].animMode[animnum] & ANIM_MODE_ROTA_Y)
                        ry += cstate;
                    if (ar_props[propi].animMode[animnum] & ANIM_MODE_ROTA_Z)
                        rz += cstate;
                }

                bool limiterchanged = false;
                // check if a positive custom limit is set to evaluate/calc flip back

                if (limiter > upper_limit)
                {
                    if (ar_props[propi].animMode[animnum] & ANIM_MODE_NOFLIP)
                    {
                        limiter = upper_limit; // stop at limit
                        ar_props[propi].animOpt5[animnum] *= -1.0f; // change cstate multiplier if bounce is set
                    }
                    else
                    {
                        limiter = lower_limit; // flip to other side at limit
                    }
                    limiterchanged = true;
                }

                if (limiter < lower_limit)
                {
                    if (ar_props[propi].animMode[animnum] & ANIM_MODE_NOFLIP)
                    {
                        limiter = lower_limit; // stop at limit
                        ar_props[propi].animOpt5[animnum] *= -1.0f; // change cstate multiplier if active
                    }
                    else
                    {
                        limiter = upper_limit; // flip to other side at limit
                    }
                    limiterchanged = true;
                }

                if (limiterchanged)
                {
                    if (ar_props[propi].animMode[animnum] & ANIM_MODE_ROTA_X)
                        ar_props[propi].rotaX = limiter;
                    if (ar_props[propi].animMode[animnum] & ANIM_MODE_ROTA_Y)
                        ar_props[propi].rotaY = limiter;
                    if (ar_props[propi].animMode[animnum] & ANIM_MODE_ROTA_Z)
                        ar_props[propi].rotaZ = limiter;
                }
            }

            //offset prop

            if ((ar_props[propi].animMode[animnum] & ANIM_MODE_OFFSET_X) || (ar_props[propi].animMode[animnum] & ANIM_MODE_OFFSET_Y) || (ar_props[propi].animMode[animnum] & ANIM_MODE_OFFSET_Z))
            {
                float offset = 0.0f;
                float autooffset = 0.0f;

                if (ar_props[propi].animMode[animnum] & ANIM_MODE_OFFSET_X)
                    offset = ar_props[propi].orgoffsetX;
                if (ar_props[propi].animMode[animnum] & ANIM_MODE_OFFSET_Y)
                    offset = ar_props[propi].orgoffsetY;
                if (ar_props[propi].animMode[animnum] & ANIM_MODE_OFFSET_Z)
                    offset = ar_props[propi].orgoffsetZ;

                if (ar_props[propi].animMode[animnum] & ANIM_MODE_AUTOANIMATE)
                {
                    // This code was formerly executed within a fixed timestep of 0.5ms and finetuned accordingly.
                    // This is now taken into account by factoring in the respective fraction of the variable timestep.
                    float const dt_frac = dt * 2000.f;
                    autooffset = offset + cstate * dt_frac;

                    if (autooffset > upper_limit)
                    {
                        if (ar_props[propi].animMode[animnum] & ANIM_MODE_NOFLIP)
                        {
                            autooffset = upper_limit; // stop at limit
                            ar_props[propi].animOpt5[animnum] *= -1.0f; // change cstate multiplier if active
                        }
                        else
                        {
                            autooffset = lower_limit; // flip to other side at limit
                        }
                    }

                    if (autooffset < lower_limit)
                    {
                        if (ar_props[propi].animMode[animnum] & ANIM_MODE_NOFLIP)
                        {
                            autooffset = lower_limit; // stop at limit
                            ar_props[propi].animOpt5[animnum] *= -1.0f; // change cstate multiplier if active
                        }
                        else
                        {
                            autooffset = upper_limit; // flip to other side at limit
                        }
                    }
                }
                offset += cstate;

                if (ar_props[propi].animMode[animnum] & ANIM_MODE_OFFSET_X)
                {
                    ar_props[propi].offsetx = offset;
                    if (ar_props[propi].animMode[animnum] & ANIM_MODE_AUTOANIMATE)
                        ar_props[propi].orgoffsetX = autooffset;
                }
                if (ar_props[propi].animMode[animnum] & ANIM_MODE_OFFSET_Y)
                {
                    ar_props[propi].offsety = offset;
                    if (ar_props[propi].animMode[animnum] & ANIM_MODE_AUTOANIMATE)
                        ar_props[propi].orgoffsetY = autooffset;
                }
                if (ar_props[propi].animMode[animnum] & ANIM_MODE_OFFSET_Z)
                {
                    ar_props[propi].offsetz = offset;
                    if (ar_props[propi].animMode[animnum] & ANIM_MODE_AUTOANIMATE)
                        ar_props[propi].orgoffsetZ = autooffset;
                }
            }
            animnum++;
        }
        //recalc the quaternions with final stacked rotation values ( rx, ry, rz )
        rx += ar_props[propi].rotaX;
        ry += ar_props[propi].rotaY;
        rz += ar_props[propi].rotaZ;
        ar_props[propi].rot = Quaternion(Degree(rz), Vector3::UNIT_Z) * Quaternion(Degree(ry), Vector3::UNIT_Y) * Quaternion(Degree(rx), Vector3::UNIT_X);
    }

    BES_STOP(BES_CORE_AnimatedProps);
}
