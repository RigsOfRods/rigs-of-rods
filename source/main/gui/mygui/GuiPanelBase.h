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
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Rigs of Rods. If not, see <http://www.gnu.org/licenses/>.
*/

/** 
	@file   
	@author Petr Ohlidal
	@date   12/2014
*/

#pragma once

namespace RoR
{

class GuiPanelBase
{
	friend class RoR::GUIManager;

public:
	GuiPanelBase(MyGUI::Window* main_widget):
		m_panel_widget(main_widget)
	{}

	void Show()            { m_panel_widget->setVisible(true); }
	void Hide()            { m_panel_widget->setVisible(false); }
	bool IsVisible() const { return m_panel_widget->isVisible(); }

	MyGUI::IntSize GetSizePixels() const { return m_panel_widget->getSize(); }
	int GetWidthPixels() const           { return GetSizePixels().width; }
	int GetHeightPixels() const          { return GetSizePixels().height; }
	
	void SetPosition(int x_pixels, int y_pixels)
	{
		m_panel_widget->setPosition(x_pixels, y_pixels);
	}
	
	void CenterToScreen()
	{
		MyGUI::IntSize parentSize = m_panel_widget->getParentSize();
		SetPosition((parentSize.width - GetWidthPixels()) / 2, (parentSize.height - GetHeightPixels()) / 2);
	}

	void SetWidth(int width_pixels)   { m_panel_widget->setSize(width_pixels, GetHeightPixels()); }
	void SetHeight(int height_pixels) { m_panel_widget->setSize(GetWidthPixels(), height_pixels); }

protected:
	MyGUI::Window* m_panel_widget;
};

} // namespace RoR
