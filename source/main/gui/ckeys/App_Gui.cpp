#include "OgreImGui.h"
#include "App.h"
#include "Util.h"
using namespace std;  using namespace ImGui;  using namespace ckeys;


//  Gui draw and process
///-----------------------------------------------------------------------------
void App::Gui()
{
	//  const
	ImGuiSetCond_ always = ImGuiSetCond_Always;
	const int flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize;
	bool open = true;

	const ImVec2  posBtm = ImVec2(set.xRWndSize, set.ywSize - set.yGuiSize);
	const ImVec2 sizeBtm = ImVec2(set.xwSize - set.xRWndSize, set.yGuiSize);
	const static int w1 = set.xGui1, w2 = set.xGui2, w3 = set.xGui3;

	bool e;  // edited, changed
	int x = 0, w;


	///  main window
	//---------------------------------------------
	if (!options && !graphics)
	{
		SetNextWindowPos(posBtm, always);
		SetNextWindowSize(ImVec2(set.xGuiSize-set.xRWndSize, set.yGuiSize), always);
		Begin("Window", &open, flags);

		//  1st line
		//---------------------------------------------
		Sep(1);
		w = w2;  Text("Layers");  x += w;  SameLine(x);
		w = 65;  //58

		for (int l=0; l <= keys.Lnum; ++l)
		#if 1
		if (l != 1)  // skip L1 (is for Display)
		#endif
		{
			const rgb& c = set.clrLay[l][2];
			const float f = 1.f/255.f;
			ImVec4 cl(c.r*f, c.g*f, c.b*f, 1.f);
			PushStyleColor(ImGuiCol_Text, cl);

			string s = "L" + i2s(l);
			Checkbox(s.c_str(), &set.bL[l]);  x += w;

			if (l < keys.Lnum && l != 5)  SameLine(x);  else x = 20;
			PopStyleColor();
		}

		//  2nd line
		//---------------------------------------------
		x = 0;  Sep(3);
		w = w2;  Text("Layout");  x += w;  SameLine(x);

		//  combo
		w = w3;  PushItemWidth(w);
		e = Combo("Cmb", &set.iCombo, &keys.vCmb[0]);
		if (e)  keys.LoadIndex();
		PopItemWidth();  x += w + 40;  x = 315;  SameLine(x);

		//  buttons
		w = 50;  e = Button("Fit");  x += w;  SameLine(x);
		if (e && set.bLayout && xMax > 0)
		{
			//  fit window to layout  ----
			set.xwSize = min(2560, max(640, xMax + 30));  //par
			set.ywSize = yMax + set.yGuiSize + 4;
			sf::Vector2u si(set.xwSize, set.ywSize);

			pWindow->setSize(si);
			Resize(set.xwSize, set.ywSize);
		}
		w = 100;  e = Button("Reset");  if (e)  set.fScale = 1.f;  x += w;


		//  3rd line
		//---------------------------------------------
		x = 0;  Sep(1);  //  scale slider
		w = w2;  Text(("Scale  "+f2s(set.fScale,2)).c_str());  x += w;  SameLine(x);
		w = set.xwSize > 720 ? 320 : 150;  //par

		PushItemWidth(w);  PushAllowKeyboardFocus(false);
		e = SliderFloat("", &set.fScale, 0.2f, 2.f, "");  PopAllowKeyboardFocus();
		if (e)  set.fScale = min(3.f, max(0.1f, set.fScale));
		PopItemWidth();  x += w + 20;

		End();
	}


	//  left window, menu tabs
	//---------------------------------------------
	int h = set.ywSize - set.yGuiSize;
	SetNextWindowPos( ImVec2(0, h), always);
	SetNextWindowSize(ImVec2(set.xRWndSize, h), always);
	Begin("WndDbg", &open, flags);

	Sep(1);
	e = Button("Help");  if (e)  help = !help;
	Sep(1);
	e = Button("Options");  if (e) {  options = !options;  graphics = 0;  }
	Sep(1);
	e = Button("Graphics");  if (e) {  graphics = !graphics;  options = 0;  }

	End();


	//  Graphics window
	//---------------------------------------------
	const int xt = 120,
		xts = 90, xs = 100, ws = xs+30;

	if (graphics)
	{
		SetNextWindowPos(posBtm, always);
		SetNextWindowSize(sizeBtm, always);
		Begin("GraphicsWnd", &open, flags);

		Sep(1);
		x = 0;  w = xt;  Text("Font: ");  x += w;  SameLine(x);
		PushItemWidth(xs);
		e = SliderInt("Font", &set.iFontH, 4, 30);  x += ws;  SameLine(x);
		Text("Gui");  x += xts/2;  SameLine(x);
		e = SliderInt("Gui", &set.iFontGui, 4, 30);  x += ws;  SameLine(x);
		PopItemWidth();
		e = Checkbox("Bold", &set.bBold);  x += w;

		Sep(3);
		x = 0;  w = xt;  Text("Graphics: ");  x += w;  SameLine(x);
		w = xt;  e = Checkbox("Vsync", &set.vsync);  x += w;  SameLine(x);
		PushItemWidth(xs);
		Text("Limit Fps");  x += xts;  SameLine(x);
		e = SliderInt("lim", &set.limitFps, 0, 200);  x += ws;  SameLine(x);
		Text("Sleep");  x += xts;  SameLine(x);
		e = SliderInt("sl", &set.iSleep, 0, 30);  x += ws;  SameLine(x);
		Text("Aliasing");  x += xts;  SameLine(x);
		e = SliderInt("al", &set.iAliasing, 0, 16);  x += ws;
		PopItemWidth();

		End();
	}

	//  Options window
	//---------------------------------------------
	if (options)
	{
		SetNextWindowPos(posBtm, always);
		SetNextWindowSize(sizeBtm, always);
		Begin("OptionsWnd", &open, flags);

		Sep(1);  x = 0;
		w = xt;  Text("Show: ");  x += w;  SameLine(x);
		w = w1;  e = Checkbox("Pressed List", &set.bList);  x += w;  SameLine(x);
		w = w1;  e = Checkbox("Simple List", &set.bListSimple);  x += w;  SameLine(x);
		w = w2;  e = Checkbox("Layout", &set.bLayout);  x += w;

		Sep(3);  x = 0;
		w = xt;  Text("Test: ");  x += w;  SameLine(x);
		w = w2;  e = Checkbox("VK", &set.bVK);  x += w;  SameLine(x);
		w = w2;  e = Checkbox("KLL", &set.bKLL);  x += w;  SameLine(x);
		w = w2;  e = Checkbox("Scan", &set.bScan);  x += w;  SameLine(x);

		w = w2;  e = Checkbox("Fps", &set.bFps);  x += w;  SameLine(x);
		w = w2;  e = Checkbox("Info", &set.bInfo);  x += w;

		Sep(1);  x = 0;
		w = xt;  Text("Program: ");  x += w;  SameLine(x);
		w = w1;  e = Checkbox("Esc Quits", &set.escQuit);  x += w;  SameLine(x);
		w = w1;  e = Checkbox("Log Output", &set.logOut);  x += w;

		End();
	}

	//  Help window
	//---------------------------------------------
	if (help)
	{
		SetNextWindowPos(ImVec2(0,0), always);
		SetNextWindowSize(ImVec2(set.xwSize, set.ywSize - set.yGuiSize), always);
		Begin("HelpWnd", &open, flags);

		Sep(5);
		Text("Crystal Keys");
		Sep(5);  x = 0;
		string s = "Version: " + f2s(set.ver/100.f);
		Text(s.c_str());

		const char* a={__DATE__}, *m={__TIME__};
		const char dt[] = {
			//  build date, time  format yyyy-mmm-dd hh:mm
			a[7],a[8],a[9],a[10],' ',a[0],a[1],a[2],' ',a[4],a[5],' ',' ',m[0],m[1],':',m[3],m[4],0};
		s = "Date: ";  s += dt;
		Text(s.c_str());

		End();
	}
}
