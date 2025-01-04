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

#pragma once

#include "Application.h"
#include "RefCountingObject.h"

#include <string>

namespace RoR {

/// @addtogroup Terrain
/// @{

/// Represents an instance of static terrain object (.ODEF file format)
class TerrainEditorObject : public RefCountingObject<TerrainEditorObject>
{
public:
    // Variables are not accessible from AngelScript
    std::string name;
    std::string instance_name;
    std::string type;
    Ogre::Vector3 position = Ogre::Vector3::ZERO;
    Ogre::Vector3 rotation = Ogre::Vector3::ZERO;
    Ogre::Vector3 initial_position = Ogre::Vector3::ZERO;
    Ogre::Vector3 initial_rotation = Ogre::Vector3::ZERO;
    Ogre::SceneNode* node = nullptr;
    bool enable_collisions = true;
    int script_handler = -1;
    int tobj_cache_id = -1;
    std::string tobj_comments;
    std::vector<int> static_collision_boxes;
    std::vector<int> static_collision_tris;
    // ~ only for preloaded actors:
    TObjSpecialObject special_object_type = TObjSpecialObject::NONE;
    ActorInstanceID_t actor_instance_id = ACTORINSTANCEID_INVALID;

    // Functions are exported to AngelScript
    Ogre::Vector3 const& getPosition();
    Ogre::Vector3 const& getRotation();
    void setPosition(Ogre::Vector3 const& pos);
    void setRotation(Ogre::Vector3 const& rot);
    std::string const& getName();
    std::string const& getInstanceName();
    std::string const& getType();
    TObjSpecialObject getSpecialObjectType();
    // ~ only for preloaded actors:
    void setSpecialObjectType(TObjSpecialObject type);
    ActorInstanceID_t getActorInstanceId();
    void setActorInstanceId(ActorInstanceID_t instance_id);
};

/// Minimalist editor mode; orig. code by Ulteq/2016
/// * Enter/Exit from/to simulation by Ctrl+Y (see EV_COMMON_TOGGLE_TERRAIN_EDITOR)
/// * Select object by middle mouse button or Enter key (closest to avatar)
/// * Rotate/move selected object with keys
/// * Select/edit also preloaded actors - this resets the actor (added 2024 by Petr Ohlidal)
/// Upon exit, file 'editor_out.cfg' is written to ROR_HOME/config (see RGN_CONFIG)
class TerrainEditor
{
public:
    void UpdateInputEvents(float dt);
    void WriteOutputFile();
    void ClearSelectedObject();
    void SetSelectedObjectByID(TerrainEditorObjectID_t id);
    TerrainEditorObjectID_t GetSelectedObjectID() const;
    const TerrainEditorObjectPtr& FetchSelectedObject();

    static const TerrainEditorObjectPtr TERRAINEDITOROBJECTPTR_NULL; // Dummy value to be returned as const reference.

private:
    bool                m_object_tracking = true;
    int                 m_rotation_axis = 1;        //!< 0=X, 1=Y, 2=Z
    std::string         m_last_object_name;

    // Use Get/Set/Clear`SelectedObject()` functions.
    TerrainEditorObjectID_t m_selected_object_id = TERRAINEDITOROBJECTID_INVALID;
};

/// @} // addtogroup Terrain

} // namespace RoR
