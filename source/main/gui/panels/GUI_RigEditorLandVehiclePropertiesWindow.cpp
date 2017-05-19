/*
    This source file is part of Rigs of Rods
    Copyright 2013-2016 Petr Ohlidal & contributors

    For more information, see http://www.rigsofrods.com/

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

/** 
    @file   
    @author Petr Ohlidal
    @date   10/2014
*/

#include "GUI_RigEditorLandVehiclePropertiesWindow.h"
#include "RigDef_File.h"
#include "RigEditor_Main.h"
#include "RigEditor_RigProperties.h"
#include "RoRPrerequisites.h"

#include <MyGUI.h>

using namespace RoR;
using namespace GUI;
using namespace RigDef;

#define CLASS         RigEditorLandVehiclePropertiesWindow
#define MAIN_WIDGET   m_land_vehicle_properties_window

CLASS::CLASS(RigEditor::IMain* rig_editor_interface):
    GuiPanelBase(MAIN_WIDGET),
    m_forward_gears_textbox_empty(true)
{
    m_rig_editor_interface = rig_editor_interface;

    // "Forward gears" long editbox
    m_editbox_placeholder_color = m_editbox_more_gear_ratios->getTextColour();
    m_gears_textbox_placeholder_text = m_editbox_more_gear_ratios->getCaption();
    m_editbox_more_gear_ratios->eventKeySetFocus  += MyGUI::newDelegate(this, &CLASS::ForwardGearsEditboxKeyFocusGained);
    m_editbox_more_gear_ratios->eventKeyLostFocus += MyGUI::newDelegate(this, &CLASS::ForwardGearsEditboxKeyFocusLost);

    // [Cancel] button
    m_button_cancel->eventMouseButtonClick += MyGUI::newDelegate(this, &CLASS::CancelButtonClicked);
    // [Save] button
    m_button_save->eventMouseButtonClick += MyGUI::newDelegate(this, &CLASS::SaveButtonClicked);

    // Close window [X] button
    MyGUI::Window* main_window = MAIN_WIDGET->castType<MyGUI::Window>();
    main_window->eventWindowButtonPressed += MyGUI::newDelegate(this, &CLASS::WindowButtonClicked);

    // Engine type radio button
    m_radiobutton_engine_truck->eventMouseButtonClick += MyGUI::newDelegate(this, &CLASS::EngineTypeRadioClicked);
    m_radiobutton_engine_car->eventMouseButtonClick   += MyGUI::newDelegate(this, &CLASS::EngineTypeRadioClicked);

    // Add/remove engine buttons
    m_add_engine_button   ->eventMouseButtonClick += MyGUI::newDelegate(this, &CLASS::AddRemoveButtonsClicked);
    m_remove_engine_button->eventMouseButtonClick += MyGUI::newDelegate(this, &CLASS::AddRemoveButtonsClicked);

    Hide();
}

void CLASS::SetVehicleHasEngine(bool has_engine)
{
    m_edit_engine_tab->setEnabled(has_engine == true);
    m_edit_engine_tab->setVisible(has_engine == true);
    m_add_engine_tab->setEnabled(has_engine == false);
    m_add_engine_tab->setVisible(has_engine == false);
    m_vehicle_has_engine = has_engine;
    m_radiobutton_engine_truck->setStateSelected(true);
}

void CLASS::WindowButtonClicked(MyGUI::Widget* sender, const std::string& name)
{
    Hide(); // There's only close [X] button -> hide window.
}

void CLASS::CancelButtonClicked(MyGUI::Widget* sender)
{
    Hide();
}

void CLASS::SaveButtonClicked(MyGUI::Widget* sender)
{
    m_rig_editor_interface->CommandSaveLandVehiclePropertiesWindowData();
    Hide();
}

void CLASS::ResetEngineEngoptionForm()
{
    // SECTION "ENGINE"

    m_editbox_shift_down_rpm->setCaption("");
    m_editbox_shift_up_rpm->setCaption("");

    m_editbox_torque->setCaption("");

    // Gearbox
    m_editbox_global_gear_ratio ->setCaption("");
    m_editbox_reverse_gear_ratio->setCaption("");
    m_editbox_neutral_gear_ratio->setCaption("");
    m_editbox_first_gear_ratio  ->setCaption("");

    // Engine type
    m_radiobutton_engine_truck->setStateSelected(true);
    m_radiobutton_engine_car->setStateSelected(false);

    // Gearbox - forward gear list
    m_editbox_more_gear_ratios->setCaption("");

    // SECTION "ENGOPTION"
    m_editbox_engoption_engine_inertia  ->setCaption("");
    m_editbox_engoption_clutch_force    ->setCaption("");
    m_editbox_engoption_shift_time      ->setCaption("");
    m_editbox_engoption_clutch_time     ->setCaption("");
    m_editbox_engoption_post_shift_time ->setCaption("");
    m_editbox_engoption_stall_rpm       ->setCaption("");
    m_editbox_engoption_idle_rpm        ->setCaption("");
    m_editbox_engoption_max_idle_mixture->setCaption("");
    m_editbox_engoption_min_idle_mixture->setCaption("");
}

void CLASS::Import(
    std::shared_ptr<RigDef::Engine> engine,
    std::shared_ptr<RigDef::Engoption> engoption
    )
{
    if (!engine || !engoption)
    {
        this->SetVehicleHasEngine(false);
        return;
    }
    this->SetVehicleHasEngine(true);

    // SECTION "ENGINE"

    m_editbox_shift_down_rpm->setCaption(TOSTRING(engine->shift_down_rpm));
    m_editbox_shift_up_rpm->setCaption(TOSTRING(engine->shift_up_rpm));

    m_editbox_torque->setCaption(TOSTRING(engine->torque));

    // Gearbox
    m_editbox_global_gear_ratio ->setCaption(TOSTRING(engine->global_gear_ratio));
    m_editbox_reverse_gear_ratio->setCaption(TOSTRING(engine->reverse_gear_ratio));
    m_editbox_neutral_gear_ratio->setCaption(TOSTRING(engine->neutral_gear_ratio));
    m_editbox_first_gear_ratio  ->setCaption(TOSTRING(engine->gear_ratios[0]));

    // Gearbox - forward gear list
    auto itor     = engine->gear_ratios.begin() + 1;
    auto itor_end = engine->gear_ratios.end();
    std::ostringstream ratios_line;
    for (; itor != itor_end; ++itor)
    {
        m_forward_gears_textbox_empty = false;
        ratios_line << *itor << " ";
    }
    m_editbox_more_gear_ratios->setCaption(ratios_line.str());

    // SECTION "ENGOPTION"
    m_editbox_engoption_engine_inertia  ->setCaption(TOSTRING(engoption->inertia));
    m_editbox_engoption_clutch_force    ->setCaption(TOSTRING(engoption->clutch_force));
    m_editbox_engoption_shift_time      ->setCaption(TOSTRING(engoption->shift_time));
    m_editbox_engoption_clutch_time     ->setCaption(TOSTRING(engoption->clutch_time));
    m_editbox_engoption_post_shift_time ->setCaption(TOSTRING(engoption->post_shift_time));
    m_editbox_engoption_stall_rpm       ->setCaption(TOSTRING(engoption->stall_rpm));
    m_editbox_engoption_idle_rpm        ->setCaption(TOSTRING(engoption->idle_rpm));
    m_editbox_engoption_max_idle_mixture->setCaption(TOSTRING(engoption->max_idle_mixture));
    m_editbox_engoption_min_idle_mixture->setCaption(TOSTRING(engoption->min_idle_mixture));
    
    // Engine type
    assert(engoption->type == Engoption::ENGINE_TYPE_c_CAR || engoption->type == Engoption::ENGINE_TYPE_t_TRUCK);
    this->SetEngineType(engoption->type);
}

std::shared_ptr<RigDef::Engine>    CLASS::ExportEngine()
{
    if (!m_vehicle_has_engine)
    {
        return std::shared_ptr<RigDef::Engine>();
    }
    auto engine = std::shared_ptr<RigDef::Engine>(new RigDef::Engine());

    engine->shift_down_rpm   = PARSEREAL(m_editbox_shift_down_rpm->getCaption());
    engine->shift_up_rpm     = PARSEREAL(m_editbox_shift_up_rpm->getCaption());
    engine->torque           = PARSEREAL(m_editbox_torque->getCaption());

    // Gearbox
    engine->global_gear_ratio  = PARSEREAL(m_editbox_global_gear_ratio->getCaption());
    engine->reverse_gear_ratio = PARSEREAL(m_editbox_reverse_gear_ratio->getCaption());
    engine->neutral_gear_ratio = PARSEREAL(m_editbox_neutral_gear_ratio->getCaption());
    engine->gear_ratios.push_back(PARSEREAL(m_editbox_first_gear_ratio->getCaption()));

    if (! m_forward_gears_textbox_empty)
    {
        // NOTE: Whitespace-only validation is done by onFocus* events.

        Ogre::String gears_str = m_editbox_more_gear_ratios->getCaption();
        Ogre::StringUtil::trim(gears_str);
        Ogre::StringVector tokens = Ogre::StringUtil::split(gears_str, " ");
        for (auto itor = tokens.begin(); itor != tokens.end(); ++itor)
        {
            engine->gear_ratios.push_back(PARSEREAL(*itor));
        }
    }
    return engine;
}

std::shared_ptr<RigDef::Engoption> CLASS::ExportEngoption()
{
    if (!m_vehicle_has_engine)
    {
        return std::shared_ptr<RigDef::Engoption>();
    }
    auto engoption = std::shared_ptr<RigDef::Engoption>(new RigDef::Engoption());

    engoption->inertia           = PARSEREAL(m_editbox_engoption_engine_inertia  ->getCaption());
    engoption->clutch_force      = PARSEREAL(m_editbox_engoption_clutch_force    ->getCaption());
    engoption->shift_time        = PARSEREAL(m_editbox_engoption_shift_time      ->getCaption());
    engoption->clutch_time       = PARSEREAL(m_editbox_engoption_clutch_time     ->getCaption());
    engoption->post_shift_time   = PARSEREAL(m_editbox_engoption_post_shift_time ->getCaption());
    engoption->stall_rpm         = PARSEREAL(m_editbox_engoption_stall_rpm       ->getCaption());
    engoption->idle_rpm          = PARSEREAL(m_editbox_engoption_idle_rpm        ->getCaption());
    engoption->max_idle_mixture  = PARSEREAL(m_editbox_engoption_max_idle_mixture->getCaption());
    engoption->min_idle_mixture  = PARSEREAL(m_editbox_engoption_min_idle_mixture->getCaption());

    // Engine type
    if (m_radiobutton_engine_car->getStateSelected())
    {
        engoption->type = Engoption::ENGINE_TYPE_c_CAR;
    }
    else if (m_radiobutton_engine_truck->getStateSelected())
    {
        engoption->type = Engoption::ENGINE_TYPE_t_TRUCK;
    }
    else
    {
        assert(false && "GUI/LandVehicleProperties: Invalid value of 'engine type' radio group");
    }
    return engoption;
}

void CLASS::SetEngineType(RigDef::Engoption::EngineType type)
{
    m_radiobutton_engine_car->setStateSelected(type == Engoption::ENGINE_TYPE_c_CAR);
    m_radiobutton_engine_truck->setStateSelected(type == Engoption::ENGINE_TYPE_t_TRUCK);
    if (type != Engoption::ENGINE_TYPE_c_CAR && type != Engoption::ENGINE_TYPE_t_TRUCK)
    {
        // Default state
        m_radiobutton_engine_truck->setStateSelected(true);
        m_radiobutton_engine_car->setStateSelected(false);
    }
}

void CLASS::ForwardGearsEditboxKeyFocusGained(MyGUI::Widget* sender, MyGUI::Widget* _)
{
    if (m_forward_gears_textbox_empty)
    {
        m_editbox_more_gear_ratios->setCaption("");
        m_editbox_more_gear_ratios->setTextColour(MyGUI::Colour::White);
    }
}

void CLASS::ForwardGearsEditboxKeyFocusLost(MyGUI::Widget* sender, MyGUI::Widget* _)
{
    Ogre::String line = m_editbox_more_gear_ratios->getCaption();
    Ogre::StringUtil::trim(line);
    m_forward_gears_textbox_empty = line.empty();
    if (line.empty())
    {
        m_editbox_more_gear_ratios->setCaption(m_gears_textbox_placeholder_text);
        m_editbox_more_gear_ratios->setTextColour(m_editbox_placeholder_color);
    }
}

void CLASS::EngineTypeRadioClicked(MyGUI::Widget* sender)
{
    m_radiobutton_engine_truck->setStateSelected(sender == m_radiobutton_engine_truck);
    m_radiobutton_engine_car  ->setStateSelected(sender == m_radiobutton_engine_car);
}

void CLASS::AddRemoveButtonsClicked(MyGUI::Widget* sender)
{
    if (sender == m_add_engine_button)
    {
        this->SetVehicleHasEngine(true);
        this->ResetEngineEngoptionForm();
    }
    else if (sender == m_remove_engine_button)
    {
        this->SetVehicleHasEngine(false);
    }
}
