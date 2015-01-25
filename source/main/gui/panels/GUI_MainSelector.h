/*
	This source file is part of Rigs of Rods
	Copyright 2005-2012 Pierre-Michel Ricordel
	Copyright 2007-2012 Thomas Fischer
	Copyright 2013-2014 Petr Ohlidal

	For more information, see http://www.rigsofrods.com/

	Rigs of Rods is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License version 3, as
	published by the Free Software Foundation.

	Rigs of Rods is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Rigs of Rods.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

/** 
	@file   GUI_MultiplayerSelector.h
	@author Moncef Ben Slimane
	@date   11/2014
*/

#include "ForwardDeclarations.h"
#include "GUI_MainSelectorLayout.h"


namespace RoR
{

namespace GUI
{

class MainSelector : public MainSelectorLayout
{

public:
	MainSelector();
	~MainSelector();

	void setupCamera(Ogre::Camera* _camera) { mCamera = _camera; }

	bool isFinishedSelecting();
	void show(LoaderType type);
	void hide();
	bool IsVisible();
	void BackToMenu();

	CacheEntry *getSelection() { return mSelectedTruck; }
	Skin *getSelectedSkin() { return mSelectedSkin; }
	std::vector<Ogre::String> getTruckConfig() { return mTruckConfigs; }
	void setEnableCancel(bool enabled);
	int ConvertType(LoaderType type) { return LoaderType(type); }
	LoaderType getLoaderType() { return LoaderType(); }
private:

	void notifyWindowButtonPressed(MyGUI::WidgetPtr _sender, const std::string& _name);

	// gui events
	void eventComboAcceptConfigComboBox(MyGUI::ComboBoxPtr _sender, size_t _index);
	void eventComboChangePositionTypeComboBox(MyGUI::ComboBoxPtr _sender, size_t _index);
	void eventKeyButtonPressed_Main(MyGUI::WidgetPtr _sender, MyGUI::KeyCode _key, MyGUI::Char _char);
	void eventListChangePositionModelList(MyGUI::ListPtr _sender, size_t _index);
	void eventListChangePositionModelListAccept(MyGUI::ListPtr _sender, size_t _index);
	void eventMouseButtonClickCancelButton(MyGUI::WidgetPtr _sender);
	void eventMouseButtonClickOkButton(MyGUI::WidgetPtr _sender);
	void eventSearchTextChange(MyGUI::EditBox *_sender);
	void eventSearchTextGotFocus(MyGUI::WidgetPtr _sender, MyGUI::WidgetPtr oldWidget);
	void notifyWindowChangeCoord(MyGUI::Window* _sender);
	void resizePreviewImage();
	void bindKeys(bool bind = true);

	// other functions
	void getData();
	void onCategorySelected(int categoryID);
	void onEntrySelected(int entryID);
	void selectionDone();
	bool searchCompare(Ogre::String searchString, CacheEntry *ce);

	void updateControls(CacheEntry *entry);
	void setPreviewImage(Ogre::String texture);
	void frameEntered(float dt);
	CacheEntry *mSelectedTruck;
	LoaderType mLoaderType;
	Ogre::Camera *mCamera;
	Ogre::String mPreviewImageTexture;
	Skin *mSelectedSkin;
	bool mSelectionDone;
	bool ready;
	int visibleCounter;
	std::vector<CacheEntry> mEntries;
	std::vector<Ogre::String> mTruckConfigs;
	std::vector<Skin *> mCurrentSkins;
	bool keysBound;
	float readytime;
	float dtsum;

};

} // namespace GUI

} // namespace RoR
