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

#define CLASS        MainSelector
#define MAIN_WIDGET  ((MyGUI::Window*)mMainWidget)

CLASS::CLASS() :
    m_keys_bound(false)
    , m_selected_skin(nullptr)
    , m_selected_entry(nullptr)
    , m_selection_done(true)
    , m_actor_spawn_rq_valid(false)
{
    MAIN_WIDGET->setVisible(false);
    m_skin_manager = RoR::App::GetContentManager()->GetSkinManager();

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
    m_selected_skin = nullptr;
    m_selected_entry = nullptr;
    m_selection_done = true;
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
        if (m_loader_type == LT_SKIN || (m_loader_type != LT_SKIN && m_selected_entry))
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
    App::sim_state.SetActive(SimState::RUNNING); // TODO: use 'Pending' mechanism!
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
    try
    {
        int categoryID = *m_Type->getItemDataAt<int>(_index);
        m_SearchLine->setCaption(_L("Search ..."));
        OnCategorySelected(categoryID);
    }
    catch (...)
    {
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

    try
    {
        int entryID = *m_Model->getItemDataAt<int>(_index);
        OnEntrySelected(entryID);
    }
    catch (...)
    {
    }
}

void CLASS::EventComboAcceptConfigComboBox(MyGUI::ComboBoxPtr _sender, size_t _index)
{
    if (!MAIN_WIDGET->getVisible())
        return;
    try
    {
        m_vehicle_configs.clear();
        Ogre::String config = *m_Config->getItemDataAt<Ogre::String>(_index);
        m_vehicle_configs.push_back(config);
    }
    catch (...)
    {
    }
}

template <typename T1, typename T2>
struct sort_cats
{
    bool operator ()(std::pair<int, Ogre::String> const& a, std::pair<int, Ogre::String> const& b) const
    {
        if (a.first == CacheSystem::CID_All)
            return true;
        if (b.first == CacheSystem::CID_All)
            return false;
        if (a.first == CacheSystem::CID_Fresh)
            return true;
        if (b.first == CacheSystem::CID_Fresh)
            return false;
        return a.second < b.second;
    }
};

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

struct sort_search_results
{
    bool operator ()(std::pair<CacheEntry*, size_t> const& a, std::pair<CacheEntry*, size_t> const& b) const
    {
        return a.second < b.second;
    }
};

void CLASS::UpdateGuiData()
{
    std::map<int, int> mCategoryUsage;
    m_Type->removeAllItems();
    m_Model->removeAllItems();
    m_entries.clear();

    if (m_loader_type == LT_SKIN)
    {
        // skin specific stuff
        m_Type->setEnabled(false);
        m_Type->setCaption(_L("Skins"));
        m_Cancel->setEnabled(false);
        m_Config->setVisible(false);

        m_Model->addItem(_L("Default Skin"), 0);
        {
            int i = 1;
            for (std::vector<RoR::SkinDef *>::iterator it = m_current_skins.begin(); it != m_current_skins.end(); it++ , i++)
            {
                m_Model->addItem((*it)->name, i);
            }
        }
        m_Model->setIndexSelected(0);
        OnEntrySelected(0);
        return;
    }
    else
    {
        m_Type->setEnabled(true);
        m_Cancel->setEnabled(true);
    }

    int ts = getTimeStamp();
    std::vector<CacheEntry>* entries = RoR::App::GetCacheSystem()->getEntries();
    std::sort(entries->begin(), entries->end(), sort_entries<CacheEntry>());
    for (std::vector<CacheEntry>::iterator it = entries->begin(); it != entries->end(); it++)
    {
        bool add = false;
        if (it->fext == "terrn2")
            add = (m_loader_type == LT_Terrain);
        else if (it->fext == "truck")
            add = (m_loader_type == LT_AllBeam || m_loader_type == LT_Vehicle || m_loader_type == LT_Truck || m_loader_type == LT_Network || m_loader_type == LT_NetworkWithBoat);
        else if (it->fext == "car")
            add = (m_loader_type == LT_AllBeam || m_loader_type == LT_Vehicle || m_loader_type == LT_Truck || m_loader_type == LT_Car || m_loader_type == LT_Network || m_loader_type == LT_NetworkWithBoat);
        else if (it->fext == "boat")
            add = (m_loader_type == LT_AllBeam || m_loader_type == LT_Boat || m_loader_type == LT_NetworkWithBoat);
        else if (it->fext == "airplane")
            add = (m_loader_type == LT_AllBeam || m_loader_type == LT_Airplane || m_loader_type == LT_Network || m_loader_type == LT_NetworkWithBoat);
        else if (it->fext == "trailer")
            add = (m_loader_type == LT_AllBeam || m_loader_type == LT_Trailer || m_loader_type == LT_Extension);
        else if (it->fext == "train")
            add = (m_loader_type == LT_AllBeam || m_loader_type == LT_Train);
        else if (it->fext == "load")
            add = (m_loader_type == LT_AllBeam || m_loader_type == LT_Load || m_loader_type == LT_Extension);

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

        m_entries.push_back(*it);
    }
    int tally_categories = 0, current_category = 0;
    std::map<int, Ogre::String>* cats = RoR::App::GetCacheSystem()->getCategories();

    std::vector<std::pair<int, Ogre::String>> sorted_cats(cats->begin(), cats->end());
    std::sort(sorted_cats.begin(), sorted_cats.end(), sort_cats<int, Ogre::String>());

    for (std::vector<std::pair<int, Ogre::String>>::iterator itc = sorted_cats.begin(); itc != sorted_cats.end(); itc++)
    {
        if (mCategoryUsage[itc->first] > 0)
            tally_categories++;
    }
    for (std::vector<std::pair<int, Ogre::String>>::iterator itc = sorted_cats.begin(); itc != sorted_cats.end(); itc++)
    {
        int num_elements = mCategoryUsage[itc->first];
        if (num_elements > 0)
        {
            Ogre::UTFString title = _L("unknown");
            if (!itc->second.empty())
            {
                title = _L(itc->second.c_str());
            }
            Ogre::UTFString txt = U("[") + TOUTFSTRING(++current_category) + U("/") + TOUTFSTRING(tally_categories) + U("] (") + TOUTFSTRING(num_elements) + U(") ") + title;
            m_Type->addItem(convertToMyGUIString(txt), itc->first);
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
        OnCategorySelected(*m_Type->getItemDataAt<int>(0));
    }
}

size_t CLASS::SearchCompare(Ogre::String searchString, CacheEntry* ce)
{
    if (searchString.find(":") == Ogre::String::npos)
    {
        // normal search

        // the name
        Ogre::String dname_lower = ce->dname;
        Ogre::StringUtil::toLowerCase(dname_lower);
        if (dname_lower.find(searchString) != Ogre::String::npos)
            return dname_lower.find(searchString);

        // the filename
        Ogre::String fname_lower = ce->fname;
        Ogre::StringUtil::toLowerCase(fname_lower);
        if (fname_lower.find(searchString) != Ogre::String::npos)
            return 100 + fname_lower.find(searchString);

        // the description
        Ogre::String desc = ce->description;
        Ogre::StringUtil::toLowerCase(desc);
        if (desc.find(searchString) != Ogre::String::npos)
            return 200 + desc.find(searchString);

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
                    return 300 + aname.find(searchString);

                // author email
                Ogre::String aemail = it->email;
                Ogre::StringUtil::toLowerCase(aemail);
                if (aemail.find(searchString) != Ogre::String::npos)
                    return 400 + aemail.find(searchString);
            }
        }
        return Ogre::String::npos;
    }
    else
    {
        Ogre::StringVector v = Ogre::StringUtil::split(searchString, ":");
        if (v.size() < 2)
            return Ogre::String::npos; //invalid syntax

        if (v[0] == "hash")
        {
            Ogre::String hash = ce->hash;
            Ogre::StringUtil::toLowerCase(hash);
            return hash.find(v[1]);
        }
        else if (v[0] == "guid")
        {
            Ogre::String guid = ce->guid;
            Ogre::StringUtil::toLowerCase(guid);
            return guid.find(v[1]);
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
                        return aname.find(v[1]);

                    // author email
                    Ogre::String aemail = it->email;
                    Ogre::StringUtil::toLowerCase(aemail);
                    if (aemail.find(v[1]) != Ogre::String::npos)
                        return aemail.find(v[1]);
                }
            }
            return Ogre::String::npos;
        }
        else if (v[0] == "wheels")
        {
            Ogre::String wheelsStr = TOUTFSTRING(ce->wheelcount) + "x" + TOUTFSTRING(ce->propwheelcount);
            return wheelsStr.find(v[1]);
        }
        else if (v[0] == "file")
        {
            Ogre::String fn = ce->fname;
            Ogre::StringUtil::toLowerCase(fn);
            return fn.find(v[1]);
        }
    }
    return Ogre::String::npos;
}

void CLASS::OnCategorySelected(int categoryID)
{
    if (m_loader_type == LT_SKIN)
        return;

    Ogre::String search_cmd = m_SearchLine->getCaption();
    Ogre::StringUtil::toLowerCase(search_cmd);

    int counter = 0;
    int ts = getTimeStamp();

    m_Model->removeAllItems();

    if (categoryID == CacheSystem::CID_SearchResults)
    {
        std::vector<std::pair<CacheEntry*, size_t>> search_results;
        search_results.reserve(m_entries.size());

        for (auto it = m_entries.begin(); it != m_entries.end(); it++)
        {
            size_t score = SearchCompare(search_cmd, &(*it));
            if (score != Ogre::String::npos)
                search_results.push_back(std::make_pair(&(*it), score));
        }

        std::stable_sort(search_results.begin(), search_results.end(), sort_search_results());

        for (auto it = search_results.begin(); it != search_results.end(); it++)
        {
            counter++;
            Ogre::String txt = TOSTRING(counter) + ". " + it->first->dname;
            try
            {
                m_Model->addItem(txt, it->first->number);
            }
            catch (...)
            {
                m_Model->addItem("ENCODING ERROR", it->first->number);
            }
        }
    }
    else
    {
        for (auto it = m_entries.begin(); it != m_entries.end(); it++)
        {
            if (it->categoryid == categoryID || categoryID == CacheSystem::CID_All
                || categoryID == CacheSystem::CID_Fresh && (ts - it->addtimestamp < CACHE_FILE_FRESHNESS))
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
        OnEntrySelected(*m_Model->getItemDataAt<int>(0));
    }
}

void CLASS::OnEntrySelected(int entryID)
{
    if (m_loader_type == LT_SKIN)
    {
        // special skin handling
        if (entryID == 0)
        {
            // default, default infos
            m_selected_skin = 0;
            this->UpdateControls(m_selected_entry);
            return;
        }
        entryID -= 1; // remove default skin :)
        RoR::SkinDef* skin = m_current_skins[entryID];

        // we assume its already loaded
        // set selected skin as current
        m_selected_skin = skin;

        SetPreviewImage(m_current_skins[entryID]->thumbnail);

        m_EntryName->setCaption(skin->name);

        Ogre::UTFString descriptiontxt = skin->description + Ogre::String("\n");
        descriptiontxt = descriptiontxt + _L("Author(s): ") + skin->author_name + Ogre::String("\n");
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
    CacheEntry* entry = RoR::App::GetCacheSystem()->getEntry(entryID);
    if (!entry)
        return;
    m_selected_entry = entry;
    this->UpdateControls(m_selected_entry);
}

void CLASS::OnSelectionDone()
{
    if (!m_selected_entry || m_selection_done)
        return;

    m_selection_done = true;
    
    bool new_actor_selected = false;

    m_selected_entry->usagecounter++;
    // TODO: Save the modified value of the usagecounter

    if (m_loader_type != LT_SKIN)
    {
        // we show the normal loader

        //Only load vehicles via the selector
        if (m_loader_type != LT_Terrain)
            RoR::App::GetCacheSystem()->checkResourceLoaded(*m_selected_entry);

        m_current_skins.clear();
        m_skin_manager->GetUsableSkins(m_selected_entry->guid, this->m_current_skins);
        if (!m_current_skins.empty())
        {
            Show(LT_SKIN);
        }
        else
        {
            m_selected_skin = nullptr;
            this->Hide(false); // false = Hide without fade-out effect
            new_actor_selected = (m_loader_type != LT_Terrain);
        }
    }
    else if ((m_loader_type == LT_Terrain) && (App::mp_state.GetActive() == MpState::CONNECTED))
    {
        App::app_state.SetPending(AppState::SIMULATION);
        this->Hide(false); // false = Hide without fade-out effect (otherwise fading selector window overlaps 'loading' box)
    }
    else
    {
        this->Hide();
        new_actor_selected = (m_loader_type != LT_Terrain);
    }

    if (new_actor_selected)
    {
        ActorSpawnRequest rq;
        if (m_actor_spawn_rq_valid)
        {
            rq = m_actor_spawn_rq;
            m_actor_spawn_rq_valid = false;
        }
        rq.asr_skin           = m_selected_skin;
        rq.asr_cache_entry    = m_selected_entry;
        rq.asr_config         = m_vehicle_configs;
        rq.asr_origin         = ActorSpawnRequest::Origin::USER;
        App::GetSimController()->QueueActorSpawn(rq);

        RoR::App::GetGuiManager()->UnfocusGui();
    }
    App::sim_state.SetActive(SimState::RUNNING); // TODO: use 'Pending' mechanism!
}

void CLASS::UpdateControls(CacheEntry* entry)
{
    Ogre::String outBasename = "";
    Ogre::String outPath = "";
    Ogre::StringUtil::splitFilename(entry->filecachename, outBasename, outPath);

    SetPreviewImage(outBasename);

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

        m_vehicle_configs.clear();
        Ogre::String configstr = *m_Config->getItemDataAt<Ogre::String>(0);
        m_vehicle_configs.push_back(configstr);
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

    if (entry->version > 0)
        descriptiontxt = descriptiontxt + _L("Version: ") + c + TOUTFSTRING(entry->version) + nc + newline;
    if (entry->wheelcount > 0)
        descriptiontxt = descriptiontxt + _L("Wheels: ") + c + TOUTFSTRING(entry->wheelcount) + U("x") + TOUTFSTRING(entry->propwheelcount) + nc + newline;
    if (entry->truckmass > 0)
        descriptiontxt = descriptiontxt + _L("Mass: ") + c + TOUTFSTRING((int)(entry->truckmass / 1000.0f)) + U(" ") + _L("tons") + nc + newline;
    if (entry->loadmass > 0)
        descriptiontxt = descriptiontxt + _L("Load Mass: ") + c + TOUTFSTRING((int)(entry->loadmass / 1000.0f)) + U(" ") + _L("tons") + nc + newline;
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

    if (entry->addtimestamp > 0)
    {
        char tmp[255] = "";
        time_t epch = entry->addtimestamp;
        sprintf(tmp, "%s", asctime(gmtime(&epch)));
        descriptiontxt = descriptiontxt + _L("Date and Time installed: ") + c + Ogre::String(tmp) + nc + newline;
    }

    Ogre::UTFString driveableStr[5] = {_L("Non-Driveable"), _L("Truck"), _L("Airplane"), _L("Boat"), _L("Machine")};
    if (entry->nodecount > 0)
        descriptiontxt = descriptiontxt + _L("Vehicle Type: ") + c + driveableStr[entry->driveable] + nc + newline;

    descriptiontxt = descriptiontxt + "#FF0000\n"; // red colour for the props

    if (entry->forwardcommands)
        descriptiontxt = descriptiontxt + _L("[forwards commands]") + newline;
    if (entry->importcommands)
        descriptiontxt = descriptiontxt + _L("[imports commands]") + newline;
    if (entry->rollon)
        descriptiontxt = descriptiontxt + _L("[is rollon]") + newline;
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
    if (!entry->hash.empty() && entry->hash != "none")
        descriptiontxt = descriptiontxt + _L("Hash: ") + entry->hash + newline;
    if (!entry->hash.empty())
        descriptiontxt = descriptiontxt + _L("Mod Number: ") + TOUTFSTRING(entry->number) + newline;

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
    m_actor_spawn_rq_valid = true;
    m_actor_spawn_rq = req;
    this->Show(type);
}

void CLASS::Show(LoaderType type)
{
    if (!m_selection_done)
        return;
    m_selection_done = false;

    m_selected_skin = 0;
    m_SearchLine->setCaption("");
    RoR::App::GetInputEngine()->resetKeys();
    App::GetGuiManager()->SetVisible_LoadingWindow(false);
    m_vehicle_configs.clear();
    //MyGUI::InputManager::getInstance().setKeyFocusWidget(mMainWidget);
    MyGUI::InputManager::getInstance().setKeyFocusWidget(m_SearchLine);
    mMainWidget->setEnabledSilent(true);

    MAIN_WIDGET->setVisibleSmooth(true);

    if (type != LT_SKIN)
        m_selected_entry = nullptr; // when in skin, we still need the info

    m_loader_type = type;
    UpdateGuiData();

    BindKeys();

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
    OnCategorySelected(CacheSystem::CID_SearchResults);
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
