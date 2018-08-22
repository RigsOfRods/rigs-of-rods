/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2016-2018 Petr Ohlidal & contributors

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
#include "AeroEngine.h"
#include "AirBrake.h"
#include "AutoPilot.h"
#include "Beam.h"
#include "beam_t.h"
#include "BeamEngine.h" // EngineSim
#include "Collisions.h"
#include "DustPool.h" // General particle gfx
#include "Flexable.h"
#include "FlexAirfoil.h"
#include "FlexBody.h"
#include "FlexMeshWheel.h"
#include "FlexObj.h"
#include "GlobalEnvironment.h" // TODO: Eliminate!
#include "InputEngine.h" // TODO: Keys shouldn't be queried from here, but buffered in sim. loop ~ only_a_ptr, 06/2018
#include "MeshObject.h"
#include "MovableText.h"
#include "RoRFrameListener.h" // SimController
#include "SkyManager.h"
#include "SoundScriptManager.h"
#include "Utils.h"
#include "TerrainManager.h"
#include "ThreadPool.h"
#include "imgui.h"
#include "TurboProp.h"
#include <OgrePass.h>
#include <OgreRenderWindow.h>
#include <OgreResourceGroupManager.h>
#include <OgreSceneManager.h>
#include <OgreTechnique.h>
#include <OgreTextureUnitState.h>
#include <OgreTextureManager.h>

#include <OgreTextureManager.h>
#include <OgreSceneManager.h>
#include <OgreRenderWindow.h>

RoR::GfxActor::GfxActor(Actor* actor, std::string ogre_resource_group,
                        std::vector<NodeGfx>& gfx_nodes, std::vector<prop_t>& props,
                        int driverseat_prop_idx):
    m_actor(actor),
    m_custom_resource_group(ogre_resource_group),
    m_vidcam_state(VideoCamState::VCSTATE_ENABLED_ONLINE),
    m_debug_view(DebugViewType::DEBUGVIEW_NONE),
    m_rods_parent_scenenode(nullptr),
    m_gfx_nodes(gfx_nodes),
    m_props(props),
    m_driverseat_prop_index(driverseat_prop_idx),
    m_prop_anim_crankfactor_prev(0.f),
    m_prop_anim_shift_timer(0.f),
    m_beaconlight_active(false)
{
    // Setup particles
    RoR::GfxScene& dustman = RoR::App::GetSimController()->GetGfxScene();
    m_particles_drip   = dustman.GetDustPool("drip");
    m_particles_misc   = dustman.GetDustPool("dust"); // Dust, water vapour, tyre smoke
    m_particles_splash = dustman.GetDustPool("splash");
    m_particles_ripple = dustman.GetDustPool("ripple");
    m_particles_sparks = dustman.GetDustPool("sparks");
    m_particles_clump  = dustman.GetDustPool("clump");

    m_simbuf.simbuf_nodes.reset(new NodeData[actor->ar_num_nodes]);
    m_simbuf.simbuf_aeroengines.resize(actor->ar_num_aeroengines);
    m_simbuf.simbuf_commandkey.resize(MAX_COMMANDS + 10);
    m_simbuf.simbuf_airbrakes.resize(actor->ar_num_airbrakes);

    // Attributes
    m_attr.xa_speedo_highest_kph = actor->ar_speedo_max_kph; // TODO: Remove the attribute from Actor altogether ~ only_a_ptr, 05/2018
    m_attr.xa_speedo_use_engine_max_rpm = actor->ar_gui_use_engine_max_rpm; // TODO: ditto
    m_attr.xa_brake_force = actor->ar_brake_force;
    m_attr.xa_camera0_pos_node  = 0;
    m_attr.xa_camera0_roll_node = 0;
    m_attr.xa_has_autopilot = (actor->ar_autopilot != nullptr);
    if (actor->ar_num_cameras > 0)
    {
        m_attr.xa_camera0_pos_node  = actor->ar_camera_node_pos[0];
        m_attr.xa_camera0_roll_node = actor->ar_camera_node_roll[0];
    }
    if (actor->ar_engine != nullptr)
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
        gEnv->sceneManager->destroyCamera(vcam.vcam_ogre_camera);

        m_videocameras.pop_back();
    }

    // Dispose rods
    if (m_rods_parent_scenenode != nullptr)
    {
        for (Rod& rod: m_rods)
        {
            Ogre::MovableObject* ogre_object = rod.rod_scenenode->getAttachedObject(0);
            rod.rod_scenenode->detachAllObjects();
            gEnv->sceneManager->destroyEntity(static_cast<Ogre::Entity*>(ogre_object));
        }
        m_rods.clear();

        m_rods_parent_scenenode->removeAndDestroyAllChildren();
        gEnv->sceneManager->destroySceneNode(m_rods_parent_scenenode);
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
            gEnv->sceneManager->destroySceneNode(m_wheels[i].wx_scenenode);
        }
    }

    // delete airbrakes
    for (AirbrakeGfx& abx: m_gfx_airbrakes)
    {
        // scene node
        abx.abx_scenenode->detachAllObjects();
        gEnv->sceneManager->destroySceneNode(abx.abx_scenenode);
        // entity
        gEnv->sceneManager->destroyEntity(abx.abx_entity);
        // mesh
        abx.abx_mesh->unload();
        abx.abx_mesh.setNull();
    }
    m_gfx_airbrakes.clear();

    // Delete props
    for (prop_t & prop: m_props)
    {
        for (int k = 0; k < 4; ++k)
        {
            if (prop.beacon_flare_billboard_scene_node[k])
            {
                Ogre::SceneNode* scene_node = prop.beacon_flare_billboard_scene_node[k];
                scene_node->removeAndDestroyAllChildren();
                gEnv->sceneManager->destroySceneNode(scene_node);
            }
            if (prop.beacon_light[k])
            {
                gEnv->sceneManager->destroyLight(prop.beacon_light[k]);
            }
        }

        if (prop.scene_node)
        {
            prop.scene_node->removeAndDestroyAllChildren();
            gEnv->sceneManager->destroySceneNode(prop.scene_node);
        }
        if (prop.wheel)
        {
            prop.wheel->removeAndDestroyAllChildren();
            gEnv->sceneManager->destroySceneNode(prop.wheel);
        }
        if (prop.mo)
        {
            delete prop.mo;
        }
        if (prop.wheelmo)
        {
            delete prop.wheelmo;
        }
    }
    m_props.clear();

    // Delete flexbodies
    for (FlexBody* fb: m_flexbodies)
    {
        delete fb;
    }

    Ogre::ResourceGroupManager::getSingleton().destroyResourceGroup(m_custom_resource_group);
}

void RoR::GfxActor::AddMaterialFlare(int flareid, Ogre::MaterialPtr m)
{
    RoR::GfxActor::FlareMaterial binding;
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

RoR::GfxActor::VideoCamera::VideoCamera():
    vcam_type(VideoCamType::VCTYPE_INVALID), // VideoCamType
    vcam_node_center(node_t::INVALID_IDX),
    vcam_node_dir_y(node_t::INVALID_IDX),
    vcam_node_dir_z(node_t::INVALID_IDX),
    vcam_node_alt_pos(node_t::INVALID_IDX),
    vcam_node_lookat(node_t::INVALID_IDX),
    vcam_pos_offset(Ogre::Vector3::ZERO), // Ogre::Vector3
    vcam_ogre_camera(nullptr),            // Ogre::Camera*
    vcam_render_target(nullptr),          // Ogre::RenderTexture*
    vcam_debug_node(nullptr),             // Ogre::SceneNode*
    vcam_render_window(nullptr),          // Ogre::RenderWindow*
    vcam_prop_scenenode(nullptr)          // Ogre::SceneNode*
{}

RoR::GfxActor::NodeGfx::NodeGfx(uint16_t node_idx):
    nx_node_idx(node_idx),
    nx_wet_time_sec(-1.f), // node is dry
    nx_no_particles(false),
    nx_may_get_wet(false),
    nx_is_hot(false),
    nx_no_sparks(true),
    nx_under_water_prev(false)
{}

void RoR::GfxActor::UpdateVideoCameras(float dt_sec)
{
    if (m_vidcam_state != VideoCamState::VCSTATE_ENABLED_ONLINE)
        return;

    for (GfxActor::VideoCamera vidcam: m_videocameras)
    {
#ifdef USE_CAELUM
        // caelum needs to know that we changed the cameras
        SkyManager* sky = App::GetSimTerrain()->getSkyManager();
        if ((sky != nullptr) && (RoR::App::app_state.GetActive() == RoR::AppState::SIMULATION))
        {
            sky->NotifySkyCameraChanged(vidcam.vcam_ogre_camera);
        }
#endif // USE_CAELUM

        if ((vidcam.vcam_type == VideoCamType::VCTYPE_MIRROR_PROP_LEFT)
            || (vidcam.vcam_type == VideoCamType::VCTYPE_MIRROR_PROP_RIGHT))
        {
            // Mirror prop - special processing.
            float mirror_angle = 0.f;
            Ogre::Vector3 offset(Ogre::Vector3::ZERO);
            if (vidcam.vcam_type == VideoCamType::VCTYPE_MIRROR_PROP_LEFT)
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
            Ogre::Vector3 project = plane.projectVector(gEnv->mainCamera->getPosition() - center);

            vidcam.vcam_ogre_camera->setPosition(center);
            vidcam.vcam_ogre_camera->lookAt(gEnv->mainCamera->getPosition() - 2.0f * project);
            vidcam.vcam_ogre_camera->roll(roll);

            continue; // Done processing mirror prop.
        }

        // update the texture now, otherwise shuttering
        if (vidcam.vcam_render_target != nullptr)
            vidcam.vcam_render_target->update();

        if (vidcam.vcam_render_window != nullptr)
            vidcam.vcam_render_window->update();

        // get the normal of the camera plane now
        GfxActor::NodeData* node_buf = m_simbuf.simbuf_nodes.get();
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

        if (vidcam.vcam_type == GfxActor::VideoCamType::VCTYPE_MIRROR)
        {
            //rotate the normal of the mirror by user rotation setting so it reflects correct
            normal = vidcam.vcam_rotation * normal;
            // merge camera direction and reflect it on our plane
            vidcam.vcam_ogre_camera->setDirection((pos - gEnv->mainCamera->getPosition()).reflect(normal));
        }
        else if (vidcam.vcam_type == GfxActor::VideoCamType::VCTYPE_VIDEOCAM)
        {
            // rotate the camera according to the nodes orientation and user rotation
            Ogre::Vector3 refx = abs_pos_z - abs_pos_center;
            refx.normalise();
            Ogre::Vector3 refy = abs_pos_center - abs_pos_y;
            refy.normalise();
            Ogre::Quaternion rot = Ogre::Quaternion(-refx, -refy, -normal);
            vidcam.vcam_ogre_camera->setOrientation(rot * vidcam.vcam_rotation); // rotate the camera orientation towards the calculated cam direction plus user rotation
        }
        else if (vidcam.vcam_type == GfxActor::VideoCamType::VCTYPE_TRACKING_VIDEOCAM)
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
    const bool use_skidmarks = m_actor->GetUseSkidmarks();
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
        if (n.nd_collision_gm != nullptr && !nfx.nx_no_particles)
        {
            switch (n.nd_collision_gm->fx_type)
            {
            case Collisions::FX_DUSTY:
                if (m_particles_misc != nullptr)
                {
                    m_particles_misc->malloc(n.AbsPosition, n.Velocity / 2.0, n.nd_collision_gm->fx_colour);
                }
                break;

            case Collisions::FX_CLUMPY:
                if (m_particles_clump != nullptr && n.Velocity.squaredLength() > 1.f)
                {
                    m_particles_clump->allocClump(n.AbsPosition, n.Velocity / 2.0, n.nd_collision_gm->fx_colour);
                }
                break;

            case Collisions::FX_HARD:
                if (n.iswheel != 0) // This is a wheel => skidmarks and tyre smoke
                {
                    const float SKID_THRESHOLD = 10.f;
                    if (n.nd_collision_slip > SKID_THRESHOLD)
                    {
                        SOUND_MODULATE(m_actor, SS_MOD_SCREETCH, (n.nd_collision_slip - SKID_THRESHOLD) / SKID_THRESHOLD);
                        SOUND_PLAY_ONCE(m_actor, SS_TRIG_SCREETCH);
                        
                        if (m_particles_misc != nullptr)
                        {
                            m_particles_misc->allocSmoke(n.AbsPosition, n.Velocity);
                        }

                        if (use_skidmarks)
                        {
                            // TODO: Ported as-is from 'calcNodes()', doesn't make sense here
                            //       -> we can update the skidmark GFX directly. ~only_a_ptr, 04/2018
                            m_actor->ar_wheels[n.wheelid].isSkiding = true;
                            if (!(n.iswheel % 2))
                            {
                                m_actor->ar_wheels[n.wheelid].lastContactInner = n.AbsPosition;
                            }
                            else
                            {
                                m_actor->ar_wheels[n.wheelid].lastContactOuter = n.AbsPosition;
                            }

                            m_actor->ar_wheels[n.wheelid].lastContactType = (n.iswheel % 2);
                            m_actor->ar_wheels[n.wheelid].lastSlip = n.nd_collision_slip;
                            m_actor->ar_wheels[n.wheelid].lastGroundModel = n.nd_collision_gm;
                        }
                    }
                    else
                    {
                        if (use_skidmarks)
                        {
                            // TODO: Ported as-is from 'calcNodes()', doesn't make sense here
                            //       -> we can update the skidmark GFX directly. ~only_a_ptr, 04/2018
                            m_actor->ar_wheels[n.wheelid].isSkiding = false;
                        }
                    }
                }
                else // Not a wheel => sparks
                {
                    if ((!nfx.nx_no_sparks) && (n.nd_collision_slip > 1.f) && (m_particles_sparks != nullptr))
                    {
                        m_particles_sparks->allocSparks(n.AbsPosition, n.Velocity);
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
const ImU32 BEAM_COMPRESS_TEXT_COLOR (0xffccbf3c);
// TODO: commands cannot be distinguished on runtime

const ImU32 NODE_COLOR               (0xff44ddff);
const float NODE_RADIUS              (2.f);
const ImU32 NODE_TEXT_COLOR          (0xffcccccf); // node ID text color
const ImU32 NODE_MASS_TEXT_COLOR     (0xff77bb66);
const ImU32 NODE_IMMOVABLE_COLOR     (0xff0033ff);
const float NODE_IMMOVABLE_RADIUS    (2.8f);

void RoR::GfxActor::UpdateDebugView()
{
    if (m_debug_view == DebugViewType::DEBUGVIEW_NONE)
    {
        return; // Nothing to do
    }

    // Original 'debugVisuals' and their replacements ~only_a_ptr, 06/2017
    // -------------------------------------------------------------------
    // [1] node-numbers:  -------  Draws node numbers (black letters with white outline), generated nodes (wheels...) get fake sequentially assigned numbers.
    //                             Replacement: DEBUGVIEW_NODES; Note: real node_t::id value is displayed - generated nodes show "-1"
    // [2] beam-numbers:  -------  Draws beam numbers (sequentially assigned) as black (thick+distorted) text - almost unreadable, barely useful IMO.
    //                             Not preserved
    // [3] node-and-beam-numbers:  [1] + [2] combined
    //                             Not preserved
    // [4] node-mass:  ----------  Shows mass in same style as [1]
    //                             Replacement: Extra info in DEBUGVIEW_NODES with different text color, like "33 (3.3Kg)"
    // [5] node-locked:  --------  Shows text "unlocked"/"locked" in same style as [1]
    //                             replacement: colored circles around nodes showing PRELOCK and LOCKED states (not shown when ulocked) - used in all DEBUGVIEW_* modes
    // [6] beam-compression:  ---  A number shown per beam, same style as [2] - unreadable. Logic:
    //                              // float stress_ratio = beams[it->id].stress / beams[it->id].minmaxposnegstress;
    //                              // float color_scale = std::abs(stress_ratio);
    //                              // color_scale = std::min(color_scale, 1.0f);
    //                              // int scale = (int)(color_scale * 100);
    //                              // it->txt->setCaption(TOSTRING(scale));
    //                             Replacement: DEBUGVIEW_BEAMS -- modified logic, simplified, specific text color
    // [7] beam-broken  ---------  Shows "BROKEN" label for broken beams (`beam_t::broken` flag) in same style as [2]
    //                             Replacement - special coloring/display in DEBUGVIEW_* modes.
    // [8] beam-stress  ---------  Shows value of `beam_t::stress` in style of [2]
    //                             Replacement: DEBUGVIEW_BEAMS + specific text color
    // [9] beam-hydro  ----------  Shows a per-hydro number in style of [2], formula: `(beams[it->id].L / beams[it->id].Lhydro) * 100`
    //                             Replacement: DEBUGVIEW_BEAMS + specific text color
    // [9] beam-commands  -------  Shows a per-beam number in style of [2], formula: `(beams[it->id].L / beams[it->id].commandLong) * 100`
    //                             Not preserved - there's no way to distinguish commands on runtime and the number makes no sense for all beams. TODO: make commands distinguishable on runtime!

    // Var
    ImVec2 screen_size = ImGui::GetIO().DisplaySize;
    World2ScreenConverter world2screen(
        gEnv->mainCamera->getViewMatrix(true), gEnv->mainCamera->getProjectionMatrix(), Ogre::Vector2(screen_size.x, screen_size.y));

    // Dummy fullscreen window to draw to
    int window_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar| ImGuiWindowFlags_NoInputs 
                     | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoBringToFrontOnFocus;
    ImGui::Begin("RoR-SoftBodyView", NULL, screen_size, 0, window_flags);
    ImDrawList* drawlist = ImGui::GetWindowDrawList();
    ImGui::End();

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
            Ogre::Vector3 pos1 = world2screen.Convert(beams[i].p1->AbsPosition);
            Ogre::Vector3 pos2 = world2screen.Convert(beams[i].p2->AbsPosition);

            // Original coloring logic for "skeletonview":
            // -------------------------------------------
            // // float stress_ratio = beams[i].stress / beams[i].minmaxposnegstress;
            // // float color_scale = std::abs(stress_ratio);
            // // color_scale = std::min(color_scale, 1.0f);
            // // 
            // // if (stress_ratio <= 0)
            // //     color = ColourValue(0.2f, 1.0f - color_scale, color_scale, 0.8f);
            // // else
            // //     color = ColourValue(color_scale, 1.0f - color_scale, 0.2f, 0.8f);

            if ((pos1.z < 0.f) && (pos2.z < 0.f))
            {
                ImVec2 pos1xy(pos1.x, pos1.y);
                ImVec2 pos2xy(pos2.x, pos2.y);

                if (beams[i].bm_broken)
                {
                    drawlist->AddLine(pos1xy, pos2xy, BEAM_BROKEN_COLOR, BEAM_BROKEN_THICKNESS);
                }
                else if (beams[i].bm_type == BEAM_HYDRO)
                {
                    drawlist->AddLine(pos1xy, pos2xy, BEAM_HYDRO_COLOR, BEAM_HYDRO_THICKNESS);
                }
                else
                {
                    drawlist->AddLine(pos1xy, pos2xy, BEAM_COLOR, BEAM_THICKNESS);
                }
            }
        }

        // Nodes
        const node_t* nodes = m_actor->ar_nodes;
        const size_t num_nodes = static_cast<size_t>(m_actor->ar_num_nodes);
        for (size_t i = 0; i < num_nodes; ++i)
        {
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
                Ogre::Vector3 pos = world2screen.Convert(nodes[i].AbsPosition);

                if (pos.z < 0.f)
                {
                    ImVec2 pos_xy(pos.x, pos.y);
                    Str<25> id_buf;
                    id_buf << nodes[i].id;
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

        // Beam-info: drawn after beams to have higher Z-order
        if (m_debug_view == DebugViewType::DEBUGVIEW_BEAMS)
        {
            for (size_t i = 0; i < num_beams; ++i)
            {
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
                if (beams[i].strength >= 1000000.f)
                {
                    snprintf(buf, BUF_LEN, "%.2fM", (beams[i].strength / 1000000.f));
                }
                else if (beams[i].strength >= 1000.f)
                {
                    snprintf(buf, BUF_LEN, "%.1fK", (beams[i].strength / 1000.f));
                }
                else
                {
                    snprintf(buf, BUF_LEN, "%.2f", beams[i].strength);
                }
                drawlist->AddText(pos, BEAM_STRENGTH_TEXT_COLOR, buf);
                const ImVec2 stren_text_size = ImGui::CalcTextSize(buf);

                // Compression
                snprintf(buf, BUF_LEN, "|%.2f",  std::abs(beams[i].stress / beams[i].minmaxposnegstress)); // NOTE: New formula (simplified); the old one didn't make sense to me ~ only_a_ptr, 06/2017
                drawlist->AddText(ImVec2(pos.x + stren_text_size.x, pos.y), BEAM_COMPRESS_TEXT_COLOR, buf);

                // Stress
                snprintf(buf, BUF_LEN, "%.1f", beams[i].stress);
                ImVec2 stress_text_pos(pos.x, pos.y + stren_text_size.y);
                drawlist->AddText(stress_text_pos, BEAM_STRESS_TEXT_COLOR, buf);

                // TODO: Hydro stress
            }
        }
    }
}

void RoR::GfxActor::CycleDebugViews()
{
    switch (m_debug_view)
    {
    case DebugViewType::DEBUGVIEW_NONE:     m_debug_view = DebugViewType::DEBUGVIEW_SKELETON; break;
    case DebugViewType::DEBUGVIEW_SKELETON: m_debug_view = DebugViewType::DEBUGVIEW_NODES;    break;
    case DebugViewType::DEBUGVIEW_NODES:    m_debug_view = DebugViewType::DEBUGVIEW_BEAMS;    break;
    case DebugViewType::DEBUGVIEW_BEAMS:    m_debug_view = DebugViewType::DEBUGVIEW_NONE;     break;
    default:;
    }
}

void RoR::GfxActor::AddRod(int beam_index,  int node1_index, int node2_index, const char* material_name, bool visible, float diameter_meters)
{
    try
    {
        Str<100> entity_name;
        entity_name << "rod" << beam_index << "@actor" << m_actor->ar_instance_id;
        Ogre::Entity* entity = gEnv->sceneManager->createEntity(entity_name.ToCStr(), "beam.mesh");
        entity->setMaterialName(material_name);

        if (m_rods_parent_scenenode == nullptr)
        {
            m_rods_parent_scenenode = gEnv->sceneManager->getRootSceneNode()->createChildSceneNode();
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

        m_rods.push_back(rod);
    }
    catch (Ogre::Exception& e)
    {
        LogFormat("[RoR|Gfx] Failed to create visuals for beam %d, message: %s", beam_index, e.getFullDescription().c_str());
    }
}

void RoR::GfxActor::UpdateRods()
{
    // TODO: Apply visibility updates from a queue (to be implemented!)
    // fulltext-label: QUEUE_VIS_CHANGE

    for (Rod& rod: m_rods)
    {
        Ogre::Vector3 pos1 = m_actor->ar_nodes[rod.rod_node1].AbsPosition;
        Ogre::Vector3 pos2 = m_actor->ar_nodes[rod.rod_node2].AbsPosition;

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
    for (prop_t& prop: m_props)
    {
        if (prop.scene_node)
            prop.scene_node->scale(ratio, ratio, ratio);

        if (prop.wheel)
            prop.wheel->scale(ratio, ratio, ratio);

        if (prop.wheel)
            prop.wheelpos = relpos + (prop.wheelpos - relpos) * ratio;

        if (prop.beacon_flare_billboard_scene_node[0])
            prop.beacon_flare_billboard_scene_node[0]->scale(ratio, ratio, ratio);

        if (prop.beacon_flare_billboard_scene_node[1])
            prop.beacon_flare_billboard_scene_node[1]->scale(ratio, ratio, ratio);

        if (prop.beacon_flare_billboard_scene_node[2])
            prop.beacon_flare_billboard_scene_node[2]->scale(ratio, ratio, ratio);

        if (prop.beacon_flare_billboard_scene_node[3])
            prop.beacon_flare_billboard_scene_node[3]->scale(ratio, ratio, ratio);
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
        gEnv->sceneManager->getRootSceneNode()->addChild(m_rods_parent_scenenode);
    }
    else if (!visible && m_rods_parent_scenenode->isInSceneGraph())
    {
        gEnv->sceneManager->getRootSceneNode()->removeChild(m_rods_parent_scenenode);
    }
}

void RoR::GfxActor::UpdateSimDataBuffer()
{
    m_simbuf.simbuf_live_local = (m_actor->ar_sim_state == Actor::SimState::LOCAL_SIMULATED);
    m_simbuf.simbuf_pos = m_actor->getPosition();
    m_simbuf.simbuf_heading_angle = m_actor->getHeadingDirectionAngle();
    m_simbuf.simbuf_tyre_pressure = m_actor->GetTyrePressure();
    m_simbuf.simbuf_aabb = m_actor->ar_bounding_box;
    m_simbuf.simbuf_wheel_speed = m_actor->ar_wheel_speed;
    m_simbuf.simbuf_beaconlight_active = m_actor->m_beacon_light_is_active;
    m_simbuf.simbuf_cur_cinecam = m_actor->ar_current_cinecam;
    m_simbuf.simbuf_parking_brake = (m_actor->ar_parking_brake != 0);
    m_simbuf.simbuf_brake = m_actor->ar_brake;
    m_simbuf.simbuf_hydro_dir_state = m_actor->ar_hydro_dir_state;
    m_simbuf.simbuf_hydro_aileron_state = m_actor->ar_hydro_aileron_state;
    m_simbuf.simbuf_hydro_elevator_state = m_actor->ar_hydro_elevator_state;
    m_simbuf.simbuf_hydro_aero_rudder_state = m_actor->ar_hydro_rudder_state;
    m_simbuf.simbuf_aero_flap_state = m_actor->ar_aerial_flap;
    m_simbuf.simbuf_airbrake_state = m_actor->ar_airbrake_intensity;
    m_simbuf.simbuf_headlight_on = m_actor->ar_lights;
    m_simbuf.simbuf_direction = m_actor->getDirection();
    m_simbuf.simbuf_node0_velo = m_actor->ar_nodes[0].Velocity;
    if (m_simbuf.simbuf_net_username != m_actor->m_net_username)
    {
        m_simbuf.simbuf_net_username = m_actor->m_net_username;
    }

    // nodes
    const int num_nodes = m_actor->ar_num_nodes;
    for (int i = 0; i < num_nodes; ++i)
    {
        m_simbuf.simbuf_nodes.get()[i].AbsPosition = m_actor->ar_nodes[i].AbsPosition;
    }

    // airbrakes
    const int num_airbrakes = m_actor->ar_num_airbrakes;
    for (int i=0; i< num_airbrakes; ++i)
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
        m_simbuf.simbuf_clutch          = m_actor->ar_engine->GetClutch();
    }
    if (m_actor->m_num_axles > 0)
    {
        m_simbuf.simbuf_diff_type = m_actor->m_axles[0]->GetActiveDiffType();
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

        dst.simbuf_ae_throttle   = src->getThrottle();
        dst.simbuf_ae_rpmpc      = src->getRPMpc();
        dst.simbuf_ae_turboprop  = (src->getType() == AeroEngine::AEROENGINE_TYPE_TURBOPROP);
        dst.simbuf_ae_ignition   = src->getIgnition();
        dst.simbuf_ae_failed     = src->isFailed();

        if (dst.simbuf_ae_turboprop)
        {
            Turboprop* tp = static_cast<Turboprop*>(src);
            dst.simbuf_tp_aetorque = (100.0 * tp->indicated_torque / tp->max_torque); // TODO: Code ported as-is from calcAnimators(); what does it do? ~ only_a_ptr, 06/2018
            dst.simbuf_tp_aepitch = tp->pitch;
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
        m_simbuf.simbuf_autopilot_heading = m_actor->ar_autopilot->heading;
        m_simbuf.simbuf_autopilot_ils_available = m_actor->ar_autopilot->IsIlsAvailable();
        m_simbuf.simbuf_autopilot_ils_vdev = m_actor->ar_autopilot->GetVerticalApproachDeviation();
        m_simbuf.simbuf_autopilot_ils_hdev = m_actor->ar_autopilot->GetHorizontalApproachDeviation();
    }
}

bool RoR::GfxActor::IsActorLive() const
{
    return (m_actor->ar_sim_state < Actor::SimState::LOCAL_SLEEPING);
}

void RoR::GfxActor::UpdateCabMesh()
{
    // TODO: Hacky, requires 'friend' access to `Actor` -> move the visuals to GfxActor!
    if ((m_actor->m_cab_entity != nullptr) && (m_actor->m_cab_mesh != nullptr))
    {
        m_actor->m_cab_scene_node->setPosition(m_actor->m_cab_mesh->UpdateFlexObj());
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
    if (gEnv->threadPool)
    {
        for (WheelGfx& w: m_wheels)
        {
            if ((w.wx_scenenode != nullptr) && w.wx_flex_mesh->flexitPrepare())
            {
                auto func = std::function<void()>([this, w]()
                    {
                        w.wx_flex_mesh->flexitCompute();
                    });
                auto task_handle = gEnv->threadPool->RunTask(func);
                m_flexwheel_tasks.push_back(task_handle);
            }
        }
    }
    else
    {
        for (WheelGfx& w: m_wheels)
        {
            if ((w.wx_scenenode != nullptr) && w.wx_flex_mesh->flexitPrepare())
            {
                w.wx_flex_mesh->flexitCompute();
                w.wx_scenenode->setPosition(w.wx_flex_mesh->flexitFinal());
            }
        }
    }
}

void RoR::GfxActor::FinishWheelUpdates()
{
    if (gEnv->threadPool)
    {
        for (auto& task: m_flexwheel_tasks)
        {
            task->join();
        }
        for (WheelGfx& w: m_wheels)
        {
            if (w.wx_scenenode != nullptr)
            {
                w.wx_scenenode->setPosition(w.wx_flex_mesh->flexitFinal());
            }
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
int RoR::GfxActor::GetActorDriveable   () const { return m_actor->ar_driveable; }

void RoR::GfxActor::RegisterAirbrakes()
{
    // TODO: Quick hacky setup with `friend` access - we rely on old init code in RigSpawner/Airbrake.
    for (int i=0; i< m_actor->ar_num_airbrakes; ++i)
    {
        Airbrake* ab = m_actor->ar_airbrakes[i];
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

void RoR::GfxActor::UpdateAirbrakes()
{
    const size_t num_airbrakes = m_gfx_airbrakes.size();
    NodeData* nodes = m_simbuf.simbuf_nodes.get();
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
    NodeData* nodes = m_simbuf.simbuf_nodes.get();
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
        // this ensures that the nickname is always in a readable size
        Ogre::Vector3 label_pos = m_simbuf.simbuf_pos;
        label_pos.y += (m_simbuf.simbuf_aabb.getMaximum().y - m_simbuf.simbuf_aabb.getMinimum().y);
        m_actor->m_net_label_node->setPosition(label_pos);
        Ogre::Vector3 vdir = m_simbuf.simbuf_pos - gEnv->mainCamera->getPosition();
        float vlen = vdir.length();
        float h = std::max(0.6, vlen / 30.0);

        m_actor->m_net_label_mt->setCharacterHeight(h);
        if (vlen > 1000) // 1000 ... vlen
        {
            m_actor->m_net_label_mt->setCaption(
                m_simbuf.simbuf_net_username + "  (" + TOSTRING((float)(ceil(vlen / 100) / 10.0) ) + " km)");
        }
        else if (vlen > 20) // 20 ... vlen ... 1000
        {
            m_actor->m_net_label_mt->setCaption(
                m_simbuf.simbuf_net_username + "  (" + TOSTRING((int)vlen) + " m)");
        }
        else // 0 ... vlen ... 20
        {
            m_actor->m_net_label_mt->setCaption(m_simbuf.simbuf_net_username);
        }
    }
}

void RoR::GfxActor::CalculateDriverPos(Ogre::Vector3& out_pos, Ogre::Quaternion& out_rot)
{
    assert(m_driverseat_prop_index != -1);
    prop_t* driverseat_prop = &m_props[m_driverseat_prop_index];

    NodeData* nodes = this->GetSimNodeBuffer();

    const Ogre::Vector3 x_pos = nodes[driverseat_prop->nodex].AbsPosition;
    const Ogre::Vector3 y_pos = nodes[driverseat_prop->nodey].AbsPosition;
    const Ogre::Vector3 center_pos = nodes[driverseat_prop->noderef].AbsPosition;

    const Ogre::Vector3 x_vec = x_pos - center_pos;
    const Ogre::Vector3 y_vec = y_pos - center_pos;
    const Ogre::Vector3 normal = (y_vec.crossProduct(x_vec)).normalisedCopy();

    // Output position
    Ogre::Vector3 pos = center_pos;
    pos += (driverseat_prop->offsetx * x_vec);
    pos += (driverseat_prop->offsety * y_vec);
    pos += (driverseat_prop->offsetz * normal);
    out_pos = pos;

    // Output orientation
    const Ogre::Vector3 x_vec_norm = x_vec.normalisedCopy();
    const Ogre::Vector3 y_vec_norm = x_vec_norm.crossProduct(normal);
    Ogre::Quaternion rot(x_vec_norm, normal, y_vec_norm);
    rot = rot * driverseat_prop->rot;
    rot = rot * Ogre::Quaternion(Ogre::Degree(180), Ogre::Vector3::UNIT_Y); // rotate towards the driving direction
    out_rot = rot;
}

void RoR::GfxActor::UpdateBeaconFlare(prop_t & prop, float dt, bool is_player_actor)
{
    // TODO: Quick and dirty port from Beam::updateFlares(), clean it up ~ only_a_ptr, 06/2018
    using namespace Ogre;

    bool enableAll = !((App::gfx_flares_mode.GetActive() == GfxFlaresMode::CURR_VEHICLE_HEAD_ONLY) && !is_player_actor);
    NodeData* nodes = this->GetSimNodeBuffer();

    if (prop.beacontype == 'b')
    {
        // Get data
        Ogre::SceneNode* beacon_scene_node = prop.scene_node;
        Ogre::Quaternion beacon_orientation = beacon_scene_node->getOrientation();
        Ogre::Light* beacon_light = prop.beacon_light[0];
        float beacon_rotation_rate = prop.beacon_light_rotation_rate[0];
        float beacon_rotation_angle = prop.beacon_light_rotation_angle[0]; // Updated at end of block

        // Transform
        beacon_light->setPosition(beacon_scene_node->getPosition() + beacon_orientation * Ogre::Vector3(0, 0, 0.12));
        beacon_rotation_angle += dt * beacon_rotation_rate;//rotate baby!
        beacon_light->setDirection(beacon_orientation * Ogre::Vector3(cos(beacon_rotation_angle), sin(beacon_rotation_angle), 0));
        //billboard
        Ogre::Vector3 vdir = beacon_light->getPosition() - gEnv->mainCamera->getPosition(); // TODO: verify the position is already updated here ~ only_a_ptr, 06/2018
        float vlen = vdir.length();
        if (vlen > 100.0)
        {
            prop.beacon_flare_billboard_scene_node[0]->setVisible(false);
            return;
        }
        //normalize
        vdir = vdir / vlen;
        prop.beacon_flare_billboard_scene_node[0]->setPosition(beacon_light->getPosition() - vdir * 0.1);
        float amplitude = beacon_light->getDirection().dotProduct(vdir);
        if (amplitude > 0)
        {
            prop.beacon_flare_billboard_scene_node[0]->setVisible(true);
            prop.beacon_flares_billboard_system[0]->setDefaultDimensions(amplitude * amplitude * amplitude, amplitude * amplitude * amplitude);
        }
        else
        {
            prop.beacon_flare_billboard_scene_node[0]->setVisible(false);
        }
        beacon_light->setVisible(enableAll);

        // Update
        prop.beacon_light_rotation_angle[0] = beacon_rotation_angle;
        // NOTE: Light position is not updated here!
    }
    else if (prop.beacontype == 'p')
    {
        for (int k = 0; k < 4; k++)
        {
            //update light
            Quaternion orientation = prop.scene_node->getOrientation();
            switch (k)
            {
            case 0: prop.beacon_light[k]->setPosition(prop.scene_node->getPosition() + orientation * Vector3(-0.64, 0, 0.14));
                break;
            case 1: prop.beacon_light[k]->setPosition(prop.scene_node->getPosition() + orientation * Vector3(-0.32, 0, 0.14));
                break;
            case 2: prop.beacon_light[k]->setPosition(prop.scene_node->getPosition() + orientation * Vector3(+0.32, 0, 0.14));
                break;
            case 3: prop.beacon_light[k]->setPosition(prop.scene_node->getPosition() + orientation * Vector3(+0.64, 0, 0.14));
                break;
            }
            prop.beacon_light_rotation_angle[k] += dt * prop.beacon_light_rotation_rate[k];//rotate baby!
            prop.beacon_light[k]->setDirection(orientation * Vector3(cos(prop.beacon_light_rotation_angle[k]), sin(prop.beacon_light_rotation_angle[k]), 0));
            //billboard
            Vector3 vdir = prop.beacon_light[k]->getPosition() - gEnv->mainCamera->getPosition();
            float vlen = vdir.length();
            if (vlen > 100.0)
            {
                prop.beacon_flare_billboard_scene_node[k]->setVisible(false);
                continue;
            }
            //normalize
            vdir = vdir / vlen;
            prop.beacon_flare_billboard_scene_node[k]->setPosition(prop.beacon_light[k]->getPosition() - vdir * 0.2);
            float amplitude = prop.beacon_light[k]->getDirection().dotProduct(vdir);
            if (amplitude > 0)
            {
                prop.beacon_flare_billboard_scene_node[k]->setVisible(true);
                prop.beacon_flares_billboard_system[k]->setDefaultDimensions(amplitude * amplitude * amplitude, amplitude * amplitude * amplitude);
            }
            else
            {
                prop.beacon_flare_billboard_scene_node[k]->setVisible(false);
            }
            prop.beacon_light[k]->setVisible(enableAll);
        }
    }
    else if (prop.beacontype == 'r')
    {
        //update light
        Quaternion orientation = prop.scene_node->getOrientation();
        prop.beacon_light[0]->setPosition(prop.scene_node->getPosition() + orientation * Vector3(0, 0, 0.06));
        prop.beacon_light_rotation_angle[0] += dt * prop.beacon_light_rotation_rate[0];//rotate baby!
        //billboard
        Vector3 vdir = prop.beacon_light[0]->getPosition() - gEnv->mainCamera->getPosition();
        float vlen = vdir.length();
        if (vlen > 100.0)
        {
            prop.beacon_flare_billboard_scene_node[0]->setVisible(false);
            return;
        }
        //normalize
        vdir = vdir / vlen;
        prop.beacon_flare_billboard_scene_node[0]->setPosition(prop.beacon_light[0]->getPosition() - vdir * 0.1);
        bool visible = false;
        if (prop.beacon_light_rotation_angle[0] > 1.0)
        {
            prop.beacon_light_rotation_angle[0] = 0.0;
            visible = true;
        }
        visible = visible && enableAll;
        prop.beacon_light[0]->setVisible(visible);
        prop.beacon_flare_billboard_scene_node[0]->setVisible(visible);
    }
    if (prop.beacontype == 'R' || prop.beacontype == 'L')
    {
        Vector3 mposition = nodes[prop.noderef].AbsPosition + prop.offsetx * (nodes[prop.nodex].AbsPosition - nodes[prop.noderef].AbsPosition) + prop.offsety * (nodes[prop.nodey].AbsPosition - nodes[prop.noderef].AbsPosition);
        //billboard
        Vector3 vdir = mposition - gEnv->mainCamera->getPosition();
        float vlen = vdir.length();
        if (vlen > 100.0)
        {
            prop.beacon_flare_billboard_scene_node[0]->setVisible(false);
            return;
        }
        //normalize
        vdir = vdir / vlen;
        prop.beacon_flare_billboard_scene_node[0]->setPosition(mposition - vdir * 0.1);
    }
    if (prop.beacontype == 'w')
    {
        Vector3 mposition = nodes[prop.noderef].AbsPosition + prop.offsetx * (nodes[prop.nodex].AbsPosition - nodes[prop.noderef].AbsPosition) + prop.offsety * (nodes[prop.nodey].AbsPosition - nodes[prop.noderef].AbsPosition);
        prop.beacon_light[0]->setPosition(mposition);
        prop.beacon_light_rotation_angle[0] += dt * prop.beacon_light_rotation_rate[0];//rotate baby!
        //billboard
        Vector3 vdir = mposition - gEnv->mainCamera->getPosition();
        float vlen = vdir.length();
        if (vlen > 100.0)
        {
            prop.beacon_flare_billboard_scene_node[0]->setVisible(false);
            return;
        }
        //normalize
        vdir = vdir / vlen;
        prop.beacon_flare_billboard_scene_node[0]->setPosition(mposition - vdir * 0.1);
        bool visible = false;
        if (prop.beacon_light_rotation_angle[0] > 1.0)
        {
            prop.beacon_light_rotation_angle[0] = 0.0;
            visible = true;
        }
        visible = visible && enableAll;
        prop.beacon_light[0]->setVisible(visible);
        prop.beacon_flare_billboard_scene_node[0]->setVisible(visible);
    }
}

void RoR::GfxActor::UpdateProps(float dt, bool is_player_actor)
{
    using namespace Ogre;

    if (m_simbuf.simbuf_beaconlight_active != m_beaconlight_active)
    {
        m_beaconlight_active = m_simbuf.simbuf_beaconlight_active;
        this->SetBeaconsEnabled(m_beaconlight_active);   
    }

    NodeData* nodes = this->GetSimNodeBuffer();

    for (prop_t& prop: m_props)
    {
        if (!prop.scene_node)
            continue;

        // -- quick ugly port from `Actor::NotifyCameraChanged()`~ 06/2018
        const bool mo_visible = (prop.cameramode == -2 || prop.cameramode == m_simbuf.simbuf_cur_cinecam);
        if (prop.mo != nullptr)
        {
            prop.mo->setVisible(mo_visible);
        }

        // -- quick ugly port from `Actor::updateProps()` --- ~ 06/2018
        Vector3 diffX = nodes[prop.nodex].AbsPosition - nodes[prop.noderef].AbsPosition;
        Vector3 diffY = nodes[prop.nodey].AbsPosition - nodes[prop.noderef].AbsPosition;

        Vector3 normal = (diffY.crossProduct(diffX)).normalisedCopy();

        Vector3 mposition = nodes[prop.noderef].AbsPosition + prop.offsetx * diffX + prop.offsety * diffY;
        prop.scene_node->setPosition(mposition + normal * prop.offsetz);

        Vector3 refx = diffX.normalisedCopy();
        Vector3 refy = refx.crossProduct(normal);
        Quaternion orientation = Quaternion(refx, normal, refy) * prop.rot;
        prop.scene_node->setOrientation(orientation);

        if (prop.wheel) // special prop - steering wheel
        {
            Quaternion brot = Quaternion(Degree(-59.0), Vector3::UNIT_X);
            brot = brot * Quaternion(Degree(m_simbuf.simbuf_hydro_dir_state * prop.wheelrotdegree), Vector3::UNIT_Y);
            prop.wheel->setPosition(mposition + normal * prop.offsetz + orientation * prop.wheelpos);
            prop.wheel->setOrientation(orientation * brot);
        }

        // --- quick ugly port from `Actor::updateFlares()` ~ 06/2018
        if ((App::gfx_flares_mode.GetActive() != GfxFlaresMode::NONE)
            && (prop.beacontype != 0)
            && (m_beaconlight_active))
        {
            this->UpdateBeaconFlare(prop, dt, is_player_actor); 
        }
    }
}

void RoR::GfxActor::SetPropsVisible(bool visible)
{
    for (prop_t& prop: m_props)
    {
        if (prop.mo)
            prop.mo->setVisible(visible);
        if (prop.wheel)
            prop.wheel->setVisible(visible);
        if (prop.beacon_flare_billboard_scene_node[0])
            prop.beacon_flare_billboard_scene_node[0]->setVisible(visible);
        if (prop.beacon_flare_billboard_scene_node[1])
            prop.beacon_flare_billboard_scene_node[1]->setVisible(visible);
        if (prop.beacon_flare_billboard_scene_node[2])
            prop.beacon_flare_billboard_scene_node[2]->setVisible(visible);
        if (prop.beacon_flare_billboard_scene_node[3])
            prop.beacon_flare_billboard_scene_node[3]->setVisible(visible);
    }
}

void RoR::GfxActor::SetBeaconsEnabled(bool beacon_light_is_active)
{
    const bool enableLight = (App::gfx_flares_mode.GetActive() != GfxFlaresMode::NO_LIGHTSOURCES);

    for (prop_t& prop: m_props)
    {
        char beacon_type = prop.beacontype;
        if (beacon_type == 'b')
        {
            prop.beacon_light[0]->setVisible(beacon_light_is_active && enableLight);
            prop.beacon_flare_billboard_scene_node[0]->setVisible(beacon_light_is_active);
            if (prop.beacon_flares_billboard_system[0] && beacon_light_is_active && !prop.beacon_flare_billboard_scene_node[0]->numAttachedObjects())
            {
                prop.beacon_flares_billboard_system[0]->setVisible(true);
                prop.beacon_flare_billboard_scene_node[0]->attachObject(prop.beacon_flares_billboard_system[0]);
            }
            else if (prop.beacon_flares_billboard_system[0] && !beacon_light_is_active)
            {
                prop.beacon_flare_billboard_scene_node[0]->detachAllObjects();
                prop.beacon_flares_billboard_system[0]->setVisible(false);
            }
        }
        else if (beacon_type == 'R' || beacon_type == 'L')
        {
            prop.beacon_flare_billboard_scene_node[0]->setVisible(beacon_light_is_active);
            if (prop.beacon_flares_billboard_system[0] && beacon_light_is_active && !prop.beacon_flare_billboard_scene_node[0]->numAttachedObjects())
                prop.beacon_flare_billboard_scene_node[0]->attachObject(prop.beacon_flares_billboard_system[0]);
            else if (prop.beacon_flares_billboard_system[0] && !beacon_light_is_active)
                prop.beacon_flare_billboard_scene_node[0]->detachAllObjects();
        }
        else if (beacon_type == 'p')
        {
            for (int k = 0; k < 4; k++)
            {
                prop.beacon_light[k]->setVisible(beacon_light_is_active && enableLight);
                prop.beacon_flare_billboard_scene_node[k]->setVisible(beacon_light_is_active);
                if (prop.beacon_flares_billboard_system[k] && beacon_light_is_active && !prop.beacon_flare_billboard_scene_node[k]->numAttachedObjects())
                    prop.beacon_flare_billboard_scene_node[k]->attachObject(prop.beacon_flares_billboard_system[k]);
                else if (prop.beacon_flares_billboard_system[k] && !beacon_light_is_active)
                    prop.beacon_flare_billboard_scene_node[k]->detachAllObjects();
            }
        }
        else
        {
            for (int k = 0; k < 4; k++)
            {
                if (prop.beacon_light[k])
                {
                    prop.beacon_light[k]->setVisible(beacon_light_is_active && enableLight);
                }
                if (prop.beacon_flare_billboard_scene_node[k])
                {
                    prop.beacon_flare_billboard_scene_node[k]->setVisible(beacon_light_is_active);

                    if (prop.beacon_flares_billboard_system[k] && beacon_light_is_active && !prop.beacon_flare_billboard_scene_node[k]->numAttachedObjects())
                    {
                        prop.beacon_flare_billboard_scene_node[k]->attachObject(prop.beacon_flares_billboard_system[k]);
                    }
                    else if (prop.beacon_flares_billboard_system[k] && !beacon_light_is_active)
                    {
                        prop.beacon_flare_billboard_scene_node[k]->detachAllObjects();
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
    if (flag_state & ANIM_FLAG_BRUDDER)
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
    if (flag_state & ANIM_FLAG_BTHROTTLE)
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
    if (flag_state & ANIM_FLAG_DIFFLOCK)
    {
        if (m_actor->m_num_axles > 0) // read-only attribute - safe to read from here
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
    if (flag_state & ANIM_FLAG_HEADING)
    {
        // rad2deg limitedrange  -1 to +1
        cstate = (m_simbuf.simbuf_heading_angle * 57.29578f) / 360.0f;
        div++;
    }

    //torque - WRITES 
    const bool has_engine = (m_actor->ar_engine!= nullptr);
    if (has_engine && flag_state & ANIM_FLAG_TORQUE)
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
    if (has_engine && (flag_state & ANIM_FLAG_SHIFTER) && option3 == 3.0f)
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
    if (has_engine && (flag_state & ANIM_FLAG_SHIFTER) && option3 == 1.0f)
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
    if (has_engine && (flag_state & ANIM_FLAG_SHIFTER) && option3 == 2.0f)
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
    if (has_engine && (flag_state & ANIM_FLAG_SHIFTER) && option3 == 4.0f)
    {
        int shifter = m_simbuf.simbuf_gear;
        int numgears = m_attr.xa_num_gears;
        cstate -= (shifter + 2.0) / (numgears + 2.0);
        div++;
    }

    //parking brake
    if (flag_state & ANIM_FLAG_PBRAKE)
    {
        float pbrake = static_cast<float>(m_simbuf.simbuf_parking_brake); // Bool --> float
        cstate -= pbrake;
        div++;
    }

    //speedo ( scales with speedomax )
    if (flag_state & ANIM_FLAG_SPEEDO)
    {
        float speedo = m_simbuf.simbuf_wheel_speed / m_attr.xa_speedo_highest_kph;
        cstate -= speedo * 3.0f;
        div++;
    }

    //engine tacho ( scales with maxrpm, default is 3500 )
    if (has_engine && flag_state & ANIM_FLAG_TACHO)
    {
        float tacho = m_simbuf.simbuf_engine_rpm / m_attr.xa_engine_max_rpm;
        cstate -= tacho;
        div++;
    }

    //turbo
    if (has_engine && flag_state & ANIM_FLAG_TURBO)
    {
        float turbo = m_simbuf.simbuf_engine_turbo_psi * 3.34;
        cstate -= turbo / 67.0f;
        div++;
    }

    //brake
    if (flag_state & ANIM_FLAG_BRAKE)
    {
        float brakes = m_simbuf.simbuf_brake / m_attr.xa_brake_force;
        cstate -= brakes;
        div++;
    }

    //accelerator
    if (has_engine && flag_state & ANIM_FLAG_ACCEL)
    {
        float accel = m_simbuf.simbuf_engine_accel;
        cstate -= accel + 0.06f;
        //( small correction, get acc is nver smaller then 0.06.
        div++;
    }

    //clutch
    if (has_engine && flag_state & ANIM_FLAG_CLUTCH)
    {
        float clutch = m_simbuf.simbuf_clutch;
        cstate -= fabs(1.0f - clutch);
        div++;
    }

    //aeroengines rpm + throttle + torque ( turboprop ) + pitch ( turboprop ) + status +  fire
    int ftp = m_simbuf.simbuf_aeroengines.size();

    // TODO: Investigate the `(ftp > option3 - 1.0f)` condition copypasted from `Actor::calcAnimators()` [see ## dev note on top]
    //       - produces bogus aeroengine index when no aeroengines are defined
    //       I had to add the `(ftp!=0)` to bypass it ~ only_a_ptr, 2018
    if ((ftp != 0) && (ftp > option3 - 1.0f))
    {
        int aenum = int(option3 - 1.0f);
        if (flag_state & ANIM_FLAG_RPM)
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
        if (flag_state & ANIM_FLAG_THROTTLE)
        {
            float throttle = m_simbuf.simbuf_aeroengines[aenum].simbuf_ae_throttle;
            cstate -= throttle;
            div++;
        }

        if (m_simbuf.simbuf_aeroengines[aenum].simbuf_ae_turboprop) // If it's a turboprop or pistonprop...
        {
            if (flag_state & ANIM_FLAG_AETORQUE)
            {
                cstate = m_simbuf.simbuf_aeroengines[aenum].simbuf_tp_aetorque / 120.0f;
                div++;
            }

            if (flag_state & ANIM_FLAG_AEPITCH)
            {
                cstate = m_simbuf.simbuf_aeroengines[aenum].simbuf_tp_aepitch / 120.0f;
                div++;
            }
        }

        if (flag_state & ANIM_FLAG_AESTATUS)
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
    if (flag_state & ANIM_FLAG_AIRSPEED)
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
    if (flag_state & ANIM_FLAG_VVI)
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
    if (flag_state & ANIM_FLAG_ALTIMETER)
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
    if (flag_state & ANIM_FLAG_AOA)
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

    Ogre::Vector3 cam_pos  = node0_pos;
    Ogre::Vector3 cam_roll = node0_pos;
    Ogre::Vector3 cam_dir  = node0_pos;

    // NOTE: "cameras" are constant attributes defined in Truckfile - safe to read from here
    if (m_actor->IsNodeIdValid(m_actor->ar_camera_node_pos[0])) // TODO: why check this on each update when it cannot change after spawn?
    {
        cam_pos  = this->GetSimNodeBuffer()[m_actor->ar_camera_node_pos[0] ].AbsPosition;
        cam_roll = this->GetSimNodeBuffer()[m_actor->ar_camera_node_roll[0]].AbsPosition;
        cam_dir  = this->GetSimNodeBuffer()[m_actor->ar_camera_node_dir[0] ].AbsPosition;
    }

    // roll
    if (flag_state & ANIM_FLAG_ROLL)
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
    if (flag_state & ANIM_FLAG_PITCH)
    {
        Ogre::Vector3 dirv = (cam_pos - cam_dir).normalisedCopy();
        float pitchangle = asin(dirv.dotProduct(Ogre::Vector3::UNIT_Y));
        // radian to degrees with a max cstate of +/- 1.0
        cstate = (Ogre::Math::RadiansToDegrees(pitchangle) / 90.0f);
        div++;
    }

    // airbrake
    if (flag_state & ANIM_FLAG_AIRBRAKE)
    {
        float airbrake = static_cast<float>(m_simbuf.simbuf_airbrake_state);
        // cstate limited to -1.0f
        cstate -= airbrake / 5.0f;
        div++;
    }

    //flaps
    if (flag_state & ANIM_FLAG_FLAP)
    {
        float flaps = flapangles[m_simbuf.simbuf_aero_flap_state];
        // cstate limited to -1.0f
        cstate = flaps;
        div++;
    }
}

void RoR::GfxActor::UpdatePropAnimations(const float dt)
{
    for (prop_t& prop: m_props)
    {
        int animnum = 0;
        float rx = 0.0f;
        float ry = 0.0f;
        float rz = 0.0f;

        while (prop.animFlags[animnum])
        {
            float cstate = 0.0f;
            int div = 0.0f;
            int flagstate = prop.animFlags[animnum];
            const float lower_limit = prop.constraints[animnum].lower_limit;
            const float upper_limit = prop.constraints[animnum].upper_limit;
            float animOpt3 = prop.animOpt3[animnum];

            this->CalcPropAnimation(flagstate, cstate, div, dt, lower_limit, upper_limit, animOpt3);

            // key triggered animations
            if ((prop.animFlags[animnum] & ANIM_FLAG_EVENT) && prop.animKey[animnum] != -1)
            {
                // TODO: Keys shouldn't be queried from here, but buffered in sim. loop ~ only_a_ptr, 06/2018
                if (RoR::App::GetInputEngine()->getEventValue(prop.animKey[animnum]))
                {
                    // keystatelock is disabled then set cstate
                    if (prop.animKeyState[animnum] == -1.0f)
                    {
                        // TODO: Keys shouldn't be queried from here, but buffered in sim. loop ~ only_a_ptr, 06/2018
                        cstate += RoR::App::GetInputEngine()->getEventValue(prop.animKey[animnum]);
                    }
                    else if (!prop.animKeyState[animnum])
                    {
                        // a key was pressed and a toggle was done already, so bypass
                        //toggle now
                        if (!prop.lastanimKS[animnum])
                        {
                            prop.lastanimKS[animnum] = 1.0f;
                            // use animkey as bool to determine keypress / release state of inputengine
                            prop.animKeyState[animnum] = 1.0f;
                        }
                        else
                        {
                            prop.lastanimKS[animnum] = 0.0f;
                            // use animkey as bool to determine keypress / release state of inputengine
                            prop.animKeyState[animnum] = 1.0f;
                        }
                    }
                    else
                    {
                        // bypas mode, get the last set position and set it
                        cstate += prop.lastanimKS[animnum];
                    }
                }
                else
                {
                    // keyevent exists and keylock is enabled but the key isnt pressed right now = get lastanimkeystatus for cstate and reset keypressed bool animkey
                    if (prop.animKeyState[animnum] != -1.0f)
                    {
                        cstate += prop.lastanimKS[animnum];
                        prop.animKeyState[animnum] = 0.0f;
                    }
                }
            }

            //propanimation placed here to avoid interference with existing hydros(cstate) and permanent prop animation
            //land vehicle steering
            if (prop.animFlags[animnum] & ANIM_FLAG_STEERING)
                cstate += m_simbuf.simbuf_hydro_dir_state;
            //aileron
            if (prop.animFlags[animnum] & ANIM_FLAG_AILERONS)
                cstate += m_simbuf.simbuf_hydro_aileron_state;
            //elevator
            if (prop.animFlags[animnum] & ANIM_FLAG_ELEVATORS)
                cstate += m_simbuf.simbuf_hydro_elevator_state;
            //rudder
            if (prop.animFlags[animnum] & ANIM_FLAG_ARUDDER)
                cstate += m_simbuf.simbuf_hydro_aero_rudder_state;
            //permanent
            if (prop.animFlags[animnum] & ANIM_FLAG_PERMANENT)
                cstate += 1.0f;

            cstate *= prop.animratio[animnum];

            // autoanimate noflip_bouncer
            if (prop.animOpt5[animnum])
                cstate *= (prop.animOpt5[animnum]);

            //rotate prop
            if ((prop.animMode[animnum] & ANIM_MODE_ROTA_X) || (prop.animMode[animnum] & ANIM_MODE_ROTA_Y) || (prop.animMode[animnum] & ANIM_MODE_ROTA_Z))
            {
                float limiter = 0.0f;
                // This code was formerly executed within a fixed timestep of 0.5ms and finetuned accordingly.
                // This is now taken into account by factoring in the respective fraction of the variable timestep.
                float const dt_frac = dt * 2000.f;
                if (prop.animMode[animnum] & ANIM_MODE_AUTOANIMATE)
                {
                    if (prop.animMode[animnum] & ANIM_MODE_ROTA_X)
                    {
                        prop.rotaX += cstate * dt_frac;
                        limiter = prop.rotaX;
                    }
                    if (prop.animMode[animnum] & ANIM_MODE_ROTA_Y)
                    {
                        prop.rotaY += cstate * dt_frac;
                        limiter = prop.rotaY;
                    }
                    if (prop.animMode[animnum] & ANIM_MODE_ROTA_Z)
                    {
                        prop.rotaZ += cstate * dt_frac;
                        limiter = prop.rotaZ;
                    }
                }
                else
                {
                    if (prop.animMode[animnum] & ANIM_MODE_ROTA_X)
                        rx += cstate;
                    if (prop.animMode[animnum] & ANIM_MODE_ROTA_Y)
                        ry += cstate;
                    if (prop.animMode[animnum] & ANIM_MODE_ROTA_Z)
                        rz += cstate;
                }

                bool limiterchanged = false;
                // check if a positive custom limit is set to evaluate/calc flip back

                if (limiter > upper_limit)
                {
                    if (prop.animMode[animnum] & ANIM_MODE_NOFLIP)
                    {
                        limiter = upper_limit; // stop at limit
                        prop.animOpt5[animnum] *= -1.0f; // change cstate multiplier if bounce is set
                    }
                    else
                    {
                        limiter = lower_limit; // flip to other side at limit
                    }
                    limiterchanged = true;
                }

                if (limiter < lower_limit)
                {
                    if (prop.animMode[animnum] & ANIM_MODE_NOFLIP)
                    {
                        limiter = lower_limit; // stop at limit
                        prop.animOpt5[animnum] *= -1.0f; // change cstate multiplier if active
                    }
                    else
                    {
                        limiter = upper_limit; // flip to other side at limit
                    }
                    limiterchanged = true;
                }

                if (limiterchanged)
                {
                    if (prop.animMode[animnum] & ANIM_MODE_ROTA_X)
                        prop.rotaX = limiter;
                    if (prop.animMode[animnum] & ANIM_MODE_ROTA_Y)
                        prop.rotaY = limiter;
                    if (prop.animMode[animnum] & ANIM_MODE_ROTA_Z)
                        prop.rotaZ = limiter;
                }
            }

            //offset prop

            if ((prop.animMode[animnum] & ANIM_MODE_OFFSET_X) || (prop.animMode[animnum] & ANIM_MODE_OFFSET_Y) || (prop.animMode[animnum] & ANIM_MODE_OFFSET_Z))
            {
                float offset = 0.0f;
                float autooffset = 0.0f;

                if (prop.animMode[animnum] & ANIM_MODE_OFFSET_X)
                    offset = prop.orgoffsetX;
                if (prop.animMode[animnum] & ANIM_MODE_OFFSET_Y)
                    offset = prop.orgoffsetY;
                if (prop.animMode[animnum] & ANIM_MODE_OFFSET_Z)
                    offset = prop.orgoffsetZ;

                if (prop.animMode[animnum] & ANIM_MODE_AUTOANIMATE)
                {
                    // This code was formerly executed within a fixed timestep of 0.5ms and finetuned accordingly.
                    // This is now taken into account by factoring in the respective fraction of the variable timestep.
                    float const dt_frac = dt * 2000.f;
                    autooffset = offset + cstate * dt_frac;

                    if (autooffset > upper_limit)
                    {
                        if (prop.animMode[animnum] & ANIM_MODE_NOFLIP)
                        {
                            autooffset = upper_limit; // stop at limit
                            prop.animOpt5[animnum] *= -1.0f; // change cstate multiplier if active
                        }
                        else
                        {
                            autooffset = lower_limit; // flip to other side at limit
                        }
                    }

                    if (autooffset < lower_limit)
                    {
                        if (prop.animMode[animnum] & ANIM_MODE_NOFLIP)
                        {
                            autooffset = lower_limit; // stop at limit
                            prop.animOpt5[animnum] *= -1.0f; // change cstate multiplier if active
                        }
                        else
                        {
                            autooffset = upper_limit; // flip to other side at limit
                        }
                    }
                }
                offset += cstate;

                if (prop.animMode[animnum] & ANIM_MODE_OFFSET_X)
                {
                    prop.offsetx = offset;
                    if (prop.animMode[animnum] & ANIM_MODE_AUTOANIMATE)
                        prop.orgoffsetX = autooffset;
                }
                if (prop.animMode[animnum] & ANIM_MODE_OFFSET_Y)
                {
                    prop.offsety = offset;
                    if (prop.animMode[animnum] & ANIM_MODE_AUTOANIMATE)
                        prop.orgoffsetY = autooffset;
                }
                if (prop.animMode[animnum] & ANIM_MODE_OFFSET_Z)
                {
                    prop.offsetz = offset;
                    if (prop.animMode[animnum] & ANIM_MODE_AUTOANIMATE)
                        prop.orgoffsetZ = autooffset;
                }
            }
            animnum++;
        }
        //recalc the quaternions with final stacked rotation values ( rx, ry, rz )
        rx += prop.rotaX;
        ry += prop.rotaY;
        rz += prop.rotaZ;

        prop.rot = Ogre::Quaternion(Ogre::Degree(rz), Ogre::Vector3::UNIT_Z) * 
                   Ogre::Quaternion(Ogre::Degree(ry), Ogre::Vector3::UNIT_Y) *
                   Ogre::Quaternion(Ogre::Degree(rx), Ogre::Vector3::UNIT_X);
    }
}

void RoR::GfxActor::UpdateFlexbodies()
{
    m_flexbody_tasks.clear();
    if (gEnv->threadPool) // TODO: This dual processing is obsolete. Configuring threadpool size to 1 should do the same thing ~ only_a_ptr, 05/2018
    {
        for (FlexBody* fb: m_flexbodies)
        {
            const int camera_mode = fb->getCameraMode();
            if ((camera_mode == -2) || (camera_mode == m_simbuf.simbuf_cur_cinecam))
            {
                auto func = std::function<void()>([fb]()
                    {
                        fb->ComputeFlexbody();
                    });
                auto task_handle = gEnv->threadPool->RunTask(func);
                m_flexbody_tasks.push_back(task_handle);
            }
            else
            {
                fb->setVisible(false);
            }
        }
    }
    else
    {
        for (FlexBody* fb: m_flexbodies)
        {
            const int camera_mode = fb->getCameraMode();
            if ((fb->getCameraMode() == -2) || (fb->getCameraMode() == m_simbuf.simbuf_cur_cinecam))
            {
                fb->ComputeFlexbody();
            }
            else
            {
                fb->setVisible(false);
            }
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
    if (gEnv->threadPool)
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
}

void RoR::GfxActor::UpdateFlares(float dt_sec, bool is_player)
{
    // == Flare states are determined in simulation, this function only applies them to OGRE objects ==

    bool enableAll = ((App::gfx_flares_mode.GetActive() == GfxFlaresMode::CURR_VEHICLE_HEAD_ONLY) && !is_player);
    NodeData* nodes = this->GetSimNodeBuffer();

    int num_flares = static_cast<int>(m_actor->ar_flares.size());
    for (int i=0; i<num_flares; ++i)
    {
        flare_t& flare = m_actor->ar_flares[i];
        
        //TODO: Following code is a quick+dirty port from `Actor::updateFlares()` - tidy it up! ~only_a_ptr, 06/2018

        if (flare.type == 'f')
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
        Ogre::Vector3 vdir = mposition - gEnv->mainCamera->getPosition();
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
    m_actor->SetPropsCastShadows(value); // TODO: move these objects to GfxActor! ~ 08/2018

    // Props
    for (prop_t& prop: m_props)
    {
        if (prop.mo != nullptr)
            prop.mo->getEntity()->setCastShadows(value);
        if (prop.wheelmo != nullptr)
            prop.wheelmo->getEntity()->setCastShadows(value);
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
