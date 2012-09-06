#include "OISInputManager.h"
#include "OISException.h"
#include "OISKeyboard.h"
#include "OISMouse.h"
#include "OISJoyStick.h"
#include "OISEvents.h"

#include <Windows.h>
#include <iostream>
#include <vector>
#include <sstream>
#include <fstream>

using namespace OIS;
using namespace std;

const char *g_DeviceType[6] = {"OISUnknown", "OISKeyboard", "OISMouse", "OISJoyStick","OISTablet", "OISOther"};

InputManager *g_InputManager = 0;
Keyboard *g_kb  = 0;
Mouse	 *g_m   = 0;
JoyStick* g_joys[50];

std::string getVendorMapFilename(std::string vendor)
{
	std::string repl = "\\/ #@?!$%^&*()+=-><.:'|\";";
	std::string vendorstr = std::string(vendor);
	for(unsigned int c1 = 0; c1 < repl.size(); c1++)
		for(unsigned int c2 = 0; c2 < vendorstr.size(); c2++)
			if(vendorstr[c2] == repl[c1]) vendorstr[c2] = '_';
	vendorstr += ".map";
	return vendorstr;
}

int main()
{
	ofstream myfile;
	myfile.open ("inputinfo.txt");
	try
	{
		ParamList pl;
		std::ostringstream wnd;
		wnd << (size_t)GetConsoleWindow();
		pl.insert(std::make_pair( std::string("WINDOW"), wnd.str() ));
		g_InputManager = InputManager::createInputSystem(pl);
		g_InputManager->enableAddOnFactory(InputManager::AddOn_All);
		unsigned int v = g_InputManager->getVersionNumber();
		myfile << "System info:\n\tOIS Version: " << (v>>16 ) << "." << ((v>>8) & 0x000000FF) << "." << (v & 0x000000FF)
			<< "\n\tOIS Release Name: " << g_InputManager->getVersionName()
			<< "\n\tInput Manager: " << g_InputManager->inputSystemName()
			<< "\n\tTotal Keyboards: " << g_InputManager->getNumberOfDevices(OISKeyboard)
			<< "\n\tTotal Mice: " << g_InputManager->getNumberOfDevices(OISMouse)
			<< "\n\tTotal JoySticks: " << g_InputManager->getNumberOfDevices(OISJoyStick);
		myfile << "\n\nDevices:" << endl;
		DeviceList list = g_InputManager->listFreeDevices();
		for( DeviceList::iterator i = list.begin(); i != list.end(); ++i )
			myfile << "\t- " << g_DeviceType[i->first] << ", Vendor: " << i->second << endl;
		int numSticks = g_InputManager->getNumberOfDevices(OISJoyStick);
		for( int i = 0; i < numSticks; ++i )
		{
			g_joys[i] = (JoyStick*)g_InputManager->createInputObject( OISJoyStick, true );
			myfile << "\n\nJoystick " << (i) << ":"
				<< "\n\tVendor: " << g_joys[i]->vendor()
				<< "\n\tVendorMapFilename: " << getVendorMapFilename(g_joys[i]->vendor())
				<< "\n\tID: " << g_joys[i]->getID()
				<< "\n\tType: [" << g_joys[i]->type() << "] " << g_DeviceType[g_joys[i]->type()]
				<< "\n\tAxes: " << g_joys[i]->getNumberOfComponents(OIS_Axis)
				<< "\n\tSliders: " << g_joys[i]->getNumberOfComponents(OIS_Slider)
				<< "\n\tPOV/HATs: " << g_joys[i]->getNumberOfComponents(OIS_POV)
				<< "\n\tButtons: " << g_joys[i]->getNumberOfComponents(OIS_Button)
				<< "\n\tVector3: " << g_joys[i]->getNumberOfComponents(OIS_Vector3)
				<< "\n\tVector3Sensitivity: " << g_joys[i]->getVector3Sensitivity();
		}
	}
	catch(std::exception &ex)
	{
		myfile << "Caught std::exception: what = " << ex.what() << std::endl;
	}
	if( g_InputManager ) InputManager::destroyInputSystem(g_InputManager);
	myfile.close();
	return 0;
}
