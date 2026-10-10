/*
    This source file is part of Rigs of Rods
    Copyright 2022 - 2026 Petr Ohlidal

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

/// @file
/// Helper/wrapper functions shared by native (ProceduralRoadAngelscript.cpp)
/// and generic (ProceduralRoadAngelscriptGeneric.cpp) procedural road bindings.
/// Native bindings register these directly (asCALL_CDECL/asCALL_CDECL_OBJFIRST),
/// generic bindings wrap them using WRAP_FN()/WRAP_OBJ_FIRST().

#pragma once

#include "Application.h"
#include "GameContext.h"
#include "ProceduralManager.h"
#include "ProceduralRoad.h"
#include "ScriptEngine.h"
#include "Terrain.h"

namespace ProceduralRoadAngelscriptWrappers {

using namespace RoR;

// Factories

static ProceduralPoint* ProceduralPointFactory()
{
    return new ProceduralPoint();
}

static ProceduralObject* ProceduralObjectFactory()
{
    return new ProceduralObject();
}

static ProceduralRoad* ProceduralRoadFactory(const std::string& groundmodelName = "")
{
    return new ProceduralRoad(groundmodelName);
}

// struct ProceduralPoint - property accessors
// NOTE: Using property-accessors because `offsetof()` cannot be used with derived classes (see https://stackoverflow.com/q/1129894)
// NOTE: for compound data types like vector3 we must return non-const references so that expressions like `ppoint.position.y = 100.f` still compile and work

static Ogre::Vector3& ProceduralPoint_get_position(ProceduralPoint* self) { return self->position; }
static Ogre::Quaternion& ProceduralPoint_get_rotation(ProceduralPoint* self) { return self->rotation; }
static float ProceduralPoint_get_width(ProceduralPoint* self) { return self->width; }
static float ProceduralPoint_get_border_width(ProceduralPoint* self) { return self->bwidth; }
static float ProceduralPoint_get_border_height(ProceduralPoint* self) { return self->bheight; }
static RoadType ProceduralPoint_get_type(ProceduralPoint* self) { return self->type; }
static int ProceduralPoint_get_pillar_type(ProceduralPoint* self) { return self->pillartype; }

static void ProceduralPoint_set_position(ProceduralPoint* self, const Ogre::Vector3& pos) { self->position = pos; }
static void ProceduralPoint_set_rotation(ProceduralPoint* self, const Ogre::Quaternion& rot) { self->rotation = rot; }
static void ProceduralPoint_set_width(ProceduralPoint* self, float width) { self->width = width; }
static void ProceduralPoint_set_border_width(ProceduralPoint* self, float bwidth) { self->bwidth = bwidth; }
static void ProceduralPoint_set_border_height(ProceduralPoint* self, float bheight) { self->bheight = bheight; }
static void ProceduralPoint_set_type(ProceduralPoint* self, RoadType type) { self->type = type; }
static void ProceduralPoint_set_pillar_type(ProceduralPoint* self, int type) { self->pillartype = type; }

// class ProceduralRoad

/// The script API has no scene node parameter - the road mesh is attached under
/// the grouping scene node of the terrain's `ProceduralManager`, same as roads created by C++.
static void ProceduralRoad_finish(ProceduralRoad* self)
{
    if (!App::GetGameContext()->GetTerrain())
    {
        App::GetScriptEngine()->SLOG("ProceduralRoad::finish(): no terrain loaded, cannot attach the road mesh.");
        return;
    }
    self->finish(App::GetGameContext()->GetTerrain()->getProceduralManager()->getGroupingSceneNode());
}

// class ProceduralObject - property accessors

static int ProceduralObject_get_smoothing_num_splits(ProceduralObject* self) { return self->smoothing_num_splits; }
static void ProceduralObject_set_smoothing_num_splits(ProceduralObject* self, int n) { self->smoothing_num_splits = n; }
static bool ProceduralObject_get_collision_enabled(ProceduralObject* self) { return self->collision_enabled; }
static void ProceduralObject_set_collision_enabled(ProceduralObject* self, bool n) { self->collision_enabled = n; }

static std::string ProceduralObject_get_custom_material(ProceduralObject* self) { return self->custom_material; }
static void ProceduralObject_set_custom_material(ProceduralObject* self, const std::string& n) { self->custom_material = n; }
static std::string ProceduralObject_get_custom_groundmodel(ProceduralObject* self) { return self->custom_groundmodel; }
static void ProceduralObject_set_custom_groundmodel(ProceduralObject* self, const std::string& n) { self->custom_groundmodel = n; }

} // namespace ProceduralRoadAngelscriptWrappers
