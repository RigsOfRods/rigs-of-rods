/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2013-2019 Petr Ohlidal & contributors

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
#include "Beam.h"
#include "BeamFactory.h"
#include "Character.h"
#include "DashBoardManager.h"
#include "BeamEngine.h"
#include "ErrorUtils.h"
#include "FlexAirfoil.h"
#include "GfxActor.h"
#include "GlobalEnvironment.h"
#include "Language.h"
#include "OgreSubsystem.h"
#include "RoRFrameListener.h" // SimController
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
    m_dashboard_visible(false),
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

Ogre::TextureUnitState* GetTexUnit(Ogre::String material_name) // Internal helper
{
    return MaterialManager::getSingleton().getByName(material_name)->getTechnique(0)->getPass(0)->getTextureUnitState(0);
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

    OverlayElement* vere = loadOverlayElement("Core/RoRVersionString");
    if (vere)
        vere->setCaption("Rigs of Rods version " + String(ROR_VERSION_STRING));

    m_machine_dashboard_overlay = loadOverlay("tracks/MachineDashboardOverlay");
    m_aerial_dashboard.dash_overlay = loadOverlay("tracks/AirDashboardOverlay", false);
    m_aerial_dashboard.needles_overlay = loadOverlay("tracks/AirNeedlesOverlay", false);
    m_marine_dashboard_overlay = loadOverlay("tracks/BoatDashboardOverlay");
    m_marine_dashboard_needles_overlay = loadOverlay("tracks/BoatNeedlesOverlay");


    //adjust dashboard size for screen ratio
    resizePanel(loadOverlayElement("tracks/pressureo"));
    resizePanel(loadOverlayElement("tracks/pressureneedle"));
    MaterialPtr m = MaterialManager::getSingleton().getByName("tracks/pressureneedle_mat");
    if (!m.isNull())
        pressuretexture = m->getTechnique(0)->getPass(0)->getTextureUnitState(0);
    else
        pressuretexture = nullptr;

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

    resizePanel(m_aerial_dashboard.engines[0].thr_element = loadOverlayElement("tracks/thrust1"));
    resizePanel(m_aerial_dashboard.engines[1].thr_element = loadOverlayElement("tracks/thrust2"));
    resizePanel(m_aerial_dashboard.engines[2].thr_element = loadOverlayElement("tracks/thrust3"));
    resizePanel(m_aerial_dashboard.engines[3].thr_element = loadOverlayElement("tracks/thrust4"));

    thrtop = 1.0f + tempoe->getTop() + m_aerial_dashboard.engines[0].thr_element->getHeight() * 0.5f;
    thrheight = tempoe->getHeight() - m_aerial_dashboard.engines[0].thr_element->getHeight() * 2.0f;
    throffset = m_aerial_dashboard.engines[0].thr_element->getHeight() * 0.5f;

    m_aerial_dashboard.thrust_track_top = thrtop;
    m_aerial_dashboard.thrust_track_height = thrheight;

    m_aerial_dashboard.engines[0].engfire_element = loadOverlayElement("tracks/engfire1");
    m_aerial_dashboard.engines[1].engfire_element = loadOverlayElement("tracks/engfire2");
    m_aerial_dashboard.engines[2].engfire_element = loadOverlayElement("tracks/engfire3");
    m_aerial_dashboard.engines[3].engfire_element = loadOverlayElement("tracks/engfire4");
    m_aerial_dashboard.engines[0].engstart_element = loadOverlayElement("tracks/engstart1");
    m_aerial_dashboard.engines[1].engstart_element = loadOverlayElement("tracks/engstart2");
    m_aerial_dashboard.engines[2].engstart_element = loadOverlayElement("tracks/engstart3");
    m_aerial_dashboard.engines[3].engstart_element = loadOverlayElement("tracks/engstart4");
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
    m_aerial_dashboard.alt_value_textarea = (TextAreaOverlayElement*)loadOverlayElement("tracks/altimeter_val");
    boat_depth_value_taoe = (TextAreaOverlayElement*)loadOverlayElement("tracks/boatdepthmeter_val");
    resizePanel(loadOverlayElement("tracks/adi-tape"));
    resizePanel(loadOverlayElement("tracks/adi"));
    resizePanel(loadOverlayElement("tracks/adi-bugs"));
    m_aerial_dashboard.adibugstexture = GetTexUnit("tracks/adi-bugs");
    m_aerial_dashboard.aditapetexture = GetTexUnit("tracks/adi-tape");
    resizePanel(loadOverlayElement("tracks/aoa"));
    resizePanel(loadOverlayElement("tracks/hsi"));
    resizePanel(loadOverlayElement("tracks/hsi-rose"));
    resizePanel(loadOverlayElement("tracks/hsi-bug"));
    resizePanel(loadOverlayElement("tracks/hsi-v"));
    resizePanel(loadOverlayElement("tracks/hsi-h"));
    m_aerial_dashboard.hsirosetexture = GetTexUnit("tracks/hsi-rose");
    m_aerial_dashboard.hsibugtexture =  GetTexUnit("tracks/hsi-bug");
    m_aerial_dashboard.hsivtexture =    GetTexUnit("tracks/hsi-v");
    m_aerial_dashboard.hsihtexture =    GetTexUnit("tracks/hsi-h");
    //autopilot
    reposPanel(loadOverlayElement("tracks/ap_hdg_pack"));
    reposPanel(loadOverlayElement("tracks/ap_wlv_but"));
    reposPanel(loadOverlayElement("tracks/ap_nav_but"));
    reposPanel(loadOverlayElement("tracks/ap_alt_pack"));
    reposPanel(loadOverlayElement("tracks/ap_vs_pack"));
    reposPanel(loadOverlayElement("tracks/ap_ias_pack"));
    reposPanel(loadOverlayElement("tracks/ap_gpws_but"));
    reposPanel(loadOverlayElement("tracks/ap_brks_but"));
    m_aerial_dashboard.hdg.Setup("tracks/ap_hdg_but", "tracks/hdg-on", "tracks/hdg-off");
    m_aerial_dashboard.hdg_trim.Setup("tracks/ap_hdg_up", "tracks/ap_hdg_dn", "tracks/ap_hdg_val");
    m_aerial_dashboard.wlv.Setup("tracks/ap_wlv_but", "tracks/wlv-on", "tracks/wlv-off");
    m_aerial_dashboard.nav.Setup("tracks/ap_nav_but", "tracks/nav-on", "tracks/nav-off");
    m_aerial_dashboard.alt.Setup("tracks/ap_alt_but", "tracks/hold-on", "tracks/hold-off");
    m_aerial_dashboard.alt_trim.Setup("tracks/ap_alt_up", "tracks/ap_alt_dn", "tracks/ap_alt_val");
    m_aerial_dashboard.vs.Setup("tracks/ap_vs_but", "tracks/vs-on", "tracks/vs-off");
    m_aerial_dashboard.vs_trim.Setup("tracks/ap_vs_up", "tracks/ap_vs_dn", "tracks/ap_vs_val");
    m_aerial_dashboard.ias.Setup("tracks/ap_ias_but", "tracks/athr-on", "tracks/athr-off");
    m_aerial_dashboard.ias_trim.Setup("tracks/ap_ias_up", "tracks/ap_ias_dn", "tracks/ap_ias_val");
    m_aerial_dashboard.gpws.Setup("tracks/ap_gpws_but", "tracks/gpws-on", "tracks/gpws-off");
    m_aerial_dashboard.brks.Setup("tracks/ap_brks_but", "tracks/brks-on", "tracks/brks-off");

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
    m_aerial_dashboard.airspeedtexture = GetTexUnit("tracks/airspeedneedle_mat");

    resizePanel(loadOverlayElement("tracks/altimeterneedle"));
    m_aerial_dashboard.altimetertexture = GetTexUnit("tracks/altimeterneedle_mat");

    resizePanel(loadOverlayElement("tracks/vvineedle"));
    m_aerial_dashboard.vvitexture = GetTexUnit("tracks/vvineedle_mat");

    resizePanel(loadOverlayElement("tracks/aoaneedle"));
    m_aerial_dashboard.aoatexture = GetTexUnit("tracks/aoaneedle_mat");

    resizePanel(loadOverlayElement("tracks/airrpm1needle"));
    m_aerial_dashboard.engines[0].rpm_texture = GetTexUnit("tracks/airrpm1needle_mat");
    resizePanel(loadOverlayElement("tracks/airrpm2needle"));
    m_aerial_dashboard.engines[1].rpm_texture = GetTexUnit("tracks/airrpm2needle_mat");
    resizePanel(loadOverlayElement("tracks/airrpm3needle"));
    m_aerial_dashboard.engines[2].rpm_texture = GetTexUnit("tracks/airrpm3needle_mat");
    resizePanel(loadOverlayElement("tracks/airrpm4needle"));
    m_aerial_dashboard.engines[3].rpm_texture = GetTexUnit("tracks/airrpm4needle_mat");

    resizePanel(loadOverlayElement("tracks/airpitch1needle"));
    m_aerial_dashboard.engines[0].pitch_texture = GetTexUnit("tracks/airpitch1needle_mat");
    resizePanel(loadOverlayElement("tracks/airpitch2needle"));
    m_aerial_dashboard.engines[1].pitch_texture = GetTexUnit("tracks/airpitch2needle_mat");
    resizePanel(loadOverlayElement("tracks/airpitch3needle"));
    m_aerial_dashboard.engines[2].pitch_texture = GetTexUnit("tracks/airpitch3needle_mat");
    resizePanel(loadOverlayElement("tracks/airpitch4needle"));
    m_aerial_dashboard.engines[3].pitch_texture = GetTexUnit("tracks/airpitch4needle_mat");

    resizePanel(loadOverlayElement("tracks/airtorque1needle"));
    m_aerial_dashboard.engines[0].torque_texture = GetTexUnit("tracks/airtorque1needle_mat");
    resizePanel(loadOverlayElement("tracks/airtorque2needle"));
    m_aerial_dashboard.engines[1].torque_texture = GetTexUnit("tracks/airtorque2needle_mat");
    resizePanel(loadOverlayElement("tracks/airtorque3needle"));
    m_aerial_dashboard.engines[2].torque_texture = GetTexUnit("tracks/airtorque3needle_mat");
    resizePanel(loadOverlayElement("tracks/airtorque4needle"));
    m_aerial_dashboard.engines[3].torque_texture = GetTexUnit("tracks/airtorque4needle_mat");

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
    laptime = (TextAreaOverlayElement*)loadOverlayElement("tracks/Racing/LapTime");
    bestlaptime = (TextAreaOverlayElement*)loadOverlayElement("tracks/Racing/BestLapTime");

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
    if (!m_debug_fps_memory_overlay)
        return;

    if (mode > 0)
    {
        m_debug_fps_memory_overlay->show();
        BITMASK_SET_1(m_visible_overlays, VisibleOverlays::DEBUG_FPS_MEMORY);
    }
    else
    {
        m_debug_fps_memory_overlay->hide();
        BITMASK_SET_0(m_visible_overlays, VisibleOverlays::DEBUG_FPS_MEMORY);
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

void OverlayWrapper::ToggleDashboardOverlays(Actor* actor)
{
    showDashboardOverlays(!m_dashboard_visible, actor);
}

void OverlayWrapper::showDashboardOverlays(bool show, Actor* actor)
{
    m_dashboard_visible = show;

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
            m_aerial_dashboard.needles_overlay->show();
            m_aerial_dashboard.dash_overlay->show();
        }
        else if (mode == BOAT)
        {
            m_marine_dashboard_needles_overlay->show();
            m_marine_dashboard_overlay->show();
        }
        else if (mode == MACHINE)
        {
            Ogre::OverlayElement* help_elem = OverlayManager::getSingleton().getOverlayElement("tracks/machinehelppanel");
            if (actor->GetGfxActor()->GetAttributes().xa_help_mat)
            {
                help_elem->setMaterial(actor->GetGfxActor()->GetAttributes().xa_help_mat);
            }
            else
            {
                help_elem->setMaterialName("tracks/black");
            }
            m_machine_dashboard_overlay->show();
        }
    }
    else
    {
        m_aerial_dashboard.needles_overlay->hide();
        m_aerial_dashboard.dash_overlay->hide();

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
    if (!m_aerial_dashboard.needles_overlay->isVisible())
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
        const int num_engines = std::min(4, player_actor->ar_num_aeroengines);

        OverlayElement* element = m_aerial_dashboard.needles_overlay->findElementAt(mouseX, mouseY);
        if (element)
        {
            res = true;
            float thr_value = 1.0f - ((mouseY - thrtop - throffset) / thrheight);
            for (int i = 0; i < num_engines; ++i)
            {
                if (element == m_aerial_dashboard.engines[i].thr_element)
                {
                    player_actor->ar_aeroengines[i]->setThrottle(thr_value);
                }
            }
        }

        element = m_aerial_dashboard.dash_overlay->findElementAt(mouseX, mouseY);
        if (element)
        {
            res = true;
            for (int i = 0; i < num_engines; ++i)
            {
                if (element == m_aerial_dashboard.engines[i].engstart_element)
                {
                    player_actor->ar_aeroengines[i]->flipStart();
                }
            }
            if (player_actor->ar_autopilot && mTimeUntilNextToggle <= 0)
            {
                //heading group
                if (element == m_aerial_dashboard.hdg.element)
                {
                    mTimeUntilNextToggle = 0.2;
                    player_actor->ar_autopilot->toggleHeading(Autopilot::HEADING_FIXED);
                }
                if (element == m_aerial_dashboard.wlv.element)
                {
                    mTimeUntilNextToggle = 0.2;
                    player_actor->ar_autopilot->toggleHeading(Autopilot::HEADING_WLV);
                }
                if (element == m_aerial_dashboard.nav.element)
                {
                    mTimeUntilNextToggle = 0.2;
                    player_actor->ar_autopilot->toggleHeading(Autopilot::HEADING_NAV);
                }
                //altitude group
                if (element == m_aerial_dashboard.alt.element)
                {
                    mTimeUntilNextToggle = 0.2;
                    player_actor->ar_autopilot->toggleAlt(Autopilot::ALT_FIXED);
                }
                if (element == m_aerial_dashboard.vs.element)
                {
                    mTimeUntilNextToggle = 0.2;
                    player_actor->ar_autopilot->toggleAlt(Autopilot::ALT_VS);
                }
                //IAS
                if (element == m_aerial_dashboard.ias.element)
                {
                    mTimeUntilNextToggle = 0.2;
                    player_actor->ar_autopilot->toggleIAS();
                }
                //GPWS
                if (element == m_aerial_dashboard.gpws.element)
                {
                    mTimeUntilNextToggle = 0.2;
                    player_actor->ar_autopilot->toggleGPWS();
                }
                //BRKS
                if (element == m_aerial_dashboard.brks.element)
                {
                    mTimeUntilNextToggle = 0.2;
                    player_actor->ToggleParkingBrake();
                }
                //trims
                if (element == m_aerial_dashboard.hdg_trim.up_button)
                {
                    mTimeUntilNextToggle = 0.1;
                    player_actor->ar_autopilot->adjHDG(1);
                }
                if (element == m_aerial_dashboard.hdg_trim.dn_button)
                {
                    mTimeUntilNextToggle = 0.1;
                    player_actor->ar_autopilot->adjHDG(-1);
                }
                if (element == m_aerial_dashboard.alt_trim.up_button)
                {
                    mTimeUntilNextToggle = 0.1;
                    player_actor->ar_autopilot->adjALT(100);
                }
                if (element == m_aerial_dashboard.alt_trim.dn_button)
                {
                    mTimeUntilNextToggle = 0.1;
                    player_actor->ar_autopilot->adjALT(-100);
                }
                if (element == m_aerial_dashboard.vs_trim.up_button)
                {
                    mTimeUntilNextToggle = 0.1;
                    player_actor->ar_autopilot->adjVS(100);
                }
                if (element == m_aerial_dashboard.vs_trim.dn_button)
                {
                    mTimeUntilNextToggle = 0.1;
                    player_actor->ar_autopilot->adjVS(-100);
                }
                if (element == m_aerial_dashboard.ias_trim.up_button)
                {
                    mTimeUntilNextToggle = 0.1;
                    player_actor->ar_autopilot->adjIAS(1);
                }
                if (element == m_aerial_dashboard.ias_trim.dn_button)
                {
                    mTimeUntilNextToggle = 0.1;
                    player_actor->ar_autopilot->adjIAS(-1);
                }
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
        Ogre::Entity* arrow_entity = gEnv->sceneManager->createEntity("arrow2.mesh");
        arrow_entity->setRenderQueueGroup(Ogre::RENDER_QUEUE_OVERLAY);

        // Add entity to the scene node
        m_direction_arrow_node = new SceneNode(gEnv->sceneManager);
        m_direction_arrow_node->attachObject(arrow_entity);
        m_direction_arrow_node->setVisible(false);
        m_direction_arrow_node->setScale(0.1, 0.1, 0.1);
        m_direction_arrow_node->setPosition(Vector3(-0.6, +0.4, -1));
        m_direction_arrow_node->setFixedYawAxis(true, Vector3::UNIT_Y);
        m_direction_arrow_overlay->add3D(m_direction_arrow_node);
    }
}

void OverlayWrapper::UpdateDirectionArrowHud(RoR::GfxActor* player_vehicle, Ogre::Vector3 point_to, Ogre::Vector3 character_pos)
{
    m_direction_arrow_node->lookAt(point_to, Node::TS_WORLD, Vector3::UNIT_Y);
    Real distance = 0.0f;
    if (player_vehicle != nullptr && player_vehicle->GetSimDataBuffer().simbuf_live_local)
    {
        distance = player_vehicle->GetSimDataBuffer().simbuf_pos.distance(point_to);
    }
    else if (gEnv->player)
    {
        distance = character_pos.distance(point_to);
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

void OverlayWrapper::UpdatePressureTexture(RoR::GfxActor* ga)
{
    const float pressure = ga->GetSimDataBuffer().simbuf_tyre_pressure;
    Real angle = 135.0 - pressure * 2.7;
    pressuretexture->setTextureRotate(Degree(angle));
}

void OverlayWrapper::UpdateLandVehicleHUD(RoR::GfxActor* ga)
{
    // gears
    int vehicle_getgear = ga->GetSimDataBuffer().simbuf_gear;
    if (vehicle_getgear > 0)
    {
        size_t numgears = ga->GetAttributes().xa_num_gears;
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
    int cg = ga->GetSimDataBuffer().simbuf_autoshift;
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
    Real guiSpeedFactor = 7.0 * (140.0 / ga->GetAttributes().xa_speedo_highest_kph);
    Real angle = 140 - fabs(ga->GetSimDataBuffer().simbuf_wheel_speed * guiSpeedFactor);
    angle = std::max(-140.0f, angle);
    speedotexture->setTextureRotate(Degree(angle));

    // calculate tach stuff
    Real tachoFactor = 0.072;
    if (ga->GetAttributes().xa_speedo_use_engine_max_rpm)
    {
        tachoFactor = 0.072 * (3500 / ga->GetAttributes().xa_engine_max_rpm);
    }
    angle = 126.0 - fabs(ga->GetSimDataBuffer().simbuf_engine_rpm * tachoFactor);
    angle = std::max(-120.0f, angle);
    angle = std::min(angle, 121.0f);
    tachotexture->setTextureRotate(Degree(angle));
}

void OverlayWrapper::UpdateAerialHUD(RoR::GfxActor* gfx_actor)
{
    RoR::GfxActor::SimBuffer& simbuf = gfx_actor->GetSimDataBuffer();
    RoR::GfxActor::SimBuffer::NodeSB* nodes = gfx_actor->GetSimNodeBuffer();
    RoR::GfxActor::Attributes& attr = gfx_actor->GetAttributes();

    auto const& simbuf_ae = simbuf.simbuf_aeroengines;
    int num_ae = static_cast<int>( simbuf_ae.size() );

    //throttles
    m_aerial_dashboard.SetThrottle(0, true, simbuf_ae[0].simbuf_ae_throttle);
    m_aerial_dashboard.SetThrottle(1, (num_ae > 1), (num_ae > 1) ? simbuf_ae[1].simbuf_ae_throttle : 0.f);
    m_aerial_dashboard.SetThrottle(2, (num_ae > 2), (num_ae > 2) ? simbuf_ae[2].simbuf_ae_throttle : 0.f);
    m_aerial_dashboard.SetThrottle(3, (num_ae > 3), (num_ae > 3) ? simbuf_ae[3].simbuf_ae_throttle : 0.f);

    //fire
    m_aerial_dashboard.SetEngineFailed(0, simbuf_ae[0].simbuf_ae_failed);
    m_aerial_dashboard.SetEngineFailed(1, num_ae > 1 && simbuf_ae[1].simbuf_ae_failed);
    m_aerial_dashboard.SetEngineFailed(2, num_ae > 2 && simbuf_ae[2].simbuf_ae_failed);
    m_aerial_dashboard.SetEngineFailed(3, num_ae > 3 && simbuf_ae[3].simbuf_ae_failed);

    //airspeed
    float angle = 0.0;
    float ground_speed_kt = simbuf.simbuf_node0_velo.length() * 1.9438; // 1.943 = m/s in knots/s

    //tropospheric model valid up to 11.000m (33.000ft)
    float altitude = nodes[0].AbsPosition.y;
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
    m_aerial_dashboard.airspeedtexture->setTextureRotate(Degree(-angle));

    // AOA
    angle = simbuf.simbuf_wing4_aoa;
    if (kt < 10.0)
        angle = 0;
    float absangle = angle;
    if (absangle < 0)
        absangle = -absangle;

    const int actor_id = gfx_actor->GetActorId();
    SOUND_MODULATE(actor_id, SS_MOD_AOA, absangle);
    if (absangle > 18.0) // TODO: magicccc
        SOUND_START(actor_id, SS_TRIG_AOA);
    else
        SOUND_STOP(actor_id, SS_TRIG_AOA);

    if (angle > 25.0)
        angle = 25.0;
    if (angle < -25.0)
        angle = -25.0;
    m_aerial_dashboard.aoatexture->setTextureRotate(Degree(-angle * 4.7 + 90.0));

    // altimeter
    angle = nodes[0].AbsPosition.y * 1.1811;
    m_aerial_dashboard.altimetertexture->setTextureRotate(Degree(-angle));
    char altc[10];
    sprintf(altc, "%03u", (int)(nodes[0].AbsPosition.y / 30.48));
    m_aerial_dashboard.alt_value_textarea->setCaption(altc);

    //adi
    //roll
    Vector3 rollv = nodes[attr.xa_camera0_pos_node].AbsPosition - nodes[attr.xa_camera0_roll_node].AbsPosition;
    rollv.normalise();
    float rollangle = asin(rollv.dotProduct(Vector3::UNIT_Y));

    //pitch
    Vector3 dirv = simbuf.simbuf_direction;
    float pitchangle = asin(dirv.dotProduct(Vector3::UNIT_Y));
    Vector3 upv = dirv.crossProduct(-rollv);
    if (upv.y < 0)
        rollangle = 3.14159 - rollangle;
    m_aerial_dashboard.adibugstexture->setTextureRotate(Radian(-rollangle));
    m_aerial_dashboard.aditapetexture->setTextureVScroll(-pitchangle * 0.25);
    m_aerial_dashboard.aditapetexture->setTextureRotate(Radian(-rollangle));

    //hsi
    float dirangle = atan2(dirv.dotProduct(Vector3::UNIT_X), dirv.dotProduct(-Vector3::UNIT_Z));
    m_aerial_dashboard.hsirosetexture->setTextureRotate(Radian(dirangle));

    //autopilot
    m_aerial_dashboard.hsibugtexture->setTextureRotate(Radian(dirangle) - Degree(simbuf.simbuf_ap_heading_value));

    float vdev = 0;
    float hdev = 0;
    if (simbuf.simbuf_ap_ils_available)
    {
        vdev = simbuf.simbuf_ap_ils_vdev;
        hdev = simbuf.simbuf_ap_ils_hdev;
    }
    if (hdev > 15)
        hdev = 15;
    if (hdev < -15)
        hdev = -15;
    m_aerial_dashboard.hsivtexture->setTextureUScroll(-hdev * 0.02);
    if (vdev > 15)
        vdev = 15;
    if (vdev < -15)
        vdev = -15;
    m_aerial_dashboard.hsihtexture->setTextureVScroll(-vdev * 0.02);

    //vvi
    float vvi = simbuf.simbuf_node0_velo.y * 196.85;
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
    m_aerial_dashboard.vvitexture->setTextureRotate(Degree(-angle + 90.0));

    //rpm
    m_aerial_dashboard.SetEngineRpm(0, simbuf_ae[0].simbuf_ae_rpmpc);
    m_aerial_dashboard.SetEngineRpm(1, (num_ae > 1) ? simbuf_ae[1].simbuf_ae_rpmpc : 0.f);
    m_aerial_dashboard.SetEngineRpm(2, (num_ae > 2) ? simbuf_ae[2].simbuf_ae_rpmpc : 0.f);
    m_aerial_dashboard.SetEngineRpm(3, (num_ae > 3) ? simbuf_ae[3].simbuf_ae_rpmpc : 0.f);

    //turboprops - pitch
    m_aerial_dashboard.SetEnginePitch(0, (simbuf_ae[0].simbuf_ae_turboprop) ? simbuf_ae[0].simbuf_tp_aepitch : 0.f);
    m_aerial_dashboard.SetEnginePitch(1, (num_ae > 1 && simbuf_ae[1].simbuf_ae_turboprop) ? simbuf_ae[1].simbuf_tp_aepitch : 0.f);
    m_aerial_dashboard.SetEnginePitch(2, (num_ae > 2 && simbuf_ae[2].simbuf_ae_turboprop) ? simbuf_ae[2].simbuf_tp_aepitch : 0.f);
    m_aerial_dashboard.SetEnginePitch(3, (num_ae > 3 && simbuf_ae[3].simbuf_ae_turboprop) ? simbuf_ae[3].simbuf_tp_aepitch : 0.f);

    //turboprops - torque
    m_aerial_dashboard.SetEngineTorque(0, (simbuf_ae[0].simbuf_ae_turboprop) ? simbuf_ae[0].simbuf_tp_aetorque : 0.f);
    m_aerial_dashboard.SetEngineTorque(1, (num_ae > 1 && simbuf_ae[1].simbuf_ae_turboprop) ? simbuf_ae[1].simbuf_tp_aetorque : 0.f);
    m_aerial_dashboard.SetEngineTorque(2, (num_ae > 2 && simbuf_ae[2].simbuf_ae_turboprop) ? simbuf_ae[2].simbuf_tp_aetorque : 0.f);
    m_aerial_dashboard.SetEngineTorque(3, (num_ae > 3 && simbuf_ae[3].simbuf_ae_turboprop) ? simbuf_ae[3].simbuf_tp_aetorque : 0.f);

    //starters
    m_aerial_dashboard.SetIgnition(0, true, simbuf_ae[0].simbuf_ae_ignition);
    m_aerial_dashboard.SetIgnition(1, num_ae > 1, num_ae > 1 && simbuf_ae[1].simbuf_ae_ignition);
    m_aerial_dashboard.SetIgnition(2, num_ae > 2, num_ae > 2 && simbuf_ae[2].simbuf_ae_ignition);
    m_aerial_dashboard.SetIgnition(3, num_ae > 3, num_ae > 3 && simbuf_ae[3].simbuf_ae_ignition);

    //autopilot - heading
    m_aerial_dashboard.hdg.SetActive(simbuf.simbuf_ap_heading_mode == Autopilot::HEADING_FIXED);
    m_aerial_dashboard.wlv.SetActive(simbuf.simbuf_ap_heading_mode == Autopilot::HEADING_WLV);
    m_aerial_dashboard.nav.SetActive(simbuf.simbuf_ap_heading_mode == Autopilot::HEADING_NAV);

    //autopilot - altitude
    m_aerial_dashboard.alt.SetActive(simbuf.simbuf_ap_alt_mode == Autopilot::ALT_FIXED);
    m_aerial_dashboard.vs.SetActive(simbuf.simbuf_ap_alt_mode == Autopilot::ALT_VS);

    //autopilot - other buttons
    m_aerial_dashboard.ias.SetActive(simbuf.simbuf_ap_ias_mode);
    m_aerial_dashboard.gpws.SetActive(simbuf.simbuf_ap_gpws_mode);
    m_aerial_dashboard.brks.SetActive(simbuf.simbuf_parking_brake);

    //autopilot - trims
    m_aerial_dashboard.hdg_trim.DisplayFormat("%.3u", simbuf.simbuf_ap_heading_value);
    m_aerial_dashboard.alt_trim.DisplayFormat("%i00", simbuf.simbuf_ap_alt_value / 100);
    m_aerial_dashboard.ias_trim.DisplayFormat("%.3u", simbuf.simbuf_ap_ias_value);
    if (simbuf.simbuf_ap_vs_value == 0)
        m_aerial_dashboard.vs_trim.display->setCaption("000");
    else if (simbuf.simbuf_ap_vs_value < 0)
        m_aerial_dashboard.vs_trim.DisplayFormat("%i00", simbuf.simbuf_ap_vs_value / 100);
    else
        m_aerial_dashboard.vs_trim.DisplayFormat("+%i00", simbuf.simbuf_ap_vs_value / 100);
}

void OverlayWrapper::UpdateMarineHUD(Actor* vehicle)
{
    // throttles
    bthro1->setTop(thrtop + thrheight * (0.5 - vehicle->ar_screwprops[0]->getThrottle() / 2.0) - 1.0);
    if (vehicle->ar_num_screwprops > 1)
    {
        bthro2->setTop(thrtop + thrheight * (0.5 - vehicle->ar_screwprops[1]->getThrottle() / 2.0) - 1.0);
    }

    // depth
    char tmp[50] = "";
    float height = vehicle->GetHeightAboveGround();
    if (height > 0.1 && height < 99.9)
    {
        sprintf(tmp, "%2.1f", height);
        boat_depth_value_taoe->setCaption(tmp);
    }
    else
    {
        boat_depth_value_taoe->setCaption("--.-");
    }

    // water speed
    Vector3 cam_dir = vehicle->getDirection();
    Vector3 velocity = vehicle->ar_nodes[vehicle->ar_main_camera_node_pos].Velocity;
    float kt = cam_dir.dotProduct(velocity) * 1.9438;
    float angle = kt * 4.2;
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

    showDashboardOverlays(!RoR::App::GetSimController()->IsGUIHidden(), current_vehicle);
}

void OverlayWrapper::UpdateRacingGui(RoR::GfxScene* gs)
{
    float time = gs->GetSimDataBuffer().simbuf_race_time;
    UTFString txt = StringUtil::format("%.2i'%.2i.%.2i", (int)(time) / 60, (int)(time) % 60, (int)(time * 100.0) % 100);
    this->laptime->setCaption(txt);
    if (gs->GetSimDataBuffer().simbuf_race_best_time > 0.0f)
    {
        time = gs->GetSimDataBuffer().simbuf_race_best_time;
        txt = StringUtil::format("%.2i'%.2i.%.2i", (int)(time) / 60, (int)(time) % 60, (int)(time * 100.0) % 100);
        this->bestlaptime->setCaption(txt);
        this->bestlaptime->show();
    }
    else
    {
        this->bestlaptime->hide();
    }

    float time_diff = gs->GetSimDataBuffer().simbuf_race_time_diff;
    Ogre::ColourValue colour = Ogre::ColourValue::White;
    if (time_diff > 0.0f)
    {
        colour = ColourValue(0.8, 0.0, 0.0);
    }
    else if (time_diff < 0.0f)
    {
        colour = ColourValue(0.0, 0.8, 0.0);
    }
    this->laptime->setColourTop(colour);
    this->laptime->setColourBottom(colour);
}

void AeroDashOverlay::SetThrottle(int engine, bool visible, float value)
{
    if (visible)
    {
        engines[engine].thr_element->show();
        engines[engine].thr_element->setTop(
            thrust_track_top + thrust_track_height * (1.0 - value) - 1.0);
    }
    else
    {
        engines[engine].thr_element->hide();
    }
}

void AeroDashOverlay::SetEngineFailed(int engine, bool value)
{
    engines[engine].engfire_element->setMaterialName(
        value ? "tracks/engfire-on" : "tracks/engfire-off");
}

void AeroDashOverlay::SetEngineRpm(int engine, float pcent)
{
    float angle;
    if (pcent < 60.0)
        angle = -5.0 + pcent * 1.9167;
    else if (pcent < 110.0)
        angle = 110.0 + (pcent - 60.0) * 4.075;
    else
        angle = 314.0;
    engines[engine].rpm_texture->setTextureRotate(Ogre::Degree(-angle));
}

void AeroDashOverlay::SetEnginePitch(int engine, float value)
{
    engines[engine].pitch_texture->setTextureRotate(Degree(-value * 2.0));
}

void AeroDashOverlay::SetEngineTorque(int engine, float pcent)
{
    float angle;
    if (pcent < 60.0)
        angle = -5.0 + pcent * 1.9167;
    else if (pcent < 110.0)
        angle = 110.0 + (pcent - 60.0) * 4.075;
    else
        angle = 314.0;
    engines[engine].torque_texture->setTextureRotate(Degree(-angle));
}

void AeroDashOverlay::SetIgnition(int engine, bool visible, bool ignited)
{
    if (visible)
    {
        engines[engine].engstart_element->show();
        engines[engine].engstart_element->setMaterialName(
            ignited ? "tracks/engstart-on" : "tracks/engstart-off");
    }
    else
    {
        engines[engine].engstart_element->hide();
    }
}

void AeroSwitchOverlay::SetActive(bool value)
{
    element->setMaterial(value ? on_material : off_material);
}

void AeroSwitchOverlay::Setup(std::string const & elem_name, std::string const & mat_on, std::string const & mat_off)
{
    element = Ogre::OverlayManager::getSingleton().getOverlayElement(elem_name);
    on_material = Ogre::MaterialManager::getSingleton().getByName(mat_on);
    off_material = Ogre::MaterialManager::getSingleton().getByName(mat_off);
}

void AeroTrimOverlay::Setup(std::string const & up, std::string const & dn, std::string const & disp)
{
    display = Ogre::OverlayManager::getSingleton().getOverlayElement(disp);
    up_button = Ogre::OverlayManager::getSingleton().getOverlayElement(up);
    dn_button = Ogre::OverlayManager::getSingleton().getOverlayElement(dn);
}

void AeroTrimOverlay::DisplayFormat(const char* fmt, ...)
{
    char buffer[500] = {};

    va_list args;
    va_start(args, fmt);
        vsprintf(buffer, fmt, args);
    va_end(args);

    display->setCaption(buffer);
}
