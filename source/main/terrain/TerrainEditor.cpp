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
#include "Actor.h"
#include "CameraManager.h"
#include "Console.h"
#include "ContentManager.h"
#include "GameContext.h"
#include "GfxScene.h"
#include "InputEngine.h"
#include "OgreImGui.h"
#include "Terrain.h"
#include "TerrainObjectManager.h"
#include "TObjFileFormat.h"
#include "PlatformUtils.h"

using namespace RoR;
using namespace Ogre;

const TerrainEditorObjectPtr TerrainEditor::TERRAINEDITOROBJECTPTR_NULL; // Dummy value to be returned as const reference.

void TerrainEditor::UpdateInputEvents(float dt)
{
    auto& object_list = App::GetGameContext()->GetTerrain()->getObjectManager()->GetEditorObjects();

    if (ImGui::IsMouseClicked(2)) // Middle button
    {
        ImVec2 mouse_screen = ImGui::GetIO().MousePos / ImGui::GetIO().DisplaySize;
        Ogre::Ray terrain_editor_mouse_ray = App::GetCameraManager()->GetCamera()->getCameraToViewportRay(mouse_screen.x, mouse_screen.y);

        float min_dist = std::numeric_limits<float>::max();
        Vector3 origin = terrain_editor_mouse_ray.getOrigin();
        Vector3 direction = terrain_editor_mouse_ray.getDirection();
        for (int i = 0; i < (int)object_list.size(); i++)
        {
            Real ray_object_distance = direction.crossProduct(object_list[i]->getPosition() - origin).length();
            if (ray_object_distance < min_dist)
            {
                min_dist = ray_object_distance;
                this->SetSelectedObjectByID(i);
            }
        }
    }
    if (m_selected_object_id != -1)
    {
        m_last_object_name = object_list[m_selected_object_id]->name;
    }
    if (App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_ENTER_OR_EXIT_TRUCK))
    {
        if (m_selected_object_id == -1)
        {
            // Select nearest object
            Vector3 ref_pos = App::GetCameraManager()->GetCurrentBehavior() == CameraManager::CAMERA_BEHAVIOR_FREE 
                              ? App::GetCameraManager()->GetCameraNode()->getPosition()
                              : App::GetGameContext()->GetPlayerCharacter()->getPosition();
            float min_dist = std::numeric_limits<float>::max();
            for (int i = 0; i < (int)object_list.size(); i++)
            {
                float dist = ref_pos.squaredDistance(object_list[i]->getPosition());
                if (dist < min_dist)
                {
                    this->SetSelectedObjectByID(i);
                    min_dist = dist;
                }
            }
        }
        else
        {
            m_selected_object_id = -1;
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
            TerrainEditorObjectID_t i = (m_selected_object_id + 1 + (int)object_list.size()) % object_list.size();
            this->SetSelectedObjectByID(i);
        }
    }
    else if (App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_ENTER_PREVIOUS_TRUCK, 0.25f))
    {
        if (object_list.size() > 0)
        {
            TerrainEditorObjectID_t i = (m_selected_object_id - 1 + (int)object_list.size()) % object_list.size();
            this->SetSelectedObjectByID(i);
        }
    }
    if (App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_RESCUE_TRUCK))
    {
        UTFString axis = _L("ry");
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
        UTFString ssmsg = _L("Rotating: ") + axis;
        App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE, ssmsg, "information.png");
    }
    if (App::GetInputEngine()->isKeyDownValueBounce(OIS::KC_SPACE))
    {
        m_object_tracking = !m_object_tracking;
        UTFString ssmsg = m_object_tracking ? _L("Enabled object tracking") : _L("Disabled object tracking");
        App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE, ssmsg, "information.png");
    }
    if (m_selected_object_id != -1 && App::GetInputEngine()->getEventBoolValueBounce(EV_COMMON_RESET_TRUCK))
    {
        object_list[m_selected_object_id]->setPosition(object_list[m_selected_object_id]->initial_position);
        object_list[m_selected_object_id]->setRotation(object_list[m_selected_object_id]->initial_rotation);
    }
    if (m_selected_object_id != -1 && App::GetCameraManager()->GetCurrentBehavior() != CameraManager::CAMERA_BEHAVIOR_FREE)
    {
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
            float scale = App::GetInputEngine()->isKeyDown(OIS::KC_LMENU) ? 0.1f : 1.0f;
            scale *= App::GetInputEngine()->isKeyDown(OIS::KC_LSHIFT) ? 3.0f : 1.0f;
            scale *= App::GetInputEngine()->isKeyDown(OIS::KC_LCONTROL) ? 10.0f : 1.0f;

            Ogre::Vector3 new_position = object_list[m_selected_object_id]->getPosition();
            new_position += translation * scale * dt;
            object_list[m_selected_object_id]->setPosition(new_position);

            Ogre::Vector3 new_rotation = object_list[m_selected_object_id]->getRotation();
            new_rotation[m_rotation_axis] += rotation * scale * dt;
            object_list[m_selected_object_id]->setRotation(new_rotation);

            if (m_object_tracking)
            {
                App::GetGameContext()->GetPlayerCharacter()->setPosition(new_position);
            }
        }
        else if (m_object_tracking && App::GetGameContext()->GetPlayerCharacter()->getPosition() != object_list[m_selected_object_id]->getPosition())
        {
            object_list[m_selected_object_id]->setPosition(App::GetGameContext()->GetPlayerCharacter()->getPosition());
        }
        if (App::GetInputEngine()->getEventBoolValue(EV_COMMON_REMOVE_CURRENT_TRUCK))
        {
            App::GetGameContext()->GetTerrain()->getObjectManager()->destroyObject(object_list[m_selected_object_id]->instance_name);
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
    TerrainPtr terrain = App::GetGameContext()->GetTerrain();

    // Assert on Debug, minimize harm on Release
    ROR_ASSERT(terrain);
    if (!terrain)
    {
        return;
    }

    // If not a project (unzipped), do nothing
    if (terrain->getCacheEntry()->resource_bundle_type != "FileSystem")
    {
        App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_TERRN, Console::CONSOLE_SYSTEM_WARNING,
            fmt::format("Cannot export terrain editor changes - terrain is not a project"));
        return;
    }

    // Loop over TOBJ files in cache, update editable elements and serialize.
    for (size_t i = 0; i < terrain->getObjectManager()->GetTobjCache().size(); i++)
    {
        // Dump original elements and rebuild them from live data.
        TObjDocumentPtr tobj = terrain->getObjectManager()->GetTobjCache()[i];
        tobj->objects.clear();
        tobj->vehicles.clear();
        for (TerrainEditorObjectPtr& src : terrain->getObjectManager()->GetEditorObjects())
        {
            if (src->tobj_cache_id == i)
            {
                if (src->special_object_type == TObjSpecialObject::NONE)
                {
                    TObjEntry dst;
                    strncpy(dst.odef_name, src->name.c_str(), TObj::STR_LEN);
                    strncpy(dst.instance_name, src->instance_name.c_str(), TObj::STR_LEN);
                    strncpy(dst.type, src->type.c_str(), TObj::STR_LEN);
                    // TBD: reconstruct 'set_default_rendering_distance'.
                    dst.position = src->position;
                    dst.rotation = src->rotation;
                    dst.comments = src->tobj_comments;

                    tobj->objects.push_back(dst);
                }
                else
                {
                    TObjVehicle dst;
                    strncpy(dst.name, src->name.c_str(), TObj::STR_LEN);
                    dst.position = src->position;
                    dst.tobj_rotation = src->rotation;
                    dst.type = src->special_object_type;

                    tobj->vehicles.push_back(dst);
                }
            }
        }

        try
        {
            Ogre::DataStreamPtr stream
                = Ogre::ResourceGroupManager::getSingleton().createResource(
                    tobj->document_name, terrain->getTerrainFileResourceGroup(), /*overwrite=*/true);
            TObj::WriteToStream(tobj, stream);
        }
        catch (...)
        {
            RoR::HandleGenericException(
                fmt::format("Error saving file '{}' to resource group '{}'",
                    tobj->document_name, terrain->getTerrainFileResourceGroup()), HANDLEGENERICEXCEPTION_CONSOLE);
        }
    }
}

void TerrainEditor::ClearSelectedObject()
{
    this->SetSelectedObjectByID(TERRAINEDITOROBJECTID_INVALID);
}

void TerrainEditor::SetSelectedObjectByID(TerrainEditorObjectID_t id)
{
    // Do nothing if already selected.
    if (id == m_selected_object_id)
    {
        return;
    }

    m_selected_object_id = id;

    if (id == TERRAINEDITOROBJECTID_INVALID)
    {
        return; // Nothing more to do.
    }

    // Notify user
    auto& object_list = App::GetGameContext()->GetTerrain()->getObjectManager()->GetEditorObjects();
    String ssmsg = _L("Selected object: [") + TOSTRING(m_selected_object_id) + "/" + TOSTRING(object_list.size()) + "] (" + object_list[m_selected_object_id]->name + ")";
    App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE, ssmsg, "information.png");
    if (m_object_tracking)
    {
        App::GetGameContext()->GetPlayerCharacter()->setPosition(object_list[m_selected_object_id]->getPosition());
    }

    // If setting special object, make sure the actor instance exists, otherwise spawn it again.
    const TerrainEditorObjectPtr object = this->FetchSelectedObject();
    ROR_ASSERT(object != TERRAINEDITOROBJECTPTR_NULL);
    if (object != TERRAINEDITOROBJECTPTR_NULL && object->special_object_type != TObjSpecialObject::NONE)
    {
        App::GetGameContext()->GetTerrain()->getObjectManager()->SpawnSinglePredefinedActor(object);
    }
}

TerrainEditorObjectID_t TerrainEditor::GetSelectedObjectID() const
{ 
    return m_selected_object_id; 
}

const TerrainEditorObjectPtr& TerrainEditor::FetchSelectedObject()
{
    if (m_selected_object_id == TERRAINEDITOROBJECTID_INVALID)
    {
        return TERRAINEDITOROBJECTPTR_NULL;
    }

    auto& object_list = App::GetGameContext()->GetTerrain()->getObjectManager()->GetEditorObjects();
    ROR_ASSERT(m_selected_object_id < (int)object_list.size());
    if (m_selected_object_id >= (int)object_list.size())
    {
        LOG(fmt::format("[RoR|TerrainEditor] INTERNAL ERROR - `m_selected_object_id` '{}' >= `(int)object_list.size()` '{}'",
            m_selected_object_id, (int)object_list.size()));
        return TERRAINEDITOROBJECTPTR_NULL;
    }

    return object_list[m_selected_object_id];
}

// -------------------
// TerrainEditorObject

Ogre::Vector3 const& TerrainEditorObject::getPosition()
{
    return position;
}

Ogre::Vector3 const& TerrainEditorObject::getRotation()
{
    return rotation;
}

void TerrainEditorObjectRefreshActorVisual(TerrainEditorObjectPtr obj)
{
    ROR_ASSERT(obj->actor_instance_id != ACTORINSTANCEID_INVALID);
    const ActorPtr& actor = App::GetGameContext()->GetActorManager()->GetActorById(obj->actor_instance_id);
    ROR_ASSERT(actor != ActorManager::ACTORPTR_NULL);
    if (actor != ActorManager::ACTORPTR_NULL)
    {
        const bool rot_yxz = App::GetGameContext()->GetTerrain()->getObjectManager()->GetEditorObjectFlagRotYXZ(obj);

        ActorModifyRequest* req = new ActorModifyRequest();
        req->amr_type = ActorModifyRequest::Type::SOFT_RESPAWN;
        req->amr_actor = actor->ar_instance_id;
        req->amr_softrespawn_position = obj->position;
        req->amr_softrespawn_rotation = TObjParser::CalcRotation(obj->rotation, rot_yxz);
        App::GetGameContext()->PushMessage(Message(MSG_SIM_MODIFY_ACTOR_REQUESTED, (void*)req));
        req = nullptr;

        ActorModifyRequest* fxreq = new ActorModifyRequest();
        fxreq->amr_type = ActorModifyRequest::Type::REFRESH_VISUALS;
        fxreq->amr_actor = actor->ar_instance_id;
        App::GetGameContext()->PushMessage(Message(MSG_SIM_MODIFY_ACTOR_REQUESTED, (void*)fxreq));
        fxreq = nullptr;
    }
}

void TerrainEditorObject::setPosition(Ogre::Vector3 const& pos)
{
    position = pos;
    if (static_object_node)
    {
        static_object_node->setPosition(pos);
    }
    else if (special_object_type != TObjSpecialObject::NONE)
    {
        TerrainEditorObjectRefreshActorVisual(this);
    }
}

void TerrainEditorObject::setRotation(Ogre::Vector3 const& rot)
{
    rotation = rot;
    if (static_object_node)
    {
        static_object_node->setOrientation(Quaternion(Degree(rot.x), Vector3::UNIT_X) * Quaternion(Degree(rot.y), Vector3::UNIT_Y) * Quaternion(Degree(rot.z), Vector3::UNIT_Z));
        static_object_node->pitch(Degree(-90));
    }
    else if (special_object_type != TObjSpecialObject::NONE)
    {
        TerrainEditorObjectRefreshActorVisual(this);
    }
}

std::string const& TerrainEditorObject::getName()
{
    return name;
}

std::string const& TerrainEditorObject::getInstanceName()
{
    return instance_name;
}

std::string const& TerrainEditorObject::getType()
{
    return type;
}

TObjSpecialObject TerrainEditorObject::getSpecialObjectType()
{
    return special_object_type;
}

void TerrainEditorObject::setSpecialObjectType(TObjSpecialObject type)
{
    special_object_type = type;
}

ActorInstanceID_t TerrainEditorObject::getActorInstanceId()
{
    return actor_instance_id;
}

void TerrainEditorObject::setActorInstanceId(ActorInstanceID_t instance_id)
{
    actor_instance_id = instance_id;
}
