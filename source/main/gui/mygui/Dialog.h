/*
	This source file is part of Rigs of Rods
	Copyright 2005-2012 Pierre-Michel Ricordel
	Copyright 2007-2012 Thomas Fischer
	Copyright 2013-2014 Petr Ohlidal

	For more information, see http://www.rigsofrods.org/

	Rigs of Rods is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License version 3, as
	published by the Free Software Foundation.

	Rigs of Rods is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Rigs of Rods. If not, see <http://www.gnu.org/licenses/>.
*/

/*
	This file was ported from MyGUI project (MIT licensed)
	https://github.com/MyGUI/mygui
	http://mygui.info/
*/

/*!
	@file
	@author		Albert Semenov
	@date		09/2008
*/

#pragma once

#include <MyGUI.h>
#include "BaseLayout.h"

namespace RoR
{

namespace GUI
{
	
class Dialog :
	public wraps::BaseLayout
{
public:
	typedef MyGUI::delegates::CDelegate2<Dialog*, bool> EventHandle_Result;

public:
	Dialog();
	Dialog(const std::string& _layout);
	virtual ~Dialog();

	void doModal();
	void endModal();
	bool isModal()
	{
		return mModal;
	}

	EventHandle_Result eventEndDialog;

protected:
	virtual void onDoModal() { }
	virtual void onEndModal() { }

private:
	bool mModal;
};

} // namespace GUI

} // namespace RoR

