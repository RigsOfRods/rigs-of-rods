
#include "Settings.h"
#include "Util.h"
using namespace std;  using namespace ckeys;


//  ctor
Settings::Settings()
{
	Default();
}


//  Defaults, init paths
//-----------------------------------------------
void Settings::Default()
{
	iFontH = 18;
	iFontGui = 17;
	bBold = true;

	iCombo = 0;

	bList = true;
	bListSimple = true;

	bLayout = true;
	fScale = 1.f;

	bFps = false;
	bInfo = true;
	escQuit = false;
	logOut = false;

	vsync = true;
	limitFps = 0;
	iAliasing = 8;
	iSleep = 5;

	bVK = false;
	bKLL = false;

	for (int i = 0; i < Lmax; ++i)
		bL[i] = false;
}

