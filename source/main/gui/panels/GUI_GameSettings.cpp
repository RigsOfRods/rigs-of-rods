/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2013+     Petr Ohlidal & contributors

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
/// @author Moncef Ben Slimane
/// @date   11/2014

#include "GUI_GameSettings.h"

#include <Ogre.h>

#include "Application.h"
#include "ContentManager.h"
#include "FileSystemInfo.h"
#include "GUIManager.h"
#include "GlobalEnvironment.h"
#include "ImprovedConfigFile.h"
#include "Language.h"
#include "MainMenu.h"
#include "OgreSubsystem.h"
#include "RoRPrerequisites.h"
#include "RoRVersion.h"
#include "Settings.h"
#include "TerrainManager.h"
#include "Utils.h"
#include "RoRnet.h"

#include <MyGUI.h>

#ifdef USE_OPENAL
#ifdef __APPLE__
    #include <OpenAL/al.h>
    #include <OpenAL/alc.h>
    #include <OpenAL/MacOSX_OALExtensions.h>
#else
#include <AL/al.h>
#include <AL/alc.h>
#include <AL/alext.h>
#endif // __APPLE__
#endif // USE_OPENAL

using namespace RoR;
using namespace GUI;

#define CLASS        GameSettings
#define MAIN_WIDGET  ((MyGUI::Window*)mMainWidget)

#ifdef _WIN32
#define strncasecmp _strnicmp
#endif

static const char* OGRECFG_RENDERSYS = "Render System";
static const char* OGRECFG_FULLSCREEN = "Full Screen";
static const char* OGRECFG_VSYNC = "VSync";
static const char* OGRECFG_VIDEO_MODE = "Video Mode";
static const char* OGRECFG_FSAA = "FSAA";

#define INIT_CHECKBOX(_VAR_)        _VAR_->eventMouseButtonClick += MyGUI::newDelegate(this, &CLASS::OnCheckboxPlain);
#define INIT_CHECKBOX_NOTICE(_VAR_) _VAR_->eventMouseButtonClick += MyGUI::newDelegate(this, &CLASS::OnCheckboxRestartNotice);

CLASS::CLASS():
    m_is_initialized(false),
    m_is_keymap_loaded(false),
    isFrameActivated(false),
    ShowRestartNotice(false)
{
    MyGUI::WindowPtr win = dynamic_cast<MyGUI::WindowPtr>(mMainWidget);
    win->eventWindowButtonPressed += MyGUI::newDelegate(this, &CLASS::notifyWindowButtonPressed); //The "X" button thing
    m_key_mapping_window->eventWindowButtonPressed += MyGUI::newDelegate(this, &CLASS::notifyWindowButtonPressed); //The "X" button thing

    //Buttons
    m_savebtn->eventMouseButtonClick += MyGUI::newDelegate(this, &CLASS::eventMouseButtonClickSaveButton);
    m_regen_cache->eventMouseButtonClick += MyGUI::newDelegate(this, &CLASS::eventMouseButtonClickRegenCache);
    m_clear_cache->eventMouseButtonClick += MyGUI::newDelegate(this, &CLASS::eventMouseButtonClickClearCache);

    // Checkboxes
    INIT_CHECKBOX_NOTICE(m_disable_multithreading)
    INIT_CHECKBOX_NOTICE(m_fullscreen)

    INIT_CHECKBOX(m_arc_mode )
    INIT_CHECKBOX(m_d_cam_pitch )
    INIT_CHECKBOX(m_d_creak_sound )
    INIT_CHECKBOX(m_vsync )
    INIT_CHECKBOX(m_sh_pf_opti )
    INIT_CHECKBOX(m_e_waves )
    INIT_CHECKBOX(m_psystem )
    INIT_CHECKBOX(m_heathaze )
    INIT_CHECKBOX(m_mirrors )
    INIT_CHECKBOX(m_mblur )
    INIT_CHECKBOX(m_hq_ref )
    INIT_CHECKBOX(m_hdr )
    INIT_CHECKBOX(m_dof )
    INIT_CHECKBOX(m_skidmarks )
    INIT_CHECKBOX(m_glow )
    INIT_CHECKBOX(m_sunburn )
    INIT_CHECKBOX(m_disable_inter_collsion )
    INIT_CHECKBOX(m_disable_intra_collision)
    INIT_CHECKBOX(m_digital_speedo )
    INIT_CHECKBOX(m_enable_replay )
    INIT_CHECKBOX(m_hq_screenshots )
    INIT_CHECKBOX(m_autohide_chatbox )
    INIT_CHECKBOX(m_flexbodies_lods )
    INIT_CHECKBOX(m_flexbody_cache_system )
    INIT_CHECKBOX(m_main_menu_music )

    // Key mapping
    m_tabCtrl->eventTabChangeSelect += MyGUI::newDelegate(this, &CLASS::OnTabChange);
    m_keymap_group->eventComboChangePosition += MyGUI::newDelegate(this, &CLASS::OnKeymapTypeChange);
    // FIXME: m_change_key->eventMouseButtonClick += MyGUI::newDelegate(this, &CLASS::OnReMapPress);
    startCounter = false;

    //Sliders
    m_volume_slider->eventScrollChangePosition += MyGUI::newDelegate(this, &CLASS::OnVolumeSlider);
    m_fps_limiter_slider->eventScrollChangePosition += MyGUI::newDelegate(this, &CLASS::OnFPSLimiterSlider);
    m_sightrange->eventScrollChangePosition += MyGUI::newDelegate(this, &CLASS::OnSightRangeSlider);

    MyGUI::IntSize gui_area = MyGUI::RenderManager::getInstance().getViewSize();
    mMainWidget->setPosition(gui_area.width / 2 - mMainWidget->getWidth() / 2, gui_area.height / 2 - mMainWidget->getHeight() / 2);
    m_key_mapping_window->setPosition(gui_area.width / 2 - m_key_mapping_window->getWidth() / 2, gui_area.height / 2 - m_key_mapping_window->getHeight() / 2);

    MAIN_WIDGET->setVisible(false);
}

CLASS::~CLASS()
{
}

void CLASS::Show()
{
    MAIN_WIDGET->setVisible(true);
    this->CheckAndInit();
    this->UpdateControls();
    ShowRestartNotice = false;
}

void CLASS::Hide(bool isMenu)
{
    MAIN_WIDGET->setVisibleSmooth(false);
    App::GetGuiManager()->SetVisible_GameMainMenu(true);
}

void CLASS::notifyWindowButtonPressed(MyGUI::WidgetPtr _sender, const std::string& _name)
{
    if (_sender == mMainWidget)
    {
        if (_name == "close")
        {
            this->Hide();
        }
    }
    else if (_sender == m_key_mapping_window)
    {
        if (_name == "close")
        {
            if (startCounter)
                startCounter = false;

            if (isFrameActivated)
                MyGUI::Gui::getInstance().eventFrameStart -= MyGUI::newDelegate(this, &CLASS::FrameEntered);

            m_key_mapping_window->setVisibleSmooth(false);
        }
    }
}

void CLASS::eventMouseButtonClickSaveButton(MyGUI::WidgetPtr _sender)
{
    SaveSettings();

    if (ShowRestartNotice == true)
    {
        RoR::App::GetGuiManager()->ShowMessageBox("Restart required", "You need to restart the game for few settings to apply. You can still play and restart next time, but the game can glitch out.", true, "Ok", true, false, "");
    }

    this->Hide();
}

void CLASS::CheckAndInit()
{
    if (m_is_initialized) { return; }

    // #### Hide un-implemented features ####

    // Delete input mapping tab (tabs cannot be hidden: http://www.ogre3d.org/addonforums/viewtopic.php?f=17&t=13475)
    m_tabCtrl->removeItem(m_inputmap_tab);

    m_lang->setVisible(false);
    m_lang_label->setVisible(false);

    m_mirrors->setVisible(false); // Checkbox
    m_autohide_chatbox->setVisible(false); // Checkbox

    // #### Init game UI entries ####

    m_gearbox_mode->addItem(CONF_GEARBOX_AUTO); // Automatic shift
    m_gearbox_mode->addItem(CONF_GEARBOX_SEMIAUTO); // Manual shift - Auto clutch
    m_gearbox_mode->addItem(CONF_GEARBOX_MANUAL); // Fully Manual: sequential shift
    m_gearbox_mode->addItem(CONF_GEARBOX_MAN_STICK); // Fully Manual: stick shift
    m_gearbox_mode->addItem(CONF_GEARBOX_MAN_RANGES); // Fully Manual: stick shift with ranges

    m_water_type->addItem("Hydrax");
    m_shadow_type->addItem("Parallel-split Shadow Maps");

    if (!BSETTING("DevMode", false))
    {
        m_heathaze->setEnabled(false);
        m_glow->setEnabled(false);
        m_sunburn->setEnabled(false);
    }

    // Sliders
    m_volume_slider->setScrollRange(101);
    m_fps_limiter_slider->setScrollRange(200);
    m_sightrange->setScrollRange(5000);

    // Things that aren't ready to be used yet.
    if (BSETTING("DevMode", false))
    {
        m_sky_type->addItem("SkyX (best looking, slower)");
    }
    else if (App::GetGfxSkyMode() == App::GFX_SKY_SKYX) // Safety check
    {
        App::SetGfxSkyMode(App::GFX_SKY_SANDSTORM);
    }

    // Audio Devices
    int audio_selection = 0;
    char* devices = (char *)alcGetString(NULL, ALC_ALL_DEVICES_SPECIFIER);
    while (devices && *devices != 0)
    {
        m_audio_dev->addItem(Ogre::String(devices));

        if (Ogre::String(devices) == App::GetAudioDeviceName())
            m_audio_dev->setIndexSelected(audio_selection);

        devices += strlen(devices) + 1; //next device
        audio_selection++;
    }

    // #### Init OGRE UI entries ####

    Ogre::RenderSystem* rs_active = Ogre::Root::getSingleton().getRenderSystem();
    const std::string rs_name = rs_active->getName();
    const auto rs_list = Ogre::Root::getSingleton().getAvailableRenderers();

    int rs_index = 0;
    for (auto itor = rs_list.begin(); itor != rs_list.end(); ++itor)
    {
        auto itor_name = (*itor)->getName();
        m_render_sys->addItem(itor_name);
        if (itor_name == rs_name)
        {
            m_render_sys->setIndexSelected(rs_index);
        }
        ++rs_index;
    }

    auto render_itor = rs_active->getConfigOptions().begin();
    auto render_end = rs_active->getConfigOptions().end();
    for (; render_itor != render_end; ++render_itor)
    {
        if (render_itor->first == OGRECFG_VIDEO_MODE)
        {
            // Fill 'Video mode' listbox & select active entry
            auto vid_modes = render_itor->second;
            int sel_index = 0;
            for (auto itor = vid_modes.possibleValues.begin(); itor != vid_modes.possibleValues.end(); ++itor)
            {
                m_resolution->addItem(*itor);
                if (*itor == vid_modes.currentValue)
                {
                    m_resolution->setIndexSelected(sel_index);
                }
                ++sel_index;
            }
        }
        else if (render_itor->first == OGRECFG_FSAA)
        {
            // Fill 'FSAA' listbox & select active entry
            auto fsaa_modes = render_itor->second;
            int sel_index = 0;
            for (auto itor = fsaa_modes.possibleValues.begin(); itor != fsaa_modes.possibleValues.end(); ++itor)
            {
                m_fsaa->addItem(*itor);
                if (*itor == fsaa_modes.currentValue)
                {
                    m_fsaa->setIndexSelected(sel_index);
                }
                ++sel_index;
            }
        }
    }

    // #### Final tasks ####
    m_ogre_cfg.load(App::GetSysConfigDir() + PATH_SLASH + "ogre.cfg");
    m_is_initialized = true;
}

static const char* CONF_SHADOWS_FAST = "Shadow optimizations";
static const char* CONF_GFX_DOF = "DOF";
static const char* CONF_GFX_MBLUR = "Motion blur";
static const char* CONF_DIGI_SPEEDO = "DigitalSpeedo";
static const char* CONF_NO_COLLISION = "DisableCollisions";
static const char* CONF_NO_SELF_COLLISION = "DisableSelfCollisions";
static const char* CONF_FLEXBODY_LOD = "Flexbody_EnableLODs";
static const char* CONF_FLEXBODY_CACHE = "Flexbody_UseCache";

void CLASS::UpdateControls()
{
    int valuecounter = 0; // Going to be usefull for selections

    // Gearbox listbox
    switch (App::GetSimGearboxMode())
    {
    case App::SIM_GEARBOX_AUTO: m_gearbox_mode->setIndexSelected(0);
        break;
    case App::SIM_GEARBOX_SEMI_AUTO: m_gearbox_mode->setIndexSelected(1);
        break;
    case App::SIM_GEARBOX_MANUAL: m_gearbox_mode->setIndexSelected(2);
        break;
    case App::SIM_GEARBOX_MANUAL_STICK: m_gearbox_mode->setIndexSelected(3);
        break;
    case App::SIM_GEARBOX_MANUAL_RANGES: m_gearbox_mode->setIndexSelected(4);
        break;
    }

    // Arcade controls checkbox
    m_arc_mode->setStateCheck(App::GetIoArcadeControls());

    // Camera pitch checkbox
    m_d_cam_pitch->setStateCheck(App::GetGfxExternCamMode() == App::GFX_EXTCAM_MODE_PITCHING);

    // Creak sound checkbox
    m_d_creak_sound->setStateCheck(App::GetAudioEnableCreak());

    // FOV
    m_fovexternal->setCaption(TOSTRING(App::GetGfxFovExternal()));
    m_fovinternal->setCaption(TOSTRING(App::GetGfxFovInternal()));

    // ogre.cfg
    std::string rs_name = Ogre::Root::getSingleton().getRenderSystem()->getName();
    m_fullscreen->setStateCheck(m_ogre_cfg.GetBool(OGRECFG_FULLSCREEN, rs_name, false));
    m_vsync->setStateCheck(m_ogre_cfg.GetBool(OGRECFG_VSYNC, rs_name, false));

    // Texture Filtering
    switch (App::GetGfxTexFiltering())
    {
    case App::GFX_TEXFILTER_NONE: m_tex_filter->setIndexSelected(0);
        break;
    case App::GFX_TEXFILTER_BILINEAR: m_tex_filter->setIndexSelected(1);
        break;
    case App::GFX_TEXFILTER_TRILINEAR: m_tex_filter->setIndexSelected(2);
        break;
    case App::GFX_TEXFILTER_ANISOTROPIC: m_tex_filter->setIndexSelected(3);
        break;
    }

    switch (App::GetGfxSkyMode())
    {
    case App::GFX_SKY_SANDSTORM: m_sky_type->setIndexSelected(0);
        break; // Sandstorm
    case App::GFX_SKY_CAELUM: m_sky_type->setIndexSelected(1);
        break; // Caelum
    case App::GFX_SKY_SKYX: m_sky_type->setIndexSelected(BSETTING("DevMode", false) ? 2 : 0);
        break; // SkyX, experimental
    }

    switch (App::GetGfxShadowType())
    {
    case App::GFX_SHADOW_TYPE_NONE: m_shadow_type->setIndexSelected(0);
        break;
    case App::GFX_SHADOW_TYPE_TEXTURE: m_shadow_type->setIndexSelected(1);
        break;
    case App::GFX_SHADOW_TYPE_PSSM: m_shadow_type->setIndexSelected(2);
        break;
    }

    switch (App::GetGfxWaterMode())
    {
    case App::GFX_WATER_BASIC: m_water_type->setIndexSelected(0);
        break;
    case App::GFX_WATER_REFLECT: m_water_type->setIndexSelected(1);
        break;
    case App::GFX_WATER_FULL_FAST: m_water_type->setIndexSelected(2);
        break;
    case App::GFX_WATER_FULL_HQ: m_water_type->setIndexSelected(3);
        break;
    case App::GFX_WATER_HYDRAX: m_water_type->setIndexSelected(4);
        break;
    }

    switch (App::GetGfxVegetationMode())
    {
    case App::GFX_VEGETATION_NONE: m_vegetation->setIndexSelected(0);
        break;
    case App::GFX_VEGETATION_20PERC: m_vegetation->setIndexSelected(1);
        break;
    case App::GFX_VEGETATION_50PERC: m_vegetation->setIndexSelected(2);
        break;
    case App::GFX_VEGETATION_FULL: m_vegetation->setIndexSelected(3);
        break;
    }

    switch (App::GetGfxFlaresMode())
    {
    case App::GFX_FLARES_NONE: m_light_source_effects->setIndexSelected(0);
        break;
    case App::GFX_FLARES_NO_LIGHTSOURCES: m_light_source_effects->setIndexSelected(1);
        break;
    case App::GFX_FLARES_CURR_VEHICLE_HEAD_ONLY: m_light_source_effects->setIndexSelected(2);
        break;
    case App::GFX_FLARES_ALL_VEHICLES_HEAD_ONLY: m_light_source_effects->setIndexSelected(3);
        break;
    case App::GFX_FLARES_ALL_VEHICLES_ALL_LIGHTS: m_light_source_effects->setIndexSelected(4);
        break;
    }

    // Speed units
    if (SSETTING("SpeedUnit", "Metric") == "Metric")
        m_speed_unit->setIndexSelected(1);
    else
        m_speed_unit->setIndexSelected(0);

    // Other
    m_psystem->setStateCheck(App::GetGfxParticlesMode() == 1);
    m_heathaze->setStateCheck(App::GetGfxUseHeathaze());
    m_sunburn->setStateCheck(App::GetGfxEnableSunburn());
    m_hdr->setStateCheck(App::GetGfxEnableHdr());
    m_skidmarks->setStateCheck(App::GetGfxSkidmarksMode() == 1);
    m_hq_ref->setStateCheck(App::GetGfxEnvmapEnabled());
    m_glow->setStateCheck(App::GetGfxEnableGlow());
    m_e_waves->setStateCheck(App::GetGfxWaterUseWaves());
    m_disable_multithreading->setStateCheck(!App::GetAppMultithread());
    m_enable_replay->setStateCheck(App::GetSimReplayEnabled());
    m_hq_screenshots->setStateCheck(App::GetAppScreenshotFormat() == "png");
    m_main_menu_music->setStateCheck(App::GetAudioMenuMusic());

    m_sh_pf_opti->setStateCheck(BSETTING("Shadow optimizations", true ));
    m_dof->setStateCheck(BSETTING("DOF", false));
    m_mblur->setStateCheck(BSETTING("Motion blur", false));
    m_digital_speedo->setStateCheck(BSETTING("DigitalSpeedo", false));
    m_disable_inter_collsion->setStateCheck(BSETTING("DisableCollisions", false));
    m_disable_intra_collision->setStateCheck(BSETTING("DisableSelfCollisions",false));
    m_flexbodies_lods->setStateCheck(BSETTING("Flexbody_EnableLODs", true ));
    m_flexbody_cache_system->setStateCheck(BSETTING("Flexbody_UseCache", false));

    // Volume slider
    float sound_volume = App::GetAudioMasterVolume();
    m_volume_slider->setScrollPosition(sound_volume - 1);
    if (m_volume_slider->getScrollPosition() >= 100)
        m_volume_indicator->setCaption("100%");
    else
        m_volume_indicator->setCaption(Ogre::StringConverter::toString(sound_volume) + "%");

    // FPS limit slider
    m_fps_limiter_slider->setScrollPosition(App::GetGfxFpsLimit() - 1);
    if (App::GetGfxFpsLimit() >= 199)
        m_fps_limiter_indicator->setCaption("Unlimited");
    else
        m_fps_limiter_indicator->setCaption(TOSTRING(App::GetGfxFpsLimit()) + " FPS");

    // SightRange slider
    int sight_range = static_cast<int>(App::GetGfxSightRange());
    m_sightrange->setScrollPosition(sight_range - 1);
    if (sight_range >= TerrainManager::UNLIMITED_SIGHTRANGE)
        m_sightrange_indicator->setCaption("Unlimited");
    else
        m_sightrange_indicator->setCaption(TOSTRING(sight_range) + " m");

    if (ISETTING("SkidmarksBuckets", 5) == 5)
        m_skidmarks_quality->setIndexSelected(1);
    else
        m_skidmarks_quality->setIndexSelected(0);
}

// ====================================
// Checkboxes

void CLASS::OnCheckboxPlain(MyGUI::WidgetPtr _sender)
{
    MyGUI::Button* chk = _sender->castType<MyGUI::Button>(); // Checkboxes are MyGUI::Button
    chk->setStateCheck(!chk->getStateCheck());
}

void CLASS::OnCheckboxRestartNotice(MyGUI::WidgetPtr _sender)
{
    this->OnCheckboxPlain(_sender);
    ShowRestartNotice = true;
}

// ====================================
// Sliders

void CLASS::OnVolumeSlider(MyGUI::ScrollBar* _sender, size_t _position)
{
    if (m_volume_slider->getScrollPosition() >= 100)
        m_volume_indicator->setCaption("100%");
    else
        m_volume_indicator->setCaption(TOSTRING(_position) + "%");
}

void CLASS::OnFPSLimiterSlider(MyGUI::ScrollBar* _sender, size_t _position)
{
    if (_position >= 199)
        m_fps_limiter_indicator->setCaption("Unlimited");
    else
        m_fps_limiter_indicator->setCaption(TOSTRING(_position) + " FPS");
}

void CLASS::OnSightRangeSlider(MyGUI::ScrollBar* _sender, size_t _position)
{
    if (_position >= 4999)
        m_sightrange_indicator->setCaption("Unlimited");
    else
        m_sightrange_indicator->setCaption(TOSTRING(_position) + " m");
}

// ====================================
// Update settings

#define _(_STR_) _STR_
#define F(_STR_) static_cast<float>(Ogre::StringConverter::parseReal(_STR_))

#define CONF_SET_BOOL(_KEY_, _VAL_) App::GetSettings().setSetting(_KEY_, _VAL_ ? "Yes" : "No");

void CLASS::SaveSettings()
{
    // Setting which need restart
    if (m_audio_dev->getCaption() != App::GetAudioDeviceName()) { ShowRestartNotice = true; }
    if (m_ogre_cfg.GetString(OGRECFG_RENDERSYS) != m_render_sys->getCaption().asUTF8()) { ShowRestartNotice = true; }
    if (m_ogre_cfg.GetString(OGRECFG_VIDEO_MODE) != m_resolution->getCaption().asUTF8()) { ShowRestartNotice = true; }
    if (m_ogre_cfg.GetString(OGRECFG_FSAA) != m_fsaa->getCaption().asUTF8()) { ShowRestartNotice = true; }

    // Combo boxes
    App__SetSimGearboxMode(_(m_gearbox_mode ->getCaption()));
    App::SetGfxFovExternal(F(m_fovexternal ->getCaption()));
    App::SetGfxFovInternal(F(m_fovinternal ->getCaption()));
    App__SetTexFiltering(_(m_tex_filter ->getCaption()));
    App__SetGfxSkyMode(_(m_sky_type ->getCaption()));
    App__SetShadowTech(_(m_shadow_type ->getCaption()));
    App__SetGfxWaterMode(_(m_water_type ->getCaption()));
    App__SetVegetationMode(_(m_vegetation ->getCaption()));
    App__SetGfxFlaresMode(_(m_light_source_effects ->getCaption()));
    App::SetAudioDeviceName(_(m_audio_dev ->getCaption()));

    App::GetSettings().setSetting("SpeedUnit", m_speed_unit->getCaption());
    App::SetAudioMasterVolume(m_volume_slider->getScrollPosition());

    // Checkboxes
    App::SetGfxParticlesMode((int)m_psystem->getStateCheck());
    App::SetGfxUseHeathaze(m_heathaze->getStateCheck());
    App::SetGfxEnableSunburn(m_sunburn->getStateCheck());
    App::SetGfxEnableHdr(m_hdr->getStateCheck());
    App::SetGfxSkidmarksMode((int)m_skidmarks->getStateCheck());
    App::SetGfxEnvmapEnabled(m_hq_ref->getStateCheck());
    App::SetGfxEnableGlow(m_glow->getStateCheck());
    App::SetGfxWaterUseWaves(m_e_waves->getStateCheck());
    App::SetAppMultithread(m_disable_multithreading->getStateCheck());
    App::SetSimReplayEnabled(m_enable_replay->getStateCheck());
    App::SetAppScreenshotFormat(m_hq_screenshots->getStateCheck() ? "png" : "jpg");
    App::SetAudioMenuMusic(m_main_menu_music->getStateCheck());
    App::SetIoArcadeControls(m_arc_mode->getStateCheck());
    App::SetGfxExternCamMode(m_d_cam_pitch->getStateCheck() ? App::GFX_EXTCAM_MODE_PITCHING : App::GFX_EXTCAM_MODE_STATIC);
    App::SetAudioEnableCreak(m_d_creak_sound->getStateCheck());

    CONF_SET_BOOL(CONF_SHADOWS_FAST , m_sh_pf_opti ->getStateCheck());
    CONF_SET_BOOL(CONF_GFX_DOF , m_dof ->getStateCheck());
    CONF_SET_BOOL(CONF_GFX_MBLUR , m_mblur ->getStateCheck());
    CONF_SET_BOOL(CONF_DIGI_SPEEDO , m_digital_speedo ->getStateCheck());
    CONF_SET_BOOL(CONF_NO_COLLISION , m_disable_inter_collsion ->getStateCheck());
    CONF_SET_BOOL(CONF_NO_SELF_COLLISION, m_disable_intra_collision ->getStateCheck());
    CONF_SET_BOOL(CONF_FLEXBODY_LOD , m_flexbodies_lods ->getStateCheck());
    CONF_SET_BOOL(CONF_FLEXBODY_CACHE , m_flexbody_cache_system ->getStateCheck());

    // Ogre.cfg values
    m_ogre_cfg.SetString(OGRECFG_RENDERSYS, m_render_sys->getCaption());
    m_ogre_cfg.SetString(OGRECFG_VIDEO_MODE, m_resolution->getCaption());
    m_ogre_cfg.SetString(OGRECFG_FSAA, m_fsaa->getCaption());
    m_ogre_cfg.SetString(OGRECFG_VSYNC, m_vsync->getStateCheck() ? "Yes" : "No");

    // Adjustments
    if (App::GetGfxWaterMode() == App::GFX_WATER_HYDRAX) { App::SetGfxSightRange(5000.f); }

    /* @only-a-ptr Wut?
    if (m_skidmarks_quality->getCaption() == "Normal")
        App::GetSettings().setSetting("SpeedUnit", "0");
    else
        App::GetSettings().setSetting("SpeedUnit", "5");
    */

    if (m_is_keymap_loaded)
    {
        App::GetInputEngine()->saveMapping("input.map", RoR::App::GetOgreSubsystem()->GetMainHWND());
    }

    // Save ogre's settings
    Ogre::RenderSystem* rs = Ogre::Root::getSingleton().getRenderSystem();
    auto itor = m_ogre_cfg.getSettingsIterator();
    while (itor.hasMoreElements())
    {
        try
        {
            rs->setConfigOption(itor.current()->first, itor.current()->second);
        }
        catch (...)
        {
            LOG("Error setting Ogre Values");
        }
        itor.moveNext();
    }
    Ogre::String err = rs->validateConfigOptions();
    if (err.length() > 0)
    {
        LOG("Ogre config validation error");
    }
    else
    {
        Ogre::Root::getSingleton().saveConfig();
    }

    // ******* TODO ******** Re-implement

    //Apply fullscreen
    //Not working correctly
    /*
    if (BSETTING("DevMode", false)) //let's disable this for now
    {
        if (OgreSettingsMap["Full Screen"].c_str() != ExOgreSettingsMap["Full Screen"].c_str())
        {
            Ogre::StringVector args = Ogre::StringUtil::split(OgreSettingsMap["Video Mode"], " ");

            static int org_width = -1, org_height = -1;
            int width = RoR::App::GetOgreSubsystem()->GetRenderWindow()->getWidth();
            int height = RoR::App::GetOgreSubsystem()->GetRenderWindow()->getHeight();
            if (org_width == -1)
                org_width = width;
            if (org_height == -1)
                org_height = height;
            bool mode = RoR::App::GetOgreSubsystem()->GetRenderWindow()->isFullScreen();
            if (!mode)
            {
                RoR::App::GetOgreSubsystem()->GetRenderWindow()->setFullscreen(true, (int)args[0].c_str(), (int)args[2].c_str());
                LOG(" ** switched to fullscreen: " + TOSTRING(width) + "x" + TOSTRING(height));
            }
            else
            {
                RoR::App::GetOgreSubsystem()->GetRenderWindow()->setFullscreen(false, 640, 480);
                RoR::App::GetOgreSubsystem()->GetRenderWindow()->setFullscreen(false, org_width, org_height);
                LOG(" ** switched to windowed mode: ");
            }
            ShowRestartNotice = false;
        }
    }*/
}

#undef _
#undef S

void CLASS::eventMouseButtonClickRegenCache(MyGUI::WidgetPtr _sender)
{
    MAIN_WIDGET->setVisibleSmooth(false);
    App::GetContentManager()->RegenCache();
    MAIN_WIDGET->setVisibleSmooth(true);
    RoR::App::GetGuiManager()->ShowMessageBox("Cache regenerated", "Cache regenerated succesfully!", true, "Ok", true, false, "");
}

void CLASS::OnTabChange(MyGUI::TabControl* _sender, size_t _index)
{
    MyGUI::TabItemPtr tab = _sender->getItemAt(_index);
    if (!tab)
        return;

    //## Disabled until keymapping is fixed
    //if (_index == 6)
    //	LoadKeyMap();
}

void CLASS::LoadKeyMap()
{
    //Cleanup first
    m_keymapping->removeAllItems();

    KeyMap = App::GetInputEngine()->getEvents();

    std::map<int, std::vector<event_trigger_t>>::iterator it;

    int counter = 0;
    char curGroup[128] = "";
    std::map<int, std::vector<event_trigger_t>>::iterator mapIt;
    std::vector<event_trigger_t>::iterator vecIt;
    for (mapIt = KeyMap.begin(); mapIt != KeyMap.end(); mapIt++)
    {
        std::vector<event_trigger_t> vec = mapIt->second;

        for (vecIt = vec.begin(); vecIt != vec.end(); vecIt++ , counter++)
        {
            if (strcmp(vecIt->group, curGroup))
            {
                strncpy(curGroup, vecIt->group, 128);
                // group title:
                m_keymap_group->addItem(vecIt->group);
            }

            if (m_keymap_group->getCaption() == "")
                m_keymap_group->setIndexSelected(0); //at least select something

            std::string prefix = m_keymap_group->getCaption();
            if (strncasecmp(App::GetInputEngine()->eventIDToName(mapIt->first).c_str(), prefix.c_str(), prefix.length()))
            {
                m_keymapping->addItem(App::GetInputEngine()->eventIDToName(mapIt->first).c_str());
                m_keymapping->setSubItemNameAt(1, m_keymapping->getItemCount() - 1, vecIt->configline);
            }

            //m_key_name->
            // print event name
            //fprintf(f, "%-30s ", eventIDToName(mapIt->first).c_str());
            // print event type
            //fprintf(f, "%-20s ", getEventTypeName(vecIt->eventtype).c_str());

            if (vecIt->eventtype == ET_Keyboard)
            {
                //fprintf(f, "%s ", vecIt->configline);
            }
        }
    }
    m_is_keymap_loaded = true;
    m_keymapping->setIndexSelected(0);
}

void CLASS::OnKeymapTypeChange(MyGUI::ComboBox* _sender, size_t _index)
{
    if (_sender->getCaption() == "")
        return;

    //Cleanup first
    m_keymapping->removeAllItems();

    std::map<int, std::vector<event_trigger_t>>::iterator it;

    int counter = 0;
    std::map<int, std::vector<event_trigger_t>>::iterator mapIt;
    std::vector<event_trigger_t>::iterator vecIt;
    for (mapIt = KeyMap.begin(); mapIt != KeyMap.end(); mapIt++)
    {
        std::vector<event_trigger_t> vec = mapIt->second;

        for (vecIt = vec.begin(); vecIt != vec.end(); vecIt++ , counter++)
        {
            std::string prefix = _sender->getItemNameAt(_index);
            if (strncasecmp(App::GetInputEngine()->eventIDToName(mapIt->first).c_str(), prefix.c_str(), prefix.length()))
            {
                m_keymapping->addItem(App::GetInputEngine()->eventIDToName(mapIt->first).c_str());
                m_keymapping->setSubItemNameAt(1, m_keymapping->getItemCount() - 1, vecIt->configline);
            }
        }
    }
    m_keymapping->setIndexSelected(0);
}

void CLASS::eventMouseButtonClickClearCache(MyGUI::WidgetPtr _sender)
{
    // List files in cache
    RoR::FileSystem::VectorFileInfo cache_files;
    std::string cache_dir_str = App::GetSysCacheDir() + PATH_SLASH;
    std::wstring cache_dir_wstr = MyGUI::UString(cache_dir_str).asWStr();
    RoR::FileSystem::getSystemFileList(cache_files, cache_dir_wstr, L"*.*");

    // Remove files
    std::for_each(cache_files.begin(), cache_files.end(), [cache_dir_wstr](RoR::FileSystem::FileInfo& file_info)
        {
            MyGUI::UString path_to_delete = RoR::FileSystem::concatenatePath(cache_dir_wstr, file_info.name);
            std::remove(path_to_delete.asUTF8_c_str());
        });

    ShowRestartNotice = true;
    RoR::App::GetGuiManager()->ShowMessageBox("Cache cleared", "Cache cleared succesfully, you need to restart the game for the changes to apply.", true, "Ok", true, false, "");
}

/* FIXME
void CLASS::OnReMapPress(MyGUI::WidgetPtr _sender)
{
		Ogre::String str_text = "";
		str_text += "Press any button/Move your joystick axis to map it to this event. \nYou can also close this window to cancel the mapping. \n\n";
		str_text += "#66FF33 Event: #FFFFFF" + m_keymapping->getSubItemNameAt(0, m_keymapping->getItemIndexSelected()) + "\n";
		str_text += "#66FF33 Current Key: #FFFFFF" + m_keymapping->getSubItemNameAt(1, m_keymapping->getItemIndexSelected());
		m_key_mapping_window->setCaptionWithReplacing("Assign new key");
		m_key_mapping_window_text->setCaptionWithReplacing(str_text);
		m_key_mapping_window->setVisibleSmooth(true);

		MyGUI::Gui::getInstance().eventFrameStart += MyGUI::newDelegate(this, &CLASS::FrameEntered);
		isFrameActivated = true;

		m_key_mapping_window_info->setCaptionWithReplacing("");
		mMainWidget->setEnabledSilent(true);

		str_text = "";
}
*/

void CLASS::FrameEntered(float dt)
{
    unsigned long Timer = Ogre::Root::getSingleton().getTimer()->getMilliseconds();

    if (RoR::App::GetInputEngine()->isKeyDown(OIS::KC_RETURN) || RoR::App::GetInputEngine()->isKeyDown(OIS::KC_ESCAPE))
    {
        MyGUI::Gui::getInstance().eventFrameStart -= MyGUI::newDelegate(this, &CLASS::FrameEntered);
        isFrameActivated = false;

        m_key_mapping_window->setVisibleSmooth(false);
        return;
    }

    std::string combo;
    int keys = RoR::App::GetInputEngine()->getCurrentKeyCombo(&combo);
    if (keys != 0)
    {
        endTime = Timer + 5000;
        startCounter = true;
        LastKeyCombo = Ogre::String(combo.c_str());

        Ogre::String str_text = "";
        str_text += "Press any button/Move your joystick axis to map it to this event. \nYou can also close this window to cancel the mapping. \n\n";
        str_text += "#66FF33 Event: #FFFFFF" + m_keymapping->getSubItemNameAt(0, m_keymapping->getItemIndexSelected()) + "\n";
        str_text += "#66FF33 Current Key: #FFFFFF" + m_keymapping->getSubItemNameAt(1, m_keymapping->getItemIndexSelected()) + "\n";
        str_text += "#66FF33 New Key: #FFFFFF" + LastKeyCombo;
        m_key_mapping_window_text->setCaptionWithReplacing(str_text);

        str_text = "";
    }

    if (startCounter)
    {
        long timer1 = Timer - endTime;
        m_key_mapping_window_info->setCaptionWithReplacing("Changes will apply in: " + Ogre::StringConverter::toString(-timer1) + " Seconds");
        if (timer1 == 0)
        {
            m_keymapping->setSubItemNameAt(1, m_keymapping->getItemIndexSelected(), LastKeyCombo);

            startCounter = false;
            LastKeyCombo = "";

            MyGUI::Gui::getInstance().eventFrameStart -= MyGUI::newDelegate(this, &CLASS::FrameEntered);
            isFrameActivated = false;

            m_key_mapping_window->setVisibleSmooth(false);
            return;
        }
    }
}

void CLASS::SetVisible(bool v)
{
    if (v) { this->Show(); }
    else { this->Hide(); }
}

bool CLASS::IsVisible() { return MAIN_WIDGET->getVisible(); }
