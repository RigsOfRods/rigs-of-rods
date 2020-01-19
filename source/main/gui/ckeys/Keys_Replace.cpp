#include "Keys.h"
#include "Util.h"
#include <string>
using namespace std;  using namespace ckeys;


//  shorten kll names
std::string Keys::ReplaceKll(const string& name)
{
	string s = name;
	replK(s, "Protocol", "");  replK(s, "Lock", "");  replK(s, "()", "");
	replK(s, "MUTE", "Mute");  replK(s, "CALCULATOR", "Calc");
	replK(s, "VOLUMEUP", "Vol+");  replK(s, "VOLUMEDOWN", "Vol-");
	replK(s, "STOP", "[]");  replK(s, "PAUSEPLAY", "||");  replK(s, "PLAY", "|>");
	replK(s, "SCANPREVIOUSTRACK", "|<");  replK(s, "SCANNEXTTRACK", ">|");

	//  seq
	bool b = replK(s, "s(", "s");  if (b)  s = s.substr(0,s.length()-1);
	if (s.length() == 3 && s[0]=='\'' && s[2]=='\'')  s = s[1];  // '('

	//  mouse
	bool m = replK(s, "mouseOut(", "M");
	std::string ws(s);
	if (m)
	{
		char x=0, y=0, b=0;
		sscanf(s.c_str(), "M%d,%d,%d", &b,&x,&y);
		if (x < 0)  ws = u8"M←";  else  if (x > 0)  ws = u8"M→";  else
		if (y < 0)  ws = u8"M↑";  else  if (y > 0)  ws = u8"M↓";  else
		if (b > 0)  ws = "M" + i2s(b);
	}
	ReplacePlayer(s,ws);  //**
	return ws;
}

//  replace json name
std::string Keys::ReplaceJson(string& s, string& sVK, string& sk, bool& ext, bool& has2)
{
	ext = false;
	has2 = replK(s, "\\n", "\n");  // key has 2 descr: upper, lower
	replK(s, "\\\\", "\\");
	sVK = s;  // copy for vk map

	replK(s, "\\\"", "\"");
	replK(s, "L_", "");  replK(s, "R_", "");  // left, right modifiers
	if (replK(s, "N_", ""))  ext = true;  // numpad

	//  shorter
	replK(s, "Lock", "");
	replK(s, "Space", " ");  replK(s, "Delete", "Del");
	replK(s, "CLEAR", "5");

	std::string ws(s);
	ReplaceArrows(s,ws);  //**

	//  vk to key  ------
	if (has2)
	{	size_t p = sVK.find("\n");

		if (!found(sVK, "N_"))  // digits, symbols
			sVK = sVK.substr(p+1);  // second part
		else  // numpad
			sVK = sVK.substr(0, p);  // first part
	}
	//  str to key
	sk = sVK;
	replK(sk, "L_", "L");  replK(sk, "R_", "R");  // left, right mod
	replK(sk, "N_", "P");  // numpad
	replK(sk, "PgUp", "PageUp");  replK(sk, "PgDn", "PageDown");
	replK(sk, "Win", "Gui");  replK(sk, "Menu", "App");
	replK(sk, "PrtSc", "PrintScreen");
	replK(sk, "`", "BackTick");  //replK(sk, " Lock", "Lock");

	return ws;
}


//  apply player symbols
void Keys::ReplacePlayer(const string& s, std::string& ws)
{
	if (s=="|>")                 ws = u8"▶";
    if (s=="||")                 ws = u8"▮▮";
    if (s=="[]")                 ws = u8"◼";
	if (s==">|" || s=="M Next")  ws = u8"▶▮";
    if (s==">>")                 ws = u8"▶▶";
	if (s=="|<" || s=="M Prev")  ws = u8"▮◀";
    if (s=="<<")                 ws = u8"◀◀";
}

//  arrow symbols
void Keys::ReplaceArrows(const string& s, std::string& ws)
{
	if (found(s, "Left"))   ws = u8"←";
    if (found(s, "Right"))  ws = u8"→";
	if (found(s, "Down"))   ws = u8"↓";
	if (!found(s, "PgUp") && found(s, "Up"))  ws = u8"↑";
	if (s=="Display")   ws = u8"❏";  //▤❏◾
}

void Keys::ReplacePressed(string& sk)
{
	if (sk.length() > 2)  // N_ to Num
	if (sk[0]=='N' && sk[1]=='_')
		sk = "Num " + sk.substr(2);

	if (sk[1]=='_')  // _ to space, for L_ R_
		sk[1] = ' ';
	if (sk == "Backspace")  // too long
		sk = "Backspc";
}


//  set clr from name
void Key::SetClr()
{
	clr = KC_Normal;
	layer = 0;

	if (sJson=="Display")
		clr = KC_Display;
	else
	if (sJson.length() == 2 && sJson[0] == 'L')
	{
		clr = KC_Layer;
		layer = sJson[1] - '0';
	}
}
