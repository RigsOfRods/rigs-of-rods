#include "utils.h"
#include "wxStrel.h"
#include "platform.h"

#include <boost/algorithm/string.hpp>

using namespace boost::filesystem;

BEGIN_EVENT_TABLE(myClickBitmap, wxStaticBitmap)
    EVT_LEFT_UP(myClickBitmap::click)
END_EVENT_TABLE()

BEGIN_EVENT_TABLE(myClickText, wxStaticText)
    EVT_LEFT_UP(myClickText::click)
END_EVENT_TABLE()

myClickBitmap::myClickBitmap(wxStrel *_s, wxWindow *parent,
                 wxWindowID id,
                 const  wxBitmap& label,
                 const wxPoint& pos,
                 const wxSize& size,
                 long style,
                 const wxString& name) : s(_s), wxStaticBitmap(parent,id,label,pos,size,style,name)
{
}

void myClickBitmap::click(wxMouseEvent &evt)
{
  s->toggle();
}

myClickText::myClickText(wxStrel *_s, wxWindow *parent,
                 wxWindowID id,
                 const wxString& label,
                 const wxPoint& pos,
                 const wxSize& size,
                 long style,
                 const wxString& name) : s(_s), wxStaticText(parent,id,label,pos,size,style,name)
{
}

void myClickText::click(wxMouseEvent &evt)
{
  s->toggle();
}


// essential string conversion methods. helps to convert wxString<-->std::string<-->char string
wxString conv(const char *s)
{
	return wxString(s, wxConvUTF8);
}

wxString conv(const std::string& s)
{
	return wxString(s.c_str(), wxConvUTF8);
}

std::string conv(const wxString& s)
{
	return std::string(s.mb_str(wxConvUTF8));
}

std::string formatFilesize(boost::uintmax_t size)
{
	char tmp[256] = "";
	if(size < 1024)
		sprintf(tmp, "%d B", (int)(size));
	else if(size/1024 < 1024)
		sprintf(tmp, "%0.2f kB", (size/1024.0f));
	else
		sprintf(tmp, "%0.2f MB", (size/1024.0f/1024.0f));
	return std::string(tmp);
}


std::string formatSeconds(float seconds)
{
	char tmp[255]="";
	if(seconds > 0 && seconds < 1)
	{
		sprintf(tmp, "%.0f milli seconds", (seconds*1000.0f));
	}
	else if(seconds >= 1 && seconds < 1000000)
	{
		if(seconds<60)
			sprintf(tmp, "%.0f seconds", seconds);
		else
			sprintf(tmp, "%.0f minutes, %.0f seconds", seconds/60, seconds - (((int)(seconds/60)) * 60));
	}
	return std::string(tmp);
}

int getTempFilename(boost::filesystem::path &tempfile)
{
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
	wchar_t *tmp_path_w   = (wchar_t *)malloc( sizeof(wchar_t) * MAX_PATH);
	wchar_t *prefix_str_w = L"WSY";
	wchar_t *tmp_name_w   = (wchar_t *)malloc( sizeof(wchar_t) * MAX_PATH);

	if(GetTempPath(MAX_PATH, tmp_path_w) == 0)
		return -2;
	//printf("GetTempPath() returned: '%s'\n", tmp_path);
	if(GetTempFileName(tmp_path_w, prefix_str_w, 0, tmp_name_w) == 0)
	{
		return -3;
	}

	// convert again
	std::wstring ws = std::wstring(tmp_name_w);
	std::string s;
	s.assign(ws.begin(), ws.end());
	tempfile = boost::filesystem::path(s);

	free(tmp_path_w);
	free(tmp_name_w);
	return 0;
#elif OGRE_PLATFORM == OGRE_PLATFORM_LINUX
	char tempBuffer[] = "/tmp/wsync_tmp_XXXXXX";
	int res = mkstemp(tempBuffer);
	if(res == -1)
		return -4;
	tempfile = path(tempBuffer);
	return 0;
#elif OGRE_PLATFORM == OGRE_PLATFORM_APPLE
	// XXX: TODO: implement
	return -1;
#endif
}

#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32

std::string wstrtostr(const std::wstring &wstr)
{
    // Convert a Unicode string to an ASCII string
    std::string strTo;
    char *szTo = new char[wstr.length() + 1];
    szTo[wstr.size()] = '\0';
    WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), -1, szTo, (int)wstr.length(), NULL, NULL);
    strTo = szTo;
    delete[] szTo;
    return strTo;
}

std::wstring strtowstr(const std::string &str)
{
    // Convert an ASCII string to a Unicode String
    std::wstring wstrTo;
    wchar_t *wszTo = new wchar_t[str.length() + 1];
    wszTo[str.size()] = L'\0';
    MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, wszTo, (int)str.length());
    wstrTo = wszTo;
    delete[] wszTo;
    return wstrTo;
}

#endif //OGRE_PLATFORM

int cleanURL(std::string &url)
{
	int position = url.find("//");
	while (position != std::string::npos)
	{
		url.replace(position, 2, "/" );
		position = url.find( "//", position + 1 );
	}
	position = url.find("//");
	while (position != std::string::npos)
	{
		url.replace(position, 2, "/" );
		position = url.find( "//", position + 1 );
	}
	return 0;
}

int ensurePathExist(boost::filesystem::path &path)
{
	boost::filesystem::path::iterator it;
	for(it=path.begin();it!=path.end(); it++)
	{
		if(!boost::filesystem::exists(*it))
			boost::filesystem::create_directories(path.branch_path());
	}
	return 0;
}