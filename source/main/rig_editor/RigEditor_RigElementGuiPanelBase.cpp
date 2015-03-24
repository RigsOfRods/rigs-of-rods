/*
	This source file is part of Rigs of Rods
	Copyright 2005-2012 Pierre-Michel Ricordel
	Copyright 2007-2012 Thomas Fischer
	Copyright 2013-2015 Petr Ohlidal

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
	@date   01/2015
*/

#include "RigEditor_RigElementGuiPanelBase.h"

#include "RigEditor_Config.h"

#include <MyGUI.h>
#include <OgreColourValue.h>
#include <OgreStringConverter.h>

using namespace RoR;
using namespace RigEditor;

// ----------------------------------------------------------------------------
// Constructor
// ----------------------------------------------------------------------------

#define CONVERT_COLOR_OGRE_TO_MYGUI(OUT, IN) OUT.red = IN.r; OUT.green = IN.g; OUT.blue = IN.b; OUT.alpha = IN.a;

RigElementGuiPanelBase::RigElementGuiPanelBase(
	RigEditor::IMain* rig_editor_interface, 
	RigEditor::Config* config, 
	MyGUI::Window* panel_widget,
	MyGUI::Button* text_color_source
):
	GuiPanelBase(panel_widget)
{
	Hide(); // Start hidden

	// Setup attributes
	m_rig_editor_interface = rig_editor_interface;
	CONVERT_COLOR_OGRE_TO_MYGUI(m_text_color_mixvalues, config->gui_nodebeam_panels_field_mixvalues_color);
	CONVERT_COLOR_OGRE_TO_MYGUI(m_text_color_tooltip,  config->gui_nodebeam_panels_tooltip_text_color);
	m_text_color_default = text_color_source->getTextColour();
}

// ----------------------------------------------------------------------------
// Editbox utils
// ----------------------------------------------------------------------------

void RigElementGuiPanelBase::EditboxRestoreValue(EditboxFieldSpec* spec)
{
	// Mark uniform state
	if (spec->label != nullptr)
	{
		spec->label->setTextColour(spec->IsSourceUniform() ? m_text_color_default : m_text_color_mixvalues);
	}
	
	// Reload GUI value
	if (spec->IsSourceUniform())
	{
		if (spec->IsSourceFloat())
		{
			char caption[30];
			sprintf(caption, "%f", *spec->GetSourceFloat());
			spec->editbox->setCaption(caption); // Restore value from source
		}
		else if (spec->IsSourceInt())
		{
			char caption[30];
			sprintf(caption, "%d", *spec->GetSourceInt());
			spec->editbox->setCaption(caption); // Restore value from source
		}
		else if (spec->IsSourceString())
		{
			spec->editbox->setCaption(*spec->GetSourceString()); // Restore value from source
		}
	}
	else
	{
		spec->editbox->setCaption("");
	}
}
void RigElementGuiPanelBase::EditboxCommitValue(EditboxFieldSpec* spec)
{
	MyGUI::InputManager::getInstance().setKeyFocusWidget(nullptr); // Remove focus
	if (spec->editbox->getCaption().empty())
	{
		return;
	}
	if (spec->label != nullptr)
	{
		spec->label->setTextColour(m_text_color_default); // Mark "not uniform"
	}
	spec->SetSourceIsUniform();
	if (spec->IsSourceFloat())
	{
		*spec->GetSourceFloat() = Ogre::StringConverter::parseReal(spec->editbox->getCaption()); // Save value
	}
	else if (spec->IsSourceInt())
	{
		*spec->GetSourceInt() = Ogre::StringConverter::parseInt(spec->editbox->getCaption()); // Save value
	}
	else if (spec->IsSourceString())
	{
		*spec->GetSourceString() = spec->editbox->getCaption(); // Save value
	}
}

void RigElementGuiPanelBase::SetupEditboxField(
		EditboxFieldSpec* spec, 
		MyGUI::TextBox* label, MyGUI::EditBox* editbox, 
		unsigned int* source_flags_ptr, unsigned int uniformity_flag, 
		void* source_ptr, unsigned int source_type_flag
	)
{
	*spec = EditboxFieldSpec(label, editbox, source_flags_ptr, uniformity_flag, source_ptr, source_type_flag);
	editbox->eventKeySetFocus      += MyGUI::newDelegate(this, &RigElementGuiPanelBase::CallbackKeyFocusGained_RestorePreviousFieldValue);
	editbox->eventKeyButtonPressed += MyGUI::newDelegate(this, &RigElementGuiPanelBase::CallbackKeyPress_EnterCommitsEscapeRestores);
	editbox->setUserData(spec);
}

void RigElementGuiPanelBase::CallbackKeyFocusGained_RestorePreviousFieldValue(MyGUI::Widget* new_widget, MyGUI::Widget* old_widget)
{
	if (old_widget == nullptr)
	{
		return;
	}
	MyGUI::EditBox* editbox = old_widget->castType<MyGUI::EditBox>(false); // False = don't throw exception
	if (editbox != nullptr) // Previous widget is an editbox
	{
		EditboxFieldSpec* spec_ptr = *(editbox->getUserData<EditboxFieldSpec*>());
		EditboxRestoreValue(spec_ptr);
	}
}

void RigElementGuiPanelBase::CallbackKeyPress_EnterCommitsEscapeRestores(MyGUI::Widget* widget, MyGUI::KeyCode key_code, MyGUI::Char key_value)
{
	bool key_escape = (key_code == MyGUI::KeyCode::Escape);
	bool key_enter = (key_code == MyGUI::KeyCode::Return);
	if (!key_escape && !key_enter)
	{
		return;
	}
	EditboxFieldSpec* field_spec_ptr = *(widget->getUserData<EditboxFieldSpec*>());
	if (key_enter)
	{
		EditboxCommitValue(field_spec_ptr);
	}
	else
	{
		EditboxRestoreValue(field_spec_ptr);
	}
}

// ----------------------------------------------------------------------------
// Checkbox utils
// ----------------------------------------------------------------------------

void RigElementGuiPanelBase::SetupFlagCheckbox(
	MyGUI::Button* checkbox, 
	unsigned int* data_flags_ptr, 
	unsigned int flag_uniform, 
	unsigned int flag_value, 
	MyGUI::TextBox* tooltip_label, 
	const char* tooltip_text)
{
	checkbox->setUserData(FlagCheckboxUserData(data_flags_ptr, flag_uniform, flag_value, tooltip_text, tooltip_label));
	checkbox->eventMouseButtonClick += MyGUI::newDelegate(this, &RigElementGuiPanelBase::CallbackClick_FlagCheckboxUpdateValue);
	checkbox->eventMouseSetFocus    += MyGUI::newDelegate(this, &RigElementGuiPanelBase::CallbackGotMouseFocus_FlagCheckboxShowTooltip);
	checkbox->eventMouseLostFocus   += MyGUI::newDelegate(this, &RigElementGuiPanelBase::CallbackLostMouseFocus_FlagCheckboxClearTooltip);
}

void RigElementGuiPanelBase::UpdateFlagCheckbox(MyGUI::Button* checkbox, bool selected, bool mixed)
{
	checkbox->setStateSelected((mixed) ? false : selected);
	auto & text_color = (mixed) ? m_text_color_mixvalues : m_text_color_default;
	checkbox->setTextColour(text_color);
}

void RigElementGuiPanelBase::CallbackClick_FlagCheckboxUpdateValue(MyGUI::Widget* sender)
{
	MyGUI::Button* checkbox = sender->castType<MyGUI::Button>();
	assert(checkbox != nullptr);

	// SWITCH CHECKBOX STATE

	bool selected = !checkbox->getStateSelected();
	checkbox->setStateSelected(selected);
	if (selected)
	{
		checkbox->setTextColour(m_text_color_default); // Clear "mixed values" state.
	}

	// UPDATE VALUE

	FlagCheckboxUserData* userdata_ptr = checkbox->getUserData<FlagCheckboxUserData>();
	assert(userdata_ptr != nullptr);

	// Uniformity flag
	BITMASK_SET_1(*userdata_ptr->data_flags_ptr, userdata_ptr->uniformity_flag);

	// Value flag
	Bitmask_SetBool( selected, *userdata_ptr->data_flags_ptr, userdata_ptr->value_flag);
}

void RigElementGuiPanelBase::CallbackGotMouseFocus_FlagCheckboxShowTooltip(MyGUI::Widget* new_widget, MyGUI::Widget* old_widget)
{
	FlagCheckboxUserData* userdata_ptr = new_widget->getUserData<FlagCheckboxUserData>();
	if (userdata_ptr == nullptr)
	{
		return;
	}
	userdata_ptr->tooltip_textbox->setCaption(userdata_ptr->tooltip_text);
}

void RigElementGuiPanelBase::CallbackLostMouseFocus_FlagCheckboxClearTooltip(MyGUI::Widget* new_widget, MyGUI::Widget* old_widget)
{
	FlagCheckboxUserData* userdata_ptr = new_widget->getUserData<FlagCheckboxUserData>();
	if (userdata_ptr == nullptr)
	{
		return;
	}
	userdata_ptr->tooltip_textbox->setCaption("");
}

// ----------------------------------------------------------------------------
// Other
// ----------------------------------------------------------------------------

void RigElementGuiPanelBase::AlignToScreen(RigEditor::GuiPanelPositionData* position_data)
{
	MyGUI::IntSize screenSize = m_panel_widget->getParentSize();
	int x = position_data->margin_left_px; // Anchor: left
	int y = position_data->margin_top_px; // Anchor: top
	if (position_data->anchor_right)
	{
		x = screenSize.width - GetWidthPixels() - position_data->margin_right_px;
	}
	if (position_data->anchor_bottom)
	{
		y = screenSize.height - GetHeightPixels() - position_data->margin_bottom_px;
	}
	SetPosition(x, y);
}


