/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2020 Thomas Fischer
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


#include "GUI_SurveyMap.h"

#include "Application.h"
#include "Beam.h"
#include "ContentManager.h"
#include "GUIManager.h"
#include "GUI_MainSelector.h"
#include "GUIUtils.h"
#include "InputEngine.h"
#include "Language.h"
#include "OgreImGui.h"
#include "OgreSubsystem.h"
#include "RoRFrameListener.h"
#include "SurveyMapTextureCreator.h"
#include "TerrainManager.h"
#include "TerrainObjectManager.h"

using namespace Ogre;

void RoR::GUI::SurveyMap::Draw()
{
    // Check special cases
    if ((mMapMode == SurveyMapMode::BIG) &&
        App::GetSimController()->AreControlsLocked() || App::GetGuiManager()->GetMainSelector()->IsVisible())
    {
        mMapMode = SurveyMapMode::NONE;
        return;
    }

    // Handle input
    if (App::GetInputEngine()->getEventBoolValueBounce(EV_SURVEY_MAP_TOGGLE_ICONS))
        App::gfx_surveymap_icons->SetActiveVal(!App::gfx_surveymap_icons->GetActiveVal<bool>());

    if (mMapMode == SurveyMapMode::SMALL)
    {
        if (App::GetInputEngine()->getEventBoolValue(EV_SURVEY_MAP_ZOOM_IN))
        {
            setMapZoomRelative(+ImGui::GetIO().DeltaTime);
        }
        if (App::GetInputEngine()->getEventBoolValue(EV_SURVEY_MAP_ZOOM_OUT))
        {
            setMapZoomRelative(-ImGui::GetIO().DeltaTime);
        }
    }

    // Calculate window position
    ImVec2 view_size(0, 0);
    if (mMapMode == SurveyMapMode::BIG)
    {
        view_size.y = ImGui::GetIO().DisplaySize.y -
            ((2 * App::GetGuiManager()->GetTheme().screen_edge_padding.y) + (2 * WINDOW_PADDING));
        Vector3 terrn_size = App::GetSimTerrain()->getMaxTerrainSize(); // Y is 'up'!
        if (!terrn_size.isZeroLength())
        {
            view_size.x = (view_size.y / terrn_size.z) * terrn_size.x;
        }
        else // Terrain has no heightmap
        {
            view_size.x = view_size.y;
        }
    }
    else if (mMapMode == SurveyMapMode::SMALL)
    {
        view_size.y = ImGui::GetIO().DisplaySize.y * 0.30f;
        view_size.x = view_size.y;
    }

    // Open window
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar;
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(WINDOW_PADDING, WINDOW_PADDING));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, WINDOW_ROUNDING);
    ImGui::SetNextWindowSize(ImVec2((view_size.x + 8), (view_size.y + 8)));

    if (mMapMode == SurveyMapMode::BIG)
    {
        ImGui::SetNextWindowPosCenter();
    }
    else if (mMapMode == SurveyMapMode::SMALL)
    {
        float x = ImGui::GetIO().DisplaySize.x -
            (view_size.x + App::GetGuiManager()->GetTheme().screen_edge_padding.x);
        ImGui::SetNextWindowPos(ImVec2(x, 100.f));
    }
    ImGui::Begin("SurveyMap", nullptr, flags);

    // Draw map texture
    ImVec2 tl_screen_pos = ImGui::GetCursorScreenPos();
    Ogre::TexturePtr tex;
    Ogre::Vector2 smallmap_center;
    Ogre::Vector2 smallmap_size;
    Ogre::Vector2 view_origin;
    if (mMapMode == SurveyMapMode::BIG)
    {
        tex = mMapTextureCreatorStatic->GetTexture();
        view_origin = mMapCenterOffset;
    }
    else if (mMapMode == SurveyMapMode::SMALL)
    {
        // Calc. view center
        smallmap_size = mTerrainSize * (1.0f - mMapZoom);
        Ogre::Vector2 player_map_pos;
        Actor* actor = App::GetSimController()->GetGfxScene().GetSimDataBuffer().simbuf_player_actor;
        if (actor)
        {
            auto& actor_data = actor->GetGfxActor()->GetSimDataBuffer();
            player_map_pos = (Vector2(actor_data.simbuf_pos.x, actor_data.simbuf_pos.z));
        }
        else
        {
            auto& scene_data = App::GetSimController()->GetGfxScene().GetSimDataBuffer();
            player_map_pos = (Vector2(scene_data.simbuf_character_pos.x, scene_data.simbuf_character_pos.z));
        }

        smallmap_center.x = Math::Clamp(player_map_pos.x - mMapCenterOffset.x, smallmap_size.x / 2, mTerrainSize.x - smallmap_size.x / 2);
        smallmap_center.y = Math::Clamp(player_map_pos.y - mMapCenterOffset.y, smallmap_size.y / 2, mTerrainSize.y - smallmap_size.y / 2);

        view_origin = ((smallmap_center + mMapCenterOffset) - smallmap_size / 2);

        // Update texture
        if ((App::GetInputEngine()->getEventBoolValue(EV_SURVEY_MAP_ZOOM_IN)) || (App::GetInputEngine()->getEventBoolValue(EV_SURVEY_MAP_ZOOM_OUT)))
        {
            mMapTextureCreatorDynamic->update(smallmap_center + mMapCenterOffset, smallmap_size);
        }
        tex = mMapTextureCreatorDynamic->GetTexture();
    }

    ImGui::Image(reinterpret_cast<ImTextureID>(tex->getHandle()), view_size);
    if (ImGui::IsItemClicked(0)) // 0 = left click
    {
        ImVec2 mouse_view_offset = (ImGui::GetMousePos() - tl_screen_pos) / view_size;
        Vector2 mouse_map_pos;
        if (mMapMode == SurveyMapMode::BIG)
        {
            mouse_map_pos = view_origin + Vector2(mouse_view_offset.x, mouse_view_offset.y) * mTerrainSize;
        }
        else if (mMapMode == SurveyMapMode::SMALL)
        {
            mouse_map_pos = view_origin + (Vector2(mouse_view_offset.x, mouse_view_offset.y) * smallmap_size);
        }
        
        App::GetSimController()->TeleportPlayerXZ(mouse_map_pos.x, mouse_map_pos.y);
    }
    else if (ImGui::IsItemHovered())
    {
        // Draw teleport cursor
        ImVec2 mouse_pos = ImGui::GetMousePos();
        ImDrawList* drawlist = ImGui::GetWindowDrawList();
        drawlist->AddCircleFilled(mouse_pos, 5, ImGui::GetColorU32(ImVec4(1,0,0,1)));
        const char* title = "Teleport";
        ImVec2 text_pos(mouse_pos.x - (ImGui::CalcTextSize(title).x/2), mouse_pos.y + 5);
        drawlist->AddText(text_pos, ImGui::GetColorU32(ImGui::GetStyle().Colors[ImGuiCol_Text]), title);
    }

    if (App::gfx_surveymap_icons->GetActiveVal<bool>())
    {
        // Draw terrain object icons
        for (TerrainObjectManager::MapEntity& e: App::GetSimTerrain()->getObjectManager()->GetMapEntities())
        {
            int id = App::GetSimController()->GetRaceId();
            bool visible = !((e.type == "checkpoint" && e.id != id) || (e.type == "racestart" && id != -1 && e.id != id));
            Str<100> filename;
            filename << "icon_" << e.type << ".dds";

            if ((visible) && (!App::gfx_declutter_map->GetActiveVal<bool>()))
            {
                this->DrawMapIcon(tl_screen_pos, view_size, view_origin, filename.ToCStr(), e.name, e.pos.x, e.pos.z, e.rot);
            }
        }

        // Draw actor icons
        for (GfxActor* gfx_actor: App::GetSimController()->GetGfxScene().GetGfxActors())
        {
            const char* type_str = this->getTypeByDriveable(gfx_actor->GetActorDriveable());
            int truckstate = gfx_actor->GetActorState();
            Str<100> fileName;

            if (truckstate == static_cast<int>(Actor::SimState::LOCAL_SIMULATED))
                fileName << "icon_" << type_str << "_activated.dds"; // green icon
            else if (truckstate == static_cast<int>(Actor::SimState::NETWORKED_OK))
                fileName << "icon_" << type_str << "_networked.dds"; // blue icon
            else
                fileName << "icon_" << type_str << ".dds"; // gray icon

            auto& simbuf = gfx_actor->GetSimDataBuffer();
            std::string caption = (App::mp_state->GetActiveEnum<MpState>() == MpState::CONNECTED) ? simbuf.simbuf_net_username : "";
            this->DrawMapIcon(tl_screen_pos, view_size, view_origin, fileName.ToCStr(), caption, 
                simbuf.simbuf_pos.x, simbuf.simbuf_pos.z, simbuf.simbuf_rotation);
    }

        // Draw character icons
        for (GfxCharacter* gfx_character: App::GetSimController()->GetGfxScene().GetGfxCharacters())
        {
            auto& simbuf = gfx_character->xc_simbuf;
            if (!simbuf.simbuf_actor_coupling)
            {
                std::string caption = (App::mp_state->GetActiveEnum<MpState>() == MpState::CONNECTED) ? simbuf.simbuf_net_username : "";
                this->DrawMapIcon(tl_screen_pos, view_size, view_origin, "icon_person.dds", caption, 
                    simbuf.simbuf_character_pos.x, simbuf.simbuf_character_pos.z,
                    simbuf.simbuf_character_rot.valueRadians());
            }
        }
    }

    ImGui::End();
    ImGui::PopStyleVar(2); // WindowPadding, WindowRounding
}

void RoR::GUI::SurveyMap::CreateTerrainTextures()
{
    mMapCenterOffset     = Ogre::Vector2::ZERO; // Reset, maybe new terrain was loaded
    AxisAlignedBox aab   = App::GetSimTerrain()->getTerrainCollisionAAB();
    Vector3 terrain_size = App::GetSimTerrain()->getMaxTerrainSize();
    bool use_aab         = App::GetSimTerrain()->isFlat() && std::min(aab.getSize().x, aab.getSize().z) > 50.0f;

    if (terrain_size.isZeroLength() || use_aab && (aab.getSize().length() < terrain_size.length()))
    {
        terrain_size = aab.getSize();
        terrain_size.y = aab.getMaximum().y;
        Vector3 offset = aab.getCenter() - terrain_size / 2;
        mMapCenterOffset = Vector2(offset.x, offset.z);
    }

    mTerrainSize = Vector2(terrain_size.x, terrain_size.z);
    Ogre::Vector2 mMapCenter = mTerrainSize / 2;

    ConfigOptionMap ropts = App::GetOgreSubsystem()->GetOgreRoot()->getRenderSystem()->getConfigOptions();
    int resolution = StringConverter::parseInt(StringUtil::split(ropts["Video Mode"].currentValue, " x ")[0], 1024);
    int fsaa = StringConverter::parseInt(ropts["FSAA"].currentValue, 0);
    int res = std::pow(2, std::floor(std::log2(resolution)));

    mMapTextureCreatorStatic = std::unique_ptr<SurveyMapTextureCreator>(new SurveyMapTextureCreator(terrain_size.y));
    mMapTextureCreatorStatic->init(res, fsaa);
    mMapTextureCreatorStatic->update(mMapCenter + mMapCenterOffset, mTerrainSize);

    // TODO: Find out how to zoom into the static texture instead
    mMapTextureCreatorDynamic = std::unique_ptr<SurveyMapTextureCreator>(new SurveyMapTextureCreator(terrain_size.y));
    mMapTextureCreatorDynamic->init(res / 4, fsaa);
    mMapTextureCreatorDynamic->update(mMapCenter + mMapCenterOffset, mTerrainSize);
}


void RoR::GUI::SurveyMap::setMapZoom(float zoom)
{
    zoom = Math::Clamp(zoom, 0.0f, std::max(0.0f, (mTerrainSize.x - 50.0f) / mTerrainSize.x));

    if (mMapZoom == zoom)
        return;

    mMapZoom = zoom;
}

void RoR::GUI::SurveyMap::setMapZoomRelative(float delta)
{
    setMapZoom(mMapZoom + 0.5f * delta * (1.0f - mMapZoom));
}


const char* RoR::GUI::SurveyMap::getTypeByDriveable(int driveable)
{
    switch (driveable)
    {
    case NOT_DRIVEABLE:
        return "load";
    case TRUCK:
        return "truck";
    case AIRPLANE:
        return "airplane";
    case BOAT:
        return "boat";
    case MACHINE:
        return "machine";
    default:
        return "unknown";
    }
}

void RoR::GUI::SurveyMap::CycleMode()
{
    switch (mMapMode)
    {
    case SurveyMapMode::NONE:  mMapLastMode = mMapMode = SurveyMapMode::SMALL; break;
    case SurveyMapMode::SMALL: mMapLastMode = mMapMode = SurveyMapMode::BIG;   break;
    case SurveyMapMode::BIG:                  mMapMode = SurveyMapMode::NONE;  break;
    default:;
    }
}

void RoR::GUI::SurveyMap::ToggleMode()
{
    mMapMode = (mMapMode == SurveyMapMode::NONE) ? mMapLastMode : SurveyMapMode::NONE;
}

void RoR::GUI::SurveyMap::DrawMapIcon(ImVec2 view_pos, ImVec2 view_size, Ogre::Vector2 view_origin,
                                      std::string const& filename, std::string const& caption, 
                                      float pos_x, float pos_y, float angle)
{
    Ogre::TexturePtr tex;
    try
    {
        tex = Ogre::TextureManager::getSingleton().load(
            filename, RoR::ContentManager::ResourcePack::TEXTURES.resource_group_name);
    }
    catch (Ogre::FileNotFoundException)
    {
        // ignore silently and try loading substitute
    }
    if (!tex)
    {
        try
        {
            tex = Ogre::TextureManager::getSingleton().load(
                "icon_missing.dds", RoR::ContentManager::ResourcePack::TEXTURES.resource_group_name);
        }
        catch (Ogre::FileNotFoundException)
        {
            return; // Draw nothing
        }
    }

    Ogre::Vector2 terrn_size_adj = mTerrainSize;
    if (mMapMode == SurveyMapMode::SMALL)
    {
        terrn_size_adj = mTerrainSize * (1.f - mMapZoom);
    }

    ImVec2 img_pos;
    img_pos.x = view_pos.x + ((pos_x - view_origin.x) / terrn_size_adj.x) * view_size.x;
    img_pos.y = view_pos.y + ((pos_y - view_origin.y) / terrn_size_adj.x) * view_size.y;

    DrawImageRotated(reinterpret_cast<ImTextureID>(tex->getHandle()), img_pos,
        ImVec2(tex->getWidth(), tex->getHeight()), angle);

    ImDrawList* drawlist = ImGui::GetWindowDrawList();
    ImVec2 text_pos(img_pos.x - (ImGui::CalcTextSize(caption.c_str()).x/2), img_pos.y + 5);
    drawlist->AddText(text_pos, ImGui::GetColorU32(ImGui::GetStyle().Colors[ImGuiCol_Text]), caption.c_str());
}
