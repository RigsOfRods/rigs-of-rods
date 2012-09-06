//////////////////////////////// OS Neutral Headers ////////////////
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

InputManager *g_InputManager = 0;	//Our Input System
Keyboard *g_kb  = 0;				//Keyboard Device
Mouse	 *g_m   = 0;				//Mouse Device
JoyStick* g_joys[50];

int main()
{
	ofstream myfile;
	myfile.open ("inputinfo.txt");
	std::cout << "\n\n*** OIS Console App is starting up... *** \n";
	try
	{
		ParamList pl;
		//Create a capture window for Input Grabbing

		std::ostringstream wnd;
		wnd << (size_t)GetConsoleWindow();

		pl.insert(std::make_pair( std::string("WINDOW"), wnd.str() ));

		//This never returns null.. it will raise an exception on errors
		g_InputManager = InputManager::createInputSystem(pl);
		g_InputManager->enableAddOnFactory(InputManager::AddOn_All);

		//Lets enable all addons that were compiled in:
		g_InputManager->enableAddOnFactory(InputManager::AddOn_All);

		//Print debugging information
		unsigned int v = g_InputManager->getVersionNumber();
		myfile << "OIS Version: " << (v>>16 ) << "." << ((v>>8) & 0x000000FF) << "." << (v & 0x000000FF)
			<< "\nRelease Name: " << g_InputManager->getVersionName()
			<< "\nManager: " << g_InputManager->inputSystemName()
			<< "\nTotal Keyboards: " << g_InputManager->getNumberOfDevices(OISKeyboard)
			<< "\nTotal Mice: " << g_InputManager->getNumberOfDevices(OISMouse)
			<< "\nTotal JoySticks: " << g_InputManager->getNumberOfDevices(OISJoyStick);

		myfile << endl;
		//List all devices
		DeviceList list = g_InputManager->listFreeDevices();
		for( DeviceList::iterator i = list.begin(); i != list.end(); ++i )
			myfile << "\n\tDevice: " << g_DeviceType[i->first] << " Vendor: " << i->second;

		g_kb = (Keyboard*)g_InputManager->createInputObject( OISKeyboard, true );

		g_m = (Mouse*)g_InputManager->createInputObject( OISMouse, true );
		const MouseState &ms = g_m->getMouseState();
		ms.width = 100;
		ms.height = 100;

		//This demo uses at most 4 joysticks - use old way to create (i.e. disregard vendor)
		int numSticks = g_InputManager->getNumberOfDevices(OISJoyStick);
		for( int i = 0; i < numSticks; ++i )
		{
			g_joys[i] = (JoyStick*)g_InputManager->createInputObject( OISJoyStick, true );

			std::string repl = "\\/ #@?!$%^&*()+=-><.:'|\";";
			std::string vendorstr = std::string(g_joys[i]->vendor());
			for(unsigned int c1 = 0; c1 < repl.size(); c1++)
				for(unsigned int c2 = 0; c2 < vendorstr.size(); c2++)
					if(vendorstr[c2] == repl[c1]) vendorstr[c2] = '_';
			vendorstr += ".map";

			myfile << "\n\nJoystick " << (i + 1)
				<< "\n\tVendor: " << g_joys[i]->vendor()
				<< "\n\tVendorMapFilename: " << vendorstr
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

	//Destroying the manager will cleanup unfreed devices
	if( g_InputManager )
		InputManager::destroyInputSystem(g_InputManager);
	
	myfile.close();
	return 0;
}
