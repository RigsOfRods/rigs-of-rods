/*
    This source file is part of Rigs of Rods
    Copyright 2005-2009 Pierre-Michel Ricordel
    Copyright 2007-2009 Thomas Fischer
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


#include "RoRConfig.h"

#include <OgreConfigFile.h>
#include <OgreRoot.h>

#include <memory>

// --------- TODO: check and cleanup these includes -----------

#include "RoRnet.h"
#include "RoRVersion.h"
#include "conf_file.h"
#include "wxValueChoice.h" // a control we wrote

#include "input_engine.h"
#include "treelistctrl.h"

#if OGRE_PLATFORM == OGRE_PLATFORM_LINUX
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

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
#include "joystick_cfg.xpm"
#include "mouse_cfg.xpm"
#include "keyboard_cfg.xpm"

#define MAX_EVENTS 2048
#define LOG_STREAM Ogre::LogManager::getSingleton().getDefaultLog()->stream()
InputEngine INPUTENGINE;

wxLocale lang_locale;
wxLanguageInfo *language=0;
std::vector<wxLanguageInfo*> avLanguages;

std::map<std::string, std::string> settings;

#ifdef USE_OPENAL
  #ifdef __APPLE__
    #include <OpenAL/al.h>
    #include <OpenAL/alc.h>
    #include <OpenAL/MacOSX_OALExtensions.h>
  #else
    #include <AL/al.h>
    #include <AL/alc.h>
    #include <AL/alext.h>
  #endif // __APPLE__
#endif // USE_OPENAL

#ifdef USE_OPENCL
#include <delayimp.h>
#endif // USE_OPENCL

#ifdef __WXGTK__
#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#endif

IMPLEMENT_APP(RoRConfigApp) // Defines `wxGetApp()` which returns instance of RoRConfigApp

class KeySelectDialog; // Forward

// Define a new frame type: this is going to be our main frame
class MyDialog : public wxDialog
{
public:
    // ctor(s)
    MyDialog(const wxString& title);
    void loadOgre();
    void addAboutEntry(wxString name, wxString desc, wxString url, int &x, int &y);
    void addAboutTitle(wxString name, int &x, int &y);
    void updateRendersystems(Ogre::RenderSystem *rs);
    void SetDefaults();
    bool LoadConfig();
    void SaveConfig();
    void updateSettingsControls(); // use after loading or after manually updating the settings map
    void getSettingsControls();    // puts the control's status into the settings map
    void remapControl();
    void loadInputControls();
    void testEvents();
    bool isSelectedControlGroup(wxTreeItemId *item = 0);
    // event handlers (these functions should _not_ be virtual)
    void OnQuit               (wxCloseEvent& event);
    void OnTimer              (wxTimerEvent& event);
    void OnTimerReset         (wxTimerEvent& event);
    void OnButCancel          (wxCommandEvent& event);
    void OnButSave            (wxCommandEvent& event);
    void OnButPlay            (wxCommandEvent& event);
    void OnButRestore         (wxCommandEvent& event);
    void OnButGetUserToken    (wxCommandEvent& event);
    void OnChoiceShadow       (wxCommandEvent& event);
    void OnChoiceLanguage     (wxCommandEvent& event);
    void OnChoiceRenderer     (wxCommandEvent& event);
    void onActivateItem       (wxTreeEvent& event);
    void OnButLoadKeymap      (wxCommandEvent& event);
    void OnButSaveKeymap      (wxCommandEvent& event);
    void onRightClickItem     (wxTreeEvent& event);
    void OnMenuJoystickOptionsClick(wxCommandEvent& event);
    void OnButRegenCache      (wxCommandEvent& event);
    void OnButTestEvents      (wxCommandEvent& event);
    void OnButAddKey          (wxCommandEvent& event);
    void OnButDeleteKey       (wxCommandEvent& event);
    void OnButClearCache      (wxCommandEvent& event);
    void OnButUpdateRoR       (wxCommandEvent& event);
    void OnButCheckOpenCL     (wxCommandEvent& event);
    void OnButCheckOpenCLBW   (wxCommandEvent& event);
    void OnButReloadControllerInfo(wxCommandEvent& event);
    void OnScrollSightRange   (wxScrollEvent& event);
    void OnScrollVolume       (wxScrollEvent& event);
    void OnScrollForceFeedback(wxScrollEvent& event);
    void OnScrollFPSLimiter   (wxScrollEvent& event);
    void updateRoR();
    std::string readVersionInfo();

    void updateItemText(wxTreeItemId item, event_trigger_t *t);
    void onTreeSelChange (wxTreeEvent &event);

private:

    Ogre::Root *ogreRoot;
    int controlItemCounter;
    std::map<int,wxTreeItemId> treeItems;
    std::vector<std::string>     renderer_name;
    std::vector<wxStaticText *>  renderer_text;
    std::vector<wxValueChoice *> renderer_choice;
    wxButton *btnAddKey;
    wxButton *btnDeleteKey;
    wxButton *btnUpdate, *btnToken;
    wxButton *btnLoadInputDeviceInfo;
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
    wxTextCtrl *controllerInfo;
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

    // Input bindings
    wxTreeListCtrl *cTree; // Key input panel
    event_trigger_t *getSelectedControlEvent();
    wxString InputMapFileName;
    KeySelectDialog *kd;

    void tryLoadOpenCL();
    int openCLAvailable;
    DECLARE_EVENT_TABLE()
    bool loadOgrePlugins(Ogre::String pluginsfile);
};

class KeyAddDialog : public wxDialog
{
public:
    KeyAddDialog(MyDialog *_dlg) : wxDialog(NULL, wxID_ANY, wxString(), wxDefaultPosition, wxSize(400,200), wxSTAY_ON_TOP|wxDOUBLE_BORDER|wxCLOSE_BOX), dlg(_dlg)
    {
        m_selected_event="";
        selectedType="";
        selectedOption="  ";
        wxString configpath = wxGetApp().GetConfigPath() + wxFileName::GetPathSeparator();
        loadEventListing(configpath.ToUTF8().data());

        wxStaticText *text = new wxStaticText(this, 4, _("Event Name: "), wxPoint(5,5), wxSize(70, 25));
        wxString eventChoices[MAX_EVENTS];
        std::map<std::string, std::string>::iterator it;
        int counter=0;
        for(it=inputevents.begin(); it!=inputevents.end(); it++, counter++)
        {
            if(counter>MAX_EVENTS)
                break;
            eventChoices[counter] = wxString(it->first.c_str(), wxConvUTF8);;
        }
        cbe = new wxComboBox(this, 1, _("Keyboard"), wxPoint(80,5), wxSize(300, 25), counter, eventChoices, wxCB_READONLY);
        cbe->SetSelection(0);


        desctext = new wxStaticText(this, wxID_ANY, _("Please select an input event."), wxPoint(5,35), wxSize(380, 80), wxST_NO_AUTORESIZE);
        desctext->Wrap(380);


        text = new wxStaticText(this, wxID_ANY, _("Input Type: "), wxPoint(5,120), wxSize(70,25));
        wxString ch[3];
        ch[0] = conv("Keyboard");
        ch[1] = conv("JoystickAxis");
        ch[2] = conv("JoystickButton");
        cbi = new wxComboBox(this, wxID_ANY, _("Keyboard"), wxPoint(80,120), wxSize(300, 25), 3, ch, wxCB_READONLY);

        wxButton *btnOK = new wxButton(this, 2, _("Add"), wxPoint(5,165), wxSize(190,25));
        btnOK->SetToolTip(wxString(_("Add the key")));

        wxButton *btnCancel = new wxButton(this, 3, _("Cancel"), wxPoint(200,165), wxSize(190,25));
        btnCancel->SetToolTip(wxString(_("close this dialog")));

        //SetBackgroundColour(wxColour("white"));
        Centre();
    }

    void onChangeEventComboBox(wxCommandEvent& event)
    {
        std::string s = inputevents[conv(cbe->GetValue())];
        desctext->SetLabel(_("Event Description: ") + conv(s.c_str()));
        m_selected_event = conv(cbe->GetValue());
    }

    void onChangeTypeComboBox(wxCommandEvent& event)
    {
        selectedType = conv(cbi->GetValue());
    }
    void onButCancel(wxCommandEvent& event)
    {
        EndModal(1);
    }
    void onButOK(wxCommandEvent& event)
    {
        m_selected_event = conv(cbe->GetValue());
        selectedType = conv(cbi->GetValue());
        // add some dummy values
        if(conv(selectedType) == conv("Keyboard"))
            selectedOption="";
        else if(conv(selectedType) == conv("JoystickAxis"))
            selectedOption=" 0";
        else if(conv(selectedType) == conv("JoystickButton"))
            selectedOption=" 0";

        EndModal(0);
    }
    std::string getEventName()
    {
        return m_selected_event;
    }
    std::string getEventType()
    {
        return selectedType;
    }
    std::string getEventOption()
    {
        return selectedOption;
    }

private:

    bool loadEventListing(const char* configpath)
    {
        char filename[255]="";
        char line[1025]="";
        sprintf(filename, "%sinputevents.cfg", configpath);
        FILE *f = fopen(filename, "r");
        if(!f)
            return false;
        while(fgets(line, 1024, f)!=NULL)
        {
            if(strnlen(line, 1024) > 5)
                processLine(line);
        }
        fclose(f);
        return true;
    }

    bool processLine(char *str)
    {
        char tmp[255]="";
        memset(tmp, 0, 255);
        char *desc = strstr(str, "\t");
        if(!desc)
            return false;
        strncpy(tmp, str, desc-str);
        if(strnlen(tmp, 250) < 3 || strnlen(desc, 250) < 3)
            return false;
        std::string eventName = std::string(tmp);
        std::string eventDescription = std::string(desc+1); // +1 because we strip the tab character with that
        inputevents[eventName] = eventDescription;
        return true;
    }

    MyDialog *dlg;
    event_trigger_t *t;
    std::map<std::string, std::string> inputevents;
    wxStaticText *desctext;
    std::string m_selected_event;
    std::string selectedType;
    std::string selectedOption;
    wxComboBox *cbe, *cbi;

    DECLARE_EVENT_TABLE()
};

#define MAX_TESTABLE_EVENTS 500

class KeyTestDialog : public wxDialog
{
public:
    KeyTestDialog(MyDialog *_dlg) : wxDialog(NULL, wxID_ANY, wxString(), wxDefaultPosition, wxSize(500,600), wxSTAY_ON_TOP|wxDOUBLE_BORDER|wxCLOSE_BOX), dlg(_dlg)
    {
        new wxStaticText(this, wxID_ANY, _("Event Name"), wxPoint(5, 0), wxSize(200, 15));
        new wxStaticText(this, wxID_ANY, _("Event Value (left = 0%, right = 100%)"), wxPoint(210, 0), wxSize(200, 15));
        new wxButton(this, 1, _("ok (end test)"), wxPoint(5,575), wxSize(490,20));
        wxScrolledWindow *vwin = new wxScrolledWindow(this, wxID_ANY, wxPoint(0,20), wxSize(490,550));

        std::map<int, std::vector<event_trigger_t> > events = INPUTENGINE.getEvents();
        std::map<int, std::vector<event_trigger_t> >::iterator it;
        std::vector<event_trigger_t>::iterator it2;
        int counter = 0;
        for(it = events.begin(); it!= events.end(); it++)
        {
            for(it2 = it->second.begin(); it2!= it->second.end(); it2++, counter++)
            {
                if(counter >= MAX_TESTABLE_EVENTS)
                    break;
                g[counter] = new wxGauge(vwin, wxID_ANY, 1000, wxPoint(210, counter*20+5), wxSize(246, 15),wxGA_SMOOTH);
                t[counter] = new wxStaticText(vwin, wxID_ANY, conv(TOSTRING(it->first)), wxPoint(5, counter*20+5), wxSize(200, 15), wxALIGN_RIGHT|wxST_NO_AUTORESIZE);
            }
        }
        // resize scroll window
        vwin->SetScrollbars(0, 20, 0, counter);

        size_t hWnd = (size_t)this->GetHandle();
        if(!INPUTENGINE.setup(Ogre::StringConverter::toString(hWnd), true, false))
        {
            wxLogStatus(wxT("Unable to open inputs!"));
        }

        timer = new wxTimer(this, 2);
        timer->Start(50);

    }
    void update()
    {
        std::map<int, std::vector<event_trigger_t> > events = INPUTENGINE.getEvents();
        std::map<int, std::vector<event_trigger_t> >::iterator it;
        std::vector<event_trigger_t>::iterator it2;
        int counter = 0;
        for(it = events.begin(); it!= events.end(); it++)
        {
            for(it2 = it->second.begin(); it2!= it->second.end(); it2++, counter++)
            {
                if(counter >= MAX_TESTABLE_EVENTS)
                    break;
                float v = INPUTENGINE.getEventValue(it->first) * 1000;
                g[counter]->SetValue((int)v);
            }
        }
    }
    void OnButOK(wxCommandEvent& event)
    {
        EndModal(0);
    }
    void OnTimer(wxTimerEvent& event)
    {
        INPUTENGINE.Capture();
        update();
    }

protected:
    MyDialog *dlg;
    wxGauge *g[MAX_TESTABLE_EVENTS];
    wxStaticText *t[MAX_TESTABLE_EVENTS];
    wxTimer *timer;

private:
    DECLARE_EVENT_TABLE()
};


class KeySelectDialog : public wxDialog
{
public:
    KeySelectDialog(MyDialog *_dlg, event_trigger_t *_t) : wxDialog(NULL, wxID_ANY, wxString(), wxDefaultPosition, wxSize(200,200), wxSTAY_ON_TOP|wxDOUBLE_BORDER|wxCLOSE_BOX), t(_t), dlg(_dlg)
    {
        size_t hWnd = (size_t)this->GetHandle();
        bool captureMouse = false;
        if(t->eventtype == ET_MouseAxisX || t->eventtype == ET_MouseAxisY || t->eventtype == ET_MouseAxisZ || t->eventtype == ET_MouseButton)
            captureMouse = true;


        for(int x=0;x<32;x++)
        {
            joyMinState[x]=0;
            joyMaxState[x]=0;
        }

        if(!INPUTENGINE.setup(Ogre::StringConverter::toString(hWnd), true, captureMouse))
        {
            LOG_STREAM << "Unable to open default input map!";
        }

        INPUTENGINE.resetKeys();

        // setup GUI
        wxBitmap bitmap;
        
        if(t->eventtype == ET_MouseAxisX || t->eventtype == ET_MouseAxisY || t->eventtype == ET_MouseAxisZ || t->eventtype == ET_MouseButton)
        {
            bitmap = wxBitmap(mousecfg_xpm);
        }
        else if(t->eventtype == ET_Keyboard)
        {
            bitmap = wxBitmap(keyboardcfg_xpm);
        }
        else if(t->eventtype == ET_JoystickAxisAbs || t->eventtype == ET_JoystickAxisRel || t->eventtype == ET_JoystickButton || t->eventtype == ET_JoystickPov || t->eventtype == ET_JoystickSliderX || t->eventtype == ET_JoystickSliderY)
        {
            bitmap = wxBitmap(joycfg_xpm);
        }

        new wxStaticPicture(this, -1, bitmap, wxPoint(0, 0), wxSize(200, 200), wxNO_BORDER);
        
        wxString st = conv(t->tmp_eventname);
        text = new wxStaticText(this, wxID_ANY, st, wxPoint(5,130), wxSize(190,20), wxALIGN_CENTRE|wxST_NO_AUTORESIZE);
        text2 = new wxStaticText(this, wxID_ANY, wxString(), wxPoint(5,150), wxSize(190,50), wxALIGN_CENTRE|wxST_NO_AUTORESIZE);
        timer = new wxTimer(this, 2);
        timer->Start(50);

        SetBackgroundColour(wxColour(conv("white")));

        // centers dialog window on the screen
        Centre();
    }

    void closeWindow()
    {
        INPUTENGINE.destroy();
        timer->Stop();
        delete timer;
        delete text;
        delete text2;
        EndModal(0);
    }

    void update()
    {
        if(lastJoyEventTime > 0)
        {
            lastJoyEventTime-=0.05; // 50 milli seconds
            if(lastJoyEventTime <=0)
                closeWindow();
        }

        if(t->eventtype == ET_Keyboard)
        {
            std::string combo;
            int keys = INPUTENGINE.GetCurrentKeyCombo(&combo);
            if(lastCombo != combo && keys != 0)
            {
                strncpy(t->configline, combo.c_str(), sizeof(event_trigger_t::configline));
                wxString s = conv(combo);
                if(text2->GetLabel() != s)
                    text2->SetLabel(s);
                lastCombo = combo;
            } else if (lastCombo != combo && keys == 0)
            {
                wxString s = _("(Please press a key)");
                wxString label = text2->GetLabel();
                if(label != s)
                {
                    text2->SetLabel(s);
                }
                lastCombo = combo;
            } else if(keys > 0)
            {
                closeWindow();
            }
        } else if(t->eventtype == ET_JoystickButton)
        {
            int out_joy_number = -1;
            int out_joy_button = -1;
            int result = INPUTENGINE.getCurrentJoyButton(out_joy_number, out_joy_button);
            std::string str = conv(wxString::Format(_T("%d"), out_joy_button));
            int btn = out_joy_button;
            if(btn != lastBtn && btn >= 0)
            {
                strncmp(t->configline, str.c_str(), sizeof(event_trigger_t::configline));
                t->joystickButtonNumber = btn;
                wxString s = conv(str);
                if(text2->GetLabel() != s)
                    text2->SetLabel(s);
                lastBtn = btn;
            } else if (btn != lastBtn && btn < 0)
            {
                wxString s = _("(Please press a Joystick Button)");
                if(text2->GetLabel() != s)
                    text2->SetLabel(s);
                lastBtn = btn;
            } else if(btn >= 0)
            {
                closeWindow();
            }
        } else if(t->eventtype == ET_JoystickAxisAbs || t->eventtype == ET_JoystickAxisRel)
        {
            std::string str = "";
            OIS::JoyStickState *j = INPUTENGINE.getCurrentJoyState(0);



            std::vector<OIS::Axis>::iterator it;
            int counter = 0;
            for(it=j->mAxes.begin();it!=j->mAxes.end();it++, counter++)
            {
                float value = 100*((float)it->abs/(float)OIS::JoyStick::MAX_AXIS);
                if(value < joyMinState[counter]) joyMinState[counter] = value;
                if(value > joyMaxState[counter]) joyMaxState[counter] = value;
                //str += "Axis " + wxString::Format(_T("%d"), counter) + ": " + wxString::Format(_T("%0.2f"), value) + "\n";
            }

            // check for delta on each axis
            str = conv(_("(Please move an Axis)\n"));
            float maxdelta=0;
            int selectedAxis = -1;
            for(int c=0;c<=counter;c++)
            {
                float delta = fabs((float)(joyMaxState[c]-joyMinState[c]));
                if(delta > maxdelta && delta > 50)
                {
                    selectedAxis = c;
                    maxdelta = delta;
                }
            }

            if(selectedAxis >= 0)
            {
                float delta = fabs((float)(joyMaxState[selectedAxis]-joyMinState[selectedAxis]));
                static float olddeta = 0;
                bool upper = (joyMaxState[selectedAxis] > 50);
                if(delta > 50 && delta < 120)
                {
                    str = std::string("Axis ") + \
                            conv(wxString::Format(_T("%d"), selectedAxis)) + \
                            (upper?std::string(" UPPER"):std::string(" LOWER")); // + " - " + wxString::Format(_T("%f"), lastJoyEventTime);
                    t->joystickAxisNumber = selectedAxis;
                    t->joystickAxisRegion = (upper?1:-1); // half axis
                    strncmp(t->configline, (upper?"UPPER":"LOWER"), sizeof(event_trigger_t::configline));
                    if(olddeta != delta)
                    {
                        olddeta = delta;
                        lastJoyEventTime = 1;
                    }
                }
                else if(delta > 120)
                {
                    str = std::string("Axis ") + conv(wxString::Format(_T("%d"), selectedAxis)); //+ " - " + wxString::Format(_T("%f"), lastJoyEventTime);
                    t->joystickAxisNumber = selectedAxis;
                    t->joystickAxisRegion = 0; // full axis
                    t->configline[0] = '\0';
                    if(olddeta != delta)
                    {
                        olddeta = delta;
                        lastJoyEventTime = 1;
                    }
                }
            }
            //str = wxString::Format(_T("%f"), joyMaxState[0]) + "/" + wxString::Format(_T("%f"), joyMinState[0]) + " - " + wxString::Format(_T("%f"), fabs((float)(joyMaxState[0]-joyMinState[0])));
            wxString s = conv(str);
            if(text2->GetLabel() != s)
                text2->SetLabel(s);
        }
    }

    void OnButCancel(wxCommandEvent& event)
    {
        EndModal(0);
    }

    void OnTimer(wxTimerEvent& event)
    {
        INPUTENGINE.Capture();
        update();
    }

private:
    event_trigger_t *t;
    MyDialog *dlg;
    wxStaticText *text;
    wxStaticText *text2;
    wxButton *btnCancel;
    wxTimer *timer;
    std::string lastCombo;
    int lastBtn;
    float joyMinState[32];
    float joyMaxState[32];
    float lastJoyEventTime;

    DECLARE_EVENT_TABLE()
};

// ====== Bind wxWidgets events to handler functions ======
BEGIN_EVENT_TABLE(MyDialog, wxDialog)

    EVT_BUTTON(              ID_BUTTON_CANCEL,            MyDialog::OnButCancel)
    EVT_BUTTON(              ID_BUTTON_CHECK_OPENCL,      MyDialog::OnButCheckOpenCL)
    EVT_BUTTON(              ID_BUTTON_CHECK_OPENCL_BW,   MyDialog::OnButCheckOpenCLBW)
    EVT_BUTTON(              ID_BUTTON_CLEAR_CACHE,       MyDialog::OnButClearCache)
    EVT_BUTTON(              ID_BUTTON_GET_USER_TOKEN,    MyDialog::OnButGetUserToken)
    EVT_BUTTON(              ID_BUTTON_PLAY,              MyDialog::OnButPlay)
    EVT_BUTTON(              ID_BUTTON_REGEN_CACHE,       MyDialog::OnButRegenCache)
    EVT_BUTTON(              ID_BUTTON_SCAN_CONTROLLERS,  MyDialog::OnButReloadControllerInfo)
    EVT_BUTTON(              ID_BUTTON_RESTORE,           MyDialog::OnButRestore)
    EVT_BUTTON(              ID_BUTTON_SAVE,              MyDialog::OnButSave)
    EVT_BUTTON(              ID_BUTTON_UPDATE_ROR,        MyDialog::OnButUpdateRoR)
    EVT_CHOICE(              ID_CHOICE_LANGUAGE,          MyDialog::OnChoiceLanguage)
    EVT_CHOICE(              ID_CHOICE_RENDERER,          MyDialog::OnChoiceRenderer)
    EVT_CHOICE(              ID_CHOICE_SHADOW,            MyDialog::OnChoiceShadow)
    EVT_COMMAND_SCROLL(      ID_SCROLL_FORCE_FEEDBACK,    MyDialog::OnScrollForceFeedback)
    EVT_COMMAND_SCROLL(      ID_SCROLL_FPS_LIMITER,       MyDialog::OnScrollFPSLimiter)
    EVT_COMMAND_SCROLL(      ID_SCROLL_SIGHT_RANGE,       MyDialog::OnScrollSightRange)
    EVT_COMMAND_SCROLL(      ID_SCROLL_VOLUME,            MyDialog::OnScrollVolume)
    EVT_TIMER(               ID_TIMER_CONTROLS,           MyDialog::OnTimer)
    EVT_TIMER(               ID_TIMER_UPDATE_RESET,       MyDialog::OnTimerReset)

    // Input bindings
    EVT_TREE_SEL_CHANGING (   ID_KEYMAP_CTREE,            MyDialog::onTreeSelChange)
    EVT_TREE_ITEM_ACTIVATED(  ID_KEYMAP_CTREE,            MyDialog::onActivateItem)
    EVT_TREE_ITEM_RIGHT_CLICK(ID_KEYMAP_CTREE,            MyDialog::onRightClickItem)
    EVT_BUTTON(               ID_COMMAND_LOAD_KEYMAP,     MyDialog::OnButLoadKeymap)
    EVT_BUTTON(               ID_COMMAND_SAVE_KEYMAP,     MyDialog::OnButSaveKeymap)
    EVT_BUTTON(               ID_COMMAND_ADD_KEY,         MyDialog::OnButAddKey)
    EVT_BUTTON(               ID_COMMAND_DELETE_KEY,      MyDialog::OnButDeleteKey)
    EVT_BUTTON(               ID_COMMAND_TESTEVENTS,      MyDialog::OnButTestEvents)

    EVT_MENU(1,  MyDialog::OnMenuJoystickOptionsClick)
    EVT_MENU(20, MyDialog::OnMenuJoystickOptionsClick)
    EVT_MENU(21, MyDialog::OnMenuJoystickOptionsClick)
    EVT_MENU(22, MyDialog::OnMenuJoystickOptionsClick)
    EVT_MENU(23, MyDialog::OnMenuJoystickOptionsClick)
    EVT_MENU(24, MyDialog::OnMenuJoystickOptionsClick)
    EVT_MENU(25, MyDialog::OnMenuJoystickOptionsClick)
    EVT_MENU(30, MyDialog::OnMenuJoystickOptionsClick)
    EVT_MENU(31, MyDialog::OnMenuJoystickOptionsClick)
    EVT_MENU(32, MyDialog::OnMenuJoystickOptionsClick)

    EVT_CLOSE(MyDialog::OnQuit)

END_EVENT_TABLE()

BEGIN_EVENT_TABLE(KeySelectDialog, wxDialog)
    EVT_BUTTON(1, KeySelectDialog::OnButCancel)
    EVT_TIMER(2, KeySelectDialog::OnTimer)
END_EVENT_TABLE()

BEGIN_EVENT_TABLE(KeyAddDialog, wxDialog)
    EVT_COMBOBOX(1, KeyAddDialog::onChangeEventComboBox)
    EVT_BUTTON(2, KeyAddDialog::onButOK)
    EVT_BUTTON(3, KeyAddDialog::onButCancel)
    EVT_COMBOBOX(4, KeyAddDialog::onChangeTypeComboBox)
END_EVENT_TABLE()

BEGIN_EVENT_TABLE(KeyTestDialog, wxDialog)
    EVT_BUTTON(1, KeyTestDialog::OnButOK)
    EVT_TIMER(2, KeyTestDialog::OnTimer)

END_EVENT_TABLE()

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
    }
    else
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
            }
            else
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
    }
    else
    {
        wxLogStatus(wxT("no Languages found!"));
    }

    try
    {
        wxString rorcfg=userpath + dirsep + wxT("config") + dirsep + wxT("RoR.cfg");
        Ogre::ConfigFile cfg;
        // Don't trim whitespace
        cfg.load((const char*)rorcfg.mb_str(wxConvUTF8), "=:\t", false);
        wxString langSavedName = conv(cfg.getSetting("Language"));

        if(langSavedName.size() > 0)
            language = const_cast<wxLanguageInfo *>(getLanguageInfoByName(langSavedName));
    }
    catch (...)
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
        }
        else
        {
            wxLogStatus(wxT("Failed to detect system language."));
            return;
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

bool RoRConfigApp::CheckAndPrepareUserDirectory()
{
    wxFileName configPath = wxFileName(this->GetUserPath(), wxEmptyString);
    configPath.AppendDir(wxT("config"));
    wxString configPathStr = configPath.GetFullPath();
    // check if the user path is valid, if not create it

    if (wxFileName::DirExists(m_user_path) && wxFileName::DirExists(this->GetConfigPath()))
    {
        if (!wxFileName::DirExists (this->GetCachePath ()))
        {
            wxLogInfo (wxT ("Checking if cache path exist: ") + this->GetCachePath ());
            wxFileName::Mkdir (this->GetCachePath ());
        }

        if (!wxFileName::DirExists (this->GetLogPath ()))
        {
            wxLogInfo (wxT ("Creating log path: ") + this->GetLogPath ());
            wxFileName::Mkdir (this->GetLogPath ());
        }
        return true;
    }

    if (!wxFileName::DirExists (m_user_path))
    {
        wxLogInfo (wxT ("Preparing user directory: ") + m_user_path);
        wxFileName::Mkdir (m_user_path);
    }
    // first: figure out the zip path
    wxFileName skeletonZip = wxFileName(m_program_path, wxEmptyString);
    skeletonZip.AppendDir(wxT("resources"));
    skeletonZip.SetFullName(wxT("skeleton.zip"));
    wxString skeletonZipFile = skeletonZip.GetFullPath();

    if(!wxFileName::FileExists(skeletonZipFile))
    {
        // try the dev dir as well
        skeletonZip = wxFileName(m_program_path, wxEmptyString);
        skeletonZip.RemoveLastDir();
        skeletonZip.AppendDir(wxT("resources"));
        skeletonZip.SetFullName(wxT("skeleton.zip"));
        skeletonZipFile = skeletonZip.GetFullPath();
    }

    if(!wxFileName::FileExists(skeletonZipFile))
    {
        // tell the user
        wxString warning = wxString::Format(_("Rigs of Rods User directory missing:\n%s\n\nit could not be created since skeleton.zip was not found in\n %s"), m_user_path.c_str(), skeletonZipFile.c_str());
        wxString caption = _("error upon loading RoR user directory");
        wxMessageDialog *w = new wxMessageDialog(NULL, warning, caption, wxOK|wxICON_ERROR|wxSTAY_ON_TOP, wxDefaultPosition);
        w->ShowModal();
        delete(w);
        exit(1); // TODO: return FALSE from `OnInit()` instead! ~ only_a_ptr, 02/2017
    }

    ExtractZipFiles(skeletonZipFile, m_user_path);

    return true;
}

bool RoRConfigApp::InitFilesystem()
{
    m_user_path    = conv(SETTINGS.GetUserPath());
    m_program_path = conv(SETTINGS.GetProgramPath());


    this->CheckAndPrepareUserDirectory();
    return true;
}

void RoRConfigApp::InitLogging()
{
    // log everything
    wxLog::SetLogLevel(wxLOG_Max);
    wxLog::SetVerbose();

    // use stdout always
    wxLog *logger_cout = new wxLogStream(&std::cout);
    wxLog::SetActiveTarget(logger_cout);

    wxFileName clfn=wxFileName(m_user_path, wxEmptyString);
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
int RoRConfigApp::OnRun()
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
bool RoRConfigApp::OnInit()
{
    try
    {
        //setup the user filesystem
        if(!SETTINGS.setupPaths())
            return false;

        if (!this->InitFilesystem()) return false;

        this->InitLogging();
        initLanguage(wxT("languages"), m_user_path);

        // call the base class initialization method, currently it only parses a
        // few common command-line options but it could be do more in the future
        if ( !wxApp::OnInit() )
            return false;

        wxLogStatus(wxT("Creating dialog"));
        // create the main application window
        wxString title = wxString::Format(_("Rigs of Rods version %s configuration"), wxString(ROR_VERSION_STRING, wxConvUTF8).c_str());
        MyDialog *dialog = new MyDialog(title);

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
    return true;
}

// ----------------------------------------------------------------------------
// main frame
// ----------------------------------------------------------------------------

// frame constructor
MyDialog::MyDialog(const wxString& title) : wxDialog(NULL, wxID_ANY, title,  wxPoint(100, 100), wxSize(500, 580), wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER), openCLAvailable(0)
{
#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE
    SetWindowVariant(wxWINDOW_VARIANT_MINI); //smaller texts for macOS
#endif
    SetWindowStyle(wxRESIZE_BORDER | wxCAPTION);

    wxFileSystem::AddHandler( new wxInternetFSHandler );
    wxToolTip::Enable(true);

    // Load default controller mapping

    wxLogStatus(wxT("InputEngine starting"));

    wxString inputmap_path_wx = wxGetApp().GetConfigPath() + wxFileName::GetPathSeparator() + wxT("input.map");
    std::string hwnd_str = Ogre::StringConverter::toString((size_t)this->GetHandle());
    if(!INPUTENGINE.setup(hwnd_str, false, false))
    {
        wxLogError(wxT("Unable to setup inputengine!"));
    }
    else
    {
        if (!INPUTENGINE.loadMapping(inputmap_path_wx.ToUTF8().data()))
        {
            wxLogError(wxT("Unable to open default input map!"));
        }
    }
    wxLogStatus(wxT("InputEngine started"));

    wxToolTip::SetDelay(100);
    wxSizer *mainsizer = new wxBoxSizer(wxVERTICAL);
    mainsizer->SetSizeHints(this);

    const wxBitmap bm(config_xpm);
    wxStaticPicture *imagePanel = new wxStaticPicture(this, -1, bm,wxPoint(0, 0), wxSize(500, 100), wxNO_BORDER);
    mainsizer->Add(imagePanel, 0, wxGROW);

    // =========================================================================
    // Widget hierarchy overview (partial)
    // -----------------------------------
    //
    // WINDOW: wxNotebook(nbook)
    // [Settings]: wxPanel(settingsPanel); wxSizer(settingsSizer)
    //     wxNotebook(snbook)
    //         [RenderSystem]: wxPanel(rsPanel)
    //         [Graphics]: wxPanel(graphicsPanel)
    //         [Gameplay]: wxPanel(gamePanel)
    //         [Advanced} ??
    //         [Debug] ??                                
    // [Controls]: wxPanel(controlsPanel); wxSizer(sizer_controls)
    //     wxNotebook(ctbook)
    //         [Bindings]: wxPanel(ctsetPanel); wxSizer(ctset_sizer)
    //             wxTreeCtrl(cTree)
    //             wxPanel(controlsInfoPanel)
    //         [Force feedback]: wxPanel(ffPanel)
    //         [Device info]: wxPanel(controller_info_panel)
    // [ForceFeedback]
    // [Network]
    // [About]
    // =========================================================================

    //notebook
    nbook=new wxNotebook(this, wxID_ANY, wxPoint(3, 100), wxSize(490, 415));
    mainsizer->Add(nbook, 1, wxGROW);

    wxSizer *btnsizer = new wxBoxSizer(wxHORIZONTAL);

    wxButton *btnRestore = new wxButton( this, ID_BUTTON_RESTORE, _("Restore Defaults"), wxPoint(35,520), wxSize(100,25));
    btnRestore->SetToolTip(_("Restore the default configuration."));
    btnsizer->Add(btnRestore, 1, wxGROW | wxALL, 5);

    wxButton *btnPlay = new wxButton( this, ID_BUTTON_PLAY, _("Save and Play"), wxPoint(140,520), wxSize(100,25));
    btnPlay->SetToolTip(_("Save the current configuration and start RoR."));
    btnsizer->Add(btnPlay, 1, wxGROW | wxALL, 5);

    wxButton *btnSaveAndExit = new wxButton( this, ID_BUTTON_SAVE, _("Save and Exit"), wxPoint(245,520), wxSize(100,25));
    btnSaveAndExit->SetToolTip(_("Save the current configuration and close the configuration program."));
    btnsizer->Add(btnSaveAndExit, 1, wxGROW | wxALL, 5);

    wxButton *btnClose = new wxButton( this, ID_BUTTON_CANCEL, _("Cancel"), wxPoint(350,520), wxSize(100,25));
    btnClose->SetToolTip(_("Cancel the configuration changes and close the configuration program."));
    btnsizer->Add(btnClose, 1, wxGROW | wxALL, 5);

    mainsizer->Add(btnsizer, 0, wxGROW);
    this->SetSizer(mainsizer);

    settingsPanel=new wxPanel(nbook, -1);
    nbook->AddPage(settingsPanel, _("Settings"), true);
    wxSizer *settingsSizer = new wxBoxSizer(wxVERTICAL);
    settingsPanel->SetSizer(settingsSizer);

    // second notebook containing the settings tabs
    wxNotebook *snbook=new wxNotebook(settingsPanel, wxID_ANY, wxPoint(0, 0), wxSize(100,250));
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

    wxSizer *sizer_controls = new wxBoxSizer(wxVERTICAL);
    controlsPanel->SetSizer(sizer_controls);

    // third notebook for input bindings
   
    wxNotebook *ctbook=new wxNotebook(controlsPanel, wxID_ANY, wxPoint(0, 0), wxSize(100,250));
    sizer_controls->Add(ctbook, 1, wxGROW);
    wxPanel *ctsetPanel=new wxPanel(ctbook, -1);

    ctbook->AddPage(ctsetPanel, _("Bindings"), false);
    wxSizer* ctset_sizer = new wxBoxSizer(wxVERTICAL);
    ctsetPanel->SetSizer(ctset_sizer);

    
    int maxTreeHeight = 310;
    int maxTreeWidth = 480;
    cTree = new wxTreeListCtrl(ctsetPanel,
                       ID_KEYMAP_CTREE,
                       wxPoint(0, 0),
                       wxSize(maxTreeWidth, maxTreeHeight));

    long style = cTree->GetWindowStyle();
    style |= wxTR_FULL_ROW_HIGHLIGHT;
    style |= wxTR_HIDE_ROOT;
    style |= wxTR_ROW_LINES;
    cTree->SetWindowStyle(style);
    ctset_sizer->Add(cTree, /*2,*/ 0, wxGROW);

    wxPanel *controlsInfoPanel = new wxPanel(ctsetPanel, wxID_ANY, wxPoint(0, 0 ), wxSize( maxTreeWidth, 390 ));
    ctset_sizer->Add(controlsInfoPanel, 0, wxGROW);

    // controlText = new wxStaticText(controlsInfoPanel, wxID_ANY, wxString("Key description..."), wxPoint(5,5));

    /*
    typeChoices[0] = "None";
    typeChoices[1] = "Keyboard";
    typeChoices[2] = "MouseButton";
    typeChoices[3] = "MouseAxisX";
    typeChoices[4] = "MouseAxisY";
    typeChoices[5] = "MouseAxisZ";
    typeChoices[6] = "JoystickButton";
    typeChoices[7] = "JoystickAxis";
    typeChoices[8] = "JoystickPov";
    typeChoices[9] = "JoystickSliderX";
    typeChoices[10] = "JoystickSliderY";
    */
    //ctrlTypeCombo = new wxComboBox(controlsInfoPanel, EVCB_BOX, "Keyboard", wxPoint(5,25), wxSize(150, 20), 11, typeChoices, wxCB_READONLY);

    btnAddKey = new wxButton(controlsInfoPanel, ID_COMMAND_ADD_KEY, _("Add"), wxPoint(5,5), wxSize(120, 50));
    btnDeleteKey = new wxButton(controlsInfoPanel, ID_COMMAND_DELETE_KEY, _("Delete selected"), wxPoint(130,5), wxSize(120, 50));
    btnDeleteKey->Enable(false);

    new wxButton(controlsInfoPanel, ID_COMMAND_LOAD_KEYMAP, _("Import Keymap"), wxPoint(maxTreeWidth-125,5), wxSize(110,25));
    new wxButton(controlsInfoPanel, ID_COMMAND_SAVE_KEYMAP, _("Export Keymap"), wxPoint(maxTreeWidth-125,30), wxSize(110,25));
    new wxButton(controlsInfoPanel, ID_COMMAND_TESTEVENTS, _("Test"), wxPoint(255,5), wxSize(95, 50));

    //btnRemap = new wxButton( controlsInfoPanel, BTN_REMAP, "Remap", wxPoint(maxTreeWidth-250,5), wxSize(120,45));

    loadInputControls();

    wxPanel *ffPanel = new wxPanel(ctbook, -1);
    ctbook->AddPage(ffPanel, _("Force Feedback"), false);

    wxPanel* controller_info_panel = new wxPanel(ctbook, -1);
    ctbook->AddPage(controller_info_panel, _("Device info"), false);

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
    renderer = new wxValueChoice(rsPanel, ID_CHOICE_RENDERER, wxPoint(220, 25), wxSize(220, -1), 0);
    // renderer options done, do the rest

    // gamePanel
    int y = 10;
    int x_row1 = 150, x_row2 = 300;

    dText = new wxStaticText(gamePanel, -1, _("Language:"), wxPoint(10, y));
    languageMode=new wxValueChoice(gamePanel, ID_CHOICE_LANGUAGE, wxPoint(x_row1, y), wxSize(200, -1), wxCB_READONLY);

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

    y+=35; // Removed widget: Multiplayer token field (not supported at the moment ~ 02/2017)

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
    dText = new wxStaticText(aboutPanel, -1, wxString::Format(_("Rigs of Rods version: %s"), wxString(ROR_VERSION_STRING, wxConvUTF8).c_str()), wxPoint(x_row1 + 15, y));
    y += dText->GetSize().GetHeight() + 2;

    dText = new wxStaticText(aboutPanel, -1, wxString::Format(_("Network Protocol version: %s"), wxString(RORNET_VERSION, wxConvUTF8).c_str()), wxPoint(x_row1 + 15, y));
    y += dText->GetSize().GetHeight() + 2;

    dText = new wxStaticText(aboutPanel, -1, wxString::Format(_("Build time: %s, %s"), ROR_BUILD_DATE, ROR_BUILD_TIME), wxPoint(x_row1 + 15, y));
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
    addAboutEntry(_("Authors"), _("You can find a complete list of the RoR's authors ingame: about->credits."), wxT("mailto:support@rigsofrods.org"), x_row1, y);

    y += 20;

    addAboutTitle(_("Missing someone?"), x_row1, y);
    addAboutEntry(_("Missing someone?"), _("If we are missing someone on this list, please drop us a line at:\nsupport@rigsofrods.org"), wxT("mailto:support@rigsofrods.org"), x_row1, y);

    wxSize size = nbook->GetBestVirtualSize();
    size.x = 400;
    aboutPanel->SetVirtualSize(size);
    aboutPanel->SetScrollRate(20, 25);

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
    wxHyperlinkCtrl *link = new wxHyperlinkCtrl(debugPanel, -1, _("(Read more on how to use these options here)"), _("http://www.rigsofrods.org/wiki/pages/Debugging_Trucks"), wxPoint(10, y));
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
    shadow=new wxValueChoice(graphicsPanel, ID_CHOICE_SHADOW, wxPoint(x_row1, y), wxSize(200, -1), 0);
    shadow->AppendValueItem(wxT("No shadows (fastest)"), _("No shadows (fastest)"));
    shadow->AppendValueItem(wxT("Texture shadows"), _("Texture shadows"));
#if OGRE_VERSION>0x010602
    shadow->AppendValueItem(wxT("Parallel-split Shadow Maps"), _("Parallel-split Shadow Maps"));
#endif //OGRE_VERSION
    shadow->SetToolTip(_("Shadow technique to be used ingame."));
    sky->SetSelection(1); //Texture shadows
    y+=25;

    dText = new wxStaticText(graphicsPanel, wxID_ANY, _("Sightrange:"), wxPoint(10,y+3));
    sightRange=new wxSlider(graphicsPanel, ID_SCROLL_SIGHT_RANGE, 5000, 10, 5000, wxPoint(x_row1, y), wxSize(200, -1));
    sightRange->SetToolTip(_("determines how far you can see in meters"));
    sightRangeText = new wxStaticText(graphicsPanel, wxID_ANY, _("Unlimited"), wxPoint(x_row1 + 210,y+3));
    y+=25;

    shadowOptimizations=new wxCheckBox(graphicsPanel, wxID_ANY, _("Shadow Performance Optimizations"), wxPoint(x_row1, y), wxSize(200, -1), 0);
    shadowOptimizations->SetToolTip(_("When turned on, it disables the vehicles shadow when driving in first person mode."));
    y+=25;

    dText = new wxStaticText(graphicsPanel, -1, _("Water type:"), wxPoint(10,y+3));
    water=new wxValueChoice(graphicsPanel, -1, wxPoint(x_row1, y), wxSize(200, -1), 0);
    //water->AppendValueItem(wxT("None"), _("None")); //we don't want that! // WTF?? ~ only_a_ptr, 02/2017
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
    screenShotFormat->AppendValueItem(wxT("jpg"), _("jpg (smaller, default)"));
    screenShotFormat->AppendValueItem(wxT("png"), _("png (bigger, no quality loss)"));
    screenShotFormat->SetToolTip(_("In what Format should screenshots be saved?"));

    //force feedback panel

    ffEnable=new wxCheckBox(ffPanel, -1, _("Enable Force Feedback"), wxPoint(150, 25));

    dText = new wxStaticText(ffPanel, -1, _("Overall force level:"), wxPoint(20,53));
    ffOverall=new wxSlider(ffPanel, ID_SCROLL_FORCE_FEEDBACK, 100, 0, 1000, wxPoint(150, 50), wxSize(200, 40));
    ffOverall->SetToolTip(_("Adjusts the level of all the forces."));
    ffOverallText=new wxStaticText(ffPanel, -1, wxString(), wxPoint(360,53));

    dText = new wxStaticText(ffPanel, -1, _("Steering feedback level:"), wxPoint(20,103));
    ffHydro=new wxSlider(ffPanel, ID_SCROLL_FORCE_FEEDBACK, 100, 0, 4000, wxPoint(150, 100), wxSize(200, 40));
    ffHydro->SetToolTip(_("Adjusts the contribution of forces coming from the wheels and the steering mechanism."));
    ffHydroText=new wxStaticText(ffPanel, -1, wxString(), wxPoint(360,103));

    dText = new wxStaticText(ffPanel, -1, _("Self-centering level:"), wxPoint(20,153));
    ffCenter=new wxSlider(ffPanel, ID_SCROLL_FORCE_FEEDBACK, 100, 0, 4000, wxPoint(150, 150), wxSize(200, 40));
    ffCenter->SetToolTip(_("Adjusts the self-centering effect applied to the driving wheel when driving at high speed."));
    ffCenterText=new wxStaticText(ffPanel, -1, wxString(), wxPoint(360,153));

    dText = new wxStaticText(ffPanel, -1, _("Inertia feedback level:"), wxPoint(20,203));
    ffCamera=new wxSlider(ffPanel, ID_SCROLL_FORCE_FEEDBACK, 100, 0, 4000, wxPoint(150, 200), wxSize(200, 40));
    ffCamera->SetToolTip(_("Adjusts the contribution of forces coming shocks and accelerations (this parameter is currently unused)."));
    ffCamera->Enable(false);
    ffCameraText=new wxStaticText(ffPanel, -1, wxString(), wxPoint(360,203));

    //update text boxes
    wxScrollEvent dummye;
    OnScrollForceFeedback(dummye);

    // [Controller] / [Device info] panel

    controllerInfo = new wxTextCtrl(
        controller_info_panel, wxID_ANY, wxString(), wxPoint(0,0), wxSize(465,330), wxTE_MULTILINE);

    btnLoadInputDeviceInfo = new wxButton(
        controller_info_panel, ID_BUTTON_SCAN_CONTROLLERS,
        _("(Re)load info"), wxPoint(300, 335), wxSize(165, 33));


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
    soundVolume=new wxSlider(advancedPanel, ID_SCROLL_VOLUME, 100, 0, 100, wxPoint(x_row1, y), wxSize(200, -1));
    soundVolume->SetToolTip(_("sets the master volume"));
    soundVolumeText = new wxStaticText(advancedPanel, -1, _("100 %"), wxPoint(x_row1 + 210, y+3));
    y+=35;
#endif //USE_OPENAL

    // Lights
    dText = new wxStaticText(advancedPanel, -1, _("Light source effects:"), wxPoint(10, y+3));
    flaresMode=new wxValueChoice(advancedPanel, -1, wxPoint(x_row1, y), wxSize(280, -1), 0);
//	flaresMode->AppendValueItem(wxT("None (fastest)"), _("None (fastest)")); //this creates a bug in the autopilot // WTF?? ~ only_a_ptr, 02/2017
    flaresMode->AppendValueItem(wxT("No light sources"), _("No light sources"));
    flaresMode->AppendValueItem(wxT("Only current vehicle, main lights"), _("Only current vehicle, main lights"));
    flaresMode->AppendValueItem(wxT("All vehicles, main lights"), _("All vehicles, main lights"));
    flaresMode->AppendValueItem(wxT("All vehicles, all lights"), _("All vehicles, all lights"));
    flaresMode->SetToolTip(_("Determines which lights will project light on the environment.\nThe more light sources are used, the slowest it will be."));
    y+=35;

    // FPS-Limiter
    dText = new wxStaticText(advancedPanel, -1, _("FPS-Limiter:"), wxPoint(10, y+3));
    fpsLimiter=new wxSlider(advancedPanel, ID_SCROLL_FPS_LIMITER, 0, 0, 200, wxPoint(x_row1, y), wxSize(200, -1));
    fpsLimiter->SetToolTip(_("sets the maximum frames per second"));
    fpsLimiterText = new wxStaticText(advancedPanel, -1, _("Unlimited"), wxPoint(x_row1 + 210, y+3));
    y+=35;

    // Cache
    dText = new wxStaticText(advancedPanel, -1, _("In case the mods cache becomes corrupted, \nuse these buttons to fix the cache."), wxPoint(10, y));

    wxButton *btn = new wxButton(advancedPanel, ID_BUTTON_REGEN_CACHE, _("Regen cache"), wxPoint(x_row2-52, y));
    btn->SetToolTip(_("Use this to regenerate the cache outside of RoR. If this does not work, use the clear cache button."));

    btn = new wxButton(advancedPanel, ID_BUTTON_CLEAR_CACHE, _("Clear cache"), wxPoint(x_row2+43, y));
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

    // controlstimer=new wxTimer(this, CONTROLS_TIMER_ID);
    timer1=new wxTimer(this, ID_TIMER_UPDATE_RESET);

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
    }
    else
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
        }
        catch (Ogre::Exception &e)
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
    wxString confdirPrefix = wxGetApp().GetConfigPath()  + wxFileName::GetPathSeparator();
    wxString logsdirPrefix = wxGetApp().GetLogPath()     + wxFileName::GetPathSeparator();
    wxString progdirPrefix = wxGetApp().GetProgramPath() + wxFileName::GetPathSeparator();
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
    }
    catch (Ogre::Exception& e)
    {
        if(e.getSource() == "D3D9RenderSystem::setConfigOption")
        {
            // this is a normal error that happens when the suers switch from ogre 1.6 to 1.7
        }
        else
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
    }
    else
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
    filterOptions["Allow DirectX9Ex"]=false;
    filterOptions["Fixed Pipeline Enabled"] = true;

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
        }
        else
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
    OnScrollForceFeedback(dummye);
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
    fpsLimiter->SetValue(0);           // 0 = unlimited
    gearBoxMode->SetSelection(0);
    glow->SetValue(false);
    hdr->SetValue(false);
    heathaze->SetValue(false);
    ingame_console->SetValue(false);
    dev_mode->SetValue(false);
    inputgrab->SetSelection(0);          // All
    mblur->SetValue(false);
    mirror->SetValue(true);
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

#ifdef USE_OPENAL
    settings["AudioDevice"] = sound->getSelectedValueAsSTDString();
    settings["Creak Sound"] = (creaksound->GetValue()) ? "No" : "Yes";
    settings["Sound Volume"] = TOSTRING(soundVolume->GetValue() / 100.0f);
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
    }
    else
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

    long fpslimit = 0;
    st = settings["FPS-Limiter"];
    if (st.length()>0)
    {
        if(conv(st).ToLong(&fpslimit))
            fpsLimiter->SetValue(fpslimit);
    }

#ifdef USE_OPENAL
    st = settings["Sound Volume"];
    double volume = 1.0f;
    if (st.length()>0)
    {
        if(conv(st).ToDouble(&volume))
            soundVolume->SetValue(volume * 100.0f);
    }
    st = settings["Creak Sound"]; if (st.length()>0) creaksound->SetValue(st=="No");
    sound->setSelectedValue(settings["AudioDevice"]);
#endif //USE_OPENAL

    st = settings["Skidmarks"]; if (st.length()>0) skidmarks->SetValue(st=="Yes");
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

    // update slider text
    OnScrollSightRange(dummye);
    OnScrollVolume(dummye);
    OnScrollFPSLimiter(dummye);
}

bool MyDialog::LoadConfig()
{
    //RoR config
    Ogre::ConfigFile cfg;
    try
    {
        wxLogStatus(wxT("Loading RoR.cfg"));

        wxString rorcfg = wxGetApp().GetConfigPath() + wxFileName::GetPathSeparator() + wxT("RoR.cfg");
        wxLogStatus(wxT("reading from Config file: ") + rorcfg);

        // Don't trim whitespace
        cfg.load((const char *)rorcfg.mb_str(wxConvUTF8), "=:\t", false);
    }
    catch (...)
    {
        wxLogError(wxT("error loading RoR.cfg"));
        return false;
    }

    // load all settings into a map!
    Ogre::ConfigFile::SettingsIterator i = cfg.getSettingsIterator();
    Ogre::String svalue, sname;
    while (i.hasMoreElements())
    {
        sname = SanitizeUtf8String(i.peekNextKey());
        Ogre::StringUtil::trim(sname);
        svalue = SanitizeUtf8String(i.getNext());
        Ogre::StringUtil::trim(svalue);
        // filter out some things that shouldnt be in there (since we cannot use RoR normally anymore after those)
        if(sname == Ogre::String("regen-cache-only"))
            continue;
        settings[sname] = svalue;
    }

    // then update the controls!
    updateSettingsControls();
    return false;
}

void MyDialog::SaveConfig()
{
    // first, update the settings map with the actual control values
    getSettingsControls();

    // then set stuff and write configs
    std::string dest_path = conv(InputMapFileName);
    INPUTENGINE.saveMapping(dest_path);
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
                }
                catch (...)
                {
                    wxMessageDialog(this, _("Error setting Ogre Values"), _("Ogre config validation error, unknown render option detected: ") + conv(key), wxOK||wxICON_ERROR).ShowModal();
                }
            }
            Ogre::String err = rs->validateConfigOptions();
            if (err.length() > 0)
            {
                wxMessageDialog(this, conv(err), _("Ogre config validation error"),wxOK||wxICON_ERROR).ShowModal();
            }
            else
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
    wxString rorcfg= wxGetApp().GetConfigPath() + wxFileName::GetPathSeparator() + wxT("RoR.cfg");

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
        }
        else
        {
            wxLogStatus(wxT("Unable to change to new rendersystem(1)"));
        }
    }
    catch (...)
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
    strcpy(tmp, SETTINGS.GetProgramPath().c_str());
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
    {
        return;
    }

    // wait until child process exits
    WaitForSingleObject( pi.hProcess, INFINITE );
    CloseHandle( pi.hProcess );
    CloseHandle( pi.hThread );
#endif
#if OGRE_PLATFORM == OGRE_PLATFORM_LINUX
    char tmp[4096] = "";
    strcpy(tmp, SETTINGS.GetProgramPath().c_str());
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

    wxString cachepath = wxGetApp().GetCachePath();
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

void MyDialog::OnButReloadControllerInfo(wxCommandEvent& event)
{
    controllerInfo->Clear();
    controllerInfo->AppendText(LoadInputDevicesInfo(this->GetHandle()));
}

void MyDialog::OnScrollSightRange(wxScrollEvent &e)
{
    wxString s;
    int v = sightRange->GetValue();
    if (v == sightRange->GetMax())
    {
        s = _("Unlimited");
    }
    else
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
    }
    else
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
    if (v == fpsLimiter->GetMin())
    {
        s = _("Unlimited");
    }
    else
    {
        s.Printf(wxT("%i fps"), v);
    }
    fpsLimiterText->SetLabel(s);
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
        return "unknown";
    }

    pbVersionInfo = new BYTE[ dwSize ];

    if ( !GetFileVersionInfo( pszFilePath, 0, dwSize, pbVersionInfo ) )
    {
        return "unknown";
    }

    if ( !VerQueryValue( pbVersionInfo, TEXT("\\"), (LPVOID*) &pFileInfo, &puLenFileInfo ) )
    {
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

void MyDialog::OnTimerReset(wxTimerEvent& event)
{
    btnUpdate->Enable(true);
}

void MyDialog::OnButGetUserToken(wxCommandEvent& event)
{
    wxLaunchDefaultBrowser(wxT("http://usertoken.rigsofrods.org"));
}

class IDData : public wxTreeItemData
{
public:
    IDData(int id) : id(id) {}
    int id;
};

void MyDialog::loadInputControls()
{
    // setup control tree
    std::map<int, std::vector<event_trigger_t> > & controls = INPUTENGINE.getEvents();
    std::map<int, std::vector<event_trigger_t> >::iterator mapIt;
    std::vector<event_trigger_t>::iterator vecIt;

    // clear everything
    controlItemCounter=0;
    cTree->DeleteRoot();

    if(cTree->GetColumnCount() < 4)
    {
        // only add them once
        cTree->AddColumn (_("Event"), 180);
        cTree->SetColumnEditable (0, false);
        cTree->AddColumn (_("Type"), 75);
        cTree->SetColumnEditable (1, false);
        cTree->AddColumn (_("Key/Axis"), 100);
        cTree->SetColumnEditable (2, false);
        cTree->AddColumn (_("Options"), 100);
        cTree->SetColumnEditable (3, false);
    }

    std::string curGroup = "";
    wxTreeItemId root = cTree->AddRoot(conv("Root"));
    wxTreeItemId *curRoot = 0;
    for(mapIt = controls.begin(); mapIt != controls.end(); mapIt++)
    {
        std::vector<event_trigger_t> vec = mapIt->second;

        for(vecIt = vec.begin(); vecIt != vec.end(); vecIt++, controlItemCounter++)
        {
            if(vecIt->group != curGroup || curRoot == 0)
            {
                //if(curRoot!=0)
                //	cTree->ExpandAll(*curRoot);
                curGroup = vecIt->group;
                wxTreeItemId tmp = cTree->AppendItem(root, conv(curGroup));
                curRoot = &tmp;
                cTree->SetItemData(curRoot, new IDData(-1));
            }

            //strip category name if possible
            wxString evName = conv(INPUTENGINE.eventIDToName(mapIt->first));
            std::string vec_item(vecIt->group);
            std::string map_item = TOSTRING(mapIt->first);
            if(vec_item.size()+1 < map_item.size())
            {
                evName = conv(map_item.substr(vec_item.size()+1));
            }
            wxTreeItemId item = cTree->AppendItem(*curRoot, evName);

            /*
            wxTreeItemId cItem_context = cTree->AppendItem(item, "Context");
            cTree->SetItemText (cItem_context, 1,  wxString(INPUTENGINE.getEventTypeName(vecIt->eventtype).c_str()));

            wxTreeItemId cItem_function = cTree->AppendItem(item, "Function");
            wxTreeItemId cItem_input = cTree->AppendItem(item, "Input");
            */

            // set some data beside the tree entry
            cTree->SetItemData(item, new IDData(vecIt->suid));
            this->updateItemText(item, (event_trigger_t *)&(*vecIt));
            treeItems[vecIt->suid] = item;

            /*
            cTree->SetItemText (item, 1,  wxString(INPUTENGINE.getEventTypeName(vecIt->eventtype).c_str()));
            if(vecIt->eventtype == ET_Keyboard)
            {
                cTree->SetItemText (cItem_function, 1,  "");
                cTree->SetItemText (cItem_input, 1, wxString(vecIt->configline.c_str()));

                cTree->SetItemText (item, 2, wxString(vecIt->configline.c_str()));
            } else if(vecIt->eventtype == ET_JoystickAxisAbs || vecIt->eventtype == ET_JoystickAxisRel)
            {
                cTree->SetItemText (cItem_function, 1,  wxString::Format(_T("%d"), vecIt->joystickAxisNumber));
                cTree->SetItemText (cItem_input, 1, wxString((vecIt->configline).c_str()));

                cTree->SetItemText (item, 2,  wxString::Format(_T("%d"), vecIt->joystickAxisNumber));
                cTree->SetItemText (item, 3,  wxString((vecIt->configline).c_str()));
            } else if(vecIt->eventtype == ET_JoystickButton)
            {
                cTree->SetItemText (cItem_function, 1,  "");
                cTree->SetItemText (cItem_input, 1, wxString::Format(_T("%d"), vecIt->joystickButtonNumber));

                cTree->SetItemText (item, 2, wxString::Format(_T("%d"), vecIt->joystickButtonNumber));
            }
            */

        }
    }
    //cTree->ExpandAll(root);
}

void MyDialog::updateItemText(wxTreeItemId item, event_trigger_t *t)
{
    cTree->SetItemText (item, 1,  conv(INPUTENGINE.getEventTypeName(t->eventtype)));
    if(t->eventtype == ET_Keyboard)
    {
        cTree->SetItemText (item, 2, conv(t->configline));
    } else if(t->eventtype == ET_JoystickAxisAbs || t->eventtype == ET_JoystickAxisRel)
    {
        cTree->SetItemText (item, 2,  wxString::Format(_T("%d"), t->joystickAxisNumber));
        cTree->SetItemText (item, 3,  conv((t->configline)));
    } else if(t->eventtype == ET_JoystickButton)
    {
        cTree->SetItemText (item, 2, wxString::Format(_T("%d"), t->joystickButtonNumber));
    }
}

void MyDialog::onTreeSelChange (wxTreeEvent &event)
{
    wxTreeItemId itm = event.GetItem();
    if(isSelectedControlGroup(&itm))
    {
        btnDeleteKey->Disable();
    } else
    {
        btnDeleteKey->Enable();
    }

    //wxTreeItemId item = event.GetItem();
    //event_trigger_t *t = getSelectedControlEvent();

    // type
    //wxString typeStr = cTree->GetItemText(item, 1);
    //ctrlTypeCombo->SetValue(typeStr);
    //ctrlTypeCombo->Enable();

    // description
    //wxString descStr = cTree->GetItemText(item, 0);
    //controlText->SetLabel((t->group + "_" + descStr.c_str()).c_str());
}


void MyDialog::OnButLoadKeymap(wxCommandEvent& event)
{
    wxFileDialog *f = new wxFileDialog(this, _("Choose a file"), wxString(), wxString(), conv("*.map"), wxFD_OPEN|wxFD_FILE_MUST_EXIST);
    if(f->ShowModal() == wxID_OK)
    {
        INPUTENGINE.loadMapping(conv(f->GetPath()));
        loadInputControls();
    }
    delete f;
}

void MyDialog::OnButSaveKeymap(wxCommandEvent& event)
{
    wxFileDialog *f = new wxFileDialog(this, _("Choose a file"), wxString(), wxString(), conv("*.map"), wxFD_SAVE|wxFD_FILE_MUST_EXIST);
    if(f->ShowModal() == wxID_OK)
    {
        INPUTENGINE.saveMapping(conv(f->GetPath()));
    }
    delete f;
}

void MyDialog::onActivateItem(wxTreeEvent& event)
{
    wxTreeItemId itm = event.GetItem();
    if(!isSelectedControlGroup(&itm))
    {
        remapControl();
    }
    else
    {
        // expand/collapse group otherwise
        if(cTree->IsExpanded(event.GetItem()))
        {
            cTree->Collapse(event.GetItem());
        }
        else
        {
            cTree->Expand(event.GetItem());
        }
    }
}

void MyDialog::OnButTestEvents(wxCommandEvent& event)
{
    testEvents();
}


void MyDialog::testEvents()
{
    KeyTestDialog *kt = new KeyTestDialog(this);
    if(kt->ShowModal()==0)
    {
    }
    kt->Destroy();
    delete kt;
}

void MyDialog::onRightClickItem(wxTreeEvent& event)
{
    if(isSelectedControlGroup())
        return;
    event_trigger_t *t = getSelectedControlEvent();
    if(!t)
        return;
    if(t->eventtype == ET_JoystickAxisAbs || t->eventtype == ET_JoystickAxisRel)
    {
        wxMenu *m = new wxMenu(_("Joystick Options"));

        wxMenuItem *i = m->AppendCheckItem(1, _("Reverse"), _("reverse axis"));
        i->Check(t->joystickAxisReverse);

        wxMenu *iregion = new wxMenu(_("Joystick Axis Region"));
        i = iregion->AppendRadioItem(30, _("Upper"));
        i->Check(t->joystickAxisRegion == 1);
        i = iregion->AppendRadioItem(31, _("Full"));
        i->Check(t->joystickAxisRegion == 0);
        i = iregion->AppendRadioItem(32, _("Lower"));
        i->Check(t->joystickAxisRegion == -1);
        i = m->AppendSubMenu(iregion, _("Region"));

        wxMenu *idead = new wxMenu(_("Joystick Axis Deadzones"));
        i = idead->AppendRadioItem(20, _("0 % (No deadzone)"), _("no deadzone"));
        i->Check(fabs(t->joystickAxisDeadzone)< 0.0001);

        i = idead->AppendRadioItem(21, _("5 %"), _("5% deadzone"));
        i->Check(fabs(t->joystickAxisDeadzone-0.05)< 0.0001);

        i = idead->AppendRadioItem(22, _("10 %"), _("10% deadzone"));
        i->Check(fabs(t->joystickAxisDeadzone-0.1)< 0.0001);

        i = idead->AppendRadioItem(23, _("15 %"), _("15% deadzone"));
        i->Check(fabs(t->joystickAxisDeadzone-0.15)< 0.0001);

        i = idead->AppendRadioItem(24, _("20 %"), _("20% deadzone"));
        i->Check(fabs(t->joystickAxisDeadzone-0.2)< 0.0001);

        i = idead->AppendRadioItem(25, _("30 %"), _("30% deadzone"));
        i->Check(fabs(t->joystickAxisDeadzone-0.3)< 0.0001);

        i = m->AppendSubMenu(idead, _("Deadzone"));
        this->PopupMenu(m);
    }
}


void MyDialog::OnMenuJoystickOptionsClick(wxCommandEvent& ev)
{
    event_trigger_t *t = getSelectedControlEvent();
    if(!t)
        return;
    if(t->eventtype != ET_JoystickAxisAbs && t->eventtype != ET_JoystickAxisRel)
        return;
    switch (ev.GetId())
    {
    case 1: //reverse
        {
            t->joystickAxisReverse = ev.IsChecked();
            break;
        }
    case 20: //0% deadzone
        {
            t->joystickAxisDeadzone = 0;
            break;
        }
    case 21: //5% deadzone
        {
            t->joystickAxisDeadzone = 0.05;
            break;
        }
    case 22: //10% deadzone
        {
            t->joystickAxisDeadzone = 0.1;
            break;
        }
    case 23: //15% deadzone
        {
            t->joystickAxisDeadzone = 0.15;
            break;
        }
    case 24: //20% deadzone
        {
            t->joystickAxisDeadzone = 0.2;
            break;
        }
    case 25: //30% deadzone
        {
            t->joystickAxisDeadzone = 0.3;
            break;
        }
    case 30: //upper
        {
            t->joystickAxisRegion = 1;
            break;
        }
    case 31: //full
        {
            t->joystickAxisRegion = 0;
            break;
        }
    case 32: //lower
        {
            t->joystickAxisRegion = -1;
            break;
        }
    }

    INPUTENGINE.updateConfigline(t);
    updateItemText(cTree->GetSelection(), t);
}

void MyDialog::OnButDeleteKey(wxCommandEvent& event)
{
    wxTreeItemId item = cTree->GetSelection();
    IDData *data = (IDData *)cTree->GetItemData(item);
    if(!data || data->id < 0)
        return;
    INPUTENGINE.deleteEventBySUID(data->id);
    cTree->Delete(item);
    INPUTENGINE.saveMapping(conv(InputMapFileName));
}

void MyDialog::OnButAddKey(wxCommandEvent& event)
{
    KeyAddDialog *ka = new KeyAddDialog(this);
    if(ka->ShowModal()!=0)
        // user pressed cancel
        return;
    std::string line = ka->getEventName() + " " + ka->getEventType() + " " + ka->getEventOption() + " !NEW!";
    //std::string line = ka->getEventName() + " " + ka->getEventType() + ;
    ka->Destroy();
    delete ka;
    ka = 0;

    INPUTENGINE.appendLineToConfig(line, conv(InputMapFileName));

    INPUTENGINE.reloadConfig(conv(InputMapFileName));
    loadInputControls();

    // find the new event now
    std::map<int, std::vector<event_trigger_t> > & events = INPUTENGINE.getEvents();
    std::map<int, std::vector<event_trigger_t> >::iterator it;
    std::vector<event_trigger_t>::iterator it2;
    int counter = 0;
    int suid = -1;
    for(it = events.begin(); it!= events.end(); it++)
    {
        for(it2 = it->second.begin(); it2!= it->second.end(); it2++, counter++)
        {
            if(it2->configline == "!NEW!")
            {
                it2->configline[0]='\0';
                suid = it2->suid;
            }
        }
    }
    if(suid != -1)
    {
        cTree->EnsureVisible(treeItems[suid]);
        cTree->SelectItem(treeItems[suid]);
        remapControl();
        INPUTENGINE.saveMapping(conv(InputMapFileName));
    }
}

bool MyDialog::isSelectedControlGroup(wxTreeItemId *item)
{
    wxTreeItemId itm = cTree->GetSelection();
    if(!item)
        item = &itm;
    IDData *data = (IDData *)cTree->GetItemData(*item);
    if(!data || data->id < 0)
        return true;
    return false;
}

void MyDialog::remapControl()
{
    event_trigger_t *t = getSelectedControlEvent();
    if(!t)
        return;
    kd = new KeySelectDialog(this, t);
    if(kd->ShowModal()==0)
    {
        wxTreeItemId item = cTree->GetSelection();
        INPUTENGINE.updateConfigline(t);
        updateItemText(item, t);
    }

    kd->Destroy();
    delete kd;
    kd = 0;
}

event_trigger_t *MyDialog::getSelectedControlEvent()
{
    int itemId = -1;
    wxTreeItemId item = cTree->GetSelection();
    IDData *data = (IDData *)cTree->GetItemData(item);
    if(!data)
        return 0;
    itemId = data->id;
    if(itemId == -1)
        return 0;
    return INPUTENGINE.getEventBySUID(itemId);
}

