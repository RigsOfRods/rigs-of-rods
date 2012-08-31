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
#ifndef WXMYUTILS_H
#define WXMYUTILS_H

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"
#include <wx/statbmp.h>
#include <wx/stattext.h>

#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>

class wxStrel;
class myClickBitmap : public wxStaticBitmap
{
public:
	myClickBitmap(wxStrel *_s, wxWindow *parent,
                   wxWindowID id,
                   const  wxBitmap& label,
                   const wxPoint& pos = wxDefaultPosition,
                   const wxSize& size = wxDefaultSize,
                   long style = 0,
				   const wxString& name = wxStaticBitmapNameStr);

    void click(wxMouseEvent &evt);
    DECLARE_EVENT_TABLE();
protected:
	wxStrel *s;
};

class myClickText : public wxStaticText
{
public:
	myClickText(wxStrel *_s, wxWindow *parent,
                   wxWindowID id,
                   const wxString& label,
                   const wxPoint& pos = wxDefaultPosition,
                   const wxSize& size = wxDefaultSize,
                   long style = 0,
				   const wxString& name = wxStaticBitmapNameStr);

    void click(wxMouseEvent &evt);
    DECLARE_EVENT_TABLE();
protected:
	wxStrel *s;
};


wxString conv(const char *s);
wxString conv(const std::string& s);
std::string conv(const wxString& s);

std::string formatFilesize(boost::uintmax_t size);
std::string formatSeconds(float seconds);

int getTempFilename(boost::filesystem::path &tempfile);
int cleanURL(std::string &url);

int ensurePathExist(boost::filesystem::path &path);


#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
std::string wstrtostr(const std::wstring &wstr);
std::wstring strtowstr(const std::string &str);
#endif // OGRE_PLATFORM

#endif //WXMYUTILS_H