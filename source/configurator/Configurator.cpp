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
#include "ImprovedConfigFile.h"
#include "OISKeyboard.h"
#include "Ogre.h"
#include "RoRVersion.h"
#include "Settings.h"
#include "rornet.h"
#include "Utils.h" // RoR utils
#include "wxValueChoice.h" // a control we wrote

#if OGRE_PLATFORM == OGRE_PLATFORM_LINUX
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <memory>

mode_t getumask(void)
{
	mode_t mask = umask(0);
	umask(mask);
	return mask;
}
#endif

#include "statpict.h"
#include <wx/cmdline.h>
#include <wx/combobox.h>
#include <wx/dir.h>
#include <wx/fs_inet.h>
#include <wx/gauge.h>
#include <wx/grid.h>
#include <wx/html/htmlwin.h>
#include <wx/intl.h>
#include <wx/log.h>
#include <wx/log.h>
#include <wx/mediactrl.h>
#include <wx/notebook.h>
#include <wx/scrolbar.h>
#include <wx/scrolwin.h>
#include <wx/statline.h>
#include <wx/textfile.h>
#include <wx/timer.h>
#include <wx/tokenzr.h>
#include <wx/tooltip.h>
#include <wx/treectrl.h>
#include <wx/uri.h>
#include <wx/version.h>
#include <wx/wfstream.h>
#include <wx/wx.h>
#include <wx/zipstrm.h>

#if wxCHECK_VERSION(2, 8, 0)
#include <wx/hyperlink.h>
#endif

#include "wxutils.h"

#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE
#define strnlen(str, len) strlen(str)
#endif

#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
#include <direct.h> // for getcwd
#include <windows.h>
#include <shellapi.h>
#endif

// xpm images
#include "config.xpm"
#include "opencllogo.xpm"

// de-comment this to enable network stuff
#define NETWORK

wxLocale lang_locale;
wxLanguageInfo *language=0;
std::vector<wxLanguageInfo*> avLanguages;

std::map<std::string, std::string> settings;

#ifdef USE_OPENAL
#include <AL/al.h>
#include <AL/alc.h>
#include <AL/alext.h>
#endif // USE_OPENAL

#ifdef USE_OPENCL
#include <delayimp.h>
#endif // USE_OPENCL

#ifdef __WXGTK__
#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#endif

// Define a new application type, each program should derive a class from wxApp
class MyApp : public wxApp
{
public:
	// override base class virtuals
	// ----------------------------

	// this one is called on application startup and is a good place for the app
	// initialization (doing it here and not in the ctor allows to have an error
	// return: if OnInit() returns false, the application terminates)
	virtual bool OnInit();
	virtual int OnRun();
	virtual void OnInitCmdLine(wxCmdLineParser& parser);
	virtual bool OnCmdLineParsed(wxCmdLineParser& parser);

	bool filesystemBootstrap();
	void recurseCopy(wxString sourceDir, wxString destinationDir);
	void initLogging();
	bool checkUserPath();
	bool extractZipFiles(const wxString& aZipFile, const wxString& aTargetDir);
	//private:
	wxString UserPath;
	wxString ProgramPath;
	wxString SkeletonPath;
	wxString languagePath;

	bool postinstall;
};

static const wxCmdLineEntryDesc g_cmdLineDesc [] =
{
#if wxCHECK_VERSION(2, 9, 0)
	{ wxCMD_LINE_SWITCH, ("postinstall"), ("postinstall"), ("do not use this")},
#else // old wxWidgets support
	{ wxCMD_LINE_SWITCH, wxT("postinstall"), wxT("postinstall"), wxT("do not use this")},
#endif
	{ wxCMD_LINE_NONE }
};

// from wxWidetgs wiki: http://wiki.wxwidgets.org/Calling_The_Default_Browser_In_WxHtmlWindow
class HtmlWindow: public wxHtmlWindow
{
public:
	HtmlWindow(wxWindow *parent, wxWindowID id = -1,
		const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize,
		long style = wxHW_SCROLLBAR_AUTO, const wxString& name = wxT("htmlWindow"));
	void OnLinkClicked(const wxHtmlLinkInfo& link);
};

HtmlWindow::HtmlWindow(wxWindow *parent, wxWindowID id, const wxPoint& pos,
	const wxSize& size, long style, const wxString& name)
: wxHtmlWindow(parent, id, pos, size, style, name)
{
}

void HtmlWindow::OnLinkClicked(const wxHtmlLinkInfo& link)
{
	wxString linkhref = link.GetHref();
	if (linkhref.StartsWith(wxT("http://")))
	{
		if(!wxLaunchDefaultBrowser(linkhref))
			// failed to launch externally, so open internally
			wxHtmlWindow::OnLinkClicked(link);
	}
	else
	{
		wxHtmlWindow::OnLinkClicked(link);
	}
}

class KeySelectDialog;
// Define a new frame type: this is going to be our main frame
class MyDialog : public wxDialog
{
public:
	// ctor(s)
	MyDialog(const wxString& title, MyApp *_app);
	void loadOgre();
	void addAboutEntry(wxString name, wxString desc, wxString url, int &x, int &y);
	void addAboutTitle(wxString name, int &x, int &y);
	void updateRendersystems(Ogre::RenderSystem *rs);
	void SetDefaults();
	bool LoadConfig();
	void SaveConfig();
	void updateSettingsControls(); // use after loading or after manually updating the settings map
	void getSettingsControls();    // puts the control's status into the settings map
	// event handlers (these functions should _not_ be virtual)
	void OnQuit(wxCloseEvent& event);
	void OnTimer(wxTimerEvent& event);
	void OnTimerReset(wxTimerEvent& event);
	void OnButCancel(wxCommandEvent& event);
	void OnButSave(wxCommandEvent& event);
	void OnButPlay(wxCommandEvent& event);
	void OnButRestore(wxCommandEvent& event);
	void OnButTestNet(wxCommandEvent& event);
	void OnButGetUserToken(wxCommandEvent& event);
#if wxCHECK_VERSION(2, 8, 0)
	void OnClickedHtmlLinkMain(wxHtmlLinkEvent& event);
	void OnClickedHtmlLinkUpdate(wxHtmlLinkEvent& event);
#endif // version 2.8
	void OnChoiceShadow(wxCommandEvent& event);
	void OnChoiceLanguage(wxCommandEvent& event);
	//void OnButRemap(wxCommandEvent& event);
	void OnChoiceRenderer(wxCommandEvent& event);
	//void OnMenuClearClick(wxCommandEvent& event);
	//void OnMenuTestClick(wxCommandEvent& event);
	//void OnMenuDeleteClick(wxCommandEvent& event);
	//void OnMenuDuplicateClick(wxCommandEvent& event);
	//void OnMenuEditEventNameClick(wxCommandEvent& event);
	//void OnMenuCheckDoublesClick(wxCommandEvent& event);
	void OnButRegenCache(wxCommandEvent& event);
	void OnButClearCache(wxCommandEvent& event);
	void OnButUpdateRoR(wxCommandEvent& event);
	void OnButCheckOpenCL(wxCommandEvent& event);
	void OnButCheckOpenCLBW(wxCommandEvent& event);
	void updateRoR();
	void OnScrollSightRange(wxScrollEvent& event);
	void OnScrollVolume(wxScrollEvent& event);
	void OnScrollForceFeedback(wxScrollEvent& event);
	void OnScrollFPSLimiter(wxScrollEvent& event);
	void OnChangedNotebook1(wxNotebookEvent& event);
	void OnChangedNotebook2(wxNotebookEvent& event);
	std::string readVersionInfo();

//	void checkLinuxPaths();
	MyApp *app;

private:

	//bool postinstall;
	//wxButton *btnRemap;
	//wxCheckBox *autodl;
	//wxCheckBox *hydrax;
	//wxCheckBox *rtshader;
	//wxComboBox *ctrlTypeCombo;
	//wxStaticText *controlText;
	KeySelectDialog *kd;
	Ogre::Root *ogreRoot;
	int controlItemCounter;
	std::map<int,wxTreeItemId> treeItems;
	std::vector<std::string>     renderer_name;
	std::vector<wxStaticText *>  renderer_text;
	std::vector<wxValueChoice *> renderer_choice;
	wxButton *btnAddKey;
	wxButton *btnDeleteKey;
	wxButton *btnUpdate, *btnToken;
	wxCheckBox *advanced_logging;
	wxCheckBox *arcadeControls;
	wxCheckBox *beam_break_debug;
	wxCheckBox *beam_deform_debug;
	wxCheckBox *collisions;
	wxCheckBox *creaksound;
	wxCheckBox *dcm;
	wxCheckBox *debug_envmap;
	wxCheckBox *debug_triggers;
	wxCheckBox *debug_vidcam;
	wxCheckBox *dismap;
	wxCheckBox *dof;
	wxCheckBox *dofdebug;
	wxCheckBox *dtm;
	wxCheckBox *envmap;
	wxCheckBox *extcam;
	wxCheckBox *ffEnable;
	wxCheckBox *glow;
	wxCheckBox *hdr;
	wxCheckBox *heathaze;
	wxCheckBox *ingame_console;
	wxCheckBox *dev_mode;
	wxCheckBox *mblur;
	wxCheckBox *mirror;
	wxCheckBox *nocrashrpt;
	wxCheckBox *particles;
	wxCheckBox *posstor;
	wxCheckBox *replaymode;
	wxCheckBox *selfcollisions;
	wxCheckBox *shadowOptimizations;
	wxCheckBox *skidmarks;
	wxCheckBox *sunburn;
	wxCheckBox *thread;
	wxCheckBox *waves;
	wxNotebook *nbook;
	wxPanel *debugPanel;
	wxPanel *gamePanel;
	wxPanel *graphicsPanel;
	wxPanel *rsPanel;
	wxPanel *settingsPanel;
	wxScrolledWindow *aboutPanel;
	wxSlider *ffCamera;
	wxSlider *ffCenter;
	wxSlider *ffHydro;
	wxSlider *ffOverall;
	wxSlider *fpsLimiter;
	wxSlider *sightRange;
	wxSlider *soundVolume;
	wxStaticText *ffCameraText;
	wxStaticText *ffCenterText;
	wxStaticText *ffHydroText;
	wxStaticText *ffOverallText;
	wxStaticText *fpsLimiterText;
	wxStaticText *sightRangeText;
	wxStaticText *soundVolumeText;
	wxTextCtrl *fovint, *fovext;
	wxTextCtrl *gputext;
	wxTextCtrl *presel_map, *presel_truck;
	wxTimer *timer1;
	wxValueChoice *flaresMode;
	wxValueChoice *gearBoxMode;
	wxValueChoice *inputgrab;
	wxValueChoice *languageMode;
	wxValueChoice *renderer;
	wxValueChoice *screenShotFormat;
	wxValueChoice *shadow;
	wxValueChoice *sky;
	wxValueChoice *sound;
	wxValueChoice *textfilt;
	wxValueChoice *vegetationMode;
	wxValueChoice *water;

#ifdef NETWORK
	wxCheckBox *network_enable;
	wxTextCtrl *nickname;
	wxTextCtrl *servername;
	wxTextCtrl *serverport;
	wxTextCtrl *serverpassword;
	wxTextCtrl *usertoken;
	wxHtmlWindow *networkhtmw;
	wxHtmlWindow *helphtmw;
//	wxTextCtrl *p2pport;
#endif

	void tryLoadOpenCL();
	int openCLAvailable;
//	wxTextCtrl *deadzone;
	//wxTimer *controlstimer;
	// any class wishing to process wxWidgets events must use this macro
	DECLARE_EVENT_TABLE()
	bool loadOgrePlugins(Ogre::String pluginsfile);
};


class IDData : public wxTreeItemData
{
public:
	IDData(int id) : id(id) {}
	int id;
};

enum control_ids
{
	button_add_key = 1,
	button_cancel,
	button_check_opencl,
	button_check_opencl_bw,
	button_clear_cache,
	button_delete_key,
	button_get_user_token,
	button_load_keymap,
	button_net_test,
	button_play,
	button_regen_cache,
	button_restore,
	button_save,
	button_save_keymap,
	button_update_ror,
	changed_notebook_1,
	changed_notebook_2,
	changed_notebook_3,
	choice_language,
	choice_renderer,
	choice_shadow,
	clicked_html_link_main,
	clicked_html_link_update,
	scroll_force_feedback,
	scroll_fps_limiter,
	scroll_sight_range,
	scroll_volume,
	timer_controls,
	timer_update_reset,
};

// ----------------------------------------------------------------------------
// event tables and other macros for wxWidgets
// ----------------------------------------------------------------------------
// the event tables connect the wxWidgets events with the functions (event
// handlers) which process them. It can be also done at run-time, but for the
// simple menu events like this the static method is much simpler.
BEGIN_EVENT_TABLE(MyDialog, wxDialog)
	EVT_BUTTON(button_cancel, MyDialog::OnButCancel)
	EVT_BUTTON(button_check_opencl, MyDialog::OnButCheckOpenCL)
	EVT_BUTTON(button_check_opencl_bw, MyDialog::OnButCheckOpenCLBW)
	EVT_BUTTON(button_clear_cache, MyDialog::OnButClearCache)
	EVT_BUTTON(button_get_user_token, MyDialog::OnButGetUserToken)
	EVT_BUTTON(button_net_test, MyDialog::OnButTestNet)
	EVT_BUTTON(button_play, MyDialog::OnButPlay)
	EVT_BUTTON(button_regen_cache, MyDialog::OnButRegenCache)
	EVT_BUTTON(button_restore, MyDialog::OnButRestore)
	EVT_BUTTON(button_save, MyDialog::OnButSave)
	EVT_BUTTON(button_update_ror, MyDialog::OnButUpdateRoR)
	EVT_CHOICE(choice_language, MyDialog::OnChoiceLanguage)
	EVT_CHOICE(choice_renderer, MyDialog::OnChoiceRenderer)
	EVT_CHOICE(choice_shadow, MyDialog::OnChoiceShadow)
	EVT_CLOSE(MyDialog::OnQuit)
	EVT_COMMAND_SCROLL(scroll_force_feedback, MyDialog::OnScrollForceFeedback)
	EVT_COMMAND_SCROLL(scroll_fps_limiter, MyDialog::OnScrollFPSLimiter)
	EVT_COMMAND_SCROLL(scroll_sight_range, MyDialog::OnScrollSightRange)
	EVT_COMMAND_SCROLL(scroll_volume, MyDialog::OnScrollVolume)
#if wxCHECK_VERSION(2, 8, 0)
	EVT_HTML_LINK_CLICKED(clicked_html_link_main, MyDialog::OnClickedHtmlLinkMain)
	EVT_HTML_LINK_CLICKED(clicked_html_link_update, MyDialog::OnClickedHtmlLinkUpdate)
#endif  // wxCHECK_VERSION(2, 8, 0)
	EVT_NOTEBOOK_PAGE_CHANGED(changed_notebook_1, MyDialog::OnChangedNotebook1)
	EVT_NOTEBOOK_PAGE_CHANGED(changed_notebook_2, MyDialog::OnChangedNotebook2)
	EVT_TIMER(timer_controls, MyDialog::OnTimer)
	EVT_TIMER(timer_update_reset, MyDialog::OnTimerReset)
END_EVENT_TABLE()

// Create a new application object: this macro will allow wxWidgets to create
// the application object during program execution (it's better than using a
// static object for many reasons) and also implements the accessor function
// wxGetApp() which will return the reference of the right type (i.e. MyApp and
// not wxApp)

#ifdef USE_CONSOLE
IMPLEMENT_APP_CONSOLE(MyApp)
#else
IMPLEMENT_APP(MyApp)
#endif

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// the application class
// ----------------------------------------------------------------------------

const wxLanguageInfo *getLanguageInfoByName(const wxString name)
{
	if(name.size() == 2)
	{
		for(int i=0;i<wxLANGUAGE_USER_DEFINED;i++)
		{
			const wxLanguageInfo *info = wxLocale::GetLanguageInfo(i);
			if(!info)
				continue;
			wxString cn = info->CanonicalName.substr(0,2);
			if(cn == name)
				return info;
		}
	} else
	{
		for(int i=0;i<wxLANGUAGE_USER_DEFINED;i++)
		{
			const wxLanguageInfo *info = wxLocale::GetLanguageInfo(i);
			if(!info)
				continue;
			if(info->Description == name)
				return info;
		}
	}
	return 0;
}

int getAvLang(wxString dir, std::vector<wxLanguageInfo*> &files)
{
	if(!wxDir::Exists(dir)) return 1;
	
	//WX portable version
	wxDir dp(dir);
	if (!dp.IsOpened())
	{
		wxLogStatus(wxT("error opening ") + dir);
		return -1;
	}
	wxString name;
	if (dp.GetFirst(&name, wxEmptyString, wxDIR_DIRS))
	{
		do
		{
			if (name.StartsWith(wxT(".")))
			{
				// do not add dot directories
				continue;
			}
			if(name.Len() != 2)
			{
				// do not add other than language 2 letter code style files
				continue;
			}
			wxLanguageInfo *li = const_cast<wxLanguageInfo *>(getLanguageInfoByName(name.substr(0,2)));
			if(li)
			{
				files.push_back(li);
			} else
			{
				wxLogStatus(wxT("failed to get Language: "+name));
			}
		} while (dp.GetNext(&name));
	}
	return 0;
}

void initLanguage(wxString languagePath, wxString userpath)
{
	// add language stuff

	// Initialize the catalogs we'll be using
	wxString dirsep = wxT("/");
	wxString basedir = languagePath;
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
	dirsep = wxT("\\");
#endif
	//basedir = basedir + dirsep + wxString("languages");
	wxLocale::AddCatalogLookupPathPrefix(languagePath);

	// get all available languages
	getAvLang(languagePath, avLanguages);
	wxLogStatus(wxT("searching languages in ")+basedir);
	if(avLanguages.size() > 0)
	{
		wxLogStatus(wxT("Available Languages:"));
		std::vector<wxLanguageInfo *>::iterator it;
		for(it = avLanguages.begin(); it!=avLanguages.end(); it++)
		{
			if(*it)
			{
				wxLogStatus(wxT(" * ") + (*it)->Description + wxT("(") + (*it)->CanonicalName + wxT(")"));
			}
		}
	} else
	{
		wxLogStatus(wxT("no Languages found!"));
	}

	try
	{
		wxString rorcfg=userpath + dirsep + wxT("config") + dirsep + wxT("RoR.cfg");
		Ogre::ImprovedConfigFile cfg;
		// Don't trim whitespace
		cfg.load((const char*)rorcfg.mb_str(wxConvUTF8), "=:\t", false);
		wxString langSavedName = conv(cfg.getSetting("Language"));

		if(langSavedName.size() > 0)
			language = const_cast<wxLanguageInfo *>(getLanguageInfoByName(langSavedName));
	}catch(...)
	{
		wxLogError(wxT(" unable to load RoR.cfg"));
	}

	if(language == 0)
	{
		wxLogStatus(wxT("asking system for default language."));
		language = const_cast<wxLanguageInfo *>(wxLocale::GetLanguageInfo(lang_locale.GetSystemLanguage()));
		if(language)
		{
			wxLogStatus(wxT(" system returned: ") + language->Description + wxT("(") + language->CanonicalName + wxT(")"));
		} else
		{
			wxLogStatus(wxT(" error getting language information!"));
		}
	}
	wxLogStatus(wxT("preferred language: ")+language->Description);
	wxString lshort = language->CanonicalName.substr(0, 2);
	wxString tmp = basedir + dirsep + lshort + dirsep + wxT("LC_MESSAGES") + dirsep + wxT("ror.mo");
	wxLogStatus(wxT("lang file: ") + tmp);
	if(wxFileName::FileExists(tmp))
	{
		wxLogStatus(wxT("language existing, using it!"));
#if wxCHECK_VERSION(2, 8, 0)
		if(!lang_locale.IsAvailable((wxLanguage)language->Language))
		{
			wxLogStatus(wxT("language file existing, but not found via wxLocale!"));
			wxLogStatus(wxT("is the language installed on your system?"));
		}
#endif // version 2.8
		bool res = lang_locale.Init((wxLanguage)language->Language);
		if(!res)
		{
			wxLogStatus(wxT("error while initializing language!"));
		}
		res = lang_locale.AddCatalog(wxT("ror.mo"));
		if(!res)
		{
			wxLogStatus(wxT("error while loading language!"));
		}
	}
	else
	{
		lang_locale.Init(wxLANGUAGE_DEFAULT);
		wxLogStatus(wxT("language not existing, no lang_locale support!"));
	}
}

void MyApp::recurseCopy(wxString sourceDir, wxString destinationDir)
{
	if (!wxDir::Exists(sourceDir) || !wxDir::Exists(destinationDir)) return;

	wxDir srcd(sourceDir);
	wxString src;
	if (!srcd.GetFirst(&src)) return; //empty dir!
	do
	{
		//ignore files and directories beginning with "." (important, for SVN!)
		if (src.StartsWith(wxT("."))) continue;
		//check if it id a directory
		wxFileName tsfn=wxFileName(sourceDir, wxEmptyString);
		tsfn.AppendDir(src);
		if (wxDir::Exists(tsfn.GetPath()))
		{
			//this is a directory, create dest dir and recurse
			wxFileName tdfn=wxFileName(destinationDir, wxEmptyString);
			tdfn.AppendDir(src);
			if (!wxDir::Exists(tdfn.GetPath())) tdfn.Mkdir();
			recurseCopy(tsfn.GetPath(), tdfn.GetPath());
		}
		else
		{
			//this is a file, copy file
			tsfn=wxFileName(sourceDir, src);
			wxFileName tdfn=wxFileName(destinationDir, src);
			//policy to be defined
			if (tdfn.FileExists())
			{
				//file already exists, should we overwrite?
//				::wxCopyFile(tsfn.GetFullPath(), tdfn.GetFullPath());
			}
			else
			{
				::wxCopyFile(tsfn.GetFullPath(), tdfn.GetFullPath());
			}
		}
	} while (srcd.GetNext(&src));
}

// from http://wiki.wxwidgets.org/WxZipInputStream
bool MyApp::extractZipFiles(const wxString& aZipFile, const wxString& aTargetDir)
{
	bool ret = true;
	//wxFileSystem fs;
	std::auto_ptr<wxZipEntry> entry(new wxZipEntry());
	do
	{
		wxFileInputStream in(aZipFile);
		if (!in)
		{
			wxLogError(_T("Can not open file '")+aZipFile+wxT("'."));
			ret = false;
			break;
		}
		wxZipInputStream zip(in);

		while (entry.reset(zip.GetNextEntry()), entry.get() != NULL)
		{
			// access meta-data
			wxString name = entry->GetName();
			name = aTargetDir + wxFileName::GetPathSeparator() + name;

			// read 'zip' to access the entry's data
			if (entry->IsDir())
			{
				int perm = entry->GetMode();
				wxFileName::Mkdir(name, perm, wxPATH_MKDIR_FULL);
			} else
			{
				zip.OpenEntry(*entry.get());
				if (!zip.CanRead())
				{
					wxLogError(_T("Can not read zip entry '") + entry->GetName() + wxT("'."));
					ret = false;
					break;
				}

				wxFileOutputStream file(name);

				if (!file)
				{
					wxLogError(_T("Can not create file '")+name+wxT("'."));
					ret = false;
					break;
				}
				zip.Read(file);
			}
		}
	} while(false);
	return ret;
}

bool MyApp::checkUserPath()
{
	wxFileName configPath = wxFileName(UserPath, wxEmptyString);
	configPath.AppendDir(wxT("config"));
	wxString configPathStr = configPath.GetFullPath();
	// check if the user path is valid, if not create it

	if (!wxFileName::DirExists(UserPath) || !wxFileName::DirExists(configPathStr))
	{
		if(!wxFileName::DirExists(UserPath))
			wxFileName::Mkdir(UserPath);

		std::auto_ptr< wxZipEntry > entry;

		// first: figure out the zip path
		wxFileName skeletonZip = wxFileName(ProgramPath, wxEmptyString);
		skeletonZip.AppendDir(wxT("resources"));
		skeletonZip.SetFullName(wxT("skeleton.zip"));
		wxString skeletonZipFile = skeletonZip.GetFullPath();

		if(!wxFileName::FileExists(skeletonZipFile))
		{
			// try the dev dir as well
			skeletonZip = wxFileName(ProgramPath, wxEmptyString);
			skeletonZip.RemoveLastDir();
			skeletonZip.AppendDir(wxT("resources"));
			skeletonZip.SetFullName(wxT("skeleton.zip"));
			skeletonZipFile = skeletonZip.GetFullPath();
		}

		if(!wxFileName::FileExists(skeletonZipFile))
		{
			// tell the user
			wxString warning = wxString::Format(_("Rigs of Rods User directory missing:\n%s\n\nit could not be created since skeleton.zip was not found in\n %s"), UserPath.c_str(), skeletonZipFile.c_str());
			wxString caption = _("error upon loading RoR user directory");
			wxMessageDialog *w = new wxMessageDialog(NULL, warning, caption, wxOK|wxICON_ERROR|wxSTAY_ON_TOP, wxDefaultPosition);
			w->ShowModal();
			delete(w);
			exit(1);
		}

		// unpack
		extractZipFiles(skeletonZipFile, UserPath);

		// tell the user
		wxLogInfo(wxT("User directory created as it was not existing: ") + UserPath);
		/*
		wxString warning = wxString::Format(_("Rigs of Rods User directory recreated, as it was missing:\n%s"), UserPath.c_str());
		wxString caption = _("error upon loading RoR user directory");
		wxMessageDialog *w = new wxMessageDialog(NULL, warning, caption, wxOK|wxICON_ERROR|wxSTAY_ON_TOP, wxDefaultPosition);
		w->ShowModal();
		*/
	} else
	{
		wxLogInfo(wxT("User directory seems to be valid: ") + UserPath);
	}
	return true;
}

bool MyApp::filesystemBootstrap()
{
	UserPath = conv(SSETTING("User Path", ""));
	ProgramPath = conv(SSETTING("Program Path", ""));


	checkUserPath();
	return true;
}

void MyApp::initLogging()
{
	// log everything
	wxLog::SetLogLevel(wxLOG_Max);
	wxLog::SetVerbose();

	// use stdout always
	wxLog *logger_cout = new wxLogStream(&std::cout);
	wxLog::SetActiveTarget(logger_cout);

	wxFileName clfn=wxFileName(UserPath, wxEmptyString);
	clfn.AppendDir(wxT("logs"));
	clfn.SetFullName(wxT("configlog.txt"));
	FILE *f = fopen(conv(clfn.GetFullPath()).c_str(), "w");
	if(f)
	{
		// and a file log
		wxLog *logger_file = new wxLogStderr(f);
		wxLogChain *logChain = new wxLogChain(logger_file);
	}
	wxLogStatus(wxT("log started"));
}

// this is run to enter the main loop
int MyApp::OnRun()
{
	// our special run method to catch exceptions
	int res = 0;
	try
	{
		res = MainLoop();
	}
	catch (std::exception &e)
	{
		wxLogError(wxT("Cought exception:"));
		wxLogError(wxString(e.what(), wxConvUTF8));
		wxString warning = wxString(e.what(), wxConvUTF8);
		wxString caption = _("caught exception");
		wxMessageDialog *w = new wxMessageDialog(NULL, warning, caption, wxOK|wxICON_ERROR|wxSTAY_ON_TOP, wxDefaultPosition);
		w->ShowModal();
		delete(w);
	}
	return res;
}

// 'Main program' equivalent: the program execution "starts" here
bool MyApp::OnInit()
{
	try
	{
		//setup the user filesystem
		if(!SETTINGS.setupPaths())
			return false;

		if (!filesystemBootstrap()) return false;

		initLogging();
		initLanguage(wxT("languages"), UserPath);

#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
		// add windows specific crash handler. It will create nice memory dumps, so we can track the error
		//LPVOID lpvState = Install(NULL, NULL, NULL);
		//AddFile(lpvState, conv("configlog.txt"), conv("Rigs of Rods Configurator Crash log"));
#endif

		// call the base class initialization method, currently it only parses a
		// few common command-line options but it could be do more in the future
		if ( !wxApp::OnInit() )
			return false;

		wxLogStatus(wxT("Creating dialog"));
		// create the main application window
		wxString title = wxString::Format(_("Rigs of Rods version %s configuration"), wxT(ROR_VERSION_STRING));
		MyDialog *dialog = new MyDialog(title, this);

		// and show it (the frames, unlike simple controls, are not shown when
		// created initially)
		wxLogStatus(wxT("Showing dialog"));
		dialog->Show(true);
		SetTopWindow(dialog);
		SetExitOnFrameDelete(false);
		// success: wxApp::OnRun() will be called which will enter the main message
		// loop and the application will run. If we returned false here, the
		// application would exit immediately.
		wxLogStatus(wxT("App ready"));
	} catch (std::exception &e)
	{
		wxLogError(wxT("Cought exception:"));
		wxLogError(wxString(e.what(), wxConvUTF8));
		wxString warning = wxString(e.what(), wxConvUTF8);
		wxString caption = _("caught exception");
		wxMessageDialog *w = new wxMessageDialog(NULL, warning, caption, wxOK|wxICON_ERROR|wxSTAY_ON_TOP, wxDefaultPosition);
		w->ShowModal();
		delete(w);
	}	
	return true;
}

void MyApp::OnInitCmdLine(wxCmdLineParser& parser)
{
	parser.SetDesc (g_cmdLineDesc);
	// must refuse '/' as parameter starter or cannot use "/path" style paths
	parser.SetSwitchChars (conv("/"));
}

bool MyApp::OnCmdLineParsed(wxCmdLineParser& parser)
{
	postinstall = parser.Found(conv("postinstall"));
	return true;
}

// ----------------------------------------------------------------------------
// main frame
// ----------------------------------------------------------------------------

// frame constructor
MyDialog::MyDialog(const wxString& title, MyApp *_app) : wxDialog(NULL, wxID_ANY, title,  wxPoint(100, 100), wxSize(500, 580), wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER), openCLAvailable(0)
{
	app=_app;

#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE
	SetWindowVariant(wxWINDOW_VARIANT_MINI); //smaller texts for macOS
#endif
	SetWindowStyle(wxRESIZE_BORDER | wxCAPTION);
	//SetMinSize(wxSize(300,300));
	
	kd=0;


	//postinstall = _postinstall;

	wxFileSystem::AddHandler( new wxInternetFSHandler );
	//build dialog here
	wxToolTip::Enable(true);
	wxToolTip::SetDelay(100);
	//image
//	wxBitmap *bitmap=new wxBitmap("data\\config.png", wxBITMAP_TYPE_BMP);
	//wxLogStatus(wxT("Loading bitmap"));
	wxSizer *mainsizer = new wxBoxSizer(wxVERTICAL);
	mainsizer->SetSizeHints(this);

	// using xpm now
	const wxBitmap bm(config_xpm);
	wxStaticPicture *imagePanel = new wxStaticPicture(this, -1, bm,wxPoint(0, 0), wxSize(500, 100), wxNO_BORDER);
	mainsizer->Add(imagePanel, 0, wxGROW);

	//notebook
	nbook=new wxNotebook(this, changed_notebook_1, wxPoint(3, 100), wxSize(490, 415));
	mainsizer->Add(nbook, 1, wxGROW);

	wxSizer *btnsizer = new wxBoxSizer(wxHORIZONTAL);
	//buttons
	if(!app->postinstall)
	{
		wxButton *btnRestore = new wxButton( this, button_restore, _("Restore Defaults"), wxPoint(35,520), wxSize(100,25));
		btnRestore->SetToolTip(_("Restore the default configuration."));
		btnsizer->Add(btnRestore, 1, wxGROW | wxALL, 5);

		wxButton *btnPlay = new wxButton( this, button_play, _("Save and Play"), wxPoint(140,520), wxSize(100,25));
		btnPlay->SetToolTip(_("Save the current configuration and start RoR."));
		btnsizer->Add(btnPlay, 1, wxGROW | wxALL, 5);

		wxButton *btnSaveAndExit = new wxButton( this, button_save, _("Save and Exit"), wxPoint(245,520), wxSize(100,25));
		btnSaveAndExit->SetToolTip(_("Save the current configuration and close the configuration program."));
		btnsizer->Add(btnSaveAndExit, 1, wxGROW | wxALL, 5);

		wxButton *btnClose = new wxButton( this, button_cancel, _("Cancel"), wxPoint(350,520), wxSize(100,25));
		btnClose->SetToolTip(_("Cancel the configuration changes and close the configuration program."));
		btnsizer->Add(btnClose, 1, wxGROW | wxALL, 5);
	} else
	{
		wxButton *btnPlay = new wxButton( this, button_play, _("Save and Play"), wxPoint(350,520), wxSize(100,25));
		btnPlay->SetToolTip(_("Save the current configuration and start RoR."));
		btnsizer->Add(btnPlay, 1, wxGROW | wxALL, 5);
	}
	mainsizer->Add(btnsizer, 0, wxGROW);
	this->SetSizer(mainsizer);

	// settings
	settingsPanel=new wxPanel(nbook, -1);
	nbook->AddPage(settingsPanel, _("Settings"), true);
	wxSizer *settingsSizer = new wxBoxSizer(wxVERTICAL);
	//settingsSizer->SetSizeHints(this);
	settingsPanel->SetSizer(settingsSizer);
	// second notebook containing the settings tabs
	wxNotebook *snbook=new wxNotebook(settingsPanel, changed_notebook_2, wxPoint(0, 0), wxSize(100,250));
	settingsSizer->Add(snbook, 1, wxGROW);

	rsPanel=new wxPanel(snbook, -1);
	snbook->AddPage(rsPanel, _("Render System"), true);

	graphicsPanel=new wxPanel(snbook, -1);
	snbook->AddPage(graphicsPanel, _("Graphics"), true);

	gamePanel=new wxPanel(snbook, -1);
	snbook->AddPage(gamePanel, _("Gameplay"), true);

	// Controls
	wxPanel *controlsPanel=new wxPanel(nbook, -1);
	nbook->AddPage(controlsPanel, _("Controls"), false);
	wxSizer *controlsSizer = new wxBoxSizer(wxVERTICAL);
	controlsPanel->SetSizer(controlsSizer);
	// third notebook for control tabs
	wxNotebook *ctbook=new wxNotebook(controlsPanel, changed_notebook_3, wxPoint(0, 0), wxSize(100,250));
	controlsSizer->Add(ctbook, 1, wxGROW);

	wxPanel *ctsetPanel=new wxPanel(ctbook, -1);
	ctbook->AddPage(ctsetPanel, _("Info"), false);
	wxStaticText *dText2 = new wxStaticText(ctsetPanel, -1, _("Since 0.4.5, you can use the ingame key mapping system. \nYou can also edit the input mappings by hand by using a texteditor.\nThe input mappings are stored in the following file:\nMy Documents\\Rigs of Rods\\config\\input.map"), wxPoint(10,10));

#if wxCHECK_VERSION(2, 8, 0)
	wxHyperlinkCtrl *link1 = new wxHyperlinkCtrl(ctsetPanel, -1, _("(more help here)"), _("http://www.rigsofrods.com/wiki/pages/Input.map"), wxPoint(10, 100));
#endif // version 2.8

	wxPanel *ffPanel=new wxPanel(ctbook, -1);
	ctbook->AddPage(ffPanel, _("Force Feedback"), false);
#ifdef NETWORK
	wxPanel *netPanel=new wxPanel(nbook, -1);
	nbook->AddPage(netPanel, _("Network"), false);
#endif
	wxPanel *advancedPanel=new wxPanel(snbook, -1);
	snbook->AddPage(advancedPanel, _("Advanced"), false);

	debugPanel=new wxPanel(snbook, -1);
	snbook->AddPage(debugPanel, _("Debug"), true);


#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
	// this is removed for now
#if 0
	wxPanel *updatePanel=new wxPanel(nbook, -1);
	nbook->AddPage(updatePanel, _("Updates"), false);
#endif //0
#endif // WIN32
	aboutPanel=new wxScrolledWindow (nbook, -1);
	nbook->AddPage(aboutPanel, _("About"), false);

#ifdef USE_OPENCL
	wxPanel *GPUPanel=new wxPanel(nbook, -1);
	nbook->AddPage(GPUPanel, _("OpenCL"), false);
#endif // USE_OPENCL

	wxStaticText *dText = 0;

	// clear the renderer settings and fill them later
	dText = new wxStaticText(rsPanel, -1, _("Render System"), wxPoint(10, 28));
	renderer = new wxValueChoice(rsPanel, choice_renderer, wxPoint(220, 25), wxSize(220, -1), 0);
	// renderer options done, do the rest

	// gamePanel
	int y = 10;
	int x_row1 = 150, x_row2 = 300;

	dText = new wxStaticText(gamePanel, -1, _("Language:"), wxPoint(10, y));
	languageMode=new wxValueChoice(gamePanel, choice_language, wxPoint(x_row1, y), wxSize(200, -1), wxCB_READONLY);

	int sel = 0;
	languageMode->AppendValueItem(wxT("English (U.S.)"));
	if(avLanguages.size() > 0)
	{
		int counter = 1;
		std::vector<wxLanguageInfo*>::iterator it;
		for(it = avLanguages.begin(); it!=avLanguages.end(); it++, counter++)
		{
			if(*it == language)
				sel = counter;
			languageMode->AppendValueItem((*it)->Description);
		}
	}

	languageMode->SetSelection(sel);
	languageMode->SetToolTip(_("This setting overrides the system's default language. You need to restart the configurator to apply the changes."));
	y+=35;

	// Gearbox
	dText = new wxStaticText(gamePanel, -1, _("Default Gearbox mode:"), wxPoint(10,y));
	gearBoxMode=new wxValueChoice(gamePanel, -1, wxPoint(x_row1, y), wxSize(200, -1), 0);
	gearBoxMode->AppendValueItem(wxT("Automatic shift"),                       _("Automatic shift"));
	gearBoxMode->AppendValueItem(wxT("Manual shift - Auto clutch"),            _("Manual shift - Auto clutch"));
	gearBoxMode->AppendValueItem(wxT("Fully Manual: sequential shift"),        _("Fully Manual: sequential shift"));
	gearBoxMode->AppendValueItem(wxT("Fully Manual: stick shift"),             _("Fully Manual: stick shift"));
	gearBoxMode->AppendValueItem(wxT("Fully Manual: stick shift with ranges"), _("Fully Manual: stick shift with ranges"));
	gearBoxMode->SetToolTip(_("The default mode for the gearbox when a new vehicle is loaded."));
	y+=35;

	dText = new wxStaticText(gamePanel, -1, _("FOV External:"), wxPoint(10,y+3));
	fovext = new wxTextCtrl(gamePanel, -1, wxT("60"), wxPoint(x_row1, y), wxSize(200, -1), 0);
	y+=35;

	dText = new wxStaticText(gamePanel, -1, _("FOV Internal:"), wxPoint(10,y+3));
	fovint = new wxTextCtrl(gamePanel, -1, wxT("75"), wxPoint(x_row1, y), wxSize(200, -1), 0);
	y+=35;

	extcam=new wxCheckBox(gamePanel, -1, _("Disable Camera Pitching"), wxPoint(x_row1, y));
	extcam->SetToolTip(_("If you dislike the pitching external vehicle camera, you can disable it here."));
	y+=25;

	arcadeControls=new wxCheckBox(gamePanel, -1, _("Arcade Controls"), wxPoint(x_row1, y));
	arcadeControls->SetToolTip(_("Braking will switch into reverse gear and accelerate."));
	y+=25;

	dText = new wxStaticText(gamePanel, -1, _("User Token: "), wxPoint(10,y+3));
	usertoken=new wxTextCtrl(gamePanel, -1, wxString(), wxPoint(x_row1, y), wxSize(200, -1));
	usertoken->SetToolTip(_("Your rigsofrods.com User Token."));
	btnToken = new wxButton(gamePanel, button_get_user_token, _("Get Token"), wxPoint(x_row1+210, y), wxSize(90,25));
	y+=35;

#ifdef USE_OPENAL
	// creak sound?
	creaksound=new wxCheckBox(gamePanel, -1, _("Disable creak sound"), wxPoint(x_row1, y));
	creaksound->SetToolTip(_("You can disable the default creak sound by checking this box"));
#endif //USE_OPENAL

	// aboutPanel
	y = 10;
	x_row1 = 10;
	x_row2 = 300;

	addAboutTitle(_("Version"), x_row1, y);
	dText = new wxStaticText(aboutPanel, -1, wxString::Format(_("Rigs of Rods version: %s"), wxT(ROR_VERSION_STRING)), wxPoint(x_row1 + 15, y));
	y += dText->GetSize().GetHeight() + 2;

	dText = new wxStaticText(aboutPanel, -1, wxString::Format(_("Network Protocol version: %s"), wxString(RORNET_VERSION, wxConvUTF8).c_str()), wxPoint(x_row1 + 15, y));
	y += dText->GetSize().GetHeight() + 2;

	// those required SVN Keyword to work properly, broken since the switch to HG
	//dText = new wxStaticText(aboutPanel, -1, wxString::Format(_("Revision: %s"), wxT(SVN_REVISION)), wxPoint(x_row1 + 15, y));
	//y += dText->GetSize().GetHeight() + 2;

	//dText = new wxStaticText(aboutPanel, -1, wxString::Format(_("Full revision: %s"), wxT(SVN_ID)), wxPoint(x_row1 + 15, y));
	//y += dText->GetSize().GetHeight() + 2;

	dText = new wxStaticText(aboutPanel, -1, wxString::Format(_("Build time: %s, %s"), wxT(__DATE__), wxT(__TIME__)), wxPoint(x_row1 + 15, y));
	y += dText->GetSize().GetHeight() + 2;


#ifdef _WIN32
#ifdef _CPPRTTI
	dText = new wxStaticText(aboutPanel, -1, wxT("Built with Run-Time Type Information (RTTI, /GR)"), wxPoint(x_row1 + 15, y)); y += dText->GetSize().GetHeight() + 2;
#endif //_CPPRTTI
#ifdef _CPPUNWIND
	dText = new wxStaticText(aboutPanel, -1, wxT("Built with Enable Exception Handling (/GX)"), wxPoint(x_row1 + 15, y)); y += dText->GetSize().GetHeight() + 2;
#endif //_CPPUNWIND
#ifdef _DEBUG
	dText = new wxStaticText(aboutPanel, -1, _("Built in DEBUG mode (/LDd, /MDd, or /MTd.)"), wxPoint(x_row1 + 15, y)); y += dText->GetSize().GetHeight() + 2;
#endif //_DEBUG

	dText = new wxStaticText(aboutPanel, -1, wxString::Format(_T("Max Integer bit size: %d"), _INTEGRAL_MAX_BITS), wxPoint(x_row1 + 15, y)); y += dText->GetSize().GetHeight() + 2;
#if _M_IX86_FP == 1
	dText = new wxStaticText(aboutPanel, -1, _("Built for ARCH SSE (/arch:SSE)"), wxPoint(x_row1 + 15, y)); y += dText->GetSize().GetHeight() + 2;
#elif _M_IX86_FP == 2
	dText = new wxStaticText(aboutPanel, -1, _("Built for ARCH SSE2 (/arch:SSE2)"), wxPoint(x_row1 + 15, y)); y += dText->GetSize().GetHeight() + 2;
#endif // _M_IX86_FP
#ifdef _M_X64
	dText = new wxStaticText(aboutPanel, -1, _("Built for x64 processors"), wxPoint(x_row1 + 15, y)); y += dText->GetSize().GetHeight() + 2;
#endif // _M_IX86_FP
#endif // _WIN32

	y += 20;

	addAboutTitle(_("Authors"), x_row1, y);
	addAboutEntry(_("Authors"), _("You can find a complete list of the RoR's authors ingame: about->credits."), wxT("mailto:support@rigsofrods.com"), x_row1, y);

	y += 20;

	addAboutTitle(_("Missing someone?"), x_row1, y);
	addAboutEntry(_("Missing someone?"), _("If we are missing someone on this list, please drop us a line at:\nsupport@rigsofrods.com"), wxT("mailto:support@rigsofrods.com"), x_row1, y);

	wxSize size = nbook->GetBestVirtualSize();
	size.x = 400;
	aboutPanel->SetVirtualSize(size);
	aboutPanel->SetScrollRate(20, 25);
	
	// complete about panel
	//aboutPanel->FitInside();

	// debugPanel
	y = 10;
	x_row1 = 150;
	x_row2 = 300;

	dText = new wxStaticText(debugPanel, -1, _("These settings are for debugging RoR in various ways.\nIf you do not know how to use these features, stay away from them."), wxPoint(10, y));
	y+=45;

	ingame_console=new wxCheckBox(debugPanel, -1, _("Ingame Console"), wxPoint(10, y));
	ingame_console->SetToolTip(_("Enables the Scripting Console ingame. Use the ~ key."));
	y+=15;

	dev_mode = new wxCheckBox(debugPanel, -1, _("Developer mode"), wxPoint(10, y));
	dev_mode->SetToolTip(_("Unlocks disabled & unfinished features for developement"));
	y += 15;

	dtm=new wxCheckBox(debugPanel, -1, _("Debug Truck Mass"), wxPoint(10, y));
	dtm->SetToolTip(_("Prints all node masses to the RoR.log"));
	y+=15;

	advanced_logging=new wxCheckBox(debugPanel, -1, _("Advanced Logging"), wxPoint(10, y));
	advanced_logging->SetToolTip(_("Enables fancy HTML logs for loaded terrains, objects and trucks"));
	y+=15;

	beam_break_debug=new wxCheckBox(debugPanel, -1, _("Beam Break Debug"), wxPoint(10, y));
	beam_break_debug->SetToolTip(_("Logs the beam info to RoR.log whenever a beam breaks"));
	y+=15;

	beam_deform_debug=new wxCheckBox(debugPanel, -1, _("Beam Deform Debug"), wxPoint(10, y));
	beam_deform_debug->SetToolTip(_("Logs the beam info to RoR.log whenever a beam deforms"));
	y+=15;

	debug_vidcam=new wxCheckBox(debugPanel, -1, _("Debug VideoCameras"), wxPoint(10, y)); //VideoCameraDebug
	debug_vidcam->SetToolTip(_("Adds a virtuals camera mesh that should help you position the camera correctly"));
	y+=15;

	debug_envmap=new wxCheckBox(debugPanel, -1, _("Debug EnvironmentMapping / Chrome"), wxPoint(10, y));
	debug_envmap->SetToolTip(_("Displays an unwrapped cube on what is used to project the reflections on vehicles"));
	y+=15;

	debug_triggers=new wxCheckBox(debugPanel, -1, _("Trigger Debug"), wxPoint(10, y));
	debug_triggers->SetToolTip(_("Enables Trigger debug messages"));
	y+=15;
	
	dcm=new wxCheckBox(debugPanel, -1, _("Debug Collision Meshes"), wxPoint(10, y));
	dcm->SetToolTip(_("Shows all Collision meshes in Red to be able to position them correctly. Only use for debugging!"));
	y+=15;

	dofdebug=new wxCheckBox(debugPanel, -1, _("Enable Depth of Field Debug"), wxPoint(10, y));
	dofdebug->SetToolTip(_("Shows the DOF debug display on the screen in order to identify DOF problems"));
	y+=15;

	nocrashrpt=new wxCheckBox(debugPanel, -1, _("Disable Crash Reporting"), wxPoint(10, y));
	nocrashrpt->SetToolTip(_("Disables the crash handling system. Only use for debugging purposes"));
	y+=25;
	
	dText = new wxStaticText(debugPanel, -1, _("Input Grabbing:"), wxPoint(10,y+3));
	inputgrab=new wxValueChoice(debugPanel, -1, wxPoint(x_row1, y), wxSize(200, -1), 0);
	inputgrab->AppendValueItem(wxT("All"),         _("All"));
	inputgrab->AppendValueItem(wxT("Dynamically"), _("Dynamically"));
	inputgrab->AppendValueItem(wxT("None"),        _("None"));
	inputgrab->SetToolTip(_("Determines the input capture mode. All is the default, Dynamically is good for windowed mode."));
	inputgrab->SetSelection(0); // All
	y+=35;

    dText = new wxStaticText(debugPanel, -1, _("Preselected Map: "), wxPoint(10, y));
	presel_map=new wxTextCtrl(debugPanel, -1, wxString(), wxPoint(x_row1, y), wxSize(170, -1));
	presel_map->SetToolTip(_("The terrain you want to load upon RoR startup. Might remove the startup selection menu."));
	y+=25;

    dText = new wxStaticText(debugPanel, -1, _("Preselected Truck: "), wxPoint(10,y));
	presel_truck=new wxTextCtrl(debugPanel, -1, wxString(), wxPoint(x_row1, y), wxSize(170, -1));
	presel_truck->SetToolTip(_("The truck you want to load upon RoR startup. Might remove the startup selection menu."));
	y+=25;

#if wxCHECK_VERSION(2, 8, 0)
	wxHyperlinkCtrl *link = new wxHyperlinkCtrl(debugPanel, -1, _("(Read more on how to use these options here)"), _("http://www.rigsofrods.com/wiki/pages/Debugging_Trucks"), wxPoint(10, y));
#endif // version 2.8

	// graphics panel
	y = 10;
	x_row1 = 150;
	x_row2 = 300;
	dText = new wxStaticText(graphicsPanel, -1, _("Texture filtering:"), wxPoint(10,y+3));
	textfilt=new wxValueChoice(graphicsPanel, -1, wxPoint(x_row1, y), wxSize(200, -1), 0);
	textfilt->AppendValueItem(wxT("None (fastest)"), _("None (fastest)"));
	textfilt->AppendValueItem(wxT("Bilinear"), _("Bilinear"));
	textfilt->AppendValueItem(wxT("Trilinear"), _("Trilinear"));
	textfilt->AppendValueItem(wxT("Anisotropic (best looking)"), _("Anisotropic (best looking)"));
	textfilt->SetToolTip(_("Most recent hardware can do Anisotropic filtering without significant slowdown.\nUse lower settings only on old or poor video chipsets."));
	textfilt->SetSelection(3); //anisotropic
	y+=25;

	dText = new wxStaticText(graphicsPanel, -1, _("Sky type:"), wxPoint(10,y+3));
	sky=new wxValueChoice(graphicsPanel, -1, wxPoint(x_row1, y), wxSize(200, -1), 0);
	sky->AppendValueItem(wxT("Sandstorm (fastest)"), _("Sandstorm (fastest)"));
	sky->AppendValueItem(wxT("Caelum (best looking, slower)"), _("Caelum (best looking, slower)"));
	sky->SetToolTip(_("Caelum sky is nice but quite slow unless you have a high-powered GPU."));
	sky->SetSelection(1); //Caelum
	y+=25;

	dText = new wxStaticText(graphicsPanel, wxID_ANY, _("Shadow type:"), wxPoint(10,y+3));
	shadow=new wxValueChoice(graphicsPanel, choice_shadow, wxPoint(x_row1, y), wxSize(200, -1), 0);
	shadow->AppendValueItem(wxT("No shadows (fastest)"), _("No shadows (fastest)"));
	shadow->AppendValueItem(wxT("Texture shadows"), _("Texture shadows"));
#if OGRE_VERSION>0x010602
	shadow->AppendValueItem(wxT("Parallel-split Shadow Maps"), _("Parallel-split Shadow Maps"));
#endif //OGRE_VERSION
	shadow->SetToolTip(_("Shadow technique to be used ingame."));
	sky->SetSelection(1); //Texture shadows
	y+=25;

	dText = new wxStaticText(graphicsPanel, wxID_ANY, _("Sightrange:"), wxPoint(10,y+3));
	sightRange=new wxSlider(graphicsPanel, scroll_sight_range, 5000, 10, 5000, wxPoint(x_row1, y), wxSize(200, -1));
	sightRange->SetToolTip(_("determines how far you can see in meters"));
	sightRangeText = new wxStaticText(graphicsPanel, wxID_ANY, _("Unlimited"), wxPoint(x_row1 + 210,y+3));
	y+=25;

	shadowOptimizations=new wxCheckBox(graphicsPanel, wxID_ANY, _("Shadow Performance Optimizations"), wxPoint(x_row1, y), wxSize(200, -1), 0);
	shadowOptimizations->SetToolTip(_("When turned on, it disables the vehicles shadow when driving in first person mode."));
	y+=25;

	dText = new wxStaticText(graphicsPanel, -1, _("Water type:"), wxPoint(10,y+3));
	water=new wxValueChoice(graphicsPanel, -1, wxPoint(x_row1, y), wxSize(200, -1), 0);
	//water->AppendValueItem(wxT("None"), _("None")); //we don't want that!
	water->AppendValueItem(wxT("Basic (fastest)"), _("Basic (fastest)"));
	water->AppendValueItem(wxT("Reflection"), _("Reflection"));
	water->AppendValueItem(wxT("Reflection + refraction (speed optimized)"), _("Reflection + refraction (speed optimized)"));
	water->AppendValueItem(wxT("Reflection + refraction (quality optimized)"), _("Reflection + refraction (quality optimized)"));
	water->AppendValueItem(wxT("Hydrax"), _("Hydrax"));
	water->SetToolTip(_("Water reflection is slower, and requires a good GPU. In speed optimized mode you may experience excessive camera shaking."));
	y+=25;

	waves=new wxCheckBox(graphicsPanel, -1, _("Enable waves"), wxPoint(x_row1, y+3));
	waves->SetToolTip(_("Enabling waves adds a lot of fun in boats, but can have a big performance impact on some specific hardwares."));
	y+=25;

	dText = new wxStaticText(graphicsPanel, -1, _("Vegetation:"), wxPoint(10, y+3));
	vegetationMode=new wxValueChoice(graphicsPanel, -1, wxPoint(x_row1, y), wxSize(200, -1), 0);
	vegetationMode->AppendValueItem(wxT("None (fastest)"), _("None (fastest)"));
	vegetationMode->AppendValueItem(wxT("20%"), _("20%"));
	vegetationMode->AppendValueItem(wxT("50%"), _("50%"));
	vegetationMode->AppendValueItem(wxT("Full (best looking, slower)"), _("Full (best looking, slower)"));
	vegetationMode->SetToolTip(_("This determines how much grass and how many trees (and such objects) should get displayed."));
	vegetationMode->SetSelection(3); // full
	y+=25;

	dText = new wxStaticText(graphicsPanel, -1, _("Particle systems:"), wxPoint(10, y));
	particles=new wxCheckBox(graphicsPanel, -1, _("Enable Particle Systems"), wxPoint(x_row1, y));
	particles->SetToolTip(_("This may hurt framerate a bit on old systems, but it looks pretty good."));
	heathaze=new wxCheckBox(graphicsPanel, -1, _("HeatHaze"), wxPoint(x_row2, y));
	heathaze->SetToolTip(_("Heat Haze from engines, major slowdown. (only activate with recent hardware)"));
	heathaze->Disable();
	y+=25;

	dText = new wxStaticText(graphicsPanel, -1, _("Cockpit options:"), wxPoint(10, y));
	mirror=new wxCheckBox(graphicsPanel, -1, _("Mirrors"), wxPoint(x_row1, y));
	mirror->SetToolTip(_("Shows the rear view mirrors in 1st person view. May cause compatibility problems for very old video cards."));
	y+=25;

	dText = new wxStaticText(graphicsPanel, -1, _("Visual effects:"), wxPoint(10, y));
	sunburn=new wxCheckBox(graphicsPanel, -1, _("Sunburn"), wxPoint(x_row1, y));
	sunburn->SetToolTip(_("Requires a recent video card. Adds a bluish blinding effect."));
	sunburn->Disable();

	hdr=new wxCheckBox(graphicsPanel, -1, _("HDR"), wxPoint(x_row2, y));
	hdr->SetToolTip(_("Requires a recent video card. Add a lightning effect that simulates the light sensitivity of the human eye."));
	y+=15;

	mblur=new wxCheckBox(graphicsPanel, -1, _("Motion blur"), wxPoint(x_row1, y));
	mblur->SetToolTip(_("Requires a recent video card. Adds a motion blur effect."));

	skidmarks=new wxCheckBox(graphicsPanel, -1, _("Skidmarks"), wxPoint(x_row2, y));
	skidmarks->SetToolTip(_("Adds tire tracks to the ground."));
	//skidmarks->Disable();
	y+=15;

	envmap=new wxCheckBox(graphicsPanel, -1, _("HQ reflections"), wxPoint(x_row1, y));
	envmap->SetToolTip(_("Enables high quality reflective effects. Causes a slowdown."));
	y+=15;

	glow=new wxCheckBox(graphicsPanel, -1, _("Glow"), wxPoint(x_row1, y));
	glow->SetToolTip(_("Adds a glow effect to lights"));
	glow->Disable();

	dof=new wxCheckBox(graphicsPanel, -1, _("DOF"), wxPoint(x_row2, y));
	dof->SetToolTip(_("Adds a nice depth of field effect to the scene."));
	y+=25;

	dText = new wxStaticText(graphicsPanel, -1, _("Screenshot Format:"), wxPoint(10, y));
	screenShotFormat=new wxValueChoice(graphicsPanel, -1, wxPoint(x_row1, y), wxSize(200, -1), 0);
	screenShotFormat->AppendValueItem(wxT("jpg (smaller, default)"), _("jpg (smaller, default)"));
	screenShotFormat->AppendValueItem(wxT("png (bigger, no quality loss)"), _("png (bigger, no quality loss)"));
	screenShotFormat->SetToolTip(_("In what Format should screenshots be saved?"));


	//force feedback panel
	ffEnable=new wxCheckBox(ffPanel, -1, _("Enable Force Feedback"), wxPoint(150, 25));

	dText = new wxStaticText(ffPanel, -1, _("Overall force level:"), wxPoint(20,53));
	ffOverall=new wxSlider(ffPanel, scroll_force_feedback, 100, 0, 1000, wxPoint(150, 50), wxSize(200, 40));
	ffOverall->SetToolTip(_("Adjusts the level of all the forces."));
	ffOverallText=new wxStaticText(ffPanel, -1, wxString(), wxPoint(360,53));

	dText = new wxStaticText(ffPanel, -1, _("Steering feedback level:"), wxPoint(20,103));
	ffHydro=new wxSlider(ffPanel, scroll_force_feedback, 100, 0, 4000, wxPoint(150, 100), wxSize(200, 40));
	ffHydro->SetToolTip(_("Adjusts the contribution of forces coming from the wheels and the steering mechanism."));
	ffHydroText=new wxStaticText(ffPanel, -1, wxString(), wxPoint(360,103));

	dText = new wxStaticText(ffPanel, -1, _("Self-centering level:"), wxPoint(20,153));
	ffCenter=new wxSlider(ffPanel, scroll_force_feedback, 100, 0, 4000, wxPoint(150, 150), wxSize(200, 40));
	ffCenter->SetToolTip(_("Adjusts the self-centering effect applied to the driving wheel when driving at high speed."));
	ffCenterText=new wxStaticText(ffPanel, -1, wxString(), wxPoint(360,153));

	dText = new wxStaticText(ffPanel, -1, _("Inertia feedback level:"), wxPoint(20,203));
	ffCamera=new wxSlider(ffPanel, scroll_force_feedback, 100, 0, 4000, wxPoint(150, 200), wxSize(200, 40));
	ffCamera->SetToolTip(_("Adjusts the contribution of forces coming shocks and accelerations (this parameter is currently unused)."));
	ffCamera->Enable(false);
	ffCameraText=new wxStaticText(ffPanel, -1, wxString(), wxPoint(360,203));

	//update text boxes
	wxScrollEvent dummye;
	OnScrollForceFeedback(dummye);

	//network panel
#ifdef NETWORK
	wxSizer *sizer_net = new wxBoxSizer(wxVERTICAL);

	wxPanel *panel_net_top = new wxPanel(netPanel);
	network_enable=new wxCheckBox(panel_net_top, -1, _("Enable network mode"), wxPoint(5, 5));
	network_enable->SetToolTip(_("This switches RoR into network mode.\nBe aware that many features are not available in network mode.\nFor example, you not be able to leave your vehicle or hook objects."));

	dText = new wxStaticText(panel_net_top, -1, _("Nickname: "), wxPoint(230,7));
	nickname=new wxTextCtrl(panel_net_top, -1, _("Anonymous"), wxPoint(300, 2), wxSize(170, -1));
	nickname->SetToolTip(_("Your network name. Maximum 20 Characters."));

	sizer_net->Add(panel_net_top, 0, wxGROW);

	//dText = new wxStaticText(netPanel, -1, wxString("Servers list: "), wxPoint(5,5));
	wxSizer *sizer_net_middle = new wxBoxSizer(wxHORIZONTAL);
	networkhtmw = new wxHtmlWindow(netPanel, clicked_html_link_main, wxPoint(0, 30), wxSize(480, 270));
	//networkhtmw->LoadPage(REPO_HTML_SERVERLIST);
	networkhtmw->SetPage(_("<p>How to play in Multiplayer:</p><br><ol><li>Click on the <b>Update</b> button to see available servers here.</li><li>Click on any underlined Server in the list.</li><li>Click on <b>Save and Play</b> button to start the game.</li></ol>"));
	networkhtmw->SetToolTip(_("Click on blue hyperlinks to select a server."));
	sizer_net_middle->Add(networkhtmw);
	sizer_net->Add(networkhtmw, 2, wxGROW | wxALL);

	wxPanel *panel_net_bottom = new wxPanel(netPanel);
	dText = new wxStaticText(panel_net_bottom, -1, _("Server host name: "), wxPoint(10,5));
	servername=new wxTextCtrl(panel_net_bottom, -1, _("127.0.0.1"), wxPoint(150, 5), wxSize(200, -1));
	servername->SetToolTip(_("This field is automatically filled if you click on an hyperlink in the list.\nDo not change it."));

	dText = new wxStaticText(panel_net_bottom, -1, _("Server port number: "), wxPoint(10,30));
	serverport=new wxTextCtrl(panel_net_bottom, -1, _("12333"), wxPoint(150, 30), wxSize(100, -1));
	serverport->SetToolTip(_("This field is automatically filled if you click on an hyperlink in the list.\nDo not change it."));
//	dText = new wxStaticText(netPanel, -1, wxString("(default is 12333)"), wxPoint(260,103));

	dText = new wxStaticText(panel_net_bottom, -1, _("Server password: "), wxPoint(10,55));
	serverpassword=new wxTextCtrl(panel_net_bottom, -1, wxString(), wxPoint(150, 55), wxSize(100, -1));
	serverpassword->SetToolTip(_("The server password, if required."));
	sizer_net->Add(panel_net_bottom, 0, wxGROW);

	btnUpdate = new wxButton(panel_net_bottom, button_net_test, _("Update"), wxPoint(360, 5), wxSize(110,65));

	netPanel->SetSizer(sizer_net);

#endif

	//advanced panel
	y = 10;
	dText = new wxStaticText(advancedPanel, -1, _("You do not need to change these settings unless you experience problems"), wxPoint(10, y));
	y+=30;

#ifdef USE_OPENAL
	dText = new wxStaticText(advancedPanel, -1, _("Audio device:"), wxPoint(10,y+3));
	sound = new wxValueChoice(advancedPanel, -1, wxPoint(x_row1, y), wxSize(280, -1), 0);
	sound->SetToolTip(_("Select the appropriate sound source.\nLeaving to Default should work most of the time."));


	// get sound devices

	wxLogStatus(wxT("Known Audio Devices: "));
	char *devices = (char *)alcGetString(NULL, ALC_ALL_DEVICES_SPECIFIER);
	while(devices && *devices != 0)
	{
		// missing header fr this:
		//ALCdevice *device = alcOpenDevice(devices);
		//wxLogStatus(wxString(device->szDeviceName));
		sound->AppendValueItem(wxString(conv(devices)));
		wxLogStatus(wxT(" *") + wxString(conv(devices)));
		devices += strlen(devices) + 1; //next device
	}
	y+=35;

	wxLogStatus(wxT("Sound Extensions: "));
	char *extensions = (char *)alcGetString(NULL, ALC_ENUMERATE_ALL_EXT);

	wxStringTokenizer tkz(wxString(conv(extensions)), wxT(" "));
	while ( tkz.HasMoreTokens() )
	{
		wxString token = tkz.GetNextToken();
		wxLogStatus(wxT(" *") + token);
	}

	dText = new wxStaticText(advancedPanel, -1, _("Sound Volume:"), wxPoint(10, y+3));
	soundVolume=new wxSlider(advancedPanel, scroll_volume, 100, 0, 100, wxPoint(x_row1, y), wxSize(200, -1));
	soundVolume->SetToolTip(_("sets the master volume"));
	soundVolumeText = new wxStaticText(advancedPanel, -1, _("100 %"), wxPoint(x_row1 + 210, y+3));
	y+=35;
#endif //USE_OPENAL

	// Lights
	dText = new wxStaticText(advancedPanel, -1, _("Light source effects:"), wxPoint(10, y+3));
	flaresMode=new wxValueChoice(advancedPanel, -1, wxPoint(x_row1, y), wxSize(280, -1), 0);
//	flaresMode->AppendValueItem(wxT("None (fastest)"), _("None (fastest)")); //this creates a bug in the autopilot
	flaresMode->AppendValueItem(wxT("No light sources"), _("No light sources"));
	flaresMode->AppendValueItem(wxT("Only current vehicle, main lights"), _("Only current vehicle, main lights"));
	flaresMode->AppendValueItem(wxT("All vehicles, main lights"), _("All vehicles, main lights"));
	flaresMode->AppendValueItem(wxT("All vehicles, all lights"), _("All vehicles, all lights"));
	flaresMode->SetToolTip(_("Determines which lights will project light on the environment.\nThe more light sources are used, the slowest it will be."));
	y+=35;

	// FPS-Limiter
	dText = new wxStaticText(advancedPanel, -1, _("FPS-Limiter:"), wxPoint(10, y+3));
	fpsLimiter=new wxSlider(advancedPanel, scroll_fps_limiter, 200, 10, 200, wxPoint(x_row1, y), wxSize(200, -1));
	fpsLimiter->SetToolTip(_("sets the maximum frames per second"));
	fpsLimiterText = new wxStaticText(advancedPanel, -1, _("Unlimited"), wxPoint(x_row1 + 210, y+3));
	y+=35;

	// Cache
	dText = new wxStaticText(advancedPanel, -1, _("In case the mods cache becomes corrupted, \nuse these buttons to fix the cache."), wxPoint(10, y));

	wxButton *btn = new wxButton(advancedPanel, button_regen_cache, _("Regen cache"), wxPoint(x_row2-52, y));
	btn->SetToolTip(_("Use this to regenerate the cache outside of RoR. If this does not work, use the clear cache button."));
	
	btn = new wxButton(advancedPanel, button_clear_cache, _("Clear cache"), wxPoint(x_row2+43, y));
	btn->SetToolTip(_("Use this to remove the whole cache and force the generation from ground up."));
	y+=45;

	// Various Settings
	dText = new wxStaticText(advancedPanel, -1, _("Various Settings:"), wxPoint(10, y+3));
	replaymode = new wxCheckBox(advancedPanel, -1, _("Replay Mode"), wxPoint(x_row1, y));
	replaymode->SetToolTip(_("Replay mode. (Will affect your frame rate)"));
	y+=15;
	posstor = new wxCheckBox(advancedPanel, -1, _("Enable Position Storage"), wxPoint(x_row1, y));
	posstor->SetToolTip(_("Can be used to quick save and load positions of trucks"));
	y+=15;
	/*
	autodl = new wxCheckBox(advancedPanel, -1, _("Enable Auto-Downloader"), wxPoint(x_row1, y));
	autodl->SetToolTip(_("This enables the automatic downloading of missing mods when using Multiplayer"));
	autodl->Disable();
	y+=15;
	hydrax=new wxCheckBox(advancedPanel, -1, _("Hydrax Water System"), wxPoint(x_row1, y));
	hydrax->SetToolTip(_("Enables the new water rendering system. EXPERIMENTAL. Overrides any water settings."));
	hydrax->Disable();
	y+=15;
	rtshader=new wxCheckBox(advancedPanel, -1, _("RT Shader System"), wxPoint(x_row1, y));
	rtshader->SetToolTip(_("Enables the Runtime Shader generation System. EXPERIMENTAL"));
	rtshader->Disable();
	y+=15;
	*/
	dismap=new wxCheckBox(advancedPanel, -1, _("Disable Overview Map"), wxPoint(x_row1, y));
	dismap->SetToolTip(_("Disabled the map. This is for testing purposes only, you should not gain any FPS with that."));
	y+=15;

	thread=new wxCheckBox(advancedPanel, -1, _("Disable multi-threading"), wxPoint(x_row1, y));
	thread->SetToolTip(_("Disables all separate physics threads (might be an option to avoid camera stuttering)."));
	y+=15;

	collisions=new wxCheckBox(advancedPanel, -1, _("Disable inter-vehicle collisions"), wxPoint(x_row1, y));
	collisions->SetToolTip(_("Disables the collision detection between multiple trucks."));
	y+=15;

	selfcollisions=new wxCheckBox(advancedPanel, -1, _("Disable intra-vehicle collisions"), wxPoint(x_row1, y));
	selfcollisions->SetToolTip(_("Disables the collision detection within one truck (no self-collisions)."));
	y+=15;

	// controls settings panel
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
#if 0
	wxSizer *sizer_updates = new wxBoxSizer(wxVERTICAL);
	helphtmw = new wxHtmlWindow(updatePanel, update_html, wxPoint(0, 0), wxSize(480, 380));
	helphtmw->SetPage(_("... loading ... (maybe you should check your internet connection)"));
	// tooltip is confusing there, better none!
	//helphtmw->SetToolTip("here you can get help");
	sizer_updates->Add(helphtmw, 1, wxGROW);

	// update button replaced with html link
	// update button only for windows users
	//wxButton *btnu = new wxButton(updatePanel, update_ror, "Update now");
	//sizer_updates->Add(btnu, 0, wxGROW);

	updatePanel->SetSizer(sizer_updates);
#endif //0
#endif // WIN32

#ifdef USE_OPENCL
	wxSizer *sizer_gpu = new wxBoxSizer(wxVERTICAL);

	wxSizer *sizer_gpu2 = new wxBoxSizer(wxHORIZONTAL);
	const wxBitmap bm_ocl(opencllogo_xpm);
	wxStaticPicture *openCLImagePanel = new wxStaticPicture(GPUPanel, wxID_ANY, bm_ocl, wxPoint(0, 0), wxSize(200, 200), wxNO_BORDER);
	sizer_gpu2->Add(openCLImagePanel, 0, wxALL|wxALIGN_CENTER_VERTICAL, 3);

	gputext = new wxTextCtrl(GPUPanel, wxID_ANY, wxString("Please use the buttons below"), wxDefaultPosition, wxDefaultSize, wxTE_READONLY|wxTE_MULTILINE);
	sizer_gpu2->Add(gputext, 1, wxGROW, 3);

	sizer_gpu->Add(sizer_gpu2, 1, wxGROW, 3);

	wxButton *btng = new wxButton(GPUPanel, button_check_opencl, _("Check for OpenCL Support"));
	sizer_gpu->Add(btng, 0, wxGROW, 3);

	wxButton *btnw = new wxButton(GPUPanel, button_check_opencl_bw, _("Check OpenCL Bandwidth"));
	sizer_gpu->Add(btnw, 0, wxGROW, 3);

	GPUPanel->SetSizer(sizer_gpu);
#endif // USE_OPENCL

	//	controlstimer=new wxTimer(this, CONTROLS_TIMER_ID);
	timer1=new wxTimer(this, timer_update_reset);

	// inititalize ogre only when we really need it due to long startup times
	ogreRoot = 0;

	// select default tabs
	// default is settings -> gameplay
	nbook->SetSelection(0);
	snbook->SetSelection(2);

	wxLogStatus(wxT("Setting default values"));
	SetDefaults();
	wxLogStatus(wxT("Loading config"));
	LoadConfig();

	// centers dialog window on the screen
	Show();
	SetSize(500,600);
	Centre();

	// important: show before we load ogre, since ogre loading can take some time
	loadOgre();
}

void MyDialog::addAboutTitle(wxString name, int &x, int &y)
{
	wxStaticText *dText = new wxStaticText(aboutPanel, wxID_ANY, name, wxPoint(x, y));
	wxFont dfont=dText->GetFont();
	dfont.SetWeight(wxFONTWEIGHT_BOLD);
	dfont.SetPointSize(dfont.GetPointSize()+3);
	dText->SetFont(dfont);
	y += dText->GetSize().y;
}

void MyDialog::addAboutEntry(wxString name, wxString desc, wxString url, int &x, int &y)
{

	wxSize s;
#if wxCHECK_VERSION(2, 8, 0)
	if(!url.empty())
	{
		wxHyperlinkCtrl *link = new wxHyperlinkCtrl(aboutPanel, wxID_ANY, name, url, wxPoint(x+15, y), wxSize(250, 25), wxHL_ALIGN_LEFT|wxHL_CONTEXTMENU);
		link->SetNormalColour(wxColour(wxT("BLACK")));
		link->SetHoverColour(wxColour(wxT("LIGHT GREY")));
		link->SetVisitedColour(wxColour(wxT("ORANGE")));
		wxFont dfont=link->GetFont();
		dfont.SetWeight(wxFONTWEIGHT_BOLD);
		dfont.SetPointSize(dfont.GetPointSize()+1);
		link->SetFont(dfont);
		s = link->GetSize();
	} else
#endif // version 2.8
	{
		wxStaticText *dText = new wxStaticText(aboutPanel, wxID_ANY, name, wxPoint(x+15, y));
		wxFont dfont=dText->GetFont();
		dfont.SetWeight(wxFONTWEIGHT_BOLD);
		dfont.SetPointSize(dfont.GetPointSize()+1);
		dText->SetFont(dfont);
		s = dText->GetSize();
	}
	y += s.y;
	wxStaticText *dText = new wxStaticText(aboutPanel, wxID_ANY, desc, wxPoint(x+30, y));
	wxFont dfont=dText->GetFont();
	dfont.SetPointSize(dfont.GetPointSize()-1);
	dText->SetFont(dfont);
	y += dText->GetSize().y + 2;

}

bool MyDialog::loadOgrePlugins(Ogre::String pluginsfile)
{
	Ogre::StringVector pluginList;
	Ogre::String pluginDir;
	Ogre::ConfigFile cfg;

	try
	{
		cfg.load( pluginsfile );
	}
	catch (Ogre::Exception)
	{
		Ogre::LogManager::getSingleton().logMessage(pluginsfile + " not found, automatic plugin loading disabled.");
		return false;
	}

	pluginDir = cfg.getSetting("PluginFolder"); // Ignored on Mac OS X, uses Resources/ directory
	pluginList = cfg.getMultiSetting("Plugin");

#if OGRE_PLATFORM != OGRE_PLATFORM_APPLE && OGRE_PLATFORM != OGRE_PLATFORM_IPHONE
	if (pluginDir.empty())
	{
		// User didn't specify plugins folder, try current one
		pluginDir = ".";
	}
#endif

	char last_char = pluginDir[pluginDir.length()-1];
	if (last_char != '/' && last_char != '\\')
	{
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
		pluginDir += "\\";
#elif OGRE_PLATFORM == OGRE_PLATFORM_LINUX
		pluginDir += "/";
#endif
	}

	for( Ogre::StringVector::iterator it = pluginList.begin(); it != pluginList.end(); ++it )
	{
		Ogre::String pluginFilename = pluginDir + (*it);
		try
		{
			ogreRoot->loadPlugin(pluginFilename);
		} catch(Ogre::Exception &e)
		{
			wxLogStatus(wxT("failed to load plugin: ") + conv(pluginFilename) + wxT(": ") + conv(e.getFullDescription()));
		}
	}
	return true;
}

void MyDialog::loadOgre()
{
	if(ogreRoot) return;
	wxLogStatus(wxT("Creating Ogre root"));
	//we must do this once
	wxFileName tcfn=wxFileName(app->UserPath, wxEmptyString);
	tcfn.AppendDir(wxT("config"));
	wxString confdirPrefix=tcfn.GetPath()+wxFileName::GetPathSeparator();

	wxFileName tlfn=wxFileName(app->UserPath, wxEmptyString);
	tlfn.AppendDir(wxT("logs"));
	wxString logsdirPrefix=tlfn.GetPath()+wxFileName::GetPathSeparator();

	wxString progdirPrefix=app->ProgramPath+wxFileName::GetPathSeparator();
	const char *pluginsfile="plugins.cfg";
	// load plugins manually to catch errors
	ogreRoot = new Ogre::Root("",
									Ogre::String((const char*)confdirPrefix.mb_str(wxConvUTF8))+"ogre.cfg",
									Ogre::String((const char*)logsdirPrefix.mb_str(wxConvUTF8))+"RoRConfig.log");

	// load plugins manually
	loadOgrePlugins(Ogre::String((const char*)progdirPrefix.mb_str(wxConvUTF8))+pluginsfile);

	wxLogStatus(wxT("Root restore config"));
	try
	{
		ogreRoot->restoreConfig();
	} catch (Ogre::Exception& e)
	{
		if(e.getSource() == "D3D9RenderSystem::setConfigOption")
		{
			// this is a normal error that happens when the suers switch from ogre 1.6 to 1.7
		} else
		{
			wxString warning = conv(e.getFullDescription());
			wxString caption = _("error upon restoring Ogre Configuration");

			wxMessageDialog *w = new wxMessageDialog(this, warning, caption, wxOK, wxDefaultPosition);
			w->ShowModal();
			delete(w);
		}
	}
	updateRendersystems(ogreRoot->getRenderSystem());
}

void MyDialog::OnChoiceShadow(wxCommandEvent &e)
{
	// TBD
	bool enable = (shadow->GetSelection() != 0);

	if(enable)
	{
		shadowOptimizations->Enable();
	} else
	{
		shadowOptimizations->Disable();
	}
}

void MyDialog::OnChoiceLanguage(wxCommandEvent& event)
{
	wxString warning = _("You must save and restart the program to activate the new language!");
	wxString caption = _("new Language selected");

	wxMessageDialog *w = new wxMessageDialog(this, warning, caption, wxOK, wxDefaultPosition);
	w->ShowModal();
	delete(w);
}


#if 0

// translations for the render menu
// do not remove, this is used even if its not compiled
_("Yes");
_("No");
_("FSAA");
_("Full Screen");
_("Rendering Device");
_("VSync");
_("Video Mode");
#endif //0

void MyDialog::updateRendersystems(Ogre::RenderSystem *rs)
{
	// beware: rs may be null if no config file is present (this is normal)
	std::map<Ogre::String, bool> filterOptions;
	filterOptions["Allow NVPerfHUD"]=true;
	filterOptions["Floating-point mode"]=true;
	filterOptions["Resource Creation Policy"]=true;
	filterOptions["VSync Interval"]=true;
	filterOptions["sRGB Gamma Conversion"]=true;
	filterOptions["Colour Depth"]=true;

	if(renderer->GetCount() == 0)
	{
		// add all rendersystems to the list
		const Ogre::RenderSystemList list = ogreRoot->getAvailableRenderers();
		int selection = 0;
		int valuecounter = 0;
		for(Ogre::RenderSystemList::const_iterator it=list.begin(); it!=list.end(); it++, valuecounter++)
		{
			if(rs && rs->getName() == (*it)->getName())
				selection = valuecounter;
			else if(!rs) {
				rs = (*it);
				ogreRoot->setRenderSystem(rs);
			}
			renderer->AppendValueItem(conv((*it)->getName()));
		}
		renderer->SetSelection(selection);
	}

	int x = 10;
	int y = 55;
	
	wxStaticLine *w = new wxStaticLine(rsPanel, -1, wxPoint(x, y+3), wxSize(440, 2));
	y += 15;

	int counter = 0;
	if(!rs)
	{
		wxString warning = _("Unable to load the render systems. Please check if all required files are there and the plugins.cfg file is correct.\nThis is a fatal error and the game will not start.");
		wxString caption = _("Error: no rendersystems found");
		wxMessageDialog *msg_dialog = new wxMessageDialog(this, warning, caption, wxOK, wxDefaultPosition);
		msg_dialog->ShowModal();
		delete(msg_dialog);
		return;
	}
	Ogre::ConfigOptionMap opts = rs->getConfigOptions();
	Ogre::ConfigOptionMap::iterator optIt = opts.begin();
	for(optIt=opts.begin(); optIt!=opts.end(); optIt++)
	{
		// filter out unwanted or disabled options
		if(filterOptions.find(optIt->first) != filterOptions.end())
			continue;
		if(optIt->second.immutable)
			continue;

		if(counter + 1 > (int)renderer_choice.size())
		{
			// not existing, add new control
			renderer_text.resize(counter + 1);
			renderer_name.resize(counter + 1);
			renderer_choice.resize(counter + 1);

			renderer_text[counter] = new wxStaticText(rsPanel, wxID_ANY, conv("."), wxPoint(x, y+3), wxSize(210, 25));
			renderer_choice[counter] = new wxValueChoice(rsPanel, wxID_ANY, wxPoint(x + 220, y), wxSize(220, -1), 0);
			renderer_name[counter] = std::string();
		} else
		{
			//existing, remove all elements
			renderer_choice[counter]->Clear();
		}

		if(optIt->second.possibleValues.empty())
		{
			wxLogStatus(wxString::Format(wxT("option: \"%s\" has no values! IGNORING option."), conv(optIt->first.c_str()).c_str() ));
			continue;
		}

		wxString s = conv(optIt->first.c_str());
		renderer_name[counter] = optIt->first;
#ifdef _WIN32
		renderer_text[counter]->SetLabel(_(s));
#else
		renderer_text[counter]->SetLabel(wxGetTranslation(s));
#endif // _WIN32
		renderer_text[counter]->Show();

		// add all values and select current value
		Ogre::String selection = "";
		int valueCounter = 0;


		for(Ogre::StringVector::iterator valIt = optIt->second.possibleValues.begin(); valIt != optIt->second.possibleValues.end(); valIt++)
		{
			if(*valIt == optIt->second.currentValue)
				selection = *valIt;

			// some debug:
			//wxLogStatus(wxString::Format(wxT("option: \"%s\" . \"%s\""), conv(optIt->first.c_str()).c_str(), conv(valIt->c_str()).c_str() ));

			Ogre::String valStr = *valIt;

			// filter video modes
			if(optIt->first == "Video Mode")
			{
				int res_x=-1, res_y=-1, res_d=-1;
				int res = sscanf(valIt->c_str(), "%d x %d @ %d-bit colour", &res_x, &res_y, &res_d);

				wxLogStatus(wxString::Format(wxT("parsed resolution \"%s\" as \"%d x %d @ %d-bit\""), conv(valIt->c_str()).c_str(), res_x, res_y, res_d));

				// opengl has no colour info in there
				if(res == 3)
				{
					// discard low resolutions and 16 bit modes
					if(res_d < 32)
					{
						wxLogStatus(wxT("discarding resolution as it is below 32 bits: ") + conv(valIt->c_str()));
						continue;
					}
				}
				if(res_x < 800)
				{
					wxLogStatus(wxT("discarding resolution as the x res is below 800 pixels: ") + conv(valIt->c_str()));
					continue;
				}

				// according to http://en.wikipedia.org/wiki/Display_resolution
				const char *ratio_str = "";
				if     ( (float)res_x/(float)res_y - 1.25 < 0.0001f)
					ratio_str = "5:4";
				else if( (float)res_x/(float)res_y - 1.333333f < 0.0001f )
					ratio_str = "4:3";
				else if( (float)res_x/(float)res_y - 1.5f < 0.0001f )
					ratio_str = "3:2";
				else if( (float)res_x/(float)res_y - 1.6f < 0.0001f )
					ratio_str = "16:10"; // or  8:5
				else if( (float)res_x/(float)res_y - 1.666666f < 0.0001f )
					ratio_str = "5:3";
				else if( (float)res_x/(float)res_y - 1.777777f < 0.0001f )
					ratio_str = "16:9";
				else if( (float)res_x/(float)res_y - 1.8962962f < 0.0001f )
					ratio_str = "17:9";

				const char *type_str = "";
				// 5:4
				if     (res_x == 1280 && res_y == 1024)
					type_str = "SXGA";
				else if(res_x == 2560 && res_y == 2048)
					type_str = "QSXGA";
				// 4:3
				else if(res_x == 800 && res_y == 600)
					type_str = "SVGA";
				else if(res_x == 1024 && res_y == 768)
					type_str = "XGA";
				else if(res_x == 1400 && res_y == 1050)
					type_str = "SXGA+";
				else if(res_x == 1600 && res_y == 1200)
					type_str = "UXGA";
				else if(res_x == 2048 && res_y == 1536)
					type_str = "QXGA";
				// 3:2 has no names
				// 16:10
				else if(res_x == 1280 && res_y == 800)
					type_str = "WXGA";
				else if(res_x == 1680 && res_y == 1050)
					type_str = "WSXGA+";
				else if(res_x == 1920 && res_y == 1200)
					type_str = "WUXGA";
				else if(res_x == 2560 && res_y == 1600)
					type_str = "WQXGA";
				// 5:3
				else if(res_x == 800 && res_y == 480)
					type_str = "WVGA";
				else if(res_x == 1280 && res_y == 768)
					type_str = "WXGA";
				// 16:9
				else if(res_x == 854 && res_y == 480)
					type_str = "WVGA";
				else if(res_x == 1280 && res_y == 720)
					type_str = "HD 720";
				else if(res_x == 1920 && res_y == 1080)
					type_str = "HD 1080";
				// 17:9
				else if(res_x == 2048 && res_y == 1080)
					type_str = "2K";

				// now compose the final string
				char tmp[255];
				sprintf(tmp, "%d x %d", res_x, res_y);

				if(strlen(type_str) > 0)
					sprintf(tmp + strlen(tmp), ", %s", type_str);
				
				if(strlen(ratio_str) > 0)
					sprintf(tmp + strlen(tmp), ", %s", ratio_str);

				valStr = Ogre::String(tmp);
				valueCounter++;
				s = conv(valIt->c_str());
				wxString s2 = conv(valStr.c_str());

				// this is special ...
				renderer_choice[counter]->AppendValueItem(s, s2, res_d>0?res_x*res_y*res_d:res_x*res_y);

				continue;
			}

			valueCounter++;
			s = conv(valStr.c_str());
#ifdef _WIN32
			renderer_choice[counter]->AppendValueItem(s, _(s));
#else
			renderer_choice[counter]->AppendValueItem(s, wxGetTranslation(s));
#endif // _WIN32
		}
		renderer_choice[counter]->sort();
		renderer_choice[counter]->setSelectedValue(selection);
		renderer_choice[counter]->Show();
		if(optIt->second.immutable)
			renderer_choice[counter]->Disable();
		else
			renderer_choice[counter]->Enable();

		// layout stuff
		y += 25;
		/*
		if(y> 25 * 5)
		{
			// use next column
			y = 25;
			x += 230;
		}
		*/
		counter++;
	}
	// hide non-used controls
	if(counter<(int)renderer_text.size())
	{
		for(int i=counter;i<(int)renderer_text.size();i++)
		{
			renderer_text[i]->Hide();
			renderer_choice[i]->Hide();
		}
	}
}

void MyDialog::SetDefaults()
{
	wxScrollEvent dummye;
	//autodl->SetValue(false);
	//hydrax->SetValue(false);
	//rtshader->SetValue(false);
	//update textboxes
	//wxCheckBox *dust;
	OnScrollForceFeedback(dummye);
	advanced_logging->SetValue(false);
	arcadeControls->SetValue(true);
	beam_break_debug->SetValue(false);
	beam_deform_debug->SetValue(false);
	collisions->SetValue(false);
	dcm->SetValue(false);
	debug_envmap->SetValue(false);
	debug_triggers->SetValue(false);
	debug_vidcam->SetValue(false);
	dismap->SetValue(false);
	dof->SetValue(false);
	dofdebug->SetValue(false);
	dtm->SetValue(false);
	envmap->SetValue(true);
	extcam->SetValue(false);
	ffCamera->SetValue(0);
	ffCenter->SetValue(50);
	ffEnable->SetValue(true);
	ffHydro->SetValue(100);
	ffOverall->SetValue(100);
	flaresMode->SetSelection(2);         // all trucks
	fovext->SetValue(wxT("60"));
	fovint->SetValue(wxT("75"));
	fpsLimiter->SetValue(200);           // 200 = unlimited
	gearBoxMode->SetSelection(0);
	glow->SetValue(false);
	hdr->SetValue(false);
	heathaze->SetValue(false);
	ingame_console->SetValue(false);
	dev_mode->SetValue(false);
	inputgrab->SetSelection(0);          // All
	mblur->SetValue(false);
	mirror->SetValue(true);
	nocrashrpt->SetValue(false);
	particles->SetValue(true);
	posstor->SetValue(false);
	presel_map->SetValue(wxString());
	presel_truck->SetValue(wxString());
	replaymode->SetValue(false);
	screenShotFormat->SetSelection(0);
	selfcollisions->SetValue(false);
	shadow->SetSelection(0);             // no shadows
	shadowOptimizations->SetValue(true);
	sightRange->SetValue(5000);          // 5k = unlimited
	skidmarks->SetValue(true);
	sky->SetSelection(1);                // caelum
	sunburn->SetValue(false);
	textfilt->SetSelection(3);           // anisotropic
	thread->SetValue(false);
	vegetationMode->SetSelection(3);     // Full
	water->SetSelection(0);              // basic water
	waves->SetValue(false);              // no waves

#ifdef NETWORK
	network_enable->SetValue(false);
	nickname->SetValue(_("Anonymous"));
	servername->SetValue(_("127.0.0.1"));
	serverport->SetValue(_("12333"));
	serverpassword->SetValue(wxString());
	usertoken->SetValue(wxString());
#endif

#ifdef USE_OPENAL
	creaksound->SetValue(true);
	sound->SetSelection(1); // default
	soundVolume->SetValue(100);
#endif // USE_OPENAL

	// update settings
	getSettingsControls();
}

void MyDialog::getSettingsControls()
{
	//settings["AutoDownload"] = (autodl->GetValue()) ? "Yes" : "No";
	//settings["Hydrax"] = (hydrax->GetValue()) ? "Yes" : "No";
	//settings["Use RTShader System"] = (rtshader->GetValue()) ? "Yes" : "No";
	settings["Advanced Logging"] = (advanced_logging->GetValue()) ? "Yes" : "No";
	settings["ArcadeControls"] = (arcadeControls->GetValue()) ? "Yes" : "No";
	settings["Beam Break Debug"] = (beam_break_debug->GetValue()) ? "Yes" : "No";
	settings["Beam Deform Debug"] = (beam_deform_debug->GetValue()) ? "Yes" : "No";
	settings["Configurator Size"] = TOSTRING(this->GetSize().x) + ", " + TOSTRING(this->GetSize().y);
	settings["DOF"] = (dof->GetValue()) ? "Yes" : "No";
	settings["DOFDebug"] = (dofdebug->GetValue()) ? "Yes" : "No";
	settings["Debug Collisions"] = (dcm->GetValue()) ? "Yes" : "No";
	settings["Debug Truck Mass"] = (dtm->GetValue()) ? "Yes" : "No";
	settings["DisableCollisions"] = (collisions->GetValue()) ? "Yes" : "No";
	settings["DisableSelfCollisions"] = (selfcollisions->GetValue()) ? "Yes" : "No";
	settings["Enable Ingame Console"] = (ingame_console->GetValue()) ? "Yes" : "No";
	settings["DevMode"] = (dev_mode->GetValue()) ? "Yes" : "No";
	settings["EnvMapDebug"] = (debug_envmap->GetValue()) ? "Yes" : "No";
	settings["Envmap"] = (envmap->GetValue()) ? "Yes" : "No";
	settings["External Camera Mode"] = (extcam->GetValue()) ? "Static" : "Pitching";
	settings["FOV External"] = conv(fovext->GetValue());
	settings["FOV Internal"] = conv(fovint->GetValue());
	settings["FPS-Limiter"] = TOSTRING(fpsLimiter->GetValue());
	settings["Force Feedback Camera"] = TOSTRING(ffCamera->GetValue());
	settings["Force Feedback Centering"] = TOSTRING(ffCenter->GetValue());
	settings["Force Feedback Gain"] = TOSTRING(ffOverall->GetValue());
	settings["Force Feedback Stress"] = TOSTRING(ffHydro->GetValue());
	settings["Force Feedback"] = (ffEnable->GetValue()) ? "Yes" : "No";
	settings["GearboxMode"]= gearBoxMode->getSelectedValueAsSTDString();
	settings["Glow"] = "No"; //(glow->GetValue()) ? "Yes" : "No";
	settings["HDR"] = (hdr->GetValue()) ? "Yes" : "No";
	settings["HeatHaze"] = "No"; //(heathaze->GetValue()) ? "Yes" : "No";
	settings["Input Grab"] = inputgrab->getSelectedValueAsSTDString();
	settings["Lights"] = flaresMode->getSelectedValueAsSTDString();
	settings["Mirrors"] = (mirror->GetValue()) ? "Yes" : "No";
	settings["Motion blur"] = (mblur->GetValue()) ? "Yes" : "No";
	settings["Multi-threading"] = (thread->GetValue()) ? "No" : "Yes";
	settings["NoCrashRpt"] = (nocrashrpt->GetValue()) ? "Yes" : "No";
	settings["Particles"] = (particles->GetValue()) ? "Yes" : "No";
	settings["Position Storage"] = (posstor->GetValue()) ? "Yes" : "No";
	settings["Preselected Map"] = conv(presel_map->GetValue());
	settings["Preselected Truck"] = conv(presel_truck->GetValue());
	settings["Replay mode"] = (replaymode->GetValue()) ? "Yes" : "No";
	settings["Screenshot Format"] = screenShotFormat->getSelectedValueAsSTDString();
	settings["Shadow optimizations"] = (shadowOptimizations->GetValue()) ? "Yes" : "No";
	settings["Shadow technique"] = shadow->getSelectedValueAsSTDString();
	settings["SightRange"] = TOSTRING(sightRange->GetValue());
	settings["Skidmarks"] = (skidmarks->GetValue()) ? "Yes" : "No";
	settings["Sky effects"] = sky->getSelectedValueAsSTDString();
	settings["Sunburn"] = "No"; //(sunburn->GetValue()) ? "Yes" : "No";
	settings["Texture Filtering"] = textfilt->getSelectedValueAsSTDString();
	settings["Trigger Debug"] = (debug_triggers->GetValue()) ? "Yes" : "No";
	settings["Vegetation"] = vegetationMode->getSelectedValueAsSTDString();
	settings["VideoCameraDebug"] = (debug_vidcam->GetValue()) ? "Yes" : "No";
	settings["Water effects"] = water->getSelectedValueAsSTDString();
	settings["Waves"] = (waves->GetValue()) ? "Yes" : "No";
	settings["disableOverViewMap"] = (dismap->GetValue()) ? "Yes" : "No";

#ifdef NETWORK
	settings["Network enable"] = (network_enable->GetValue()) ? "Yes" : "No";
	settings["Nickname"] = conv(nickname->GetValue().SubString(0,20));
	settings["Server name"] = conv(servername->GetValue());
	settings["Server password"] = conv(serverpassword->GetValue());
	settings["Server port"] = conv(serverport->GetValue());
	settings["User Token"] = conv(usertoken->GetValue());
#endif

#ifdef USE_OPENAL
	settings["AudioDevice"] = sound->getSelectedValueAsSTDString();
	settings["Creak Sound"] = (creaksound->GetValue()) ? "No" : "Yes";
	settings["Sound Volume"] = TOSTRING(soundVolume->GetValue());
#endif //USE_OPENAL

	// save language, if one is set
	if(avLanguages.size() > 0 && languageMode->GetSelection() > 0)
	{
		settings["Language"] = languageMode->getSelectedValueAsSTDString();

		std::vector<wxLanguageInfo*>::iterator it;
		for(it = avLanguages.begin(); it!=avLanguages.end(); it++)
		{
			if((*it)->Description == languageMode->getSelectedValue())
			{
				settings["Language Short"] = conv((*it)->CanonicalName);
			}
		}
	} else
	{
		settings["Language"] = std::string("English (U.S.)");
		settings["Language Short"] = std::string("en_US");
	}
}

void MyDialog::updateSettingsControls()
{
	// this method "applies" the settings and updates the controls

	Ogre::String st;
	textfilt->setSelectedValue(settings["Texture Filtering"]);
	inputgrab->setSelectedValue(settings["Input Grab"]);
	sky->setSelectedValue(settings["Sky effects"]);
	shadow->setSelectedValue(settings["Shadow technique"]);
	water->setSelectedValue(settings["Water effects"]);
	gearBoxMode->setSelectedValue(settings["GearboxMode"]);

	double sightrange = 10;
	st = settings["SightRange"];
	if (st.length()>0)
	{
		if(conv(st).ToDouble(&sightrange))
			sightRange->SetValue(sightrange);
	}

	long fpslimit = 200;
	st = settings["FPS-Limiter"];
	if (st.length()>0)
	{
		if(conv(st).ToLong(&fpslimit))
			fpsLimiter->SetValue(fpslimit);
	}

#ifdef USE_OPENAL
	st = settings["Sound Volume"];
	long volume = 100;
	if (st.length()>0)
	{
		if(conv(st).ToLong(&volume))
			soundVolume->SetValue(volume);
	}
	st = settings["Creak Sound"]; if (st.length()>0) creaksound->SetValue(st=="No");
	sound->setSelectedValue(settings["AudioDevice"]);
#endif //USE_OPENAL

	//st = settings["AutoDownload"]; if (st.length()>0) autodl->SetValue(st=="Yes");
	//st = settings["Glow"]; if (st.length()>0) glow->SetValue(st=="Yes");
	//st = settings["Hydrax"]; if (st.length()>0) hydrax->SetValue(st=="Yes");
	st = settings["Skidmarks"]; if (st.length()>0) skidmarks->SetValue(st=="Yes");
	//st = settings["Sunburn"]; if (st.length()>0) sunburn->SetValue(st=="Yes");
	//st = settings["Use RTShader System"]; if (st.length()>0) rtshader->SetValue(st=="Yes");
	st = settings["Advanced Logging"]; if (st.length()>0) advanced_logging->SetValue(st=="Yes");
	st = settings["ArcadeControls"]; if (st.length()>0) arcadeControls->SetValue(st=="Yes");
	st = settings["Beam Break Debug"]; if (st.length()>0) beam_break_debug->SetValue(st=="Yes");
	st = settings["Beam Deform Debug"]; if (st.length()>0) beam_deform_debug->SetValue(st=="Yes");
	st = settings["DOF"]; if (st.length()>0) dof->SetValue(st=="Yes");
	st = settings["DOFDebug"]; if (st.length()>0) dofdebug->SetValue(st=="Yes");
	st = settings["Debug Collisions"]; if (st.length()>0) dcm->SetValue(st=="Yes");
	st = settings["Debug Truck Mass"]; if (st.length()>0) dtm->SetValue(st=="Yes");
	st = settings["DisableCollisions"]; if (st.length()>0) collisions->SetValue(st=="Yes");
	st = settings["DisableSelfCollisions"]; if (st.length()>0) selfcollisions->SetValue(st=="Yes");
	st = settings["Enable Ingame Console"]; if (st.length()>0) ingame_console->SetValue(st=="Yes");
	st = settings["DevMode"]; if (st.length()>0) dev_mode->SetValue(st == "Yes");
	st = settings["EnvMapDebug"]; if (st.length()>0) debug_envmap->SetValue(st=="Yes");
	st = settings["Envmap"]; if (st.length()>0) envmap->SetValue(st=="Yes");
	st = settings["External Camera Mode"]; if (st.length()>0) extcam->SetValue(st=="Static");
	st = settings["HDR"]; if (st.length()>0) hdr->SetValue(st=="Yes");
	st = settings["HeatHaze"]; if (st.length()>0) heathaze->SetValue(st=="Yes");
	st = settings["Mirrors"]; if (st.length()>0) mirror->SetValue(st=="Yes");
	st = settings["Motion blur"]; if (st.length()>0) mblur->SetValue(st=="Yes");
	st = settings["Multi-threading"]; if (st.length()>0) thread->SetValue(st=="No");
	st = settings["NoCrashRpt"]; if (st.length()>0) nocrashrpt->SetValue(st=="Yes");
	st = settings["Particles"]; if (st.length()>0) particles->SetValue(st=="Yes");
	st = settings["Position Storage"]; if (st.length()>0) posstor->SetValue(st=="Yes");
	st = settings["Replay mode"]; if (st.length()>0) replaymode->SetValue(st=="Yes");
	st = settings["Shadow optimizations"]; if (st.length()>0) shadowOptimizations->SetValue(st=="Yes");
	st = settings["Trigger Debug"]; if (st.length()>0) debug_triggers->SetValue(st=="Yes");
	st = settings["VideoCameraDebug"]; if (st.length()>0) debug_vidcam->SetValue(st=="Yes");
	st = settings["Waves"]; if (st.length()>0) waves->SetValue(st=="Yes");
	st = settings["disableOverViewMap"]; if (st.length()>0) dismap->SetValue(st=="Yes");

	double flt = 0;
	st = settings["Force Feedback"]; if (st.length()>0) ffEnable->SetValue(st=="Yes");
	st = settings["Force Feedback Gain"]; if (st.length()>0 && conv(st).ToDouble(&flt)) ffOverall->SetValue(flt);
	st = settings["Force Feedback Stress"]; if (st.length()>0 && conv(st).ToDouble(&flt)) ffHydro->SetValue(flt);
	st = settings["Force Feedback Centering"]; if (st.length()>0 && conv(st).ToDouble(&flt)) ffCenter->SetValue(flt);
	st = settings["Force Feedback Camera"]; if (st.length()>0 && conv(st).ToDouble(&flt)) ffCamera->SetValue(flt);

	st = settings["FOV Internal"]; if (st.length()>0) fovint->SetValue(conv(st));
	st = settings["FOV External"]; if (st.length()>0) fovext->SetValue(conv(st));

	st = settings["Preselected Map"]; if (st.length()>0) presel_map->SetValue(conv(st));
	st = settings["Preselected Truck"]; if (st.length()>0) presel_truck->SetValue(conv(st));

	// update textboxes
	wxScrollEvent dummye;
	OnScrollForceFeedback(dummye);
	
	flaresMode->setSelectedValue(settings["Lights"]);
	vegetationMode->setSelectedValue(settings["Vegetation"]);
	screenShotFormat->setSelectedValue(settings["Screenshot Format"]);

	st = settings["Configurator Size"];
	if (st.length()>0)
	{
		int sx=0, sy=0;
		int res = sscanf(st.c_str(), "%d, %d", &sx, &sy);
		if(res == 2)
		{
			SetSize(wxSize(sx, sy));
		}
	}

#ifdef NETWORK
	st = settings["Network enable"]; if (st.length()>0) network_enable->SetValue(st=="Yes");
	st = settings["Nickname"]; if (st.length()>0) nickname->SetValue(conv(st));
	st = settings["Server name"]; if (st.length()>0) servername->SetValue(conv(st));
	st = settings["Server port"]; if (st.length()>0) serverport->SetValue(conv(st));
	st = settings["Server password"]; if (st.length()>0) serverpassword->SetValue(conv(st));
	st = settings["User Token"]; if (st.length()>0) usertoken->SetValue(conv(st));
#endif
	// update slider text
	OnScrollSightRange(dummye);
	OnScrollVolume(dummye);
	OnScrollFPSLimiter(dummye);
}

bool MyDialog::LoadConfig()
{
	//RoR config
	Ogre::ImprovedConfigFile cfg;
	try
	{
		wxLogStatus(wxT("Loading RoR.cfg"));

		wxString rorcfg=app->UserPath + wxFileName::GetPathSeparator() + wxT("config") + wxFileName::GetPathSeparator() + wxT("RoR.cfg");
		wxLogStatus(wxT("reading from Config file: ") + rorcfg);

		// Don't trim whitespace
		cfg.load((const char *)rorcfg.mb_str(wxConvUTF8), "=:\t", false);
	} catch(...)
	{
		wxLogError(wxT("error loading RoR.cfg"));
		return false;
	}

	// load all settings into a map!
	Ogre::ConfigFile::SettingsIterator i = cfg.getSettingsIterator();
	Ogre::String svalue, sname;
	while (i.hasMoreElements())
	{
		sname = i.peekNextKey();
		Ogre::StringUtil::trim(sname);
		svalue = i.getNext();
		Ogre::StringUtil::trim(svalue);
		// filter out some things that shouldnt be in there (since we cannot use RoR normally anymore after those)
		if(sname == Ogre::String("Benchmark") || sname == Ogre::String("streamCacheGenerationOnly")|| sname == Ogre::String("regen-cache-only"))
			continue;
		settings[sname] = svalue;
	}


	// enforce default settings
	if(settings["Allow NVPerfHUD"] == "") settings["Allow NVPerfHUD"] = "No";
	if(settings["Floating-point mode"] == "") settings["Floating-point mode"] = "Fastest";
	if(settings["Colour Depth"] == "") settings["Colour Depth"] = "32";

	// then update the controls!
	updateSettingsControls();
	return false;
}

void MyDialog::SaveConfig()
{
	// first, update the settings map with the actual control values
	getSettingsControls();

	// then set stuff and write configs
	//save Ogre stuff if we loaded ogre in the first place
	try
	{
		if(ogreRoot)
		{
			Ogre::RenderSystem *rs = ogreRoot->getRenderSystem();
			for(int i=0;i<(int)renderer_text.size();i++)
			{
				if(!renderer_text[i]->IsShown())
					break;
				std::string key = renderer_name[i];
				std::string val = renderer_choice[i]->getSelectedValueAsSTDString();
				try
				{
					rs->setConfigOption(key, val);
				} catch(...)
				{
					wxMessageDialog(this, _("Error setting Ogre Values"), _("Ogre config validation error, unknown render option detected: ") + conv(key), wxOK||wxICON_ERROR).ShowModal();
				}
			}
			Ogre::String err = rs->validateConfigOptions();
			if (err.length() > 0)
			{
				wxMessageDialog(this, conv(err), _("Ogre config validation error"),wxOK||wxICON_ERROR).ShowModal();
			} else
			{
				Ogre::Root::getSingleton().saveConfig();
			}
		}
	}
	catch (std::exception &e)
	{
		wxLogError(wxT("Cought exception:"));
		wxLogError(wxString(e.what(), wxConvUTF8));
		wxString warning = wxString(e.what(), wxConvUTF8);
		wxString caption = _("caught exception");
		wxMessageDialog *w = new wxMessageDialog(NULL, warning, caption, wxOK|wxICON_ERROR|wxSTAY_ON_TOP, wxDefaultPosition);
		w->ShowModal();
		delete(w);
	}

	//save my stuff
	FILE *fd;
	wxString rorcfg=app->UserPath + wxFileName::GetPathSeparator() + wxT("config") + wxFileName::GetPathSeparator() + wxT("RoR.cfg");

	wxLogStatus(wxT("saving to Config file: ") + rorcfg);
	fd=fopen((const char *)rorcfg.mb_str(wxConvUTF8), "w");
	if (!fd)
	{
		wxMessageDialog(this, _("Could not write config file"), _("Configure error"),wxOK||wxICON_ERROR).ShowModal();
		return;
	}

	// now save the settings to RoR.cfg
	std::map<std::string, std::string>::iterator it;
	for(it=settings.begin(); it!=settings.end(); it++)
	{
		fprintf(fd, "%s=%s\n", it->first.c_str(), it->second.c_str());
	}

	fclose(fd);
}

void MyDialog::OnTimer(wxTimerEvent& event)
{
}

void MyDialog::OnQuit(wxCloseEvent& WXUNUSED(event))
{
	Destroy();
	exit(0);
}

void MyDialog::OnChoiceRenderer(wxCommandEvent& ev)
{
	if(!ogreRoot) return;
	try
	{
		Ogre::RenderSystem *rs = ogreRoot->getRenderSystemByName(renderer->getSelectedValueAsSTDString());
		if(rs)
		{
			ogreRoot->setRenderSystem(rs);
			updateRendersystems(rs);
		} else
		{
			wxLogStatus(wxT("Unable to change to new rendersystem(1)"));
		}
	}
	catch(...)
	{
		wxLogStatus(wxT("Unable to change to new rendersystem(2)"));
	}
}

void MyDialog::OnButCancel(wxCommandEvent& event)
{
	Destroy();
	exit(0);
}

void MyDialog::OnButPlay(wxCommandEvent& event)
{
	SaveConfig();

#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
	PROCESS_INFORMATION pi;
	STARTUPINFO si;
	DWORD               dwCode  =   0;
	ZeroMemory(&si,sizeof(STARTUPINFO));
	si.cb           =   sizeof(STARTUPINFO);
	si.dwFlags      =   STARTF_USESHOWWINDOW;
	si.wShowWindow  =   SW_SHOWNORMAL;

	char path[2048];
	if(getcwd(path, 2048) != path)
	{
		// error
		return;
	}
	
	strcat(path, "\\RoR.exe");
	wxLogStatus(wxT("using RoR: ") + wxString(path));

	int buffSize = (int)strlen(path) + 1;
	LPWSTR wpath = new wchar_t[buffSize];
	MultiByteToWideChar(CP_ACP, 0, path, buffSize, wpath, buffSize);

	if(!CreateProcess(NULL, wpath, NULL, NULL, false, NORMAL_PRIORITY_CLASS, NULL, NULL, &si, &pi))
	{
		// error starting
		return;
	}
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);
#endif

#if OGRE_PLATFORM == OGRE_PLATFORM_LINUX
	char tmp[4096] = "";
	strcpy(tmp, SSETTING("Program Path", "").c_str());
	strcat(tmp, "RoR");
	execl(tmp, "", (char *) 0);
#endif
#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE
	FSRef ref;
	FSPathMakeRef((const UInt8*)"./RoR.app", &ref, NULL);
	LSOpenFSRef(&ref, NULL);
	//execl("./RoR.app/Contents/MacOS/RoR", "", (char *) 0);
#endif
	Destroy();
	exit(0);
}

void MyDialog::OnButSave(wxCommandEvent& event)
{
	SaveConfig();
	Destroy();
	exit(0);
}

void MyDialog::OnButRestore(wxCommandEvent& event)
{
	SetDefaults();
}

void MyDialog::updateRoR()
{
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32

	// get paths to update.exe
	char path[2048];
	if(getcwd(path, 2048) != path)
	{
		//error
		return;
	}
	strcat(path, "\\updater.exe");
	wxLogStatus(wxT("using updater: ") + wxString(path));

	int buffSize = (int)strlen(path) + 1;
	LPWSTR wpath = new wchar_t[buffSize];
	MultiByteToWideChar(CP_ACP, 0, path, buffSize, wpath, buffSize);

	if(getcwd(path, 2048) != path)
	{
		//error
		return;
	}
	buffSize = (int)strlen(path) + 1;
	LPWSTR cwpath = new wchar_t[buffSize];
	MultiByteToWideChar(CP_ACP, 0, path, buffSize, cwpath, buffSize);

	// now construct struct that has the required starting info
    SHELLEXECUTEINFO sei = { sizeof(sei) };
	sei.cbSize = sizeof(SHELLEXECUTEINFOA);
	sei.fMask = 0;
	sei.hwnd = NULL;
	sei.lpVerb = wxString("runas");
	sei.lpFile = wpath;
	sei.lpParameters = NULL;//wxT("--update");
	sei.lpDirectory = cwpath;
	sei.nShow = SW_NORMAL;

	ShellExecuteEx(&sei);

	// we want to exit to be able to update the configurator!
	exit(0);
#else
	wxMessageDialog *w = new wxMessageDialog(this, _("Warning"), _("The update service is currently only available for windows users."), wxOK, wxDefaultPosition);
	w->ShowModal();
	delete(w);
#endif
}

void MyDialog::OnButUpdateRoR(wxCommandEvent& event)
{
	updateRoR();
}

#ifdef USE_OPENCL
#include <oclUtils.h>

#include "ocl_bwtest.h"
#endif // USE_OPENCL

#ifdef USE_OPENCL
// late loading DLLs
// see http://msdn.microsoft.com/en-us/library/8yfshtha.aspx
int tryLoadOpenCL2()
{
#ifdef _WIN32
	bool failed = false;
	__try
	{
		failed = FAILED(__HrLoadAllImportsForDll("OpenCL.dll"));
	} __except (EXCEPTION_EXECUTE_HANDLER)
	{
		failed = true;
	}

	if(failed)
		return -1;
	return 1;
#else // _WIN32
	// TODO: implement late loading under different OSs
#endif // _WIN32
}

void MyDialog::tryLoadOpenCL()
{
	if(openCLAvailable != 0) return;

	openCLAvailable = tryLoadOpenCL2();
	if(openCLAvailable != 1)
	{
		wxMessageDialog *msg=new wxMessageDialog(this, "OpenCL.dll not found\nAre the Display drivers up to date?", "OpenCL.dll not found", wxOK | wxICON_ERROR );
		msg->ShowModal();

		gputext->SetValue("failed to load OpenCL.dll");
	}
}
#endif // USE_OPENCL

void MyDialog::OnButCheckOpenCLBW(wxCommandEvent& event)
{
#ifdef USE_OPENCL
	tryLoadOpenCL();
	if(openCLAvailable != 1) return;
	gputext->SetValue("");
	ostream tstream(gputext);
	OpenCLTestBandwidth bw_test(tstream);
#endif // USE_OPENCL
}

void MyDialog::OnButCheckOpenCL(wxCommandEvent& event)
{
#ifdef USE_OPENCL
	tryLoadOpenCL();
	if(openCLAvailable != 1) return;
	gputext->SetValue("");
	ostream tstream(gputext);
    bool bPassed = true;

    // Get OpenCL platform ID for NVIDIA if avaiable, otherwise default
    tstream << "OpenCL Software Information:" << endl;
    char cBuffer[1024];
    cl_platform_id clSelectedPlatformID = NULL;
    cl_int ciErrNum = oclGetPlatformID (&clSelectedPlatformID);
    if(ciErrNum != CL_SUCCESS)
	{
		tstream << "error getting platform info" << endl;
		bPassed = false;
	}

	if(bPassed)
	{
		// Get OpenCL platform name and version
		ciErrNum = clGetPlatformInfo (clSelectedPlatformID, CL_PLATFORM_NAME, sizeof(cBuffer), cBuffer, NULL);
		if (ciErrNum == CL_SUCCESS)
		{
			tstream << "Platform Name: " << cBuffer << endl;
		}
		else
		{
			tstream << "Platform Name: ERROR " << ciErrNum << endl;
			bPassed = false;
		}

		ciErrNum = clGetPlatformInfo (clSelectedPlatformID, CL_PLATFORM_VERSION, sizeof(cBuffer), cBuffer, NULL);
		if (ciErrNum == CL_SUCCESS)
		{
			tstream << "Platform Version: " << cBuffer << endl;
		}
		else
		{
			tstream << "Platform Version: ERROR " << ciErrNum << endl;
			bPassed = false;
		}
		tstream.flush();

		// Log OpenCL SDK Revision #
		tstream << "OpenCL SDK Revision: " << OCL_SDKREVISION << endl;

		// Get and log OpenCL device info
		cl_uint ciDeviceCount;
		cl_device_id *devices;
		ciErrNum = clGetDeviceIDs (clSelectedPlatformID, CL_DEVICE_TYPE_ALL, 0, NULL, &ciDeviceCount);

		tstream << endl;

		tstream << "OpenCL Hardware Information:" << endl;
		tstream << "Devices found: " << ciDeviceCount << endl;

		// check for 0 devices found or errors...
		if (ciDeviceCount == 0)
		{
			tstream << "No devices found supporting OpenCL (return code " << ciErrNum << ")" << endl;
			bPassed = false;
		}
		else if (ciErrNum != CL_SUCCESS)
		{
			tstream << "Error in clGetDeviceIDs call: " << ciErrNum << endl;
			bPassed = false;
		}
		else
		{
			if ((devices = (cl_device_id*)malloc(sizeof(cl_device_id) * ciDeviceCount)) == NULL)
			{
				tstream << "ERROR: Failed to allocate memory for devices" << endl;
				bPassed = false;
			}
			ciErrNum = clGetDeviceIDs (clSelectedPlatformID, CL_DEVICE_TYPE_ALL, ciDeviceCount, devices, &ciDeviceCount);
			if (ciErrNum == CL_SUCCESS)
			{
				//Create a context for the devices
				cl_context cxGPUContext = clCreateContext(0, ciDeviceCount, devices, NULL, NULL, &ciErrNum);
				if (ciErrNum != CL_SUCCESS)
				{
					tstream << "ERROR in clCreateContext call: " << ciErrNum << endl;
					bPassed = false;
				}
				else
				{
					// show info for each device in the context
					for(unsigned int i = 0; i < ciDeviceCount; ++i )
					{
						clGetDeviceInfo(devices[i], CL_DEVICE_NAME, sizeof(cBuffer), &cBuffer, NULL);
						tstream << (i + 1) << " : Device " << cBuffer << endl;
						//oclPrintDevInfo(LOGBOTH, devices[i]);
					}
				}
			}
			else
			{
				tstream << "ERROR in clGetDeviceIDs call: " << ciErrNum << endl;
				bPassed = false;
			}
		}
	}
    // finish
	if(bPassed)
		tstream << "=== PASSED, OpenCL working ===" << endl;
	else
		tstream << "=== FAILED, OpenCL NOT working ===" << endl;
#endif // USE_OPENCL
}

void MyDialog::OnButRegenCache(wxCommandEvent& event)
{
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
	PROCESS_INFORMATION pi;
	STARTUPINFO si;
	DWORD               dwCode  =   0;
	ZeroMemory(&si,sizeof(STARTUPINFO));
	si.cb           =   sizeof(STARTUPINFO);
	si.dwFlags      =   STARTF_USESHOWWINDOW;
	si.wShowWindow  =   SW_SHOWNORMAL;

	char path[2048];
	if(getcwd(path, 2048) != path)
		return;
	strcat(path, "\\RoR.exe -checkcache");
	wxLogStatus(wxT("executing RoR: ") + wxString(path));

	int buffSize = (int)strlen(path) + 1;
	LPWSTR wpath = new wchar_t[buffSize];
	MultiByteToWideChar(CP_ACP, 0, path, buffSize, wpath, buffSize);

	if(!CreateProcess(NULL, wpath, NULL, NULL, false, NORMAL_PRIORITY_CLASS, NULL, NULL, &si, &pi))
	      return;
	
	// wait until child process exits
	WaitForSingleObject( pi.hProcess, INFINITE );
	CloseHandle( pi.hProcess );
	CloseHandle( pi.hThread );	
#endif
#if OGRE_PLATFORM == OGRE_PLATFORM_LINUX
	char tmp[4096] = "";
	strcpy(tmp, SSETTING("Program Path", "").c_str());
	strcat(tmp, "RoR -checkcache");
	execl(tmp, "", (char *) 0);
#endif
#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE
	FSRef ref;
	FSPathMakeRef((const UInt8*)"./RoR.app -checkcache", &ref, NULL);
	LSOpenFSRef(&ref, NULL);
	//execl("./RoR.app/Contents/MacOS/RoR", "", (char *) 0);
#endif
}

void MyDialog::OnButClearCache(wxCommandEvent& event)
{
	wxFileName cfn;
	cfn.AssignCwd();

	wxString cachepath=app->UserPath+wxFileName::GetPathSeparator()+wxT("cache");
	wxDir srcd(cachepath);
	wxString src;
	if (!srcd.GetFirst(&src))
	{
		wxMessageBox(_("Cache is already empty"), wxT("RoR: Cache is already empty"), wxICON_INFORMATION);
		return; //empty dir!
	}
	do
	{
		//ignore files and directories beginning with "." (important, for SVN!)
		if (src.StartsWith(wxT("."))) continue;
		//check if it id a directory
		wxFileName tsfn=wxFileName(cachepath, wxEmptyString);
		tsfn.AppendDir(src);
		if (wxDir::Exists(tsfn.GetPath()))
		{
			//this is a directory, leave it alone
			continue;
		}
		else
		{
			//this is a file, delete file
			tsfn=wxFileName(cachepath, src);
			::wxRemoveFile(tsfn.GetFullPath());
		}
	} while (srcd.GetNext(&src));
	wxMessageBox(_("Cache cleared"), wxT("RoR: Cache cleared"), wxICON_INFORMATION);
}

void MyDialog::OnScrollSightRange(wxScrollEvent &e)
{
	wxString s;
	int v = sightRange->GetValue();
	if (v == sightRange->GetMax())
	{
		s = _("Unlimited");
	} else
	{
		s.Printf(wxT("%i m"), v);
	}
	sightRangeText->SetLabel(s);
}

void MyDialog::OnScrollVolume(wxScrollEvent &e)
{
#ifdef USE_OPENAL
	wxString s;
	int v = soundVolume->GetValue();
	if (v == soundVolume->GetMin())
	{
		s = _("Muted");
	} else
	{
		s.Printf(wxT("%i %%"), v);
	}
	soundVolumeText->SetLabel(s);
#endif //USE_OPENAL
}

void MyDialog::OnScrollForceFeedback(wxScrollEvent & event)
{
	wxString s;
	int val=ffOverall->GetValue();
	s.Printf(wxT("%i%%"), val);
	ffOverallText->SetLabel(s);

	val=ffHydro->GetValue();
	s.Printf(wxT("%i%%"), val);
	ffHydroText->SetLabel(s);

	val=ffCenter->GetValue();
	s.Printf(wxT("%i%%"), val);
	ffCenterText->SetLabel(s);

	val=ffCamera->GetValue();
	s.Printf(wxT("%i%%"), val);
	ffCameraText->SetLabel(s);
}

void MyDialog::OnScrollFPSLimiter(wxScrollEvent & event)
{
	wxString s;
	int v = fpsLimiter->GetValue();
	if (v == fpsLimiter->GetMax())
	{
		s = _("Unlimited");
	} else
	{
		s.Printf(wxT("%i fps"), v);
	}
	fpsLimiterText->SetLabel(s);
}

#if wxCHECK_VERSION(2, 8, 0)
void MyDialog::OnClickedHtmlLinkMain(wxHtmlLinkEvent& event)
{
	wxHtmlLinkInfo linkinfo=event.GetLinkInfo();
	wxString href=linkinfo.GetHref();
	wxURI *uri=new wxURI(href);
	if (uri->GetScheme()==conv("rorserver"))
	{
		network_enable->SetValue(true);
		servername->SetValue(uri->GetServer());
		serverport->SetValue(uri->GetPort());
		//serverport->SetValue(uri->GetPassword());
		//serverpassword->SetValue(uri->GetPath().AfterFirst('/'));
		if(uri->GetPassword() != wxString())
		{
			serverpassword->Enable(true);
		}else
			serverpassword->Enable(false);
	} else if (uri->GetScheme()==conv("rorinstaller"))
	{
		if(uri->GetServer() == wxT("update"))
		{
			updateRoR();
		}
	} else
	{
		networkhtmw->OnLinkClicked(linkinfo);
	}
//	wxMessageDialog *res=new wxMessageDialog(this, href, "Success", wxOK | wxICON_INFORMATION );
//	res->ShowModal();
}

void MyDialog::OnClickedHtmlLinkUpdate(wxHtmlLinkEvent& event)
{
	wxHtmlLinkInfo linkinfo=event.GetLinkInfo();
	wxString href=linkinfo.GetHref();
	wxURI *uri=new wxURI(href);
	if (uri->GetScheme()==conv("rorinstaller"))
	{
		if(uri->GetServer() == wxT("update"))
		{
			updateRoR();
		}
	} else
	{
		helphtmw->OnLinkClicked(linkinfo);
	}
}
#endif // version 2.8

void MyDialog::OnChangedNotebook2(wxNotebookEvent& event)
{
	// settings notebook page change
	if(event.GetSelection() == 0)
	{
		// render settings, load ogre!
		//loadOgre();
	}
}

#ifdef _WIN32
// code borrowed from updater
void wxStringToTCHAR(TCHAR *tCharString, wxString &myString)
{
	const wxChar* myStringChars = myString.c_str();
	for (unsigned int i = 0; i < myString.Len(); i++) {
		tCharString[i] = myStringChars [i];
	}
	tCharString[myString.Len()] = wxChar('\0');
}

std::string MyDialog::readVersionInfo()
{
	// http://stackoverflow.com/questions/940707/how-do-i-programatically-get-the-version-of-a-dll-or-exe

	//wxString rorpath = getInstallationPath() + wxT("RoR.exe");
	wxString rorpath = wxT("RoR.exe");

	char buffer[4096]="";
	TCHAR pszFilePath[MAX_PATH];

	wxStringToTCHAR(pszFilePath, rorpath);

	DWORD               dwSize              = 0;
	BYTE                *pbVersionInfo      = NULL;
	VS_FIXEDFILEINFO    *pFileInfo          = NULL;
	UINT                puLenFileInfo       = 0;

	// get the version info for the file requested
	dwSize = GetFileVersionInfoSize( pszFilePath, NULL );
	if ( dwSize == 0 )
	{
		//wxMessageBox(wxString::Format("Error in GetFileVersionInfoSize: %d\n", GetLastError()),wxT("GetFileVersionInfoSize Error"),0);
		return "unknown";
	}

	pbVersionInfo = new BYTE[ dwSize ];

	if ( !GetFileVersionInfo( pszFilePath, 0, dwSize, pbVersionInfo ) )
	{
		//printf( "Error in GetFileVersionInfo: %d\n", GetLastError() );
		//delete[] pbVersionInfo;
		return "unknown";
	}

	if ( !VerQueryValue( pbVersionInfo, TEXT("\\"), (LPVOID*) &pFileInfo, &puLenFileInfo ) )
	{
		//printf( "Error in VerQueryValue: %d\n", GetLastError() );
		//delete[] pbVersionInfo;
		return "unknown";
	}

	if (pFileInfo->dwSignature == 0xfeef04bd)
	{
		int major = HIWORD(pFileInfo->dwFileVersionMS);
		int minor = LOWORD(pFileInfo->dwFileVersionMS);
		int patch = HIWORD(pFileInfo->dwFileVersionLS);
		int rev   = LOWORD(pFileInfo->dwFileVersionLS);
		sprintf(buffer, "%d.%d.%d", major, minor, patch);
		return std::string(buffer);
	}
	return "unknown";
}
#endif // _WIN32

void MyDialog::OnChangedNotebook1(wxNotebookEvent& event)
{
	wxString tabname = nbook->GetPageText(event.GetSelection());
	if(tabname == _("Updates"))
	{
		// try to find our version
#ifdef _WIN32
		helphtmw->LoadPage(wxString(conv(NEWS_HTML_PAGE))+
			wxString(conv("?netversion="))+wxString(conv(RORNET_VERSION))+
			wxString(conv("&version="))+wxString(readVersionInfo())+
			wxString(conv("&lang="))+conv(conv(language->CanonicalName))
			);
#else
		helphtmw->LoadPage(wxString(conv(NEWS_HTML_PAGE))+
			wxString(conv("?netversion="))+wxString(conv(RORNET_VERSION))+
			wxString(conv("&lang="))+conv(conv(language->CanonicalName))
			);
#endif // _WIN32
	}
}

void MyDialog::OnTimerReset(wxTimerEvent& event)
{
	btnUpdate->Enable(true);
}

void MyDialog::OnButGetUserToken(wxCommandEvent& event)
{
	wxLaunchDefaultBrowser(wxT("http://usertoken.rigsofrods.com"));
}

void MyDialog::OnButTestNet(wxCommandEvent& event)
{
#ifdef NETWORK
	btnUpdate->Enable(false);
	timer1->Start(10000);
	std::string lshort = conv(language->CanonicalName).substr(0, 2);
	networkhtmw->LoadPage(wxString(conv(REPO_HTML_SERVERLIST))+
						  wxString(conv("?version="))+wxString(conv(RORNET_VERSION))+
						  wxString(conv("&lang="))+conv(lshort));

//	networkhtmw->LoadPage("http://rigsofrods.blogspot.com/");
	/*
	SWInetSocket mySocket;
	SWBaseSocket::SWBaseError error;
	long port;
	serverport->GetValue().ToLong(&port);
	mySocket.connect(port, servername->GetValue().c_str(), &error);
	if (error!=SWBaseSocket::ok)
	{
		wxMessageDialog *err=new wxMessageDialog(this, wxString(error.get_error()), "Error", wxOK | wxICON_ERROR);
		err->ShowModal();
		return;
	}
	header_t head;
	head.command=MSG_VERSION;
	head.size=strlen(RORNET_VERSION);
	mySocket.send((char*)&head, sizeof(header_t), &error);
	if (error!=SWBaseSocket::ok)
	{
		wxMessageDialog *err=new wxMessageDialog(this, wxString(error.get_error()), "Error", wxOK | wxICON_ERROR);
		err->ShowModal();
		return;
	}
	mySocket.send(RORNET_VERSION, head.size, &error);
	if (error!=SWBaseSocket::ok)
	{
		wxMessageDialog *err=new wxMessageDialog(this, wxString(error.get_error()), "Error", wxOK | wxICON_ERROR);
		err->ShowModal();
		return;
	}
	mySocket.recv((char*)&head, sizeof(header_t), &error);
	if (error!=SWBaseSocket::ok)
	{
		wxMessageDialog *err=new wxMessageDialog(this, wxString(error.get_error()), "Error", wxOK | wxICON_ERROR);
		err->ShowModal();
		return;
	}
	char sversion[256];
	if (head.command!=MSG_VERSION)
	{
		wxMessageDialog *err=new wxMessageDialog(this, wxString("Unexpected message"), "Error", wxOK | wxICON_ERROR);
		err->ShowModal();
		return;
	}
	if (head.size>255)
	{
		wxMessageDialog *err=new wxMessageDialog(this, wxString("Message too long"), "Error", wxOK | wxICON_ERROR);
		err->ShowModal();
		return;
	}
	int rlen=0;
	while (rlen<head.size)
	{
		rlen+=mySocket.recv(sversion+rlen, head.size-rlen, &error);
		if (error!=SWBaseSocket::ok)
		{
			wxMessageDialog *err=new wxMessageDialog(this, wxString("Unexpected message"), "Error", wxOK | wxICON_ERROR);
			err->ShowModal();
			return;
		}
	}
	sversion[rlen]=0;
	mySocket.disconnect(&error);
	char message[512];
	sprintf(message, "Game server found, version %s", sversion);
	wxMessageDialog *res=new wxMessageDialog(this, wxString(message), "Success", wxOK | wxICON_INFORMATION );
	res->ShowModal();
	*/
#endif
}
