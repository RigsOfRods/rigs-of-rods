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
	@date		08/2008
*/

#include "Dialog.h"

using namespace RoR;
using namespace GUI;

Dialog::Dialog() :
	wraps::BaseLayout(),
	mModal(false)
{
}

Dialog::Dialog(const std::string& _layout) :
	wraps::BaseLayout(_layout),
	mModal(false)
{
}

Dialog::~Dialog()
{
}

void Dialog::doModal()
{
	MYGUI_ASSERT(mModal != true, "Already modal mode");
	mModal = true;

	MyGUI::InputManager::getInstance().addWidgetModal(mMainWidget);
	MyGUI::LayerManager::getInstance().upLayerItem(mMainWidget);

	onDoModal();

	mMainWidget->setVisible(true);
}

void Dialog::endModal()
{
	MYGUI_ASSERT(mModal != false, "Already modal mode");
	mModal = false;

	mMainWidget->setVisible(false);

	MyGUI::InputManager::getInstance().removeWidgetModal(mMainWidget);

	onEndModal();
}

