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

#pragma once

/** 
	@file   
	@author Petr Ohlidal
	@date   12/2014
*/

#include "ForwardDeclarations.h"
#include "GUI_RigEditorHydrosPanelLayout.h"
#include "RigEditor_ForwardDeclarations.h"
#include "RigEditor_RigElementGuiPanelBase.h"
#include "RigEditor_IMain.h"
#include "RigEditor_RigElementsAggregateData.h"

namespace RoR
{

namespace GUI
{

class RigEditorHydrosPanel: public RigEditorHydrosPanelLayout, public RigEditor::RigElementGuiPanelBase
{

public:

	RigEditorHydrosPanel(RigEditor::IMain* rig_editor_interface, RigEditor::Config* config);

	void UpdateHydrosData(RigEditor::RigAggregateHydrosData* query_result);

	inline const RigEditor::RigAggregateHydrosData* GetHydrosData() { return &m_data; }

	inline void HideAndReset()
	{
		Hide();
		m_data.Reset();
	}

private:

	// Aggregate rig data
	RigEditor::RigAggregateHydrosData m_data;

	// GUI form fields
	RigEditor::RigElementGuiPanelBase::EditboxFieldSpec m_extension_factor_field;
	RigEditor::RigElementGuiPanelBase::EditboxFieldSpec m_start_delay_field;
	RigEditor::RigElementGuiPanelBase::EditboxFieldSpec m_stop_delay_field;
	RigEditor::RigElementGuiPanelBase::EditboxFieldSpec m_start_function_field; 
	RigEditor::RigElementGuiPanelBase::EditboxFieldSpec m_stop_function_field;  
	RigEditor::RigElementGuiPanelBase::EditboxFieldSpec m_detacher_group_field;  
};

} // namespace GUI

} // namespace RoR
