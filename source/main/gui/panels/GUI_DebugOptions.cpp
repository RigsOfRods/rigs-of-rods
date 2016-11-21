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
/// @date   12/2014

#include "GUI_DebugOptions.h"

#include "Application.h"
#include "RoRPrerequisites.h"
#include "Utils.h"
#include "RoRVersion.h"
#include "rornet.h"
#include "Language.h"
#include "MainThread.h" //Get MainMenu pointer

#include <MyGUI.h>

using namespace RoR;
using namespace GUI;

#define CLASS        DebugOptions
#define MAIN_WIDGET  ((MyGUI::Window*)mMainWidget)

CLASS::CLASS()
{
    MyGUI::WindowPtr win = dynamic_cast<MyGUI::WindowPtr>(mMainWidget);
    win->eventWindowButtonPressed += MyGUI::newDelegate(this, &CLASS::notifyWindowButtonPressed); //The "X" button thing

    MyGUI::IntSize gui_area = MyGUI::RenderManager::getInstance().getViewSize();
    mMainWidget->setPosition(gui_area.width / 2 - mMainWidget->getWidth() / 2, gui_area.height / 2 - mMainWidget->getHeight() / 2);

    //checkboxes
    m_debug_truck_mass->eventMouseButtonClick += MyGUI::newDelegate(this, &CLASS::OnDebugTruckMassCheck);
    m_debug_collision_meshes->eventMouseButtonClick += MyGUI::newDelegate(this, &CLASS::OnDebugColiMeshesCheck);
    m_ingame_console->eventMouseButtonClick += MyGUI::newDelegate(this, &CLASS::OnIngameConsoleCheck);
    m_debug_envmap->eventMouseButtonClick += MyGUI::newDelegate(this, &CLASS::OnDebugEnvMapCheck);
    m_debug_videocameras->eventMouseButtonClick += MyGUI::newDelegate(this, &CLASS::OnDebugVideoCameraCheck);
    m_trigger_debug->eventMouseButtonClick += MyGUI::newDelegate(this, &CLASS::OnDebugTriggerCheck);
    m_debug_dof->eventMouseButtonClick += MyGUI::newDelegate(this, &CLASS::OnDebugDOFCheck);
    m_beam_break_debug->eventMouseButtonClick += MyGUI::newDelegate(this, &CLASS::OnBeamBreakCheck);
    m_beam_deform_debug->eventMouseButtonClick += MyGUI::newDelegate(this, &CLASS::OnBeamDeformCheck);
    m_adv_logging->eventMouseButtonClick += MyGUI::newDelegate(this, &CLASS::OnAdvLoggingCheck);
    m_disable_crash_reporting->eventMouseButtonClick += MyGUI::newDelegate(this, &CLASS::OnCrashReportCheck);

    //buttons
    m_Save_btn->eventMouseButtonClick += MyGUI::newDelegate(this, &CLASS::eventMouseButtonClickSaveButton);

    Hide();
}

CLASS::~CLASS()
{
}

void CLASS::Show()
{
    MAIN_WIDGET->setVisibleSmooth(true);
    DebugOptionsMap = Settings::getSingleton().GetSettingMap();
    UpdateControls();
}

void CLASS::Hide()
{
    MAIN_WIDGET->setVisible(false);
}

void CLASS::UpdateControls()
{
    m_debug_truck_mass->setStateCheck(App::GetDiagTruckMass());

    m_debug_collision_meshes->setStateCheck(App::GetDiagCollisions());

    m_ingame_console->setStateCheck(Settings::getSingleton().getBooleanSetting("Enable Ingame Console", false));

    m_debug_envmap->setStateCheck(Settings::getSingleton().getBooleanSetting("EnvMapDebug", false));

    m_debug_videocameras->setStateCheck(App::GetDiagVideoCameras());

    m_trigger_debug->setStateCheck(Settings::getSingleton().getBooleanSetting("Trigger Debug", false));

    m_debug_dof->setStateCheck(Settings::getSingleton().getBooleanSetting("DOFDebug", false));

    m_beam_break_debug->setStateCheck(Settings::getSingleton().getBooleanSetting("Beam Break Debug", false));

    m_beam_deform_debug->setStateCheck(Settings::getSingleton().getBooleanSetting("Beam Deform Debug", false));

    m_adv_logging->setStateCheck(Settings::getSingleton().getBooleanSetting("Advanced Logging", false));

    m_disable_crash_reporting->setStateCheck(Settings::getSingleton().getBooleanSetting("NoCrashRpt", true));

    if (App::GetIoInputGrabMode() == App::INPUT_GRAB_DYNAMIC)
        m_input_grabing->setIndexSelected(1);
    else if (App::GetIoInputGrabMode() == App::INPUT_GRAB_NONE)
        m_input_grabing->setIndexSelected(2);
    else
        m_input_grabing->setIndexSelected(0);

    m_preselected_map->setCaption(RoR::App::GetSimNextTerrain());
    m_preselected_truck->setCaption(DebugOptionsMap["Preselected Truck"]);
}

void CLASS::notifyWindowButtonPressed(MyGUI::WidgetPtr _sender, const std::string& _name)
{
    if (_name == "close")
        Hide();
}

void CLASS::OnDebugTruckMassCheck(MyGUI::WidgetPtr _sender)
{
    m_debug_truck_mass->setStateCheck(!m_debug_truck_mass->getStateCheck());
    if (m_debug_truck_mass->getStateCheck() == false)
        DebugOptionsMap["Debug Truck Mass"] = "No";
    else
        DebugOptionsMap["Debug Truck Mass"] = "Yes";
}

void CLASS::OnDebugColiMeshesCheck(MyGUI::WidgetPtr _sender)
{
    m_debug_collision_meshes->setStateCheck(!m_debug_collision_meshes->getStateCheck());
    if (m_debug_collision_meshes->getStateCheck() == false)
        DebugOptionsMap["Debug Collisions"] = "No";
    else
        DebugOptionsMap["Debug Collisions"] = "Yes";
}

void CLASS::OnIngameConsoleCheck(MyGUI::WidgetPtr _sender)
{
    m_ingame_console->setStateCheck(!m_ingame_console->getStateCheck());
    if (m_ingame_console->getStateCheck() == false)
        DebugOptionsMap["Enable Ingame Console"] = "No";
    else
        DebugOptionsMap["Enable Ingame Console"] = "Yes";
}

void CLASS::OnDebugEnvMapCheck(MyGUI::WidgetPtr _sender)
{
    m_debug_envmap->setStateCheck(!m_debug_envmap->getStateCheck());
    if (m_debug_envmap->getStateCheck() == false)
        DebugOptionsMap["EnvMapDebug"] = "No";
    else
        DebugOptionsMap["EnvMapDebug"] = "Yes";
}

void CLASS::OnDebugVideoCameraCheck(MyGUI::WidgetPtr _sender)
{
    const bool val = !m_debug_videocameras->getStateCheck();
    m_debug_videocameras->setStateCheck(val);
    App::SetDiagVideoCameras(val);
}

void CLASS::OnDebugTriggerCheck(MyGUI::WidgetPtr _sender)
{
    m_trigger_debug->setStateCheck(!m_trigger_debug->getStateCheck());
    if (m_trigger_debug->getStateCheck() == false)
        DebugOptionsMap["Trigger Debug"] = "No";
    else
        DebugOptionsMap["Trigger Debug"] = "Yes";
}

void CLASS::OnDebugDOFCheck(MyGUI::WidgetPtr _sender)
{
    m_debug_dof->setStateCheck(!m_debug_dof->getStateCheck());
    if (m_debug_dof->getStateCheck() == false)
        DebugOptionsMap["DOFDebug"] = "No";
    else
        DebugOptionsMap["DOFDebug"] = "Yes";
}

void CLASS::OnBeamBreakCheck(MyGUI::WidgetPtr _sender)
{
    m_beam_break_debug->setStateCheck(!m_beam_break_debug->getStateCheck());
    if (m_beam_break_debug->getStateCheck() == false)
        DebugOptionsMap["Beam Break Debug"] = "No";
    else
        DebugOptionsMap["Beam Break Debug"] = "Yes";
}

void CLASS::OnBeamDeformCheck(MyGUI::WidgetPtr _sender)
{
    m_beam_deform_debug->setStateCheck(!m_beam_deform_debug->getStateCheck());
    if (m_beam_deform_debug->getStateCheck() == false)
        DebugOptionsMap["Beam Deform Debug"] = "No";
    else
        DebugOptionsMap["Beam Deform Debug"] = "Yes";
}

void CLASS::OnAdvLoggingCheck(MyGUI::WidgetPtr _sender)
{
    m_adv_logging->setStateCheck(!m_adv_logging->getStateCheck());
    if (m_adv_logging->getStateCheck() == false)
        DebugOptionsMap["Advanced Logging"] = "No";
    else
        DebugOptionsMap["Advanced Logging"] = "Yes";
}

void CLASS::OnCrashReportCheck(MyGUI::WidgetPtr _sender)
{
    m_disable_crash_reporting->setStateCheck(!m_disable_crash_reporting->getStateCheck());
    if (m_disable_crash_reporting->getStateCheck() == false)
        DebugOptionsMap["NoCrashRpt"] = "No";
    else
        DebugOptionsMap["NoCrashRpt"] = "Yes";
}

void CLASS::SaveConfig()
{
    std::map<std::string, std::string>::iterator it;
    // now save the GameSettingsMap
    for (it = DebugOptionsMap.begin(); it != DebugOptionsMap.end(); it++)
    {
        if (it->first.c_str() == "User Token" || it->first.c_str() == "User Token Hash")
            return;

        Settings::getSingleton().setSetting(it->first.c_str(), it->second.c_str()); //Avoid restarting the game in few cases.
        Settings::getSingleton().SaveSettings();
    }
}

void CLASS::SetVisible(bool v)
{
    MAIN_WIDGET->setVisible(false);
}

bool CLASS::IsVisible()
{
    return MAIN_WIDGET->getVisible();
}

void CLASS::eventMouseButtonClickSaveButton(MyGUI::WidgetPtr _sender)
{
    SaveConfig();
    Hide();
}
