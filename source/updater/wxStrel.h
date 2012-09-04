/*
This source file is part of Rigs of Rods
Copyright 2005,2006,2007,2008,2009 Pierre-Michel Ricordel
Copyright 2007,2008,2009 Thomas Fischer

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
#ifndef WXSTREL_H
#define WXSTREL_H

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

// for all others, include the necessary headers
#ifndef WX_PRECOMP
    #include "wx/frame.h"
    #include "wx/panel.h"
    #include "wx/stattext.h"
    #include "wx/sizer.h"
	#include "wx/statbmp.h"
	#include <wx/checkbox.h>
#endif

#include "ConfigManager.h"
#include "utils.h"

#define STREL_HEIGHT 60



class wxStrel : public wxPanel
{
public:
    wxStrel(wxWindow *parent, stream_desc_t* desc) : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxSize(-1, STREL_HEIGHT), wxBORDER_SIMPLE)
	{
		this->desc=desc;
		if(desc->beta)
			SetBackgroundColour(wxColour(255,200,200,255));
		else
			SetBackgroundColour(*wxWHITE);

		wxBoxSizer *mainSizer = new wxBoxSizer(wxHORIZONTAL);
		SetSizer(mainSizer);
		mainSizer->Add(chk=new wxCheckBox(this, wxID_ANY, wxString()), 0, wxALL|wxALIGN_CENTER, 5);
		chk->SetValue(desc->checked);
		chk->Enable(!desc->disabled);


		bmp = new myClickBitmap(this, this, wxID_ANY, desc->icon);
		mainSizer->Add((wxWindow*)bmp, 0, wxALL, 1);
        wxBoxSizer *textSizer = new wxBoxSizer(wxVERTICAL);
		myClickText *tst;

		tst=new myClickText(this, this, wxID_ANY, desc->title);
		textSizer->Add((wxWindow *)tst, 0, wxALL, 1);
		wxFont dfont=tst->GetFont();
		dfont.SetWeight(wxFONTWEIGHT_BOLD);
		dfont.SetPointSize(dfont.GetPointSize()+3);
		tst->SetFont(dfont);
		tst->Wrap(300);
		tst=new myClickText(this, this, wxID_ANY, desc->desc);
        textSizer->Add((wxWindow *)tst, 0, wxALL, 1);
		tst->Wrap(300);

		char tmp[255]="";
		sprintf(tmp, "%0.2f MB", desc->size/1024.0f);
		tst=new myClickText(this, this, wxID_ANY, _("Total size: ") + wxString(tmp, wxConvUTF8));
		textSizer->Add((wxWindow *)tst, 0, wxALL, 1);
		tst->Wrap(300);


		mainSizer->Add(textSizer, 1, wxALL|wxEXPAND , 2);

		//mainSizer->Fit(this);
	}

	~wxStrel()
	{
	}

	stream_desc_t *getDesc() {return desc;}
	bool getSelection() {return chk->GetValue();}

	void toggle()
	{
		if(!chk->IsEnabled()) return;
		if(chk->IsChecked())
			chk->SetValue(false);
		else
			chk->SetValue(true);
	}

	void clickEvent(wxMouseEvent &event)
	{
		toggle();
	}
protected:
	DECLARE_EVENT_TABLE()
private:
	wxCheckBox *chk;
	stream_desc_t *desc;
	myClickBitmap *bmp;

	// RTTI things
	DECLARE_ABSTRACT_CLASS(wxStrel)
	//DECLARE_NO_COPY_CLASS(wxStrel)
	//DECLARE_EVENT_TABLE()
};

#endif
