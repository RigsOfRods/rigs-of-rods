
#include "Settings.h"
#include "Util.h"
using namespace std;  using namespace ckeys;


//  ctor
Settings::Settings()
{
	Default();
}

void Settings::GetWndDim(sf::Window* wnd)
{
	xwPos = wnd->getPosition().x;
	ywPos = wnd->getPosition().y;
	xwSize = wnd->getSize().x;
	ywSize = wnd->getSize().y;
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

	strcpy(pathSet, "ckeys.xml");
}

//  find data path
bool Settings::FindData()
{
    return true;
}


///  Load
//------------------------------------------------------------------------------------------------
bool Settings::Load()
{
	Default();

	XMLDocument doc;
	XMLError er = doc.LoadFile(pathSet);
	if (er != XML_SUCCESS)
	{	/*Can't load: "+file);*/  return false;  }

	XMLElement* root = doc.RootElement();
	if (!root)  return false;
	string rn = root->Name();
	if (rn != "ckeys")  return false;

	Default();

	XMLElement* e;  const char* a;

	e = root->FirstChildElement("dim");
	if (e)
	{	a = e->Attribute("iFontH");    if (a)  iFontH = atoi(a);
		a = e->Attribute("iFontGui");  if (a)  iFontGui = atoi(a);
		a = e->Attribute("scale");   if (a)  fScale = atof(a);
		a = e->Attribute("combo");   if (a)  iCombo = atoi(a);
		a = e->Attribute("bold");   if (a)  bBold = atoi(a);
	}
	e = root->FirstChildElement("show");
	if (e)
	{	a = e->Attribute("list");    if (a)  bList = atoi(a) > 0;
		a = e->Attribute("simple");  if (a)  bListSimple = atoi(a) > 0;
		a = e->Attribute("layout");  if (a)  bLayout = atoi(a) > 0;

		a = e->Attribute("fps");     if (a)  bFps = atoi(a) > 0;
		a = e->Attribute("info");    if (a)  bInfo = atoi(a) > 0;

		a = e->Attribute("vk");      if (a)  bVK  = atoi(a) > 0;
		a = e->Attribute("kll");     if (a)  bKLL = atoi(a) > 0;
		a = e->Attribute("scan");    if (a)  bScan = atoi(a) > 0;
	}
	e = root->FirstChildElement("layer");
	if (e)
	for (int l=0; l < Lmax; ++l)
	{
		string s = "L" + i2s(l);
		a = e->Attribute(s.c_str());  if (a)  bL[l] = atoi(a) > 0;
	}
	e = root->FirstChildElement("window");
	if (e)
	{	a = e->Attribute("x");  if (a)  xwPos = atoi(a);
		a = e->Attribute("y");  if (a)  ywPos = atoi(a);
		a = e->Attribute("sx");  if (a)  xwSize = atoi(a);
		a = e->Attribute("sy");  if (a)  ywSize = atoi(a);
		a = e->Attribute("escQuit");  if (a)  escQuit = atoi(a) > 0;

		a = e->Attribute("vsync");  if (a)  vsync = atoi(a) > 0;
		a = e->Attribute("limitFps");  if (a)  limitFps = atoi(a);
		a = e->Attribute("aliasing");  if (a)  iAliasing = atoi(a);
		a = e->Attribute("sleep");  if (a)  iSleep = atoi(a);
	}
	e = root->FirstChildElement("program");
	if (e)
	{	a = e->Attribute("escQuit");  if (a)  escQuit = atoi(a) > 0;
		a = e->Attribute("logOut");  if (a)  logOut = atoi(a) > 0;
	}
	return true;
}

///  Save
//------------------------------------------------------------------------------------------------
bool Settings::Save()
{
	XMLDocument xml;
	XMLElement* root = xml.NewElement("ckeys");
	root->SetAttribute("ver", ver);
	XMLElement* e;

	e = xml.NewElement("dim");
		e->SetAttribute("iFontH", iFontH);
		e->SetAttribute("iFontGui", iFontGui);
		e->SetAttribute("scale", f2s(fScale,3).c_str());
		e->SetAttribute("combo", iCombo);
		e->SetAttribute("bold", bBold);
	root->InsertEndChild(e);

	e = xml.NewElement("show");
		e->SetAttribute("list", bList ? 1 : 0);
		e->SetAttribute("simple", bListSimple ? 1 : 0);
		e->SetAttribute("layout", bLayout ? 1 : 0);

		e->SetAttribute("fps", bFps ? 1 : 0);
		e->SetAttribute("info", bInfo ? 1 : 0);

		e->SetAttribute("vk",  bVK  ? 1 : 0);
		e->SetAttribute("kll", bKLL ? 1 : 0);
		e->SetAttribute("scan", bScan ? 1 : 0);
	root->InsertEndChild(e);

	e = xml.NewElement("layer");
	for (int l=0; l < Lmax; ++l)
	{
		string s = "L" + i2s(l);
		e->SetAttribute(s.c_str(), bL[l] ? 1 : 0);
	}
	root->InsertEndChild(e);

	e = xml.NewElement("window");
		e->SetAttribute("x", xwPos);
		e->SetAttribute("y", ywPos);
		e->SetAttribute("sx", xwSize);
		e->SetAttribute("sy", ywSize);

		e->SetAttribute("vsync", vsync);
		e->SetAttribute("limitFps", limitFps);
		e->SetAttribute("aliasing", iAliasing);
		e->SetAttribute("sleep", iSleep);
	root->InsertEndChild(e);

	e = xml.NewElement("program");
		e->SetAttribute("escQuit", escQuit ? 1 : 0);
		e->SetAttribute("logOut", logOut ? 1 : 0);
	root->InsertEndChild(e);

	xml.InsertEndChild(root);
	return xml.SaveFile(pathSet) == XML_SUCCESS;
}
