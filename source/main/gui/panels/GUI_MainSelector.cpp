/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2013-2017 Petr Ohlidal & contributors

    For more information, see http://www.rigsofrods.org/

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

#include "GUI_MainSelector.h"

#include "Application.h"
#include "BeamFactory.h"
#include "CacheSystem.h"
#include "ContentManager.h"
#include "GUIManager.h"
#include "GUI_LoadingWindow.h"
#include "InputEngine.h"
#include "Language.h"
#include "MainMenu.h"
#include "RoRFrameListener.h"
#include "RoRPrerequisites.h"
#include "SkinManager.h"
#include "Utils.h"

#include <MyGUI.h>

using namespace RoR;
using namespace GUI;

// MAIN SELECTOR WINDOW
// --------------------
// KEY CONTROLS
//   '/' forward slash: set keyboard focus to search box if not already
//   tabulator: toggle search box focus
//   arrow left/right: if search box is not in focus, select previous(left) or next(right) category in "categories" combobox (does not wrap, i.e. when already on first/last item, do not skip to the other end).
//   arrow up/down: select prev/next entry. When already on top/bottom item, wrap to other end of the list.
//   enter: activate highlighted entry
// SEARCHING
//   The result list is sorted descending by 'score'.
//   Syntax 'abcdef': searches fulltext (ingoring case) in: name, filename, description, author name/mail (in this order, with descending rank) and returns rank+string pos as score
//   Syntax 'AREA:abcdef': searches (ignoring case) in AREA: 'guid'(guid string), 'author' (name/email), 'wheels' (string "WHEELCOUNTxPROPWHEELCOUNT"), 'file' (filename); returns string pos as score

#define CLASS        MainSelector
#define MAIN_WIDGET  ((MyGUI::Window*)mMainWidget)

CLASS::CLASS() :
    m_keys_bound(false)
    , m_selected_entry(nullptr)
    , m_selection_done(true)
    , m_searching(false)
{
    MAIN_WIDGET->setVisible(false);

    MyGUI::WindowPtr win = dynamic_cast<MyGUI::WindowPtr>(mMainWidget);
    win->eventWindowButtonPressed += MyGUI::newDelegate(this, &CLASS::NotifyWindowButtonPressed); //The "X" button thing
    win->eventWindowChangeCoord += MyGUI::newDelegate(this, &CLASS::NotifyWindowChangeCoord);

    MyGUI::IntSize windowSize = MAIN_WIDGET->getSize();
    MyGUI::IntSize parentSize = MAIN_WIDGET->getParentSize();

    m_Type->eventComboChangePosition += MyGUI::newDelegate(this, &CLASS::EventComboChangePositionTypeComboBox);

    m_Model->eventListSelectAccept += MyGUI::newDelegate(this, &CLASS::EventListChangePositionModelListAccept);
    m_Model->eventListChangePosition += MyGUI::newDelegate(this, &CLASS::EventListChangePositionModelList);
    m_Config->eventComboAccept += MyGUI::newDelegate(this, &CLASS::EventComboAcceptConfigComboBox);
    m_Ok->eventMouseButtonClick += MyGUI::newDelegate(this, &CLASS::EventMouseButtonClickOkButton);
    m_Cancel->eventMouseButtonClick += MyGUI::newDelegate(this, &CLASS::EventMouseButtonClickCancelButton);

    // search stuff
    m_SearchLine->eventEditTextChange += MyGUI::newDelegate(this, &CLASS::EventSearchTextChange);
    m_SearchLine->eventMouseSetFocus += MyGUI::newDelegate(this, &CLASS::EventSearchTextGotFocus);
    m_SearchLine->eventKeySetFocus += MyGUI::newDelegate(this, &CLASS::EventSearchTextGotFocus);

    MAIN_WIDGET->setPosition((parentSize.width - windowSize.width) / 2, (parentSize.height - windowSize.height) / 2);

    //From old file
    MAIN_WIDGET->setCaption(_L("Loader"));

    m_SearchLine->setCaption("");
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

void CLASS::Reset()
{
    m_selected_entry = nullptr;
    m_selection_done = true;
    m_actor_spawn_rq = RoR::ActorSpawnRequest(); // Clear
}

void CLASS::BindKeys(bool bind)
{
    if (bind && !m_keys_bound)
    {
        MAIN_WIDGET->eventKeyButtonPressed += MyGUI::newDelegate(this, &CLASS::EventKeyButtonPressed_Main);
        m_Type->eventKeyButtonPressed += MyGUI::newDelegate(this, &CLASS::EventKeyButtonPressed_Main);
        m_SearchLine->eventKeyButtonPressed += MyGUI::newDelegate(this, &CLASS::EventKeyButtonPressed_Main);
        m_keys_bound = true;
    }
    else if (!bind && m_keys_bound)
    {
        MAIN_WIDGET->eventKeyButtonPressed -= MyGUI::newDelegate(this, &CLASS::EventKeyButtonPressed_Main);
        m_Type->eventKeyButtonPressed -= MyGUI::newDelegate(this, &CLASS::EventKeyButtonPressed_Main);
        m_SearchLine->eventKeyButtonPressed -= MyGUI::newDelegate(this, &CLASS::EventKeyButtonPressed_Main);
        m_keys_bound = false;
    }
}

void CLASS::NotifyWindowChangeCoord(MyGUI::Window* _sender)
{
    if (m_Preview->isVisible())
        ResizePreviewImage();
}

void CLASS::EventKeyButtonPressed_Main(MyGUI::WidgetPtr _sender, MyGUI::KeyCode _key, MyGUI::Char _char)
{
    if (!mMainWidget->getVisible())
        return;
    int cid = (int)m_Type->getIndexSelected();
    int iid = (int)m_Model->getIndexSelected();

    bool searching = MyGUI::InputManager::getInstance().getKeyFocusWidget() == m_SearchLine;

    if (_key == MyGUI::KeyCode::Slash)
    {
        MyGUI::InputManager::getInstance().setKeyFocusWidget(m_SearchLine);
        m_SearchLine->setCaption("");
        searching = true;
    }
    else if (_key == MyGUI::KeyCode::Tab)
    {
        if (searching)
        {
            MyGUI::InputManager::getInstance().setKeyFocusWidget(mMainWidget);
            m_SearchLine->setCaption(_L("Search ..."));
        }
        else
        {
            MyGUI::InputManager::getInstance().setKeyFocusWidget(m_SearchLine);
            m_SearchLine->setCaption("");
        }
        searching = !searching;
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
        EventComboChangePositionTypeComboBox(m_Type, newitem);
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

        EventListChangePositionModelList(m_Model, newitem);

        // fix cursor position
        if (searching)
        {
            m_SearchLine->setTextCursor(m_SearchLine->getTextLength());
        }
    }
    else if (_key == MyGUI::KeyCode::Return)
    {
        if (m_loader_type == LT_Skin || (m_loader_type != LT_Skin && m_selected_entry))
        {
            OnSelectionDone();
        }
    }
}

void CLASS::Cancel()
{
    m_selected_entry = nullptr;
    Hide();

    if (App::app_state.GetActive() == AppState::MAIN_MENU)
    {
        RoR::App::GetMainMenu()->LeaveMultiplayerServer();
        App::GetGuiManager()->SetVisible_GameMainMenu(true);
    }

    if (App::sim_state.GetActive() == SimState::SELECTING)
    {    
        App::sim_state.SetActive(SimState::RUNNING);
    }
}

void CLASS::EventMouseButtonClickOkButton(MyGUI::WidgetPtr _sender)
{
    OnSelectionDone();
}

void CLASS::EventMouseButtonClickCancelButton(MyGUI::WidgetPtr _sender)
{
    Cancel();
}

void CLASS::EventComboChangePositionTypeComboBox(MyGUI::ComboBoxPtr _sender, size_t _index)
{
    if (!MAIN_WIDGET->getVisible())
        return;

    if (_index < 0 || _index >= m_Type->getItemCount())
        return;

    int categoryID = *m_Type->getItemDataAt<int>(_index);
    m_SearchLine->setCaption(_L("Search ..."));
    OnCategorySelected(categoryID);
    if (!m_searching)
    {
        m_category_last_index[m_loader_type] = static_cast<int>(_index);
    }
}

void CLASS::EventListChangePositionModelListAccept(MyGUI::ListPtr _sender, size_t _index)
{
    EventListChangePositionModelList(_sender, _index);
    OnSelectionDone();
}

void CLASS::EventListChangePositionModelList(MyGUI::ListPtr _sender, size_t _index)
{
    if (!MAIN_WIDGET->getVisible())
        return;

    if (_index < 0 || _index >= m_Model->getItemCount())
        return;

    int entryID = *m_Model->getItemDataAt<int>(_index);
    OnEntrySelected(entryID);
    if (!m_searching)
    {
        m_entry_index[m_loader_type] = static_cast<int>(_index);
    }
}

void CLASS::EventComboAcceptConfigComboBox(MyGUI::ComboBoxPtr _sender, size_t _index)
{
    if (!MAIN_WIDGET->getVisible())
        return;

    if (_index < 0 || _index >= m_Config->getItemCount())
        return;

    m_actor_spawn_rq.asr_config = *m_Config->getItemDataAt<Ogre::String>(_index);
}

template <typename T1>
struct sort_entries
{
    bool operator ()(CacheEntry const& a, CacheEntry const& b) const
    {
        Ogre::String first = a.dname;
        Ogre::String second = b.dname;
        Ogre::StringUtil::toLowerCase(first);
        Ogre::StringUtil::toLowerCase(second);
        return first < second;
    }
};

bool CLASS::IsFresh(CacheEntry* entry)
{
    return entry->filetime >= m_cache_file_freshness - 86400;
}

void CLASS::UpdateGuiData()
{
    m_Type->removeAllItems();
    m_Model->removeAllItems();
    m_entries.clear();

    // Find all relevant entries
    CacheQuery query;
    query.cqy_filter_type = m_loader_type;
    App::GetCacheSystem()->Query(query);
    m_cache_file_freshness = query.cqy_res_last_update;
    for (CacheQueryResult const& res: query.cqy_results)
    {
        m_entries.push_back(*res.cqr_entry);

        if (this->IsFresh(res.cqr_entry))
        {
            query.cqy_res_category_usage[CacheCategoryId::CID_Fresh]++;
        }
    }

    // Count used categories
    size_t tally_categories = 0;
    for (size_t i = 0; i < CacheSystem::NUM_CATEGORIES; ++i)
    {
        if (query.cqy_res_category_usage[CacheSystem::CATEGORIES[i].ccg_id] > 0)
        {
            tally_categories++;
        }
    }

    // Display used categories
    size_t display_number = 1;
    for (size_t i = 0; i < CacheSystem::NUM_CATEGORIES; ++i)
    {
        size_t num_entries = query.cqy_res_category_usage[CacheSystem::CATEGORIES[i].ccg_id];
        if (query.cqy_res_category_usage[CacheSystem::CATEGORIES[i].ccg_id] > 0)
        {
            Str<300> title;
            title << "[" << display_number << "/" << tally_categories
                  << "] (" << num_entries << ") " << CacheSystem::CATEGORIES[i].ccg_name;
            m_Type->addItem(title.ToCStr(), CacheSystem::CATEGORIES[i].ccg_id);
            display_number++;
        }
    }

    if (m_Type->getItemCount() > 0)
    {
        int idx = m_category_last_index[m_loader_type] < m_Type->getItemCount() ? m_category_last_index[m_loader_type] : 0;
        m_Type->setIndexSelected(idx);
        m_Type->beginToItemSelected();

        // FIXME: currently this performs duplicate search. Will be remade soon (using DearIMGUI) 
        OnCategorySelected(*m_Type->getItemDataAt<int>(idx));
    }
}

void CLASS::UpdateSearchParams()
{
    std::string searchString = m_SearchLine->getCaption();

    if (searchString.find(":") == std::string::npos)
    {
        m_search_method = CacheSearchMethod::FULLTEXT;
        m_search_query = searchString;
    }
    else
    {
        Ogre::StringVector v = Ogre::StringUtil::split(searchString, ":");
        if (v.size() < 2)
        {
            m_search_method = CacheSearchMethod::NONE;
            m_search_query = "";
        }
        else if (v[0] == "guid")
        {
            m_search_method = CacheSearchMethod::GUID;
            m_search_query = v[1];
        }
        else if (v[0] == "author")
        {
            m_search_method = CacheSearchMethod::AUTHORS;
            m_search_query = v[1];
        }
        else if (v[0] == "wheels")
        {
            m_search_method = CacheSearchMethod::WHEELS;
            m_search_query = v[1];
        }
        else if (v[0] == "file")
        {
            m_search_method = CacheSearchMethod::FILENAME;
            m_search_query = v[1];
        }
        else
        {
            m_search_method = CacheSearchMethod::NONE;
            m_search_query = "";
        }
    }
}

void CLASS::OnCategorySelected(int categoryID)
{
    m_Model->removeAllItems();
    m_entries.clear();

    CacheQuery query;
    query.cqy_filter_type = m_loader_type;
    query.cqy_filter_category_id = categoryID;
    query.cqy_search_method = m_search_method;
    query.cqy_search_string = m_search_query;
    if (m_loader_type == LT_Skin && m_selected_entry)
    {
        query.cqy_filter_guid = m_selected_entry->guid;
    }
    App::GetCacheSystem()->Query(query);

    for (CacheQueryResult const& res: query.cqy_results)
    {
        if (categoryID != CacheCategoryId::CID_Fresh || this->IsFresh(res.cqr_entry))
        {
            m_entries.push_back(*res.cqr_entry);
        }
    }

    if (m_loader_type == LT_Skin)
    {
        m_Model->addItem(_L("Default Skin"), 0); // Virtual entry
    }

    size_t display_num = 1;
    for (const auto& entry : m_entries)
    {
        m_Model->addItem(Ogre::StringUtil::format("%u. %s", display_num, entry.dname.c_str()), entry.number);
        display_num++;
    }

    if (m_Model->getItemCount() > 0)
    {
        int idx = m_entry_index[m_loader_type] < m_Model->getItemCount() ? m_entry_index[m_loader_type] : 0;
        m_Model->setIndexSelected(idx);
        m_Model->beginToItemSelected();
        OnEntrySelected(*m_Model->getItemDataAt<int>(idx));
    }

    m_searching = categoryID == CacheCategoryId::CID_SearchResults;
}

void CLASS::OnEntrySelected(int entryID)
{
    if (m_loader_type == LT_Skin && entryID == 0) // This is the "Default skin" entry
    {
        m_selected_entry = nullptr;
        m_EntryName->setCaption("Default skin");
        m_EntryDescription->setCaption("");
        this->SetPreviewImage(m_actor_spawn_rq.asr_cache_entry->filecachename); // Always available in `LT_Skin`
    }
    else
    {
        m_selected_entry = RoR::App::GetCacheSystem()->GetEntry(entryID);
        m_EntryName->setCaption(Utils::SanitizeUtf8String(m_selected_entry->dname));
        this->SetPreviewImage(m_selected_entry->filecachename);
        this->UpdateControls(m_selected_entry);
    }
}

void CLASS::OnSelectionDone()
{
    if ((m_loader_type != LT_Skin && !m_selected_entry) || m_selection_done)
        return;

    m_selection_done = true;
    
    bool new_actor_selected = false;

    if (m_selected_entry != nullptr) // Default skin is nullptr
        m_selected_entry->usagecounter++;
    // TODO: Save the modified value of the usagecounter

    if (m_loader_type != LT_Skin)
    {
        if (m_loader_type != LT_Terrain && m_loader_type != LT_Skin)
            RoR::App::GetCacheSystem()->LoadResource(*m_selected_entry);

        // Check if skins are available and show skin selector
        std::vector<CacheEntry> skin_entries = App::GetCacheSystem()->GetUsableSkins(m_selected_entry->guid);
        if (!skin_entries.empty())
        {
            m_actor_spawn_rq.asr_cache_entry = m_selected_entry;
            this->Show(LT_Skin);
        }
        else
        {
            this->Hide(false); // false = Hide without fade-out effect
            if (m_loader_type != LT_Terrain)
            {
                m_actor_spawn_rq.asr_cache_entry = m_selected_entry;
                new_actor_selected = true;
            }
        }
    }
    else if ((m_loader_type == LT_Terrain) && (App::mp_state.GetActive() == MpState::CONNECTED))
    {
        App::app_state.SetPending(AppState::SIMULATION);
        this->Hide(false); // false = Hide without fade-out effect (otherwise fading selector window overlaps 'loading' box)
    }
    else // Skin
    {
        this->Hide();
        if (m_loader_type != LT_Terrain)
        {
            m_actor_spawn_rq.asr_skin_entry = m_selected_entry;
            new_actor_selected = true;
        }
    }

    if (new_actor_selected)
    {
        ActorSpawnRequest rq = m_actor_spawn_rq; // actor+skin+sectionconfig entries already filled
        rq.asr_origin         = ActorSpawnRequest::Origin::USER;
        App::GetSimController()->QueueActorSpawn(rq);
        this->Reset();
        RoR::App::GetGuiManager()->UnfocusGui();
    }
    App::sim_state.SetActive(SimState::RUNNING); // TODO: use 'Pending' mechanism!
}

void CLASS::UpdateControls(CacheEntry* entry)
{
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

        m_actor_spawn_rq.asr_config = *m_Config->getItemDataAt<Ogre::String>(0);
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

    Ogre::UTFString c = U("#FF7D02"); // colour key shortcut
    Ogre::UTFString nc = U("#FFFFFF"); // colour key shortcut

    Ogre::UTFString newline = U("\n");

    Ogre::UTFString descriptiontxt = U("#66FF33") + ANSI_TO_UTF(entry->description) + nc + newline;

    descriptiontxt = descriptiontxt + _L("Author(s): ") + c + authors + nc + newline;

    if (entry->version > 0)
        descriptiontxt = descriptiontxt + _L("Version: ") + c + TOUTFSTRING(entry->version) + nc + newline;
    if (entry->wheelcount > 0)
        descriptiontxt = descriptiontxt + _L("Wheels: ") + c + TOUTFSTRING(entry->wheelcount) + U("x") + TOUTFSTRING(entry->propwheelcount) + nc + newline;
    if (entry->truckmass > 0)
        descriptiontxt = descriptiontxt + _L("Mass: ") + c + TOUTFSTRING(Round(entry->truckmass / 1000.0f, 3)) + U(" ") + _L("tons") + nc + newline;
    if (entry->loadmass > 0)
        descriptiontxt = descriptiontxt + _L("Load Mass: ") + c + TOUTFSTRING(Round(entry->loadmass / 1000.0f, 3)) + U(" ") + _L("tons") + nc + newline;
    if (entry->nodecount > 0)
        descriptiontxt = descriptiontxt + _L("Nodes: ") + c + TOUTFSTRING(entry->nodecount) + nc + newline;
    if (entry->beamcount > 0)
        descriptiontxt = descriptiontxt + _L("Beams: ") + c + TOUTFSTRING(entry->beamcount) + nc + newline;
    if (entry->shockcount > 0)
        descriptiontxt = descriptiontxt + _L("Shocks: ") + c + TOUTFSTRING(entry->shockcount) + nc + newline;
    if (entry->hydroscount > 0)
        descriptiontxt = descriptiontxt + _L("Hydros: ") + c + TOUTFSTRING(entry->hydroscount) + nc + newline;
    if (entry->soundsourcescount > 0)
        descriptiontxt = descriptiontxt + _L("SoundSources: ") + c + TOUTFSTRING(entry->soundsourcescount) + nc + newline;
    if (entry->commandscount > 0)
        descriptiontxt = descriptiontxt + _L("Commands: ") + c + TOUTFSTRING(entry->commandscount) + nc + newline;
    if (entry->rotatorscount > 0)
        descriptiontxt = descriptiontxt + _L("Rotators: ") + c + TOUTFSTRING(entry->rotatorscount) + nc + newline;
    if (entry->exhaustscount > 0)
        descriptiontxt = descriptiontxt + _L("Exhausts: ") + c + TOUTFSTRING(entry->exhaustscount) + nc + newline;
    if (entry->flarescount > 0)
        descriptiontxt = descriptiontxt + _L("Flares: ") + c + TOUTFSTRING(entry->flarescount) + nc + newline;
    if (entry->torque > 0)
        descriptiontxt = descriptiontxt + _L("Torque: ") + c + TOUTFSTRING(entry->torque) + nc + newline;
    if (entry->flexbodiescount > 0)
        descriptiontxt = descriptiontxt + _L("Flexbodies: ") + c + TOUTFSTRING(entry->flexbodiescount) + nc + newline;
    if (entry->propscount > 0)
        descriptiontxt = descriptiontxt + _L("Props: ") + c + TOUTFSTRING(entry->propscount) + nc + newline;
    if (entry->wingscount > 0)
        descriptiontxt = descriptiontxt + _L("Wings: ") + c + TOUTFSTRING(entry->wingscount) + nc + newline;
    if (entry->hasSubmeshs)
        descriptiontxt = descriptiontxt + _L("Using Submeshs: ") + c + TOUTFSTRING(entry->hasSubmeshs) + nc + newline;
    if (entry->numgears > 0)
        descriptiontxt = descriptiontxt + _L("Transmission Gear Count: ") + c + TOUTFSTRING(entry->numgears) + nc + newline;
    if (entry->minrpm > 0)
        descriptiontxt = descriptiontxt + _L("Engine RPM: ") + c + TOUTFSTRING(entry->minrpm) + U(" - ") + TOUTFSTRING(entry->maxrpm) + nc + newline;
    if (!entry->uniqueid.empty() && entry->uniqueid != "no-uid")
        descriptiontxt = descriptiontxt + _L("Unique ID: ") + c + entry->uniqueid + nc + newline;
    if (!entry->guid.empty() && entry->guid != "no-guid")
        descriptiontxt = descriptiontxt + _L("GUID: ") + c + entry->guid + nc + newline;
    if (entry->usagecounter > 0)
        descriptiontxt = descriptiontxt + _L("Times used: ") + c + TOUTFSTRING(entry->usagecounter) + nc + newline;

    if (entry->filetime > 0)
    {
        Ogre::String filetime = Ogre::StringUtil::format("%s", asctime(gmtime(&entry->filetime)));
        descriptiontxt = descriptiontxt + _L("Date and Time modified: ") + c + filetime + nc;
    }
    if (entry->addtimestamp > 0)
    {
        Ogre::String addtimestamp = Ogre::StringUtil::format("%s", asctime(gmtime(&entry->addtimestamp)));
        descriptiontxt = descriptiontxt + _L("Date and Time installed: ") + c + addtimestamp + nc;
    }

    Ogre::UTFString driveableStr[5] = {_L("Non-Driveable"), _L("Truck"), _L("Airplane"), _L("Boat"), _L("Machine")};
    if (entry->nodecount > 0)
        descriptiontxt = descriptiontxt + _L("Vehicle Type: ") + c + driveableStr[entry->driveable] + nc + newline;

    descriptiontxt = descriptiontxt + "#FF0000\n"; // red colour for the props

    if (entry->forwardcommands)
        descriptiontxt = descriptiontxt + _L("[forwards commands]") + newline;
    if (entry->importcommands)
        descriptiontxt = descriptiontxt + _L("[imports commands]") + newline;
    if (entry->rescuer)
        descriptiontxt = descriptiontxt + _L("[is rescuer]") + newline;
    if (entry->custom_particles)
        descriptiontxt = descriptiontxt + _L("[uses custom particles]") + newline;
    if (entry->fixescount > 0)
        descriptiontxt = descriptiontxt + _L("[has fixes]") + newline;
    // t is the default, do not display it
    //if (entry->enginetype == 't') descriptiontxt = descriptiontxt +_L("[TRUCK ENGINE]") + newline;
    if (entry->enginetype == 'c')
        descriptiontxt = descriptiontxt + _L("[car engine]") + newline;
    if (entry->resource_bundle_type == "Zip")
        descriptiontxt = descriptiontxt + _L("[zip archive]") + newline;
    if (entry->resource_bundle_type == "FileSystem")
        descriptiontxt = descriptiontxt + _L("[unpacked in directory]") + newline;

    descriptiontxt = descriptiontxt + "#66CCFF\n"; // now blue-ish color*

    if (!entry->resource_bundle_path.empty())
        descriptiontxt = descriptiontxt + _L("Source: ") + entry->resource_bundle_path + newline;
    if (!entry->fname.empty())
        descriptiontxt = descriptiontxt + _L("Filename: ") + entry->fname + newline;

    if (!entry->sectionconfigs.empty())
    {
        descriptiontxt = descriptiontxt + U("\n\n#e10000") + _L("Please select a configuration below!") + nc + U("\n\n");
    }

    trimUTFString(descriptiontxt);

    m_EntryDescription->setCaption(convertToMyGUIString(descriptiontxt));
}

void CLASS::SetPreviewImage(Ogre::String texture)
{
    if (texture == "" || texture == "none")
    {
        m_Preview->setVisible(false);
        return;
    }

    m_preview_image_texture = texture;

    try
    {
        ResizePreviewImage();
        m_Preview->setImageTexture(texture);
        m_Preview->setVisible(true);
    }
    catch (...)
    {
        Ogre::LogManager::getSingleton().stream() << "[RoR|SelectorGUI] Failed to load preview image: " << m_preview_image_texture;
        m_Preview->setVisible(false);
    }
}

void CLASS::ResizePreviewImage()
{
    MyGUI::IntSize imgSize(0, 0);
    Ogre::TexturePtr t = Ogre::TextureManager::getSingleton().load(m_preview_image_texture, Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME);
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

bool CLASS::IsFinishedSelecting()
{
    return m_selection_done;
}

void CLASS::Show(LoaderType type, RoR::ActorSpawnRequest req)
{
    if (!m_selection_done)
    {
        return;
    }
    m_actor_spawn_rq = req;
    this->Show(type);
}

void CLASS::Show(LoaderType type)
{
    if (!m_selection_done)
        return;

    m_selection_done = false;
    m_selected_entry = nullptr;
    m_SearchLine->setCaption("");
    RoR::App::GetInputEngine()->resetKeys();
    App::GetGuiManager()->SetVisible_LoadingWindow(false);
    MyGUI::InputManager::getInstance().setKeyFocusWidget(m_SearchLine);
    mMainWidget->setEnabledSilent(true);
    MAIN_WIDGET->setVisibleSmooth(true);
    m_loader_type = type;
    UpdateGuiData();
    BindKeys();

    if (type != LT_Terrain && type != LT_Skin) // Is this a vehicle type?
    {
        m_actor_spawn_rq.asr_config.clear(); // Only reset what we need, some fields come pre-configured!
    }

    if (type == LT_Terrain && (RoR::App::mp_state.GetActive() == RoR::MpState::CONNECTED))
    {
        m_Cancel->setCaption(_L("Cancel (disconnect)"));
    }
    else
    {
        m_Cancel->setCaption(_L("Cancel"));
    }
}

void CLASS::Hide(bool smooth)
{
    m_selection_done = true;
    RoR::App::GetGuiManager()->UnfocusGui();
    if (smooth)
    {
        MAIN_WIDGET->setVisibleSmooth(false);
    }
    else
    {
        MAIN_WIDGET->setVisible(false);
    }
    MAIN_WIDGET->setEnabledSilent(false);
    BindKeys(false);
}

void CLASS::EventSearchTextChange(MyGUI::EditBox* _sender)
{
    if (!MAIN_WIDGET->getVisible())
        return;

    this->UpdateSearchParams();
    OnCategorySelected(CacheCategoryId::CID_SearchResults);
    if (m_SearchLine->getTextLength() > 0)
    {
        m_Type->setCaption(_L("Search Results"));
    }
}

void CLASS::EventSearchTextGotFocus(MyGUI::WidgetPtr _sender, MyGUI::WidgetPtr oldWidget)
{
    if (!MAIN_WIDGET->getVisible())
        return;

    if (m_SearchLine->getCaption() == _L("Search ..."))
    {
        m_SearchLine->setCaption("");
    }
}

bool CLASS::IsVisible()
{
    return MAIN_WIDGET->isVisible();
}

void CLASS::NotifyWindowButtonPressed(MyGUI::WidgetPtr _sender, const std::string& _name)
{
    if (_name == "close")
    {
        Cancel();
    }
}
