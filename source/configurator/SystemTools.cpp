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

/// @file   SystemTools.cpp
/// @author Petr Ohlidal
/// @date   02/2017
/// @brief  System/library integration layer for RoRConfig

#include "RoRConfig.h"

#include "OISKeyboard.h"
#include "OISJoyStick.h"
#include "OISInputManager.h"
#include <wx/wfstream.h>
#include <wx/zipstrm.h>
#include <wx/log.h>

#include <memory>
#include <string>
#include <sstream>

// Internal helper declarations
std::string ComposeVendorMapFilename(std::string vendor);

// ========== The tools ===========

wxString LoadInputDevicesInfo(WXWidget wx_window_handle)
{
    // Code cloned from original utility "inputtool.exe" located in ~REPO/tools/windows
    // Useful reference:
    //     http://docs.wxwidgets.org/3.1/classwx_text_ctrl.html
    //     https://wiki.wxwidgets.org/Writing_Your_First_Application-Using_The_WxTextCtrl

    using namespace OIS;
    const char *DeviceTypes[6] = {"OISUnknown", "OISKeyboard", "OISMouse", "OISJoyStick", "OISTablet", "OISOther"};
    InputManager* input_man = nullptr;
    wxString      output;

    try
    {
        ParamList pl;
        std::ostringstream wnd;
        wnd << (size_t)wx_window_handle;
        pl.insert(std::make_pair( std::string("WINDOW"), wnd.str() ));
        input_man = InputManager::createInputSystem(pl);
        input_man->enableAddOnFactory(InputManager::AddOn_All);
        unsigned int v = input_man->getVersionNumber();
        std::stringstream out_stream;
        out_stream << "System info:\n\tOIS Version: " << (v>>16 ) << "." << ((v>>8) & 0x000000FF) << "." << (v & 0x000000FF)
            << "\n\tOIS Release Name: " << input_man->getVersionName()
            << "\n\tInput Manager: "    << input_man->inputSystemName()
            << "\n\tTotal Keyboards: "  << input_man->getNumberOfDevices(OISKeyboard)
            << "\n\tTotal Mice: "       << input_man->getNumberOfDevices(OISMouse)
            << "\n\tTotal JoySticks: "  << input_man->getNumberOfDevices(OISJoyStick);
        out_stream << "\n\nDevices:" << std::endl;
        DeviceList list = input_man->listFreeDevices();

        for( DeviceList::iterator i = list.begin(); i != list.end(); ++i )
        {
            out_stream << "\t- " << DeviceTypes[i->first] << ", Vendor: " << i->second << std::endl;
        }

        JoyStick* g_joys[50];
        int numSticks = input_man->getNumberOfDevices(OISJoyStick);
        for( int i = 0; i < numSticks; ++i )
        {
            g_joys[i] = (JoyStick*)input_man->createInputObject( OISJoyStick, true );
            out_stream << "\n\nJoystick " << (i) << ":"
                << "\n\tVendor: "             << g_joys[i]->vendor()
                << "\n\tVendorMapFilename: "  << ComposeVendorMapFilename(g_joys[i]->vendor())
                << "\n\tID: "                 << g_joys[i]->getID()
                << "\n\tType: ["              << g_joys[i]->type() << "] " << DeviceTypes[g_joys[i]->type()]
                << "\n\tAxes: "               << g_joys[i]->getNumberOfComponents(OIS_Axis)
                << "\n\tSliders: "            << g_joys[i]->getNumberOfComponents(OIS_Slider)
                << "\n\tPOV/HATs: "           << g_joys[i]->getNumberOfComponents(OIS_POV)
                << "\n\tButtons: "            << g_joys[i]->getNumberOfComponents(OIS_Button)
                << "\n\tVector3: "            << g_joys[i]->getNumberOfComponents(OIS_Vector3)
                << "\n\tVector3Sensitivity: " << g_joys[i]->getVector3Sensitivity();
        }

        output = wxString::FromUTF8(out_stream.str().c_str());
    }
    catch (std::exception &ex)
    {
        output = wxT("\n\tAn error occurred!\n\nMessage: ") + wxString::FromUTF8(ex.what());
    }
    catch (...)
    {
        output = wxT("\n\tAn error occurred!");
    }

    if(input_man != nullptr)
        InputManager::destroyInputSystem(input_man);

    return output;
}

bool ExtractZipFiles(const wxString& aZipFile, const wxString& aTargetDir)
{
    // from http://wiki.wxwidgets.org/WxZipInputStream
    bool ret = true;
    //wxFileSystem fs;
    std::unique_ptr<wxZipEntry> entry(new wxZipEntry());
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

// ========== Internal helpers ==========

std::string ComposeVendorMapFilename(std::string vendor)
{
    std::string repl = "\\/ #@?!$%^&*()+=-><.:'|\";";
    std::string vendorstr = std::string(vendor);
    for(unsigned int c1 = 0; c1 < repl.size(); c1++)
        for(unsigned int c2 = 0; c2 < vendorstr.size(); c2++)
            if(vendorstr[c2] == repl[c1]) vendorstr[c2] = '_';
    vendorstr += ".map";
    return vendorstr;
}
