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
	@file   GUI_GameAbout.cpp
	@author Moncef Ben Slimane
	@date   11/2014
*/

#include "GUI_MainSelector.h"

#include "RoRPrerequisites.h"
#include "Utils.h"
#include "RoRVersion.h"
#include "rornet.h"
#include "Language.h"
#include "GUIManager.h"
#include "Application.h"
#include "CacheSystem.h"
#include "SkinManager.h"
#include "LoadingWindow.h"
#include "RoRFrameListener.h"

#include <MyGUI.h>


using namespace RoR;
using namespace GUI;

#define CLASS        MainSelector
#define MAIN_WIDGET  ((MyGUI::Window*)mMainWidget)

CLASS::CLASS() :
dtsum(0)
, keysBound(false)
, mSelectedSkin(0)
, mSelectedTruck(0)
, mSelectionDone(true)
, ready(false)
, readytime(1.0f)
, visibleCounter(0)
{
	MAIN_WIDGET->setVisible(false);

	MyGUI::WindowPtr win = dynamic_cast<MyGUI::WindowPtr>(mMainWidget);
	win->eventWindowButtonPressed += MyGUI::newDelegate(this, &CLASS::notifyWindowButtonPressed); //The "X" button thing
	win->eventWindowChangeCoord += MyGUI::newDelegate(this, &CLASS::notifyWindowChangeCoord);

	MyGUI::IntSize windowSize = MAIN_WIDGET->getSize();
	MyGUI::IntSize parentSize = MAIN_WIDGET->getParentSize();

	m_Type->eventComboChangePosition += MyGUI::newDelegate(this, &CLASS::eventComboChangePositionTypeComboBox);

	m_Model->eventListSelectAccept += MyGUI::newDelegate(this, &CLASS::eventListChangePositionModelListAccept);
	m_Model->eventListChangePosition += MyGUI::newDelegate(this, &CLASS::eventListChangePositionModelList);
	m_Config->eventComboAccept += MyGUI::newDelegate(this, &CLASS::eventComboAcceptConfigComboBox);
	m_Ok->eventMouseButtonClick += MyGUI::newDelegate(this, &CLASS::eventMouseButtonClickOkButton);
	m_Cancel->eventMouseButtonClick += MyGUI::newDelegate(this, &CLASS::eventMouseButtonClickCancelButton);

	// search stuff
	m_SearchLine->eventEditTextChange += MyGUI::newDelegate(this, &CLASS::eventSearchTextChange);
	m_SearchLine->eventMouseSetFocus += MyGUI::newDelegate(this, &CLASS::eventSearchTextGotFocus);
	m_SearchLine->eventKeySetFocus += MyGUI::newDelegate(this, &CLASS::eventSearchTextGotFocus);

	readytime = 0.5f;

	MAIN_WIDGET->setPosition((parentSize.width - windowSize.width) / 2, (parentSize.height - windowSize.height) / 2);
	
	//From old file
	MAIN_WIDGET->setCaption(_L("Loader"));

	m_SearchLine->setCaption(_L("Search ..."));
	m_Ok->setCaption(_L("OK"));
	m_Cancel->setCaption(_L("Cancel"));

	// setup controls
	m_Config->addItem("Default", Ogre::String("Default"));
	m_Config->setIndexSelected(0);

	MAIN_WIDGET->setRealPosition(0.1, 0.1);
	MAIN_WIDGET->setRealSize(0.8, 0.8);
}

CLASS::~CLASS()
{

}

void CLASS::reset()
{
	dtsum = 0;
	keysBound = false;
	mSelectedSkin = 0;
	mSelectedTruck = 0;
	mSelectionDone = true;
	ready = false;
	readytime = 1.0f;
	visibleCounter = 0;
}

void CLASS::frameEntered(float dt)
{
	if (dtsum < readytime)
	{
		dtsum += dt;
	}
	else
	{
		readytime = 0;
		dtsum = 0;
		ready = true;
		MyGUI::Gui::getInstance().eventFrameStart -= MyGUI::newDelegate(this, &CLASS::frameEntered);
	}
}

void CLASS::bindKeys(bool bind)
{
	if (bind && !keysBound)
	{
		MAIN_WIDGET->eventKeyButtonPressed += MyGUI::newDelegate(this, &CLASS::eventKeyButtonPressed_Main);
		m_Type->eventKeyButtonPressed += MyGUI::newDelegate(this, &CLASS::eventKeyButtonPressed_Main);
		m_SearchLine->eventKeyButtonPressed += MyGUI::newDelegate(this, &CLASS::eventKeyButtonPressed_Main);
		keysBound = true;
	}
	else if (!bind && keysBound)
	{
		MAIN_WIDGET->eventKeyButtonPressed -= MyGUI::newDelegate(this, &CLASS::eventKeyButtonPressed_Main);
		m_Type->eventKeyButtonPressed -= MyGUI::newDelegate(this, &CLASS::eventKeyButtonPressed_Main);
		m_SearchLine->eventKeyButtonPressed -= MyGUI::newDelegate(this, &CLASS::eventKeyButtonPressed_Main);
		keysBound = false;
	}
}

void CLASS::notifyWindowChangeCoord(MyGUI::Window* _sender)
{
	resizePreviewImage();
}

void CLASS::eventKeyButtonPressed_Main(MyGUI::WidgetPtr _sender, MyGUI::KeyCode _key, MyGUI::Char _char)
{
	if (!ready || !mMainWidget->getVisible()) return;
	int cid = (int)m_Type->getIndexSelected();
	int iid = (int)m_Model->getIndexSelected();

	bool searching = (m_Type->getCaption() == _L("Search Results"));

	// search
	if (_key == MyGUI::KeyCode::Slash)
	{
		MyGUI::InputManager::getInstance().setKeyFocusWidget(m_SearchLine);
		m_SearchLine->setCaption("");
		searching = true;
	}


	// category
	if (!searching && (_key == MyGUI::KeyCode::ArrowLeft || _key == MyGUI::KeyCode::ArrowRight))
	{
		int newitem = cid;

		if (_key == MyGUI::KeyCode::ArrowLeft)
		{
			newitem--;
			if (cid == 0)
			{
				newitem = (int)m_Type->getItemCount() - 1;
			}
		}
		else
		{
			newitem++;
			if (cid == (int)m_Type->getItemCount() - 1)
			{
				newitem = 0;
			}
		}

		try
		{
			m_Type->setIndexSelected(newitem);
			m_Type->beginToItemSelected();
		}
		catch (...)
		{
			return;
		}
		eventComboChangePositionTypeComboBox(m_Type, newitem);
	}
	else if (_key == MyGUI::KeyCode::ArrowUp || _key == MyGUI::KeyCode::ArrowDown)
	{
		int newitem = iid;

		if (_key == MyGUI::KeyCode::ArrowUp)
			newitem--;
		else
			newitem++;


		//Annd fixed :3
		if (iid == 0 && _key == MyGUI::KeyCode::ArrowUp)
		{
			newitem = (int)m_Model->getItemCount() - 1;
		}
		else if (iid == (int)m_Model->getItemCount() - 1 && _key == MyGUI::KeyCode::ArrowDown)
		{
			newitem = 0;
		}


		try
		{
			m_Model->setIndexSelected(newitem);
			m_Model->beginToItemSelected();
		}
		catch (...)
		{
			return;
		}

		eventListChangePositionModelList(m_Model, newitem);

		// fix cursor position
		if (searching)
		{
			m_SearchLine->setTextCursor(m_SearchLine->getTextLength());
		}

	}
	else if (_key == MyGUI::KeyCode::Return)
	{
		if (mLoaderType == LT_SKIN || (mLoaderType != LT_SKIN && mSelectedTruck))
		{
			selectionDone();
		}
	}
}

void CLASS::eventMouseButtonClickOkButton(MyGUI::WidgetPtr _sender)
{
	if (!ready) return;
	selectionDone();
}

void CLASS::eventMouseButtonClickCancelButton(MyGUI::WidgetPtr _sender)
{
	if (!ready) return;
	mSelectedTruck = nullptr;
	mSelectionDone = true;
	hide();
	//Do this on cancel only
	if (gEnv->frameListener->loading_state == NONE_LOADED)
		Application::GetGuiManager()->ShowMainMenu(true);
}

void CLASS::eventComboChangePositionTypeComboBox(MyGUI::ComboBoxPtr _sender, size_t _index)
{
	if (!MAIN_WIDGET->getVisible()) return;
	try
	{
		int categoryID = *m_Type->getItemDataAt<int>(_index);
		m_SearchLine->setCaption(_L("Search ..."));
		onCategorySelected(categoryID);
	}
	catch (...)
	{
	}
}

void CLASS::eventListChangePositionModelListAccept(MyGUI::ListPtr _sender, size_t _index)
{
	eventListChangePositionModelList(_sender, _index);
	selectionDone();
}

void CLASS::eventListChangePositionModelList(MyGUI::ListPtr _sender, size_t _index)
{
	if (!MAIN_WIDGET->getVisible()) return;

	if (_index < 0 || _index >= m_Model->getItemCount()) return;

	try
	{
		int entryID = *m_Model->getItemDataAt<int>(_index);
		onEntrySelected(entryID);
	}
	catch (...)
	{
	}
}

void CLASS::eventComboAcceptConfigComboBox(MyGUI::ComboBoxPtr _sender, size_t _index)
{
	if (!MAIN_WIDGET->getVisible()) return;
	try
	{
		mTruckConfigs.clear();
		Ogre::String config = *m_Config->getItemDataAt<Ogre::String>(_index);
		mTruckConfigs.push_back(config);
	}
	catch (...)
	{
	}
}

void CLASS::getData()
{
	std::map<int, int> mCategoryUsage;
	m_Type->removeAllItems();
	m_Model->removeAllItems();
	mEntries.clear();

	if (mLoaderType == LT_SKIN)
	{
		// skin specific stuff
		m_Type->setEnabled(false);
		m_Type->setCaption(_L("Skins"));
		m_Cancel->setEnabled(false);
		m_Config->setVisible(false);

		m_Model->addItem(_L("Default Skin"), 0);
		{
			int i = 1;
			for (std::vector<Skin *>::iterator it = mCurrentSkins.begin(); it != mCurrentSkins.end(); it++, i++)
			{
				m_Model->addItem((*it)->getName(), i);
			}
		}
		m_Model->setIndexSelected(0);
		onEntrySelected(0);
		return;
	}
	else
	{
		m_Type->setEnabled(true);
		m_Cancel->setEnabled(true);
	}

	int ts = getTimeStamp();
	std::vector<CacheEntry> *entries = RoR::Application::GetCacheSystem()->getEntries();
	for (std::vector<CacheEntry>::iterator it = entries->begin(); it != entries->end(); it++)
	{
		// category hidden
		/*if (it->categoryid == CacheSystem::CID_Unsorted)
		continue;
		*/
		//printf("category: %d\n", it->categoryid);
		bool add = false;
		if (it->fext == "terrn2")
			add = (mLoaderType == LT_Terrain);
		else if (it->fext == "truck")
			add = (mLoaderType == LT_AllBeam || mLoaderType == LT_Vehicle || mLoaderType == LT_Truck || mLoaderType == LT_Network || mLoaderType == LT_NetworkWithBoat);
		else if (it->fext == "car")
			add = (mLoaderType == LT_AllBeam || mLoaderType == LT_Vehicle || mLoaderType == LT_Car || mLoaderType == LT_Network || mLoaderType == LT_NetworkWithBoat);
		else if (it->fext == "boat")
			add = (mLoaderType == LT_AllBeam || mLoaderType == LT_Boat || mLoaderType == LT_NetworkWithBoat);
		else if (it->fext == "airplane")
			add = (mLoaderType == LT_AllBeam || mLoaderType == LT_Airplane || mLoaderType == LT_Network || mLoaderType == LT_NetworkWithBoat);
		else if (it->fext == "trailer")
			add = (mLoaderType == LT_AllBeam || mLoaderType == LT_Trailer || mLoaderType == LT_Extension);
		else if (it->fext == "train")
			add = (mLoaderType == LT_AllBeam || mLoaderType == LT_Train);
		else if (it->fext == "load")
			add = (mLoaderType == LT_AllBeam || mLoaderType == LT_Load || mLoaderType == LT_Extension);

		if (!add)
			continue;

		// remove invalid category ID's
		if (it->categoryid >= CacheSystem::CID_Max)
			it->categoryid = -1;

		// category unsorted
		if (it->categoryid == -1)
			it->categoryid = CacheSystem::CID_Unsorted;

		mCategoryUsage[it->categoryid]++;

		// category all
		mCategoryUsage[CacheSystem::CID_All]++;

		// category fresh
		if (ts - it->addtimestamp < CACHE_FILE_FRESHNESS)
			mCategoryUsage[CacheSystem::CID_Fresh]++;

		mEntries.push_back(*it);
	}
	int tally_categories = 0, current_category = 0;
	std::map<int, Category_Entry> *cats = RoR::Application::GetCacheSystem()->getCategories();
	for (std::map<int, Category_Entry>::iterator itc = cats->begin(); itc != cats->end(); itc++)
	{
		if (mCategoryUsage[itc->second.number] > 0)
			tally_categories++;
	}
	for (std::map<int, Category_Entry>::iterator itc = cats->begin(); itc != cats->end(); itc++)
	{
		int num_elements = mCategoryUsage[itc->second.number];
		if (num_elements > 0)
		{
			Ogre::UTFString title = _L("unknown");
			if (!itc->second.title.empty())
			{
				title = _L(itc->second.title.c_str());
			}
			Ogre::UTFString txt = U("[") + TOUTFSTRING(++current_category) + U("/") + TOUTFSTRING(tally_categories) + U("] (") + TOUTFSTRING(num_elements) + U(") ") + title;
			m_Type->addItem(convertToMyGUIString(txt), itc->second.number);
		}
	}
	if (tally_categories > 0)
	{
		try
		{
			m_Type->setIndexSelected(0);
			m_Type->beginToItemSelected();
		}
		catch (...)
		{
			return;
		}
		onCategorySelected(*m_Type->getItemDataAt<int>(0));
	}
}

bool CLASS::searchCompare(Ogre::String searchString, CacheEntry *ce)
{
	if (searchString.find(":") == Ogre::String::npos)
	{
		// normal search

		// the name
		Ogre::String dname_lower = ce->dname;
		Ogre::StringUtil::toLowerCase(dname_lower);
		if (dname_lower.find(searchString) != Ogre::String::npos)
			return true;

		// the filename
		Ogre::String fname_lower = ce->fname;
		Ogre::StringUtil::toLowerCase(fname_lower);
		if (fname_lower.find(searchString) != Ogre::String::npos)
			return true;

		// the description
		Ogre::String desc = ce->description;
		Ogre::StringUtil::toLowerCase(desc);
		if (desc.find(searchString) != Ogre::String::npos)
			return true;

		// the authors
		if (!ce->authors.empty())
		{
			std::vector<AuthorInfo>::const_iterator it;
			for (it = ce->authors.begin(); it != ce->authors.end(); it++)
			{
				// author name
				Ogre::String aname = it->name;
				Ogre::StringUtil::toLowerCase(aname);
				if (aname.find(searchString) != Ogre::String::npos)
					return true;

				// author email
				Ogre::String aemail = it->email;
				Ogre::StringUtil::toLowerCase(aemail);
				if (aemail.find(searchString) != Ogre::String::npos)
					return true;
			}
		}
		return false;
	}
	else
	{
		Ogre::StringVector v = Ogre::StringUtil::split(searchString, ":");
		if (v.size() < 2) return false; //invalid syntax

		if (v[0] == "hash")
		{
			Ogre::String hash = ce->hash;
			Ogre::StringUtil::toLowerCase(hash);
			return (hash.find(v[1]) != Ogre::String::npos);
		}
		else if (v[0] == "guid")
		{
			Ogre::String guid = ce->guid;
			Ogre::StringUtil::toLowerCase(guid);
			return (guid.find(v[1]) != Ogre::String::npos);
		}
		else if (v[0] == "author")
		{
			// the authors
			if (!ce->authors.empty())
			{
				std::vector<AuthorInfo>::const_iterator it;
				for (it = ce->authors.begin(); it != ce->authors.end(); it++)
				{
					// author name
					Ogre::String aname = it->name;
					Ogre::StringUtil::toLowerCase(aname);
					if (aname.find(v[1]) != Ogre::String::npos)
						return true;

					// author email
					Ogre::String aemail = it->email;
					Ogre::StringUtil::toLowerCase(aemail);
					if (aemail.find(v[1]) != Ogre::String::npos)
						return true;
				}
			}
			return false;
		}
		else if (v[0] == "wheels")
		{
			Ogre::String wheelsStr = TOUTFSTRING(ce->wheelcount) + "x" + TOUTFSTRING(ce->propwheelcount);
			return (wheelsStr == v[1]);
		}
		else if (v[0] == "file")
		{
			Ogre::String fn = ce->fname;
			Ogre::StringUtil::toLowerCase(fn);
			return (fn.find(v[1]) != Ogre::String::npos);
		}


	}
	return false;
}

void CLASS::onCategorySelected(int categoryID)
{
	if (mLoaderType == LT_SKIN) return;

	Ogre::String search_cmd = m_SearchLine->getCaption();
	Ogre::StringUtil::toLowerCase(search_cmd);

	int counter = 0;
	int ts = getTimeStamp();

	m_Model->removeAllItems();

	for (auto it = mEntries.begin(); it != mEntries.end(); it++)
	{
		if (it->categoryid == categoryID || categoryID == CacheSystem::CID_All
			|| categoryID == CacheSystem::CID_Fresh && (ts - it->addtimestamp < CACHE_FILE_FRESHNESS)
			|| categoryID == CacheSystem::CID_SearchResults && searchCompare(search_cmd, &(*it)))
		{
			counter++;
			Ogre::String txt = TOSTRING(counter) + ". " + it->dname;
			try
			{
				m_Model->addItem(txt, it->number);
			}
			catch (...)
			{
				m_Model->addItem("ENCODING ERROR", it->number);
			}
		}
	}

	if (counter > 0)
	{
		try
		{
			m_Model->setIndexSelected(0);
			m_Model->beginToItemSelected();
		}
		catch (...)
		{
			return;
		}
		onEntrySelected(*m_Model->getItemDataAt<int>(0));
	}
}

void CLASS::onEntrySelected(int entryID)
{
	if (mLoaderType == LT_SKIN)
	{
		// special skin handling
		if (entryID == 0)
		{
			// default, default infos
			updateControls(mSelectedTruck);
			return;
		}
		entryID -= 1; // remove default skin :)
		Skin *skin = mCurrentSkins[entryID];

		// we assume its already loaded
		// set selected skin as current
		mSelectedSkin = skin;

		setPreviewImage(mCurrentSkins[entryID]->thumbnail);

		m_EntryName->setCaption(skin->name);

		Ogre::UTFString descriptiontxt = skin->description + Ogre::String("\n");
		descriptiontxt = descriptiontxt + _L("Author(s): ") + skin->authorName + Ogre::String("\n");
		descriptiontxt = descriptiontxt + _L("Description: ") + skin->description + Ogre::String("\n");

		try
		{
			m_EntryDescription->setCaption(Ogre::String(descriptiontxt));
		}
		catch (...)
		{
			m_EntryDescription->setCaption("ENCODING ERROR");
		}
		return;
	}
	CacheEntry *entry = RoR::Application::GetCacheSystem()->getEntry(entryID);
	if (!entry) return;
	mSelectedTruck = entry;
	updateControls(mSelectedTruck);
}

void CLASS::selectionDone()
{
	if (!ready || !mSelectedTruck || mSelectionDone)
		return;

	mSelectionDone = true;

	mSelectedTruck->usagecounter++;
	// TODO: Save the modified value of the usagecounter

	if (mLoaderType != LT_SKIN)
	{
		// we show the normal loader
		RoR::Application::GetCacheSystem()->checkResourceLoaded(*mSelectedTruck);

		mCurrentSkins.clear();
		SkinManager::getSingleton().getUsableSkins(mSelectedTruck->guid, this->mCurrentSkins);
		if (!mCurrentSkins.empty())
		{
			hide();
			show(LT_SKIN);
		}
		else
		{
			mSelectedSkin = 0;
			hide();
		}
	}
	else
	{
		// we show the skin loader, set final skin and exit!
		// mSelectedSkin should be set already!
		hide();
	}
}

void CLASS::updateControls(CacheEntry *entry)
{
	Ogre::String outBasename = "";
	Ogre::String outPath = "";
	Ogre::StringUtil::splitFilename(entry->filecachename, outBasename, outPath);

	setPreviewImage(outBasename);

	if (entry->sectionconfigs.size())
	{
		m_Config->setVisible(true);
		m_Config->removeAllItems();
		for (std::vector<Ogre::String>::iterator its = entry->sectionconfigs.begin(); its != entry->sectionconfigs.end(); its++)
		{
			try
			{
				m_Config->addItem(*its, *its);
			}
			catch (...)
			{
				m_Config->addItem("ENCODING ERROR", *its);
			}
		}
		m_Config->setIndexSelected(0);

		mTruckConfigs.clear();
		Ogre::String configstr = *m_Config->getItemDataAt<Ogre::String>(0);
		mTruckConfigs.push_back(configstr);
	}
	else
	{
		m_Config->setVisible(false);
	}
	Ogre::UTFString authors = "";
	std::set<Ogre::String> author_names;
	for (auto it = entry->authors.begin(); it != entry->authors.end(); it++)
	{
		if (!it->type.empty() && !it->name.empty())
		{
			Ogre::String name = it->name;
			Ogre::StringUtil::trim(name);
			author_names.insert(name);
		}
	}
	for (std::set<Ogre::String>::iterator it = author_names.begin(); it != author_names.end(); it++)
	{
		Ogre::UTFString name = ANSI_TO_UTF(*it);
		authors.append(U(" ") + name);
	}
	if (authors.length() == 0)
	{
		authors = _L("no author information available");
	}

	try
	{
		m_EntryName->setCaption(convertToMyGUIString(ANSI_TO_UTF(entry->dname)));
	}
	catch (...)
	{
		m_EntryName->setCaption("ENCODING ERROR");
	}

	Ogre::UTFString c = U("#FF7D02"); // colour key shortcut
	Ogre::UTFString nc = U("#FFFFFF"); // colour key shortcut

	Ogre::UTFString newline = U("\n");

	Ogre::UTFString descriptiontxt = U("#66FF33") + ANSI_TO_UTF(entry->description) + nc + newline;

	descriptiontxt = descriptiontxt + _L("Author(s): ") + c + authors + nc + newline;


	if (entry->version > 0)           descriptiontxt = descriptiontxt + _L("Version: ") + c + TOUTFSTRING(entry->version) + nc + newline;
	if (entry->wheelcount > 0)        descriptiontxt = descriptiontxt + _L("Wheels: ") + c + TOUTFSTRING(entry->wheelcount) + U("x") + TOUTFSTRING(entry->propwheelcount) + nc + newline;
	if (entry->truckmass > 0)         descriptiontxt = descriptiontxt + _L("Mass: ") + c + TOUTFSTRING((int)(entry->truckmass / 1000.0f)) + U(" ") + _L("tons") + nc + newline;
	if (entry->loadmass > 0)          descriptiontxt = descriptiontxt + _L("Load Mass: ") + c + TOUTFSTRING((int)(entry->loadmass / 1000.0f)) + U(" ") + _L("tons") + nc + newline;
	if (entry->nodecount > 0)         descriptiontxt = descriptiontxt + _L("Nodes: ") + c + TOUTFSTRING(entry->nodecount) + nc + newline;
	if (entry->beamcount > 0)         descriptiontxt = descriptiontxt + _L("Beams: ") + c + TOUTFSTRING(entry->beamcount) + nc + newline;
	if (entry->shockcount > 0)        descriptiontxt = descriptiontxt + _L("Shocks: ") + c + TOUTFSTRING(entry->shockcount) + nc + newline;
	if (entry->hydroscount > 0)       descriptiontxt = descriptiontxt + _L("Hydros: ") + c + TOUTFSTRING(entry->hydroscount) + nc + newline;
	if (entry->soundsourcescount > 0) descriptiontxt = descriptiontxt + _L("SoundSources: ") + c + TOUTFSTRING(entry->soundsourcescount) + nc + newline;
	if (entry->commandscount > 0)     descriptiontxt = descriptiontxt + _L("Commands: ") + c + TOUTFSTRING(entry->commandscount) + nc + newline;
	if (entry->rotatorscount > 0)     descriptiontxt = descriptiontxt + _L("Rotators: ") + c + TOUTFSTRING(entry->rotatorscount) + nc + newline;
	if (entry->exhaustscount > 0)     descriptiontxt = descriptiontxt + _L("Exhausts: ") + c + TOUTFSTRING(entry->exhaustscount) + nc + newline;
	if (entry->flarescount > 0)       descriptiontxt = descriptiontxt + _L("Flares: ") + c + TOUTFSTRING(entry->flarescount) + nc + newline;
	if (entry->torque > 0)            descriptiontxt = descriptiontxt + _L("Torque: ") + c + TOUTFSTRING(entry->torque) + nc + newline;
	if (entry->flexbodiescount > 0)   descriptiontxt = descriptiontxt + _L("Flexbodies: ") + c + TOUTFSTRING(entry->flexbodiescount) + nc + newline;
	if (entry->propscount > 0)        descriptiontxt = descriptiontxt + _L("Props: ") + c + TOUTFSTRING(entry->propscount) + nc + newline;
	if (entry->wingscount > 0)        descriptiontxt = descriptiontxt + _L("Wings: ") + c + TOUTFSTRING(entry->wingscount) + nc + newline;
	if (entry->hasSubmeshs)           descriptiontxt = descriptiontxt + _L("Using Submeshs: ") + c + TOUTFSTRING(entry->hasSubmeshs) + nc + newline;
	if (entry->numgears > 0)          descriptiontxt = descriptiontxt + _L("Transmission Gear Count: ") + c + TOUTFSTRING(entry->numgears) + nc + newline;
	if (entry->minrpm > 0)            descriptiontxt = descriptiontxt + _L("Engine RPM: ") + c + TOUTFSTRING(entry->minrpm) + U(" - ") + TOUTFSTRING(entry->maxrpm) + nc + newline;
	if (!entry->uniqueid.empty() && entry->uniqueid != "no-uid") descriptiontxt = descriptiontxt + _L("Unique ID: ") + c + entry->uniqueid + nc + newline;
	if (!entry->guid.empty() && entry->guid != "no-guid")		descriptiontxt = descriptiontxt + _L("GUID: ") + c + entry->guid + nc + newline;
	if (entry->usagecounter > 0)      descriptiontxt = descriptiontxt + _L("Times used: ") + c + TOUTFSTRING(entry->usagecounter) + nc + newline;

	if (entry->addtimestamp > 0)
	{
		char tmp[255] = "";
		time_t epch = entry->addtimestamp;
		sprintf(tmp, "%s", asctime(gmtime(&epch)));
		descriptiontxt = descriptiontxt + _L("Date and Time installed: ") + c + Ogre::String(tmp) + nc + newline;
	}

	Ogre::UTFString driveableStr[5] = { _L("Non-Driveable"), _L("Truck"), _L("Airplane"), _L("Boat"), _L("Machine") };
	if (entry->nodecount > 0) descriptiontxt = descriptiontxt + _L("Vehicle Type: ") + c + driveableStr[entry->driveable] + nc + newline;

	descriptiontxt = descriptiontxt + "#FF0000\n"; // red colour for the props

	if (entry->forwardcommands) descriptiontxt = descriptiontxt + _L("[forwards commands]") + newline;
	if (entry->importcommands) descriptiontxt = descriptiontxt + _L("[imports commands]") + newline;
	if (entry->rollon) descriptiontxt = descriptiontxt + _L("[is rollon]") + newline;
	if (entry->rescuer) descriptiontxt = descriptiontxt + _L("[is rescuer]") + newline;
	if (entry->custom_particles) descriptiontxt = descriptiontxt + _L("[uses custom particles]") + newline;
	if (entry->fixescount > 0) descriptiontxt = descriptiontxt + _L("[has fixes]") + newline;
	// t is the default, do not display it
	//if (entry->enginetype == 't') descriptiontxt = descriptiontxt +_L("[TRUCK ENGINE]") + newline;
	if (entry->enginetype == 'c') descriptiontxt = descriptiontxt + _L("[car engine]") + newline;
	if (entry->type == "Zip") descriptiontxt = descriptiontxt + _L("[zip archive]") + newline;
	if (entry->type == "FileSystem") descriptiontxt = descriptiontxt + _L("[unpacked in directory]") + newline;

	descriptiontxt = descriptiontxt + "#66CCFF\n"; // now blue-ish color*

	if (!entry->dirname.empty()) descriptiontxt = descriptiontxt + _L("Source: ") + entry->dirname + newline;
	if (!entry->fname.empty()) descriptiontxt = descriptiontxt + _L("Filename: ") + entry->fname + newline;
	if (!entry->hash.empty() && entry->hash != "none") descriptiontxt = descriptiontxt + _L("Hash: ") + entry->hash + newline;
	if (!entry->hash.empty()) descriptiontxt = descriptiontxt + _L("Mod Number: ") + TOUTFSTRING(entry->number) + newline;

	if (!entry->sectionconfigs.empty())
	{
		descriptiontxt = descriptiontxt + U("\n\n#e10000") + _L("Please select a configuration below!") + nc + U("\n\n");
	}

	trimUTFString(descriptiontxt);

	m_EntryDescription->setCaption(convertToMyGUIString(descriptiontxt));
}

void CLASS::setPreviewImage(Ogre::String texture)
{
	if (texture == "" || texture == "none")
	{
		m_Preview->setVisible(false);
		return;
	}

	mPreviewImageTexture = texture;

	try
	{
		resizePreviewImage();
		m_Preview->setImageTexture(texture);
		m_Preview->setVisible(true);
	}
	catch (...)
	{
		m_Preview->setVisible(false);
	}
}

void CLASS::resizePreviewImage()
{
	MyGUI::IntSize imgSize(0, 0);
	Ogre::TexturePtr t = Ogre::TextureManager::getSingleton().load(mPreviewImageTexture, Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME);
	if (!t.isNull())
	{
		imgSize.width = (int)t->getWidth() * 10;
		imgSize.height = (int)t->getHeight() * 10;
	}

	if (imgSize.width != 0 && imgSize.height != 0)
	{
		MyGUI::IntSize maxSize = m_PreviewBox->getSize();

		float imgRatio = imgSize.width / (float)imgSize.height;
		float maxRatio = maxSize.width / (float)maxSize.height;

		MyGUI::IntSize newSize(0, 0);
		MyGUI::IntPoint newPosition(0, 0);

		// scale with aspect ratio
		if (imgRatio > maxRatio)
		{
			newSize.width = maxSize.width;
			newSize.height = maxSize.width / imgRatio;
			newPosition.left = 0;
			newPosition.top = maxSize.height - newSize.height;
		}
		else
		{
			newSize.width = maxSize.height * imgRatio;
			newSize.height = maxSize.height;
			newPosition.left = maxSize.width - newSize.width;
			newPosition.top = 0;
		}

		m_Preview->setSize(newSize);
		m_Preview->setPosition(newPosition);
	}
}

bool CLASS::isFinishedSelecting()
{
	return mSelectionDone;
}

void CLASS::show(LoaderType type)
{
	if (!mSelectionDone) return;
	mSelectionDone = false;

	mSelectedSkin = 0;
	m_SearchLine->setCaption(_L("Search ..."));
	RoR::Application::GetInputEngine()->resetKeys();
	LoadingWindow::getSingleton().hide();
	// focus main mMainWidget (for key input)
	mTruckConfigs.clear();
	MyGUI::InputManager::getInstance().setKeyFocusWidget(mMainWidget);
	mMainWidget->setEnabledSilent(true);

	// first time fast
	/*if (!visibleCounter)
	mMainWidget->castType<MyGUI::Window>()->setVisible(true);
	else*/
	MAIN_WIDGET->setVisibleSmooth(true);

	if (type != LT_SKIN) mSelectedTruck = 0; // when in skin, we still need the info

	mLoaderType = type;
	getData();
	visibleCounter++;

	// so want to sleep 0.5 before the controls start working
	readytime = 0.5f;
	MyGUI::Gui::getInstance().eventFrameStart += MyGUI::newDelegate(this, &CLASS::frameEntered);
	bindKeys();
}

void CLASS::hide()
{
	mSelectionDone = true;
	RoR::Application::GetGuiManager()->unfocus();
	MAIN_WIDGET->setVisibleSmooth(false);
	MAIN_WIDGET->setEnabledSilent(false);
	ready = false;
	bindKeys(false);
}

void CLASS::setEnableCancel(bool enabled)
{
	m_Cancel->setEnabled(enabled);
}

void CLASS::eventSearchTextChange(MyGUI::EditBox *_sender)
{
	if (!MAIN_WIDGET->getVisible()) return;
	onCategorySelected(CacheSystem::CID_SearchResults);
	m_Type->setCaption(_L("Search Results"));
}

void CLASS::eventSearchTextGotFocus(MyGUI::WidgetPtr _sender, MyGUI::WidgetPtr oldWidget)
{
	if (!MAIN_WIDGET->getVisible()) return;

	if (m_SearchLine->getCaption() == _L("Search ..."))
	{
		m_SearchLine->setCaption("");
	}
}

bool CLASS::IsVisible()
{
	return MAIN_WIDGET->isVisible();
}

void CLASS::notifyWindowButtonPressed(MyGUI::WidgetPtr _sender, const std::string& _name)
{
	if (_name == "close")
		hide();
}
