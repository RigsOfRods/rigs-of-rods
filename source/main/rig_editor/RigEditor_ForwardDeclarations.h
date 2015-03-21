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
	@file   RigEditor_ForwardDeclarations.h
	@date   09/2014
	@author Petr Ohlidal
*/

#pragma once

namespace RoR
{

	namespace RigEditor
	{
		class  Beam;
		class  BeamTypeCommandHydro;
		class  BeamTypeGenerated;
		class  BeamTypePlain;
		class  BeamTypeShock;
		class  BeamTypeSteeringHydro;
		class  CameraHandler;
		class  CineCamera;
		struct Config;
		class  FlexBodyWheel;
		struct GuiPanelPositionData;
		class  GuiPopupDynamicListBase;
		class  GuiPopupWheelsList;
		class  HighlightBoxesDynamicMesh;
		class  IMain;
		class  InputHandler;
		class  LandVehicleWheel;
		class  Main;
		class  MeshWheel;
		class  MeshWheel2;
		class  Module;
		class  Node;
		class  Rig;
		class  RigProperties;
		class  RigWheelVisuals;

		// Rig aggregate data
		struct MixedBeamsAggregateData;
		struct RigAggregateNodesData;
		struct RigAggregatePlainBeamsData;
		struct RigAggregateShocksData;
		struct RigAggregateShocks2Data;
		struct RigAggregateCommands2Data;
		struct RigAggregateHydrosData;
		struct RigAggregateBeams2Data;

	} // namespace RigEditor

	namespace GUI
	{
		class  RigEditorBeamsPanel;
		class  RigEditorCommands2Panel;
		class  RigEditorDeleteMenu;
		class  RigEditorHelpWindow;
		class  RigEditorHydrosPanel;
		class  RigEditorLandVehiclePropertiesWindow;
		class  RigEditorMenubar;
		class  RigEditorNodePanel;
		class  RigEditorRigPropertiesWindow;
		class  RigEditorShocksPanel;
		class  RigEditorShocks2Panel;

	} // namespace GUI

} // namespace RoR
