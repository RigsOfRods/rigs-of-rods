/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2016-2020 Petr Ohlidal

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

#include "GfxActor.h"

#include "ApproxMath.h"
#include "AirBrake.h"
#include "Actor.h"
#include "Collisions.h"
#include "DustPool.h" // General particle gfx
#include "EngineSim.h"
#include "GfxScene.h"
#include "GUIManager.h"
#include "HydraxWater.h"
#include "FlexAirfoil.h"
#include "FlexBody.h"
#include "FlexMeshWheel.h"
#include "FlexObj.h"
#include "InputEngine.h" // TODO: Keys shouldn't be queried from here, but buffered in sim. loop ~ only_a_ptr, 06/2018
#include "MeshObject.h"
#include "MovableText.h"
#include "OgreImGui.h"
#include "Renderdash.h" // classic 'renderdash' material
#include "ActorSpawner.h"
#include "SlideNode.h"
#include "SkyManager.h"
#include "SoundScriptManager.h"
#include "TerrainManager.h"
#include "TurboJet.h"
#include "TurboProp.h"
#include "Utils.h"

#include <Ogre.h>

RoR::GfxActor::GfxActor(Actor* actor, ActorSpawner* spawner, std::string ogre_resource_group,
                        std::vector<NodeGfx>& gfx_nodes, RoR::Renderdash* renderdash):
    m_actor(actor),
    m_custom_resource_group(ogre_resource_group),
    m_vidcam_state(VideoCamState::VCSTATE_ENABLED_ONLINE),
    m_debug_view(DebugViewType::DEBUGVIEW_NONE),
    m_last_debug_view(DebugViewType::DEBUGVIEW_SKELETON),
    m_rods_parent_scenenode(nullptr),
    m_gfx_nodes(gfx_nodes),
    m_cab_scene_node(nullptr),
    m_cab_mesh(nullptr),
    m_cab_entity(nullptr),
    m_renderdash(renderdash),
    m_prop_anim_crankfactor_prev(0.f),
    m_prop_anim_shift_timer(0.f),
    m_beaconlight_active(true), // 'true' will trigger SetBeaconsEnabled(false) on the first buffer update
    m_initialized(false)
{
    // Setup particles
    m_particles_drip   = App::GetGfxScene()->GetDustPool("drip");
    m_particles_misc   = App::GetGfxScene()->GetDustPool("dust"); // Dust, water vapour, tyre smoke
    m_particles_splash = App::GetGfxScene()->GetDustPool("splash");
    m_particles_ripple = App::GetGfxScene()->GetDustPool("ripple");
    m_particles_sparks = App::GetGfxScene()->GetDustPool("sparks");
    m_particles_clump  = App::GetGfxScene()->GetDustPool("clump");

    m_simbuf.simbuf_nodes.reset(new SimBuffer::NodeSB[actor->ar_num_nodes]);
    m_simbuf.simbuf_aeroengines.resize(actor->ar_num_aeroengines);
    m_simbuf.simbuf_commandkey.resize(MAX_COMMANDS + 10);
    m_simbuf.simbuf_airbrakes.resize(spawner->GetMemoryRequirements().num_airbrakes);

    // Attributes
    m_attr.xa_speedo_highest_kph = actor->ar_speedo_max_kph; // TODO: Remove the attribute from Actor altogether ~ only_a_ptr, 05/2018
    m_attr.xa_speedo_use_engine_max_rpm = actor->ar_gui_use_engine_max_rpm; // TODO: ditto
    m_attr.xa_camera0_pos_node  = 0;
    m_attr.xa_camera0_roll_node = 0;
    m_attr.xa_has_autopilot = (actor->ar_autopilot != nullptr);
    m_attr.xa_has_engine = (actor->ar_engine != nullptr);
    m_attr.xa_driveable = actor->ar_driveable;
    if (actor->ar_num_cameras > 0)
    {
        m_attr.xa_camera0_pos_node  = actor->ar_camera_node_pos[0];
        m_attr.xa_camera0_roll_node = actor->ar_camera_node_roll[0];
    }
    if (m_attr.xa_has_engine)
    {
        m_attr.xa_num_gears = actor->ar_engine->getNumGears();
        m_attr.xa_engine_max_rpm = actor->ar_engine->getMaxRPM();
    }
}

RoR::GfxActor::~GfxActor()
{
    // Dispose videocameras
    this->SetVideoCamState(VideoCamState::VCSTATE_DISABLED);
    while (!m_videocameras.empty())
    {
        VideoCamera& vcam = m_videocameras.back();
        Ogre::TextureManager::getSingleton().remove(vcam.vcam_render_tex->getHandle());
        vcam.vcam_render_tex.setNull();
        vcam.vcam_render_target = nullptr; // Invalidated with parent texture
        App::GetGfxScene()->GetSceneManager()->destroyCamera(vcam.vcam_ogre_camera);

        m_videocameras.pop_back();
    }

    // Dispose rods
    if (m_rods_parent_scenenode != nullptr)
    {
        for (Rod& rod: m_rods)
        {
            Ogre::MovableObject* ogre_object = rod.rod_scenenode->getAttachedObject(0);
            rod.rod_scenenode->detachAllObjects();
            App::GetGfxScene()->GetSceneManager()->destroyEntity(static_cast<Ogre::Entity*>(ogre_object));
        }
        m_rods.clear();

        m_rods_parent_scenenode->removeAndDestroyAllChildren();
        App::GetGfxScene()->GetSceneManager()->destroySceneNode(m_rods_parent_scenenode);
        m_rods_parent_scenenode = nullptr;
    }

    // delete meshwheels
    for (size_t i = 0; i < m_wheels.size(); i++)
    {
        if (m_wheels[i].wx_flex_mesh != nullptr)
        {
            delete m_wheels[i].wx_flex_mesh;
        }
        if (m_wheels[i].wx_scenenode != nullptr)
        {
            m_wheels[i].wx_scenenode->removeAndDestroyAllChildren();
            App::GetGfxScene()->GetSceneManager()->destroySceneNode(m_wheels[i].wx_scenenode);
        }
    }

    // delete airbrakes
    for (AirbrakeGfx& abx: m_gfx_airbrakes)
    {
        // scene node
        abx.abx_scenenode->detachAllObjects();
        App::GetGfxScene()->GetSceneManager()->destroySceneNode(abx.abx_scenenode);
        // entity
        App::GetGfxScene()->GetSceneManager()->destroyEntity(abx.abx_entity);
        // mesh
        Ogre::MeshManager::getSingleton().remove(abx.abx_mesh);
    }
    m_gfx_airbrakes.clear();

    // Delete props
    for (Prop & prop: m_props)
    {
        for (int k = 0; k < 4; ++k)
        {
            if (prop.pp_beacon_scene_node[k])
            {
                Ogre::SceneNode* scene_node = prop.pp_beacon_scene_node[k];
                scene_node->removeAndDestroyAllChildren();
                App::GetGfxScene()->GetSceneManager()->destroySceneNode(scene_node);
            }
            if (prop.pp_beacon_light[k])
            {
                App::GetGfxScene()->GetSceneManager()->destroyLight(prop.pp_beacon_light[k]);
            }
        }

        if (prop.pp_scene_node)
        {
            prop.pp_scene_node->removeAndDestroyAllChildren();
            App::GetGfxScene()->GetSceneManager()->destroySceneNode(prop.pp_scene_node);
        }
        if (prop.pp_wheel_scene_node)
        {
            prop.pp_wheel_scene_node->removeAndDestroyAllChildren();
            App::GetGfxScene()->GetSceneManager()->destroySceneNode(prop.pp_wheel_scene_node);
        }
        if (prop.pp_mesh_obj)
        {
            delete prop.pp_mesh_obj;
        }
        if (prop.pp_wheel_mesh_obj)
        {
            delete prop.pp_wheel_mesh_obj;
        }
    }
    m_props.clear();

    // Delete flexbodies
    for (FlexBody* fb: m_flexbodies)
    {
        delete fb;
    }

    // Delete old cab mesh
    if (m_cab_mesh != nullptr)
    {
        m_cab_scene_node->detachAllObjects();
        m_cab_scene_node->getParentSceneNode()->removeAndDestroyChild(m_cab_scene_node);
        m_cab_scene_node = nullptr;

        m_cab_entity->_getManager()->destroyEntity(m_cab_entity);
        m_cab_entity = nullptr;

        delete m_cab_mesh; // Unloads the ManualMesh resource; do this last
        m_cab_mesh = nullptr;
    }

    // Delete old dashboard RTT
    if (m_renderdash != nullptr)
    {
        delete m_renderdash;
    }
}

void RoR::GfxActor::AddMaterialFlare(int flareid, Ogre::MaterialPtr m)
{
    RoR::FlareMaterial binding;
    binding.flare_index = flareid;
    binding.mat_instance = m;

    if (m.isNull())
        return;
    Ogre::Technique* tech = m->getTechnique(0);
    if (!tech)
        return;
    Ogre::Pass* p = tech->getPass(0);
    if (!p)
        return;
    // save emissive colour and then set to zero (light disabled by default)
    binding.emissive_color = p->getSelfIllumination();
    p->setSelfIllumination(Ogre::ColourValue::ZERO);

    m_flare_materials.push_back(binding);
}

void RoR::GfxActor::SetMaterialFlareOn(int flare_index, bool state_on)
{
    for (FlareMaterial& entry: m_flare_materials)
    {
        if (entry.flare_index != flare_index)
        {
            continue;
        }

        const int num_techniques = static_cast<int>(entry.mat_instance->getNumTechniques());
        for (int i = 0; i < num_techniques; i++)
        {
            Ogre::Technique* tech = entry.mat_instance->getTechnique(i);
            if (!tech)
                continue;

            if (tech->getSchemeName() == "glow")
            {
                // glowing technique
                // set the ambient value as glow amount
                Ogre::Pass* p = tech->getPass(0);
                if (!p)
                    continue;

                if (state_on)
                {
                    p->setSelfIllumination(entry.emissive_color);
                    p->setAmbient(Ogre::ColourValue::White);
                    p->setDiffuse(Ogre::ColourValue::White);
                }
                else
                {
                    p->setSelfIllumination(Ogre::ColourValue::ZERO);
                    p->setAmbient(Ogre::ColourValue::Black);
                    p->setDiffuse(Ogre::ColourValue::Black);
                }
            }
            else
            {
                // normal technique
                Ogre::Pass* p = tech->getPass(0);
                if (!p)
                    continue;

                Ogre::TextureUnitState* tus = p->getTextureUnitState(0);
                if (!tus)
                    continue;

                if (tus->getNumFrames() < 2)
                    continue;

                int frame = state_on ? 1 : 0;

                tus->setCurrentFrame(frame);

                if (state_on)
                    p->setSelfIllumination(entry.emissive_color);
                else
                    p->setSelfIllumination(Ogre::ColourValue::ZERO);
            }
        } // for each technique
    }
}

void RoR::GfxActor::RegisterCabMaterial(Ogre::MaterialPtr mat, Ogre::MaterialPtr mat_trans)
{
    // Material instances of this actor
    m_cab_mat_visual = mat;
    m_cab_mat_visual_trans = mat_trans;

    if (mat->getTechnique(0)->getNumPasses() == 1)
        return; // No emissive pass -> nothing to do.

    m_cab_mat_template_emissive = mat->clone("CabMaterialEmissive-" + mat->getName(), true, m_custom_resource_group);

    m_cab_mat_template_plain = mat->clone("CabMaterialPlain-" + mat->getName(), true, m_custom_resource_group);
    m_cab_mat_template_plain->getTechnique(0)->removePass(1);
    m_cab_mat_template_plain->compile();
}

void RoR::GfxActor::SetCabLightsActive(bool state_on)
{
    if (m_cab_mat_template_emissive.isNull()) // Both this and '_plain' are only set when emissive pass is present.
        return;

    // NOTE: Updating material in-place like this is probably inefficient,
    //       but in order to maintain all the existing material features working together,
    //       we need to avoid any material swapping on runtime. ~ only_a_ptr, 05/2017
    Ogre::MaterialPtr template_mat = (state_on) ? m_cab_mat_template_emissive : m_cab_mat_template_plain;
    Ogre::Technique* dest_tech = m_cab_mat_visual->getTechnique(0);
    Ogre::Technique* templ_tech = template_mat->getTechnique(0);
    dest_tech->removeAllPasses();
    for (unsigned short i = 0; i < templ_tech->getNumPasses(); ++i)
    {
        Ogre::Pass* templ_pass = templ_tech->getPass(i);
        Ogre::Pass* dest_pass = dest_tech->createPass();
        *dest_pass = *templ_pass; // Copy the pass! Reference: http://www.ogre3d.org/forums/viewtopic.php?f=2&t=83453
    }
    m_cab_mat_visual->compile();
}

void RoR::GfxActor::SetVideoCamState(VideoCamState state)
{
    if (state == m_vidcam_state)
    {
        return; // Nothing to do.
    }

    const bool enable = (state == VideoCamState::VCSTATE_ENABLED_ONLINE);
    for (VideoCamera vidcam: m_videocameras)
    {
        if (vidcam.vcam_render_target != nullptr)
        {
            vidcam.vcam_render_target->setActive(enable);
            if (enable)
                vidcam.vcam_material->getTechnique(0)->getPass(0)->getTextureUnitState(0)->setTextureName(vidcam.vcam_render_tex->getName());
            else
                vidcam.vcam_material->getTechnique(0)->getPass(0)->getTextureUnitState(0)->setTextureName(vidcam.vcam_off_tex_name);
            continue;
        }

        if (vidcam.vcam_render_window != nullptr)
        {
            vidcam.vcam_render_window->setActive(enable);
        }
    }
    m_vidcam_state = state;
}

void RoR::GfxActor::UpdateVideoCameras(float dt_sec)
{
    if (m_vidcam_state != VideoCamState::VCSTATE_ENABLED_ONLINE)
        return;

    for (VideoCamera& vidcam: m_videocameras)
    {
#ifdef USE_CAELUM
        // caelum needs to know that we changed the cameras
        SkyManager* sky = App::GetSimTerrain()->getSkyManager();
        if ((sky != nullptr) && (RoR::App::app_state->GetEnum<AppState>() == RoR::AppState::SIMULATION))
        {
            sky->NotifySkyCameraChanged(vidcam.vcam_ogre_camera);
        }
#endif // USE_CAELUM

        if ((vidcam.vcam_type == VCTYPE_MIRROR_PROP_LEFT)
            || (vidcam.vcam_type == VCTYPE_MIRROR_PROP_RIGHT))
        {
            // Mirror prop - special processing.
            float mirror_angle = 0.f;
            Ogre::Vector3 offset(Ogre::Vector3::ZERO);
            if (vidcam.vcam_type == VCTYPE_MIRROR_PROP_LEFT)
            {
                mirror_angle = m_actor->ar_left_mirror_angle;
                offset = Ogre::Vector3(0.07f, -0.22f, 0);
            }
            else
            {
                mirror_angle = m_actor->ar_right_mirror_angle;
                offset = Ogre::Vector3(0.07f, +0.22f, 0);
            }

            Ogre::Vector3 normal = vidcam.vcam_prop_scenenode->getOrientation()
                    * Ogre::Vector3(cos(mirror_angle), sin(mirror_angle), 0.0f);
            Ogre::Vector3 center = vidcam.vcam_prop_scenenode->getPosition()
                    + vidcam.vcam_prop_scenenode->getOrientation() * offset;
            Ogre::Radian roll = Ogre::Degree(360)
                - Ogre::Radian(asin(m_actor->getDirection().dotProduct(Ogre::Vector3::UNIT_Y)));

            Ogre::Plane plane = Ogre::Plane(normal, center);
            Ogre::Vector3 project = plane.projectVector(App::GetCameraManager()->GetCameraNode()->getPosition() - center);

            vidcam.vcam_ogre_camera->setPosition(center);
            vidcam.vcam_ogre_camera->lookAt(App::GetCameraManager()->GetCameraNode()->getPosition() - 2.0f * project);
            vidcam.vcam_ogre_camera->roll(roll);

            continue; // Done processing mirror prop.
        }

        // update the texture now, otherwise shuttering
        if (vidcam.vcam_render_target != nullptr)
            vidcam.vcam_render_target->update();

        if (vidcam.vcam_render_window != nullptr)
            vidcam.vcam_render_window->update();

        // get the normal of the camera plane now
        GfxActor::SimBuffer::NodeSB* node_buf = m_simbuf.simbuf_nodes.get();
        const Ogre::Vector3 abs_pos_center = node_buf[vidcam.vcam_node_center].AbsPosition;
        const Ogre::Vector3 abs_pos_z = node_buf[vidcam.vcam_node_dir_z].AbsPosition;
        const Ogre::Vector3 abs_pos_y = node_buf[vidcam.vcam_node_dir_y].AbsPosition;
        Ogre::Vector3 normal = (-(abs_pos_center - abs_pos_z)).crossProduct(-(abs_pos_center - abs_pos_y));
        normal.normalise();

        // add user set offset
        Ogre::Vector3 pos = node_buf[vidcam.vcam_node_alt_pos].AbsPosition +
            (vidcam.vcam_pos_offset.x * normal) +
            (vidcam.vcam_pos_offset.y * (abs_pos_center - abs_pos_y)) +
            (vidcam.vcam_pos_offset.z * (abs_pos_center - abs_pos_z));

        //avoid the camera roll
        // camup orientates to frustrum of world by default -> rotating the cam related to trucks yaw, lets bind cam rotation videocamera base (nref,ny,nz) as frustum
        // could this be done faster&better with a plane setFrustumExtents ?
        Ogre::Vector3 frustumUP = abs_pos_center - abs_pos_y;
        frustumUP.normalise();
        vidcam.vcam_ogre_camera->setFixedYawAxis(true, frustumUP);

        if (vidcam.vcam_type == VCTYPE_MIRROR)
        {
            //rotate the normal of the mirror by user rotation setting so it reflects correct
            normal = vidcam.vcam_rotation * normal;
            // merge camera direction and reflect it on our plane
            vidcam.vcam_ogre_camera->setDirection((pos - App::GetCameraManager()->GetCameraNode()->getPosition()).reflect(normal));
        }
        else if (vidcam.vcam_type == VCTYPE_VIDEOCAM)
        {
            // rotate the camera according to the nodes orientation and user rotation
            Ogre::Vector3 refx = abs_pos_z - abs_pos_center;
            refx.normalise();
            Ogre::Vector3 refy = abs_pos_center - abs_pos_y;
            refy.normalise();
            Ogre::Quaternion rot = Ogre::Quaternion(-refx, -refy, -normal);
            vidcam.vcam_ogre_camera->setOrientation(rot * vidcam.vcam_rotation); // rotate the camera orientation towards the calculated cam direction plus user rotation
        }
        else if (vidcam.vcam_type == VCTYPE_TRACKING_VIDEOCAM)
        {
            normal = node_buf[vidcam.vcam_node_lookat].AbsPosition - pos;
            normal.normalise();
            Ogre::Vector3 refx = abs_pos_z - abs_pos_center;
            refx.normalise();
            // why does this flip ~2-3� around zero orientation and only with trackercam. back to slower crossproduct calc, a bit slower but better .. sigh
            // Ogre::Vector3 refy = abs_pos_center - abs_pos_y;
            Ogre::Vector3 refy = refx.crossProduct(normal);
            refy.normalise();
            Ogre::Quaternion rot = Ogre::Quaternion(-refx, -refy, -normal);
            vidcam.vcam_ogre_camera->setOrientation(rot * vidcam.vcam_rotation); // rotate the camera orientation towards the calculated cam direction plus user rotation
        }

        if (vidcam.vcam_debug_node != nullptr)
        {
            vidcam.vcam_debug_node->setPosition(pos);
            vidcam.vcam_debug_node->setOrientation(vidcam.vcam_ogre_camera->getOrientation());
        }

        // set the new position
        vidcam.vcam_ogre_camera->setPosition(pos);
    }
}

void RoR::GfxActor::UpdateParticles(float dt_sec)
{
    float water_height = 0.f; // Unused if terrain has no water
    if (App::GetSimTerrain()->getWater() != nullptr)
    {
        water_height = App::GetSimTerrain()->getWater()->GetStaticWaterHeight();
    }

    for (NodeGfx& nfx: m_gfx_nodes)
    {
        const node_t& n = m_actor->ar_nodes[nfx.nx_node_idx];

        // 'Wet' effects - water dripping and vapour
        if (nfx.nx_may_get_wet && !nfx.nx_no_particles)
        {
            // Getting out of water?
            if (!n.nd_under_water && nfx.nx_under_water_prev)
            {
                nfx.nx_wet_time_sec = 0.f;
            }

            // Dripping water?
            if (nfx.nx_wet_time_sec != -1)
            {
                nfx.nx_wet_time_sec += dt_sec;
                if (nfx.nx_wet_time_sec > 5.f) // Dries off in 5 sec
                {
                    nfx.nx_wet_time_sec = -1.f;
                }
                else if (nfx.nx_may_get_wet)
                {
                    if (m_particles_drip != nullptr)
                    {
                        m_particles_drip->allocDrip(n.AbsPosition, n.Velocity, nfx.nx_wet_time_sec); // Dripping water particles
                    }
                    if (nfx.nx_is_hot && m_particles_misc != nullptr)
                    {
                        m_particles_misc->allocVapour(n.AbsPosition, n.Velocity, nfx.nx_wet_time_sec); // Water vapour particles
                    }
                }
            }
        }

        // Water splash and ripple
        if (n.nd_under_water && !nfx.nx_no_particles)
        {
            if ((water_height - n.AbsPosition.y < 0.2f) && (n.Velocity.squaredLength() > 4.f))
            {
                if (m_particles_splash)
                {
                    m_particles_splash->allocSplash(n.AbsPosition, n.Velocity);
                }
                if (m_particles_ripple)
                {
                    m_particles_ripple->allocRipple(n.AbsPosition, n.Velocity);     
                }
            }
        }

        // Ground collision (dust, sparks, tyre smoke, clumps...)
        if (!nfx.nx_no_particles && n.nd_has_ground_contact && n.nd_last_collision_gm != nullptr)
        {
            switch (n.nd_last_collision_gm->fx_type)
            {
            case Collisions::FX_DUSTY:
                if (m_particles_misc != nullptr)
                {
                    m_particles_misc->malloc(n.AbsPosition, n.Velocity / 2.0, n.nd_last_collision_gm->fx_colour);
                }
                break;

            case Collisions::FX_CLUMPY:
                if (m_particles_clump != nullptr && n.Velocity.squaredLength() > 1.f)
                {
                    m_particles_clump->allocClump(n.AbsPosition, n.Velocity / 2.0, n.nd_last_collision_gm->fx_colour);
                }
                break;

            case Collisions::FX_HARD:
                if (n.nd_tyre_node) // skidmarks and tyre smoke
                {
                    const float SMOKE_THRESHOLD = 8.f;
                    const float SCREECH_THRESHOLD = 5.f;
                    const float slipv = n.nd_last_collision_slip.length();
                    const float screech = std::min(slipv, n.nd_avg_collision_slip) - SCREECH_THRESHOLD;
                    if (screech > 0.0f)
                    {
                        SOUND_MODULATE(m_actor, SS_MOD_SCREETCH, screech / SCREECH_THRESHOLD);
                        SOUND_PLAY_ONCE(m_actor, SS_TRIG_SCREETCH);
                    }
                    if (m_particles_misc != nullptr && n.nd_avg_collision_slip > SMOKE_THRESHOLD)
                    {
                        m_particles_misc->allocSmoke(n.AbsPosition, n.Velocity);
                    }
                }
                else if (!nfx.nx_no_sparks) // Not a wheel => sparks
                {
                    if (m_particles_sparks != nullptr && n.nd_avg_collision_slip > 5.f)
                    {
                        if (n.nd_last_collision_slip.squaredLength() > 25.f)
                        {
                            m_particles_sparks->allocSparks(n.AbsPosition, n.Velocity);
                        }
                    }
                }
                break;

            default:;
            }
        }

        nfx.nx_under_water_prev = n.nd_under_water;
    }
}

const ImU32 BEAM_COLOR               (0xff556633); // All colors are in ABGR format (alpha, blue, green, red)
const float BEAM_THICKNESS           (1.2f);
const ImU32 BEAM_BROKEN_COLOR        (0xff4466dd);
const float BEAM_BROKEN_THICKNESS    (1.8f);
const ImU32 BEAM_HYDRO_COLOR         (0xff55a3e0);
const float BEAM_HYDRO_THICKNESS     (1.4f);
const ImU32 BEAM_STRENGTH_TEXT_COLOR (0xffcfd0cc);
const ImU32 BEAM_STRESS_TEXT_COLOR   (0xff58bbfc);
// TODO: commands cannot be distinguished on runtime

const ImU32 NODE_COLOR               (0xff44ddff);
const float NODE_RADIUS              (2.f);
const ImU32 NODE_TEXT_COLOR          (0xffcccccf); // node ID text color
const ImU32 NODE_MASS_TEXT_COLOR     (0xff77bb66);
const ImU32 NODE_IMMOVABLE_COLOR     (0xff0033ff);
const float NODE_IMMOVABLE_RADIUS    (2.8f);

void RoR::GfxActor::UpdateDebugView()
{
    if (m_debug_view == DebugViewType::DEBUGVIEW_NONE && !m_actor->ar_physics_paused)
    {
        return; // Nothing to do
    }

    // Var
    ImVec2 screen_size = ImGui::GetIO().DisplaySize;
    World2ScreenConverter world2screen(
        App::GetCameraManager()->GetCamera()->getViewMatrix(true), App::GetCameraManager()->GetCamera()->getProjectionMatrix(), Ogre::Vector2(screen_size.x, screen_size.y));

    // Dummy fullscreen window to draw to
    int window_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar| ImGuiWindowFlags_NoInputs 
                     | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoBringToFrontOnFocus;
    ImGui::SetNextWindowPos(ImVec2(0,0));
    ImGui::SetNextWindowSize(screen_size);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0,0,0,0)); // Fully transparent background!
    ImGui::Begin(("RoR-SoftBodyView-" + TOSTRING(m_actor->ar_instance_id)).c_str(), NULL, window_flags);
    ImDrawList* drawlist = ImGui::GetWindowDrawList();
    ImGui::End();
    ImGui::PopStyleColor(1); // WindowBg

    if (m_actor->ar_physics_paused && !App::GetGuiManager()->IsGuiHidden())
    {
        // Should we replace this circle with a proper bounding box?
        Ogre::Vector3 pos_xyz = world2screen.Convert(m_actor->getPosition());
        if (pos_xyz.z < 0.f)
        {
            ImVec2 pos(pos_xyz.x, pos_xyz.y);

            float radius = 0.0f;
            for (int i = 0; i < m_actor->ar_num_nodes; ++i)
            {
                radius = std::max(radius, pos_xyz.distance(world2screen.Convert(m_actor->ar_nodes[i].AbsPosition)));
            }

            drawlist->AddCircleFilled(pos, radius * 1.05f, 0x22222222, 36);
        }
    }

    // Skeleton display. NOTE: Order matters, it determines Z-ordering on render
    if ((m_debug_view == DebugViewType::DEBUGVIEW_SKELETON) ||
        (m_debug_view == DebugViewType::DEBUGVIEW_NODES) ||
        (m_debug_view == DebugViewType::DEBUGVIEW_BEAMS))
    {
        // Beams
        const beam_t* beams = m_actor->ar_beams;
        const size_t num_beams = static_cast<size_t>(m_actor->ar_num_beams);
        for (size_t i = 0; i < num_beams; ++i)
        {
            if (App::diag_hide_wheels->GetBool() &&
                    (beams[i].p1->nd_tyre_node || beams[i].p1->nd_rim_node ||
                     beams[i].p2->nd_tyre_node || beams[i].p2->nd_rim_node))
                continue;

            Ogre::Vector3 pos1 = world2screen.Convert(beams[i].p1->AbsPosition);
            Ogre::Vector3 pos2 = world2screen.Convert(beams[i].p2->AbsPosition);

            if ((pos1.z < 0.f) && (pos2.z < 0.f))
            {
                ImVec2 pos1xy(pos1.x, pos1.y);
                ImVec2 pos2xy(pos2.x, pos2.y);

                if (beams[i].bm_broken)
                {
                    if (!App::diag_hide_broken_beams->GetBool())
                    {
                        drawlist->AddLine(pos1xy, pos2xy, BEAM_BROKEN_COLOR, BEAM_BROKEN_THICKNESS);
                    }
                }
                else if (beams[i].bm_type == BEAM_HYDRO)
                {
                    if (!beams[i].bm_disabled)
                    {
                        drawlist->AddLine(pos1xy, pos2xy, BEAM_HYDRO_COLOR, BEAM_HYDRO_THICKNESS);
                    }
                }
                else
                {
                    ImU32 color = BEAM_COLOR;
                    if (!App::diag_hide_beam_stress->GetBool())
                    {
                        if (beams[i].stress > 0)
                        {
                            float stress_ratio = pow(beams[i].stress / beams[i].maxposstress, 2.0f);
                            float s = std::min(stress_ratio, 1.0f);
                            color = Ogre::ColourValue(0.2f * (1 + 2.0f * s), 0.4f * (1.0f - s), 0.33f, 1.0f).getAsABGR();
                        }
                        else if (beams[i].stress < 0)
                        {
                            float stress_ratio = pow(beams[i].stress / beams[i].maxnegstress, 2.0f);
                            float s = std::min(stress_ratio, 1.0f);
                            color = Ogre::ColourValue(0.2f, 0.4f * (1.0f - s), 0.33f * (1 + 1.0f * s), 1.0f).getAsABGR();
                        }
                    }
                    drawlist->AddLine(pos1xy, pos2xy, color, BEAM_THICKNESS);
                }
            }
        }

        if (!App::diag_hide_nodes->GetBool())
        {
            // Nodes
            const node_t* nodes = m_actor->ar_nodes;
            const size_t num_nodes = static_cast<size_t>(m_actor->ar_num_nodes);
            for (size_t i = 0; i < num_nodes; ++i)
            {
                if (App::diag_hide_wheels->GetBool() && (nodes[i].nd_tyre_node || nodes[i].nd_rim_node))
                    continue;

                Ogre::Vector3 pos_xyz = world2screen.Convert(nodes[i].AbsPosition);

                if (pos_xyz.z < 0.f)
                {
                    ImVec2 pos(pos_xyz.x, pos_xyz.y);
                    if (nodes[i].nd_immovable)
                    {
                        drawlist->AddCircleFilled(pos, NODE_IMMOVABLE_RADIUS, NODE_IMMOVABLE_COLOR);
                    }
                    else
                    {
                        drawlist->AddCircleFilled(pos, NODE_RADIUS, NODE_COLOR);
                    }
                }
            }

            // Node info; drawn after nodes to have higher Z-order
            if ((m_debug_view == DebugViewType::DEBUGVIEW_NODES) || (m_debug_view == DebugViewType::DEBUGVIEW_BEAMS))
            {
                for (size_t i = 0; i < num_nodes; ++i)
                {
                    if ((App::diag_hide_wheels->GetBool() || App::diag_hide_wheel_info->GetBool()) && 
                            (nodes[i].nd_tyre_node || nodes[i].nd_rim_node))
                        continue;

                    Ogre::Vector3 pos = world2screen.Convert(nodes[i].AbsPosition);

                    if (pos.z < 0.f)
                    {
                        ImVec2 pos_xy(pos.x, pos.y);
                        Str<25> id_buf;
                        id_buf << nodes[i].pos;
                        drawlist->AddText(pos_xy, NODE_TEXT_COLOR, id_buf.ToCStr());

                        if (m_debug_view != DebugViewType::DEBUGVIEW_BEAMS)
                        {
                            char mass_buf[50];
                            snprintf(mass_buf, 50, "|%.1fKg", nodes[i].mass);
                            ImVec2 offset = ImGui::CalcTextSize(id_buf.ToCStr());
                            drawlist->AddText(ImVec2(pos.x + offset.x, pos.y), NODE_MASS_TEXT_COLOR, mass_buf);
                        }
                    }
                }
            }
        }

        // Beam-info: drawn after beams to have higher Z-order
        if (m_debug_view == DebugViewType::DEBUGVIEW_BEAMS)
        {
            for (size_t i = 0; i < num_beams; ++i)
            {
                if ((App::diag_hide_wheels->GetBool() || App::diag_hide_wheel_info->GetBool()) &&
                        (beams[i].p1->nd_tyre_node || beams[i].p1->nd_rim_node ||
                         beams[i].p2->nd_tyre_node || beams[i].p2->nd_rim_node))
                    continue;

                // Position
                Ogre::Vector3 world_pos = (beams[i].p1->AbsPosition + beams[i].p2->AbsPosition) / 2.f;
                Ogre::Vector3 pos_xyz = world2screen.Convert(world_pos);
                if (pos_xyz.z >= 0.f)
                {
                    continue; // Behind the camera
                }
                ImVec2 pos(pos_xyz.x, pos_xyz.y);

                // Strength is usually in thousands or millions - we shorten it.
                const size_t BUF_LEN = 50;
                char buf[BUF_LEN];
                if (beams[i].strength >= 1000000000000.f)
                {
                    snprintf(buf, BUF_LEN, "%.1fT", (beams[i].strength / 1000000000000.f));
                }
                else if (beams[i].strength >= 1000000000.f)
                {
                    snprintf(buf, BUF_LEN, "%.1fG", (beams[i].strength / 1000000000.f));
                }
                else if (beams[i].strength >= 1000000.f)
                {
                    snprintf(buf, BUF_LEN, "%.1fM", (beams[i].strength / 1000000.f));
                }
                else if (beams[i].strength >= 1000.f)
                {
                    snprintf(buf, BUF_LEN, "%.1fK", (beams[i].strength / 1000.f));
                }
                else
                {
                    snprintf(buf, BUF_LEN, "%.1f", beams[i].strength);
                }
                const ImVec2 stren_text_size = ImGui::CalcTextSize(buf);
                drawlist->AddText(ImVec2(pos.x - stren_text_size.x, pos.y), BEAM_STRENGTH_TEXT_COLOR, buf);

                // Stress
                snprintf(buf, BUF_LEN, "|%.1f",  beams[i].stress);
                drawlist->AddText(pos, BEAM_STRESS_TEXT_COLOR, buf);
            }
        }
    } else if (m_debug_view == DebugViewType::DEBUGVIEW_WHEELS)
    {
        // Wheels
        const wheel_t* wheels = m_actor->ar_wheels;
        const size_t num_wheels = static_cast<size_t>(m_actor->ar_num_wheels);
        for (int i = 0; i < num_wheels; i++)
        {
            Ogre::Vector3 axis = wheels[i].wh_axis_node_1->RelPosition - wheels[i].wh_axis_node_0->RelPosition;
            axis.normalise();

            // Wheel axle
            {
                Ogre::Vector3 pos1_xyz = world2screen.Convert(wheels[i].wh_axis_node_1->AbsPosition);
                if (pos1_xyz.z < 0.f)
                {
                    ImVec2 pos(pos1_xyz.x, pos1_xyz.y);
                    drawlist->AddCircleFilled(pos, NODE_IMMOVABLE_RADIUS, NODE_COLOR);
                }
                Ogre::Vector3 pos2_xyz = world2screen.Convert(wheels[i].wh_axis_node_0->AbsPosition);
                if (pos2_xyz.z < 0.f)
                {
                    ImVec2 pos(pos2_xyz.x, pos2_xyz.y);
                    drawlist->AddCircleFilled(pos, NODE_IMMOVABLE_RADIUS, NODE_COLOR);
                }
                if ((pos1_xyz.z < 0.f) && (pos2_xyz.z < 0.f))
                {
                    ImVec2 pos1xy(pos1_xyz.x, pos1_xyz.y);
                    ImVec2 pos2xy(pos2_xyz.x, pos2_xyz.y);
                    drawlist->AddLine(pos1xy, pos2xy, BEAM_COLOR, BEAM_BROKEN_THICKNESS);
                }

                // Id, Speed, Torque
                Ogre::Vector3 pos_xyz = pos1_xyz.midPoint(pos2_xyz);
                if (pos_xyz.z < 0.f)
                {
                    float v = ImGui::GetTextLineHeightWithSpacing();
                    ImVec2 pos(pos_xyz.x, pos_xyz.y);
                    Str<25> wheel_id_buf;
                    wheel_id_buf << "Id: " << (i + 1);
                    float h1 = ImGui::CalcTextSize(wheel_id_buf.ToCStr()).x / 2.0f;
                    drawlist->AddText(ImVec2(pos.x - h1, pos.y), NODE_TEXT_COLOR, wheel_id_buf.ToCStr());
                    Str<25> rpm_buf;
                    rpm_buf << "Speed: " << static_cast<int>(Round(wheels[i].debug_rpm)) << " rpm";
                    float h2 = ImGui::CalcTextSize(rpm_buf.ToCStr()).x / 2.0f;
                    drawlist->AddText(ImVec2(pos.x - h2, pos.y + v), NODE_TEXT_COLOR, rpm_buf.ToCStr());
                    Str<25> torque_buf;
                    torque_buf << "Torque: " << static_cast<int>(Round(wheels[i].debug_torque)) << " Nm";
                    float h3 = ImGui::CalcTextSize(torque_buf.ToCStr()).x / 2.0f;
                    drawlist->AddText(ImVec2(pos.x - h3, pos.y + v + v), NODE_TEXT_COLOR, torque_buf.ToCStr());
                }
            }

            Ogre::Vector3 rradius = wheels[i].wh_arm_node->RelPosition - wheels[i].wh_near_attach_node->RelPosition;

            // Reference arm
            {
                Ogre::Vector3 pos1_xyz = world2screen.Convert(wheels[i].wh_arm_node->AbsPosition);
                if (pos1_xyz.z < 0.f)
                {
                    ImVec2 pos(pos1_xyz.x, pos1_xyz.y);
                    drawlist->AddCircleFilled(pos, NODE_IMMOVABLE_RADIUS, NODE_IMMOVABLE_COLOR);
                }
                Ogre::Vector3 pos2_xyz = world2screen.Convert(wheels[i].wh_near_attach_node->AbsPosition);
                if (pos2_xyz.z < 0.f)
                {
                    ImVec2 pos(pos2_xyz.x, pos2_xyz.y);
                    drawlist->AddCircleFilled(pos, NODE_IMMOVABLE_RADIUS, NODE_COLOR);
                }
                if ((pos1_xyz.z < 0.f) && (pos2_xyz.z < 0.f))
                {
                    ImVec2 pos1xy(pos1_xyz.x, pos1_xyz.y);
                    ImVec2 pos2xy(pos2_xyz.x, pos2_xyz.y);
                    drawlist->AddLine(pos1xy, pos2xy, BEAM_BROKEN_COLOR, BEAM_BROKEN_THICKNESS);
                }
            }

            Ogre::Vector3 radius = Ogre::Plane(axis, wheels[i].wh_near_attach_node->RelPosition).projectVector(rradius);

            // Projection plane
#if 0
            {
                Ogre::Vector3 up       = axis.crossProduct(radius);
                Ogre::Vector3 pos1_xyz = world2screen.Convert(wheels[i].wh_near_attach_node->AbsPosition + radius - up);
                Ogre::Vector3 pos2_xyz = world2screen.Convert(wheels[i].wh_near_attach_node->AbsPosition + radius + up);
                Ogre::Vector3 pos3_xyz = world2screen.Convert(wheels[i].wh_near_attach_node->AbsPosition - radius + up);
                Ogre::Vector3 pos4_xyz = world2screen.Convert(wheels[i].wh_near_attach_node->AbsPosition - radius - up);
                if ((pos1_xyz.z < 0.f) && (pos2_xyz.z < 0.f) && (pos3_xyz.z < 0.f) && (pos4_xyz.z < 0.f))
                {
                    ImVec2 pos1xy(pos1_xyz.x, pos1_xyz.y);
                    ImVec2 pos2xy(pos2_xyz.x, pos2_xyz.y);
                    ImVec2 pos3xy(pos3_xyz.x, pos3_xyz.y);
                    ImVec2 pos4xy(pos4_xyz.x, pos4_xyz.y);
                    drawlist->AddQuadFilled(pos1xy, pos2xy, pos3xy, pos4xy, 0x22888888);
                }
            }
#endif
            // Projected reference arm & error arm
            {
                Ogre::Vector3 pos1_xyz = world2screen.Convert(wheels[i].wh_near_attach_node->AbsPosition);
                Ogre::Vector3 pos2_xyz = world2screen.Convert(wheels[i].wh_near_attach_node->AbsPosition + radius);
                Ogre::Vector3 pos3_xyz = world2screen.Convert(wheels[i].wh_arm_node->AbsPosition);
                if (pos2_xyz.z < 0.f)
                {
                    ImVec2 pos(pos2_xyz.x, pos2_xyz.y);
                    drawlist->AddCircleFilled(pos, NODE_IMMOVABLE_RADIUS, 0x660033ff);
                }
                if ((pos1_xyz.z < 0.f) && (pos2_xyz.z < 0.f))
                {
                    ImVec2 pos1xy(pos1_xyz.x, pos1_xyz.y);
                    ImVec2 pos2xy(pos2_xyz.x, pos2_xyz.y);
                    drawlist->AddLine(pos1xy, pos2xy, 0x664466dd, BEAM_BROKEN_THICKNESS);
                }
                if ((pos2_xyz.z < 0.f) && (pos3_xyz.z < 0.f))
                {
                    ImVec2 pos1xy(pos2_xyz.x, pos2_xyz.y);
                    ImVec2 pos2xy(pos3_xyz.x, pos3_xyz.y);
                    drawlist->AddLine(pos1xy, pos2xy, 0x99555555, BEAM_BROKEN_THICKNESS);
                }
            }
            // Reaction torque
            {
                Ogre::Vector3 cforce = wheels[i].debug_scaled_cforce;
                {
                    Ogre::Vector3 pos1_xyz = world2screen.Convert(wheels[i].wh_arm_node->AbsPosition);
                    Ogre::Vector3 pos2_xyz = world2screen.Convert(wheels[i].wh_arm_node->AbsPosition - cforce);
                    if ((pos1_xyz.z < 0.f) && (pos2_xyz.z < 0.f))
                    {
                        ImVec2 pos1xy(pos1_xyz.x, pos1_xyz.y);
                        ImVec2 pos2xy(pos2_xyz.x, pos2_xyz.y);
                        drawlist->AddLine(pos1xy, pos2xy, 0xffcc4444, BEAM_BROKEN_THICKNESS);
                    }
                }
                {
                    Ogre::Vector3 pos1_xyz = world2screen.Convert(wheels[i].wh_near_attach_node->AbsPosition);
                    Ogre::Vector3 pos2_xyz = world2screen.Convert(wheels[i].wh_near_attach_node->AbsPosition + cforce);
                    if ((pos1_xyz.z < 0.f) && (pos2_xyz.z < 0.f))
                    {
                        ImVec2 pos1xy(pos1_xyz.x, pos1_xyz.y);
                        ImVec2 pos2xy(pos2_xyz.x, pos2_xyz.y);
                        drawlist->AddLine(pos1xy, pos2xy, 0xffcc4444, BEAM_BROKEN_THICKNESS);
                    }
                }
            }

            // Wheel slip
            {
                Ogre::Vector3 m = wheels[i].wh_axis_node_0->AbsPosition.midPoint(wheels[i].wh_axis_node_1->AbsPosition);
                Ogre::Real    w = wheels[i].wh_axis_node_0->AbsPosition.distance(m);
                Ogre::Vector3 u = - axis.crossProduct(m_simbuf.simbuf_direction);
                if (!wheels[i].debug_force.isZeroLength())
                {
                    u = - wheels[i].debug_force.normalisedCopy();
                }
                Ogre::Vector3 f = axis.crossProduct(u);
                Ogre::Vector3 a = - axis * w + f * std::max(w, wheels[i].wh_radius * 0.5f);
                Ogre::Vector3 b = + axis * w + f * std::max(w, wheels[i].wh_radius * 0.5f);
                Ogre::Vector3 c = + axis * w - f * std::max(w, wheels[i].wh_radius * 0.5f);
                Ogre::Vector3 d = - axis * w - f * std::max(w, wheels[i].wh_radius * 0.5f);
                Ogre::Quaternion r = Ogre::Quaternion::IDENTITY;
                if (wheels[i].debug_vel.length() > 1.0f)
                {
                    r = Ogre::Quaternion(f.angleBetween(wheels[i].debug_vel), u);
                }
                Ogre::Vector3 pos1_xyz = world2screen.Convert(m - u * wheels[i].wh_radius + r * a);
                Ogre::Vector3 pos2_xyz = world2screen.Convert(m - u * wheels[i].wh_radius + r * b);
                Ogre::Vector3 pos3_xyz = world2screen.Convert(m - u * wheels[i].wh_radius + r * c);
                Ogre::Vector3 pos4_xyz = world2screen.Convert(m - u * wheels[i].wh_radius + r * d);
                if ((pos1_xyz.z < 0.f) && (pos2_xyz.z < 0.f) && (pos3_xyz.z < 0.f) && (pos4_xyz.z < 0.f))
                {
                    ImVec2 pos1xy(pos1_xyz.x, pos1_xyz.y);
                    ImVec2 pos2xy(pos2_xyz.x, pos2_xyz.y);
                    ImVec2 pos3xy(pos3_xyz.x, pos3_xyz.y);
                    ImVec2 pos4xy(pos4_xyz.x, pos4_xyz.y);
                    if (!wheels[i].debug_force.isZeroLength())
                    {
                        float slipv = wheels[i].debug_slip.length();
                        float wheelv = wheels[i].debug_vel.length();
                        float slip_ratio = std::min(slipv, wheelv) / std::max(1.0f, wheelv);
                        float scale = pow(slip_ratio, 2);
                        ImU32 col = Ogre::ColourValue(scale, 1.0f - scale, 0.0f, 0.2f).getAsABGR();
                        drawlist->AddQuadFilled(pos1xy, pos2xy, pos3xy, pos4xy, col);
                    }
                    else
                    {
                        drawlist->AddQuadFilled(pos1xy, pos2xy, pos3xy, pos4xy, 0x55555555);
                    }
                }
            }

            // Slip vector
            if (!wheels[i].debug_vel.isZeroLength())
            {
                Ogre::Vector3 m = wheels[i].wh_axis_node_0->AbsPosition.midPoint(wheels[i].wh_axis_node_1->AbsPosition);
                Ogre::Real    w = wheels[i].wh_axis_node_0->AbsPosition.distance(m);
                Ogre::Vector3 d = axis.crossProduct(m_simbuf.simbuf_direction) * wheels[i].wh_radius;
                Ogre::Real slipv  = wheels[i].debug_slip.length();
                Ogre::Real wheelv = wheels[i].debug_vel.length();
                Ogre::Vector3 s = wheels[i].debug_slip * (std::min(slipv, wheelv) / std::max(1.0f, wheelv)) / slipv;
                Ogre::Vector3 pos1_xyz = world2screen.Convert(m + d);
                Ogre::Vector3 pos2_xyz = world2screen.Convert(m + d + s * std::max(w, wheels[i].wh_radius * 0.5f));
                if ((pos1_xyz.z < 0.f) && (pos2_xyz.z < 0.f))
                {
                    ImVec2 pos1xy(pos1_xyz.x, pos1_xyz.y);
                    ImVec2 pos2xy(pos2_xyz.x, pos2_xyz.y);
                    drawlist->AddLine(pos1xy, pos2xy, 0xbb4466dd, BEAM_BROKEN_THICKNESS);
                }
            }

            // Down force
            {
                Ogre::Real f = wheels[i].debug_force.length();
                Ogre::Real mass = m_actor->getTotalMass(false) * num_wheels;
                Ogre::Vector3 normalised_force = wheels[i].debug_force.normalisedCopy() * std::min(f / mass, 1.0f);
                Ogre::Vector3 m = wheels[i].wh_axis_node_0->AbsPosition.midPoint(wheels[i].wh_axis_node_1->AbsPosition);
                Ogre::Vector3 pos5_xyz = world2screen.Convert(m);
                Ogre::Vector3 pos6_xyz = world2screen.Convert(m + normalised_force * wheels[i].wh_radius);
                if ((pos5_xyz.z < 0.f) && (pos6_xyz.z < 0.f))
                {
                    ImVec2 pos1xy(pos5_xyz.x, pos5_xyz.y);
                    ImVec2 pos2xy(pos6_xyz.x, pos6_xyz.y);
                    drawlist->AddLine(pos1xy, pos2xy, 0x88888888, BEAM_BROKEN_THICKNESS);
                }
            }
        }
    } else if (m_debug_view == DebugViewType::DEBUGVIEW_SHOCKS)
    {
        // Shocks
        const beam_t* beams = m_actor->ar_beams;
        const size_t num_beams = static_cast<size_t>(m_actor->ar_num_beams);
        std::set<int> node_ids;
        for (size_t i = 0; i < num_beams; ++i)
        {
            if (beams[i].bm_type != BEAM_HYDRO)
                continue;
            if (!(beams[i].bounded == SHOCK1 || beams[i].bounded == SHOCK2 || beams[i].bounded == SHOCK3))
                continue;

            Ogre::Vector3 pos1_xyz = world2screen.Convert(beams[i].p1->AbsPosition);
            Ogre::Vector3 pos2_xyz = world2screen.Convert(beams[i].p2->AbsPosition);

            if (pos1_xyz.z < 0.f)
            {
                node_ids.insert(beams[i].p1->pos);
            }
            if (pos2_xyz.z < 0.f)
            {
                node_ids.insert(beams[i].p2->pos);
            }

            if ((pos1_xyz.z < 0.f) && (pos2_xyz.z < 0.f))
            {
                ImVec2 pos1xy(pos1_xyz.x, pos1_xyz.y);
                ImVec2 pos2xy(pos2_xyz.x, pos2_xyz.y);

                ImU32 beam_color = (beams[i].bounded == SHOCK1) ? BEAM_HYDRO_COLOR : BEAM_BROKEN_COLOR;

                drawlist->AddLine(pos1xy, pos2xy, beam_color, 1.25f * BEAM_BROKEN_THICKNESS);
            }
        }
        for (auto id : node_ids)
        {
            Ogre::Vector3 pos_xyz = world2screen.Convert(m_actor->ar_nodes[id].AbsPosition);
            if (pos_xyz.z < 0.f)
            {
                ImVec2 pos_xy(pos_xyz.x, pos_xyz.y);
                drawlist->AddCircleFilled(pos_xy, NODE_RADIUS, NODE_COLOR);
                // Node info
                Str<25> id_buf;
                id_buf << id;
                drawlist->AddText(pos_xy, NODE_TEXT_COLOR, id_buf.ToCStr());
            }
        }
        for (size_t i = 0; i < num_beams; ++i)
        {
            if (beams[i].bm_type != BEAM_HYDRO)
                continue;
            if (!(beams[i].bounded == SHOCK1 || beams[i].bounded == SHOCK2 || beams[i].bounded == SHOCK3))
                continue;

            Ogre::Vector3 pos1_xyz = world2screen.Convert(beams[i].p1->AbsPosition);
            Ogre::Vector3 pos2_xyz = world2screen.Convert(beams[i].p2->AbsPosition);
            Ogre::Vector3 pos_xyz  = pos1_xyz.midPoint(pos2_xyz);

            if (pos_xyz.z < 0.f)
            {
                // Shock info
                float diff = beams[i].p1->AbsPosition.distance(beams[i].p2->AbsPosition) - beams[i].L;
                ImU32 text_color = (diff < 0.0f) ? 0xff66ee66 : 0xff8888ff;
                float bound = (diff < 0.0f) ? beams[i].shortbound : beams[i].longbound;
                float ratio = Ogre::Math::Clamp(diff / (bound * beams[i].L), -2.0f, +2.0f);

                float v = ImGui::GetTextLineHeightWithSpacing();
                ImVec2 pos(pos_xyz.x, pos_xyz.y - v - v);
                Str<25> len_buf;
                len_buf << "L: " << static_cast<int>(Round(std::abs(ratio) * 100.0f)) << " %";
                float h1 = ImGui::CalcTextSize(len_buf.ToCStr()).x / 2.0f;
                drawlist->AddText(ImVec2(pos.x - h1, pos.y), text_color, len_buf.ToCStr());
                Str<25> spring_buf;
                spring_buf << "S: " << static_cast<int>(Round(beams[i].debug_k)) << " N";
                float h2 = ImGui::CalcTextSize(spring_buf.ToCStr()).x / 2.0f;
                drawlist->AddText(ImVec2(pos.x - h2, pos.y + v), text_color, spring_buf.ToCStr());
                Str<25> damp_buf;
                damp_buf << "D: " << static_cast<int>(Round(beams[i].debug_d)) << " N";
                float h3 = ImGui::CalcTextSize(damp_buf.ToCStr()).x / 2.0f;
                drawlist->AddText(ImVec2(pos.x - h3, pos.y + v + v), text_color, damp_buf.ToCStr());
                char vel_buf[25];
                snprintf(vel_buf, 25, "V: %.2f m/s", beams[i].debug_v);
                float h4 = ImGui::CalcTextSize(vel_buf).x / 2.0f;
                drawlist->AddText(ImVec2(pos.x - h4, pos.y + v + v + v), text_color, vel_buf);
            }
        }
    } else if (m_debug_view == DebugViewType::DEBUGVIEW_ROTATORS)
    {
        // Rotators
        const node_t* nodes = m_actor->ar_nodes;
        const rotator_t* rotators = m_actor->ar_rotators;
        const size_t num_rotators = static_cast<size_t>(m_actor->ar_num_rotators);
        for (int i = 0; i < num_rotators; i++)
        {
            Ogre::Vector3 pos1_xyz = world2screen.Convert(nodes[rotators[i].axis1].AbsPosition);
            Ogre::Vector3 pos2_xyz = world2screen.Convert(nodes[rotators[i].axis2].AbsPosition);

            // Rotator axle
            {
                if (pos1_xyz.z < 0.f)
                {
                    ImVec2 pos(pos1_xyz.x, pos1_xyz.y);
                    drawlist->AddCircleFilled(pos, 1.25f * NODE_IMMOVABLE_RADIUS, NODE_COLOR);
                    Str<25> id_buf;
                    id_buf << nodes[rotators[i].axis1].pos;
                    drawlist->AddText(pos, NODE_TEXT_COLOR, id_buf.ToCStr());
                }
                if (pos2_xyz.z < 0.f)
                {
                    ImVec2 pos(pos2_xyz.x, pos2_xyz.y);
                    drawlist->AddCircleFilled(pos, 1.25f * NODE_IMMOVABLE_RADIUS, NODE_COLOR);
                    Str<25> id_buf;
                    id_buf << nodes[rotators[i].axis2].pos;
                    drawlist->AddText(pos, NODE_TEXT_COLOR, id_buf.ToCStr());
                }
                if ((pos1_xyz.z < 0.f) && (pos2_xyz.z < 0.f))
                {
                    ImVec2 pos1xy(pos1_xyz.x, pos1_xyz.y);
                    ImVec2 pos2xy(pos2_xyz.x, pos2_xyz.y);
                    drawlist->AddLine(pos1xy, pos2xy, BEAM_COLOR, 1.25f * BEAM_BROKEN_THICKNESS);
                }

                // Id, RPM, Error
                Ogre::Vector3 pos_xyz = pos1_xyz.midPoint(pos2_xyz);
                if (pos_xyz.z < 0.f)
                {
                    float v = ImGui::GetTextLineHeightWithSpacing();
                    ImVec2 pos(pos_xyz.x, pos_xyz.y);
                    Str<25> rotator_id_buf;
                    rotator_id_buf << "Id: " << (i + 1);
                    float h1 = ImGui::CalcTextSize(rotator_id_buf.ToCStr()).x / 2.0f;
                    drawlist->AddText(ImVec2(pos.x - h1, pos.y), NODE_TEXT_COLOR, rotator_id_buf.ToCStr());
                    char angle_buf[25];
                    snprintf(angle_buf, 25, "Rate: %.1f rpm", 60.0f * rotators[i].debug_rate / Ogre::Math::TWO_PI);
                    float h2 = ImGui::CalcTextSize(angle_buf).x / 2.0f;
                    drawlist->AddText(ImVec2(pos.x - h2, pos.y + v), NODE_TEXT_COLOR, angle_buf);
                    char aerror_buf[25];
                    snprintf(aerror_buf, 25, "Error: %.1f mrad", 1000.0f * std::abs(rotators[i].debug_aerror));
                    float h3 = ImGui::CalcTextSize(aerror_buf).x / 2.0f;
                    drawlist->AddText(ImVec2(pos.x - h3, pos.y + v + v), NODE_TEXT_COLOR, aerror_buf);
                }
            }

            // Reference arms
            for (int j = 0; j < 4; j++)
            {
                // Base plate
                {
                    ImU32 node_color = Ogre::ColourValue(0.33f, 0.33f, 0.33f, j < 2 ? 1.0f : 0.5f).getAsABGR();
                    ImU32 beam_color = Ogre::ColourValue(0.33f, 0.33f, 0.33f, j < 2 ? 1.0f : 0.5f).getAsABGR();

                    Ogre::Vector3 pos3_xyz = world2screen.Convert(nodes[rotators[i].nodes1[j]].AbsPosition);
                    if (pos3_xyz.z < 0.f)
                    {
                        ImVec2 pos(pos3_xyz.x, pos3_xyz.y);
                        drawlist->AddCircleFilled(pos, NODE_RADIUS, node_color);
                        Str<25> id_buf;
                        id_buf << nodes[rotators[i].nodes1[j]].pos;
                        drawlist->AddText(pos, NODE_TEXT_COLOR, id_buf.ToCStr());
                    }
                    if ((pos1_xyz.z < 0.f) && (pos3_xyz.z < 0.f))
                    {
                        ImVec2 pos1xy(pos1_xyz.x, pos1_xyz.y);
                        ImVec2 pos2xy(pos3_xyz.x, pos3_xyz.y);
                        drawlist->AddLine(pos1xy, pos2xy, beam_color, BEAM_BROKEN_THICKNESS);
                    }
                }
                // Rotating plate
                {
                    ImU32 node_color = Ogre::ColourValue(1.00f, 0.87f, 0.27f, j < 2 ? 1.0f : 0.5f).getAsABGR();
                    ImU32 beam_color = Ogre::ColourValue(0.88f, 0.64f, 0.33f, j < 2 ? 1.0f : 0.5f).getAsABGR();

                    Ogre::Vector3 pos3_xyz = world2screen.Convert(nodes[rotators[i].nodes2[j]].AbsPosition);
                    if (pos3_xyz.z < 0.f)
                    {
                        ImVec2 pos(pos3_xyz.x, pos3_xyz.y);
                        drawlist->AddCircleFilled(pos, NODE_RADIUS, node_color);
                        Str<25> id_buf;
                        id_buf << nodes[rotators[i].nodes2[j]].pos;
                        drawlist->AddText(pos, NODE_TEXT_COLOR, id_buf.ToCStr());
                    }
                    if ((pos2_xyz.z < 0.f) && (pos3_xyz.z < 0.f))
                    {
                        ImVec2 pos1xy(pos2_xyz.x, pos2_xyz.y);
                        ImVec2 pos2xy(pos3_xyz.x, pos3_xyz.y);
                        drawlist->AddLine(pos1xy, pos2xy, beam_color, BEAM_BROKEN_THICKNESS);
                    }
                }
            }

            // Projection plane
            {
                Ogre::Vector3 mid = nodes[rotators[i].axis1].AbsPosition.midPoint(nodes[rotators[i].axis2].AbsPosition);
                Ogre::Vector3 axis = nodes[rotators[i].axis1].RelPosition - nodes[rotators[i].axis2].RelPosition;
                Ogre::Vector3 perp = axis.perpendicular(); 
                axis.normalise();

                const int steps = 64;
                Ogre::Plane plane = Ogre::Plane(axis, mid);

                Ogre::Real radius1 = 0.0f;
                Ogre::Real offset1 = 0.0f;
                for (int k = 0; k < 2; k++)
                {
                    Ogre::Vector3 r1 = nodes[rotators[i].nodes1[k]].RelPosition - nodes[rotators[i].axis1].RelPosition;
                    Ogre::Real r = plane.projectVector(r1).length();
                    if (r > radius1)
                    {
                        radius1 = r;
                        offset1 = plane.getDistance(nodes[rotators[i].nodes1[k]].AbsPosition);
                    }
                }
                std::vector<ImVec2> pos1_xy;
                for (int k = 0; k < steps; k++)
                {
                    Ogre::Quaternion rotation(Ogre::Radian(((float)k / steps) * Ogre::Math::TWO_PI), axis);
                    Ogre::Vector3 pos_xyz = world2screen.Convert(mid + axis * offset1 + rotation * perp * radius1);
                    if (pos_xyz.z < 0.f)
                    {
                        pos1_xy.push_back(ImVec2(pos_xyz.x, pos_xyz.y));
                    }
                }
                if (!pos1_xy.empty())
                {
                    drawlist->AddConvexPolyFilled(pos1_xy.data(), static_cast<int>(pos1_xy.size()), 0x33666666);
                }

                Ogre::Real radius2 = 0.0f;
                Ogre::Real offset2 = 0.0f;
                for (int k = 0; k < 2; k++)
                {
                    Ogre::Vector3 r2 = nodes[rotators[i].nodes2[k]].RelPosition - nodes[rotators[i].axis2].RelPosition;
                    Ogre::Real r = plane.projectVector(r2).length();
                    if (r > radius2)
                    {
                        radius2 = r;
                        offset2 = plane.getDistance(nodes[rotators[i].nodes2[k]].AbsPosition);
                    }
                }
                std::vector<ImVec2> pos2_xy;
                for (int k = 0; k < steps; k++)
                {
                    Ogre::Quaternion rotation(Ogre::Radian(((float)k / steps) * Ogre::Math::TWO_PI), axis);
                    Ogre::Vector3 pos_xyz = world2screen.Convert(mid + axis * offset2 + rotation * perp * radius2);
                    if (pos_xyz.z < 0.f)
                    {
                        pos2_xy.push_back(ImVec2(pos_xyz.x, pos_xyz.y));
                    }
                }
                if (!pos2_xy.empty())
                {
                    drawlist->AddConvexPolyFilled(pos2_xy.data(), static_cast<int>(pos2_xy.size()), 0x1155a3e0);
                }

                for (int k = 0; k < 2; k++)
                {
                    // Projected and rotated base plate arms (theory vectors)
                    Ogre::Vector3 ref1 = plane.projectVector(nodes[rotators[i].nodes1[k]].AbsPosition - mid);
                    Ogre::Vector3 th1 = Ogre::Quaternion(Ogre::Radian(rotators[i].angle), axis) * ref1;
                    {
                        Ogre::Vector3 pos1_xyz = world2screen.Convert(mid + axis * offset1);
                        Ogre::Vector3 pos2_xyz = world2screen.Convert(mid + axis * offset1 + th1);
                        if ((pos1_xyz.z < 0.f) && (pos2_xyz.z < 0.f))
                        {
                            ImVec2 pos1xy(pos1_xyz.x, pos1_xyz.y);
                            ImVec2 pos2xy(pos2_xyz.x, pos2_xyz.y);
                            drawlist->AddLine(pos1xy, pos2xy, 0x44888888, BEAM_BROKEN_THICKNESS);
                        }
                    }
                    // Projected rotation plate arms
                    Ogre::Vector3 ref2 = plane.projectVector(nodes[rotators[i].nodes2[k]].AbsPosition - mid);
                    {
                        Ogre::Vector3 pos1_xyz = world2screen.Convert(mid + axis * offset2);
                        Ogre::Vector3 pos2_xyz = world2screen.Convert(mid + axis * offset2 + ref2);
                        if ((pos1_xyz.z < 0.f) && (pos2_xyz.z < 0.f))
                        {
                            ImVec2 pos1xy(pos1_xyz.x, pos1_xyz.y);
                            ImVec2 pos2xy(pos2_xyz.x, pos2_xyz.y);
                            drawlist->AddLine(pos1xy, pos2xy, 0x4455a3e0, BEAM_BROKEN_THICKNESS);
                        }
                    }
                    // Virtual plate connections
                    th1.normalise();
                    Ogre::Real radius = std::min(radius1, radius2);
                    Ogre::Vector3 pos3_xyz = world2screen.Convert(mid + axis * offset1 + th1 * radius);
                    Ogre::Vector3 pos4_xyz = world2screen.Convert(mid + axis * offset2 + th1 * radius);
                    if ((pos3_xyz.z < 0.f) && (pos4_xyz.z < 0.f))
                    {
                        ImVec2 pos1xy(pos3_xyz.x, pos3_xyz.y);
                        ImVec2 pos2xy(pos4_xyz.x, pos4_xyz.y);
                        drawlist->AddLine(pos1xy, pos2xy, BEAM_COLOR, BEAM_BROKEN_THICKNESS);
                    }
                }
            }
        }
    } else if (m_debug_view == DebugViewType::DEBUGVIEW_SLIDENODES)
    {
        // Slide nodes
        const node_t* nodes = m_actor->ar_nodes;
        std::set<int> node_ids;
        for (auto railgroup : m_actor->m_railgroups)
        {
            for (auto railsegment : railgroup->rg_segments)
            {
                Ogre::Vector3 pos1 = world2screen.Convert(railsegment.rs_beam->p1->AbsPosition);
                Ogre::Vector3 pos2 = world2screen.Convert(railsegment.rs_beam->p2->AbsPosition);

                if (pos1.z < 0.f)
                {
                    node_ids.insert(railsegment.rs_beam->p1->pos);
                }
                if (pos2.z < 0.f)
                {
                    node_ids.insert(railsegment.rs_beam->p2->pos);
                }
                if ((pos1.z < 0.f) && (pos2.z < 0.f))
                {
                    ImVec2 pos1xy(pos1.x, pos1.y);
                    ImVec2 pos2xy(pos2.x, pos2.y);

                    drawlist->AddLine(pos1xy, pos2xy, BEAM_COLOR, BEAM_BROKEN_THICKNESS);
                }
            }
        }
        for (auto id : node_ids)
        {
            Ogre::Vector3 pos_xyz = world2screen.Convert(nodes[id].AbsPosition);
            if (pos_xyz.z < 0.f)
            {
                ImVec2 pos_xy(pos_xyz.x, pos_xyz.y);
                drawlist->AddCircleFilled(pos_xy, NODE_RADIUS, NODE_COLOR);
                // Node info
                Str<25> id_buf;
                id_buf << id;
                drawlist->AddText(pos_xy, NODE_TEXT_COLOR, id_buf.ToCStr());
            }
        }
        for (auto slidenode :  m_actor->m_slidenodes)
        {
            auto id = slidenode.GetSlideNodeId();
            Ogre::Vector3 pos_xyz = world2screen.Convert(nodes[id].AbsPosition);

            if (pos_xyz.z < 0.f)
            {
                ImVec2 pos(pos_xyz.x, pos_xyz.y);
                drawlist->AddCircleFilled(pos, NODE_IMMOVABLE_RADIUS, NODE_IMMOVABLE_COLOR);
                // Node info
                Str<25> id_buf;
                id_buf << id;
                drawlist->AddText(pos, NODE_TEXT_COLOR, id_buf.ToCStr());
            }
        }
    } else if (m_debug_view == DebugViewType::DEBUGVIEW_SUBMESH)
    {
        // Cabs
        const node_t* nodes = m_actor->ar_nodes;
        const auto cabs = m_actor->ar_cabs;
        const auto num_cabs = m_actor->ar_num_cabs;
        const auto buoycabs = m_actor->ar_buoycabs;
        const auto num_buoycabs = m_actor->ar_num_buoycabs;
        const auto collcabs = m_actor->ar_collcabs;
        const auto num_collcabs = m_actor->ar_num_collcabs;

        std::vector<std::pair<float, int>> render_cabs;
        for (int i = 0; i < num_cabs; i++)
        {
            Ogre::Vector3 pos1_xyz = world2screen.Convert(nodes[cabs[i*3+0]].AbsPosition);
            Ogre::Vector3 pos2_xyz = world2screen.Convert(nodes[cabs[i*3+1]].AbsPosition);
            Ogre::Vector3 pos3_xyz = world2screen.Convert(nodes[cabs[i*3+2]].AbsPosition);
            if ((pos1_xyz.z < 0.f) && (pos2_xyz.z < 0.f) && (pos3_xyz.z < 0.f))
            {
                float depth = pos1_xyz.z;
                depth = std::max(depth, pos2_xyz.z);
                depth = std::max(depth, pos3_xyz.z);
                render_cabs.push_back({depth, i});
            }
        }
        std::sort(render_cabs.begin(), render_cabs.end());

        // Cabs and contacters (which are part of a cab)
        std::vector<int> node_ids;
        for (auto render_cab : render_cabs)
        {
            int i = render_cab.second;
            bool coll = std::find(collcabs, collcabs + num_collcabs, i) != (collcabs + num_collcabs);
            bool buoy = std::find(buoycabs, buoycabs + num_buoycabs, i) != (buoycabs + num_buoycabs);

            ImU32 fill_color = Ogre::ColourValue(0.5f * coll, 0.5f * !buoy, 0.5f * (coll ^ buoy), 0.27f).getAsABGR();
            ImU32 beam_color = Ogre::ColourValue(0.5f * coll, 0.5f * !buoy, 0.5f * (coll ^ buoy), 0.53f).getAsABGR();

            Ogre::Vector3 pos1_xyz = world2screen.Convert(nodes[cabs[i*3+0]].AbsPosition);
            Ogre::Vector3 pos2_xyz = world2screen.Convert(nodes[cabs[i*3+1]].AbsPosition);
            Ogre::Vector3 pos3_xyz = world2screen.Convert(nodes[cabs[i*3+2]].AbsPosition);
            if ((pos1_xyz.z < 0.f) && (pos2_xyz.z < 0.f) && (pos3_xyz.z < 0.f))
            {
                ImVec2 pos1_xy(pos1_xyz.x, pos1_xyz.y);
                ImVec2 pos2_xy(pos2_xyz.x, pos2_xyz.y);
                ImVec2 pos3_xy(pos3_xyz.x, pos3_xyz.y);
                drawlist->AddTriangleFilled(pos1_xy, pos2_xy, pos3_xy, fill_color);
                drawlist->AddTriangle(pos1_xy, pos2_xy, pos3_xy, beam_color, BEAM_THICKNESS);
            }
            for (int k = 0; k < 3; k++)
            {
                int id = cabs[i*3+k];
                if (std::find(node_ids.begin(), node_ids.end(), id) == node_ids.end())
                {
                    Ogre::Vector3 pos_xyz = world2screen.Convert(nodes[id].AbsPosition);
                    if (pos_xyz.z < 0.f)
                    {
                        ImVec2 pos_xy(pos_xyz.x, pos_xyz.y);
                        drawlist->AddCircleFilled(pos_xy, NODE_RADIUS, nodes[id].nd_contacter ? 0xbb0033ff : 0x88888888);
                        // Node info
                        Str<25> id_buf;
                        id_buf << id;
                        drawlist->AddText(pos_xy, NODE_TEXT_COLOR, id_buf.ToCStr());
                    }
                    node_ids.push_back(id);
                }
            }
        }
    }
}

void RoR::GfxActor::ToggleDebugView()
{
    if (m_debug_view == DebugViewType::DEBUGVIEW_NONE)
        m_debug_view = m_last_debug_view;
    else
        m_debug_view = DebugViewType::DEBUGVIEW_NONE;
}

void RoR::GfxActor::SetDebugView(DebugViewType dv)
{
    if (dv == DebugViewType::DEBUGVIEW_WHEELS     && m_actor->ar_num_wheels   == 0 ||
        dv == DebugViewType::DEBUGVIEW_SHOCKS     && m_actor->ar_num_shocks   == 0 ||
        dv == DebugViewType::DEBUGVIEW_ROTATORS   && m_actor->ar_num_rotators == 0 ||
        dv == DebugViewType::DEBUGVIEW_SLIDENODES && m_actor->hasSlidenodes() == 0 ||
        dv == DebugViewType::DEBUGVIEW_SUBMESH    && m_actor->ar_num_cabs     == 0)
    {
        dv = DebugViewType::DEBUGVIEW_NONE;
    }

    m_debug_view = dv;
    if (dv != DebugViewType::DEBUGVIEW_NONE)
    {
        m_last_debug_view = dv;
    }
}

void RoR::GfxActor::CycleDebugViews()
{
    switch (m_debug_view)
    {
    case DebugViewType::DEBUGVIEW_NONE:     SetDebugView(DebugViewType::DEBUGVIEW_SKELETON); break;
    case DebugViewType::DEBUGVIEW_SKELETON: SetDebugView(DebugViewType::DEBUGVIEW_NODES);    break;
    case DebugViewType::DEBUGVIEW_NODES:    SetDebugView(DebugViewType::DEBUGVIEW_BEAMS);    break;
    case DebugViewType::DEBUGVIEW_BEAMS:
    {
        if      (m_actor->ar_num_wheels)    SetDebugView(DebugViewType::DEBUGVIEW_WHEELS);
        else if (m_actor->ar_num_shocks)    SetDebugView(DebugViewType::DEBUGVIEW_SHOCKS);
        else if (m_actor->ar_num_rotators)  SetDebugView(DebugViewType::DEBUGVIEW_ROTATORS);
        else if (m_actor->hasSlidenodes())  SetDebugView(DebugViewType::DEBUGVIEW_SLIDENODES);
        else if (m_actor->ar_num_cabs)      SetDebugView(DebugViewType::DEBUGVIEW_SUBMESH);
        else                                SetDebugView(DebugViewType::DEBUGVIEW_SKELETON);
        break;
    }
    case DebugViewType::DEBUGVIEW_WHEELS:
    {
             if (m_actor->ar_num_shocks)    SetDebugView(DebugViewType::DEBUGVIEW_SHOCKS);
        else if (m_actor->ar_num_rotators)  SetDebugView(DebugViewType::DEBUGVIEW_ROTATORS);
        else if (m_actor->hasSlidenodes())  SetDebugView(DebugViewType::DEBUGVIEW_SLIDENODES);
        else if (m_actor->ar_num_cabs)      SetDebugView(DebugViewType::DEBUGVIEW_SUBMESH);
        else                                SetDebugView(DebugViewType::DEBUGVIEW_SKELETON);
        break;
    }
    case DebugViewType::DEBUGVIEW_SHOCKS:
    {
             if (m_actor->ar_num_rotators)  SetDebugView(DebugViewType::DEBUGVIEW_ROTATORS);
        else if (m_actor->hasSlidenodes())  SetDebugView(DebugViewType::DEBUGVIEW_SLIDENODES);
        else if (m_actor->ar_num_cabs)      SetDebugView(DebugViewType::DEBUGVIEW_SUBMESH);
        else                                SetDebugView(DebugViewType::DEBUGVIEW_SKELETON);
        break;
    }
    case DebugViewType::DEBUGVIEW_ROTATORS:
    {
             if (m_actor->hasSlidenodes())  SetDebugView(DebugViewType::DEBUGVIEW_SLIDENODES);
        else if (m_actor->ar_num_cabs)      SetDebugView(DebugViewType::DEBUGVIEW_SUBMESH);
        else                                SetDebugView(DebugViewType::DEBUGVIEW_SKELETON);
        break;
    }
    case DebugViewType::DEBUGVIEW_SLIDENODES:
    {
             if (m_actor->ar_num_cabs)      SetDebugView(DebugViewType::DEBUGVIEW_SUBMESH);
        else                                SetDebugView(DebugViewType::DEBUGVIEW_SKELETON);
        break;
    }
    case DebugViewType::DEBUGVIEW_SUBMESH:  SetDebugView(DebugViewType::DEBUGVIEW_SKELETON); break;
    default:;
    }
}

void RoR::GfxActor::AddRod(int beam_index,  int node1_index, int node2_index, const char* material_name, bool visible, float diameter_meters)
{
    try
    {
        Str<100> entity_name;
        entity_name << "rod" << beam_index << "@actor" << m_actor->ar_instance_id;
        Ogre::Entity* entity = App::GetGfxScene()->GetSceneManager()->createEntity(entity_name.ToCStr(), "beam.mesh");
        entity->setMaterialName(material_name);

        if (m_rods_parent_scenenode == nullptr)
        {
            m_rods_parent_scenenode = App::GetGfxScene()->GetSceneManager()->getRootSceneNode()->createChildSceneNode();
        }

        Rod rod;
        rod.rod_scenenode = m_rods_parent_scenenode->createChildSceneNode();
        rod.rod_scenenode->attachObject(entity);
        rod.rod_scenenode->setVisible(visible, /*cascade=*/ false);

        rod.rod_scenenode->setScale(diameter_meters, -1, diameter_meters);
        rod.rod_diameter_mm = uint16_t(diameter_meters * 1000.f);

        rod.rod_beam_index = static_cast<uint16_t>(beam_index);
        rod.rod_node1 = static_cast<uint16_t>(node1_index);
        rod.rod_node2 = static_cast<uint16_t>(node2_index);
        rod.rod_target_actor = m_actor;
        rod.rod_is_visible = false;

        m_rods.push_back(rod);
    }
    catch (Ogre::Exception& e)
    {
        LogFormat("[RoR|Gfx] Failed to create visuals for beam %d, message: %s", beam_index, e.getFullDescription().c_str());
    }
}

void RoR::GfxActor::UpdateRods()
{
    for (Rod& rod: m_rods)
    {
        rod.rod_scenenode->setVisible(rod.rod_is_visible);
        if (!rod.rod_is_visible)
            continue;

        SimBuffer::NodeSB* nodes1 = this->GetSimNodeBuffer();
        Ogre::Vector3 pos1 = nodes1[rod.rod_node1].AbsPosition;
        SimBuffer::NodeSB* nodes2 = rod.rod_target_actor->GetGfxActor()->GetSimNodeBuffer();
        Ogre::Vector3 pos2 = nodes2[rod.rod_node2].AbsPosition;

        // Classic method
        float beam_diameter = static_cast<float>(rod.rod_diameter_mm) * 0.001;
        float beam_length = pos1.distance(pos2);

        rod.rod_scenenode->setPosition(pos1.midPoint(pos2));
        rod.rod_scenenode->setScale(beam_diameter, beam_length, beam_diameter);
        rod.rod_scenenode->setOrientation(GfxActor::SpecialGetRotationTo(Ogre::Vector3::UNIT_Y, (pos1 - pos2)));
    }
}

Ogre::Quaternion RoR::GfxActor::SpecialGetRotationTo(const Ogre::Vector3& src, const Ogre::Vector3& dest)
{
    // Based on Stan Melax's article in Game Programming Gems
    Ogre::Quaternion q;
    // Copy, since cannot modify local
    Ogre::Vector3 v0 = src;
    Ogre::Vector3 v1 = dest;
    v0.normalise();
    v1.normalise();

    // NB if the crossProduct approaches zero, we get unstable because ANY axis will do
    // when v0 == -v1
    Ogre::Real d = v0.dotProduct(v1);
    // If dot == 1, vectors are the same
    if (d >= 1.0f)
    {
        return Ogre::Quaternion::IDENTITY;
    }
    if (d < (1e-6f - 1.0f))
    {
        // Generate an axis
        Ogre::Vector3 axis = Ogre::Vector3::UNIT_X.crossProduct(src);
        if (axis.isZeroLength()) // pick another if colinear
            axis = Ogre::Vector3::UNIT_Y.crossProduct(src);
        axis.normalise();
        q.FromAngleAxis(Ogre::Radian(Ogre::Math::PI), axis);
    }
    else
    {
        Ogre::Real s = fast_sqrt((1 + d) * 2);
        if (s == 0)
            return Ogre::Quaternion::IDENTITY;

        Ogre::Vector3 c = v0.crossProduct(v1);
        Ogre::Real invs = 1 / s;

        q.x = c.x * invs;
        q.y = c.y * invs;
        q.z = c.z * invs;
        q.w = s * 0.5;
    }
    return q;
}

void RoR::GfxActor::ScaleActor(Ogre::Vector3 relpos, float ratio)
{
    for (Rod& rod: m_rods)
    {
        float diameter2 = static_cast<float>(rod.rod_diameter_mm) * (ratio*1000.f);
        rod.rod_diameter_mm = static_cast<uint16_t>(diameter2);
    }

    // props and stuff
    // TOFIX: care about prop positions as well!
    for (Prop& prop: m_props)
    {
        if (prop.pp_scene_node)
            prop.pp_scene_node->scale(ratio, ratio, ratio);

        if (prop.pp_wheel_scene_node)
        {
            prop.pp_wheel_scene_node->scale(ratio, ratio, ratio);
            prop.pp_wheel_pos = relpos + (prop.pp_wheel_pos - relpos) * ratio;
        }

        if (prop.pp_beacon_scene_node[0])
            prop.pp_beacon_scene_node[0]->scale(ratio, ratio, ratio);

        if (prop.pp_beacon_scene_node[1])
            prop.pp_beacon_scene_node[1]->scale(ratio, ratio, ratio);

        if (prop.pp_beacon_scene_node[2])
            prop.pp_beacon_scene_node[2]->scale(ratio, ratio, ratio);

        if (prop.pp_beacon_scene_node[3])
            prop.pp_beacon_scene_node[3]->scale(ratio, ratio, ratio);
    }

    // Old cab mesh
    if (m_cab_mesh)
    {
        m_cab_mesh->ScaleFlexObj(ratio);
    }
}

void RoR::GfxActor::SetRodsVisible(bool visible)
{
    if (m_rods_parent_scenenode == nullptr)
    {
        return; // Vehicle has no visual softbody beams -> nothing to do.
    }

    // NOTE: We don't use Ogre::SceneNode::setVisible() for performance reasons:
    //       1. That function traverses all attached Entities and updates their visibility - too much overhead
    //       2. For OGRE up to 1.9 (I don't know about 1.10+) OGRE developers recommended to detach rather than hide.
    //       ~ only_a_ptr, 12/2017
    if (visible && !m_rods_parent_scenenode->isInSceneGraph())
    {
        App::GetGfxScene()->GetSceneManager()->getRootSceneNode()->addChild(m_rods_parent_scenenode);
    }
    else if (!visible && m_rods_parent_scenenode->isInSceneGraph())
    {
        App::GetGfxScene()->GetSceneManager()->getRootSceneNode()->removeChild(m_rods_parent_scenenode);
    }
}

void RoR::GfxActor::UpdateSimDataBuffer()
{
    m_simbuf.simbuf_live_local = (m_actor->ar_sim_state == Actor::SimState::LOCAL_SIMULATED);
    m_simbuf.simbuf_physics_paused = m_actor->ar_physics_paused;
    m_simbuf.simbuf_pos = m_actor->GetRotationCenter();
    m_simbuf.simbuf_rotation = m_actor->getRotation();
    m_simbuf.simbuf_tyre_pressure = m_actor->GetTyrePressure().GetCurPressure();
    m_simbuf.simbuf_tyre_pressurizing = m_actor->GetTyrePressure().IsPressurizing();
    m_simbuf.simbuf_aabb = m_actor->ar_bounding_box;
    m_simbuf.simbuf_wheel_speed = m_actor->ar_wheel_speed;
    m_simbuf.simbuf_beaconlight_active = m_actor->m_beacon_light_is_active;
    m_simbuf.simbuf_cur_cinecam = m_actor->ar_current_cinecam;
    m_simbuf.simbuf_parking_brake = m_actor->ar_parking_brake;
    m_simbuf.simbuf_brake = m_actor->ar_brake;
    m_simbuf.simbuf_hydro_dir_state = m_actor->ar_hydro_dir_state;
    m_simbuf.simbuf_hydro_aileron_state = m_actor->ar_hydro_aileron_state;
    m_simbuf.simbuf_hydro_elevator_state = m_actor->ar_hydro_elevator_state;
    m_simbuf.simbuf_hydro_aero_rudder_state = m_actor->ar_hydro_rudder_state;
    m_simbuf.simbuf_aero_flap_state = m_actor->ar_aerial_flap;
    m_simbuf.simbuf_airbrake_state = m_actor->ar_airbrake_intensity;
    m_simbuf.simbuf_headlight_on = m_actor->ar_lights != 0;
    m_simbuf.simbuf_direction = m_actor->getDirection();
    m_simbuf.simbuf_top_speed = m_actor->ar_top_speed;
    m_simbuf.simbuf_node0_velo = m_actor->ar_nodes[0].Velocity;
    m_simbuf.simbuf_net_username = m_actor->m_net_username;
    m_simbuf.simbuf_is_remote = m_actor->ar_sim_state == Actor::SimState::NETWORKED_OK;

    // nodes
    const int num_nodes = m_actor->ar_num_nodes;
    for (int i = 0; i < num_nodes; ++i)
    {
        auto node = m_actor->ar_nodes[i];
        m_simbuf.simbuf_nodes.get()[i].AbsPosition = node.AbsPosition;
        m_simbuf.simbuf_nodes.get()[i].nd_has_contact = node.nd_has_ground_contact || node.nd_has_mesh_contact;
    }

    for (NodeGfx& nx: m_gfx_nodes)
    {
        m_simbuf.simbuf_nodes.get()[nx.nx_node_idx].nd_is_wet = (nx.nx_wet_time_sec != -1.f);
    }

    // beams
    for (Rod& rod: m_rods)
    {
        const beam_t& beam = m_actor->ar_beams[rod.rod_beam_index];
        rod.rod_node1 = static_cast<uint16_t>(beam.p1->pos);
        rod.rod_node2 = static_cast<uint16_t>(beam.p2->pos);
        if (beam.bm_inter_actor)
        {
            rod.rod_target_actor = beam.bm_locked_actor;
        }
        rod.rod_is_visible = !beam.bm_disabled && !beam.bm_broken;
    }

    // airbrakes
    const size_t num_airbrakes = m_actor->ar_airbrakes.size();
    for (size_t i=0; i< num_airbrakes; ++i)
    {
        m_simbuf.simbuf_airbrakes[i].simbuf_ab_ratio = m_actor->ar_airbrakes[i]->ratio;
    }

    // Engine (+drivetrain)
    if (m_actor->ar_engine != nullptr)
    {
        m_simbuf.simbuf_gear            = m_actor->ar_engine->GetGear();
        m_simbuf.simbuf_autoshift       = m_actor->ar_engine->getAutoShift();
        m_simbuf.simbuf_engine_rpm      = m_actor->ar_engine->GetEngineRpm();
        m_simbuf.simbuf_engine_turbo_psi= m_actor->ar_engine->GetTurboPsi();
        m_simbuf.simbuf_engine_accel    = m_actor->ar_engine->GetAcceleration();
        m_simbuf.simbuf_engine_torque   = m_actor->ar_engine->GetEngineTorque();
        m_simbuf.simbuf_inputshaft_rpm  = m_actor->ar_engine->GetInputShaftRpm();
        m_simbuf.simbuf_drive_ratio     = m_actor->ar_engine->GetDriveRatio();
        m_simbuf.simbuf_clutch          = m_actor->ar_engine->GetClutch();
    }
    if (m_actor->m_num_wheel_diffs > 0)
    {
        m_simbuf.simbuf_diff_type = m_actor->m_wheel_diffs[0]->GetActiveDiffType();
    }

    // Command keys
    const int num_commandkeys = MAX_COMMANDS + 10;
    for (int i = 0; i < num_commandkeys; ++i)
    {
        m_simbuf.simbuf_commandkey[i].simbuf_cmd_value = m_actor->ar_command_key[i].commandValue;
    }

    // Aeroengines
    for (int i = 0; i < m_actor->ar_num_aeroengines; ++i)
    {
        AeroEngine* src = m_actor->ar_aeroengines[i];
        SimBuffer::AeroEngineSB& dst = m_simbuf.simbuf_aeroengines[i];

        dst.simbuf_ae_type       = src->getType();
        dst.simbuf_ae_throttle   = src->getThrottle();
        dst.simbuf_ae_rpm        = src->getRPM();
        dst.simbuf_ae_rpmpc      = src->getRPMpc();
        dst.simbuf_ae_rpm        = src->getRPM();
        dst.simbuf_ae_ignition   = src->getIgnition();
        dst.simbuf_ae_failed     = src->isFailed();

        if (dst.simbuf_ae_type == AeroEngineType::AEROENGINE_TURBOPROP_PISTONPROP)
        {
            Turboprop* tp = static_cast<Turboprop*>(src);
            dst.simbuf_tp_aetorque = (100.0 * tp->indicated_torque / tp->max_torque); // TODO: Code ported as-is from calcAnimators(); what does it do? ~ only_a_ptr, 06/2018
            dst.simbuf_tp_aepitch = tp->pitch;
        }
        else // turbojet
        {
            Turbojet* tj = static_cast<Turbojet*>(src);
            dst.simbuf_tj_afterburn = tj->getAfterburner() != 0.f;
            dst.simbuf_tj_ab_thrust = tj->getAfterburnThrust();
            dst.simbuf_tj_exhaust_velo = tj->getExhaustVelocity();
        }
    }

    // Wings
    if (m_actor->ar_num_wings > 4)
    {
        m_simbuf.simbuf_wing4_aoa = m_actor->ar_wings[4].fa->aoa;
    }
    else
    {
        m_simbuf.simbuf_wing4_aoa = 0.f;
    }

    // Autopilot
    if (m_attr.xa_has_autopilot)
    {
        m_simbuf.simbuf_ap_heading_mode  = m_actor->ar_autopilot->GetHeadingMode();
        m_simbuf.simbuf_ap_heading_value = m_actor->ar_autopilot->heading;
        m_simbuf.simbuf_ap_alt_mode      = m_actor->ar_autopilot->GetAltMode();
        m_simbuf.simbuf_ap_alt_value     = m_actor->ar_autopilot->GetAltValue();
        m_simbuf.simbuf_ap_ias_mode      = m_actor->ar_autopilot->GetIasMode();
        m_simbuf.simbuf_ap_ias_value     = m_actor->ar_autopilot->GetIasValue();
        m_simbuf.simbuf_ap_gpws_mode     = m_actor->ar_autopilot->GetGpwsMode();
        m_simbuf.simbuf_ap_ils_available = m_actor->ar_autopilot->IsIlsAvailable();
        m_simbuf.simbuf_ap_ils_vdev      = m_actor->ar_autopilot->GetVerticalApproachDeviation();
        m_simbuf.simbuf_ap_ils_hdev      = m_actor->ar_autopilot->GetHorizontalApproachDeviation();
        m_simbuf.simbuf_ap_vs_value      = m_actor->ar_autopilot->GetVsValue();
    }

    // Linked Actors
    m_linked_gfx_actors.clear();
    for (auto actor : m_actor->GetAllLinkedActors())
    {
        m_linked_gfx_actors.insert(actor->GetGfxActor());
    }
}

bool RoR::GfxActor::IsActorLive() const
{
    return (m_actor->ar_sim_state < Actor::SimState::LOCAL_SLEEPING);
}

void RoR::GfxActor::UpdateCabMesh()
{
    if ((m_cab_entity != nullptr) && (m_cab_mesh != nullptr))
    {
        m_cab_scene_node->setPosition(m_cab_mesh->UpdateFlexObj());
    }
}

void RoR::GfxActor::SetWheelVisuals(uint16_t index, WheelGfx wheel_gfx)
{
    if (m_wheels.size() <= index)
    {
        m_wheels.resize(index + 1);
    }
    m_wheels[index] = wheel_gfx;
}

void RoR::GfxActor::UpdateWheelVisuals()
{
    m_flexwheel_tasks.clear();

    for (WheelGfx& w: m_wheels)
    {
        if (w.wx_flex_mesh != nullptr && w.wx_flex_mesh->flexitPrepare())
        {
            auto func = std::function<void()>([this, w]()
                {
                    w.wx_flex_mesh->flexitCompute();
                });
            auto task_handle = App::GetThreadPool()->RunTask(func);
            m_flexwheel_tasks.push_back(task_handle);
        }
    }
}

void RoR::GfxActor::FinishWheelUpdates()
{
    for (auto& task: m_flexwheel_tasks)
    {
        task->join();
    }
    for (WheelGfx& w: m_wheels)
    {
        if (w.wx_scenenode != nullptr && w.wx_flex_mesh != nullptr)
        {
            w.wx_scenenode->setPosition(w.wx_flex_mesh->flexitFinal());
        }
    }
}

void RoR::GfxActor::SetWheelsVisible(bool value)
{
    for (WheelGfx& w: m_wheels)
    {
        if (w.wx_scenenode != nullptr)
        {
            w.wx_scenenode->setVisible(value);
        }
        if (w.wx_flex_mesh != nullptr)
        {
            w.wx_flex_mesh->setVisible(value);
            if (w.wx_is_meshwheel)
            {
                Ogre::Entity* e = ((FlexMeshWheel*)(w.wx_flex_mesh))->getRimEntity();
                if (e != nullptr)
                {
                    e->setVisible(false);
                }
            }
        }
    }
}


int RoR::GfxActor::GetActorId          () const { return m_actor->ar_instance_id; }
int RoR::GfxActor::GetActorState       () const { return static_cast<int>(m_actor->ar_sim_state); }

ActorType RoR::GfxActor::GetActorDriveable() const
{
    return m_actor->ar_driveable;
}

void RoR::GfxActor::RegisterAirbrakes()
{
    // TODO: Quick hacky setup with `friend` access - we rely on old init code in RigSpawner/Airbrake.
    for (Airbrake* ab: m_actor->ar_airbrakes)
    {
        AirbrakeGfx abx;
        // entity
        abx.abx_entity = ab->ec;
        ab->ec = nullptr;
        // mesh
        abx.abx_mesh = ab->msh;
        ab->msh.setNull();
        // scenenode
        abx.abx_scenenode = ab->snode;
        ab->snode = nullptr;
        // offset
        abx.abx_offset = ab->offset;
        ab->offset = Ogre::Vector3::ZERO;
        // Nodes - just copy
        abx.abx_ref_node = ab->noderef->pos;
        abx.abx_x_node = ab->nodex->pos;
        abx.abx_y_node = ab->nodey->pos;

        m_gfx_airbrakes.push_back(abx);
    }
}

void RoR::GfxActor::RegisterProps(std::vector<Prop> const& props, int driverseat_prop_idx)
{
    m_props = props;
    m_driverseat_prop_index = driverseat_prop_idx;
}

void RoR::GfxActor::UpdateAirbrakes()
{
    const size_t num_airbrakes = m_gfx_airbrakes.size();
    SimBuffer::NodeSB* nodes = m_simbuf.simbuf_nodes.get();
    for (size_t i=0; i<num_airbrakes; ++i)
    {
        AirbrakeGfx abx = m_gfx_airbrakes[i];
        const float ratio = m_simbuf.simbuf_airbrakes[i].simbuf_ab_ratio;
        const float maxangle = m_actor->ar_airbrakes[i]->maxangle; // Friend access
        Ogre::Vector3 ref_node_pos = nodes[m_gfx_airbrakes[i].abx_ref_node].AbsPosition;
        Ogre::Vector3 x_node_pos   = nodes[m_gfx_airbrakes[i].abx_x_node].AbsPosition;
        Ogre::Vector3 y_node_pos   = nodes[m_gfx_airbrakes[i].abx_y_node].AbsPosition;

        // -- Ported from `AirBrake::updatePosition()` --
        Ogre::Vector3 normal = (y_node_pos - ref_node_pos).crossProduct(x_node_pos - ref_node_pos);
        normal.normalise();
        //position
        Ogre::Vector3 mposition = ref_node_pos + abx.abx_offset.x * (x_node_pos - ref_node_pos) + abx.abx_offset.y * (y_node_pos - ref_node_pos);
        abx.abx_scenenode->setPosition(mposition + normal * abx.abx_offset.z);
        //orientation
        Ogre::Vector3 refx = x_node_pos - ref_node_pos;
        refx.normalise();
        Ogre::Vector3 refy = refx.crossProduct(normal);
        Ogre::Quaternion orientation = Ogre::Quaternion(Ogre::Degree(-ratio * maxangle), (x_node_pos - ref_node_pos).normalisedCopy()) * Ogre::Quaternion(refx, normal, refy);
        abx.abx_scenenode->setOrientation(orientation);

    }
}

// TODO: Also move the data structure + setup code to GfxActor ~ only_a_ptr, 05/2018
void RoR::GfxActor::UpdateCParticles()
{
    //update custom particle systems
    SimBuffer::NodeSB* nodes = m_simbuf.simbuf_nodes.get();
    for (int i = 0; i < m_actor->ar_num_custom_particles; i++)
    {
        Ogre::Vector3 pos = nodes[m_actor->ar_custom_particles[i].emitterNode].AbsPosition;
        Ogre::Vector3 dir = pos - nodes[m_actor->ar_custom_particles[i].directionNode].AbsPosition;
        dir = fast_normalise(dir);
        m_actor->ar_custom_particles[i].snode->setPosition(pos);
        for (int j = 0; j < m_actor->ar_custom_particles[i].psys->getNumEmitters(); j++)
        {
            m_actor->ar_custom_particles[i].psys->getEmitter(j)->setDirection(dir);
        }
    }
}

void RoR::GfxActor::UpdateAeroEngines()
{
    for (int i = 0; i < m_actor->ar_num_aeroengines; i++)
    {
        m_actor->ar_aeroengines[i]->updateVisuals(this);
    }
}

void RoR::GfxActor::UpdateNetLabels(float dt)
{
    // TODO: Remake network player labels via GUI... they shouldn't be billboards inside the scene ~ only_a_ptr, 05/2018
    if (m_actor->m_net_label_node && m_actor->m_net_label_mt)
    {
        if (App::mp_hide_net_labels->GetBool() || (!m_simbuf.simbuf_is_remote && App::mp_hide_own_net_label->GetBool()))
        {
            m_actor->m_net_label_mt->setVisible(false);
            return;
        }

        float vlen = m_simbuf.simbuf_pos.distance(App::GetCameraManager()->GetCameraNode()->getPosition());

        float y_offset = (m_simbuf.simbuf_aabb.getMaximum().y - m_simbuf.simbuf_pos.y) + (vlen / 100.0);
        m_actor->m_net_label_node->setPosition(m_simbuf.simbuf_pos + Ogre::Vector3::UNIT_Y * y_offset);

        // this ensures that the nickname is always in a readable size
        m_actor->m_net_label_mt->setCharacterHeight(std::max(0.6, vlen / 40.0));

        if (vlen > 1000) // 1000 ... vlen
        {
            m_actor->m_net_label_mt->setCaption(
                m_simbuf.simbuf_net_username + " (" + TOSTRING((float)(ceil(vlen / 100) / 10.0) ) + " km)");
        }
        else if (vlen > 20) // 20 ... vlen ... 1000
        {
            m_actor->m_net_label_mt->setCaption(
                m_simbuf.simbuf_net_username + " (" + TOSTRING((int)vlen) + " m)");
        }
        else // 0 ... vlen ... 20
        {
            m_actor->m_net_label_mt->setCaption(m_simbuf.simbuf_net_username);
        }
        m_actor->m_net_label_mt->setVisible(true);
    }
}

void RoR::GfxActor::CalculateDriverPos(Ogre::Vector3& out_pos, Ogre::Quaternion& out_rot)
{
    ROR_ASSERT(m_driverseat_prop_index != -1);
    Prop* driverseat_prop = &m_props[m_driverseat_prop_index];

    SimBuffer::NodeSB* nodes = this->GetSimNodeBuffer();

    const Ogre::Vector3 x_pos = nodes[driverseat_prop->pp_node_x].AbsPosition;
    const Ogre::Vector3 y_pos = nodes[driverseat_prop->pp_node_y].AbsPosition;
    const Ogre::Vector3 center_pos = nodes[driverseat_prop->pp_node_ref].AbsPosition;

    const Ogre::Vector3 x_vec = x_pos - center_pos;
    const Ogre::Vector3 y_vec = y_pos - center_pos;
    const Ogre::Vector3 normal = (y_vec.crossProduct(x_vec)).normalisedCopy();

    // Output position
    Ogre::Vector3 pos = center_pos;
    pos += (driverseat_prop->pp_offset.x * x_vec);
    pos += (driverseat_prop->pp_offset.y * y_vec);
    pos += (driverseat_prop->pp_offset.z * normal);
    out_pos = pos;

    // Output orientation
    const Ogre::Vector3 x_vec_norm = x_vec.normalisedCopy();
    const Ogre::Vector3 y_vec_norm = x_vec_norm.crossProduct(normal);
    Ogre::Quaternion rot(x_vec_norm, normal, y_vec_norm);
    rot = rot * driverseat_prop->pp_rot;
    rot = rot * Ogre::Quaternion(Ogre::Degree(180), Ogre::Vector3::UNIT_Y); // rotate towards the driving direction
    out_rot = rot;
}

void RoR::GfxActor::UpdateBeaconFlare(Prop & prop, float dt, bool is_player_actor)
{
    // TODO: Quick and dirty port from Beam::updateFlares(), clean it up ~ only_a_ptr, 06/2018
    using namespace Ogre;

    bool enableAll = !((App::gfx_flares_mode->GetEnum<GfxFlaresMode>() == GfxFlaresMode::CURR_VEHICLE_HEAD_ONLY) && !is_player_actor);
    SimBuffer::NodeSB* nodes = this->GetSimNodeBuffer();

    if (prop.pp_beacon_type == 'b')
    {
        // Get data
        Ogre::SceneNode* beacon_scene_node = prop.pp_scene_node;
        Ogre::Quaternion beacon_orientation = beacon_scene_node->getOrientation();
        Ogre::Light* pp_beacon_light = prop.pp_beacon_light[0];
        float beacon_rotation_rate = prop.pp_beacon_rot_rate[0];
        float beacon_rotation_angle = prop.pp_beacon_rot_angle[0]; // Updated at end of block

        // Transform
        pp_beacon_light->setPosition(beacon_scene_node->getPosition() + beacon_orientation * Ogre::Vector3(0, 0, 0.12));
        beacon_rotation_angle += dt * beacon_rotation_rate;//rotate baby!
        pp_beacon_light->setDirection(beacon_orientation * Ogre::Vector3(cos(beacon_rotation_angle), sin(beacon_rotation_angle), 0));
        //billboard
        Ogre::Vector3 vdir = pp_beacon_light->getPosition() - App::GetCameraManager()->GetCameraNode()->getPosition(); // TODO: verify the position is already updated here ~ only_a_ptr, 06/2018
        float vlen = vdir.length();
        if (vlen > 100.0)
        {
            prop.pp_beacon_scene_node[0]->setVisible(false);
            return;
        }
        //normalize
        vdir = vdir / vlen;
        prop.pp_beacon_scene_node[0]->setPosition(pp_beacon_light->getPosition() - vdir * 0.1);
        float amplitude = pp_beacon_light->getDirection().dotProduct(vdir);
        if (amplitude > 0)
        {
            prop.pp_beacon_scene_node[0]->setVisible(true);
            prop.pp_beacon_bbs[0]->setDefaultDimensions(amplitude * amplitude * amplitude, amplitude * amplitude * amplitude);
        }
        else
        {
            prop.pp_beacon_scene_node[0]->setVisible(false);
        }
        pp_beacon_light->setVisible(enableAll);

        // Update
        prop.pp_beacon_rot_angle[0] = beacon_rotation_angle;
        // NOTE: Light position is not updated here!
    }
    else if (prop.pp_beacon_type == 'p')
    {
        for (int k = 0; k < 4; k++)
        {
            //update light
            Quaternion orientation = prop.pp_scene_node->getOrientation();
            switch (k)
            {
            case 0: prop.pp_beacon_light[k]->setPosition(prop.pp_scene_node->getPosition() + orientation * Vector3(-0.64, 0, 0.14));
                break;
            case 1: prop.pp_beacon_light[k]->setPosition(prop.pp_scene_node->getPosition() + orientation * Vector3(-0.32, 0, 0.14));
                break;
            case 2: prop.pp_beacon_light[k]->setPosition(prop.pp_scene_node->getPosition() + orientation * Vector3(+0.32, 0, 0.14));
                break;
            case 3: prop.pp_beacon_light[k]->setPosition(prop.pp_scene_node->getPosition() + orientation * Vector3(+0.64, 0, 0.14));
                break;
            }
            prop.pp_beacon_rot_angle[k] += dt * prop.pp_beacon_rot_rate[k];//rotate baby!
            prop.pp_beacon_light[k]->setDirection(orientation * Vector3(cos(prop.pp_beacon_rot_angle[k]), sin(prop.pp_beacon_rot_angle[k]), 0));
            //billboard
            Vector3 vdir = prop.pp_beacon_light[k]->getPosition() - App::GetCameraManager()->GetCameraNode()->getPosition();
            float vlen = vdir.length();
            if (vlen > 100.0)
            {
                prop.pp_beacon_scene_node[k]->setVisible(false);
                continue;
            }
            //normalize
            vdir = vdir / vlen;
            prop.pp_beacon_scene_node[k]->setPosition(prop.pp_beacon_light[k]->getPosition() - vdir * 0.2);
            float amplitude = prop.pp_beacon_light[k]->getDirection().dotProduct(vdir);
            if (amplitude > 0)
            {
                prop.pp_beacon_scene_node[k]->setVisible(true);
                prop.pp_beacon_bbs[k]->setDefaultDimensions(amplitude * amplitude * amplitude, amplitude * amplitude * amplitude);
            }
            else
            {
                prop.pp_beacon_scene_node[k]->setVisible(false);
            }
            prop.pp_beacon_light[k]->setVisible(enableAll);
        }
    }
    else if (prop.pp_beacon_type == 'r')
    {
        //update light
        Quaternion orientation = prop.pp_scene_node->getOrientation();
        prop.pp_beacon_light[0]->setPosition(prop.pp_scene_node->getPosition() + orientation * Vector3(0, 0, 0.06));
        prop.pp_beacon_rot_angle[0] += dt * prop.pp_beacon_rot_rate[0];//rotate baby!
        //billboard
        Vector3 vdir = prop.pp_beacon_light[0]->getPosition() - App::GetCameraManager()->GetCameraNode()->getPosition();
        float vlen = vdir.length();
        if (vlen > 100.0)
        {
            prop.pp_beacon_scene_node[0]->setVisible(false);
            return;
        }
        //normalize
        vdir = vdir / vlen;
        prop.pp_beacon_scene_node[0]->setPosition(prop.pp_beacon_light[0]->getPosition() - vdir * 0.1);
        bool visible = false;
        if (prop.pp_beacon_rot_angle[0] > 1.0)
        {
            prop.pp_beacon_rot_angle[0] = 0.0;
            visible = true;
        }
        visible = visible && enableAll;
        prop.pp_beacon_light[0]->setVisible(visible);
        prop.pp_beacon_scene_node[0]->setVisible(visible);
    }
    else if (prop.pp_beacon_type == 'R' || prop.pp_beacon_type == 'L') // Avionic navigation lights (red/green)
    {
        Vector3 mposition = nodes[prop.pp_node_ref].AbsPosition + prop.pp_offset.x * (nodes[prop.pp_node_x].AbsPosition - nodes[prop.pp_node_ref].AbsPosition) + prop.pp_offset.y * (nodes[prop.pp_node_y].AbsPosition - nodes[prop.pp_node_ref].AbsPosition);
        //billboard
        Vector3 vdir = mposition - App::GetCameraManager()->GetCameraNode()->getPosition();
        float vlen = vdir.length();
        if (vlen > 100.0)
        {
            prop.pp_beacon_scene_node[0]->setVisible(false);
            return;
        }
        //normalize
        vdir = vdir / vlen;
        prop.pp_beacon_scene_node[0]->setPosition(mposition - vdir * 0.1);
    }
    else if (prop.pp_beacon_type == 'w') // Avionic navigation lights (white rotating beacon)
    {
        Vector3 mposition = nodes[prop.pp_node_ref].AbsPosition + prop.pp_offset.x * (nodes[prop.pp_node_x].AbsPosition - nodes[prop.pp_node_ref].AbsPosition) + prop.pp_offset.y * (nodes[prop.pp_node_y].AbsPosition - nodes[prop.pp_node_ref].AbsPosition);
        prop.pp_beacon_light[0]->setPosition(mposition);
        prop.pp_beacon_rot_angle[0] += dt * prop.pp_beacon_rot_rate[0];//rotate baby!
        //billboard
        Vector3 vdir = mposition - App::GetCameraManager()->GetCameraNode()->getPosition();
        float vlen = vdir.length();
        if (vlen > 100.0)
        {
            prop.pp_beacon_scene_node[0]->setVisible(false);
            return;
        }
        //normalize
        vdir = vdir / vlen;
        prop.pp_beacon_scene_node[0]->setPosition(mposition - vdir * 0.1);
        bool visible = false;
        if (prop.pp_beacon_rot_angle[0] > 1.0)
        {
            prop.pp_beacon_rot_angle[0] = 0.0;
            visible = true;
        }
        visible = visible && enableAll;
        prop.pp_beacon_light[0]->setVisible(visible);
        prop.pp_beacon_scene_node[0]->setVisible(visible);
    }
}

void RoR::GfxActor::UpdateProps(float dt, bool is_player_actor)
{
    using namespace Ogre;

    SimBuffer::NodeSB* nodes = this->GetSimNodeBuffer();

    // Update prop meshes
    for (Prop& prop: m_props)
    {
        if (prop.pp_scene_node == nullptr) // Wing beacons don't have scenenodes
            continue;

        // Update visibility
        if (prop.pp_aero_propeller_blade || prop.pp_aero_propeller_spin)
        {
            const float SPINNER_THRESHOLD = 200.f; // TODO: magic! ~ only_a_ptr, 09/2018
            const bool show_spinner = m_simbuf.simbuf_aeroengines[prop.pp_aero_engine_idx].simbuf_ae_rpm > SPINNER_THRESHOLD;
            if (prop.pp_aero_propeller_blade)
                prop.pp_scene_node->setVisible(!show_spinner);
            else if (prop.pp_aero_propeller_spin)
                prop.pp_scene_node->setVisible(show_spinner);
        }
        else
        {
            const bool mo_visible = (prop.pp_camera_mode == -2 || prop.pp_camera_mode == m_simbuf.simbuf_cur_cinecam);
            prop.pp_mesh_obj->setVisible(mo_visible);
            if (!mo_visible)
            {
                continue; // No need to update hidden meshes
            }
        }

        // Update position and orientation
        // -- quick ugly port from `Actor::updateProps()` --- ~ 06/2018
        Vector3 diffX = nodes[prop.pp_node_x].AbsPosition - nodes[prop.pp_node_ref].AbsPosition;
        Vector3 diffY = nodes[prop.pp_node_y].AbsPosition - nodes[prop.pp_node_ref].AbsPosition;

        Vector3 normal = (diffY.crossProduct(diffX)).normalisedCopy();

        Vector3 mposition = nodes[prop.pp_node_ref].AbsPosition + prop.pp_offset.x * diffX + prop.pp_offset.y * diffY;
        prop.pp_scene_node->setPosition(mposition + normal * prop.pp_offset.z);

        Vector3 refx = diffX.normalisedCopy();
        Vector3 refy = refx.crossProduct(normal);
        Quaternion orientation = Quaternion(refx, normal, refy) * prop.pp_rot;
        prop.pp_scene_node->setOrientation(orientation);

        if (prop.pp_wheel_scene_node) // special prop - steering wheel
        {
            Quaternion brot = Quaternion(Degree(-59.0), Vector3::UNIT_X);
            brot = brot * Quaternion(Degree(m_simbuf.simbuf_hydro_dir_state * prop.pp_wheel_rot_degree), Vector3::UNIT_Y);
            prop.pp_wheel_scene_node->setPosition(mposition + normal * prop.pp_offset.z + orientation * prop.pp_wheel_pos);
            prop.pp_wheel_scene_node->setOrientation(orientation * brot);
        }
    }

    // Update beacon flares
    if (m_simbuf.simbuf_beaconlight_active != m_beaconlight_active)
    {
        m_beaconlight_active = m_simbuf.simbuf_beaconlight_active;
        this->SetBeaconsEnabled(m_beaconlight_active);
    }

    if ((App::gfx_flares_mode->GetEnum<GfxFlaresMode>() != GfxFlaresMode::NONE)
        && m_beaconlight_active)
    {
        for (Prop& prop: m_props)
        {
            if (prop.pp_beacon_type != 0)
            {
                this->UpdateBeaconFlare(prop, dt, is_player_actor);
            }
        }
    }
}

void RoR::GfxActor::SetPropsVisible(bool visible)
{
    for (Prop& prop: m_props)
    {
        if (prop.pp_mesh_obj)
            prop.pp_mesh_obj->setVisible(visible);
        if (prop.pp_wheel_mesh_obj)
            prop.pp_wheel_mesh_obj->setVisible(visible);
        if (prop.pp_beacon_scene_node[0])
            prop.pp_beacon_scene_node[0]->setVisible(visible);
        if (prop.pp_beacon_scene_node[1])
            prop.pp_beacon_scene_node[1]->setVisible(visible);
        if (prop.pp_beacon_scene_node[2])
            prop.pp_beacon_scene_node[2]->setVisible(visible);
        if (prop.pp_beacon_scene_node[3])
            prop.pp_beacon_scene_node[3]->setVisible(visible);
    }
}

void RoR::GfxActor::SetRenderdashActive(bool active)
{
    if (m_renderdash != nullptr)
    {
        m_renderdash->setEnable(active);
    }
}

void RoR::GfxActor::UpdateRenderdashRTT()
{
    if (m_renderdash != nullptr)
    {
        m_renderdash->getRenderTarget()->update();
    }
}

void RoR::GfxActor::SetBeaconsEnabled(bool beacon_light_is_active)
{
    const bool enableLight = (App::gfx_flares_mode->GetEnum<GfxFlaresMode>() != GfxFlaresMode::NO_LIGHTSOURCES);

    for (Prop& prop: m_props)
    {
        char beacon_type = prop.pp_beacon_type;
        if (beacon_type == 'b')
        {
            prop.pp_beacon_light[0]->setVisible(beacon_light_is_active && enableLight);
            prop.pp_beacon_scene_node[0]->setVisible(beacon_light_is_active);
            if (prop.pp_beacon_bbs[0] && beacon_light_is_active && !prop.pp_beacon_scene_node[0]->numAttachedObjects())
            {
                prop.pp_beacon_bbs[0]->setVisible(true);
                prop.pp_beacon_scene_node[0]->attachObject(prop.pp_beacon_bbs[0]);
            }
            else if (prop.pp_beacon_bbs[0] && !beacon_light_is_active)
            {
                prop.pp_beacon_scene_node[0]->detachAllObjects();
                prop.pp_beacon_bbs[0]->setVisible(false);
            }
        }
        else if (beacon_type == 'R' || beacon_type == 'L')
        {
            prop.pp_beacon_scene_node[0]->setVisible(beacon_light_is_active);
            if (prop.pp_beacon_bbs[0] && beacon_light_is_active && !prop.pp_beacon_scene_node[0]->numAttachedObjects())
                prop.pp_beacon_scene_node[0]->attachObject(prop.pp_beacon_bbs[0]);
            else if (prop.pp_beacon_bbs[0] && !beacon_light_is_active)
                prop.pp_beacon_scene_node[0]->detachAllObjects();
        }
        else if (beacon_type == 'p')
        {
            for (int k = 0; k < 4; k++)
            {
                prop.pp_beacon_light[k]->setVisible(beacon_light_is_active && enableLight);
                prop.pp_beacon_scene_node[k]->setVisible(beacon_light_is_active);
                if (prop.pp_beacon_bbs[k] && beacon_light_is_active && !prop.pp_beacon_scene_node[k]->numAttachedObjects())
                    prop.pp_beacon_scene_node[k]->attachObject(prop.pp_beacon_bbs[k]);
                else if (prop.pp_beacon_bbs[k] && !beacon_light_is_active)
                    prop.pp_beacon_scene_node[k]->detachAllObjects();
            }
        }
        else
        {
            for (int k = 0; k < 4; k++)
            {
                if (prop.pp_beacon_light[k])
                {
                    prop.pp_beacon_light[k]->setVisible(beacon_light_is_active && enableLight);
                }
                if (prop.pp_beacon_scene_node[k])
                {
                    prop.pp_beacon_scene_node[k]->setVisible(beacon_light_is_active);

                    if (prop.pp_beacon_bbs[k] && beacon_light_is_active && !prop.pp_beacon_scene_node[k]->numAttachedObjects())
                    {
                        prop.pp_beacon_scene_node[k]->attachObject(prop.pp_beacon_bbs[k]);
                    }
                    else if (prop.pp_beacon_bbs[k] && !beacon_light_is_active)
                    {
                        prop.pp_beacon_scene_node[k]->detachAllObjects();
                    }
                }
            }
        }
    }
}


void RoR::GfxActor::CalcPropAnimation(const int flag_state, float& cstate, int& div, float dt, const float lower_limit, const float upper_limit, const float option3)
{
    // ## DEV NOTE:
    // ## This function is a modified copypaste of `Actor::calcAnimators()` which was used
    // ## for both animator-beams (physics, part of softbody) and animated props (visual-only).
    // ## ~ only_a_ptr, 06/2018

    //boat rudder
    if (flag_state & PROP_ANIM_FLAG_BRUDDER)
    {
        size_t spi;
        float ctmp = 0.0f;
        for (spi = 0; spi < m_simbuf.simbuf_screwprops.size(); spi++)
        {
            ctmp += m_simbuf.simbuf_screwprops[spi].simbuf_sp_rudder;
        }

        if (spi > 0)
            ctmp = ctmp / spi;
        cstate = ctmp;
        div++;
    }

    //boat throttle
    if (flag_state & PROP_ANIM_FLAG_BTHROTTLE)
    {
        size_t spi;
        float ctmp = 0.0f;
        for (spi = 0; spi < m_simbuf.simbuf_screwprops.size(); spi++)
        {
            ctmp += m_simbuf.simbuf_screwprops[spi].simbuf_sp_throttle;
        }

        if (spi > 0)
            ctmp = ctmp / spi;
        cstate = ctmp;
        div++;
    }

    //differential lock status
    if (flag_state & PROP_ANIM_FLAG_DIFFLOCK)
    {
        if (m_actor->m_num_wheel_diffs > 0) // read-only attribute - safe to read from here
        {
            switch (m_simbuf.simbuf_diff_type)
            {
            case DiffType::OPEN_DIFF:
                cstate = 0.0f;
                break;
            case DiffType::SPLIT_DIFF:
                cstate = 0.5f;
                break;
            case DiffType::LOCKED_DIFF:
                cstate = 1.0f;
                break;
            default:;
            }
        }
        else // no axles/diffs avail, mode is split by default
            cstate = 0.5f;

        div++;
    }

    //heading
    if (flag_state & PROP_ANIM_FLAG_HEADING)
    {
        // rad2deg limitedrange  -1 to +1
        cstate = (m_simbuf.simbuf_rotation * 57.29578f) / 360.0f;
        div++;
    }

    //torque - WRITES 
    const bool has_engine = (m_actor->ar_engine!= nullptr);
    if (has_engine && flag_state & PROP_ANIM_FLAG_TORQUE)
    {
        float torque = m_simbuf.simbuf_engine_crankfactor;
        if (torque <= 0.0f)
            torque = 0.0f;
        if (torque >= m_prop_anim_crankfactor_prev)
            cstate -= torque / 10.0f;
        else
            cstate = 0.0f;

        if (cstate <= -1.0f)
            cstate = -1.0f;
        m_prop_anim_crankfactor_prev = torque;
        div++;
    }

    //shifterseq, to amimate sequentiell shifting
    if (has_engine && (flag_state & PROP_ANIM_FLAG_SHIFTER) && option3 == 3.0f)
    {
        // opt1 &opt2 = 0   this is a shifter
        if (!lower_limit && !upper_limit)
        {
            int shifter = m_simbuf.simbuf_gear;
            if (shifter > m_prop_anim_prev_gear)
            {
                cstate = 1.0f;
                m_prop_anim_shift_timer = 0.2f;
            }
            if (shifter < m_prop_anim_prev_gear)
            {
                cstate = -1.0f;
                m_prop_anim_shift_timer = -0.2f;
            }
            m_prop_anim_prev_gear = shifter;

            if (m_prop_anim_shift_timer > 0.0f)
            {
                cstate = 1.0f;
                m_prop_anim_shift_timer -= dt;
                if (m_prop_anim_shift_timer < 0.0f)
                    m_prop_anim_shift_timer = 0.0f;
            }
            if (m_prop_anim_shift_timer < 0.0f)
            {
                cstate = -1.0f;
                m_prop_anim_shift_timer += dt;
                if (m_prop_anim_shift_timer > 0.0f)
                    m_prop_anim_shift_timer = 0.0f;
            }
        }
        else
        {
            // check if lower_limit is a valid to get commandvalue, then get commandvalue
            if (lower_limit >= 1.0f && lower_limit <= 48.0)
                if (m_simbuf.simbuf_commandkey[int(lower_limit)].simbuf_cmd_value > 0)
                    cstate += 1.0f;
            // check if upper_limit is a valid to get commandvalue, then get commandvalue
            if (upper_limit >= 1.0f && upper_limit <= 48.0)
                if (m_simbuf.simbuf_commandkey[int(upper_limit)].simbuf_cmd_value > 0)
                    cstate -= 1.0f;
        }

        div++;
    }

    //shifterman1, left/right
    if (has_engine && (flag_state & PROP_ANIM_FLAG_SHIFTER) && option3 == 1.0f)
    {
        int shifter = m_simbuf.simbuf_gear;
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
    if (has_engine && (flag_state & PROP_ANIM_FLAG_SHIFTER) && option3 == 2.0f)
    {
        int shifter = m_simbuf.simbuf_gear;
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
    if (has_engine && (flag_state & PROP_ANIM_FLAG_SHIFTER) && option3 == 4.0f)
    {
        int shifter = m_simbuf.simbuf_gear;
        int numgears = m_attr.xa_num_gears;
        cstate -= (shifter + 2.0) / (numgears + 2.0);
        div++;
    }

    //parking brake
    if (flag_state & PROP_ANIM_FLAG_PBRAKE)
    {
        float pbrake = static_cast<float>(m_simbuf.simbuf_parking_brake); // Bool --> float
        cstate -= pbrake;
        div++;
    }

    //speedo ( scales with speedomax )
    if (flag_state & PROP_ANIM_FLAG_SPEEDO)
    {
        float speedo = m_simbuf.simbuf_wheel_speed / m_attr.xa_speedo_highest_kph;
        cstate -= speedo * 3.0f;
        div++;
    }

    //engine tacho ( scales with maxrpm, default is 3500 )
    if (has_engine && flag_state & PROP_ANIM_FLAG_TACHO)
    {
        float tacho = m_simbuf.simbuf_engine_rpm / m_attr.xa_engine_max_rpm;
        cstate -= tacho;
        div++;
    }

    //turbo
    if (has_engine && flag_state & PROP_ANIM_FLAG_TURBO)
    {
        float turbo = m_simbuf.simbuf_engine_turbo_psi * 3.34;
        cstate -= turbo / 67.0f;
        div++;
    }

    //brake
    if (flag_state & PROP_ANIM_FLAG_BRAKE)
    {
        float brakes = m_simbuf.simbuf_brake;
        cstate -= brakes;
        div++;
    }

    //accelerator
    if (has_engine && flag_state & PROP_ANIM_FLAG_ACCEL)
    {
        float accel = m_simbuf.simbuf_engine_accel;
        cstate -= accel + 0.06f;
        //( small correction, get acc is nver smaller then 0.06.
        div++;
    }

    //clutch
    if (has_engine && flag_state & PROP_ANIM_FLAG_CLUTCH)
    {
        float clutch = m_simbuf.simbuf_clutch;
        cstate -= fabs(1.0f - clutch);
        div++;
    }

    //aeroengines rpm + throttle + torque ( turboprop ) + pitch ( turboprop ) + status +  fire
    // `option3` is aeroengine number (starting from 1)
    if (option3 > 0.f && option3 <= float(m_simbuf.simbuf_aeroengines.size()))
    {
        const int aenum = int(option3 - 1.f);
        if (flag_state & PROP_ANIM_FLAG_RPM)
        {
            float angle;
            float pcent = m_simbuf.simbuf_aeroengines[aenum].simbuf_ae_rpmpc;
            if (pcent < 60.0)
                angle = -5.0 + pcent * 1.9167;
            else if (pcent < 110.0)
                angle = 110.0 + (pcent - 60.0) * 4.075;
            else
                angle = 314.0;
            cstate -= angle / 314.0f;
            div++;
        }
        if (flag_state & PROP_ANIM_FLAG_THROTTLE)
        {
            float throttle = m_simbuf.simbuf_aeroengines[aenum].simbuf_ae_throttle;
            cstate -= throttle;
            div++;
        }

        if (m_simbuf.simbuf_aeroengines[aenum].simbuf_ae_type == AEROENGINE_TURBOPROP_PISTONPROP)
        {
            if (flag_state & PROP_ANIM_FLAG_AETORQUE)
            {
                cstate = m_simbuf.simbuf_aeroengines[aenum].simbuf_tp_aetorque / 120.0f;
                div++;
            }

            if (flag_state & PROP_ANIM_FLAG_AEPITCH)
            {
                cstate = m_simbuf.simbuf_aeroengines[aenum].simbuf_tp_aepitch / 120.0f;
                div++;
            }
        }

        if (flag_state & PROP_ANIM_FLAG_AESTATUS)
        {
            if (!m_simbuf.simbuf_aeroengines[aenum].simbuf_ae_ignition)
                cstate = 0.0f;
            else
                cstate = 0.5f;
            if (m_simbuf.simbuf_aeroengines[aenum].simbuf_ae_failed)
                cstate = 1.0f;
            div++;
        }
    }

    const Ogre::Vector3 node0_pos = this->GetSimNodeBuffer()[0].AbsPosition;
    const Ogre::Vector3 node0_velo = m_simbuf.simbuf_node0_velo;

    //airspeed indicator
    if (flag_state & PROP_ANIM_FLAG_AIRSPEED)
    {
        float ground_speed_kt = node0_velo.length() * 1.9438;
        float altitude = node0_pos.y;

        float sea_level_pressure = 101325; //in Pa

        float airpressure = sea_level_pressure * pow(1.0 - 0.0065 * altitude / 288.15, 5.24947); //in Pa
        float airdensity = airpressure * 0.0000120896;//1.225 at sea level
        float kt = ground_speed_kt * sqrt(airdensity / 1.225);
        cstate -= kt / 100.0f;
        div++;
    }

    //vvi indicator
    if (flag_state & PROP_ANIM_FLAG_VVI)
    {
        float vvi = node0_velo.y * 196.85;
        // limit vvi scale to +/- 6m/s
        cstate -= vvi / 6000.0f;
        if (cstate >= 1.0f)
            cstate = 1.0f;
        if (cstate <= -1.0f)
            cstate = -1.0f;
        div++;
    }

    //altimeter
    if (flag_state & PROP_ANIM_FLAG_ALTIMETER)
    {
        //altimeter indicator 1k oscillating
        if (option3 == 3.0f)
        {
            float altimeter = (node0_pos.y * 1.1811) / 360.0f;
            int alti_int = int(altimeter);
            float alti_mod = (altimeter - alti_int);
            cstate -= alti_mod;
        }

        //altimeter indicator 10k oscillating
        if (option3 == 2.0f)
        {
            float alti = node0_pos.y * 1.1811 / 3600.0f;
            int alti_int = int(alti);
            float alti_mod = (alti - alti_int);
            cstate -= alti_mod;
            if (cstate <= -1.0f)
                cstate = -1.0f;
        }

        //altimeter indicator 100k limited
        if (option3 == 1.0f)
        {
            float alti = node0_pos.y * 1.1811 / 36000.0f;
            cstate -= alti;
            if (cstate <= -1.0f)
                cstate = -1.0f;
        }
        div++;
    }

    //AOA
    if (flag_state & PROP_ANIM_FLAG_AOA)
    {
        float aoa = m_simbuf.simbuf_wing4_aoa / 25.f;
        if ((node0_velo.length() * 1.9438) < 10.0f)
            aoa = 0;
        cstate -= aoa;
        if (cstate <= -1.0f)
            cstate = -1.0f;
        if (cstate >= 1.0f)
            cstate = 1.0f;
        div++;
    }

    Ogre::Vector3 cam_pos  = this->GetSimNodeBuffer()[m_actor->ar_main_camera_node_pos ].AbsPosition;
    Ogre::Vector3 cam_roll = this->GetSimNodeBuffer()[m_actor->ar_main_camera_node_roll].AbsPosition;
    Ogre::Vector3 cam_dir  = this->GetSimNodeBuffer()[m_actor->ar_main_camera_node_dir ].AbsPosition;

    // roll
    if (flag_state & PROP_ANIM_FLAG_ROLL)
    {
        Ogre::Vector3 rollv = (cam_pos - cam_roll).normalisedCopy();
        Ogre::Vector3 dirv = (cam_pos - cam_dir).normalisedCopy();
        Ogre::Vector3 upv = dirv.crossProduct(-rollv);
        float rollangle = asin(rollv.dotProduct(Ogre::Vector3::UNIT_Y));
        // rad to deg
        rollangle = Ogre::Math::RadiansToDegrees(rollangle);
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
    if (flag_state & PROP_ANIM_FLAG_PITCH)
    {
        Ogre::Vector3 dirv = (cam_pos - cam_dir).normalisedCopy();
        float pitchangle = asin(dirv.dotProduct(Ogre::Vector3::UNIT_Y));
        // radian to degrees with a max cstate of +/- 1.0
        cstate = (Ogre::Math::RadiansToDegrees(pitchangle) / 90.0f);
        div++;
    }

    // airbrake
    if (flag_state & PROP_ANIM_FLAG_AIRBRAKE)
    {
        float airbrake = static_cast<float>(m_simbuf.simbuf_airbrake_state);
        // cstate limited to -1.0f
        cstate -= airbrake / 5.0f;
        div++;
    }

    //flaps
    if (flag_state & PROP_ANIM_FLAG_FLAP)
    {
        float flaps = FLAP_ANGLES[m_simbuf.simbuf_aero_flap_state];
        // cstate limited to -1.0f
        cstate = flaps;
        div++;
    }
}

void RoR::GfxActor::UpdatePropAnimations(float dt, bool is_player_connected)
{
    for (Prop& prop: m_props)
    {
        int animnum = 0;
        float rx = 0.0f;
        float ry = 0.0f;
        float rz = 0.0f;

        for (PropAnim& anim: prop.pp_animations)
        {
            float cstate = 0.0f;
            int div = 0.0f;
            const float lower_limit = anim.lower_limit;
            const float upper_limit = anim.upper_limit;
            float animOpt3 = anim.animOpt3;

            this->CalcPropAnimation(anim.animFlags, cstate, div, dt, lower_limit, upper_limit, animOpt3);

            // key triggered animations
            if ((anim.animFlags & ANIM_FLAG_EVENT) && anim.animKey != -1 && is_player_connected)
            {
                // TODO: Keys shouldn't be queried from here, but buffered in sim. loop ~ only_a_ptr, 06/2018
                if (RoR::App::GetInputEngine()->getEventValue(anim.animKey))
                {
                    // keystatelock is disabled then set cstate
                    if (anim.animKeyState == -1.0f)
                    {
                        // TODO: Keys shouldn't be queried from here, but buffered in sim. loop ~ only_a_ptr, 06/2018
                        cstate += RoR::App::GetInputEngine()->getEventValue(anim.animKey);
                    }
                    else if (!anim.animKeyState)
                    {
                        // a key was pressed and a toggle was done already, so bypass
                        //toggle now
                        if (!anim.lastanimKS)
                        {
                            anim.lastanimKS = 1.0f;
                            // use animkey as bool to determine keypress / release state of inputengine
                            anim.animKeyState = 1.0f;
                        }
                        else
                        {
                            anim.lastanimKS = 0.0f;
                            // use animkey as bool to determine keypress / release state of inputengine
                            anim.animKeyState = 1.0f;
                        }
                    }
                    else
                    {
                        // bypas mode, get the last set position and set it
                        cstate += anim.lastanimKS;
                    }
                }
                else
                {
                    // keyevent exists and keylock is enabled but the key isnt pressed right now = get lastanimkeystatus for cstate and reset keypressed bool animkey
                    if (anim.animKeyState != -1.0f)
                    {
                        cstate += anim.lastanimKS;
                        anim.animKeyState = 0.0f;
                    }
                }
            }

            //propanimation placed here to avoid interference with existing hydros(cstate) and permanent prop animation
            //land vehicle steering
            if (anim.animFlags & PROP_ANIM_FLAG_STEERING)
                cstate += m_simbuf.simbuf_hydro_dir_state;
            //aileron
            if (anim.animFlags & PROP_ANIM_FLAG_AILERONS)
                cstate += m_simbuf.simbuf_hydro_aileron_state;
            //elevator
            if (anim.animFlags & PROP_ANIM_FLAG_ELEVATORS)
                cstate += m_simbuf.simbuf_hydro_elevator_state;
            //rudder
            if (anim.animFlags & PROP_ANIM_FLAG_ARUDDER)
                cstate += m_simbuf.simbuf_hydro_aero_rudder_state;
            //permanent
            if (anim.animFlags & PROP_ANIM_FLAG_PERMANENT)
                cstate += 1.0f;

            cstate *= anim.animratio;

            // autoanimate noflip_bouncer
            if (anim.animOpt5)
                cstate *= (anim.animOpt5);

            //rotate prop
            if ((anim.animMode & PROP_ANIM_MODE_ROTA_X) || (anim.animMode & PROP_ANIM_MODE_ROTA_Y) || (anim.animMode & PROP_ANIM_MODE_ROTA_Z))
            {
                float limiter = 0.0f;
                // This code was formerly executed within a fixed timestep of 0.5ms and finetuned accordingly.
                // This is now taken into account by factoring in the respective fraction of the variable timestep.
                float const dt_frac = dt * 2000.f;
                if (anim.animMode & PROP_ANIM_MODE_AUTOANIMATE)
                {
                    if (anim.animMode & PROP_ANIM_MODE_ROTA_X)
                    {
                        prop.pp_rota.x += cstate * dt_frac;
                        limiter = prop.pp_rota.x;
                    }
                    if (anim.animMode & PROP_ANIM_MODE_ROTA_Y)
                    {
                        prop.pp_rota.y += cstate * dt_frac;
                        limiter = prop.pp_rota.y;
                    }
                    if (anim.animMode & PROP_ANIM_MODE_ROTA_Z)
                    {
                        prop.pp_rota.z += cstate * dt_frac;
                        limiter = prop.pp_rota.z;
                    }
                }
                else
                {
                    if (anim.animMode & PROP_ANIM_MODE_ROTA_X)
                        rx += cstate;
                    if (anim.animMode & PROP_ANIM_MODE_ROTA_Y)
                        ry += cstate;
                    if (anim.animMode & PROP_ANIM_MODE_ROTA_Z)
                        rz += cstate;
                }

                bool limiterchanged = false;
                // check if a positive custom limit is set to evaluate/calc flip back

                if (limiter > upper_limit)
                {
                    if (anim.animMode & PROP_ANIM_MODE_NOFLIP)
                    {
                        limiter = upper_limit; // stop at limit
                        anim.animOpt5 *= -1.0f; // change cstate multiplier if bounce is set
                    }
                    else
                    {
                        limiter = lower_limit; // flip to other side at limit
                    }
                    limiterchanged = true;
                }

                if (limiter < lower_limit)
                {
                    if (anim.animMode & PROP_ANIM_MODE_NOFLIP)
                    {
                        limiter = lower_limit; // stop at limit
                        anim.animOpt5 *= -1.0f; // change cstate multiplier if active
                    }
                    else
                    {
                        limiter = upper_limit; // flip to other side at limit
                    }
                    limiterchanged = true;
                }

                if (limiterchanged)
                {
                    if (anim.animMode & PROP_ANIM_MODE_ROTA_X)
                        prop.pp_rota.x = limiter;
                    if (anim.animMode & PROP_ANIM_MODE_ROTA_Y)
                        prop.pp_rota.y = limiter;
                    if (anim.animMode & PROP_ANIM_MODE_ROTA_Z)
                        prop.pp_rota.z = limiter;
                }
            }

            //offset prop

            if ((anim.animMode & PROP_ANIM_MODE_OFFSET_X) || (anim.animMode & PROP_ANIM_MODE_OFFSET_Y) || (anim.animMode & PROP_ANIM_MODE_OFFSET_Z))
            {
                float offset = 0.0f;
                float autooffset = 0.0f;

                if (anim.animMode & PROP_ANIM_MODE_OFFSET_X)
                    offset = prop.pp_offset_orig.x;
                if (anim.animMode & PROP_ANIM_MODE_OFFSET_Y)
                    offset = prop.pp_offset_orig.y;
                if (anim.animMode & PROP_ANIM_MODE_OFFSET_Z)
                    offset = prop.pp_offset_orig.z;

                if (anim.animMode & PROP_ANIM_MODE_AUTOANIMATE)
                {
                    // This code was formerly executed within a fixed timestep of 0.5ms and finetuned accordingly.
                    // This is now taken into account by factoring in the respective fraction of the variable timestep.
                    float const dt_frac = dt * 2000.f;
                    autooffset = offset + cstate * dt_frac;

                    if (autooffset > upper_limit)
                    {
                        if (anim.animMode & PROP_ANIM_MODE_NOFLIP)
                        {
                            autooffset = upper_limit; // stop at limit
                            anim.animOpt5 *= -1.0f; // change cstate multiplier if active
                        }
                        else
                        {
                            autooffset = lower_limit; // flip to other side at limit
                        }
                    }

                    if (autooffset < lower_limit)
                    {
                        if (anim.animMode & PROP_ANIM_MODE_NOFLIP)
                        {
                            autooffset = lower_limit; // stop at limit
                            anim.animOpt5 *= -1.0f; // change cstate multiplier if active
                        }
                        else
                        {
                            autooffset = upper_limit; // flip to other side at limit
                        }
                    }
                }
                offset += cstate;

                if (anim.animMode & PROP_ANIM_MODE_OFFSET_X)
                {
                    prop.pp_offset.x = offset;
                    if (anim.animMode & PROP_ANIM_MODE_AUTOANIMATE)
                        prop.pp_offset_orig.x = autooffset;
                }
                if (anim.animMode & PROP_ANIM_MODE_OFFSET_Y)
                {
                    prop.pp_offset.y = offset;
                    if (anim.animMode & PROP_ANIM_MODE_AUTOANIMATE)
                        prop.pp_offset_orig.y = autooffset;
                }
                if (anim.animMode & PROP_ANIM_MODE_OFFSET_Z)
                {
                    prop.pp_offset.z = offset;
                    if (anim.animMode & PROP_ANIM_MODE_AUTOANIMATE)
                        prop.pp_offset_orig.z = autooffset;
                }
            }
            animnum++;
        }
        //recalc the quaternions with final stacked rotation values ( rx, ry, rz )
        rx += prop.pp_rota.x;
        ry += prop.pp_rota.y;
        rz += prop.pp_rota.z;

        prop.pp_rot = Ogre::Quaternion(Ogre::Degree(rz), Ogre::Vector3::UNIT_Z) * 
                      Ogre::Quaternion(Ogre::Degree(ry), Ogre::Vector3::UNIT_Y) *
                      Ogre::Quaternion(Ogre::Degree(rx), Ogre::Vector3::UNIT_X);
    }
}

void RoR::GfxActor::SortFlexbodies()
{
    std::sort(m_flexbodies.begin(), m_flexbodies.end(), [](FlexBody* a, FlexBody* b) { return a->size() > b->size(); });
}

void RoR::GfxActor::UpdateFlexbodies()
{
    m_flexbody_tasks.clear();

    for (FlexBody* fb: m_flexbodies)
    {
        const int camera_mode = fb->getCameraMode();
        if ((camera_mode == -2) || (camera_mode == m_simbuf.simbuf_cur_cinecam))
        {
            auto func = std::function<void()>([fb]()
                {
                    fb->ComputeFlexbody();
                });
            auto task_handle = App::GetThreadPool()->RunTask(func);
            m_flexbody_tasks.push_back(task_handle);
        }
        else
        {
            fb->setVisible(false);
        }
    }
}

void RoR::GfxActor::ResetFlexbodies()
{
    for (FlexBody* fb: m_flexbodies)
    {
        fb->reset();
    }
}

void RoR::GfxActor::SetFlexbodyVisible(bool visible)
{
    for (FlexBody* fb: m_flexbodies)
    {
        fb->setVisible(visible);
    }
}

void RoR::GfxActor::FinishFlexbodyTasks()
{
    for (auto& task: m_flexbody_tasks)
    {
        task->join();
    }
    for (FlexBody* fb: m_flexbodies)
    {
        fb->UpdateFlexbodyVertexBuffers();
    }
}

void RoR::GfxActor::UpdateFlares(float dt_sec, bool is_player)
{
    // == Flare states are determined in simulation, this function only applies them to OGRE objects ==

    bool enableAll = ((App::gfx_flares_mode->GetEnum<GfxFlaresMode>() == GfxFlaresMode::CURR_VEHICLE_HEAD_ONLY) && !is_player);
    SimBuffer::NodeSB* nodes = this->GetSimNodeBuffer();

    int num_flares = static_cast<int>(m_actor->ar_flares.size());
    for (int i=0; i<num_flares; ++i)
    {
        Flare& flare = m_actor->ar_flares[i];
        
        //TODO: Following code is a quick+dirty port from `Actor::updateFlares()` - tidy it up! ~only_a_ptr, 06/2018

        if (flare.fl_type == FlareType::HEADLIGHT)
        {
            this->SetMaterialFlareOn(i, m_simbuf.simbuf_headlight_on);
        }
        else
        {
            this->SetMaterialFlareOn(i, flare.isVisible);
            flare.snode->setVisible(flare.isVisible);
            if (flare.light != nullptr)
            {
                flare.light->setVisible(flare.isVisible && enableAll);
            }
        }

        Ogre::Vector3 normal = (nodes[flare.nodey].AbsPosition - nodes[flare.noderef].AbsPosition).crossProduct(nodes[flare.nodex].AbsPosition - nodes[flare.noderef].AbsPosition);
        normal.normalise();
        Ogre::Vector3 mposition = nodes[flare.noderef].AbsPosition + flare.offsetx * (nodes[flare.nodex].AbsPosition - nodes[flare.noderef].AbsPosition) + flare.offsety * (nodes[flare.nodey].AbsPosition - nodes[flare.noderef].AbsPosition);
        Ogre::Vector3 vdir = mposition - App::GetCameraManager()->GetCameraNode()->getPosition();
        float vlen = vdir.length();
        // not visible from 500m distance
        if (vlen > 500.0)
        {
            flare.snode->setVisible(false);
            continue;
        }
        //normalize
        vdir = vdir / vlen;
        float amplitude = normal.dotProduct(vdir);
        flare.snode->setPosition(mposition - 0.1 * amplitude * normal * flare.offsetz);
        flare.snode->setDirection(normal);
        float fsize = flare.size;
        if (fsize < 0)
        {
            amplitude = 1;
            fsize *= -1;
        }
        if (flare.light)
        {
            flare.light->setPosition(mposition - 0.2 * amplitude * normal);
            // point the real light towards the ground a bit
            flare.light->setDirection(-normal - Ogre::Vector3(0, 0.2, 0));
        }
        if (flare.isVisible)
        {
            if (amplitude > 0)
            {
                flare.bbs->setDefaultDimensions(amplitude * fsize, amplitude * fsize);
                flare.snode->setVisible(true);
            }
            else
            {
                flare.snode->setVisible(false);
            }
        }
    }
}

void RoR::GfxActor::SetCastShadows(bool value)
{
    // Cab mesh
    if (m_cab_scene_node != nullptr)
    {
        static_cast<Ogre::Entity*>(m_cab_scene_node->getAttachedObject(0))->setCastShadows(value);
    }

    // Props
    for (Prop& prop: m_props)
    {
        if (prop.pp_mesh_obj != nullptr && prop.pp_mesh_obj->getEntity())
            prop.pp_mesh_obj->getEntity()->setCastShadows(value);
        if (prop.pp_wheel_mesh_obj != nullptr && prop.pp_wheel_mesh_obj->getEntity())
            prop.pp_wheel_mesh_obj->getEntity()->setCastShadows(value);
    }

    // Wheels
    for (WheelGfx& wheel: m_wheels)
    {
        static_cast<Ogre::Entity*>(wheel.wx_scenenode->getAttachedObject(0))->setCastShadows(value);
    }

    // Softbody beams
    for (Rod& rod: m_rods)
    {
        static_cast<Ogre::Entity*>(rod.rod_scenenode->getAttachedObject(0))->setCastShadows(value);
    }

    // Flexbody meshes
    for (FlexBody* fb: m_flexbodies)
    {
        fb->SetFlexbodyCastShadow(value);
    }
}

void RoR::GfxActor::RegisterCabMesh(Ogre::Entity* ent, Ogre::SceneNode* snode, FlexObj* flexobj)
{
    m_cab_mesh = flexobj;
    m_cab_entity = ent;
    m_cab_scene_node = snode;
}

void RoR::GfxActor::SetAllMeshesVisible(bool visible)
{
    if (m_cab_entity != nullptr)
    {
        m_cab_entity->setVisible(visible);
    }
    this->SetWheelsVisible(visible);
    this->SetPropsVisible(visible);
    this->SetFlexbodyVisible(visible);
}

void RoR::GfxActor::UpdateWingMeshes()
{
    for (int i = 0; i < m_actor->ar_num_wings; ++i)
    {
        wing_t& wing = m_actor->ar_wings[i];
        wing.cnode->setPosition(wing.fa->updateVerticesGfx(this));
        wing.fa->uploadVertices();
    }
}

std::string   RoR::GfxActor::FetchActorDesignName() const                { return m_actor->GetActorDesignName(); }
int           RoR::GfxActor::FetchNumBeams      () const                 { return m_actor->ar_num_beams; }
int           RoR::GfxActor::FetchNumNodes      () const                 { return m_actor->ar_num_nodes; }
int           RoR::GfxActor::FetchNumWheelNodes () const                 { return m_actor->getWheelNodeCount(); }
