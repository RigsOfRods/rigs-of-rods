/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2016-2017 Petr Ohlidal & contributors

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

#include "Beam.h"
#include "beam_t.h"
#include "GlobalEnvironment.h" // TODO: Eliminate!
#include "SkyManager.h"
#include "imgui.h"
#include "Utils.h"

#include <OgreResourceGroupManager.h>
#include <OgreTechnique.h>
#include <OgreTextureUnitState.h>
#include <OgrePass.h>

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
    vcam_node_center(nullptr),            // node_t*              
    vcam_node_dir_y(nullptr),             // node_t*              
    vcam_node_dir_z(nullptr),             // node_t*              
    vcam_node_alt_pos(nullptr),           // node_t*              
    vcam_node_lookat(nullptr),            // node_t*              
    vcam_pos_offset(Ogre::Vector3::ZERO), // Ogre::Vector3         
    vcam_ogre_camera(nullptr),            // Ogre::Camera*        
    vcam_render_target(nullptr),          // Ogre::RenderTexture* 
    vcam_debug_node(nullptr),             // Ogre::SceneNode*     
    vcam_render_window(nullptr),          // Ogre::RenderWindow*  
    vcam_prop_scenenode(nullptr)          // Ogre::SceneNode*     
{}  

void RoR::GfxActor::UpdateVideoCameras(float dt_sec)
{
    if (m_vidcam_state != VideoCamState::VCSTATE_ENABLED_ONLINE)
        return;

    for (GfxActor::VideoCamera vidcam: m_videocameras)
    {
#ifdef USE_CAELUM
        // caelum needs to know that we changed the cameras
        if (gEnv->sky && RoR::App::app_state.GetActive() == RoR::AppState::SIMULATION)
        {
            gEnv->sky->notifyCameraChanged(vidcam.vcam_ogre_camera);
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
                mirror_angle = m_actor->leftMirrorAngle;
                offset = Ogre::Vector3(0.07f, -0.22f, 0);
            }
            else
            {
                mirror_angle = m_actor->rightMirrorAngle;
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
        const Ogre::Vector3 abs_pos_center = vidcam.vcam_node_center->AbsPosition;
        const Ogre::Vector3 abs_pos_z = vidcam.vcam_node_dir_z->AbsPosition;
        const Ogre::Vector3 abs_pos_y = vidcam.vcam_node_dir_y->AbsPosition;
        Ogre::Vector3 normal = (-(abs_pos_center - abs_pos_z)).crossProduct(-(abs_pos_center - abs_pos_y));
        normal.normalise();

        // add user set offset
        Ogre::Vector3 pos = vidcam.vcam_node_alt_pos->AbsPosition +
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
            normal = vidcam.vcam_node_lookat->AbsPosition - pos;
            normal.normalise();
            Ogre::Vector3 refx = abs_pos_z - abs_pos_center;
            refx.normalise();
            // why does this flip ~2-3° around zero orientation and only with trackercam. back to slower crossproduct calc, a bit slower but better .. sigh
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

const ImU32 BEAM_COLOR               (0xff556633); // All colors are in ABGR format (alpha, blue, green, red)
const float BEAM_THICKNESS           (1.2f);
const ImU32 BEAM_BROKEN_COLOR        (0xff4466dd);
const float BEAM_BROKEN_THICKNESS    (1.8f);
const ImU32 BEAM_HYDRO_COLOR         (0xff55a3e0);
const float BEAM_HYDRO_THICKNESS     (1.4f);
const ImU32 BEAM_STRENGTH_TEXT_COLOR (0xffcfd0cc);
const ImU32 BEAM_STRESS_TEXT_COLOR   (0xff58bbfc);
const ImU32 BEAM_COMPRESS_TEXT_COLOR (0xffccbf3c);
const ImU32 BEAM_HYDRO_TEXT_COLOR    (0xffaa8844);
// TODO: commands cannot be distinguished on runtime

const ImU32 NODE_COLOR               (0xff44ddff);
const float NODE_RADIUS              (2.f);
const ImU32 NODE_TEXT_COLOR          (0xffcccccf); // node ID text color
const ImU32 NODE_MASS_TEXT_COLOR     (0xff77bb66);
const ImU32 NODE_PRELOCK_COLOR       (0xffee9933); // PRELOCK/LOCKED states are indicated by extra circle around node
const float NODE_PRELOCK_THICKNESS   (2.3f);
const float NODE_PRELOCK_RADIUS      (4.f);
const ImU32 NODE_LOCKED_COLOR        (0xffbb8844);
const float NODE_LOCKED_THICKNESS    (1.4f);
const float NODE_LOCKED_RADIUS       (2.6f);
const ImU32 NODE_PREUNLOCK_COLOR     (0xffbb8899);
const float NODE_PREUNLOCK_THICKNESS (1.9f);
const float NODE_PREUNLOCK_RADIUS    (3.8f);

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
        const beam_t* beams = m_actor->beams;
        const size_t num_beams = static_cast<size_t>(m_actor->free_beam);
        for (size_t i = 0; i < num_beams; ++i)
        {
            ImVec2 pos1 = world2screen.Convert(beams[i].p1->AbsPosition);
            ImVec2 pos2 = world2screen.Convert(beams[i].p2->AbsPosition);

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

            if (beams[i].broken)
            {
                drawlist->AddLine(pos1, pos2, BEAM_BROKEN_COLOR, BEAM_BROKEN_THICKNESS);
            }
            else if ((beams[i].type == BEAM_HYDRO || (beams[i].type == BEAM_INVISIBLE_HYDRO)))
            {
                drawlist->AddLine(pos1, pos2, BEAM_HYDRO_COLOR, BEAM_HYDRO_THICKNESS);
            }
            else
            {
                drawlist->AddLine(pos1, pos2, BEAM_COLOR, BEAM_THICKNESS);
            }
        }

        // Nodes
        const node_t* nodes = m_actor->nodes;
        const size_t num_nodes = static_cast<size_t>(m_actor->free_node);
        for (size_t i = 0; i < num_nodes; ++i)
        {
            ImVec2 pos = world2screen.Convert(nodes[i].AbsPosition);

            drawlist->AddCircleFilled(pos, NODE_RADIUS, NODE_COLOR);
            switch (nodes[i].locked)
            {
            case PRELOCK:   drawlist->AddCircle(pos, NODE_PRELOCK_RADIUS,   NODE_PRELOCK_COLOR,   12, NODE_PRELOCK_THICKNESS);
            case LOCKED:    drawlist->AddCircle(pos, NODE_LOCKED_RADIUS,    NODE_LOCKED_COLOR,    12, NODE_LOCKED_THICKNESS);
            case PREUNLOCK: drawlist->AddCircle(pos, NODE_PREUNLOCK_RADIUS, NODE_PREUNLOCK_COLOR, 12, NODE_PREUNLOCK_THICKNESS);
            default:; // Nothing displayed on state `UNLOCKED`
            }
        }

        // Node info; drawn after nodes to have higher Z-order
        if ((m_debug_view == DebugViewType::DEBUGVIEW_NODES) || (m_debug_view == DebugViewType::DEBUGVIEW_BEAMS))
        {
            for (size_t i = 0; i < num_nodes; ++i)
            {
                ImVec2 pos = world2screen.Convert(nodes[i].AbsPosition);

                GStr<25> id_buf;
                id_buf << nodes[i].id;
                drawlist->AddText(pos, NODE_TEXT_COLOR, id_buf.ToCStr());

                if (m_debug_view != DebugViewType::DEBUGVIEW_BEAMS)
                {
                    char mass_buf[50];
                    snprintf(mass_buf, 50, "|%.1fKg", nodes[i].mass);
                    ImVec2 offset = ImGui::CalcTextSize(id_buf.ToCStr());
                    drawlist->AddText(ImVec2(pos.x + offset.x, pos.y), NODE_MASS_TEXT_COLOR, mass_buf);
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
                ImVec2 pos = world2screen.Convert(world_pos);

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

                if ((beams[i].type == BEAM_HYDRO || (beams[i].type == BEAM_INVISIBLE_HYDRO)))
                {
                    ImVec2 hydro_text_pos((pos.x + ImGui::CalcTextSize(buf).x), stress_text_pos.y);
                    snprintf(buf, BUF_LEN, "%.1f", ((beams[i].L / beams[i].Lhydro) * 100));
                    drawlist->AddText(hydro_text_pos, BEAM_HYDRO_TEXT_COLOR, buf);
                }
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
