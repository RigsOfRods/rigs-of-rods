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
    ImGui::PushStyleColor(ImGuiCol_WindowBg, theme.semitransparent_window_bg);

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

        // Update texture
        if (mMapZoom != 0.0f)
        {
            mMapTextureCreatorDynamic->update(smallmap_center + mMapCenterOffset, smallmap_size);
        }
        tex = mMapTextureCreatorDynamic->GetTexture();
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

    ImGui::Image(reinterpret_cast<ImTextureID>(tex->getHandle()), view_size);
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
        for (TerrainObjectManager::MapEntity& e: App::GetGameContext()->GetTerrain()->getObjectManager()->GetMapEntities())
        {
            int id = App::GetGameContext()->GetRaceSystem().GetRaceId();
            bool visible = !((e.type == "checkpoint" && e.id != id) || (e.type == "racestart" && id != -1 && e.id != id));
            Str<100> filename;
            filename << "icon_" << e.type << ".dds";

            if ((visible) && (!App::gfx_declutter_map->getBool()))
            {
                this->DrawMapIcon(tl_screen_pos, view_size, view_origin, filename.ToCStr(), e.name, e.pos.x, e.pos.z, e.rot);
            }
        }

        // Draw actor icons
        for (GfxActor* gfx_actor: App::GetGfxScene()->GetGfxActors())
        {
            const char* type_str = this->getTypeByDriveable(gfx_actor->GetActorDriveable(), gfx_actor->GetActor());
            int truckstate = gfx_actor->GetActorState();
            Str<100> fileName;

            if (truckstate == static_cast<int>(ActorState::LOCAL_SIMULATED))
                fileName << "icon_" << type_str << "_activated.dds"; // green icon
            else if (truckstate == static_cast<int>(ActorState::NETWORKED_OK))
                fileName << "icon_" << type_str << "_networked.dds"; // blue icon
            else
                fileName << "icon_" << type_str << ".dds"; // gray icon

            auto& simbuf = gfx_actor->GetSimDataBuffer();
            std::string caption = (App::mp_state->getEnum<MpState>() == MpState::CONNECTED) ? simbuf.simbuf_net_username : "";
            this->DrawMapIcon(tl_screen_pos, view_size, view_origin, fileName.ToCStr(), caption, 
                simbuf.simbuf_pos.x, simbuf.simbuf_pos.z, simbuf.simbuf_rotation);
        }

        // Draw character icons
        for (GfxCharacter* gfx_character: App::GetGfxScene()->GetGfxCharacters())
        {
            auto& simbuf = gfx_character->xc_simbuf;
            if (!simbuf.simbuf_actor_coupling)
            {
                std::string caption = (App::mp_state->getEnum<MpState>() == MpState::CONNECTED) ? simbuf.simbuf_net_username : "";
                this->DrawMapIcon(tl_screen_pos, view_size, view_origin, "icon_person_activated.dds", caption, 
                    simbuf.simbuf_character_pos.x, simbuf.simbuf_character_pos.z,
                    simbuf.simbuf_character_rot.valueRadians());
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
    ImGui::PopStyleColor(); // WindowBg
    ImGui::PopStyleVar(2); // WindowPadding, WindowRounding
}

void SurveyMap::CreateTerrainTextures()
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

    ConfigOptionMap ropts = App::GetAppContext()->GetOgreRoot()->getRenderSystem()->getConfigOptions();
    int resolution = StringConverter::parseInt(StringUtil::split(ropts["Video Mode"].currentValue, " x ")[0], 1024);
    int fsaa = StringConverter::parseInt(ropts["FSAA"].currentValue, 0);
    int res = std::pow(2, std::floor(std::log2(resolution)));

    mMapTextureCreatorStatic = std::unique_ptr<SurveyMapTextureCreator>(new SurveyMapTextureCreator(terrain_size.y));
    mMapTextureCreatorStatic->init(res / 1.5, fsaa);
    mMapTextureCreatorStatic->update(mMapCenter + mMapCenterOffset, mTerrainSize);

    // TODO: Find out how to zoom into the static texture instead
    mMapTextureCreatorDynamic = std::unique_ptr<SurveyMapTextureCreator>(new SurveyMapTextureCreator(terrain_size.y));
    mMapTextureCreatorDynamic->init(res / 4, fsaa);
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


const char* SurveyMap::getTypeByDriveable(ActorType driveable, ActorPtr actor)
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
    case AI:
        return this->getAIType(actor);
    default:
        return "unknown";
    }
}

const char* SurveyMap::getAIType(ActorPtr actor)
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
}

void SurveyMap::ToggleMode()
{
    mMapMode = (mMapMode == SurveyMapMode::NONE) ? mMapLastMode : SurveyMapMode::NONE;
}

void SurveyMap::DrawMapIcon(ImVec2 view_pos, ImVec2 view_size, Ogre::Vector2 view_origin,
                                      std::string const& filename, std::string const& caption, 
                                      float pos_x, float pos_y, float angle)
{
    Ogre::TexturePtr tex;
    try
    {
        tex = Ogre::TextureManager::getSingleton().load(
            filename, ContentManager::ResourcePack::TEXTURES.resource_group_name);
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
                "icon_missing.dds", ContentManager::ResourcePack::TEXTURES.resource_group_name);
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
    img_pos.y = view_pos.y + ((pos_y - view_origin.y) / terrn_size_adj.y) * view_size.y;

    DrawImageRotated(reinterpret_cast<ImTextureID>(tex->getHandle()), img_pos,
        ImVec2(tex->getWidth(), tex->getHeight()), angle);

    ImDrawList* drawlist = ImGui::GetWindowDrawList();
    ImVec2 text_pos(img_pos.x - (ImGui::CalcTextSize(caption.c_str()).x/2), img_pos.y + 5);
    drawlist->AddText(text_pos, ImGui::GetColorU32(ImGui::GetStyle().Colors[ImGuiCol_Text]), caption.c_str());
}

ImVec2 SurveyMap::DrawWaypoint(ImVec2 view_pos, ImVec2 view_size, Ogre::Vector2 view_origin,
                                      std::string const& caption, int idx)
{
    Ogre::Vector2 terrn_size_adj = mTerrainSize;
    if (mMapMode == SurveyMapMode::SMALL)
    {
        terrn_size_adj = mTerrainSize * (1.f - mMapZoom);
    }

    ImVec2 pos1;
    pos1.x = view_pos.x + ((App::GetGuiManager()->TopMenubar.ai_waypoints[idx].position.x - view_origin.x) / terrn_size_adj.x) * view_size.x;
    pos1.y = view_pos.y + ((App::GetGuiManager()->TopMenubar.ai_waypoints[idx].position.z - view_origin.y) / terrn_size_adj.y) * view_size.y;

    ImDrawList* drawlist = ImGui::GetWindowDrawList();
    ImVec4 col = ImVec4(1,0,0,1);
    ImVec2 dist = ImGui::GetMousePos() - pos1;
    if (abs(dist.x) <= 5 && abs(dist.y) <= 5)
    {
        col = ImVec4(1,1,0,1);
    }
    drawlist->AddCircleFilled(pos1, 5, ImGui::GetColorU32(ImVec4(col)));
    ImVec2 text_pos(pos1.x - (ImGui::CalcTextSize(caption.c_str()).x/2), pos1.y + 5);
    drawlist->AddText(text_pos, ImGui::GetColorU32(ImGui::GetStyle().Colors[ImGuiCol_Text]), caption.c_str());

    if (App::GetGuiManager()->TopMenubar.ai_waypoints.size() >= 2 && idx != App::GetGuiManager()->TopMenubar.ai_waypoints.size() - 1)
    {
        ImVec2 pos2;
        pos2.x = view_pos.x + ((App::GetGuiManager()->TopMenubar.ai_waypoints[idx+1].position.x - view_origin.x) / terrn_size_adj.x) * view_size.x;
        pos2.y = view_pos.y + ((App::GetGuiManager()->TopMenubar.ai_waypoints[idx+1].position.z - view_origin.y) / terrn_size_adj.y) * view_size.y;
        drawlist->AddLine(pos1, pos2, ImGui::GetColorU32(ImVec4(1,0,0,1)));
    }

    return ImGui::GetMousePos() - pos1;
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
