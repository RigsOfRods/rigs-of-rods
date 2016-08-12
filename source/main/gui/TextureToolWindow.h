/*
This source file is part of Rigs of Rods
Copyright 2005-2012 Pierre-Michel Ricordel
Copyright 2007-2012 Thomas Fischer

For more information, see http://www.rigsofrods.org/

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
#ifdef USE_MYGUI

#pragma once
#ifndef __TextureToolWindow_H_
#define __TextureToolWindow_H_

#include "RoRPrerequisites.h"
#include "Singleton.h"
#include "mygui/BaseLayout.h"

ATTRIBUTE_CLASS_LAYOUT(TextureToolWindow, "TextureTool.layout");

class TextureToolWindow : public wraps::BaseLayout, public RoRSingleton<TextureToolWindow>, public ZeroedMemoryAllocator
{
	friend class RoRSingleton<TextureToolWindow>;

public:

	TextureToolWindow();
	~TextureToolWindow();

	void show();
	void hide();

private:

	ATTRIBUTE_FIELD_WIDGET_NAME(TextureToolWindow, mTxt, "txtInfo");
	MyGUI::TextBox* mTxt;

	ATTRIBUTE_FIELD_WIDGET_NAME(TextureToolWindow, mCBo, "cboTextures");
	MyGUI::ComboBox* mCBo;

	ATTRIBUTE_FIELD_WIDGET_NAME(TextureToolWindow, mBtnSavePNG, "btnSavePNG");
	MyGUI::Button* mBtnSavePNG;

	ATTRIBUTE_FIELD_WIDGET_NAME(TextureToolWindow, mBtnSaveRAW, "btnSaveRAW");
	MyGUI::Button* mBtnSaveRAW;

	ATTRIBUTE_FIELD_WIDGET_NAME(TextureToolWindow, mImage, "imgTexture");
	MyGUI::ImageBox* mImage;

	ATTRIBUTE_FIELD_WIDGET_NAME(TextureToolWindow, mChkDynamic, "chkDynamic");
	MyGUI::Button* mChkDynamic;
	
	void notifyWindowPressed(MyGUI::Window* _widget, const std::string& _name);
	void eventClickSavePNGButton(MyGUI::WidgetPtr _sender);
	void eventClickSaveRAWButton(MyGUI::WidgetPtr _sender);
	void eventClickDynamicButton(MyGUI::WidgetPtr _sender);
	void eventSelectTexture(MyGUI::WidgetPtr _sender);
	void eventSelectTexture2(MyGUI::ComboBoxPtr _sender, size_t _index);

	void updateControls(Ogre::String texName);
	void saveTexture( Ogre::String texName, bool png );
	void fillCombo();
};

#endif // __TextureToolWindow_H_
#endif // USE_MYGUI
