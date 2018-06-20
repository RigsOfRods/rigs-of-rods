/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2013-2017 Petr Ohlidal & contributors

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

/// @file   OverlayWrapper.cpp
/// @author Thomas Fischer
/// @date   6th of May 2010

#include <Ogre.h>
#include <Overlay/OgreOverlayManager.h>
#include <Overlay/OgreOverlay.h>
#include <Overlay/OgreFontManager.h>
#include "OverlayWrapper.h"

#include "AeroEngine.h"
#include "Application.h"
#include "AutoPilot.h"
#include "BeamFactory.h"
#include "Character.h"
#include "DashBoardManager.h"
#include "BeamEngine.h"
#include "ErrorUtils.h"
#include "FlexAirfoil.h"
#include "GlobalEnvironment.h"
#include "Language.h"
#include "OgreSubsystem.h"
#include "RoRFrameListener.h"
#include "RoRVersion.h"
#include "ScrewProp.h"
#include "SoundScriptManager.h"
#include "TerrainManager.h"
#include "TurboProp.h"
#include "Utils.h"

using namespace Ogre;

bool g_is_scaled = false;

OverlayWrapper::OverlayWrapper():
    m_direction_arrow_node(nullptr),
    mTimeUntilNextToggle(0),
    m_visible_overlays(0)
{
    win = RoR::App::GetOgreSubsystem()->GetRenderWindow();
    init();
}

OverlayWrapper::~OverlayWrapper()
{
    showDashboardOverlays(false, nullptr);
    HideRacingOverlay();
    HideDirectionOverlay();
}

void OverlayWrapper::resizePanel(OverlayElement* oe)
{
    if (g_is_scaled)
        return;
    oe->setHeight(oe->getHeight() * (Real)win->getWidth() / (Real)win->getHeight());
    oe->setTop(oe->getTop() * (Real)win->getWidth() / (Real)win->getHeight());
}

void OverlayWrapper::reposPanel(OverlayElement* oe)
{
    if (g_is_scaled)
        return;
    oe->setTop(oe->getTop() * (Real)win->getWidth() / (Real)win->getHeight());
}

void OverlayWrapper::placeNeedle(SceneNode* node, float x, float y, float len)
{/**(Real)win->getHeight()/(Real)win->getWidth()*/
    node->setPosition((x - 640.0) / 444.0, (512 - y) / 444.0, -2.0);
    node->setScale(0.0025, 0.007 * len, 0.007);
}

Overlay* OverlayWrapper::loadOverlay(String name, bool autoResizeRation)
{
    Overlay* o = OverlayManager::getSingleton().getByName(name);
    if (!o)
        return NULL;

    if (autoResizeRation)
    {
        struct LoadedOverlay lo;
        lo.o = o;
        lo.orgScaleX = o->getScaleX();
        lo.orgScaleY = o->getScaleY();

        m_loaded_overlays.push_back(lo);
        resizeOverlay(lo);
    }
    return o;
}

void OverlayWrapper::resizeOverlay(LoadedOverlay& lo)
{
    // enforce 4:3 for overlays
    float w = win->getWidth();
    float h = win->getHeight();
    float s = (4.0f / 3.0f) / (w / h);

    // window is higher than wide
    if (s > 1)
        s = (3.0f / 4.0f) / (h / w);

    // originals
    lo.o->setScale(lo.orgScaleX, lo.orgScaleY);
    lo.o->setScroll(0, 0);

    // now the new values
    lo.o->setScale(s, s);
    lo.o->scroll(1 - s, s - 1);
}

void OverlayWrapper::windowResized()
{
    for (auto it = m_loaded_overlays.begin(); it != m_loaded_overlays.end(); it++)
    {
        resizeOverlay(*it);
    }
}

OverlayElement* OverlayWrapper::loadOverlayElement(String name)
{
    return OverlayManager::getSingleton().getOverlayElement(name);
}

int OverlayWrapper::init()
{
    m_direction_arrow_overlay = loadOverlay("tracks/DirectionArrow", false);
    try
    {
        directionArrowText = (TextAreaOverlayElement*)loadOverlayElement("tracks/DirectionArrow/Text");
    }
    catch (...)
    {
        ErrorUtils::ShowError("Resources not found!", "please ensure that your installation is complete and the resources are installed properly. If this error persists please re-install RoR.");
    }
    directionArrowDistance = (TextAreaOverlayElement*)loadOverlayElement("tracks/DirectionArrow/Distance");

    // openGL fix
    m_direction_arrow_overlay->show();
    m_direction_arrow_overlay->hide();

    m_debug_fps_memory_overlay = loadOverlay("Core/DebugOverlay", false);
    m_debug_beam_timing_overlay = loadOverlay("tracks/DebugBeamTiming", false);
    m_debug_beam_timing_overlay->hide();

    OverlayElement* vere = loadOverlayElement("Core/RoRVersionString");
    if (vere)
        vere->setCaption("Rigs of Rods version " + String(ROR_VERSION_STRING));

    m_machine_dashboard_overlay = loadOverlay("tracks/MachineDashboardOverlay");
    m_aerial_dashboard_overlay = loadOverlay("tracks/AirDashboardOverlay", false);
    m_aerial_dashboard_needles_overlay = loadOverlay("tracks/AirNeedlesOverlay", false);
    m_marine_dashboard_overlay = loadOverlay("tracks/BoatDashboardOverlay");
    m_marine_dashboard_needles_overlay = loadOverlay("tracks/BoatNeedlesOverlay");


    //adjust dashboard size for screen ratio
    resizePanel(loadOverlayElement("tracks/pressureo"));
    resizePanel(loadOverlayElement("tracks/pressureneedle"));
    MaterialPtr m = MaterialManager::getSingleton().getByName("tracks/pressureneedle_mat");
    if (!m.isNull())
        pressuretexture = m->getTechnique(0)->getPass(0)->getTextureUnitState(0);

    resizePanel(loadOverlayElement("tracks/machineinstructions"));
    resizePanel(loadOverlayElement("tracks/machinehelppanel"));

    resizePanel(OverlayManager::getSingleton().getOverlayElement("tracks/machinedashbar"));
    resizePanel(OverlayManager::getSingleton().getOverlayElement("tracks/machinedashfiller"));

    resizePanel(OverlayManager::getSingleton().getOverlayElement("tracks/airdashbar"));
    resizePanel(OverlayManager::getSingleton().getOverlayElement("tracks/airdashfiller"));

    OverlayElement* tempoe;
    resizePanel(tempoe = OverlayManager::getSingleton().getOverlayElement("tracks/thrusttrack1"));

    resizePanel(OverlayManager::getSingleton().getOverlayElement("tracks/thrusttrack2"));
    resizePanel(OverlayManager::getSingleton().getOverlayElement("tracks/thrusttrack3"));
    resizePanel(OverlayManager::getSingleton().getOverlayElement("tracks/thrusttrack4"));

    resizePanel(thro1 = loadOverlayElement("tracks/thrust1"));
    resizePanel(thro2 = loadOverlayElement("tracks/thrust2"));
    resizePanel(thro3 = loadOverlayElement("tracks/thrust3"));
    resizePanel(thro4 = loadOverlayElement("tracks/thrust4"));

    thrtop = 1.0f + tempoe->getTop() + thro1->getHeight() * 0.5f;
    thrheight = tempoe->getHeight() - thro1->getHeight() * 2.0f;
    throffset = thro1->getHeight() * 0.5f;

    engfireo1 = loadOverlayElement("tracks/engfire1");
    engfireo2 = loadOverlayElement("tracks/engfire2");
    engfireo3 = loadOverlayElement("tracks/engfire3");
    engfireo4 = loadOverlayElement("tracks/engfire4");
    engstarto1 = loadOverlayElement("tracks/engstart1");
    engstarto2 = loadOverlayElement("tracks/engstart2");
    engstarto3 = loadOverlayElement("tracks/engstart3");
    engstarto4 = loadOverlayElement("tracks/engstart4");
    resizePanel(loadOverlayElement("tracks/airrpm1"));
    resizePanel(loadOverlayElement("tracks/airrpm2"));
    resizePanel(loadOverlayElement("tracks/airrpm3"));
    resizePanel(loadOverlayElement("tracks/airrpm4"));
    resizePanel(loadOverlayElement("tracks/airpitch1"));
    resizePanel(loadOverlayElement("tracks/airpitch2"));
    resizePanel(loadOverlayElement("tracks/airpitch3"));
    resizePanel(loadOverlayElement("tracks/airpitch4"));
    resizePanel(loadOverlayElement("tracks/airtorque1"));
    resizePanel(loadOverlayElement("tracks/airtorque2"));
    resizePanel(loadOverlayElement("tracks/airtorque3"));
    resizePanel(loadOverlayElement("tracks/airtorque4"));
    resizePanel(loadOverlayElement("tracks/airspeed"));
    resizePanel(loadOverlayElement("tracks/vvi"));
    resizePanel(loadOverlayElement("tracks/altimeter"));
    resizePanel(loadOverlayElement("tracks/altimeter_val"));
    alt_value_taoe = (TextAreaOverlayElement*)loadOverlayElement("tracks/altimeter_val");
    boat_depth_value_taoe = (TextAreaOverlayElement*)loadOverlayElement("tracks/boatdepthmeter_val");
    resizePanel(loadOverlayElement("tracks/adi-tape"));
    resizePanel(loadOverlayElement("tracks/adi"));
    resizePanel(loadOverlayElement("tracks/adi-bugs"));
    adibugstexture = ((MaterialPtr)(MaterialManager::getSingleton().getByName("tracks/adi-bugs")))->getTechnique(0)->getPass(0)->getTextureUnitState(0);
    aditapetexture = ((MaterialPtr)(MaterialManager::getSingleton().getByName("tracks/adi-tape")))->getTechnique(0)->getPass(0)->getTextureUnitState(0);
    resizePanel(loadOverlayElement("tracks/aoa"));
    resizePanel(loadOverlayElement("tracks/hsi"));
    resizePanel(loadOverlayElement("tracks/hsi-rose"));
    resizePanel(loadOverlayElement("tracks/hsi-bug"));
    resizePanel(loadOverlayElement("tracks/hsi-v"));
    resizePanel(loadOverlayElement("tracks/hsi-h"));
    hsirosetexture = ((MaterialPtr)(MaterialManager::getSingleton().getByName("tracks/hsi-rose")))->getTechnique(0)->getPass(0)->getTextureUnitState(0);
    hsibugtexture = ((MaterialPtr)(MaterialManager::getSingleton().getByName("tracks/hsi-bug")))->getTechnique(0)->getPass(0)->getTextureUnitState(0);
    hsivtexture = ((MaterialPtr)(MaterialManager::getSingleton().getByName("tracks/hsi-v")))->getTechnique(0)->getPass(0)->getTextureUnitState(0);
    hsihtexture = ((MaterialPtr)(MaterialManager::getSingleton().getByName("tracks/hsi-h")))->getTechnique(0)->getPass(0)->getTextureUnitState(0);
    //autopilot
    reposPanel(loadOverlayElement("tracks/ap_hdg_pack"));
    reposPanel(loadOverlayElement("tracks/ap_wlv_but"));
    reposPanel(loadOverlayElement("tracks/ap_nav_but"));
    reposPanel(loadOverlayElement("tracks/ap_alt_pack"));
    reposPanel(loadOverlayElement("tracks/ap_vs_pack"));
    reposPanel(loadOverlayElement("tracks/ap_ias_pack"));
    reposPanel(loadOverlayElement("tracks/ap_gpws_but"));
    reposPanel(loadOverlayElement("tracks/ap_brks_but"));

    //boat
    resizePanel(loadOverlayElement("tracks/boatdashbar"));
    resizePanel(loadOverlayElement("tracks/boatdashfiller"));
    resizePanel(loadOverlayElement("tracks/boatthrusttrack1"));
    resizePanel(loadOverlayElement("tracks/boatthrusttrack2"));

    //resizePanel(boatmapo=loadOverlayElement("tracks/boatmap"));
    //resizePanel(boatmapdot=loadOverlayElement("tracks/boatreddot"));

    resizePanel(bthro1 = loadOverlayElement("tracks/boatthrust1"));
    resizePanel(bthro2 = loadOverlayElement("tracks/boatthrust2"));

    resizePanel(loadOverlayElement("tracks/boatspeed"));
    resizePanel(loadOverlayElement("tracks/boatsteer"));
    resizePanel(loadOverlayElement("tracks/boatspeedneedle"));
    resizePanel(loadOverlayElement("tracks/boatsteer/fg"));
    boatspeedtexture = ((MaterialPtr)(MaterialManager::getSingleton().getByName("tracks/boatspeedneedle_mat")))->getTechnique(0)->getPass(0)->getTextureUnitState(0);
    boatsteertexture = ((MaterialPtr)(MaterialManager::getSingleton().getByName("tracks/boatsteer/fg_mat")))->getTechnique(0)->getPass(0)->getTextureUnitState(0);

    //prepare needles
    speedotexture = ((MaterialPtr)(MaterialManager::getSingleton().getByName("tracks/speedoneedle_mat")))->getTechnique(0)->getPass(0)->getTextureUnitState(0); // Needed for dashboard-prop
    tachotexture  = ((MaterialPtr)(MaterialManager::getSingleton().getByName("tracks/tachoneedle_mat"))) ->getTechnique(0)->getPass(0)->getTextureUnitState(0); // Needed for dashboard-prop

    resizePanel(loadOverlayElement("tracks/airspeedneedle"));
    airspeedtexture = ((MaterialPtr)(MaterialManager::getSingleton().getByName("tracks/airspeedneedle_mat")))->getTechnique(0)->getPass(0)->getTextureUnitState(0);

    resizePanel(loadOverlayElement("tracks/altimeterneedle"));
    altimetertexture = ((MaterialPtr)(MaterialManager::getSingleton().getByName("tracks/altimeterneedle_mat")))->getTechnique(0)->getPass(0)->getTextureUnitState(0);

    resizePanel(loadOverlayElement("tracks/vvineedle"));
    vvitexture = ((MaterialPtr)(MaterialManager::getSingleton().getByName("tracks/vvineedle_mat")))->getTechnique(0)->getPass(0)->getTextureUnitState(0);

    resizePanel(loadOverlayElement("tracks/aoaneedle"));
    aoatexture = ((MaterialPtr)(MaterialManager::getSingleton().getByName("tracks/aoaneedle_mat")))->getTechnique(0)->getPass(0)->getTextureUnitState(0);

    resizePanel(loadOverlayElement("tracks/airrpm1needle"));
    airrpm1texture = ((MaterialPtr)(MaterialManager::getSingleton().getByName("tracks/airrpm1needle_mat")))->getTechnique(0)->getPass(0)->getTextureUnitState(0);
    resizePanel(loadOverlayElement("tracks/airrpm2needle"));
    airrpm2texture = ((MaterialPtr)(MaterialManager::getSingleton().getByName("tracks/airrpm2needle_mat")))->getTechnique(0)->getPass(0)->getTextureUnitState(0);
    resizePanel(loadOverlayElement("tracks/airrpm3needle"));
    airrpm3texture = ((MaterialPtr)(MaterialManager::getSingleton().getByName("tracks/airrpm3needle_mat")))->getTechnique(0)->getPass(0)->getTextureUnitState(0);
    resizePanel(loadOverlayElement("tracks/airrpm4needle"));
    airrpm4texture = ((MaterialPtr)(MaterialManager::getSingleton().getByName("tracks/airrpm4needle_mat")))->getTechnique(0)->getPass(0)->getTextureUnitState(0);

    resizePanel(loadOverlayElement("tracks/airpitch1needle"));
    airpitch1texture = ((MaterialPtr)(MaterialManager::getSingleton().getByName("tracks/airpitch1needle_mat")))->getTechnique(0)->getPass(0)->getTextureUnitState(0);
    resizePanel(loadOverlayElement("tracks/airpitch2needle"));
    airpitch2texture = ((MaterialPtr)(MaterialManager::getSingleton().getByName("tracks/airpitch2needle_mat")))->getTechnique(0)->getPass(0)->getTextureUnitState(0);
    resizePanel(loadOverlayElement("tracks/airpitch3needle"));
    airpitch3texture = ((MaterialPtr)(MaterialManager::getSingleton().getByName("tracks/airpitch3needle_mat")))->getTechnique(0)->getPass(0)->getTextureUnitState(0);
    resizePanel(loadOverlayElement("tracks/airpitch4needle"));
    airpitch4texture = ((MaterialPtr)(MaterialManager::getSingleton().getByName("tracks/airpitch4needle_mat")))->getTechnique(0)->getPass(0)->getTextureUnitState(0);

    resizePanel(loadOverlayElement("tracks/airtorque1needle"));
    airtorque1texture = ((MaterialPtr)(MaterialManager::getSingleton().getByName("tracks/airtorque1needle_mat")))->getTechnique(0)->getPass(0)->getTextureUnitState(0);
    resizePanel(loadOverlayElement("tracks/airtorque2needle"));
    airtorque2texture = ((MaterialPtr)(MaterialManager::getSingleton().getByName("tracks/airtorque2needle_mat")))->getTechnique(0)->getPass(0)->getTextureUnitState(0);
    resizePanel(loadOverlayElement("tracks/airtorque3needle"));
    airtorque3texture = ((MaterialPtr)(MaterialManager::getSingleton().getByName("tracks/airtorque3needle_mat")))->getTechnique(0)->getPass(0)->getTextureUnitState(0);
    resizePanel(loadOverlayElement("tracks/airtorque4needle"));
    airtorque4texture = ((MaterialPtr)(MaterialManager::getSingleton().getByName("tracks/airtorque4needle_mat")))->getTechnique(0)->getPass(0)->getTextureUnitState(0);

    guiGear = loadOverlayElement("tracks/Gear");
    guiGear3D = loadOverlayElement("tracks/3DGear");

    guiAuto[0] = (TextAreaOverlayElement*)loadOverlayElement("tracks/AGearR");
    guiAuto[1] = (TextAreaOverlayElement*)loadOverlayElement("tracks/AGearN");
    guiAuto[2] = (TextAreaOverlayElement*)loadOverlayElement("tracks/AGearD");
    guiAuto[3] = (TextAreaOverlayElement*)loadOverlayElement("tracks/AGear2");
    guiAuto[4] = (TextAreaOverlayElement*)loadOverlayElement("tracks/AGear1");

    guiAuto3D[0] = (TextAreaOverlayElement*)loadOverlayElement("tracks/3DAGearR");
    guiAuto3D[1] = (TextAreaOverlayElement*)loadOverlayElement("tracks/3DAGearN");
    guiAuto3D[2] = (TextAreaOverlayElement*)loadOverlayElement("tracks/3DAGearD");
    guiAuto3D[3] = (TextAreaOverlayElement*)loadOverlayElement("tracks/3DAGear2");
    guiAuto3D[4] = (TextAreaOverlayElement*)loadOverlayElement("tracks/3DAGear1");

    m_truck_pressure_overlay = loadOverlay("tracks/PressureOverlay");
    m_truck_pressure_needle_overlay = loadOverlay("tracks/PressureNeedleOverlay");

    m_racing_overlay = loadOverlay("tracks/Racing", false);
    laptimemin = (TextAreaOverlayElement*)loadOverlayElement("tracks/LapTimemin");
    laptimes = (TextAreaOverlayElement*)loadOverlayElement("tracks/LapTimes");
    laptimems = (TextAreaOverlayElement*)loadOverlayElement("tracks/LapTimems");
    lasttime = (TextAreaOverlayElement*)loadOverlayElement("tracks/LastTime");
    lasttime->setCaption("");

    // openGL fix
    m_racing_overlay->show();
    m_racing_overlay->hide();

    g_is_scaled = true;

    return 0;
}

void OverlayWrapper::update(float dt)
{
    if (mTimeUntilNextToggle > 0)
        mTimeUntilNextToggle -= dt;
}

void OverlayWrapper::showDebugOverlay(int mode)
{
    if (!m_debug_fps_memory_overlay || !m_debug_beam_timing_overlay)
        return;

    if (mode > 0)
    {
        m_debug_fps_memory_overlay->show();
        BITMASK_SET_1(m_visible_overlays, VisibleOverlays::DEBUG_FPS_MEMORY);

        if (mode > 1)
        {
            BITMASK_SET_1(m_visible_overlays, VisibleOverlays::DEBUG_BEAM_TIMING);
            m_debug_beam_timing_overlay->show();
        }
        else
        {
            BITMASK_SET_0(m_visible_overlays, VisibleOverlays::DEBUG_BEAM_TIMING);
            m_debug_beam_timing_overlay->hide();
        }
    }
    else
    {
        m_debug_fps_memory_overlay->hide();
        BITMASK_SET_0(m_visible_overlays, VisibleOverlays::DEBUG_FPS_MEMORY);

        m_debug_beam_timing_overlay->hide();
        BITMASK_SET_0(m_visible_overlays, VisibleOverlays::DEBUG_BEAM_TIMING);
    }
}

void OverlayWrapper::showPressureOverlay(bool show)
{
    if (m_truck_pressure_overlay)
    {
        if (show)
        {
            m_truck_pressure_overlay->show();
            m_truck_pressure_needle_overlay->show();
            BITMASK_SET_1(m_visible_overlays, VisibleOverlays::TRUCK_TIRE_PRESSURE_OVERLAY);
        }
        else
        {
            m_truck_pressure_overlay->hide();
            m_truck_pressure_needle_overlay->hide();
            BITMASK_SET_0(m_visible_overlays, VisibleOverlays::TRUCK_TIRE_PRESSURE_OVERLAY);
        }
    }
}

void OverlayWrapper::showDashboardOverlays(bool show, Actor* actor)
{
    // check if we use the new style dashboards
    if (actor && actor->ar_dashboard && actor->ar_dashboard->WasDashboardLoaded())
    {
        actor->ar_dashboard->setVisible(show);
        return;
    }

    if (show)
    {
        int mode = actor ? actor->ar_driveable : -1;

        if (mode == AIRPLANE)
        {
            m_aerial_dashboard_needles_overlay->show();
            m_aerial_dashboard_overlay->show();
        }
        else if (mode == BOAT)
        {
            m_marine_dashboard_needles_overlay->show();
            m_marine_dashboard_overlay->show();
        }
        else if (mode == MACHINE)
        {
            m_machine_dashboard_overlay->show();
        }
    }
    else
    {

        m_aerial_dashboard_needles_overlay->hide();
        m_aerial_dashboard_overlay->hide();

        m_marine_dashboard_needles_overlay->hide();
        m_marine_dashboard_overlay->hide();

        m_machine_dashboard_overlay->hide();
    }
}

void OverlayWrapper::updateStats(bool detailed)
{
    static UTFString currFps = _L("Current FPS: ");
    static UTFString avgFps = _L("Average FPS: ");
    static UTFString bestFps = _L("Best FPS: ");
    static UTFString worstFps = _L("Worst FPS: ");
    static UTFString tris = _L("Triangle Count: ");
    const RenderTarget::FrameStats& stats = win->getStatistics();

    // update stats when necessary
    try
    {
        OverlayElement* guiAvg = OverlayManager::getSingleton().getOverlayElement("Core/AverageFps");
        OverlayElement* guiCurr = OverlayManager::getSingleton().getOverlayElement("Core/CurrFps");
        OverlayElement* guiBest = OverlayManager::getSingleton().getOverlayElement("Core/BestFps");
        OverlayElement* guiWorst = OverlayManager::getSingleton().getOverlayElement("Core/WorstFps");

        guiAvg->setCaption(avgFps + TOUTFSTRING(stats.avgFPS));
        guiCurr->setCaption(currFps + TOUTFSTRING(stats.lastFPS));
        guiBest->setCaption(bestFps + TOUTFSTRING(stats.bestFPS) + U(" ") + TOUTFSTRING(stats.bestFrameTime) + U(" ms"));
        guiWorst->setCaption(worstFps + TOUTFSTRING(stats.worstFPS) + U(" ") + TOUTFSTRING(stats.worstFrameTime) + U(" ms"));

        OverlayElement* guiTris = OverlayManager::getSingleton().getOverlayElement("Core/NumTris");
        UTFString triss = tris + TOUTFSTRING(stats.triangleCount);
        if (stats.triangleCount > 1000000)
            triss = tris + TOUTFSTRING(stats.triangleCount/1000000.0f) + U(" M");
        else if (stats.triangleCount > 1000)
            triss = tris + TOUTFSTRING(stats.triangleCount/1000.0f) + U(" k");
        guiTris->setCaption(triss);

        // create some memory texts
        UTFString memoryText;
        if (TextureManager::getSingleton().getMemoryUsage() > 1)
            memoryText = memoryText + _L("Textures: ") + formatBytes(TextureManager::getSingleton().getMemoryUsage()) + U(" / ") + formatBytes(TextureManager::getSingleton().getMemoryBudget()) + U("\n");
        if (CompositorManager::getSingleton().getMemoryUsage() > 1)
            memoryText = memoryText + _L("Compositors: ") + formatBytes(CompositorManager::getSingleton().getMemoryUsage()) + U(" / ") + formatBytes(CompositorManager::getSingleton().getMemoryBudget()) + U("\n");
        if (FontManager::getSingleton().getMemoryUsage() > 1)
            memoryText = memoryText + _L("Fonts: ") + formatBytes(FontManager::getSingleton().getMemoryUsage()) + U(" / ") + formatBytes(FontManager::getSingleton().getMemoryBudget()) + U("\n");
        if (GpuProgramManager::getSingleton().getMemoryUsage() > 1)
            memoryText = memoryText + _L("GPU Program: ") + formatBytes(GpuProgramManager::getSingleton().getMemoryUsage()) + U(" / ") + formatBytes(GpuProgramManager::getSingleton().getMemoryBudget()) + U("\n");
        if (HighLevelGpuProgramManager::getSingleton().getMemoryUsage() > 1)
            memoryText = memoryText + _L("HL GPU Program: ") + formatBytes(HighLevelGpuProgramManager::getSingleton().getMemoryUsage()) + U(" / ") + formatBytes(HighLevelGpuProgramManager::getSingleton().getMemoryBudget()) + U("\n");
        if (MaterialManager::getSingleton().getMemoryUsage() > 1)
            memoryText = memoryText + _L("Materials: ") + formatBytes(MaterialManager::getSingleton().getMemoryUsage()) + U(" / ") + formatBytes(MaterialManager::getSingleton().getMemoryBudget()) + U("\n");
        if (MeshManager::getSingleton().getMemoryUsage() > 1)
            memoryText = memoryText + _L("Meshes: ") + formatBytes(MeshManager::getSingleton().getMemoryUsage()) + U(" / ") + formatBytes(MeshManager::getSingleton().getMemoryBudget()) + U("\n");
        if (SkeletonManager::getSingleton().getMemoryUsage() > 1)
            memoryText = memoryText + _L("Skeletons: ") + formatBytes(SkeletonManager::getSingleton().getMemoryUsage()) + U(" / ") + formatBytes(SkeletonManager::getSingleton().getMemoryBudget()) + U("\n");
        if (MaterialManager::getSingleton().getMemoryUsage() > 1)
            memoryText = memoryText + _L("Materials: ") + formatBytes(MaterialManager::getSingleton().getMemoryUsage()) + U(" / ") + formatBytes(MaterialManager::getSingleton().getMemoryBudget()) + U("\n");
        memoryText = memoryText + U("\n");

        OverlayElement* memoryDbg = OverlayManager::getSingleton().getOverlayElement("Core/MemoryText");
        memoryDbg->setCaption(memoryText);

        float sumMem = TextureManager::getSingleton().getMemoryUsage() + CompositorManager::getSingleton().getMemoryUsage() + FontManager::getSingleton().getMemoryUsage() + GpuProgramManager::getSingleton().getMemoryUsage() + HighLevelGpuProgramManager::getSingleton().getMemoryUsage() + MaterialManager::getSingleton().getMemoryUsage() + MeshManager::getSingleton().getMemoryUsage() + SkeletonManager::getSingleton().getMemoryUsage() + MaterialManager::getSingleton().getMemoryUsage();
        String sumMemoryText = _L("Memory (Ogre): ") + formatBytes(sumMem) + U("\n");

        OverlayElement* memorySumDbg = OverlayManager::getSingleton().getOverlayElement("Core/CurrMemory");
        memorySumDbg->setCaption(sumMemoryText);
    }
    catch (...)
    {
        // ignore
    }
}

bool OverlayWrapper::mouseMoved(const OIS::MouseEvent& _arg)
{
    if (!m_aerial_dashboard_needles_overlay->isVisible())
        return false;
    bool res = false;
    const OIS::MouseState ms = _arg.state;
    
    Actor* player_actor = RoR::App::GetSimController()->GetPlayerActor();

    if (!player_actor)
        return res;

    float mouseX = ms.X.abs / (float)ms.width;
    float mouseY = ms.Y.abs / (float)ms.height;

    // TODO: fix: when the window is scaled, the findElementAt doesn not seem to pick up the correct element :-\

    if (player_actor->ar_driveable == AIRPLANE && ms.buttonDown(OIS::MB_Left))
    {
        OverlayElement* element = m_aerial_dashboard_needles_overlay->findElementAt(mouseX, mouseY);
        if (element)
        {
            res = true;
            char name[256];
            strcpy(name, element->getName().c_str());
            if (!strncmp(name, "tracks/thrust1", 14))
                player_actor->ar_aeroengines[0]->setThrottle(1.0f - ((mouseY - thrtop - throffset) / thrheight));
            if (!strncmp(name, "tracks/thrust2", 14) && player_actor->ar_num_aeroengines > 1)
                player_actor->ar_aeroengines[1]->setThrottle(1.0f - ((mouseY - thrtop - throffset) / thrheight));
            if (!strncmp(name, "tracks/thrust3", 14) && player_actor->ar_num_aeroengines > 2)
                player_actor->ar_aeroengines[2]->setThrottle(1.0f - ((mouseY - thrtop - throffset) / thrheight));
            if (!strncmp(name, "tracks/thrust4", 14) && player_actor->ar_num_aeroengines > 3)
                player_actor->ar_aeroengines[3]->setThrottle(1.0f - ((mouseY - thrtop - throffset) / thrheight));
        }
        //also for main dashboard
        OverlayElement* element2 = m_aerial_dashboard_overlay->findElementAt(mouseX, mouseY);
        if (element2)
        {
            res = true;
            char name[256];
            strcpy(name, element2->getName().c_str());
            //LogManager::getSingleton().logMessage("element "+element2->getName());
            if (!strncmp(name, "tracks/engstart1", 16))
                player_actor->ar_aeroengines[0]->flipStart();
            if (!strncmp(name, "tracks/engstart2", 16) && player_actor->ar_num_aeroengines > 1)
                player_actor->ar_aeroengines[1]->flipStart();
            if (!strncmp(name, "tracks/engstart3", 16) && player_actor->ar_num_aeroengines > 2)
                player_actor->ar_aeroengines[2]->flipStart();
            if (!strncmp(name, "tracks/engstart4", 16) && player_actor->ar_num_aeroengines > 3)
                player_actor->ar_aeroengines[3]->flipStart();
            //heading group
            if (!strcmp(name, "tracks/ap_hdg_but") && player_actor->ar_autopilot && mTimeUntilNextToggle <= 0)
            {
                mTimeUntilNextToggle = 0.2;
                if (player_actor->ar_autopilot->toggleHeading(Autopilot::HEADING_FIXED) == Autopilot::HEADING_FIXED)
                    OverlayManager::getSingleton().getOverlayElement("tracks/ap_hdg_but")->setMaterialName("tracks/hdg-on");
                else
                    OverlayManager::getSingleton().getOverlayElement("tracks/ap_hdg_but")->setMaterialName("tracks/hdg-off");
                OverlayManager::getSingleton().getOverlayElement("tracks/ap_wlv_but")->setMaterialName("tracks/wlv-off");
                OverlayManager::getSingleton().getOverlayElement("tracks/ap_nav_but")->setMaterialName("tracks/nav-off");
            }
            if (!strcmp(name, "tracks/ap_wlv_but") && player_actor->ar_autopilot && mTimeUntilNextToggle <= 0)
            {
                mTimeUntilNextToggle = 0.2;
                if (player_actor->ar_autopilot->toggleHeading(Autopilot::HEADING_WLV) == Autopilot::HEADING_WLV)
                    OverlayManager::getSingleton().getOverlayElement("tracks/ap_wlv_but")->setMaterialName("tracks/wlv-on");
                else
                    OverlayManager::getSingleton().getOverlayElement("tracks/ap_wlv_but")->setMaterialName("tracks/wlv-off");
                OverlayManager::getSingleton().getOverlayElement("tracks/ap_hdg_but")->setMaterialName("tracks/hdg-off");
                OverlayManager::getSingleton().getOverlayElement("tracks/ap_nav_but")->setMaterialName("tracks/nav-off");
            }
            if (!strcmp(name, "tracks/ap_nav_but") && player_actor->ar_autopilot && mTimeUntilNextToggle <= 0)
            {
                mTimeUntilNextToggle = 0.2;
                if (player_actor->ar_autopilot->toggleHeading(Autopilot::HEADING_NAV) == Autopilot::HEADING_NAV)
                    OverlayManager::getSingleton().getOverlayElement("tracks/ap_nav_but")->setMaterialName("tracks/nav-on");
                else
                    OverlayManager::getSingleton().getOverlayElement("tracks/ap_nav_but")->setMaterialName("tracks/nav-off");
                OverlayManager::getSingleton().getOverlayElement("tracks/ap_wlv_but")->setMaterialName("tracks/wlv-off");
                OverlayManager::getSingleton().getOverlayElement("tracks/ap_hdg_but")->setMaterialName("tracks/hdg-off");
            }
            //altitude group
            if (!strcmp(name, "tracks/ap_alt_but") && player_actor->ar_autopilot && mTimeUntilNextToggle <= 0)
            {
                mTimeUntilNextToggle = 0.2;
                if (player_actor->ar_autopilot->toggleAlt(Autopilot::ALT_FIXED) == Autopilot::ALT_FIXED)
                    OverlayManager::getSingleton().getOverlayElement("tracks/ap_alt_but")->setMaterialName("tracks/hold-on");
                else
                    OverlayManager::getSingleton().getOverlayElement("tracks/ap_alt_but")->setMaterialName("tracks/hold-off");
                OverlayManager::getSingleton().getOverlayElement("tracks/ap_vs_but")->setMaterialName("tracks/vs-off");
            }
            if (!strcmp(name, "tracks/ap_vs_but") && player_actor->ar_autopilot && mTimeUntilNextToggle <= 0)
            {
                mTimeUntilNextToggle = 0.2;
                if (player_actor->ar_autopilot->toggleAlt(Autopilot::ALT_VS) == Autopilot::ALT_VS)
                    OverlayManager::getSingleton().getOverlayElement("tracks/ap_vs_but")->setMaterialName("tracks/vs-on");
                else
                    OverlayManager::getSingleton().getOverlayElement("tracks/ap_vs_but")->setMaterialName("tracks/vs-off");
                OverlayManager::getSingleton().getOverlayElement("tracks/ap_alt_but")->setMaterialName("tracks/hold-off");
            }
            //IAS
            if (!strcmp(name, "tracks/ap_ias_but") && player_actor->ar_autopilot && mTimeUntilNextToggle <= 0)
            {
                mTimeUntilNextToggle = 0.2;
                if (player_actor->ar_autopilot->toggleIAS())
                    OverlayManager::getSingleton().getOverlayElement("tracks/ap_ias_but")->setMaterialName("tracks/athr-on");
                else
                    OverlayManager::getSingleton().getOverlayElement("tracks/ap_ias_but")->setMaterialName("tracks/athr-off");
            }
            //GPWS
            if (!strcmp(name, "tracks/ap_gpws_but") && player_actor->ar_autopilot && mTimeUntilNextToggle <= 0)
            {
                mTimeUntilNextToggle = 0.2;
                if (player_actor->ar_autopilot->toggleGPWS())
                    OverlayManager::getSingleton().getOverlayElement("tracks/ap_gpws_but")->setMaterialName("tracks/gpws-on");
                else
                    OverlayManager::getSingleton().getOverlayElement("tracks/ap_gpws_but")->setMaterialName("tracks/gpws-off");
            }
            //BRKS
            if (!strcmp(name, "tracks/ap_brks_but") && player_actor->ar_autopilot && mTimeUntilNextToggle <= 0)
            {
                player_actor->ToggleParkingBrake();
                if (player_actor->ar_parking_brake)
                    OverlayManager::getSingleton().getOverlayElement("tracks/ap_brks_but")->setMaterialName("tracks/brks-on");
                else
                    OverlayManager::getSingleton().getOverlayElement("tracks/ap_brks_but")->setMaterialName("tracks/brks-off");
                mTimeUntilNextToggle = 0.2;
            }
            //trims
            if (!strcmp(name, "tracks/ap_hdg_up") && player_actor->ar_autopilot && mTimeUntilNextToggle <= 0)
            {
                mTimeUntilNextToggle = 0.1;
                int val = player_actor->ar_autopilot->adjHDG(1);
                char str[10];
                sprintf(str, "%.3u", val);
                OverlayManager::getSingleton().getOverlayElement("tracks/ap_hdg_val")->setCaption(str);
            }
            if (!strcmp(name, "tracks/ap_hdg_dn") && player_actor->ar_autopilot && mTimeUntilNextToggle <= 0)
            {
                mTimeUntilNextToggle = 0.1;
                int val = player_actor->ar_autopilot->adjHDG(-1);
                char str[10];
                sprintf(str, "%.3u", val);
                OverlayManager::getSingleton().getOverlayElement("tracks/ap_hdg_val")->setCaption(str);
            }
            if (!strcmp(name, "tracks/ap_alt_up") && player_actor->ar_autopilot && mTimeUntilNextToggle <= 0)
            {
                mTimeUntilNextToggle = 0.1;
                int val = player_actor->ar_autopilot->adjALT(100);
                char str[10];
                sprintf(str, "%i00", val / 100);
                OverlayManager::getSingleton().getOverlayElement("tracks/ap_alt_val")->setCaption(str);
            }
            if (!strcmp(name, "tracks/ap_alt_dn") && player_actor->ar_autopilot && mTimeUntilNextToggle <= 0)
            {
                mTimeUntilNextToggle = 0.1;
                int val = player_actor->ar_autopilot->adjALT(-100);
                char str[10];
                sprintf(str, "%i00", val / 100);
                OverlayManager::getSingleton().getOverlayElement("tracks/ap_alt_val")->setCaption(str);
            }
            if (!strcmp(name, "tracks/ap_vs_up") && player_actor->ar_autopilot && mTimeUntilNextToggle <= 0)
            {
                mTimeUntilNextToggle = 0.1;
                int val = player_actor->ar_autopilot->adjVS(100);
                char str[10];
                if (val < 0)
                    sprintf(str, "%i00", val / 100);
                else if (val == 0)
                    strcpy(str, "000");
                else
                    sprintf(str, "+%i00", val / 100);
                OverlayManager::getSingleton().getOverlayElement("tracks/ap_vs_val")->setCaption(str);
            }
            if (!strcmp(name, "tracks/ap_vs_dn") && player_actor->ar_autopilot && mTimeUntilNextToggle <= 0)
            {
                mTimeUntilNextToggle = 0.1;
                int val = player_actor->ar_autopilot->adjVS(-100);
                char str[10];
                if (val < 0)
                    sprintf(str, "%i00", val / 100);
                else if (val == 0)
                    strcpy(str, "000");
                else
                    sprintf(str, "+%i00", val / 100);
                OverlayManager::getSingleton().getOverlayElement("tracks/ap_vs_val")->setCaption(str);
            }
            if (!strcmp(name, "tracks/ap_ias_up") && player_actor->ar_autopilot && mTimeUntilNextToggle <= 0)
            {
                mTimeUntilNextToggle = 0.1;
                int val = player_actor->ar_autopilot->adjIAS(1);
                char str[10];
                sprintf(str, "%.3u", val);
                OverlayManager::getSingleton().getOverlayElement("tracks/ap_ias_val")->setCaption(str);
            }
            if (!strcmp(name, "tracks/ap_ias_dn") && player_actor->ar_autopilot && mTimeUntilNextToggle <= 0)
            {
                mTimeUntilNextToggle = 0.1;
                int val = player_actor->ar_autopilot->adjIAS(-1);
                char str[10];
                sprintf(str, "%.3u", val);
                OverlayManager::getSingleton().getOverlayElement("tracks/ap_ias_val")->setCaption(str);
            }
        }
    }
    return res;
}

bool OverlayWrapper::mousePressed(const OIS::MouseEvent& _arg, OIS::MouseButtonID _id)
{
    return mouseMoved(_arg);
}

bool OverlayWrapper::mouseReleased(const OIS::MouseEvent& _arg, OIS::MouseButtonID _id)
{
    return mouseMoved(_arg);
}

void OverlayWrapper::SetupDirectionArrow()
{
    if (RoR::App::GetOverlayWrapper() != nullptr)
    {
        // setup direction arrow
        Ogre::Entity* arrow_entity = gEnv->sceneManager->createEntity("dirArrowEntity", "arrow2.mesh");
#if OGRE_VERSION<0x010602
		arrow_entity->setNormaliseNormals(true);
#endif //OGRE_VERSION

        // Add entity to the scene node
        m_direction_arrow_node = new SceneNode(gEnv->sceneManager);
        m_direction_arrow_node->attachObject(arrow_entity);
        m_direction_arrow_node->setVisible(false);
        m_direction_arrow_node->setScale(0.1, 0.1, 0.1);
        m_direction_arrow_node->setPosition(Vector3(-0.6, +0.4, -1));
        m_direction_arrow_node->setFixedYawAxis(true, Vector3::UNIT_Y);
        RoR::App::GetOverlayWrapper()->m_direction_arrow_overlay->add3D(m_direction_arrow_node);
    }
}

void OverlayWrapper::UpdateDirectionArrow(Actor* vehicle, Ogre::Vector3 const& point_to)
{
    m_direction_arrow_node->lookAt(point_to, Node::TS_WORLD, Vector3::UNIT_Y);
    Real distance = 0.0f;
    if (vehicle != nullptr && vehicle->ar_sim_state == Actor::SimState::LOCAL_SIMULATED)
    {
        distance = vehicle->getPosition().distance(point_to);
    }
    else if (gEnv->player)
    {
        distance = gEnv->player->getPosition().distance(point_to);
    }
    char tmp[256];
    sprintf(tmp, "%0.1f meter", distance);
    this->directionArrowDistance->setCaption(tmp);
}

void OverlayWrapper::HideDirectionOverlay()
{
    m_direction_arrow_overlay->hide();
    m_direction_arrow_node->setVisible(false);
    BITMASK_SET_0(m_visible_overlays, VisibleOverlays::DIRECTION_ARROW);
}

void OverlayWrapper::ShowDirectionOverlay(Ogre::String const& caption)
{
    m_direction_arrow_overlay->show();
    directionArrowText->setCaption(caption);
    directionArrowDistance->setCaption("");
    m_direction_arrow_node->setVisible(true);
    BITMASK_SET_1(m_visible_overlays, VisibleOverlays::DIRECTION_ARROW);
}

void OverlayWrapper::UpdatePressureTexture(float pressure)
{
    Real angle = 135.0 - pressure * 2.7;
    pressuretexture->setTextureRotate(Degree(angle));
}

void OverlayWrapper::UpdateLandVehicleHUD(Actor* vehicle)
{
    // gears
    int vehicle_getgear = vehicle->ar_engine->GetGear();
    if (vehicle_getgear > 0)
    {
        size_t numgears = vehicle->ar_engine->getNumGears();
        String gearstr = TOSTRING(vehicle_getgear) + "/" + TOSTRING(numgears);
        guiGear->setCaption(gearstr);
        guiGear3D->setCaption(gearstr);
    }
    else if (vehicle_getgear == 0)
    {
        guiGear->setCaption("N");
        guiGear3D->setCaption("N");
    }
    else
    {
        guiGear->setCaption("R");
        guiGear3D->setCaption("R");
    }

    //autogears
    int cg = vehicle->ar_engine->getAutoShift();
    for (int i = 0; i < 5; i++)
    {
        if (i == cg)
        {
            if (i == 1)
            {
                guiAuto[i]->setColourTop(ColourValue(1.0, 0.2, 0.2, 1.0));
                guiAuto[i]->setColourBottom(ColourValue(0.8, 0.1, 0.1, 1.0));
                guiAuto3D[i]->setColourTop(ColourValue(1.0, 0.2, 0.2, 1.0));
                guiAuto3D[i]->setColourBottom(ColourValue(0.8, 0.1, 0.1, 1.0));
            }
            else
            {
                guiAuto[i]->setColourTop(ColourValue(1.0, 1.0, 1.0, 1.0));
                guiAuto[i]->setColourBottom(ColourValue(0.8, 0.8, 0.8, 1.0));
                guiAuto3D[i]->setColourTop(ColourValue(1.0, 1.0, 1.0, 1.0));
                guiAuto3D[i]->setColourBottom(ColourValue(0.8, 0.8, 0.8, 1.0));
            }
        }
        else
        {
            if (i == 1)
            {
                guiAuto[i]->setColourTop(ColourValue(0.4, 0.05, 0.05, 1.0));
                guiAuto[i]->setColourBottom(ColourValue(0.3, 0.02, 0.2, 1.0));
                guiAuto3D[i]->setColourTop(ColourValue(0.4, 0.05, 0.05, 1.0));
                guiAuto3D[i]->setColourBottom(ColourValue(0.3, 0.02, 0.2, 1.0));
            }
            else
            {
                guiAuto[i]->setColourTop(ColourValue(0.4, 0.4, 0.4, 1.0));
                guiAuto[i]->setColourBottom(ColourValue(0.3, 0.3, 0.3, 1.0));
                guiAuto3D[i]->setColourTop(ColourValue(0.4, 0.4, 0.4, 1.0));
                guiAuto3D[i]->setColourBottom(ColourValue(0.3, 0.3, 0.3, 1.0));
            }
        }
    }

    // speedo / calculate speed
    Real guiSpeedFactor = 7.0 * (140.0 / vehicle->ar_speedo_max_kph);
    Real angle = 140 - fabs(vehicle->ar_wheel_speed * guiSpeedFactor);
    angle = std::max(-140.0f, angle);
    speedotexture->setTextureRotate(Degree(angle));

    // calculate tach stuff
    Real tachoFactor = 0.072;
    if (vehicle->ar_gui_use_engine_max_rpm)
    {
        tachoFactor = 0.072 * (3500 / vehicle->ar_engine->getMaxRPM());
    }
    angle = 126.0 - fabs(vehicle->ar_engine->GetEngineRpm() * tachoFactor);
    angle = std::max(-120.0f, angle);
    angle = std::min(angle, 121.0f);
    tachotexture->setTextureRotate(Degree(angle));
}

void OverlayWrapper::UpdateAerialHUD(Actor* vehicle)
{
    int ftp = vehicle->ar_num_aeroengines;

    //throttles
    thro1->setTop(thrtop + thrheight * (1.0 - vehicle->ar_aeroengines[0]->getThrottle()) - 1.0);
    if (ftp > 1)
        thro2->setTop(thrtop + thrheight * (1.0 - vehicle->ar_aeroengines[1]->getThrottle()) - 1.0);
    if (ftp > 2)
        thro3->setTop(thrtop + thrheight * (1.0 - vehicle->ar_aeroengines[2]->getThrottle()) - 1.0);
    if (ftp > 3)
        thro4->setTop(thrtop + thrheight * (1.0 - vehicle->ar_aeroengines[3]->getThrottle()) - 1.0);

    //fire
    if (vehicle->ar_aeroengines[0]->isFailed())
        engfireo1->setMaterialName("tracks/engfire-on");
    else
        engfireo1->setMaterialName("tracks/engfire-off");
    if (ftp > 1 && vehicle->ar_aeroengines[1]->isFailed())
        engfireo2->setMaterialName("tracks/engfire-on");
    else
        engfireo2->setMaterialName("tracks/engfire-off");
    if (ftp > 2 && vehicle->ar_aeroengines[2]->isFailed())
        engfireo3->setMaterialName("tracks/engfire-on");
    else
        engfireo3->setMaterialName("tracks/engfire-off");
    if (ftp > 3 && vehicle->ar_aeroengines[3]->isFailed())
        engfireo4->setMaterialName("tracks/engfire-on");
    else
        engfireo4->setMaterialName("tracks/engfire-off");

    //airspeed
    float angle = 0.0;
    float ground_speed_kt = vehicle->ar_nodes[0].Velocity.length() * 1.9438; // 1.943 = m/s in knots/s

    //tropospheric model valid up to 11.000m (33.000ft)
    float altitude = vehicle->ar_nodes[0].AbsPosition.y;
    //float sea_level_temperature=273.15+15.0; //in Kelvin
    float sea_level_pressure = 101325; //in Pa
    //float airtemperature=sea_level_temperature-altitude*0.0065; //in Kelvin
    float airpressure = sea_level_pressure * pow(1.0 - 0.0065 * altitude / 288.15, 5.24947); //in Pa
    float airdensity = airpressure * 0.0000120896;//1.225 at sea level

    float kt = ground_speed_kt * sqrt(airdensity / 1.225); //KIAS
    if (kt > 23.0)
    {
        if (kt < 50.0)
            angle = ((kt - 23.0) / 1.111);
        else if (kt < 100.0)
            angle = (24.0 + (kt - 50.0) / 0.8621);
        else if (kt < 300.0)
            angle = (82.0 + (kt - 100.0) / 0.8065);
        else
            angle = 329.0;
    }
    airspeedtexture->setTextureRotate(Degree(-angle));

    // AOA
    angle = 0;
    if (vehicle->ar_num_wings > 4)
        angle = vehicle->ar_wings[4].fa->aoa;
    if (kt < 10.0)
        angle = 0;
    float absangle = angle;
    if (absangle < 0)
        absangle = -absangle;

    SOUND_MODULATE(vehicle, SS_MOD_AOA, absangle);
    if (absangle > 18.0) // TODO: magicccc
        SOUND_START(vehicle, SS_TRIG_AOA);
    else
        SOUND_STOP(vehicle, SS_TRIG_AOA);

    if (angle > 25.0)
        angle = 25.0;
    if (angle < -25.0)
        angle = -25.0;
    aoatexture->setTextureRotate(Degree(-angle * 4.7 + 90.0));

    // altimeter
    angle = vehicle->ar_nodes[0].AbsPosition.y * 1.1811;
    altimetertexture->setTextureRotate(Degree(-angle));
    char altc[10];
    sprintf(altc, "%03u", (int)(vehicle->ar_nodes[0].AbsPosition.y / 30.48));
    alt_value_taoe->setCaption(altc);

    //adi
    //roll
    Vector3 rollv = vehicle->ar_nodes[vehicle->ar_camera_node_pos[0]].RelPosition - vehicle->ar_nodes[vehicle->ar_camera_node_roll[0]].RelPosition;
    rollv.normalise();
    float rollangle = asin(rollv.dotProduct(Vector3::UNIT_Y));

    //pitch
    Vector3 dirv = vehicle->getDirection();
    float pitchangle = asin(dirv.dotProduct(Vector3::UNIT_Y));
    Vector3 upv = dirv.crossProduct(-rollv);
    if (upv.y < 0)
        rollangle = 3.14159 - rollangle;
    adibugstexture->setTextureRotate(Radian(-rollangle));
    aditapetexture->setTextureVScroll(-pitchangle * 0.25);
    aditapetexture->setTextureRotate(Radian(-rollangle));

    //hsi
    float dirangle = atan2(dirv.dotProduct(Vector3::UNIT_X), dirv.dotProduct(-Vector3::UNIT_Z));
    hsirosetexture->setTextureRotate(Radian(dirangle));
    if (vehicle->ar_autopilot)
    {
        hsibugtexture->setTextureRotate(Radian(dirangle) - Degree(vehicle->ar_autopilot->heading));

        float vdev = 0;
        float hdev = 0;
        if (vehicle->ar_autopilot->IsIlsAvailable())
        {
            vdev = vehicle->ar_autopilot->GetVerticalApproachDeviation();
            hdev = vehicle->ar_autopilot->GetHorizontalApproachDeviation();
        }
        if (hdev > 15)
            hdev = 15;
        if (hdev < -15)
            hdev = -15;
        hsivtexture->setTextureUScroll(-hdev * 0.02);
        if (vdev > 15)
            vdev = 15;
        if (vdev < -15)
            vdev = -15;
        hsihtexture->setTextureVScroll(-vdev * 0.02);
    }

    //vvi
    float vvi = vehicle->ar_nodes[0].Velocity.y * 196.85;
    if (vvi < 1000.0 && vvi > -1000.0)
        angle = vvi * 0.047;
    if (vvi > 1000.0 && vvi < 6000.0)
        angle = 47.0 + (vvi - 1000.0) * 0.01175;
    if (vvi > 6000.0)
        angle = 105.75;
    if (vvi < -1000.0 && vvi > -6000.0)
        angle = -47.0 + (vvi + 1000.0) * 0.01175;
    if (vvi < -6000.0)
        angle = -105.75;
    vvitexture->setTextureRotate(Degree(-angle + 90.0));

    //rpm
    float pcent = vehicle->ar_aeroengines[0]->getRPMpc();
    if (pcent < 60.0)
        angle = -5.0 + pcent * 1.9167;
    else if (pcent < 110.0)
        angle = 110.0 + (pcent - 60.0) * 4.075;
    else
        angle = 314.0;
    airrpm1texture->setTextureRotate(Degree(-angle));

    if (ftp > 1)
        pcent = vehicle->ar_aeroengines[1]->getRPMpc();
    else
        pcent = 0;
    if (pcent < 60.0)
        angle = -5.0 + pcent * 1.9167;
    else if (pcent < 110.0)
        angle = 110.0 + (pcent - 60.0) * 4.075;
    else
        angle = 314.0;
    airrpm2texture->setTextureRotate(Degree(-angle));

    if (ftp > 2)
        pcent = vehicle->ar_aeroengines[2]->getRPMpc();
    else
        pcent = 0;
    if (pcent < 60.0)
        angle = -5.0 + pcent * 1.9167;
    else if (pcent < 110.0)
        angle = 110.0 + (pcent - 60.0) * 4.075;
    else
        angle = 314.0;
    airrpm3texture->setTextureRotate(Degree(-angle));

    if (ftp > 3)
        pcent = vehicle->ar_aeroengines[3]->getRPMpc();
    else
        pcent = 0;
    if (pcent < 60.0)
        angle = -5.0 + pcent * 1.9167;
    else if (pcent < 110.0)
        angle = 110.0 + (pcent - 60.0) * 4.075;
    else
        angle = 314.0;
    airrpm4texture->setTextureRotate(Degree(-angle));

    if (vehicle->ar_aeroengines[0]->getType() == AeroEngine::AEROENGINE_TYPE_TURBOPROP)
    {
        Turboprop* tp = (Turboprop*)vehicle->ar_aeroengines[0];
        //pitch
        airpitch1texture->setTextureRotate(Degree(-tp->pitch * 2.0));
        //torque
        pcent = 100.0 * tp->indicated_torque / tp->max_torque;
        if (pcent < 60.0)
            angle = -5.0 + pcent * 1.9167;
        else if (pcent < 110.0)
            angle = 110.0 + (pcent - 60.0) * 4.075;
        else
            angle = 314.0;
        airtorque1texture->setTextureRotate(Degree(-angle));
    }

    if (ftp > 1 && vehicle->ar_aeroengines[1]->getType() == AeroEngine::AEROENGINE_TYPE_TURBOPROP)
    {
        Turboprop* tp = (Turboprop*)vehicle->ar_aeroengines[1];
        //pitch
        airpitch2texture->setTextureRotate(Degree(-tp->pitch * 2.0));
        //torque
        pcent = 100.0 * tp->indicated_torque / tp->max_torque;
        if (pcent < 60.0)
            angle = -5.0 + pcent * 1.9167;
        else if (pcent < 110.0)
            angle = 110.0 + (pcent - 60.0) * 4.075;
        else
            angle = 314.0;
        airtorque2texture->setTextureRotate(Degree(-angle));
    }

    if (ftp > 2 && vehicle->ar_aeroengines[2]->getType() == AeroEngine::AEROENGINE_TYPE_TURBOPROP)
    {
        Turboprop* tp = (Turboprop*)vehicle->ar_aeroengines[2];
        //pitch
        airpitch3texture->setTextureRotate(Degree(-tp->pitch * 2.0));
        //torque
        pcent = 100.0 * tp->indicated_torque / tp->max_torque;
        if (pcent < 60.0)
            angle = -5.0 + pcent * 1.9167;
        else if (pcent < 110.0)
            angle = 110.0 + (pcent - 60.0) * 4.075;
        else
            angle = 314.0;
        airtorque3texture->setTextureRotate(Degree(-angle));
    }

    if (ftp > 3 && vehicle->ar_aeroengines[3]->getType() == AeroEngine::AEROENGINE_TYPE_TURBOPROP)
    {
        Turboprop* tp = (Turboprop*)vehicle->ar_aeroengines[3];
        //pitch
        airpitch4texture->setTextureRotate(Degree(-tp->pitch * 2.0));
        //torque
        pcent = 100.0 * tp->indicated_torque / tp->max_torque;
        if (pcent < 60.0)
            angle = -5.0 + pcent * 1.9167;
        else if (pcent < 110.0)
            angle = 110.0 + (pcent - 60.0) * 4.075;
        else
            angle = 314.0;
        airtorque4texture->setTextureRotate(Degree(-angle));
    }

    //starters
    if (vehicle->ar_aeroengines[0]->getIgnition())
        engstarto1->setMaterialName("tracks/engstart-on");
    else
        engstarto1->setMaterialName("tracks/engstart-off");
    if (ftp > 1 && vehicle->ar_aeroengines[1]->getIgnition())
        engstarto2->setMaterialName("tracks/engstart-on");
    else
        engstarto2->setMaterialName("tracks/engstart-off");
    if (ftp > 2 && vehicle->ar_aeroengines[2]->getIgnition())
        engstarto3->setMaterialName("tracks/engstart-on");
    else
        engstarto3->setMaterialName("tracks/engstart-off");
    if (ftp > 3 && vehicle->ar_aeroengines[3]->getIgnition())
        engstarto4->setMaterialName("tracks/engstart-on");
    else
        engstarto4->setMaterialName("tracks/engstart-off");
}

void OverlayWrapper::UpdateMarineHUD(Actor* vehicle)
{
    int fsp = vehicle->ar_num_screwprops;
    //throttles
    bthro1->setTop(thrtop + thrheight * (0.5 - vehicle->ar_screwprops[0]->getThrottle() / 2.0) - 1.0);
    if (fsp > 1)
    {
        bthro2->setTop(thrtop + thrheight * (0.5 - vehicle->ar_screwprops[1]->getThrottle() / 2.0) - 1.0);
    }

    //position
    Vector3 dir = vehicle->getDirection();

    char tmp[50] = "";
    if (vehicle->getLowestNode() != -1)
    {
        Vector3 pos = vehicle->ar_nodes[vehicle->getLowestNode()].AbsPosition;
        float height = pos.y - RoR::App::GetSimTerrain()->GetHeightAt(pos.x, pos.z);
        if (height > 0.1 && height < 99.9)
        {
            sprintf(tmp, "%2.1f", height);
            boat_depth_value_taoe->setCaption(tmp);
        }
        else
        {
            boat_depth_value_taoe->setCaption("--.-");
        }
    }

    //waterspeed
    float angle = 0.0;
    float kt = dir.dotProduct(vehicle->ar_nodes[vehicle->ar_camera_node_pos[0]].Velocity) * 1.9438;
    angle = kt * 4.2;
    boatspeedtexture->setTextureRotate(Degree(-angle));
    boatsteertexture->setTextureRotate(Degree(vehicle->ar_screwprops[0]->getRudder() * 170));
}

void OverlayWrapper::ShowRacingOverlay()
{
    m_racing_overlay->show();
    BITMASK_SET_1(m_visible_overlays, VisibleOverlays::RACING);
}

void OverlayWrapper::HideRacingOverlay()
{
    m_racing_overlay->hide();
    BITMASK_SET_0(m_visible_overlays, VisibleOverlays::RACING);
}

void OverlayWrapper::TemporarilyHideAllOverlays(Actor* current_vehicle)
{
    m_racing_overlay->hide();
    m_direction_arrow_overlay->hide();
    m_debug_fps_memory_overlay->hide();
    m_debug_beam_timing_overlay->hide();

    showDashboardOverlays(false, current_vehicle);
}

void OverlayWrapper::RestoreOverlaysVisibility(Actor* current_vehicle)
{
    if (BITMASK_IS_1(m_visible_overlays, VisibleOverlays::RACING))
    {
        m_racing_overlay->show();
    }
    else if (BITMASK_IS_1(m_visible_overlays, VisibleOverlays::DIRECTION_ARROW))
    {
        m_direction_arrow_overlay->show();
    }
    else if (BITMASK_IS_1(m_visible_overlays, VisibleOverlays::DEBUG_FPS_MEMORY))
    {
        m_debug_fps_memory_overlay->show();
    }
    else if (BITMASK_IS_1(m_visible_overlays, VisibleOverlays::DEBUG_BEAM_TIMING))
    {
        m_debug_beam_timing_overlay->show();
    }

    showDashboardOverlays(true, current_vehicle);
}
