/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer

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

#include "SurveyMapManager.h"

#include "Application.h"
#include "Beam.h"
#include "GUIManager.h"
#include "GUI_MainSelector.h"
#include "InputEngine.h"
#include "Language.h"
#include "OgreSubsystem.h"
#include "RoRFrameListener.h"
#include "SurveyMapEntity.h"
#include "SurveyMapTextureCreator.h"
#include "TerrainManager.h"

using namespace RoR;
using namespace Ogre;

SurveyMapManager::SurveyMapManager()
    : mMapCenter(Vector2::ZERO), mMapCenterOffset(Vector2::ZERO), mMapLastMode(SurveyMapMode::SMALL),
      mMapMode(SurveyMapMode::NONE), mMapSize(Vector2::ZERO), mMapTextureCreatorStatic(), mMapTextureCreatorDynamic(),
      mMapZoom(0.0f), mMapLastZoom(0.0f), mHidden(false), mPlayerPosition(Vector2::ZERO), mTerrainSize(Vector2::ZERO)
{
    initialiseByAttributes(this);
    mMainWidget->setVisible(false);
}

SurveyMapManager::~SurveyMapManager()
{
    for (auto entity : mMapEntities)
    {
        if (entity) { delete entity; }
    }
}

void SurveyMapManager::init()
{
    AxisAlignedBox aab          = App::GetSimTerrain()->getTerrainCollisionAAB();
    Vector3        terrain_size = App::GetSimTerrain()->getMaxTerrainSize();
    bool           use_aab      = App::GetSimTerrain()->isFlat() && std::min(aab.getSize().x, aab.getSize().z) > 50.0f;

    if (terrain_size.isZeroLength() || use_aab && (aab.getSize().length() < terrain_size.length()))
    {
        terrain_size     = aab.getSize();
        terrain_size.y   = aab.getMaximum().y;
        Vector3 offset   = aab.getCenter() - terrain_size / 2;
        mMapCenterOffset = Vector2(offset.x, offset.z);
    }

    mTerrainSize    = Vector2(terrain_size.x, terrain_size.z);
    mPlayerPosition = mTerrainSize / 2;
    mMapCenter      = mTerrainSize / 2;
    mMapSize        = mTerrainSize;

    ConfigOptionMap ropts      = App::GetOgreSubsystem()->GetOgreRoot()->getRenderSystem()->getConfigOptions();
    int             resolution = StringConverter::parseInt(StringUtil::split(ropts["Video Mode"].currentValue, " x ")[0], 1024);
    int             fsaa       = StringConverter::parseInt(ropts["FSAA"].currentValue, 0);
    int             res        = std::pow(2, std::floor(std::log2(resolution)));

    mMapTextureCreatorStatic = std::unique_ptr<SurveyMapTextureCreator>(new SurveyMapTextureCreator(terrain_size.y));
    mMapTextureCreatorStatic->init(res, fsaa);
    mMapTextureCreatorStatic->update(mMapCenter + mMapCenterOffset, mMapSize);

    // TODO: Find out how to zoom into the static texture instead
    mMapTextureCreatorDynamic = std::unique_ptr<SurveyMapTextureCreator>(new SurveyMapTextureCreator(terrain_size.y));
    mMapTextureCreatorDynamic->init(res / 4, fsaa);
    mMapTextureCreatorDynamic->update(mMapCenter + mMapCenterOffset, mMapSize);

    mMapTexture->eventMouseSetFocus += MyGUI::newDelegate(this, &SurveyMapManager::setFocus);
    mMapTexture->eventMouseLostFocus += MyGUI::newDelegate(this, &SurveyMapManager::lostFocus);
    mMapTexture->eventMouseMove += MyGUI::newDelegate(this, &SurveyMapManager::mouseMove);
    mMapTexture->eventMouseButtonPressed += MyGUI::newDelegate(this, &SurveyMapManager::mousePressed);

    mCursorEntity = createMapEntity("other");
    mCursorEntity->setVisibility(false);
    mCursorEntity->setCaption(_L("Teleport"));
}

SurveyMapEntity *SurveyMapManager::createMapEntity(String type)
{
    auto entity = new SurveyMapEntity(type, mMapTexture);
    mMapEntities.insert(entity);
    return entity;
}

void SurveyMapManager::deleteMapEntity(SurveyMapEntity *entity)
{
    if (entity)
    {
        mMapEntities.erase(entity);
        delete entity;
    }
}

void SurveyMapManager::updateWindow()
{
    switch (mMapMode)
    {
    case SurveyMapMode::SMALL: setWindowPosition(1, -1, 0.30f); break;
    case SurveyMapMode::BIG: setWindowPosition(0, 0, 0.98f); break;
    default:;
    }
}

void SurveyMapManager::windowResized()
{
    this->updateWindow();
}

void SurveyMapManager::setMapZoom(Real zoom)
{
    zoom = Math::Clamp(zoom, 0.0f, std::max(0.0f, (mTerrainSize.x - 50.0f) / mTerrainSize.x));

    if (mMapZoom == zoom) return;

    mMapZoom = zoom;
    updateMap();
}

void SurveyMapManager::setMapZoomRelative(Real delta)
{
    setMapZoom(mMapZoom + 0.5f * delta * (1.0f - mMapZoom));
}

void SurveyMapManager::updateMap()
{
    mMapSize = mTerrainSize * (1.0f - mMapZoom);

    mMapCenter.x = Math::Clamp(mPlayerPosition.x - mMapCenterOffset.x, mMapSize.x / 2, mTerrainSize.x - mMapSize.x / 2);
    mMapCenter.y = Math::Clamp(mPlayerPosition.y - mMapCenterOffset.y, mMapSize.y / 2, mTerrainSize.y - mMapSize.y / 2);

    if (mMapMode == SurveyMapMode::SMALL) { mMapTextureCreatorDynamic->update(mMapCenter + mMapCenterOffset, mMapSize); }
}

void SurveyMapManager::setPlayerPosition(Ogre::Vector2 position)
{
    if ((mPlayerPosition - position).isZeroLength()) return;

    mPlayerPosition = position;
    updateMap();
}

void SurveyMapManager::setWindowPosition(int x, int y, float size)
{
    int          rWinLeft, rWinTop;
    unsigned int rWinWidth, rWinHeight, rWinDepth;
    App::GetOgreSubsystem()->GetRenderWindow()->getMetrics(rWinWidth, rWinHeight, rWinDepth, rWinLeft, rWinTop);

    int realx = 0;
    int realy = 0;
    int realw = size * std::min(rWinWidth, rWinHeight) * mMapSize.x / std::max(mMapSize.x, mMapSize.y);
    int realh = size * std::min(rWinWidth, rWinHeight) * mMapSize.y / std::max(mMapSize.x, mMapSize.y);

    if (x == -1) { realx = 0; }
    else if (x == 0)
    {
        realx = (rWinWidth - realw) / 2;
    }
    else if (x == 1)
    {
        realx = rWinWidth - realw;
    }

    if (y == -1) { realy = 0; }
    else if (y == 0)
    {
        realy = (rWinHeight - realh) / 2;
    }
    else if (y == 1)
    {
        realy = rWinHeight - realh;
    }

    mMainWidget->setCoord(realx, realy, realw, realh);
}

Ogre::String SurveyMapManager::getTypeByDriveable(int driveable)
{
    switch (driveable)
    {
    case NOT_DRIVEABLE: return "load";
    case TRUCK: return "truck";
    case AIRPLANE: return "airplane";
    case BOAT: return "boat";
    case MACHINE: return "machine";
    default: return "unknown";
    }
}

void SurveyMapManager::Update(Ogre::Real dt, Actor *curr_truck)
{
    if (App::GetInputEngine()->getEventBoolValueBounce(EV_SURVEY_MAP_CYCLE))
    {
        mHidden = false;
        cycleMode();
    }

    if (App::GetInputEngine()->getEventBoolValueBounce(EV_SURVEY_MAP_TOGGLE))
    {
        mHidden = false;
        toggleMode();
    }

    mMainWidget->setVisible(!mHidden && mMapMode != SurveyMapMode::NONE);

    if (mHidden || mMapMode == SurveyMapMode::NONE) return;

    if (App::GetInputEngine()->getEventBoolValueBounce(EV_SURVEY_MAP_TOGGLE_ICONS))
        App::gfx_surveymap_icons.SetActive(!App::gfx_surveymap_icons.GetActive());

    switch (mMapMode)
    {
    case SurveyMapMode::SMALL:
        if (mMapLastZoom > 0.0f)
        {
            setMapZoom(mMapLastZoom);
            mMapLastZoom = 0.0f;
        }
        if (App::GetInputEngine()->getEventBoolValue(EV_SURVEY_MAP_ZOOM_IN)) { setMapZoomRelative(+dt); }
        if (App::GetInputEngine()->getEventBoolValue(EV_SURVEY_MAP_ZOOM_OUT)) { setMapZoomRelative(-dt); }
        if (mMapZoom > 0.0f)
        {
            mMapTexture->setImageTexture(mMapTextureCreatorDynamic->getTextureName());
            if (curr_truck)
            {
                auto &simbuf = curr_truck->GetGfxActor()->GetSimDataBuffer();
                setPlayerPosition(Vector2(simbuf.simbuf_pos.x, simbuf.simbuf_pos.z));
            }
            else
            {
                auto &simbuf = App::GetSimController()->GetGfxScene().GetSimDataBuffer();
                setPlayerPosition(Vector2(simbuf.simbuf_character_pos.x, simbuf.simbuf_character_pos.z));
            }
            mMapLastZoom = mMapZoom;
        }
        else
        {
            mMapTexture->setImageTexture(mMapTextureCreatorStatic->getTextureName());
            setPlayerPosition(mTerrainSize / 2);
        }
        break;

    case SurveyMapMode::BIG:
        if (App::GetSimController()->AreControlsLocked() || App::GetGuiManager()->GetMainSelector()->IsVisible())
        {
            toggleMode();
            mMainWidget->setVisible(!mHidden && mMapMode != SurveyMapMode::NONE);
        }
        else
        {
            mMapTexture->setImageTexture(mMapTextureCreatorStatic->getTextureName());
            setPlayerPosition(mTerrainSize / 2);
            setMapZoom(0.0f);
        }
        break;

    default: break;
    }
}

void SurveyMapManager::cycleMode()
{
    switch (mMapMode)
    {
    case SurveyMapMode::NONE: mMapLastMode = mMapMode = SurveyMapMode::SMALL; break;
    case SurveyMapMode::SMALL: mMapLastMode = mMapMode = SurveyMapMode::BIG; break;
    case SurveyMapMode::BIG: mMapMode = SurveyMapMode::NONE; break;
    default:;
    }
    updateWindow();
}

void SurveyMapManager::toggleMode()
{
    mMapMode = (mMapMode == SurveyMapMode::NONE) ? mMapLastMode : SurveyMapMode::NONE;
    updateWindow();
}

void SurveyMapManager::UpdateMapEntity(SurveyMapEntity *e, String caption, Vector3 pos, float rot, int state, bool visible)
{
    if (e)
    {
        Vector2 origin    = mMapCenter + mMapCenterOffset - mMapSize / 2;
        Vector2 relPos    = Vector2(pos.x, pos.z) - origin;
        bool    culled    = !(Vector2(0) < relPos && relPos < mMapSize);
        bool    declutter = App::gfx_declutter_map.GetActive() && mMapMode == SurveyMapMode::SMALL && !e->isPlayable();
        e->setState(state);
        e->setRotation(rot);
        e->setCaption(declutter ? "" : caption);
        e->setPosition(relPos.x / mMapSize.x, relPos.y / mMapSize.y);
        e->setVisibility(App::gfx_surveymap_icons.GetActive() && visible && !culled);
    }
}

void SurveyMapManager::setFocus(MyGUI::Widget *_sender, MyGUI::Widget *_new)
{
    mCursorEntity->setVisibility(true);
}

void SurveyMapManager::lostFocus(MyGUI::Widget *_sender, MyGUI::Widget *_old)
{
    mCursorEntity->setVisibility(false);
}

void SurveyMapManager::mouseMove(MyGUI::Widget *_sender, int _left, int _top)
{
    float left = (float)(_left - mMapTexture->getAbsoluteLeft()) / (float)mMapTexture->getWidth();
    float top  = (float)(_top - mMapTexture->getAbsoluteTop()) / (float)mMapTexture->getHeight();

    mCursorEntity->setPosition(left, top);

    if (App::sim_state.GetActive() == SimState::RUNNING)
    { RoR::App::GetGuiManager()->SetMouseCursorVisibility(RoR::GUIManager::MouseCursorVisibility::HIDDEN); }
}

void SurveyMapManager::mousePressed(MyGUI::Widget *_sender, int _left, int _top, MyGUI::MouseButton _id)
{
    float left = (float)(_left - mMapTexture->getAbsoluteLeft()) / (float)mMapTexture->getWidth();
    float top  = (float)(_top - mMapTexture->getAbsoluteTop()) / (float)mMapTexture->getHeight();

    Vector2 origin = mMapCenter + mMapCenterOffset - mMapSize / 2;
    Vector2 pos    = origin + Vector2(left, top) * mMapSize;

    App::GetSimController()->TeleportPlayerXZ(pos.x, pos.y);
}
