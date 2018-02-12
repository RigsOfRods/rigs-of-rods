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

// ##############################
//            DUMMY!
// This GUI never worked right.
// It's temporarily replaced by a dummy
// until reworked using DearIMGUI. ~ only_a_ptr, 06/2017
// ##############################

/// @file
/// @author Moncef Ben Slimane
/// @date   12/2014

#include "GUI_DebugOptions.h"

#include "Application.h"
#include "RoRPrerequisites.h"
#include "Utils.h"
#include "RoRVersion.h"
#include "RoRnet.h"
#include "Language.h"
#include "MainMenu.h" //Get MainMenu pointer

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
    m_debug_truck_mass->eventMouseButtonClick += MyGUI::newDelegate(this, &CLASS::OnDebugActorMassCheck);
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

}

void CLASS::Hide()
{
    MAIN_WIDGET->setVisible(false);
}

void CLASS::UpdateControls()
{
    
}

void CLASS::notifyWindowButtonPressed(MyGUI::WidgetPtr _sender, const std::string& _name)
{
    if (_name == "close")
        Hide();
}

void CLASS::OnDebugActorMassCheck(MyGUI::WidgetPtr _sender)
{

}

void CLASS::OnDebugColiMeshesCheck(MyGUI::WidgetPtr _sender)
{

}

void CLASS::OnIngameConsoleCheck(MyGUI::WidgetPtr _sender)
{

}

void CLASS::OnDebugEnvMapCheck(MyGUI::WidgetPtr _sender)
{

}

void CLASS::OnDebugVideoCameraCheck(MyGUI::WidgetPtr _sender)
{

}

void CLASS::OnDebugTriggerCheck(MyGUI::WidgetPtr _sender)
{

}

void CLASS::OnDebugDOFCheck(MyGUI::WidgetPtr _sender)
{

}

void CLASS::OnBeamBreakCheck(MyGUI::WidgetPtr _sender)
{

}

void CLASS::OnBeamDeformCheck(MyGUI::WidgetPtr _sender)
{

}

void CLASS::OnAdvLoggingCheck(MyGUI::WidgetPtr _sender)
{

}

void CLASS::OnCrashReportCheck(MyGUI::WidgetPtr _sender)
{

}

void CLASS::SaveConfig()
{

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
