
#include "Settings.h"
#include "Util.h"

using namespace ckeys;

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

// Stubs created for Rigs of Rods (I couldn't find the original implementations) ~ Petr Ohlidal, 01/2020
//-----------------------------------------------

// static vars
const rgb
    Settings::clrKC[KC_Layer][3],  // rft [rect,frame,text]
    Settings::clrLay[Settings::Lmax][3],
    Settings::clrOver;

const rgb* Settings::getC(const KClr& kc, const int& lay, const int& rft)
{
    // STUB!
    static rgb val = {1,1,1};
    return &val;
}

