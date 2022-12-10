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

#include "TerrainEditor.h"

#include "AppContext.h"
#include "CameraManager.h"
#include "Console.h"
#include "ContentManager.h"
#include "GameContext.h"
#include "GfxScene.h"
#include "InputEngine.h"
#include "OgreImGui.h"
#include "Terrain.h"
#include "TerrainObjectManager.h"
#include "PlatformUtils.h"

using namespace RoR;
using namespace Ogre;

void TerrainEditor::UpdateInputEvents(float dt)
{
    auto& object_list = App::GetGameContext()->GetTerrain()->getObjectManager()->GetEditorObjects();
    bool update = false;

    if (ImGui::IsMouseClicked(2)) // Middle button
    {
        ImVec2 mouse_screen = ImGui::GetIO().MousePos / ImGui::GetIO().DisplaySize;
        Ogre::Ray terrain_editor_mouse_ray = App::GetCameraManager()->GetCamera()->getCameraToViewportRay(mouse_screen.x, mouse_screen.y);

        float min_dist = std::numeric_limits<float>::max();
        Vector3 origin = terrain_editor_mouse_ray.getOrigin();
        Vector3 direction = terrain_editor_mouse_ray.getDirection();
        for (int i = 0; i < (int)object_list.size(); i++)
        {
            Real ray_object_distance = direction.crossProduct(object_list[i].node->getPosition() - origin).length();
            if (ray_object_distance < min_dist)
            {
                min_dist = ray_object_distance;
                update = (m_object_index != i);
                m_object_index = i;
            }
        }
    }
    if (m_object_index != -1)
    {
        m_last_object_name = object_list[m_object_index].name;
    }
    if (App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_ENTER_OR_EXIT_TRUCK))
    {
        if (m_object_index == -1)
        {
            // Select nearest object
            Vector3 ref_pos = App::GetCameraManager()->GetCurrentBehavior() == CameraManager::CAMERA_BEHAVIOR_FREE 
                              ? App::GetCameraManager()->GetCameraNode()->getPosition()
                              : App::GetGameContext()->GetPlayerCharacter()->getPosition();
            float min_dist = std::numeric_limits<float>::max();
            for (int i = 0; i < (int)object_list.size(); i++)
            {
                float dist = ref_pos.squaredDistance(object_list[i].node->getPosition());
                if (dist < min_dist)
                {
                    m_object_index = i;
                    min_dist = dist;
                    update = true;
                }
            }
        }
        else
        {
            m_object_index = -1;
        }
    }
    else if (App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_RESPAWN_LAST_TRUCK) &&
            m_last_object_name != "")
    {
        Vector3 pos = App::GetGameContext()->GetPlayerCharacter()->getPosition();

        try
        {
            App::GetGameContext()->GetTerrain()->getObjectManager()->LoadTerrainObject(m_last_object_name, pos, Vector3::ZERO, "Console", "");

            App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_REPLY, _L("Spawned object at position: ") + String("x: ") + TOSTRING(pos.x) + String("z: ") + TOSTRING(pos.z), "world.png");
        }
        catch (std::exception& e)
        {
            App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_ERROR, e.what(), "error.png");
        }
    }
    else if (App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_ENTER_NEXT_TRUCK, 0.25f))
    {
        if (object_list.size() > 0)
        {
            m_object_index = (m_object_index + 1 + (int)object_list.size()) % object_list.size();
            update = true;
        }
    }
    else if (App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_ENTER_PREVIOUS_TRUCK, 0.25f))
    {
        if (object_list.size() > 0)
        {
            m_object_index = (m_object_index - 1 + (int)object_list.size()) % object_list.size();
            update = true;
        }
    }
    if (App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_RESCUE_TRUCK))
    {
        std::string axis = _L("ry");
        if (m_rotation_axis == 0)
        {
            axis = _L("ry");
            m_rotation_axis = 1;
        }
        else if (m_rotation_axis == 1)
        {
            axis = _L("rz");
            m_rotation_axis = 2;
        }
        else if (m_rotation_axis == 2)
        {
            axis = _L("rx");
            m_rotation_axis = 0;
        }
        std::string ssmsg = _L("Rotating: ") + axis;
        App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE, ssmsg, "information.png");
    }
    if (App::GetInputEngine()->isKeyDownValueBounce(OgreBites::SDLK_SPACE))
    {
        m_object_tracking = !m_object_tracking;
        std::string ssmsg = m_object_tracking ? _L("Enabled object tracking") : _L("Disabled object tracking");
        App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE, ssmsg, "information.png");
    }
    if (m_object_index != -1 && update)
    {
        String ssmsg = _L("Selected object: [") + TOSTRING(m_object_index) + "/" + TOSTRING(object_list.size()) + "] (" + object_list[m_object_index].name + ")";
        App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE, ssmsg, "information.png");
        if (m_object_tracking)
        {
            App::GetGameContext()->GetPlayerCharacter()->setPosition(object_list[m_object_index].node->getPosition());
        }
    }
    if (m_object_index != -1 && App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_RESET_TRUCK))
    {
        SceneNode* sn = object_list[m_object_index].node;

        object_list[m_object_index].position = object_list[m_object_index].initial_position;
        sn->setPosition(object_list[m_object_index].position);

        object_list[m_object_index].rotation = object_list[m_object_index].initial_rotation;
        Vector3 rot = object_list[m_object_index].rotation;
        sn->setOrientation(Quaternion(Degree(rot.x), Vector3::UNIT_X) * Quaternion(Degree(rot.y), Vector3::UNIT_Y) * Quaternion(Degree(rot.z), Vector3::UNIT_Z));
        sn->pitch(Degree(-90));
    }
    if (m_object_index != -1 && App::GetCameraManager()->GetCurrentBehavior() != CameraManager::CAMERA_BEHAVIOR_FREE)
    {
        SceneNode* sn = object_list[m_object_index].node;

        Vector3 translation = Vector3::ZERO;
        float rotation = 0.0f;

        if (App::GetInputEngine()->getEventBoolValue(EV_TRUCK_STEER_LEFT))
        {
            rotation += 2.0f;
        }
        else if (App::GetInputEngine()->getEventBoolValue(EV_TRUCK_STEER_RIGHT))
        {
            rotation -= 2.0f;
        }
        if (App::GetInputEngine()->getEventBoolValue(EV_TRUCK_ACCELERATE))
        {
            translation.y += 0.5f;
        }
        else if (App::GetInputEngine()->getEventBoolValue(EV_TRUCK_BRAKE))
        {
            translation.y -= 0.5f;
        }
        if (App::GetInputEngine()->getEventBoolValue(EV_CHARACTER_FORWARD))
        {
            translation.x += 0.5f;
        }
        else if (App::GetInputEngine()->getEventBoolValue(EV_CHARACTER_BACKWARDS))
        {
            translation.x -= 0.5f;
        }
        if (App::GetInputEngine()->getEventBoolValue(EV_CHARACTER_SIDESTEP_RIGHT))
        {
            translation.z += 0.5f;
        }
        else if (App::GetInputEngine()->getEventBoolValue(EV_CHARACTER_SIDESTEP_LEFT))
        {
            translation.z -= 0.5f;
        }

        if (translation != Vector3::ZERO || rotation != 0.0f)
        {
            float scale = App::GetInputEngine()->isKeyDown(SDLK_LALT) ? 0.1f : 1.0f;
            scale *= App::GetInputEngine()->isKeyDown(SDLK_LSHIFT) ? 3.0f : 1.0f;
            scale *= App::GetInputEngine()->isKeyDown(SDLK_LCTRL) ? 10.0f : 1.0f;

            object_list[m_object_index].position += translation * scale * dt;
            sn->setPosition(object_list[m_object_index].position);

            object_list[m_object_index].rotation[m_rotation_axis] += rotation * scale * dt;
            Vector3 rot = object_list[m_object_index].rotation;
            sn->setOrientation(Quaternion(Degree(rot.x), Vector3::UNIT_X) * Quaternion(Degree(rot.y), Vector3::UNIT_Y) * Quaternion(Degree(rot.z), Vector3::UNIT_Z));
            sn->pitch(Degree(-90));

            if (m_object_tracking)
            {
                App::GetGameContext()->GetPlayerCharacter()->setPosition(sn->getPosition());
            }
        }
        else if (m_object_tracking && App::GetGameContext()->GetPlayerCharacter()->getPosition() != sn->getPosition())
        {
            object_list[m_object_index].position = App::GetGameContext()->GetPlayerCharacter()->getPosition();
            sn->setPosition(App::GetGameContext()->GetPlayerCharacter()->getPosition());
        }
        if (App::GetInputEngine()->getEventBoolValue(EV_COMMON_REMOVE_CURRENT_TRUCK))
        {
            App::GetGameContext()->GetTerrain()->getObjectManager()->unloadObject(object_list[m_object_index].instance_name);
        }
    }
    else
    {
        App::GetGameContext()->GetCharacterFactory()->Update(dt);
    }    

    if (App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_TOGGLE_TERRAIN_EDITOR))
    {
        App::GetGameContext()->PushMessage(MSG_EDI_LEAVE_TERRN_EDITOR_REQUESTED);
    }
}

void TerrainEditor::WriteOutputFile()
{
    const char* filename = "editor_out.log";
    std::string editor_logpath = PathCombine(App::sys_logs_dir->getStr(), filename);
    try
    {
        Ogre::DataStreamPtr stream
            = Ogre::ResourceGroupManager::getSingleton().createResource(
                editor_logpath, RGN_CONFIG, /*overwrite=*/true);

        for (auto object : App::GetGameContext()->GetTerrain()->getObjectManager()->GetEditorObjects())
        {
            SceneNode* sn = object.node;
            if (sn != nullptr)
            {
                String pos = StringUtil::format("%8.3f, %8.3f, %8.3f"   , object.position.x, object.position.y, object.position.z);
                String rot = StringUtil::format("% 6.1f, % 6.1f, % 6.1f", object.rotation.x, object.rotation.y, object.rotation.z);

                String line = pos + ", " + rot + ", " + object.name + "\n";
                stream->write(line.c_str(), line.length());
            }
        }
        
        // Export procedural roads
        int num_roads = App::GetGameContext()->GetTerrain()->getProceduralManager()->getNumObjects();
        for (int i = 0; i < num_roads; i++)
        {
            ProceduralObjectPtr obj = App::GetGameContext()->GetTerrain()->getProceduralManager()->getObject(i);
            int num_points = obj->getNumPoints();
            if (num_points > 0)
            {
                stream->write("\nbegin_procedural_roads\n", 24);
                std::string smoothing_line = fmt::format("\tsmoothing_num_splits {}\n", obj->smoothing_num_splits);
                stream->write(smoothing_line.c_str(), smoothing_line.length());
                for (int j = 0; j < num_points; j++)
                {
                    ProceduralPointPtr point = obj->getPoint(j);
                    std::string type_str;
                    switch (point->type)
                    {
                    case RoadType::ROAD_AUTOMATIC: type_str = "both"; break; // ??
                    case RoadType::ROAD_FLAT: type_str = "flat"; break;
                    case RoadType::ROAD_LEFT: type_str = "left"; break;
                    case RoadType::ROAD_RIGHT: type_str = "right"; break;
                    case RoadType::ROAD_BOTH: type_str = "both"; break;
                    case RoadType::ROAD_BRIDGE: type_str = (point->pillartype == 1) ? "bridge" : "bridge_no_pillars"; break;
                    case RoadType::ROAD_MONORAIL: type_str = (point->pillartype == 2) ? "monorail" : "monorail2"; break;
                    }

                    std::string line = fmt::format(
                        "\t{:13f}, {:13f}, {:13f}, 0, {:13f}, 0, {:13f}, {:13f}, {:13f}, {}\n",
                        point->position.x, point->position.y, point->position.z,
                        point->rotation.getYaw().valueDegrees(),
                        point->width, point->bwidth, point->bheight, type_str);
                    stream->write(line.c_str(), line.length());
                }
                stream->write("end_procedural_roads\n", 21);
            }
        }
    }
    catch (std::exception& e)
    {
        RoR::LogFormat("[RoR|MapEditor]"
                        "Error saving file '%s' (resource group '%s'), message: '%s'",
                        filename, RGN_CONFIG, e.what());
    }
}

void TerrainEditor::ClearSelection()
{
    m_object_index = -1;
}
