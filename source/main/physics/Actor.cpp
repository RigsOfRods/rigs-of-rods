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

#include "Actor.h"

#include "AirBrake.h"
#include "Airfoil.h"
#include "Application.h"
#include "AutoPilot.h"
#include "SimData.h"
#include "ActorManager.h"
#include "Buoyance.h"
#include "CacheSystem.h"
#include "ChatSystem.h"
#include "CmdKeyInertia.h"
#include "Collisions.h"
#include "DashBoardManager.h"
#include "Differentials.h"
#include "DynamicCollisions.h"
#include "EngineSim.h"
#include "ErrorUtils.h"
#include "FlexAirfoil.h"
#include "FlexBody.h"
#include "FlexMesh.h"
#include "FlexMeshWheel.h"
#include "FlexObj.h"
#include "GameContext.h"
#include "GfxScene.h"
#include "GUIManager.h"
#include "Console.h"
#include "GfxActor.h"
#include "InputEngine.h"
#include "Language.h"
#include "MeshObject.h"
#include "MovableText.h"
#include "Network.h"
#include "PointColDetector.h"
#include "Replay.h"
#include "ActorSpawner.h"
#include "RoRnet.h"
#include "ScrewProp.h"
#include "ScriptEngine.h"
#include "Skidmark.h"
#include "SlideNode.h"
#include "SoundScriptManager.h"
#include "TerrainManager.h"
#include "TurboJet.h"
#include "TurboProp.h"
#include "Utils.h"
#include "VehicleAI.h"
#include "Water.h"

#include <sstream>
#include <iomanip>

using namespace Ogre;
using namespace RoR;

static const Ogre::Vector3 BOUNDING_BOX_PADDING(0.05f, 0.05f, 0.05f);

Actor::~Actor()
{
    TRIGGER_EVENT(SE_GENERIC_DELETED_TRUCK, ar_instance_id);

    // TODO: IMPROVE below: delete/destroy prop entities, etc

    this->DisjoinInterActorBeams();

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
            App::GetGfxScene()->GetSceneManager()->destroySceneNode(m_deletion_scene_nodes[i]);
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
            App::GetGfxScene()->GetSceneManager()->destroyEntity(m_deletion_entities[i]->getName());
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
            App::GetGfxScene()->GetSceneManager()->destroySceneNode(ar_wings[i].cnode);
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
    for (Airbrake* ab: ar_airbrakes)
    {
        delete ab;
    }
    ar_airbrakes.clear();

    // delete skidmarks
    for (int i = 0; i < ar_num_wheels; ++i)
    {
        delete m_skid_trails[i];
        m_skid_trails[i] = nullptr;
    }

    // delete flares
    for (size_t i = 0; i < this->ar_flares.size(); i++)
    {
        if (ar_flares[i].snode)
        {
            ar_flares[i].snode->removeAndDestroyAllChildren();
            App::GetGfxScene()->GetSceneManager()->destroySceneNode(ar_flares[i].snode);
        }
        if (ar_flares[i].bbs)
            App::GetGfxScene()->GetSceneManager()->destroyBillboardSet(ar_flares[i].bbs);
        if (ar_flares[i].light)
            App::GetGfxScene()->GetSceneManager()->destroyLight(ar_flares[i].light);
    }
    this->ar_flares.clear();

    // delete exhausts
    for (std::vector<Exhaust>::iterator it = exhausts.begin(); it != exhausts.end(); it++)
    {
        if (it->smokeNode)
        {
            it->smokeNode->removeAndDestroyAllChildren();
            App::GetGfxScene()->GetSceneManager()->destroySceneNode(it->smokeNode);
        }
        if (it->smoker)
        {
            it->smoker->removeAllAffectors();
            it->smoker->removeAllEmitters();
            App::GetGfxScene()->GetSceneManager()->destroyParticleSystem(it->smoker);
        }
    }

    // delete ar_custom_particles
    for (int i = 0; i < ar_num_custom_particles; i++)
    {
        if (ar_custom_particles[i].snode)
        {
            ar_custom_particles[i].snode->removeAndDestroyAllChildren();
            App::GetGfxScene()->GetSceneManager()->destroySceneNode(ar_custom_particles[i].snode);
        }
        if (ar_custom_particles[i].psys)
        {
            ar_custom_particles[i].psys->removeAllAffectors();
            ar_custom_particles[i].psys->removeAllEmitters();
            App::GetGfxScene()->GetSceneManager()->destroyParticleSystem(ar_custom_particles[i].psys);
        }
    }

    // delete Rails
    for (std::vector<RailGroup*>::iterator it = m_railgroups.begin(); it != m_railgroups.end(); it++)
    {
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

    if (m_transfer_case)
        delete m_transfer_case;

    for (int i = 0; i < m_num_axle_diffs; ++i)
    {
        if (m_axle_diffs[i] != nullptr)
            delete m_axle_diffs[i];
    }

    for (int i = 0; i < m_num_wheel_diffs; ++i)
    {
        if (m_wheel_diffs[i] != nullptr)
            delete m_wheel_diffs[i];
    }

    delete ar_nodes;
    delete ar_beams;
    delete ar_shocks;
    delete ar_rotators;
    delete ar_wings;
}

// This method scales actors. Stresses should *NOT* be scaled, they describe
// the material type and they do not depend on length or scale.
void Actor::ScaleActor(float value)
{
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
    }
    // scale hydros
    for (hydrobeam_t& hbeam: ar_hydros)
    {
        hbeam.hb_ref_length *= value;
        hbeam.hb_speed *= value;
    }
    // scale nodes
    Vector3 refpos = ar_nodes[0].AbsPosition;
    Vector3 relpos = ar_nodes[0].RelPosition;
    for (int i = 1; i < ar_num_nodes; i++)
    {
        ar_initial_node_positions[i] = refpos + (ar_initial_node_positions[i] - refpos) * value;
        ar_nodes[i].AbsPosition = refpos + (ar_nodes[i].AbsPosition - refpos) * value;
        ar_nodes[i].RelPosition = relpos + (ar_nodes[i].RelPosition - relpos) * value;
        ar_nodes[i].Velocity *= value;
        ar_nodes[i].Forces *= value;
        ar_nodes[i].mass *= value;
    }
    updateSlideNodePositions();

    m_gfx_actor->ScaleActor(relpos, value);

}

float Actor::getRotation()
{
    Vector3 dir = getDirection();

    return atan2(dir.dotProduct(Vector3::UNIT_X), dir.dotProduct(-Vector3::UNIT_Z));
}

Vector3 Actor::getDirection()
{
    return ar_main_camera_dir_corr * this->GetCameraDir();
}

Vector3 Actor::getPosition()
{
    return m_avg_node_position; //the position is already in absolute position
}

void Actor::PushNetwork(char* data, int size)
{
#if USE_SOCKETW
    NetUpdate update;

    update.veh_state.resize(sizeof(RoRnet::VehicleState));
    update.node_data.resize(m_net_node_buf_size);
    update.wheel_data.resize(ar_num_wheels * sizeof(float));

    // check if the size of the data matches to what we expected
    if ((unsigned int)size == (m_net_buffer_size + sizeof(RoRnet::VehicleState)))
    {
        // we walk through the incoming data and separate it a bit
        char* ptr = data;

        // put the RoRnet::VehicleState in front, describes actor basics, engine state, flares, etc
        memcpy(update.veh_state.data(), ptr, sizeof(RoRnet::VehicleState));
        ptr += sizeof(RoRnet::VehicleState);

        // then copy the node data
        memcpy(update.node_data.data(), ptr, m_net_node_buf_size);
        ptr += m_net_node_buf_size;

        // then take care of the wheel speeds
        for (int i = 0; i < ar_num_wheels; i++)
        {
            float wspeed = *(float*)(ptr);
            update.wheel_data[i] = wspeed;
            ptr += sizeof(float);
        }
    }
    else
    {
        if (!m_net_initialized)
        {
            // Update stream status (remote and local)
            RoRnet::StreamRegister reg;
            memset(&reg, 0, sizeof(RoRnet::StreamRegister));
            reg.status = -2;
            reg.origin_sourceid = ar_net_source_id;
            reg.origin_streamid = ar_net_stream_id;
            strncpy(reg.name, ar_filename.c_str(), 128);
            App::GetNetwork()->AddPacket(reg.origin_streamid, RoRnet::MSG2_STREAM_REGISTER_RESULT,
                    sizeof(RoRnet::StreamRegister), (char *)&reg);
            App::GetGameContext()->GetActorManager()->AddStreamMismatch(ar_net_source_id, ar_net_stream_id);

            // Inform the local player
            RoRnet::UserInfo info;
            App::GetNetwork()->GetUserInfo(reg.origin_sourceid, info);
            Str<400> text;
            text << info.username << _L(" content mismatch: ") << reg.name;
            App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_WARNING, text.ToCStr());

            // Remove self
            App::GetGameContext()->PushMessage(Message(MSG_SIM_DELETE_ACTOR_REQUESTED, (void*)this));

            m_net_initialized = true;
        }
        RoR::LogFormat("[RoR|Network] Stream mismatch, filename: '%s'", ar_filename.c_str());
        return;
    }

    // Required to catch up when joining late (since the StreamRegister time stamp is received delayed)
    if (!m_net_initialized)
    {
        RoRnet::VehicleState* oob = (RoRnet::VehicleState*)update.veh_state.data();
        int tnow = App::GetGameContext()->GetActorManager()->GetNetTime();
        int rnow = std::max(0, tnow + App::GetGameContext()->GetActorManager()->GetNetTimeOffset(ar_net_source_id));
        if (oob->time > rnow + 100)
        {
            App::GetGameContext()->GetActorManager()->UpdateNetTimeOffset(ar_net_source_id, oob->time - rnow);
        }
    }

    m_net_updates.push_back(update);
#endif // USE_SOCKETW
}

void Actor::CalcNetwork()
{
    using namespace RoRnet;

    if (m_net_updates.size() < 2)
        return;

    int tnow = App::GetGameContext()->GetActorManager()->GetNetTime();
    int rnow = std::max(0, tnow + App::GetGameContext()->GetActorManager()->GetNetTimeOffset(ar_net_source_id));

    // Find index offset into the stream data for the current time
    int index_offset = 0;
    for (int i = 0; i < m_net_updates.size() - 1; i++)
    {
        VehicleState* oob = (VehicleState*)m_net_updates[i].veh_state.data();
        if (oob->time > rnow)
            break;
        index_offset = i;
    }

    VehicleState* oob1 = (VehicleState*)m_net_updates[index_offset    ].veh_state.data();
    VehicleState* oob2 = (VehicleState*)m_net_updates[index_offset + 1].veh_state.data();
    char*        netb1 = (char*)        m_net_updates[index_offset    ].node_data.data();
    char*        netb2 = (char*)        m_net_updates[index_offset + 1].node_data.data();
    float*     net_rp1 = (float*)       m_net_updates[index_offset    ].wheel_data.data();
    float*     net_rp2 = (float*)       m_net_updates[index_offset + 1].wheel_data.data();

    float tratio = (float)(rnow - oob1->time) / (float)(oob2->time - oob1->time);

    if (tratio > 4.0f)
    {
        m_net_updates.clear();
        return; // Wait for new data
    }
    else if (tratio > 1.0f)
    {
        App::GetGameContext()->GetActorManager()->UpdateNetTimeOffset(ar_net_source_id, -std::pow(2, tratio));
    }
    else if (index_offset == 0 && (m_net_updates.size() > 5 || (tratio < 0.125f && m_net_updates.size() > 2)))
    {
        App::GetGameContext()->GetActorManager()->UpdateNetTimeOffset(ar_net_source_id, +1);
    }

    short* sp1 = (short*)(netb1 + sizeof(float) * 3);
    short* sp2 = (short*)(netb2 + sizeof(float) * 3);
    Vector3 p1ref = Vector3::ZERO;
    Vector3 p2ref = Vector3::ZERO;
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
            p1.x = (float)(sp1[(i - 1) * 3 + 0]) / m_net_node_compression;
            p1.y = (float)(sp1[(i - 1) * 3 + 1]) / m_net_node_compression;
            p1.z = (float)(sp1[(i - 1) * 3 + 2]) / m_net_node_compression;
            p1 = p1 + p1ref;

            p2.x = (float)(sp2[(i - 1) * 3 + 0]) / m_net_node_compression;
            p2.y = (float)(sp2[(i - 1) * 3 + 1]) / m_net_node_compression;
            p2.z = (float)(sp2[(i - 1) * 3 + 2]) / m_net_node_compression;
            p2 = p2 + p2ref;
        }

        // linear interpolation
        ar_nodes[i].AbsPosition = p1 + tratio * (p2 - p1);
        ar_nodes[i].RelPosition = ar_nodes[i].AbsPosition - ar_origin;
        ar_nodes[i].Velocity    = (p2 - p1) * 1000.0f / (float)(oob2->time - oob1->time);
    }

    for (int i = 0; i < ar_num_wheels; i++)
    {
        float rp = net_rp1[i] + tratio * (net_rp2[i] - net_rp1[i]);
        //compute ideal positions
        Vector3 axis = ar_wheels[i].wh_axis_node_1->RelPosition - ar_wheels[i].wh_axis_node_0->RelPosition;
        axis.normalise();
        Plane pplan = Plane(axis, ar_wheels[i].wh_axis_node_0->AbsPosition);
        Vector3 ortho = -pplan.projectVector(ar_wheels[i].wh_near_attach_node->AbsPosition) - ar_wheels[i].wh_axis_node_0->AbsPosition;
        Vector3 ray = ortho.crossProduct(axis);
        ray.normalise();
        ray *= ar_wheels[i].wh_radius;
        float drp = Math::TWO_PI / (ar_wheels[i].wh_num_nodes / 2);
        for (int j = 0; j < ar_wheels[i].wh_num_nodes / 2; j++)
        {
            Vector3 uray = Quaternion(Radian(rp - drp * j), axis) * ray;

            ar_wheels[i].wh_nodes[j * 2 + 0]->AbsPosition = ar_wheels[i].wh_axis_node_0->AbsPosition + uray;
            ar_wheels[i].wh_nodes[j * 2 + 0]->RelPosition = ar_wheels[i].wh_nodes[j * 2]->AbsPosition - ar_origin;

            ar_wheels[i].wh_nodes[j * 2 + 1]->AbsPosition = ar_wheels[i].wh_axis_node_1->AbsPosition + uray;
            ar_wheels[i].wh_nodes[j * 2 + 1]->RelPosition = ar_wheels[i].wh_nodes[j * 2 + 1]->AbsPosition - ar_origin;
        }
        ray.normalise();
        ray *= ar_wheels[i].wh_rim_radius;
        for (int j = 0; j < ar_wheels[i].wh_num_rim_nodes / 2; j++)
        {
            Vector3 uray = Quaternion(Radian(rp - drp * j), axis) * ray;

            ar_wheels[i].wh_rim_nodes[j * 2 + 0]->AbsPosition = ar_wheels[i].wh_axis_node_0->AbsPosition + uray;
            ar_wheels[i].wh_rim_nodes[j * 2 + 0]->RelPosition = ar_wheels[i].wh_rim_nodes[j * 2]->AbsPosition - ar_origin;

            ar_wheels[i].wh_rim_nodes[j * 2 + 1]->AbsPosition = ar_wheels[i].wh_axis_node_1->AbsPosition + uray;
            ar_wheels[i].wh_rim_nodes[j * 2 + 1]->RelPosition = ar_wheels[i].wh_rim_nodes[j * 2 + 1]->AbsPosition - ar_origin;
        }
    }
    this->UpdateBoundingBoxes();
    this->calculateAveragePosition();

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
        SOUND_MODULATE(ar_instance_id, SS_MOD_INJECTOR, engforce);
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

        ar_engine->PushNetworkState(engspeed, engforce, engclutch, gear, running, contact, automode);
    }

    // set particle cannon
    if (((flagmask & NETMASK_PARTICLE) != 0) != m_custom_particles_enabled)
        ToggleCustomParticles();

    // set lights
    if (((flagmask & NETMASK_LIGHTS) != 0) != ar_lights)
        ToggleLights();
    if (((flagmask & NETMASK_BEACONS) != 0) != m_beacon_light_is_active)
        ToggleBeacons();

    m_antilockbrake = flagmask & NETMASK_ALB_ACTIVE;
    m_tractioncontrol = flagmask & NETMASK_TC_ACTIVE;
    ar_parking_brake = flagmask & NETMASK_PBRAKE;

    BlinkType btype = BLINK_NONE;
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

    if (m_net_reverse_light && ar_engine && ar_engine->IsRunning())
        SOUND_START(ar_instance_id, SS_TRIG_REVERSE_GEAR);
    else
        SOUND_STOP(ar_instance_id, SS_TRIG_REVERSE_GEAR);

    for (int i = 0; i < index_offset; i++)
    {
        m_net_updates.pop_front();
    }

    m_net_initialized = true;
}

void Actor::RecalculateNodeMasses(Real total)
{
    //reset
    for (int i = 0; i < ar_num_nodes; i++)
    {
        if (!ar_nodes[i].nd_tyre_node)
        {
            if (!ar_nodes[i].nd_loaded_mass)
            {
                ar_nodes[i].mass = 0;
            }
            else if (!ar_nodes[i].nd_override_mass)
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
            if (!ar_beams[i].p1->nd_tyre_node)
                len += half_newlen;
            if (!ar_beams[i].p2->nd_tyre_node)
                len += half_newlen;
        }
    }

    for (int i = 0; i < ar_num_beams; i++)
    {
        if (ar_beams[i].bm_type != BEAM_VIRTUAL)
        {
            Real half_mass = ar_beams[i].L * total / len / 2.0f;
            if (!ar_beams[i].p1->nd_tyre_node)
                ar_beams[i].p1->mass += half_mass;
            if (!ar_beams[i].p2->nd_tyre_node)
                ar_beams[i].p2->mass += half_mass;
        }
    }
    //fix rope masses
    for (std::vector<rope_t>::iterator it = ar_ropes.begin(); it != ar_ropes.end(); it++)
    {
        it->rp_beam->p2->mass = 100.0f;
    }

    // Apply pre-defined cinecam node mass
    for (int i = 0; i < this->ar_num_cinecams; ++i)
    {
        ar_nodes[ar_cinecam_node[i]].mass = ar_cinecam_node_predef_mass[i];
    }

    //update mass
    for (int i = 0; i < ar_num_nodes; i++)
    {
        if (!ar_nodes[i].nd_tyre_node &&
            !(ar_minimass_skip_loaded_nodes && ar_nodes[i].nd_loaded_mass) &&
            ar_nodes[i].mass < ar_minimass[i])
        {
            if (App::diag_truck_mass->GetBool())
            {
                char buf[300];
                snprintf(buf, 300, "Node '%d' mass (%f Kg) is too light. Resetting to 'minimass' (%f Kg)", i, ar_nodes[i].mass, ar_minimass[i]);
                LOG(buf);
            }
            ar_nodes[i].mass = ar_minimass[i];
        }
    }

    m_total_mass = 0;
    for (int i = 0; i < ar_num_nodes; i++)
    {
        if (App::diag_truck_mass->GetBool())
        {
            String msg = "Node " + TOSTRING(i) + " : " + TOSTRING((int)ar_nodes[i].mass) + " kg";
            if (ar_nodes[i].nd_loaded_mass)
            {
                if (ar_nodes[i].nd_override_mass)
                    msg += " (overriden by node mass)";
                else
                    msg += " (normal load node: " + TOSTRING(m_load_mass) + " kg / " + TOSTRING(m_masscount) + " nodes)";
            }
            LOG(msg);
        }
        m_total_mass += ar_nodes[i].mass;
    }
    LOG("TOTAL VEHICLE MASS: " + TOSTRING((int)m_total_mass) +" kg");
}

float Actor::getTotalMass(bool withLocked)
{
    if (!withLocked)
        return m_total_mass; // already computed in RecalculateNodeMasses

    float mass = m_total_mass;

    for (auto actor : m_linked_actors)
    {
        mass += actor->m_total_mass;
    }

    return mass;
}

void Actor::DetermineLinkedActors() //TODO: Refactor this - logic iterating over all actors should be in `ActorManager`! ~ only_a_ptr, 01/2018
{
    m_linked_actors.clear();

    bool found = true;
    std::map<Actor*, bool> lookup_table;
    std::pair<std::map<Actor*, bool>::iterator, bool> ret;

    lookup_table.insert(std::pair<Actor*, bool>(this, false));
    
    auto inter_actor_links = App::GetGameContext()->GetActorManager()->inter_actor_links; // TODO: Shouldn't this have been a reference?? Also, ugly, see the TODO note above ~ only_a_ptr, 01/2018

    while (found)
    {
        found = false;

        for (std::map<Actor*, bool>::iterator it_beam = lookup_table.begin(); it_beam != lookup_table.end(); ++it_beam)
        {
            if (!it_beam->second)
            {
                auto actor = it_beam->first;
                for (auto it = inter_actor_links.begin(); it != inter_actor_links.end(); it++)
                {
                    auto actor_pair = it->second;
                    if (actor == actor_pair.first || actor == actor_pair.second)
                    {
                        auto other_actor = (actor != actor_pair.first) ? actor_pair.first : actor_pair.second;
                        ret = lookup_table.insert(std::pair<Actor*, bool>(other_actor, false));
                        if (ret.second)
                        {
                            m_linked_actors.push_back(other_actor);
                            found = true;
                        }
                    }
                }
                it_beam->second = true;
            }
        }
    }
}

int Actor::getWheelNodeCount() const
{
    return m_wheel_node_count;
}

void Actor::calcNodeConnectivityGraph()
{
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
}

bool Actor::Intersects(Actor* actor, Vector3 offset)
{
    Vector3 bb_min = ar_bounding_box.getMinimum() + offset;
    Vector3 bb_max = ar_bounding_box.getMaximum() + offset;
    AxisAlignedBox bb = AxisAlignedBox(bb_min, bb_max);

    if (!bb.intersects(actor->ar_bounding_box))
        return false;

    // Test own (contactable) beams against others cabs
    for (int i = 0; i < ar_num_beams; i++)
    {
        if (!(ar_beams[i].p1->nd_contacter || ar_beams[i].p1->nd_contactable) ||
            !(ar_beams[i].p2->nd_contacter || ar_beams[i].p2->nd_contactable))
            continue;

        Vector3 origin = ar_beams[i].p1->AbsPosition + offset;
        Vector3 target = ar_beams[i].p2->AbsPosition + offset;

        Ray ray(origin, target - origin);

        for (int j = 0; j < actor->ar_num_collcabs; j++)
        {
            int index = actor->ar_collcabs[j] * 3;
            Vector3 a = actor->ar_nodes[actor->ar_cabs[index + 0]].AbsPosition;
            Vector3 b = actor->ar_nodes[actor->ar_cabs[index + 1]].AbsPosition;
            Vector3 c = actor->ar_nodes[actor->ar_cabs[index + 2]].AbsPosition;

            auto result = Ogre::Math::intersects(ray, a, b, c);
            if (result.first && result.second < 1.0f)
            {
                return true;
            }
        }
    }

    // Test own cabs against others (contactable) beams
    for (int i = 0; i < actor->ar_num_beams; i++)
    {
        if (!(actor->ar_beams[i].p1->nd_contacter || actor->ar_beams[i].p1->nd_contactable) ||
            !(actor->ar_beams[i].p2->nd_contacter || actor->ar_beams[i].p2->nd_contactable))
            continue;

        Vector3 origin = actor->ar_beams[i].p1->AbsPosition;
        Vector3 target = actor->ar_beams[i].p2->AbsPosition;

        Ray ray(origin, target - origin);

        for (int j = 0; j < ar_num_collcabs; j++)
        {
            int index = ar_collcabs[j] * 3;
            Vector3 a = ar_nodes[ar_cabs[index + 0]].AbsPosition + offset;
            Vector3 b = ar_nodes[ar_cabs[index + 1]].AbsPosition + offset;
            Vector3 c = ar_nodes[ar_cabs[index + 2]].AbsPosition + offset;

            auto result = Ogre::Math::intersects(ray, a, b, c);
            if (result.first && result.second < 1.0f)
            {
                return true;
            }
        }
    }

    return false;
}

Vector3 Actor::calculateCollisionOffset(Vector3 direction)
{
    if (direction == Vector3::ZERO)
        return Vector3::ZERO;

    Real max_distance = direction.normalise();

    // collision displacement
    Vector3 collision_offset = Vector3::ZERO;

    while (collision_offset.length() < max_distance)
    {
        Vector3 bb_min = ar_bounding_box.getMinimum() + collision_offset;
        Vector3 bb_max = ar_bounding_box.getMaximum() + collision_offset;
        AxisAlignedBox bb = AxisAlignedBox(bb_min, bb_max);

        bool collision = false;

        for (auto actor : App::GetGameContext()->GetActorManager()->GetActors())
        {
            if (actor == this)
                continue;
            if (!bb.intersects(actor->ar_bounding_box))
                continue;

            // Test own contactables against others cabs
            if (m_intra_point_col_detector)
            {
                for (int i = 0; i < actor->ar_num_collcabs; i++)
                {
                    int tmpv = actor->ar_collcabs[i] * 3;
                    node_t* no = &actor->ar_nodes[actor->ar_cabs[tmpv]];
                    node_t* na = &actor->ar_nodes[actor->ar_cabs[tmpv + 1]];
                    node_t* nb = &actor->ar_nodes[actor->ar_cabs[tmpv + 2]];

                    m_intra_point_col_detector->query(no->AbsPosition - collision_offset,
                        na->AbsPosition - collision_offset,
                        nb->AbsPosition - collision_offset,
                        actor->ar_collision_range * 3.0f);

                    if (collision = !m_intra_point_col_detector->hit_list.empty())
                        break;
                }

                if (collision)
                    break;
            }

            float proximity = std::max(.05f, std::sqrt(std::max(m_min_camera_radius, actor->m_min_camera_radius)) / 50.f);

            // Test proximity of own nodes against others nodes
            for (int i = 0; i < ar_num_nodes; i++)
            {
                if (!ar_nodes[i].nd_contacter && !ar_nodes[i].nd_contactable)
                    continue;

                Vector3 query_position = ar_nodes[i].AbsPosition + collision_offset;

                for (int j = 0; j < actor->ar_num_nodes; j++)
                {
                    if (!actor->ar_nodes[j].nd_contacter && !actor->ar_nodes[j].nd_contactable)
                        continue;

                    if (collision = query_position.squaredDistance(actor->ar_nodes[j].AbsPosition) < proximity)
                        break;
                }

                if (collision)
                    break;
            }

            if (collision)
                break;
        }

        // Test own cabs against others contacters
        if (!collision && m_inter_point_col_detector)
        {
            for (int i = 0; i < ar_num_collcabs; i++)
            {
                int tmpv = ar_collcabs[i] * 3;
                node_t* no = &ar_nodes[ar_cabs[tmpv]];
                node_t* na = &ar_nodes[ar_cabs[tmpv + 1]];
                node_t* nb = &ar_nodes[ar_cabs[tmpv + 2]];

                m_inter_point_col_detector->query(no->AbsPosition + collision_offset,
                    na->AbsPosition + collision_offset,
                    nb->AbsPosition + collision_offset,
                    ar_collision_range * 3.0f);

                if (collision = !m_inter_point_col_detector->hit_list.empty())
                    break;
            }
        }

        // Test beams (between contactable nodes) against cabs
        if (!collision)
        {
            for (auto actor : App::GetGameContext()->GetActorManager()->GetActors())
            {
                if (actor == this)
                    continue;
                if (collision = this->Intersects(actor, collision_offset))
                    break;
            }
        }

        if (!collision)
            break;

        collision_offset += direction * 0.05f;
    }

    return collision_offset;
}

void Actor::resolveCollisions(Vector3 direction)
{
    if (m_intra_point_col_detector)
        m_intra_point_col_detector->UpdateIntraPoint(true);

    if (m_inter_point_col_detector)
        m_inter_point_col_detector->UpdateInterPoint(true);

    Vector3 offset = calculateCollisionOffset(direction);

    if (offset == Vector3::ZERO)
        return;

    // Additional 20 cm safe-guard (horizontally)
    offset += 0.2f * Vector3(offset.x, 0.0f, offset.z).normalisedCopy();

    ResetPosition(ar_nodes[0].AbsPosition.x + offset.x, ar_nodes[0].AbsPosition.z + offset.z, false, this->GetMinHeight() + offset.y);
}

void Actor::resolveCollisions(float max_distance, bool consider_up)
{
    if (m_intra_point_col_detector)
        m_intra_point_col_detector->UpdateIntraPoint(true);

    if (m_inter_point_col_detector)
        m_inter_point_col_detector->UpdateInterPoint(true);

    Vector3 u = Vector3::UNIT_Y;
    Vector3 f = Vector3(getDirection().x, 0.0f, getDirection().z).normalisedCopy();
    Vector3 l = u.crossProduct(f);

    // Calculate an ideal collision avoidance direction (prefer left over right over [front / back / up])
    Vector3 left  = calculateCollisionOffset(+l * max_distance);
    Vector3 right = calculateCollisionOffset(-l * left.length());
    Vector3 lateral = left.length() < right.length() * 1.1f ? left : right;

    Vector3 front = calculateCollisionOffset(+f * lateral.length());
    Vector3 back  = calculateCollisionOffset(-f * front.length());
    Vector3 sagittal = front.length() < back.length() * 1.1f ? front : back;

    Vector3 offset = lateral.length() < sagittal.length() * 1.2f ? lateral : sagittal;

    if (consider_up)
    {
        Vector3 up = calculateCollisionOffset(+u * offset.length());
        if (up.length() * 1.2f < offset.length())
            offset = up;
    }

    if (offset == Vector3::ZERO)
        return;

    // Additional 20 cm safe-guard (horizontally)
    offset += 0.2f * Vector3(offset.x, 0.0f, offset.z).normalisedCopy();

    ResetPosition(ar_nodes[0].AbsPosition.x + offset.x, ar_nodes[0].AbsPosition.z + offset.z, true, this->GetMinHeight() + offset.y);
}

void Actor::calculateAveragePosition()
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
    else if (ar_extern_camera_mode == 2 && ar_extern_camera_node != node_t::INVALID_IDX)
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

inline void PadBoundingBox(Ogre::AxisAlignedBox& box) // Internal helper
{
    box.setMinimum(box.getMinimum() - BOUNDING_BOX_PADDING);
    box.setMaximum(box.getMaximum() + BOUNDING_BOX_PADDING);
}

void Actor::UpdateBoundingBoxes()
{
    // Reset
    ar_bounding_box = AxisAlignedBox::BOX_NULL;
    ar_predicted_bounding_box = AxisAlignedBox::BOX_NULL;
    for (size_t i = 0; i < ar_collision_bounding_boxes.size(); ++i)
    {
        ar_collision_bounding_boxes[i] = AxisAlignedBox::BOX_NULL;
        ar_predicted_coll_bounding_boxes[i] = AxisAlignedBox::BOX_NULL;
    }

    // Update
    for (int i = 0; i < ar_num_nodes; i++)
    {
        Vector3 vel = ar_nodes[i].Velocity;
        Vector3 pos = ar_nodes[i].AbsPosition;
        int16_t cid = ar_nodes[i].nd_coll_bbox_id;

        ar_bounding_box.merge(pos);                                  // Current box
        ar_predicted_bounding_box.merge(pos);                        // Predicted box (current position)
        ar_predicted_bounding_box.merge(pos + vel);                  // Predicted box (future position)
        if (cid != node_t::INVALID_BBOX)
        {
            ar_collision_bounding_boxes[cid].merge(pos);
            ar_predicted_coll_bounding_boxes[cid].merge(pos);
            ar_predicted_coll_bounding_boxes[cid].merge(pos + vel);
        }
    }

    // Finalize - add padding
    PadBoundingBox(ar_bounding_box);
    PadBoundingBox(ar_predicted_bounding_box);
    for (size_t i = 0; i < ar_collision_bounding_boxes.size(); ++i)
    {
        PadBoundingBox(ar_collision_bounding_boxes[i]);
        PadBoundingBox(ar_predicted_coll_bounding_boxes[i]);
    }
}

void Actor::UpdatePhysicsOrigin()
{
    if (ar_nodes[0].RelPosition.squaredLength() > 10000.0)
    {
        Vector3 offset = ar_nodes[0].RelPosition;
        ar_origin += offset;
        for (int i = 0; i < ar_num_nodes; i++)
        {
            ar_nodes[i].RelPosition -= offset;
        }
    }
}

void Actor::ResetAngle(float rot)
{
    // Set origin of rotation to camera node
    Vector3 origin = ar_nodes[ar_main_camera_node_pos].AbsPosition;

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

    this->UpdateBoundingBoxes();
    calculateAveragePosition();
}

void Actor::UpdateInitPosition()
{
    for (int i = 0; i < ar_num_nodes; i++)
    {
        ar_initial_node_positions[i] = ar_nodes[i].AbsPosition;
    }
}

void Actor::ResetPosition(float px, float pz, bool setInitPosition, float miny)
{
    // horizontal displacement
    Vector3 offset = Vector3(px, ar_nodes[0].AbsPosition.y, pz) - ar_nodes[0].AbsPosition;
    for (int i = 0; i < ar_num_nodes; i++)
    {
        ar_nodes[i].AbsPosition += offset;
        ar_nodes[i].RelPosition = ar_nodes[i].AbsPosition - ar_origin;
    }

    // vertical displacement
    float vertical_offset = miny - this->GetMinHeight();
    if (App::GetSimTerrain()->getWater())
    {
        vertical_offset += std::max(0.0f, App::GetSimTerrain()->getWater()->GetStaticWaterHeight() - miny);
    }
    for (int i = 1; i < ar_num_nodes; i++)
    {
        if (ar_nodes[i].nd_no_ground_contact)
            continue;
        float terrainHeight = App::GetSimTerrain()->GetHeightAt(ar_nodes[i].AbsPosition.x, ar_nodes[i].AbsPosition.z);
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
        if (ar_nodes[i].nd_no_ground_contact)
            continue;
        float offset = mesh_offset;
        while (offset < 1.0f)
        {
            Vector3 query = ar_nodes[i].AbsPosition + Vector3(0.0f, offset, 0.0f);
            if (!App::GetSimTerrain()->GetCollisions()->collisionCorrect(&query, false))
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

    ResetPosition(Vector3::ZERO, setInitPosition);
}

void Actor::ResetPosition(Vector3 translation, bool setInitPosition)
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
            ar_initial_node_positions[i] = ar_nodes[i].AbsPosition;
        }
    }

    this->UpdateBoundingBoxes();
    calculateAveragePosition();
}

void Actor::HandleMouseMove(int node, Vector3 pos, float force)
{
    m_mouse_grab_node = node;
    m_mouse_grab_move_force = force * std::pow(m_total_mass / 3000.0f, 0.75f);
    m_mouse_grab_pos = pos;
}

void Actor::ToggleWheelDiffMode()
{
    for (int i = 0; i < m_num_wheel_diffs; ++i)
    {
        m_wheel_diffs[i]->ToggleDifferentialMode();
    }
}

void Actor::ToggleAxleDiffMode()
{
    for (int i = 0; i < m_num_axle_diffs; ++i)
    {
        m_axle_diffs[i]->ToggleDifferentialMode();
    }
}

void Actor::DisplayAxleDiffMode()
{
    if (m_num_axle_diffs == 0)
    {
        App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE,
                _L("No inter-axle differential installed on current vehicle!"), "warning.png", 3000);
    }
    else
    {
        String message = "";
        for (int i = 0; i < m_num_axle_diffs; ++i)
        {
            if (m_axle_diffs[i])
            {
                if (i > 0)
                    message += "\n";

                int a1 = m_axle_diffs[i]->di_idx_1 + 1;
                int a2 = m_axle_diffs[i]->di_idx_2 + 1;
                message += _L("Axle ") + TOSTRING(a1) + " <--> " + _L("Axle ") + TOSTRING(a2) + ": ";
                message += m_axle_diffs[i]->GetDifferentialTypeName();
            }
        }
        App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE,
                "Inter-axle differentials:\n" + message, "cog.png", 3000);
    }
}

void Actor::DisplayWheelDiffMode()
{
    if (m_num_wheel_diffs == 0)
    {
        App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE,
                _L("No inter-wheel differential installed on current vehicle!"), "warning.png", 3000);
    }
    else
    {
        String message = "";
        for (int i = 0; i < m_num_wheel_diffs; ++i)
        {
            if (m_wheel_diffs[i])
            {
                if (i > 0)
                    message += "\n";

                message += _L("Axle ") + TOSTRING(i + 1) + ": ";
                message += m_wheel_diffs[i]->GetDifferentialTypeName();
            }
        }
        App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE,
                "Inter-wheel differentials:\n" + message, "cog.png", 3000);
    }
}

void Actor::DisplayTransferCaseMode()
{
    if (m_transfer_case)
    {
        App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE,
                _L("Transfercase switched to: ") + this->GetTransferCaseName(), "cog.png", 3000);
    }
    else
    {
        App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE,
                _L("No transfercase installed on current vehicle!"), "warning.png", 3000);
    }
}

void Actor::ToggleTransferCaseMode()
{
    if (!ar_engine || !m_transfer_case || m_transfer_case->tr_ax_2 < 0 || !m_transfer_case->tr_2wd)
        return;

    if (m_transfer_case->tr_4wd_mode && !m_transfer_case->tr_2wd_lo)
    {
        for (int i = 0; i < m_transfer_case->tr_gear_ratios.size(); i++)
        {
            this->ToggleTransferCaseGearRatio();
            if (m_transfer_case->tr_gear_ratios[0] == 1.0f)
                break;
        }
    }

    m_transfer_case->tr_4wd_mode = !m_transfer_case->tr_4wd_mode;

    if (m_transfer_case->tr_4wd_mode)
    {
        ar_wheels[m_wheel_diffs[m_transfer_case->tr_ax_2]->di_idx_1].wh_propulsed = true;
        ar_wheels[m_wheel_diffs[m_transfer_case->tr_ax_2]->di_idx_2].wh_propulsed = true;
        m_num_proped_wheels += 2;
    }
    else
    {
        ar_wheels[m_wheel_diffs[m_transfer_case->tr_ax_2]->di_idx_1].wh_propulsed = false;
        ar_wheels[m_wheel_diffs[m_transfer_case->tr_ax_2]->di_idx_2].wh_propulsed = false;
        m_num_proped_wheels -= 2;
    }
}

void Actor::ToggleTransferCaseGearRatio()
{
    if (!ar_engine || !m_transfer_case || m_transfer_case->tr_gear_ratios.size() < 2)
        return;

    if (m_transfer_case->tr_4wd_mode || m_transfer_case->tr_2wd_lo)
    {
        auto gear_ratios = &m_transfer_case->tr_gear_ratios;
        std::rotate(gear_ratios->begin(), gear_ratios->begin() + 1, gear_ratios->end());

        ar_engine->SetTCaseRatio(m_transfer_case->tr_gear_ratios[0]);
    }
}

String Actor::GetTransferCaseName()
{
    String name = "";
    if (m_transfer_case)
    {
        name += m_transfer_case->tr_4wd_mode ? "4WD " : "2WD ";
        if (m_transfer_case->tr_gear_ratios[0] > 1.0f)
            name += "Lo (" + TOSTRING(m_transfer_case->tr_gear_ratios[0]) + ":1)";
        else
            name += "Hi";
    }
    return name;
}

Ogre::Vector3 Actor::GetRotationCenter()
{
    Vector3 sum = Vector3::ZERO;
    std::vector<Vector3> positions;
    for (int i = 0; i < ar_num_nodes; i++)
    {
        Vector3 pos = ar_nodes[i].AbsPosition;
        const auto it = std::find_if(positions.begin(), positions.end(),
            [pos](const Vector3 ref) { return pos.positionEquals(ref, 0.01f); });
        if (it == positions.end())
        {
            sum += pos;
            positions.push_back(pos);
        }
    }
    return sum / positions.size();
}

float Actor::GetMinHeight(bool skip_virtual_nodes)
{
    float height = std::numeric_limits<float>::max(); 
    for (int i = 0; i < ar_num_nodes; i++)
    {
        if (!skip_virtual_nodes || !ar_nodes[i].nd_no_ground_contact)
        {
            height = std::min(ar_nodes[i].AbsPosition.y, height);
        }
    }
    return (!skip_virtual_nodes || height < std::numeric_limits<float>::max()) ? height : GetMinHeight(false);
}

float Actor::GetMaxHeight(bool skip_virtual_nodes)
{
    float height = std::numeric_limits<float>::min(); 
    for (int i = 0; i < ar_num_nodes; i++)
    {
        if (!skip_virtual_nodes || !ar_nodes[i].nd_no_ground_contact)
        {
            height = std::max(height, ar_nodes[i].AbsPosition.y);
        }
    }
    return (!skip_virtual_nodes || height > std::numeric_limits<float>::min()) ? height : GetMaxHeight(false);
}

float Actor::GetHeightAboveGround(bool skip_virtual_nodes)
{
    float agl = std::numeric_limits<float>::max(); 
    for (int i = 0; i < ar_num_nodes; i++)
    {
        if (!skip_virtual_nodes || !ar_nodes[i].nd_no_ground_contact)
        {
            Vector3 pos = ar_nodes[i].AbsPosition;
            agl = std::min(pos.y - App::GetSimTerrain()->GetCollisions()->getSurfaceHeight(pos.x, pos.z), agl);
        }
    }
    return (!skip_virtual_nodes || agl < std::numeric_limits<float>::max()) ? agl : GetHeightAboveGround(false);
}

float Actor::GetHeightAboveGroundBelow(float height, bool skip_virtual_nodes)
{
    float agl = std::numeric_limits<float>::max(); 
    for (int i = 0; i < ar_num_nodes; i++)
    {
        if (!skip_virtual_nodes || !ar_nodes[i].nd_no_ground_contact)
        {
            Vector3 pos = ar_nodes[i].AbsPosition;
            agl = std::min(pos.y - App::GetSimTerrain()->GetCollisions()->getSurfaceHeightBelow(pos.x, pos.z, height), agl);
        }
    }
    return (!skip_virtual_nodes || agl < std::numeric_limits<float>::max()) ? agl : GetHeightAboveGroundBelow(height, false);
}

void Actor::SoftReset()
{
    TRIGGER_EVENT(SE_TRUCK_RESET, ar_instance_id);

    float agl = this->GetHeightAboveGroundBelow(this->GetMaxHeight(true), true);

    if (App::GetSimTerrain()->getWater())
    {
        agl = std::min(this->GetMinHeight(true) - App::GetSimTerrain()->getWater()->GetStaticWaterHeight(), agl);
    }

    if (agl < 0.0f)
    {
        Vector3 translation = -agl * Vector3::UNIT_Y;
        this->ResetPosition(ar_nodes[0].AbsPosition + translation, false);
        for (auto actor : m_linked_actors)
        {
            actor->ResetPosition(actor->ar_nodes[0].AbsPosition + translation, false);
        }
    }

    m_ongoing_reset = true;
}

void Actor::SyncReset(bool reset_position)
{
    TRIGGER_EVENT(SE_TRUCK_RESET, ar_instance_id);

    m_reset_timer.reset();

    m_camera_local_gforces_cur = Vector3::ZERO;
    m_camera_local_gforces_max = Vector3::ZERO;

    ar_hydro_dir_state = 0.0;
    ar_hydro_aileron_state = 0.0;
    ar_hydro_rudder_state = 0.0;
    ar_hydro_elevator_state = 0.0;
    ar_hydro_dir_wheel_display = 0.0;
	
    ar_fusedrag = Vector3::ZERO;
    m_blink_type = BLINK_NONE;
    ar_parking_brake = false;
    ar_trailer_parking_brake = false;
    ar_avg_wheel_speed = 0.0f;
    ar_wheel_speed = 0.0f;
    ar_wheel_spin = 0.0f;
    cc_mode = false;

    ar_origin = Vector3::ZERO;
    float cur_rot = getRotation();
    Vector3 cur_position = ar_nodes[0].AbsPosition;

    for (int i = 0; i < ar_num_nodes; i++)
    {
        ar_nodes[i].AbsPosition = ar_initial_node_positions[i];
        ar_nodes[i].RelPosition = ar_nodes[i].AbsPosition - ar_origin;
        ar_nodes[i].Velocity = Vector3::ZERO;
        ar_nodes[i].Forces = Vector3::ZERO;
    }

    for (int i = 0; i < ar_num_beams; i++)
    {
        ar_beams[i].maxposstress    = ar_beams[i].default_beam_deform;
        ar_beams[i].maxnegstress    = -ar_beams[i].default_beam_deform;
        ar_beams[i].minmaxposnegstress = ar_beams[i].default_beam_deform;
        ar_beams[i].strength        = ar_beams[i].initial_beam_strength;
        ar_beams[i].L               = ar_beams[i].refL;
        ar_beams[i].stress          = 0.0;
        ar_beams[i].bm_broken       = false;
        ar_beams[i].bm_disabled     = false;
    }

    this->ApplyNodeBeamScales();

    this->DisjoinInterActorBeams();

    for (auto& h : ar_hooks)
    {
        h.hk_locked = UNLOCKED;
        h.hk_lock_node = nullptr;
        h.hk_locked_actor = nullptr;
        h.hk_beam->p2 = &ar_nodes[0];
        h.hk_beam->bm_disabled = true;
        h.hk_beam->bm_inter_actor = false;
        h.hk_beam->L = (ar_nodes[0].AbsPosition - h.hk_hook_node->AbsPosition).length();
        this->RemoveInterActorBeam(h.hk_beam);
    }

    for (auto& r : ar_ropes)
    {
        r.rp_locked = UNLOCKED;
        r.rp_locked_ropable = nullptr;
        r.rp_locked_actor = nullptr;
        this->RemoveInterActorBeam(r.rp_beam);
    }

    for (auto& t : ar_ties)
    {
        t.ti_tied = false;
        t.ti_tying = false;
        t.ti_locked_actor = nullptr;
        t.ti_locked_ropable = nullptr;
        t.ti_beam->p2 = &ar_nodes[0];
        t.ti_beam->bm_disabled = true;
        t.ti_beam->bm_inter_actor = false;
        this->RemoveInterActorBeam(t.ti_beam);
    }

    for (auto& r : ar_ropables)
    {
        r.attached_ties = 0;
        r.attached_ropes = 0;
    }

    for (int i = 0; i < ar_num_wheels; i++)
    {
        ar_wheels[i].wh_speed = 0.0;
        ar_wheels[i].wh_torque = 0.0;
        ar_wheels[i].wh_avg_speed = 0.0;
        ar_wheels[i].wh_is_detached = false;
    }

    if (ar_engine)
    {
        if (App::sim_spawn_running->GetBool())
        {
            ar_engine->StartEngine();
        }
        ar_engine->SetWheelSpin(0.0f);
    }

    int num_axle_diffs = (m_transfer_case && m_transfer_case->tr_4wd_mode) ? m_num_axle_diffs + 1 : m_num_axle_diffs;
    for (int i = 0; i < num_axle_diffs; i++)
        m_axle_diffs[i]->di_delta_rotation = 0.0f;
    for (int i = 0; i < m_num_wheel_diffs; i++)
        m_wheel_diffs[i]->di_delta_rotation = 0.0f;
    for (int i = 0; i < ar_num_aeroengines; i++)
        ar_aeroengines[i]->reset();
    for (int i = 0; i < ar_num_screwprops; i++)
        ar_screwprops[i]->reset();
    for (int i = 0; i < ar_num_rotators; i++)
        ar_rotators[i].angle = 0.0;
    for (int i = 0; i < ar_num_wings; i++)
        ar_wings[i].fa->broken = false;
    if (ar_autopilot)
        this->ar_autopilot->reset();
    if (m_buoyance)
        m_buoyance->sink = false;

    for (hydrobeam_t& hydrobeam: ar_hydros)
    {
        hydrobeam.hb_inertia.ResetCmdKeyDelay();
    }

    this->GetGfxActor()->ResetFlexbodies();

    // reset on spot with backspace
    if (!reset_position)
    {
        this->ResetAngle(cur_rot);
        this->ResetPosition(cur_position, false);
        float agl = this->GetHeightAboveGroundBelow(this->GetMaxHeight(true), true);
        if (App::GetSimTerrain()->getWater())
        {
            agl = std::min(this->GetMinHeight(true) - App::GetSimTerrain()->getWater()->GetStaticWaterHeight(), agl);
        }
        if (agl < 0.0f)
        {
            this->ResetPosition(ar_nodes[0].AbsPosition - agl * Vector3::UNIT_Y, false);
        }
    }
    else
    {
        this->UpdateBoundingBoxes();
        this->calculateAveragePosition();
    }

    for (int i = 0; i < MAX_COMMANDS; i++)
    {
        ar_command_key[i].commandValue = 0.0;
        ar_command_key[i].triggerInputValue = 0.0f;
        ar_command_key[i].playerInputValue = 0.0f;
        for (auto& b : ar_command_key[i].beams)
        {
            b.cmb_state->auto_moving_mode = 0;
            b.cmb_state->pressed_center_mode = false;
        }
    }

    this->resetSlideNodes();
    if (m_slidenodes_locked)
    {
        this->ToggleSlideNodeLock();
    }

    m_ongoing_reset = true;
}

void Actor::ApplyNodeBeamScales()
{
    for (int i = 0; i < ar_num_nodes; i++)
    {
        ar_nodes[i].mass = ar_initial_node_masses[i] * ar_nb_mass_scale;
    }

    m_total_mass = ar_initial_total_mass * ar_nb_mass_scale;

    for (int i = 0; i < ar_num_beams; i++)
    {
        if ((ar_beams[i].p1->nd_tyre_node || ar_beams[i].p1->nd_rim_node) ||
            (ar_beams[i].p2->nd_tyre_node || ar_beams[i].p2->nd_rim_node))
        {
            ar_beams[i].k = ar_initial_beam_defaults[i].first * ar_nb_wheels_scale.first;
            ar_beams[i].d = ar_initial_beam_defaults[i].second * ar_nb_wheels_scale.second;
        }
        else if (ar_beams[i].bounded == SHOCK1 || ar_beams[i].bounded == SHOCK2 || ar_beams[i].bounded == SHOCK3)
        {
            ar_beams[i].k = ar_initial_beam_defaults[i].first * ar_nb_shocks_scale.first;;
            ar_beams[i].d = ar_initial_beam_defaults[i].second * ar_nb_shocks_scale.second;
        }
        else
        {
            ar_beams[i].k = ar_initial_beam_defaults[i].first * ar_nb_beams_scale.first;
            ar_beams[i].d = ar_initial_beam_defaults[i].second * ar_nb_beams_scale.second;
        }
    }
}

void Actor::ForceFeedbackStep(int steps)
{
    m_force_sensors.out_body_forces = m_force_sensors.accu_body_forces / steps;
    if (!ar_hydros.empty()) // Vehicle has hydros?
    {
        m_force_sensors.out_hydros_forces = (m_force_sensors.accu_hydros_forces / steps) / ar_hydros.size();    
    }
}

void Actor::HandleAngelScriptEvents(float dt)
{
#ifdef USE_ANGELSCRIPT

    // TODO: restore events SE_TRUCK_LOCKED and SE_TRUCK_UNLOCKED
    if (m_water_contact && !m_water_contact_old)
    {
        m_water_contact_old = m_water_contact;
        App::GetScriptEngine()->triggerEvent(SE_TRUCK_TOUCHED_WATER, ar_instance_id);
    }
#endif // USE_ANGELSCRIPT
}

void Actor::SearchBeamDefaults()
{
    SyncReset(true);

    auto old_beams_scale = ar_nb_beams_scale;
    auto old_shocks_scale = ar_nb_shocks_scale;
    auto old_wheels_scale = ar_nb_wheels_scale;

    if (ar_nb_initialized)
    {
        ar_nb_beams_scale.first   = Math::RangeRandom(ar_nb_beams_k_interval.first,  ar_nb_beams_k_interval.second);
        ar_nb_beams_scale.second  = Math::RangeRandom(ar_nb_beams_d_interval.first,  ar_nb_beams_d_interval.second);
        ar_nb_shocks_scale.first  = Math::RangeRandom(ar_nb_shocks_k_interval.first, ar_nb_shocks_k_interval.second);
        ar_nb_shocks_scale.second = Math::RangeRandom(ar_nb_shocks_d_interval.first, ar_nb_shocks_d_interval.second);
        ar_nb_wheels_scale.first  = Math::RangeRandom(ar_nb_wheels_k_interval.first, ar_nb_wheels_k_interval.second);
        ar_nb_wheels_scale.second = Math::RangeRandom(ar_nb_wheels_d_interval.first, ar_nb_wheels_d_interval.second);
    }
    else
    {
        ar_nb_beams_scale.first   = Math::Clamp(1.0f, ar_nb_beams_k_interval.first,  ar_nb_beams_k_interval.second);
        ar_nb_beams_scale.second  = Math::Clamp(1.0f, ar_nb_beams_d_interval.first,  ar_nb_beams_d_interval.second);
        ar_nb_shocks_scale.first  = Math::Clamp(1.0f, ar_nb_shocks_k_interval.first, ar_nb_shocks_k_interval.second);
        ar_nb_shocks_scale.second = Math::Clamp(1.0f, ar_nb_shocks_d_interval.first, ar_nb_shocks_d_interval.second);
        ar_nb_wheels_scale.first  = Math::Clamp(1.0f, ar_nb_wheels_k_interval.first, ar_nb_wheels_k_interval.second);
        ar_nb_wheels_scale.second = Math::Clamp(1.0f, ar_nb_wheels_d_interval.first, ar_nb_wheels_d_interval.second);
        ar_nb_reference = std::vector<float>(ar_nb_reference.size(), std::numeric_limits<float>::max());
        ar_nb_optimum   = std::vector<float>(ar_nb_reference.size(), std::numeric_limits<float>::max());
    }

    this->ApplyNodeBeamScales();

    m_ongoing_reset = false;
    this->CalcForcesEulerPrepare(true);
    for (int i = 0; i < ar_nb_skip_steps; i++)
    {
        this->CalcForcesEulerCompute(i == 0, ar_nb_skip_steps);
        if (m_ongoing_reset)
            break;
    }
    m_ongoing_reset = true;

    float sum_movement = 0.0f;
    float movement = 0.0f;
    float sum_velocity = 0.0f;
    float velocity = 0.0f;
    float sum_stress = 0.0f;
    float stress = 0.0f;
    int sum_broken = 0;
    for (int k = 0; k < ar_nb_measure_steps; k++)
    {
        this->CalcForcesEulerCompute(false, ar_nb_measure_steps);
        for (int i = 0; i < ar_num_nodes; i++)
        {
            float v = ar_nodes[i].Velocity.length();
            sum_movement += v / (float)ar_nb_measure_steps;
            movement = std::max(movement, v);
        }
        for (int i = 0; i < ar_num_beams; i++)
        {
            Vector3 dis = (ar_beams[i].p1->RelPosition - ar_beams[i].p2->RelPosition).normalisedCopy();
            float v = (ar_beams[i].p1->Velocity - ar_beams[i].p2->Velocity).dotProduct(dis);
            sum_velocity += std::abs(v) / (float)ar_nb_measure_steps;
            velocity = std::max(velocity, std::abs(v));
            sum_stress += std::abs(ar_beams[i].stress) / (float)ar_nb_measure_steps;
            stress = std::max(stress, std::abs(ar_beams[i].stress));
            if (k == 0 && ar_beams[i].bm_broken)
            {
                sum_broken++;
            }
        }
        if (sum_broken > ar_nb_reference[6] ||
                stress > ar_nb_reference[0] ||     velocity > ar_nb_reference[2] ||     movement > ar_nb_optimum[4] ||
            sum_stress > ar_nb_reference[1] || sum_velocity > ar_nb_reference[3] || sum_movement > ar_nb_optimum[5] * 2.f)
        {
            ar_nb_beams_scale  = old_beams_scale;
            ar_nb_shocks_scale = old_shocks_scale;
            ar_nb_wheels_scale = old_wheels_scale;
            SyncReset(true);
            return;
        }
    }
    SyncReset(true);

    ar_nb_optimum = {stress, sum_stress, velocity, sum_velocity, movement, sum_movement, (float)sum_broken};
    if (!ar_nb_initialized)
    {
        ar_nb_reference = ar_nb_optimum;
    }
    ar_nb_initialized = true;
}

void Actor::HandleInputEvents(float dt)
{
    if (!m_ongoing_reset)
        return;

    if (m_anglesnap_request > 0)
    {
        float rotation = Radian(getRotation()).valueDegrees();
        float target_rotation = std::round(rotation / m_anglesnap_request) * m_anglesnap_request;
        m_rotation_request = -Degree(target_rotation - rotation).valueRadians();
	m_rotation_request_center = GetRotationCenter();
        m_anglesnap_request = 0;
    }

    if (m_rotation_request != 0.0f)
    {
        Quaternion rot = Quaternion(Radian(m_rotation_request), Vector3::UNIT_Y);

        for (int i = 0; i < ar_num_nodes; i++)
        {
            ar_nodes[i].AbsPosition -= m_rotation_request_center;
            ar_nodes[i].AbsPosition = rot * ar_nodes[i].AbsPosition;
            ar_nodes[i].AbsPosition += m_rotation_request_center;
            ar_nodes[i].RelPosition = ar_nodes[i].AbsPosition - ar_origin;
            ar_nodes[i].Velocity = rot * ar_nodes[i].Velocity;
            ar_nodes[i].Forces = rot * ar_nodes[i].Forces;
        }

        m_rotation_request = 0.0f;
        this->UpdateBoundingBoxes();
        calculateAveragePosition();
    }

    if (m_translation_request != Vector3::ZERO)
    {
        for (int i = 0; i < ar_num_nodes; i++)
        {
            ar_nodes[i].AbsPosition += m_translation_request;
            ar_nodes[i].RelPosition = ar_nodes[i].AbsPosition - ar_origin;
        }

        m_translation_request = Vector3::ZERO;
        UpdateBoundingBoxes();
        calculateAveragePosition();
    }
}

void Actor::sendStreamSetup()
{
    RoRnet::ActorStreamRegister reg;
    memset(&reg, 0, sizeof(RoRnet::ActorStreamRegister));
    reg.status = 0;
    reg.type = 0;
    reg.time = App::GetGameContext()->GetActorManager()->GetNetTime();
    strncpy(reg.name, ar_filename.c_str(), 128);
    if (m_used_skin_entry != nullptr)
    {
        strncpy(reg.skin, m_used_skin_entry->dname.c_str(), 60);
    }
    strncpy(reg.sectionconfig, m_section_config.c_str(), 60);

#ifdef USE_SOCKETW
    App::GetNetwork()->AddLocalStream((RoRnet::StreamRegister *)&reg, sizeof(RoRnet::ActorStreamRegister));
#endif // USE_SOCKETW

    ar_net_source_id = reg.origin_sourceid;
    ar_net_stream_id = reg.origin_streamid;
}

void Actor::sendStreamData()
{
    using namespace RoRnet;
#ifdef USE_SOCKETW
    if (ar_net_timer.getMilliseconds() - ar_net_last_update_time < 100)
        return;

    ar_net_last_update_time = ar_net_timer.getMilliseconds();

    //look if the packet is too big first
    if (m_net_buffer_size + sizeof(RoRnet::VehicleState) > 8192)
    {
        ErrorUtils::ShowError(_L("Actor is too big to be sent over the net."), _L("Network error!"));
        exit(126);
    }

    char send_buffer[8192] = {0};

    unsigned int packet_len = 0;

    // RoRnet::VehicleState is at the beginning of the buffer
    {
        RoRnet::VehicleState* send_oob = (RoRnet::VehicleState *)send_buffer;
        packet_len += sizeof(RoRnet::VehicleState);

        send_oob->flagmask = 0;

        send_oob->time = App::GetGameContext()->GetActorManager()->GetNetTime();
        if (ar_engine)
        {
            send_oob->engine_speed = ar_engine->GetEngineRpm();
            send_oob->engine_force = ar_engine->GetAcceleration();
            send_oob->engine_clutch = ar_engine->GetClutch();
            send_oob->engine_gear = ar_engine->GetGear();

            if (ar_engine->HasStarterContact())
                send_oob->flagmask += NETMASK_ENGINE_CONT;
            if (ar_engine->IsRunning())
                send_oob->flagmask += NETMASK_ENGINE_RUN;

            switch (ar_engine->GetAutoShiftMode())
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

        BlinkType b = getBlinkType();
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
        char* ptr = send_buffer + sizeof(RoRnet::VehicleState);
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
            sbuf[(i - 1) * 3 + 0] = (short int)(relpos.x * m_net_node_compression);
            sbuf[(i - 1) * 3 + 1] = (short int)(relpos.y * m_net_node_compression);
            sbuf[(i - 1) * 3 + 2] = (short int)(relpos.z * m_net_node_compression);

            ptr += sizeof(short int) * 3; // increase pointer
        }

        // then to the wheels
        float* wfbuf = (float*)ptr;
        for (i = 0; i < ar_num_wheels; i++)
        {
            wfbuf[i] = ar_wheels[i].wh_net_rp;
        }
    }

    App::GetNetwork()->AddPacket(ar_net_stream_id, MSG2_STREAM_DATA_DISCARDABLE, packet_len, send_buffer);
#endif //SOCKETW
}

void Actor::CalcAnimators(hydrobeam_t const& hydrobeam, float &cstate, int &div)
{
    // boat rudder
    if (hydrobeam.hb_anim_flags & ANIM_FLAG_BRUDDER)
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

    // boat throttle
    if (hydrobeam.hb_anim_flags & ANIM_FLAG_BTHROTTLE)
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

    // differential lock status
    if (hydrobeam.hb_anim_flags & ANIM_FLAG_DIFFLOCK)
    {
        if (m_num_wheel_diffs && m_wheel_diffs[0])
        {
            String name = m_wheel_diffs[0]->GetDifferentialTypeName();
            if (name == "Open")
                cstate = 0.0f;
            if (name == "Split")
                cstate = 0.5f;
            if (name == "Locked")
                cstate = 1.0f;
        }
        else // no axles/diffs avail, mode is split by default
            cstate = 0.5f;

        div++;
    }

    // heading
    if (hydrobeam.hb_anim_flags & ANIM_FLAG_HEADING)
    {
        float heading = getRotation();
        // rad2deg limitedrange  -1 to +1
        cstate = (heading * 57.29578f) / 360.0f;
        div++;
    }

    // torque
    if (ar_engine && hydrobeam.hb_anim_flags & ANIM_FLAG_TORQUE)
    {
        float torque = ar_engine->GetCrankFactor();
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

    // shifterseq, to amimate sequentiell shifting
    if (ar_engine && (hydrobeam.hb_anim_flags & ANIM_FLAG_SHIFTER) && hydrobeam.hb_anim_param == 3.0f)
    {

        int shifter = ar_engine->GetGear();
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
            ar_anim_shift_timer -= PHYSICS_DT;
            if (ar_anim_shift_timer < 0.0f)
                ar_anim_shift_timer = 0.0f;
        }
        if (ar_anim_shift_timer < 0.0f)
        {
            cstate = -1.0f;
            ar_anim_shift_timer += PHYSICS_DT;
            if (ar_anim_shift_timer > 0.0f)
                ar_anim_shift_timer = 0.0f;
        }

        div++;
    }

    // shifterman1, left/right
    if (ar_engine && (hydrobeam.hb_anim_flags & ANIM_FLAG_SHIFTER) && hydrobeam.hb_anim_param == 1.0f)
    {
        int shifter = ar_engine->GetGear();
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

    // shifterman2, up/down
    if (ar_engine && (hydrobeam.hb_anim_flags & ANIM_FLAG_SHIFTER) && hydrobeam.hb_anim_param == 2.0f)
    {
        int shifter = ar_engine->GetGear();
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

    // shifterlinear, to amimate cockpit gearselect gauge and autotransmission stick
    if (ar_engine && (hydrobeam.hb_anim_flags & ANIM_FLAG_SHIFTER) && hydrobeam.hb_anim_param == 4.0f)
    {
        int shifter = ar_engine->GetGear();
        int numgears = ar_engine->getNumGears();
        cstate -= (shifter + 2.0) / (numgears + 2.0);
        div++;
    }

    // parking brake
    if (hydrobeam.hb_anim_flags & ANIM_FLAG_PBRAKE)
    {
        float pbrake = ar_parking_brake;
        cstate -= pbrake;
        div++;
    }

    // speedo ( scales with speedomax )
    if (hydrobeam.hb_anim_flags & ANIM_FLAG_SPEEDO)
    {
        float speedo = ar_wheel_speed / ar_speedo_max_kph;
        cstate -= speedo * 3.0f;
        div++;
    }

    // engine tacho ( scales with maxrpm, default is 3500 )
    if (ar_engine && hydrobeam.hb_anim_flags & ANIM_FLAG_TACHO)
    {
        float tacho = ar_engine->GetEngineRpm() / ar_engine->getMaxRPM();
        cstate -= tacho;
        div++;
    }

    // turbo
    if (ar_engine && hydrobeam.hb_anim_flags & ANIM_FLAG_TURBO)
    {
        float turbo = ar_engine->GetTurboPsi() * 3.34;
        cstate -= turbo / 67.0f;
        div++;
    }

    // brake
    if (hydrobeam.hb_anim_flags & ANIM_FLAG_BRAKE)
    {
        cstate -= ar_brake;
        div++;
    }

    // accelerator
    if (ar_engine && hydrobeam.hb_anim_flags & ANIM_FLAG_ACCEL)
    {
        float accel = ar_engine->GetAcceleration();
        cstate -= accel + 0.06f;
        //( small correction, get acc is nver smaller then 0.06.
        div++;
    }

    // clutch
    if (ar_engine && hydrobeam.hb_anim_flags & ANIM_FLAG_CLUTCH)
    {
        float clutch = ar_engine->GetClutch();
        cstate -= fabs(1.0f - clutch);
        div++;
    }

    // aeroengines (hb_anim_param is the engine index)
    if ((int)hydrobeam.hb_anim_param < ar_num_aeroengines)
    {
        int aenum = (int)hydrobeam.hb_anim_param;
        if (hydrobeam.hb_anim_flags & ANIM_FLAG_RPM)
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
        if (hydrobeam.hb_anim_flags & ANIM_FLAG_THROTTLE)
        {
            float throttle = ar_aeroengines[aenum]->getThrottle();
            cstate -= throttle;
            div++;
        }

        if (hydrobeam.hb_anim_flags & ANIM_FLAG_AETORQUE)
            if (ar_aeroengines[aenum]->getType() == AEROENGINE_TURBOPROP_PISTONPROP)
            {
                Turboprop* tp = (Turboprop*)ar_aeroengines[aenum];
                cstate = (100.0 * tp->indicated_torque / tp->max_torque) / 120.0f;
                div++;
            }

        if (hydrobeam.hb_anim_flags & ANIM_FLAG_AEPITCH)
            if (ar_aeroengines[aenum]->getType() == AEROENGINE_TURBOPROP_PISTONPROP)
            {
                Turboprop* tp = (Turboprop*)ar_aeroengines[aenum];
                cstate = tp->pitch / 120.0f;
                div++;
            }

        if (hydrobeam.hb_anim_flags & ANIM_FLAG_AESTATUS)
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

    // airspeed indicator
    if (hydrobeam.hb_anim_flags & ANIM_FLAG_AIRSPEED)
    {
        float ground_speed_kt = ar_nodes[0].Velocity.length() * 1.9438;
        float altitude = ar_nodes[0].AbsPosition.y;
        float sea_level_pressure = 101325; //in Pa
        float airpressure = sea_level_pressure * pow(1.0 - 0.0065 * altitude / 288.15, 5.24947); //in Pa
        float airdensity = airpressure * 0.0000120896;//1.225 at sea level
        float kt = ground_speed_kt * sqrt(airdensity / 1.225);
        cstate -= kt / 100.0f;
        div++;
    }

    // vvi indicator
    if (hydrobeam.hb_anim_flags & ANIM_FLAG_VVI)
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

    // altimeter
    if (hydrobeam.hb_anim_flags & ANIM_FLAG_ALTIMETER)
    {
        //altimeter indicator 1k oscillating
        if (hydrobeam.hb_anim_param == 3.0f)
        {
            float altimeter = (ar_nodes[0].AbsPosition.y * 1.1811) / 360.0f;
            int alti_int = int(altimeter);
            float alti_mod = (altimeter - alti_int);
            cstate -= alti_mod;
        }

        //altimeter indicator 10k oscillating
        if (hydrobeam.hb_anim_param == 2.0f)
        {
            float alti = ar_nodes[0].AbsPosition.y * 1.1811 / 3600.0f;
            int alti_int = int(alti);
            float alti_mod = (alti - alti_int);
            cstate -= alti_mod;
            if (cstate <= -1.0f)
                cstate = -1.0f;
        }

        //altimeter indicator 100k limited
        if (hydrobeam.hb_anim_param == 1.0f)
        {
            float alti = ar_nodes[0].AbsPosition.y * 1.1811 / 36000.0f;
            cstate -= alti;
            if (cstate <= -1.0f)
                cstate = -1.0f;
        }
        div++;
    }

    // AOA
    if (hydrobeam.hb_anim_flags & ANIM_FLAG_AOA)
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

    // roll
    if (hydrobeam.hb_anim_flags & ANIM_FLAG_ROLL)
    {
        Vector3 rollv = this->GetCameraRoll();
        Vector3 dirv = this->GetCameraDir();
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
    if (hydrobeam.hb_anim_flags & ANIM_FLAG_PITCH)
    {
        Vector3 dirv = this->GetCameraDir();
        float pitchangle = asin(dirv.dotProduct(Vector3::UNIT_Y));
        // radian to degrees with a max cstate of +/- 1.0
        cstate = (Math::RadiansToDegrees(pitchangle) / 90.0f);
        div++;
    }

    // airbrake
    if (hydrobeam.hb_anim_flags & ANIM_FLAG_AIRBRAKE)
    {
        float airbrake = ar_airbrake_intensity;
        // cstate limited to -1.0f
        cstate -= airbrake / 5.0f;
        div++;
    }

    // flaps
    if (hydrobeam.hb_anim_flags & ANIM_FLAG_FLAP)
    {
        float flaps = FLAP_ANGLES[ar_aerial_flap];
        // cstate limited to -1.0f
        cstate = flaps;
        div++;
    }
}

void Actor::CalcCabCollisions()
{
    for (int i = 0; i < ar_num_nodes; i++)
    {
        ar_nodes[i].nd_has_mesh_contact = false;
    }
    if (m_intra_point_col_detector != nullptr)
    {
        m_intra_point_col_detector->UpdateIntraPoint();
        ResolveIntraActorCollisions(PHYSICS_DT,
            *m_intra_point_col_detector,
            ar_num_collcabs,
            ar_collcabs,
            ar_cabs,
            ar_intra_collcabrate,
            ar_nodes,
            ar_collision_range,
            *ar_submesh_ground_model);
    }
}

void Actor::CalcShocks2(int i, Real difftoBeamL, Real& k, Real& d, Real v)
{
    if (v > 0) // Extension
    {
        k = ar_beams[i].shock->springout;
        d = ar_beams[i].shock->dampout;
        // add progression
        float logafactor = 1.0f;
        if (ar_beams[i].longbound != 0.0f)
        {
            logafactor = difftoBeamL / (ar_beams[i].longbound * ar_beams[i].L);
            logafactor = std::min(logafactor * logafactor, 1.0f);
        }
        k += ar_beams[i].shock->sprogout * k * logafactor;
        d += ar_beams[i].shock->dprogout * d * logafactor;
    }
    else // Compression
    {
        k = ar_beams[i].shock->springin;
        d = ar_beams[i].shock->dampin;
        // add progression
        float logafactor = 1.0f;
        if (ar_beams[i].shortbound != 0.0f)
        {
            logafactor = difftoBeamL / (ar_beams[i].shortbound * ar_beams[i].L);
            logafactor = std::min(logafactor * logafactor, 1.0f);
        }
        k += ar_beams[i].shock->sprogin * k * logafactor;
        d += ar_beams[i].shock->dprogin * d * logafactor;
    }
    if (ar_beams[i].shock->flags & SHOCK_FLAG_SOFTBUMP)
    {
        // soft bump shocks
        float beamsLep = ar_beams[i].L * 0.8f;
        float longboundprelimit = ar_beams[i].longbound * beamsLep;
        float shortboundprelimit = -ar_beams[i].shortbound * beamsLep;
        if (difftoBeamL > longboundprelimit)
        {
            // reset to longbound progressive values (oscillating beam workaround)
            k = ar_beams[i].shock->springout;
            d = ar_beams[i].shock->dampout;
            // add progression
            float logafactor = 1.0f;
            if (ar_beams[i].longbound != 0.0f)
            {
                logafactor = difftoBeamL / (ar_beams[i].longbound * ar_beams[i].L);
                logafactor = std::min(logafactor * logafactor, 1.0f);
            }
            k += ar_beams[i].shock->sprogout * k * logafactor;
            d += ar_beams[i].shock->dprogout * d * logafactor;
            // add shortbump progression
            logafactor = 1.0f;
            if (ar_beams[i].longbound != 0.0f)
            {
                logafactor = ((difftoBeamL - longboundprelimit) * 5.0f) / (ar_beams[i].longbound * ar_beams[i].L);
                logafactor = std::min(logafactor * logafactor, 1.0f);
            }
            k += (k + 100.0f) * ar_beams[i].shock->sprogout * logafactor;
            d += (d + 100.0f) * ar_beams[i].shock->dprogout * logafactor;
            if (v < 0)
            // rebound mode..get new values
            {
                k = ar_beams[i].shock->springin;
                d = ar_beams[i].shock->dampin;
            }
        }
        else if (difftoBeamL < shortboundprelimit)
        {
            // reset to shortbound progressive values (oscillating beam workaround)
            k = ar_beams[i].shock->springin;
            d = ar_beams[i].shock->dampin;
            // add progression
            float logafactor = 1.0f;
            if (ar_beams[i].shortbound != 0.0f)
            {
                logafactor = difftoBeamL / (ar_beams[i].shortbound * ar_beams[i].L);
                logafactor = std::min(logafactor * logafactor, 1.0f);
            }
            k += ar_beams[i].shock->sprogin * k * logafactor;
            d += ar_beams[i].shock->dprogin * d * logafactor;
            // add shortbump progression
            logafactor = 1.0f;
            if (ar_beams[i].shortbound != 0.0f)
            {
                logafactor = ((difftoBeamL - shortboundprelimit) * 5.0f) / (ar_beams[i].shortbound * ar_beams[i].L);
                logafactor = std::min(logafactor * logafactor, 1.0f);
            }
            k += (k + 100.0f) * ar_beams[i].shock->sprogout * logafactor;
            d += (d + 100.0f) * ar_beams[i].shock->dprogout * logafactor;
            if (v > 0)
            // rebound mode..get new values
            {
                k = ar_beams[i].shock->springout;
                d = ar_beams[i].shock->dampout;
            }
        }
        if (difftoBeamL > ar_beams[i].longbound * ar_beams[i].L || difftoBeamL < -ar_beams[i].shortbound * ar_beams[i].L)
        {
            // block reached...hard bump in soft mode with 4x default damping
            k = std::max(k, ar_beams[i].shock->sbd_spring);
            d = std::max(d, ar_beams[i].shock->sbd_damp);
        }
    }
    else if (difftoBeamL > ar_beams[i].longbound * ar_beams[i].L || difftoBeamL < -ar_beams[i].shortbound * ar_beams[i].L)
    {
        // hard (normal) shock bump
        k = ar_beams[i].shock->sbd_spring;
        d = ar_beams[i].shock->sbd_damp;
    }
}

void Actor::CalcShocks3(int i, Real difftoBeamL, Real &k, Real& d, Real v)
{
    if (difftoBeamL > ar_beams[i].longbound * ar_beams[i].L)
    {
        float interp_ratio =  difftoBeamL - ar_beams[i].longbound  * ar_beams[i].L;
        k += (ar_beams[i].shock->sbd_spring - k) * interp_ratio;
        d += (ar_beams[i].shock->sbd_damp   - d) * interp_ratio;
    }
    else if (difftoBeamL < -ar_beams[i].shortbound * ar_beams[i].L)
    {
        float interp_ratio = -difftoBeamL - ar_beams[i].shortbound * ar_beams[i].L;
        k += (ar_beams[i].shock->sbd_spring - k) * interp_ratio;
        d += (ar_beams[i].shock->sbd_damp   - d) * interp_ratio;
    }
    else if (v > 0) // Extension
    {
        v = Math::Clamp(std::abs(v), +0.15f, +20.0f);
        k = ar_beams[i].shock->springout;
        d = ar_beams[i].shock->dampout * ar_beams[i].shock->dslowout * std::min(v,        ar_beams[i].shock->splitout) +
            ar_beams[i].shock->dampout * ar_beams[i].shock->dfastout * std::max(0.0f, v - ar_beams[i].shock->splitout);
        d /= v;
    }
    else if (v < 0) // Compression
    {
        v = Math::Clamp(std::abs(v), +0.15f, +20.0f);
        k = ar_beams[i].shock->springin;
        d = ar_beams[i].shock->dampin  * ar_beams[i].shock->dslowin  * std::min(v,        ar_beams[i].shock->splitin ) +
            ar_beams[i].shock->dampin  * ar_beams[i].shock->dfastin  * std::max(0.0f, v - ar_beams[i].shock->splitin );
        d /= v;
    }
}

void Actor::CalcTriggers(int i, Real difftoBeamL, bool trigger_hooks)
{
    if ((ar_beams[i].shock->flags & SHOCK_FLAG_ISTRIGGER) && ar_beams[i].shock->trigger_enabled) // this is a trigger and its enabled
    {
        const float dt = PHYSICS_DT;

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
                ar_command_key[ar_beams[i].shock->trigger_cmdshort].trigger_cmdkeyblock_state = false; // Release the cmdKey
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
                        if (trigger_hooks)
                        {
                            //autolock hooktoggle unlock
                            ToggleHooks(ar_beams[i].shock->trigger_cmdlong, HOOK_UNLOCK, -1);
                        }
                    }
                    else if (ar_beams[i].shock->flags & SHOCK_FLAG_TRG_HOOK_LOCK)
                    {
                        if (trigger_hooks)
                        {
                            //autolock hooktoggle lock
                            ToggleHooks(ar_beams[i].shock->trigger_cmdlong, HOOK_LOCK, -1);
                        }
                    }
                    else if (ar_beams[i].shock->flags & SHOCK_FLAG_TRG_ENGINE)
                    {
                        EngineTriggerHelper(ar_beams[i].shock->trigger_cmdshort, EngineTriggerType(ar_beams[i].shock->trigger_cmdlong), 1.0f);
                    }
                    else
                    {
                        //just a trigger
                        if (!ar_command_key[ar_beams[i].shock->trigger_cmdlong].trigger_cmdkeyblock_state) // related cmdkey is not blocked
                        {
                            if (ar_beams[i].shock->flags & SHOCK_FLAG_TRG_CONTINUOUS)
                                ar_command_key[ar_beams[i].shock->trigger_cmdshort].triggerInputValue = 1; // continuous trigger only operates on trigger_cmdshort
                            else
                                ar_command_key[ar_beams[i].shock->trigger_cmdlong].triggerInputValue = 1;
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
                        if (trigger_hooks)
                        {
                            //autolock hooktoggle unlock
                            ToggleHooks(ar_beams[i].shock->trigger_cmdshort, HOOK_UNLOCK, -1);
                        }
                    }
                    else if (ar_beams[i].shock->flags & SHOCK_FLAG_TRG_HOOK_LOCK)
                    {
                        if (trigger_hooks)
                        {
                            //autolock hooktoggle lock
                            ToggleHooks(ar_beams[i].shock->trigger_cmdshort, HOOK_LOCK, -1);
                        }
                    }
                    else if (ar_beams[i].shock->flags & SHOCK_FLAG_TRG_ENGINE)
                    {
                        bool triggerValue = !(ar_beams[i].shock->flags & SHOCK_FLAG_TRG_CONTINUOUS); // 0 if trigger is continuous, 1 otherwise

                        EngineTriggerHelper(ar_beams[i].shock->trigger_cmdshort, EngineTriggerType(ar_beams[i].shock->trigger_cmdlong), triggerValue);
                    }
                    else
                    {
                        //just a trigger
                        if (!ar_command_key[ar_beams[i].shock->trigger_cmdshort].trigger_cmdkeyblock_state) // related cmdkey is not blocked
                        {
                            if (ar_beams[i].shock->flags & SHOCK_FLAG_TRG_CONTINUOUS)
                                ar_command_key[ar_beams[i].shock->trigger_cmdshort].triggerInputValue = 0; // continuous trigger only operates on trigger_cmdshort
                            else
                                ar_command_key[ar_beams[i].shock->trigger_cmdshort].triggerInputValue = 1;

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
                        EngineTriggerHelper(ar_beams[i].shock->trigger_cmdshort, EngineTriggerType(ar_beams[i].shock->trigger_cmdlong), triggerValue);
                    }
                    else
                    {
                        // normal trigger
                        ar_command_key[ar_beams[i].shock->trigger_cmdshort].triggerInputValue = triggerValue;
                        ar_command_key[ar_beams[i].shock->trigger_cmdlong].triggerInputValue = triggerValue;
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
            else if ((ar_beams[i].shock->flags & SHOCK_FLAG_TRG_CMD_BLOCKER) && !ar_command_key[ar_beams[i].shock->trigger_cmdshort].trigger_cmdkeyblock_state) // this cmdkeyblocker is inside boundaries and cmdkeystate is diabled
            {
                ar_command_key[ar_beams[i].shock->trigger_cmdshort].trigger_cmdkeyblock_state = true; // activate trigger blocking
                if (m_trigger_debug_enabled && ar_beams[i].shock->last_debug_state != 8)
                {
                    LOG(" F-key trigger blocked. Blocker BeamID " + TOSTRING(i) + " Blocked F" + TOSTRING(ar_beams[i].shock->trigger_cmdshort));
                    ar_beams[i].shock->last_debug_state = 8;
                }
            }
        }
    }
}

void Actor::setAirbrakeIntensity(float intensity)
{
    ar_airbrake_intensity = intensity;
    for (Airbrake* ab: ar_airbrakes)
    {
        ab->updatePosition((float)ar_airbrake_intensity / 5.0);
    }
}

// call this once per frame in order to update the skidmarks
void Actor::updateSkidmarks()
{
    for (int i = 0; i < ar_num_wheels; i++)
    {
        if (!m_skid_trails[i])
            continue;

        for (int j = 0; j < ar_wheels[i].wh_num_nodes; j++)
        {
            auto n = ar_wheels[i].wh_nodes[j];
            if (!n || !n->nd_has_ground_contact || n->nd_last_collision_gm == nullptr ||
                    n->nd_last_collision_gm->fx_type != Collisions::FX_HARD)
            {
                continue;
            }
            if (n->nd_avg_collision_slip > 6.f && n->nd_last_collision_slip.squaredLength() > 9.f)
            {
                m_skid_trails[i]->update(n->AbsPosition, j, n->nd_avg_collision_slip, n->nd_last_collision_gm->name);
                return;
            }
        }
    }
}

void Actor::prepareInside(bool inside)
{
    // TODO: this whole function belongs to GfxActor ~ 08/2018
    if (inside)
    {
        App::GetCameraManager()->GetCamera()->setNearClipDistance(0.1f);

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

        App::GetCameraManager()->GetCamera()->setNearClipDistance(0.5f);

        // disable transparent seat
        MaterialPtr seatmat = (MaterialPtr)(MaterialManager::getSingleton().getByName("driversseat"));
        seatmat->setDepthWriteEnabled(true);
        seatmat->setSceneBlending(SBT_REPLACE);
    }

  // TEMPORARY - until this function is moved to GfxActor ~ 08/2018
  //  if (m_cab_scene_node != nullptr)
  //  {
  //      m_gfx_actor->GetCabTransMaterial()->setReceiveShadows(!inside);
  //  }

    if (App::gfx_reduce_shadows->GetBool())
    {
        m_gfx_actor->SetCastShadows(!inside);
    }
}

void Actor::ToggleLights()
{
    // export light command
    Actor* player_actor = App::GetGameContext()->GetPlayerActor();
    if (ar_sim_state == Actor::SimState::LOCAL_SIMULATED && this == player_actor && ar_forward_commands)
    {
        for (auto actor : App::GetGameContext()->GetActorManager()->GetActors())
        {
            if (actor->ar_sim_state == Actor::SimState::LOCAL_SIMULATED && this != actor && actor->ar_import_commands)
                actor->ToggleLights();
        }
    }
    ar_lights = !ar_lights;
    if (!ar_lights)
    {
        for (size_t i = 0; i < ar_flares.size(); i++)
        {
            if (ar_flares[i].fl_type == FlareType::HEADLIGHT)
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
            if (ar_flares[i].fl_type == FlareType::HEADLIGHT)
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

void Actor::UpdateFlareStates(float dt)
{
    if (m_custom_light_toggle_countdown > -1)
        m_custom_light_toggle_countdown -= dt;

    if (m_flares_mode == GfxFlaresMode::NONE) { return; }

    // NOTE: Beacon flares are now updated in GfxActor::UpdateBeaconFlares()

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
        // NOTE: headlight (type 'f') is updated
        if (ar_flares[i].fl_type == FlareType::HEADLIGHT)
        {
            // NOTE: Material flare is updated in GfxActor
            if (!ar_lights)
                continue;
        }
        else if (ar_flares[i].fl_type == FlareType::BRAKE_LIGHT)
        {
            isvisible = getBrakeLightVisible();
        }
        else if (ar_flares[i].fl_type == FlareType::REVERSE_LIGHT)
        {
            if (ar_engine || m_reverse_light_active)
                isvisible = getReverseLightVisible();
            else
                isvisible = false;
        }
        else if (ar_flares[i].fl_type == FlareType::USER && ar_flares[i].controlnumber != -1) // controlnumber = read only attribute
        {
            if (ar_sim_state == Actor::SimState::LOCAL_SIMULATED && this == App::GetGameContext()->GetPlayerActor()) // no network!!
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
        else if (ar_flares[i].fl_type == FlareType::BLINKER_LEFT)
        {
            isvisible = (m_blink_type == BLINK_LEFT || m_blink_type == BLINK_WARN);
        }
        else if (ar_flares[i].fl_type == FlareType::BLINKER_RIGHT)
        {
            isvisible = (m_blink_type == BLINK_RIGHT || m_blink_type == BLINK_WARN);
        }
        // apply blinking
        isvisible = isvisible && ar_flares[i].blinkdelay_state;

        if (ar_flares[i].fl_type == FlareType::BLINKER_LEFT && m_blink_type == BLINK_LEFT)
        {
            ar_left_blink_on = isvisible;

            if (ar_left_blink_on)
                SOUND_PLAY_ONCE(ar_instance_id, SS_TRIG_TURN_SIGNAL_TICK);
        }
        else if (ar_flares[i].fl_type == FlareType::BLINKER_RIGHT && m_blink_type == BLINK_RIGHT)
        {
            ar_right_blink_on = isvisible;

            if (ar_right_blink_on)
                SOUND_PLAY_ONCE(ar_instance_id, SS_TRIG_TURN_SIGNAL_TICK);
        }
        else if (ar_flares[i].fl_type == FlareType::BLINKER_LEFT && m_blink_type == BLINK_WARN)
        {
            ar_warn_blink_on = isvisible;

            if (ar_warn_blink_on)
                SOUND_PLAY_ONCE(ar_instance_id, SS_TRIG_TURN_SIGNAL_WARN_TICK);
        }

        ar_flares[i].isVisible = isvisible; // 3D engine objects are updated in GfxActor
    }
    if (keysleep)
        m_custom_light_toggle_countdown = 0.2;
}

void Actor::toggleBlinkType(BlinkType blink)
{
    if (m_blink_type == blink)
        setBlinkType(BLINK_NONE);
    else
        setBlinkType(blink);
}

void Actor::setBlinkType(BlinkType blink)
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

void Actor::autoBlinkReset()
{
    // TODO: make this set-able per actor
    const float blink_lock_range = App::io_blink_lock_range->GetFloat();

    if (m_blink_type == BLINK_LEFT && ar_hydro_dir_state < -blink_lock_range)
    // passed the threshold: the turn signal gets locked
        m_blinker_autoreset = true;

    if (m_blink_type == BLINK_LEFT && m_blinker_autoreset && ar_hydro_dir_state > -blink_lock_range)
    {
        // steering wheel turned back: turn signal gets automatically unlocked
        setBlinkType(BLINK_NONE);
        m_blinker_autoreset = false;
    }

    // same for the right turn signal
    if (m_blink_type == BLINK_RIGHT && ar_hydro_dir_state > blink_lock_range)
        m_blinker_autoreset = true;

    if (m_blink_type == BLINK_RIGHT && m_blinker_autoreset && ar_hydro_dir_state < blink_lock_range)
    {
        setBlinkType(BLINK_NONE);
        m_blinker_autoreset = false;
    }

    bool stopblink = false;
    ar_dashboard->setBool(DD_SIGNAL_TURNLEFT, stopblink);
    ar_dashboard->setBool(DD_SIGNAL_TURNRIGHT, stopblink);
}

void Actor::ToggleCustomParticles()
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

void Actor::UpdateSoundSources()
{
#ifdef USE_OPENAL
    if (App::GetSoundScriptManager()->isDisabled())
        return;
    for (int i = 0; i < ar_num_soundsources; i++)
    {
        // TODO: Investigate segfaults after terrain reloads ~ ulteq 11/2018
        ar_soundsources[i].ssi->setPosition(ar_nodes[ar_soundsources[i].nodenum].AbsPosition, ar_nodes[ar_soundsources[i].nodenum].Velocity);
    }
    //also this, so it is updated always, and for any vehicle
    SOUND_MODULATE(ar_instance_id, SS_MOD_AIRSPEED, ar_nodes[0].Velocity.length() * 1.9438);
    SOUND_MODULATE(ar_instance_id, SS_MOD_WHEELSPEED, ar_wheel_speed * 3.6);
#endif //OPENAL
}

void Actor::updateVisual(float dt)
{
    Vector3 ref(Vector3::UNIT_Y);
    autoBlinkReset();
    UpdateSoundSources();

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

    // update exhausts
    // TODO: Move to GfxActor, don't forget dt*m_simulation_speed
    if (!m_disable_smoke && ar_engine && exhausts.size() > 0)
    {
        std::vector<Exhaust>::iterator it;
        for (it = exhausts.begin(); it != exhausts.end(); it++)
        {
            if (!it->smoker)
                continue;
            Vector3 dir = ar_nodes[it->emitterNode].AbsPosition - ar_nodes[it->directionNode].AbsPosition;
            //			dir.normalise();
            ParticleEmitter* emit = it->smoker->getEmitter(0);
            it->smokeNode->setPosition(ar_nodes[it->emitterNode].AbsPosition);
            emit->setDirection(dir);
            if (ar_engine->GetSmoke() != -1.0)
            {
                emit->setEnabled(true);
                emit->setColour(ColourValue(0.0, 0.0, 0.0, 0.02 + ar_engine->GetSmoke() * 0.06));
                emit->setTimeToLive((0.02 + ar_engine->GetSmoke() * 0.06) / 0.04);
            }
            else
            {
                emit->setEnabled(false);
            }
            emit->setParticleVelocity(1.0 + ar_engine->GetSmoke() * 2.0, 2.0 + ar_engine->GetSmoke() * 3.0);
        }
    }

    // Wings (only physics, graphics are updated in GfxActor)
    float autoaileron = 0;
    float autorudder = 0;
    float autoelevator = 0;
    if (ar_autopilot)
    {
        ar_autopilot->UpdateIls(App::GetSimTerrain()->getObjectManager()->GetLocalizers());
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
            ar_wings[i].fa->setControlDeflection(FLAP_ANGLES[ar_aerial_flap]);
        if (ar_wings[i].fa->type == 'c' || ar_wings[i].fa->type == 'V')
            ar_wings[i].fa->setControlDeflection((autoaileron + autoelevator) / 2.0);
        if (ar_wings[i].fa->type == 'd' || ar_wings[i].fa->type == 'U')
            ar_wings[i].fa->setControlDeflection((-autoaileron + autoelevator) / 2.0);
        if (ar_wings[i].fa->type == 'g')
            ar_wings[i].fa->setControlDeflection((autoaileron + FLAP_ANGLES[ar_aerial_flap]) / 2.0);
        if (ar_wings[i].fa->type == 'h')
            ar_wings[i].fa->setControlDeflection((-autoaileron + FLAP_ANGLES[ar_aerial_flap]) / 2.0);
        if (ar_wings[i].fa->type == 'i')
            ar_wings[i].fa->setControlDeflection((-autoelevator + autorudder) / 2.0);
        if (ar_wings[i].fa->type == 'j')
            ar_wings[i].fa->setControlDeflection((autoelevator + autorudder) / 2.0);
        ar_wings[i].fa->updateVerticesPhysics(); // Actual graphics update moved to GfxActor
    }
    //setup commands for hydros
    ar_hydro_aileron_command = autoaileron;
    ar_hydro_rudder_command = autorudder;
    ar_hydro_elevator_command = autoelevator;
}

void Actor::AddInterActorBeam(beam_t* beam, Actor* a, Actor* b)
{
    beam->bm_locked_actor = b;

    auto pos = std::find(ar_inter_beams.begin(), ar_inter_beams.end(), beam);
    if (pos == ar_inter_beams.end())
    {
        ar_inter_beams.push_back(beam);
    }

    std::pair<Actor*, Actor*> actor_pair(a, b);
    App::GetGameContext()->GetActorManager()->inter_actor_links[beam] = actor_pair;

    a->DetermineLinkedActors();
    for (auto actor : a->m_linked_actors)
        actor->DetermineLinkedActors();

    b->DetermineLinkedActors();
    for (auto actor : b->m_linked_actors)
        actor->DetermineLinkedActors();
}

void Actor::RemoveInterActorBeam(beam_t* beam)
{
    auto pos = std::find(ar_inter_beams.begin(), ar_inter_beams.end(), beam);
    if (pos != ar_inter_beams.end())
    {
        ar_inter_beams.erase(pos);
    }

    auto it = App::GetGameContext()->GetActorManager()->inter_actor_links.find(beam);
    if (it != App::GetGameContext()->GetActorManager()->inter_actor_links.end())
    {
        auto actor_pair = it->second;
        App::GetGameContext()->GetActorManager()->inter_actor_links.erase(it);

        actor_pair.first->DetermineLinkedActors();
        for (auto actor : actor_pair.first->m_linked_actors)
            actor->DetermineLinkedActors();

        actor_pair.second->DetermineLinkedActors();
        for (auto actor : actor_pair.second->m_linked_actors)
            actor->DetermineLinkedActors();
    }
}

void Actor::DisjoinInterActorBeams()
{
    ar_inter_beams.clear();
    auto inter_actor_links = &App::GetGameContext()->GetActorManager()->inter_actor_links;
    for (auto it = inter_actor_links->begin(); it != inter_actor_links->end();)
    {
        auto actor_pair = it->second;
        if (this == actor_pair.first || this == actor_pair.second)
        {
            it->first->bm_locked_actor = nullptr;
            it->first->bm_inter_actor = false;
            it->first->bm_disabled = true;
            inter_actor_links->erase(it++);

            actor_pair.first->DetermineLinkedActors();
            for (auto actor : actor_pair.first->m_linked_actors)
                actor->DetermineLinkedActors();

            actor_pair.second->DetermineLinkedActors();
            for (auto actor : actor_pair.second->m_linked_actors)
                actor->DetermineLinkedActors();
        }
        else
        {
            ++it;
        }
    }
}

void Actor::ToggleTies(int group)
{
    Actor* player_actor = App::GetGameContext()->GetPlayerActor();

    // untie all ties if one is tied
    bool istied = false;

    for (std::vector<tie_t>::iterator it = ar_ties.begin(); it != ar_ties.end(); it++)
    {
        // only handle ties with correct group
        if (group != -1 && (it->ti_group != -1 && it->ti_group != group))
            continue;

        // if tied, untie it. And the other way round
        if (it->ti_tied)
        {
            istied = !it->ti_beam->bm_disabled;

            // tie is locked and should get unlocked and stop tying
            it->ti_tied = false;
            it->ti_tying = false;
            if (it->ti_locked_ropable)
                it->ti_locked_ropable->attached_ties--;
            // disable the ties beam
            it->ti_beam->p2 = &ar_nodes[0];
            it->ti_beam->bm_inter_actor = false;
            it->ti_beam->bm_disabled = true;
            if (it->ti_locked_actor != this)
            {
                this->RemoveInterActorBeam(it->ti_beam);
                // update skeletonview on the untied actors
                auto linked_actors = it->ti_locked_actor->GetAllLinkedActors();
                if (!(std::find(linked_actors.begin(), linked_actors.end(), this) != linked_actors.end()))
                {
                    if (this == player_actor)
                    {
                        it->ti_locked_actor->GetGfxActor()->SetDebugView(GfxActor::DebugViewType::DEBUGVIEW_NONE);
                        for (auto actor : it->ti_locked_actor->GetAllLinkedActors())
                        {
                            actor->GetGfxActor()->SetDebugView(GfxActor::DebugViewType::DEBUGVIEW_NONE);
                        }
                    }
                    else if (it->ti_locked_actor == player_actor)
                    {
                        m_gfx_actor->SetDebugView(GfxActor::DebugViewType::DEBUGVIEW_NONE);
                        for (auto actor : this->GetAllLinkedActors())
                        {
                            actor->GetGfxActor()->SetDebugView(GfxActor::DebugViewType::DEBUGVIEW_NONE);
                        }
                    }
                }
            }
            it->ti_locked_actor = nullptr;
        }
    }

    // iterate over all ties
    if (!istied)
    {
        for (std::vector<tie_t>::iterator it = ar_ties.begin(); it != ar_ties.end(); it++)
        {
            // only handle ties with correct group
            if (group != -1 && (it->ti_group != -1 && it->ti_group != group))
                continue;

            if (!it->ti_tied)
            {
                // tie is unlocked and should get locked, search new remote ropable to lock to
                float mindist = it->ti_beam->refL;
                node_t* nearest_node = 0;
                Actor* nearest_actor = 0;
                ropable_t* locktedto = 0;
                // iterate over all actors
                for (auto actor : App::GetGameContext()->GetActorManager()->GetActors())
                {
                    if (actor->ar_sim_state == SimState::LOCAL_SLEEPING ||
                        (actor == this && it->ti_no_self_lock))
                    {
                        continue;
                    }

                    // and their ropables
                    for (std::vector<ropable_t>::iterator itr = actor->ar_ropables.begin(); itr != actor->ar_ropables.end(); itr++)
                    {
                        // if the ropable is not multilock and used, then discard this ropable
                        if (!itr->multilock && itr->attached_ties > 0)
                            continue;

                        // skip if tienode is ropable too (no selflock)
                        if (this == actor && itr->node->pos == it->ti_beam->p1->pos)
                            continue;

                        // calculate the distance and record the nearest ropable
                        float dist = (it->ti_beam->p1->AbsPosition - itr->node->AbsPosition).length();
                        if (dist < mindist)
                        {
                            mindist = dist;
                            nearest_node = itr->node;
                            nearest_actor = actor;
                            locktedto = &(*itr);
                        }
                    }
                }
                // if we found a ropable, then tie towards it
                if (nearest_node)
                {
                    // enable the beam and visually display the beam
                    it->ti_beam->bm_disabled = false;
                    // now trigger the tying action
                    it->ti_locked_actor = nearest_actor;
                    it->ti_beam->p2 = nearest_node;
                    it->ti_beam->bm_inter_actor = nearest_actor != this;
                    it->ti_beam->stress = 0;
                    it->ti_beam->L = it->ti_beam->refL;
                    it->ti_tied = true;
                    it->ti_tying = true;
                    it->ti_locked_ropable = locktedto;
                    it->ti_locked_ropable->attached_ties++;
                    if (it->ti_beam->bm_inter_actor)
                    {
                        AddInterActorBeam(it->ti_beam, this, nearest_actor);
                        // update skeletonview on the tied actors
                        if (this == player_actor)
                        {
                            nearest_actor->GetGfxActor()->SetDebugView(m_gfx_actor->GetDebugView());
                            for (auto actor : nearest_actor->GetAllLinkedActors())
                            {
                                actor->GetGfxActor()->SetDebugView(m_gfx_actor->GetDebugView());
                            }
                        }
                        else if (nearest_actor == player_actor)
                        {
                            m_gfx_actor->SetDebugView(player_actor->GetGfxActor()->GetDebugView());
                            for (auto actor : this->GetAllLinkedActors())
                            {
                                actor->GetGfxActor()->SetDebugView(player_actor->GetGfxActor()->GetDebugView());
                            }
                        }
                    }
                }
            }
        }
    }

    //ScriptEvent - Tie toggle
    TRIGGER_EVENT(SE_TRUCK_TIE_TOGGLE, ar_instance_id);
}

void Actor::ToggleRopes(int group)
{
    Actor* player_actor = App::GetGameContext()->GetPlayerActor();

    // iterate over all ropes
    for (std::vector<rope_t>::iterator it = ar_ropes.begin(); it != ar_ropes.end(); it++)
    {
        // only handle ropes with correct group
        if (group != -1 && (it->rp_group != -1 && it->rp_group != group))
            continue;

        if (it->rp_locked == LOCKED || it->rp_locked == PRELOCK)
        {
            // we unlock ropes
            it->rp_locked = UNLOCKED;
            // remove node locking
            if (it->rp_locked_ropable)
                it->rp_locked_ropable->attached_ropes--;
            if (it->rp_locked_actor != this)
            {
                this->RemoveInterActorBeam(it->rp_beam);
                // update skeletonview on the unroped actors
                auto linked_actors = it->rp_locked_actor->GetAllLinkedActors();
                if (!(std::find(linked_actors.begin(), linked_actors.end(), this) != linked_actors.end()))
                {
                    if (this == player_actor)
                    {
                        it->rp_locked_actor->GetGfxActor()->SetDebugView(GfxActor::DebugViewType::DEBUGVIEW_NONE);
                        for (auto actor : it->rp_locked_actor->GetAllLinkedActors())
                        {
                            actor->GetGfxActor()->SetDebugView(GfxActor::DebugViewType::DEBUGVIEW_NONE);
                        }
                    }
                    else if (it->rp_locked_actor == player_actor)
                    {
                        m_gfx_actor->SetDebugView(GfxActor::DebugViewType::DEBUGVIEW_NONE);
                        for (auto actor : this->GetAllLinkedActors())
                        {
                            actor->GetGfxActor()->SetDebugView(GfxActor::DebugViewType::DEBUGVIEW_NONE);
                        }
                    }
                }
            }
            it->rp_locked_actor = nullptr;
            it->rp_locked_ropable = nullptr;
        }
        else
        {
            //we lock ropes
            // search new remote ropable to lock to
            float mindist = it->rp_beam->L;
            Actor* nearest_actor = nullptr;
            ropable_t* rop = 0;
            // iterate over all actor_slots
            for (auto actor : App::GetGameContext()->GetActorManager()->GetActors())
            {
                if (actor->ar_sim_state == SimState::LOCAL_SLEEPING)
                    continue;
                // and their ropables
                for (std::vector<ropable_t>::iterator itr = actor->ar_ropables.begin(); itr != actor->ar_ropables.end(); itr++)
                {
                    // if the ropable is not multilock and used, then discard this ropable
                    if (!itr->multilock && itr->attached_ropes > 0)
                        continue;

                    // calculate the distance and record the nearest ropable
                    float dist = (it->rp_beam->p1->AbsPosition - itr->node->AbsPosition).length();
                    if (dist < mindist)
                    {
                        mindist = dist;
                        nearest_actor = actor;
                        rop = &(*itr);
                    }
                }
            }
            // if we found a ropable, then lock it
            if (nearest_actor)
            {
                //okay, we have found a rope to tie
                it->rp_locked_actor = nearest_actor;
                it->rp_locked = LOCKED;
                it->rp_locked_ropable = rop;
                it->rp_locked_ropable->attached_ropes++;
                if (nearest_actor != this)
                {
                    AddInterActorBeam(it->rp_beam, this, nearest_actor);
                    // update skeletonview on the roped up actors
                    if (this == player_actor)
                    {
                        nearest_actor->GetGfxActor()->SetDebugView(m_gfx_actor->GetDebugView());
                        for (auto actor : nearest_actor->GetAllLinkedActors())
                        {
                            actor->GetGfxActor()->SetDebugView(m_gfx_actor->GetDebugView());
                        }
                    }
                    else if (nearest_actor == player_actor)
                    {
                        m_gfx_actor->SetDebugView(player_actor->GetGfxActor()->GetDebugView());
                        for (auto actor : this->GetAllLinkedActors())
                        {
                            actor->GetGfxActor()->SetDebugView(player_actor->GetGfxActor()->GetDebugView());
                        }
                    }
                }
            }
        }
    }
}

void Actor::ToggleHooks(int group, HookAction mode, int node_number)
{
    // iterate over all hooks
    for (std::vector<hook_t>::iterator it = ar_hooks.begin(); it != ar_hooks.end(); it++)
    {
        if (mode == MOUSE_HOOK_TOGGLE && it->hk_hook_node->pos != node_number)
        {
            //skip all other nodes except the one manually toggled by mouse
            continue;
        }
        if (mode == HOOK_TOGGLE && group == -1)
        {
            //manually triggerd (EV_COMMON_LOCK). Toggle all hooks groups with group#: -1, 0, 1 ++
            if (it->hk_group <= -2)
                continue;
        }
        if (mode == HOOK_LOCK && group == -2)
        {
            //automatic lock attempt (cyclic with doupdate). Toggle all hooks groups with group#: -2, -3, -4 --, skip the ones which are not autolock (triggered only)
            if (it->hk_group >= -1 || !it->hk_autolock)
                continue;
        }
        if (mode == HOOK_UNLOCK && group == -2)
        {
            //manual unlock ALL autolock and triggerlock, do not unlock standard hooks (EV_COMMON_AUTOLOCK)
            if (it->hk_group >= -1 || !it->hk_autolock)
                continue;
        }
        if ((mode == HOOK_LOCK || mode == HOOK_UNLOCK) && group <= -3)
        {
            //trigger beam lock or unlock. Toggle one hook group with group#: group
            if (it->hk_group != group)
                continue;
        }
        if ((mode == HOOK_LOCK || mode == HOOK_UNLOCK) && group >= -1)
        {
            continue;
        }
        if (mode == HOOK_LOCK && it->hk_timer > 0.0f)
        {
            //check relock delay timer for autolock nodes and skip if not 0
            continue;
        }

        Actor* prev_locked_actor = it->hk_locked_actor; // memorize current value

        // do this only for toggle or lock attempts, skip prelocked or locked nodes for performance
        if (mode != HOOK_UNLOCK && it->hk_locked == UNLOCKED)
        {
            // we lock hooks
            // search new remote ropable to lock to
            float mindist = it->hk_lockrange;
            float distance = 100000000.0f;
            // iterate over all actor_slots
            for (auto actor : App::GetGameContext()->GetActorManager()->GetActors())
            {
                if (actor->ar_sim_state == SimState::LOCAL_SLEEPING)
                    continue;
                if (this == actor && !it->hk_selflock)
                    continue; // don't lock to self

                node_t* nearest_node = nullptr;
                for (int i = 0; i < actor->ar_num_nodes; i++)
                {
                    // skip all nodes with lockgroup 9999 (deny lock)
                    if (actor->ar_nodes[i].nd_lockgroup == 9999)
                        continue;

                    // exclude this truck and its current hooknode from the locking search
                    if (this == actor && i == it->hk_hook_node->pos)
                        continue;

                    // a lockgroup for this hooknode is set -> skip all nodes that do not have the same lockgroup (-1 = default(all nodes))
                    if (it->hk_lockgroup != -1 && it->hk_lockgroup != actor->ar_nodes[i].nd_lockgroup)
                        continue;

                    // measure distance
                    float n2n_distance = (it->hk_hook_node->AbsPosition - actor->ar_nodes[i].AbsPosition).length();
                    if (n2n_distance < mindist)
                    {
                        if (distance >= n2n_distance)
                        {
                            // located a node that is closer
                            distance = n2n_distance;
                            nearest_node = &actor->ar_nodes[i];
                        }
                    }
                }
                if (nearest_node)
                {
                    // we found a node, lock to it
                    it->hk_lock_node = nearest_node;
                    it->hk_locked_actor = actor;
                    it->hk_locked = PRELOCK;
                }
            }
        }
        // this is a locked or prelocked hook and its not a locking attempt or the locked actor was removed (bm_inter_actor == false)
        else if ((it->hk_locked == LOCKED || it->hk_locked == PRELOCK) && (mode != HOOK_LOCK || !it->hk_beam->bm_inter_actor))
        {
            // we unlock ropes
            it->hk_locked = PREUNLOCK;
            if (it->hk_group <= -2)
            {
                it->hk_timer = it->hk_timer_preset; //timer reset for autolock nodes
            }
            it->hk_lock_node = 0;
            it->hk_locked_actor = 0;
            //disable hook-assistance beam
            it->hk_beam->p2 = &ar_nodes[0];
            it->hk_beam->bm_inter_actor = false;
            it->hk_beam->L = (ar_nodes[0].AbsPosition - it->hk_hook_node->AbsPosition).length();
            it->hk_beam->bm_disabled = true;
        }

        // update skeletonview on the (un)hooked actor
        if (it->hk_locked_actor != prev_locked_actor)
        {
            if (it->hk_locked_actor)
            {
                it->hk_locked_actor->GetGfxActor()->SetDebugView(m_gfx_actor->GetDebugView());
                for (auto actor : it->hk_locked_actor->GetAllLinkedActors())
                {
                    actor->GetGfxActor()->SetDebugView(m_gfx_actor->GetDebugView());
                }
            }
            else if (prev_locked_actor != this)
            {
                prev_locked_actor->GetGfxActor()->SetDebugView(m_gfx_actor->GetDebugView());
                for (auto actor : prev_locked_actor->GetAllLinkedActors())
                {
                    actor->GetGfxActor()->SetDebugView(m_gfx_actor->GetDebugView());
                }
            }
        }
    }
}

void Actor::ToggleParkingBrake()
{
    ar_parking_brake = !ar_parking_brake;

    if (ar_parking_brake)
        SOUND_START(ar_instance_id, SS_TRIG_PARK);
    else
        SOUND_STOP(ar_instance_id, SS_TRIG_PARK);

    //ScriptEvent - Parking Brake toggle
    TRIGGER_EVENT(SE_TRUCK_PARKINGBREAK_TOGGLE, ar_instance_id);
}

void Actor::ToggleAntiLockBrake()
{
    if (!alb_notoggle)
        alb_mode = !alb_mode;
}

void Actor::ToggleTractionControl()
{
    if (!tc_notoggle)
        tc_mode = !tc_mode;
}

void Actor::ToggleBeacons()
{
    if (m_flares_mode == GfxFlaresMode::NONE)
    {
        return;
    }

    m_beacon_light_is_active = !m_beacon_light_is_active;

    //ScriptEvent - Beacon toggle
    TRIGGER_EVENT(SE_TRUCK_BEACONS_TOGGLE, ar_instance_id);
}

bool Actor::getReverseLightVisible()
{
    if (ar_sim_state == SimState::NETWORKED_OK)
        return m_net_reverse_light;

    if (ar_engine)
        return (ar_engine->GetGear() < 0);

    return m_reverse_light_active;
}

void Actor::StopAllSounds()
{
#ifdef USE_OPENAL
    for (int i = 0; i < ar_num_soundsources; i++)
    {
        if (ar_soundsources[i].ssi)
            ar_soundsources[i].ssi->setEnabled(false);
    }
#endif // USE_OPENAL
}

void Actor::UnmuteAllSounds()
{
#ifdef USE_OPENAL
    for (int i = 0; i < ar_num_soundsources; i++)
    {
        bool enabled = (ar_soundsources[i].type == -2 || ar_soundsources[i].type == ar_current_cinecam);
        ar_soundsources[i].ssi->setEnabled(enabled);
    }
#endif // USE_OPENAL
}

void Actor::NotifyActorCameraChanged()
{
    // change sound setup
#ifdef USE_OPENAL
    for (int i = 0; i < ar_num_soundsources; i++)
    {
        bool enabled = (ar_soundsources[i].type == -2 || ar_soundsources[i].type == ar_current_cinecam);
        ar_soundsources[i].ssi->setEnabled(enabled);
    }
#endif // USE_OPENAL

    // NOTE: Prop visibility now updated in GfxActor::UpdateProps() ~ only_a_ptr, 06/2018


}

//Returns the number of active (non bounded) beams connected to a node
int Actor::GetNumActiveConnectedBeams(int nodeid)
{
    int totallivebeams = 0;
    for (unsigned int ni = 0; ni < ar_node_to_beam_connections[nodeid].size(); ++ni)
    {
        if (!ar_beams[ar_node_to_beam_connections[nodeid][ni]].bm_disabled && !ar_beams[ar_node_to_beam_connections[nodeid][ni]].bounded)
            totallivebeams++;
    }
    return totallivebeams;
}

bool Actor::isTied()
{
    for (std::vector<tie_t>::iterator it = ar_ties.begin(); it != ar_ties.end(); it++)
        if (it->ti_tied)
            return true;
    return false;
}

bool Actor::isLocked()
{
    for (std::vector<hook_t>::iterator it = ar_hooks.begin(); it != ar_hooks.end(); it++)
        if (it->hk_locked == LOCKED)
            return true;
    return false;
}

void Actor::updateDashBoards(float dt)
{
    if (!ar_dashboard)
        return;
    // some temp vars
    Vector3 dir;

    // engine and gears
    if (ar_engine)
    {
        // gears first
        int gear = ar_engine->GetGear();
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
        if (cg != EngineSim::MANUALMODE)
        {
            str = ((cg == EngineSim::REAR) ? "#ffffff" : "#868686") + String("R\n");
            str += ((cg == EngineSim::NEUTRAL) ? "#ff0012" : "#8a000a") + String("N\n");
            str += ((cg == EngineSim::DRIVE) ? "#12ff00" : "#248c00") + String("D\n");
            str += ((cg == EngineSim::TWO) ? "#ffffff" : "#868686") + String("2\n");
            str += ((cg == EngineSim::ONE) ? "#ffffff" : "#868686") + String("1");
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
        float clutch = ar_engine->GetClutch();
        ar_dashboard->setFloat(DD_ENGINE_CLUTCH, clutch);

        // accelerator
        float acc = ar_engine->GetAcceleration();
        ar_dashboard->setFloat(DD_ACCELERATOR, acc);

        // RPM
        float rpm = ar_engine->GetEngineRpm();
        ar_dashboard->setFloat(DD_ENGINE_RPM, rpm);

        // turbo
        float turbo = ar_engine->GetTurboPsi() * 3.34f; // MAGIC :/
        ar_dashboard->setFloat(DD_ENGINE_TURBO, turbo);

        // ignition
        bool ign = (ar_engine->HasStarterContact() && !ar_engine->IsRunning());
        ar_dashboard->setBool(DD_ENGINE_IGNITION, ign);

        // battery
        bool batt = (ar_engine->HasStarterContact() && !ar_engine->IsRunning());
        ar_dashboard->setBool(DD_ENGINE_BATTERY, batt);

        // clutch warning
        bool cw = (fabs(ar_engine->GetTorque()) >= ar_engine->GetClutchForce() * 10.0f);
        ar_dashboard->setBool(DD_ENGINE_CLUTCH_WARNING, cw);
    }

    // brake
    ar_dashboard->setFloat(DD_BRAKE, ar_brake);

    Vector3 cam_dir  = this->GetCameraDir();
    Vector3 cam_roll = this->GetCameraRoll();

    // speedo
    float velocity = ar_nodes[0].Velocity.length();
    if (cam_dir != Vector3::ZERO)
    {
        velocity = cam_dir.dotProduct(ar_nodes[0].Velocity);
    }

    // KPH
    float cur_speed_kph = ar_wheel_speed * 3.6f;
    float smooth_kph = (cur_speed_kph * 0.3) + (ar_dashboard->_getFloat(DD_ENGINE_SPEEDO_KPH) * 0.7);
    ar_dashboard->setFloat(DD_ENGINE_SPEEDO_KPH, smooth_kph);

    // MPH
    float cur_speed_mph = ar_wheel_speed * 2.23693629f;
    float smooth_mph = (cur_speed_mph * 0.3) + (ar_dashboard->_getFloat(DD_ENGINE_SPEEDO_MPH) * 0.7);
    ar_dashboard->setFloat(DD_ENGINE_SPEEDO_MPH, smooth_mph);

    // roll
    if (cam_roll != Vector3::ZERO)
    {
        float angle = asin(cam_roll.dotProduct(Vector3::UNIT_Y));
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
    if (cam_dir != Vector3::ZERO)
    {
        float angle = asin(cam_dir.dotProduct(Vector3::UNIT_Y));
        if (angle < -1)
            angle = -1;
        if (angle > 1)
            angle = 1;

        float f = Radian(angle).valueDegrees();
        ar_dashboard->setFloat(DD_PITCH, f);
    }

    // parking brake
    ar_dashboard->setBool(DD_PARKINGBRAKE, ar_parking_brake);

    // locked lamp
    bool locked = isLocked();
    ar_dashboard->setBool(DD_LOCKED, locked);

    // low pressure lamp
    bool low_pres = !ar_engine_hydraulics_ready;
    ar_dashboard->setBool(DD_LOW_PRESSURE, low_pres);

    // lights
    bool lightsOn = (ar_lights > 0);
    ar_dashboard->setBool(DD_LIGHTS, lightsOn);

    // turn signals
    bool rightTurnOn = ar_right_blink_on || ar_warn_blink_on;
    bool leftTurnOn = ar_left_blink_on || ar_warn_blink_on;
    ar_dashboard->setBool(DD_SIGNAL_TURNRIGHT, rightTurnOn);
    ar_dashboard->setBool(DD_SIGNAL_TURNLEFT, leftTurnOn);

    // Traction Control
    if (!tc_nodash)
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
    if (!alb_nodash)
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
        if (fabs(ar_command_key[0].commandValue) > 0.000001f)
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
        float depth = this->GetHeightAboveGround();
        ar_dashboard->setFloat(DD_WATER_DEPTH, depth);

        // water speed
        Vector3 hdir = this->GetCameraDir();
        float knots = hdir.dotProduct(ar_nodes[ar_main_camera_node_pos].Velocity) * 1.9438f; // 1.943 = m/s in knots/s
        ar_dashboard->setFloat(DD_WATER_SPEED, knots);
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
            hasturbo = ar_engine->HasTurbo();
            autogearVisible = (ar_engine->getAutoShift() != EngineSim::MANUALMODE);
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

        ar_dashboard->setEnabled(DD_TRACTIONCONTROL_MODE, !tc_nodash);
        ar_dashboard->setEnabled(DD_ANTILOCKBRAKE_MODE, !alb_nodash);
        ar_dashboard->setEnabled(DD_TIES_MODE, !ar_ties.empty());
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


	if (curr_truck->aeroengines[0]->getType() == AEROENGINE_TURBOPROP_PISTONPROP)
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

	if (ftp>1 && curr_truck->aeroengines[1]->getType()==AEROENGINE_TURBOPROP_PISTONPROP)
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

	if (ftp>2 && curr_truck->aeroengines[2]->getType()==AEROENGINE_TURBOPROP_PISTONPROP)
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

	if (ftp>3 && curr_truck->aeroengines[3]->getType()==AEROENGINE_TURBOPROP_PISTONPROP)
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

void Actor::calculateLocalGForces()
{
    Vector3 cam_dir  = this->GetCameraDir();
    Vector3 cam_roll = this->GetCameraRoll();
    Vector3 cam_up   = cam_dir.crossProduct(cam_roll);

    float gravity = App::GetSimTerrain()->getGravity();

    float vertacc = m_camera_gforces.dotProduct(cam_up) + gravity;
    float longacc = m_camera_gforces.dotProduct(cam_dir);
    float latacc  = m_camera_gforces.dotProduct(cam_roll);

    m_camera_local_gforces_cur = Vector3(vertacc, longacc, latacc) / gravity;

    // Let it settle before we start recording the maximum forces
    if (m_reset_timer.getMilliseconds() > 500)
    {
        m_camera_local_gforces_max.makeCeil(-m_camera_local_gforces_cur);
        m_camera_local_gforces_max.makeCeil(+m_camera_local_gforces_cur);
    }
}

void Actor::EngineTriggerHelper(int engineNumber, EngineTriggerType type, float triggerValue)
{
    // engineNumber tells us which engine
    EngineSim* e = ar_engine; // placeholder: actors do not have multiple engines yet

    switch (type)
    {
    case TRG_ENGINE_CLUTCH:
        if (e)
            e->SetClutch(triggerValue);
        break;
    case TRG_ENGINE_BRAKE:
        ar_brake = triggerValue;
        break;
    case TRG_ENGINE_ACC:
        if (e)
            e->SetAcceleration(triggerValue);
        break;
    case TRG_ENGINE_RPM:
        // TODO: Implement setTargetRPM in the EngineSim.cpp
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

Actor::Actor(
    int actor_id,
    unsigned int vector_index,
    Truck::DocumentPtr def,
    RoR::ActorSpawnRequest rq
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
    , m_blink_type(BLINK_NONE)
    , m_blinker_autoreset(false)
    , ar_brake(0.0)
    , m_camera_gforces_accu(Ogre::Vector3::ZERO)
    , m_camera_gforces(Ogre::Vector3::ZERO)
    , m_camera_local_gforces_cur(Ogre::Vector3::ZERO)
    , m_camera_local_gforces_max(Ogre::Vector3::ZERO)
    , ar_engine_hydraulics_ready(true)
    , m_custom_particles_enabled(false)
    , ar_scale(1)
    , ar_current_cinecam(-1) // -1 = external
    , ar_dashboard(nullptr)
    , ar_disable_aerodyn_turbulent_drag(false)
    , ar_elevator(0)
    , ar_aerial_flap(0)
    , ar_fusedrag(Ogre::Vector3::ZERO)
    , ar_hydro_aileron_command(0)
    , ar_hydro_aileron_state(0)
    , ar_hydro_dir_command(0)
    , ar_hydro_dir_state(0)
    , ar_hydro_dir_wheel_display(0.0)
    , ar_hydro_elevator_command(0)
    , ar_hydro_elevator_state(0)
    , ar_hydro_rudder_command(0)
    , ar_hydro_rudder_state(0)
    , ar_nb_initialized(false)
    , ar_nb_skip_steps(0)
    , ar_nb_measure_steps(500)
    , ar_nb_optimum(7, std::numeric_limits<float>::max())
    , ar_nb_reference(7, std::numeric_limits<float>::max())
    , ar_nb_mass_scale(1.0f)
    , ar_nb_beams_scale(std::make_pair(1.0f, 1.0f))
    , ar_nb_shocks_scale(std::make_pair(1.0f, 1.0f))
    , ar_nb_wheels_scale(std::make_pair(1.0f, 1.0f))
    , ar_nb_beams_k_interval(std::make_pair(0.1f, 2.0f))
    , ar_nb_beams_d_interval(std::make_pair(0.1f, 2.0f))
    , ar_nb_shocks_k_interval(std::make_pair(0.1f, 8.0f))
    , ar_nb_shocks_d_interval(std::make_pair(0.1f, 8.0f))
    , ar_nb_wheels_k_interval(std::make_pair(1.0f, 1.0f))
    , ar_nb_wheels_d_interval(std::make_pair(1.0f, 1.0f))
    , m_inter_point_col_detector(nullptr)
    , m_intra_point_col_detector(nullptr)
    , ar_net_last_update_time(0)
    , m_avg_node_position_prev(rq.asr_position)
    , ar_left_mirror_angle(0.52)
    , ar_lights(1)
    , m_avg_node_velocity(Ogre::Vector3::ZERO)
    , ar_custom_camera_node(-1)
    , ar_main_camera_dir_corr(Ogre::Quaternion::IDENTITY)
    , m_preloaded_with_terrain(rq.asr_origin == RoR::ActorSpawnRequest::Origin::TERRN_DEF)
    , ar_net_source_id(0)
    , m_spawn_rotation(0.0)
    , ar_net_stream_id(0)
    , m_custom_light_toggle_countdown(0)
    , m_min_camera_radius(0.0f)
    , m_mouse_grab_move_force(0.0f)
    , m_mouse_grab_node(-1)
    , m_mouse_grab_pos(Ogre::Vector3::ZERO)
    , m_net_initialized(false)
    , m_net_brake_light(false)
    , m_net_label_node(0)
    , m_net_label_mt(0)
    , m_net_reverse_light(false)
    , ar_initial_total_mass(0)
    , ar_parking_brake(false)
    , ar_trailer_parking_brake(false)
    , m_avg_node_position(rq.asr_position)
    , m_previous_gear(0)
    , m_tyre_pressure(this)
    , m_replay_handler(nullptr)
    , m_reverse_light_active(false)
    , ar_right_mirror_angle(-0.52)
    , ar_rudder(0)
    , ar_update_physics(false)
    , ar_sleep_counter(0.0f)
    , m_stabilizer_shock_request(0)
    , m_stabilizer_shock_ratio(0.0)
    , m_stabilizer_shock_sleep(0.0)
    , m_total_mass(0)
    , m_water_contact(false)
    , m_water_contact_old(false)
    , m_num_axle_diffs(0)
    , m_axle_diffs{} // Init array to nullptr
    , m_num_wheel_diffs(0)
    , m_wheel_diffs{} // Init array to nullptr
    , m_has_command_beams(false)
    , m_num_command_beams(0)
    , m_load_mass(0.f)
    , m_dry_mass(0.f)
    , ar_gui_use_engine_max_rpm(false)
    , ar_autopilot(nullptr)
    , ar_is_police(false)
    , m_disable_default_sounds(false)
    , ar_engine(nullptr)
    , ar_driveable(NOT_DRIVEABLE)
    , m_skid_trails{} // Init array to nullptr
    , ar_collision_range(DEFAULT_COLLISION_RANGE)
    , ar_instance_id(actor_id)
    , ar_vector_index(vector_index)
    , ar_rescuer_flag(false)
    , m_antilockbrake(false)
    , m_tractioncontrol(false)
    , ar_forward_commands(false)
    , ar_import_commands(false)
    , ar_toggle_ties(false)
    , ar_airbrakes{} // Init array to nullptr
    , ar_cabs{} // Init array to 0
    , ar_num_cabs(0)
    , ar_num_contactable_nodes(0)
    , ar_num_contacters(0)
    , ar_screwprops{} // Init array to nullptr
    , ar_num_screwprops(0)
    , ar_num_camera_rails(0)
    , ar_aeroengines() // Zero-init array
    , ar_num_aeroengines() // Zero-init
    , ar_wheels() // array
    , ar_num_wheels() // int
    , m_avg_proped_wheel_radius(0.2f)
    , ar_filename(rq.asr_filename)
    , m_section_config(rq.asr_config)
    , m_ongoing_reset(false)
    , ar_top_speed(0.0f)
    , ar_last_fuzzy_ground_model(nullptr)
    , m_transfer_case(nullptr)
    , m_cache_entry(rq.asr_cache_entry)
    , ar_slidenodes_connect_instantly(false)
{
}

float Actor::getSteeringAngle()
{
    return ar_hydro_dir_command;
}

std::vector<authorinfo_t> Actor::getAuthors()
{
    return authors;
}

std::vector<std::string> Actor::getDescription()
{
    return description;
}

void Actor::setMass(float m)
{
    m_dry_mass = m;
}

bool Actor::getBrakeLightVisible()
{
    if (ar_sim_state == SimState::NETWORKED_OK)
        return m_net_brake_light;

    return (ar_brake > 0.01f && !ar_parking_brake);
}

bool Actor::getCustomLightVisible(int number)
{
    if (number < 0 || number > 3)
    {
        LOG("AngelScript: Invalid Light ID (" + TOSTRING(number) + "), allowed range is (0 - 4)");
        return false;
    }

    unsigned int flareID = m_net_custom_lights[number];

    return flareID < ar_flares.size() && ar_flares[flareID].controltoggle_status;
}

void Actor::setCustomLightVisible(int number, bool visible)
{
    if (number < 0 || number > 3)
    {
        LOG("AngelScript: Invalid Light ID (" + TOSTRING(number) + "), allowed range is (0 - 3)");
        return;
    }

    unsigned int flareID = m_net_custom_lights[number];

    if (flareID < ar_flares.size() && ar_flares[flareID].snode)
    {
        ar_flares[flareID].controltoggle_status = visible;
    }
}

bool Actor::getBeaconMode() // Angelscript export
{
    return m_beacon_light_is_active;
}

BlinkType Actor::getBlinkType()
{
    return m_blink_type;
}

bool Actor::getCustomParticleMode()
{
    return m_custom_particles_enabled;
}

Ogre::Real Actor::getMinimalCameraRadius()
{
    return m_min_camera_radius;
}

Replay* Actor::GetReplay()
{
    if (m_replay_handler && m_replay_handler->isValid())
        return m_replay_handler;
    else
        return nullptr;
}

Vector3 Actor::getNodePosition(int nodeNumber)
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

void Actor::WriteDiagnosticDump(std::string const& fileName)
{
    // Purpose: to diff against output from https://github.com/only-a-ptr/rigs-of-rods/tree/retro-0407
    std::stringstream buf;

    buf << "[nodes]" << std::endl;
    for (int i = 0; i < ar_num_nodes; i++)
    {
        buf 
            << "  pos:"              << std::setw(3) << ar_nodes[i].pos // indicated pos in node buffer
                                        << ((ar_nodes[i].pos != i) ? " !!sync " : "") // warn if the indicated pos doesn't match
            << " (nodes)"
            << " id:"                << std::setw(3) << ar_nodes_id[i]
            << " name:"              << std::setw(ar_nodes_name_top_length) << ar_nodes_name[i]
            << ", buoyancy:"         << std::setw(8) << ar_nodes[i].buoyancy
            << ", loaded:"           << (int)(ar_nodes[i].nd_loaded_mass)
            << " (wheels)"
            << " wheel_rim:"         << (int)ar_nodes[i].nd_rim_node
            << ", wheel_tyre:"       << (int)ar_nodes[i].nd_tyre_node
            << " (set_node_defaults)"
            << " mass:"              << std::setw(8) << ar_nodes[i].mass // param 1 load weight
            << ", friction_coef:"    << std::setw(5) << ar_nodes[i].friction_coef // param 2 friction coef
            << ", volume_coef:"      << ar_nodes[i].volume_coef // param 3 volume coef
            << ", surface_coef:"     << ar_nodes[i].surface_coef // param 4 surface coef
            << ", overrideMass:"     << ar_nodes[i].nd_override_mass // depends on param 1 load weight

            // only set by `ActorSpawner::UpdateCollcabContacterNodes()` based on collcabs
            // The 'retro-0407' equivalent is `node::contacter` set by `Beam::updateContacterNodes()` based on collcabs!
            << " (collcabs)"
            << " "                   << ar_nodes[i].nd_cab_node
            << std::endl;
    }

    buf << "[beams]" << std::endl;
    for (int i = 0; i < ar_num_beams; i++)
    {
        buf
            << "  "                  << std::setw(4) << i // actual pos in beam buffer
            << ", node1:"            << std::setw(3) << ((ar_beams[i].p1) ? ar_nodes_id[ar_beams[i].p1->pos] : -1)
            << ", node2:"            << std::setw(3) << ((ar_beams[i].p2) ? ar_nodes_id[ar_beams[i].p2->pos] : -1)
            << ", refLen:"           << std::setw(9) << ar_beams[i].refL
            << " (set_beam_defaults/scale)"
            << " spring:"            << std::setw(8) << ar_beams[i].k //param1 default_spring
            << ", damp:"             << std::setw(8) << ar_beams[i].d //param2 default_damp
            << ", default_deform:"   << std::setw(8) << ar_beams[i].default_beam_deform //param3 default_deform
            << ", strength:"         << std::setw(8) << ar_beams[i].strength //param4 default_break
                                        //param5 default_beam_diameter ~ only visual
                                        //param6 default_beam_material2 ~ only visual
            << ", plastic_coef:"     << std::setw(8) << ar_beams[i].plastic_coef //param7 default_plastic_coef
            << std::endl;
    }

    if (ar_node_to_node_connections.size() == (size_t)ar_num_nodes
        && ar_node_to_beam_connections.size() == (size_t)ar_num_nodes) // not present when dumping 'raw'
    {
        buf << "[node connections]" << std::endl;
        for (int n1 = 0; n1 < ar_num_nodes; n1++)
        {
            buf << std::setw(4) << n1 << ": nodes ";
            for (int n2: ar_node_to_node_connections[n1])
            {
                buf << n2 << " ";
            }
            buf << ", beams ";
            for (int b: ar_node_to_beam_connections[n1])
            {
                buf << b << " ";
            }
            buf << std::endl;
        }
    }

    // Write out to 'logs' using OGRE resource system - complicated, but works with Unicode paths on Windows
    Ogre::String rgName = "dumpRG";
    Ogre::ResourceGroupManager::getSingleton().addResourceLocation(
        App::sys_logs_dir->GetStr(), "FileSystem", rgName, /*recursive=*/false, /*readOnly=*/false);
    Ogre::ResourceGroupManager::getSingleton().initialiseResourceGroup(rgName);
    Ogre::DataStreamPtr outStream = Ogre::ResourceGroupManager::getSingleton().createResource(fileName, rgName, /*overwrite=*/true);
    std::string text = buf.str();
    outStream->write(text.c_str(), text.length());
    Ogre::ResourceGroupManager::getSingleton().destroyResourceGroup(rgName);
}
