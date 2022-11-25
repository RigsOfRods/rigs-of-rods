/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2020 Thomas Fischer
    Copyright 2013-2023 Petr Ohlidal

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

#include "AppContext.h"
#include "Actor.h"
#include "ContentManager.h"
#include "GameContext.h"
#include "GfxActor.h"
#include "GfxScene.h"
#include "GUIManager.h"
#include "GUI_MainSelector.h"
#include "GUIUtils.h"
#include "InputEngine.h"
#include "Language.h"
#include "OgreImGui.h"
#include "SurveyMapTextureCreator.h"
#include "Terrain.h"
#include "TerrainObjectManager.h"
#include "Collisions.h"

using namespace RoR;
using namespace GUI;
using namespace Ogre;

void SurveyMap::Draw()
{
    // Check special cases
    if ((mMapMode == SurveyMapMode::BIG &&
        App::GetCameraManager()->GetCurrentBehavior() == CameraManager::CAMERA_BEHAVIOR_FREE) || 
        App::GetGuiManager()->MainSelector.IsVisible())
    {
        mMapMode = SurveyMapMode::NONE;
        return;
    }

    // Handle input
    if (App::GetInputEngine()->getEventBoolValueBounce(EV_SURVEY_MAP_TOGGLE_ICONS))
        App::gfx_surveymap_icons->setVal(!App::gfx_surveymap_icons->getBool());

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

        // Zoom in/out with mouse wheel also
        if (mWindowMouseHovered)
        {
            setMapZoomRelative(ImGui::GetIO().MouseWheel*0.5f);
        }
    }

    // Calculate window position
    Ogre::RenderWindow* rw = RoR::App::GetAppContext()->GetRenderWindow();
    ImVec2 view_size(0, 0);
    ImVec2 view_padding(8, 8);
    if (mMapMode == SurveyMapMode::BIG)
    {
        view_padding.y = 40; // Extra space for hints
        view_size.y = (rw->getWidth() * 0.55f) -
            ((2 * App::GetGuiManager()->GetTheme().screen_edge_padding.y) + (2 * WINDOW_PADDING));
        Vector3 terrn_size = App::GetGameContext()->GetTerrain()->getMaxTerrainSize(); // Y is 'up'!
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
        view_size.y = rw->getWidth() * 0.2f;
        view_size.x = view_size.y;
    }

    // Open window
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(WINDOW_PADDING, WINDOW_PADDING));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, WINDOW_ROUNDING);
    ImGui::SetNextWindowSize(ImVec2((view_size.x + view_padding.x), (view_size.y + view_padding.y)));

    GUIManager::GuiTheme const& theme = App::GetGuiManager()->GetTheme();

    if (mMapMode == SurveyMapMode::BIG)
    {
        ImGui::PushStyleColor(ImGuiCol_WindowBg, theme.semitransparent_window_bg);
        ImGui::SetNextWindowPosCenter();
    }
    else if (mMapMode == SurveyMapMode::SMALL)
    {
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0,0,0,0));
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0,0,0,0));

        float x = ImGui::GetIO().DisplaySize.x -
            (view_size.x + App::GetGuiManager()->GetTheme().screen_edge_padding.x);
        ImGui::SetNextWindowPos(ImVec2(x, 100.f));
    }
    ImGui::Begin("SurveyMap", nullptr, flags);

    // Draw map texture
    ImVec2 tl_screen_pos = ImGui::GetCursorScreenPos();
    Ogre::Vector2 smallmap_center;
    Ogre::Vector2 smallmap_size;
    Ogre::Vector2 view_origin;
    Ogre::Vector2 texcoords_top_left(0,0);
    Ogre::Vector2 texcoords_bottom_right(1,1);
    if (mMapMode == SurveyMapMode::BIG)
    {
        view_origin = mMapCenterOffset;
    }
    else if (mMapMode == SurveyMapMode::SMALL)
    {
        // Calc. view center
        smallmap_size = mTerrainSize * (1.0f - mMapZoom);
        Ogre::Vector2 player_map_pos;
        ActorPtr actor = App::GetGfxScene()->GetSimDataBuffer().simbuf_player_actor;
        if (actor)
        {
            auto& actor_data = actor->GetGfxActor()->GetSimDataBuffer();
            player_map_pos = (Vector2(actor_data.simbuf_pos.x, actor_data.simbuf_pos.z));
        }
        else
        {
            auto& scene_data = App::GetGfxScene()->GetSimDataBuffer();
            player_map_pos = (Vector2(scene_data.simbuf_character_pos.x, scene_data.simbuf_character_pos.z));
        }

        smallmap_center.x = Math::Clamp(player_map_pos.x - mMapCenterOffset.x, smallmap_size.x / 2, mTerrainSize.x - smallmap_size.x / 2);
        smallmap_center.y = Math::Clamp(player_map_pos.y - mMapCenterOffset.y, smallmap_size.y / 2, mTerrainSize.y - smallmap_size.y / 2);

        view_origin = ((smallmap_center + mMapCenterOffset) - smallmap_size / 2);

        texcoords_top_left = (smallmap_center - (smallmap_size/2)) / mTerrainSize;
        texcoords_bottom_right = (smallmap_center + (smallmap_size/2)) / mTerrainSize;
    }

    bool w_adj = false;
    if (!App::GetGuiManager()->TopMenubar.ai_waypoints.empty())
    {
        for (int i = 0; i < App::GetGuiManager()->TopMenubar.ai_waypoints.size(); i++)
        {
            ImVec2 cw_dist = this->CalcWaypointMapPos(tl_screen_pos, view_size, view_origin, i);
            if (abs(cw_dist.x) <= 5 && abs(cw_dist.y) <= 5)
            {
                w_adj = true;
            }
        }
    }

    ImGui::BeginChild("map", ImVec2(0.f, view_size.y), false);

    if (mMapMode == SurveyMapMode::BIG)
    {
        ImGui::Image(reinterpret_cast<ImTextureID>(mMapTexture->getHandle()), view_size);
    }
    else if (mMapMode == SurveyMapMode::SMALL)
    {
        ImDrawList* drawlist = ImGui::GetWindowDrawList();

        ImVec2 p_min = ImGui::GetCursorScreenPos();
        ImVec2 p_max = ImVec2(p_min.x + view_size.x, p_min.y + view_size.y);
        ImVec2 p_offset = ImVec2(5.f, 5.f);
        ImVec2 _min(p_min + p_offset);
        ImVec2 _max(p_max + p_offset);
        ImVec2 uv_min(texcoords_top_left.x, texcoords_top_left.y);
        ImVec2 uv_max(texcoords_bottom_right.x, texcoords_bottom_right.y);
        m_circle_center =_min + ((_max-_min)/2);
        m_circle_radius = view_size.x/2 - p_offset.x;

        // Outline
        drawlist->AddCircle(ImVec2(p_min.x + view_size.x/2,  p_min.y + view_size.y/2), view_size.x/2 - 2*p_offset.x, ImGui::GetColorU32(theme.semitransparent_window_bg), 96, 20);

        // The texture
        drawlist->AddCircularImage(reinterpret_cast<ImTextureID>(mMapTexture->getHandle()), _min, _max, uv_min, uv_max, ImGui::GetColorU32(ImVec4(1,1,1,1)), m_circle_radius);

        // An invisible button so we can catch it below
        ImGui::InvisibleButton("circle", view_size);
    }

    if (ImGui::IsItemClicked(0) || ImGui::IsItemClicked(1)) // 0 = left click, 1 = right click
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

        if (ImGui::IsItemClicked(1)) // Set AI waypoint
        {
            float y = App::GetGameContext()->GetTerrain()->GetCollisions()->getSurfaceHeight(mouse_map_pos.x, mouse_map_pos.y);
            ai_events waypoint;
            waypoint.position = Ogre::Vector3(mouse_map_pos.x, y, mouse_map_pos.y);
            App::GetGuiManager()->TopMenubar.ai_waypoints.push_back(waypoint);
        }
        else if (!w_adj) // Teleport
        {
            Ogre::Vector3* payload = new Ogre::Vector3(mouse_map_pos.x, 0.f, mouse_map_pos.y);
            App::GetGameContext()->PushMessage(Message(MSG_SIM_TELEPORT_PLAYER_REQUESTED, (void*)payload));
         }
    }
    else if (ImGui::IsItemClicked(2) && !w_adj) // 2 = middle click, clear AI waypoints
    {
        App::GetGuiManager()->TopMenubar.ai_waypoints.clear();
    }
    else if (ImGui::IsItemHovered())
    {
        // Draw teleport cursor
        ImVec2 mouse_pos = ImGui::GetMousePos();
        ImDrawList* drawlist = ImGui::GetWindowDrawList();
        drawlist->AddCircleFilled(mouse_pos, 5, ImGui::GetColorU32(ImVec4(1,0,0,1)));
        if (mMapMode == SurveyMapMode::SMALL)
        {
            const char* title = "Teleport/Waypoint";
            ImVec2 text_pos(mouse_pos.x - (ImGui::CalcTextSize(title).x/2), mouse_pos.y - 25);
            drawlist->AddText(text_pos, ImGui::GetColorU32(ImGui::GetStyle().Colors[ImGuiCol_Text]), title);
        }
    }

    // Draw AI waypoints
    if (!App::GetGuiManager()->TopMenubar.ai_waypoints.empty())
    {
        for (int i = 0; i < App::GetGuiManager()->TopMenubar.ai_waypoints.size(); i++)
        {
            ImVec2 cw_dist = this->DrawWaypoint(tl_screen_pos, view_size, view_origin, std::to_string(i), i);

            if (abs(cw_dist.x) <= 5 && abs(cw_dist.y) <= 5)
            {
                if (ImGui::IsMouseClicked(0))
                {
                    mMouseClicked = true;
                    mWaypointNum = i;
                }
                else if (ImGui::IsMouseReleased(0))
                {
                    mMouseClicked = false;
                }
            }

            if (mMouseClicked)
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

                float y = App::GetGameContext()->GetTerrain()->GetCollisions()->getSurfaceHeight(mouse_map_pos.x, mouse_map_pos.y);
                App::GetGuiManager()->TopMenubar.ai_waypoints[mWaypointNum].position = Ogre::Vector3(mouse_map_pos.x, y, mouse_map_pos.y);
            }
            else if (abs(cw_dist.x) <= 5 && abs(cw_dist.y) <= 5 && ImGui::IsItemClicked(2))
            {
                App::GetGuiManager()->TopMenubar.ai_waypoints.erase(App::GetGuiManager()->TopMenubar.ai_waypoints.begin() + i);
            }
        }
    }

    if (App::gfx_surveymap_icons->getBool())
    {
        // Draw terrain object icons
        for (SurveyMapEntity& e: App::GetGameContext()->GetTerrain()->getSurveyMapEntities())
        {
            int id = App::GetGameContext()->GetRaceSystem().GetRaceId();
            bool visible = !((e.type == "checkpoint" && e.id != id) || (e.type == "racestart" && id != -1 && e.id != id));

            if ((visible) && (!App::gfx_declutter_map->getBool()))
            {
                this->CacheMapIcon(e);
                this->DrawMapIcon(e, tl_screen_pos, view_size, view_origin);
            }
        }

        // Draw actor icons
        for (GfxActor* gfx_actor: App::GetGfxScene()->GetGfxActors())
        {
            const char* type_str = this->getTypeByDriveable(gfx_actor->GetActor());
            int truckstate = gfx_actor->GetActorState();
            Str<100> fileName;

            if (truckstate == static_cast<int>(ActorState::LOCAL_SIMULATED))
                fileName << "icon_" << type_str << "_activated.dds"; // green icon
            else if (truckstate == static_cast<int>(ActorState::NETWORKED_OK))
                fileName << "icon_" << type_str << "_networked.dds"; // blue icon
            else
                fileName << "icon_" << type_str << ".dds"; // gray icon

            auto& simbuf = gfx_actor->GetSimDataBuffer();

            // Update the surveymap icon entry
            SurveyMapEntity& e = gfx_actor->getSurveyMapEntity();
            e.pos = simbuf.simbuf_pos;
            e.rot_angle = Ogre::Radian(simbuf.simbuf_rotation);
            e.filename = fileName.ToCStr();

            // Update net player name
            if (simbuf.simbuf_actor_state == ActorState::NETWORKED_OK)
            {
                e.caption = simbuf.simbuf_net_username;
                e.caption_color = App::GetNetwork()->GetPlayerColor(simbuf.simbuf_net_colornum);
                e.draw_caption = true;
            }

            // Cache and draw icon
            this->CacheMapIcon(e);
            this->DrawMapIcon(e, tl_screen_pos, view_size, view_origin);
        }

        // Draw character icons
        for (GfxCharacter* gfx_character: App::GetGfxScene()->GetGfxCharacters())
        {
            auto& simbuf = gfx_character->xc_simbuf;
            if (!simbuf.simbuf_actor_coupling)
            {
                // Update the surveymap icon entry
                SurveyMapEntity& e = gfx_character->xc_surveymap_entity;
                e.pos = simbuf.simbuf_character_pos;
                e.rot_angle = simbuf.simbuf_character_rot;
                e.filename = "icon_person_activated.dds"; // green icon

                // Update net player name
                if (simbuf.simbuf_is_remote)
                {
                    e.filename = "icon_person_networked.dds"; // blue icon
                    e.caption = simbuf.simbuf_net_username;
                    e.caption_color = App::GetNetwork()->GetPlayerColor(simbuf.simbuf_color_number);
                    e.draw_caption = true;
                }

                // Cache and draw icon
                this->CacheMapIcon(e);
                this->DrawMapIcon(e, tl_screen_pos, view_size, view_origin);
            }
        }
    }
    ImGui::EndChild();

    // Bottom area
    float orig_y = ImGui::GetCursorPosY();

    if (!m_icons_cached)
    {
        this->CacheIcons();
    }

    ImGui::Image(reinterpret_cast<ImTextureID>(m_left_mouse_button->getHandle()), ImVec2(28, 24));
    ImGui::SameLine();
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + ImGui::GetStyle().ItemSpacing.y);

    const char* text = "Teleport";
    if (w_adj || mMouseClicked)
    {
        text = "Drag to adjust this waypoint";
    }
    ImGui::Text(text);

    ImGui::SameLine();
    ImGui::SetCursorPosY(orig_y);
    ImGui::Image(reinterpret_cast<ImTextureID>(m_right_mouse_button->getHandle()), ImVec2(28, 24));
    ImGui::SameLine();
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + ImGui::GetStyle().ItemSpacing.y);

    text = "Set AI waypoint";
    ImGui::Text(text);

    ImGui::SameLine();
    ImGui::SetCursorPosY(orig_y);
    ImGui::Image(reinterpret_cast<ImTextureID>(m_middle_mouse_button->getHandle()), ImVec2(28, 24));
    ImGui::SameLine();
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + ImGui::GetStyle().ItemSpacing.y);

    text = "Remove all AI waypoints";
    if (w_adj || mMouseClicked)
    {
        text = "Remove this waypoint";
    }
    ImGui::Text(text);

    ImGui::SameLine();
    ImGui::SetCursorPosY(orig_y);
    ImGui::Image(reinterpret_cast<ImTextureID>(m_middle_mouse_scroll_button->getHandle()), ImVec2(28, 24));
    ImGui::SameLine();
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + ImGui::GetStyle().ItemSpacing.y);

    text = "Zoom mini map";
    ImGui::Text(text);

    mWindowMouseHovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows);

    ImGui::End();

    if (mMapMode == SurveyMapMode::BIG)
    {
        ImGui::PopStyleColor(1); // WindowBg
    }
    else if (mMapMode == SurveyMapMode::SMALL)
    {
        ImGui::PopStyleColor(2); // WindowBg, ChildBg
    }

    ImGui::PopStyleVar(2); // WindowPadding, WindowRounding
}

void SurveyMap::CreateTerrainTextures()
{
    mMapCenterOffset     = Ogre::Vector2::ZERO; // Reset, maybe new terrain was loaded
    if (mMapTexture)
    {
        if (mMapTexture->getLoadingState() != Ogre::Resource::LoadingState::LOADSTATE_UNLOADED)
        {
            Ogre::TextureManager::getSingleton().unload(mMapTexture->getName(), mMapTexture->getGroup());
        }
        Ogre::TextureManager::getSingleton().remove(mMapTexture->getName(), mMapTexture->getGroup());
        mMapTexture.setNull();
    }
    mMapZoom = 0.5f;
    mMapMode = SurveyMapMode::NONE;

    AxisAlignedBox aab   = App::GetGameContext()->GetTerrain()->getTerrainCollisionAAB();
    Vector3 terrain_size = App::GetGameContext()->GetTerrain()->getMaxTerrainSize();
    bool use_aab         = App::GetGameContext()->GetTerrain()->isFlat() && std::min(aab.getSize().x, aab.getSize().z) > 50.0f;

    if (terrain_size.isZeroLength() || use_aab && (aab.getSize().length() < terrain_size.length()))
    {
        terrain_size = aab.getSize();
        terrain_size.y = aab.getMaximum().y;
        Vector3 offset = aab.getCenter() - terrain_size / 2;
        mMapCenterOffset = Vector2(offset.x, offset.z);
    }

    mTerrainSize = Vector2(terrain_size.x, terrain_size.z);
    Ogre::Vector2 mMapCenter = mTerrainSize / 2;

    ConfigOptionMap ropts = App::GetAppContext()->GetOgreRoot()->getRenderSystem()->getConfigOptions();
    int fsaa = StringConverter::parseInt(ropts["FSAA"].currentValue, 0);

    SurveyMapTextureCreator texCreatorStatic(terrain_size.y);
    texCreatorStatic.init(4096, fsaa);
    texCreatorStatic.update(mMapCenter + mMapCenterOffset, mTerrainSize);
    mMapTexture = texCreatorStatic.convertTextureToStatic(
        "SurveyMapStatic", App::GetGameContext()->GetTerrain()->getTerrainFileResourceGroup());
}

void SurveyMap::UpdateTerrainTextures()
{
    mMapCenterOffset     = Ogre::Vector2::ZERO; // Reset, maybe new terrain was loaded
    AxisAlignedBox aab   = App::GetGameContext()->GetTerrain()->getTerrainCollisionAAB();
    Vector3 terrain_size = App::GetGameContext()->GetTerrain()->getMaxTerrainSize();
    bool use_aab         = App::GetGameContext()->GetTerrain()->isFlat() && std::min(aab.getSize().x, aab.getSize().z) > 50.0f;

    if (terrain_size.isZeroLength() || use_aab && (aab.getSize().length() < terrain_size.length()))
    {
        terrain_size = aab.getSize();
        terrain_size.y = aab.getMaximum().y;
        Vector3 offset = aab.getCenter() - terrain_size / 2;
        mMapCenterOffset = Vector2(offset.x, offset.z);
    }

    mTerrainSize = Vector2(terrain_size.x, terrain_size.z);
    Ogre::Vector2 mMapCenter = mTerrainSize / 2;

    mMapTextureCreatorStatic->update(mMapCenter + mMapCenterOffset, mTerrainSize);
    mMapTextureCreatorDynamic->update(mMapCenter + mMapCenterOffset, mTerrainSize);
}

void SurveyMap::setMapZoom(float zoom)
{
    zoom = Math::Clamp(zoom, 0.0f, std::max(0.0f, (mTerrainSize.x - 50.0f) / mTerrainSize.x));

    if (mMapZoom == zoom)
        return;

    mMapZoom = zoom;
}

void SurveyMap::setMapZoomRelative(float delta)
{
    setMapZoom(mMapZoom + 0.5f * delta * (1.0f - mMapZoom));
}

void SurveyMap::Close()
{
    mMapMode = SurveyMapMode::NONE;
}

const char* SurveyMap::getTypeByDriveable(const ActorPtr& actor)
{
    switch (actor->ar_driveable)
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
    case AI:
        return this->getAIType(actor);
    default:
        return "unknown";
    }
}

const char* SurveyMap::getAIType(const ActorPtr& actor)
{
    if (actor->ar_engine)
    {
        return "truck";
    }
    else if (actor->ar_num_aeroengines > 0)
    {
        return "airplane";
    }
    else if (actor->ar_num_screwprops > 0)
    {
        return "boat";
    }

    return "unknown";
}

void SurveyMap::CycleMode()
{
    switch (mMapMode)
    {
    case SurveyMapMode::NONE:  mMapLastMode = mMapMode = SurveyMapMode::SMALL; break;
    case SurveyMapMode::SMALL: mMapLastMode = mMapMode = SurveyMapMode::BIG;   break;
    case SurveyMapMode::BIG:                  mMapMode = SurveyMapMode::NONE;  break;
    default:;
    }

   if (mMapMode != SurveyMapMode::NONE)
   {
       this->UpdateTerrainTextures();
   }
}

void SurveyMap::ToggleMode()
{
    mMapMode = (mMapMode == SurveyMapMode::NONE) ? mMapLastMode : SurveyMapMode::NONE;
}

void SurveyMap::DrawMapIcon(const SurveyMapEntity& e, ImVec2 view_pos, ImVec2 view_size, Ogre::Vector2 view_origin)
{
    Ogre::Vector2 terrn_size_adj = mTerrainSize;
    if (mMapMode == SurveyMapMode::SMALL)
    {
        terrn_size_adj = mTerrainSize * (1.f - mMapZoom);
    }

    ImVec2 img_pos;
    img_pos.x = view_pos.x + ((e.pos.x - view_origin.x) / terrn_size_adj.x) * view_size.x;
    img_pos.y = view_pos.y + ((e.pos.z - view_origin.y) / terrn_size_adj.y) * view_size.y;
    float img_dist = (img_pos.x - m_circle_center.x) * (img_pos.x - m_circle_center.x) + (img_pos.y - m_circle_center.y) * (img_pos.y - m_circle_center.y);

    if (!e.cached_icon
        || (mMapMode == SurveyMapMode::SMALL && img_dist > (m_circle_radius * m_circle_radius)*0.8))
    {
        return;
    }

    DrawImageRotated(reinterpret_cast<ImTextureID>(e.cached_icon->getHandle()), img_pos,
        ImVec2(e.cached_icon->getWidth(), e.cached_icon->getHeight()), e.rot_angle.valueRadians());

    if (e.draw_caption)
    {
        ImVec2 text_pos(img_pos.x - (ImGui::CalcTextSize(e.caption.c_str()).x/2), img_pos.y + 5);
        ImVec4 text_color(e.caption_color.r, e.caption_color.g, e.caption_color.b, 1.f);
        ImGui::GetWindowDrawList()->AddText(text_pos, ImColor(text_color), e.caption.c_str());
    }
    else
    {
        ImVec2 dist = ImGui::GetMousePos() - img_pos;
        if (!e.caption.empty() && abs(dist.x) <= 5 && abs(dist.y) <= 5)
        {
            ImGui::BeginTooltip();
            ImGui::Text("%s", e.caption.c_str());
            ImGui::EndTooltip();
        }
    }
}

ImVec2 SurveyMap::DrawWaypoint(ImVec2 view_pos, ImVec2 view_size, Ogre::Vector2 view_origin,
                                      std::string const& caption, int idx)
{
    Ogre::Vector2 terrn_size_adj = mTerrainSize;
    if (mMapMode == SurveyMapMode::SMALL)
    {
        terrn_size_adj = mTerrainSize * (1.f - mMapZoom);
    }

    ImVec2 wp_pos;
    wp_pos.x = view_pos.x + ((App::GetGuiManager()->TopMenubar.ai_waypoints[idx].position.x - view_origin.x) / terrn_size_adj.x) * view_size.x;
    wp_pos.y = view_pos.y + ((App::GetGuiManager()->TopMenubar.ai_waypoints[idx].position.z - view_origin.y) / terrn_size_adj.y) * view_size.y;

    float wp_dist = (wp_pos.x - m_circle_center.x) * (wp_pos.x - m_circle_center.x) + (wp_pos.y - m_circle_center.y) * (wp_pos.y - m_circle_center.y);
    bool wp_draw = true;
    if (mMapMode == SurveyMapMode::SMALL && wp_dist > (m_circle_radius * m_circle_radius)*0.8)
    {
        wp_draw = false;
    }

    ImDrawList* drawlist = ImGui::GetWindowDrawList();
    ImVec4 col = ImVec4(1,0,0,1);
    ImVec2 dist = ImGui::GetMousePos() - wp_pos;
    if (abs(dist.x) <= 5 && abs(dist.y) <= 5)
    {
        col = ImVec4(1,1,0,1);

        ImGui::BeginTooltip();
        ImGui::Text("%s %s", "Waypoint", caption.c_str());
        if (mMapMode == SurveyMapMode::BIG)
        {
            std::string wpos = "x:" + std::to_string(App::GetGuiManager()->TopMenubar.ai_waypoints[idx].position.x) + " y:" + std::to_string(App::GetGuiManager()->TopMenubar.ai_waypoints[idx].position.y) + " z:" + std::to_string(App::GetGuiManager()->TopMenubar.ai_waypoints[idx].position.z);
            ImGui::TextDisabled("%s %s", "Position: ", wpos.c_str());
        }
        ImGui::EndTooltip();
    }

    if (mMapMode == SurveyMapMode::BIG || (mMapMode == SurveyMapMode::SMALL && wp_draw))
    {
        drawlist->AddCircleFilled(wp_pos, 5, ImGui::GetColorU32(ImVec4(col)));
    }

    if (App::GetGuiManager()->TopMenubar.ai_waypoints.size() >= 2 && idx != App::GetGuiManager()->TopMenubar.ai_waypoints.size() - 1)
    {
        ImVec2 next_wp_pos;
        next_wp_pos.x = view_pos.x + ((App::GetGuiManager()->TopMenubar.ai_waypoints[idx+1].position.x - view_origin.x) / terrn_size_adj.x) * view_size.x;
        next_wp_pos.y = view_pos.y + ((App::GetGuiManager()->TopMenubar.ai_waypoints[idx+1].position.z - view_origin.y) / terrn_size_adj.y) * view_size.y;

        float next_wp_dist = (next_wp_pos.x - m_circle_center.x) * (next_wp_pos.x - m_circle_center.x) + (next_wp_pos.y - m_circle_center.y) * (next_wp_pos.y - m_circle_center.y);
        bool next_wp_draw = true;
        if (mMapMode == SurveyMapMode::SMALL && next_wp_dist > (m_circle_radius * m_circle_radius)*0.8)
        {
            next_wp_draw = false;
        }

        if (mMapMode == SurveyMapMode::BIG || (mMapMode == SurveyMapMode::SMALL && wp_draw && next_wp_draw))
        {
            drawlist->AddLine(wp_pos, next_wp_pos, ImGui::GetColorU32(ImVec4(1,0,0,1)));
        }
    }

    return ImGui::GetMousePos() - wp_pos;
}

ImVec2 SurveyMap::CalcWaypointMapPos(ImVec2 view_pos, ImVec2 view_size, Ogre::Vector2 view_origin, int idx)
{
    Ogre::Vector2 terrn_size_adj = mTerrainSize;
    if (mMapMode == SurveyMapMode::SMALL)
    {
        terrn_size_adj = mTerrainSize * (1.f - mMapZoom);
    }

    ImVec2 pos;
    pos.x = view_pos.x + ((App::GetGuiManager()->TopMenubar.ai_waypoints[idx].position.x - view_origin.x) / terrn_size_adj.x) * view_size.x;
    pos.y = view_pos.y + ((App::GetGuiManager()->TopMenubar.ai_waypoints[idx].position.z - view_origin.y) / terrn_size_adj.y) * view_size.y;

    return ImGui::GetMousePos() - pos;
}

void SurveyMap::CacheIcons()
{
    // Thanks WillM for the icons!

    m_left_mouse_button = FetchIcon("left-mouse-button.png");
    m_middle_mouse_button = FetchIcon("middle-mouse-button.png");
    m_middle_mouse_scroll_button = FetchIcon("middle-mouse-scroll-button.png");
    m_right_mouse_button = FetchIcon("right-mouse-button.png");

    m_icons_cached = true;
}

void SurveyMap::CacheMapIcon(SurveyMapEntity& e)
{
    // Check if requested icon changed
    if (e.cached_icon && e.cached_icon->getName() != e.filename)
        e.cached_icon.setNull();

    // Check if valid icon is already loaded
    if (e.cached_icon)
        return;

    if (e.resource_group == "")
        e.resource_group = ContentManager::ResourcePack::TEXTURES.resource_group_name;

    if (e.filename != "")
    {
        try
        {
            e.cached_icon = Ogre::TextureManager::getSingleton().load(e.filename, e.resource_group);
        }
        catch (Ogre::FileNotFoundException)
        {
            // ignore silently and try loading substitute
        }
    }

    if (!e.cached_icon)
    {
        try
        {
            e.cached_icon = Ogre::TextureManager::getSingleton().load(
                "icon_missing.dds", ContentManager::ResourcePack::TEXTURES.resource_group_name);
            e.filename = "icon_missing.dds"; // Prevent constant reloading
        }
        catch (Ogre::FileNotFoundException)
        {
            return; // Draw nothing
        }
    }
}
