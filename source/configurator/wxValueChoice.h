/*
This source file is part of Rigs of Rods
Copyright 2005,2006,2007,2008,2009 Pierre-Michel Ricordel
Copyright 2007,2008,2009 Thomas Fischer

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

// written by thomas fischer thomas{AT}thomasfischer{DOT}biz on 18th of June 2011
// to cope the problems of the buggy i18n translations in wxChoice

#pragma once
#ifndef WXVALUECHOICE
#define WXVALUECHOICE

#include <wx/choice.h>
#include <wx/string.h>
#include <map>

//some shortcut
//#define ADD_TRANSLATED_CHOICE(x,y) x->AppendValueItem(wxT(y), _(y));

class wxValueChoice : public wxChoice
{
protected:
	// some shortcuts
	typedef std::pair<wxString, wxString> strPair;  // value, caption
	typedef std::map <unsigned long, strPair> valMap; // id, [value/caption]
	typedef std::map <unsigned int, unsigned long> ctrlValMap; // control id -> map id,
	
	valMap id_value_map;
	ctrlValMap id_id_map;
	bool sortable;

	
public:
	wxValueChoice() : sortable(false) , id_value_map(), id_id_map()
	{
	}

	wxValueChoice(wxWindow *parent,
		wxWindowID id,
		const wxPoint& pos = wxDefaultPosition,
		const wxSize& size = wxDefaultSize,
		long style = 0,
		const wxValidator& validator = wxDefaultValidator,
		const wxString& name = wxChoiceNameStr) : sortable(false), id_value_map(), id_id_map()
	{
		// create control
		Create(parent, id, pos, size, 0, NULL, style, validator, name);
	}

	void AppendValueItem(wxString valueString, wxString caption, unsigned long sortNum = 0)
	{
		int id = this->Append(caption);
		unsigned long mapid = (sortNum==0?id:sortNum);
		id_value_map[mapid] = strPair(valueString, caption);
		id_id_map[id] = mapid;
		if(sortNum != 0) sortable = true;
	}
	
	void AppendValueItem(wxString captionAndValue, unsigned long sortNum = 0)
	{
		int id = this->Append(captionAndValue);
		unsigned long mapid = (sortNum==0?id:sortNum);
		id_value_map[mapid] = strPair(captionAndValue, captionAndValue);
		id_id_map[id] = mapid;
		if(sortNum != 0) sortable = true;
	}

	void Clear()
	{
		wxChoice::Clear();
		id_value_map.clear();
		id_id_map.clear();
		sortable=false;
	}

	void sort(bool descending = true)
	{
		if(!sortable) return;
		// remove all items from the control and re-add them in the order of the map
		wxChoice::Clear(); // clear items from control
		id_id_map.clear();

		// use the caption for the control (2. in the pair)

		if(descending)
		{
			valMap::reverse_iterator it;
			for(it = id_value_map.rbegin(); it != id_value_map.rend(); it++)
			{
				int id = this->Append(it->second.second);
				id_id_map[id] = it->first;
			}
		} else
		{
			valMap::iterator it;
			for(it = id_value_map.begin(); it != id_value_map.end(); it++)
			{
				int id = this->Append(it->second.second);
				id_id_map[id] = it->first;
			}
		}
	}

	wxString getSelectedValue()
	{
		int id = this->GetSelection();
		unsigned long mapid = id_id_map[id];
		strPair p = id_value_map[mapid];
		if(p.first.empty())
		{
			return wxT("error");
		}
		// return the value (first in pair)
		return p.first;
	}

	std::string getSelectedValueAsSTDString()
	{
		wxString s = getSelectedValue();
		return std::string(s.mb_str());
	}

	int setSelectedValue(std::string val)
	{
		return setSelectedValue(wxString(val.c_str(), wxConvUTF8));
	}

	int setSelectedValue(wxString val)
	{
		if(val.empty()) return -1;
		valMap::iterator it;
		for(it = id_value_map.begin(); it != id_value_map.end(); it++)
		{
			if(it->second.first == val)
			{
				this->SetStringSelection(it->second.second);
				return it->first;
			}
		}

		this->SetSelection(0);
		return -1;
	}
};

#endif //WXVALUECHOICE
